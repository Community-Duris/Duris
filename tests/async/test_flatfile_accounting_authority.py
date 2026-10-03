#!/usr/bin/env python3
"""Native retained flatfile lifetime/epoch metadata qualification."""
import os
from pathlib import Path
import subprocess
import tempfile
from test_flatfile_accounting_store import ROOT, SOURCES


def main():
    sources = ["tests/async/flatfile_accounting_authority_test.cpp",
               "src/flatfile/flatfile_accounting_authority.c", *SOURCES[1:]]
    with tempfile.TemporaryDirectory(prefix="duris-economic-authority-") as temporary:
        binary = Path(temporary) / "authority"
        subprocess.run([
            "g++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
            "-O1", "-g", "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
            "-fno-pie", "-no-pie", "-DDURIS_FLATFILE_ACCOUNTING_TEST",
            "-DDURIS_FLATFILE_AUTHORITY_FAULT_TEST", "-Isrc", *sources,
            "-Wl,--wrap=_Znwm,--wrap=_Znam,--wrap=openat", "-lcrypto", "-pthread", "-o", str(binary),
        ], cwd=ROOT, check=True)
        subprocess.run([str(binary), str(Path(temporary) / "state")], cwd=ROOT, check=True,
                       env=dict(os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
                                UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1"))


if __name__ == "__main__":
    main()
