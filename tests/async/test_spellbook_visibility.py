#!/usr/bin/env python3
"""Exercise owned spellbook visibility using production lookup and bitmap functions."""
from pathlib import Path
import subprocess
import tempfile

from _paths import ROOT, SRC, extract_function

production = "\n".join(extract_function("memorize.c", signature) for signature in (
    "int SpellInThisSpellBook_p(",
    "int SpellInThisSpellBook(",
    "struct extra_descr_data *find_spell_description(",
    "P_obj Find_process_entry(",
    "static P_obj scribing_implement(",
    "P_obj SpellBookAtHand(",
    "P_obj FindSpellBookWithSpell(",
    "P_obj SpellInSpellBook(",
))
fixture = (ROOT / "tests/async/spellbook_visibility_fixture.cpp").read_text()
build = ROOT / "bin/tests"
build.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(dir=build) as td:
    source = Path(td) / "spellbook.cpp"
    binary = Path(td) / "spellbook"
    source.write_text(fixture.replace("// PRODUCTION_FUNCTIONS", production))
    subprocess.run([
        "g++", "-std=c++20", "-g", "-fsanitize=address,undefined",
        "-fno-omit-frame-pointer", "-fno-pie", "-no-pie",
        "-I" + str(SRC), str(source), "-o", str(binary),
    ], check=True)
    subprocess.run([str(binary)], check=True)
print("Owned spellbook visibility regression passed (ASan/UBSan).")
