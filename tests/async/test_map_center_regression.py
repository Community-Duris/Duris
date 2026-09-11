#!/usr/bin/env python3
"""Execute production map coordinate and center-glyph selection code for #196.

World lookup and ordinary room-content inspection are stubbed; this does not
boot the server or exercise terminal/GMCP transport.
"""
from pathlib import Path
import os
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / 'src/world/map.c').read_text(encoding='utf-8')
start = source.index('unsigned int calculate_relative_room(')
coordinate = source[start:source.index('\n/*', start)]
start = source.index('if (x == 0 && y == 0 && ship', source.index('void display_map_room('))
selection = source[start:source.index('\n\t\t\tconst AnsiString &symb', start)]

harness = r'''
#include <cassert>
#include <cstdio>
#include <vector>
struct room_data { int number; int zone; };
struct zone_data { int flags; int real_bottom; int mapx; int mapy; };
std::vector<room_data> world(160010);
zone_data zone_table[1];
#define ZONE_MAP 1
#define IS_SET(flags, bit) ((flags) & (bit))
unsigned int real_room0(int vnum) {
    const int index = vnum - 500000 + 1;
    return index > 0 && index < (int)world.size() && world[index].number == vnum ? index : 0;
}
''' + coordinate + r'''
struct character { int in_room; };
struct vessel { int location; bool docked; };
#define SHIP_DOCKED(ship) ((ship)->docked)
enum { CONTAINS_NOTHING = 0, CONTAINS_CH = 40, CONTAINS_YOUR_SHIP,
       CONTAINS_MAX = 80 };
int inspected_room;
int contents = CONTAINS_NOTHING;
int whats_in_maproom(character *, int room, int, int) {
    inspected_room = room;
    return contents;
}
int glyph(character *ch, vessel *ship, int from_room, int x, int y) {
    int where_rnum = calculate_relative_room(from_room, x, y);
    int distance = 0, show_map_regardless = 0;
    int whats_in, what = 12; // ocean fallback
    inspected_room = -1;
''' + selection + r'''
    return what;
}
int main() {
    for (int i = 1; i < (int)world.size(); ++i)
        world[i] = {500000 + i - 1, 0};
    zone_table[0] = {ZONE_MAP, 1, 400, 400};
    const unsigned int nw = 1, se = 160000;
    assert(calculate_relative_room(nw, 0, 0) == nw);
    assert(calculate_relative_room(se, 0, 0) == se);
    assert(calculate_relative_room(nw, -1, -1) == se);
    assert(calculate_relative_room(se, 1, 1) == nw);
    assert(calculate_relative_room(nw, 1, 0) == 2);
    assert(calculate_relative_room(nw, 0, 1) == 401);
    character ch{1};
    for (int vnum = 660000; vnum <= 660003; ++vnum) {
        unsigned int tail = real_room0(vnum);
        assert(tail && calculate_relative_room(tail, 0, 0) == tail);
        for (int x = -1; x <= 1; ++x)
            for (int y = -1; y <= 1; ++y)
                if (x || y) assert(calculate_relative_room(tail, x, y) == 0);
        ch.in_room = tail;
        assert(glyph(&ch, nullptr, tail, 0, 0) == CONTAINS_CH);
        assert(glyph(&ch, nullptr, tail, 1, 0) == 12);
    }
    // A non-map swimming view has no grid, but still has its own center.
    zone_table[0].flags = 0;
    assert(calculate_relative_room(ch.in_room, 0, 0) == 0);
    assert(glyph(&ch, nullptr, ch.in_room, 0, 0) == CONTAINS_CH);
    assert(glyph(&ch, nullptr, ch.in_room, 1, 0) == 12);
    zone_table[0].flags = ZONE_MAP;
    // Malformed dimensions and a room before the grid must not divide by zero
    // or manufacture neighbors.
    for (int dim : {0, -1}) {
        zone_table[0].mapx = dim;
        assert(calculate_relative_room(nw, 0, 0) == nw);
        assert(calculate_relative_room(nw, 1, 0) == 0);
        zone_table[0].mapx = 400;
        zone_table[0].mapy = dim;
        assert(calculate_relative_room(nw, 0, 0) == nw);
        assert(calculate_relative_room(nw, 0, 1) == 0);
        zone_table[0].mapy = 400;
    }
    world[nw].number = 499999;
    zone_table[0].real_bottom = 2;
    assert(calculate_relative_room(nw, 0, 0) == nw);
    assert(calculate_relative_room(nw, 1, 0) == 0);
    world[nw].number = 500000;
    zone_table[0].real_bottom = 1;
    ch.in_room = nw;
    assert(glyph(&ch, nullptr, nw, 0, 0) == CONTAINS_CH);
    // Remote lookout/quest anchors do not claim the player is at their center.
    contents = 55;
    assert(glyph(&ch, nullptr, 402, 0, 0) == 55);
    assert(inspected_room == 402);
    assert(glyph(&ch, nullptr, nw, 1, 0) == 55);
    assert(inspected_room == 2);
    vessel ship{402, false};
    assert(glyph(&ch, &ship, 402, 0, 0) == CONTAINS_YOUR_SHIP);
    assert(inspected_room == -1);
    ship.docked = true;
    assert(glyph(&ch, &ship, 402, 0, 0) == 55);
    assert(inspected_room == 402);
    ship.docked = false;
    assert(glyph(&ch, &ship, nw, 0, 0) == 55);
    assert(inspected_room == (int)nw);
    assert(glyph(&ch, &ship, 402, 1, 0) == 55);
    puts("PASS: map center, tail boundaries, wrapping, non-map views, ships and remote anchors");
}
'''
build_root = ROOT / 'bin/tests'
build_root.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix='map-center-', dir=build_root) as tmp:
    cpp = Path(tmp) / 'map_center.cpp'
    exe = Path(tmp) / 'map_center'
    cpp.write_text(harness, encoding='utf-8')
    subprocess.run([os.environ.get('CXX', 'g++'), '-std=c++20', '-Wall', '-Wextra',
                    '-Werror', '-fsanitize=undefined', '-fno-sanitize-recover=all',
                    str(cpp), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
