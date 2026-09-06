#!/usr/bin/env python3
"""Contracts for bounded immutable player snapshot capture."""

from _paths import SRC
import re
import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
DTO = (SRC / "player_snapshot.h").read_text()
CAPTURE_HEADER = (SRC / "player_snapshot_capture.h").read_text()
CAPTURE = (SRC / "player_snapshot_capture.c").read_text()
CAPTURE_COMPACT = re.sub(r"\s+", "", CAPTURE)
LOAD_ITEMS = (SRC / "player_load_items.c").read_text()
LOAD_ITEMS_COMPACT = re.sub(r"\s+", "", LOAD_ITEMS)


HARNESS = r'''
#include "player/player_snapshot.h"

#include <cassert>
#include <string>
#include <type_traits>

int main()
{
    static_assert(std::is_move_constructible_v<player_snapshot>);
    static_assert(std::is_move_assignable_v<player_snapshot>);
    static_assert(PLAYER_SNAPSHOT_SCHEMA_VERSION == 1);
    static_assert(PLAYER_SNAPSHOT_MAX_BYTES == 4 * 1024 * 1024);
    static_assert(PLAYER_SNAPSHOT_MAX_OBJECTS < PLAYER_SNAPSHOT_MAX_ROWS);
    static_assert(PLAYER_SNAPSHOT_MAX_DEPTH > 0);

    std::string live_name = "original";
    player_snapshot snapshot = {};
    snapshot.schema_version = PLAYER_SNAPSHOT_SCHEMA_VERSION;
    snapshot.pid = 42;
    snapshot.revision = 9;
    snapshot.components = PLAYER_COMPONENT_STATUS | PLAYER_COMPONENT_INVENTORY;
    snapshot.status_strings.push_back({player_status_string_field::name, live_name});
    snapshot.items.push_back({});
    snapshot.items[0].parent_index = PLAYER_SNAPSHOT_NO_PARENT;
    snapshot.items[0].name = "container";
    snapshot.items.push_back({});
    snapshot.items[1].parent_index = 0;
    snapshot.items[1].name = "nested";

    live_name[0] = 'X';
    assert(snapshot.status_strings[0].value == "original");
    assert(snapshot.items[0].parent_index == PLAYER_SNAPSHOT_NO_PARENT);
    assert(snapshot.items[1].parent_index == 0);

    player_snapshot sealed = std::move(snapshot);
    assert(sealed.pid == 42);
    assert(sealed.revision == 9);
    assert(sealed.items[1].name == "nested");
    return 0;
}
'''


with tempfile.TemporaryDirectory(prefix="duris-player-snapshot-") as temp_dir:
    source = Path(temp_dir) / "snapshot_test.cpp"
    binary = Path(temp_dir) / "snapshot_test"
    source.write_text(HARNESS)
    subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Wpedantic",
            "-Werror",
            "-Isrc",
            str(source),
            "-o",
            str(binary),
        ],
        cwd=ROOT,
        check=True,
        capture_output=True,
        text=True,
    )
    subprocess.run([str(binary)], check=True)

for forbidden in ("P_char", "P_obj", "char_data", "obj_data", "descriptor_data", "room_data"):
    assert forbidden not in DTO, f"live engine type escaped into DTO: {forbidden}"
for required in (
    "schema_version",
    "pid",
    "revision",
    "components",
    "save_intent",
    "room_vnum",
    "encoded_size_bound",
):
    assert required in DTO
for limit in (
    "PLAYER_SNAPSHOT_MAX_BYTES",
    "PLAYER_SNAPSHOT_MAX_ROWS",
    "PLAYER_SNAPSHOT_MAX_OBJECTS",
    "PLAYER_SNAPSHOT_MAX_DEPTH",
    "PLAYER_SNAPSHOT_MAX_STRING_BYTES",
):
    assert limit in DTO and limit in CAPTURE
print("[PASS] DTO metadata and relationships are pointer-free, value-owned, and bounded")

