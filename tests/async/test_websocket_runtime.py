#!/usr/bin/env python3
"""Compile and execute the WebSocket parser and output runtime harness."""

import pathlib
import subprocess
import tempfile


ROOT = pathlib.Path(__file__).resolve().parents[2]
SRC = ROOT / "src"
HARNESS = ROOT / "tests/async/websocket_runtime_harness.cpp"


with tempfile.TemporaryDirectory(prefix="duris-websocket-runtime-") as directory:
    binary = pathlib.Path(directory) / "websocket_runtime_harness"
    build = subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Werror",
            "-D__NO_MYSQL__",
            f"-I{SRC}",
            f"-I{SRC / 'no_mysql'}",
            "-I/usr/include/cjson",
            str(HARNESS),
            str(SRC / "websocket.c"),
            "-lcjson",
            "-lssl",
            "-lcrypto",
            "-lz",
            "-o",
            str(binary),
        ],
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        timeout=120,
    )
    if build.returncode != 0:
        raise AssertionError("WebSocket runtime harness did not compile:\n" + build.stdout)

    run = subprocess.run(
        [str(binary)],
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        timeout=30,
    )
    if run.returncode != 0:
        raise AssertionError("WebSocket runtime harness failed:\n" + run.stdout)
    if "WebSocket runtime harness passed" not in run.stdout:
        raise AssertionError("WebSocket runtime harness omitted its completion marker")

print("WebSocket parser and output runtime harness passed")
