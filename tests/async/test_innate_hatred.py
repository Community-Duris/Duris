#!/usr/bin/env python3
"""Execute Hatred's production matching, effects and scheduler under ASan/UBSan."""
import os
import re
import subprocess
import sys
import tempfile
from pathlib import Path

from _paths import ROOT, HARNESS_STUBS, extract_function, source

recover = extract_function("copyover.c", "int copyover_recover(")
assert recover.index("char_to_room(ch, save_room,") < recover.index("start_hatred_check(ch);")
properties = (ROOT / "lib/duris.properties").read_text()
assert re.search(r"^innate\.timer\.hatred=4\.000$", properties, re.M)
assert not re.search(r"^innate\.timer\.secs\.hatred=", properties, re.M)

innates = source("innates.c").read_text()
macros = innates[innates.index("#define ADD_RACIAL_INNATE"):innates.index("float regen_factor")]
arrays = innates[innates.index("char racial_innates["):innates.index("extern const struct race_names")]
table = re.search(r"const int race_hatred_data\[\]\[MAX_HATRED\] = .*?\n};",
                  source("constant.c").read_text(), re.S).group()
functions = [
    ("utility.c", "int flag2idx("),
    ("affects.c", "struct affected_type *get_spell_from_char("),
    ("affects.c", "bool affected_by_spell("),
    ("innates.c", "static int innate_char_race("),
    ("innates.c", "static int innate_unlock_level("),
    ("innates.c", "bool has_innate("),
    ("innates.c", "void assign_innates("),
    ("utility.c", "bool ac_can_see("),
    ("affects.c", "void affect_modify("),
    ("affects.c", "struct affected_type *affect_to_char("),
    ("affects.c", "void affect_remove("),
    ("affects.c", "void affect_from_char("),
    ("affects.c", "void wear_off_message("),
    ("affects.c", "void affect_update("),
    ("files.c", "int writeAffects("),
    ("files.c", "ush_int getShort("),
    ("nanny.c", "void schedule_pc_events("),
    ("innates.c", "static void clear_hatred("),
    ("innates.c", "bool start_hatred_check("),
    ("innates.c", "static P_char visible_hatred_enemy("),
    ("innates.c", "void event_hatred_check("),
]
production = macros + arrays + table + "\n" + "\n".join(
    extract_function(file, signature) for file, signature in functions)
build = Path(os.environ.get("BIN_ROOT", ROOT / "bin")) / "tests"
build.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix="hatred-", dir=build) as directory:
    directory = Path(directory)
    (directory / "production.inc").write_text(production)
    binary = directory / "hatred"
    subprocess.run([
        os.environ.get("CXX", "g++"), "-std=c++20", "-g", "-O1", "-pthread",
        "-ffunction-sections", "-fdata-sections", "-fsanitize=address,undefined",
        "-fno-sanitize-recover=all", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie",
        "-I" + str(ROOT / "src"), "-I" + str(directory),
        str(ROOT / "tests/async/innate_hatred_harness.cpp"), str(HARNESS_STUBS),
        str(source("persistence/latency_trace.c")), "-Wl,--gc-sections", "-lbsd",
        "-o", str(binary),
    ], check=True)
    for mode in sys.argv[1:] or ["bounds", "cleanup", "partial", "matching", "visibility",
                               "eligibility", "normalization", "scheduler", "timing",
                               "serialization"]:
        subprocess.run([str(binary), mode], check=True)
