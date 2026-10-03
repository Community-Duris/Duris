#!/usr/bin/env python3
"""Exercise bounded application queues with real Telnet/WebSocket receive paths."""
import os
from pathlib import Path
import subprocess
import tempfile

from _paths import ROOT, extract_function


functions = [extract_function("comm.c", signature) for signature in [
    "int get_from_q(", "static int get_filtered_cmd_from_q(",
    "int get_casting_cmd_from_q(", "int get_item_movement_cmd_from_q(",
    "int get_pending_transaction_cmd_from_q(", "static int get_playing_cmd_from_q(",
    "static bool casting_input_for_descriptor(", "static session_input_route select_session_input(",
    "void write_to_q(", "void queue_websocket_input(", "bool admit_session_oob(",
    "static void report_input_queue_overflow(", "void flush_queues(",
    "static void note_player_input_activity(", "static void process_line(P_desc t, char *in)\n{",
    "int process_input(", "void delete_doubledollar(", "int process_output(",
    "static void run_output_phase(",
]]
functions += [extract_function("mccp.c", "int parse_telnet_options("),
              extract_function("ws_handlers.c", "void ws_cmd_game(")]

# Every browser route that creates ordinary commands must use bounded admission.
handlers = (ROOT / "src/net/ws_handlers.c").read_text()
assert "queue_websocket_input(d, cmd);" in extract_function("ws_handlers.c", "void ws_handle_command(")
assert "write_to_q(cmd, &d->input, 0)" not in handlers
assert "flush_queues(d);" in extract_function("comm.c", "void close_socket(")

build = ROOT / "bin/tests"
build.mkdir(parents=True, exist_ok=True)
flags = ["-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie"] if os.environ.get("SANITIZE") == "1" else []
with tempfile.TemporaryDirectory(prefix="session-queues-", dir=build) as directory:
    temp = Path(directory)
    (temp / "production_session_queues.inc").write_text("\n\n".join(functions))
    binary = temp / "harness"
    subprocess.run([
        "g++", "-std=c++20", "-Wall", "-Wextra", "-Werror", "-Og", "-g",
        "-D__NO_MYSQL__", *flags, f"-I{ROOT / 'src'}", f"-I{ROOT / 'src/no_mysql'}",
        f"-I{temp}", str(ROOT / "tests/async/session_queues_runtime_harness.cpp"),
        str(ROOT / "src/net/websocket.c"), str(ROOT / "src/net/ansi.c"),
        str(ROOT / "src/net/unicode.c"), str(ROOT / "src/core/safe_format.c"),
        "-lcjson", "-lssl", "-lcrypto", "-lz", "-lbsd", "-lgnutls", "-o", str(binary),
    ], check=True, cwd=ROOT, timeout=120)
    subprocess.run([str(binary)], check=True, cwd=ROOT, timeout=30)

print("Bounded session queue runtime regressions passed")
