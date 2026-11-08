#!/usr/bin/env python3
"""Exercise the real flee handler's direction control separately from escape."""
from pathlib import Path
import subprocess
import tempfile

from _paths import ROOT, extract_function

PRELUDE = r'''
#include "core/prototypes.h"
#include "core/utils.h"
#include "net/comm.h"
#include "world/events.h"
#include "world/world_activity.h"
#include "world/buildings.h"
#include "combat/grapple.h"
#include "cmd/interp.h"
#include "magic/spells.h"
#include "telemetry/telemetry_runtime.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <string>
#include <strings.h>

static room_data rooms[4]{};
P_room world = rooms;
static index_data indexes[1]{};
P_index mob_index = indexes, obj_index = indexes;
const char *dirs[] = {"north", "east", "south", "west", "up", "down",
                      "northwest", "southwest", "northeast", "southeast"};
static int skill = 100, control_roll = 1, escape_roll = 0;
static int moved = -1, notches = 0, control_rolls = 0, forbidden_room = -1;
static bool notch_result = false, blocked_wall = false, fail_move = false;
static std::string output;
int number(int low, int high) {
    assert(low <= high);
    if (low == 1 && (high == 100 || high == 130)) {
        ++control_rolls; return control_roll;
    }
    if (low == 0 && high == 100) return escape_roll;
    if (low == 0) return high; // Pick the last valid exit on random fallback.
    return low;
}
int GET_CLASS(P_char ch, uint cls) { return (ch->player.m_class & cls) != 0; }
int GET_SECONDARY_CLASS(P_char, uint) { return 0; }
int GET_CHAR_SKILL_P(P_char, int which) {
    return which == SKILL_CONTROL_FLEE ? skill : 0;
}
bool notch_skill(P_char, int which, float) {
    if (which != SKILL_CONTROL_FLEE) return false;
    ++notches; return notch_result;
}
int get_property(const char *, int fallback) { return fallback; }
P_char get_linked_char(P_char, ush_int) { return nullptr; }
P_char get_linking_char(P_char, ush_int) { return nullptr; }
int grapple_flee_check(P_char) { return 0; }
int grapple_check_entrapment(P_char) { return 0; }
bool check_crippling_strike(P_char) { return false; }
bool check_castle_walls(int, int room) { return blocked_wall && room == 1; }
int can_enter_room(P_char, int room, int) { return room != forbidden_room; }
void send_to_char(const char *text, P_char) { output += text; }
void act(const char *text, int, P_char, P_obj, void *, int where) {
    if (where == TO_CHAR) output += text;
}
void logit(const char *, const char *, ...) { assert(false); }
int strn_cmp(const char *a, const char *b, unsigned int n) { return strncasecmp(a, b, n); }
int panic_corruption_int(const char *, const char *, ...) { std::abort(); }
int fill_word(char *) { return 0; }
int NumAttackers(P_char ch) { return IS_FIGHTING(ch) ? 1 : 0; }
void StopAllAttackers(P_char ch) { ch->specials.fighting = nullptr; }
void stop_fighting(P_char) { assert(false); }
void set_fighting(P_char, P_char) { assert(false); }
room_affect *get_spell_from_room(P_room, int) { return nullptr; }
int do_simple_move(P_char ch, int direction, unsigned int flags) {
    assert(flags == (MVFLG_DRAG_FOLLOWERS | MVFLG_FLEE));
    moved = direction;
    if (!fail_move) ch->in_room = EXIT(ch, direction)->to_room;
    return !fail_move;
}
bool affected_by_spell(P_char, int spell) { return spell == SKILL_AWARENESS; }
affected_type *affect_to_char(P_char, affected_type *) { assert(false); return nullptr; }
bool has_innate(P_char, int) { return false; }
void StartRegen(P_char, regen_resource) {}
telemetry_capture_result telemetry_runtime_game_encounter_leave(P_char, telemetry_encounter_outcome) { return {}; }
void disarm_char_nevents(P_char, event_func_type) { assert(false); }
void world_activity_schedule_mundane_after(P_char, int) { assert(false); }
void event_mob_mundane(P_char, P_char, P_obj, void *) {}
#define DRAGONSLAYER_VNUM 80569
'''

