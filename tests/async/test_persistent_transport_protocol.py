#!/usr/bin/env python3
"""Execute the real private protocol with credentials, malformed frames and backpressure."""
from pathlib import Path
import subprocess
import tempfile
from _paths import extract_function

ROOT = Path(__file__).resolve().parents[2]
BUILD = ROOT / "bin/tests"
BUILD.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix="transport-protocol-", dir=BUILD) as directory:
    binary = Path(directory) / "protocol"
    harness = Path(directory) / "protocol.cpp"
    production = extract_function("net/transport.c", "bool transport_descriptor_eligible(")
    configure = extract_function("net/transport.c", "bool transport_world_configure(")
    harness.write_text((ROOT / "tests/async/transport_protocol_runtime_harness.cpp").read_text()
                       .replace("/* PRODUCTION_SESSION_ELIGIBILITY */", production)
                       .replace("/* PRODUCTION_WORLD_CONFIGURE */", configure))
    subprocess.run(["g++", "-std=c++20", "-Wall", "-Wextra", "-Werror", "-Wlogical-op",
                    "-fsanitize=address,undefined", "-g", "-Isrc",
                    str(harness), "-o", str(binary)],
                   cwd=ROOT, check=True, timeout=120)
    subprocess.run([str(binary)], cwd=ROOT, check=True, timeout=30)
