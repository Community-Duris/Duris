#!/usr/bin/env python3
"""Native flatfile peer wallet coin root and exact replay journey."""
import os
from pathlib import Path
import subprocess
import tempfile
from test_flatfile_accounting_store import ROOT, SOURCES


def main():
    with tempfile.TemporaryDirectory(prefix="duris-flat-coin-") as temporary:
        work = Path(temporary)
        fixture = (ROOT / "tests/async/flatfile_accounting_bank_test.cpp").read_text()
        marker = "int main(int argc, char **argv)"
        assert fixture.count(marker) == 1
        (work / "phase8_bank_fixture.h").write_text(fixture.split(marker)[0])
        sources = [
            "tests/async/flatfile_accounting_coin_test.cpp",
            "src/flatfile/flatfile_accounting_coin_transaction.c",
            "src/flatfile/flatfile_accounting_pile_state.c",
            "src/flatfile/flatfile_accounting_pile_baseline.c",
            "src/flatfile/flatfile_accounting_baseline.c",
            "src/flatfile/flatfile_accounting_lifecycle_transaction.c",
            "src/flatfile/flatfile_item_repository.c",
            "src/flatfile/flatfile_item_accounting_reference.c",
            "src/flatfile/flatfile_locker_repository.c",
            "src/flatfile/flatfile_player_snapshot_file.c",
            "src/flatfile/flatfile_world_item_repository.c",
            "src/flatfile/flatfile_accounting_authority.c",
            "src/flatfile/flatfile_identity_repository.c",
            "src/flatfile/flatfile_player_domain_repository.c",
            "src/world/epic_command.c", "src/combat/combat_outcome_command.c",
            "src/economy/coin_transfer_command.c",
            "src/economy/coin_transfer_accounting.c",
            "src/economy/economic_baseline_adapter.c",
            "src/economy/economic_baseline_codec.c",
            "src/economy/economic_baseline_command.c",
            "src/item/economic_accounting_item_reference.c",
            "src/economy/item_transfer_accounting.c",
            *SOURCES[1:],
        ]
        binary = work / "coin"
        subprocess.run([
            "g++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
            "-ffunction-sections", "-fdata-sections", "-Wl,--gc-sections",
            "-O1", "-g", "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
            "-fno-pie", "-no-pie", "-D__NO_MYSQL__",
            "-DDURIS_FLATFILE_ACCOUNTING_TEST", "-DDURIS_FLATFILE_AUTHORITY_FAULT_TEST",
            "-DDURIS_ECONOMIC_GAMEPLAY_AUTHORITY_TEST",
            "-Isrc", "-Isrc/no_mysql", "-I" + str(work), *sources,
            "-Wl,--wrap=_Znwm,--wrap=_Znam", "-lcrypto", "-lz", "-pthread",
            "-o", str(binary),
        ], cwd=ROOT, check=True)
        subprocess.run([str(binary), str(work / "state")], check=True, timeout=120,
                       env=dict(os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
                                UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1"))


if __name__ == "__main__":
    main()
