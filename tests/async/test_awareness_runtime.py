#!/usr/bin/env python3
"""Exercise trained Awareness scaling and its active hamstring defense."""
from pathlib import Path
import subprocess
import tempfile

from _paths import ROOT, extract_function, source
from _source_contract import function_body

PRELUDE = r'''
#include "core/prototypes.h"
#include "core/utils.h"
#include "net/comm.h"
#include "magic/spells.h"
#include "world/events.h"
#include "item/objmisc.h"
#include <cassert>
#include <cstdio>
#include <cstring>

static room_data rooms[1]{};
P_room world = rooms;
static int trained = 0, hamstring_roll = 87, regenerations = 0;
static bool defending = false;
int GET_CHAR_SKILL_P(P_char, int skill) {
    return skill == SKILL_AWARENESS ? trained : skill == SKILL_HAMSTRING ? 100 : 0;
}
int number(int low, int high) {
    if (defending && low == 1 && high == 100) return hamstring_roll;
    return defending ? low : high;
}
bool notch_skill(P_char, int, float) { return false; }
int get_property(const char *, int fallback) { return fallback; }
void send_to_char(const char *, P_char) {}
void act(const char *, int, P_char, P_obj, void *, int) {}
void logit(const char *, const char *, ...) { assert(false); }
void CharWait(P_char, int) {}
void StartRegen(P_char, regen_resource resource) {
    assert(resource == regen_resource::vitality); ++regenerations;
}
void appear(P_char, bool) {}
void stop_fighting(P_char) { assert(false); }
P_char ParseTarget(P_char, char *) { assert(false); return nullptr; }
bool ac_can_see(P_char, P_char, bool) { return true; }
void remember(P_char, P_char) { assert(false); }
void MobStartFight(P_char, P_char) { assert(false); }
int char_in_list(const P_char) { return true; }
int CanDoFightMove(P_char, P_char) { return true; }
void update_pos(P_char) { assert(false); }
static void rebuild_flags(P_char ch) {
    ch->specials.affected_by = 0;
    for (auto *af = ch->affected; af; af = af->next)
        ch->specials.affected_by |= af->bitvector;
}
affected_type *affect_to_char(P_char ch, affected_type *af) {
    auto *copy = new affected_type(*af);
    copy->next = ch->affected;
    ch->affected = copy;
    rebuild_flags(ch);
    return copy;
}
void affect_remove(P_char ch, affected_type *af) {
    auto **link = &ch->affected;
    while (*link && *link != af) link = &(*link)->next;
    assert(*link == af);
    *link = af->next;
    delete af;
    rebuild_flags(ch);
}
bool affected_by_spell(P_char ch, int spell) {
    for (auto *af = ch->affected; af; af = af->next)
        if (af->type == spell) return true;
    return false;
}
'''

DRIVER = r'''
static void clear(P_char ch) {
    while (ch->affected) affect_remove(ch, ch->affected);
}
static void hamstring(P_char attacker, P_char victim, bool expect_hit) {
    GET_VITALITY(victim) = 200;
    regenerations = 0;
    defending = true;
    do_hamstring(attacker, nullptr, 0);
    defending = false;
    assert(GET_VITALITY(victim) == (expect_hit ? 160 : 200));
    assert(regenerations == (expect_hit ? 1 : 0));
}
int main() {
    char_data defender{}, attacker{};
    obj_data sword{};
    defender.player.race = attacker.player.race = RACE_HUMAN;
    defender.player.level = attacker.player.level = 56;
    defender.specials.act = attacker.specials.act = PLR_MORTAL;
    SET_POS(&defender, STAT_NORMAL + POS_STANDING);
    SET_POS(&attacker, STAT_NORMAL + POS_STANDING);
    defender.curr_stats.Int = defender.curr_stats.Dex = 100;
    defender.in_room = attacker.in_room = 0;
    attacker.specials.fighting = &defender;
    sword.type = ITEM_WEAPON;
    sword.value[0] = WEAPON_LONGSWORD;
    attacker.equipment[WIELD] = &sword;
    const int base = STAT_INDEX(100) * 2 + (56 >> 3);

    // Refusing an untrained user or NPC must not install an affect.
    do_awareness(&defender, nullptr, 0);
    assert(!defender.affected);
    defender.specials.act = ACT_ISNPC;
    trained = 100;
    do_awareness(&defender, nullptr, 0);
    assert(!defender.affected);
    defender.specials.act = PLR_MORTAL;
    for (int skill : {1, 30, 74, 75, 99, 100}) {
        trained = skill;
        do_awareness(&defender, nullptr, 0);
        assert(defender.affected && !defender.affected->next);
        assert(defender.affected->type == SKILL_AWARENESS);
        assert(defender.affected->modifier == base * skill / 75);
        assert(IS_AFFECTED(&defender, AFF_SKILL_AWARE));
        assert(!IS_AFFECTED(&defender, AFF_AWARE));
        assert(defender.affected->duration == (1 + (56 >> 3) * skill / 75) * (STAT_INDEX(100) / 9));
        if (skill >= 30 && skill < 75) assert(defender.affected->modifier > 0);
    }
    clear(&defender);
    hamstring(&attacker, &defender, true);
    clear(&defender);

    trained = 30;
    do_awareness(&defender, nullptr, 0);
    hamstring(&attacker, &defender, false);
    // The existing awake requirement and general Awareness precedence survive.
    SET_POS(&defender, STAT_SLEEPING + POS_PRONE);
    hamstring(&attacker, &defender, true);
    SET_POS(&defender, STAT_NORMAL + POS_STANDING);
    affected_type general{};
    general.type = SPELL_BLESS;
    general.bitvector = AFF_AWARE;
    auto *ordinary = affect_to_char(&defender, &general);
    hamstring(&attacker, &defender, true);
    do_awareness(&defender, nullptr, 0);
    assert(ordinary == defender.affected->next && ordinary->type == SPELL_BLESS);
    assert(IS_AFFECTED(&defender, AFF_AWARE) && IS_AFFECTED(&defender, AFF_SKILL_AWARE));
    clear(&defender);
    puts("Awareness scaling, refresh, flag separation and real hamstring defense passed");
}
'''

# Ignore the archived, commented-out STAT_INDEX implementation in utility.c.
stat_body = function_body(source("utility.c").read_text(), r"\bint\s+STAT_INDEX\s*\(")
assert stat_body
production = "int STAT_INDEX(int v)\n" + stat_body + "\n"
production += extract_function("new_skills.c", "void do_awareness(") + "\n"
production += extract_function("actnew.c", "void do_hamstring(")
with tempfile.TemporaryDirectory(prefix="duris-awareness-") as temporary:
    harness = Path(temporary) / "harness.cpp"
    binary = Path(temporary) / "harness"
    harness.write_text(PRELUDE + production + DRIVER)
    subprocess.run([
        "g++", "-std=c++20", "-Wall", "-Wextra", "-Werror",
        "-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-fno-pie", "-no-pie",
        "-I" + str(ROOT / "src"), str(harness), "-o", str(binary),
    ], check=True)
    subprocess.run([str(binary)], check=True)
