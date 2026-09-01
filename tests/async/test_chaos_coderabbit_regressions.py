#!/usr/bin/env python3
"""Regression contracts for the CodeRabbit-reviewed Chaos analysis paths."""

from __future__ import annotations

import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SCRIPTS = ROOT / "scripts"
SRC = ROOT / "src"
sys.path.insert(0, str(SCRIPTS))

from chaos_eq_analyze import ordered_field_union  # noqa: E402
from chaos_eq_validate import catalog_referenced_vnums  # noqa: E402

ANALYZE = (SCRIPTS / "chaos_eq_analyze.py").read_text(encoding="utf-8")
CATALOG = (SCRIPTS / "chaos_eq_catalog.py").read_text(encoding="utf-8")
REPORT = (SCRIPTS / "chaos_eq_report.py").read_text(encoding="utf-8")
VALIDATE = (SCRIPTS / "chaos_eq_validate.py").read_text(encoding="utf-8")
NANNY = (SRC / "account" / "nanny.c").read_text(encoding="utf-8")


assert ordered_field_union(
    [{"role": "spellbook", "vnum": 7}, {"instrument_type": "flute", "vnum": 1734}],
    ["role", "vnum"],
) == ["role", "vnum", "instrument_type"]
assert ordered_field_union([], ["role", "vnum"]) == ["role", "vnum"]

assert "(obj.values[0] & TOTEM_SPHERE_MASK) != TOTEM_SPHERE_MASK" in ANALYZE
for token in (
    'defines.get("ITEM_INSTRUMENT", 32)',
    'defines.get("ITEM_SPELLBOOK", 33)',
    'defines.get("ITEM_TOTEM", 34)',
    "ordered_field_union(fundamentals, [\"role\", \"vnum\"])",
):
    assert token in ANALYZE, token

assert "for class_id in class_ids.values()" in CATALOG
assert "range(1, 31)" not in CATALOG
assert 'alt_book = choose_book(metrics, "enhanceable", class_ids)' in CATALOG

append_body = NANNY.split("static bool append_chaos_kit_item", 1)[1].split(
    "static void load_chaos_new_character_kit", 1
)[0]
unusable_body = append_body.split("if (item->slot >= 0 && !can_char_use_item", 1)[1]
assert "extract_obj(obj, FALSE);" in unusable_body
assert "return true;" in unusable_body

assert "assert vnum in objects, (array_name, vnum)" in (
    ROOT / "tests/async/test_chaos_new_character_kit.py"
).read_text(encoding="utf-8")
assert "catalog_referenced_vnums" in VALIDATE
assert "ambiguous duplicate AREA prototypes" in VALIDATE
assert "book_obj = objects.get(book_vnum)" in VALIDATE
assert "book_obj.wear_flags & attach_belt" in VALIDATE

sample_catalog = {
    "profiles": {
        "standard": {"Warrior": {"equipment": [{"vnum": 101}], "support_items": []}},
        "enhanceable": {},
    },
    "fundamentals": {
        "standard": {"spellbook": {"vnum": 7}, "bard_instruments": [{"vnum": 1734}], "shaman_totem": None}
    },
    "optional_race_slot_variations": {"standard": [{"status": "available", "vnum": 202}]},
    "consumables": [{"vnum": 303}],
}
assert catalog_referenced_vnums(sample_catalog) == {7, 101, 1734, 202, 303}

assert "Shadow Beast" in REPORT
assert "Psionic Beast" not in REPORT
assert "format_database_counts(counts)" in REPORT
for stale_literal in ("2,733", "131-character", "player_data` 785", "50,239", "289,015", "6 passed, 0 failed"):
    assert stale_literal not in REPORT, stale_literal

print("CodeRabbit Chaos regression contracts passed")
