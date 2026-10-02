"""Content-verified native fixtures; only immutable compilation outputs are shared."""

from concurrent.futures import ThreadPoolExecutor
import fcntl
import hashlib
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import tempfile
import time

import server_build_artifacts as server

ROOT = Path(__file__).resolve().parents[2]


def fingerprint(compiler, flags, environment):
    digest = hashlib.sha256()

    def add(value):
        digest.update(json.dumps(value, sort_keys=True).encode() + b"\0")

    add({"format": 1, "root": str(ROOT), "compiler": compiler, "flags": flags})
    add({key: value for key, value in environment.items() if key not in server.IGNORED_ENV})
    add(server.file_hash(__file__))
    add(server.file_hash(server.__file__))
    # Conservative native dependencies, including included fixture bodies and
    # untracked headers. Changes are by content, never just timestamp.
    for directory in (ROOT / "src", ROOT / "tests/async"):
        for path in sorted(directory.rglob("*")):
            if path.is_file() and path.suffix in {".c", ".cpp", ".h", ".hpp", ".inc"}:
                add((str(path.relative_to(ROOT)), server.file_hash(path)))
    configuration = shlex.join(compiler) + "\n" + shlex.join(flags)
    add(server.toolchain_key(environment, configuration=configuration, working_directory=ROOT))
    return digest.hexdigest()


def verified(cache, key, executable=False):
    try:
        row = json.loads((cache / (key + ".json")).read_text())
        path = cache / row["file"]
        if (path.parent != cache or path.is_symlink() or path.stat().st_mode & 0o222
                or row["key"] != key or server.file_hash(path) != row["sha256"]
                or (executable and not os.access(path, os.X_OK))):
            return None
        return path
    except (OSError, ValueError, KeyError, TypeError):
        return None


def publish(cache, key, path, executable=False):
    path.chmod(0o555 if executable else 0o444)
    metadata = path.with_suffix(path.suffix + ".json")
    metadata.write_text(json.dumps({"key": key, "file": path.name,
                                   "sha256": server.file_hash(path)}))
    os.replace(metadata, cache / (key + ".json"))


def command(arguments, environment):
    completed = subprocess.run(arguments, cwd=ROOT, env=environment, text=True,
                               stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=600)
    if completed.returncode:
        raise AssertionError("native fixture build failed:\n" + completed.stdout[-12000:])


def build_native(destination, sources, flags, link_flags, *, compiler="g++", name="fixture"):
    """Compile matching objects once; preserve macros, sanitizers and link wrappers."""
    started = time.monotonic()
    destination = Path(destination).resolve()
    if not destination.is_relative_to((ROOT / "bin").resolve()):
        raise ValueError("native fixture binaries must be below bin/")
    environment = dict(os.environ)
    compiler = shlex.split(compiler)
    # Link options are omitted from object compilation but participate in its
    # toolchain fingerprint (selected -L paths/libraries are real inputs).
    base = fingerprint(compiler, flags, environment)
    link_tools = server.toolchain_key(
        environment, configuration=shlex.join(compiler) + "\n" + shlex.join(flags + link_flags),
        working_directory=ROOT)
    key = hashlib.sha256(json.dumps([base, sources, link_flags, link_tools]).encode()).hexdigest()
    value = environment.get(server.CACHE_ENV, str(ROOT / "bin/regression-artifacts"))
    if value == "off":
        destination.parent.mkdir(parents=True, exist_ok=True)
        command(compiler + flags + sources + link_flags + ["-o", str(destination)], environment)
        print(f"NATIVE_BUILD built name={name} compile={time.monotonic()-started:.3f}s "
              "link=0.000s lookup=0.000s objects_reused=0", flush=True)
        return destination
    cache = Path(value).resolve()
    if not cache.is_relative_to((ROOT / "bin").resolve()):
        raise ValueError("native artifact cache must be below bin/")
    cache = cache / "native"
    objects = cache / "objects"
    binaries = cache / "binaries"
    objects.mkdir(parents=True, exist_ok=True)
    binaries.mkdir(parents=True, exist_ok=True)
    with (binaries / (key + ".lock")).open("a") as lock:
        fcntl.flock(lock, fcntl.LOCK_EX)
        existing = verified(binaries, key, executable=True)
        if existing:
            print(f"NATIVE_BUILD reused name={name} compile=0.000s link=0.000s "
                  f"lookup={time.monotonic()-started:.3f}s objects_reused={len(sources)}", flush=True)
            return existing
        # Hold the object-family lock through qualification. New objects have
        # no trusted manifests until every input has been verified after link.
        # A killed build can leave temporary bytes, never a poisoned cache hit.
        with (objects / (base + ".lock")).open("a") as family_lock:
            fcntl.flock(family_lock, fcntl.LOCK_EX)
            compile_started = time.monotonic()
            created = []

            def object_file(source):
                object_key = hashlib.sha256(json.dumps([base, source]).encode()).hexdigest()
                existing_object = verified(objects, object_key)
                if existing_object:
                    return existing_object, True
                descriptor, filename = tempfile.mkstemp(prefix=object_key + "-", suffix=".o", dir=objects)
                os.close(descriptor)
                path = Path(filename)
                created.append((object_key, path))
                command(compiler + flags + ["-c", source, "-o", str(path)], environment)
                return path, False

            jobs = max(1, min(2, int(environment.get("DURIS_NATIVE_BUILD_JOBS", "2"))))
            binary = None
            qualified = False
            try:
                with ThreadPoolExecutor(max_workers=jobs) as executor:
                    built = list(executor.map(object_file, sources))
                compile_elapsed = time.monotonic() - compile_started
                descriptor, filename = tempfile.mkstemp(prefix=key + "-", dir=binaries)
                os.close(descriptor)
                binary = Path(filename)
                link_started = time.monotonic()
                command(compiler + flags + [str(path) for path, _ in built] + link_flags
                        + ["-o", str(binary)], environment)
                link_elapsed = time.monotonic() - link_started
                if (fingerprint(compiler, flags, environment) != base or server.toolchain_key(
                        environment, configuration=shlex.join(compiler) + "\n" + shlex.join(flags + link_flags),
                        working_directory=ROOT) != link_tools):
                    raise RuntimeError("native build inputs changed during compilation; retry")
                qualified = True
                for object_key, path in created:
                    publish(objects, object_key, path)
                publish(binaries, key, binary, executable=True)
            except BaseException:
                if binary is not None:
                    binary.unlink(missing_ok=True)
                if not qualified:
                    for _, path in created:
                        path.unlink(missing_ok=True)
                raise
            print(f"NATIVE_BUILD built name={name} compile={compile_elapsed:.3f}s "
                  f"link={link_elapsed:.3f}s lookup={max(0, time.monotonic()-started-compile_elapsed-link_elapsed):.3f}s "
                  f"objects_reused={sum(reused for _, reused in built)}", flush=True)
            return binary
