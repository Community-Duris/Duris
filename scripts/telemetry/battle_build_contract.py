"""Definition-1 point observations of native shared-battle build context.

Exact fields preserve independent availability and an observed association
reference. Intrinsic validation proves no complete battle, continuous build
validity, support origin, arena outcome or applied equipment effect.
"""
from __future__ import annotations

from collections.abc import Mapping

try:
    from . import battle_contract as battle
except ImportError:
    import battle_contract as battle

DEFINITION_VERSION = 1
NATIVE_CONTEXT_VERSION = 1
UTC_UNKNOWN = -(1 << 63)
AVAILABLE = 1023
CONTEXT_QUALITY = 255

# Independent immutable order/width/sign contract. None denotes exact bytes.
FIELD_LAYOUT = (
    ('bctx_battle_boot_id', 8, False),
    ('bctx_battle_process_id', 8, False),
    ('bctx_battle_seq', 8, False),
    ('bctx_environment_id', 8, False),
    ('bctx_season_id', 8, False),
    ('bctx_config_id', 8, False),
    ('bctx_actor_id', 8, False),
    ('bctx_sequence', 8, False),
    ('bctx_association_revision', 8, False),
    ('bctx_at_monotonic_usec', 8, False),
    ('bctx_at_utc_usec', 8, True),
    ('bctx_association_fact_sequence', 4, False),
    ('bctx_definition_version', 2, False),
    ('bctx_native_context_version', 2, False),
    ('bctx_boundary', 1, False),
    ('bctx_status', 1, False),
    ('bctx_actor_kind', 1, False),
    ('bctx_build_version', 4, False),
    ('bctx_content_version', 4, False),
    ('bctx_available', 4, False),
    ('bctx_context_quality', 4, False),
    ('bctx_quality_flags', 4, False),
    ('bctx_primary_class_mask', 4, False),
    ('bctx_secondary_class_mask', 4, False),
    ('bctx_level', 2, False),
    ('bctx_race', 2, False),
    ('bctx_faction', 2, False),
    ('bctx_specialization', 1, False),
    ('bctx_base_str', 2, True),
    ('bctx_base_dex', 2, True),
    ('bctx_base_agi', 2, True),
    ('bctx_base_con', 2, True),
    ('bctx_base_pow', 2, True),
    ('bctx_base_int', 2, True),
    ('bctx_base_wis', 2, True),
    ('bctx_base_cha', 2, True),
    ('bctx_base_kar', 2, True),
    ('bctx_base_luk', 2, True),
    ('bctx_effective_str', 2, True),
    ('bctx_effective_dex', 2, True),
    ('bctx_effective_agi', 2, True),
    ('bctx_effective_con', 2, True),
    ('bctx_effective_pow', 2, True),
    ('bctx_effective_int', 2, True),
    ('bctx_effective_wis', 2, True),
    ('bctx_effective_cha', 2, True),
    ('bctx_effective_kar', 2, True),
    ('bctx_effective_luk', 2, True),
    ('bctx_base_hit', 4, True),
    ('bctx_base_mana', 4, True),
    ('bctx_base_vitality', 4, True),
    ('bctx_base_ward', 4, True),
    ('bctx_effective_hit', 4, True),
    ('bctx_effective_mana', 4, True),
    ('bctx_effective_vitality', 4, True),
    ('bctx_effective_ward', 4, True),
    ('bctx_current_hit', 4, True),
    ('bctx_current_mana', 4, True),
    ('bctx_current_vitality', 4, True),
    ('bctx_current_ward', 4, True),
    ('bctx_base_armor', 4, True),
    ('bctx_base_hitroll', 4, True),
    ('bctx_base_damroll', 4, True),
    ('bctx_effective_armor', 4, True),
    ('bctx_effective_hitroll', 4, True),
    ('bctx_effective_damroll', 4, True),
    ('bctx_saving_para', 1, True),
    ('bctx_saving_rod', 1, True),
    ('bctx_saving_fear', 1, True),
    ('bctx_saving_breath', 1, True),
    ('bctx_saving_spell', 1, True),
    ('bctx_effective_flags_1', 8, False),
    ('bctx_effective_flags_2', 8, False),
    ('bctx_effective_flags_3', 8, False),
    ('bctx_effective_flags_4', 8, False),
    ('bctx_effective_flags_5', 8, False),
    ('bctx_equipment_flags_1', 8, False),
    ('bctx_equipment_flags_2', 8, False),
    ('bctx_equipment_flags_3', 8, False),
    ('bctx_equipment_flags_4', 8, False),
    ('bctx_equipment_flags_5', 8, False),
    ('bctx_equipment_hit', 4, True),
    ('bctx_equipment_mana', 4, True),
    ('bctx_equipment_armor', 4, True),
    ('bctx_equipment_hitroll', 4, True),
    ('bctx_equipment_damroll', 4, True),
    ('bctx_equipment_occupied_slots_count', 1, False),
    ('bctx_equipment_melee_weapons_count', 1, False),
    ('bctx_equipment_ranged_weapons_count', 1, False),
    ('bctx_equipment_shields_count', 1, False),
    ('bctx_equipment_armor_count', 1, False),
    ('bctx_equipment_other_items_count', 1, False),
    ('bctx_equipment_dynamic_affects_count', 1, False),
    ('bctx_equipment_digest', 32, None),
    ('bctx_epic_catalog_skills', 2, False),
    ('bctx_epic_learned_skills', 2, False),
    ('bctx_epic_digest', 32, None),
    ('bctx_affect_nodes', 2, False),
    ('bctx_offensive_modifier_nodes', 2, False),
    ('bctx_armor_modifier_nodes', 2, False),
    ('bctx_resource_modifier_nodes', 2, False),
    ('bctx_unapplied_nodes', 2, False),
    ('bctx_affects_complete', 1, False),
    ('bctx_arena_membership', 1, False),
    ('bctx_arena_room', 1, False),
    ('bctx_arena_enabled', 1, False),
    ('bctx_arena_type', 1, False),
    ('bctx_arena_stage', 1, False),
    ('bctx_arena_team', 1, False),
    ('bctx_arena_player_flags', 4, True),
)

