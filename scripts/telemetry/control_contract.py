"""Typed reviewed control operations and disjoint target-state evidence.

Intrinsic validation establishes no verified battle link, continuous source
coverage, caster credit for elapsed time, or whole-battle result.
"""
from __future__ import annotations

from collections.abc import Mapping

try:
    from . import battle_contract as battle
except ImportError:
    import battle_contract as battle

DEFINITION_VERSION = 1
PRODUCER_VERSION = 1
STATE_MASK = 255
FLAGS = 63
UTC_UNKNOWN = -(1 << 63)
FIELD_LAYOUT = (
    ('ctl_boot_id', 8, False),
    ('ctl_process_id', 8, False),
    ('ctl_sequence', 8, False),
    ('ctl_previous_state_sequence', 8, False),
    ('ctl_environment_id', 8, False),
    ('ctl_season_id', 8, False),
    ('ctl_config_id', 8, False),
    ('ctl_classifier_version', 4, False),
    ('ctl_policy_version', 4, False),
    ('ctl_scope_zone_vnum', 4, True),
    ('ctl_scope_group_key', 8, False),
    ('ctl_build_version', 4, False),
    ('ctl_content_version', 4, False),
    ('ctl_source_actor_id', 8, False),
    ('ctl_source_actor_pid', 4, True),
    ('ctl_source_owner_subject_id', 8, False),
    ('ctl_source_actor_kind', 1, False),
    ('ctl_source_power_band', 2, False),
    ('ctl_source_session_boot_id', 8, False),
    ('ctl_source_session_process_id', 8, False),
    ('ctl_source_session_seq', 8, False),
    ('ctl_source_level_band', 2, False),
    ('ctl_source_class_id', 2, False),
    ('ctl_source_race_id', 2, False),
    ('ctl_source_faction_id', 2, False),
    ('ctl_source_zone_vnum', 4, True),
    ('ctl_source_group_size', 4, False),
    ('ctl_source_group_key', 8, False),
    ('ctl_source_group_revision', 2, False),
    ('ctl_source_context_version', 2, False),
    ('ctl_source_quality_flags', 4, False),
    ('ctl_source_battle_seq', 8, False),
    ('ctl_source_association_revision', 8, False),
    ('ctl_source_association_fact_sequence', 4, False),
    ('ctl_target_actor_id', 8, False),
    ('ctl_target_actor_pid', 4, True),
    ('ctl_target_owner_subject_id', 8, False),
    ('ctl_target_actor_kind', 1, False),
    ('ctl_target_power_band', 2, False),
    ('ctl_target_session_boot_id', 8, False),
    ('ctl_target_session_process_id', 8, False),
    ('ctl_target_session_seq', 8, False),
    ('ctl_target_level_band', 2, False),
    ('ctl_target_class_id', 2, False),
    ('ctl_target_race_id', 2, False),
    ('ctl_target_faction_id', 2, False),
    ('ctl_target_zone_vnum', 4, True),
    ('ctl_target_group_size', 4, False),
    ('ctl_target_group_key', 8, False),
    ('ctl_target_group_revision', 2, False),
    ('ctl_target_context_version', 2, False),
    ('ctl_target_quality_flags', 4, False),
    ('ctl_target_battle_seq', 8, False),
    ('ctl_target_association_revision', 8, False),
    ('ctl_target_association_fact_sequence', 4, False),
    ('ctl_last_target_association_revision', 8, False),
    ('ctl_last_target_association_fact_sequence', 4, False),
    ('ctl_start_usec', 8, False),
    ('ctl_start_utc_usec', 8, True),
    ('ctl_at_usec', 8, False),
    ('ctl_at_utc_usec', 8, True),
    ('ctl_decision_usec', 8, False),
    ('ctl_decision_utc_usec', 8, True),
    ('ctl_quality_flags', 4, False),
    ('ctl_configured_ticks', 4, True),
    ('ctl_definition_version', 2, False),
    ('ctl_producer_version', 2, False),
    ('ctl_flags', 2, False),
    ('ctl_before_mask', 2, False),
    ('ctl_after_mask', 2, False),
    ('ctl_state_available', 2, False),
    ('ctl_duration_coverage', 2, False),
    ('ctl_kind', 1, False),
    ('ctl_family', 1, False),
    ('ctl_result', 1, False),
    ('ctl_boundary', 1, False),
)
FIELDS = tuple(name for name, _, _ in FIELD_LAYOUT)
WIRE_BYTES = sum(width for _, width, _ in FIELD_LAYOUT)


