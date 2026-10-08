#!/usr/bin/env python3
"""Execute native conversion/rating for the Arcaneum's eight dream fragments.

File cost is not runtime cost. Extract the real convertObj and RateObject
functions with repository headers; deterministic randomness and unrelated
lookup/string endpoints are fixtures. No server, database or area is changed.
"""
from pathlib import Path
import re
import subprocess
import tempfile
from _paths import ROOT, extract_function

records = {
    int(match[1]): match[2]
    for match in re.finditer(
        r"^#(\d+)\s*\n([\s\S]*?)(?=^#\d+\s*$|\Z)",
        (ROOT / "areas/obj/library.obj").read_text(encoding="utf-8"),
        re.M,
    )
}
rows = []
for vnum in range(402041, 402049):
    values = list(map(int, records[vnum].split("~", 4)[4].split()))
    assert len(values) == 22 and values[0] == 8 and values[19:22] == [0, 0, 100]
    rows.append("{" + ",".join(map(str, values)) + "}")

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
static int variation = 0;
int number(int low, int high) {
    if (low == -2 && high == 2) return variation;
    return low + (high - low) / 2;
}
int real_object(int) { return -1; }
int IS_MAGE(P_char) { return 0; }
int IS_CLERIC(P_char) { return 0; }
int IS_WARRIOR(P_char) { return 0; }
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
    const int rows[][22] = {ROWS};
    char_data creature{};
    char name[] = "fragment dream ethereal";
    for (const auto &row : rows) {
        for (int percent : {-2, 0, 2}) {
            obj_data object{};
            object.type = row[0]; object.material = row[1];
            object.craftsmanship = row[4]; object.extra_flags = row[6];
            object.wear_flags = row[7]; object.extra2_flags = row[8];
            object.anti_flags = row[9]; object.anti2_flags = row[10];
            for (int i = 0; i < 8; ++i) object.value[i] = row[11 + i];
            object.weight = row[19]; object.cost = row[20];
            object.condition = row[21]; object.name = name;
            assert(object.cost == 0 && CAN_WEAR(&object, ITEM_TAKE));
            assert(IS_SET(object.extra_flags, ITEM_GLOW));
            assert(!IS_SET(object.extra_flags, ITEM_IGNORE | ITEM_NODROP | ITEM_TRANSIENT |
                                               ITEM_SECRET | ITEM_NOSHOW | ITEM_BURIED));
            variation = percent;
            convertObj(&object);
            assert(object.cost == 100 + percent && object.weight == 0);
            assert(RateObject(&creature, 0, &object) == 0);
            for (int roll : {75, 100, 125}) assert(object.cost * 100 / roll > 1);
        }
    }
    puts("Arcaneum: native conversion gives all eight fragments positive scavenger value; source and custody episodes remain separate.");
}
'''.replace("ROWS", ",".join(rows))
with tempfile.TemporaryDirectory(prefix="arcaneum-conversion-") as temporary:
    folder = Path(temporary)
    source = folder / "fragment_conversion.cpp"
    binary = folder / "fragment_conversion"
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
