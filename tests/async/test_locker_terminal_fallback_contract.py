#!/usr/bin/env python3
"""Fail-closed contracts for every terminal locker sync fallback path."""
from _paths import SRC
from pathlib import Path

source = (SRC / "locker_async.c").read_text()

helper_start = source.index("static int locker_sync_fallback_durable")
helper_end = source.index("\n}\n", helper_start) + 3
helper = source[helper_start:helper_end]
assert "sql_save_locker" in helper
assert "writeCharacter" in helper
assert helper.index("sql_save_locker") < helper.index("writeCharacter")
assert "return 1;" in helper
assert "return 0;" in helper

start = source.index("static int start_one_snapshot")
end = source.index("\n}\n", start) + 3
body = source[start:end]

# Snapshot-build failure and job-queue-full must share the same durability gate.
assert body.count("locker_sync_fallback_durable(s, chLocker)") == 2
assert body.count("if (s->terminal && durable_ok)") == 2
assert body.count('"terminal_not_durable"') == 2
assert body.count("s->state = LCHK_DIRTY;") >= 2

# Neither failure branch may discard the recovery fence unconditionally.
snapshot_failed = body[body.index("if (!sql)"):body.index("if (!job_push")]
job_full = body[body.index("if (!job_push"):]
for label, branch in (("snapshot_failed", snapshot_failed), ("job_queue_full", job_full)):
    assert "if (s->terminal && durable_ok)" in branch, label
    assert "else if (s->terminal)" in branch, label
    assert branch.index("if (s->terminal && durable_ok)") < branch.index("extract_char(chLocker)"), label

print("locker terminal fallback fail-closed checks passed")
