#!/usr/bin/env python3
"""Native typed flatfile ATM domain/evidence root journeys."""
import os
from pathlib import Path
import subprocess
import tempfile
from native_build_artifacts import build_native
from test_flatfile_accounting_store import ROOT, SOURCES


def main():
    compiler = os.environ.get("CXX", "g++")
    sources = ["tests/async/flatfile_accounting_bank_test.cpp",
               "src/flatfile/flatfile_accounting_authority.c",
               "src/flatfile/flatfile_accounting_bank_transaction.c",
               "src/flatfile/flatfile_identity_repository.c",
               "src/flatfile/flatfile_player_domain_repository.c",
               "src/flatfile/flatfile_player_snapshot_file.c",
               "src/economy/economic_gameplay_authority.c",
               "src/economy/economic_command_admission.c",
               "src/economy/coin_transfer_accounting.c",
               "src/economy/item_transfer_accounting.c",
               "src/economy/coin_transfer_command.c",
               "src/persistence/critical_command_coordinator.c",
               "src/persistence/critical_command_journal.c",
               "src/world/epic_command.c", "src/combat/combat_outcome_command.c", *SOURCES[1:]]
    (ROOT / "bin/tests").mkdir(parents=True, exist_ok=True)
    with (tempfile.TemporaryDirectory(prefix="duris-flat-bank-", dir=ROOT / "bin/tests") as build,
          tempfile.TemporaryDirectory(prefix="duris-flat-bank-state-") as temporary):
        binary = Path(build) / "bank"
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
                "-D__NO_MYSQL__",
                "-DDURIS_FLATFILE_ACCOUNTING_TEST",
                "-DDURIS_FLATFILE_AUTHORITY_FAULT_TEST",
                "-DDURIS_ECONOMIC_GAMEPLAY_AUTHORITY_TEST",
                "-Isrc",
                "-Isrc/no_mysql",
                "-pthread",
            ],
            [
                "-Wl,--wrap=_Znwm,--wrap=_Znam",
                "-lcrypto",
                "-lz",
                "-pthread",
            ],
            compiler=compiler, name="accounting-bank",
        )
        print("BANK-NATIVE compiled", flush=True)
        arguments = [str(binary), str(Path(temporary) / "state")]
        if os.environ.get("DURIS_BANK_GENERATED_ONLY") == "1":
            arguments.append("--generated-only")
        subprocess.run(arguments, cwd=ROOT, check=True,
                       timeout=660,
                       env=dict(os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
                                UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1"))


if __name__ == "__main__":
    main()
