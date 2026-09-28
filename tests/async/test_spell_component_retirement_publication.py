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
    publish = extract_function("magic.c", "bool spell_component_retirement_published(")
    program = r'''
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
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
using item_movement_completion_fn = void (*)(P_char, bool,
    const item_transfer_result &, unsigned int, const uint8_t *, size_t);
std::unordered_map<uint64_t, item_ownership_runtime_entry> custody;
bool item_ownership_runtime_lookup(uint64_t uid, item_ownership_runtime_entry *entry) {
    auto found = custody.find(uid);
    if (found == custody.end()) return false;
    *entry = found->second;
    return true;
}
int extracted = 0, continued = 0, failures = 0, alerts = 0;
void logit(int, const char *, ...) { ++alerts; }
void extract_obj(P_obj item) {
    assert(item);
    P_obj *link = &object_list;
    while (*link && *link != item) link = &(*link)->next;
    assert(*link == item);
    *link = item->next;
    ++extracted;
}
void continue_spell(P_char, bool committed, const item_transfer_result &,
                    unsigned int, const uint8_t *, size_t) {
    if (committed) ++continued;
    else ++failures;
}
''' + context + "\n" + find_item + "\n" + publish + r'''
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
    context.continuation = continue_spell;
    context.item_uids[0] = 10;
    context.item_uids[1] = 20;
    context.item_count = 2;
    item_transfer_result result {10, 2};
    auto publish = [&](bool committed) {
        return spell_component_retirement_published(
            &actor, committed, result, 0,
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
