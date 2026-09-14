#!/usr/bin/env python3
"""Production floor impacts stop at lethal damage; living falls still continue."""
from pathlib import Path
import subprocess
import tempfile
from _paths import ROOT, extract_function
PREFIX = r'''

#include "core/prototypes.h"
#include "core/utils.h"
#include "magic/spells.h"
#include "world/db.h"
#include "world/events.h"
#include "world/vnum.obj.h"
#include "net/comm.h"
#include <cassert>
#include <cstdio>
#include <cstring>

static room_data rooms[2] = {};
P_room world = rooms;
static int safe_skill = 0, climb_skill = 0, roll = 1, applied_damage = 0, scheduled = 0;
static char_data mount = {}, rider = {};
static bool riding = false, has_rider = false;
static int rider_damage = 0, deaths = 0, stuns = 0, unlinks = 0;
static int post_death_dispels = 0, post_death_schedules = 0;
static bool climbing = false, lethal = false;
static char_data person = {};
static obj_data wall = {};
static room_direction_data downward = {};

void logit(const char *, const char *, ...) {}
void act(const char *, int, P_char, P_obj, void *, int) {}
void send_to_char(const char *, P_char) {}
void do_look(P_char, char *, int) {}
bool affected_by_spell(P_char, int spell) { return climbing && spell == SKILL_CLIMB; }
P_char get_linked_char(P_char ch, ush_int) { return riding && ch == &person ? &mount : nullptr; }
P_char get_linking_char(P_char ch, ush_int) { return has_rider && ch == &person ? &rider : nullptr; }
void unlink_char(P_char, P_char, ush_int) { ++unlinks; }
int GET_CHAR_SKILL_P(P_char, int id) { return id == SKILL_SAFE_FALL ? safe_skill : climb_skill; }
int number(int low, int high) {
    if (low == 80 && high == 120) return 100;
    if (low == 1 && (high == 100 || high == 101)) return roll;
    return low;
}
int STAT_INDEX(int) { return 20; }
bool check_castle_walls(int, int) { return false; }
bool notch_skill(P_char, int, float) { return false; }
void Stun(P_char, P_char, int, bool) { ++stuns; }
void KnockOut(P_char, int) {}
void update_pos(P_char) {}
void char_from_room(P_char ch) { ch->in_room = NOWHERE; }
bool char_to_room(P_char ch, int room, int) { ch->in_room = room; return true; }
int real_object(const int) { return 42; }
bool damage(P_char, P_char victim, double amount, int) {
    assert(IS_ALIVE(victim));
    if (victim == &rider) rider_damage = int(amount); else applied_damage = int(amount);
    GET_HIT(victim) -= int(amount);
    if (lethal || GET_HIT(victim) <= 0) {
        ++deaths;
        SET_POS(victim, STAT_DEAD);
        victim->in_room = NOWHERE;
        victim->only.npc = nullptr; // teardown outcome from free_char; pooled char survives.
        return true;
    }
    return false;
}
void spell_dispel_magic(int, P_char ch, char *, int, P_char, P_obj) {
    assert(IS_ALIVE(ch)); ++post_death_dispels;
}
void event_falling_char(P_char, P_char, P_obj, void *) {}
nevent_schedule_result add_event(event_func, int, P_char ch, P_char, P_obj, int, const void *, int) {
    ++scheduled;
    assert(IS_ALIVE(ch));
    return {};
}
static void reset() {
    person = {}; rooms[0] = {}; rooms[1] = {}; downward = {}; wall = {};
    person.in_room = 0;
    person.curr_stats.Agi = 100; person.curr_stats.Con = 100;
    person.points.max_hit = 1000; person.points.hit = 1000;
    SET_POS(&person, STAT_NORMAL + POS_STANDING);
    mount = person; rider = person; riding = has_rider = false;
    rider_damage = deaths = stuns = unlinks = 0;
    safe_skill = climb_skill = 0; roll = 1; applied_damage = scheduled = 0;
    post_death_dispels = post_death_schedules = 0; climbing = lethal = false;
}

'''
SUFFIX = r'''
static void floor() {
    downward.to_room = 1; downward.exit_info = EX_BREAKABLE;
    rooms[0].dir_option[DIR_DOWN] = &downward;
    wall.R_num = 42; wall.value[1] = 5; wall.value[2] = 10;
    rooms[0].contents = &wall;
}
int main() {
    for (bool npc : {false, true}) for (bool mounted : {false, true}) {
        reset(); floor();
        if (npc) person.specials.act |= ACT_ISNPC;
        riding = mounted;
        assert(!falling_char(&person, 60, true));
        assert(applied_damage == 240 && scheduled == 1 && post_death_dispels == 1);
        reset(); floor(); lethal = true;
        if (npc) person.specials.act |= ACT_ISNPC;
        riding = mounted;
        assert(falling_char(&person, 60, true));
        assert(deaths == 1 && scheduled == 0 && post_death_dispels == 0);
        assert(rooms[0].contents == &wall); // No world-side dispel was invoked.
    }
    puts("lethal player/NPC and mounted impacts stop; nonlethal floor continuations retained");
}
'''
with tempfile.TemporaryDirectory(prefix='lethal-floor-') as directory:
    root = Path(directory)
    source = root / 'floor.cpp'
    source.write_text(PREFIX + extract_function('affects.c', 'bool falling_char(') + SUFFIX)
    subprocess.run(['g++', '-std=c++20', '-g', '-fsanitize=address,undefined',
                    '-fno-omit-frame-pointer', '-Isrc', str(source), '-o', str(root / 'floor')], cwd=ROOT, check=True)
    subprocess.run([str(root / 'floor')], check=True)