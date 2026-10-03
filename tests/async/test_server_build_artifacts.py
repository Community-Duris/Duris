#!/usr/bin/env python3
"""Exercise artifact reuse, invalidation, isolation and failed publication."""

import json
import hashlib
from contextlib import redirect_stdout
import io
import multiprocessing
import os
from pathlib import Path
import tempfile
from unittest.mock import patch

import server_build_artifacts as artifacts

# Fingerprint actual bytes in an isolated installed-toolchain layout. Unrelated
# nested Python/data files are not linker inputs; explicit headers and libraries
# remain content-sensitive, even when their timestamps have not changed.
with tempfile.TemporaryDirectory() as tmp:
    root = Path(tmp)
    for directory in ("src", "include", "lib", "lib/python/site-packages", "custom-headers", "custom-libs", "tools"):
        (root / directory).mkdir(parents=True, exist_ok=True)
    header = root / "include/system.h"
    custom_header = root / "custom-headers/feature.h"
    library = root / "lib/libexample.so"
    custom_library = root / "custom-libs/libcustom.a"
    compiler = root / "tools/compiler"
    internal = root / "tools/cc1plus"
    unused = root / "lib/python/site-packages/unrelated.so"
    for path in (header, custom_header, library, custom_library, compiler, internal, unused):
        path.write_bytes(b"original")

    def tool_output(command, **kwargs):
        if "-print-search-dirs" in command:
            return f"libraries: ={root / 'lib'}\n"
        if any(arg.startswith("-print-prog-name=") for arg in command):
            return str(internal)
        return b"isolated compiler version\n"

    with (patch.object(artifacts, "ROOT", root),
          patch.object(artifacts, "SYSTEM_HEADER_DIRECTORIES", (str(root / "include"),)),
          patch.object(artifacts, "SYSTEM_LIBRARY_DIRECTORIES", (str(root / "lib"),)),
          patch.object(artifacts, "compiler_configuration", return_value="g++\n-I../custom-headers -L../custom-libs"),
          patch.object(artifacts.shutil, "which", return_value=str(compiler)),
          patch.object(artifacts.subprocess, "check_output", side_effect=tool_output)):
        original = artifacts.toolchain_key({})
        unused.write_bytes(b"unrelated package update")
        assert original == artifacts.toolchain_key({}), "unsearched nested package invalidated the server"
        for path in (header, custom_header, library, custom_library, compiler, internal):
            timestamp = path.stat()
            path.write_bytes(b"modified")
            os.utime(path, ns=(timestamp.st_atime_ns, timestamp.st_mtime_ns))
            assert original != artifacts.toolchain_key({}), f"changed input was missed: {path}"
            path.write_bytes(b"original")
        nested_library_dir = unused.parent
        baseline = artifacts.toolchain_key({"LIBRARY_PATH": str(nested_library_dir)})
        unused.write_bytes(b"now a selected link input")
        assert baseline != artifacts.toolchain_key({"LIBRARY_PATH": str(nested_library_dir)})


# Recursive/parallel Make may prepend directory or progress lines. They are
# not compiler names, and must not reach shutil.which / Path(None).
with patch.object(artifacts.subprocess, 'check_output', return_value=
                  "make[2]: Entering directory '/tmp/build'\n"
                  "DURIS_ARTIFACT_CC=g++\nDURIS_ARTIFACT_FLAGS=-O1 -pthread\n"
                  "make[2]: Leaving directory '/tmp/build'\n"):
    assert artifacts.compiler_configuration({}) == 'g++\n-O1 -pthread'
with patch.object(artifacts.subprocess, 'check_output', return_value='g++\n'):
    try:
        artifacts.compiler_configuration({})
    except RuntimeError:
        pass
    else:
        raise AssertionError('incomplete artifact configuration was accepted')


# Exercise real GNU Make, including recursive invocation banners and inherited
# command-line overrides; mocking toolchain_key alone missed the CI failure.
with tempfile.TemporaryDirectory() as tmp:
    root = Path(tmp)
    (root / "src").mkdir()
    (root / "src/Makefile").write_text("CC = g++\nCFLAGS = -O1\n")
    with patch.object(artifacts, "ROOT", root):
        environment = dict(os.environ, MAKEFLAGS="w -- CC=clang++ CFLAGS=-O2")
        assert artifacts.compiler_configuration(environment).splitlines() == ["clang++", "-O2"]
        environment["MAKEFLAGS"] = ""
        assert artifacts.compiler_configuration(environment).splitlines() == ["g++", "-O1"]


