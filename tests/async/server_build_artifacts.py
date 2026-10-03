"""Verified, immutable server artifacts shared by isolated regression journeys."""

import fcntl
from contextlib import contextmanager
import hashlib
import json
import os
from pathlib import Path
import re
import shlex
import shutil
import signal
import stat
import subprocess
import sys
import tempfile
import threading
import time

ROOT = Path(__file__).resolve().parents[2]
CACHE_ENV = "DURIS_REGRESSION_BUILD_CACHE"
COMPILER_WRAPPER = Path(__file__).with_name("server_build_compiler.py")
BUILD_TIMEOUT = 600
STOP_GRACE = 1
SYSTEM_HEADER_DIRECTORIES = ("/usr/include", "/usr/lib/gcc", "/usr/local/include")
SYSTEM_LIBRARY_DIRECTORIES = ("/usr/local/lib",)
# Only observation/runtime controls are excluded; build variables remain inputs.
IGNORED_ENV = {
    CACHE_ENV, "PWD", "OLDPWD", "SHLVL", "_", "HOSTNAME",
    "DURIS_FULL_WORLD_BINARY_CACHE", "DURIS_FULL_WORLD_ARTIFACT_DIR",
    "DURIS_FULL_WORLD_REPEATS", "DURIS_FULL_WORLD_DELAY_CAMP",
    "DURIS_FULL_WORLD_CRASH_PHASE", "DURIS_NEVENT_ANALYTICS",
    "DURIS_NEVENT_TRACE_PLAYER",
}


def file_hash(path):
    digest = hashlib.sha256()
    with Path(path).open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def input_key(environment):
    """Hash source contents (including untracked headers), tools and environment."""
    digest = hashlib.sha256()

    def add(value):
        digest.update(json.dumps(value, sort_keys=True).encode() + b"\0")

    add({key: value for key, value in environment.items() if key not in IGNORED_ENV})
    add({"contract": 2, "backend": "flatfile", "jobs": 2, "timeout": BUILD_TIMEOUT})
    add(str(ROOT))  # Absolute source paths are embedded in compiler debug info.
    # Do not depend on Git: exported source trees and dirty worktrees are valid.
    paths = list((ROOT / "src").rglob("*"))
    paths += list((ROOT / "tests/async").glob("*.h"))
    add(file_hash(__file__))
    add(file_hash(COMPILER_WRAPPER))
    add((sys.executable, file_hash(sys.executable), sys.version))
    for path in sorted(paths):
        if path.is_file():
            add((str(path.relative_to(ROOT)), file_hash(path)))
    add(toolchain_key(environment))
    return digest.hexdigest()


def compiler_configuration(environment):
    # Read the effective compiler, including command-line overrides inherited
    # through MAKEFLAGS. A no-op target avoids compiling or updating outputs.
    configuration = subprocess.check_output(
        ["make", "-s", "--no-print-directory", "-C", "src", "PERSISTENCE_BACKEND=flatfile",
         "--eval=.PHONY: artifact-config",
         "--eval=artifact-config:;@echo DURIS_ARTIFACT_CC=$(CC); echo DURIS_ARTIFACT_FLAGS=$(CFLAGS) $(INCLUDES) $(LDFLAGS) $(LIBS)",
         "artifact-config"], cwd=ROOT, env=environment, text=True).strip()
    # Inherited MAKEFLAGS can print directory banners even with the command-line
    # --no-print-directory above. Never interpret those banners as the compiler.
    values = []
    for prefix in ("DURIS_ARTIFACT_CC=", "DURIS_ARTIFACT_FLAGS="):
        matches = [line[len(prefix):] for line in configuration.splitlines() if line.startswith(prefix)]
        if len(matches) != 1:
            raise RuntimeError("make did not report an unambiguous compiler configuration")
        values.append(matches[0])
    return "\n".join(values)