class ControlContractError(ValueError):
    """Invalid control evidence, without source payloads in diagnostics."""


def _require(condition, reason):
    if not condition:
        raise ControlContractError(reason)


def _actor_valid(v, prefix):
    def value(name):
        return v["ctl_" + prefix + "_" + name]
    actor, pid, owner, kind = (value(name) for name in
        ("actor_id", "actor_pid", "owner_subject_id", "actor_kind"))
    if kind == 1:
        valid = 0 < actor == pid <= battle.INT32_MAX and owner == actor
    else:
        valid = (kind in (2, 3) and actor & battle.GENERATION_TAG != 0 and
                 actor & ~battle.GENERATION_TAG != 0 and pid == -1 and
                 (0 < owner <= battle.INT32_MAX if kind == 2 else owner == 0))
    group, revision = value("group_key"), value("group_revision")
    valid = valid and (bool(group & ~battle.GENERATION_TAG) and revision > 0
                      if group & battle.GENERATION_TAG else revision == 0)
    session = tuple(value(name) for name in ("session_boot_id", "session_process_id", "session_seq"))
    return (valid and value("zone_vnum") >= -1 and value("context_version") == 1 and
            value("quality_flags") & ~battle.QUALITY_KNOWN == 0 and
            (session == (0, 0, 0) or kind == 1 and all(session)))


def _association(v, prefix):
    return tuple(v["ctl_" + prefix + "_" + name] for name in
                 ("battle_seq", "association_revision", "association_fact_sequence"))


def observation_key(v):
    return v["ctl_boot_id"], v["ctl_process_id"], v["ctl_sequence"]


def actor_context(v, prefix):
    """Name the captured actor fields for the shared association/identity reader."""
    _require(prefix in ("source", "target"), "control actor prefix")
    def field(name):
        suffix = name.removeprefix("battle_actor_")
        return "ctl_" + prefix + "_" + ("actor_" if suffix in ("id", "pid", "kind") else "") + suffix
    # The compact control contract has no encounter identifiers. Their absence
    # cannot be filled from a later roster or interpreted as a captured zero.
    return {name: v[field(name)] for name in battle.ACTOR_VALUES if field(name) in v}


