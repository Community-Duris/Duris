"""Definition-1 sealed shared-battle contribution segments.

Intrinsic validation preserves exact fields and association references. It does
not establish complete association packets, collector coverage, or publication.
Kind-11 SQL rows preserve the transport receipt and the domain segment key.
"""
from __future__ import annotations

from collections.abc import Mapping

try:
    from . import battle_contract as battle
except ImportError:
    import battle_contract as battle

DEFINITION_VERSION = 1
UTC_UNKNOWN = -(1 << 63)
METRICS = 31
QUALITY_CONTEXT_UNKNOWN = 1
MODIFIER_CONTROL = 1 << 5

# Immutable order/width/sign contract, independently checked against the native
# descriptor and actual pure-collector rows, including rejected sink attempts.
FIELD_LAYOUT = (
    ("bc_battle_boot_id", 8, False),
    ("bc_battle_process_id", 8, False),
    ("bc_battle_seq", 8, False),
    ("bc_environment_id", 8, False),
    ("bc_season_id", 8, False),
    ("bc_config_id", 8, False),
    ("bc_classifier_version", 4, False),
    ("bc_policy_version", 4, False),
    ("bc_scope_zone_vnum", 4, True),
    ("bc_scope_group_key", 8, False),
    ("bc_actor_id", 8, False),
    ("bc_actor_pid", 4, True),
    ("bc_actor_owner_subject_id", 8, False),
    ("bc_actor_kind", 1, False),
    ("bc_actor_power_band", 2, False),
    ("bc_actor_encounter_boot_id", 8, False),
    ("bc_actor_encounter_process_id", 8, False),
    ("bc_actor_encounter_seq", 8, False),
    ("bc_actor_session_boot_id", 8, False),
    ("bc_actor_session_process_id", 8, False),
    ("bc_actor_session_seq", 8, False),
    ("bc_actor_level_band", 2, False),
    ("bc_actor_class_id", 2, False),
    ("bc_actor_race_id", 2, False),
    ("bc_actor_faction_id", 2, False),
    ("bc_actor_zone_vnum", 4, True),
    ("bc_actor_group_size", 4, False),
    ("bc_actor_group_key", 8, False),
    ("bc_actor_group_revision", 2, False),
    ("bc_actor_context_version", 2, False),
    ("bc_actor_quality_flags", 4, False),
    ("bc_first_association_revision", 8, False),
    ("bc_first_association_fact_sequence", 4, False),
    ("bc_available_metrics", 4, False),
    ("bc_side_status", 1, False),
    ("bc_mode", 1, False),
    ("bc_actor_side", 1, False),
    ("bc_context_quality_flags", 4, False),
    ("bc_segment_seq", 8, False),
    ("bc_last_association_revision", 8, False),
    ("bc_last_association_fact_sequence", 4, False),
    ("bc_modifier_flags", 4, False),
    ("bc_start_monotonic_usec", 8, False),
    ("bc_start_utc_usec", 8, True),
    ("bc_observed_through_monotonic_usec", 8, False),
    ("bc_observed_through_utc_usec", 8, True),
    ("bc_decision_monotonic_usec", 8, False),
    ("bc_decision_utc_usec", 8, True),
    ("bc_damage_dealt", 8, False),
    ("bc_damage_taken", 8, False),
    ("bc_healing_attempted", 8, False),
    ("bc_effective_healing", 8, False),
    ("bc_overhealing", 8, False),
    ("bc_healing_received", 8, False),
    ("bc_control_applications", 8, False),
    ("bc_control_received", 8, False),
    ("bc_casting_attempts", 8, False),
    ("bc_casting_completions", 8, False),
    ("bc_casting_aborts", 8, False),
    ("bc_casting_unresolved", 8, False),
    ("bc_casting_elapsed_usec", 8, False),
    ("bc_engaged_target_usec", 8, False),
    ("bc_quality_flags", 4, False),
    ("bc_definition_version", 2, False),
    ("bc_end_reason", 1, False),
)
FIELDS = tuple(name for name, _, _ in FIELD_LAYOUT)
WIRE_BYTES = sum(width for _, width, _ in FIELD_LAYOUT)
BATTLE = ("bc_battle_boot_id", "bc_battle_process_id", "bc_battle_seq")
COUNTER_FAMILIES = (
    (1, ("damage_dealt", "damage_taken")),
    (2, ("healing_attempted", "effective_healing", "overhealing", "healing_received")),
    (4, ("control_applications", "control_received")),
    (8, ("casting_attempts", "casting_completions", "casting_aborts", "casting_unresolved", "casting_elapsed_usec")),
    (16, ("engaged_target_usec",)),
)


