#!/usr/bin/env python3
"""Dirty-player inflight retry contracts."""

from pathlib import Path

text = (Path(__file__).resolve().parents[2] / "src/redis.c").read_text()

flush_start = text.index("void flush_dirty_players(void)")
flush_end = text.index("int get_dirty_player_count(void)", flush_start)
flush = text[flush_start:flush_end]

poll = flush.index("redis_poll_child(")
success = flush.index("child_result == REDIS_CHILD_SUCCEEDED")
clear = flush.index('redis_command(redis_ctx, "DEL %s", inflight_key)', success)
failure = flush.index("child_result == REDIS_CHILD_FAILED")
restore_failure = flush.index("redis_restore_dirty_snapshot(inflight_key);", failure)
preflight_restore = flush.index("if (!redis_restore_dirty_snapshot(inflight_key))")
rename = flush.index('"RENAME mud:dirty_players %s"')
fork_failure = flush[flush.index("if (pid < 0)"):flush.index("if (pid == 0)")]

checks = {
    "child poll precedes acknowledgment": poll < success < clear,
    "only successful child deletes inflight": clear < failure,
    "failed child restores inflight": failure < restore_failure,
    "stale inflight merges before rename": preflight_restore < rename,
    "fork failure restores instead of saving synchronously": (
        "redis_restore_dirty_snapshot(inflight_key);" in fork_failure
        and "sql_save_player" not in fork_failure
    ),
    "child exit represents aggregate save status": "_exit(all_ok ? 0 : 1);" in flush,
    "child has its own hard deadline": "alarm(REDIS_DIRTY_CHILD_TIMEOUT_SEC);" in flush,
}

for label, passed in checks.items():
    print(f"[{'PASS' if passed else 'FAIL'}] {label}")
assert all(checks.values())
print("dirty flush retry semantics look correct")
