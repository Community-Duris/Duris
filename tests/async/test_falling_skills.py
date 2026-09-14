#!/usr/bin/env python3
"""Exercise the production falling executor with controlled world callbacks.

Damage, flight, water, mounts, floors, relocation, lifetime invalidation and
schedule rejection are isolated here. The deterministic arithmetic has a
separate direct policy test; real server journeys remain separate.
"""

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
#include <cstring>

static room_data rooms[3] = {};
P_room world = rooms;
std::uint64_t character_removal_generation = 0;

static int safe_skill = 0, climb_skill = 0, roll = 1, applied_damage = 0;
static int schedule_attempts = 0, scheduled_speed = -1, scheduled_delay = -1;
static char_data mount = {}, rider = {}, person = {};
static bool riding = false, has_rider = false;
static bool person_listed = true, mount_listed = true, rider_listed = true;
static int rider_damage = 0, deaths = 0, stuns = 0, unlinks = 0;
static int post_death_dispels = 0, post_death_schedules = 0;
static bool climbing = false, lethal = false, leave_veto = false;
static bool remove_on_leave = false, reject_entry = false, relocate_on_entry = false;
static nevent_schedule_status schedule_status = nevent_schedule_status::scheduled;
static char random_trace[32] = {};
static int random_calls = 0;
static obj_data wall = {};
static room_direction_data downward = {}, next_downward = {};