def toolchain_key(environment):
    """Fingerprint installed compiler, headers and link libraries by content."""
    digest = hashlib.sha256()

    def add(value):
        digest.update(json.dumps(value, sort_keys=True).encode() + b"\0")

    def source_path(value):
        return (ROOT / "src" / value).resolve()

    configuration = compiler_configuration(environment)
    lines = configuration.splitlines()
    compiler = shlex.split(lines[0])
    for command in sorted({compiler[0], "g++", "make", "ld", "as"}):
        executable = Path(shutil.which(command, path=environment.get("PATH"))).resolve()
        add((str(executable), file_hash(executable)))
        add(subprocess.check_output([str(executable), "--version"], env=environment).decode())
    add(configuration)
    search = subprocess.check_output(compiler + ["-print-search-dirs"],
                                     cwd=ROOT / "src", env=environment, text=True)
    add(search)
    # Hash actual installed inputs, not package versions: locally edited headers
    # and libraries must invalidate the artifact too.
    directories = set(SYSTEM_HEADER_DIRECTORIES)
    library_directories = {source_path(path) for path in SYSTEM_LIBRARY_DIRECTORIES}
    for variable in ("CPATH", "CPLUS_INCLUDE_PATH", "C_INCLUDE_PATH"):
        if variable in environment:
            directories.update(str(source_path(p or "."))
                               for p in environment[variable].split(os.pathsep))
    if "LIBRARY_PATH" in environment:
        library_directories.update(source_path(p or ".")
                                   for p in environment["LIBRARY_PATH"].split(os.pathsep))
    # Include paths supplied by feature/hardening overrides are inputs too.
    # Resolve relative paths exactly as Make's recipes do, from src/.
    flags = iter(shlex.split(" ".join(lines[1:])))
    for flag in flags:
        path_value = None
        for option in ("-isystem", "-iquote", "-include", "-imacros", "-I", "-L"):
            if flag == option:
                path_value = next(flags)
                break
            if flag.startswith(option):
                path_value = flag[len(option):]
                break
        if path_value:
            path = source_path(path_value)
            if path.is_dir():
                if option == "-L":
                    library_directories.add(path)
                else:
                    directories.add(str(path))
            elif path.is_file():
                add((str(path), file_hash(path)))
    for directory in sorted(directories):
        for path in sorted(Path(directory).rglob("*")):
            if path.is_file() and "__pycache__" not in path.parts:
                # The test include directory supplies C/C++ headers. Python
                # journey edits cannot affect the server executable and must
                # not invalidate a verified build during fixture iteration.
                if path.suffix == ".py" and path.is_relative_to(ROOT / "tests/async"):
                    continue
                add((str(path), file_hash(path)))
    libraries = re.search(r"^libraries: =(.+)$", search, re.MULTILINE)
    if not libraries:
        raise RuntimeError("compiler did not report library search directories")
    library_directories.update(source_path(p) for p in libraries[1].split(os.pathsep))
    # Linker search directories are not recursive. An unrelated Python package
    # below /usr/local/lib cannot be a link input unless its own directory is
    # explicitly selected with -L/LIBRARY_PATH. Keep content hashes for every
    # directly searchable library, object and linker script.
    for directory in sorted(library_directories):
        for path in sorted(directory.glob("*")):
            if path.is_file() and (".so" in path.name or path.suffix in (".a", ".o", ".ld", ".lds")):
                add((str(path), file_hash(path)))
    for name in ("cc1plus", "collect2", "lto1"):
        path = Path(subprocess.check_output(compiler + [f"-print-prog-name={name}"],
                                           cwd=ROOT / "src", env=environment, text=True).strip())
        if path.is_file():
            add((str(path), file_hash(path)))
    return digest.hexdigest()


@contextmanager
def cancellation_handlers():
    previous = {}
    def cancelled(signum, _frame):
        if signum == signal.SIGINT:
            raise KeyboardInterrupt
        raise SystemExit(128 + signum)
    if threading.current_thread() is threading.main_thread():
        for signum in (signal.SIGTERM, signal.SIGINT, signal.SIGHUP):
            previous[signum] = signal.signal(signum, cancelled)
    try:
        yield
    finally:
        for signum, handler in previous.items():
            signal.signal(signum, handler)


def stop_build(process):
    def send(signum):
        try:
            os.killpg(process.pid, signum)
        except ProcessLookupError:
            pass
    send(signal.SIGTERM)
    try:
        process.communicate(timeout=STOP_GRACE)
    except subprocess.TimeoutExpired:
        pass
    # Kill descendants even if the leader exited and closed its own pipe.
    send(signal.SIGKILL)
    try:
        return process.communicate(timeout=STOP_GRACE)[0] or ""
    except subprocess.TimeoutExpired:
        if process.stdout:
            process.stdout.close()
        process.wait(timeout=STOP_GRACE)
        return ""


