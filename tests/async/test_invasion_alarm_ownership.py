#!/usr/bin/env python3
"""Exercise the room-entry alarm hook and pet-owner eligibility with real links.

Each accompanying pet contributes one ordinary arrival, just like another enemy
player. NPC race/faction cannot independently cause an alarm. No live world or DB.
"""
import os
from pathlib import Path
import subprocess
import tempfile

from _paths import ROOT, extract_function, source

justice = source("justice.c").read_text()
towns = justice[justice.index("struct hometown_data hometowns[LAST_HOME]"):
                justice.index("struct mm_ds *dead_justice_guard_pool")]
placement = extract_function("handler.c", "bool char_to_room(")
hook = placement[placement.index("\t/*\n\t * justice hook"):
                 placement.index("\t// Purge NPCs from safe rooms?")]
guards = extract_function("justice.c", "int justice_send_guards(")
assert guards.index("return FALSE; // disabling") < guards.index("// Only good hts get guards atm.")

PRELUDE = r'''
#include "core/prototypes.h"
#include "core/utils.h"
#include "combat/justice.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <initializer_list>
#include <string>
#include <vector>
static room_data rooms[3]{};
P_room world = rooms;
static zone_data zones[3]{};
zone_data *zone_table = zones;
P_desc descriptor_list = nullptr;
static char_data owner{}, pets[4]{}, players[4]{}, listener{};
static npc_only_data npc[4]{};
static char_link_data links[4]{};
static descriptor_data descriptor{};
static std::vector<int> rolls;
static size_t roll_pos;
static int alarm_count, property_enabled = 1;
static P_char dummy;
static std::string text;
static time_t now = 10000;
static time_t test_time(time_t *) { return now; }
int number(int low, int high) {
    assert(roll_pos < rolls.size());
    const int value = rolls[roll_pos++];
    assert(value >= low && value <= high);
    return value;
}
int get_property(const char *key, int) {
    assert(!strcmp(key, "justice.alarms.good") || !strcmp(key, "justice.alarms.evil"));
    return property_enabled;
}
bool training_dummy_is(P_char ch) { return ch == dummy; }
int justice_send_guards(int, P_char ch, int) {
    assert(IS_PC(ch)); // The dormant dispatcher would receive the controller.
    return false;
}
void send_to_char(const char *message, P_char ch) {
    assert(ch == &listener);
    text += message;
    if (strcmp(message, "\r\n")) ++alarm_count;
}
int real_room(const int vnum) { return vnum >= 100 && vnum < 103 ? vnum - 100 : NOWHERE; }
int IS_MORPH(P_char ch) { return ch->only.npc && ch->only.npc->orig_char; }
int GET_CLASS(P_char ch, uint cls) { return (ch->player.m_class & cls) != 0; }
'''

