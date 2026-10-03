#!/usr/bin/env python3
"""Execute the production network-turn/deadline seams against real sockets.

Transport faults and game owners are isolated; poll(), wakeup notification,
registration, turn ordering and deadline waiting are the production functions.
"""
from pathlib import Path
import subprocess
import tempfile

from _paths import ROOT, extract_function

functions = "\n\n".join(extract_function("net/comm.c", signature) for signature in (
    "static int drain_network_transport(",
    "static bool service_network_turn(",
    "static void network_wait_until(",
))
template = (ROOT / "tests/async/network_readiness_runtime_harness.cpp").read_text()
transport_input = extract_function("net/comm.c", "int process_input(").replace(
    "int process_input(", "static int transport_process_input(", 1)
connection = extract_function("net/comm.c", "static bool run_connection_phase(")
boundary_start = connection.index("for (P_desc point = ready ? descriptor_list")
boundary = connection[boundary_start:connection.index("ctx.connections_us =", boundary_start)]
build_root = ROOT / "bin/tests"
build_root.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix="network-readiness-", dir=build_root) as directory:
    build = Path(directory)
    harness = build / "harness.cpp"
    binary = build / "readiness"
    harness.write_text(template.replace("/* PRODUCTION_NETWORK_FUNCTIONS */", functions)
                      .replace("/* PRODUCTION_INPUT_FUNCTION */", transport_input)
                      .replace("/* PRODUCTION_CONNECTION_BOUNDARY */", boundary))
    subprocess.run([
        "g++", "-std=c++20", "-Wall", "-Wextra", "-Werror", "-g", "-O1",
        "-fsanitize=address,undefined", "-D__NO_MYSQL__",
        f"-I{ROOT / 'src'}", f"-I{ROOT / 'src/no_mysql'}", str(harness),
        "-pthread", "-o", str(binary)
    ], check=True, timeout=120)
    subprocess.run([str(binary)], check=True, timeout=30)
