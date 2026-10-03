#!/usr/bin/env python3
"""Run real death capture/resume/wait entrypoints with controlled capture/ACK I/O."""
from pathlib import Path
import runpy
import subprocess
import tempfile
from _paths import SRC, extract_function

# Reuse the production helper harness and its identity/ACK negative cases. It
# runs first; the second executable adds the public APIs whose old coverage was
# previously removed from the ordinary-terminal harness.
base = runpy.run_path(str(Path(__file__).with_name('test_stable_terminal_death_request.py')))
ROOT = base['ROOT']
PIPELINE = base['PIPELINE']
section = base['section']
HARNESS = base['HARNESS'].replace('int main()', 'int retained_helper_checks()', 1)
HARNESS += r'''
#include "player/player_snapshot_capture.h"
#include "player/player_snapshot_codec.h"
#include "core/defines.h"
#include <chrono>
#include <thread>

struct char_data { int pid; unsigned int runtime_flags = 0; bool npc = false; };
struct obj_data { uint64_t obj_uid; };
#define IS_SET(flag, bit) ((flag) & (bit))
#define IS_NPC(ch) ((ch)->npc)
#define GET_PID(ch) ((ch)->pid)
#define LOG_STATUS 0

namespace {
struct target_save_login_fence {};
target_save_login_fence *find_target_save_login_fence_locked(int) { return nullptr; }
bool trace_player_saves() { return false; }
uint64_t persistence_observability_now_usec() { return 0; }
void logit(int, const char *, ...) {}
bool database_ready = false;
bool journal_ready = false;
bool player_save_journal_pid_quarantined(int) { return false; }
bool capture_component_mismatch = false;
std::vector<player_quest_xp_receipt_snapshot> pending_xp;
std::vector<player_craft_receipt_snapshot> pending_craft;
bool craft_receipts_ready = true;
bool merge_spell_effect_receipts(player_snapshot *, const player_snapshot &) { return true; }
size_t capture_bytes = 256;
player_snapshot_capture_result capture_result = player_snapshot_capture_result::ok;
player_save_terminal_result await_terminal_fence(int, player_revision_t, uint64_t, bool);
'''
HARNESS += extract_function("player_save_pipeline.c", "bool merge_quest_xp_receipts(") + "\n"
HARNESS += section((SRC / "craft_progression_hooks.h").read_text(),
                   "constexpr player_component_mask_t CRAFT_PROGRESSION_COMPONENTS =",
                   "inline bool")
HARNESS += extract_function("player_save_pipeline.c", "bool merge_craft_receipts(") + "\n"
HARNESS += r'''
} // namespace
bool quest_reward_recovery_pending_save_receipts(int,
    std::vector<player_quest_xp_receipt_snapshot> *receipts, player_component_mask_t *components) {
    *receipts = pending_xp;
    *components = pending_xp.empty() ? 0 : PLAYER_COMPONENT_STATUS | PLAYER_COMPONENT_TROPHIES;
    return true;
}
bool spell_component_retirement_pending_save_receipts(uint32_t, std::vector<player_spell_effect_receipt_snapshot> *) { return true; }
bool craft_progression_pending_save_receipts(uint32_t,
    std::vector<player_craft_receipt_snapshot> *receipts) {
    if (!craft_receipts_ready) return false;
    *receipts = pending_craft;
    return true;
}
player_snapshot_codec_result player_snapshot_encode(const player_snapshot &, std::vector<uint8_t> *) { return player_snapshot_codec_result::ok; }
namespace {
'''
HARNESS += section(PIPELINE, 'terminal_fence *allocate_terminal_fence_locked(int pid)',
                   '/** Find a recipient-only save/login fence;')
HARNESS += section(PIPELINE, 'bool begin_terminal_fence(int pid, player_revision_t *revision,',
                   'bool retain_and_enqueue_death_snapshot')
