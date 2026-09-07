#!/usr/bin/env python3
"""Offline role/policy generation, source validation, and deterministic artifacts."""
from __future__ import annotations

from copy import deepcopy
import json
from pathlib import Path
import re
import shutil
import sys
import tempfile
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))
from chaos_eq_catalog import (
    CORE_SLOTS, PHYSICAL_CLASSES, PERMANENT_POLICY, UTILITY_POLICY, WEAPON_SLOTS,
    build_catalog, emit_header, emit_policy_report, role_item_valid, static_analysis,
)
from chaos_eq_validate import validate

seed = json.loads((ROOT / "docs/data/chaos_eq_seed.json").read_text())
analysis = static_analysis(seed, ROOT)
catalog = build_catalog(analysis)
assert validate(catalog, ROOT) == []
assert analysis["cohort"]["observed_characters"] == 0
assert all(item["observed_players"] == 0 for item in analysis["candidates"])
metrics = {item["vnum"]: item for item in analysis["candidates"]}
expected_physical = {
    "Warrior", "Ranger", "Paladin", "Anti-Paladin", "Monk", "Rogue", "Assassin",
    "Mercenary", "Bard", "Thief", "Berserker", "Reaver", "Dreadlord", "Avenger", "Dragoon",
}
assert PHYSICAL_CLASSES == expected_physical
for profile, matrix in catalog["profiles"].items():
    assert set(matrix) == set(seed["class_ids"])
    for class_name, row in matrix.items():
        items = row["equipment"]
        for item in items:
            assert item["permanent_policy"] == PERMANENT_POLICY
            assert role_item_valid(metrics[item["vnum"]], class_name, item["slot"])
        globes = [item for item in items if item.get("granted_bitvector2")]
        if class_name in expected_physical:
            slots = set(CORE_SLOTS) - (WEAPON_SLOTS if class_name == "Monk" else set())
            assert {item["slot"] for item in items} == slots
            assert len(globes) == 1 and globes[0]["slot"] == 3
            assert globes[0]["granted_bitvector2"] == "AFF2_GLOBE"
        else:
            assert not globes
        if class_name == "Monk":
            for item in items + row["support_items"]:
                prototype = metrics[item["vnum"]]
                assert item["slot"] not in {16, 17, 25, 26}
                assert prototype["item_type"] not in {5, 6, 7}
                assert not prototype["static"]["wear_flags"] & (1 << 13)
    for item in catalog["optional_race_slot_variations"][profile]:
        if item["status"] == "available":
            assert role_item_valid(metrics[item["vnum"]], "Warrior", item["slot"])

assert [(item["vnum"], item["skill"], item["count"]) for item in UTILITY_POLICY] == [
    (336, "SKILL_FISHING", 1), (412, "SKILL_PICK_LOCK", 1),
    (73, "SKILL_TRAP", 3), (400227, "SKILL_SALVAGE", 3),
]
assert len({item["vnum"] for item in UTILITY_POLICY}) == len(UTILITY_POLICY)
assert metrics[336]["item_type"] == 8
assert metrics[412]["item_type"] == 31
for item in UTILITY_POLICY:
    assert metrics[item["vnum"]]["race_portable"]
    assert all(metrics[item["vnum"]]["class_eligible"].values())
# Actual consuming/arming implementations back these grants; no invented bait,
# disabled legacy hammer/parchment, or temporary globe potion is used.
assert "real_object0(73)].func.obj = huntsman_ward" in (ROOT / "src/specs/specs.assign.c").read_text()
assert "#define VOBJ_EPIC_LANTAN_TOOLS 400227" in (ROOT / "src/world/vnum.obj.h").read_text()
assert "vnum_from_inv(ch, crafting_scientific_tools_vnum(), 1)" in (ROOT / "src/item/salvage.c").read_text()

# Sparse-cohort fallbacks cannot choose max-WIS, even with high popularity or
# nominal power; a mixed physical/mental item can still support a hybrid role.
assert not role_item_valid({"effect_summary": {"affects": {"wis_max": 1, "damroll": 20}}}, "Warrior", 1)
assert not role_item_valid({"effect_summary": {"affects": {"int_max": 5, "mana": 100}}}, "Assassin", 1)
assert role_item_valid({"effect_summary": {"affects": {"mana": 10, "damroll": 2}}}, "Paladin", 1)
for item_type in (5, 6, 7):
    assert not role_item_valid({"item_type": item_type}, "Monk", 18)

# Corrupt independent policies in one catalog so a single canonical source pass
# exercises fail-closed behavior without repeatedly reparsing the world.
bad = deepcopy(catalog)
warrior = bad["profiles"]["standard"]["Warrior"]["equipment"]
next(item for item in warrior if item["slot"] == 3).pop("granted_bitvector2")
warrior[0]["permanent_policy"] = {}
warrior.pop()
monk = bad["profiles"]["enhanceable"]["Monk"]["equipment"]
weapon = next(item for item in catalog["profiles"]["enhanceable"]["Warrior"]["equipment"] if item["slot"] == 16)
monk.append(deepcopy(weapon))
wis = next(item for item in analysis["candidates"] if item["effect_summary"]["affects"].get("wis_max", 0) > 0)
# The source-based validator must reject max-WIS even if emitted summaries lie.
warrior[1]["vnum"] = wis["vnum"]
warrior[1]["effect_summary"] = {"affects": {}}
optional = next(item for item in bad["optional_race_slot_variations"]["standard"] if item["status"] == "available")
optional["vnum"] = wis["vnum"]
bad["utility_items"] = deepcopy(UTILITY_POLICY) + [deepcopy(UTILITY_POLICY[0])]
issues = validate(bad, ROOT)
for expected in ("incorrect permanent globe", "missing permanent policy", "incomplete physical role",
                 "weapon-bearing item", "max-WIS item", "role-inappropriate fallback", "duplicate object policy", "skill, count, or object policy mismatch"):
    assert any(expected in issue for issue in issues), (expected, issues)

# The generated instance effect must remain enhanceable, not just its prototype.
import chaos_eq_validate as validator
original_parse = validator.parse_enhance_config
def without_globe(path, constants):
    config = original_parse(path, constants)
    config["allow_masks"][1] &= ~constants["AFF2_GLOBE"]
    return config
with patch.object(validator, "parse_enhance_config", without_globe):
    assert any("starter globe is excluded" in issue for issue in validate(catalog, ROOT))

# Compare actual generated artifacts, not an alternate hand-written header.
with tempfile.TemporaryDirectory(prefix="duris-chaos-policy-") as temp_dir:
    header = Path(temp_dir) / "chaos_eq_data.h"
    emit_header(header, analysis, catalog, ROOT)
    expected_header = (ROOT / "src/account/chaos_eq_data.h").read_text()
    if shutil.which("clang-format"):
        assert header.read_text() == expected_header
    else:
        # Formatter availability affects whitespace only; compare tokens.
        assert re.findall(r"\S+", re.sub(r"\s+", " ", header.read_text())) == re.findall(r"\S+", re.sub(r"\s+", " ", expected_header))
    report = Path(temp_dir) / "CHAOS_KIT_CATALOG.md"
    emit_policy_report(report, catalog)
    assert report.read_text() == (ROOT / "docs/reference/CHAOS_KIT_CATALOG.md").read_text()
print("CHAOS policy: all 60 profiles, Monk/physical roles, permanent globe, utility gates, corruption rejection and regeneration passed")
