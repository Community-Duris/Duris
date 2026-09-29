#!/usr/bin/env python3
"""Verify exact live forests before a committed component sink is published."""

from pathlib import Path
import subprocess
import tempfile

from _paths import extract_function


def main() -> None:
    capture = extract_function("item_movement_transaction.c", "bool capture(")
    find_item = extract_function("item_movement_transaction.c", "P_obj find_item(")
    ready = extract_function(
        "item_movement_transaction.c", "bool destruction_publication_live_ready("
    )
    program = r'''
#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <new>
#include <unordered_map>
#include <vector>

constexpr size_t ITEM_TRANSFER_MAX_ITEMS = 16;
constexpr size_t MAX_WEAR = 4;
constexpr int LOG_FILE = 1;
struct object;
struct character {
    int pid = 42;
    object *carrying = nullptr;
    object *equipment[MAX_WEAR] = {};
};
struct object {
    uint64_t obj_uid = 0;
    int vnum = 0;
    object *next = nullptr;
    object *next_content = nullptr;
    object *contains = nullptr;
    int loc_p = 0;
    union { character *carrying; character *wearing; object *inside; } loc = {};
};
using P_obj = object *;
using P_char = character *;
#define IS_PC(actor) true
#define GET_PID(actor) ((actor)->pid)
#define OBJ_VNUM(item) ((item)->vnum)
#define OBJ_INSIDE(item) ((item)->loc_p == 3)
#define OBJ_CARRIED_BY(item, actor) ((item)->loc_p == 1 && (item)->loc.carrying == (actor))
#define OBJ_WORN_BY(item, actor) ((item)->loc_p == 2 && (item)->loc.wearing == (actor))
enum class item_owner_type { player, destruction };
enum class item_custody_state { active, destroyed };
enum class item_transfer_reason { destruction, quest_turnin };
struct item_owner_identity { item_owner_type type; uint64_t id; };
struct item_transfer_entry {
    uint64_t item_uid;
    uint64_t root_item_uid;
    uint64_t parent_item_uid;
    uint64_t expected_item_revision;
    int vnum;
    item_custody_state expected_state;
};
struct item_transfer_payload {
    item_owner_identity from_owner;
    item_owner_identity to_owner;
    item_transfer_reason reason;
    uint16_t item_count;
    std::array<item_transfer_entry, ITEM_TRANSFER_MAX_ITEMS> items;
};
struct item_ownership_runtime_entry {
    uint64_t item_uid;
    uint64_t root_item_uid;
    uint64_t parent_item_uid;
    uint64_t item_revision;
    int vnum;
    item_custody_state state;
};
std::unordered_map<uint64_t, item_ownership_runtime_entry> custody;
P_obj object_list = nullptr;
void logit(int, const char *, ...) {}
bool item_ownership_runtime_lookup(uint64_t uid, item_ownership_runtime_entry *entry) {
    auto found = custody.find(uid);
    if (found == custody.end()) return false;
    *entry = found->second;
    return true;
}
''' + capture + "\n" + find_item + "\n" + ready + r'''
int main() {
    character actor;
    object root {10, 7};
    object child {11, 8};
    object second {20, 9};
    root.next = &child;
    child.next = &second;
    root.next_content = &second;
    root.contains = &child;
    root.loc_p = second.loc_p = 1;
    root.loc.carrying = second.loc.carrying = &actor;
    child.loc_p = 3;
    child.loc.inside = &root;
    actor.carrying = &root;
    object_list = &root;
    custody[10] = {10, 10, 0, 3, 7, item_custody_state::active};
    custody[11] = {11, 10, 10, 3, 8, item_custody_state::active};
    custody[20] = {20, 20, 0, 3, 9, item_custody_state::active};
    item_transfer_payload payload = {};
    payload.reason = item_transfer_reason::destruction;
    payload.from_owner = {item_owner_type::player, 42};
    payload.to_owner = {item_owner_type::destruction, 0};
    payload.item_count = 3;
    payload.items[0] = {10, 10, 0, 3, 7, item_custody_state::active};
    payload.items[1] = {11, 10, 10, 3, 8, item_custody_state::active};
    payload.items[2] = {20, 20, 0, 3, 9, item_custody_state::active};
    auto valid = [&] { return destruction_publication_live_ready(&actor, payload); };
    assert(valid());

    object extra {12, 6};
    extra.next = &second;
    extra.loc_p = 3;
    extra.loc.inside = &root;
    child.next = &extra;
    child.next_content = &extra;
    custody[12] = {12, 10, 10, 3, 6, item_custody_state::active};
    assert(!valid());
    child.next = &second;
    child.next_content = nullptr;
    custody.erase(12);
    assert(valid());

    root.contains = nullptr;
    assert(!valid());
    root.contains = &child;
    child.loc.inside = &second;
    assert(!valid());
    child.loc.inside = &root;
    child.vnum = 99;
    assert(!valid());
    child.vnum = 8;

    object duplicate {11, 8};
    second.next = &duplicate;
    assert(!valid());
    second.next = nullptr;
    actor.carrying = &second;
    assert(!valid());
    actor.carrying = &root;
    custody[11].parent_item_uid = 20;
    assert(!valid());
    custody[11].parent_item_uid = 10;
    assert(valid());
}
'''
    with tempfile.TemporaryDirectory() as directory:
        path = Path(directory)
        source_file = path / "spell_component_forest.cpp"
        binary = path / "spell_component_forest"
        source_file.write_text(program, encoding="utf-8")
        subprocess.run(["g++", "-std=c++20", "-Wall", "-Wextra", "-Werror",
                        "-fsanitize=address,undefined", str(source_file), "-o", str(binary)],
                       check=True)
        subprocess.run([str(binary)], check=True)
    print("spell component forest publication: ok")


if __name__ == "__main__":
    main()
