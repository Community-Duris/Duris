#!/usr/bin/env python3
"""Native production capture/codec regression for selected literal inventory trees."""
from pathlib import Path
import os
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
HARNESS = r'''
#include "core/utils.h"
#include "classes/necromancy.h"
#include "core/files.h"
#include "player/player_snapshot_capture.h"
#include "player/player_snapshot_codec.h"
#include "player/pet_restore_runtime.h"
#include "item/item_ownership_runtime.h"
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

index_data indexes[2]{};
P_index obj_index = indexes;
P_index mob_index = nullptr;
room_data rooms[1]{};
P_room world = rooms;
int top_of_objt = 1, top_of_mobt = -1;
extern const int top_of_world = 0;
Skill skills[MAX_SKILLS]{};
bool training_dummy_capture_target_allowed(P_char) { std::abort(); }
bool has_innate(P_char, int) { std::abort(); }
P_char get_linked_char(P_char, ush_int) { std::abort(); }
void logit(const char *, const char *, ...) {}
int panic_corruption_int(const char *, const char *, ...) { std::abort(); }

std::vector<uint8_t> bytes(const std::vector<player_item_snapshot> &rows)
{
    std::vector<uint8_t> out;
    assert(player_item_snapshot_list_encode(rows, &out) == player_snapshot_codec_result::ok);
    return out;
}
std::vector<uint8_t> bytes(const player_snapshot &snapshot)
{
    std::vector<uint8_t> out;
    assert(player_snapshot_encode(snapshot, &out) == player_snapshot_codec_result::ok);
    return out;
}
void setup(obj_data &object, uint64_t uid, P_char actor)
{
    object.obj_uid = uid; object.R_num = 1; object.g_key = 90 + uid;
    object.type = ITEM_CONTAINER; object.loc_p = LOC_CARRIED; object.loc.carrying = actor;
    object.name = const_cast<char *>("keys literal");
    object.short_description = const_cast<char *>("short literal");
    object.description = const_cast<char *>("room literal\r\n");
    object.action_description = const_cast<char *>("action literal");
    object.weight = 13; object.cost = 97; object.condition = 21; object.craftsmanship = 3;
    object.wear_flags = 2; object.extra_flags = ITEM_NORENT;
    object.anti_flags = 3; object.anti2_flags = 4; object.extra2_flags = 5;
    object.material = 1;
    object.bitvector = 1; object.bitvector2 = 2; object.bitvector3 = 3;
    object.bitvector4 = 4; object.bitvector5 = 5;
    for (int i = 0; i < 8; ++i) object.value[i] = 101 + i;
    for (int i = 0; i < 6; ++i) object.timer[i] = 201 + i;
    for (int i = 0; i < MAX_OBJ_AFFECT; ++i) {
        object.affected[i].location = 1 + i; object.affected[i].modifier = 301 + i;
    }
}
void assert_fields(const obj_data &live, const player_item_snapshot &row)
{
    assert(row.object_uid == live.obj_uid && row.generated_key == live.g_key);
    assert(row.vnum == indexes[live.R_num].virtual_number && row.type == live.type);
    assert(row.name == (live.name ? live.name : ""));
    assert(row.short_description == (live.short_description ? live.short_description : ""));
    assert(row.description == (live.description ? live.description : ""));
    assert(row.action_description == (live.action_description ? live.action_description : ""));
    assert(row.wear_flags == live.wear_flags && row.extra_flags == live.extra_flags);
    assert(row.anti_flags == live.anti_flags && row.anti2_flags == live.anti2_flags);
    assert(row.extra2_flags == live.extra2_flags && row.weight == live.weight);
    assert(row.material == live.material && row.cost == live.cost);
    assert(row.condition == live.condition && row.craftsmanship == live.craftsmanship);
    assert(row.bitvectors[0] == live.bitvector && row.bitvectors[1] == live.bitvector2);
    assert(row.bitvectors[2] == live.bitvector3 && row.bitvectors[3] == live.bitvector4);
    assert(row.bitvectors[4] == live.bitvector5);
    for (size_t i = 0; i < row.values.size(); ++i) assert(row.values[i] == live.value[i]);
    for (size_t i = 0; i < row.timers.size(); ++i) assert(row.timers[i] == live.timer[i]);
    for (size_t i = 0; i < row.affects.size(); ++i) {
        assert(row.affects[i][0] == live.affected[i].location);
        assert(row.affects[i][1] == live.affected[i].modifier);
    }
    size_t n = 0;
    for (const obj_affect *affect = live.affects; affect; affect = affect->next, ++n) {
        assert(n < row.dynamic_affects.size());
        assert(row.dynamic_affects[n].type == affect->type);
        assert(row.dynamic_affects[n].data == affect->data && row.dynamic_affects[n].extra2 == affect->extra2);
    }
    assert(n == row.dynamic_affects.size()); n = 0;
    for (const extra_descr_data *extra = live.ex_description; extra; extra = extra->next, ++n) {
        assert(n < row.extra_descriptions.size() && !row.extra_descriptions[n].spellbook);
        assert(row.extra_descriptions[n].keyword == extra->keyword);
        assert(row.extra_descriptions[n].description == extra->description);
    }
    assert(n == row.extra_descriptions.size());
}
int main()
{
    indexes[1].virtual_number = 100; rooms[0].number = 22800;
    char_data actor{}; pc_only_data pc{}; actor.only.pc = &pc; pc.pid = 42;
    obj_data root{}, child{}, sibling{}, equipment{};
    setup(root, 10, &actor); setup(child, 11, &actor);
    setup(sibling, 12, &actor); setup(equipment, 13, &actor);
    root.contains = &child; child.loc_p = LOC_INSIDE; child.loc.inside = &root;
    root.next_content = &sibling; actor.carrying = &root;
    equipment.loc_p = LOC_WORN; equipment.loc.wearing = &actor;
    actor.equipment[0] = &equipment;
    obj_affect affect{}; affect.type = 7; affect.data = 9; affect.extra2 = 11;
    root.affects = &affect;
    extra_descr_data extra{}; extra.keyword = const_cast<char *>("literal detail");
    extra.description = const_cast<char *>("literal extra\n"); root.ex_description = &extra;
    const item_owner_identity owner{item_owner_type::player,42,0};
    for (uint64_t uid : {10,11,12,13}) {
        item_ownership_runtime_entry row{uid, uid == 11 ? 10U : uid,
            uid == 11 ? 10U : 0U, owner, 2, 3, 100, item_custody_state::active};
        assert(item_ownership_runtime_hydrate(row));
    }
    player_held_pet_state held; player_pet_snapshot pet{};
    pet.mob_vnum = 1201; pet.hit = pet.max_hit = 10;
    pet.hold_reason = pet_hold_reason::legacy_summon; pet.restore_state = "held";
    pet.items.push_back({}); pet.items[0].parent_index = PLAYER_SNAPSHOT_NO_PARENT;
    pet.items[0].object_uid = 77; pet.items[0].vnum = 100;
    held.pets.push_back(pet); pc.held_pets = &held;
    const auto components = PLAYER_COMPONENT_EQUIPMENT | PLAYER_COMPONENT_INVENTORY |
                            PLAYER_COMPONENT_PETS;
    const auto all_strings = STRUNG_KEYS | STRUNG_DESC1 | STRUNG_DESC2 | STRUNG_DESC3;
    root.str_mask = child.str_mask = all_strings;
    sibling.str_mask = STRUNG_KEYS; equipment.str_mask = STRUNG_DESC2;
    std::vector<player_item_snapshot> expected;
    assert(player_item_snapshot_tree_capture(&root,&expected,nullptr) == player_snapshot_capture_result::ok);
    const auto expected_bytes = bytes(expected);
    root.str_mask = 0; child.str_mask = STRUNG_DESC2;
    std::vector<player_item_snapshot> ordinary, literal;
    size_t estimate = 0;
    assert(player_item_snapshot_tree_capture(&root,&ordinary,nullptr) == player_snapshot_capture_result::ok);
    assert(ordinary[0].string_mask == 0 && ordinary[0].name.empty());
    assert(ordinary[1].string_mask == STRUNG_DESC2 && ordinary[1].name.empty());
    assert(player_item_snapshot_tree_capture_literal(&root,&literal,&estimate) == player_snapshot_capture_result::ok);
    assert(bytes(literal) == expected_bytes && estimate <= PLAYER_SNAPSHOT_MAX_BYTES);
    assert_fields(root,literal[0]); assert_fields(child,literal[1]);
    assert(root.str_mask == 0 && child.str_mask == STRUNG_DESC2);
    assert(root.contains == &child && root.next_content == &sibling && actor.carrying == &root);
    player_snapshot normal{}, scoped{}, zero{};
    assert(player_snapshot_capture(&actor,9,components,RENT_CRASH,22800,&normal) == player_snapshot_capture_result::ok);
    assert(player_snapshot_capture_literal_inventory(&actor,9,components,RENT_CRASH,22800,0,&zero) == player_snapshot_capture_result::ok);
    assert(bytes(normal) == bytes(zero));
    assert(player_snapshot_capture_literal_inventory(&actor,9,components,RENT_CRASH,22800,10,&scoped) == player_snapshot_capture_result::ok);
    assert(scoped.items.size() == 4 && scoped.items[0].object_uid == 13);
    assert(bytes({scoped.items[0]}) == bytes({normal.items[0]}));
    assert(bytes({scoped.items[3]}) == bytes({normal.items[3]}));
    assert(scoped.items[1].string_mask == all_strings && scoped.items[2].string_mask == all_strings);
    assert(scoped.pets[0].items[0].string_mask == 0 && normal.pets[0].items[0].string_mask == 0);
    player_snapshot decoded;
    auto encoded = bytes(scoped);
    assert(player_snapshot_decode(encoded.data(),encoded.size(),&decoded) == player_snapshot_codec_result::ok);
    assert(bytes(decoded) == encoded);
    // Nested sibling topology is captured from the physical links, not prototypes.
    obj_data child_two{}, grandchild{};
    setup(child_two,21,&actor); setup(grandchild,22,&actor);
    child_two.loc_p = grandchild.loc_p = LOC_INSIDE;
    child_two.loc.inside = &root; grandchild.loc.inside = &child;
    child.next_content = &child_two; child.contains = &grandchild;
    assert(player_item_snapshot_tree_capture_literal(&root,&literal,nullptr) == player_snapshot_capture_result::ok);
    assert(literal.size() == 4 && literal[0].object_uid == 10 && literal[1].object_uid == 11);
    assert(literal[2].object_uid == 22 && literal[2].parent_index == 1);
    assert(literal[3].object_uid == 21 && literal[3].parent_index == 0);
    assert_fields(root,literal[0]); assert_fields(child,literal[1]);
    assert_fields(grandchild,literal[2]); assert_fields(child_two,literal[3]);
    for (const auto &row : literal) assert(row.string_mask == all_strings);
    child.next_content = nullptr; child.contains = nullptr;
    assert(player_item_snapshot_tree_capture_literal(&root,&literal,nullptr) == player_snapshot_capture_result::ok);
    const auto tree_sentinel = bytes(literal);
    child.obj_uid = 0; estimate = 909;
    assert(player_item_snapshot_tree_capture_literal(&root,&literal,&estimate) == player_snapshot_capture_result::malformed_source);
    assert(bytes(literal) == tree_sentinel && estimate == 909); child.obj_uid = 11;
    root.next_content = &root;
    assert(player_item_snapshot_tree_capture_literal(&root,&literal,nullptr) == player_snapshot_capture_result::ok);
    assert(bytes(literal) == tree_sentinel); root.next_content = &sibling;
    const auto sentinel = bytes(scoped);
    auto refuse = [&](uint64_t uid, player_snapshot_capture_result result) {
        assert(player_snapshot_capture_literal_inventory(&actor,9,components,RENT_CRASH,22800,uid,&scoped) == result);
        assert(bytes(scoped) == sentinel);
    };
    refuse(99,player_snapshot_capture_result::invalid_identity);
    refuse(13,player_snapshot_capture_result::invalid_identity);
    refuse(11,player_snapshot_capture_result::invalid_identity);
    refuse(UINT64_MAX,player_snapshot_capture_result::invalid_identity);
    assert(player_snapshot_capture_literal_inventory(&actor,9,PLAYER_COMPONENT_INVENTORY,RENT_CRASH,22800,10,&scoped) == player_snapshot_capture_result::invalid_identity);
    sibling.obj_uid = 11; refuse(10,player_snapshot_capture_result::malformed_source); sibling.obj_uid = 12;
    equipment.obj_uid = 10; refuse(10,player_snapshot_capture_result::malformed_source); equipment.obj_uid = 13;
    child.obj_uid = UINT64_MAX; refuse(10,player_snapshot_capture_result::malformed_source); child.obj_uid = 11;
    child.obj_uid = 0; refuse(10,player_snapshot_capture_result::malformed_source); child.obj_uid = 11;
    root.loc_p = LOC_NOWHERE; refuse(10,player_snapshot_capture_result::malformed_source); root.loc_p = LOC_CARRIED;
    child.contains = &root; refuse(10,player_snapshot_capture_result::object_cycle); child.contains = nullptr;
    sibling.next_content = &root; refuse(10,player_snapshot_capture_result::object_cycle); sibling.next_content = nullptr;
    affect.next = &affect; refuse(10,player_snapshot_capture_result::object_cycle); affect.next = nullptr;
    extra.next = &extra; refuse(10,player_snapshot_capture_result::object_cycle); extra.next = nullptr;
    sibling.contains = &child; refuse(10,player_snapshot_capture_result::object_cycle); sibling.contains = nullptr;
    std::string too_long(PLAYER_SNAPSHOT_MAX_STRING_BYTES + 1,'x');
    auto *old_name = root.name; root.name = too_long.data();
    refuse(10,player_snapshot_capture_result::limit_exceeded); root.name = old_name;
    // Depth, object, row and aggregate-byte bounds apply to the selected literal tree.
    std::vector<obj_data> deep(PLAYER_SNAPSHOT_MAX_DEPTH);
    for (size_t i = 0; i < deep.size(); ++i) {
        setup(deep[i],1000+i,&actor);
        if (i+1 < deep.size()) deep[i].contains = &deep[i+1];
    }
    root.contains = deep.data(); refuse(10,player_snapshot_capture_result::limit_exceeded);
    root.contains = &child;
    std::vector<obj_data> wide(PLAYER_SNAPSHOT_MAX_OBJECTS);
    for (size_t i = 0; i < wide.size(); ++i) {
        setup(wide[i],1000+i,&actor);
        if (i+1 < wide.size()) wide[i].next_content = &wide[i+1];
    }
    root.contains = wide.data(); refuse(10,player_snapshot_capture_result::limit_exceeded);
    root.contains = &child;
    std::vector<obj_affect> many_affects(PLAYER_SNAPSHOT_MAX_ROWS);
    for (size_t i = 0; i+1 < many_affects.size(); ++i) many_affects[i].next = &many_affects[i+1];
    root.affects = many_affects.data(); refuse(10,player_snapshot_capture_result::limit_exceeded);
    root.affects = &affect;
    std::string maximum_string(PLAYER_SNAPSHOT_MAX_STRING_BYTES,'x');
    std::vector<obj_data> large(320);
    for (size_t i = 0; i < large.size(); ++i) {
        setup(large[i],1000+i,&actor);
        large[i].name = large[i].short_description = large[i].description = large[i].action_description = maximum_string.data();
        if (i+1 < large.size()) large[i].next_content = &large[i+1];
    }
    root.contains = large.data(); refuse(10,player_snapshot_capture_result::limit_exceeded);
    root.contains = &child;
    // No-rent descendants without custody are included, never silently omitted.
    item_ownership_runtime_reset();
    assert(player_snapshot_capture_literal_inventory(&actor,9,components,RENT_CRASH,22800,10,&scoped) == player_snapshot_capture_result::ok);
    assert(scoped.items.size() == 2 && scoped.items[0].object_uid == 10 && scoped.items[1].object_uid == 11);
    assert(player_snapshot_capture(&actor,9,components,RENT_CRASH,22800,&normal) == player_snapshot_capture_result::ok);
    assert(normal.items.empty());
    // NULL and empty source strings freeze literal empty values, without prototype text.
    root.name = nullptr; root.action_description = const_cast<char *>("");
    assert(player_item_snapshot_tree_capture_literal(&root,&literal,nullptr) == player_snapshot_capture_result::ok);
    assert(literal[0].string_mask == all_strings && literal[0].name.empty() && literal[0].action_description.empty());
    std::cout << "PASS: native selected literal capture, exact bytes/fields, scope isolation, source identity/cycles/strings and atomic refusal\n";
}
'''

with tempfile.TemporaryDirectory(prefix="duris-player-literal-") as directory:
    source = Path(directory) / "regression.cpp"
    binary = Path(directory) / "regression"
    source.write_text(HARNESS)
    flags = ["-std=c++20", "-g", "-O1", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
             "-ffunction-sections", "-fdata-sections", "-Wl,--gc-sections"]
    if os.environ.get("DURIS_TEST_SANITIZERS") == "1":
        flags += ["-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie"]
    subprocess.run(["g++", *flags, "-Isrc", str(source),
        "src/player/player_snapshot_capture.c", "src/player/player_snapshot_codec.c",
        "src/player/pet_restore_state.c", "src/player/pet_restore_runtime.c",
        "src/item/item_ownership_runtime.c", "src/item/item_transfer_command.c", "src/world/quest_mobile_native_reference.c", "src/economy/economic_source_event.c",
        "src/item/craft_pouch_mutation.c", "src/combat/chaos_pouch_ledger.c",
        "src/persistence/critical_command.c", "-lcrypto", "-o", str(binary)],
        cwd=ROOT, check=True, timeout=600)
    subprocess.run([str(binary)], cwd=ROOT, check=True, timeout=30)
