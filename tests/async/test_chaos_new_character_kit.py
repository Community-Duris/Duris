#!/usr/bin/env python3
"""Contracts for generated, policy-checked CHAOS starter-kit data."""

from __future__ import annotations

import re
import sys
from pathlib import Path

from _paths import ROOT, SRC

sys.path.insert(0, str(ROOT / "scripts"))
from chaos_eq_analyze import (  # noqa: E402
    area_file_names,
    item_exclusion_reasons,
    object_class_allowed,
    parse_defines,
    reconcile_area_objects,
)

NANNY = (SRC / "nanny.c").read_text(encoding="utf-8", errors="replace")
DATA = (SRC / "chaos_eq_data.h").read_text(encoding="utf-8", errors="replace")
CONFIG = (SRC / "chaos_config.c").read_text(encoding="utf-8", errors="replace")
ENV_EXAMPLE = (ROOT / ".env.example").read_text(encoding="utf-8", errors="replace")
DEFINES_PATH = ROOT / "src/core/defines.h"
DEFINES = parse_defines(DEFINES_PATH)

CLASS_NAMES = [
    "Warrior", "Ranger", "Psionicist", "Paladin", "Anti-Paladin", "Cleric",
    "Monk", "Druid", "Shaman", "Sorcerer", "Necromancer", "Conjurer",
    "Rogue", "Assassin", "Mercenary", "Bard", "Thief", "Warlock",
    "MindFlayer", "Alchemist", "Berserker", "Reaver", "Illusionist",
    "Blighter", "Dreadlord", "Ethermancer", "Avenger", "Theurgist",
    "Summoner", "Dragoon",
]
CLASS_IDS = {name: index for index, name in enumerate(CLASS_NAMES, 1)}

# Equipment in the generated per-class arrays. HOLD is populated only by the
# class fundamentals; support entries use WEAR_NONE and are still bag items.
CORE_SLOTS = set(range(1, 18)) | set(range(19, 25)) | set(range(27, 31))
SLOT_WEAR_BITS = {
    **{slot: 1 << 1 for slot in (1, 2)},
    **{slot: 1 << 2 for slot in (3, 4)},
    5: 1 << 3, 6: 1 << 4, 7: 1 << 5, 8: 1 << 6, 9: 1 << 7,
    10: 1 << 8, 11: 1 << 9, 12: 1 << 10, 13: 1 << 11,
    14: 1 << 12, 15: 1 << 12, 16: 1 << 13, 17: 1 << 13,
    18: 1 << 14, 19: 1 << 17, 20: 1 << 18, 21: 1 << 19,
    22: 1 << 19, 23: 1 << 20, 24: 1 << 21, 25: 1 << 13,
    26: 1 << 13, 27: 1 << 22, 28: 1 << 23, 29: 1 << 23,
    30: 1 << 23, 31: 1 << 8, 32: 1 << 7, 33: 1 << 12,
    34: 1 << 12, 35: 1 << 24, 36: 1 << 5, 37: 1 << 25,
    38: 1 << 6, 39: 1 << 26, 40: 1 << 27, 41: 1 << 28, 42: 1 << 29,
}

area_paths = area_file_names(ROOT / "areas/obj", ROOT / "areas/AREA")
area_text = "\n".join(path.read_text(encoding="utf-8", errors="replace") for path in area_paths)
active_vnums = {int(value) for value in re.findall(r"(?m)^#(\d+)$", area_text) if int(value) != 9999999}
objects, area_diagnostics = reconcile_area_objects(area_paths, {}, DEFINES)
assert not area_diagnostics["parse_errors"], area_diagnostics["parse_errors"]
assert 96443 in active_vnums
assert 7 in active_vnums

array_matches = [
    (name, body)
    for name, body in re.findall(
        r"static const chaos_kit_item (chaos_eq_(?:standard|enhanceable)_[a-z0-9_]+)\[\] = \{(.*?)\};",
        DATA,
        re.S,
    )
    if not name.endswith("_optional_slots")
]
assert len(array_matches) == 60, len(array_matches)
array_names = {name for name, _ in array_matches}
for profile in ("standard", "enhanceable"):
    assert {f"chaos_eq_{profile}_{re.sub(r'[^a-z0-9]+', '_', name.lower()).strip('_')}" for name in CLASS_NAMES} <= array_names

all_data_vnums: set[int] = set()
parsed_arrays: dict[str, list[tuple[int, int]]] = {}
for array_name, body in array_matches:
    pairs = re.findall(r"\{\s*(-?\d+|WEAR_NONE),\s*(\d+)\s*\}", body)
    assert pairs and pairs[-1] == ("WEAR_NONE", "0"), array_name
    parsed: list[tuple[int, int]] = []
    seen_slots: set[int] = set()
    for raw_slot, raw_vnum in pairs[:-1]:
        slot = -1 if raw_slot == "WEAR_NONE" else int(raw_slot)
        vnum = int(raw_vnum)
        assert vnum != 1252, (array_name, vnum)
        assert vnum in objects, (array_name, vnum)
        all_data_vnums.add(vnum)
        if slot >= 0:
            assert slot in CORE_SLOTS or slot == 18, (array_name, slot)
            assert slot not in seen_slots, (array_name, slot)
            seen_slots.add(slot)
            obj = objects[vnum]
            assert obj.wear_flags & SLOT_WEAR_BITS[slot], (array_name, slot, vnum)
        parsed.append((slot, vnum))
    parsed_arrays[array_name] = parsed

