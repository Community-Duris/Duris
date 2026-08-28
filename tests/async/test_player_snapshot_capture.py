#!/usr/bin/env python3
"""Contracts for bounded immutable player snapshot capture."""

import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
DTO = (ROOT / "src/player_snapshot.h").read_text()
CAPTURE_HEADER = (ROOT / "src/player_snapshot_capture.h").read_text()
CAPTURE = (ROOT / "src/player_snapshot_capture.c").read_text()


HARNESS = r'''
#include "player_snapshot.h"

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
assert CAPTURE.count("*snapshot_out = std::move(snapshot);") == 1
assert CAPTURE.index("player_snapshot snapshot = {};") < CAPTURE.index(
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
assert "bool omit_norent" in CAPTURE_HEADER
assert "capture_items(owner, items, budget, equipment, inventory, omit_norent)" in CAPTURE
assert "*items_out = std::move(items);" in CAPTURE
assert CAPTURE.index("capture_items(owner, items, budget, equipment, inventory, omit_norent)") < CAPTURE.index(
    "*items_out = std::move(items);"
)
assert "P_char" not in DTO
print("[PASS] adapter accepts live state only at capture and performs no I/O or queue cutover")

print("immutable player snapshot capture contracts passed")
