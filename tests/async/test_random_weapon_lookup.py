#!/usr/bin/env python3
"""Exercise the production random-weapon table and identification lookup under UBSan."""
from pathlib import Path
import subprocess
import tempfile
from _paths import ROOT, extract_function, source

text = source("randomeq.c").read_text()
start = text.index("struct random_spells spells_data[")
end = text.index("\n};", start) + 3
harness = r'''
#include "core/prototypes.h"
#include "core/utils.h"
#include "magic/spells.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
void __free(void *p, const char *, int) { free(p); }
char *str_dup(const char *s) { return strdup(s); }
''' + text[start:end] + "\n" + extract_function("randomeq.c", "bool identify_random(") + r'''
int main() {
    obj_data object{};
    object.short_description = const_cast<char *>("a test weapon");
    for (int missing : {-1, 1000000, 0}) {
        object.value[5] = missing;
        assert(!identify_random(&object));
        assert(!strcmp(object.short_description, "a test weapon"));
    }
    object.value[5] = spells_data[0].spell;
    assert(identify_random(&object));
    assert(std::string(object.short_description) ==
           std::string("a test weapon of ") + spells_data[0].name + "&n");
    assert(!identify_random(&object)); // Already named: no repeated allocation.
    free(object.short_description);
    object.short_description = const_cast<char *>("a test weapon");
    object.str_mask = 0;
    for (const auto &spell : spells_data) {
        object.value[5] = spell.spell;
        const bool changed = identify_random(&object);
        if (changed) {
            assert(strstr(object.short_description, " of "));
            free(object.short_description);
        } else assert(!strcmp(object.short_description, "a test weapon"));
        object.short_description = const_cast<char *>("a test weapon");
        object.str_mask = 0;
    }
    puts("random weapon lookup bounds and every known spell passed");
}
'''
with tempfile.TemporaryDirectory(prefix="random-weapon-") as tmp:
    test, binary = Path(tmp) / "test.cpp", Path(tmp) / "test"
    test.write_text(harness)
    subprocess.run(["g++", "-std=c++20", "-g", "-fsanitize=address,undefined",
                    "-fno-sanitize-recover=all", "-fno-pie", "-no-pie", "-Isrc",
                    str(test), str(source("safe_format.c")), "-o", str(binary)],
                   cwd=ROOT, check=True)
    subprocess.run([str(binary)], check=True)
