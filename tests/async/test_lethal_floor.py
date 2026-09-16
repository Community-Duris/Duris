#!/usr/bin/env python3
"""Production floor impacts stop at lethal damage; living falls still continue."""

from pathlib import Path
import subprocess
import tempfile

from _paths import ROOT


HARNESS = r'''
#include "core/prototypes.h"
#include "core/utils.h"
#include "magic/spells.h"
#include "net/comm.h"
#include "world/db.h"
#include "world/events.h"
#include "world/falling.h"
#include "world/vnum.obj.h"

#include <cassert>
#include <cstdint>
#include <cstdio>

static room_data rooms[2] = {};
P_room world = rooms;
std::uint64_t character_removal_generation = 0;
static int applied_damage = 0, scheduled = 0, deaths = 0, dispels = 0;
static char_data mount = {}, rider = {}, person = {};
static bool riding = false, listed = true, lethal = false;
static obj_data wall = {};
static room_direction_data downward = {};

void logit(const char *, const char *, ...) {}
void act(const char *, int, P_char, P_obj, void *, int) {}
void send_to_char(const char *, P_char) {}
void do_look(P_char, char *, int) {}
bool affected_by_spell(P_char, int) { return false; }
P_char get_linked_char(P_char ch, ush_int) {
    return riding && ch == &person ? &mount : nullptr;
}
P_char get_linking_char(P_char, ush_int) { return nullptr; }
void unlink_char(P_char, P_char, ush_int) {}
int GET_CHAR_SKILL_P(P_char, int) { return 0; }
int char_in_list(const P_char ch) {
    return listed && (ch == &person || ch == &mount || ch == &rider);
}
int number(int low, int high) { return low == 80 && high == 120 ? 100 : low; }
int STAT_INDEX(int) { return 20; }
bool check_castle_walls(int, int) { return false; }
bool notch_skill(P_char, int, float) { return false; }
void Stun(P_char, P_char, int, bool) {}
void KnockOut(P_char, int) {}
void update_pos(P_char) {}
void char_from_room(P_char ch) { ch->in_room = NOWHERE; }
bool char_to_room(P_char ch, int room, int) { ch->in_room = room; return true; }
int real_object(const int) { return 42; }
bool damage(P_char, P_char victim, double amount, int) {
    assert(IS_ALIVE(victim));
    applied_damage = int(amount);
    GET_HIT(victim) -= int(amount);
    if (lethal || GET_HIT(victim) <= 0) {
        ++deaths;
        SET_POS(victim, STAT_DEAD);
        victim->in_room = NOWHERE;
        victim->only.npc = nullptr;
        listed = false;
        ++character_removal_generation;
        return true;
    }
    return false;
}
void spell_dispel_magic(int, P_char ch, char *, int, P_char, P_obj) {
    assert(IS_ALIVE(ch));
    ++dispels;
}
nevent_schedule_result add_event(event_func, int, P_char ch, P_char, P_obj, int,
                                  const void *, int) {
    ++scheduled;
    assert(IS_ALIVE(ch));
    return {nevent_schedule_status::scheduled, {}};
}

static void reset() {
    person = {}; mount = {}; rider = {}; rooms[0] = {}; rooms[1] = {};
    downward = {}; wall = {};
    person.in_room = 0;
    person.curr_stats.Agi = 100; person.curr_stats.Con = 100;
    person.points.max_hit = 1000; person.points.hit = 1000;
    SET_POS(&person, STAT_NORMAL + POS_STANDING);
    mount = person; rider = person;
    applied_damage = scheduled = deaths = dispels = 0;
    riding = false; listed = true; lethal = false;
    character_removal_generation = 0;
}
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
        assert(falling_step(&person, 60) == falling_step_result::continued);
        assert(applied_damage == 240 && scheduled == 1 && dispels == 1);

        reset(); floor(); lethal = true;
        if (npc) person.specials.act |= ACT_ISNPC;
        riding = mounted;
        assert(falling_step(&person, 60) == falling_step_result::actor_removed);
        assert(deaths == 1 && scheduled == 0 && dispels == 0);
        assert(rooms[0].contents == &wall);
    }
    puts("lethal player/NPC and mounted impacts stop; nonlethal floor continuations retained");
}
'''


with tempfile.TemporaryDirectory(prefix="lethal-floor-") as directory:
    root = Path(directory)
    source = root / "floor.cpp"
    binary = root / "floor"
    source.write_text(HARNESS)
    subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-g",
            "-fsanitize=address,undefined",
            "-fno-omit-frame-pointer",
            "-Isrc",
            "src/world/falling.c",
            "src/world/falling_policy.c",
            str(source),
            "-o",
            str(binary),
        ],
        cwd=ROOT,
        check=True,
    )
    subprocess.run([str(binary)], check=True)
