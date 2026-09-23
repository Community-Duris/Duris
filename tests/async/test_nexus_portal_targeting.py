#!/usr/bin/env python3
"""Every way of naming the obj 7371 nexus portal reaches its map-shift proc.

`enter 1.portal` and a god's `enter 7371` used to skip the nexus proc and fall
through to the generic ITEM_TELEPORT handler, which sent the player to the
portal's fixed value[0] room (7497) instead of a random surface-map room.
Compiles the real proc with the real keyword parsing and object targeting.
"""
from pathlib import Path
import os
import subprocess
import tempfile

from _paths import ROOT, SRC, extract_function

production = "\n".join([
    extract_function("utility.c", "int strn_cmp("),
    extract_function("interp.c", "const char *fill_words[]") + ";",
    extract_function("interp.c", "int search_block("),
    extract_function("interp.c", "int fill_word("),
    extract_function("interp.c", "char *one_argument("),
    extract_function("handler.c", "bool isname(const char *str,"),
    extract_function("handler.c", "int get_number("),
    extract_function("handler.c", "P_obj get_obj_in_list_vis("),
    extract_function("handler.c", "int generic_find("),
    extract_function("specs.underworld.c", "int nexus("),
])
fixture = (ROOT / "tests/async/nexus_portal_targeting_fixture.cpp").read_text()
assert fixture.count("// PRODUCTION_FUNCTIONS") == 1
build = ROOT / "bin/tests"
build.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(dir=build) as td:
    source = Path(td) / "nexus_portal_targeting.cpp"
    binary = Path(td) / "nexus_portal_targeting"
    source.write_text(fixture.replace("// PRODUCTION_FUNCTIONS", production))
    subprocess.run([
        "g++", "-std=c++20", "-g", "-Wall", "-Wextra", "-Werror",
        "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
        "-I" + str(SRC), str(source), "-o", str(binary),
    ], check=True)
    subprocess.run([str(binary)], check=True, env={
        **os.environ, "ASAN_OPTIONS": "detect_leaks=1:halt_on_error=1",
    })
print("Nexus portal targeting regression passed (ASan/UBSan).")
