#!/usr/bin/env python3
"""Source contract joining casting-pulse policy, properties, and do_cast."""

from __future__ import annotations

from pathlib import Path

from _paths import ROOT, extract_function


SPARSER = ROOT / "src/net/sparser.c"
EPIC = ROOT / "src/classes/epic_skills.c"
MAKEFILE = ROOT / "src/Makefile"
PROPERTIES = ROOT / "lib/duris.properties"
DOC = ROOT / "docs/reference/CASTING_PULSE.md"

source = SPARSER.read_text(encoding="utf-8", errors="replace")
makefile = MAKEFILE.read_text(encoding="utf-8", errors="replace")
properties = PROPERTIES.read_text(encoding="utf-8", errors="replace")
documentation = DOC.read_text(encoding="utf-8", errors="replace")

assert "net/casting_pulse_policy.o" in makefile
assert '#include "net/casting_pulse_policy.h"' in source

keys = (
    "spellcast.quickChant.durationMultiplier",
    "spellcast.quickChant.tankSuccessPercent",
    "spellcast.quickChant.skillBasePercent",
    "spellcast.quickChant.skillPercentPerPoint",
    "spellcast.maxCircleAbort.basePercent",
    "spellcast.maxCircleAbort.agilityReductionPerPoint",
    "spellcast.maxCircleAbort.capPercent",
)
config_loader = extract_function("sparser.c", "static casting_pulse_config current_casting_pulse_config()")
for key in keys:
    assert properties.count(f"{key}=") == 1, key
    assert f'get_property("{key}"' in config_loader, key
    assert f"`{key}`" in documentation, key

do_cast = extract_function("sparser.c", "void do_cast(P_char ch, char *argument, int cmd)")
timing_start = do_cast.index("const casting_pulse_config pulse_config")
timing_end = do_cast.index("tmp_spl.timeleft = dura;", timing_start)
timing = do_cast[timing_start:timing_end]
assert "casting_pulse_quick_chant_percent" in timing
assert "number(1, CASTING_PULSE_ROLL_SCALE)" in timing
assert "casting_pulse_timing_for" in timing
assert "dura = timing.landing_beats;" in timing
assert "CharWait(ch, timing.command_gate_beats);" in timing
assert timing.index("casting_pulse_timing_for") < timing.index("CharWait(")
assert timing.count("CharWait(") == 1
assert "Your quick chant falters" in timing

abort = do_cast[timing_end : do_cast.index("DelayCommune(ch, dura);", timing_end)]
assert "casting_pulse_max_circle_abort_percent" in abort
assert "max_circle_abort_percent > 0.0f" in abort
assert "number(1, CASTING_PULSE_ROLL_SCALE)" in abort
assert "MAX(1, number(0, 9) * dura / 10)" in abort
assert "GET_C_AGI(ch) / 2 + 50" not in do_cast
assert "GET_CHAR_SKILL(ch, SKILL_QUICK_CHANT) > number(1, 100)" not in do_cast

chant_mastery = extract_function("epic_skills.c", "int chant_mastery_bonus(P_char ch, int dura)")
assert "CharWait(" not in chant_mastery

print("Casting pulse integration contract passed.")
