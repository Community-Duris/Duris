#!/usr/bin/env python3
"""Exercise production falling_char under ASan/UBSan with controlled world callbacks.

Exhaustive random rolls prove Climb probabilities. Damage, flight, water,
mount/rider and floor cases preserve the surrounding rules. Real server journeys
are separate: these callbacks isolate arithmetic and event initiation.
"""
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
    if (!IS_ALIVE(ch)) ++post_death_dispels;
}
void event_falling_char(P_char, P_char, P_obj, void *) {}
nevent_schedule_result add_event(event_func, int, P_char ch, P_char, P_obj, int, const void *, int) {
    ++scheduled;
    if (!IS_ALIVE(ch)) ++post_death_schedules;
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

static void ledge() {
    rooms[0].sector_type = SECT_NO_GROUND;
    downward.to_room = 1;
    rooms[0].dir_option[DIR_DOWN] = &downward;
}
static void floor() {
    downward.to_room = 1; downward.exit_info = EX_BREAKABLE;
    rooms[0].dir_option[DIR_DOWN] = &downward;
    wall.R_num = 42; wall.value[1] = 5; wall.value[2] = 10;
    rooms[0].contents = &wall;
}
int main() {
    for (int speed : {1, 31, 43, 60, 100, 250}) {
        reset(); falling_char(&person, speed, true);
        const int baseline = applied_damage;
        reset(); safe_skill = 100; falling_char(&person, speed, true);
        assert(applied_damage == baseline / 2 && applied_damage >= 1);
        printf("safe fall speed=%d: baseline=%d success=%d\n", speed, baseline, applied_damage);
        reset(); safe_skill = 100; roll = 100; falling_char(&person, speed, true);
        assert(applied_damage == baseline); // Existing strict success comparison.
        reset(); safe_skill = 100; roll = 101; falling_char(&person, speed, true);
        assert(applied_damage == baseline);
    }
    reset(); person.curr_stats.Agi = 101; falling_char(&person, 43, true);
    assert(applied_damage == 171);
    reset(); person.curr_stats.Agi = 101; safe_skill = 100; falling_char(&person, 43, true);
    assert(applied_damage == 85); // Round down after the existing pre-skill minimum.
    reset(); safe_skill = 100; person.curr_stats.Agi = 200; falling_char(&person, 1, true);
    assert(applied_damage == 1);
    reset(); person.points.hit = 100; falling_char(&person, 43, true);
    assert(deaths == 1 && stuns == 0);
    reset(); person.points.hit = 100; safe_skill = 100; falling_char(&person, 43, true);
    assert(deaths == 0 && GET_HIT(&person) == 14);
    for (int skill : {0, 100}) {
        reset(); safe_skill = skill; rooms[0].sector_type = SECT_WATER_SWIM;
        falling_char(&person, 43, true);
        assert(applied_damage == 0 && scheduled == 0 && GET_HIT(&person) == 1000);
        reset(); safe_skill = skill; floor(); falling_char(&person, 60, true);
        assert(applied_damage == (skill ? 120 : 240) && scheduled == 1);
        reset(); safe_skill = skill; has_rider = true; falling_char(&person, 43, true);
        const int expected = skill ? 86 : 172;
        assert(applied_damage == 0 && rider_damage == expected);
        assert(GET_HIT(&person) == 1000 - expected && GET_HIT(&rider) == 1000 - expected);
        assert(unlinks == 1);
    }
    for (int learned : {-10, 0, 1, 2, 50, 99, 100, 150}) {
        int caught = 0;
        for (int die_roll = 1; die_roll <= 100; ++die_roll) {
            reset(); ledge(); climbing = true; climb_skill = learned; roll = die_roll;
            if (!falling_char(&person, 0, false)) { ++caught; assert(scheduled == 0); }
            else assert(scheduled == 1);
            assert(applied_damage == 0);
        }
        const int expected = MAX(0, MIN(100, learned)) / 2;
        printf("climb skill=%d: catches=%d/100\n", learned, caught);
        assert(caught == expected);
    }
    reset(); ledge(); climb_skill = 100; // No active Climb affect.
    assert(falling_char(&person, 0, false) && scheduled == 1);
    reset(); ledge(); climbing = true; climb_skill = 100;
    person.specials.affected_by5 = AFF5_MENTAL_ANGUISH; roll = 6;
    assert(falling_char(&person, 0, false) && scheduled == 1);
    for (auto flag : {AFF_FLY, AFF_LEVITATE}) {
        reset(); ledge(); person.specials.affected_by = flag;
        assert(!falling_char(&person, 0, false) && scheduled == 0);
        reset(); ledge(); riding = true; mount.specials.affected_by = flag;
        assert(!falling_char(&person, 0, false) && scheduled == 0);
    }
    reset(); ledge(); riding = true; climbing = true; climb_skill = 100; roll = 51;
    assert(falling_char(&person, 0, false) && scheduled == 1);
    reset(); ledge(); climbing = true; climb_skill = 100; roll = 1;
    falling_char(&person, 31, true); // An already falling player cannot catch at initiation again.
    assert(person.in_room == 1 && applied_damage > 0);
    reset(); floor(); climbing = true; climb_skill = 100;
    assert(!falling_char(&person, 0, false) && scheduled == 0);
    puts("production falling skill boundaries, mount/rider, water and floor regressions passed");
}

'''

with tempfile.TemporaryDirectory(prefix="duris-falling-skills-") as temporary:
    source = Path(temporary) / "falling.cpp"
    binary = Path(temporary) / "falling"
    body = extract_function('affects.c', 'bool falling_char(P_char ch, const int kill_char, bool caller_is_event)')
    source.write_text(PREFIX + body + SUFFIX)
    subprocess.run(['g++', '-std=c++20', '-g', '-Og', '-D__NO_MYSQL__',
                    '-Isrc', '-Isrc/no_mysql', '-I/usr/include/libxml2',
                    '-fsanitize=address,undefined', '-fno-omit-frame-pointer',
                    '-fno-pie', '-no-pie', str(source), '-o', str(binary)], cwd=ROOT, check=True)
    subprocess.run([str(binary)], check=True, timeout=30)