HARNESS += '} // namespace\n'
HARNESS += r'''
player_snapshot_capture_result player_death_snapshot_capture(
    P_char ch, P_obj corpse, P_obj wallet, const critical_operation_id &operation,
    player_revision_t revision, int, const std::vector<critical_operation_id> &,
    player_snapshot *snapshot)
{
    if (capture_result != player_snapshot_capture_result::ok)
        return capture_result;
    make_snapshot(*snapshot, ch->pid, revision, operation, corpse->obj_uid,
                  wallet ? wallet->obj_uid : 0, capture_bytes);
    if (capture_component_mismatch)
        snapshot->components = PLAYER_COMPONENT_STATUS;
    return capture_result;
}

// The only simulated persistence boundary: journal publication and an exact
// worker completion. The real public APIs, pin, queue, revision state, wait,
// cleanup and ACK gate are compiled unchanged below.
void player_save_pipeline_pulse()
{
    for (const player_snapshot &snapshot : pending_append)
        if (terminal_fence *fence = find_terminal_fence_locked(snapshot.pid))
            fence->journaled = journal_ready;
    if (!database_ready || pending_append.empty())
        return;
    player_snapshot snapshot = std::move(pending_append.front());
    pending_append.pop_front();
    retained_bytes -= snapshot.encoded_size_bound;
    assert(player_revision_begin_inflight(snapshot.pid, snapshot.revision, snapshot.components));
    assert(player_revision_acknowledge(snapshot.pid, snapshot.revision, snapshot.components));
    player_save_completion completion = {};
    completion.pid = snapshot.pid;
    completion.revision = snapshot.revision;
    completion.components = snapshot.components;
    completion.durable_revision = snapshot.revision;
    completion.outcome = player_save_apply_outcome::applied;
    assert(acknowledge_terminal_fence_completion_locked(completion));
}
'''
HARNESS += section(PIPELINE, '/** Record one immutable death disposition;', '\nnamespace\n{')
HARNESS += '\nnamespace\n{\n' + section(
    PIPELINE, '/** Pump the pipeline until this player revision is durable or the deadline passes. */',
    'void player_save_pipeline_pulse')
