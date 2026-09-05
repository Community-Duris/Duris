#!/usr/bin/env python3
"""Exercise the real Telnet/MCCP sender with deterministic transport faults."""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
BUILD = ROOT / "bin/tests"
BUILD.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix="telnet-output-", dir=BUILD) as directory:
    binary = Path(directory) / "telnet_output_runtime"
    subprocess.run([
        "g++", "-std=c++20", "-Wall", "-Wextra", "-Werror", "-Wlogical-op", "-D__NO_MYSQL__",
        "-ffunction-sections", "-fdata-sections", f"-I{ROOT / 'src'}",
        f"-I{ROOT / 'src/no_mysql'}", str(ROOT / "tests/async/telnet_output_runtime_harness.cpp"),
        str(ROOT / "src/net/mccp.c"), str(ROOT / "src/net/unicode.c"), "-Wl,--gc-sections", "-Wl,--wrap=write", "-lz",
        "-o", str(binary)
    ], check=True, cwd=ROOT, timeout=120)
    subprocess.run([str(binary)], check=True, cwd=ROOT, timeout=30)
