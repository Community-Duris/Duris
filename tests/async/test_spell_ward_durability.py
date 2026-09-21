#!/usr/bin/env python3
"""Source contracts for finite, renewable spell wards (issue #453)."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"
ward = (SRC / "combat" / "spell_wards.c").read_text()
fight = (SRC / "combat" / "fight.c").read_text()
affects = (SRC / "magic" / "affects.c").read_text()
magic = (SRC / "magic" / "magic.c").read_text()
smagic = (SRC / "magic" / "smagic.c").read_text()
utility = (SRC / "core" / "utility.c").read_text()
json_utils = (SRC / "core" / "json_utils.c").read_text()
files = (SRC / "core" / "files.c").read_text()
sql = (SRC / "sql" / "sql_player.c").read_text()
snapshot = (SRC / "player" / "player_snapshot_repository.c").read_text()
schema = (ROOT / "migrations" / "spell_ward_durability.sql").read_text()


for key, value in (
    ("Minor Globe budget", 'spell.ward.damagePerTick.minorGlobe'),
    ("Spirit Ward budget", 'spell.ward.damagePerTick.spiritWard'),
    ("Greater Spirit Ward budget", 'spell.ward.damagePerTick.greaterSpiritWard'),
    ("Globe budget", 'spell.ward.damagePerTick.globe'),
):
    assert value in ward, key

assert "result.remaining = damage - result.blocked" in ward
assert "std::ceil(result.blocked)" in ward
assert "return has_available_for_flags(victim, flags)" in ward
assert "ward_source_uid" in ward and "ward_refresh_remaining" in ward
assert "spell_ward_mask_equipment_bits" in affects
assert "if (IS_PC(ch))" in affects
assert "const spell_ward_absorb_result ward = spell_ward_absorb(ch, victim, dam, flags)" in fight
assert "dam = ward.remaining" in fight
assert "spell_ward_apply_cast(victim, &af, duration)" in magic
assert "spell_ward_apply_cast(victim, &af, duration)" in smagic
assert "return !spell_ward_has_available(ch, spl)" in utility
assert 'cJSON_AddStringToObject(affect_obj, "state", state)' in json_utils
assert "ADD_ULL(buf, af->ward_source_uid)" in files
assert "af.ward_capacity = static_cast<int64_t>(sql_row_ull(row, 15, 0))" in sql
assert "ward_capacity_max,ward_refresh_remaining" in snapshot
for column in (
    "ward_source_uid",
    "ward_full_duration",
    "ward_capacity",
    "ward_capacity_max",
    "ward_refresh_remaining",
    "ward_source_type",
    "ward_source_worn",
    "ward_active",
):
    assert column in schema, column

assert "#define SAV_AFFVERS 9" in (SRC / "core" / "files.h").read_text()
print("spell ward durability contracts passed")
