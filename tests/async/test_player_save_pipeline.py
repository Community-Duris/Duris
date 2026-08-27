#!/usr/bin/env python3
"""Runtime revision-state and source contracts for nonterminal player-save cutover."""

import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
PIPELINE = (ROOT / "src/player_save_pipeline.c").read_text()
HEADER = (ROOT / "src/player_save_pipeline.h").read_text()
FILES = (ROOT / "src/files.c").read_text()
ACTOTH = (ROOT / "src/actoth.c").read_text()
REDIS = (ROOT / "src/redis.c").read_text()
EVENTS = (ROOT / "src/new_events.c").read_text()
COMM = (ROOT / "src/comm.c").read_text()
WORKER = (ROOT / "src/player_save_worker.c").read_text()
SQL_PLAYER = (ROOT / "src/sql_player.c").read_text()


def section(text: str, start: str, end: str) -> str:
    first = text.index(start)
    return text[first : text.index(end, first)]


HARNESS = r'''
#include "player_revision_state.h"
#include <cassert>

int main()
{
    player_revision_reset_for_tests();
    assert(player_revision_hydrate(41, 7));
    assert(player_revision_dirty_count() == 0);
    player_revision_t revision = 0;
    assert(player_revision_mark(41, PLAYER_COMPONENT_STATUS, &revision));
    assert(revision == 8);
    assert(player_revision_dirty_count() == 1);
    player_revision_t queued = 0;
    player_component_mask_t components = 0;
    assert(player_revision_queue(41, &queued, &components));
    assert(queued == 8 && components == PLAYER_COMPONENT_STATUS);
    assert(player_revision_dirty_count() == 1);
    assert(player_revision_begin_inflight(41, queued, components));
    assert(player_revision_acknowledge(41, queued, components));
    assert(player_revision_dirty_count() == 0);
    return 0;
}
'''

with tempfile.TemporaryDirectory(prefix="duris-player-save-pipeline-") as temp_dir:
    source = Path(temp_dir) / "revision_test.cpp"
    binary = Path(temp_dir) / "revision_test"
    source.write_text(HARNESS)
    subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Wpedantic",
            "-Werror",
            "-Isrc",
            str(source),
            "src/player_revision_state.c",
            "-o",
            str(binary),
        ],
        cwd=ROOT,
        check=True,
        capture_output=True,
        text=True,
    )
    subprocess.run([str(binary)], check=True)
print("[PASS] unchanged, dirty, queued, inflight, and exact ACK counts are deterministic")

for contract in (
    "PLAYER_SAVE_PIPELINE_MAX_SNAPSHOTS = 256",
    "PLAYER_SAVE_PIPELINE_MAX_BYTES = 32 * 1024 * 1024",
    "PLAYER_SAVE_PIPELINE_PULSE_BUDGET = 32",
    "pending_append",
    "durable_ready",
    "retained_bytes",
    "high_water_snapshots",
    "append_failures",
    "overloads",
):
    assert contract in HEADER
dispatcher = section(PIPELINE, "void dispatcher_main()", "player_save_pipeline_result enqueue_snapshot")
assert dispatcher.index("player_save_journal_append(snapshot)") < dispatcher.index(
    "durable_ready.push_back"
)
assert "player_save_journal_replay(player_snapshot_repository_apply_from_pool" in dispatcher
assert "sleep_for(std::chrono::milliseconds(100))" in dispatcher
print("[PASS] bounded dispatcher journals before worker eligibility and retains append failures")

checkpoint = section(
    PIPELINE,
    "player_save_pipeline_result player_save_pipeline_checkpoint_dirty",
    "player_save_pipeline_result player_save_pipeline_request",
)
assert checkpoint.index("if (!revision.dirty_components)") < checkpoint.index(
    "player_revision_queue"
)
assert checkpoint.index("player_revision_queue") < checkpoint.index("player_snapshot_capture")
for forbidden in ("player_save_journal_", "sql_", "redis_", "fopen", "open(", "write("):
    assert forbidden not in checkpoint
pulse = section(PIPELINE, "void player_save_pipeline_pulse", "player_save_pipeline_health")
assert "player_save_worker_pulse" in pulse
assert "player_save_worker_submit_retained" in pulse
for forbidden in ("player_save_journal_", "sql_", "redis_", "fopen", "open(", "write("):
    assert forbidden not in pulse
assert "if (append && !acknowledge)" in WORKER
print("[PASS] simulation-thread checkpoint and completion paths contain no external I/O")

