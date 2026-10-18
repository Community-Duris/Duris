#!/usr/bin/env python3
"""char_to_room()'s map update skips a character that is in no room.

When anything moves on a map, char_to_room() walks the descriptors for the players who can
see it and reads each one's room. A character at the account menu after a rent or a quit
is in no room (NOWHERE), so the walk read world[-1] and used what it found there as a zone
index. The real loop runs here under ASan with such a character among the descriptors.
"""
from pathlib import Path
import os
import subprocess
import tempfile

from _paths import ROOT, source

handler = source("handler.c").read_text()
start = handler.index("\t// if anything has moved on the map")
loop = handler[start:handler.index(
    "\tif (ch && ch->desc && ch->desc->term_type == TERM_MSP)", start)]

HARNESS = r'''
#include <cassert>
#include <cstdio>
constexpr int NOWHERE = -1, TERM_MSP = 1, ZONE_MAP = 1, STAT_NORMAL = 0, POS_STANDING = 0;
constexpr int AFF5_FOLLOWING = 1, VROOM_SHIPS_START = 40000, VROOM_SHIPS_END = 49999;
struct room_data { int number; int zone; };
struct zone_data { int flags; int number; };
struct char_data;
struct descriptor_data {
    char_data *character; descriptor_data *next; int term_type; int last_map_update;
    int gmcp_enabled;
};
struct ship_data { int location; };
struct char_data { int in_room; descriptor_data *desc; char_data *following; };
using P_char = char_data *;
using P_desc = descriptor_data *;
using P_ship = ship_data *;
room_data *world;
zone_data zone_table[2] = {{ZONE_MAP, 5000}, {0, 1}};
int top_of_world = 1;
P_desc descriptor_list;
#define IS_SET(flags, bit) ((flags) & (bit))
#define IS_MAP_ROOM(r) (IS_SET(zone_table[world[r].zone].flags, ZONE_MAP))
#define IS_SHIP_ROOM(r) \
    ((world[r].number >= VROOM_SHIPS_START) && (world[r].number <= VROOM_SHIPS_END))
#define GMCP_ENABLED(ch) ((ch) && (ch)->desc && (ch)->desc->gmcp_enabled)
#define CAN_SEE_Z_CORD(sub, obj) true
#define GET_STAT(ch) STAT_NORMAL
#define GET_POS(ch) POS_STANDING
#define CAN_ACT(ch) true
#define IS_AFFECTED5(ch, bit) false
P_ship get_ship_from_char(P_char) { return nullptr; }
int calculate_map_distance(int, int) { return 0; }
int map_view_distance(P_char, int) { return 5; }

void map_update(P_char ch, int was_in)
{
    P_desc d;
    P_char who;
''' + loop + r'''}

int main()
{
    // Room 0 is on a map, room 1 is not.
    world = new room_data[2]{{500000, 0}, {1200, 1}};
    descriptor_data at_menu = {nullptr, nullptr, TERM_MSP, 0, 0};
    char_data rented = {NOWHERE, &at_menu, nullptr};
    at_menu.character = &rented;
    descriptor_data watching = {nullptr, &at_menu, TERM_MSP, 0, 0};
    char_data player = {0, &watching, nullptr};
    watching.character = &player;
    descriptor_list = &watching;

    char_data mob = {0, nullptr, nullptr};
    map_update(&mob, 1);
    assert(watching.last_map_update == 1 && at_menu.last_map_update == 0);
    delete[] world;
    std::puts("the map update skips a character in no room");
}
'''

(ROOT / "bin/tests").mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix="map-observer-", dir=ROOT / "bin/tests") as tmp:
    test, binary = Path(tmp) / "test.cpp", Path(tmp) / "test"
    test.write_text(HARNESS)
    subprocess.run(["g++", "-std=c++20", "-g", "-fsanitize=address,undefined",
                    "-fno-omit-frame-pointer", str(test), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True,
                   env={**os.environ, "ASAN_OPTIONS": "detect_leaks=1:halt_on_error=1"})