def compile_server(build_root, environment, *, lock_fd=None):
    build_root = Path(build_root).absolute()
    build_root.mkdir(parents=True, exist_ok=True)
    safe_tree(build_root)
    binary = build_root / "server/dms_new"
    # Force a fresh link and its backend flags even when every object was reused.
    # This is private output: published executable inodes are never link targets.
    binary.unlink(missing_ok=True)
    compiler = shlex.split(compiler_configuration(environment).splitlines()[0])
    launcher = shlex.join([sys.executable, "-B", str(COMPILER_WRAPPER), "--compile",
                           str(build_root / "objects/server"), "--", *compiler])
    command = ["make", "-C", "src", "PERSISTENCE_BACKEND=flatfile", f"CC={launcher}",
               f"BIN_ROOT={build_root}", f"OBJDIR={build_root / 'objects/server'}",
               f"SERVER_BIN_DIR={binary.parent}", f"DMS_BINARY={binary}", "-j2"]
    started = time.monotonic()
    read_fd, write_fd = os.pipe()
    process = None
    output = ""
    try:
        with cancellation_handlers():
            process = subprocess.Popen(
                [sys.executable, "-B", str(COMPILER_WRAPPER), "--supervise", str(read_fd),
                 "--", *command], cwd=ROOT, env=environment, text=True,
                stdout=subprocess.PIPE, stderr=subprocess.STDOUT, start_new_session=True,
                pass_fds=(read_fd,) if lock_fd is None else (read_fd, lock_fd),
            )
            os.close(read_fd)
            read_fd = None
            try:
                output = process.communicate(timeout=BUILD_TIMEOUT)[0]
            except BaseException:
                output = stop_build(process)
                raise
            finally:
                # A completed Make can leave a background recipe child behind.
                stop_build(process)
    finally:
        os.close(write_fd)
        if read_fd is not None:
            os.close(read_fd)
        if process is not None:
            with tempfile.NamedTemporaryFile(mode="w", prefix="attempt-", suffix=".log",
                                             dir=build_root, delete=False) as log:
                log.write(output)
    if process.returncode:
        raise AssertionError("flat-file server build failed:\n" + output[-8000:])
    validate_log(output)
    return binary, output, time.monotonic() - started


def validate_log(output):
    if "-D__NO_MYSQL__" not in output:
        raise AssertionError("flat build did not select __NO_MYSQL__")
    if "-I/usr/include/mysql" in output:
        raise AssertionError("flat build used system MySQL headers")
    if "-lmysqlclient" in output:
        raise AssertionError("flat build linked the MySQL client")


def verified_artifact(cache, key):
    try:
        identity = cache / f"{key}.json"
        if identity.is_symlink() or not stat.S_ISREG(identity.stat().st_mode):
            return None
        metadata = json.loads(identity.read_text())
        directory = cache / metadata["directory"]
        if (directory.parent != cache or directory.is_symlink() or
                not directory.name.startswith("artifact-")):
            return None
        binary, log = directory / "server/dms_new", directory / "build.log"
        binary_mode, log_mode = binary.stat(), log.stat()
        if (metadata["inputs"] != key or (directory / "server").is_symlink() or
                binary.is_symlink() or log.is_symlink() or
                not stat.S_ISREG(binary_mode.st_mode) or not stat.S_ISREG(log_mode.st_mode) or
                binary_mode.st_nlink != 1 or log_mode.st_nlink != 1 or
                binary_mode.st_mode & 0o222 or log_mode.st_mode & 0o222 or
                not os.access(binary, os.X_OK) or
                file_hash(binary) != metadata["binary"] or file_hash(log) != metadata["log"]):
            return None
        validate_log(log.read_text())
        return binary
    except (OSError, ValueError, KeyError, TypeError, AssertionError):
        return None


def safe_tree(directory):
    if directory.is_symlink() or directory.resolve() != directory.absolute():
        raise ValueError("build workspace has a symlink ancestor")
    for root, directories, files in os.walk(directory):
        for name in directories + files:
            path = Path(root) / name
            if path.is_symlink():
                raise ValueError("build workspace contains a symlink")
            mode = path.stat()
            if name in files and (not stat.S_ISREG(mode.st_mode) or mode.st_nlink != 1):
                raise ValueError("build workspace contains a linked or special file")


