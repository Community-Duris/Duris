#!/usr/bin/env python3
"""Contracts for the epic point and epic skill level gates.

epic.gain.minLevel (default 50): no epic points below it, from any source.
epic.skills.minLevel (default 56): no epic skills learned below it.
Touch-stone level costs for 51-56 are untouched.
"""

from __future__ import annotations

import subprocess
import tempfile
from pathlib import Path

from _paths import ROOT, source


PROPERTIES = (ROOT / "lib" / "duris.properties").read_text()


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


def test_helpers_read_the_levels_and_clamp_them() -> None:
    helpers = "\n".join(_body("world/epic.c", signature) for signature in (
        "int epic_gain_min_level()", "bool epic_level_can_gain(P_char ch)",
        "int epic_skills_min_level()"))
    harness = r'''
#include <cstring>
#include <map>
#include <string>
#define MAXLVL 62
#define BOUNDED(low, value, high) ((value) < (low) ? (low) : (value) > (high) ? (high) : (value))
struct test_player { struct { int level; } player; };
using P_char = test_player *;
#define GET_LEVEL(ch) ((ch)->player.level)
static std::map<std::string, float> props;
float get_property(const char *key, double fallback) {
    const auto found = props.find(key);
    return found == props.end() ? static_cast<float>(fallback) : found->second;
}
''' + helpers + r'''
int main() {
    // Defaults: epics from 50, epic skills from 56.
    if (epic_gain_min_level() != 50 || epic_skills_min_level() != 56) return 1;
    test_player low{{49}}, gate{{50}}, top{{56}};
    if (epic_level_can_gain(&low) || !epic_level_can_gain(&gate) || !epic_level_can_gain(&top)) return 2;
    if (epic_level_can_gain(nullptr)) return 3;
    // Any level can be chosen; out-of-range values are clamped to 1..MAXLVL.
    props["epic.gain.minLevel"] = 56; props["epic.skills.minLevel"] = 40;
    if (epic_level_can_gain(&gate) || !epic_level_can_gain(&top)) return 4;
    if (epic_skills_min_level() != 40) return 5;
    props["epic.gain.minLevel"] = 0; props["epic.skills.minLevel"] = 500;
    if (epic_gain_min_level() != 1 || epic_skills_min_level() != 62) return 6;
    return 0;
}
'''
    with tempfile.TemporaryDirectory(prefix="duris-epic-level-gate-") as directory:
        root = Path(directory)
        source_path, binary = root / "harness.cpp", root / "harness"
        source_path.write_text(harness)
        subprocess.run(["g++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                        str(source_path), "-o", str(binary)], cwd=ROOT, check=True)
        subprocess.run([str(binary)], cwd=ROOT, check=True)


def test_every_epic_source_is_gated() -> None:
    prepare = _flat(_body("world/epic.c", "static bool prepare_epic_award("))
    npc = prepare.index(_flat("if (amount < 1 || IS_NPC(ch))"))
    gate = prepare.index(_flat("if (!epic_level_can_gain(ch)) return false;"))
    assert npc < gate < prepare.index(_flat("bool blessing ="))
    pvp = _flat(_body("world/epic.c", "int epic_calculate_pvp_award("))
    assert _flat("|| !epic_level_can_gain(ch)) return 0;") in pvp


def test_stones_refuse_ineligible_touchers_and_skip_ineligible_members() -> None:
    stone = _flat(_body("world/epic.c", "int epic_stone(P_obj obj, P_char ch, int cmd, char *arg)"))
    refusal = stone.index(_flat("if (!epic_level_can_gain(ch))"))
    assert refusal < stone.index(_flat("int epic_value = epic_stone_payout(obj, ch);"))
    assert refusal < stone.index(_flat("vector<P_char> participants = { ch };"))
    assert _flat("gl->ch->in_room == ch->in_room && epic_level_can_gain(gl->ch)) "
                 "participants.push_back(gl->ch);") in stone
    # The touch-stone level purchase is untouched.
    level = _flat(_body("world/epic.c", "static void epic_stone_level_char_from_level("))
    assert "epic_level_can_gain" not in level and "epic_gain_min_level" not in level


def test_epic_teachers_refuse_below_the_skill_level() -> None:
    teacher = _flat(_body("classes/epic_skills.c", "int epic_teacher(P_char ch, P_char pl, int cmd, char *arg)"))
    gate = teacher.index(_flat("if (GET_LEVEL(pl) < epic_skills_min_level())"))
    assert gate < teacher.index(_flat("epics_cost = 3 *"))
    assert gate < teacher.index("epic_transaction_submit(")


def test_properties_ship_the_defaults() -> None:
    section = PROPERTIES[PROPERTIES.index("[epic]"):]
    section = section[: section.index("\n[", 1)].splitlines()
    assert "epic.gain.minLevel=50.000" in section
    assert "epic.skills.minLevel=56.000" in section
    # Levels 51-56 still cost what they cost.
    for level, cost in ((51, "1000.000"), (56, "13500.000")):
        assert f"epic.forLevel.{level}={cost}" in section


if __name__ == "__main__":
    tests = [
        test_helpers_read_the_levels_and_clamp_them,
        test_every_epic_source_is_gated,
        test_stones_refuse_ineligible_touchers_and_skip_ineligible_members,
        test_epic_teachers_refuse_below_the_skill_level,
        test_properties_ship_the_defaults,
    ]
    for test in tests:
        test()
    print("epic level gate contracts passed")
