#!/usr/bin/env python3
"""UID keyed pile lifetime, replay protection, recovery, and corruption."""

import os
from pathlib import Path
import subprocess
import tempfile

from test_flatfile_accounting_store import ROOT, SOURCES


def main():
    with tempfile.TemporaryDirectory(prefix="duris-flat-pile-state-") as temporary:
        binary = Path(temporary) / "pile-state"
        sources = [
            "tests/async/flatfile_accounting_pile_state_test.cpp",
            "src/flatfile/flatfile_accounting_pile_state.c",
            *SOURCES[1:],
        ]
        subprocess.run(
            [
                "g++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                "-O1", "-g", "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
                "-fno-pie", "-no-pie", "-D__NO_MYSQL__",
                "-DDURIS_FLATFILE_ACCOUNTING_TEST",
                "-DDURIS_FLATFILE_AUTHORITY_FAULT_TEST", "-Isrc", *sources,
                "-lcrypto", "-pthread", "-o", str(binary),
            ],
            cwd=ROOT,
            check=True,
        )
        subprocess.run(
            [str(binary), str(Path(temporary) / "state")],
            cwd=ROOT,
            check=True,
            env=dict(os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
                     UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1"),
        )


if __name__ == "__main__":
    main()
