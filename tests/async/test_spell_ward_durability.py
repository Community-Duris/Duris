#!/usr/bin/env python3
"""Execute production renewable spell wards and snapshot codecs (issue #453)."""
from pathlib import Path
import subprocess
import tempfile
from _paths import ROOT, extract_function

with tempfile.TemporaryDirectory(prefix="duris-wards-") as directory:
    native = Path(directory) / "wards"
    spirit = Path(directory) / "spirit.cpp"
    spirit.write_text('\n'.join([
        '#include "combat/spell_wards.h"', '#include "core/prototypes.h"',
        '#include "core/utils.h"', '#include "net/comm.h"', '#include "magic/spells.h"',
        '#include <strings.h>', 'void do_point(P_char, P_char);',
        extract_function("magic/smagic.c", "void spell_spirit_ward("),
        extract_function("magic/smagic.c", "void spell_greater_spirit_ward("),
    ]))
    subprocess.run([
        "g++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-Isrc",
        "tests/async/spell_ward_durability_harness.cpp", "src/combat/spell_wards.c",
        "src/magic/spell_globes.c", str(spirit), "src/player/player_snapshot_codec.c", "-o", str(native),
    ], cwd=ROOT, check=True)
    subprocess.run([str(native)], check=True)

fight = extract_function("combat/fight.c", "int spell_damage(")
ward = fight.index("spell_ward_absorb(ch, victim, dam, flags)")
assert fight.index("check_damage_ward") < ward
assert "dam = ward.remaining" in fight[ward:]
assert "IS_NPC(victim)" in fight[ward:]