void logit(const char *, const char *, ...) {}
void act(const char *, int, P_char, P_obj, void *, int) {}
void send_to_char(const char *, P_char) {}
void do_look(P_char, char *, int) {}
bool affected_by_spell(P_char, int spell) { return climbing && spell == SKILL_CLIMB; }
P_char get_linked_char(P_char ch, ush_int) {
    return riding && ch == &person ? &mount : nullptr;
}
P_char get_linking_char(P_char ch, ush_int) {
    return has_rider && ch == &person ? &rider : nullptr;
}
void unlink_char(P_char, P_char, ush_int) { ++unlinks; }
int GET_CHAR_SKILL_P(P_char, int id) {
    return id == SKILL_SAFE_FALL ? safe_skill : climb_skill;
}
int char_in_list(const P_char ch) {
    if (ch == &person) return person_listed;
    if (ch == &mount) return mount_listed;
    if (ch == &rider) return rider_listed;
    return false;
}
int number(int low, int high) {
    if (random_calls < int(sizeof random_trace) - 1) {
        if (low == 80 && high == 120) random_trace[random_calls++] = 'I';
        else if (low == 1 && high == 101) random_trace[random_calls++] = 'S';
        else if (low == 1 && high == 100) random_trace[random_calls++] = 'C';
        else random_trace[random_calls++] = 'O';
    }
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
void char_from_room(P_char ch) {
    if (remove_on_leave && ch == &person) {
        person_listed = false;
        ++character_removal_generation;
        ch->in_room = NOWHERE;
        ch->only.npc = nullptr;
        return;
    }
    if (leave_veto) return;
    ch->in_room = NOWHERE;
}
bool char_to_room(P_char ch, int room, int) {
    if (reject_entry) return false;
    ch->in_room = relocate_on_entry ? 2 : room;
    return true;
}
int real_object(const int) { return 42; }
bool damage(P_char, P_char victim, double amount, int) {
    assert(IS_ALIVE(victim));
    if (victim == &rider) rider_damage = int(amount); else applied_damage = int(amount);
    GET_HIT(victim) -= int(amount);
    if (lethal || GET_HIT(victim) <= 0) {
        ++deaths;
        SET_POS(victim, STAT_DEAD);
        victim->in_room = NOWHERE;
        victim->only.npc = nullptr;
        if (victim == &person) person_listed = false;
        if (victim == &rider) rider_listed = false;
        ++character_removal_generation;
        return true;
    }
    return false;
}
void spell_dispel_magic(int, P_char ch, char *, int, P_char, P_obj) {
    if (!IS_ALIVE(ch)) ++post_death_dispels;
}
nevent_schedule_result add_event(event_func, int delay, P_char ch, P_char, P_obj, int,
                                  const void *data, int data_size) {
    ++schedule_attempts;
    scheduled_delay = delay;
    assert(data && data_size == int(sizeof(int)));
    scheduled_speed = *static_cast<const int *>(data);
    if (!IS_ALIVE(ch)) ++post_death_schedules;
    return {schedule_status, {}};
}

static void reset() {
    person = {}; mount = {}; rider = {};
    for (auto &room : rooms) room = {};
    downward = {}; next_downward = {}; wall = {};
    person.in_room = 0;
    person.curr_stats.Agi = 100; person.curr_stats.Con = 100;
    person.points.max_hit = 1000; person.points.hit = 1000;
    SET_POS(&person, STAT_NORMAL + POS_STANDING);
    mount = person; rider = person;
    riding = has_rider = false;
    person_listed = mount_listed = rider_listed = true;
    rider_damage = deaths = stuns = unlinks = 0;
    safe_skill = climb_skill = 0; roll = 1; applied_damage = 0;
    schedule_attempts = 0; scheduled_speed = scheduled_delay = -1;
    post_death_dispels = post_death_schedules = 0;
    climbing = lethal = leave_veto = remove_on_leave = false;
    reject_entry = relocate_on_entry = false;
    schedule_status = nevent_schedule_status::scheduled;
    character_removal_generation = 0;
    memset(random_trace, 0, sizeof random_trace); random_calls = 0;
}

static void ledge() {
    rooms[0].sector_type = SECT_NO_GROUND;
    downward.to_room = 1;
    rooms[0].dir_option[DIR_DOWN] = &downward;
}
static void falling_chain() {
    ledge();
    next_downward.to_room = 2;
    rooms[1].dir_option[DIR_DOWN] = &next_downward;
}
static void floor() {
    downward.to_room = 1; downward.exit_info = EX_BREAKABLE;
    rooms[0].dir_option[DIR_DOWN] = &downward;
    wall.R_num = 42; wall.value[1] = 5; wall.value[2] = 10;
    rooms[0].contents = &wall;
}

int main() {
    for (int speed : {1, 31, 43, 60, 100, 250}) {
        reset(); falling_step(&person, speed);
        const int baseline = applied_damage;
        reset(); safe_skill = 100; falling_step(&person, speed);
        assert(applied_damage == baseline / 2 && applied_damage >= 1);
        assert(random_trace[0] == 'I' && random_trace[1] == 'S');
        printf("safe fall speed=%d: baseline=%d success=%d\n", speed, baseline,
               applied_damage);
        reset(); safe_skill = 100; roll = 100; falling_step(&person, speed);
        assert(applied_damage == baseline);
        reset(); safe_skill = 100; roll = 101; falling_step(&person, speed);
        assert(applied_damage == baseline);
    }
    reset(); person.curr_stats.Agi = 101; falling_step(&person, 43);
    assert(applied_damage == 171);
    reset(); person.curr_stats.Agi = 101; safe_skill = 100; falling_step(&person, 43);
    assert(applied_damage == 85);
    reset(); safe_skill = 100; person.curr_stats.Agi = 200; falling_step(&person, 1);
    assert(applied_damage == 1);
    reset(); person.points.hit = 100;
    assert(falling_step(&person, 43) == falling_step_result::actor_removed);
    assert(deaths == 1 && stuns == 0);
    reset(); person.points.hit = 100; safe_skill = 100; falling_step(&person, 43);
    assert(deaths == 0 && GET_HIT(&person) == 14);

    for (int skill : {0, 100}) {
        reset(); safe_skill = skill; rooms[0].sector_type = SECT_WATER_SWIM;
        assert(falling_step(&person, 43) == falling_step_result::landed);
        assert(applied_damage == 0 && schedule_attempts == 0 && GET_HIT(&person) == 1000);
        reset(); safe_skill = skill; floor();
        assert(falling_step(&person, 60) == falling_step_result::continued);
        assert(applied_damage == (skill ? 120 : 240) && schedule_attempts == 1);
        reset(); safe_skill = skill; has_rider = true;
        assert(falling_step(&person, 43) == falling_step_result::landed);
        const int expected = skill ? 86 : 172;
        assert(applied_damage == 0 && rider_damage == expected);
        assert(GET_HIT(&person) == 1000 - expected && GET_HIT(&rider) == 1000 - expected);
        assert(unlinks == 1);
    }

    for (int learned : {-10, 0, 1, 2, 50, 99, 100, 150}) {
        int caught = 0;
        for (int die_roll = 1; die_roll <= 100; ++die_roll) {
            reset(); ledge(); climbing = true; climb_skill = learned; roll = die_roll;
            const falling_start_result result = falling_start(&person);
            if (result == falling_start_result::caught) {
                ++caught; assert(schedule_attempts == 0);
            } else {
                assert(result == falling_start_result::scheduled && schedule_attempts == 1);
            }
            assert(applied_damage == 0 && random_trace[0] == 'C');
        }
        const int expected = MAX(0, MIN(100, learned)) / 2;
        printf("climb skill=%d: catches=%d/100\n", learned, caught);
        assert(caught == expected);
    }

    reset(); ledge(); climb_skill = 100;
    assert(falling_start(&person) == falling_start_result::scheduled);
    reset(); ledge(); climbing = true; climb_skill = 100;
    person.specials.affected_by5 = AFF5_MENTAL_ANGUISH; roll = 6;
    assert(falling_start(&person) == falling_start_result::scheduled);
    for (auto flag : {AFF_FLY, AFF_LEVITATE}) {
        reset(); ledge(); person.specials.affected_by = flag;
        assert(falling_start(&person) == falling_start_result::not_applicable);
        assert(schedule_attempts == 0);
        reset(); ledge(); riding = true; mount.specials.affected_by = flag;
        assert(falling_start(&person) == falling_start_result::not_applicable);
        assert(schedule_attempts == 0);
    }
    reset(); ledge(); riding = true; climbing = true; climb_skill = 100; roll = 51;
    assert(falling_start(&person) == falling_start_result::scheduled);
    reset(); ledge(); climbing = true; climb_skill = 100; roll = 1;
    falling_step(&person, 31);
    assert(person.in_room == 1 && applied_damage > 0);
    reset(); floor(); climbing = true; climb_skill = 100;
    assert(falling_start(&person) == falling_start_result::not_applicable);

    reset(); ledge(); schedule_status = nevent_schedule_status::wrong_thread;
    assert(falling_start(&person) == falling_start_result::schedule_rejected);
    assert(schedule_attempts == 1);
    reset(); falling_chain(); schedule_status = nevent_schedule_status::sequence_exhausted;
    assert(falling_step(&person, 1) == falling_step_result::schedule_rejected);
    assert(schedule_attempts == 1 && scheduled_speed == 31 && scheduled_delay == 4);

    reset(); ledge(); leave_veto = true;
    assert(falling_step(&person, 1) == falling_step_result::movement_rejected);
    assert(person.in_room == 0 && schedule_attempts == 0);
    reset(); ledge(); reject_entry = true;
    assert(falling_step(&person, 1) == falling_step_result::movement_rejected);
    assert(person.in_room == NOWHERE && schedule_attempts == 0);
    reset(); ledge(); remove_on_leave = true;
    assert(falling_step(&person, 1) == falling_step_result::actor_removed);
    assert(schedule_attempts == 0 && !person_listed);
    reset(); ledge(); relocate_on_entry = true;
    assert(falling_step(&person, 1) == falling_step_result::landed);
    assert(person.in_room == 2 && applied_damage > 0 && schedule_attempts == 0);

    puts("falling executor skill, lifetime, relocation and scheduling regressions passed");
}
'''


with tempfile.TemporaryDirectory(prefix="duris-falling-skills-") as temporary:
    source = Path(temporary) / "falling.cpp"
    binary = Path(temporary) / "falling"
    source.write_text(HARNESS)
    subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-g",
            "-Og",
            "-D__NO_MYSQL__",
            "-Isrc",
            "-Isrc/no_mysql",
            "-I/usr/include/libxml2",
            "-fsanitize=address,undefined",
            "-fno-omit-frame-pointer",
            "-fno-pie",
            "-no-pie",
            "src/world/falling.c",
            "src/world/falling_policy.c",
            str(source),
            "-o",
            str(binary),
        ],
        cwd=ROOT,
        check=True,
    )
    subprocess.run([str(binary)], check=True, timeout=30)