def validate_observation(row: Mapping) -> dict:
    _require(isinstance(row, Mapping) and set(row) == set(FIELDS), "exact control field set")
    for name, width, signed in FIELD_LAYOUT:
        value = row[name]
        limit = 1 << (width * 8 - int(signed))
        _require(type(value) is int and (-limit if signed else 0) <= value < limit,
                 "control width/type: " + name)
    v = dict(row)
    def value(name):
        return v["ctl_" + name]
    versions = tuple(value(name) for name in ("config_id", "classifier_version", "policy_version",
        "build_version", "content_version"))
    configuration_unknown = value("kind") == 4 and value("boundary") == 8
    _require(all(value(name) > 0 for name in ("boot_id", "process_id", "sequence",
        "environment_id", "season_id")) and
        (all(item == 0 for item in versions) if configuration_unknown else all(versions)) and
        value("scope_zone_vnum") == -1 and
        value("scope_group_key") == 0 and value("definition_version") == DEFINITION_VERSION and
        value("producer_version") == PRODUCER_VERSION, "control scope/version/identity")
    _require(_actor_valid(v, "target"), "control target lifetime/context")
    for prefix in ("source", "target"):
        reference = _association(v, prefix)
        _require(reference == (0, 0, 0) or all(reference), "control complete optional association")
    target = _association(v, "target")
    last_reference = (value("last_target_association_revision"), value("last_target_association_fact_sequence"))
    _require(last_reference == (0, 0) if target == (0, 0, 0) else
        last_reference[0] >= target[1] and last_reference[1] >= target[2] and
        (last_reference[1] != target[2] or last_reference[0] == target[1]),
        "control ordered target association prefix")
    quality, flags = value("quality_flags"), value("flags")
    _require(quality & ~battle.QUALITY_KNOWN == 0 and
        (value("source_quality_flags") | value("target_quality_flags")) & ~quality == 0 and
        flags & ~FLAGS == 0 and
        all(value(name) & ~STATE_MASK == 0 for name in
            ("before_mask", "after_mask", "state_available", "duration_coverage")),
        "control quality/flags/state mask")
    start, end, decision = (value(name + "_usec") for name in ("start", "at", "decision"))
    clocks = tuple(value(name + "_utc_usec") for name in ("start", "at", "decision"))
    _require(start <= end <= decision and value("previous_state_sequence") < value("sequence") and
             1 <= value("boundary") <= 11, "control ordered clocks/chain/boundary")
    if any(a != UTC_UNKNOWN and b != UTC_UNKNOWN and b < a for a, b in zip(clocks, clocks[1:])):
        _require(quality & 128 != 0, "control explicit UTC discontinuity")
    kind, family, result = (value(name) for name in ("kind", "family", "result"))
    if kind == 1:
        self_effect = (value("source_actor_id") == value("target_actor_id") and
                       value("source_actor_kind") == value("target_actor_kind"))
        _require(_actor_valid(v, "source") and value("previous_state_sequence") == 0 and
            value("boundary") == 11 and start == end == decision and clocks[0] == clocks[1] == clocks[2] and
            1 <= family <= 8 and 1 <= result <= 14 and value("state_available") == STATE_MASK and
            last_reference == target[1:] and
            value("duration_coverage") == 0 and bool(flags & 32) == self_effect,
            "control operation shape")
        if result != 1:
            _require(value("configured_ticks") == 0 and flags & (4 | 8 | 16) == 0,
                     "rejected control has no accepted declaration")
        else:
            effect = 136 if family == 8 else 1 << (family - 1)
            _require(value("after_mask") & effect != 0 and
                (not flags & 8 or family == 6 and value("before_mask") & 32 != 0) and
                (not flags & 16 or family == 2) and (not flags & 4 or family in (2, 3, 6)),
                "accepted control effect/bypass/refresh")
        return v
    source_fields = [name for name in FIELDS if name.startswith("ctl_source_")]
    _require(all(v[name] == 0 for name in source_fields) and family == result == flags == 0 and
        value("configured_ticks") == 0 and value("boundary") != 11,
        "target interval has no guessed source operation")
    if kind == 4:
        _require(value("state_available") == value("duration_coverage") ==
            value("before_mask") == value("after_mask") == 0 and quality & 1 != 0 and
            value("boundary") >= 7, "control gap clears preceding state")
        return v
    _require(value("state_available") == STATE_MASK and
        (value("duration_coverage") == STATE_MASK or quality & 1 != 0) and
        any(_association(v, "target")), "control interval source availability")
    if kind == 2:
        _require(value("previous_state_sequence") == 0 and start == end == decision and
            clocks[0] == clocks[1] == clocks[2] and value("before_mask") == value("after_mask") and
            last_reference == target[1:] and
            value("boundary") in (1, 4), "control state entry")
    else:
        _require(kind == 3 and value("previous_state_sequence") != 0 and
            2 <= value("boundary") <= 10 and (end == decision or quality & 64 != 0),
            "control measured prefix/censored decision")
    return v


def encode_observation(row: Mapping) -> bytes:
    v = validate_observation(row)
    return b"".join(v[name].to_bytes(width, "big", signed=signed)
                    for name, width, signed in FIELD_LAYOUT)


def decode_observation(data: bytes) -> dict:
    _require(type(data) is bytes and len(data) == WIRE_BYTES, "exact control wire length/type")
    position, value = 0, {}
    for name, width, signed in FIELD_LAYOUT:
        value[name] = int.from_bytes(data[position:position + width], "big", signed=signed)
        position += width
    return validate_observation(value)


def validate_raw_observation(row: Mapping) -> dict:
    """Exact kind-13 transport binding; links and source continuity stay separate."""
    _require(isinstance(row, Mapping), "raw control mapping")
    _require(type(row.get("record_kind")) is int and row["record_kind"] == 13, "raw control tag")
    _require(type(row.get("schema_version")) is int and row["schema_version"] == 1, "raw control schema")
    value = validate_observation({name: row.get(name) for name in FIELDS})
    for name in ("boot_id", "process_id", "record_seq"):
        _require(type(row.get(name)) is int and 0 < row[name] < (1 << 64), "raw control replay identity")
    _require((row["boot_id"], row["process_id"]) == observation_key(value)[:2] and
             type(row.get("occurrence_utc_usec")) is int and
             row["occurrence_utc_usec"] == value["ctl_decision_utc_usec"],
             "raw control producer/occurrence binding")
    header = {"ingest_id", "boot_id", "process_id", "record_seq", "schema_version",
              "record_kind", "occurrence_utc_usec", "ingested_utc_usec"}
    _require(all(item is None for name, item in row.items() if name not in FIELDS and name not in header),
             "raw control inactive family payload")
    return value