HARNESS += r'''
int main()
{
    assert(retained_helper_checks() == 0);
    player_revision_reset_for_tests();
    health = {};
    health.initialized = true;
    assert(pending_append.empty() && retained_bytes == 0);
    assert(pinned_death_count_locked() == 0);
    char_data player{71};
    obj_data corpse{71001}, wallet{71002};
    critical_operation_id operation = {};
    operation.bytes[0] = 0xE1;
    assert(player_revision_hydrate(player.pid, 70));
    const auto saved = [&](P_char ch, P_obj body, bool journal = false) {
        return player_save_pipeline_terminal_death(ch, body, &wallet, operation,
                                                  22806, 1, journal);
    };
    assert(saved(&player, nullptr) == player_save_terminal_result::invalid);
    player_revision_snapshot before = {};
    assert(player_revision_snapshot_copy(player.pid, &before));
    assert(before.current_revision == 70 && !find_terminal_fence_locked(player.pid));
    auto &literal = literal_inventory_checkpoints[0];
    literal.token = {player.pid, 71, corpse.obj_uid, 1};
    literal.payload = {3, 4, 5};
    literal.held = true;
    const auto held_captures = capture_count;
    const auto held_pipeline_captures = health.captured;
    assert(saved(&player, &corpse) == player_save_terminal_result::unavailable);
    player_revision_snapshot after_held = {};
    assert(player_revision_snapshot_copy(player.pid, &after_held));
    assert(after_held.current_revision == before.current_revision &&
           after_held.acknowledged_revision == before.acknowledged_revision &&
           after_held.dirty_components == before.dirty_components &&
           after_held.queued_components == before.queued_components &&
           !find_terminal_fence_locked(player.pid) && pinned_death_count_locked() == 0 &&
           pending_append.empty() && !retained_bytes && capture_count == held_captures &&
           health.captured == held_pipeline_captures && literal.held);
    literal.held = false;
    capture_result = player_snapshot_capture_result::limit_exceeded;
    assert(saved(&player, &corpse) == player_save_terminal_result::invalid);
    assert(!literal.token.pid && literal.payload.empty() &&
           !find_terminal_fence_locked(player.pid) && !pinned_death_count_locked() &&
           pending_append.empty() && !retained_bytes);
    capture_result = player_snapshot_capture_result::ok;
    // Restore the baseline independently after the superseding failed intent.
    player_revision_forget(player.pid);
    assert(player_revision_hydrate(player.pid, 70));
    assert(player_save_pipeline_terminal_death(nullptr, &corpse, &wallet, operation,
                                              22806, 1, false) == player_save_terminal_result::invalid);
    player.npc = true;
    assert(saved(&player, &corpse) == player_save_terminal_result::invalid);
    player.npc = false;

    // A stale ordinary ACK cannot release a new failed death capture. Repeated
    // refusals beyond the fence-array capacity must not leak pins or bytes.
    capture_result = player_snapshot_capture_result::limit_exceeded;
    for (int pid = 1000; pid < 1300; ++pid) {
        char_data other{pid};
        assert(player_revision_hydrate(pid, 1));
        terminal_fence *old = allocate_terminal_fence_locked(pid);
        assert(old);
        old->revision = 1;
        old->acknowledged = true;
        old->journaled = true;
        const auto failures = health.capture_failures;
        assert(saved(&other, &corpse, true) == player_save_terminal_result::invalid);
        assert(health.capture_failures == failures + 1);
        assert(find_terminal_fence_locked(pid) == nullptr);
        assert(pinned_death_count_locked() == 0 && retained_bytes == 0);
        assert(player_revision_mark(pid, PLAYER_COMPONENT_STATUS, nullptr));
    }
    capture_result = player_snapshot_capture_result::ok;
    // A refused progression capture cannot leave a terminal pin or queued bytes.
    craft_receipts_ready = false;
    assert(saved(&player, &corpse) == player_save_terminal_result::unavailable);
    assert(find_terminal_fence_locked(player.pid) == nullptr);
    assert(pending_append.empty() && retained_bytes == 0 && pinned_death_count_locked() == 0);
    craft_receipts_ready = true;
    capture_component_mismatch = true;
    assert(saved(&player, &corpse) == player_save_terminal_result::unavailable);
    assert(find_terminal_fence_locked(player.pid) == nullptr);
    assert(pending_append.empty() && retained_bytes == 0);
    capture_component_mismatch = false;
    capture_bytes = PLAYER_SAVE_PIPELINE_MAX_BYTES / 2 + 1;
    assert(saved(&player, &corpse) == player_save_terminal_result::unavailable);
    assert(find_terminal_fence_locked(player.pid) == nullptr);
    assert(pending_append.empty() && retained_bytes == 0);
    capture_bytes = 256;

    player.runtime_flags = CHAR_RFLAG_LOAD_DEGRADED;
    assert(saved(&player, &corpse) == player_save_terminal_result::unavailable);
    assert(find_terminal_fence_locked(player.pid) == nullptr);
    player.runtime_flags |= CHAR_RFLAG_LOAD_ITEM_PAYLOAD_GAP;
    player_quest_xp_receipt_snapshot xp = {};
    xp.offering_operation.bytes[0] = 88;
    xp.amount = 75;
    pending_xp.push_back(xp);
    player_craft_receipt_snapshot craft = {};
    craft.operation_id.bytes[0] = 89;
    craft.discipline = 2;
    craft.experience = 7000;
    pending_craft.push_back(craft);
    assert(saved(&player, &corpse) == player_save_terminal_result::timed_out);
    const int captures = capture_count;
    terminal_fence *pinned = find_terminal_fence_locked(player.pid);
    assert(pinned && pinned->death_pinned);
    const player_revision_t revision = pinned->revision;
    const auto pinned_bytes = (capture_bytes + 32 + 28) * 2;
    assert(pending_append.size() == 1 && retained_bytes == pinned_bytes);
    assert(pinned->death_snapshot->schema_version == PLAYER_SNAPSHOT_DEATH_CRAFT_RECEIPT_SCHEMA_VERSION);
    assert(pinned->death_snapshot->quest_xp_receipts.size() == 1 &&
           pinned->death_snapshot->quest_xp_receipts[0].amount == 75);
    assert(pinned->death_snapshot->craft_receipts.size() == 1 &&
           pinned->death_snapshot->craft_receipts[0].operation_id.bytes == craft.operation_id.bytes &&
           pinned->death_snapshot->craft_receipts[0].discipline == 2 &&
           pinned->death_snapshot->craft_receipts[0].experience == 7000);
    // Later recovery state changes must not recapture or alter this request.
    pending_xp[0].amount = 76;
    pending_craft[0].experience = 7001;
    pending_craft[0].discipline = 1;
    assert(player_save_pipeline_terminal_death_resume(&player, corpse.obj_uid + 1, 1) ==
           player_save_terminal_result::unavailable);

    journal_ready = true;
    assert(saved(&player, &corpse, true) == player_save_terminal_result::timed_out);
    assert(pinned->journaled && !pinned->acknowledged);
    assert(capture_count == captures && pending_append.size() == 1);
    assert(pinned->revision == revision && pinned->wallet_pile_uid == wallet.obj_uid);
    assert(player_save_pipeline_terminal_death_resume(&player, corpse.obj_uid, 1) ==
           player_save_terminal_result::timed_out);
    assert(capture_count == captures && pinned->revision == revision);
    assert(pinned->death_snapshot->quest_xp_receipts[0].amount == 75 &&
           pending_append.front().quest_xp_receipts[0].amount == 75);

    assert(pinned->death_snapshot->craft_receipts[0].experience == 7000 &&
           pinned->death_snapshot->craft_receipts[0].discipline == 2 &&
           pending_append.front().craft_receipts[0].experience == 7000 &&
           pending_append.front().craft_receipts[0].operation_id.bytes == craft.operation_id.bytes);
    database_ready = true;
    assert(player_save_pipeline_terminal_death_resume(&player, corpse.obj_uid, 20) ==
           player_save_terminal_result::database_acknowledged);
    assert(find_terminal_fence_locked(player.pid) == nullptr);
    assert(pending_append.empty() && retained_bytes == 0 && pinned_death_count_locked() == 0);
    assert(player_save_pipeline_terminal_death_resume(&player, corpse.obj_uid, 1) ==
           player_save_terminal_result::not_pending);
    assert(player_revision_mark(player.pid, PLAYER_COMPONENT_STATUS, nullptr));
    return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='duris-death-entrypoints-') as temp:
    source = Path(temp) / 'entrypoints.cpp'
    binary = Path(temp) / 'entrypoints'
    source.write_text(HARNESS)
    subprocess.run([base['compiler'], *base['SANITIZER_FLAGS'], '-std=c++20', '-Wall', '-Wextra', '-Wpedantic',
                    '-Werror', '-Isrc', str(source), 'src/player/player_revision_state.c',
                    '-pthread', '-o', str(binary)], cwd=ROOT, check=True)
    subprocess.run([str(binary)], cwd=ROOT, check=True, timeout=15)
print('[PASS] real death entrypoints preserve missing-corpse, capture/queue refusal, and degraded-load guards')
print('[PASS] repeated capture failures beyond fence capacity release every pin and retained byte')
print('[PASS] real timeout/resume/wait retain identity and reject journal-only release until exact database ACK')
print('[PASS] pending craft capture refusal clears pins; exact frozen craft identity/discipline/XP survive timeout and retry')
print('[PASS] held literal publication refuses death before capture/pin; unheld lease is superseded even when later capture fails')
