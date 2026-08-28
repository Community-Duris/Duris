#!/usr/bin/env python3
"""Source contracts for Redis and player-checkpoint module ownership."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
REDIS_HEADER = (ROOT / "src/redis.h").read_text(encoding="ascii")
REDIS_SOURCE = (ROOT / "src/redis.c").read_text(encoding="ascii")
CHECKPOINT_HEADER = (ROOT / "src/persistence_checkpoint.h").read_text(encoding="ascii")
CHECKPOINT_SOURCE = (ROOT / "src/persistence_checkpoint.c").read_text(encoding="ascii")
MAKEFILE = (ROOT / "src/Makefile").read_text(encoding="ascii")


CHECKPOINT_API = (
    "mark_player_dirty",
    "mark_player_dirty_components",
    "flush_dirty_players",
    "get_dirty_player_count",
    "persistence_dirty_save_snapshot_copy",
    "event_flush_dirty_players",
)

for symbol in CHECKPOINT_API:
    assert symbol in CHECKPOINT_HEADER
    assert symbol in CHECKPOINT_SOURCE
    assert symbol not in REDIS_HEADER
    assert symbol not in REDIS_SOURCE

for include in ("persistence_observability.h", "player_revision_state.h"):
    assert include in CHECKPOINT_HEADER
    assert include not in REDIS_HEADER

assert "player_save_pipeline.h" in CHECKPOINT_SOURCE
assert "player_save_worker.h" in CHECKPOINT_SOURCE
assert "player_save_pipeline.h" not in REDIS_SOURCE
assert "player_save_worker.h" not in REDIS_SOURCE
assert "persistence_checkpoint.o" in MAKEFILE

checkpoint_only_callers = (
    "actinf.c",
    "auction_houses.c",
    "fight.c",
    "limits.c",
    "memorize.c",
    "nexus_stones.c",
    "utility.c",
    "world_quest.c",
)
for filename in checkpoint_only_callers:
    source = (ROOT / "src" / filename).read_text(encoding="utf-8")
    assert '#include "persistence_checkpoint.h"' in source
    assert '#include "redis.h"' not in source

redis_includers = []
for source_path in (ROOT / "src").rglob("*.[ch]"):
    if '#include "redis.h"' in source_path.read_text(encoding="utf-8"):
        redis_includers.append(source_path)
assert len(redis_includers) <= 22

print("Redis and player-checkpoint module boundaries passed")