for component in (
    "PLAYER_COMPONENT_STATUS",
    "PLAYER_COMPONENT_LANGUAGES",
    "PLAYER_COMPONENT_INTRODUCTIONS",
    "PLAYER_COMPONENT_TIMERS",
    "PLAYER_COMPONENT_UNDEAD_SLOTS",
    "PLAYER_COMPONENT_FORGED_ITEMS",
    "PLAYER_COMPONENT_GRANTED_COMMANDS",
    "PLAYER_COMPONENT_SKILLS",
    "PLAYER_COMPONENT_AFFECTS",
    "PLAYER_COMPONENT_EQUIPMENT",
    "PLAYER_COMPONENT_INVENTORY",
    "PLAYER_COMPONENT_PETS",
    "PLAYER_COMPONENT_SHAPECHANGES",
    "PLAYER_COMPONENT_TROPHIES",
):
    assert component in CAPTURE
assert "recipes_are_external = true" in CAPTURE
assert "AFFTYPE_NOSAVE" in CAPTURE
assert "ITEM_NORENT" in CAPTURE
for string_flag in ("STRUNG_KEYS", "STRUNG_DESC1", "STRUNG_DESC2", "STRUNG_DESC3"):
    assert string_flag in CAPTURE
assert "save_intent != RENT_CRASH && save_intent != RENT_CRASH2" in CAPTURE
assert "pet->in_room != ch->in_room" in CAPTURE
assert "pet_affects_seen" in CAPTURE
assert "wear_off_message_index" in CAPTURE
assert 'copy_string("SPELLBOOK"' in CAPTURE
assert "spell_ids.push_back" in CAPTURE
print("[PASS] component adapters preserve current replacement, filter, pet, and affect semantics")

for mutation in (
    "unequip_char(",
    "equip_char(",
    "all_affects(",
    "affect_remove(",
    "extract_obj(",
    "obj_from_char(",
    "obj_from_obj(",
):
    assert mutation not in CAPTURE, f"capture mutates live state through {mutation}"
assert "std::unordered_set<const obj_data *> &seen" in CAPTURE
assert "PLAYER_SNAPSHOT_NO_PARENT" in CAPTURE
assert "parent_index" in CAPTURE
assert "row.string_mask=object->str_mask&(STRUNG_KEYS|STRUNG_DESC1|STRUNG_DESC2|STRUNG_DESC3)" in CAPTURE_COMPACT
assert "STRUNG_DESC3|STRUNG_EDESC" in LOAD_ITEMS_COMPACT
ordinary_capture, death_capture = CAPTURE[CAPTURE.index(
    "player_snapshot_capture_result player_snapshot_capture("
):].split("player_death_snapshot_capture(", 1)
for capture in (ordinary_capture, death_capture):
    assert capture.count("*snapshot_out = std::move(snapshot);") == 1
    assert capture.index("player_snapshot snapshot") < capture.index(
        "*snapshot_out = std::move(snapshot);"
    )
print("[PASS] capture is non-mutating, cycle-aware, and atomically publishes one complete DTO")

for forbidden_route in (
    "sql_run_query",
    "db_query",
    "player_revision_mark",
    "player_revision_queue",
    "writeCharacter",
    "sql_save_player",
):
    assert forbidden_route not in CAPTURE
assert "P_char ch" in CAPTURE_HEADER
assert "player_item_snapshot_list_capture" in CAPTURE_HEADER
assert "player_item_snapshot_tree_capture" in CAPTURE_HEADER
assert "bool omit_norent" in CAPTURE_HEADER
assert "capture_items(owner, items, budget, equipment, inventory," in CAPTURE
assert "omit_norent, false)" in CAPTURE
assert "capture_item_tree(root, PLAYER_SNAPSHOT_NO_PARENT, 0, items" in CAPTURE
assert "*items_out = std::move(items);" in CAPTURE
assert CAPTURE.index("capture_items(owner, items, budget, equipment, inventory,") < CAPTURE.index(
    "*items_out = std::move(items);"
)
assert "P_char" not in DTO
print("[PASS] adapter accepts live state only at capture and performs no I/O or queue cutover")

print("immutable player snapshot capture contracts passed")


