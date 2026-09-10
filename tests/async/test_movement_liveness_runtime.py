#!/usr/bin/env python3
"""Run the production post-movement tail under controlled lifetime outcomes.

Movement/scripts and extraction are fixtures, not a full server simulation.
The tail, membership predicate, character layout, and death/flag macros are
production code. Hook placement is checked by test_kingdom_contract.py.
"""

import subprocess
import tempfile
from pathlib import Path

from _paths import ROOT, extract_function


move = extract_function("actmove.c", "void do_move(")
tail = move[move.index("\tconst auto removal_before ="):]
predicate = extract_function("utility.c", "int char_in_list(const P_char ch)")
# Instrument only calls and visited nodes; retain the actual lookup algorithm.
predicate = predicate.replace("\n{", "\n{\n    ++scans;", 1).replace(
    "if (tmp == ch)", "++visited;\n        if (tmp == ch)"
)
old_tail = tail.replace(
    "const auto removal_before = character_removal_generation;", ""
).replace(
    "(removal_before != character_removal_generation && !char_in_list(ch))",
    "!char_in_list(ch)",
)
assert old_tail != tail and "removal_before" not in old_tail

PRELUDE = r'''
#include "core/prototypes.h"
#include "core/utils.h"
#include "magic/spells.h"
#include <cassert>
#include <cstdio>
#include <limits>
#include <new>
#include <vector>

P_char character_list = nullptr;
uint64_t character_removal_generation = 0;
static size_t scans, visited;
static int ice;
static P_char other;
static bool legacy;
enum Scenario { ordinary, blocked, extracted, freed, unrelated, nested,
                killed, reused, extraction_returned, direct_free, blocked_unrelated };
static Scenario scenario;

void movement_tail(P_char ch, int cmd);
void legacy_tail(P_char ch, int cmd);
bool affected_by_spell(P_char, int) { return true; }
void make_ice(P_char) { ++ice; }

static void unlink_fixture(P_char ch)
{
    P_char *link = &character_list;
    while (*link && *link != ch)
        link = &(*link)->next;
    assert(*link == ch);
    *link = ch->next;
}

int do_simple_move(P_char ch, int, unsigned int)
{
    switch (scenario)
    {
    case blocked:
        return false;
    case extracted:
        ++character_removal_generation;
        unlink_fixture(ch);
        return false;
    case freed:
        ++character_removal_generation; // extract_char entry
        unlink_fixture(ch);
        ++character_removal_generation; // free_char entry
        delete ch; // stronger than the server's deferred pooled release
        return false;
    case direct_free:
        unlink_fixture(ch); // already unlinked by the fixture's caller
        ++character_removal_generation; // free_char without extract_char
        delete ch;
        return false;
    case unrelated:
    case blocked_unrelated:
        ++character_removal_generation;
        unlink_fixture(other);
        return scenario != blocked_unrelated;
    case nested:
        scenario = extracted;
        if (legacy)
            legacy_tail(other, 0);
        else
            movement_tail(other, 0);
        scenario = nested;
        return true;
    case killed:
        SET_POS(ch, STAT_DEAD);
        return true;
    case reused:
    {
        ++character_removal_generation;
        P_char next = ch->next;
        const auto act = ch->specials.act;
        ch->~char_data();
        new (ch) char_data{}; // deterministic same-address replacement
        ch->next = next;
        ch->specials.act = act;
        SET_POS(ch, STAT_NORMAL + POS_STANDING);
        SET_BIT(ch->specials.affected_by3, AFF3_TRACKING);
        return true;
    }
    case extraction_returned:
        ++character_removal_generation; // conservative invalidation, no unlink
        return true;
    case ordinary:
        return true;
    }
    std::abort();
}
'''

DRIVER = r'''
struct Result { int ice_count; bool tracking; size_t scans, visited; };

static Result run(Scenario selected, bool npc, bool old)
{
    constexpr size_t population = 4096;
    std::vector<char_data> world(population);
    auto *actor = new char_data{};
    auto *bystander = new char_data{};
    for (size_t i = 0; i + 1 < population; ++i)
        world[i].next = &world[i + 1];
    world.back().next = actor;
    actor->next = bystander;
    actor->specials.act = npc ? ACT_ISNPC : 0;
    bystander->specials.act = ACT_ISNPC;
    SET_POS(actor, STAT_NORMAL + POS_STANDING);
    SET_POS(bystander, STAT_NORMAL + POS_STANDING);
    SET_BIT(actor->specials.affected_by3, AFF3_TRACKING);
    character_list = world.data();
    other = bystander;
    scans = visited = ice = 0;
    // Also exercise unsigned wrap for the single-invalidation scenarios.
    character_removal_generation = std::numeric_limits<uint64_t>::max();
    scenario = selected;
    legacy = old;
    if (old)
        legacy_tail(actor, 0);
    else
        movement_tail(actor, 0);
    bool tracking = false;
    if (selected != freed && selected != direct_free)
    {
        tracking = IS_SET(actor->specials.affected_by3, AFF3_TRACKING);
        delete actor;
    }
    Result result{ice, tracking, scans, visited};
    delete bystander;
    character_list = other = nullptr;
    return result;
}

int main()
{
    for (bool npc : {false, true})
    {
        for (Scenario selected : {ordinary, blocked, extracted, freed, unrelated,
                                  nested, killed, reused, extraction_returned,
                                  direct_free, blocked_unrelated})
        {
            const auto before = run(selected, npc, true);
            const auto after = run(selected, npc, false);
            assert(before.ice_count == after.ice_count);
            assert(before.tracking == after.tracking);
            const bool no_removal = selected == ordinary || selected == blocked ||
                                    selected == killed;
            assert(after.scans == (no_removal ? 0 : before.scans));
            assert(after.visited == (no_removal ? 0 : before.visited));
            if (no_removal)
                assert(before.scans == 1 && before.visited == 4097);
            else
                assert(after.scans == (selected == nested ? 2 : 1));
            const bool frost = selected == ordinary || selected == unrelated ||
                               selected == nested || selected == reused ||
                               selected == extraction_returned;
            assert(after.ice_count == (frost ? 1 : 0));
            if (selected != freed && selected != direct_free)
                assert(after.tracking == (selected != blocked &&
                                          selected != blocked_unrelated));
        }
    }
    std::puts("22 PC/NPC lifetime cases passed; ordinary/blocked movement: "
              "1 scan / 4097 visits -> 0 scans / 0 visits; fallback matches old guard");
}
'''

# Keep compiled artifacts under bin/, matching repository conventions.
build_root = ROOT / "bin" / "tests"
build_root.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix="movement-liveness-", dir=build_root) as directory:
    harness = Path(directory) / "harness.cpp"
    binary = Path(directory) / "harness"
    harness.write_text("\n".join((
        PRELUDE, predicate,
        "void movement_tail(P_char ch, int cmd) {\n" + tail,
        "void legacy_tail(P_char ch, int cmd) {\n" + old_tail,
        DRIVER,
    )), encoding="utf-8")
    subprocess.run([
        "g++", "-std=c++20", "-Wall", "-Wextra", "-Werror",
        "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie",
        f"-I{ROOT / 'src'}", str(harness), "-o", str(binary),
    ], cwd=ROOT, check=True)
    subprocess.run([str(binary)], cwd=ROOT, check=True)
