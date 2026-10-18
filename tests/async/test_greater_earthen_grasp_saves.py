#!/usr/bin/env python3
"""Execute the production Earthen Grasp save chain with scripted rolls.

Greater Earthen Grasp used the hold duration (attdiff, in pulses) as its
paralysis save modifier, which NewSaves() multiplies by 5, so a STR edge
decided the hold almost alone. It now saves on a capped STR modifier less the
spell.gEarthGrasp.paraSaveBonus property, while single Earthen Grasp keeps its
original saves (issue #35).

The harness compiles the real earthen_grasp(), its two spell callers and
check_freedom_of_movement() and NewSaves(). Controlled d100 rolls exercise real
save thresholds; rolls 1/100 also force branches for the duration and immunity
cases. Dice, damage, messages and affects record what the spell asked for.
"""

from pathlib import Path
import re
import subprocess
import tempfile

from _paths import ROOT, HARNESS_STUBS, extract_function, source
from _source_contract import function_body, strip_comments


BOUNDED = extract_function("utility.c", "int BOUNDED(int a, int b, int c)")
FREEDOM = extract_function("magic.c", "bool check_freedom_of_movement(P_char ch, bool clear)")
GRASP = extract_function(
    "smagic.c", "void earthen_grasp(int level, P_char ch, P_char victim, bool greater)"
)
SAVES = extract_function("sparser.c", "bool NewSaves(P_char ch, int save_type, int mod)").replace(
    "bool NewSaves(", "bool production_saves(", 1
)
SINGLE = extract_function("smagic.c", "void spell_earthen_grasp(int level")
GREATER_TARGET = extract_function("smagic.c", "void spell_single_earthen_grasp(int level")

PRELUDE = r'''
#include "core/prototypes.h"
#include "core/utils.h"
#include "combat/damage.h"
#include "magic/spells.h"
#include "net/comm.h"
#include "world/difficulty.h"
#include "world/difficulty_math.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <vector>

static std::deque<int> para_rolls;
static std::deque<bool> para_outcomes;  // next SAVING_PARA results, in call order
static std::vector<int> para_mods;      // the mod each SAVING_PARA call received
static int jitter;                      // number(-WAIT_SEC, WAIT_SEC)
static int freedom_clear;               // number(0, 1)
static double para_bonus = 2.0;
static int bonus_reads;
static bool grasped, freedom, freedom_removed, victim_dead;
static ulong victim_act;
static int damage_result, damage_calls, applications;
static affected_type applied;

int find_save(P_char, int) { return 45; }
bool difficulty_world_npc(P_char) { return false; }
double difficulty_multiplier(difficulty_dial) { return 1.0; }
bool production_saves(P_char, int, int);

bool NewSaves(P_char ch, int save_type, int mod)
{
    if (save_type == SAVING_SPELL)
        return true; // the damage save; not under test
    if (save_type != SAVING_PARA)
        std::abort();
    para_mods.push_back(mod);
    return production_saves(ch, save_type, mod);
}

int number(int low, int high)
{
    if (low == 1 && high == 100)
    {
        if (!para_rolls.empty())
        {
            int roll = para_rolls.front();
            para_rolls.pop_front();
            return roll;
        }
        if (para_outcomes.empty())
            std::abort();
        int roll = para_outcomes.front() ? 100 : 1;
        para_outcomes.pop_front();
        return roll;
    }
    if (low == -WAIT_SEC && high == WAIT_SEC)
        return jitter;
    if (low == 0 && high == 1)
        return freedom_clear;
    std::abort();
}

float get_property(const char *name, double fallback)
{
    if (!std::strcmp(name, "spell.gEarthGrasp.paraSaveBonus"))
    {
        ++bonus_reads;
        return (float)para_bonus;
    }
    return (float)fallback;
}

bool affected_by_spell(P_char, int spell)
{
    return (spell == SPELL_EARTHEN_GRASP && grasped) ||
           (spell == SPELL_FREEDOM_OF_MOVEMENT && freedom);
}

void affect_from_char(P_char, int spell)
{
    if (spell == SPELL_FREEDOM_OF_MOVEMENT)
        freedom_removed = true;
}

affected_type *affect_to_char(P_char, affected_type *af)
{
    ++applications;
    applied = *af;
    return nullptr;
}

int spell_damage(P_char, P_char, double, int, uint, damage_messages *, int *)
{
    ++damage_calls;
    return damage_result;
}

bool has_innate(P_char, int) { return false; }
int get_default_save_mod(P_char, P_char, int, int) { return 0; }
int dice(int, int) { return 1; }
void stop_fighting(P_char) {}
void StopMercifulAttackers(P_char) {}
''' + BOUNDED + FREEDOM + SAVES

