#!/usr/bin/env python3
"""Native borrowed-lock authority reads and legacy evidence preservation."""
import os
from pathlib import Path
import subprocess
import tempfile
from test_flatfile_accounting_store import ROOT, SOURCES


def main():
    sources = ["tests/async/flatfile_accounting_native_reads_test.cpp",
               "src/flatfile/flatfile_identity_repository.c",
               "src/flatfile/flatfile_player_domain_repository.c",
               "src/world/epic_command.c", "src/combat/combat_outcome_command.c",
               *SOURCES[2:]]
    with tempfile.TemporaryDirectory(prefix="duris-borrowed-reads-") as temporary:
        binary = Path(temporary) / "reads"
        subprocess.run([
            "g++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
            "-O1", "-g", "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
            "-fno-pie", "-no-pie", "-D__NO_MYSQL__",
            "-DDURIS_FLATFILE_TRANSACTION_FAULT_TEST",
            "-DDURIS_FLATFILE_AUTHORITY_FAULT_TEST", "-Isrc", "-Isrc/no_mysql", *sources,
            "-Wl,--wrap=_Znwm,--wrap=_Znam", "-lcrypto", "-pthread", "-o", str(binary),
        ], cwd=ROOT, check=True)
        subprocess.run([str(binary), str(Path(temporary) / "state")], cwd=ROOT, check=True,
                       timeout=90,
                       env=dict(os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
                                UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1"))


if __name__ == "__main__":
    main()
