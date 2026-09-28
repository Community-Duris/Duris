#!/usr/bin/env python3
"""Native segmented accounting storage and process-crash recovery qualification."""

import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
SOURCES = [
    "tests/async/flatfile_accounting_store_test.cpp",
    "src/flatfile/flatfile_accounting_store.c",
    "src/flatfile/flatfile_authority_transaction.c",
    "src/flatfile/flatfile_store.c",
    "src/persistence/critical_command.c",
    "src/economy/currency_command.c",
    "src/item/item_transfer_command.c",
    "src/economy/economic_accounting_types.c",
    "src/economy/economic_accounting_plan.c",
    "src/economy/economic_accounting_intent.c",
    "src/economy/economic_currency_adapter.c",
]


def main():
    # Private authority metadata requires a native filesystem, including when
    # the checkout itself lives on a Windows mount under WSL.
    with tempfile.TemporaryDirectory(prefix="duris-accounting-") as temporary:
        binary = Path(temporary) / "store"
        subprocess.run(
            [
                "g++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                "-O1", "-g", "-fsanitize=address,undefined",
                "-fno-omit-frame-pointer", "-fno-pie", "-no-pie",
                "-DDURIS_FLATFILE_ACCOUNTING_TEST",
                "-DDURIS_FLATFILE_AUTHORITY_FAULT_TEST", "-Isrc", *SOURCES,
                "-Wl,--wrap=write,--wrap=fdatasync,--wrap=fsync,--wrap=renameat,--wrap=unlinkat,--wrap=_Znwm",
                "-lcrypto", "-pthread", "-o", str(binary),
            ],
            cwd=ROOT,
            check=True,
        )
        subprocess.run(
            [str(binary), str(Path(temporary) / "state")],
            cwd=ROOT,
            check=True,
            env=dict(
                os.environ,
                ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
                UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1",
            ),
        )


if __name__ == "__main__":
    main()
