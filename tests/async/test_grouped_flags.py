#!/usr/bin/env python3
"""Grouping must not mutate act flags, including through AoE/bard/map selection."""
import subprocess
import tempfile
from pathlib import Path

from _paths import ROOT, SRC, extract_function


def condition(text, anchor):
    """Extract the production if condition containing a distinctive expression."""
    for start in range(len(text)):
        if not text.startswith("if (", start):
            continue
        opening = start + 3
        depth = 0
        for end in range(opening, len(text)):
            if text[end] == "(":
                depth += 1
            elif text[end] == ")":
                depth -= 1
                if depth == 0:
                    expression = text[opening + 1:end]
                    if anchor in expression:
                        return expression
                    break
    raise AssertionError(anchor)


bard = extract_function("bard.c", "void event_bardsong(P_char ch,")
echo = extract_function("bard.c", "void event_echosong(P_char ch,")
riff = extract_function("bard.c", "void do_riff(P_char ch,")
map_source = (SRC / "map.c").read_text()
# Execute actual selection predicates; the rest of each event (scheduling,
# saving throws, spell effects) is deliberately outside this isolated harness.
selectors = {
    "bard_aggressive": (bard, "!should_area_hit(ch, tch)", True),
    "bard_allied": (bard, "!grouped(ch, tch)", True),
    "echo_aggressive": (echo, "!should_area_hit(ch, victim)", True),
    "echo_allied": (echo, "!grouped(ch, victim)", True),
    "riff_selected": (riff, "grouped(ch, tch)", False),
    "map_group": (map_source, "distance < 8 && grouped(ch, who)", False),
}
wrappers = []
for name, (text, anchor, reject) in selectors.items():
    expression = condition(text, anchor)
    wrappers.append(f"bool {name}(P_char ch, P_char target, int flags) {{\n"
                    "[[maybe_unused]] P_char tch = target, victim = target, who = target;\n"
                    "struct Song { int flags; } song{flags};\n"
                    "[[maybe_unused]] auto *sd = &song, *songDescrip = &song;\n"
                    "[[maybe_unused]] int aggr_chance = 100, distance = 1;\n"
                    f"return {'!' if reject else ''}({expression});\n}}\n")

harness = r'''
#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utils.h"
#include "magic/spells.h"
#include <cassert>
#include <cstdio>

room_data rooms[1]{};
P_room world = rooms;
int number(int low, int) { return low; }
int IS_MORPH(P_char) { return false; }
bool AdjacentInRoom(P_char, P_char) { return true; }
affected_type *get_spell_from_char(P_char, int, void *, int) { return nullptr; }
void affect_remove(P_char, affected_type *) { assert(false); }
P_char get_linking_char(P_char, ush_int) { return nullptr; }
'''
# Use production flag definitions, including the bard-local song flags.
for line in (SRC / "bard.c").read_text().splitlines():
    if line.startswith(("#define SONG_AGGRESSIVE ", "#define SONG_ALLIES ")):
        harness += line + "\n"
harness += extract_function("utility.c", "bool grouped(P_char ch,") + "\n"
harness += extract_function("utility.c", "bool should_area_hit(P_char ch,") + "\n"
harness += "\n".join(wrappers)
harness += r'''
int main()
{
    static_assert(ACT_GUILD_GOLEM == PLR_AFK);
    const unsigned long flags[] = {0, PLR_AFK, ACT_ISNPC, ACT_ISNPC | ACT_GUILD_GOLEM};
    group_list first{}, second{};
    unsigned cases = 0;
    for (unsigned a = 0; a < 4; ++a)
    for (unsigned b = 0; b < 4; ++b)
    for (int different_race = 0; different_race < 2; ++different_race)
    for (int groups = 0; groups < 4; ++groups)
    {
        char_data ch{}, target{};
        ch.specials.act = flags[a] | BIT_30;
        target.specials.act = flags[b] | BIT_31;
        ch.player.racewar = 0;
        target.player.racewar = different_race;
        ch.specials.position = target.specials.position = POS_STANDING | STAT_NORMAL;
        ch.group = groups ? &first : nullptr;
        target.group = groups == 1 ? &first : groups == 2 ? &second : nullptr;
        const auto before = ch.specials.act, target_before = target.specials.act;
        const bool expected = groups == 1 || (!different_race && (a == 3 || b == 3));
        assert(grouped(&ch, &target) == expected);
        assert(grouped(&target, &ch) == expected);
        const bool area = !expected && !(a >= 2 && b >= 2);
        assert(should_area_hit(&ch, &target) == area);
        // Later verses/echoes and repeated map checks must also remain read-only.
        for (int verse = 0; verse < 3; ++verse)
        {
            assert(bard_aggressive(&ch, &target, SONG_AGGRESSIVE) == area);
            assert(echo_aggressive(&ch, &target, SONG_AGGRESSIVE) == area);
            assert(bard_allied(&ch, &target, SONG_ALLIES) == expected);
            assert(echo_allied(&ch, &target, SONG_ALLIES) == expected);
            assert(riff_selected(&ch, &target, SONG_ALLIES) == expected);
            assert(riff_selected(&ch, &target, SONG_AGGRESSIVE) == !expected);
            assert(map_group(&ch, &target, 0) == expected);
            assert(ch.specials.act == before);
            assert(target.specials.act == target_before);
        }
        ++cases;
    }
    puts("grouped/AoE/bard/echo/riff/map flags: 128 cases passed");
    assert(cases == 128);
}
'''
build = ROOT / "bin/tests"
build.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix="grouped-flags-", dir=build) as directory:
    source_path = Path(directory) / "grouped.cpp"
    binary = Path(directory) / "grouped"
    source_path.write_text(harness)
    subprocess.run(["g++", "-std=c++20", "-Wall", "-Wextra", "-Werror",
                    f"-I{SRC}", str(source_path), "-o", str(binary)],
                   check=True, cwd=ROOT, timeout=60)
    subprocess.run([str(binary)], check=True, cwd=ROOT, timeout=10)
