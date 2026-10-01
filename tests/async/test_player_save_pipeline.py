#!/usr/bin/env python3
"""Runtime revision-state and source contracts for nonterminal player-save cutover."""

from _paths import SRC, rel, extract_function
import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
PIPELINE = (SRC / "player_save_pipeline.c").read_text()
HEADER = (SRC / "player_save_pipeline.h").read_text()
FILES = (SRC / "files.c").read_text()
ACTOTH = (SRC / "actoth.c").read_text()
CHECKPOINT = (SRC / "persistence_checkpoint.c").read_text()
EVENTS = (SRC / "new_events.c").read_text()
COMM = (SRC / "comm.c").read_text()
NANNY = (SRC / "nanny.c").read_text()
WORKER = (SRC / "player_save_worker.c").read_text()
SQL_PLAYER = (SRC / "sql_player.c").read_text()


def section(text: str, start: str, end: str) -> str:
    """Extract a production function region for contract checks and compiled harnesses."""
    first = text.index(start)
    return text[first : text.index(end, first)]


HARNESS = r'''
#include "player/player_revision_state.h"
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
    assert(player_revision_pin_terminal_death(41, queued));
    assert(!player_revision_hydrate(41, queued + 1));
    assert(!player_revision_mark(41, PLAYER_COMPONENT_STATUS, &revision));
    assert(player_revision_unpin_terminal_death(41, queued));
    assert(player_revision_mark(41, PLAYER_COMPONENT_STATUS, &revision));
    assert(revision == queued + 1);
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
            rel("player_revision_state.c"),
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
assert "player_save_journal_replay(selected_snapshot_apply()" in dispatcher
selector = section(PIPELINE, "player_save_apply_fn selected_snapshot_apply()", "struct terminal_fence")
assert "flatfile_player_snapshot_apply_selected" in selector
assert "player_snapshot_repository_apply_from_pool" in selector
assert "sleep_for(std::chrono::milliseconds(100))" in dispatcher
print("[PASS] bounded dispatcher journals before worker eligibility and retains append failures")

checkpoint = section(
    PIPELINE,
    "static player_save_pipeline_result checkpoint_dirty_with_quest_xp(",
    "player_save_pipeline_result player_save_pipeline_checkpoint_dirty",
)
checkpoint_entry = section(
    PIPELINE,
    "player_save_pipeline_result player_save_pipeline_checkpoint_dirty",
    "player_save_pipeline_result player_save_pipeline_request",
)
assert "return checkpoint_dirty_with_quest_xp(ch, save_intent, room_vnum, nullptr, 0);" in checkpoint_entry
assert checkpoint.index("if (!revision.dirty_components)") < checkpoint.index(
    "player_revision_queue"
)
assert checkpoint.index("player_revision_queue") < checkpoint.index("player_snapshot_capture")
assert checkpoint.index("player_save_journal_pid_quarantined") < checkpoint.index(
    "player_revision_snapshot_copy"
)
for forbidden in ("sql_", "redis_", "fopen", "open(", "write("):
    assert forbidden not in checkpoint
pulse = section(PIPELINE, "void player_save_pipeline_pulse", "player_save_pipeline_health")
assert "quest_reward_recovery_save_acknowledged(" in pulse
assert "player_save_worker_pulse" in pulse
assert "acknowledge_terminal_fence_completion_locked(completions[index])" in pulse
assert "player_save_worker_submit_retained" in pulse
for forbidden in ("player_save_journal_", "sql_", "redis_", "fopen", "open(", "write("):
    assert forbidden not in pulse
assert "if (append && !acknowledge)" in WORKER
print("[PASS] simulation-thread checkpoint and completion paths contain no external I/O")

write_character = section(FILES, "int writeCharacter(P_char ch", "int deleteCharacter")
admission = section(FILES, "static character_save_admission admit_character_save(",
                    "int writeCharacter(P_char ch")
assert admission.index("CHAR_RFLAG_LOAD_DEGRADED") < admission.index(
    "player_save_pipeline_save_admitted") < admission.index(
    "corpse_raise_player_save_fenced")
assert "collector_service_recover_player(ch)" in admission
assert write_character.index("admit_character_save(ch, is_locker_char)") < write_character.index(
    "// locker hook (pre-save)")
branch = write_character.index("player_save_pipeline_is_nonterminal_type")
assert "!sql_in_transaction()" in write_character[:branch]
for legacy in (
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

flat_fence = section(write_character, "#ifdef __NO_MYSQL__", "#endif")
assert "player_save_pipeline_terminal(ch, type, room, 5000, false)" in flat_fence
assert flat_fence.index("player_save_pipeline_terminal") < flat_fence.index("unequip_char")
assert flat_fence.index("database_acknowledged") < flat_fence.index(
    "REMOVE_BIT(ch->runtime_flags, CHAR_RFLAG_NO_DB_BASELINE)"
)
terminal_save = section(
    ACTOTH,
    "static bool persistence_save_character_terminal_with_policy",
    "bool persistence_save_all_characters_terminal",
)
assert "allow_journal_handoff = false" in terminal_save
assert "persistence_save_character_terminal_with_policy(ch, type, 2000, true)" in terminal_save
assert "persistence_save_character_terminal_with_policy(ch, type, 5000, false)" in terminal_save
init_char = section(NANNY, "void init_char(P_char ch)", "int approve_mode")
assert "player_revision_hydrate(ch->only.pc->pid, 0)" in init_char
print("[PASS] flat baselines and terminal extraction wait for materialized authority")

legacy_start = SQL_PLAYER.rindex("bool sql_save_player(P_char ch")
legacy_save = SQL_PLAYER[legacy_start : SQL_PLAYER.index("bool sql_save_player_status", legacy_start)]
assert "player_revision_mark(GET_PID(ch), PLAYER_CHECKPOINT_COMPONENT_ALL" in legacy_save
assert "SET save_revision=%llu WHERE pid=%d AND save_revision<%llu" in legacy_save
assert "mysql_affected_rows(DB) != 1" in legacy_save
assert legacy_save.index("sql_save_player_shapechanges") < legacy_save.index("SET save_revision")
assert legacy_save.index("SET save_revision") < legacy_save.index("if (own_txn)")
assert "player_revision_acknowledge_durable" in legacy_save
assert legacy_save.index("if (own_txn)") < legacy_save.index(
    "player_revision_acknowledge_durable"
)
assert "if (own_txn)\n\t{\n\t\tif (compatibility_revision" in legacy_save
assert legacy_save.index("player_revision_acknowledge_durable") < legacy_save.index(
    "clear_player_dirty_container_flags(ch)"
)
print("[PASS] transactional compatibility saves fence every older immutable revision")

mark = section(CHECKPOINT, "void mark_player_dirty(int pid)", "void flush_dirty_players(void)")
flush = section(CHECKPOINT, "void flush_dirty_players(void)", "int get_dirty_player_count(void)")
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
assert "acknowledge_terminal_fence_completion_locked" in PIPELINE
assert "completion.durable_revision >= fence->revision" in PIPELINE
assert "std::chrono::steady_clock::now()" in terminal
# Death retries must resolve the retained request before allocating a new operation.
assert "player_save_pipeline_terminal_death_resume" in HEADER
# Both terminal entry points reserve the fence and mark a fresh ALL revision.
begin_fence = section(
    PIPELINE,
    "bool begin_terminal_fence(int pid, player_revision_t *revision, bool pin_death = false)",
    "player_save_terminal_result await_terminal_fence",
)
assert "player_revision_mark(pid, PLAYER_CHECKPOINT_COMPONENT_ALL, revision)" in begin_fence
assert terminal.count("begin_terminal_fence(pid, &revision, false)") == 1
assert terminal.count("begin_terminal_fence(pid, &revision, true)") == 1
# A refused corpse handoff is captured once and retained in the pinned request.
# checkpointing a character whose refused assets are still only live objects.
death_terminal = section(
    PIPELINE,
    "player_save_pipeline_terminal_death(P_char ch, P_obj corpse, P_obj wallet_pile,",
    "/** Pump the pipeline until this player revision is durable",
)
assert "player_death_snapshot_capture(ch, corpse, wallet_pile, operation_id, revision" in death_terminal
assert death_terminal.index("player_death_snapshot_capture") < death_terminal.index(
    "retain_and_enqueue_death_snapshot"
) < death_terminal.index("await_terminal_fence(pid, revision")
assert "player_save_pipeline_checkpoint_dirty" not in death_terminal
assert "CHAR_RFLAG_LOAD_ITEM_PAYLOAD_GAP" in death_terminal
assert "player_save_pipeline_terminal_death_resume" in death_terminal
assert "promote_existing" not in terminal
assert terminal.index("if (fence->acknowledged)") < terminal.index("allow_journal_handoff && fence->journaled")
assert "clear_terminal_fence_locked(*fence)" in terminal
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

# Run the actual terminal coordinator against controlled worker ACKs. In
# particular, an ACKed nonterminal retry must not authorize a later camp.
# The slice starts inside the anonymous namespace holding the fence helpers.
await_start = PIPELINE.rindex("/** Pump the pipeline until this player revision is durable or the deadline passes. */")
await_end = PIPELINE.index("void player_save_pipeline_pulse", await_start)
terminal_slice = (
    "namespace\n{\n"
    + section(
        PIPELINE,
        "bool begin_terminal_fence(int pid, player_revision_t *revision, bool pin_death = false)",
        "bool retain_and_enqueue_death_snapshot",
    )
    + "} // namespace\n"
    + "namespace { player_save_terminal_result await_terminal_fence(int, player_revision_t, uint64_t, bool); }\n"
    + section(
        PIPELINE,
        "/** Capture fresh terminal intent and wait for its durability fence within the caller timeout. */",
        "/** Record one immutable death disposition",
    )
    + "namespace\n{\n"
    + PIPELINE[await_start:await_end]
)
terminal_preamble = r'''
#include "player/player_save_pipeline.h"
#include "player/player_snapshot.h"
#include "player/player_snapshot_capture.h"
#include "core/defines.h"
#include <cassert>
#include <chrono>
#include <mutex>
#include <thread>
bool player_save_journal_pid_quarantined(int) { return false; }
struct char_data { int pid; unsigned int runtime_flags; };
struct obj_data { int uid; };
#define IS_SET(flag, bit) ((flag) & (bit))
player_snapshot_capture_result death_capture_result = player_snapshot_capture_result::ok;
int death_enqueued = 0;
bool enqueue_refused = false;
bool queue_mismatch = false;
player_snapshot_capture_result player_death_snapshot_capture(
    P_char ch, P_obj, P_obj, const critical_operation_id &, player_revision_t revision, int,
    const std::vector<critical_operation_id> &, player_snapshot *snapshot)
{
    snapshot->pid = ch->pid;
    snapshot->revision = revision;
    snapshot->components = PLAYER_CHECKPOINT_COMPONENT_ALL;
    if (queue_mismatch) snapshot->components = PLAYER_COMPONENT_STATUS;
    return death_capture_result;
}
player_save_pipeline_result enqueue_snapshot(player_snapshot snapshot)
{
    player_revision_snapshot current = {};
    assert(player_revision_snapshot_copy(snapshot.pid, &current));
    assert(current.queued_revision == snapshot.revision);
    assert(current.queued_components == snapshot.components);
    ++death_enqueued;
    return enqueue_refused ? player_save_pipeline_result::unavailable : player_save_pipeline_result::queued;
}
#define IS_NPC(ch) false
#define GET_PID(ch) ((ch)->pid)
#define LOG_STATUS 0
struct terminal_fence { int pid; player_revision_t revision; bool journaled; bool acknowledged; bool death_pinned = false; };
struct target_save_login_fence { int pid; player_revision_t expected_revision; };
terminal_fence fence = {};
target_save_login_fence target_fence = {};
std::mutex pipeline_mutex;
player_save_pipeline_health health = {};
void clear_terminal_fence_locked(terminal_fence &value) { value = {}; }
int captured_intent = 1, captured_room = 0;
player_revision_t captured_revision = 0;
bool database_ready = true, journal_ready = false;
terminal_fence *find_terminal_fence_locked(int pid) { return fence.pid == pid ? &fence : nullptr; }
target_save_login_fence *find_target_save_login_fence_locked(int pid) {
    return target_fence.pid == pid ? &target_fence : nullptr;
}
terminal_fence *allocate_terminal_fence_locked(int pid) { if (fence.pid && fence.pid != pid) return nullptr; fence.pid = pid; return &fence; }
bool trace_player_saves() { return true; }
bool snapshot_is_journaled_locked(const player_revision_snapshot &) { return true; }
uint64_t persistence_observability_now_usec() { return 0; }
void logit(int, const char *, ...) {}
player_save_pipeline_result player_save_pipeline_checkpoint_dirty(P_char ch, int intent, int room) {
    player_revision_snapshot current = {};
    assert(player_revision_snapshot_copy(ch->pid, &current));
    if (!current.dirty_components) return player_save_pipeline_result::unchanged;
    captured_intent = intent;
    captured_room = room;
    captured_revision = current.current_revision;
    player_revision_t queued;
    player_component_mask_t components;
    assert(player_revision_queue(ch->pid, &queued, &components));
    if (journal_ready && fence.revision == captured_revision) fence.journaled = true;
    return player_save_pipeline_result::queued;
}
void player_save_pipeline_pulse() {
    if (!database_ready) return;
    player_revision_snapshot current = {};
    assert(player_revision_snapshot_copy(1, &current));
    if (current.queued_components) {
        assert(player_revision_begin_inflight(1, current.queued_revision, current.queued_components));
        assert(player_revision_acknowledge(1, current.queued_revision, current.queued_components));
        if (fence.revision == current.queued_revision) fence.acknowledged = true;
    }
}
'''
terminal_main = r'''
int main() {
    char_data player{1};
    health.initialized = true;
    for (int trial = 0; trial < 10; ++trial) {
        player_revision_reset_for_tests();
        const player_revision_t old_revision = 50 + trial * 10;
        assert(player_revision_hydrate(1, old_revision));
        fence = {1, old_revision, true, true};
        captured_intent = 1; captured_room = 999;
        database_ready = true; journal_ready = false;
        assert(player_save_pipeline_terminal(&player, 6, 22800, 20, false) ==
               player_save_terminal_result::database_acknowledged);
        assert(captured_intent == 6 && captured_room == 22800);
        assert(captured_revision > old_revision);

        // A full pending snapshot also belongs to its original save intent.
        player_revision_t pending;
        assert(player_revision_mark(1, PLAYER_CHECKPOINT_COMPONENT_ALL, &pending));
        player_save_pipeline_checkpoint_dirty(&player, 1, 777);
        assert(player_save_pipeline_terminal(&player, 6, 22801, 20, false) ==
               player_save_terminal_result::database_acknowledged);
        assert(captured_intent == 6 && captured_room == 22801 && captured_revision > pending);

        database_ready = false;
        assert(player_save_pipeline_terminal(&player, 6, 22802, 1, false) ==
               player_save_terminal_result::timed_out);
        database_ready = true;
        player_save_pipeline_pulse();
        assert(player_revision_mark(1, PLAYER_CHECKPOINT_COMPONENT_ALL, &pending));
        fence = {1, pending, false, false};
        player_save_pipeline_checkpoint_dirty(&player, 1, 777);
        player_save_pipeline_pulse();
        assert(fence.acknowledged);
        assert(player_save_pipeline_terminal(&player, 6, 22803, 20, false) ==
               player_save_terminal_result::database_acknowledged);
        assert(captured_intent == 6 && captured_room == 22803 && captured_revision > pending);

        // MariaDB's permitted journal handoff still requires this terminal intent.
        database_ready = false; journal_ready = true;
        assert(player_save_pipeline_terminal(&player, 4, 22804, 20, true) ==
               player_save_terminal_result::journal_durable);
        assert(captured_intent == 4 && captured_room == 22804);
        assert(player_save_pipeline_terminal(&player, 4, 22804, 1, false) ==
               player_save_terminal_result::timed_out);

        // Generic degraded loads, including payload gaps, must still refuse
        // ordinary terminal capture. The specific death API is tested separately.
        const auto before_degraded = captured_revision;
        player.runtime_flags = CHAR_RFLAG_LOAD_DEGRADED;
        assert(player_save_pipeline_terminal(&player, 6, 22807, 20, false) ==
               player_save_terminal_result::unavailable);
        player.runtime_flags |= CHAR_RFLAG_LOAD_ITEM_PAYLOAD_GAP;
        assert(player_save_pipeline_terminal(&player, 6, 22807, 20, false) ==
               player_save_terminal_result::unavailable);
        assert(captured_revision == before_degraded);
        // Public death guards run in test_terminal_death_entrypoints.py;
        // immutable retry helpers run in test_stable_terminal_death_request.py.
        player.runtime_flags = 0;
    }
}
'''
terminal_build = ROOT / "bin/tests/terminal-save-intent"
terminal_build.mkdir(parents=True, exist_ok=True)
terminal_source = terminal_build / "regression.cpp"
terminal_binary = terminal_build / "regression"
terminal_source.write_text(terminal_preamble + terminal_slice + terminal_main)
subprocess.run([
    "g++", "-std=c++20", "-Isrc", str(terminal_source),
    "src/player/player_revision_state.c", "-pthread", "-o", str(terminal_binary),
], cwd=ROOT, check=True)
subprocess.run([str(terminal_binary)], check=True, timeout=10)
print("[PASS] ten terminal-intent trials cover prior ACK, pending save, timed-out camp retry, and journal handoff")

# Compile the real checkpoint and receipt merge with controlled admission/capture
# failures. Revision state and codec remain real so component and wire checks run.
receipt_harness = r'''
#include "player/player_save_pipeline.h"
#include "player/player_snapshot_capture.h"
#include "player/player_snapshot_codec.h"
#include "core/defines.h"
#include <algorithm>
#include <cassert>
#include <mutex>
#include <new>
struct char_data { int pid; unsigned int runtime_flags = 0; };
#undef GET_PID
#undef IS_NPC
#define GET_PID(ch) ((ch)->pid)
#define IS_NPC(ch) false
#define IS_SET(flag, bit) ((flag) & (bit))
#define LOG_STATUS 0
constexpr int RENT_CRASH = 1, RENT_INN = 3;
std::mutex pipeline_mutex;
struct { int unchanged = 0, capture_failures = 0; } health;
struct terminal_fence { bool death_pinned = false; };
terminal_fence *find_terminal_fence_locked(int) { return nullptr; }
void *find_target_save_login_fence_locked(int) { return nullptr; }
bool player_save_journal_pid_quarantined(int) { return false; }
bool snapshot_is_retained_locked(int, player_revision_t) { return false; }
bool trace_player_saves() { return false; }
uint64_t persistence_observability_now_usec() { return 0; }
void logit(int, const char *, ...) {}
bool player_save_pipeline_mark(int pid, player_component_mask_t components) {
    return player_revision_mark(pid, components, nullptr);
}
bool capture_fails = false;
bool creation_pending = false;
bool item_movement_transaction_player_creation_busy(P_char) { return creation_pending; }
bool refuse_enqueue = false;
player_snapshot pending, captured;
constexpr auto CRAFT_PROGRESSION_COMPONENTS = PLAYER_COMPONENT_STATUS | PLAYER_COMPONENT_SKILLS | PLAYER_COMPONENT_AFFECTS | PLAYER_COMPONENT_TROPHIES;
struct { bool (*pending)(uint32_t, std::vector<player_craft_receipt_snapshot> *) = nullptr; } craft_progression_hooks;
bool craft_progression_pending_save_receipts(uint32_t pid, std::vector<player_craft_receipt_snapshot> *receipts) {
    return craft_progression_hooks.pending ? craft_progression_hooks.pending(pid, receipts) : true;
}
bool quest_reward_recovery_pending_save_receipts(
    int, std::vector<player_quest_xp_receipt_snapshot> *receipts,
    player_component_mask_t *components) {
    *receipts = pending.quest_xp_receipts;
    *components = receipts->empty() ? 0 : PLAYER_COMPONENT_STATUS | PLAYER_COMPONENT_TROPHIES;
    return true;
}
bool spell_component_retirement_pending_save_receipts(
    uint32_t, std::vector<player_spell_effect_receipt_snapshot> *receipts) {
    *receipts = pending.spell_effect_receipts;
    return true;
}
player_snapshot_capture_result player_snapshot_capture(
    P_char ch, player_revision_t revision, player_component_mask_t components,
    int intent, int room, player_snapshot *snapshot) {
    if (capture_fails) return player_snapshot_capture_result::malformed_source;
    snapshot->schema_version = PLAYER_SNAPSHOT_SCHEMA_VERSION;
    snapshot->pid = ch->pid;
    snapshot->revision = revision;
    snapshot->components = components;
    snapshot->save_intent = intent;
    snapshot->room_vnum = room;
    snapshot->encoded_size_bound = 4096;
    return player_snapshot_capture_result::ok;
}
player_save_pipeline_result enqueue_snapshot(player_snapshot snapshot) {
    captured = std::move(snapshot);
    return refuse_enqueue ? player_save_pipeline_result::overloaded : player_save_pipeline_result::queued;
}
''' + extract_function("player_save_pipeline.c", "bool merge_quest_xp_receipts(") + "\n" + extract_function("player_save_pipeline.c", "bool merge_spell_effect_receipts(") + "\n" + extract_function("player_save_pipeline.c", "bool merge_craft_receipts(") + "\n" + extract_function(
    "player_save_pipeline.c", "static player_save_pipeline_result checkpoint_dirty_with_quest_xp("
) + r'''
void verify() {
    assert(captured.components & PLAYER_COMPONENT_AFFECTS);
    assert(captured.spell_effect_receipts.size() == 1);
    assert(captured.spell_effect_receipts[0].operation_id.bytes[0] == 77);
    std::vector<uint8_t> bytes;
    assert(player_snapshot_encode(captured, &bytes) == player_snapshot_codec_result::ok);
    player_snapshot decoded;
    assert(player_snapshot_decode(bytes.data(), bytes.size(), &decoded) == player_snapshot_codec_result::ok);
    assert(decoded.spell_effect_receipts[0].effect_id == 6);
}
int main() {
    char_data player {41};
    assert(player_revision_hydrate(41, 0));
    player_spell_effect_receipt_snapshot receipt = {};
    receipt.operation_id.bytes[0] = 77;
    receipt.effect_id = 6;
    pending.spell_effect_receipts.push_back(receipt);
    creation_pending = true;
    assert(checkpoint_dirty_with_quest_xp(&player, RENT_CRASH, 1201, nullptr, 0, &receipt) == player_save_pipeline_result::unavailable);
    assert(pending.spell_effect_receipts.size() == 1 && !captured.pid);
    creation_pending = false;
    capture_fails = true;
    assert(checkpoint_dirty_with_quest_xp(&player, RENT_CRASH, 1201, nullptr, 0, &receipt) == player_save_pipeline_result::capture_failed);
    capture_fails = false;
    assert(player_save_pipeline_mark(41, PLAYER_COMPONENT_STATUS));
    assert(checkpoint_dirty_with_quest_xp(&player, RENT_CRASH, 1201, nullptr, 0) == player_save_pipeline_result::queued);
    verify();
    refuse_enqueue = true;
    assert(checkpoint_dirty_with_quest_xp(&player, RENT_CRASH, 1201, nullptr, 0, &receipt) == player_save_pipeline_result::overloaded);
    refuse_enqueue = false;
    assert(player_save_pipeline_mark(41, PLAYER_CHECKPOINT_COMPONENT_ALL));
    player_revision_snapshot before;
    assert(player_revision_snapshot_copy(41, &before));
    assert(checkpoint_dirty_with_quest_xp(&player, RENT_INN, 1201, nullptr, 0) == player_save_pipeline_result::queued);
    verify();
    assert(captured.revision == before.current_revision && captured.save_intent == RENT_INN);
    assert(captured.components == PLAYER_CHECKPOINT_COMPONENT_ALL);
    // Conflicting identity is refused; an explicit duplicate is never appended twice.
    receipt.effect_id = 1;
    assert(checkpoint_dirty_with_quest_xp(&player, RENT_CRASH, 1201, nullptr, 0, &receipt) == player_save_pipeline_result::invalid);
    auto death = captured;
    death.schema_version = PLAYER_SNAPSHOT_DEATH_SCHEMA_VERSION;
    death.death.emplace();
    death.spell_effect_receipts.clear();
    const auto bound = death.encoded_size_bound;
    assert(merge_spell_effect_receipts(&death, pending));
    assert(death.schema_version == PLAYER_SNAPSHOT_DEATH_SPELL_RECEIPT_SCHEMA_VERSION);
    assert(death.encoded_size_bound == bound + 24);
    // Applied XP from a refused capture/admission enters subsequent ordinary
    // and terminal checkpoints together with its progression components.
    player_quest_xp_receipt_snapshot xp = {};
    xp.offering_operation.bytes[0] = 88;
    xp.reward_index = 0;
    xp.amount = 75;
    pending.quest_xp_receipts.push_back(xp);
    capture_fails = true;
    assert(checkpoint_dirty_with_quest_xp(&player, RENT_CRASH, 1201, &xp, 1) == player_save_pipeline_result::capture_failed);
    capture_fails = false;
    refuse_enqueue = true;
    assert(checkpoint_dirty_with_quest_xp(&player, RENT_CRASH, 1201, &xp, 1) == player_save_pipeline_result::overloaded);
    refuse_enqueue = false;
    assert(player_save_pipeline_mark(41, PLAYER_COMPONENT_LANGUAGES));
    assert(checkpoint_dirty_with_quest_xp(&player, RENT_CRASH, 1201, nullptr, 0) == player_save_pipeline_result::queued);
    verify();
    assert(captured.quest_xp_receipts.size() == 1 && captured.quest_xp_receipts[0].amount == 75);
    assert((captured.components & (PLAYER_COMPONENT_STATUS | PLAYER_COMPONENT_TROPHIES)) ==
           (PLAYER_COMPONENT_STATUS | PLAYER_COMPONENT_TROPHIES));
    assert(player_save_pipeline_mark(41, PLAYER_CHECKPOINT_COMPONENT_ALL));
    assert(checkpoint_dirty_with_quest_xp(&player, RENT_INN, 1201, nullptr, 0) == player_save_pipeline_result::queued);
    assert(captured.quest_xp_receipts.size() == 1);
    auto mixed_death = captured;
    mixed_death.schema_version = PLAYER_SNAPSHOT_DEATH_SPELL_RECEIPT_SCHEMA_VERSION;
    mixed_death.death.emplace();
    mixed_death.quest_xp_receipts.clear();
    const auto mixed_bound = mixed_death.encoded_size_bound;
    assert(merge_quest_xp_receipts(&mixed_death, pending));
    assert(mixed_death.schema_version == PLAYER_SNAPSHOT_DEATH_QUEST_RECEIPT_SCHEMA_VERSION);
    assert(mixed_death.encoded_size_bound == mixed_bound + 28);
    assert(merge_spell_effect_receipts(&mixed_death, pending));
    assert(mixed_death.schema_version == PLAYER_SNAPSHOT_DEATH_QUEST_RECEIPT_SCHEMA_VERSION);
    assert(mixed_death.quest_xp_receipts.size() == 1 && mixed_death.spell_effect_receipts.size() == 1);
    auto xp_death = mixed_death;
    xp_death.schema_version = PLAYER_SNAPSHOT_DEATH_SCHEMA_VERSION;
    xp_death.quest_xp_receipts.clear();
    xp_death.spell_effect_receipts.clear();
    const auto xp_bound = xp_death.encoded_size_bound;
    assert(merge_quest_xp_receipts(&xp_death, pending));
    assert(xp_death.schema_version == PLAYER_SNAPSHOT_DEATH_QUEST_RECEIPT_SCHEMA_VERSION);
    assert(xp_death.encoded_size_bound == xp_bound + 32);
    assert(merge_spell_effect_receipts(&death, pending) && death.spell_effect_receipts.size() == 1);
    assert(death.encoded_size_bound == bound + 24);
    player_craft_receipt_snapshot craft = {};
    craft.operation_id.bytes[0] = 99;
    craft.discipline = 2;
    craft.experience = 7000;
    pending.craft_receipts.push_back(craft);
    craft_progression_hooks.pending = [](uint32_t, std::vector<player_craft_receipt_snapshot> *out) {
        *out = pending.craft_receipts;
        return true;
    };
    assert(player_save_pipeline_mark(41, PLAYER_COMPONENT_LANGUAGES));
    assert(checkpoint_dirty_with_quest_xp(&player, RENT_CRASH, 1201, nullptr, 0) == player_save_pipeline_result::queued);
    verify();
    assert(captured.schema_version == PLAYER_SNAPSHOT_CRAFT_RECEIPT_SCHEMA_VERSION);
    assert(captured.craft_receipts.size() == 1 && captured.quest_xp_receipts.size() == 1);
    assert((captured.components & CRAFT_PROGRESSION_COMPONENTS) == CRAFT_PROGRESSION_COMPONENTS);
    auto craft_death = mixed_death;
    const auto craft_bound = craft_death.encoded_size_bound;
    assert(merge_craft_receipts(&craft_death, pending));
    assert(craft_death.schema_version == PLAYER_SNAPSHOT_DEATH_CRAFT_RECEIPT_SCHEMA_VERSION);
    assert(craft_death.encoded_size_bound == craft_bound + 28);
    assert(merge_craft_receipts(&craft_death, pending));
    assert(craft_death.encoded_size_bound == craft_bound + 28 && craft_death.craft_receipts.size() == 1);
    auto conflict = pending;
    ++conflict.craft_receipts[0].experience;
    assert(!merge_craft_receipts(&craft_death, conflict));
    craft_death.components &= ~PLAYER_COMPONENT_SKILLS;
    assert(!merge_craft_receipts(&craft_death, pending));
}
'''
with tempfile.TemporaryDirectory(prefix="duris-pending-spell-save-") as directory:
    path = Path(directory)
    program = path / "pending.cpp"
    binary = path / "pending"
    program.write_text(receipt_harness)
    subprocess.run(["g++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-Isrc",
                    str(program), rel("player_revision_state.c"), rel("player_snapshot_codec.c"),
                    "-o", str(binary)], cwd=ROOT, check=True)
    subprocess.run([str(binary)], check=True)
print("[PASS] capture/admission failures retain quest XP and spell receipts in later ordinary/terminal saves and combined death merges")
