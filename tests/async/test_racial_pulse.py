#!/usr/bin/env python3
"""Contracts for racial pulse: the 'pulse' command, the shipped rates, and pulse-free gear.

The command sets absolute values with exactly three decimals and routes through
'properties set' like the difficulty dials. Player gear no longer makes a pulse faster;
mob-only proc objects keep theirs.
"""

from __future__ import annotations

import re
import subprocess
import tempfile
from pathlib import Path

from _paths import ROOT, source


PROPERTIES = (ROOT / "lib" / "duris.properties").read_text()

CAST = {"Kobold": "0.895", "Gnome": "0.895", "Goblin": "0.920", "Halfling": "0.920",
        "DrowElf": "0.955", "GreyElf": "0.955", "MountainDwarf": "0.960", "DuergarDwarf": "0.960",
        "Githzerai": "0.975", "Githyanki": "0.975", "Tiefling": "0.975", "Drider": "0.985",
        "Human": "1.000", "Orc": "1.000", "Thri-Kreen": "1.000", "Centaur": "1.020",
        "Barbarian": "1.035", "Troll": "1.035", "Minotaur": "1.040", "Firbolg": "1.055",
        "Ogre": "1.055"}
MELEE = {"Kobold": "12.000", "Gnome": "12.000", "Goblin": "12.500", "Halfling": "13.000",
         "DrowElf": "14.000", "GreyElf": "14.000", "Drider": "14.000", "Tiefling": "14.250",
         "Human": "15.000", "Orc": "15.000", "Githzerai": "15.000", "Githyanki": "15.000",
         "Centaur": "15.250", "MountainDwarf": "16.000", "DuergarDwarf": "16.000",
         "Thri-Kreen": "16.000", "Barbarian": "16.250", "Troll": "16.250", "Minotaur": "16.250",
         "Firbolg": "16.250", "Ogre": "16.250"}


def _flat(text: str) -> str:
    return "".join(text.split())


def _body(name: str, signature: str) -> str:
    """A function definition: its signature to the closing brace in column 0."""
    text = source(name).read_text()
    at = text.find(signature)
    while at >= 0:
        brace, semicolon = text.find("{", at), text.find(";", at)
        if brace >= 0 and (semicolon < 0 or brace < semicolon):
            return text[at:text.index("\n}\n", brace) + 2]
        at = text.find(signature, at + 1)
    raise AssertionError(f"{name}: no definition for {signature}")


def test_values_take_exactly_three_decimals() -> None:
    harness = r'''
#include "world/racial_pulse_math.h"
int main() {
    double value = -1;
    const char *good[] = {"0.900", "12.000", "1.055", "40.000", "0.010", "5.000"};
    const double want[] = {0.900, 12.000, 1.055, 40.000, 0.010, 5.000};
    for (int i = 0; i < 6; ++i)
        if (!racial_pulse_parse_value(good[i], &value) || value != want[i]) return 1 + i;
    const char *bad[] = {"0.9", "0.90", "0.9000", ".900", "0.900x", "-0.900", "+0.900",
                         " 0.900", "0.900 ", "", "abc", "1e3", "12", "1,000", "1000.000", "0..900"};
    for (const char *text : bad)
        if (racial_pulse_parse_value(text, &value)) return 20;
    if (racial_pulse_parse_value(nullptr, &value)) return 21;
    // Ranges: casting is a multiplier, melee a round in beats.
    if (!racial_pulse_in_range(RACIAL_PULSE_CAST, 0.010) || !racial_pulse_in_range(RACIAL_PULSE_CAST, 5.000)) return 30;
    if (racial_pulse_in_range(RACIAL_PULSE_CAST, 0.009) || racial_pulse_in_range(RACIAL_PULSE_CAST, 5.001)) return 31;
    if (!racial_pulse_in_range(RACIAL_PULSE_MELEE, 1.000) || !racial_pulse_in_range(RACIAL_PULSE_MELEE, 40.000)) return 32;
    if (racial_pulse_in_range(RACIAL_PULSE_MELEE, 0.999) || racial_pulse_in_range(RACIAL_PULSE_MELEE, 40.001)) return 33;
    return 0;
}
'''
    with tempfile.TemporaryDirectory(prefix="duris-racial-pulse-") as directory:
        root = Path(directory)
        source_path, binary = root / "harness.cpp", root / "harness"
        source_path.write_text(harness)
        subprocess.run(["g++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-Isrc",
                        str(source_path), "-o", str(binary)], cwd=ROOT, check=True)
        subprocess.run([str(binary)], cwd=ROOT, check=True)