support_body = re.search(r"static const chaos_kit_item chaos_eq_support_consumables\[\] = \{(.*?)\};", DATA, re.S)
assert support_body
support_pairs = re.findall(r"\{\s*(-?\d+|WEAR_NONE),\s*(\d+)\s*\}", support_body.group(1))
assert support_pairs and support_pairs[-1] == ("WEAR_NONE", "0")
for raw_slot, raw_vnum in support_pairs[:-1]:
    vnum = int(raw_vnum)
    assert vnum != 1252
    assert vnum in objects
    all_data_vnums.add(vnum)

optional_matches = re.findall(
    r"static const chaos_kit_item chaos_eq_(?:standard|enhanceable)_optional_slots\[\] = \{(.*?)\};",
    DATA,
    re.S,
)
assert len(optional_matches) == 2
for body in optional_matches:
    pairs = re.findall(r"\{\s*(-?\d+|WEAR_NONE),\s*(\d+)\s*\}", body)
    assert pairs and pairs[-1] == ("WEAR_NONE", "0")
    for _, raw_vnum in pairs[:-1]:
        vnum = int(raw_vnum)
        assert vnum != 1252
        assert vnum in objects
        all_data_vnums.add(vnum)

# Every core item must be free of item-level policy violations and usable by
# the class owning the generated array. Fundamental support rows may carry the
# explicitly allowed quest/no-sell markers, but never artifact/Ioun/unique or
# race restrictions.
allowed_fundamental_exclusions = {"item_transient", "item_norent", "item_noshow", "item_nosell", "quest_item"}
fundamental_vnums = {7, 83336, 88315, 138533, 138534, 138535, 138536, 138537, 138538, 1734, 1736, 1737, 1738, 28971, 33705}
for array_name, pairs in parsed_arrays.items():
    profile = "standard" if "_standard_" in array_name else "enhanceable"
    class_key = array_name.split(f"chaos_eq_{profile}_", 1)[1]
    class_name = next(name for name in CLASS_NAMES if re.sub(r"[^a-z0-9]+", "_", name.lower()).strip("_") == class_key)
    class_id = CLASS_IDS[class_name]
    for slot, vnum in pairs:
        obj = objects[vnum]
        reasons = item_exclusion_reasons(obj, DEFINES, [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 13, 14, 15, 16, 17, 20, 30, 31, 32, 36, 37])
        if slot >= 0:
            if vnum in fundamental_vnums:
                assert not [reason for reason in reasons if reason not in allowed_fundamental_exclusions], (array_name, slot, vnum, reasons)
            else:
                assert not reasons, (array_name, slot, vnum, reasons)
            assert object_class_allowed(obj, class_id, DEFINES), (array_name, class_name, vnum)
        else:
            assert vnum in fundamental_vnums, (array_name, vnum)
            assert not [reason for reason in reasons if reason not in allowed_fundamental_exclusions], (array_name, vnum, reasons)

# Profile table contains the unused zero row plus one row per class.
profiles = DATA.split("static const chaos_eq_profile chaos_eq_profiles[CLASS_COUNT + 1][2] = {", 1)[1].split("};", 1)[0]
assert len(re.findall(r"\{\s*\{\s*(?:NULL|chaos_eq_[a-z0-9_]+)\s*\},\s*\{\s*(?:NULL|chaos_eq_[a-z0-9_]+)\s*\}\s*\},", profiles)) == 31

# The loader builds a single nested tree, preserves spellbook population, and
# selects standard vs strict-enhanceable data through the profile gate.
chaos_code = NANNY.split("static void prepare_chaos_kit_item", 1)[1].split("void load_obj_to_newbies", 1)[0]
assert "chaos_eq_profiles[class_id][profile_id]" in chaos_code
assert "chaos_eq_support_consumables" in chaos_code
assert "chaos_eq_enhanceable_optional_slots" in chaos_code
assert "chaos_eq_standard_optional_slots" in chaos_code
assert "obj_to_obj(obj, bag)" in chaos_code
assert "AddSpellToSpellBook(ch, obj, j)" in chaos_code
assert "has_eq_slot(ch, item->slot)" in chaos_code
assert "can_char_use_item(ch, obj)" in chaos_code
assert "REMOVE_BIT(obj->extra_flags, ITEM_SECRET);" in chaos_code
assert chaos_code.count("item_creation_grant_submit_to_player_before_entry(ch, bag, ch)") == 1
assert "item_creation_grant_submit_to_player(ch, obj, ch, bag)" not in chaos_code
assert "1252" not in chaos_code
assert chaos_code.index("if (item_failure)") < chaos_code.index("item_creation_grant_submit_to_player_before_entry(ch, bag, ch)")
# The master spellbook remains the dynamic all-spells object and is beltable.
master = objects[7]
assert master.object_type == DEFINES["ITEM_SPELLBOOK"]
assert master.wear_flags & DEFINES["ITEM_ATTACH_BELT"]
assert "CHAOS_EQ_PROFILE" in CONFIG
assert "CHAOS_EQ_PROFILE=standard" in ENV_EXAMPLE

print("CHAOS generated kit, restrictions, fundamentals, and one-root grant contracts passed")