DRIVER = r'''
static char_data actor{}, enemy{};
static pc_only_data pc{};
static room_direction_data north{}, south{};
static void reset() {
    actor = {}; enemy = {}; pc = {};
    for (auto &room : rooms) room = {};
    north = {}; south = {};
    north.to_room = 1; south.to_room = 2;
    rooms[0].dir_option[0] = &north;
    rooms[0].dir_option[2] = &south;
    actor.only.pc = &pc;
    actor.player.m_class = CLASS_ROGUE;
    actor.player.race = enemy.player.race = RACE_HUMAN;
    SET_POS(&actor, STAT_NORMAL + POS_STANDING);
    SET_POS(&enemy, STAT_NORMAL + POS_STANDING);
    GET_VITALITY(&actor) = 100;
    skill = 100; control_roll = 1; escape_roll = 0;
    moved = -1; notches = control_rolls = 0; forbidden_room = -1;
    notch_result = blocked_wall = fail_move = false;
    output.clear();
}
static void flee(const char *text) {
    char argument[128]; std::snprintf(argument, sizeof argument, "%s", text);
    do_flee(&actor, argument, CMD_FLEE);
}
static bool control_failed() { return output.find("lose control of your fleeing direction") != std::string::npos; }
int main(int argc, char **argv) {
    assert(argc == 2);
    const std::string mode = argv[1];
    if (mode == "chance") {
        for (int trained : {1, 50, 100, 120}) {
            for (bool training : {false, true}) {
                int controlled = 0;
                for (int roll = 1; roll <= 100; ++roll) {
                    reset(); skill = trained; control_roll = roll; notch_result = training;
                    flee("north");
                    controlled += moved == 0;
                    assert(moved == 0 || moved == 2);
                    assert(control_failed() == (moved == 2));
                    assert(notches == 1 && control_rolls == 1);
                }
                assert(controlled == std::min(trained, 100) * MAX_CHANCE_TO_CONTROL_FLEE / 100);
            }
        }
    } else if (mode == "exits") {
        for (int invalid = 0; invalid < 9; ++invalid) {
            reset();
            const char *direction = "north";
            switch (invalid) {
                case 0: direction = "nonsense"; break;
                case 1: rooms[0].dir_option[0] = nullptr; break;
                case 2: north.to_room = NOWHERE; break;
                case 3: north.exit_info = EX_CLOSED; break;
                case 4: north.exit_info = EX_SECRET; break;
                case 5: north.exit_info = EX_BLOCKED; break;
                case 6: rooms[1].chance_fall = 1; break;
                case 7: forbidden_room = 1; break;
                case 8: blocked_wall = true; break;
            }
            flee(direction);
            assert(moved == -1 && actor.in_room == 0 && notches == 0);
            assert(output.find(invalid == 0 ? "Flee in which direction?" : "You cannot flee in that direction.") != std::string::npos);
        }
        reset(); north.exit_info = EX_CLOSED; actor.specials.affected_by2 = AFF2_PASSDOOR;
        flee("  NoRtH  "); assert(moved == 0);
        reset(); rooms[1].chance_fall = 1; actor.specials.affected_by = AFF_FLY;
        flee("n"); assert(moved == 0);
        reset(); rooms[0].dir_option[0] = nullptr; rooms[0].dir_option[2] = nullptr;
        flee(""); assert(moved == -1 && output.find("couldn't escape") != std::string::npos);
    } else if (mode == "escape") {
        reset(); actor.specials.fighting = &enemy; escape_roll = 100;
        flee("north");
        assert(moved == -1 && !control_failed() && output.find("PANIC!") != std::string::npos);
        reset(); actor.specials.fighting = &enemy; control_roll = 100;
        flee("north"); assert(moved == 2 && control_failed() && !IS_FIGHTING(&actor));
        reset(); actor.specials.fighting = &enemy; control_roll = 100; escape_roll = 100;
        flee("north"); assert(moved == -1 && control_failed() && output.find("PANIC!") != std::string::npos);
        // Pin the existing independent ordinary escape threshold with two exits.
        int escaped = 0;
        for (int roll = 0; roll <= 100; ++roll) {
            reset(); actor.specials.fighting = &enemy; escape_roll = roll;
            flee("north"); escaped += moved == 0;
        }
        assert(escaped == MIN_CHANCE_TO_FLEE + (MAX_CHANCE_TO_FLEE - MIN_CHANCE_TO_FLEE) / 3);
        reset(); fail_move = true; flee("north");
        assert(actor.in_room == 0 && output.find("couldn't escape") != std::string::npos);
        reset(); skill = 0; flee("north"); assert(moved == 2 && notches == 0 && !control_failed());
        reset(); actor.player.m_class = CLASS_WARRIOR; flee("north");
        assert(moved == 2 && notches == 0 && !control_failed());
        reset(); flee(""); assert(moved == 2 && notches == 0 && !control_failed());
    } else assert(false);
    std::printf("Control Flee production case passed: %s\n", argv[1]);
}
'''

production = "\n".join(extract_function(file, signature) for file, signature in [
    ("utility.c", "int BOUNDED("),
    ("interp.c", "int search_block("),
    ("interp.c", "char *one_argument("),
    ("utility.c", "int dir_from_keyword("),
    ("actoff.c", "bool bad_flee_dir("),
    ("actoff.c", "static bool restore_equipment_sneak_after_flee("),
    ("actoff.c", "void do_flee("),
])
with tempfile.TemporaryDirectory(prefix="duris-control-flee-") as temporary:
    harness = Path(temporary) / "harness.cpp"
    binary = Path(temporary) / "harness"
    harness.write_text(PRELUDE + production + DRIVER)
    subprocess.run([
        "g++", "-std=c++20", "-Wall", "-Wextra", "-Werror", "-g",
        "-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-fno-pie", "-no-pie",
        "-I" + str(ROOT / "src"), str(harness), "-o", str(binary),
    ], check=True)
    failures = 0
    for mode in ("chance", "exits", "escape"):
        failures += subprocess.run([str(binary), mode]).returncode != 0
    assert not failures, f"{failures} Control Flee scenarios failed"
