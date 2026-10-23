#!/usr/bin/env python3
"""Execute PSP rates, lifecycle and spell expenditure under ASan/UBSan.

The queue is deterministic; full-server lifecycle coverage is a separate journey.
Rates, elapsed credit, bounds, difficulty and use_spell are production code.
"""
import os
from pathlib import Path
import subprocess
import tempfile

from _paths import ROOT, HARNESS_STUBS, extract_function, source

PRELUDE = r'''
#include "core/prototypes.h"
#include "core/utils.h"
#include "net/comm.h"
#include "magic/spells.h"
#include "world/events.h"
#include "world/falling.h"
#include "world/difficulty.c"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <map>
#include <vector>

struct regen_event_state { float accumulated; unsigned long long last_tick; };
int pulse = 0;
unsigned long long ne_event_tick = 0;
static room_data rooms[1]{};
P_room world = rooms;
static std::map<std::string, float> properties;
static bool sun = false, twilight = false, vulnerable = false;
static int completion_messages = 0, vitals = 0;
static char_data ch{}, enemy{};
static pc_only_data pc{};
static npc_only_data npc{};
P_char character_list = nullptr;
extern const struct racial_data_type racial_data[];
const struct racial_data_type racial_data[LAST_RACE + 1] = {};
void remove_disguise(P_char, bool) { assert(false); }
void wear_off_message(P_char, affected_type *) { assert(false); }
P_char un_morph(P_char owner) { return owner; }
bool char_falling(P_char) { return false; }
falling_start_result falling_start(P_char) { assert(false); return {}; }
struct time_info_data age(P_char) { return {}; }
int IS_MORPH(P_char) { return false; }
bool rested_bonus_effect_active(const affected_type *) { assert(false); return false; }

float get_property(const char *key, double fallback) {
    const auto i = properties.find(key);
    return i == properties.end() ? fallback : i->second;
}
bool affected_by_spell(P_char owner, int type) {
    for (auto *af = owner->affected; af; af = af->next)
        if (af->type == type) return true;
    return false;
}
bool has_innate(P_char, int innate) { return innate == INNATE_VULN_SUN && vulnerable; }
bool IS_TWILIGHT_ROOM(int) { return twilight; }
#undef IS_SUNLIT
#define IS_SUNLIT(r) sun
int GET_CLASS(P_char owner, uint cls) { return (owner->player.m_class & cls) != 0; }
int GET_PRIME_CLASS(P_char owner, uint cls) { return GET_CLASS(owner, cls); }
int GET_SECONDARY_CLASS(P_char, uint) { return 0; }
ClassSkillInfo SKILL_DATA_ALL(P_char, int) { assert(false); return {}; }
int GET_CHAR_SKILL_P(P_char, int skill) { return skill == SKILL_MEDITATE ? 75 : 0; }
void gmcp_char_vitals(P_char) { ++vitals; }
void send_to_char(const char *text, P_char) {
    if (std::strstr(text, "mana reserves are now full")) ++completion_messages;
}
void stop_meditation(P_char owner) { REMOVE_BIT(owner->specials.affected_by, AFF_MEDITATE); }
int hit_regen(P_char, bool) { return 0; }
int ward_regen(P_char, bool) { return 0; }
int move_regen(P_char, bool) { return 0; }
void event_hit_regen(P_char, P_char, P_obj, void *) { assert(false); }
void event_ward_regen(P_char, P_char, P_obj, void *) { assert(false); }
void event_move_regen(P_char, P_char, P_obj, void *) { assert(false); }

struct pending_event {
    P_char owner;
    event_func func;
    unsigned long long due;
    regen_event_state state;
};
static std::vector<pending_event> queue;
static nevent_data dummy_event{};
nevent_schedule_result add_event(event_func func, int delay, P_char owner, P_char, P_obj,
                                 int, const void *data, int size) {
    assert(size == sizeof(regen_event_state));
    queue.push_back({owner, func, ne_event_tick + delay,
                     *static_cast<const regen_event_state *>(data)});
    return {};
}
P_nevent get_scheduled(P_char owner, event_func_type func) {
    for (const auto &event : queue)
        if (event.owner == owner && event.func == func) return &dummy_event;
    return nullptr;
}

// Non-mana spell paths must never be reached by these psionicist/mindflayer cases.
struct s_skill skills[MAX_SKILLS]{};
int number(int low, int) { return low; }
int get_spell_circle(P_char, int) { return 2; }
const char *get_god_name(P_char) { assert(false); return ""; }
void affect_to_end(P_char, affected_type *) { assert(false); }
void affect_remove(P_char, affected_type *) { assert(false); }
void event_memorize(P_char, P_char, P_obj, void *) { assert(false); }
void schedule_memorize(P_char, int) { assert(false); }
int get_circle_memtime(P_char, int) { assert(false); return 0; }

'''

