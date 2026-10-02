#!/usr/bin/env python3
"""Executable shared-movement publication retention regression."""

from pathlib import Path
import subprocess
import tempfile

from _paths import ROOT, rel

HARNESS = r'''
#include "core/utils.h"
#include "economy/economic_gameplay_authority.h"
#include "economy/item_transfer_accounting.h"
#include "item/item_movement_transaction.h"
#include "item/item_ownership_runtime.h"
#include "item/item_transfer_command.h"
#include "player/player_snapshot.h"
#include "player/player_snapshot_capture.h"
#include "player/player_snapshot_codec.h"
#include "player/craft_progression_hooks.h"
#include "persistence/persistence_checkpoint.h"

#include <array>
#include <cassert>
#include <chrono>
#include <cstdarg>
#include <cstring>
#include <cstdio>
#include <filesystem>
#include <thread>
#include <vector>

P_char character_list = nullptr;
P_obj object_list = nullptr;
P_room world = nullptr;
P_index obj_index = nullptr;
extern const int top_of_world = 1;

static item_ownership_runtime_entry runtime_entry = {};
static item_ownership_runtime_entry pouch_runtime = {};
static int publication_attempts = 0;
static int completion_calls = 0;
static int craft_callbacks = 0;
static int extractions = 0;
static int restores = 0;
static bool allow_restore = false;
static bool accounting_active = false;
static obj_data restored_output = {};
static critical_apply_outcome forced_outcome = critical_apply_outcome::applied;
static bool publication_malloc_fail = false;
static uint16_t expected_craft_inputs = 1;
static bool recipe_progression_ready = false;
static int recipe_publications = 0, recipe_acknowledgements = 0;
static critical_command recipe_command = {};
bool restore_replay(const critical_command &command, void *) {
    return item_movement_transaction_restore_replayed_command(command);
}
craft_progression_publication_result publish_recipe(const critical_operation_id &, P_char actor,
                                                    const craft_recipe_continuation &terms) {
    assert(actor && terms.player_pid == 1001 && terms.experience == 7000 && terms.output_uid == 5030);
    ++recipe_publications;
    return recipe_progression_ready ? craft_progression_publication_result::ready :
                                    craft_progression_publication_result::waiting;
}
void recipe_acknowledged(const critical_operation_id &) { ++recipe_acknowledgements; }

void *__malloc(size_t size, const char *, const char *, int) { return publication_malloc_fail ? nullptr : std::malloc(size); }
void __free(void *value, const char *, int) { std::free(value); }
char *str_dup(const char *value) { return ::strdup(value); }
void str_free(const char *value) { std::free(const_cast<char *>(value)); }

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
bool player_save_pipeline_sealed_save_pending(int) { return false; }
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
    assert(actor && committed && result.item_count == expected_craft_inputs);
    ++craft_callbacks;
}

bool item_ownership_runtime_lookup(uint64_t item_uid, item_ownership_runtime_entry *entry)
{
    if (!entry)
        return false;
    if (item_uid == pouch_runtime.item_uid) { *entry = pouch_runtime; return true; }
    if (item_uid != runtime_entry.item_uid) return false;
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
bool spell_component_retirement_restore_context(const item_transfer_payload &,
    std::array<uint8_t,ITEM_MOVEMENT_CONTEXT_MAX_BYTES> *,size_t *,uint32_t *,uint32_t *) { assert(false); return false; }
bool spell_component_retirement_restore_replayed_effect(const critical_operation_id &,uint32_t,uint32_t,uint32_t) { assert(false); return false; }
bool spell_component_retirement_replayed_publication(const critical_operation_id &,P_char,bool,
    const item_transfer_result &,unsigned int,const uint8_t *,size_t) { assert(false); return false; }
bool account_reward_retirement_publication(const critical_operation_id &,P_char,bool,
    const item_transfer_result &,unsigned int,const uint8_t *,size_t) { assert(false); return false; }
bool account_reward_duplicate_promotion_publication(const critical_operation_id &,P_char,bool,
    const item_transfer_result &,unsigned int,const uint8_t *,size_t) { assert(false); return false; }
bool collector_transaction_item_busy(uint64_t) { return false; }
bool economic_gameplay_authority::active() { return accounting_active; }
economic_accounting_error economic_gameplay_authority::prepare_item_transfer(
    critical_command *command, uint32_t pid, economic_source_kind source) {
    assert(accounting_active && source == economic_source_kind::crafting);
    critical_operation_id lineage = {}, epoch = {};
    lineage.bytes[0] = 1; epoch.bytes[0] = 2;
    std::vector<uint8_t> intent;
    const auto status = item_transfer_accounting_intent(*command, lineage, epoch, pid,
                                                       &intent, source);
    if (status == economic_accounting_error::ok) {
        command->schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
        command->accounting_intent = std::move(intent);
    }
    return status;
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
    snapshot.vnum = obj_index[object->R_num].virtual_number;
    for (auto extra = object->ex_description; extra; extra = extra->next)
        snapshot.extra_descriptions.push_back({extra->keyword, extra->description, false, {}});
    snapshots->push_back(std::move(snapshot));
    return player_snapshot_capture_result::ok;
}

critical_apply_result apply_transfer(const critical_command &command, void *)
{
    item_transfer_payload payload = {};
    assert(item_transfer_command_decode_payload(command, &payload));
    if (payload.reason == item_transfer_reason::craft) {
        if (accounting_active) {
            assert(command.schema_version == CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION);
            assert(item_transfer_accounting_command_supported(command));
        }
        if (payload.continuation.kind == item_transfer_continuation_kind::craft_recipe)
            recipe_command = command;
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
    index_data indexes[2] = {};
    indexes[0].virtual_number = 42;
    indexes[1].virtual_number = 400300;
    obj_index = indexes;

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

    assert(critical_command_coordinator_init(argv[1], apply_transfer, nullptr, 1,
                                            nullptr, nullptr, item_transfer_accounting_command_supported));
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
    accounting_active = true;
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
    // Retained pouch counters publish after the commit, and a failed allocation
    // leaves both the counter and physical input untouched under the same fence.
    obj_data pouch = {};
    pouch.obj_uid = 4999; pouch.R_num = 1;
    pouch.loc_p = LOC_CARRIED; pouch.loc.carrying = &actor;
    pouch.next = &object;
    object_list = &pouch;
    object.obj_uid = runtime_entry.item_uid;
    object.next = nullptr;
    object.loc_p = LOC_CARRIED; object.loc.carrying = &actor;
    pouch_runtime = {4999,4999,0,destination,1,1,400300,item_custody_state::active};
    const chaos_material_pouch_usage usage = {400291,1};
    expected_craft_inputs = 2;
    assert(item_movement_transaction_submit_craft(&actor, inputs, 1, nullptr, 0,
        400291, craft_callback, nullptr, 0, &reject, &pouch, &usage, 1));
    character_list = nullptr;
    craft_completed = false;
    for (int spin = 0; spin < 1000 && !craft_completed; ++spin) {
        const size_t count = critical_command_coordinator_pulse(completions, 8);
        item_movement_transaction_handle_completions(completions, count);
        craft_completed = count > 0;
        if (!craft_completed) std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    assert(craft_completed && pouch.ex_description == nullptr && extractions == 1 && craft_callbacks == 1);
    assert(critical_command_coordinator_is_fenced({critical_entity_type::item, pouch.obj_uid}, nullptr));
    character_list = &actor;
    publication_malloc_fail = true;
    item_movement_transaction_handle_completions(nullptr, 0);
    assert(pouch.ex_description == nullptr && extractions == 1 && craft_callbacks == 1);
    assert(item_movement_transaction_health_copy().pending == 1);
    publication_malloc_fail = false;
    std::this_thread::sleep_for(std::chrono::milliseconds(70));
    item_movement_transaction_handle_completions(nullptr, 0);
    assert(pouch.obj_uid == 4999 && OBJ_CARRIED_BY(&pouch, &actor));
    assert(pouch.ex_description && std::strcmp(pouch.ex_description->description,"210:1:0;") == 0);
    assert(extractions == 2 && craft_callbacks == 2);
    assert(item_movement_transaction_health_copy().pending == 0);
    assert(!critical_command_coordinator_is_fenced({critical_entity_type::item, pouch.obj_uid}, nullptr));
    item_movement_transaction_handle_completions(nullptr, 0);
    assert(extractions == 2 && craft_callbacks == 2);
    str_free(pouch.ex_description->keyword);
    str_free(pouch.ex_description->description);
    __free(pouch.ex_description,nullptr,0);
    pouch.ex_description = nullptr;
    // Recipe output publication retains the root until progression has its
    // independent player-save ACK. Retrying must not retire inputs twice.
    obj_data recipe_output = {};
    recipe_output.obj_uid = 5030; recipe_output.R_num = 0; recipe_output.loc_p = LOC_NOWHERE;
    object.obj_uid = runtime_entry.item_uid;
    object.loc_p = LOC_CARRIED; object.loc.carrying = &actor; object.next = nullptr;
    recipe_output.next = &object; object_list = &recipe_output;
    runtime_entry.owner = destination;
    expected_craft_inputs = 1;
    craft_progression_hooks.publish = publish_recipe;
    craft_progression_hooks.acknowledged = recipe_acknowledged;
    craft_recipe_continuation recipe;
    recipe.player_pid=1001; recipe.experience=7000; recipe.recipe_vnum=42; recipe.output_uid=5030;
    P_obj missing_output[] = {nullptr};
    assert(!item_movement_transaction_submit_craft(&actor, inputs, 1, missing_output, 1,
        42, craft_callback, nullptr, 0, &reject, nullptr, nullptr, 0,
        chaos_pouch_usage_mode::generated, &recipe));
    assert(reject == item_movement_reject::invalid_request);
    assert(extractions == 2 && recipe_publications == 0 && recipe_acknowledgements == 0);
    assert(item_movement_transaction_health_copy().pending == 0);
    P_obj recipe_outputs[] = {&recipe_output};
    assert(item_movement_transaction_submit_craft(&actor, inputs, 1, recipe_outputs, 1,
        42, craft_callback, nullptr, 0, &reject, nullptr, nullptr, 0,
        chaos_pouch_usage_mode::generated, &recipe));
    assert(item_movement_transaction_player_creation_busy(&actor));
    craft_completed=false;
    for(int spin=0;spin<1000&&!craft_completed;++spin) {
        const size_t count=critical_command_coordinator_pulse(completions,8);
        item_movement_transaction_handle_completions(completions,count);
        craft_completed=count>0;
        if(!craft_completed) std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    assert(craft_completed && recipe_publications==1 && recipe_acknowledgements==0);
    assert(extractions==3 && craft_callbacks==2 && OBJ_CARRIED_BY(&recipe_output,&actor));
    assert(!item_movement_transaction_player_creation_busy(&actor));
    assert(item_movement_transaction_health_copy().pending==1);
    assert(critical_command_coordinator_health_copy().publication_pending==1);
    std::vector<critical_operation_id> pending_recipes;
    assert(item_movement_transaction_pending_craft_progression(1001,&pending_recipes) && pending_recipes.size()==1);
    recipe_progression_ready=true;
    // Make checkpoint creation fail even as root, while retaining the open
    // journal and its durable frame. Restore the same directory for retry.
    const std::string held_journal = std::string(argv[1]) + ".held";
    std::filesystem::rename(argv[1], held_journal);
    FILE *blocked_directory = std::fopen(argv[1], "w");
    assert(blocked_directory && std::fclose(blocked_directory) == 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(70));
    item_movement_transaction_handle_completions(nullptr,0);
    assert(extractions==3 && craft_callbacks==3 && recipe_acknowledgements==0);
    assert(item_movement_transaction_health_copy().publication_ack_pending==1);
    const int published_recipe = recipe_publications;
    character_list = nullptr; // ACK cleanup no longer needs the notified actor.
    for (int retry=0;retry<8;++retry)
        item_movement_transaction_handle_completions(nullptr,0);
    assert(recipe_publications==published_recipe && craft_callbacks==3);
    assert(critical_command_journal_health_copy().records==1);
    std::filesystem::remove(argv[1]);
    std::filesystem::rename(held_journal, argv[1]);
    item_movement_transaction_handle_completions(nullptr,0);
    assert(extractions==3 && craft_callbacks==3 && recipe_acknowledgements==1);
    assert(item_movement_transaction_health_copy().pending==0);
    assert(item_movement_transaction_pending_craft_progression(1001,&pending_recipes) && pending_recipes.empty());
    character_list = &actor;
    // Cold replay of the unchanged legacy wire format must infer the recipe's
    // publication obligation from its validated continuation. No envelope bit
    // existed in schema 1, but an unapplied progression receipt still needs ACK.
    assert(critical_command_coordinator_shutdown());
    item_movement_transaction_reset_for_tests();
    accounting_active = false;
    recipe_command.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
    recipe_command.accounting_intent.clear();
    recipe_command.publication_required = false;
    assert(critical_operation_id_generate(&recipe_command.operation_id));
    std::vector<uint8_t> legacy_bytes;
    assert(critical_command_encode(recipe_command, &legacy_bytes) == critical_command_codec_result::ok);
    critical_command legacy_recipe = {};
    assert(critical_command_decode(legacy_bytes.data(),legacy_bytes.size(),&legacy_recipe) == critical_command_codec_result::ok);
    assert(!legacy_recipe.publication_required);
    bool retain_recipe = false;
    assert(item_transfer_command_replay_publication(legacy_recipe,&retain_recipe) && retain_recipe);
    auto malformed_recipe = legacy_recipe;
    malformed_recipe.payload.pop_back();
    assert(!item_transfer_command_replay_publication(malformed_recipe,&retain_recipe));
    item_transfer_payload unrelated_payload = {};
    assert(item_transfer_command_decode_payload(legacy_recipe,&unrelated_payload));
    unrelated_payload.continuation = {};
    critical_command unrelated_command = {};
    assert(item_transfer_command_build(&unrelated_command,legacy_recipe.operation_id,unrelated_payload,
        critical_source_site::command,critical_deadline_class::interactive));
    assert(item_transfer_command_replay_publication(unrelated_command,&retain_recipe) && !retain_recipe);
    assert(critical_command_journal_init(argv[1]));
    assert(critical_command_journal_append(legacy_recipe) == critical_command_journal_result::ok);
    assert(critical_command_journal_sync() == critical_command_journal_result::ok);
    critical_command_journal_shutdown();
    recipe_progression_ready = false;
    recipe_output.next = nullptr; object_list = &recipe_output;
    for (int restart=0;restart<2;++restart) {
        character_list = nullptr;
        assert(critical_command_coordinator_init(argv[1],apply_transfer,nullptr,1,restore_replay,nullptr,
            item_transfer_accounting_command_supported,item_transfer_command_replay_publication));
        assert(item_movement_transaction_pending_craft_progression(1001,&pending_recipes) && pending_recipes.size()==1);
        craft_completed=false;
        for(int spin=0;spin<1000&&!craft_completed;++spin) {
            const size_t count=critical_command_coordinator_pulse(completions,8);
            item_movement_transaction_handle_completions(completions,count);
            craft_completed=count>0;
            if(!craft_completed) std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        assert(craft_completed && critical_command_coordinator_health_copy().publication_pending==1);
        assert(critical_command_journal_health_copy().records==1);
        character_list=&actor;
        item_movement_transaction_handle_completions(nullptr,0);
        assert(extractions==3 && recipe_acknowledgements==1 && craft_callbacks==3);
        assert(item_movement_transaction_health_copy().pending==1);
        if (!restart) {
            assert(critical_command_coordinator_shutdown());
            item_movement_transaction_reset_for_tests();
            forced_outcome = critical_apply_outcome::already_applied;
        }
    }
    recipe_progression_ready=true;
    item_movement_transaction_handle_completions(nullptr,0);
    assert(recipe_acknowledgements==2 && extractions==3 && craft_callbacks==3);
    assert(critical_command_journal_health_copy().records==0);
    assert(item_movement_transaction_health_copy().pending==0);
    assert(!critical_command_coordinator_is_fenced({critical_entity_type::player,1001},nullptr));
    craft_progression_hooks={};
    accounting_active = false;
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
            rel("item/item_movement_transaction.c"), rel("item/item_transfer_command.c"), rel("craft_pouch_mutation.c"), rel("chaos_pouch_ledger.c"), rel("chaos_pouch_publication.c"), rel("player_snapshot_codec.c"),
            rel("item_transfer_accounting.c"), rel("economic_accounting_types.c"),
            rel("economic_accounting_plan.c"), rel("economic_accounting_intent.c"),
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
