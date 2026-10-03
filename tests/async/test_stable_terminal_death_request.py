#!/usr/bin/env python3
"""Executable checks for immutable terminal-death request retention and retry."""

from pathlib import Path
import shutil
import subprocess
import tempfile
import os

from _paths import SRC

ROOT = Path(__file__).resolve().parents[2]
PIPELINE = (SRC / "player_save_pipeline.c").read_text()
HEADER = (SRC / "player_save_pipeline.h").read_text()
FIGHT = (SRC / "combat" / "fight.c").read_text()
REVISION = (SRC / "player_revision_state.c").read_text()
SANITIZER_FLAGS = (["-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie"]
                   if os.environ.get("DURIS_TEST_SANITIZERS") == "1" else [])


def section(text: str, start: str, end: str) -> str:
    first = text.index(start)
    return text[first : text.index(end, first)]


# Keep the fight-level ordering explicit: pending work is resumed before any
# new random operation or temporary wallet object can be created.
disposition = section(
    FIGHT,
    "static bool save_disputed_death_disposition(P_char ch, uint64_t corpse_uid)",
    "struct death_extract_retry_context",
)
assert disposition.index("player_save_pipeline_terminal_death_resume") < disposition.index(
    "critical_operation_id_generate"
)
assert disposition.index("player_save_pipeline_terminal_death_resume") < disposition.index(
    "create_money("
)
assert "player_save_terminal_result::not_pending" in disposition
assert "terminal_death_resume" in HEADER

mark = section(
    REVISION,
    "bool player_revision_mark(int pid",
    "bool player_revision_queue(int pid",
)
assert "state->terminal_death_pinned" in mark
capture = section(
    PIPELINE,
    "bool retain_and_enqueue_death_snapshot",
    "bool requeue_pinned_death(int pid, uint64_t corpse_uid);",
)
for token in (
    "player_snapshot_is_death_request_schema(snapshot.schema_version)",
    "snapshot.death->operation_id.bytes",
    "snapshot.death->wallet_pile_uid",
    "snapshot.death->corpse.front().object_uid",
    "fence->death_snapshot.emplace",
    "retained_bytes += snapshot_bytes * 2",
):
    assert token in capture
ack = section(
    PIPELINE,
    "bool acknowledge_terminal_fence_completion_locked",
    "/** Pump the pipeline",
)
death_ack = ack[ack.index("if (fence->death_pinned)") : ack.index("if ((applied", ack.index("if (fence->death_pinned)"))]
assert "completion.outcome == player_save_apply_outcome::stale_revision" not in death_ack
assert "completion.durable_revision != fence->revision" in death_ack
assert "completion.components != fence->death_snapshot->components" in death_ack
await_body = section(
    PIPELINE,
    "player_save_terminal_result await_terminal_fence(int pid, player_revision_t revision,\n\t\t\t\t\t\t uint64_t timeout_msec, bool allow_journal_handoff)\n{",
    "void player_save_pipeline_pulse",
)
assert "!fence->death_pinned && allow_journal_handoff" in await_body

# Compile the production fence/request functions into a small deterministic
# harness. This exercises the real copy, retained-byte accounting, revision
# pin, exact requeue, and completion gate without DB or worker infrastructure.
fence_struct = section(
    PIPELINE,
    "struct terminal_fence\n{",
    "std::array<terminal_fence, PLAYER_SAVE_PIPELINE_MAX_SNAPSHOTS>",
)
find_fence = section(
    PIPELINE,
    "terminal_fence *find_terminal_fence_locked(int pid)",
    "terminal_fence *allocate_terminal_fence_locked(int pid)",
)
retained_scan = section(
    PIPELINE,
    "bool snapshot_is_retained_locked(int pid, player_revision_t revision)",
    "/** Check retained queues for any snapshot belonging to a player. */",
)
accounting = section(
    PIPELINE,
    "size_t pinned_death_count_locked();",
    "/** Replay the journal, then append queued snapshots",
)
retain = section(
    PIPELINE,
    "bool retain_and_enqueue_death_snapshot(player_snapshot snapshot, uint64_t corpse_uid,",
    "bool requeue_pinned_death(int pid, uint64_t corpse_uid);",
)
requeue = section(
    PIPELINE,
    "/** Requeue only the pinned immutable bytes after the worker has released a failed slot. */",
    "bool acknowledge_terminal_fence_completion_locked",
)
ack_gate = section(
    PIPELINE,
    "bool acknowledge_terminal_fence_completion_locked",
    "/** Pump the pipeline",
)

