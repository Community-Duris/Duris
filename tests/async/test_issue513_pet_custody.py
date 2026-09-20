#!/usr/bin/env python3
"""Regression contracts for issue #513 stale held-pet custody rows."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
LOAD_PETS = (ROOT / "src/player/player_load_pets.c").read_text()
LOAD_REPOSITORY = (ROOT / "src/player/player_load_repository.c").read_text()
REPOSITORY = (ROOT / "src/player/player_snapshot_repository.c").read_text()
REPAIR = (ROOT / "migrations/repair_stale_pet_custody.sh").read_text()
UNIT_TEST = (ROOT / "tests/async/test_player_load_items.py").read_text()


# The runtime regression lives in the existing native materialization harness;
# pin the exact stale-held case here so it cannot be removed while the harness
# continues to cover active and held room mismatches.
assert "snapshot.pets[0].hold_reason = pet_hold_reason::custody_pending;" in UNIT_TEST
assert "owner.pc.held_pets->pets[0].room_vnum == 122" in UNIT_TEST
assert "snapshot.room_vnum != result.snapshot.room_vnum" not in LOAD_PETS
assert "pet.room_vnum > 0" not in LOAD_PETS
assert "values[9] <= 0" not in LOAD_REPOSITORY

# A positive owner revision is only historical evidence. Retention requires a
# live or quarantined pet-owned custody row, and refreshes the projection room.
assert "FROM item_current_owner own" in REPOSITORY
assert "item_custody_state::active" in REPOSITORY
assert "item_custody_state::quarantined" in REPOSITORY
assert "pet_hold_reason::custody_pending" in REPOSITORY
assert ",room_vnum=" in REPOSITORY

# Production cleanup is exact-row, backup-first, and target-acknowledged.
for contract in (
    "PET_CUSTODY_REPAIR_OWNER_PID",
    "PET_CUSTODY_REPAIR_UID",
    "PET_CUSTODY_REPAIR_BACKUP",
    "PET_CUSTODY_REPAIR_PRODUCTION_ACK",
    "START TRANSACTION;",
    "AND pp.hold_reason=6",
    "AND pp.room_vnum=$room_before",
    "SELECT ROW_COUNT();",
):
    assert contract in REPAIR, contract

print("issue 513 stale held-pet custody contracts passed")