def workspace_state(directory, key, clean):
    temporary = directory / "inputs.tmp"
    temporary.write_text(json.dumps(dict(inputs=key, clean=clean), sort_keys=True))
    os.replace(temporary, directory / "inputs.json")


def pending_workspace(cache, key):
    directory = cache / f"pending-{key}"
    if directory.exists() or directory.is_symlink():
        safe_tree(directory)
        metadata = json.loads((directory / "inputs.json").read_text())
        if (not isinstance(metadata, dict) or metadata.get("inputs") != key or
                type(metadata.get("clean")) is not bool):
            raise ValueError("build workspace belongs to different inputs")
        if not metadata["clean"]:
            # A killed owner could not confirm that inputs stayed fixed. The
            # supervisor inherits our flock, so no old builder is still writing.
            shutil.rmtree(directory)
    if not directory.exists():
        initial = Path(tempfile.mkdtemp(prefix=".pending-init-", dir=cache))
        try:
            workspace_state(initial, key, True)
            os.rename(initial, directory)
        except BaseException:
            shutil.rmtree(initial)
            raise
    for path in directory.rglob(".duris-compile-*"):
        if path.is_file():
            path.unlink()
    return directory


def build_flatfile_server(build_root, *, legacy_cache=None):
    """Return a read-only executable; callers own every runtime fixture and process."""
    started = time.monotonic()
    environment = dict(os.environ)
    cache_value = environment.get(CACHE_ENV)
    if not cache_value and legacy_cache:
        cache_value = str(legacy_cache) + ".artifacts"
    if not cache_value or cache_value == "off":
        binary, _, elapsed = compile_server(Path(build_root), environment)
        print(f"SERVER_BUILD built build={elapsed:.3f}s lookup=0.000s", flush=True)
        return binary
    cache = Path(cache_value).absolute()
    if cache.resolve() != cache or not cache.is_relative_to((ROOT / "bin").resolve()):
        raise ValueError("server artifact cache must be below bin/")
    cache.mkdir(parents=True, exist_ok=True)
    key = input_key(environment)
    if not re.fullmatch(r"[0-9a-f]{64}", key):
        raise ValueError("server artifact input key is invalid")
    # Serialize publication across independent runners. Never replace an inode
    # that another journey may still be executing, even after corruption.
    fd = os.open(cache / f"{key}.lock", os.O_CREAT | os.O_RDWR | os.O_NOFOLLOW, 0o600)
    with os.fdopen(fd, "a") as lock:
        mode = os.fstat(lock.fileno())
        if not stat.S_ISREG(mode.st_mode) or mode.st_nlink != 1:
            raise ValueError("server artifact lock is not a private regular file")
        fcntl.flock(lock, fcntl.LOCK_EX)
        binary = verified_artifact(cache, key)
        if binary:
            print(f"SERVER_BUILD reused build=0.000s lookup={time.monotonic() - started:.3f}s", flush=True)
            return binary
        workspace = pending_workspace(cache, key)
        workspace_state(workspace, key, False)
        try:
            binary, output, elapsed = compile_server(workspace, environment, lock_fd=lock.fileno())
        except BaseException:
            # Retain only completed objects from a confirmed identical input
            # attempt. If this check itself is interrupted, clean stays false.
            if input_key(environment) != key:
                shutil.rmtree(workspace)
            else:
                workspace_state(workspace, key, True)
            raise
        if input_key(environment) != key:
            shutil.rmtree(workspace)
            raise RuntimeError("server build inputs changed during compilation; retry")
        workspace_state(workspace, key, True)
        safe_tree(workspace)
        directory = Path(tempfile.mkdtemp(prefix="artifact-", dir=cache))
        try:
            published = directory / "server/dms_new"
            published.parent.mkdir()
            shutil.copyfile(binary, published)
            binary = published
            log = directory / "build.log"
            log.write_text(output)
            binary.chmod(0o555)
            log.chmod(0o444)
            metadata = dict(inputs=key, directory=directory.name,
                            binary=file_hash(binary), log=file_hash(log))
            manifest = directory / "manifest.json"
            manifest.write_text(json.dumps(metadata, sort_keys=True))
            os.replace(manifest, cache / f"{key}.json")
        except BaseException:
            shutil.rmtree(directory)
            raise
        print(f"SERVER_BUILD built build={elapsed:.3f}s lookup={time.monotonic() - started - elapsed:.3f}s", flush=True)
        return binary
