#!/usr/bin/env python3
"""Guard the fresh NPC corpse adoption-to-raise continuation."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / "src/world/handler.c").read_text()

start = source.index("bool persistence_defer_corpse_raise(")
body = source[start:source.index("bool persistence_defer_corpse_resurrection(", start)]
completion_start = source.index("void complete_world_corpse_raise_admission(")
completion = source[completion_start:start]

assert "item_ownership_runtime_lookup(corpse->obj_uid, &existing_root)" in body
assert "if (!registered)" in body
assert "corpse_raise_admissions.emplace(" in body
assert "item_movement_transaction_submit(" in body
assert "caster, corpse, nullptr, room, room" in body
assert "complete_world_corpse_raise_admission" in body
assert "has_durable_contents" not in body
assert "!corpse->obj_uid" not in body

assert "if (!committed" in completion
assert "find_live_world_corpse(key)" in completion
assert "persistence_defer_corpse_raise(corpse, caster, follower" in completion
assert "context.globe" in completion
assert "extract_char(follower)" in completion

print("fresh NPC corpse graphs adopt into room custody and automatically continue raising PASS")