# Exercise the complete production adapter and codec with live engine structures.
# Engine-only services are inert; custody uses the real runtime implementation.
DEATH_HARNESS = r"""
#include "core/utils.h"
#include "classes/necromancy.h"
#include "core/files.h"
#include "world/vnum.obj.h"
#include "player/player_snapshot_capture.h"
#include "player/player_snapshot_codec.h"
#include "item/item_ownership_runtime.h"
#include <cassert>
#include <cstring>
#include <iostream>

index_data indexes[3] = {};
P_index obj_index = indexes;
P_index mob_index = nullptr;
room_data rooms[1] = {};
P_room world = rooms;
int top_of_objt = 2;
int top_of_mobt = 0;
extern const int top_of_world = 0;
Skill skills[MAX_SKILLS] = {};
bool has_innate(P_char, int) { return false; }
void logit(const char *, const char *, ...) {}
int panic_corruption_int(const char *, const char *, ...) { std::abort(); }

int main()
{
    char_data ch = {};
    pc_only_data pc = {};
    ch.only.pc = &pc;
    pc.pid = 42;
    pc.wallet_revision = 7;
    GET_GOLD(&ch) = 19;
    indexes[0].virtual_number = VOBJ_CORPSE;
    indexes[1].virtual_number = VOBJ_COINS;
    indexes[2].virtual_number = 100;
    rooms[0].number = 22800;
    obj_data corpse = {}, content = {}, refused = {}, wallet = {};
    corpse.obj_uid = 1;
    corpse.type = ITEM_CORPSE;
    corpse.value[CORPSE_PID] = 42;
    corpse.value[CORPSE_SAVEID] = 3;
    corpse.value[CORPSE_FLAGS] = PC_CORPSE;
    corpse.loc_p = LOC_ROOM;
    corpse.loc.room = 0;
    corpse.contains = &content;
    content.obj_uid = 2;
    content.R_num = 2;
    content.loc_p = LOC_INSIDE;
    content.loc.inside = &corpse;
    refused.obj_uid = 3;
    refused.R_num = 2;
    refused.loc_p = LOC_CARRIED;
    refused.loc.carrying = &ch;
    ch.carrying = &refused;
    wallet.obj_uid = 4;
    wallet.R_num = 1;
    wallet.type = ITEM_MONEY;
    wallet.value[2] = 19;
    wallet.loc_p = LOC_NOWHERE;
    const item_owner_identity owner{item_owner_type::player, 42, 0};
    const item_ownership_runtime_entry observation{
        3, 3, 0, owner, 1, 1, 100, item_custody_state::active};
    assert(item_ownership_runtime_hydrate(observation));
    critical_operation_id operation = {};
    operation.bytes[0] = 1;
    player_snapshot output = {};
    auto capture = [&] {
        return player_death_snapshot_capture(&ch, &corpse, &wallet, operation,
                                              10, 22800, {}, &output);
    };
    auto live_intact = [&] {
        assert(GET_GOLD(&ch) == 19 && pc.wallet_revision == 7);
        assert(ch.carrying == &refused && corpse.contains == &content);
        assert(content.loc.inside == &corpse && refused.loc.carrying == &ch);
        assert(OBJ_NOWHERE(&wallet) && wallet.value[2] == 19);
        item_ownership_runtime_entry row = {};
        assert(item_ownership_runtime_lookup(3, &row));
        assert(row.item_revision == 1 && row.owner.id == 42);
    };
    assert(capture() == player_snapshot_capture_result::ok);
    live_intact();
    assert(output.items.empty() && output.death->corpse.size() == 4);
    assert(output.death->custody.size() == 3);
    assert(output.death->wallet_before[2] == 19 && output.death->wallet_pile_uid == 4);
    assert(output.death->corpse[0].parent_index == PLAYER_SNAPSHOT_NO_PARENT);
    for (size_t i = 1; i < 4; ++i) assert(output.death->corpse[i].parent_index == 0);
    for (const auto &row : output.status_integers)
        if (row.field == player_status_field::gold) assert(row.signed_value == 0);
    std::vector<uint8_t> encoded;
    assert(player_snapshot_encode(output, &encoded) == player_snapshot_codec_result::ok);
    player_snapshot decoded;
    assert(player_snapshot_decode(encoded.data(), encoded.size(), &decoded) ==
           player_snapshot_codec_result::ok);
    assert(decoded.death->corpse.size() == 4 && decoded.death->custody.size() == 3);
    auto reject = [&] {
        output.pid = 987;
        assert(capture() == player_snapshot_capture_result::limit_exceeded);
        assert(output.pid == 987 && output.death->corpse.size() == 4);
        live_intact();
    };

    // One row budget includes status, corpse/refused/wallet objects and custody.
    std::vector<int> commands(PLAYER_SNAPSHOT_MAX_ROWS -
        output.status_integers.size() - output.status_strings.size() - 4);
    pc.gcmd_arr = commands.data();
    pc.numb_gcmd = commands.size();
    player_snapshot ordinary;
    assert(player_snapshot_capture(&ch, 10,
        PLAYER_CHECKPOINT_COMPONENT_ALL & ~(PLAYER_COMPONENT_INVENTORY | PLAYER_COMPONENT_EQUIPMENT),
        RENT_DEATH, 22800, &ordinary) == player_snapshot_capture_result::ok);
    assert(player_snapshot_encode(ordinary, &encoded) == player_snapshot_codec_result::ok);
    reject();
    pc.gcmd_arr = nullptr;
    pc.numb_gcmd = 0;

    // Individually valid custody sets combine with status and live item rows.
    for (uint64_t i = 0; i < PLAYER_SNAPSHOT_MAX_OBJECTS; ++i) {
        auto row = observation;
        row.item_uid = row.root_item_uid = 10000 + i;
        row.owner = {item_owner_type::corpse, item_corpse_owner_id(42, 3), 0};
        assert(item_ownership_runtime_hydrate(row));
        if (i + 1 < PLAYER_SNAPSHOT_MAX_OBJECTS) {
            row.item_uid = row.root_item_uid = 20000 + i;
            row.owner = owner;
            assert(item_ownership_runtime_hydrate(row));
        }
    }
    reject();
    item_ownership_runtime_reset();
    assert(item_ownership_runtime_hydrate(observation));

    // These trees fit separately; adding refused inventory and wallet exceeds
    // the common object budget (the original corpse/content remain included).
    std::vector<obj_data> objects(PLAYER_SNAPSHOT_MAX_OBJECTS - 2);
    for (size_t i = 0; i < objects.size(); ++i) {
        objects[i].obj_uid = 100 + i;
        objects[i].R_num = 2;
        objects[i].next_content = i + 1 < objects.size() ? &objects[i + 1] : nullptr;
    }
    content.next_content = objects.data();
    reject();
    content.next_content = nullptr;

    // All strings meet their own bound but the combined trees exceed bytes.
    std::string text(PLAYER_SNAPSHOT_MAX_STRING_BYTES, 'x');
    objects.resize(600);
    for (size_t i = 0; i < objects.size(); ++i) {
        objects[i].next_content = i + 1 < objects.size() ? &objects[i + 1] : nullptr;
        objects[i].str_mask = STRUNG_KEYS | STRUNG_DESC1;
        objects[i].name = objects[i].description = text.data();
    }
    content.next_content = objects.data();
    reject();
    content.next_content = nullptr;

    // A refused root gains a corpse parent: that level counts toward depth.
    objects.assign(PLAYER_SNAPSHOT_MAX_DEPTH - 1, {});
    for (size_t i = 0; i < objects.size(); ++i) {
        objects[i].obj_uid = 100 + i;
        objects[i].R_num = 2;
        objects[i].contains = i + 1 < objects.size() ? &objects[i + 1] : nullptr;
    }
    refused.contains = objects.data();
    reject();
    refused.contains = nullptr;

    std::string oversized(PLAYER_SNAPSHOT_MAX_STRING_BYTES + 1, 'x');
    wallet.str_mask = STRUNG_KEYS;
    wallet.name = oversized.data();
    reject();
    wallet.str_mask = 0;
    assert(capture() == player_snapshot_capture_result::ok);
    live_intact();
    std::cout << "[PASS] real death capture: combined record, row/object/byte/depth/string limits, atomic publication and live asset retention\n";
}
"""

build = ROOT / "bin/tests/player-death-capture"
build.mkdir(parents=True, exist_ok=True)
source = build / "regression.cpp"
binary = build / "regression"
source.write_text(DEATH_HARNESS)
subprocess.run([
    "g++", "-std=c++20", "-g", "-O1", "-ffunction-sections", "-fdata-sections",
    "-Isrc", str(source), "src/player/player_snapshot_capture.c",
    "src/player/player_snapshot_codec.c", "src/item/item_ownership_runtime.c",
    "src/item/item_transfer_command.c", "src/persistence/critical_command.c",
    "-Wl,--gc-sections", "-lcrypto", "-o", str(binary),
], cwd=ROOT, check=True)
subprocess.run([str(binary)], check=True)
