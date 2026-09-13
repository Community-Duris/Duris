#!/usr/bin/env python3
"""Exercise the production quaff command at the configured epic award boundary."""

import subprocess
import tempfile
from pathlib import Path

from _paths import ROOT, extract_function


PRELUDE = r'''
#include "core/prototypes.h"
#include "core/utils.h"
#include "core/utility.h"
#include "combat/damage.h"
#include "world/db.h"
#include "world/epic.h"
#include "world/vnum.obj.h"
#include "magic/spells.h"
#include "net/comm.h"
#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

P_index obj_index;
P_room world;
Skill skills[MAX_SKILLS]{};
static int minimum_level = 50;
static int awards, consumed, waits, unequips, potion_effects;
static std::string output;

float get_property(const char *key, double fallback) {
    return !strcmp(key, "epic.gain.minLevel") ? minimum_level : fallback;
}
void send_to_char(const char *text, P_char) { output += text; }
void send_to_char_f(P_char, const char *format, ...) {
    char buffer[512];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof buffer, format, args);
    va_end(args);
    output += buffer;
}
void act(const char *text, int, P_char, P_obj, void *, int) { output += text; }
char *one_argument(const char *text, char *out) {
    std::strcpy(out, text);
    return const_cast<char *>(text + std::strlen(text));
}
P_obj get_obj_in_list_vis(P_char, const char *, P_obj list, bool) { return list; }
bool isname(const char *a, const char *b) { return !strcmp(a, b); }
bool affected_by_spell(P_char, int) { return false; }
bool has_innate(P_char, int) { return false; }
int number(int, int high) { return high; }
void CharWait(P_char, int) { ++waits; }
P_obj unequip_char(P_char ch, int slot, bool) {
    ++unequips;
    P_obj result = ch->equipment[slot];
    ch->equipment[slot] = nullptr;
    return result;
}
void gain_epic(P_char ch, int type, int, int amount) {
    assert(type == EPIC_BOTTLE && amount == 75);
    if (epic_level_can_gain(ch)) ++awards;
}
void extract_obj(P_obj, int) { ++consumed; }
int spell_damage(P_char, P_char, double, int, uint, damage_messages *, int *) {
    return 0;
}
affected_type *affect_to_char(P_char, affected_type *) { ++potion_effects; return nullptr; }
int char_in_list(const P_char) { return 1; }
[[noreturn]] int panic_corruption_int(const char *, const char *, ...) { std::abort(); }
'''

DRIVER = r'''
static void check_quaff(int level, int gate, bool held, bool epic, bool allowed) {
    char_data ch{};
    obj_data bottle{};
    index_data index[1]{};
    room_data room[1]{};
    obj_index = index;
    world = room;
    index[0].virtual_number = epic ? VOBJ_EPIC_BOTTLE_EPICS : 1;
    ch.specials.position = STAT_NORMAL;
    ch.player.level = level;
    bottle.type = ITEM_POTION;
    bottle.R_num = 0;
    bottle.name = const_cast<char *>("potion");
    if (held) ch.equipment[HOLD] = &bottle;
    else ch.carrying = &bottle;
    minimum_level = gate;
    awards = consumed = waits = unequips = potion_effects = 0;
    output.clear();
    char argument[] = "potion";
    do_quaff(&ch, argument, 0);
    assert(consumed == (allowed ? 1 : 0));
    assert(awards == (allowed && epic ? 1 : 0));
    assert(waits == (allowed ? 1 : 0));
    assert(unequips == (allowed && held ? 1 : 0));
    assert(potion_effects == (allowed && !epic ? 1 : 0));
    if (!allowed) {
        assert(output.find(std::to_string(gate)) != std::string::npos);
        assert(output.find("suddenly feel.. epic") == std::string::npos);
        assert(held ? ch.equipment[HOLD] == &bottle : ch.carrying == &bottle);
    }
}
int main() {
    for (bool held : {false, true}) {
        for (int level : {46, 47, 48, 49}) check_quaff(level, 50, held, true, false);
        check_quaff(50, 50, held, true, true);
        check_quaff(55, 56, held, true, false);
        check_quaff(56, 56, held, true, true);
        check_quaff(39, 40, held, true, false);
        check_quaff(40, 40, held, true, true);
        check_quaff(1, 50, held, false, true);
    }
    std::puts("Epic potion level gate: inventory, held, default/custom boundaries and ordinary potions passed.");
}
'''

code = PRELUDE + extract_function("utility.c", "int BOUNDED(") + "\n".join(
    extract_function("epic.c", signature) for signature in (
        "int epic_gain_min_level()", "bool epic_level_can_gain(P_char ch)"
    )
) + extract_function("actoth.c", "void do_quaff(") + DRIVER

with tempfile.TemporaryDirectory(prefix="duris-epic-potion-level-") as directory:
    source = Path(directory) / "quaff.cpp"
    binary = Path(directory) / "quaff"
    source.write_text(code)
    subprocess.run([
        "g++", "-std=c++20", "-Wall", "-Wextra", "-Werror",
        "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-no-pie",
        "-I" + str(ROOT / "src"), str(source), "-o", str(binary),
    ], check=True)
    subprocess.run([str(binary)], check=True)
