#!/usr/bin/env python3
"""Exercise entry-room choices and missing-room fallbacks before world indexing."""

from _paths import SRC, extract_function
from pathlib import Path
import subprocess
import tempfile


decision = extract_function("account/nanny.c", "static int resolve_entry_room(")
nanny = (SRC / "nanny.c").read_text()
entry = nanny[nanny.index("void enter_game(P_desc d)"):]
assert entry.index("resolve_entry_room(ch, d->rtype, ct)") < entry.index("char_to_room(ch, r_room")

harness = r'''
#include <cassert>
#include <cstdio>
#include <ctime>
constexpr int NOWHERE = -1;
constexpr int RENT_QUIT = 1, RENT_DEATH = 2, RENT_CRASH = 3;
constexpr int PC_TIMER_HEAVEN = 0;
constexpr int GOOD_HEAVEN_ROOM = 400, EVIL_HEAVEN_ROOM = 401;
constexpr int UNDEAD_HEAVEN_ROOM = 402, NEUTRAL_HEAVEN_ROOM = 403;
constexpr int VROOM_CAGE = 404, ZONE_CLOSED = 1;
struct pc_data { time_t pc_timer[1] = {}; };
struct char_data {
    int level = 2, birthplace = 100, home = 0, original_birthplace = 100;
    int in_room = NOWHERE, race = 0;
    bool trusted = false;
    struct { int was_in_room = 200; } specials;
    struct { pc_data *pc = nullptr; } only;
};
using P_char = char_data *;
struct room_data { int zone = 0; bool ship = false; };
struct zone_data { int flags = 0; };
room_data world[10] = {};
zone_data zone_table[2] = {};
int top_of_world = 9;
int forced_gh_room = -2;
int real_room(int vnum)
{
    switch (vnum) {
    case 11: return 1;
    case 100: return 2;
    case 200: return 3;
    case 300: return 4;
    case 400: return 5;
    case 1197: return 8;
    case 1200: return 7;
    default: return NOWHERE;
    }
}
int real_room0(int vnum) { return real_room(vnum); }
int check_gh_home(P_char, int room)
{ return forced_gh_room == -2 ? room : forced_gh_room; }
#define GET_LEVEL(ch) ((ch)->level)
#define GET_BIRTHPLACE(ch) ((ch)->birthplace)
#define GET_HOME(ch) ((ch)->home)
#define GET_ORIG_BIRTHPLACE(ch) ((ch)->original_birthplace)
#define IS_TRUSTED(ch) ((ch)->trusted)
#define IS_RACEWAR_GOOD(ch) ((ch)->race == 1)
#define IS_RACEWAR_EVIL(ch) ((ch)->race == 2)
#define IS_RACEWAR_UNDEAD(ch) ((ch)->race == 3)
#define IS_ILLITHID(ch) ((ch)->race == 4)
#define IS_RACEWAR_NEUTRAL(ch) ((ch)->race == 5)
#define IS_SHIP_ROOM(room) (world[room].ship)
''' + decision + r'''
int main()
{
    pc_data pc;
    char_data ch;
    ch.only.pc = &pc;
    assert(resolve_entry_room(&ch, RENT_CRASH, 10) == 3);
    ch.level = 1;
    assert(resolve_entry_room(&ch, RENT_QUIT, 10) == 2);
    assert(resolve_entry_room(&ch, RENT_DEATH, 10) == 2);
    ch.level = 2;
    pc.pc_timer[0] = 20;
    ch.race = 1;
    assert(resolve_entry_room(&ch, RENT_CRASH, 10) == 5);
    pc.pc_timer[0] = 0;
    ch.race = 0;
    ch.specials.was_in_room = 300;
    ch.birthplace = 999;
    world[4].ship = true;
    assert(resolve_entry_room(&ch, RENT_CRASH, 10) == 8);
    ch.birthplace = 100;
    ch.specials.was_in_room = 200;
    world[3].zone = 1;
    zone_table[1].flags = ZONE_CLOSED;
    assert(resolve_entry_room(&ch, RENT_CRASH, 10) == 2);
    forced_gh_room = NOWHERE;
    assert(resolve_entry_room(&ch, RENT_DEATH, 10) == 1);
    std::puts("entry room resolution passed");
}
'''

with tempfile.TemporaryDirectory(prefix="duris-entry-room-") as directory:
    source = Path(directory) / "entry_room.cpp"
    binary = Path(directory) / "entry_room"
    source.write_text(harness)
    subprocess.run(["g++", "-std=c++20", "-Wall", "-Wextra", "-Werror",
                    str(source), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
