#!/usr/bin/env python3
"""Source and runtime contracts for batched pet graph hydration."""

from _paths import SRC
import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
REPOSITORY = (SRC / "player_load_repository.c").read_text()
MATERIALIZE = (SRC / "player_load_materialize.c").read_text()
PETS = (SRC / "player_load_pets.c").read_text()
NANNY = (SRC / "nanny.c").read_text()
COPYOVER = (SRC / "copyover.c").read_text()

subprocess.run(
    ["python3", "tests/async/test_player_load_items.py"],
    cwd=ROOT,
    check=True,
    timeout=30,
)

for contract in (
    "FROM player_pets WHERE owner_pid=",
    "FROM player_pet_items ppi",
    "player_pet_item_affects",
    "player_pet_item_extra_descr",
    "load_pets(connection",
    "PLAYER_LOAD_SESSION03_COMPONENTS",
):
    assert contract in REPOSITORY
assert REPOSITORY.count("load_pets(connection") == 1

for contract in (
    "player_load_pets_stage",
    "player_load_pets_discard",
    "item_ownership_runtime_hydrate_batch",
    "player_load_pets_commit",
):
    assert contract in MATERIALIZE
for contract in (
    "player_load_item_graph_materialize",
    "read_mobile",
    "setup_pet",
    "add_follower",
    "char_to_room",
):
    assert contract in PETS

assert "request.include_pets = false" in COPYOVER
assert "player_load_pets_place(ch)" in NANNY
assert "sql_load_player_pets(ch)" not in NANNY
assert "DELETE FROM player_pets WHERE owner_pid" not in REPOSITORY

print("batched pet graph hydration contracts passed")
