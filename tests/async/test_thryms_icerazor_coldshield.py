#!/usr/bin/env python3
"""Execute the production Thrym's Icerazor proc with scripted rolls.

The Coldshield damage bonus tested IS_AFFECTED2(ch, AFF3_COLDSHIELD). That mask
is BIT_22, which in affected_by2 is AFF2_WATER_AURA, so a real Coldshield gave
no bonus and a Water Aura alone did (issue #92). The bonus now reads
affected_by3, like the Coldshield Slow proc below it.

The harness compiles the real thryms_icerazor(). Controlled number() rolls pick
the base damage, the Coldshield bonus, the Fireshield penalty and the Slow
proc; spell_damage() and affect_to_char() record what the proc asked for.
"""

from pathlib import Path
import subprocess
import tempfile

from _paths import ROOT, HARNESS_STUBS, extract_function


ICERAZOR = extract_function("reavers.c", "bool thryms_icerazor(P_char ch, P_char victim, P_obj wpn)")

PRELUDE = r'''
#include "core/prototypes.h"
#include "core/utils.h"
#include "combat/damage.h"
#include "item/objmisc.h"
#include "magic/spells.h"
#include "net/comm.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

static int base_roll = 7;    // number(8, 6)
static int bonus_roll = 3;   // number(4, 3)
static int slow_roll = 100;  // number(1, 100)
static int bonus_rolls, slow_rolls;
static int damage_calls, damage_type, damage_result;
static uint damage_flags;
static double damage_dealt;
static int applications;
static affected_type applied;

int number(int low, int high)
{
    if (low == 8 && high == 6)
        return base_roll;
    if (low == 4 && high == 3)
    {
        ++bonus_rolls;
        return bonus_roll;
    }
    if (low == 4 && high == 4)
        return 4;
    if (low == 1 && high == 100)
    {
        ++slow_rolls;
        return slow_roll;
    }
    std::abort();
}

int spell_damage(P_char, P_char, double dam, int type, uint flags, damage_messages *, int *)
{
    ++damage_calls;
    damage_dealt = dam;
    damage_type = type;
    damage_flags = flags;
    return damage_result;
}

affected_type *affect_to_char(P_char, affected_type *af)
{
    ++applications;
    applied = *af;
    return nullptr;
}
'''

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

static void require_damage(const char *name, double expected)
{
    require(damage_calls == 1, name, "spell_damage was not called once");
    if (damage_dealt != expected)
    {
        std::fprintf(stderr, "%s: damage %.0f, expected %.0f\n", name, damage_dealt, expected);
        ++failures;
    }
    require(damage_type == SPLDAM_COLD, name, "damage is not cold");
    require(damage_flags == (SPLDAM_NODEFLECT | SPLDAM_NOSHRUG), name, "damage flags changed");
}

static char_data attacker, victim;
static obj_data weapon;

static bool strike(ulong affects2, ulong affects3, int weapon_type = WEAPON_MACE)
{
    attacker = {};
    victim = {};
    weapon = {};
    SET_POS(&attacker, POS_STANDING + STAT_NORMAL);
    SET_POS(&victim, POS_STANDING + STAT_NORMAL);
    attacker.specials.affected_by2 = affects2;
    attacker.specials.affected_by3 = affects3;
    weapon.type = ITEM_WEAPON;
    weapon.value[0] = weapon_type;
    bonus_rolls = slow_rolls = damage_calls = applications = 0;
    damage_dealt = 0;
    return thryms_icerazor(&attacker, &victim, &weapon);
}

int main()
{
    // The bit collision the bug hid behind.
    require(AFF3_COLDSHIELD == AFF2_WATER_AURA, "masks", "Coldshield no longer shares a bit with Water Aura");

    // Every base roll, with and without Coldshield and Water Aura.
    for (int base = 6; base <= 8; ++base)
    {
        base_roll = base;
        for (int bonus = 3; bonus <= 4; ++bonus)
        {
            bonus_roll = bonus;

            strike(0, 0);
            require_damage("no shield", base);
            require(bonus_rolls == 0, "no shield", "rolled a Coldshield bonus");

            strike(0, AFF3_COLDSHIELD);
            require_damage("Coldshield only", base + bonus);
            require(bonus_rolls == 1, "Coldshield only", "no Coldshield bonus roll");

            strike(AFF2_WATER_AURA, 0);
            require_damage("Water Aura only", base);
            require(bonus_rolls == 0, "Water Aura only", "Water Aura granted the Coldshield bonus");

            strike(AFF2_WATER_AURA, AFF3_COLDSHIELD);
            require_damage("Coldshield and Water Aura", base + bonus);
        }
    }

    // Fireshield still takes 4 and the floor is still 1.
    base_roll = 7;
    bonus_roll = 3;
    strike(AFF2_FIRESHIELD, AFF3_COLDSHIELD);
    require_damage("Coldshield and Fireshield", 7 + 3 - 4);
    base_roll = 6;
    strike(AFF2_FIRESHIELD, 0);
    require_damage("Fireshield only", 2);

    // Slow: Coldshield and a 5% roll, read from affected_by3.
    base_roll = 7;
    slow_roll = 5;
    strike(0, AFF3_COLDSHIELD);
    require(applications == 1, "Slow proc", "Coldshield on a 5 did not slow");
    require(applied.type == SPELL_SLOW && applied.bitvector2 == AFF2_SLOW, "Slow proc", "wrong slow affect");
    require(applied.duration == 2 && applied.modifier == 2, "Slow proc", "slow strength changed");
    slow_roll = 6;
    strike(0, AFF3_COLDSHIELD);
    require(applications == 0, "Slow proc miss", "slowed on a 6");
    slow_roll = 5;
    strike(AFF2_WATER_AURA, 0);
    require(applications == 0 && slow_rolls == 0, "Slow without Coldshield", "Water Aura reached the slow proc");

    // Non-bludgeoning weapons get no proc.
    require(!strike(0, AFF3_COLDSHIELD, WEAPON_LONGSWORD), "sword", "proc reported a kill");
    require(damage_calls == 0 && bonus_rolls == 0, "sword", "a sword procced");

    // A kill is still reported; a survivor is not.
    slow_roll = 100;
    damage_result = DAM_VICTDEAD;
    require(strike(0, AFF3_COLDSHIELD), "killing strike", "kill not reported");
    damage_result = DAM_NONEDEAD;
    require(!strike(0, AFF3_COLDSHIELD), "surviving strike", "survivor reported as a kill");

    if (failures)
        return 1;
    std::puts("Thrym's Icerazor Coldshield runtime regressions passed");
    return 0;
}
'''


with tempfile.TemporaryDirectory(prefix="duris-icerazor-") as directory:
    temporary = Path(directory)
    harness = temporary / "harness.cpp"
    binary = temporary / "harness"
    harness.write_text("\n".join((PRELUDE, ICERAZOR, DRIVER)), encoding="utf-8")
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
