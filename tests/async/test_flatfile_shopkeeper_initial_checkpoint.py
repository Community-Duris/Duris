#!/usr/bin/env python3
"""Future native regression: run only after relevant major-plan readiness.

Requires a POSIX C++20 toolchain, OpenSSL development files and pthreads.
The harness uses synthetic records and this invocation's temporary flatfile
fixture only; it does not connect to SQL, Redis, or the live game. Compiling
and running this file is execution, and was deliberately deferred at creation.
"""

from _paths import rel
import pathlib
import subprocess
import tempfile


ROOT = pathlib.Path(__file__).resolve().parents[2]


def main():
    with tempfile.TemporaryDirectory(prefix="duris-initial-shopkeeper-") as temporary:
        temporary_path = pathlib.Path(temporary)
        binary = temporary_path / "flatfile_shopkeeper_initial_checkpoint_test"
        compile_result = subprocess.run(
            [
                "g++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-Isrc",
                "tests/async/flatfile_shopkeeper_initial_checkpoint_harness.cpp",
                rel("flatfile_shopkeeper_repository.c"), rel("player_snapshot_codec.c"),
                rel("shop_trade_command.c"), rel("shop_trade_recovery_manifest.c"),
                rel("item_transfer_command.c"), rel("quest_mobile_native_reference.c"),
                rel("economic_source_event.c"), rel("craft_pouch_mutation.c"),
                rel("chaos_pouch_ledger.c"), rel("currency_command.c"),
                rel("critical_command.c"), rel("lockpick_retirement_continuation.c"),
                rel("native_quest_cost.c"), rel("native_quest_coin_give.c"),
                rel("flatfile_authority_transaction.c"), rel("flatfile_store.c"),
                "-lcrypto", "-pthread", "-o", str(binary),
            ],
            cwd=ROOT, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
        )
        if compile_result.returncode:
            raise SystemExit(compile_result.stdout)
        run_result = subprocess.run(
            [str(binary), str(temporary_path / "state")], cwd=ROOT, text=True,
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
        )
        if run_result.returncode:
            raise SystemExit(run_result.stdout)
        print(run_result.stdout.strip())


if __name__ == "__main__":
    main()
