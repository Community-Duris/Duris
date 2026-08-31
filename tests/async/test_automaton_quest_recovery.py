#!/usr/bin/env python3
"""Keep automaton quest setup on the side of the world-authority boundary."""

from _paths import SRC
from pathlib import Path


root = Path(__file__).resolve().parents[2]
ship_base = (SRC / "ships" / "ship_base.c").read_text()
comm = (SRC / "comm.c").read_text()

initializer_start = ship_base.index("void initialize_ships()")
initializer = ship_base[initializer_start:ship_base.index("\nvoid shutdown_ships()", initializer_start)]
assert "!redis_world_recovery_boot_active() && !load_moonstone_fragments()" in initializer

recovery_start = comm.index("// redis crash recovery - restore world state from redis snapshot")
recovery_end = comm.index("\n\tPROFILES(RESET);", recovery_start)
recovery = comm[recovery_start:recovery_end]
failure = recovery[recovery.index("else\n\t\t{"):]
assert failure.index("reset_zone(zone, 2)") < failure.index("load_moonstone_fragments()")

print("automaton quest recovery ordering contract passed")
