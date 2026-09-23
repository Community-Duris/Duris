#!/usr/bin/env python3
"""Production zone_purge and die_follower with recursive extraction under sanitizers.

zone_purge() also keeps ferry ships and ticket automats: the zone reset that follows
won't load them back, and each Ferry holds a pointer to its ship.
"""
from pathlib import Path
import subprocess
import tempfile
from _paths import ROOT, extract_function

PREFIX = r'''
#include "core/prototypes.h"
#include "core/utils.h"
#include "world/db.h"
#include "world/vnum.obj.h"
#include <cassert>
#include <cstdio>
#include <map>
#include <set>
#include <vector>

static room_data rooms[2] = {};
P_room world = rooms;
static zone_data zones[1] = {};
zone_data *zone_table = zones;
static std::map<uint64_t, int> removed;
static std::set<P_char> morphs;
static std::vector<P_char> retained_allocations;
static P_char master, pet, survivor, mover, replacement;
static bool free_immediately = false, alter_room = false;
static follow_type follower = {};
static int object_extractions = 0;
static P_obj ferry_ship = nullptr;

int IS_MORPH(P_char ch) { return morphs.contains(ch); }
int real_object(const int) { return 42; }
// Only init_ferries() loads a ferry's ship and automats, so the reset can't restore them.
bool is_ferry_object(P_obj obj) { return obj && obj == ferry_ship; }
P_char get_linked_char(P_char ch, ush_int) { return ch == pet ? master : nullptr; }
void stop_follower(P_char ch) {
    if (ch->following) ch->following->followers = nullptr;
    ch->following = nullptr;
}
static void remove_from_room(P_char ch) {
    P_char *at = &world[ch->in_room].people;
    while (*at && *at != ch) at = &(*at)->next_in_room;
    assert(*at == ch);
    *at = ch->next_in_room;
    ch->next_in_room = nullptr;
    ch->in_room = NOWHERE;
}
void extract_char(P_char ch) {
    assert(++removed[ch->runtime_id] == 1);
    assert(IS_ALIVE(ch));
    if (ch->followers || ch->following) die_follower(ch);
    remove_from_room(ch);
    for (auto current = world[0].people; current; current = current->next_in_room)
        if (current->specials.fighting == ch) current->specials.fighting = nullptr;
    if (ch == master && alter_room) {
        remove_from_room(mover);
        mover->in_room = 1;
        world[1].people = mover;
        ++replacement->runtime_id; // Reused storage is a different character.
    }
    SET_POS(ch, STAT_DEAD);
    ch->only.npc = nullptr;
    if (free_immediately) delete ch;
    else retained_allocations.push_back(ch);
}
void die(P_char, P_char ch) { extract_char(ch); }
void extract_obj(P_obj obj, bool) {
    ++object_extractions;
    P_obj *at = &world[0].contents;
    while (*at && *at != obj) at = &(*at)->next_content;
    assert(*at == obj);
    *at = obj->next_content;
}
static P_char make_character(uint64_t id, bool npc = true) {
    P_char ch = new char_data{};
    ch->runtime_id = id;
    ch->in_room = 0;
    if (npc) ch->specials.act = ACT_ISNPC;
    SET_POS(ch, STAT_NORMAL + POS_STANDING);
    return ch;
}
'''
SUFFIX = r'''
static void run_case(bool immediate, bool move) {
    free_immediately = immediate; alter_room = move;
    removed.clear(); morphs.clear(); retained_allocations.clear();
    rooms[0] = {}; rooms[1] = {}; zones[0] = {};
    zones[0].real_bottom = zones[0].real_top = 0;
    master = make_character(1); pet = make_character(2);
    survivor = make_character(3, false);
    P_char tail = make_character(4), morph = make_character(5);
    mover = make_character(6); replacement = make_character(7);
    morphs.insert(morph);
    const std::vector<P_char> occupants = {master, pet, survivor, tail, morph, mover, replacement};
    for (size_t i = 1; i < occupants.size(); ++i)
        occupants[i-1]->next_in_room = occupants[i];
    world[0].people = master;
    follower = {pet, nullptr}; master->followers = &follower; pet->following = master;
    survivor->specials.fighting = pet;
    obj_data wall = {}, artifact = {}, ordinary = {}, ferry = {}, empty_corpse = {};
    wall.R_num = 42; artifact.extra_flags = ITEM_ARTIFACT; empty_corpse.type = ITEM_CORPSE;
    wall.next_content = &artifact; artifact.next_content = &ordinary; ordinary.next_content = &ferry;
    ferry.next_content = &empty_corpse; ferry_ship = &ferry;
    world[0].contents = &wall; object_extractions = 0;
    zone_purge(0);
    assert(removed[1] == 1 && removed[2] == 1 && removed[4] == 1);
    assert(removed[3] == 0 && removed[5] == 0);
    assert(!survivor->specials.fighting);
    if (move) {
        assert(mover->in_room == 1 && replacement->runtime_id == 8);
        assert(removed[6] == 0 && removed[7] == 0 && removed[8] == 0);
    } else assert(removed[6] == 1 && removed[7] == 1);
    assert(world[0].people == survivor && survivor->next_in_room == morph);
    assert(object_extractions == 1 && artifact.next_content == &ferry && ferry.next_content == &empty_corpse);
    printf("zone purge: immediate_free=%d moved_and_replaced=%d; recursive pet and tail removed once, PC/morph/ferry retained\n",
           immediate, move);
    for (auto ch = world[0].people, next = ch; ch; ch = next) {
        next = ch->next_in_room; delete ch;
    }
    if (move) delete mover;
    for (auto ch : retained_allocations) delete ch;
}
int main() {
    for (bool immediate : {false, true})
        for (bool move : {false, true}) run_case(immediate, move);
}
'''

with tempfile.TemporaryDirectory(prefix='duris-zone-purge-') as temporary:
    source = Path(temporary) / 'purge.cpp'
    binary = Path(temporary) / 'purge'
    body = extract_function('new_events.c', 'void zone_purge(int zone_number)')
    followers = extract_function('sparser.c', 'void die_follower(P_char ch)')
    source.write_text(PREFIX + followers + body + SUFFIX)
    subprocess.run(['g++', '-std=c++20', '-g', '-Og', '-D__NO_MYSQL__',
                    '-Isrc', '-Isrc/no_mysql', '-I/usr/include/libxml2',
                    '-fsanitize=address,undefined', '-fno-omit-frame-pointer',
                    '-fno-pie', '-no-pie', str(source), '-o', str(binary)], cwd=ROOT, check=True)
    subprocess.run([str(binary)], check=True, timeout=30)
