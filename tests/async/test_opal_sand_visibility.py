#!/usr/bin/env python3
"""Check Opal's delivered sand against production mortal visibility and selection."""
import argparse
import os
from pathlib import Path
import re
import subprocess
import tempfile

from _paths import ROOT, SRC, extract_function

parser = argparse.ArgumentParser()
parser.add_argument("--source-root", type=Path, default=ROOT)
args = parser.parse_args()
text = (args.source_root / "areas/obj/opalphoenix.obj").read_text(encoding="utf8")
body = re.search(r"^#70823\s*\n(.*?)(?=^#\d+|^\$|\Z)", text, re.M | re.S)[1]
values = list(map(int, re.match(r"\s*([\d\s-]+)", body.split("~")[4])[1].split()))
assert values[0] == 12 and values[6] & 16384 and values[7] & 1
production = "\n".join(extract_function(name, signature) for name, signature in (
    ("utility.c", "bool ac_can_see_obj("),
    ("handler.c", "bool isname(const char *str, const char *namelist)"),
    ("handler.c", "int get_number("),
    ("handler.c", "P_obj get_obj_in_list_vis("),
))
fixture = (ROOT / "tests/async/opal_sand_visibility_fixture.cpp").read_text(encoding="utf8")
build = ROOT / "bin/tests"
build.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(dir=build) as temporary:
    cpp = Path(temporary) / "opal.cpp"
    binary = Path(temporary) / "opal"
    cpp.write_text(fixture.replace("// PRODUCTION_FUNCTIONS", production), encoding="utf8")
    subprocess.run(["g++", "-std=c++20", "-g", "-fsanitize=address,undefined",
                    "-fno-omit-frame-pointer", "-fno-pie", "-no-pie",
                    "-I" + str(SRC), str(cpp), "-o", str(binary)], check=True)
    subprocess.run([str(binary), str(values[6])], check=True,
                   env={**os.environ, "ASAN_OPTIONS": "detect_leaks=1:halt_on_error=1"})
print("Opal sand reward production visibility regression passed (ASan/UBSan).")
