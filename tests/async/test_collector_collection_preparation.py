#!/usr/bin/env python3
"""Collector collection captures and validates live custody roots."""

from pathlib import Path
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[2]


def main() -> None:
    source = (ROOT / "src/economy/collector_collection_preparation.c").read_text()
    assert "ITEM2_ACCOUNT_BOUND" in source
    assert "ITEM_NORENT" in source
    assert "ITEM_NOSELL" in source
    assert "ITEM_TRANSIENT" in source
    assert "collector_collection_detach_live" in source
    output = ROOT / "bin" / "tests"
    output.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="collector-collection-preparation-", dir=output) as directory:
        binary = Path(directory) / "collector-collection-preparation"
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
                str(ROOT / "src/economy/collector_collection_preparation.c"),
                str(ROOT / "tests/async/collector_collection_preparation_harness.cpp"),
                "-o",
                str(binary),
            ],
            check=True,
        )
        subprocess.run([str(binary)], check=True, timeout=30)


if __name__ == "__main__":
    main()
