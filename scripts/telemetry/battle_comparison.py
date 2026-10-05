"""Versioned matching dimensions for exact, retained native build points.

The definition describes observed fields, not a combat-strength model.  A caller
must name its matching dimensions; there is no default set that controls away
the race, class or mechanic a study intends to measure.  Point qualification and
each family's availability remain independent.  Nothing extends a point into
an interval or assigns damage, healing or a battle outcome to a sampled build.
"""
from __future__ import annotations

import hashlib
import json
from collections.abc import Mapping, Sequence

try:
    from . import battle_contract as battle, battle_build_contract as builds, identity_history as identity
    from .rollup_definitions import (ROLLUP_QUALITY_PROCESS_GAP, ROLLUP_QUALITY_INCIDENT_GAP,
        ROLLUP_QUALITY_INCIDENT_INVENTORY_UNKNOWN, ROLLUP_QUALITY_MASK)
except ImportError:
    import battle_contract as battle
    import battle_build_contract as builds
    import identity_history as identity
    from rollup_definitions import (ROLLUP_QUALITY_PROCESS_GAP, ROLLUP_QUALITY_INCIDENT_GAP,
        ROLLUP_QUALITY_INCIDENT_INVENTORY_UNKNOWN, ROLLUP_QUALITY_MASK)

CLASSIFICATION_VERSION = 1

# Native context v1 copies the CLASS_* masks from src/core/defines.h.  The
# specialization inventory is the nonempty, non-"Not Used" specdata cells in
# src/core/common.c, indexed by the primary class's single bit.  Definitions in
# structs.h alone do not establish a currently classified specialization.
CLASS_MASK = (1 << 30) - 1
SPECIALIZATIONS = (
    (), (1, 2, 3), (1, 2, 3), (1, 2, 3), (1, 2), (1, 2, 3), (1, 2, 3),
    (1, 2), (1, 2), (1, 2, 3), (1, 2, 3), (1, 2, 3), (1, 2, 3, 4),
    (1, 2, 4), (), (1, 2), (1, 2, 3), (), (), (), (1, 2), (1, 2),
    (1, 2, 3, 4), (1, 2), (1, 2, 3), (1, 2), (1, 2, 3), (1, 2),
    (1, 2, 3), (1, 2, 3), (1, 2, 3),
)
CATALOG_DIGEST = hashlib.sha256(json.dumps([CLASSIFICATION_VERSION, CLASS_MASK, SPECIALIZATIONS],
    separators=(",", ":")).encode("ascii")).hexdigest()
DIMENSIONS = (
    "level", "classes", "specialization", "race", "faction", "base_setup", "learned_epics",
    "equipment", "effective_setup", "current_resources", "saving_modifiers", "effective_flags",
    "listed_effects", "temporary_effect_origin", "support_origin", "arena_room", "arena_roster",
)
FATAL_POINT_QUALITY = (battle.QUALITY_KNOWN & ~1) | ROLLUP_QUALITY_PROCESS_GAP | identity.UTC_ATTRIBUTION_FLAGS | (
    ROLLUP_QUALITY_INCIDENT_GAP | ROLLUP_QUALITY_INCIDENT_INVENTORY_UNKNOWN)


class ComparisonError(ValueError):
    """Refused comparison; diagnostics do not contain source values."""


def _require(condition, reason):
    if not condition:
        raise ComparisonError(reason)


def point_context_verified(point: Mapping) -> bool:
    # Keep the established v6/v7 point qualification unchanged. UNKNOWN_CONTEXT
    # describes independently unavailable families and unknown effect origin.
    return (point["bctx_status"] == 1 and point["link_status"] == point["point_clock_status"] ==
        point["configuration_status"] == "verified" and not point["publication_quality_flags"] & FATAL_POINT_QUALITY)


