#!/usr/bin/env python3
"""Complete original journal TU CRC equivalence; no journal/service/native worker run."""
import argparse
import os
from pathlib import Path
import signal
import tempfile

from test_item_transfer_complete_bounded import execute, terminate_on_signal

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compiler", default=os.environ.get("CXX", "g++"))
    args = parser.parse_args()
    signal.signal(signal.SIGTERM, terminate_on_signal)
    parent = Path(os.environ.get("BIN_ROOT", str(ROOT / "bin"))).resolve() / "tests"
    parent.mkdir(parents=True, exist_ok=True)
    scratch = Path(tempfile.mkdtemp(prefix="save-journal-crc-", dir=parent))
    binary = scratch / "crc-controls"
    execute([args.compiler, "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
             "-O1", "-g0", "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
             "-fno-pie", "-no-pie", "-ffunction-sections", "-fdata-sections",
             "-I" + str(ROOT / "src"),
             str(ROOT / "tests/async/player_save_journal_crc_test.cpp"),
             "-Wl,--gc-sections", "-pthread", "-o", str(binary)],
            cwd=ROOT, timeout=120)
    output = execute([str(binary)], cwd=ROOT, timeout=20, capture_output=True,
                     env=dict(os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
                              UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1"))
    (scratch / "output.txt").write_text(output, encoding="utf-8")
    print(output, end="")
    print("Retained checksum evidence: " + str(scratch))


if __name__ == "__main__":
    main()
