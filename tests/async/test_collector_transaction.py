#!/usr/bin/env python3
"""Collector coordinator, publication, reconnect, and outbox regressions."""

from pathlib import Path
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[2]


def main() -> None:
    output = ROOT / "bin" / "tests"
    output.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="collector-transaction-", dir=output) as directory:
        binary = Path(directory) / "collector-transaction"
        subprocess.run(
            [
                "g++",
                "-std=c++20",
                "-Wall",
                "-Wextra",
                "-Wpedantic",
                "-Werror",
                "-O2",
                "-I",
                str(ROOT / "src"),
                str(ROOT / "src/economy/collector_policy.c"),
                str(ROOT / "src/economy/collector_codec.c"),
                str(ROOT / "src/economy/collector_command.c"),
                str(ROOT / "src/economy/collector_transaction.c"),
                str(ROOT / "src/economy/currency_command.c"),
                str(ROOT / "src/item/item_transfer_command.c"),
                str(ROOT / "src/persistence/critical_command.c"),
                str(ROOT / "tests/async/collector_transaction_harness.cpp"),
                "-lcrypto",
                "-pthread",
                "-o",
                str(binary),
            ],
            check=True,
        )
        subprocess.run([str(binary)], check=True, timeout=30)


if __name__ == "__main__":
    main()
