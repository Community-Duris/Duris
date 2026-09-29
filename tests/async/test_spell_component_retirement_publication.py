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
    movement_restore = extract_function(
        "item_movement_transaction.c",
        "bool item_movement_transaction_restore_replayed_command(",
    )
    movement_publish = extract_function("item_movement_transaction.c", "void publish(")
    movement_retry = extract_function("item_movement_transaction.c", "void retry_publications(void)")
    assert "payload.items[index].item_uid" in restore_context
    assert "data[5] != data.size() - 6" in restore_context
    assert "payload.continuation.kind" in restore_context
    assert "if (committed)" in replay_publish
    assert "return spell_component_retirement_published(operation_id, actor, false" in replay_publish
    assert "spell_component_retirement_restore_context(payload" in movement_restore
    assert "spell_component_retirement_replayed_publication" in movement_restore
    assert "spell_component_retirement_waiting_for_effect" in movement_publish
    assert "publication_state::owner_waiting" in movement_publish
    assert "publication_state::owner_waiting" in movement_retry
    program = r'''
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <new>
#include <string>
#include <unordered_map>

struct character { int pid = 42; };
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
constexpr int LOG_FILE = 1;
enum class item_spell_component_effect { faerie_sight = 1, spore_burst_initial, spore_burst_repeat, summon_insects, wall_of_bones, vines };
struct critical_operation_id {
std::array<uint8_t, 16> bytes = {77};
};
enum class spell_component_effect_status { retry, waiting_for_owner, complete };
enum class spell_component_retirement_stage { items_pending, items_retired, effect_pending };
std::unordered_map<std::string, spell_component_retirement_stage> spell_component_retired_items;
std::string spell_component_operation_key(const critical_operation_id &operation_id) {
    return std::string(reinterpret_cast<const char *>(operation_id.bytes.data()), operation_id.bytes.size());
}
enum class item_owner_type { player, destruction };
enum class item_custody_state { active, destroyed };
struct item_owner_identity { item_owner_type type = item_owner_type::player; };
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
uint64_t operation_token_seen = 0;
spell_component_effect_status next_effect_status = spell_component_effect_status::complete;
bool spell_component_retirement_waiting_for_effect(const critical_operation_id &operation_id) {
    const auto found = spell_component_retired_items.find(spell_component_operation_key(operation_id));
    return found != spell_component_retired_items.end() &&
        found->second == spell_component_retirement_stage::effect_pending;
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
spell_component_effect_status spell_faerie_sight_component_completed(const critical_operation_id &o, P_char a, bool c, const item_transfer_result &r, unsigned int e, const uint8_t *p, size_t n) { return continue_spell(o,a,c,r,e,p,n); }
spell_component_effect_status spell_spore_burst_initial_components_completed(const critical_operation_id &o, P_char a, bool c, const item_transfer_result &r, unsigned int e, const uint8_t *p, size_t n) { return continue_spell(o,a,c,r,e,p,n); }
spell_component_effect_status spell_spore_burst_repeat_components_completed(const critical_operation_id &o, P_char a, bool c, const item_transfer_result &r, unsigned int e, const uint8_t *p, size_t n) { return continue_spell(o,a,c,r,e,p,n); }
spell_component_effect_status spell_summon_insects_component_completed(const critical_operation_id &o, P_char a, bool c, const item_transfer_result &r, unsigned int e, const uint8_t *p, size_t n) { return continue_spell(o,a,c,r,e,p,n); }
spell_component_effect_status spell_wall_of_bones_scales_completed(const critical_operation_id &o, P_char a, bool c, const item_transfer_result &r, unsigned int e, const uint8_t *p, size_t n) { return continue_spell(o,a,c,r,e,p,n); }
spell_component_effect_status spell_vines_component_retirement_completed(const critical_operation_id &o, P_char a, bool c, const item_transfer_result &r, unsigned int e, const uint8_t *p, size_t n) { return continue_spell(o,a,c,r,e,p,n); }
''' + context + "\n" + dispatch + "\n" + find_item + "\n" + publish + r'''
int main() {
    character actor;
    character other;
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
    assert(alerts >= 4);
}
'''
    with tempfile.TemporaryDirectory() as directory:
        path = Path(directory)
        source_file = path / "spell_component_retirement.cpp"
        binary = path / "spell_component_retirement"
        source_file.write_text(program, encoding="utf-8")
        subprocess.run(["g++", "-std=c++20", "-Wall", "-Wextra", "-Werror",
                        "-fsanitize=address,undefined", str(source_file), "-o", str(binary)],
                       check=True)
        subprocess.run([str(binary)], check=True)
    print("spell component publication: ok")


if __name__ == "__main__":
    main()
