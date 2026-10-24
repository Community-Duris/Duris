#!/usr/bin/env python3
"""Run the real kick-message selector for every table index under ASan/UBSan."""
import os
from pathlib import Path
import subprocess
import tempfile

from _paths import ROOT, source

HARNESS = r'''
#include "core/prototypes.h"
#include "core/utils.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <initializer_list>

void kick_messages(P_char, P_char, bool, damage_messages *);
static int roll;
int number(int low, int high) {
    assert(roll >= low && roll <= high);
    return roll;
}

static damage_messages messages_for(int race, bool hit) {
    char_data victim{};
    victim.player.race = race;
    damage_messages messages{};
    kick_messages(nullptr, &victim, hit, &messages);
    assert(messages.attacker && messages.victim && messages.room);
    assert(messages.type == 0);
    if (hit) {
        assert(messages.death_attacker && messages.death_victim && messages.death_room);
        for (const char *text : {messages.attacker, messages.victim, messages.room,
                                messages.death_attacker, messages.death_victim, messages.death_room})
            assert(strlen(text) > 0 || race == RACE_GHOST);
    } else {
        assert(*messages.attacker && *messages.victim && *messages.room);
        assert(!messages.death_attacker && !messages.death_victim && !messages.death_room);
    }
    return messages;
}

int main(int argc, char **argv) {
    assert(argc == 2);
    if (!strcmp(argv[1], "mapping")) {
        for (int race : {RACE_INSECT, RACE_ARACHNID, RACE_DRIDER})
            assert(strstr(messages_for(race, true).victim, "exoskeleton"));
        assert(strstr(messages_for(RACE_AQUATIC_ANIMAL, true).victim, "scales"));
        for (int race : {RACE_BEHOLDER, RACE_BEHOLDERKIN})
            assert(strstr(messages_for(race, true).victim, "crashes into you"));
    } else if (!strcmp(argv[1], "default")) {
        assert(strstr(messages_for(RACE_TIEFLING, true).victim, "solid kick"));
        messages_for(RACE_TIEFLING, false);
    } else {
        assert(!strcmp(argv[1], "all-indices"));
        // Representatives for fixed indices 8 through 19.
        for (int race : {RACE_PLANT, RACE_SLIME, RACE_GNOME, RACE_GIANT,
                         RACE_GHOST, RACE_FLYING_ANIMAL, RACE_UNDEAD, RACE_INSECT,
                         RACE_AQUATIC_ANIMAL, RACE_BEHOLDER, RACE_TIEFLING}) {
            for (bool hit : {false, true}) {
                roll = 0;
                messages_for(race, hit);
                if (race == RACE_GIANT) {
                    roll = 1; // Index 15 instead of 11.
                    messages_for(race, hit);
                }
            }
        }
        // Humanoid indices 0..3 and animal/reptile indices 4..7.
        for (bool hit : {false, true}) {
            for (roll = 0; roll <= 3; ++roll) messages_for(RACE_HUMAN, hit);
            for (roll = 4; roll <= 6; ++roll) messages_for(RACE_ANIMAL, hit);
            for (roll = 4; roll <= 7; ++roll) messages_for(RACE_DRAGON, hit);
        }
    }
    printf("kick messages passed: %s\n", argv[1]);
}
'''

with tempfile.TemporaryDirectory(prefix="kick-messages-") as directory:
    build = Path(directory)
    harness, binary = build / "harness.cpp", build / "harness"
    harness.write_text(HARNESS)
    subprocess.run([
        "g++", "-std=c++20", "-g", "-O1", "-Wall", "-Wextra", "-Werror",
        "-fsanitize=address,undefined", "-fno-sanitize-recover=all",
        "-fno-omit-frame-pointer", "-fno-pie", "-no-pie", "-I" + str(ROOT / "src"),
        str(harness), str(source("kick.c")), "-o", str(binary),
    ], check=True)
    failures = []
    for case in ("mapping", "default", "all-indices"):
        result = subprocess.run([str(binary), case], env={
            **os.environ, "ASAN_OPTIONS": "detect_leaks=1:halt_on_error=1",
            "UBSAN_OPTIONS": "halt_on_error=1:print_stacktrace=1",
        })
        if result.returncode:
            failures.append(case)
    assert not failures, failures
