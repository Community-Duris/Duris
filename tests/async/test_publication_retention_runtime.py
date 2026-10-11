#!/usr/bin/env python3
"""Executable shared-movement publication retention regression."""

from pathlib import Path
import subprocess
import tempfile

from _paths import ROOT, rel

HARNESS = r'''
#define DURIS_CHARACTER_IDENTITY_TEST_PANIC_STUB
#include "character_identity_test_fixture.h"
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
int panic_corruption_int(const char *, const char *, ...) { std::abort(); }
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
        assert(command.schema_version == CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION);
        assert(item_transfer_accounting_command_supported(command));
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
    fixture_check_runtime_identity_retirement();
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
    fixture_character_registration identity(&actor);
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

    fixture_retire_character(&actor);
    character_list = nullptr;
    item_movement_transaction_handle_completions(nullptr, 0);
    assert(publication_attempts == 1);
    assert(critical_command_coordinator_health_copy().publication_pending == 1);

    char_data replacement = actor;
    replacement.runtime_id = 7002;
    fixture_register_character(&replacement);
    character_list = &replacement;
    item_movement_transaction_handle_completions(nullptr, 0);
    assert(publication_attempts == 1);
    assert(item_movement_transaction_health_copy().pending == 1);
    assert(critical_command_coordinator_is_fenced(
        {critical_entity_type::item, object.obj_uid}, nullptr));

    fixture_retire_character(&replacement);
    fixture_register_character(&actor);
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
    fixture_retire_character(&actor);
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
    fixture_register_character(&actor);
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
    fixture_retire_character(&actor);
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
    fixture_register_character(&actor);
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
    craft_completed=false;
    for(int spin=0;spin<1000&&!craft_completed;++spin) {
        const size_t count=critical_command_coordinator_pulse(completions,8);
        item_movement_transaction_handle_completions(completions,count);
        craft_completed=count>0;
        if(!craft_completed) std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    assert(craft_completed && recipe_publications==1 && recipe_acknowledgements==0);
    assert(extractions==3 && craft_callbacks==2 && OBJ_CARRIED_BY(&recipe_output,&actor));
    assert(item_movement_transaction_health_copy().pending==1);
    assert(critical_command_coordinator_health_copy().publication_pending==1);
    std::vector<critical_operation_id> pending_recipes;
    assert(item_movement_transaction_pending_craft_progression(1001,&pending_recipes) && pending_recipes.size()==1);
    recipe_progression_ready=true;
    std::this_thread::sleep_for(std::chrono::milliseconds(70));
    item_movement_transaction_handle_completions(nullptr,0);
    assert(extractions==3 && craft_callbacks==3 && recipe_acknowledgements==1);
    assert(item_movement_transaction_health_copy().pending==0);
    assert(item_movement_transaction_pending_craft_progression(1001,&pending_recipes) && pending_recipes.empty());
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

LINKAGE_EXTERNALS = r'''
// Explicit service/world exclusions for these basic give/craft cases.
#include <atomic>
#include <cstdlib>
#include <cstdio>
#include "core/files.h"
#include "core/prototypes.h"
#include "core/structs.h"
#include "economy/currency_transaction.h"
#include "item/held_retirement_recovery.h"
#include "item/item_actions.h"
#include "item/item_movement_transaction.h"
#include "item/item_ownership_runtime.h"
#include "item/ordinary_drop_recovery.h"
#include "net/comm.h"
#include "persistence/critical_command_repository.h"
#include "persistence/economic_sql_item_transfer_transaction.h"
#include "persistence/quest_reward_obligation_repository.h"
#include "player/player_save_pipeline.h"
#include "player/player_snapshot_capture.h"
#include "sql/sql_pool.h"
#include "world/handler.h"
#include "world/native_quest_frozen_continuation.h"
#include "world/quest_mobile_native.h"
#include "world/quest_mobile_native_binding.h"
static std::atomic<size_t> linkage_external_leaf_calls{0};
[[noreturn]] static void linkage_external_leaf_unexpected(const char *symbol) noexcept {
    linkage_external_leaf_calls.fetch_add(1);
    std::fprintf(stderr, "UNSELECTED_EXTERNAL_LEAF_CALLED: %s\n", symbol);
    std::abort();
}
struct linkage_external_leaf_exit_guard {
    ~linkage_external_leaf_exit_guard() {
        if (linkage_external_leaf_calls.load() != 0) std::abort();
    }
};
static linkage_external_leaf_exit_guard linkage_external_leaf_exit_check;
// Same controlled descriptor-list world input, no native observer selected.
P_desc descriptor_list = nullptr;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
int IS_MORPH(P_char ch) {
    linkage_external_leaf_unexpected("IS_MORPH(char_data*)");
}
critical_apply_result critical_command_repository_verify_native_quest_in_transaction(
	MYSQL *connection, const critical_command &command) noexcept {
    linkage_external_leaf_unexpected("critical_command_repository_verify_native_quest_in_transaction(st_mysql*, critical_command const&)");
}
bool currency_transaction_publish_wallet(P_char character, const currency_vector &wallet,
					 uint64_t wallet_revision) {
    linkage_external_leaf_unexpected("currency_transaction_publish_wallet(char_data*, currency_vector const&, unsigned long)");
}
unsigned int economic_sql_native_fee_lock_publication(
	MYSQL *connection, const critical_command &command, const critical_completion &completion,
	std::span<const player_item_snapshot> original_native_before,
	std::span<const player_item_snapshot> original_player_before,
	std::span<const player_item_snapshot> original_player_after,
	uint64_t acknowledged_save_revision, economic_sql_native_fee_publication *output) noexcept {
    linkage_external_leaf_unexpected("economic_sql_native_fee_lock_publication(st_mysql*, critical_command const&, critical_completion const&, std::span<player_item_snapshot const, 18446744073709551615ul>, std::span<player_item_snapshot const, 18446744073709551615ul>, std::span<player_item_snapshot const, 18446744073709551615ul>, unsigned long, economic_sql_native_fee_publication*)");
}
unsigned int economic_sql_native_money_lock_publication(
	MYSQL *connection, const critical_command &command, const critical_completion &completion,
	std::span<const player_item_snapshot> original_native_before,
	std::span<const player_item_snapshot> original_player_before,
	std::span<const player_item_snapshot> original_player_after,
	uint64_t acknowledged_save_revision, economic_sql_native_money_publication *output) noexcept {
    linkage_external_leaf_unexpected("economic_sql_native_money_lock_publication(st_mysql*, critical_command const&, critical_completion const&, std::span<player_item_snapshot const, 18446744073709551615ul>, std::span<player_item_snapshot const, 18446744073709551615ul>, std::span<player_item_snapshot const, 18446744073709551615ul>, unsigned long, economic_sql_native_money_publication*)");
}
unsigned int economic_sql_native_quest_lock_publication(
	MYSQL *connection, const critical_command &command, const critical_completion &completion,
	std::span<const player_item_snapshot> original_native_before,
	std::span<const player_item_snapshot> original_player_before,
	std::span<const player_item_snapshot> original_player_after,
	uint64_t acknowledged_save_revision, economic_sql_native_quest_publication *output) noexcept {
    linkage_external_leaf_unexpected("economic_sql_native_quest_lock_publication(st_mysql*, critical_command const&, critical_completion const&, std::span<player_item_snapshot const, 18446744073709551615ul>, std::span<player_item_snapshot const, 18446744073709551615ul>, std::span<player_item_snapshot const, 18446744073709551615ul>, unsigned long, economic_sql_native_quest_publication*)");
}
bool held_retirement_repository_observe_current(const critical_command &command,
						const critical_completion &completion,
						const held_retirement_recovery &captured,
						held_retirement_current_observation *output) noexcept {
    linkage_external_leaf_unexpected("held_retirement_repository_observe_current(critical_command const&, critical_completion const&, held_retirement_recovery const&, held_retirement_current_observation*)");
}
bool item_actions_object_busy(uint64_t object_uid) {
    linkage_external_leaf_unexpected("item_actions_object_busy(unsigned long)");
}
bool item_ownership_runtime_hydrate_many_atomic(const item_ownership_runtime_entry *batch,
						size_t count) {
    linkage_external_leaf_unexpected("item_ownership_runtime_hydrate_many_atomic(item_ownership_runtime_entry const*, unsigned long)");
}
bool item_ownership_runtime_hydrate_owner(const item_owner_identity &owner, uint64_t revision) {
    linkage_external_leaf_unexpected("item_ownership_runtime_hydrate_owner(item_owner_identity const&, unsigned long)");
}
bool item_ownership_runtime_native_quest_publication_owner::apply(
	const item_transfer_payload &payload, const item_transfer_result &result,
	publication_result phase, std::span<const item_ownership_runtime_entry> current_custody,
	uint64_t current_from_revision, uint64_t current_to_revision,
	uint64_t current_player_revision) noexcept {
    linkage_external_leaf_unexpected("item_ownership_runtime_native_quest_publication_owner::apply(item_transfer_payload const&, item_transfer_result const&, item_ownership_runtime_native_quest_publication_owner::publication_result, std::span<item_ownership_runtime_entry const, 18446744073709551615ul>, unsigned long, unsigned long, unsigned long)");
}
bool item_ownership_runtime_peek_owner_revision(const item_owner_identity &owner,
						uint64_t *revision) noexcept {
    linkage_external_leaf_unexpected("item_ownership_runtime_peek_owner_revision(item_owner_identity const&, unsigned long*)");
}
bool item_ownership_runtime_snapshot_owner(const item_owner_identity &owner, size_t limit,
					   std::vector<item_ownership_runtime_entry> *snapshot) {
    linkage_external_leaf_unexpected("item_ownership_runtime_snapshot_owner(item_owner_identity const&, unsigned long, std::vector<item_ownership_runtime_entry, std::allocator<item_ownership_runtime_entry> >*)");
}
bool item_ownership_runtime_snapshot_root(uint64_t root_item_uid, size_t limit,
					  std::vector<item_ownership_runtime_entry> *snapshot) {
    linkage_external_leaf_unexpected("item_ownership_runtime_snapshot_root(unsigned long, unsigned long, std::vector<item_ownership_runtime_entry, std::allocator<item_ownership_runtime_entry> >*)");
}
// Completion handling selects this even when the native queue is empty.
// Use the fixture's actual game-thread identity, matching its existing
// nevent_require_game_thread seam; workers cannot impersonate that thread.
bool nevent_is_game_thread() {
    return std::this_thread::get_id() == fixture_game_thread;
}
obj_to_char_result obj_to_char_checked(P_obj object, P_char ch) {
    linkage_external_leaf_unexpected("obj_to_char_checked(obj_data*, char_data*)");
}
inert_item_stage_result ordinary_drop_recovery_eligibility(const player_item_snapshot *items,
							   size_t count) noexcept {
    linkage_external_leaf_unexpected("ordinary_drop_recovery_eligibility(player_item_snapshot const*, unsigned long)");
}
ordinary_drop_observation ordinary_drop_recovery_publish_live(
	const critical_command &command, const critical_completion &completion,
	uint64_t actor_runtime_id, ordinary_drop_live_publication_state &state) noexcept {
    linkage_external_leaf_unexpected("ordinary_drop_recovery_publish_live(critical_command const&, critical_completion const&, unsigned long, ordinary_drop_live_publication_state&)");
}
player_snapshot_capture_result
player_item_snapshot_list_capture(P_char owner, bool equipment, bool inventory, bool omit_norent,
				  std::vector<player_item_snapshot> *items_out,
				  size_t *estimated_bytes_out) {
    linkage_external_leaf_unexpected("player_item_snapshot_list_capture(char_data*, bool, bool, bool, std::vector<player_item_snapshot, std::allocator<player_item_snapshot> >*, unsigned long*)");
}
player_snapshot_capture_result
player_item_snapshot_tree_capture_literal(P_obj root, std::vector<player_item_snapshot> *items_out,
					  size_t *estimated_bytes_out) {
    linkage_external_leaf_unexpected("player_item_snapshot_tree_capture_literal(obj_data*, std::vector<player_item_snapshot, std::allocator<player_item_snapshot> >*, unsigned long*)");
}
bool player_save_held_retirement_publication_owner::checkpoint_recovery_context(
	const critical_native_recovery_envelope &expected,
	const critical_native_recovery_envelope &successor) noexcept {
    linkage_external_leaf_unexpected("player_save_held_retirement_publication_owner::checkpoint_recovery_context(critical_native_recovery_envelope const&, critical_native_recovery_envelope const&)");
}
bool player_save_held_retirement_publication_owner::copy_publication_context(
	const critical_command &command, const critical_completion &completion,
	held_retirement_publication_snapshot *output) noexcept {
    linkage_external_leaf_unexpected("player_save_held_retirement_publication_owner::copy_publication_context(critical_command const&, critical_completion const&, held_retirement_publication_snapshot*)");
}
bool player_save_held_retirement_publication_owner::publish_held_retirement(
	const critical_command &command, const critical_completion &completion,
	const held_retirement_publication_snapshot &snapshot,
	bool (*native_publish)(const critical_command &, const critical_completion &,
			       void *) noexcept,
	void *context) noexcept {
    linkage_external_leaf_unexpected("player_save_held_retirement_publication_owner::publish_held_retirement(critical_command const&, critical_completion const&, held_retirement_publication_snapshot const&, bool (*)(critical_command const&, critical_completion const&, void*) noexcept, void*)");
}
bool player_save_held_retirement_publication_owner::rebind_recovery_checkpoint(
	const critical_native_recovery_envelope &envelope, uint64_t runtime_id) noexcept {
    linkage_external_leaf_unexpected("player_save_held_retirement_publication_owner::rebind_recovery_checkpoint(critical_native_recovery_envelope const&, unsigned long)");
}
bool player_save_native_quest_checkpoint_owner::observe_held(
	const player_native_quest_checkpoint_token &token, P_char actor,
	const critical_operation_id &operation, player_native_quest_checkpoint_stage *output,
	std::vector<player_item_snapshot> *items_out) noexcept {
    linkage_external_leaf_unexpected("player_save_native_quest_checkpoint_owner::observe_held(player_native_quest_checkpoint_token const&, char_data*, critical_operation_id const&, player_native_quest_checkpoint_stage*, std::vector<player_item_snapshot, std::allocator<player_item_snapshot> >*)");
}
bool player_save_native_quest_checkpoint_owner::original_held_bodies(
	const critical_command &command, std::vector<player_item_snapshot> *before,
	std::vector<player_item_snapshot> *after,
	player_native_quest_checkpoint_stage *stage) noexcept {
    linkage_external_leaf_unexpected("player_save_native_quest_checkpoint_owner::original_held_bodies(critical_command const&, std::vector<player_item_snapshot, std::allocator<player_item_snapshot> >*, std::vector<player_item_snapshot, std::allocator<player_item_snapshot> >*, player_native_quest_checkpoint_stage*)");
}
critical_submit_result player_save_native_quest_checkpoint_owner::submit_owned(
	const player_native_quest_checkpoint_token &token, critical_command command,
	std::span<const uint8_t> original_native_before, bool *checkpoint_released) noexcept {
    linkage_external_leaf_unexpected("player_save_native_quest_checkpoint_owner::submit_owned(player_native_quest_checkpoint_token const&, critical_command, std::span<unsigned char const, 18446744073709551615ul>, bool*)");
}
bool player_save_native_quest_publication_owner::checkpoint_recovery_context(
	const critical_native_recovery_envelope &expected,
	const critical_native_recovery_envelope &successor) noexcept {
    linkage_external_leaf_unexpected("player_save_native_quest_publication_owner::checkpoint_recovery_context(critical_native_recovery_envelope const&, critical_native_recovery_envelope const&)");
}
bool player_save_native_quest_publication_owner::copy_recovery_context(
	const critical_command &command, critical_native_recovery_envelope *output) noexcept {
    linkage_external_leaf_unexpected("player_save_native_quest_publication_owner::copy_recovery_context(critical_command const&, critical_native_recovery_envelope*)");
}
bool player_save_native_quest_publication_owner::publication_held_bodies(
	const critical_command &command, std::vector<player_item_snapshot> *before,
	std::vector<player_item_snapshot> *after,
	player_native_quest_checkpoint_stage *stage) noexcept {
    linkage_external_leaf_unexpected("player_save_native_quest_publication_owner::publication_held_bodies(critical_command const&, std::vector<player_item_snapshot, std::allocator<player_item_snapshot> >*, std::vector<player_item_snapshot, std::allocator<player_item_snapshot> >*, player_native_quest_checkpoint_stage*)");
}
bool player_save_native_quest_publication_owner::publish_native_quest(
	const critical_command &original, const critical_completion &completion,
	bool (*native_publish)(const critical_command &, const critical_completion &,
			       void *) noexcept,
	void *context) noexcept {
    linkage_external_leaf_unexpected("player_save_native_quest_publication_owner::publish_native_quest(critical_command const&, critical_completion const&, bool (*)(critical_command const&, critical_completion const&, void*) noexcept, void*)");
}
bool player_save_native_quest_publication_owner::rebind_recovery_checkpoint(
	const critical_native_recovery_envelope &original,
	uint64_t actual_player_runtime_id) noexcept {
    linkage_external_leaf_unexpected("player_save_native_quest_publication_owner::rebind_recovery_checkpoint(critical_native_recovery_envelope const&, unsigned long)");
}
bool player_save_pipeline_literal_inventory_hold(const player_literal_inventory_token &token,
						 const critical_operation_id &operation_id) {
    linkage_external_leaf_unexpected("player_save_pipeline_literal_inventory_hold(player_literal_inventory_token const&, critical_operation_id const&)");
}
bool player_save_pipeline_literal_inventory_release(const player_literal_inventory_token &token,
						    const critical_operation_id &operation_id) {
    linkage_external_leaf_unexpected("player_save_pipeline_literal_inventory_release(player_literal_inventory_token const&, critical_operation_id const&)");
}
bool player_save_pipeline_native_quest_checkpoint_cancel(
	const player_native_quest_checkpoint_token &token) {
    linkage_external_leaf_unexpected("player_save_pipeline_native_quest_checkpoint_cancel(player_native_quest_checkpoint_token const&)");
}
bool player_save_pipeline_native_quest_checkpoint_hold(
	const player_native_quest_checkpoint_token &token,
	const critical_operation_id &operation_id) {
    linkage_external_leaf_unexpected("player_save_pipeline_native_quest_checkpoint_hold(player_native_quest_checkpoint_token const&, critical_operation_id const&)");
}
player_literal_inventory_state
player_save_pipeline_native_quest_checkpoint_poll(const player_native_quest_checkpoint_token &token,
						  P_char actor,
						  player_native_quest_checkpoint_stage *stage_out) {
    linkage_external_leaf_unexpected("player_save_pipeline_native_quest_checkpoint_poll(player_native_quest_checkpoint_token const&, char_data*, player_native_quest_checkpoint_stage*)");
}
bool player_save_pipeline_native_quest_checkpoint_release(
	const player_native_quest_checkpoint_token &token,
	const critical_operation_id &operation_id) {
    linkage_external_leaf_unexpected("player_save_pipeline_native_quest_checkpoint_release(player_native_quest_checkpoint_token const&, critical_operation_id const&)");
}
bool player_save_restored_publication_owner::publish(const critical_completion &completion) noexcept {
    linkage_external_leaf_unexpected("player_save_restored_publication_owner::publish(critical_completion const&)");
}
player_snapshot_capture_result player_snapshot_capture_literal_inventory(
	P_char ch, player_revision_t revision, player_component_mask_t components, int save_intent,
	int room_vnum, uint64_t inventory_root_uid, player_snapshot *snapshot_out) {
    linkage_external_leaf_unexpected("player_snapshot_capture_literal_inventory(char_data*, unsigned long, unsigned long, int, int, unsigned long, player_snapshot*)");
}
bool quest_mobile_native_cash_reference_copy(const char_data *character, std::uint64_t runtime,
					     quest_mobile_native_cash_reference *output) noexcept {
    linkage_external_leaf_unexpected("quest_mobile_native_cash_reference_copy(char_data const*, unsigned long, quest_mobile_native_cash_reference*)");
}
bool quest_mobile_native_publication_binding::advance(
	char_data *character, std::uint64_t expected_runtime_id,
	const quest_mobile_native_reference &before,
	const quest_mobile_native_reference &after) noexcept {
    linkage_external_leaf_unexpected("quest_mobile_native_publication_binding::advance(char_data*, unsigned long, quest_mobile_native_reference const&, quest_mobile_native_reference const&)");
}
bool quest_mobile_native_publication_binding::apply_cost(
	char_data *character, uint64_t runtime, const quest_mobile_native_cash_reference &before,
	const native_quest_cost_projection &cost,
	const quest_mobile_native_reference &after) noexcept {
    linkage_external_leaf_unexpected("quest_mobile_native_publication_binding::apply_cost(char_data*, unsigned long, quest_mobile_native_cash_reference const&, native_quest_cost_projection const&, quest_mobile_native_reference const&)");
}
bool quest_mobile_native_publication_binding::apply_money(
	char_data *player, uint64_t player_runtime, uint32_t player_pid, char_data *mobile,
	uint64_t mobile_runtime, const quest_mobile_native_cash_reference &before,
	const native_quest_coin_give_projection &money,
	const quest_mobile_native_reference &after) noexcept {
    linkage_external_leaf_unexpected("quest_mobile_native_publication_binding::apply_money(char_data*, unsigned long, unsigned int, char_data*, unsigned long, quest_mobile_native_cash_reference const&, native_quest_coin_give_projection const&, quest_mobile_native_reference const&)");
}
bool quest_mobile_native_publication_binding::restore_money_metadata(
	char_data *mobile, uint64_t runtime, const quest_mobile_native_image &current,
	const native_mobile_wallet_origin &origin) noexcept {
    linkage_external_leaf_unexpected("quest_mobile_native_publication_binding::restore_money_metadata(char_data*, unsigned long, quest_mobile_native_image const&, native_mobile_wallet_origin const&)");
}
bool quest_mobile_native_reference_copy(const char_data *character,
					std::uint64_t expected_runtime_id,
					quest_mobile_native_reference *output) noexcept {
    linkage_external_leaf_unexpected("quest_mobile_native_reference_copy(char_data const*, unsigned long, quest_mobile_native_reference*)");
}
bool quest_native_coin_give_notice_owner::publish(P_char actor, P_char mobile, uint8_t denomination,
						  int32_t quantity,
						  int32_t original_room_vnum) noexcept {
    linkage_external_leaf_unexpected("quest_native_coin_give_notice_owner::publish(char_data*, char_data*, unsigned char, int, int)");
}
bool quest_native_frozen_continuation_owner::publish(const critical_command &command,
						     const quest_reward_obligation_record &record,
						     uint64_t original_player_runtime_id,
						     bool *started) noexcept {
    linkage_external_leaf_unexpected("quest_native_frozen_continuation_owner::publish(critical_command const&, quest_reward_obligation_record const&, unsigned long, bool*)");
}
bool quest_native_frozen_continuation_owner::published(const critical_command &command) noexcept {
    linkage_external_leaf_unexpected("quest_native_frozen_continuation_owner::published(critical_command const&)");
}
quest_reward_obligation_result
quest_reward_obligation_repository_pending(MYSQL *connection, uint32_t player_pid,
					   std::vector<quest_reward_obligation_record> *obligations,
					   unsigned int *database_error_code,
					   quest_reward_read_metrics *metrics) {
    linkage_external_leaf_unexpected("quest_reward_obligation_repository_pending(st_mysql*, unsigned int, std::vector<quest_reward_obligation_record, std::allocator<quest_reward_obligation_record> >*, unsigned int*, quest_reward_read_metrics*)");
}
int real_room(const int room) {
    linkage_external_leaf_unexpected("real_room(int)");
}
MYSQL *sql_pool_acquire(void) {
    linkage_external_leaf_unexpected("sql_pool_acquire");
}
void sql_pool_discard_connection(MYSQL *conn) {
    linkage_external_leaf_unexpected("sql_pool_discard_connection");
}
void sql_pool_release(MYSQL *conn) {
    linkage_external_leaf_unexpected("sql_pool_release");
}
#ifndef DURIS_CRAFT_ACK_HAS_ACT_FIXTURE
void act(const char *s, int hide, P_char actor, P_obj object, void *victim, int type) {
    linkage_external_leaf_unexpected("act");
}
#endif
P_obj unequip_char(P_char actor, int position, bool defer) {
    linkage_external_leaf_unexpected("unequip_char");
}
#include "mob/studioproclib.h"
proclib_recovery_chain_stage::~proclib_recovery_chain_stage() noexcept
{
	reset();
}
void proclib_recovery_chain_stage::reset() noexcept
{
	free(allocation_);
	allocation_ = nullptr;
	prepared_ = false;
	requests_.clear();
}
#pragma GCC diagnostic pop
'''

with tempfile.TemporaryDirectory(prefix="duris-publication-retention-") as temporary:
    temp = Path(temporary)
    source = temp / "publication_retention_test.cpp"
    binary = temp / "publication_retention_test"
    source.write_text(HARNESS + LINKAGE_EXTERNALS, encoding="utf-8")
    subprocess.run(
        [
            "g++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
            "-D__NO_MYSQL__", "-pthread", "-ffunction-sections", "-fdata-sections",
            "-Isrc", "-Isrc/no_mysql", "-Itests/async", str(source),
            rel("account/character_identity.c"),
            "-g", "-Og", "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
            "-fno-pie", "-no-pie",
            rel("item/item_movement_transaction.c"), rel("item/held_retirement_publication.c"), rel("item/item_transfer_command.c"), rel("world/quest_mobile_native_reference.c"), rel("craft_pouch_mutation.c"), rel("chaos_pouch_ledger.c"), rel("chaos_pouch_publication.c"), rel("player_snapshot_codec.c"),
            rel("item_transfer_accounting.c"), rel("economic_accounting_types.c"),
            rel("economic_accounting_plan.c"), rel("economic_source_event.c"), rel("economic_accounting_intent.c"),
            rel("critical_command.c"), rel("persistence/critical_command_journal.c"),
            rel("persistence/critical_command_coordinator.c"),
            rel("economy/native_quest_cost.c"),
            rel("economy/native_quest_coin_give.c"),
            rel("economy/shop_trade_recovery_manifest.c"),
            rel("world/native_quest_recovery_context.c"),
            rel("economy/native_mobile_birth_recovery.c"),
            rel("economy/native_mobile_birth_result.c"),
            rel("economy/native_mobile_birth_cash_role_result.c"),
            rel("economy/native_mobile_birth_recipe.c"),
            rel("economy/native_mobile_birth_constructor_recipe.c"),
            rel("economy/native_mobile_birth_command.c"),
            rel("economy/native_mobile_birth_cash_role_recipe.c"),
            rel("economy/native_mobile_birth_cash_role_command.c"),
            rel("economy/native_mobile_birth_accounting.c"),
            rel("economy/native_mobile_birth_cash_role_accounting.c"),
            rel("world/quest_mobile_native.c"),
            rel("world/quest_mobile_native_birth.c"),
            rel("economy/zone_reset_item_command.c"),
            rel("item/lockpick_retirement_continuation.c"),
            rel("item/held_retirement_recovery.c"),
            rel("cmd/lockpick_retirement.c"),
            rel("flatfile/flatfile_accounting_native_mobile_birth_ordinary_transaction.c"),
            rel("flatfile/flatfile_accounting_native_mobile_birth_shared_shop_transaction.c"),
            rel("flatfile/flatfile_accounting_zone_reset_item_transaction.c"),
            rel("flatfile/flatfile_shopkeeper_repository.c"),
            rel("persistence/persistence_mode.c"),
            rel("economy/collector_accounting.c"),
            rel("economy/collector_command.c"),
            rel("economy/collector_codec.c"),
            rel("economy/auction_repository.c"),
            rel("economy/auction_accounting.c"),
            rel("economy/auction_listing_accounting.c"),
            rel("economy/auction_item_claim_accounting.c"),
            rel("economy/auction_money_claim_accounting.c"),
            rel("economy/auction_settlement_accounting.c"),
            rel("economy/auction_command.c"),
            rel("economy/auction_native_command_context.c"),
            rel("economy/economic_command_admission.c"),
            rel("economy/economic_currency_adapter.c"),
            rel("economy/currency_command.c"),
            rel("economy/coin_transfer_accounting.c"),
            rel("economy/coin_transfer_command.c"),
            "-Wl,--gc-sections", "-lz", "-lcrypto", "-o", str(binary),
        ],
        cwd=ROOT,
        check=True,
    )
    journal = temp / "journal"
    subprocess.run([str(binary), str(journal)], check=True, timeout=30)

print("shared item movement publication retention passed")
''
