#!/usr/bin/env python3
"""Collector NPC reconciliation and protected-service boundary regressions."""

from pathlib import Path
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[2]


def main() -> None:
    heavens = (ROOT / "areas/mob/heavens.mob").read_text()
    assert heavens.count("#1257\n") == 1
    assert heavens.index("#1256\n") < heavens.index("#1257\n")
    assert "Collector of Antiquities" in heavens

    comm = (ROOT / "src/net/comm.c").read_text()
    assert "collector_presence_init()" in comm
    assert "collector_presence_pulse();" in comm
    assert "collector_presence_shutdown();" in comm
    service = (ROOT / "src/economy/collector_service.c").read_text()
    assert "collector_presence_room_active(character->in_room)" in service

    guarded_sources = {
        "src/cmd/actoff.c": 1,
        "src/classes/innates.c": 1,
        "src/combat/fight.c": 5,
        "src/cmd/actobj.c": 2,
        "src/cmd/actoth.c": 1,
        "src/classes/necromancy.c": 1,
        "src/net/sparser.c": 1,
        "src/mob/mobact.c": 1,
    }
    for relative, minimum in guarded_sources.items():
        source = (ROOT / relative).read_text()
        assert source.count("collector_presence_is_npc") >= minimum, relative

    output = ROOT / "bin" / "tests"
    output.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="collector-presence-", dir=output) as directory:
        binary = Path(directory) / "collector-presence"
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
                str(ROOT / "src/economy/collector_presence.c"),
                str(ROOT / "tests/async/collector_presence_harness.cpp"),
                "-o",
                str(binary),
            ],
            check=True,
        )
        subprocess.run([str(binary)], check=True, timeout=30)


if __name__ == "__main__":
    main()
