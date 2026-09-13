#!/usr/bin/env python3
"""#252: execute production SpellCastTime against bounded modifier fixtures."""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
text = (ROOT / "src/net/sparser.c").read_text()
start = text.index("int SpellCastTime(P_char ch, int spl)")
opening = text.index("{", start)
depth = 0
for pos in range(opening, len(text)):
    depth += (text[pos] == "{") - (text[pos] == "}")
    if depth == 0:
        body = text[start:pos + 1]
        break
else:
    raise AssertionError("unterminated SpellCastTime")

harness = r'''
#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utils.h"
#include "magic/spells.h"
#include <cassert>
#include <cstdio>
#include <cstring>

Skill skills[MAX_AFFECT_TYPES + 1];
float spell_pulse_data[LAST_RACE + 1] = {};
static float multiplier = 1.0f;
static bool missing = false;
static int calls = 0;
float get_property(const char *key, double fallback)
{
    assert(std::strcmp(key, "spellcast.pulse.racial.All") == 0);
    assert(fallback == 1.0);
    ++calls;
    return missing ? static_cast<float>(fallback) : multiplier;
}
__BODY__

static void expect(int base, float racial, float global, int expected,
                   int gear = 0, bool haste = false, bool flurry = false)
{
    char_data ch = {};
    ch.player.race = RACE_HUMAN;
    ch.points.spell_pulse = gear;
    if (haste) SET_BIT(ch.specials.affected_by, AFF_HASTE);
    if (flurry) SET_BIT(ch.specials.affected_by2, AFF2_FLURRY);
    skills[SPELL_FULL_HEAL].beats = base;
    spell_pulse_data[RACE_HUMAN] = racial;
    multiplier = global;
    calls = 0;
    const auto before = ch.specials;
    const int result = SpellCastTime(&ch, SPELL_FULL_HEAL);
    if (result != expected)
        std::fprintf(stderr, "base=%d race=%g All=%g: got %d expected %d\n",
                     base, racial, global, result, expected);
    assert(result == expected);
    assert(calls == 1);
    assert(ch.points.spell_pulse == gear);
    assert(ch.specials.affected_by == before.affected_by);
    assert(ch.specials.affected_by2 == before.affected_by2);
}

int main()
{
    // Identity, speeding and slowing: an additive implementation fails these.
    expect(18, 1.0f, 1.0f, 18);
    expect(18, 1.0f, 0.5f, 9);
    expect(18, 1.0f, 2.0f, 36);
    missing = true;
    expect(18, 1.0f, 2.0f, 18);
    missing = false;
    // Preserve each existing truncation stage, including the racial stage.
    expect(18, 0.825f, 1.0f, 14);
    expect(18, 0.9f, 1.0f, 16);
    expect(18, 0.825f, 2.0f, 28);
    expect(18, 0.9f, 0.5f, 8);
    expect(18, 1.0f, 1.0f, 45, 50);
    expect(18, 1.0f, 1.0f, 14, 0, true);
    expect(18, 1.0f, 1.0f, 5, 0, false, true);
    expect(18, 1.0f, 1.0f, 5, 0, true, true);
    // Minimum survives sub-beat racial/global factors and zero base duration.
    expect(18, 0.025f, 1.0f, 1);
    expect(1, 1.0f, 0.5f, 1);
    expect(1, 1.0f, 1.0f, 1);
    expect(0, 1.0f, 2.0f, 1);
    expect(18, 1.0f, 0.0f, 1);
    std::puts("[PASS] production SpellCastTime: missing/identity/fractional/doubled multiplier, racial truncation, gear, haste/flurry, one-beat floor");
}
'''.replace("__BODY__", body)
with tempfile.TemporaryDirectory(prefix="duris-issue-252-") as temporary:
    path = Path(temporary)
    source = path / "cast_time.cpp"
    binary = path / "cast_time"
    source.write_text(harness)
    subprocess.run(["g++", "-std=c++20", "-Wall", "-Wextra", "-Werror",
                    "-D__NO_MYSQL__", "-Isrc", str(source), "-o", str(binary)],
                   cwd=ROOT, check=True)
    subprocess.run([str(binary)], cwd=ROOT, check=True)
