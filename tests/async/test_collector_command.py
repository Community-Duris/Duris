#!/usr/bin/env python3
"""Versioned collector critical-command codec and fence regressions."""

from pathlib import Path
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[2]


def main() -> None:
    output = ROOT / "bin" / "tests"
    output.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="collector-command-", dir=output) as directory:
        binary = Path(directory) / "collector-command"
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
                str(ROOT / "src/economy/currency_command.c"),
                str(ROOT / "src/item/item_transfer_command.c"),
                str(ROOT / "src/persistence/critical_command.c"),
                str(ROOT / "tests/async/collector_command_harness.cpp"),
                "-lcrypto",
                "-o",
                str(binary),
            ],
            check=True,
        )
        subprocess.run([str(binary)], check=True, timeout=30)


if __name__ == "__main__":
    main()