HARNESS = r'''#include "player/player_save_pipeline.h"
#include "player/player_save_worker.h"
#include "player/player_snapshot.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <optional>

// Extending the public result type must not renumber the existing outcomes.
static_assert(static_cast<unsigned>(player_save_terminal_result::database_acknowledged) == 0);
static_assert(static_cast<unsigned>(player_save_terminal_result::journal_durable) == 1);
static_assert(static_cast<unsigned>(player_save_terminal_result::invalid) == 2);
static_assert(static_cast<unsigned>(player_save_terminal_result::unavailable) == 3);
static_assert(static_cast<unsigned>(player_save_terminal_result::timed_out) == 4);

namespace {
std::deque<player_snapshot> pending_append;
std::deque<player_snapshot> durable_ready;
player_save_pipeline_health health = {};
std::mutex pipeline_mutex;
std::condition_variable append_available;
size_t retained_bytes = 0;
bool accepting = true;
bool stop_requested = false;
bool append_inflight = false;
int append_inflight_pid = 0;
player_revision_t append_inflight_revision = 0;
bool worker_pending = false;
int capture_count = 0;
bool player_save_worker_pid_pending(int pid) { (void)pid; return worker_pending; }
void make_snapshot(player_snapshot &snapshot, int pid, player_revision_t revision,
                   const critical_operation_id &operation, uint64_t corpse_uid,
                   uint64_t wallet_uid, size_t bytes)
{
    ++capture_count;
    snapshot.schema_version = PLAYER_SNAPSHOT_DEATH_SCHEMA_VERSION;
    snapshot.pid = pid;
    snapshot.revision = revision;
    snapshot.components = PLAYER_CHECKPOINT_COMPONENT_ALL;
    snapshot.encoded_size_bound = bytes;
    snapshot.death.emplace();
    snapshot.death->operation_id = operation;
    snapshot.death->wallet_pile_uid = wallet_uid;
    player_item_snapshot corpse = {};
    corpse.object_uid = corpse_uid;
    snapshot.death->corpse.push_back(corpse);
}
'''

HARNESS += fence_struct + "\n"
HARNESS += "std::array<terminal_fence, PLAYER_SAVE_PIPELINE_MAX_SNAPSHOTS> terminal_fences = {};\n"
literal_start = PIPELINE.index("struct literal_inventory_checkpoint")
literal_array_end = PIPELINE.index("literal_inventory_checkpoints = {};", literal_start)
literal_array_end += len("literal_inventory_checkpoints = {};")
# Extract only actual storage; a backend guard around the next declaration
# must not leak an unmatched preprocessor block into this component harness.
HARNESS += PIPELINE[literal_start:literal_array_end] + "\n"
HARNESS += section(PIPELINE, "literal_inventory_checkpoint *find_literal_inventory_locked(int pid)",
                   "bool literal_inventory_blob(")