events = source("events.c").read_text()
HELPERS = "\n".join([
    *(line for line in events.splitlines()
      if line.startswith("#define MOB_") and "_REGEN_DELAY" in line),
    extract_function("events.c", "static struct regen_event_state regen_state_from_data("),
    extract_function("events.c", "static unsigned long long regen_elapsed_ticks("),
    extract_function("limits.c", "int mana_regen("),
    extract_function("events.c", "void event_mana_regen(P_char ch,"),
    extract_function("events.c", "void StartRegen(P_char ch,"),
    extract_function("memorize.c", "void use_spell("),
    extract_function("affects.c", "void affect_update("),
])

DRIVER = r'''
static void reset(int dial = 5, bool mob = false) {
    ch = {}; pc = {}; npc = {}; enemy = {};
    if (mob) { ch.only.npc = &npc; ch.specials.act = ACT_ISNPC; npc.idnum = 3; }
    else ch.only.pc = &pc;
    ch.player.level = 50;
    ch.player.m_class = CLASS_PSIONICIST;
    ch.player.race = RACE_HUMAN;
    ch.in_room = 0;
    ch.specials.position = STAT_NORMAL | POS_STANDING;
    ch.curr_stats.Pow = ch.curr_stats.Int = 100;
    GET_COND(&ch, FULL) = GET_COND(&ch, THIRST) = 24;
    GET_MANA(&ch) = GET_VITALITY(&ch) = 100;
    GET_MAX_MANA(&ch) = GET_MAX_VITALITY(&ch) = 10000;
    queue.clear(); ne_event_tick = 0; completion_messages = vitals = 0;
    sun = twilight = vulnerable = false;
    properties["difficulty.dial.player.recovery"] = dial;
    update_difficulty_dials();
}

static void advance(int pulses, int interval = 1) {
    assert(pulses % interval == 0);
    const auto end = ne_event_tick + pulses;
    while (ne_event_tick < end) {
        ne_event_tick += interval;
        // Process one pass: callbacks added during it run on a later pulse.
        auto pending = queue;
        queue.clear();
        for (auto event : pending) {
            if (event.due <= ne_event_tick)
                event.func(event.owner, nullptr, nullptr, &event.state);
            else queue.push_back(event);
        }
    }
}

static void check_rate(int expected) {
    assert(mana_regen(&ch, false) == expected);
    assert(mana_regen(&ch, true) == expected);
    const int before = GET_MANA(&ch);
    StartRegen(&ch, regen_resource::mana);
    StartRegen(&ch, regen_resource::mana);
    assert(queue.size() == (expected ? 1 : 0));
    advance(PULSES_IN_TICK);
    assert(std::abs(GET_MANA(&ch) - before - expected) <= (expected ? 1 : 0));
}

static void rates() {
    for (int dial : {1, 5, 10}) {
        for (bool mob : {false, true}) {
            reset(dial, mob);
            check_rate(mob ? 80 : dial == 1 ? 160 : dial == 5 ? 80 : 40);
        }
        for (int interval : {1, 4}) {
            reset(dial);
            const int rate = mana_regen(&ch, false);
            StartRegen(&ch, regen_resource::mana);
            advance(3000, interval);
            assert(std::abs(GET_MANA(&ch) - 100 - 10 * rate) <= 1);
        }
    }
    for (int status : {STAT_NORMAL, STAT_RESTING, STAT_SLEEPING,
                       STAT_INCAP, STAT_DYING, STAT_DEAD}) {
        for (int posture : {POS_PRONE, POS_SITTING, POS_KNEELING, POS_STANDING}) {
            for (bool meditate : {false, true}) {
                reset();
                ch.specials.position = status | posture;
                if (meditate) SET_BIT(ch.specials.affected_by, AFF_MEDITATE);
                const int base = meditate ? 1000 : 80;
                int rate = status <= STAT_INCAP ? 0 : status == STAT_SLEEPING ?
                           base * 2 : status == STAT_RESTING ? base + base / 2 : base;
                if (status <= STAT_INCAP) ch.points.mana_reg = 1000;
                check_rate(rate);
                if (status <= STAT_INCAP) {
                    GET_MANA(&ch) = GET_MAX_MANA(&ch) + 20;
                    assert(mana_regen(&ch, true) == 0);
                    regen_event_state stale{.9f, 0};
                    event_mana_regen(&ch, nullptr, nullptr, &stale);
                    assert(GET_MANA(&ch) == GET_MAX_MANA(&ch) + 20 && queue.empty());
                }
            }
        }
    }
    for (int mod : {10, -30, -80, -100}) {
        reset(); ch.points.mana_reg = mod; check_rate(80 + mod);
    }
    reset(); ch.curr_stats.Pow = 99; ch.curr_stats.Int = 100; check_rate(79);
    reset(); ch.curr_stats.Pow = 99; ch.curr_stats.Int = 100;
    SET_BIT(ch.specials.affected_by, AFF_MEDITATE); check_rate(995);
    reset(); affected_type af{}; af.type = SPELL_FEEBLEMIND; ch.affected = &af;
    check_rate(20);
    reset(); ch.affected = &af; ch.specials.position = STAT_RESTING | POS_SITTING;
    ch.points.mana_reg = 10; check_rate(40);
    for (bool globe : {false, true}) {
        for (bool dusk : {false, true}) {
            reset(); vulnerable = sun = true; twilight = dusk;
            if (globe) SET_BIT(ch.specials.affected_by4, AFF4_GLOBE_OF_DARKNESS);
            check_rate(globe || dusk ? 80 : 0);
        }
    }
    reset(); sun = true; check_rate(80);
    reset(); vulnerable = true; check_rate(80);
}

static void transitions() {
    reset(); ch.specials.fighting = &enemy; check_rate(0);
    reset(); StartRegen(&ch, regen_resource::mana); advance(3);
    assert(GET_MANA(&ch) == 100 && queue[0].state.accumulated > .7f);
    ch.specials.fighting = &enemy;
    advance(600, 4);
    assert(GET_MANA(&ch) == 100 && queue.empty() && mana_regen(&ch, true) == 0);
    ch.specials.fighting = nullptr;
    StartRegen(&ch, regen_resource::mana);
    assert(queue[0].state.accumulated == 0 && queue[0].state.last_tick == ne_event_tick);
    advance(3); assert(GET_MANA(&ch) == 100);
    advance(1); assert(GET_MANA(&ch) == 101);

    reset(); StartRegen(&ch, regen_resource::mana);
    advance(300); assert(GET_MANA(&ch) == 180);
    SET_BIT(ch.specials.affected_by, AFF_MEDITATE);
    advance(300); assert(std::abs(GET_MANA(&ch) - 1180) <= 1);
    stop_meditation(&ch);
    int before = GET_MANA(&ch); advance(300);
    assert(std::abs(GET_MANA(&ch) - before - 80) <= 1);

    reset(); vulnerable = true; StartRegen(&ch, regen_resource::mana); advance(3);
    sun = true; advance(300); assert(GET_MANA(&ch) == 100 && queue.empty());
    sun = false; StartRegen(&ch, regen_resource::mana); advance(300);
    assert(GET_MANA(&ch) == 180);
    reset(); ch.points.mana_reg = -80; check_rate(0);
    ch.points.mana_reg = 0; check_rate(80);
}

static void bounds_and_spending() {
    for (int mod : {0, -30, -80}) {
        reset(); GET_MAX_MANA(&ch) = GET_MANA(&ch); ch.points.mana_reg = mod;
        assert(mana_regen(&ch, false) == 0 && mana_regen(&ch, true) == 80 + mod);
        StartRegen(&ch, regen_resource::mana); assert(queue.empty());
    }
    for (int dial : {1, 5, 10}) {
        reset(dial); GET_MAX_MANA(&ch) = 100; ch.points.mana_reg = -100;
        assert(mana_regen(&ch, false) == -20);
        StartRegen(&ch, regen_resource::mana); advance(300);
        assert(std::abs(GET_MANA(&ch) - 80) <= 1);
        advance(3000, 300); // A delayed decrement crosses zero and terminates.
        assert(GET_MANA(&ch) == 0 && queue.empty() && mana_regen(&ch, true) == 0);
        StartRegen(&ch, regen_resource::mana); assert(queue.empty());
        reset(dial); GET_MANA(&ch) = 120; GET_MAX_MANA(&ch) = 100;
        assert(mana_regen(&ch, false) == -4);
        StartRegen(&ch, regen_resource::mana);
        while (GET_MANA(&ch) == 120) advance(1);
        assert(GET_MANA(&ch) == 119);
        advance(300); assert(GET_MANA(&ch) < 119 && GET_MANA(&ch) > 100);
        advance(3000, 3000); // Delayed surplus decay must stop at maximum.
        assert(GET_MANA(&ch) == 100 && queue[0].state.accumulated == 0);
        advance(1); assert(queue.empty());
    }
    reset(); GET_MANA(&ch) = 99; GET_MAX_MANA(&ch) = 100;
    SET_BIT(ch.specials.affected_by, AFF_MEDITATE);
    StartRegen(&ch, regen_resource::mana); advance(300);
    assert(GET_MANA(&ch) == 100 && queue.empty() && completion_messages == 1);
    assert(!IS_AFFECTED(&ch, AFF_MEDITATE));
    advance(300); assert(completion_messages == 1);

    for (bool mindflayer : {false, true}) {
        reset(); GET_MAX_MANA(&ch) = GET_MANA(&ch);
        if (mindflayer) { ch.player.race = RACE_PILLITHID; ch.player.m_class = CLASS_MINDFLAYER; }
        StartRegen(&ch, regen_resource::mana); assert(queue.empty());
        use_spell(&ch, SPELL_MAGIC_MISSILE);
        assert(GET_MANA(&ch) == 100 - 2 * MANA_PER_CIRCLE && queue.size() == 1);
        StartRegen(&ch, regen_resource::mana); assert(queue.size() == 1);
        advance(300); assert(GET_MANA(&ch) == 100 && queue.empty());
    }
    reset(5, true); StartRegen(&ch, regen_resource::mana);
    assert(queue.size() == 1 && queue[0].due == 5);
    advance(4); assert(GET_MANA(&ch) == 100);
    advance(1); assert(GET_MANA(&ch) == 101 && queue[0].due == 10);
    advance(5); assert(GET_MANA(&ch) == 102);
}

static void periodic_restart() {
    for (int race : {RACE_HUMAN, RACE_PILLITHID, RACE_ILLITHID}) {
        for (uint cls : {CLASS_WARRIOR, CLASS_SORCERER, CLASS_PSIONICIST, CLASS_MINDFLAYER}) {
            for (bool sunlight : {false, true}) {
                for (int resource : {90, 100, 120}) {
                    reset(); ch.player.race = race; ch.player.m_class = cls;
                    ch.player.level = 60; // trusted; the fixture racial ages are zero
                    character_list = &ch;
                    GET_MANA(&ch) = resource; GET_MAX_MANA(&ch) = 100;
                    GET_MAX_VITALITY(&ch) = GET_VITALITY(&ch);
                    if (resource == 100) ch.points.mana_reg = -100;
                    if (!USES_MANA(character_list)) continue;
                    StartRegen(&ch, regen_resource::mana); advance(3);
                    if (sunlight) vulnerable = sun = true;
                    else ch.specials.fighting = &enemy;
                    advance(1); assert(queue.empty());
                    affect_update(); assert(queue.empty());
                    ch.specials.fighting = nullptr; sun = false;
                    affect_update();
                    assert(queue.size() == 1 && queue[0].func == event_mana_regen);
                    assert(queue[0].state.accumulated == 0 &&
                           queue[0].state.last_tick == ne_event_tick);
                    affect_update(); assert(queue.size() == 1);
                    advance(300);
                    assert(resource < 100 ? GET_MANA(&ch) > resource :
                                           GET_MANA(&ch) < resource);
                }
            }
        }
    }
    reset(); ch.player.level = 60; ch.player.m_class = CLASS_WARRIOR;
    GET_MAX_VITALITY(&ch) = GET_VITALITY(&ch); character_list = &ch;
    affect_update(); assert(queue.empty());
    character_list = nullptr;
    puts("Periodic PSP restart after combat/sunlight covers racial users and signed/full rates");
}

int main(int argc, char **argv) {
    assert(argc == 2);
    std::ifstream input("lib/duris.properties");
    assert(input.good());
    for (std::string line; std::getline(input, line);) {
        auto sep = line.find('=');
        if (line.rfind("difficulty.", 0) == 0 && sep != std::string::npos)
            properties[line.substr(0, sep)] = std::stof(line.substr(sep + 1));
    }
    if (!std::strcmp(argv[1], "rates")) rates();
    else if (!std::strcmp(argv[1], "transitions")) transitions();
    else if (!std::strcmp(argv[1], "bounds")) bounds_and_spending();
    else if (!std::strcmp(argv[1], "restart")) periodic_restart();
    else assert(false);
    std::printf("PSP production case passed: %s\n", argv[1]);
}
'''

with tempfile.TemporaryDirectory(prefix="psp-regen-") as directory:
    path = Path(directory)
    harness, binary = path / "harness.cpp", path / "harness"
    harness.write_text(PRELUDE + HELPERS + DRIVER)
    subprocess.run([
        "g++", "-std=c++20", "-O1", "-fsanitize=address,undefined",
        "-fno-sanitize-recover=all", "-fno-pie", "-no-pie",
        "-fno-omit-frame-pointer", "-ffunction-sections", "-fdata-sections",
        "-I" + str(ROOT / "src"), str(harness), str(HARNESS_STUBS),
        "-Wl,--gc-sections", "-o", str(binary),
    ], check=True, cwd=ROOT)
    failures = 0
    for mode in ("rates", "transitions", "bounds", "restart"):
        failures += subprocess.run([str(binary), mode], cwd=ROOT, env={
            **os.environ, "ASAN_OPTIONS": "detect_leaks=1:halt_on_error=1",
            "UBSAN_OPTIONS": "halt_on_error=1:print_stacktrace=1",
        }).returncode != 0
    assert not failures, f"{failures} PSP scenarios failed"

