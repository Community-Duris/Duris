#!/usr/bin/env python3
"""Qualify the large club's runtime scavenger selection inputs.

The giant's reset does not equip the club, but native scavenging may claim and
equip its floor stock. Execute production conversion/rating with repository
headers, fixture class/visibility/lookup endpoints and bounded random rolls.
This proves selection inputs, not actual pickup, equipment or accounting supply.
"""
from pathlib import Path
import re
import subprocess
import tempfile
from _paths import ROOT, extract_function

record = re.search(
    r"^#95001\s*\n([\s\S]*?)(?=^#\d+\s*$|\Z)",
    (ROOT / "areas/obj/mistywood.obj").read_text(encoding="utf-8"),
    re.M,
)[1]
lines = record.split("~", 4)[4].strip().splitlines()
values = list(map(int, " ".join(lines[:3]).split()))
assert len(values) == 22 and values[0] == 5 and values[19:22] == [21, 5, 100]
affects = [tuple(map(int, m.groups())) for m in re.finditer(r"^A\s*\n(-?\d+)\s+(-?\d+)", record, re.M)]
assert affects == [(2, -4), (19, 2)]
mob = re.search(
    r"^#95002\s*\n([\s\S]*?)(?=^#\d+\s*$|\Z)",
    (ROOT / "areas/mob/mistywood.mob").read_text(encoding="utf-8"),
    re.M,
)[1]
flags = int(mob.split("~", 4)[4].split()[0])
assert flags & 2 and flags & 4
prelude = r'''
#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utils.h"
#include "item/objmisc.h"
#include "magic/spells.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <string>
static index_data indexes[1]{};
P_index obj_index = indexes;
static int variation = 0, rating_roll = 100, warrior = 0;
int number(int low, int high) {
    if (low == -2 && high == 2) return variation;
    if (low == 90 && high == 110) return rating_roll;
    return low + (high - low) / 2;
}
int real_object(int) { return -1; }
int IS_MAGE(P_char) { return 0; }
int IS_CLERIC(P_char) { return 0; }
int IS_WARRIOR(P_char) { return warrior; }
int IS_THIEF(P_char) { return 0; }
bool ac_can_see(P_char, P_char, bool) { return true; }
char *str_dup(const char *text) { return strdup(text); }
bool isname(const char *keyword, const char *names) {
    if (!keyword || !names) return false;
    std::istringstream words(names); std::string word;
    while (words >> word) if (word == keyword) return true;
    return false;
}
'''
main = r'''
int main() {
    const int row[] = {ROW};
    char_data giant{};
    giant.player.level = 35;
    char name[] = "large club";
    for (int percent : {-2, 0, 2}) {
        obj_data club{};
        club.type = row[0]; club.material = row[1];
        club.craftsmanship = row[4]; club.extra_flags = row[6];
        club.wear_flags = row[7]; club.extra2_flags = row[8];
        club.anti_flags = row[9]; club.anti2_flags = row[10];
        for (int i = 0; i < 8; ++i) club.value[i] = row[11 + i];
        club.weight = row[19]; club.cost = row[20];
        club.condition = row[21]; club.name = name;
        club.affected[0].location = APPLY_DEX; club.affected[0].modifier = -4;
        club.affected[1].location = APPLY_DAMROLL; club.affected[1].modifier = 2;
        assert(CAN_WEAR(&club, ITEM_TAKE) && CAN_WEAR(&club, ITEM_WIELD));
        assert(!IS_SET(club.extra_flags, ITEM_SECRET | ITEM_NOSHOW | ITEM_BURIED));
        variation = percent; convertObj(&club);
        assert(IS_SET(club.extra_flags, ITEM_FLOAT | ITEM_TWOHANDS));
        assert(club.cost == 5200 + 52 * percent && club.weight == 21);
        for (int roll : {75, 100, 125}) assert(club.cost * 100 / roll > 1);
        for (int role : {0, 1}) {
            warrior = role;
            for (int roll : {90, 100, 110}) {
                rating_roll = roll;
                assert(RateObject(&giant, 0, &club) > 0);
            }
        }
    }
    puts("Mistywood: native club conversion/rating permits scavenger selection; pickup, equipment and source episodes remain separate.");
}
'''.replace("ROW", ",".join(map(str, values)))
with tempfile.TemporaryDirectory(prefix="mistywood-conversion-") as temporary:
    folder = Path(temporary)
    source = folder / "club_conversion.cpp"
    binary = folder / "club_conversion"
    source.write_text(
        prelude
        + extract_function("utility.c", "int BOUNDED(int a, int b, int c)")
        + "\n"
        + extract_function("objconv.c", "void convertObj(P_obj obj)")
        + "\n"
        + extract_function("mobact.c", "int RateObject(P_char ch, int")
        + main,
        encoding="utf-8",
    )
    subprocess.run(
        ["g++", "-std=c++20", "-O1", "-I", str(ROOT / "src"), str(source), "-o", str(binary)],
        check=True,
    )
    subprocess.run([str(binary)], check=True)
