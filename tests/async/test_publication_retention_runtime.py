#!/usr/bin/env python3
"""Executable shared-movement publication retention regression."""

from pathlib import Path
import subprocess
import tempfile

from _paths import ROOT, rel

HARNESS = r'''
#include "core/utils.h"
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
static item_ownership_runtime_entry target_runtime = {};
static int publication_attempts = 0;
static bool simulate_adoption = false;
static std::vector<item_transfer_reason> adoption_reasons;
static item_transfer_reason expected_adoption_reason = item_transfer_reason::destruction;
static int adoption_publications = 0;
static int adoption_completions = 0;
static int room_decay_publications = 0;
static int room_move_publications = 0;
static critical_apply_outcome forced_outcome = critical_apply_outcome::applied;

void obj_to_obj(P_obj, P_obj) {}
void extract_obj(P_obj, int) {}
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
void player_load_item_graph_materialize_creation(const item_transfer_payload &, const item_transfer_result &, std::vector<P_obj> *) {}

bool item_ownership_runtime_lookup(uint64_t item_uid, item_ownership_runtime_entry *entry)
{
    if (!entry)
        return false;
    if (item_uid == runtime_entry.item_uid)
    {
        *entry = runtime_entry;
        return true;
    }
    if (target_runtime.item_uid && item_uid == target_runtime.item_uid)
    {
        *entry = target_runtime;
        return true;
    }
    return false;
}

bool item_ownership_runtime_owner_revision(const item_owner_identity &, uint64_t *revision)
{
    if (!revision)
        return false;
    *revision = 1;
    return true;
}

bool item_ownership_runtime_apply(const item_transfer_payload &payload, const item_transfer_result &)
{
    if (simulate_adoption)
    {
        runtime_entry.item_uid = payload.selected_item_uid;
        runtime_entry.root_item_uid = payload.selected_item_uid;
        runtime_entry.parent_item_uid = 0;
        runtime_entry.owner = payload.to_owner;
        runtime_entry.item_revision = 1;
        runtime_entry.owner_revision = 1;
        runtime_entry.vnum = 42;
        runtime_entry.state = payload.to_owner.type == item_owner_type::destruction ?
            item_custody_state::destroyed : item_custody_state::active;
    }
    return true;
}

bool currency_transaction_coin_item_busy(uint64_t) { return false; }
bool collector_transaction_item_busy(uint64_t) { return false; }

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

player_snapshot_codec_result player_item_snapshot_list_encode(const std::vector<player_item_snapshot> &snapshots,
                                      std::vector<uint8_t> *encoded)
{
    if (snapshots.empty() || !encoded)
        return player_snapshot_codec_result::invalid_value;
    encoded->assign(1, 1);
    return player_snapshot_codec_result::ok;
}

critical_apply_result apply_transfer(const critical_command &command, void *)
{
    item_transfer_payload payload = {};
    assert(item_transfer_command_decode_payload(command, &payload));
    if (simulate_adoption)
    {
        adoption_reasons.push_back(payload.reason);
        assert(payload.target_parent_item_uid ==
               (adoption_reasons.size() == 1 ? 0 : target_runtime.item_uid));
    }
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

bool publication_callback(P_char actor, bool committed, const item_transfer_result &result,
                          unsigned int, const uint8_t *, size_t)
{
    assert(committed);
    assert(result.item_count == 1);
    if (!actor || actor->runtime_id != 7001)
        return false;
    ++publication_attempts;
    return publication_attempts >= 2;
}

bool adoption_publication(P_char actor, bool committed, const item_transfer_result &,
                          unsigned int, const uint8_t *, size_t)
{
    assert(actor && committed);
    assert(adoption_reasons.size() == 2);
    assert(adoption_reasons[0] == item_transfer_reason::creation);
    assert(adoption_reasons[1] == expected_adoption_reason);
    ++adoption_publications;
    return true;
}

void adoption_completion(P_char actor, bool committed, const item_transfer_result &,
                         unsigned int, const uint8_t *, size_t)
{
    assert(actor && committed && adoption_publications == 1);
    ++adoption_completions;
}

bool room_decay_publication(P_char actor, bool committed, const item_transfer_result &result,
                            unsigned int, const uint8_t *, size_t)
{
    assert(!actor && committed && result.item_count == 1);
    ++room_decay_publications;
    return room_decay_publications >= 2;
}

bool room_move_publication(P_char actor, bool committed, const item_transfer_result &result,
                           unsigned int, const uint8_t *, size_t)
{
    assert(!actor && committed && result.item_count == 1);
    ++room_move_publications;
    return room_move_publications >= 2;
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

    // An absent item's admission must finish before its requested destruction.
    // The caller's publication and completion run only for the second command.
    const item_ownership_runtime_entry previous_runtime = runtime_entry;
    runtime_entry.item_uid = 0;
    simulate_adoption = true;
    const item_owner_identity player_owner = {item_owner_type::player, 1001, 0};
    const item_owner_identity destruction = {item_owner_type::destruction, 0, 0};
    assert(item_movement_transaction_submit(
        &actor, &object, nullptr, player_owner, destruction,
        item_transfer_reason::destruction, 42, adoption_completion, nullptr, 0,
        nullptr, &reject, adoption_publication));
    bool adoption_done = false;
    for (int spin = 0; spin < 1000 && !adoption_done; ++spin)
    {
        const size_t count = critical_command_coordinator_pulse(completions, 8);
        item_movement_transaction_handle_completions(completions, count);
        adoption_done = adoption_completions == 1;
        if (!adoption_done)
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    assert(adoption_done && adoption_publications == 1);
    assert(adoption_reasons.size() == 2);
    assert(item_movement_transaction_health_copy().pending == 0);

    // Admission for a cross-owner put must not name the destination parent
    // until the second transfer has moved the item into that owner's domain.
    obj_data well = {};
    well.obj_uid = 6001;
    object.next = &well;
    target_runtime.item_uid = well.obj_uid;
    target_runtime.root_item_uid = well.obj_uid;
    target_runtime.parent_item_uid = 0;
    target_runtime.owner = {item_owner_type::room, 123, 0};
    target_runtime.item_revision = 1;
    target_runtime.owner_revision = 1;
    target_runtime.vnum = 42;
    target_runtime.state = item_custody_state::active;
    runtime_entry.item_uid = 0;
    adoption_reasons.clear();
    adoption_publications = 0;
    adoption_completions = 0;
    expected_adoption_reason = item_transfer_reason::player_put;
    assert(item_movement_transaction_submit(
        &actor, &object, &well, player_owner, target_runtime.owner,
        item_transfer_reason::player_put, well.obj_uid, adoption_completion,
        nullptr, 0, nullptr, &reject, adoption_publication));
    adoption_done = false;
    for (int spin = 0; spin < 1000 && !adoption_done; ++spin)
    {
        const size_t count = critical_command_coordinator_pulse(completions, 8);
        item_movement_transaction_handle_completions(completions, count);
        adoption_done = adoption_completions == 1;
        if (!adoption_done)
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    assert(adoption_done && adoption_publications == 1);
    assert(adoption_reasons.size() == 2);
    assert(item_movement_transaction_health_copy().pending == 0);
    object.next = nullptr;
    target_runtime = {};
    simulate_adoption = false;
    runtime_entry = previous_runtime;

    // Autonomous room decay retains the room/item fences when its live
    // publication cannot finish, then retries without a player actor.
    room_data rooms[2] = {};
    rooms[0].number = 123;
    world = rooms;
    object.loc_p = LOC_ROOM;
    object.loc.room = 0;
    runtime_entry.owner = {item_owner_type::room, 123, 0};
    assert(item_movement_transaction_submit(
        nullptr, &object, nullptr, runtime_entry.owner, destruction,
        item_transfer_reason::destruction, 42, nullptr, nullptr, 0, nullptr,
        &reject, room_decay_publication));
    bool room_completion_seen = false;
    for (int spin = 0; spin < 1000 && !room_completion_seen; ++spin)
    {
        const size_t count = critical_command_coordinator_pulse(completions, 8);
        if (count)
        {
            item_movement_transaction_handle_completions(completions, count);
            room_completion_seen = true;
        }
        else
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    assert(room_completion_seen && room_decay_publications == 1);
    assert(item_movement_transaction_health_copy().pending == 1);
    assert(critical_command_coordinator_is_fenced(
        {critical_entity_type::room, 123}, nullptr));
    item_movement_transaction_handle_completions(nullptr, 0);
    assert(room_decay_publications == 2);
    assert(item_movement_transaction_health_copy().pending == 0);
    assert(!critical_command_coordinator_is_fenced(
        {critical_entity_type::room, 123}, nullptr));

    // A room-to-room move holds both owner fences until actorless publication
    // succeeds, then releases them together after acknowledgement.
    rooms[1].number = 456;
    assert(item_movement_transaction_submit_room_move(
        &object, 1, nullptr, 0, room_move_publication, &reject));
    bool move_completion_seen = false;
    for (int spin = 0; spin < 1000 && !move_completion_seen; ++spin)
    {
        const size_t count = critical_command_coordinator_pulse(completions, 8);
        if (count)
        {
            item_movement_transaction_handle_completions(completions, count);
            move_completion_seen = true;
        }
        else
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    assert(move_completion_seen && room_move_publications == 1);
    assert(critical_command_coordinator_is_fenced(
        {critical_entity_type::room, 123}, nullptr));
    assert(critical_command_coordinator_is_fenced(
        {critical_entity_type::room, 456}, nullptr));
    item_movement_transaction_handle_completions(nullptr, 0);
    assert(room_move_publications == 2);
    assert(item_movement_transaction_health_copy().pending == 0);
    assert(!critical_command_coordinator_is_fenced(
        {critical_entity_type::room, 123}, nullptr));
    assert(!critical_command_coordinator_is_fenced(
        {critical_entity_type::room, 456}, nullptr));
    object.extra_flags |= ITEM_ARTIFACT;
    assert(!item_movement_transaction_submit_room_move(
        &object, 1, nullptr, 0, room_move_publication, &reject));
    assert(reject == item_movement_reject::invalid_request);
    object.extra_flags &= ~ITEM_ARTIFACT;
    runtime_entry = previous_runtime;
    object.loc_p = LOC_CARRIED;
    object.loc.carrying = &actor;
    world = nullptr;

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
            rel("item/item_movement_transaction.c"), rel("item/item_transfer_command.c"),
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
