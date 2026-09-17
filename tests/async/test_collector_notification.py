#!/usr/bin/env python3
"""Player-facing Collector recovery notification contract."""

from pathlib import Path
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[2]


def main() -> None:
    output = ROOT / "bin" / "tests"
    output.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="collector-notification-", dir=output) as directory:
        binary = Path(directory) / "collector-notification"
        subprocess.run(
            [
                "g++",
                "-std=c++20",
                "-Wall",
                "-Wextra",
                "-Wpedantic",
                "-Werror",
                "-O2",
                "-D__NO_MYSQL__",
                "-I",
                str(ROOT / "src"),
                "-I",
                str(ROOT / "src/no_mysql"),
                str(ROOT / "src/economy/collector_policy.c"),
                str(ROOT / "src/economy/collector_notification.c"),
                str(ROOT / "src/persistence/critical_command.c"),
                str(ROOT / "tests/async/collector_notification_harness.cpp"),
                "-lcrypto",
                "-o",
                str(binary),
            ],
            check=True,
        )
        subprocess.run([str(binary)], check=True, timeout=30)
    print("collector notification contract passed")


if __name__ == "__main__":
    main()
