#!/usr/bin/env python3
"""Execute the room-unlink boundary: only accepted removal ends battle presence."""
from pathlib import Path
import subprocess
import tempfile

from _paths import ROOT, extract_function

PRELUDE = r'''
#include <cassert>
#include <cstdarg>
#include <cstddef>
struct char_data;
using P_char = char_data *;
struct char_data {
    int in_room = 0;
    P_char next_in_room = nullptr;
    struct { int was_in_room = -1; P_char fighting = nullptr; } specials;
    bool anchored = false;
};
struct room_data {
    P_char people = nullptr;
    int number = 701;
    const char *name = "fixture room";
    int (*funct)(int, P_char, int, void *) = nullptr;
};
room_data world[1];
constexpr int NOWHERE = -1, CMD_FROMROOM = -75, ROOM_PROC_LEAVE_VETO = 2;
constexpr int LOG_DEBUG = 0, AFF2_CASTING = 0, REAL = 0;
#define J_NAME(ch) "fixture character"
#define GET_NAME(ch) "fixture character"
#define GET_RNUM(ch) 1
#define GET_POS(ch) 1
#define IS_NPC(ch) false
#define IS_AFFECTED2(ch, flag) false
#define GET_HIT(ch) 100
#define GET_MAX_HIT(ch) 100
int hook_result = 0, battle_leaves = 0, activity_leaves = 0;
bool training_dummy_can_leave_room(P_char ch) { return !ch->anchored; }
void logit(int, const char *, ...) {}
void item_actions_character_leaving(P_char) {}
void gmcp_mark_room_dirty(int) {}
void DelCharFromZone(P_char) {}
void StopCasting(P_char) {}
void char_light(P_char) {}
void room_light(int, int) {}
void make_bloodstain(P_char) {}
void world_activity_character_leave(P_char ch) {
    assert(ch->in_room == 0);
    ++activity_leaves;
}
void telemetry_runtime_game_battle_leave(P_char ch) {
    assert(ch->in_room == 0 && ch->specials.was_in_room == 701);
    for (auto *member = world[0].people; member; member = member->next_in_room)
        assert(member != ch);
    ++battle_leaves;
}
int room_hook(int room, P_char ch, int command, void *) {
    assert(room == 0 && ch->in_room == 0 && command == CMD_FROMROOM);
    return hook_result;
}
'''

DRIVER = r'''
int main() {
    char_from_room(nullptr);
    char_data first, second;
    first.next_in_room = &second;
    world[0].people = &first;
    world[0].funct = room_hook;
    first.anchored = true;
    char_from_room(&first);
    assert(world[0].people == &first && first.in_room == 0 && battle_leaves == 0);
    first.anchored = false;
    hook_result = ROOM_PROC_LEAVE_VETO;
    char_from_room(&first);
    assert(world[0].people == &first && first.in_room == 0 && battle_leaves == 0);
    hook_result = 1; // Legacy handled TRUE is an accepted removal.
    char_from_room(&second);
    assert(first.next_in_room == nullptr && second.in_room == NOWHERE);
    assert(battle_leaves == 1 && activity_leaves == 1);
    char_from_room(&second); // Already removed: no second observation.
    assert(battle_leaves == 1);
    char_data absent;
    char_from_room(&absent); // Failed unlink retains presence.
    assert(absent.in_room == 0 && battle_leaves == 1);
    char_from_room(&first);
    assert(world[0].people == nullptr && first.in_room == NOWHERE && !first.next_in_room);
    assert(battle_leaves == 2 && activity_leaves == 2);
}
'''


def main() -> None:
    output = ROOT / "bin/tests"
    output.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="telemetry-room-", dir=output) as directory:
        source = Path(directory) / "room.cc"
        binary = Path(directory) / "room"
        source.write_text(PRELUDE + extract_function("handler.c", "void char_from_room(P_char ch)") + DRIVER)
        subprocess.run(["g++", "-std=c++20", "-Wall", "-Wextra", "-Werror", "-g",
            "-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-fno-omit-frame-pointer",
            "-fno-pie", "-no-pie", str(source), "-o", str(binary)], check=True, timeout=60)
        subprocess.run([str(binary)], check=True, timeout=30)
    print("accepted/refused room unlink preserves exact battle presence boundary")


if __name__ == "__main__":
    main()