DRIVER = r'''
static int failures;

static void require(bool ok, const char *name, const char *what)
{
    if (!ok)
    {
        std::fprintf(stderr, "%s: %s\n", name, what);
        ++failures;
    }
}

static void require_mods(const char *name, std::vector<int> expected)
{
    if (para_mods == expected)
        return;
    std::fprintf(stderr, "%s: PARA save mods were", name);
    for (int mod : para_mods)
        std::fprintf(stderr, " %d", mod);
    std::fprintf(stderr, ", expected");
    for (int mod : expected)
        std::fprintf(stderr, " %d", mod);
    std::fputc('\n', stderr);
    ++failures;
}

// Expected hold: 0 for none, otherwise the duration in pulses.
static void require_hold(const char *name, int duration)
{
    if (!duration)
    {
        require(applications == 0, name, "a hold was applied");
        return;
    }
    require(applications == 1, name, "no hold was applied");
    require(applied.type == SPELL_EARTHEN_GRASP, name, "wrong affect type");
    require(applied.bitvector2 == AFF2_MINOR_PARALYSIS, name, "hold is not minor paralysis");
    require(IS_SET(applied.flags, AFFTYPE_SHORT), name, "hold is not in pulses");
    if (applied.duration != duration)
    {
        std::fprintf(stderr, "%s: duration %d, expected %d\n", name, applied.duration, duration);
        ++failures;
    }
}

static char_data caster, victim;

static void cast(bool greater, int caster_str, int victim_str, std::deque<bool> outcomes)
{
    caster = {};
    victim = {};
    SET_POS(&caster, POS_STANDING + STAT_NORMAL);
    SET_POS(&victim, POS_STANDING + STAT_NORMAL);
    caster.in_room = 1;
    caster.curr_stats.Str = caster_str;
    victim.curr_stats.Str = victim_str;
    victim.specials.act = victim_act;
    if (victim_dead)
        SET_POS(&victim, POS_STANDING + STAT_DEAD);
    para_outcomes = outcomes;
    para_mods.clear();
    applications = damage_calls = 0;
    applied = {};
    freedom_removed = false;
    if (greater)
        spell_single_earthen_grasp(56, &caster, nullptr, 0, &victim, nullptr);
    else
        spell_earthen_grasp(56, &caster, nullptr, 0, &victim, nullptr);
}

constexpr bool FAIL = false, SAVE = true;

int main()
{
    // Greater Earthen Grasp: mod = BOUNDED(-6, STR gap / 15, 6) - bonus.
    para_bonus = 2.0;
    jitter = 0;
    cast(true, 100, 100, {FAIL, FAIL});
    require_mods("equal STR, long hold", {-2, -2});
    require_hold("equal STR, long hold", 5 * WAIT_SEC);
    require(damage_calls == 1, "equal STR, long hold", "grasp did no damage");

    jitter = WAIT_SEC;
    cast(true, 160, 100, {FAIL, SAVE, FAIL});
    require_mods("+60 STR, short hold", {2, 0, 1});
    require_hold("+60 STR, short hold", 60 / 3 + WAIT_SEC);

    cast(true, 160, 100, {FAIL, SAVE, SAVE});
    require_mods("+60 STR, both easier saves made", {2, 0, 1});
    require_hold("+60 STR, both easier saves made", 0);
    require(damage_calls == 0, "+60 STR, both easier saves made", "a missed grasp did damage");

    jitter = 0;
    cast(true, 189, 100, {FAIL, FAIL});
    require_mods("+89 STR, below the cap", {3, 1});
    cast(true, 190, 100, {FAIL, FAIL});
    require_mods("+90 STR, at the cap", {4, 1});
    cast(true, 220, 100, {FAIL, FAIL});
    require_mods("+120 STR, capped", {4, 1});
    cast(true, 100, 220, {FAIL, SAVE, SAVE});
    require_mods("-120 STR, capped", {-8, -8, -8});

    // Integer division truncates toward zero on both the STR gap and the divided saves.
    cast(true, 71, 100, {FAIL, SAVE, SAVE});
    require_mods("-29 STR truncates toward zero", {-3, -3, -3});

    // Passing the STR save leaves only the fallback save, at the bonus alone.
    cast(true, 220, 100, {SAVE, FAIL});
    require_mods("fallback save, bonus 2", {4, -2});
    require_hold("fallback save, bonus 2", 40);
    cast(true, 220, 100, {SAVE, SAVE});
    require_mods("fallback save made", {4, -2});
    require_hold("fallback save made", 0);

    // Bonus 0 keeps the capped scale; it does not fall back to the duration.
    para_bonus = 0.0;
    jitter = WAIT_SEC;
    cast(true, 160, 100, {FAIL, SAVE, FAIL});
    require_mods("bonus 0, +60 STR", {4, 1, 2});
    cast(true, 220, 100, {SAVE, FAIL});
    require_mods("bonus 0, fallback", {6, 0});

    // The bonus is read on every Greater cast, so a property reload takes effect at once.
    bonus_reads = 0;
    para_bonus = 3.0;
    cast(true, 100, 100, {FAIL, FAIL});
    require_mods("bonus 3 after reload", {-3, -3});
    require(bonus_reads == 1, "bonus 3 after reload", "bonus was not read per cast");

    // Sweep real NewSaves d100 thresholds for all follow-up branches. With a
    // base threshold of 45, mods -2 and -8 fail at <=35 and <=5, respectively.
    // Both follow-ups must keep those easier odds, including the roll boundary.
    para_bonus = 2.0;
    jitter = 0;
    for (int str : {100, 10})
    {
        int threshold = str == 100 ? 35 : 5;
        for (int roll = 1; roll <= 100; ++roll)
        {
            para_rolls = {1, roll, 100};
            cast(true, str, 100, {});
            require_hold("real long-save threshold", roll <= threshold ? 5 * WAIT_SEC : 0);
            para_rolls = {1, 100, roll};
            cast(true, str, 100, {});
            require_hold("real short-save threshold", roll <= threshold ? WAIT_SEC : 0);
        }
    }
    para_rolls.clear();

    // Single Earthen Grasp keeps the duration as its save mod and never reads the bonus.
    bonus_reads = 0;
    para_bonus = 2.0;
    jitter = WAIT_SEC;
    cast(false, 160, 100, {FAIL, SAVE, FAIL});
    require_mods("single grasp, +60 STR", {24, 8, 12});
    require_hold("single grasp, +60 STR", 24);
    cast(false, 160, 100, {SAVE, FAIL});
    require_mods("single grasp, fallback", {24, 0});
    jitter = 0;
    cast(false, 100, 160, {FAIL, SAVE, SAVE});
    require_mods("single grasp, negative mods unchanged", {-20, -6, -10});
    cast(false, 250, 100, {FAIL, FAIL});
    require_mods("single grasp, duration bound", {40, 13});
    require(bonus_reads == 0, "single grasp", "single grasp read the GEG bonus");

    // Durations still come from attdiff, with the same limits, for both spells.
    jitter = 0;
    cast(true, 250, 100, {FAIL, FAIL});
    require_hold("longest hold", 40 + 5 * WAIT_SEC);
    jitter = -WAIT_SEC;
    cast(true, 100, 100, {FAIL, SAVE, FAIL});
    require_hold("shortest hold", WAIT_SEC);
    cast(true, 40, 100, {FAIL, FAIL});
    require_hold("long hold floor", 5 * WAIT_SEC);

    // Immunity, an existing grasp and Freedom of Movement stop the spell before any save.
    jitter = 0;
    std::deque<bool> none;
    victim_act = ACT_ISNPC | ACT_IMMUNE_TO_PARA;
    cast(true, 220, 100, none);
    require_mods("paralysis-immune NPC", {});
    require_hold("paralysis-immune NPC", 0);
    victim_act = 0;

    grasped = true;
    cast(true, 220, 100, none);
    require_mods("already grasped", {});
    require(damage_calls == 0, "already grasped", "a second fist did damage");
    grasped = false;

    freedom = true;
    freedom_clear = 1;
    cast(true, 220, 100, none);
    require_mods("Freedom of Movement", {});
    require_hold("Freedom of Movement", 0);
    require(freedom_removed, "Freedom of Movement", "spell was not spent on a clear roll");
    freedom_clear = 0;
    cast(true, 220, 100, none);
    require(!freedom_removed, "Freedom of Movement kept", "spell spent without a clear roll");
    freedom = false;

    // A grasp that kills applies no hold; a dead target is never grasped.
    damage_result = DAM_VICTDEAD;
    cast(true, 100, 100, {FAIL, FAIL});
    require(damage_calls == 1, "killing grasp", "no damage was dealt");
    require_hold("killing grasp", 0);
    damage_result = DAM_NONEDEAD;

    victim_dead = true;
    cast(true, 100, 100, none);
    require_mods("dead target", {});
    require(damage_calls == 0, "dead target", "a dead target was damaged");

    if (failures)
        return 1;
    std::puts("Earthen Grasp save chain runtime regressions passed");
    return 0;
}
'''


