#!/usr/bin/env python3
"""convertObj() prices an object without overflowing an int.

A container costs 25 a pound held, and the cargo hold of the Dark Sun (#40218) holds
124,515,151: the product overflowed at every boot and the hold cost nothing. The real
function runs here under UBSan; the cost comes out at the function's own bound.
"""
from pathlib import Path
import subprocess
import tempfile

from _paths import ROOT, extract_function

HARNESS = r'''
#include "core/structs.h"
#include "core/utils.h"
#include "item/objmisc.h"
#include <cassert>
#include <cstdio>
#include <cstring>
bool isname(const char *, const char *) { return false; }
char *str_dup(const char *text) { return strdup(text); }
int real_object(int) { return -1; }
int GetCircle(int) { return 2; }
int number(int, int) { return 0; }
#define BOUNDED(low, value, high) ((value) < (low) ? (low) : (value) > (high) ? (high) : (value))
''' + extract_function("objconv.c", "void convertObj(P_obj obj)") + r'''
int main()
{
    obj_data hold = {};
    char name[] = "cargo hold";
    hold.name = name;
    hold.condition = 100;
    hold.type = ITEM_CONTAINER;
    hold.weight = 1;
    hold.value[0] = 124515151;
    convertObj(&hold);
    assert(hold.cost == 1000000);

    obj_data chest = {};
    chest.name = name;
    chest.condition = 100;
    chest.type = ITEM_STORAGE;
    chest.value[0] = 1000000;
    convertObj(&chest);
    assert(chest.cost == 1000000);

    obj_data sack = {};
    sack.name = name;
    sack.condition = 100;
    sack.type = ITEM_CONTAINER;
    sack.weight = 1;
    sack.value[0] = 40;
    convertObj(&sack);
    assert(sack.cost == 1000);
    std::puts("convertObj() prices the widest container inside its bound");
}
'''

(ROOT / "bin/tests").mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix="object-cost-", dir=ROOT / "bin/tests") as tmp:
    source, binary = Path(tmp) / "test.cpp", Path(tmp) / "test"
    source.write_text(HARNESS)
    subprocess.run(["g++", "-std=c++20", "-g", "-fsanitize=undefined",
                    "-fno-sanitize-recover=undefined", "-I", str(ROOT / "src"), str(source),
                    "-o", str(binary)], cwd=ROOT, check=True)
    subprocess.run([str(binary)], check=True)