FIELDS = tuple(name for name, _, _ in FIELD_LAYOUT)
BYTE_FIELDS = tuple(name for name, _, signed in FIELD_LAYOUT if signed is None)
WIRE_BYTES = sum(width for _, width, _ in FIELD_LAYOUT)
BATTLE = ("bctx_battle_boot_id", "bctx_battle_process_id", "bctx_battle_seq")
STATS = ("str", "dex", "agi", "con", "pow", "int", "wis", "cha", "kar", "luk")
RESOURCES = ("hit", "mana", "vitality", "ward")
COMBAT = ("armor", "hitroll", "damroll")
EQUIPMENT_COUNTS = ("occupied_slots", "melee_weapons", "ranged_weapons", "shields",
                    "armor", "other_items", "dynamic_affects")
BASE_FIELDS = tuple("bctx_base_" + name for name in (*STATS, *RESOURCES, *COMBAT))
EFFECTIVE_FIELDS = tuple("bctx_effective_" + name for name in (*STATS, *RESOURCES, *COMBAT))
RESOURCE_FIELDS = tuple("bctx_current_" + name for name in RESOURCES)
SAVE_FIELDS = tuple("bctx_saving_" + name for name in ("para", "rod", "fear", "breath", "spell"))
FLAG_FIELDS = tuple("bctx_effective_flags_" + str(index) for index in range(1, 6))
EQUIPMENT_FIELDS = tuple(name for name in FIELDS if name.startswith("bctx_equipment_"))
EPIC_FIELDS = ("bctx_epic_catalog_skills", "bctx_epic_learned_skills", "bctx_epic_digest")
AFFECT_FIELDS = ("bctx_affect_nodes", "bctx_offensive_modifier_nodes", "bctx_armor_modifier_nodes",
                 "bctx_resource_modifier_nodes", "bctx_unapplied_nodes", "bctx_affects_complete")