with tempfile.TemporaryDirectory() as tmp, redirect_stdout(io.StringIO()):
    root = Path(tmp)
    (root / "src").mkdir()
    source = root / "src/example.c"
    source.write_text("original")
    with (patch.object(artifacts, "ROOT", root),
          patch.object(artifacts, "toolchain_key", return_value="compiler-v1") as compiler):
        original = artifacts.input_key({"CXXFLAGS": "-O1"})
        assert original == artifacts.input_key({"CXXFLAGS": "-O1", "PWD": "/other"})
        assert original == artifacts.input_key({"CXXFLAGS": "-O1", "HOSTNAME": "another-container"})
        assert original != artifacts.input_key({"CXXFLAGS": "-O2"})
        assert original != artifacts.input_key({"CXXFLAGS": "-O1", "BUILD_PROFILE": "production"})
        source.write_text("modified")
        assert original != artifacts.input_key({"CXXFLAGS": "-O1"})
        source.write_text("original")
        header = root / "src/untracked.h"
        header.write_text("new header")
        assert original != artifacts.input_key({"CXXFLAGS": "-O1"})
        header.unlink()
        assert original == artifacts.input_key({"CXXFLAGS": "-O1"})
        wrapper = root / "private-compiler.py"
        wrapper.write_bytes(artifacts.COMPILER_WRAPPER.read_bytes())
        with patch.object(artifacts, "COMPILER_WRAPPER", wrapper):
            assert original == artifacts.input_key({"CXXFLAGS": "-O1"})
            wrapper.write_text("changed atomic object publisher")
            assert original != artifacts.input_key({"CXXFLAGS": "-O1"})
        compiler.return_value = "compiler-v2"
        assert original != artifacts.input_key({"CXXFLAGS": "-O1"})


with tempfile.TemporaryDirectory() as tmp, redirect_stdout(io.StringIO()):
    root = Path(tmp)
    cache = root / "bin/cache"
    build_root = root / "bin/private"
    calls = []
    def key_for(value):
        return hashlib.sha256(value.encode()).hexdigest()

    key = key_for("source-toolchain-environment-v1")

    def compile_server(directory, environment, *, lock_fd=None):
        calls.append(directory)
        binary = directory / "server/dms_new"
        binary.parent.mkdir(parents=True, exist_ok=True)
        (directory / "objects").mkdir(exist_ok=True)
        binary.write_bytes(b"test executable " + str(len(calls)).encode())
        binary.chmod(0o755)
        return binary, "g++ -D__NO_MYSQL__", 0.0

    with (patch.object(artifacts, "ROOT", root),
          patch.object(artifacts, "input_key", side_effect=lambda env: key),
          patch.object(artifacts, "compile_server", side_effect=compile_server),
          patch.dict(os.environ, {artifacts.CACHE_ENV: str(cache)})):
        first = artifacts.build_flatfile_server(build_root)
        assert artifacts.build_flatfile_server(root / "another-fixture") == first
        assert len(calls) == 1 and not first.stat().st_mode & 0o222
        assert not build_root.exists(), "reuse must not create runtime state"

        key = key_for("changed-source-or-flags")
        second = artifacts.build_flatfile_server(build_root)
        assert second != first and first.is_file() and len(calls) == 2

        second.chmod(0o755)
        second.write_bytes(b"corrupt")
        second.chmod(0o555)
        third = artifacts.build_flatfile_server(build_root)
        assert third != second and len(calls) == 3
        assert second.read_bytes() == b"corrupt", "never replace a published inode"

        manifest = cache / f"{key}.json"
        manifest.write_text("invalid json")
        fourth = artifacts.build_flatfile_server(build_root)
        assert fourth != third and len(calls) == 4
        metadata = json.loads(manifest.read_text())
        metadata["directory"] = "../escape"
        manifest.write_text(json.dumps(metadata))
        assert artifacts.verified_artifact(cache, key) is None

        key = key_for("concurrent-build")
        context = multiprocessing.get_context("fork")
        results = context.Queue()

        def concurrent_build():
            results.put(str(artifacts.build_flatfile_server(build_root)))

        before = set(cache.glob("artifact-*"))
        processes = [context.Process(target=concurrent_build) for _ in range(3)]
        for process in processes:
            process.start()
        paths = [results.get(timeout=10) for _ in processes]
        for process in processes:
            process.join(timeout=10)
            assert process.exitcode == 0
        assert len(set(paths)) == 1
        assert len(set(cache.glob("artifact-*")) - before) == 1

        log = Path(paths[0]).parents[1] / "build.log"
        log.chmod(0o644)
        log.write_text("corrupt log")
        assert artifacts.verified_artifact(cache, key) is None

        key = key_for("failed-build")
        with patch.object(artifacts, "compile_server", side_effect=RuntimeError("failed")):
            try:
                artifacts.build_flatfile_server(build_root)
            except RuntimeError:
                pass
            else:
                raise AssertionError("failed build was accepted")
        assert not (cache / f"{key}.json").exists()

        key = key_for("changed-during-build")
        with patch.object(artifacts, "input_key", side_effect=[key, "new-input"]):
            try:
                artifacts.build_flatfile_server(build_root)
            except RuntimeError:
                pass
            else:
                raise AssertionError("mixed-input build was published")
        assert not (cache / f"{key}.json").exists()

        with patch.dict(os.environ, {artifacts.CACHE_ENV: "off"}):
            assert artifacts.build_flatfile_server(build_root).is_file()

        with patch.dict(os.environ, {artifacts.CACHE_ENV: str(root / "outside-bin")}):
            try:
                artifacts.build_flatfile_server(build_root)
            except ValueError:
                pass
            else:
                raise AssertionError("cache outside bin was accepted")

print("server artifact reuse, corruption, isolation and failed publication passed")
