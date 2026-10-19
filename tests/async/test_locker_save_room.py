#!/usr/bin/env python3
"""A character saved inside a locker is saved in the room outside its door.

Locker rooms are made for one visit and nothing restores one after a restart. The legacy
save rewrote the room in the locker's pre-save hook, which queued saves (checkpoints,
terminal saves) do not run, so a character whose last save landed inside a locker came
back stranded in an empty locker room with no exits. The capture now records the room
outside the locker's door.
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

bool training_dummy_capture_target_allowed(P_char) { return true; }

index_data indexes[1] = {};
P_index obj_index = indexes;
P_index mob_index = indexes;
room_data rooms[3] = {};
P_room world = rooms;
int top_of_objt = 0;
int top_of_mobt = 0;
extern const int top_of_world = 2;
Skill skills[MAX_SKILLS] = {};
bool has_innate(P_char, int) { return false; }
void logit(const char *, const char *, ...) {}
int panic_corruption_int(const char *, const char *, ...) { std::abort(); }
P_char get_linked_char(P_char, ush_int) { return nullptr; }

int main() {
    rooms[1].number = 3001;  // outside the locker
    rooms[2].number = 65201; // the locker room, its door north to room 1
    rooms[2].room_flags = ROOM_LOCKER;
    room_direction_data door = {};
    door.to_room = 1;
    rooms[2].dir_option[0] = &door;

    char_data ch = {};
    pc_only_data pc = {};
    ch.only.pc = &pc;
    pc.pid = 42;
    auto saved_room = [&](int room_vnum) {
        player_snapshot snapshot;
        assert(player_snapshot_capture(&ch, 1, PLAYER_COMPONENT_STATUS, RENT_CRASH, room_vnum,
                                       &snapshot) == player_snapshot_capture_result::ok);
        return snapshot.room_vnum;
    };
    ch.in_room = 2;
    assert(saved_room(65201) == 3001); // the locker room itself: saved outside the door
    assert(saved_room(5555) == 5555);  // another room the caller chose is kept
    rooms[2].dir_option[0] = nullptr;
    assert(saved_room(65201) == 65201); // no door to go by: unchanged
    rooms[2].dir_option[0] = &door;
    ch.in_room = 1;
    assert(saved_room(3001) == 3001); // an ordinary room is unchanged
    std::cout << "[PASS] a save inside a locker records the room outside its door\n";
}
'''

with tempfile.TemporaryDirectory(prefix="duris-locker-save-room-") as temporary:
    source = Path(temporary) / "locker_save_room.cpp"
    binary = Path(temporary) / "locker_save_room"
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