write_character = section(FILES, "int writeCharacter(P_char ch", "int deleteCharacter")
branch = write_character.index("player_save_pipeline_is_nonterminal_type")
assert "!sql_in_transaction()" in write_character[:branch]
for legacy in (
    "sql_save_player_shapechanges",
    "sql_update_money",
    "unequip_char",
    "all_affects(ch, FALSE)",
    "sql_save_player(ch",
):
    assert branch < write_character.index(legacy)
silent = section(ACTOTH, "bool do_save_silent(P_char ch", "void do_save(P_char")
assert silent.index("player_save_pipeline_is_nonterminal_type") < silent.index("fopen(tmp_buf")
assert silent.index("player_save_pipeline_request") < silent.index("writeCharacter(ch")
print("[PASS] ordinary direct and manual saves branch before legacy mutation and I/O")

legacy_start = SQL_PLAYER.rindex("bool sql_save_player(P_char ch")
legacy_save = SQL_PLAYER[legacy_start : SQL_PLAYER.index("bool sql_save_player_status", legacy_start)]
assert "player_revision_mark(GET_PID(ch), PLAYER_CHECKPOINT_COMPONENT_ALL" in legacy_save
assert "SET save_revision=%llu WHERE pid=%d AND save_revision<%llu" in legacy_save
assert "mysql_affected_rows(DB) != 1" in legacy_save
assert legacy_save.index("sql_save_player_shapechanges") < legacy_save.index("SET save_revision")
assert legacy_save.index("SET save_revision") < legacy_save.index("if (own_txn)")
print("[PASS] transactional compatibility saves fence every older immutable revision")

mark = section(REDIS, "void mark_player_dirty(int pid)", "void flush_dirty_players(void)")
flush = section(REDIS, "void flush_dirty_players(void)", "int get_dirty_player_count(void)")
assert "player_save_pipeline_mark" in mark
assert "player_save_pipeline_checkpoint_dirty" in flush
for retired in ("redis_command", "redis_reconnect", "sql_save_player", "fork("):
    assert retired not in mark and retired not in flush
event_init = section(EVENTS, "void ne_init_events", "void zone_purge")
assert '"dirty-player-checkpoint", event_flush_dirty_players' in event_init
assert "nevent_periodic_policy::fixed_delay, true" in event_init
print("[PASS] autosave durability is local and the Redis dirty-save fork is retired")

assert 'getenv("PLAYER_SAVE_JOURNAL_DIR")' in COMM
assert "player_save_pipeline_init(journal_directory)" in COMM
assert "player_save_pipeline_pulse();" in COMM
assert "player_save_pipeline_shutdown();" in COMM
assert "PLAYER_SAVE_JOURNAL_DIR" in (ROOT / ".env.example").read_text()
print("[PASS] production lifecycle and explicit absolute journal configuration are wired")

terminal = section(
    PIPELINE,
    "player_save_terminal_result player_save_pipeline_terminal",
    "void player_save_pipeline_pulse",
)
assert "std::array<terminal_fence, PLAYER_SAVE_PIPELINE_MAX_SNAPSHOTS>" in PIPELINE
assert "fence->revision == durable_ready.back().revision" in dispatcher
assert "fence->revision == completions[index].revision" in pulse
assert "completions[index].durable_revision >= fence->revision" in pulse
assert "std::chrono::steady_clock::now()" in terminal
assert "current.unacknowledged_components == PLAYER_CHECKPOINT_COMPONENT_ALL" in terminal
assert "revision = current.current_revision" in terminal
assert "snapshot_is_journaled_locked(current)" in terminal
assert terminal.index("if (fence->acknowledged)") < terminal.index("if (allow_journal_handoff")
assert "*fence = {};" in terminal
assert "++health.terminal_timeouts" in terminal
mark_body = section(
    PIPELINE, "bool player_save_pipeline_mark", "player_save_pipeline_result player_save_pipeline_checkpoint_dirty"
)
assert "if (!accepting)" in mark_body
assert "fence->revision = revision" in mark_body
assert "fence->journaled = false" in mark_body
drain = section(PIPELINE, "bool player_save_pipeline_drain", "player_save_pipeline_health")
assert "pending_append.empty() && !append_inflight" in drain
assert "std::chrono::steady_clock::now()" in drain
assert "++health.drain_failures" in drain
print("[PASS] terminal fences, exact durability outcomes, retry tracking, and bounded drain are wired")

print("nonterminal player save pipeline contracts passed")
