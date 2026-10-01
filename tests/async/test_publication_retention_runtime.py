#!/usr/bin/env python3
"""Executable shared-movement publication retention regression."""

from pathlib import Path
import subprocess
import tempfile

from _paths import ROOT, rel

HARNESS = r'''
#include "core/utils.h"
#include "economy/economic_gameplay_authority.h"
#include "item/item_movement_transaction.h"
#include "item/item_ownership_runtime.h"
#include "item/item_transfer_command.h"
#include "player/player_snapshot.h"
#include "player/player_snapshot_capture.h"
#include "player/player_snapshot_codec.h"
#include "persistence/persistence_checkpoint.h"

#include <array>
#include <cassert>
#include <chrono>
#include <cstdarg>
#include <cstring>
#include <cstdio>
#include <thread>
#include <vector>

P_char character_list = nullptr;
P_obj object_list = nullptr;
P_room world = nullptr;
P_index obj_index = nullptr;
extern const int top_of_world = 1;

static item_ownership_runtime_entry runtime_entry = {};
static int publication_attempts = 0;
static int completion_calls = 0;
static int craft_callbacks = 0;
static int extractions = 0;
static int restores = 0;
static bool allow_restore = false;
static obj_data restored_output = {};
static critical_apply_outcome forced_outcome = critical_apply_outcome::applied;

void obj_to_obj(P_obj, P_obj) {}
void extract_obj(P_obj object, int) {
    ++extractions;
    P_obj *link = &object_list;
    while (*link && *link != object) link = &(*link)->next;
    if (*link) *link = object->next;
    object->loc_p = LOC_NOWHERE;
    object->obj_uid = 0;
}
void obj_to_char(P_obj object, P_char actor)
{
    object->loc_p = LOC_CARRIED;
    object->loc.carrying = actor;
}
void obj_to_room(P_obj, int) {}
void obj_from_char(P_obj object) { object->loc_p = LOC_NOWHERE; object->loc.carrying = nullptr; }
void send_to_char(const char *, P_char) {}
void persistence_alert(int, const char *, const char *, const char *, const char *, const char *, const char *, ...) {}
void logit(const char *, const char *, ...) {}
void statuslog(int, const char *, ...) {}
int panic_corruption_int(const char *, const char *, ...) { return 0; }
void mark_player_dirty_components(int, uint64_t) {}
void collector_catalog_cache_invalidate() {}
void collector_death_enrollment_note_committed(P_obj, const item_transfer_payload &) {}
bool player_load_item_graph_materialize_creation(const item_transfer_payload &payload, const item_transfer_result &, std::vector<P_obj> *roots) {
    ++restores;
    if (!allow_restore) return false;
    assert(payload.item_count == 1 && payload.items[0].item_uid == 5021);
    restored_output.obj_uid = payload.items[0].item_uid;
    restored_output.R_num = 0;
    restored_output.loc_p = LOC_NOWHERE;
    restored_output.next = object_list;
    object_list = &restored_output;
    roots->push_back(&restored_output);
    return true;
}
void craft_callback(P_char actor, bool committed, const item_transfer_result &result, unsigned int, const uint8_t *, size_t) {
    assert(actor && committed && result.item_count == 1);
    ++craft_callbacks;
}

bool item_ownership_runtime_lookup(uint64_t item_uid, item_ownership_runtime_entry *entry)
{
    if (item_uid != runtime_entry.item_uid || !entry)
        return false;
    *entry = runtime_entry;
    return true;
}

bool item_ownership_runtime_owner_revision(const item_owner_identity &, uint64_t *revision)
{
    if (!revision)
        return false;
    *revision = 1;
    return true;
}

bool item_ownership_runtime_apply(const item_transfer_payload &, const item_transfer_result &)
{
    return true;
}

bool currency_transaction_coin_item_busy(uint64_t) { return false; }
bool spell_component_retirement_waiting_for_effect(const critical_operation_id &) { return false; }
bool collector_transaction_item_busy(uint64_t) { return false; }
bool economic_gameplay_authority::active() { return false; }
economic_accounting_error economic_gameplay_authority::prepare_item_transfer(
    critical_command *, uint32_t, economic_source_kind) {
    return economic_accounting_error::ok;
}

bool collector_death_enrollment_attach(P_char, P_obj, const critical_operation_id &,
                                        const std::vector<player_item_snapshot> &,
                                        item_transfer_payload *)
{
    return true;
}

player_snapshot_capture_result player_item_snapshot_tree_capture(P_obj object,
                                                               std::vector<player_item_snapshot> *snapshots,
                                                               size_t *)
{
    if (!object || !snapshots)
        return player_snapshot_capture_result::invalid_identity;
    player_item_snapshot snapshot = {};
    snapshot.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
    snapshot.object_uid = object->obj_uid;
    snapshot.vnum = 42;
    snapshots->push_back(std::move(snapshot));
    return player_snapshot_capture_result::ok;
}

critical_apply_result apply_transfer(const critical_command &command, void *)
{
    item_transfer_payload payload = {};
    assert(item_transfer_command_decode_payload(command, &payload));
    item_transfer_result result = {};
    result.root_item_uid = payload.selected_item_uid;
    result.item_count = payload.item_count;
    result.from_owner_revision = payload.expected_from_revision + 1;
    result.to_owner_revision = payload.expected_to_revision + 1;
    result.max_item_revision = 2;
    std::array<uint8_t, ITEM_TRANSFER_RESULT_BYTES> encoded = {};
    assert(item_transfer_command_encode_result(result, &encoded));
    critical_apply_result applied = {};
    applied.outcome = forced_outcome;
    applied.durable_revision = 1;
    applied.result_size = encoded.size();
    std::copy(encoded.begin(), encoded.end(), applied.result_payload.begin());
    return applied;
}

bool publication_callback(const critical_operation_id &operation, P_char actor, bool committed, const item_transfer_result &result,
                          unsigned int, const uint8_t *, size_t)
{
    assert(std::any_of(operation.bytes.begin(), operation.bytes.end(), [](uint8_t byte) { return byte != 0; }));
    assert(committed);
    assert(result.item_count == 1);
    if (!actor || actor->runtime_id != 7001)
        return false;
    ++publication_attempts;
    return publication_attempts >= 2;
}

void completion_callback(P_char actor, bool committed, const item_transfer_result &result,
                         unsigned int, const uint8_t *, size_t)
{
    assert(actor && committed && result.item_count == 1);
    ++completion_calls;
}

int main(int argc, char **argv)
{
    assert(argc == 2);
    index_data index = {};
    index.virtual_number = 42;
    obj_index = &index;

    pc_only_data player = {};
    player.pid = 1001;
    char_data actor = {};
    actor.only.pc = &player;
    actor.runtime_id = 7001;
    character_list = &actor;

    obj_data object = {};
    object.obj_uid = 5001;
    object.R_num = 0;
    object.loc_p = LOC_CARRIED;
    object.loc.carrying = &actor;
    object_list = &object;

    runtime_entry.item_uid = object.obj_uid;
    runtime_entry.root_item_uid = object.obj_uid;
    runtime_entry.parent_item_uid = 0;
    runtime_entry.owner = {item_owner_type::player, 2002, 0};
    runtime_entry.item_revision = 1;
    runtime_entry.owner_revision = 1;
    runtime_entry.vnum = 42;
    runtime_entry.state = item_custody_state::active;

    assert(critical_command_coordinator_init(argv[1], apply_transfer, nullptr, 1));
    item_movement_reject reject = item_movement_reject::none;
    const item_owner_identity destination = {item_owner_type::player, 1001, 0};
    if (!item_movement_transaction_submit(
        &actor, &object, nullptr, runtime_entry.owner, destination,
        item_transfer_reason::player_give, 2002, nullptr, nullptr, 0, nullptr,
        &reject, publication_callback))
    {
        std::fprintf(stderr, "submit rejected: %s\\n", item_movement_reject_name(reject));
        return 2;
    }

    critical_completion completions[8] = {};
    bool first_completion_seen = false;
    for (int spin = 0; spin < 1000 && !first_completion_seen; ++spin)
    {
        const size_t count = critical_command_coordinator_pulse(completions, 8);
        if (count)
        {
            item_movement_transaction_handle_completions(completions, count);
            first_completion_seen = true;
        }
        else
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    assert(first_completion_seen);
    assert(publication_attempts == 1);
    item_movement_health retained = item_movement_transaction_health_copy();
    assert(retained.pending == 1 && retained.publication_retrying == 1);
    critical_coordinator_health coordinator = critical_command_coordinator_health_copy();
    assert(coordinator.publication_pending == 1);
    assert(critical_command_coordinator_is_fenced(
        {critical_entity_type::item, object.obj_uid}, nullptr));
    assert(critical_command_coordinator_is_fenced(
        {critical_entity_type::player, 1001}, nullptr));

    character_list = nullptr;
    item_movement_transaction_handle_completions(nullptr, 0);
    assert(publication_attempts == 1);
    assert(critical_command_coordinator_health_copy().publication_pending == 1);

    char_data replacement = actor;
    replacement.runtime_id = 7002;
    character_list = &replacement;
    item_movement_transaction_handle_completions(nullptr, 0);
    assert(publication_attempts == 1);
    assert(item_movement_transaction_health_copy().pending == 1);
    assert(critical_command_coordinator_is_fenced(
        {critical_entity_type::item, object.obj_uid}, nullptr));

    character_list = &actor;
    item_movement_transaction_handle_completions(nullptr, 0);
    assert(publication_attempts == 2);
    item_movement_health published = item_movement_transaction_health_copy();
    assert(published.pending == 0 && published.committed == 1);
    assert(!critical_command_coordinator_is_fenced(
        {critical_entity_type::item, object.obj_uid}, nullptr));
    assert(!critical_command_coordinator_is_fenced(
        {critical_entity_type::player, 1001}, nullptr));

    // A completion that submits a successor grant runs after publication has
    // released the movement fence, and a later pulse cannot invoke it twice.
    publication_attempts = 1;
    assert(item_movement_transaction_submit(
        &actor, &object, nullptr, runtime_entry.owner, destination,
        item_transfer_reason::player_give, 2002, completion_callback, nullptr, 0,
        nullptr, &reject, publication_callback));
    for (int spin = 0; spin < 1000 && !completion_calls; ++spin) {
        const size_t count = critical_command_coordinator_pulse(completions, 8);
        item_movement_transaction_handle_completions(completions, count);
        if (!completion_calls)
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    assert(completion_calls == 1 && item_movement_transaction_health_copy().pending == 0);
    item_movement_transaction_handle_completions(nullptr, 0);
    assert(completion_calls == 1);
    // A committed craft stays fenced while the player is absent or the exact
    // output UID cannot be restored. Publication then retires inputs and notifies once.
    runtime_entry.owner = destination;
    obj_data output_a = {};
    obj_data output_b = {};
    output_a.obj_uid = 5020; output_b.obj_uid = 5021;
    output_a.R_num = output_b.R_num = 0;
    output_a.loc_p = output_b.loc_p = LOC_NOWHERE;
    output_a.next = &output_b; output_b.next = &object;
    object_list = &output_a;
    P_obj inputs[] = {&object};
    P_obj outputs[] = {&output_a, &output_b};
    assert(item_movement_transaction_submit_craft(&actor, inputs, 1, outputs, 2,
        551, craft_callback, nullptr, 0, &reject));
    character_list = nullptr;
    bool craft_completed = false;
    for (int spin = 0; spin < 1000 && !craft_completed; ++spin) {
        const size_t count = critical_command_coordinator_pulse(completions, 8);
        item_movement_transaction_handle_completions(completions, count);
        craft_completed = count > 0;
        if (!craft_completed) std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    assert(craft_completed && craft_callbacks == 0 && extractions == 0);
    // Simulate an output lost from the live world after durable commit.
    output_a.next = &object;
    character_list = &actor;
    item_movement_transaction_handle_completions(nullptr, 0);
    assert(restores == 1 && extractions == 0 && craft_callbacks == 0);
    assert(critical_command_coordinator_health_copy().publication_pending == 1);
    allow_restore = true;
    std::this_thread::sleep_for(std::chrono::milliseconds(70));
    item_movement_transaction_handle_completions(nullptr, 0);
    assert(restores == 2 && extractions == 1 && craft_callbacks == 1);
    assert(OBJ_CARRIED_BY(&output_a, &actor) && OBJ_CARRIED_BY(&restored_output, &actor));
    assert(item_movement_transaction_health_copy().pending == 0);
    item_movement_transaction_handle_completions(nullptr, 0);
    assert(extractions == 1 && craft_callbacks == 1);
    // Restore the independent movement fixture for the uncertainty scenario.
    object.obj_uid = 5001;
    object.loc_p = LOC_CARRIED; object.loc.carrying = &actor;
    object.next = nullptr; object_list = &object;
    runtime_entry.owner = {item_owner_type::player, 2002, 0};

    // Exhausted uncertainty must not invoke the command callback as failure,
    // and must not checkpoint away the only durable retry/reconciliation record.
    publication_attempts = 0;
    forced_outcome = critical_apply_outcome::ambiguous_commit;
    assert(item_movement_transaction_submit(
        &actor, &object, nullptr, runtime_entry.owner, destination,
        item_transfer_reason::player_give, 2002, nullptr, nullptr, 0, nullptr,
        &reject, publication_callback));
    bool uncertain_seen = false;
    for (int spin = 0; spin < 1000 && !uncertain_seen; ++spin) {
        const size_t count = critical_command_coordinator_pulse(completions, 8);
        item_movement_transaction_handle_completions(completions, count);
        uncertain_seen = critical_command_coordinator_health_copy().blocked == 1;
        if (!uncertain_seen)
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    assert(uncertain_seen && publication_attempts == 0);
    assert(item_movement_transaction_health_copy().pending == 1);
    assert(critical_command_coordinator_is_fenced(
        {critical_entity_type::item, object.obj_uid}, nullptr));
    assert(critical_command_journal_health_copy().records == 1);
    critical_command_coordinator_shutdown();
    return 0;
}
'''

with tempfile.TemporaryDirectory(prefix="duris-publication-retention-") as temporary:
    temp = Path(temporary)
    source = temp / "publication_retention_test.cpp"
    binary = temp / "publication_retention_test"
    source.write_text(HARNESS, encoding="utf-8")
    subprocess.run(
        [
            "g++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
            "-D__NO_MYSQL__", "-pthread", "-ffunction-sections", "-fdata-sections",
            "-Isrc", "-Isrc/no_mysql", str(source),
            rel("item/item_movement_transaction.c"), rel("item/item_transfer_command.c"), rel("player_snapshot_codec.c"),
            rel("critical_command.c"), rel("persistence/critical_command_journal.c"),
            rel("persistence/critical_command_coordinator.c"),
            "-Wl,--gc-sections", "-lz", "-lcrypto", "-o", str(binary),
        ],
        cwd=ROOT,
        check=True,
    )
    journal = temp / "journal"
    subprocess.run([str(binary), str(journal)], check=True, timeout=30)

print("shared item movement publication retention passed")
''
