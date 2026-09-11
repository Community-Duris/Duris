"""Verified, immutable server artifacts shared by isolated regression journeys."""

import fcntl
import hashlib
import json
import os
from pathlib import Path
import re
import shlex
import shutil
import subprocess
import tempfile
import time

ROOT = Path(__file__).resolve().parents[2]
CACHE_ENV = "DURIS_REGRESSION_BUILD_CACHE"
# Only observation/runtime controls are excluded; build variables remain inputs.
IGNORED_ENV = {
    CACHE_ENV, "PWD", "OLDPWD", "SHLVL", "_",
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
    add({"contract": 1, "backend": "flatfile", "jobs": 2, "timeout": 600})
    add(str(ROOT))  # Absolute source paths are embedded in compiler debug info.
    # Do not depend on Git: exported source trees and dirty worktrees are valid.
    paths = list((ROOT / "src").rglob("*"))
    paths += list((ROOT / "tests/async").glob("*.h"))
    add(file_hash(__file__))
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
    directories = {"/usr/include", "/usr/lib/gcc", "/usr/local/include", "/usr/local/lib"}
    for variable in ("CPATH", "CPLUS_INCLUDE_PATH", "C_INCLUDE_PATH", "LIBRARY_PATH"):
        if variable in environment:
            directories.update(str(source_path(p or "."))
                               for p in environment[variable].split(os.pathsep))
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
                directories.add(str(path))
            elif path.is_file():
                add((str(path), file_hash(path)))
    for directory in sorted(directories):
        for path in sorted(Path(directory).rglob("*")):
            if path.is_file() and "__pycache__" not in path.parts:
                add((str(path), file_hash(path)))
    libraries = re.search(r"^libraries: =(.+)$", search, re.MULTILINE)
    if not libraries:
        raise RuntimeError("compiler did not report library search directories")
    for directory in sorted({source_path(p) for p in libraries[1].split(os.pathsep)}):
        for path in sorted(directory.glob("*")):
            if path.is_file() and (".so" in path.name or path.suffix in (".a", ".o")):
                add((str(path), file_hash(path)))
    for name in ("cc1plus", "collect2", "lto1"):
        path = Path(subprocess.check_output(compiler + [f"-print-prog-name={name}"],
                                           cwd=ROOT / "src", env=environment, text=True).strip())
        if path.is_file():
            add((str(path), file_hash(path)))
    return digest.hexdigest()


def compile_server(build_root, environment):
    binary = build_root / "server/dms_new"
    started = time.monotonic()
    build = subprocess.run(
        ["make", "-C", "src", "PERSISTENCE_BACKEND=flatfile",
         f"BIN_ROOT={build_root}", f"OBJDIR={build_root / 'objects/server'}",
         f"SERVER_BIN_DIR={binary.parent}", f"DMS_BINARY={binary}", "-j2"],
        cwd=ROOT, env=environment, text=True, stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT, timeout=600,
    )
    if build.returncode:
        raise AssertionError("flat-file server build failed:\n" + build.stdout[-8000:])
    validate_log(build.stdout)
    return binary, build.stdout, time.monotonic() - started


def validate_log(output):
    if "-D__NO_MYSQL__" not in output:
        raise AssertionError("flat build did not select __NO_MYSQL__")
    if "-I/usr/include/mysql" in output:
        raise AssertionError("flat build used system MySQL headers")
    if "-lmysqlclient" in output:
        raise AssertionError("flat build linked the MySQL client")


def verified_artifact(cache, key):
    try:
        metadata = json.loads((cache / f"{key}.json").read_text())
        directory = cache / metadata["directory"]
        if (directory.parent != cache or directory.is_symlink() or
                not directory.name.startswith("artifact-")):
            return None
        binary, log = directory / "server/dms_new", directory / "build.log"
        if (metadata["inputs"] != key or binary.is_symlink() or
                binary.stat().st_mode & 0o222 or not os.access(binary, os.X_OK) or
                file_hash(binary) != metadata["binary"] or file_hash(log) != metadata["log"]):
            return None
        validate_log(log.read_text())
        return binary
    except (OSError, ValueError, KeyError, TypeError, AssertionError):
        return None


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
    cache = Path(cache_value).resolve()
    if not cache.is_relative_to((ROOT / "bin").resolve()):
        raise ValueError("server artifact cache must be below bin/")
    cache.mkdir(parents=True, exist_ok=True)
    key = input_key(environment)
    # Serialize publication across independent runners. Never replace an inode
    # that another journey may still be executing, even after corruption.
    with (cache / f"{key}.lock").open("a") as lock:
        fcntl.flock(lock, fcntl.LOCK_EX)
        binary = verified_artifact(cache, key)
        if binary:
            print(f"SERVER_BUILD reused build=0.000s lookup={time.monotonic() - started:.3f}s", flush=True)
            return binary
        directory = Path(tempfile.mkdtemp(prefix="artifact-", dir=cache))
        try:
            binary, output, elapsed = compile_server(directory, environment)
            if input_key(environment) != key:
                raise RuntimeError("server build inputs changed during compilation; retry")
            log = directory / "build.log"
            log.write_text(output)
            binary.chmod(0o555)
            log.chmod(0o444)
            metadata = dict(inputs=key, directory=directory.name,
                            binary=file_hash(binary), log=file_hash(log))
            manifest = directory / "manifest.json"
            manifest.write_text(json.dumps(metadata, sort_keys=True))
            shutil.rmtree(directory / "objects")
            os.replace(manifest, cache / f"{key}.json")
        except BaseException:
            shutil.rmtree(directory)
            raise
        print(f"SERVER_BUILD built build={elapsed:.3f}s lookup={time.monotonic() - started - elapsed:.3f}s", flush=True)
        return binary