HARNESS += find_fence + accounting + retained_scan + retain + requeue + ack_gate
HARNESS += r'''
}

int main()
{
    constexpr int pid = 51;
    constexpr uint64_t corpse_uid = 61001;
    constexpr uint64_t wallet_uid = 61002;
    constexpr size_t snapshot_bytes = 256;
    health.initialized = true;
    assert(player_revision_hydrate(pid, 10));
    player_revision_t revision = 0;
    assert(player_revision_mark(pid, PLAYER_CHECKPOINT_COMPONENT_ALL, &revision));
    assert(player_revision_pin_terminal_death(pid, revision));
    player_revision_t queued_revision = 0;
    player_component_mask_t components = 0;
    assert(player_revision_queue(pid, &queued_revision, &components));
    assert(queued_revision == revision && components == PLAYER_CHECKPOINT_COMPONENT_ALL);

    terminal_fence &fence = terminal_fences[0];
    fence.pid = pid;
    fence.revision = revision;
    fence.death_pinned = true;
    critical_operation_id operation = {};
    operation.bytes[0] = 0xA7;
    operation.bytes[15] = 0x4C;
    player_snapshot captured;
    make_snapshot(captured, pid, revision, operation, corpse_uid, wallet_uid, snapshot_bytes);
    captured.schema_version = PLAYER_SNAPSHOT_DEATH_SPELL_RECEIPT_SCHEMA_VERSION;
    player_spell_effect_receipt_snapshot spell_receipt = {};
    spell_receipt.operation_id.bytes[0] = 77;
    spell_receipt.effect_id = 6;
    captured.spell_effect_receipts.push_back(spell_receipt);
    auto &literal = literal_inventory_checkpoints[0];
    literal.token = {pid, 71, corpse_uid, 1};
    literal.held = true;
    player_revision_snapshot held_before = {}, held_after = {};
    assert(player_revision_snapshot_copy(pid, &held_before));
    const auto captures_before = health.captured;
    assert(!retain_and_enqueue_death_snapshot(captured, corpse_uid, wallet_uid, operation));
    assert(pending_append.empty() && !fence.death_snapshot && retained_bytes == 0 &&
           health.captured == captures_before && literal.held);
    assert(player_revision_snapshot_copy(pid, &held_after));
    assert(held_before.current_revision == held_after.current_revision &&
           held_before.queued_revision == held_after.queued_revision &&
           held_before.queued_components == held_after.queued_components);
    literal.token.pid = pid + 1;
    assert(!find_literal_inventory_locked(pid) && find_literal_inventory_locked(pid + 1) == &literal);
    assert(!retain_and_enqueue_death_snapshot(captured, corpse_uid + 1,
                                              wallet_uid, operation));
    assert(pending_append.empty() && !fence.death_snapshot && retained_bytes == 0);
    assert(retain_and_enqueue_death_snapshot(captured, corpse_uid, wallet_uid, operation));
    assert(literal.held && literal.token.pid == pid + 1);
    literal_inventory_checkpoints.fill({});
    assert(pending_append.size() == 1);
    assert(fence.death_snapshot.has_value());
    assert(fence.death_snapshot->schema_version == PLAYER_SNAPSHOT_DEATH_SPELL_RECEIPT_SCHEMA_VERSION);
    assert(fence.revision == revision && fence.corpse_uid == corpse_uid);
    assert(fence.operation_id.bytes == operation.bytes && fence.wallet_pile_uid == wallet_uid);
    assert(retained_bytes == snapshot_bytes * 2);
    assert(health.retained_bytes == retained_bytes && health.high_water_snapshots >= 2);
    assert(!player_revision_mark(pid, PLAYER_COMPONENT_STATUS, nullptr));

    // Journal-only durability and an unrelated newer durable revision do not
    // authorize release of this request.
    fence.journaled = true;
    player_save_completion stale = {};
    stale.pid = pid;
    stale.revision = revision;
    stale.components = components;
    stale.outcome = player_save_apply_outcome::stale_revision;
    stale.durable_revision = revision + 3;
    assert(!acknowledge_terminal_fence_completion_locked(stale));
    assert(!fence.acknowledged && fence.death_pinned);
    player_save_completion unrelated = stale;
    unrelated.revision = revision + 1;
    unrelated.outcome = player_save_apply_outcome::applied;
    unrelated.durable_revision = revision + 1;
    assert(!acknowledge_terminal_fence_completion_locked(unrelated));
    assert(!fence.acknowledged && fence.death_pinned);
    player_save_completion wrong_components = {};
    wrong_components.pid = pid;
    wrong_components.revision = revision;
    wrong_components.components = PLAYER_COMPONENT_STATUS;
    wrong_components.outcome = player_save_apply_outcome::applied;
    wrong_components.durable_revision = revision;
    assert(!acknowledge_terminal_fence_completion_locked(wrong_components));
    assert(!fence.acknowledged && fence.death_pinned);
    assert(!player_revision_mark(pid, PLAYER_COMPONENT_STATUS, nullptr));

    auto fail_attempt = [&](player_save_apply_outcome outcome) {
        assert(pending_append.size() == 1);
        player_snapshot submitted = std::move(pending_append.front());
        pending_append.pop_front();
        retained_bytes -= submitted.encoded_size_bound;
        assert(submitted.revision == revision);
        assert(submitted.death->operation_id.bytes == operation.bytes);
        assert(submitted.death->wallet_pile_uid == wallet_uid);
        assert(submitted.death->corpse.front().object_uid == corpse_uid);
        assert(submitted.spell_effect_receipts.size() == 1);
        assert(submitted.spell_effect_receipts[0].operation_id.bytes == spell_receipt.operation_id.bytes);
        assert(submitted.spell_effect_receipts[0].effect_id == 6);
        assert(player_revision_begin_inflight(pid, revision, components));
        assert(player_revision_fail_inflight(pid, revision, components));
        player_save_completion failure = {};
        failure.pid = pid;
        failure.revision = revision;
        failure.components = components;
        failure.outcome = outcome;
        failure.durable_revision = 0;
        assert(!acknowledge_terminal_fence_completion_locked(failure));
        worker_pending = false;
        assert(requeue_pinned_death(pid, corpse_uid));
        assert(pending_append.size() == 1);
        const player_snapshot &retried = pending_append.front();
        assert(retried.revision == revision);
        assert(retried.schema_version == PLAYER_SNAPSHOT_DEATH_SPELL_RECEIPT_SCHEMA_VERSION);
        assert(retried.death->operation_id.bytes == operation.bytes);
        assert(retried.death->wallet_pile_uid == wallet_uid);
        assert(retried.death->corpse.front().object_uid == corpse_uid);
        assert(capture_count == 1);
    };

    fail_attempt(player_save_apply_outcome::terminal_failure);
    fail_attempt(player_save_apply_outcome::ambiguous_commit);
    assert(health.terminal_death_requeues == 2);

    // Only the exact successful database completion opens the ACK gate.
    player_snapshot submitted = std::move(pending_append.front());
    pending_append.pop_front();
    retained_bytes -= submitted.encoded_size_bound;
    assert(player_revision_begin_inflight(pid, revision, components));
    assert(player_revision_acknowledge(pid, revision, components));
    assert(death_snapshot_identity_matches(fence));
    player_revision_snapshot after_ack = {};
    assert(player_revision_snapshot_copy(pid, &after_ack));
    assert(after_ack.current_revision == revision);
    player_save_completion applied = {};
    applied.pid = pid;
    applied.revision = revision;
    applied.components = components;
    applied.outcome = player_save_apply_outcome::applied;
    applied.durable_revision = revision;
    assert(acknowledge_terminal_fence_completion_locked(applied));
    assert(fence.acknowledged && fence.death_pinned);
    assert(!player_revision_mark(pid, PLAYER_COMPONENT_STATUS, nullptr));
    clear_terminal_fence_locked(fence);
    assert(fence.pid == 0 && retained_bytes == 0);
    assert(player_revision_mark(pid, PLAYER_COMPONENT_STATUS, nullptr));

    // Capacity refusal is fail closed and reset cleanup releases only its own pin.
    constexpr int capacity_pid = 52;
    assert(player_revision_hydrate(capacity_pid, 20));
    player_revision_t capacity_revision = 0;
    assert(player_revision_mark(capacity_pid, PLAYER_CHECKPOINT_COMPONENT_ALL,
                                &capacity_revision));
    assert(player_revision_pin_terminal_death(capacity_pid, capacity_revision));
    terminal_fence &capacity_fence = terminal_fences[1];
    capacity_fence.pid = capacity_pid;
    capacity_fence.revision = capacity_revision;
    capacity_fence.death_pinned = true;
    assert(player_revision_queue(capacity_pid, &queued_revision, &components));
    player_snapshot too_large;
    critical_operation_id capacity_operation = {};
    make_snapshot(too_large, capacity_pid, capacity_revision, capacity_operation,
                  62001, 62002, 128);
    retained_bytes = PLAYER_SAVE_PIPELINE_MAX_BYTES - 100;
    const uint64_t overloads_before = health.overloads;
    assert(!retain_and_enqueue_death_snapshot(too_large, 62001, 62002,
                                              capacity_operation));
    assert(health.overloads == overloads_before + 1);
    assert(pending_append.empty() && !capacity_fence.death_snapshot);
    clear_terminal_fence_locked(capacity_fence);
    assert(player_revision_mark(capacity_pid, PLAYER_COMPONENT_STATUS, nullptr));
    retained_bytes = 0;
    health.retained_bytes = 0;
    return 0;
}
'''

compiler = shutil.which("g++-14") or shutil.which("g++")
assert compiler, "g++ compiler is required for the executable regression"
with tempfile.TemporaryDirectory(prefix="duris-stable-death-request-") as temp_dir:
    source = Path(temp_dir) / "stable_death.cpp"
    binary = Path(temp_dir) / "stable_death"
    source.write_text(HARNESS)
    subprocess.run(
        [
            compiler,
            *SANITIZER_FLAGS,
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Wpedantic",
            "-Werror",
            "-Isrc",
            str(source),
            "src/player/player_revision_state.c",
            "-pthread",
            "-o",
            str(binary),
        ],
        cwd=ROOT,
        check=True,
    )
    subprocess.run([str(binary)], cwd=ROOT, check=True, timeout=10)

print("[PASS] format-13 death request keeps revision, operation, corpse, wallet and spell receipt identity across failure/ambiguous retries")
print("[PASS] journal/stale completions hold; exact database ACK releases the pinned request")
print("[PASS] pinned memory is byte-bounded and capacity refusal can be safely reset")
print("[PASS] held literal lease rejects retained death without queue/byte/revision changes; unrelated PID remains independent")
