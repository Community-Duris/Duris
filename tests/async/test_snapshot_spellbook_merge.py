#!/usr/bin/env python3
"""A captured item keeps one spellbook marker with every spell its markers hold.

The legacy SQL writers merged an item's spellbook markers into one row, and the game
reads only the first marker. Every snapshot writer (player, pet, corpse, saved item,
private chest and public locker) now gets the same single marker from the capture.
"""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
HARNESS = r'''
#include "core/utils.h"
#include "core/files.h"
#include "core/config.h"
#include "player/player_snapshot_capture.h"
#include <cassert>
#include <iostream>
#include <vector>

bool training_dummy_capture_target_allowed(P_char) { return true; }

index_data indexes[1] = {};
P_index obj_index = indexes;
P_index mob_index = indexes;
room_data rooms[1] = {};
P_room world = rooms;
int top_of_objt = 0;
int top_of_mobt = 0;
extern const int top_of_world = 0;
Skill skills[MAX_SKILLS] = {};
bool has_innate(P_char, int) { return false; }
void logit(const char *, const char *, ...) {}
int panic_corruption_int(const char *, const char *, ...) { std::abort(); }
P_char get_linked_char(P_char, ush_int) { return nullptr; }

int main() {
    indexes[0].virtual_number = 700;
    char marker[] = {3, 1, 3, 0};
    char first_bits[(MAX_SKILLS + 1) / 8 + 1] = {};
    char second_bits[(MAX_SKILLS + 1) / 8 + 1] = {};
    for (int spell : {3, 10})
        first_bits[spell / 8] |= static_cast<char>(1 << (spell % 8));
    for (int spell : {10, 42})
        second_bits[spell / 8] |= static_cast<char>(1 << (spell % 8));
    char note_keyword[] = "note";
    char note_text[] = "A note in the margin.";
    extra_descr_data second = {marker, second_bits, nullptr};
    extra_descr_data note = {note_keyword, note_text, &second};
    extra_descr_data first = {marker, first_bits, &note};
    obj_data book = {};
    book.R_num = 0;
    book.type = ITEM_SPELLBOOK;
    book.ex_description = &first;

    std::vector<player_item_snapshot> items;
    assert(player_item_snapshot_tree_capture(&book, &items, nullptr) ==
           player_snapshot_capture_result::ok);
    assert(items.size() == 1);
    const auto &descriptions = items[0].extra_descriptions;
    assert(descriptions.size() == 2);
    assert(descriptions[0].spellbook && descriptions[0].keyword == "SPELLBOOK");
    assert((descriptions[0].spell_ids == std::vector<int32_t>{3, 10, 42}));
    assert(descriptions[0].description.empty());
    assert(!descriptions[1].spellbook && descriptions[1].keyword == "note" &&
           descriptions[1].description == note_text);
    std::cout << "[PASS] one spellbook marker holds every spell the item's markers hold\n";
}
'''

with tempfile.TemporaryDirectory(prefix="duris-spellbook-merge-") as temporary:
    source = Path(temporary) / "spellbook_merge.cpp"
    binary = Path(temporary) / "spellbook_merge"
    source.write_text(HARNESS)
    subprocess.run([
        "g++", "-std=c++20", "-g", "-O1", "-ffunction-sections", "-fdata-sections",
        "-Isrc", str(source), "src/player/player_snapshot_capture.c",
        "src/player/player_snapshot_codec.c", "src/player/pet_restore_state.c",
        "src/player/pet_restore_runtime.c", "src/item/item_ownership_runtime.c",
        "src/item/item_transfer_command.c", "src/persistence/critical_command.c",
        "-Wl,--gc-sections", "-lcrypto", "-o", str(binary),
    ], cwd=ROOT, check=True)
    subprocess.run([str(binary)], check=True)
