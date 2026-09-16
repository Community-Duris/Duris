#!/usr/bin/env python3
"""The auction and collector services share one explicit room registry."""

from pathlib import Path
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[2]


def main() -> None:
    auction_source = (ROOT / "src/economy/auction_houses.c").read_text()
    assert auction_source.count("install_auction_house_room_procedures();") == 2
    for room_vnum in (16885, 83117, 97756, 17736, 55193, 888, 1200, 69, 420, 132821):
        assert f"real_room0({room_vnum})" not in auction_source

    output = ROOT / "bin" / "tests"
    output.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="auction-room-registry-", dir=output) as directory:
        binary = Path(directory) / "auction-room-registry"
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
                str(ROOT / "src/economy/auction_room_registry.c"),
                str(ROOT / "tests/async/auction_room_registry_harness.cpp"),
                "-o",
                str(binary),
            ],
            check=True,
        )
        subprocess.run([str(binary)], check=True, timeout=30)


if __name__ == "__main__":
    main()
