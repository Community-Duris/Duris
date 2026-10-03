#!/usr/bin/env python3
"""Exercise the live publication boundary after durable spell component retirement."""

from pathlib import Path
import subprocess
import tempfile

from _paths import extract_function, source


def main() -> None:
    magic = source("magic.c").read_text(encoding="utf-8")
    start = magic.index("struct spell_component_retirement_context\n{")
    context = magic[start:magic.index("\n};", start) + 3]
    find_item = extract_function("magic.c", "P_obj spell_component_by_uid(")
    dispatch = extract_function("magic.c", "spell_component_effect_completion_fn spell_component_effect_callback(")
    publish = extract_function("magic.c", "bool spell_component_retirement_published(")
    restore_context = extract_function(
        "magic.c", "bool spell_component_retirement_restore_context("
    )
    replay_publish = extract_function(
        "magic.c", "bool spell_component_retirement_replayed_publication("
    )
    restore_effect = extract_function(
        "magic.c", "bool spell_component_retirement_restore_replayed_effect("
    )
    recover_receipts = extract_function(
        "magic.c", "void spell_component_retirement_recover_receipts("
    )
    save_completed = extract_function(
        "magic.c", "void spell_component_retirement_save_completed("
    )
    effect_applied = extract_function(
        "magic.c", "bool spell_component_retirement_effect_applied("
    )
    mark_effect_applied = extract_function(
        "magic.c", "bool spell_component_retirement_effect_applied_once("
    )
    save_effect = extract_function(
        "magic.c", "spell_component_effect_status\nspell_component_retirement_save_effect("
    )
    bind_owner = extract_function("magic.c", "bool spell_component_retirement_bind_effect_owner(")
    append_owner = extract_function("magic.c", "bool spell_component_retirement_append_owner_operations(")
    pending_save = extract_function("magic.c", "bool spell_component_retirement_pending_save_receipts(")
    faerie_publish = extract_function("ethermancer.c", "spell_component_effect_status\nspell_faerie_sight_component_completed(")
    lifecycle = source("spell_item_lifecycle.h").read_text(encoding="utf-8")
    reader_start = lifecycle.index("struct spell_component_context_reader\n{")
    reader = lifecycle[reader_start:lifecycle.index("\n};", reader_start) + 3]
    movement_restore = extract_function(
        "item_movement_transaction.c",
        "bool item_movement_transaction_restore_replayed_command(",
    )
    movement_publish = extract_function("item_movement_transaction.c", "void publish(")
    movement_retry = extract_function("item_movement_transaction.c", "void retry_publications(void)")
    pending_effects = extract_function(
        "item_movement_transaction.c",
        "bool item_movement_transaction_pending_spell_effects("
    )
    assert "payload.items[index]" in restore_context
    assert "data[5] != data.size() - 6" in restore_context
    assert "payload.continuation.kind" in restore_context
    assert "if (committed)" in replay_publish
    assert "return spell_component_retirement_published(operation_id, actor, false" in replay_publish
    assert "spell_component_retirement_restore_context(" in movement_restore
    assert "spell_component_retirement_replayed_publication" in movement_restore
    assert "spell_component_retirement_waiting_for_effect" in movement_publish
    assert "publication_state::owner_waiting" in movement_publish
    assert "publication_state::owner_waiting" in movement_retry
    assert "spell_component_retirement_restore_replayed_effect(" in movement_restore
    assert "entry.payload.continuation.kind" in pending_effects
    assert "entry.actor_pid != actor_pid" in pending_effects
    assert "spell_component_retirement_recover_receipts(" in extract_function(
        "player_load_materialize.c", "bool player_load_materialize("
    )
    assert "spell_component_retirement_save_completed(" in extract_function(
        "player_save_pipeline.c", "void player_save_pipeline_pulse(void)"
    )
    vines_publish = extract_function(
        "spells.c", "spell_component_effect_status\nspell_vines_component_retirement_completed("
    )
    spells = source("spells.c").read_text(encoding="utf-8")
    vines_start = spells.index("struct vines_component_context\n{")
    vines_context = spells[vines_start:spells.index("\n};", vines_start) + 3]
    vines_production = vines_publish.replace(
        "spell_vines_component_retirement_completed(", "vines_production_completion(", 1
    )
    assert "spell_component_retirement_save_effect(" in vines_publish
    assert "CHAR_RFLAG_LOAD_DEGRADED" in vines_publish
    assert "target_pid" in faerie_publish
    assert "spell_component_retirement_save_effect(" in faerie_publish
    assert "operation_id, victim" in " ".join(faerie_publish.split())
    identity_source = source("character_identity.c").read_text(encoding="utf-8")
    identity = identity_source[identity_source.index("static uint64_t next_runtime_id = 0;"):]
    program = r'''
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <new>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

struct character { int pid = 42; int in_room = 0; uint64_t runtime_id = 0;
    character *next = nullptr; unsigned runtime_flags = 0; bool alive = true; bool pc = true; };
struct object {
    uint64_t obj_uid = 0;
    object *next = nullptr;
    character *carrier = nullptr;
    int vnum = 7;
};
using P_char = character *;
using P_obj = object *;
P_obj object_list = nullptr;
#define OBJ_CARRIED_BY(item, actor) ((item)->carrier == (actor))
#define OBJ_VNUM(item) ((item)->vnum)
#define GET_PID(actor) ((actor)->pid)
#define IS_PC(actor) ((actor)->pc)
#define IS_ALIVE(actor) ((actor)->alive)
#define IS_SET(flags, bit) ((flags) & (bit))
constexpr unsigned CHAR_RFLAG_LOAD_DEGRADED = 1;
constexpr size_t ITEM_MOVEMENT_PENDING_MAX = 1024;
constexpr size_t ITEM_MOVEMENT_CONTEXT_MAX_BYTES = 256;
constexpr size_t ITEM_TRANSFER_MAX_ITEMS = 16;
P_char character_list = nullptr;
static const auto game_thread = std::this_thread::get_id();
bool nevent_require_game_thread(const char *) { return std::this_thread::get_id() == game_thread; }
void panic_corruption(const char *, const char *, ...) { std::abort(); }
@IDENTITY@
constexpr int LOG_FILE = 1;
enum class item_spell_component_effect { faerie_sight = 1, spore_burst_initial, spore_burst_repeat, summon_insects, wall_of_bones, vines };
struct critical_operation_id {
std::array<uint8_t, 16> bytes = {77};
};
struct player_load_spell_effect_receipt { critical_operation_id operation_id; uint32_t effect_id = 0; };
struct player_spell_effect_receipt_snapshot { critical_operation_id operation_id; uint32_t effect_id = 0; };
enum class player_save_pipeline_result { queued, coalesced, unavailable };
constexpr uint64_t PLAYER_COMPONENT_AFFECTS = 1;
constexpr int NOWHERE = -1;
struct room { int number = 100; };
room rooms[2];
room *world = rooms;
enum class spell_component_effect_status { retry, waiting_for_owner, complete };
enum class spell_component_retirement_stage { items_pending, items_retired, effect_pending };
struct spell_component_retirement_state {
    spell_component_retirement_stage stage = spell_component_retirement_stage::items_pending;
    uint32_t actor_pid = 0;
    uint32_t owner_pid = 0;
    item_spell_component_effect effect = item_spell_component_effect::faerie_sight;
    bool effect_applied = false;
    bool save_requested = false;
    bool save_acknowledged = false;
    uint64_t next_save_request_at_usec = 0;
};
std::unordered_map<std::string, spell_component_retirement_state> spell_component_retired_items;
std::string spell_component_operation_key(const critical_operation_id &operation_id) {
    return std::string(reinterpret_cast<const char *>(operation_id.bytes.data()), operation_id.bytes.size());
}
enum class item_owner_type { player, destruction };
enum class item_custody_state { active, destroyed };
struct item_owner_identity { item_owner_type type = item_owner_type::player; uint64_t id = 0; };
enum class item_transfer_reason { destruction };
enum class item_transfer_continuation_kind { spell_component_retirement };
struct item_transfer_entry { uint64_t item_uid = 0, root_item_uid = 0, parent_item_uid = 0; };
struct item_transfer_payload {
    item_owner_identity from_owner;
    item_transfer_reason reason = item_transfer_reason::destruction;
    bool multi_root = true;
    size_t item_count = 1;
    item_transfer_entry items[ITEM_TRANSFER_MAX_ITEMS];
    struct { item_transfer_continuation_kind kind = item_transfer_continuation_kind::spell_component_retirement;
        std::vector<uint8_t> data; } continuation;
};
struct item_ownership_runtime_entry {
    item_owner_identity owner;
    item_custody_state state = item_custody_state::active;
    uint64_t root_item_uid = 0;
    uint64_t parent_item_uid = 0;
    int vnum = 7;
};
struct item_transfer_result { uint64_t root_item_uid = 0; uint16_t item_count = 0; };
using spell_component_effect_completion_fn = spell_component_effect_status (*)(const critical_operation_id &, P_char, bool,
    const item_transfer_result &, unsigned int, const uint8_t *, size_t);
std::unordered_map<uint64_t, item_ownership_runtime_entry> custody;
bool item_ownership_runtime_lookup(uint64_t uid, item_ownership_runtime_entry *entry) {
    auto found = custody.find(uid);
    if (found == custody.end()) return false;
    *entry = found->second;
    return true;
}
int extracted = 0, continued = 0, failures = 0, alerts = 0;
int applied_effects = 0, saves_requested = 0;
int faerie_applications = 0, last_saved_pid = 0;
bool use_receipt_flow = false;
bool save_available = true;
uint64_t current_usec = 1;
uint64_t persistence_observability_now_usec() { return current_usec; }
uint64_t operation_token_seen = 0;
spell_component_effect_status next_effect_status = spell_component_effect_status::complete;
bool spell_component_retirement_effect_applied(const critical_operation_id &);
bool spell_component_retirement_effect_applied_once(const critical_operation_id &);
spell_component_effect_status spell_component_retirement_save_effect(
    const critical_operation_id &, P_char, item_spell_component_effect);
bool spell_component_retirement_bind_effect_owner(const critical_operation_id &, uint32_t, uint32_t, item_spell_component_effect);
player_save_pipeline_result player_save_pipeline_request_spell_effect(
    P_char owner, uint64_t, const player_spell_effect_receipt_snapshot *, int) {
    ++saves_requested;
    last_saved_pid = owner->pid;
    return save_available ? player_save_pipeline_result::queued : player_save_pipeline_result::unavailable;
}
void send_to_char(const char *, P_char) {}
bool spell_component_retirement_waiting_for_effect(const critical_operation_id &operation_id) {
    const auto found = spell_component_retired_items.find(spell_component_operation_key(operation_id));
    return found != spell_component_retired_items.end() &&
        found->second.stage == spell_component_retirement_stage::effect_pending;
}
void logit(int, const char *, ...) { ++alerts; }
void extract_obj(P_obj item) {
    assert(item);
    P_obj *link = &object_list;
    while (*link && *link != item) link = &(*link)->next;
    assert(*link == item);
    *link = item->next;
    ++extracted;
}
spell_component_effect_status continue_spell(const critical_operation_id &operation_id, P_char, bool committed, const item_transfer_result &,
                    unsigned int, const uint8_t *, size_t) {
    operation_token_seen = operation_id.bytes[0];
    if (committed) ++continued;
    else ++failures;
    return next_effect_status;
}
void apply_faerie_sight(int, P_char, P_char victim, int, bool live_consume) {
    assert(victim->pid == 43 && !live_consume);
    ++faerie_applications;
}
constexpr int SPELL_VINES = 445, AFFTYPE_NOSHOW = 1, AFFTYPE_NODISPEL = 2;
constexpr int AFF5_VINES = 4, FALSE = 0, TO_CHAR = 1, TO_NOTVICT = 2;
struct affected_type { int type, flags, bitvector5, duration, modifier; };
int vines_modifier = 0;
void act(const char *, int, P_char, int, int, int) {}
void affect_to_char(P_char, const affected_type *effect) {
    ++applied_effects;
    vines_modifier = effect->modifier;
}
''' + reader + "\n" + faerie_publish + "\n" + vines_context + "\n" + vines_production + r'''
spell_component_effect_status spell_spore_burst_initial_components_completed(const critical_operation_id &o, P_char a, bool c, const item_transfer_result &r, unsigned int e, const uint8_t *p, size_t n) { return continue_spell(o,a,c,r,e,p,n); }
spell_component_effect_status spell_spore_burst_repeat_components_completed(const critical_operation_id &o, P_char a, bool c, const item_transfer_result &r, unsigned int e, const uint8_t *p, size_t n) { return continue_spell(o,a,c,r,e,p,n); }
spell_component_effect_status spell_summon_insects_component_completed(const critical_operation_id &o, P_char a, bool c, const item_transfer_result &r, unsigned int e, const uint8_t *p, size_t n) { return continue_spell(o,a,c,r,e,p,n); }
spell_component_effect_status spell_wall_of_bones_scales_completed(const critical_operation_id &o, P_char a, bool c, const item_transfer_result &r, unsigned int e, const uint8_t *p, size_t n) { return continue_spell(o,a,c,r,e,p,n); }
spell_component_effect_status spell_vines_component_retirement_completed(const critical_operation_id &o, P_char a, bool c, const item_transfer_result &r, unsigned int e, const uint8_t *p, size_t n) {
    if (!use_receipt_flow || !c) return continue_spell(o,a,c,r,e,p,n);
    if (!spell_component_retirement_effect_applied(o)) {
        if (!spell_component_retirement_effect_applied_once(o)) return spell_component_effect_status::retry;
        ++applied_effects;
    }
    return spell_component_retirement_save_effect(o, a, item_spell_component_effect::vines);
}
''' + context + "\n" + dispatch + "\n" + find_item + "\n" + publish + "\n" + restore_context + "\n" + restore_effect + "\n" + bind_owner + "\n" + append_owner + "\n" + "constexpr size_t PLAYER_SPELL_EFFECT_RECEIPT_MAX = 4096;\n" + pending_save + "\n" + recover_receipts + "\n" + save_completed + "\n" + effect_applied + "\n" + mark_effect_applied + "\n" + save_effect + "\n" + replay_publish + r'''
int main() {
    character actor;
    character other;
    actor.runtime_id = allocate_character_runtime_id();
    register_character_runtime_id(&actor);
    character_list = &actor;
    const auto stale_actor_id = actor.runtime_id;
    unregister_character_runtime_id(&actor);
    assert(find_character_by_runtime_id(stale_actor_id) == nullptr);
    actor.runtime_id = allocate_character_runtime_id();
    register_character_runtime_id(&actor);
    assert(find_character_by_runtime_id(stale_actor_id) == nullptr);
    assert(find_character_by_runtime_id(actor.runtime_id) == &actor);
    bool worker_refused = false;
    std::thread worker([&] { worker_refused = find_character_by_runtime_id(actor.runtime_id) == nullptr; });
    worker.join();
    assert(worker_refused);
    character_list = nullptr;
    object first {10, nullptr, &actor};
    object second {20, nullptr, &actor};
    first.next = &second;
    object_list = &first;
    custody[10] = {{item_owner_type::destruction}, item_custody_state::destroyed, 10, 0};
    custody[20] = {{item_owner_type::destruction}, item_custody_state::destroyed, 20, 0};
    spell_component_retirement_context context;
    context.effect = item_spell_component_effect::vines;
    context.item_uids[0] = 10;
    context.item_uids[1] = 20;
    context.item_count = 2;
    item_transfer_result result {10, 2};
    critical_operation_id operation_id {};
    auto publish = [&](bool committed) {
        return spell_component_retirement_published(
            operation_id, &actor, committed, result, 0,
            reinterpret_cast<const uint8_t *>(&context), sizeof(context));
    };

    first.next = nullptr;
    assert(!publish(true) && extracted == 0 && continued == 0);
    first.next = &second;
    second.carrier = &other;
    assert(!publish(true) && extracted == 0 && continued == 0);
    second.carrier = &actor;
    custody[20].state = item_custody_state::active;
    assert(!publish(true) && extracted == 0 && continued == 0);
    custody[20].state = item_custody_state::destroyed;
    second.vnum = 8;
    assert(!publish(true) && extracted == 0 && continued == 0);
    second.vnum = 7;
    result.item_count = 1;
    assert(!publish(true) && extracted == 0 && continued == 0);
    result.item_count = 2;
    assert(publish(true) && extracted == 2 && continued == 1);
    assert(operation_token_seen == 77);
    assert(object_list == nullptr);

    object rejected {30, nullptr, &actor};
    object_list = &rejected;
    context.item_count = 1;
    context.item_uids[0] = 30;
    assert(publish(false) && extracted == 2 && failures == 1);
    object duplicate {30, nullptr, &actor};
    rejected.next = &duplicate;
    custody[30] = {{item_owner_type::destruction}, item_custody_state::destroyed, 30, 0};
    assert(!publish(true) && extracted == 2 && continued == 1);

    operation_id.bytes[0] = 78;
    object deferred {40, nullptr, &actor};
    object_list = &deferred;
    custody[40] = {{item_owner_type::destruction}, item_custody_state::destroyed, 40, 0};
    context.item_uids[0] = 40;
    result.item_count = 1;
    next_effect_status = spell_component_effect_status::waiting_for_owner;
    assert(!publish(true) && extracted == 3 && continued == 2);
    assert(spell_component_retirement_waiting_for_effect(operation_id));
    next_effect_status = spell_component_effect_status::complete;
    assert(publish(true) && extracted == 3 && continued == 3);
    assert(!spell_component_retirement_waiting_for_effect(operation_id));

    use_receipt_flow = true;
    operation_id.bytes[0] = 79;
    object vines {50, nullptr, &actor};
    object_list = &vines;
    custody[50] = {{item_owner_type::destruction}, item_custody_state::destroyed, 50, 0};
    context.item_uids[0] = 50;
    assert(!publish(true) && extracted == 4 && applied_effects == 1 && saves_requested == 1);
    assert(spell_component_retirement_waiting_for_effect(operation_id));
    assert(!publish(true) && applied_effects == 1 && saves_requested == 1);
    player_spell_effect_receipt_snapshot receipt {operation_id, 6};
    spell_component_retirement_save_completed(42, false, &receipt, 1);
    save_available = false;
    assert(!publish(true) && applied_effects == 1 && saves_requested == 1);
    current_usec += 5000000;
    assert(!publish(true) && applied_effects == 1 && saves_requested == 2);
    // Admission failure still exposes the applied effect to a later generic save.
    std::vector<player_spell_effect_receipt_snapshot> pending_save_receipts;
    assert(spell_component_retirement_pending_save_receipts(42, &pending_save_receipts));
    assert(pending_save_receipts.size() == 1 && pending_save_receipts[0].effect_id == 6);
    assert(pending_save_receipts[0].operation_id.bytes == operation_id.bytes);
    pending_save_receipts.clear();
    assert(spell_component_retirement_pending_save_receipts(43, &pending_save_receipts));
    assert(pending_save_receipts.empty());
    save_available = true;
    current_usec += 5000000;
    assert(!publish(true) && applied_effects == 1 && saves_requested == 3);
    player_spell_effect_receipt_snapshot wrong {operation_id, 5};
    spell_component_retirement_save_completed(42, true, &wrong, 1);
    assert(!publish(true) && applied_effects == 1);
    spell_component_retirement_save_completed(42, true, &receipt, 1);
    assert(spell_component_retirement_pending_save_receipts(42, &pending_save_receipts));
    assert(pending_save_receipts.empty());
    assert(publish(true) && applied_effects == 1);

    // A restarted process has only the durable command and the loaded receipt.
    spell_component_retired_items.clear();
    assert(spell_component_retirement_restore_replayed_effect(operation_id, 42, 6, 0));
    player_load_spell_effect_receipt loaded {operation_id, 6};
    spell_component_retirement_recover_receipts(42, &loaded, 1);
    const auto *encoded = reinterpret_cast<const uint8_t *>(&context);
    assert(spell_component_retirement_replayed_publication(
        operation_id, &actor, true, result, 0, encoded, sizeof(context)));
    assert(applied_effects == 1 && saves_requested == 3);

    spell_component_retired_items.clear();
    assert(spell_component_retirement_restore_replayed_effect(operation_id, 42, 6, 0));
    spell_component_retirement_recover_receipts(42, nullptr, 0);
    assert(!spell_component_retirement_replayed_publication(
        operation_id, &actor, true, result, 0, encoded, sizeof(context)));
    assert(applied_effects == 2 && saves_requested == 4);

    // The recipient survives a runtime-ID change and owns the save receipt.
    spell_component_retired_items.clear();
    character recipient;
    recipient.pid = 43;
    recipient.runtime_id = 200;
    register_character_runtime_id(&recipient);
    unregister_character_runtime_id(&recipient);
    ++recipient.runtime_id;
    register_character_runtime_id(&recipient);
    assert(find_character_by_runtime_id(200) == nullptr);
    character_list = &recipient;
    std::vector<uint8_t> terms;
    auto put = [&](uint64_t value, size_t bytes) {
        for (size_t byte = 0; byte < bytes; ++byte) terms.push_back(value >> (8 * byte));
    };
    put(100, 8); put(42, 4); put(50, 4); put(1, 1); put(0, 1); put(43, 4);
    item_transfer_payload payload;
    payload.from_owner.id = 42;
    payload.items[0] = {60, 60, 0};
    payload.continuation.data = {1,1,0,0,0,22};
    payload.continuation.data.insert(payload.continuation.data.end(), terms.begin(), terms.end());
    std::array<uint8_t, ITEM_MOVEMENT_CONTEXT_MAX_BYTES> restored_context = {};
    size_t restored_size = 0;
    uint32_t restored_effect = 0, restored_owner = 0;
    assert(spell_component_retirement_restore_context(payload, &restored_context,
        &restored_size, &restored_effect, &restored_owner));
    assert(restored_effect == 1 && restored_owner == 43);
    operation_id.bytes[0] = 80;
    assert(spell_component_retirement_restore_replayed_effect(operation_id, 42, 1, 43));
    std::vector<critical_operation_id> owner_operations;
    assert(spell_component_retirement_append_owner_operations(43, &owner_operations));
    assert(owner_operations.size() == 1 && owner_operations[0].bytes == operation_id.bytes);
    assert(!spell_component_retirement_replayed_publication(operation_id, &actor, true,
        result, 0, restored_context.data(), restored_size));
    assert(faerie_applications == 1 && last_saved_pid == 43);
    const int saves_before_retry = saves_requested;
    assert(!spell_component_retirement_replayed_publication(operation_id, &actor, true,
        result, 0, restored_context.data(), restored_size));
    assert(faerie_applications == 1 && saves_requested == saves_before_retry);
    player_spell_effect_receipt_snapshot target_receipt {operation_id, 1};
    spell_component_retirement_save_completed(42, true, &target_receipt, 1);
    assert(!spell_component_retirement_replayed_publication(operation_id, &actor, true,
        result, 0, restored_context.data(), restored_size));
    spell_component_retirement_save_completed(43, true, &target_receipt, 1);
    unregister_character_runtime_id(&recipient);
    character_list = nullptr;
    assert(spell_component_retirement_replayed_publication(operation_id, &actor, true,
        result, 0, restored_context.data(), restored_size));
    assert(faerie_applications == 1);

    // An offline target retains publication, then receives it after reconnect.
    assert(spell_component_retirement_restore_replayed_effect(operation_id, 42, 1, 43));
    spell_component_retirement_recover_receipts(43, nullptr, 0);
    assert(!spell_component_retirement_replayed_publication(operation_id, &actor, true,
        result, 0, restored_context.data(), restored_size));
    assert(spell_component_retirement_waiting_for_effect(operation_id));
    register_character_runtime_id(&recipient);
    character_list = &recipient;
    assert(!spell_component_retirement_replayed_publication(operation_id, &actor, true,
        result, 0, restored_context.data(), restored_size));
    assert(faerie_applications == 2 && last_saved_pid == 43);

    spell_component_retired_items.clear();
    operation_id.bytes[0] = 81;
    object dust {60, nullptr, &actor};
    object_list = &dust;
    custody[60] = {{item_owner_type::destruction}, item_custody_state::destroyed, 60, 0};
    context.effect = item_spell_component_effect::faerie_sight;
    context.item_uids[0] = 60;
    context.continuation_context_size = terms.size();
    std::memcpy(context.continuation_context.data(), terms.data(), terms.size());
    assert(!publish(true) && extracted == 5 && faerie_applications == 3);
    assert(last_saved_pid == 43);
    assert(!spell_component_retirement_bind_effect_owner(operation_id, 42, 44,
        item_spell_component_effect::faerie_sight));
    target_receipt.operation_id = operation_id;
    spell_component_retirement_save_completed(43, true, &target_receipt, 1);
    assert(publish(true) && faerie_applications == 3);

    spell_component_retired_items.clear();
    assert(spell_component_retirement_restore_replayed_effect(operation_id, 42, 1, 43));
    player_load_spell_effect_receipt loaded_target {operation_id, 1};
    spell_component_retirement_recover_receipts(43, &loaded_target, 1);
    unregister_character_runtime_id(&recipient);
    character_list = nullptr;
    assert(spell_component_retirement_replayed_publication(operation_id, &actor, true,
        result, 0, restored_context.data(), restored_size));
    assert(faerie_applications == 3);

    // Moving away preserves the existing fizzle outcome with a durable receipt.
    assert(spell_component_retirement_restore_replayed_effect(operation_id, 42, 1, 43));
    recipient.in_room = 1;
    register_character_runtime_id(&recipient);
    character_list = &recipient;
    assert(!spell_component_retirement_replayed_publication(operation_id, &actor, true,
        result, 0, restored_context.data(), restored_size));
    assert(faerie_applications == 3 && last_saved_pid == 43);
    spell_component_retirement_save_completed(43, true, &target_receipt, 1);
    assert(spell_component_retirement_replayed_publication(operation_id, &actor, true,
        result, 0, restored_context.data(), restored_size));

    // Old self casts are recoverable; old other-target casts have no stable PID.
    payload.continuation.data.resize(6 + 18);
    payload.continuation.data[5] = 18;
    payload.continuation.data[6 + 17] = 1;
    assert(spell_component_retirement_restore_context(payload, &restored_context,
        &restored_size, &restored_effect, &restored_owner));
    assert(restored_owner == 42);
    payload.continuation.data[6 + 17] = 0;
    assert(spell_component_retirement_restore_context(payload, &restored_context,
        &restored_size, &restored_effect, &restored_owner));
    assert(restored_owner == 0);
    spell_component_retired_items.clear();
    assert(spell_component_retirement_restore_replayed_effect(operation_id, 42, 1, 0));
    assert(!spell_component_retirement_replayed_publication(operation_id, &actor, true,
        result, 0, restored_context.data(), restored_size));
    assert(faerie_applications == 3);
    assert(alerts >= 4);

    // A retained vines command cannot award strength for unconsumed herbs.
    payload.continuation.data = {1,6,0,0,0,8,50,0,0,0,1,0,0,0};
    payload.item_count = 1;
    assert(spell_component_retirement_restore_context(payload, &restored_context,
        &restored_size, &restored_effect, &restored_owner));
    assert(restored_effect == 6 && restored_owner == 42);
    const auto valid_vines = payload.continuation.data;
    const int effects_before_invalid = applied_effects;
    const int saves_before_invalid = saves_requested;
    for (int32_t count : {0, -1, 2, 5, INT32_MAX}) {
        payload.continuation.data = valid_vines;
        for (size_t byte = 0; byte < 4; ++byte)
            payload.continuation.data[10 + byte] = static_cast<uint32_t>(count) >> (8 * byte);
        restored_size = 123;
        restored_effect = restored_owner = 99;
        assert(!spell_component_retirement_restore_context(payload, &restored_context,
            &restored_size, &restored_effect, &restored_owner));
        assert(restored_size == 123 && restored_effect == 99 && restored_owner == 99);
        if (count != 2) {
            assert(vines_production_completion(operation_id, &actor, true, result, 0,
                payload.continuation.data.data() + 6, 8) == spell_component_effect_status::retry);
        }
        assert(applied_effects == effects_before_invalid && saves_requested == saves_before_invalid);
    }
    for (size_t length : {size_t(0), size_t(7), size_t(9)}) {
        payload.continuation.data = valid_vines;
        payload.continuation.data.resize(6 + length);
        payload.continuation.data[5] = length;
        assert(!spell_component_retirement_restore_context(payload, &restored_context,
            &restored_size, &restored_effect, &restored_owner));
    }
    // Both wire generations retain valid one-to-four herb effects.
    for (int32_t count = 1; count <= 4; ++count) {
        payload.continuation.data = valid_vines;
        payload.continuation.data[10] = count;
        payload.item_count = count;
        for (int32_t index = 0; index < count; ++index) payload.items[index] = {uint64_t(100 + index), uint64_t(100 + index), 0};
        assert(spell_component_retirement_restore_context(payload, &restored_context,
            &restored_size, &restored_effect, &restored_owner));
        payload.continuation.data.erase(payload.continuation.data.begin() + 5);
        payload.continuation.data.erase(payload.continuation.data.begin());
        assert(spell_component_retirement_restore_context(payload, &restored_context,
            &restored_size, &restored_effect, &restored_owner));
        spell_component_retired_items.clear();
        ++operation_id.bytes[0];
        assert(spell_component_retirement_restore_replayed_effect(operation_id, 42, 6, 42));
        assert(vines_production_completion(operation_id, &actor, true, result, 0,
            payload.continuation.data.data() + 4, 8) == spell_component_effect_status::waiting_for_owner);
        assert(vines_modifier == 40 * count);
        const int applied = applied_effects, saved = saves_requested;
        assert(vines_production_completion(operation_id, &actor, true, result, 0,
            payload.continuation.data.data() + 4, 8) == spell_component_effect_status::waiting_for_owner);
        assert(applied_effects == applied && saves_requested == saved);
    }
    // Children in the retired forest are not extra spell components.
    payload.item_count = 10;
    payload.items[0] = {100, 100, 0};
    for (size_t index = 1; index < payload.item_count; ++index)
        payload.items[index] = {100 + index, 100, 100};
    payload.continuation.data = valid_vines;
    assert(spell_component_retirement_restore_context(payload, &restored_context,
        &restored_size, &restored_effect, &restored_owner));
    spell_component_retirement_context forest_context = {};
    std::memcpy(&forest_context, restored_context.data(), restored_size);
    assert(forest_context.item_count == 1 && forest_context.item_uids[0] == 100);
    payload.continuation.data = {1,1,0,0,0,22};
    payload.continuation.data.insert(payload.continuation.data.end(), terms.begin(), terms.end());
    assert(spell_component_retirement_restore_context(payload, &restored_context,
        &restored_size, &restored_effect, &restored_owner));
    assert(restored_owner == 43);
    std::memcpy(&forest_context, restored_context.data(), restored_size);
    assert(forest_context.item_count == 1 && forest_context.item_uids[0] == 100);
    payload.items[9].item_uid = payload.items[8].item_uid;
    assert(!spell_component_retirement_restore_context(payload, &restored_context,
        &restored_size, &restored_effect, &restored_owner));
    payload.items[9].item_uid = 0;
    assert(!spell_component_retirement_restore_context(payload, &restored_context,
        &restored_size, &restored_effect, &restored_owner));
    payload.items[9] = {109, 100, 100};
    payload.items[0].root_item_uid = 99;
    assert(!spell_component_retirement_restore_context(payload, &restored_context,
        &restored_size, &restored_effect, &restored_owner));
    payload.items[0].root_item_uid = 100;
    payload.items[0].parent_item_uid = 109;
    assert(!spell_component_retirement_restore_context(payload, &restored_context,
        &restored_size, &restored_effect, &restored_owner));
    for (size_t index = 0; index < payload.item_count; ++index)
        payload.items[index] = {100 + index, 100 + index, 0};
    assert(!spell_component_retirement_restore_context(payload, &restored_context,
        &restored_size, &restored_effect, &restored_owner));
    unregister_character_runtime_id(&recipient);
    unregister_character_runtime_id(&actor);
    character_list = nullptr;
    assert(character_runtime_index_is_consistent());
}
'''
    program = program.replace("@IDENTITY@", identity)
    with tempfile.TemporaryDirectory() as directory:
        path = Path(directory)
        source_file = path / "spell_component_retirement.cpp"
        binary = path / "spell_component_retirement"
        source_file.write_text(program, encoding="utf-8")
        subprocess.run(["g++", "-std=c++20", "-Wall", "-Wextra", "-Werror",
                        "-g", "-Og", "-pthread", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie",
                        "-fsanitize=address,undefined", str(source_file), "-o", str(binary)],
                       check=True, timeout=120)
        subprocess.run([str(binary)], check=True, timeout=30)
    print("spell component publication: ok")


if __name__ == "__main__":
    main()
