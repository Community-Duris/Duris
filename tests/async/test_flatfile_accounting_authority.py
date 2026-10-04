#!/usr/bin/env python3
"""Native retained flatfile lifetime/epoch metadata qualification."""
import os
from pathlib import Path
import subprocess
import tempfile
from native_build_artifacts import build_native
from test_flatfile_accounting_store import ROOT, SOURCES


def main():
    sources = ["tests/async/flatfile_accounting_authority_test.cpp",
               "src/flatfile/flatfile_accounting_authority.c", *SOURCES[1:]]
    (ROOT / "bin/tests").mkdir(parents=True, exist_ok=True)
    with (tempfile.TemporaryDirectory(prefix="duris-economic-authority-", dir=ROOT / "bin/tests") as build,
          tempfile.TemporaryDirectory(prefix="duris-economic-authority-state-") as temporary):
        binary = Path(build) / "authority"
        binary = build_native(
            binary,
            sources,
            [
                "-std=c++20",
                "-Wall",
                "-Wextra",
                "-Wpedantic",
                "-Werror",
                "-O1",
                "-g",
                "-fsanitize=address,undefined",
                "-fno-omit-frame-pointer",
                "-fno-pie",
                "-no-pie",
                "-DDURIS_FLATFILE_ACCOUNTING_TEST",
                "-DDURIS_FLATFILE_AUTHORITY_FAULT_TEST",
                "-Isrc",
                "-pthread",
            ],
            [
                "-Wl,--wrap=_Znwm,--wrap=_Znam,--wrap=openat",
                "-lcrypto",
                "-pthread",
            ],
            compiler="g++", name="accounting-authority",
        )
        subprocess.run([str(binary), str(Path(temporary) / "state")], cwd=ROOT, check=True,
                       env=dict(os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
                                UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1"))


if __name__ == "__main__":
    main()
