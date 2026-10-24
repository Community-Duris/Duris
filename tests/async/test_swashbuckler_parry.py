#!/usr/bin/env python3
"""Execute the production parry decision with deterministic opposed rolls."""

from pathlib import Path
import subprocess
import tempfile

from _paths import ROOT, extract_function

PRELUDE = r'''
#include "core/prototypes.h"
#include "core/utils.h"
#include "net/comm.h"
#include "item/objmisc.h"
#include "magic/spells.h"
#include <cassert>
#include <cstdio>

str_app_type str_app[1]{};
dex_app_type dex_app[1]{};
wis_app_type wis_app[1]{};
static int attacker_percent, defender_percent, opposed_calls, attacker_bound, defender_bound;
static bool repeated_draw = false;
int STAT_INDEX(int) { return 0; }
int GET_CLASS(P_char ch, uint cls) { return (ch->player.m_class & cls) != 0; }
int GET_CHAR_SKILL_P(P_char, int skill) { return skill == SKILL_PARRY ? 100 : 0; }
bool affected_by_spell(P_char, int) { return false; }
bool notch_skill(P_char, int, float) { return false; }
float get_property(const char *, double fallback) { return fallback; }
int get_property(const char *, int fallback) { return fallback; }
int WeaponSkill(P_char, P_obj) { return 100; }
bool is_wielding_paladin_sword(P_char) { return false; }
bool rapier_dirk(P_char, P_char) { return false; }
int try_riposte(P_char, P_char, P_obj) { return false; }
void act(const char *, int, P_char, P_obj, void *, int) {}
int number(int low, int high) {
    if (low == 0) return 0; // equal luck, no random bonus
    if (high == 250) return high; // no expert parry message
    assert(low == 1 && high >= low);
    assert(opposed_calls < 2);
    int value;
    if (opposed_calls == 0) {
        defender_bound = high;
        value = MAX(1, high * defender_percent / 100);
    } else {
        attacker_bound = high;
        value = MAX(1, high * attacker_percent / 100);
    }
    // Production MAX(5, number(...)) evaluates number twice when its first
    // result is >= 5. Return the same quantile for both draws in that phase.
    if (value < 5 || repeated_draw) {
        ++opposed_calls;
        repeated_draw = false;
    } else repeated_draw = true;
    return value;
}
'''

DRIVER = r'''
static int check(P_char victim, P_char attacker, P_obj weapon, int spec) {
    attacker->player.spec = spec;
    opposed_calls = 0;
    repeated_draw = false;
    const int result = parrySucceed(victim, attacker, weapon);
    assert(opposed_calls == 2);
    return result;
}

int main() {
    for (bool npc : {false, true}) {
        char_data attacker{}, victim{};
        obj_data sword{};
        attacker.player.m_class = victim.player.m_class = CLASS_WARRIOR;
        attacker.player.race = victim.player.race = RACE_HUMAN;
        attacker.specials.position = victim.specials.position = STAT_NORMAL + POS_STANDING;
        if (npc) attacker.specials.act = victim.specials.act = ACT_ISNPC;
        victim.specials.fighting = &attacker;
        victim.equipment[WIELD] = &sword;
        sword.type = ITEM_WEAPON;
        int ordinary_parries = 0, swashbuckler_parries = 0;
        for (attacker_percent = 1; attacker_percent <= 100; ++attacker_percent) {
            for (defender_percent = 1; defender_percent <= 100; ++defender_percent) {
                const int ordinary = check(&victim, &attacker, &sword, 0);
                assert(attacker_bound == 100 && defender_bound == 100);
                assert(check(&victim, &attacker, &sword, SPEC_SWORDSMAN) == ordinary);
                assert(attacker_bound == 100 && defender_bound == 100);
                const int swashbuckler = check(&victim, &attacker, &sword, SPEC_SWASHBUCKLER);
                assert(attacker_bound == 110 && defender_bound == 100);
                assert(swashbuckler <= ordinary);
                ordinary_parries += ordinary;
                swashbuckler_parries += swashbuckler;
            }
        }
        assert(swashbuckler_parries < ordinary_parries);
        printf("%s opposed-roll grid: ordinary=%d, Swashbuckler=%d parries\n",
               npc ? "NPC" : "PC", ordinary_parries, swashbuckler_parries);

        // Existing defender-side specialization bonus remains independent.
        victim.player.spec = SPEC_SWASHBUCKLER;
        attacker_percent = 90;
        defender_percent = 95;
        assert(check(&victim, &attacker, &sword, SPEC_SWASHBUCKLER));
        assert(attacker_bound == 110 && defender_bound == 110);
        victim.player.spec = 0;
        assert(!check(&victim, &attacker, &sword, SPEC_SWASHBUCKLER));
        assert(check(&victim, &attacker, &sword, SPEC_SWORDSMAN));
    }
    puts("Swashbuckler anti-parry direction and unchanged attacker/defender controls passed");
}
'''

with tempfile.TemporaryDirectory(prefix="duris-swashbuckler-parry-") as temporary:
    harness = Path(temporary) / "harness.cpp"
    binary = Path(temporary) / "harness"
    harness.write_text(PRELUDE + extract_function("fight.c", "int parrySucceed(") + DRIVER)
    subprocess.run([
        "g++", "-std=c++20", "-Wall", "-Wextra", "-Werror",
        "-fsanitize=address,undefined", "-I" + str(ROOT / "src"),
        str(harness), "-o", str(binary),
    ], check=True)
    subprocess.run([str(binary)], check=True)