class ContributionContractError(ValueError):
    pass


def _require(condition: bool, reason: str) -> None:
    if not condition:
        raise ContributionContractError(reason)


def segment_key(row: Mapping[str, int]) -> tuple[int, int, int]:
    """Domain identity, independent of actor/battle aliases and transport IDs."""
    return row["bc_battle_boot_id"], row["bc_battle_process_id"], row["bc_segment_seq"]


def validate_segment(row: Mapping[str, int]) -> dict[str, int]:
    _require(isinstance(row, Mapping) and set(row) == set(FIELDS), "exact contribution field set")
    for name, width, signed in FIELD_LAYOUT:
        value = row[name]
        limit = 1 << (width * 8 - int(signed))
        _require(type(value) is int and (-limit if signed else 0) <= value < limit,
                 "contribution width/type: " + name)
    v = dict(row)
    _require(all(v[name] > 0 for name in BATTLE) and v["bc_segment_seq"] > 0 and
             v["bc_definition_version"] == DEFINITION_VERSION and 1 <= v["bc_end_reason"] <= 4,
             "contribution identity/version/end")
    _require(all(v["bc_" + name] > 0 for name in (
        "environment_id", "season_id", "config_id", "classifier_version", "policy_version")) and
        v["bc_scope_zone_vnum"] == -1 and v["bc_scope_group_key"] == 0, "global contribution scope")
    first_revision, last_revision = v["bc_first_association_revision"], v["bc_last_association_revision"]
    first_fact, last_fact = v["bc_first_association_fact_sequence"], v["bc_last_association_fact_sequence"]
    _require(0 < first_revision <= last_revision and 0 < first_fact <= last_fact and
             (first_fact != last_fact or first_revision == last_revision), "contribution association references")
    mask, quality, context_quality, actor_quality = (v["bc_" + name] for name in (
        "available_metrics", "quality_flags", "context_quality_flags", "actor_quality_flags"))
    _require(mask & ~METRICS == 0 and quality & ~battle.QUALITY_KNOWN == 0 and
             context_quality & ~quality == 0 and actor_quality & ~context_quality == 0,
             "contribution availability/quality")
    status, side = v["bc_side_status"], v["bc_actor_side"]
    _require(1 <= status <= 3 and 0 <= v["bc_mode"] <= 3 and 0 <= side <= 2 and
             (side > 0 and context_quality & battle.INCOMPLETE_GRAPH == 0 if status == 1 else side == 0),
             "contribution observed side")
    actor, kind, pid, owner = (v["bc_actor_" + name] for name in ("id", "kind", "pid", "owner_subject_id"))
    _require(owner <= battle.INT32_MAX and
             (0 < actor <= battle.INT32_MAX and pid == actor and owner == actor if kind == 1 else
              kind in (2, 3) and actor & battle.GENERATION_TAG != 0 and actor & ~battle.GENERATION_TAG != 0 and
              pid == -1 and (owner > 0 if kind == 2 else owner == 0)), "contribution live actor/owner")
    _require(v["bc_actor_context_version"] == 1 and v["bc_actor_zone_vnum"] >= -1,
             "contribution native context")
    group = v["bc_actor_group_key"]
    _require((bool(group & ~battle.GENERATION_TAG) and v["bc_actor_group_revision"] > 0) if group & battle.GENERATION_TAG else
             v["bc_actor_group_revision"] == 0, "contribution formal group revision")
    for link in ("encounter", "session"):
        parts = tuple(v["bc_actor_" + link + "_" + part] for part in ("boot_id", "process_id", "seq"))
        _require(all(part == 0 for part in parts) or all(part > 0 for part in parts), "contribution optional linkage")
        if link == "encounter":
            _require(parts[2] == 0 or parts[:2] == tuple(v[name] for name in BATTLE[:2]), "contribution encounter producer")
        else:
            _require(kind == 1 or all(part == 0 for part in parts), "contribution logical session belongs to a PC")
    _require(v["bc_modifier_flags"] & ~((1 << 8) - 1) == 0, "contribution modifiers")
    for bit, names in COUNTER_FAMILIES:
        _require(mask & bit != 0 or all(v["bc_" + name] == 0 for name in names), "unavailable contribution family")
    attempted, effective, over = (v["bc_" + name] for name in ("healing_attempted", "effective_healing", "overhealing"))
    _require(effective <= attempted and over <= attempted and
             (quality & battle.QUALITY_CARDINALITY_OVERFLOW != 0 or over == attempted - effective),
             "contribution healing partition")
    attempts, completions, aborts, unresolved, elapsed = (v["bc_casting_" + name] for name in (
        "attempts", "completions", "aborts", "unresolved", "elapsed_usec"))
    _require(completions + aborts + unresolved == attempts and unresolved <= 1 and
             (attempts > 0 or elapsed == 0) and (unresolved == 0 or quality & battle.QUALITY_UNCLOSED_TAIL != 0),
             "contribution casting partition")
    _require(v["bc_control_applications"] == 0 or v["bc_modifier_flags"] & MODIFIER_CONTROL != 0,
             "contribution control modifier")
    start, through, decision = (v["bc_" + name + "_monotonic_usec"] for name in ("start", "observed_through", "decision"))
    _require(start <= through <= decision and elapsed <= through - start and
             v["bc_engaged_target_usec"] <= through - start, "contribution observed clocks")
    start_utc, through_utc, decision_utc = (v["bc_" + name + "_utc_usec"] for name in ("start", "observed_through", "decision"))
    reversed_utc = ((start_utc != UTC_UNKNOWN and through_utc != UTC_UNKNOWN and through_utc < start_utc) or
                    (through_utc != UTC_UNKNOWN and decision_utc != UTC_UNKNOWN and decision_utc < through_utc))
    _require(not reversed_utc or quality & battle.QUALITY_CLOCK_DISCONTINUITY != 0, "contribution UTC reversal")
    gap_quality = QUALITY_CONTEXT_UNKNOWN | battle.QUALITY_QUEUE_DROP
    _require(v["bc_end_reason"] != 4 or quality & gap_quality == gap_quality, "contribution source gap evidence")
    return v