METADATA_FIELDS = frozenset((*BATTLE, *("bctx_" + name for name in (
    "environment_id", "season_id", "config_id", "actor_id", "actor_kind", "sequence",
    "association_revision", "association_fact_sequence", "at_monotonic_usec", "at_utc_usec",
    "definition_version", "native_context_version", "boundary", "status", "build_version",
    "content_version", "quality_flags", "context_quality"))))


class BuildContractError(ValueError):
    """Invalid build point observation, with no source payload in diagnostics."""


def _require(condition: bool, reason: str) -> None:
    if not condition:
        raise BuildContractError(reason)


def observation_key(row: Mapping) -> tuple[int, int, int]:
    return row["bctx_battle_boot_id"], row["bctx_battle_process_id"], row["bctx_sequence"]


def validate_observation(row: Mapping) -> dict:
    _require(isinstance(row, Mapping) and set(row) == set(FIELDS), "exact build field set")
    for name, width, signed in FIELD_LAYOUT:
        value = row[name]
        if signed is None:
            _require(type(value) is bytes and len(value) == width, "build digest shape: " + name)
        else:
            limit = 1 << (width * 8 - int(signed))
            _require(type(value) is int and (-limit if signed else 0) <= value < limit,
                     "build width/type: " + name)
    v = dict(row)
    _require(all(v[name] > 0 for name in (*BATTLE, "bctx_environment_id", "bctx_season_id",
        "bctx_sequence", "bctx_association_revision", "bctx_association_fact_sequence")) and
        v["bctx_definition_version"] == DEFINITION_VERSION and
        v["bctx_native_context_version"] == NATIVE_CONTEXT_VERSION, "build identity/version/reference")
    actor, kind = v["bctx_actor_id"], v["bctx_actor_kind"]
    _require((0 < actor <= battle.INT32_MAX if kind == 1 else
              kind in (2, 3) and actor & battle.GENERATION_TAG != 0 and
              actor & ~battle.GENERATION_TAG != 0), "build live actor generation")
    mask, context_quality, quality = (v["bctx_" + name] for name in (
        "available", "context_quality", "quality_flags"))
    boundary, status = v["bctx_boundary"], v["bctx_status"]
    _require(mask & ~AVAILABLE == 0 and context_quality & ~CONTEXT_QUALITY == 0 and
             quality & ~battle.QUALITY_KNOWN == 0 and 1 <= boundary <= 8, "build availability/quality/boundary")
    versions = tuple(v["bctx_" + name] for name in ("config_id", "build_version", "content_version"))
    _require(all(value == 0 for value in versions) if boundary == 8 else
             all(value > 0 for value in versions), "build configuration evidence")

    def zero(names):
        return all(not any(v[name]) if name in BYTE_FIELDS else v[name] == 0 for name in names)

    if status == 2:
        _require(boundary >= 6 and quality & 1 != 0 and zero(set(FIELDS) - METADATA_FIELDS),
                 "unavailable build carries no preceding profile")
        return v
    _require(status == 1 and mask != 0 and boundary <= 5 and context_quality & 128 != 0,
             "build observed snapshot boundary/origin")
    for bit, names in ((1, BASE_FIELDS), (2, EFFECTIVE_FIELDS), (4, RESOURCE_FIELDS),
                       (8, SAVE_FIELDS), (16, FLAG_FIELDS)):
        _require(mask & bit != 0 or zero(names), "unavailable build value family")
    if mask & 32:
        counts = tuple(v["bctx_equipment_" + name + "_count"] for name in EQUIPMENT_COUNTS)
        _require(counts[0] <= 43 and counts[6] <= counts[0] and sum(counts[1:6]) == counts[0] and
                 any(v["bctx_equipment_digest"]) and context_quality & 2 == 0, "build fixed equipment")
    else:
        _require(zero(EQUIPMENT_FIELDS), "unavailable equipment family")
    if mask & 64:
        _require(kind == 1 and 0 < v["bctx_epic_catalog_skills"] <= 309 and
                 v["bctx_epic_learned_skills"] <= v["bctx_epic_catalog_skills"] and
                 any(v["bctx_epic_digest"]) and context_quality & 1 == 0, "build learned epic catalog")
    else:
        _require(zero(EPIC_FIELDS), "unavailable epic family")
    if mask & 128:
        nodes, unapplied, complete = (v["bctx_" + name] for name in (
            "affect_nodes", "unapplied_nodes", "affects_complete"))
        modifiers = sum(v["bctx_" + name + "_modifier_nodes"] for name in ("offensive", "armor", "resource"))
        partial = context_quality & 12
        _require(nodes <= 64 and unapplied <= nodes and modifiers <= nodes - unapplied and
                 complete <= 1 and (partial == 0 if complete else partial != 0) and
                 (context_quality & 4 == 0 or nodes == 64) and
                 (context_quality & 8 == 0 or nodes > 0), "build listed affect prefix")
    else:
        _require(zero(AFFECT_FIELDS), "unavailable listed affect family")
    _require(v["bctx_arena_room"] <= 1 and (mask & 256 != 0 or v["bctx_arena_room"] == 0) and
             (mask & 256 == 0 or context_quality & 64 == 0) and
             v["bctx_arena_enabled"] <= 1 and v["bctx_arena_type"] <= 5 and v["bctx_arena_stage"] <= 5,
             "build independent arena room and global values")
    membership, team, flags = (v["bctx_arena_" + name] for name in ("membership", "team", "player_flags"))
    if mask & 512:
        _require(context_quality & 48 == 0 and
                 (team == 0 and flags == 0 if membership == 1 else
                  membership == 2 and kind == 1 and 1 <= team <= 3), "build observed arena roster")
    else:
        _require((membership == 0 or membership == 3 and context_quality & 32 != 0) and
                 team == 0 and flags == 0, "build unavailable arena membership")
    return v


