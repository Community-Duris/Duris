#!/usr/bin/env python3
"""Revisioned dirty-player checkpoint contracts."""

from pathlib import Path

text = (Path(__file__).resolve().parents[2] / "src/redis.c").read_text()

flush_start = text.index("void flush_dirty_players(void)")
flush_end = text.index("int get_dirty_player_count(void)", flush_start)
flush = text[flush_start:flush_end]

mark_start = text.index("void mark_player_dirty(int pid)")
mark = text[mark_start:flush_start]

checks = {
    "dirty marks are local and cumulative": (
        "mark_player_dirty_components(pid, PLAYER_CHECKPOINT_COMPONENT_ALL)" in mark
        and "player_save_pipeline_mark(pid, components)" in mark
    ),
    "autosave scans online PCs": "for (P_char ch = character_list" in flush,
    "autosave captures only dirty state": "player_save_pipeline_checkpoint_dirty" in flush,
    "Redis is not a durability dependency": "redis_" not in flush,
    "database work is absent": "sql_" not in flush,
    "forked player flush is absent": "fork(" not in flush and "waitpid" not in flush,
}

for label, passed in checks.items():
    print(f"[{'PASS' if passed else 'FAIL'}] {label}")
assert all(checks.values())
print("revisioned dirty checkpoint semantics look correct")