def test_command_is_registered() -> None:
    assert "#define CMD_PULSE 861" in source("cmd/interp.h").read_text()
    interp = source("cmd/interp.c").read_text()
    assert '\t"pulse",\n\t"collector",\n\t"restitution",\n\t"dummy",\n\t"\\n" /* MAX_CMD = 865' in interp
    assert "CMD_GRT(CMD_PULSE, STAT_DEAD + POS_PRONE, do_pulse, LESSER_G);" in interp
    assert re.search(r"#define MAX_CMD 865\b", source("core/config.h").read_text())
    assert "void do_pulse(P_char, char *, int);" in source("core/prototypes.h").read_text()
    assert "world/racial_pulse.o" in (ROOT / "src" / "Makefile").read_text()
    attributes = (ROOT / "docs/lib/information/command_attributes.txt").read_text()
    assert "\npulse\n~\n" in attributes


def test_changes_are_forger_only_and_go_through_properties() -> None:
    command = _flat(_body("world/racial_pulse.c", "void do_pulse(P_char ch, char *argument, int"))
    forger = command.index(_flat("if (GET_LEVEL(ch) < FORGER)"))
    request = command.index(_flat('std::string request = "set " + key + " " + value_word;'))
    assert forger < request < command.index(_flat("do_properties(ch, request.data(), 0);"))
    # The typed value is validated (three decimals, in range) before it reaches 'set'.
    assert command.index(_flat("racial_pulse_parse_value(value_word, &value)")) < request
    assert command.index(_flat("racial_pulse_in_range(table, value)")) < request
    # Melee is fixed into each character at affect_total, so everyone of the race is re-totalled.
    assert _flat("if (GET_RACE(tch) == race) balance_affects(tch);") in command
    math = source("world/racial_pulse_math.h").read_text()
    assert '"spellcast.pulse.racial."' in math and '"damage.pulse.racial."' in math


def test_rates_ship_the_revised_values() -> None:
    lines = set(PROPERTIES.splitlines())
    for race, value in CAST.items():
        assert f"spellcast.pulse.racial.{race}={value}" in lines, race
    for race, value in MELEE.items():
        assert f"damage.pulse.racial.{race}={value}" in lines, race


def _objects():
    """Yield (file, vnum, wear, extra, applies) for every object record."""
    for path in sorted((ROOT / "areas" / "obj").glob("*.obj")):
        lines = path.read_bytes().decode("latin-1").splitlines()
        starts = [i for i, line in enumerate(lines) if re.match(r"^#\d+\s*$", line)]
        for n, start in enumerate(starts):
            end = starts[n + 1] if n + 1 < len(starts) else len(lines)
            strings, i = 0, start + 1
            while i < end and strings < 4:
                if lines[i].rstrip().endswith("~"):
                    strings += 1
                i += 1
            head = lines[i].split() if i < end else []
            if len(head) < 8:
                continue
            found, j = [], i + 3
            while j < end:
                if lines[j].strip() == "A" and j + 1 < end:
                    parts = lines[j + 1].split()
                    if len(parts) == 2 and all(re.match(r"^-?\d+$", p) for p in parts):
                        found.append((int(parts[0]), int(parts[1])))
                    j += 2
                    continue
                j += 1
            yield path.name, int(lines[start].strip()[1:]), int(head[7]), int(head[6]), found


def test_player_gear_has_no_faster_pulse() -> None:
    faster, mob_only, scanned = [], [], 0
    for name, vnum, wear, extra, found in _objects():
        scanned += 1
        pulse = [(loc, mod) for loc, mod in found if loc in (57, 58) and mod < 0]
        if not pulse:
            continue
        if wear & 1 and not extra & 2:  # ITEM_TAKE and not ITEM_NOSHOW: a player can have it
            faster.append((name, vnum, pulse))
        else:
            mob_only.append(vnum)
    assert scanned > 5000, scanned
    # The scan must still see the mob-only procs, or it is not reading applies at all.
    assert mob_only, "no mob-only pulse objects found"
    assert not faster, faster


def test_spirit_totem_gives_wisdom_not_pulse() -> None:
    totem = source("classes/new_skills.c").read_text()
    assert "totem->affected[2].location = APPLY_WIS;" in totem
    assert "APPLY_SPELL_PULSE" not in totem


if __name__ == "__main__":
    for test in (test_values_take_exactly_three_decimals, test_command_is_registered,
                 test_changes_are_forger_only_and_go_through_properties,
                 test_rates_ship_the_revised_values, test_player_gear_has_no_faster_pulse,
                 test_spirit_totem_gives_wisdom_not_pulse):
        test()
    print("racial pulse contracts passed")