def encode_segment(row: Mapping[str, int]) -> bytes:
    value = validate_segment(row)
    return b"".join(value[name].to_bytes(width, "big", signed=signed) for name, width, signed in FIELD_LAYOUT)


def decode_segment(wire: bytes) -> dict[str, int]:
    _require(isinstance(wire, (bytes, bytearray)) and len(wire) == WIRE_BYTES, "exact contribution wire length")
    value, offset = {}, 0
    for name, width, signed in FIELD_LAYOUT:
        value[name] = int.from_bytes(wire[offset:offset + width], "big", signed=signed)
        offset += width
    return validate_segment(value)


def validate_raw_segment(row: Mapping[str, int]) -> dict[str, int]:
    """Strict kind-11 storage boundary, independent of earlier combat families."""
    _require(isinstance(row, Mapping), "raw contribution mapping")
    _require(type(row.get("record_kind")) is int and row["record_kind"] == 11, "raw contribution tag")
    _require(type(row.get("schema_version")) is int and row["schema_version"] == 1, "raw contribution schema")
    value = validate_segment({name: row.get(name) for name in FIELDS})
    for name in ("boot_id", "process_id", "record_seq"):
        _require(type(row.get(name)) is int and 0 < row[name] < (1 << 64), "raw contribution replay identity")
    _require((row["boot_id"], row["process_id"]) == segment_key(value)[:2] and
             type(row.get("occurrence_utc_usec")) is int and row["occurrence_utc_usec"] == value["bc_decision_utc_usec"],
             "raw contribution producer/occurrence binding")
    header = {"ingest_id", "boot_id", "process_id", "record_seq", "schema_version",
              "record_kind", "occurrence_utc_usec", "ingested_utc_usec"}
    _require(all(item is None for name, item in row.items() if name not in FIELDS and name not in header),
             "raw contribution inactive family payload")
    return value