def encode_observation(row: Mapping) -> bytes:
    value = validate_observation(row)
    return b"".join(value[name] if signed is None else value[name].to_bytes(width, "big", signed=signed)
                    for name, width, signed in FIELD_LAYOUT)


def decode_observation(wire: bytes) -> dict:
    _require(isinstance(wire, (bytes, bytearray)) and len(wire) == WIRE_BYTES, "exact build wire length")
    value, offset = {}, 0
    for name, width, signed in FIELD_LAYOUT:
        part = wire[offset:offset + width]
        value[name] = bytes(part) if signed is None else int.from_bytes(part, "big", signed=signed)
        offset += width
    return validate_observation(value)


def validate_raw_observation(row: Mapping) -> dict:
    """Strict kind-12 storage binding; association completeness stays separate."""
    _require(isinstance(row, Mapping), "raw build mapping")
    _require(type(row.get("record_kind")) is int and row["record_kind"] == 12, "raw build tag")
    _require(type(row.get("schema_version")) is int and row["schema_version"] == 1, "raw build schema")
    value = validate_observation({name: row.get(name) for name in FIELDS})
    for name in ("boot_id", "process_id", "record_seq"):
        _require(type(row.get(name)) is int and 0 < row[name] < (1 << 64), "raw build replay identity")
    _require((row["boot_id"], row["process_id"]) == observation_key(value)[:2] and
             type(row.get("occurrence_utc_usec")) is int and
             row["occurrence_utc_usec"] == value["bctx_at_utc_usec"], "raw build producer/occurrence binding")
    header = {"ingest_id", "boot_id", "process_id", "record_seq", "schema_version",
              "record_kind", "occurrence_utc_usec", "ingested_utc_usec"}
    _require(all(item is None for name, item in row.items() if name not in FIELDS and name not in header),
             "raw build inactive family payload")
    return value
