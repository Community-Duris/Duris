#!/usr/bin/env python3
"""Guard the required level-cap singleton across fresh and upgraded databases."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
bootstrap = (ROOT / "migrations/bootstrap_multithread_safe.sql").read_text()
migration = (ROOT / "migrations/immutable/0005_level_cap_singleton.sql").read_text()
verifier = (ROOT / "migrations/immutable/0005_level_cap_singleton.sh").read_text()
runtime = (ROOT / "migrations/verify_runtime_compatibility.sh").read_text()

singleton = "VALUES (1, 0, 2, 56, NOW())"
assert singleton in bootstrap
assert "WHERE NOT EXISTS (SELECT 1 FROM level_cap)" in migration
assert singleton not in migration
for token in (
    "id=1",
    "most_frags>=0",
    "racewar_leader BETWEEN 0 AND 4",
    "level BETWEEN 1 AND 56",
    "next_update IS NOT NULL",
    "SELECT COUNT(*) FROM level_cap",
):
    assert token in verifier
assert "level_cap" in runtime and "level-cap singleton" in runtime

print("level-cap singleton bootstrap, migration, and runtime contracts passed")