DRIVER = r'''
static void setup(int room = 0, int side = RACEWAR_EVIL) {
    owner = {}; listener = {}; descriptor = {};
    for (int i = 0; i < 3; ++i) {
        rooms[i] = {}; zones[i] = {};
        rooms[i].zone = i; rooms[i].number = 100 + i;
    }
    zones[0].hometown = HOME_WOODSEER;
    zones[1].hometown = 3; // Drow hometown, JUSTICE_EVILHOME.
    zones[2].hometown = HOME_WOODSEER; // Another room/zone, same hometown.
    owner.player.race = side == RACEWAR_GOOD ? RACE_HALFLING : RACE_DROW;
    owner.player.racewar = side; owner.player.level = 50;
    owner.in_room = listener.in_room = room;
    owner.specials.position = listener.specials.position = STAT_NORMAL + POS_STANDING;
    descriptor.character = &listener; descriptor.connected = CON_PLAYING;
    descriptor_list = &descriptor;
    for (int i = 0; i < 4; ++i) {
        pets[i] = {}; npc[i] = {}; players[i] = owner; links[i] = {};
        pets[i].specials.act = ACT_ISNPC;
        pets[i].specials.position = STAT_NORMAL + POS_STANDING;
        pets[i].only.npc = &npc[i]; pets[i].in_room = room;
        pets[i].player.race = RACE_WRAITH;
        pets[i].player.birthplace = 999; // A foreign NPC under the old rule.
        pets[i].player.racewar = RACEWAR_EVIL;
        links[i].type = LNK_PET; links[i].linking = &pets[i]; links[i].linked = &owner;
        pets[i].linking = &links[i];
    }
    property_enabled = 1; dummy = nullptr; alarm_count = 0;
    text.clear(); rolls.clear(); roll_pos = 0;
}
static void invoke(P_char arrival, std::initializer_list<int> values, bool alarm) {
    rolls = values; roll_pos = 0; alarm_count = 0; text.clear();
    assert(room_entry_hook(arrival));
    assert(roll_pos == rolls.size());
    assert(alarm_count == (alarm ? 1 : 0));
}
static void friendly() {
    setup(0, RACEWAR_GOOD);
    invoke(&owner, {}, false);
    // These cover hostile-race minions and friendly-looking images/elementals.
    int races[] = {RACE_WRAITH, RACE_HALFLING, RACE_F_ELEMENTAL, RACE_ZOMBIE};
    for (int i = 0; i < 4; ++i) {
        pets[i].player.race = races[i];
        invoke(&pets[i], {}, false);
    }
    assert(zones[0].last_raid == 0);
}
static void excluded() {
    for (int kind = 0; kind < 10; ++kind) {
        setup();
        if (kind == 0) pets[0].linking = nullptr; // Wild/unowned NPC.
        if (kind == 1) owner.in_room = 2; // Same hometown, different room.
        if (kind == 2) owner.in_room = NOWHERE;
        if (kind == 3) owner.specials.act = ACT_ISNPC; // No player controller.
        if (kind == 4) owner.specials.position = STAT_DEAD;
        if (kind == 5) owner.player.level = MINLVLIMMORTAL;
        if (kind == 6) owner.disguise.active_pc = true;
        if (kind == 7) owner.specials.affected_by |= AFF_BOUND;
        if (kind == 8) dummy = &pets[0];
        if (kind == 9) pets[0].specials.affected_by |= AFF_BOUND;
        invoke(&pets[0], {}, false);
        assert(zones[0].last_raid == 0);
    }
}
static void inherited() {
    for (int room : {0, 1}) {
        setup(room, room == 0 ? RACEWAR_EVIL : RACEWAR_GOOD);
        pets[0].player.race = RACE_F_ELEMENTAL;
        pets[0].player.racewar = room == 0 ? RACEWAR_GOOD : RACEWAR_EVIL;
        pets[0].player.birthplace = 100 + room; // Owner, not NPC birthplace, determines eligibility.
        if (room == 0) invoke(&pets[0], {1, 0, 1}, true);
        else invoke(&pets[0], {1, 0}, true);
        assert(text.find(room == 0 ? "Alarm bells" : "Horns") != std::string::npos);
        assert(zones[room].last_raid == now);
        property_enabled = 0;
        invoke(&pets[0], {1, 1, 0}, false);
    }
}
static void probability() {
    for (int room : {0, 1}) {
        int first = 0, recent = 0;
        for (int alarm = 0; alarm < 16; ++alarm) {
            setup(room, room ? RACEWAR_GOOD : RACEWAR_EVIL);
            if (room == 0 && alarm == 0) invoke(&owner, {1, alarm, 1}, true);
            else invoke(&owner, {1, alarm}, alarm == 0);
            first += alarm_count;
            for (int skip = 0; skip < 2; ++skip) {
                setup(room, room ? RACEWAR_GOOD : RACEWAR_EVIL);
                zones[room].last_raid = now - 100;
                if (!skip) invoke(&pets[0], {skip}, false);
                else if (room == 0 && alarm == 0) invoke(&pets[0], {skip, 1, alarm, 1}, true);
                else invoke(&pets[0], {skip, 1, alarm}, alarm == 0);
                recent += alarm_count;
                assert(zones[room].last_raid == now - 100);
            }
        }
        assert(first == 1 && recent == 1);
    }
}
static void groups() {
    // Compare five real players with one controller plus four companions. Each
    // candidate is the only successful arrival in turn; also test no/all alarms.
    for (int winner = -2; winner < 5; ++winner) {
        int results[2]{};
        for (int pets_in_group = 0; pets_in_group < 2; ++pets_in_group) {
            setup();
            for (int i = 0; i < 4; ++i) pets[i].in_room = 2; // Not arrived yet.
            for (int member = 0; member < 5; ++member) {
                P_char arrival = member == 0 ? &owner :
                    pets_in_group ? &pets[member - 1] : &players[member - 1];
                arrival->in_room = 0;
                bool success = winner == -2 || winner == member;
                if (member == 0) {
                    if (success) invoke(arrival, {1, 0, 1}, true);
                    else invoke(arrival, {1, 1}, false);
                } else {
                    if (success) invoke(arrival, {1, 1, 0, 1}, true);
                    else invoke(arrival, {0}, false);
                }
                results[pets_in_group] += alarm_count;
                assert(zones[0].last_raid == now);
            }
        }
        assert(results[0] == results[1]);
        assert(results[0] == (winner == -2 ? 5 : winner == -1 ? 0 : 1));
    }
}
int main(int argc, char **argv) {
    assert(argc == 2);
    if (!strcmp(argv[1], "friendly")) friendly();
    else if (!strcmp(argv[1], "excluded")) excluded();
    else if (!strcmp(argv[1], "inherited")) inherited();
    else if (!strcmp(argv[1], "probability")) probability();
    else if (!strcmp(argv[1], "groups")) groups();
    else assert(false);
    printf("invasion alarm ownership passed: %s\n", argv[1]);
}
'''

with tempfile.TemporaryDirectory(prefix="invasion-alarm-ownership-") as directory:
    harness, binary = Path(directory) / "harness.cpp", Path(directory) / "harness"
    harness.write_text(PRELUDE + towns + "\n" +
                      extract_function("affects.c", "P_char get_linked_char(") +
                      "\n#define time test_time\n" +
                      extract_function("justice.c", "void justice_action_invader(") + "\n" +
                      extract_function("justice.c", "void justice_hometown_echo(") +
                      "\n#undef time\nbool room_entry_hook(P_char ch) {\n" + hook +
                      "\nreturn true;\n}\n" + DRIVER)
    subprocess.run([
        "g++", "-std=c++20", "-g", "-O1", "-fsanitize=address,undefined",
        "-fno-sanitize-recover=all", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie",
        "-I" + str(ROOT / "src"), str(harness), "-o", str(binary),
    ], check=True)
    failures = []
    for case in ("friendly", "excluded", "inherited", "probability", "groups"):
        result = subprocess.run([str(binary), case], env={
            **os.environ, "ASAN_OPTIONS": "detect_leaks=1:halt_on_error=1",
            "UBSAN_OPTIONS": "halt_on_error=1:print_stacktrace=1",
        })
        if result.returncode:
            failures.append(case)
    assert not failures, failures
