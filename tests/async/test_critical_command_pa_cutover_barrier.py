#!/usr/bin/env python3
"""Exercise the coordinator's pending-work cutover barrier end to end."""

from pathlib import Path
import subprocess
import tempfile

from _paths import ROOT, rel


SOURCE = Path(__file__).with_name("critical_command_pa_cutover_barrier.cpp")

with tempfile.TemporaryDirectory(prefix="duris-pa-cutover-barrier-") as temporary:
    root = Path(temporary)
    binary = root / "critical_command_pa_cutover_barrier"
    subprocess.run(
        [
            "g++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
            "-pthread", "-Isrc", str(SOURCE), rel("critical_command.c"),
            rel("critical_command_journal.c"), rel("critical_command_coordinator.c"),
            "-lz", "-lcrypto", "-Wl,--wrap=close", "-o", str(binary),
        ],
        cwd=ROOT,
        check=True,
    )
    subprocess.run(
        [str(binary), str(root / "journals")], check=True, timeout=30
    )

print("critical command pending-work cutover barrier checks passed")