def classify_point(point: Mapping) -> dict:
    """Classify a publication point without borrowing another point or roster.

    The input must have passed retained-source/history/configuration review;
    this value function cannot authenticate that review.  Recompute the derived
    qualification bit and validate the native fields so a caller cannot turn an
    unavailable family or a stale reference into a matching value.
    """
    _require(isinstance(point, Mapping), "comparison_point_type")
    _require(all(name in point for name in (*builds.FIELDS, "canonical_battle", "link_status",
        "point_clock_status", "configuration_status", "publication_quality_flags")), "comparison_point_fields")
    raw = {name: point[name] for name in builds.FIELDS}
    for name in builds.BYTE_FIELDS:
        if type(raw[name]) is str:
            _require(len(raw[name]) == 64 and all(ch in "0123456789abcdef" for ch in raw[name]), "comparison_digest")
            raw[name] = bytes.fromhex(raw[name])
    builds.validate_observation(raw)
    _require(point["link_status"] in ("verified", "missing_packet", "partial_history", "stale_association", "outside_observed_prefix") and
        point["point_clock_status"] in ("verified", "unverified", "unknown", "mismatch", "discontinuous") and
        point["configuration_status"] in ("verified", "unknown", "unavailable"), "comparison_qualification_status")
    _require(type(point["publication_quality_flags"]) is int and point["publication_quality_flags"] >= 0 and
        not point["publication_quality_flags"] & ~ROLLUP_QUALITY_MASK,
        "comparison_point_quality")
    canonical = point["canonical_battle"]
    _require(type(canonical) in (tuple, list) and len(canonical) == 3 and
        all(type(value) is int and 0 < value <= battle.UINT64_MAX for value in canonical) and
        tuple(canonical[:2]) == tuple(raw[name] for name in builds.BATTLE[:2]) and
        canonical[2] <= raw["bctx_battle_seq"], "comparison_canonical_battle")
    qualified = point_context_verified(point)
    if "point_context_verified" in point:
        _require(type(point["point_context_verified"]) is bool and point["point_context_verified"] == qualified,
            "comparison_qualification_conflict")
    dimensions = {}

    def family(name, fields, available, *, status="observed"):
        present = raw["bctx_status"] == 1 and bool(raw["bctx_available"] & available)
        values = {field: (raw[field].hex() if type(raw[field]) is bytes else raw[field]) for field in fields} if present else None
        dimensions[name] = {"status": status if present else "unavailable", "values": values,
            "usable_for_matching": qualified and present and status == "observed"}

    family("level", ("bctx_level",), 1)
    primary, secondary = raw["bctx_primary_class_mask"], raw["bctx_secondary_class_mask"]
    class_status = "observed" if primary and not (primary | secondary) & ~CLASS_MASK else "unclassified"
    family("classes", ("bctx_primary_class_mask", "bctx_secondary_class_mask"), 1, status=class_status)
    spec = raw["bctx_specialization"]
    single_class = primary and primary.bit_count() == 1 and not primary & ~CLASS_MASK
    classified_spec = class_status == "observed" and (spec == 0 or single_class and spec in SPECIALIZATIONS[primary.bit_length()])
    family("specialization", ("bctx_primary_class_mask", "bctx_specialization"), 1,
        status="observed" if classified_spec else "unclassified")
    for name in ("race", "faction"):
        family(name, ("bctx_" + name,), 1)
    family("base_setup", builds.BASE_FIELDS, 1)
    family("learned_epics", builds.EPIC_FIELDS, 64)
    family("equipment", builds.EQUIPMENT_FIELDS, 32)
    family("effective_setup", builds.EFFECTIVE_FIELDS, 2)
    family("current_resources", builds.RESOURCE_FIELDS, 4)
    family("saving_modifiers", builds.SAVE_FIELDS, 8)
    family("effective_flags", builds.FLAG_FIELDS, 16)
    family("listed_effects", builds.AFFECT_FIELDS, 128, status="observed" if raw["bctx_affects_complete"] else "partial")
    # Counts/flags and base-effective differences establish neither temporary
    # duration nor caster provenance. Effective support has separate exact
    # native relationship/contribution facts, never a guessed buff origin.
    for name in ("temporary_effect_origin", "support_origin"):
        dimensions[name] = {"status": "unclassified" if raw["bctx_status"] == 1 else "unavailable",
            "values": None, "usable_for_matching": False}
    family("arena_room", ("bctx_arena_room",), 256)
    family("arena_roster", ("bctx_arena_membership", "bctx_arena_enabled", "bctx_arena_type",
        "bctx_arena_stage", "bctx_arena_team", "bctx_arena_player_flags"), 512,
        status="unclassified" if raw["bctx_arena_membership"] == 3 else "observed")
    _require(tuple(dimensions) == DIMENSIONS, "comparison_dimension_contract")
    return {"classification_version": CLASSIFICATION_VERSION, "catalog_digest": CATALOG_DIGEST,
        "point_key": list(builds.observation_key(raw)), "source_battle": [raw[name] for name in builds.BATTLE],
        "canonical_battle": list(point["canonical_battle"]),
        "actor": [raw["bctx_actor_id"], raw["bctx_actor_kind"]],
        "association": [raw["bctx_association_revision"], raw["bctx_association_fact_sequence"]],
        "scope": [raw["bctx_" + name] for name in ("environment_id", "season_id", "config_id", "build_version", "content_version")],
        "at_monotonic_usec": raw["bctx_at_monotonic_usec"], "at_utc_usec": raw["bctx_at_utc_usec"],
        "point_context_verified": qualified, "link_status": point["link_status"],
        "point_clock_status": point["point_clock_status"], "configuration_status": point["configuration_status"],
        "publication_quality_flags": point["publication_quality_flags"], "dimensions": dimensions,
        "level_is_combat_strength": False, "continuous_build_exposure_implied": False,
        "applied_equipment_effects_implied": False, "complete_intrinsic_setup_implied": False,
        "contributions_attributed_to_builds": False}


def compare_points(left: Mapping, right: Mapping, dimensions: Sequence[str]) -> dict:
    """Match only explicitly selected observed dimensions within one config.

    Reclassify source points rather than trusting a supplied classification.
    Equal values describe the selected recorded fields. They do not establish
    equal combat strength or carry validity between the two observation times.
    """
    _require(type(dimensions) in (tuple, list) and 0 < len(dimensions) <= len(DIMENSIONS) and
        all(type(name) is str and name in DIMENSIONS for name in dimensions) and
        len(set(dimensions)) == len(dimensions), "comparison_dimension_selection")
    first, second = classify_point(left), classify_point(right)
    matched, different, unknown = [], [], []
    same_configuration = first["scope"] == second["scope"]
    for name in dimensions:
        a, b = first["dimensions"][name], second["dimensions"][name]
        if not same_configuration or not a["usable_for_matching"] or not b["usable_for_matching"]:
            unknown.append(name)
        elif a["values"] == b["values"]:
            matched.append(name)
        else:
            different.append(name)
    return {"classification_version": CLASSIFICATION_VERSION, "catalog_digest": CATALOG_DIGEST,
        "left_point_key": first["point_key"], "right_point_key": second["point_key"],
        "selected_dimensions": list(dimensions), "matched_dimensions": matched,
        "different_dimensions": different, "unknown_dimensions": unknown,
        "configuration_matches": same_configuration, "selected_observed_dimensions_match": not different and not unknown,
        "combat_strength_equivalence_implied": False, "continuous_build_exposure_implied": False}