with tempfile.TemporaryDirectory(prefix="duris-earthen-grasp-") as directory:
    temporary = Path(directory)
    harness = temporary / "harness.cpp"
    binary = temporary / "harness"
    harness.write_text("\n".join((PRELUDE, GRASP, SINGLE, GREATER_TARGET, DRIVER)), encoding="utf-8")
    subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Werror",
            "-Wno-unused-parameter",
            "-fsanitize=address,undefined",
            "-ffunction-sections",
            "-fdata-sections",
            f"-I{ROOT / 'src'}",
            str(harness),
            "-Wl,--gc-sections",
            str(HARNESS_STUBS),
            "-o",
            str(binary),
        ],
        cwd=ROOT,
        check=True,
    )
    subprocess.run([str(binary)], cwd=ROOT, check=True)


# Every Greater Earthen Grasp cast reaches the greater path: the spell itself (and so
# NPC casts, which go through the skill table) and the elemental storm use the same
# per-target callback. The Earth Reaver keeps the single-target spell.
SMAGIC = strip_comments(source("smagic.c").read_text(encoding="utf-8"))
greater = function_body(SMAGIC, r"\bvoid\s+spell_greater_earthen_grasp\s*\(")
assert re.search(r"cast_as_damage_area\(\s*ch\s*,\s*spell_single_earthen_grasp\b", greater), (
    "Greater Earthen Grasp must hit each target through spell_single_earthen_grasp"
)
assert re.search(r"spell_greater_earthen_grasp\(\s*GET_LEVEL\(ch\)", SMAGIC), (
    "the elemental storm must still cast Greater Earthen Grasp"
)
SKILLS = strip_comments(source("skills.c").read_text(encoding="utf-8"))
assert re.search(
    r'"greater earthen grasp"\s*,\s*SPELL_GREATER_EARTHEN_GRASP\s*,[^;]*?spell_greater_earthen_grasp',
    SKILLS,
    re.S,
), "the skill table must map Greater Earthen Grasp to spell_greater_earthen_grasp"
REAVERS = strip_comments(source("reavers.c").read_text(encoding="utf-8"))
assert "spell_earthen_grasp(" in REAVERS and "spell_single_earthen_grasp" not in REAVERS

properties = (ROOT / "lib/duris.properties").read_text(encoding="utf-8")
assert "\nspell.gEarthGrasp.paraSaveBonus=2.000\n" in properties

print("Greater Earthen Grasp save regressions passed")
