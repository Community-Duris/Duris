#!/usr/bin/env python3
"""Guarded real-factory test with credential-selection spy; never changes grants."""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
if os.environ.get("TELEMETRY_REPOSITORY_DISPOSABLE") != "1":
    print("SKIP: real-factory SQL test requires explicitly disposable loopback fixture")
    raise SystemExit(0)
flags = shlex.split(subprocess.check_output(["mysql_config", "--cflags"], text=True))
libs = shlex.split(subprocess.check_output(["mysql_config", "--libs"], text=True))
with tempfile.TemporaryDirectory(prefix="telemetry-connection-") as directory:
    obj = str(Path(directory) / "sql.o")
    exe = str(Path(directory) / "factory")
    subprocess.run(["g++", "-std=c++20", "-DTEST_MUD", "-ffunction-sections",
                    "-fdata-sections", "-Isrc", "-I/usr/include/libxml2", *flags,
                    "-c", "src/sql/sql.c", "-o", obj], cwd=ROOT, check=True)
    subprocess.run(["g++", "-std=c++20", "-Wall", "-Wextra", "-Werror", "-Isrc",
                    *flags, "tests/async/telemetry_connection_harness.cc", obj,
                    "-Wl,--gc-sections", "-Wl,--wrap=mysql_real_connect",
                    "-Wl,--wrap=mysql_close", "-Wl,--wrap=_Znwm", *libs,
                    "-o", exe], cwd=ROOT, check=True)
    subprocess.run([exe], cwd=ROOT, check=True)