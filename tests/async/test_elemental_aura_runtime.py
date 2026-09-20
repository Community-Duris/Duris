#!/usr/bin/env python3
"""Exercise elemental-aura eligibility and its resource-preserving gate."""

import subprocess
from pathlib import Path

from _paths import ROOT, extract_function, source

PRELUDE = r'''
#include "core/prototypes.h"
#include "core/utils.h"
#include "magic/spells.h"
#include "net/comm.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <string>
#include <strings.h>

static room_data rooms[1]{};
P_room world = rooms;
static std::string output;

void send_to_char(const char *text, P_char) {
    output += text ? text : "";
}
void act(const char *, int, P_char, P_obj, void *, int) {}
bool NewSaves(P_char, int, int) { return false; }
bool affected_by_spell(P_char ch, int type) {
    for (auto *af = ch->affected; af; af = af->next)
        if (af->type == type) return true;
    return false;
}
static void rebuild_bits(P_char ch) {
    ch->specials.affected_by2 = 0;
    ch->specials.affected_by4 = 0;
    for (auto *af = ch->affected; af; af = af->next) {
        ch->specials.affected_by2 |= af->bitvector2;
        ch->specials.affected_by4 |= af->bitvector4;
    }
}
affected_type *affect_to_char(P_char ch, affected_type *af) {
    auto *copy = new affected_type(*af);
    copy->next = ch->affected;
    ch->affected = copy;
    rebuild_bits(ch);
    return copy;
}
static void clear_effects(P_char ch) {
    while (ch->affected) {
        auto *next = ch->affected->next;
        delete ch->affected;
        ch->affected = next;
    }
    rebuild_bits(ch);
}
static int effect_count(P_char ch) {
    int count = 0;
    for (auto *af = ch->affected; af; af = af->next) ++count;
    return count;
}
static void reset(char_data &ch, int sector) {
    clear_effects(&ch);
    ch = {};
    ch.in_room = 0;
    rooms[0].sector_type = sector;
    output.clear();
}
'''

DRIVER = r'''
static void expect_message(P_char ch, const char *expected) {
    const int effects_before = effect_count(ch);
    assert(elemental_aura_failure_message(ch) != nullptr);
    assert(std::strcmp(elemental_aura_failure_message(ch), expected) == 0);
    spell_elemental_aura(50, ch, nullptr, 0, ch, nullptr);
    assert(output == expected);
    assert(effect_count(ch) == effects_before);
}

int main() {
    char_data caster{};

    // The four supported plane sectors remain eligible and preserve the
    // existing four-stat effect behavior when no full-form save succeeds.
    for (int sector : {SECT_FIREPLANE, SECT_WATER_PLANE, SECT_AIR_PLANE,
                       SECT_EARTH_PLANE}) {
        reset(caster, sector);
        assert(elemental_aura_failure_message(&caster) == nullptr);
        spell_elemental_aura(50, &caster, nullptr, 0, &caster, nullptr);
        assert(effect_count(&caster) == 4);
        clear_effects(&caster);
    }

    const char *location_failure =
        "There is no elemental planar essence here to draw upon.\n";
    reset(caster, SECT_INSIDE);
    expect_message(&caster, location_failure);

    const char *conflict_failure = "An elemental aura already surrounds you.\n";
    reset(caster, SECT_FIREPLANE);
    affected_type existing{};
    existing.type = SPELL_ELEMENTAL_AURA;
    caster.affected = &existing;
    expect_message(&caster, conflict_failure);
    caster.affected = nullptr;

    const unsigned int affected2_flags[] = {
        AFF2_EARTH_AURA, AFF2_WATER_AURA, AFF2_FIRE_AURA, AFF2_AIR_AURA,
    };
    for (const auto flag : affected2_flags) {
        reset(caster, SECT_AIR_PLANE);
        caster.specials.affected_by2 = flag;
        expect_message(&caster, conflict_failure);
    }

    reset(caster, SECT_AIR_PLANE);
    caster.specials.affected_by4 = AFF4_ICE_AURA;
    expect_message(&caster, conflict_failure);

    reset(caster, SECT_FIREPLANE);
    caster.in_room = NOWHERE;
    expect_message(&caster, location_failure);

    std::puts("Elemental aura eligibility, messages, and valid-plane effects passed.");
}
'''

sparser = source("sparser.c").read_text(encoding="utf-8")
event = sparser[sparser.index("void event_spellcast(") :]
preflight = event.index("if (!weaving && arg->spell == SPELL_ELEMENTAL_AURA)")
resource = event.index("use_spell(ch, arg->spell)")
assert preflight < resource, "elemental-aura rejection must precede use_spell()"
assert "StopCasting(ch);" in event[preflight:resource]
magic = source("magic.c").read_text(encoding="utf-8")
assert "failure = elemental_aura_failure_message(ch);" in magic

functions = [
    extract_function("magic.c", "const char *elemental_aura_failure_message("),
    extract_function("magic.c", "void spell_elemental_aura("),
]
out_dir = ROOT / "bin/tests/elemental-aura"
out_dir.mkdir(parents=True, exist_ok=True)
harness = out_dir / "harness.cpp"
harness.write_text(PRELUDE + "\n".join(functions) + DRIVER, encoding="utf-8")
binary = out_dir / "harness"
subprocess.run([
    "g++", "-std=c++20", "-Wall", "-Wextra", "-Werror",
    "-fsanitize=address,undefined", "-I" + str(ROOT / "src"),
    str(harness), "-o", str(binary),
], check=True)
subprocess.run([str(binary)], check=True)
