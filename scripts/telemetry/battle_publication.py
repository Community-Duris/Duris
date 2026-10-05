"""Bounded public battle observations with exact dated identity attribution.

The caller verifies its source cursor, reserved identity review and independent
schema-4 incident snapshot in the publication transaction. Presence can be split
at observed boundaries. Metric amounts are never apportioned across a boundary
whose individual actions were not captured.
"""
from __future__ import annotations

from collections import defaultdict
from dataclasses import dataclass, replace
from datetime import date
import hashlib
import json
import re
from typing import Any, Callable, Mapping

try:
    from . import battle_source as source, battle_history as history, battle_contract as battle
    from . import battle_contribution_contract as contribution, battle_build_contract as builds, control_contract as controls, battle_result_contract as results, identity_history as identity
    from . import identity_publication, observation_semantics as observations, incident, battle_comparison as comparison
    from .rollup_definitions import ROLLUP_QUALITY_MASK, ROLLUP_QUALITY_UTC_FANOUT, UNKNOWN_DAY
except ImportError:
    import battle_source as source
    import battle_history as history
    import battle_contract as battle
    import battle_contribution_contract as contribution
    import battle_build_contract as builds
    import control_contract as controls
    import battle_result_contract as results
    import identity_history as identity
    import identity_publication
    import observation_semantics as observations
    import incident
    import battle_comparison as comparison
    from rollup_definitions import ROLLUP_QUALITY_MASK, ROLLUP_QUALITY_UTC_FANOUT, UNKNOWN_DAY

SCOPE = source.SCOPE
MAX_OUTPUT_ROWS = 4_096
MAX_PAYLOAD_BYTES = 8_192
ROW_FETCH_BYTE_BOUND = 12_288
ROW_VALUE_BYTE_BOUND = 32_768
SNAPSHOT_ROW_BYTE_BOUND = 512
REVIEW_BYTE_BOUND = 2 * identity.MAX_PACKET_BYTES + 2 * incident.MAX_PACKET_BYTES + (
    incident.MAX_INCIDENTS + 1) * incident.INCIDENT_ROW_BYTE_BOUND
REPORT_METADATA_BYTE_BOUND = 3 * source.HEADER_BYTE_BOUND + 2 * incident.MAX_PACKET_BYTES + (
    incident.MAX_INCIDENTS + 1) * incident.INCIDENT_ROW_BYTE_BOUND + (MAX_OUTPUT_ROWS + 1) * SNAPSHOT_ROW_BYTE_BOUND
ROW_COLUMNS = (*SCOPE, "row_kind", "row_key", "payload", "payload_digest", "quality_flags")
ROW_KINDS = {"battle_observations": 1, "battle_actors": 2, "battle_contributions": 3, "battle_exposure": 4, "battle_associations": 5}
BUILD_ROW_KINDS = {**ROW_KINDS, "battle_build_points": 6}
CONTROL_ROW_KINDS = {**BUILD_ROW_KINDS, "battle_control_operations": 7, "battle_control_states": 8}
RESULT_ROW_KINDS = {**CONTROL_ROW_KINDS, "battle_build_comparisons": 6, "battle_outcomes": 9}
COUNT_FIELDS = ("battle_row_count", "actor_row_count", "contribution_row_count", "exposure_row_count", "association_row_count")
COVERAGE_COLUMNS = (*source.HEADER_COLUMNS, "snapshot_digest", *COUNT_FIELDS,
    "complete_packet_count", "incomplete_packet_count", "alias_count", "canonical_battle_count",
    "verified_contribution_links", "partial_contribution_links", "observed_present_usec",
    "observed_pc_present_usec", "non_pc_present_usec", "owned_pc_present_usec",
    "unknown_account_pc_present_usec", "confirmed_controller_pc_present_usec", "unlinked_controller_pc_present_usec")
BATTLE_FIELDS = ("battle", "canonical_battle", "canonical", "start_seen", "close_seen", "retired_by_alias",
    "packet_history_complete", "observed_graph_verified", "source_fact_count", "packet_count",
    "incomplete_packet_count", "last_revision", "last_fact_sequence", "declared_actor_count", "observed_actor_count",
    "close_reason", "end_censored", "observed_through_monotonic_usec", "decision_monotonic_usec",
    "contribution_count", "available_metric_mask", *(name for _bit, names in contribution.COUNTER_FAMILIES for name in names),
    "quality_flags", "outcome", "complete_metric_coverage_implied", "account_or_controller_identity_implied")
ACTOR_FIELDS = ("canonical_battle", *history.ACTOR_CONTEXT, *battle.EFFORT,
    "active_at_last_observation", "roles", "effort_replay_verified", "quality_flags")
IDENTITY_FIELDS = ("registry_version", "account_token", "controller_token", "association_id", "linkage_status")
CONTRIBUTION_FIELDS = (*history.HEADER, *contribution.FIELDS, "canonical_battle", "link_status",
    "publication_quality_flags", "complete_metric_coverage_implied", *IDENTITY_FIELDS,
    "attribution_status", "attribution_quality_flags")
EXPOSURE_FIELDS = ("source_battle", "start_association", "through_association", "start_record_seq", "through_record_seq",
    *history.SCOPE, *history.ACTOR_CONTEXT, "battle_actor_roles", "battle_actor_side", "battle_mode", "battle_side_status",
    "start_monotonic_usec", "observed_through_monotonic_usec", "start_utc_usec", "observed_through_utc_usec",
    "observed_side_owners", "observed_opposing_owners", "observed_roster_digest", *battle.EFFORT, "quality_flags",
    "history_verified", "complete_population_coverage_implied", "canonical_battle", "utc_day", *IDENTITY_FIELDS)
ASSOCIATION_FIELDS = (*source.SOURCE_COLUMNS[10], "projection_quality_flags")
VALUE_FIELDS = {1: BATTLE_FIELDS, 2: ACTOR_FIELDS, 3: CONTRIBUTION_FIELDS, 4: EXPOSURE_FIELDS, 5: ASSOCIATION_FIELDS}
BUILD_FIELDS = (*source.SOURCE_COLUMNS[12], "canonical_battle", "link_status", "point_clock_status",
    "configuration_status", "publication_quality_flags", "point_context_verified", "continuous_build_exposure_implied")
CONTROL_IDENTITY_FIELDS = ("account_token", "controller_token", "association_id", "linkage_status", "attribution_status", "attribution_quality_flags")
CONTROL_FIELDS = (*source.SOURCE_COLUMNS[13], "source_canonical_battle", "target_canonical_battle",
    "source_link_status", "target_link_status", "chain_status", "clock_status", "configuration_status",
    "publication_quality_flags", "observed_prefix_usec", "accepted_application_count", "qualified_duration_mask",
    "qualified_status_usec", "proven_action_restriction_usec", "caster_attributed_duration_usec", "registry_version",
    *(prefix + "_" + name for prefix in ("source", "target") for name in CONTROL_IDENTITY_FIELDS))
CONTROL_COVERAGE_FIELDS = ("control_operation_count", "control_state_count", "control_accepted_applications",
    "control_entry_count", "control_interval_count", "control_gap_count", "verified_control_target_links",
    "partial_control_target_links", "verified_control_chains", "qualified_control_prefixes", "configuration_unknown_control_points")
RESULT_KINDS = ("death", "flee_movement", "withdrawal", "escape", "objective_request", "objective_commit", "unresolved", "censored")
RESULT_COVERAGE_FIELDS = ("result_row_count", *("observed_" + name + "_count" for name in RESULT_KINDS),
    "qualified_result_evidence_count", "unknown_result_evidence_count", "qualified_result_context_count",
    "configuration_unknown_result_count", "recovered_objective_count", "duplicate_objective_count",
    "qualified_objective_commit_count", "comparison_observed_cells", "comparison_partial_cells",
    "comparison_unavailable_cells", "comparison_unclassified_cells", "comparison_matching_cells")
RESULT_FIELDS = (*source.SOURCE_COLUMNS[14], "source_canonical_battle", "target_canonical_battle",
    "source_link_status", "target_link_status", "clock_status", "chain_status", "objective_status",
    "parent_event_key", "reference_origin", "native_source_status", "observed_pre_roster",
    "publication_quality_flags", "event_quality_flags", "whole_battle_victory_implied", "full_zone_clear_implied",
    "complete_population_coverage_implied", "configuration_status", "event_evidence_qualified", "battle_context_qualified",
    "registry_version", *(prefix + "_" + name for prefix in ("source", "target") for name in CONTROL_IDENTITY_FIELDS))
_BOOLEANS = frozenset(("canonical", "start_seen", "close_seen", "retired_by_alias", "packet_history_complete",
    "observed_graph_verified", "end_censored", "active_at_last_observation", "effort_replay_verified",
    "history_verified", "complete_population_coverage_implied", "complete_metric_coverage_implied",
    "account_or_controller_identity_implied"))
_REFERENCES = {"battle": 3, "canonical_battle": 3, "source_battle": 3, "start_association": 5, "through_association": 5}


def row_kinds(scope):
    return RESULT_ROW_KINDS if scope[0] == source.RESULT_DEFINITION_VERSION else CONTROL_ROW_KINDS if scope[0] == source.CONTROL_DEFINITION_VERSION else BUILD_ROW_KINDS if scope[0] == source.BUILD_DEFINITION_VERSION else ROW_KINDS


def value_fields(scope):
    if scope[0] == source.RESULT_DEFINITION_VERSION:
        return {**VALUE_FIELDS, 6: (*BUILD_FIELDS, "comparison"), 7: CONTROL_FIELDS, 8: CONTROL_FIELDS, 9: RESULT_FIELDS}
    return {**VALUE_FIELDS, 6: BUILD_FIELDS, 7: CONTROL_FIELDS, 8: CONTROL_FIELDS} if scope[0] == source.CONTROL_DEFINITION_VERSION else {
        **VALUE_FIELDS, 6: BUILD_FIELDS} if scope[0] == source.BUILD_DEFINITION_VERSION else VALUE_FIELDS


def count_fields(scope):
    if scope[0] == source.RESULT_DEFINITION_VERSION:
        return (*COUNT_FIELDS, "build_row_count", "control_operation_count", "control_state_count", "result_row_count")
    return (*COUNT_FIELDS, "build_row_count", "control_operation_count", "control_state_count") if scope[0] == source.CONTROL_DEFINITION_VERSION else (
        *COUNT_FIELDS, "build_row_count") if scope[0] == source.BUILD_DEFINITION_VERSION else COUNT_FIELDS


def coverage_columns(scope):
    if scope[0] in source.BUILD_DEFINITION_VERSIONS:
        return (*source.header_columns(scope), *COVERAGE_COLUMNS[len(source.HEADER_COLUMNS):],
            "build_row_count", "verified_build_links", "partial_build_links", "qualified_build_points",
            "unavailable_build_points", "configuration_unknown_build_points", *(
                CONTROL_COVERAGE_FIELDS if scope[0] in source.CONTROL_DEFINITION_VERSIONS else ()), *(
                RESULT_COVERAGE_FIELDS if scope[0] == source.RESULT_DEFINITION_VERSION else ()))
    return COVERAGE_COLUMNS


class PublicationError(ValueError):
    """Payload-free publication conflict or bounded capacity refusal."""


def _require(condition, reason):
    if not condition:
        raise PublicationError(reason)


def _canonical(value):
    return json.dumps(value, sort_keys=True, separators=(",", ":"), allow_nan=False).encode("utf-8")


def _logical_key(kind, value):
    if kind == 1:
        return value["battle"]
    if kind == 2:
        return (*value["canonical_battle"], value["battle_actor_id"])
    if kind == 3:
        return contribution.segment_key(value)
    if kind == 5:
        return battle.fact_key(value)
    if kind == 6:
        return builds.observation_key(value)
    if kind in (7, 8):
        return controls.observation_key(value)
    if kind == 9:
        return results.observation_key(value)
    return (*value["source_battle"], value["battle_actor_id"],
        value["start_monotonic_usec"], value["observed_through_monotonic_usec"])


def _key(scope, kind, value):
    return hashlib.sha256(b"duris-battle-public-row-v1:" + _canonical([scope, kind, _logical_key(kind, value)])).digest()


def snapshot_digest(scope, rows, *, check_deadline=lambda: None):
    """Verify a bounded, sorted receipt list without fetching public payloads."""
    source._scope(scope)
    _require(len(rows) <= MAX_OUTPUT_ROWS, "battle_public_snapshot_capacity")
    folded = hashlib.sha256(b"duris-battle-public-snapshot-v1:" + _canonical(scope))
    previous = None
    for row in rows:
        check_deadline()
        kind, key, digest = row["row_kind"], row["row_key"], row["payload_digest"]
        _require(type(kind) is int and kind in value_fields(scope) and type(key) is bytes and len(key) == 32 and
            type(digest) is bytes and len(digest) == 32, "battle_public_snapshot_receipt")
        current = kind, key
        _require(previous is None or previous < current, "battle_public_snapshot_order")
        previous = current
        folded.update(bytes((kind,)) + key + digest)
    return folded.digest()


def _quality(value):
    main, attribution = value.get("quality_flags", value.get("publication_quality_flags", value.get("projection_quality_flags", 0))), value.get("attribution_quality_flags", 0)
    if "ctl_sequence" in value or "bout_sequence" in value:
        attribution |= value["source_attribution_quality_flags"] | value["target_attribution_quality_flags"]
    _require(type(main) is int and type(attribution) is int and main >= 0 and attribution >= 0 and
        (main | attribution) & ~ROLLUP_QUALITY_MASK == 0, "battle_public_quality")
    return main | attribution


def _validate_value(kind, value):
    if kind == 9:
        _validate_result_value(value)
        return
    if kind in (7, 8):
        _validate_control_value(kind, value)
        return
    if kind == 6:
        _require(type(value) is dict and set(value) in (set(BUILD_FIELDS), set((*BUILD_FIELDS, "comparison"))), "battle_public_build_fields")
        raw = {name: value[name] for name in source.SOURCE_COLUMNS[12]}
        for name in builds.BYTE_FIELDS:
            item = raw[name]
            _require(type(item) is str and re.fullmatch("[0-9a-f]{64}", item) is not None, "battle_public_build_digest")
            raw[name] = bytes.fromhex(item)
        builds.validate_raw_observation(raw)
        _require(type(raw["ingest_id"]) is int and raw["ingest_id"] > 0 and type(raw["ingested_utc_usec"]) is int and
            -(1 << 63) <= raw["ingested_utc_usec"] < (1 << 63), "battle_public_build_arrival")
        root = value["canonical_battle"]
        _require(isinstance(root, (tuple, list)) and len(root) == 3 and all(type(item) is int and 0 < item <= battle.UINT64_MAX for item in root),
            "battle_public_build_reference")
        _require(tuple(root[:2]) == builds.observation_key(raw)[:2] and
            (value["configuration_status"] == "unavailable") == (raw["bctx_config_id"] == 0), "battle_public_build_scope")
        _require(value["link_status"] in ("verified", "missing_packet", "partial_history", "stale_association", "outside_observed_prefix") and
            value["point_clock_status"] in ("verified", "unknown", "unverified", "mismatch", "discontinuous") and
            value["configuration_status"] in ("verified", "unknown", "unavailable"), "battle_public_build_status")
        _require(type(value["point_context_verified"]) is bool and value["continuous_build_exposure_implied"] is False and
            value["point_context_verified"] == _build_verified(value), "battle_public_build_qualification")
        _quality(value)
        if "comparison" in value:
            _require(value["comparison"] == _comparison_value(value), "battle_public_comparison_conflict")
        return
    _require(kind in VALUE_FIELDS and type(value) is dict and set(value) == set(VALUE_FIELDS[kind]), "battle_public_row_fields")
    for name, item in value.items():
        if name in _REFERENCES:
            _require(isinstance(item, (tuple, list)) and len(item) == _REFERENCES[name] and
                all(type(number) is int and 0 < number <= battle.UINT64_MAX for number in item), "battle_public_reference")
        elif name in _BOOLEANS:
            _require(type(item) is bool or name == "end_censored" and item is None, "battle_public_boolean")
        elif name == "observed_roster_digest":
            _require(type(item) is str and re.fullmatch("[0-9a-f]{64}", item) is not None, "battle_public_roster_digest")
        elif name == "utc_day":
            if item is not None:
                _require(type(item) is str, "battle_public_day")
                try:
                    valid = date.fromisoformat(item) > UNKNOWN_DAY and date.fromisoformat(item).isoformat() == item
                except ValueError:
                    valid = False
                _require(valid, "battle_public_day")
        elif name in ("link_status", "linkage_status", "attribution_status"):
            _require(type(item) is str and item in {
                "verified", "missing_packet", "partial_history", "missing_lifecycle",
                "unknown_account", "clock_unknown", "confirmed", "unknown", "outside_reviewed_window",
                "no_reviewed_mapping", "not_player_actor", "unproven_owner_identity", "missing_session",
                "uniform_observed_identity", "identity_changes_inside_segment", "unverified_association", "unobserved_point_identity",
                "mixed_review_linkage", "confirmed_across_associations", "identity_unavailable"},
                "battle_public_status")
        else:
            _require(item is None or type(item) is int and -(1 << 63) <= item <= battle.UINT64_MAX, "battle_public_scalar")
    _require(value.get("outcome") is None and not value.get("complete_metric_coverage_implied", False) and
        not value.get("complete_population_coverage_implied", False) and not value.get("account_or_controller_identity_implied", False),
        "battle_public_unproven_coverage")
    flags = _quality(value)
    _require(type(flags) is int and flags >= 0 and flags & ~ROLLUP_QUALITY_MASK == 0, "battle_public_quality")
    if kind == 3:
        native = {name: value[name] for name in contribution.FIELDS}
        for bit, names in contribution.COUNTER_FAMILIES:
            for name in names:
                field = "bc_" + name
                if not native["bc_available_metrics"] & bit:
                    _require(native[field] is None, "battle_public_unavailable_metric")
                    native[field] = 0
        contribution.validate_segment(native)
    if kind == 5:
        battle.validate_raw_fact({name: value[name] for name in source.SOURCE_COLUMNS[10]})
    if kind in (3, 4):
        _require(value["controller_token"] is None or value["account_token"] is not None and value["registry_version"] is not None and
            (value["association_id"] is not None and value["linkage_status"] == "confirmed" or
             kind == 3 and value["association_id"] is None and value["linkage_status"] == "confirmed_across_associations"),
            "battle_public_controller_authority")
        for name in IDENTITY_FIELDS[:-1]:
            _require(value[name] is None or type(value[name]) is int and value[name] > 0, "battle_public_identity")
        if value["account_token"] is not None:
            prefix = "bc_actor_" if kind == 3 else "battle_actor_"
            _require(value[prefix + "kind"] == 1 and value[prefix + "pid"] == value[prefix + "id"] ==
                value[prefix + "owner_subject_id"] and all(value[prefix + name] for name in observations.SESSION),
                "battle_public_authenticated_actor")
    if kind == 4:
        duration = value["observed_through_monotonic_usec"] - value["start_monotonic_usec"]
        _require(duration > 0 and value["battle_present_usec"] == duration and
            all(value[name] in (0, duration) for name in battle.EFFORT) and
            sum(value["battle_" + mode + "_usec"] for mode in ("unknown_mode", "pve", "pvp", "mixed")) == duration,
            "battle_public_exposure_conservation")


def retain_row(scope, kind, value):
    source._scope(scope)
    _require(type(kind) is int and kind in value_fields(scope), "battle_public_row_kind")
    _require(type(value) is dict and set(value) == set(value_fields(scope)[kind]), "battle_public_row_fields")
    _validate_value(kind, value)
    if kind == 6:
        _require((value["bctx_environment_id"], value["bctx_season_id"]) == scope[2:], "battle_public_build_scope")
    if kind in (7, 8):
        _require((value["ctl_environment_id"], value["ctl_season_id"]) == scope[2:], "battle_public_control_scope")
    if kind == 9:
        _require((value["bout_environment_id"], value["bout_season_id"]) == scope[2:], "battle_public_result_scope")
    payload = _canonical(value)
    _require(len(payload) <= MAX_PAYLOAD_BYTES, "battle_public_payload_capacity")
    return dict(zip(SCOPE, scope, strict=True), row_kind=kind, row_key=_key(scope, kind, value), payload=payload,
        payload_digest=hashlib.sha256(payload).digest(), quality_flags=_quality(value))


def decode_row(scope, row):
    _require(isinstance(row, Mapping) and set(row) == set(ROW_COLUMNS) and
        tuple(row[name] for name in SCOPE) == scope and type(row["row_kind"]) is int and row["row_kind"] in value_fields(scope) and
        type(row["row_key"]) is bytes and len(row["row_key"]) == 32 and type(row["payload_digest"]) is bytes and
        len(row["payload_digest"]) == 32 and type(row["quality_flags"]) is int, "battle_public_stored_scope")
    payload = row["payload"]
    _require(type(payload) is bytes and 0 < len(payload) <= MAX_PAYLOAD_BYTES, "battle_public_payload_capacity")
    _require(hashlib.sha256(payload).digest() == row["payload_digest"], "battle_public_payload_digest")
    try:
        value = incident.decode_evidence_packet(payload, max_bytes=MAX_PAYLOAD_BYTES)
    except incident.IncidentError as error:
        raise PublicationError("battle_public_payload_json") from error
    _require(_canonical(value) == payload, "battle_public_payload_canonical")
    _require(type(value) is dict and set(value) == set(value_fields(scope)[row["row_kind"]]), "battle_public_row_fields")
    _validate_value(row["row_kind"], value)
    if row["row_kind"] == 6:
        _require((value["bctx_environment_id"], value["bctx_season_id"]) == scope[2:], "battle_public_build_scope")
    if row["row_kind"] in (7, 8):
        _require((value["ctl_environment_id"], value["ctl_season_id"]) == scope[2:], "battle_public_control_scope")
    if row["row_kind"] == 9:
        _require((value["bout_environment_id"], value["bout_season_id"]) == scope[2:], "battle_public_result_scope")
    _require(_key(scope, row["row_kind"], value) == row["row_key"] and
        _quality(value) == row["quality_flags"], "battle_public_stored_identity")
    return value


@dataclass(slots=True)
class _Budget:
    limit: int
    max_rows: int
    deadline: Callable[[], None]
    used: int = 0
    rows: int = 0

    def reserve(self, amount):
        self.deadline()
        _require(amount <= self.limit - self.used, "battle_public_byte_capacity")
        self.used += amount

    def output(self):
        _require(self.rows < self.max_rows, "battle_public_output_capacity")
        self.reserve(ROW_FETCH_BYTE_BOUND + ROW_VALUE_BYTE_BOUND)
        self.rows += 1


def _ownership(window):
    result = defaultdict(list)
    for row, quality in zip(window.facts, window.projection_qualities, strict=True):
        if row["record_kind"] != 9:
            continue
        point = observations.validate_observation({name: row[name] for name in identity_publication.SOURCE_COLUMNS[9]})
        _day, flags, _utc = observations.point_day(point)
        key = tuple(row[name] for name in observations.SESSION), row["subject_id"], row["pid"]
        result[key].append(identity.OwnershipObservation(tuple(window.header[name] for name in SCOPE[2:]), key[0],
            tuple(row[name] for name in source.REPLAY), key[1], key[2], point["at_monotonic_usec"], point["account_token"] or None,
            identity_publication.OWNERSHIP_SOURCES[point["source"]],
            None if point["at_utc_usec"] == contribution.UTC_UNKNOWN else point["at_utc_usec"], flags | quality))
        _require(len(result[key]) <= identity.MAX_OWNERSHIP_OBSERVATIONS, "battle_public_ownership_capacity")
    return result


def _pieces(row, samples, registry, coverage, budget):
    """Use the existing identity boundary attribution for measured presence."""
    session = tuple(row["battle_actor_" + name] for name in observations.SESSION)
    actor, pid = row["battle_actor_id"], row["battle_actor_pid"]
    first, last = row["start_monotonic_usec"], row["observed_through_monotonic_usec"]
    utc_first, utc_last = row["start_utc_usec"], row["observed_through_utc_usec"]
    utc_first = None if utc_first == contribution.UTC_UNKNOWN else utc_first
    utc_last = None if utc_last == contribution.UTC_UNKNOWN else utc_last
    # Unsessioned PCs and pets have no authenticated owner authority. A pet's
    # owner subject alone cannot borrow another actor's account observations.
    player = row["battle_actor_kind"] == 1 and pid == actor == row["battle_actor_owner_subject_id"] and all(session)
    if not player:
        status = "not_player_actor" if row["battle_actor_kind"] == 3 else "unproven_owner_identity" if row["battle_actor_kind"] == 2 else "missing_session"
        quality = row["quality_flags"]
        comparable = (utc_first is not None and utc_last is not None and utc_last - utc_first == last - first and
            not quality & identity.UTC_ATTRIBUTION_FLAGS)
        if comparable and (identity_publication._day(utc_first) == UNKNOWN_DAY or
                identity_publication._day(utc_last - 1) == UNKNOWN_DAY or
                (utc_last - 1) // identity_publication.DAY_USEC - utc_first // identity_publication.DAY_USEC + 1 > identity_publication.MAX_DAY_SLICES):
            comparable = False
            quality |= ROLLUP_QUALITY_UTC_FANOUT
        cuts = [first, last]
        if comparable:
            point = (utc_first // identity_publication.DAY_USEC + 1) * identity_publication.DAY_USEC
            while point < utc_last:
                cuts.insert(-1, first + point - utc_first)
                point += identity_publication.DAY_USEC
        return [(a, b, utc_first + a - first if comparable else None, utc_first + b - first if comparable else None,
            None, None, None, status, quality) for a, b in zip(cuts, cuts[1:])]
    interval = identity.ObservedInterval((row["battle_environment_id"], row["battle_season_id"]), session,
        (*row["source_battle"][:2], row["start_record_seq"]), actor, pid, row["battle_config_id"], "presence",
        first, last, utc_first, utc_last, row["quality_flags"])
    if interval.comparable_utc and (identity_publication._day(utc_first) == UNKNOWN_DAY or
            identity_publication._day(utc_last - 1) == UNKNOWN_DAY or
            (utc_last - 1) // identity_publication.DAY_USEC - utc_first // identity_publication.DAY_USEC + 1 > identity_publication.MAX_DAY_SLICES):
        interval = replace(interval, quality_flags=interval.quality_flags | ROLLUP_QUALITY_UTC_FANOUT)
    shim = dict(environment_id=interval.scope[0], season_id=interval.scope[1],
        **dict(zip(observations.SESSION, session, strict=True)), subject_id=actor, pid=pid,
        boot_id=interval.replay[0], process_id=interval.replay[1], start_monotonic_usec=first, end_monotonic_usec=last,
        start_utc_usec=utc_first, end_utc_usec=utc_last, projection_quality=interval.quality_flags)
    gaps = identity_publication._monotonic_gaps(interval, identity_publication._gap_windows(shim, coverage, samples))
    pieces = identity.attribute_interval(interval, samples, registry, ownership_gap_windows=gaps)
    result = []
    for piece in pieces:
        for dated in identity_publication._day_slices(piece):
            budget.deadline()
            result.append((dated.start_monotonic_usec, dated.end_monotonic_usec, dated.start_utc_usec, dated.end_utc_usec,
                dated.account_token, dated.controller_token, dated.association_id, dated.linkage_status, dated.attribution_quality))
            _require(len(result) <= MAX_OUTPUT_ROWS, "battle_public_slice_capacity")
    return result


def _piece_reservation(samples, registry, coverage):
    # Reserve boundary dataclasses, converted tuples and day splits before
    # attribution allocates them. Reviewed association and ownership caps apply.
    accounts = {sample.account_token for sample in samples} - {None}
    associations = 0 if registry is None else sum(row.account_token in accounts for row in registry.associations)
    cuts = 4 + len(samples) + 2 * associations + 2 * len(coverage["incidents"]) + identity_publication.MAX_DAY_SLICES
    return cuts * 4_096


def _metric_identity(row, ownership, registry, coverage, budget):
    result = dict(row)
    for bit, names in contribution.COUNTER_FAMILIES:
        if not row["bc_available_metrics"] & bit:
            for name in names:
                result["bc_" + name] = None
    result.update(registry_version=None if registry is None else registry.registry_version,
        account_token=None, controller_token=None, association_id=None, linkage_status="unknown_account",
        attribution_status="unverified_association", attribution_quality_flags=row["publication_quality_flags"])
    if row["link_status"] != "verified":
        return result
    shim = {"battle_" + name.removeprefix("bc_"): row[name] for name in row if name.startswith("bc_actor_")}
    shim.update(source_battle=tuple(row[name] for name in contribution.BATTLE),
        battle_environment_id=row["bc_environment_id"], battle_season_id=row["bc_season_id"], battle_config_id=row["bc_config_id"],
        start_record_seq=row["record_seq"], start_monotonic_usec=row["bc_start_monotonic_usec"],
        observed_through_monotonic_usec=row["bc_observed_through_monotonic_usec"], start_utc_usec=row["bc_start_utc_usec"],
        observed_through_utc_usec=row["bc_observed_through_utc_usec"], quality_flags=row["publication_quality_flags"])
    if shim["observed_through_monotonic_usec"] == shim["start_monotonic_usec"]:
        # A zero-duration amount has no captured per-action ownership boundary.
        result["attribution_status"] = "unobserved_point_identity"
        return result
    session = tuple(shim["battle_actor_" + name] for name in observations.SESSION)
    samples = ownership[(session, shim["battle_actor_id"], shim["battle_actor_pid"])]
    reservation = _piece_reservation(samples, registry, coverage)
    budget.reserve(reservation)
    pieces = _pieces(shim, samples, registry, coverage, budget)
    identities = {tuple(piece[4:8]) for piece in pieces}
    if len(identities) == 1:
        account, controller, association, status = next(iter(identities))
        result.update(account_token=account, controller_token=controller, association_id=association,
            linkage_status=status, attribution_status="uniform_observed_identity")
    else:
        result["attribution_status"] = "identity_changes_inside_segment"
        accounts, controllers = {piece[4] for piece in pieces}, {piece[5] for piece in pieces}
        if len(accounts) == 1:
            result["account_token"] = next(iter(accounts))
            if result["account_token"] is not None:
                result["linkage_status"] = "mixed_review_linkage"
                if len(controllers) == 1 and None not in controllers and all(piece[7] == "confirmed" for piece in pieces):
                    result.update(controller_token=next(iter(controllers)), linkage_status="confirmed_across_associations",
                        attribution_status="uniform_observed_identity")
    if result["account_token"] is None and result["attribution_status"] == "uniform_observed_identity":
        result["attribution_status"] = "identity_unavailable"
    for piece in pieces:
        result["attribution_quality_flags"] |= piece[8]
    budget.used -= reservation
    return result


@dataclass(frozen=True, slots=True)
class Publication:
    header: Mapping[str, Any]
    rows: tuple[Mapping[str, Any], ...]
    reserved_bytes: int


def _build_verified(value):
    # This proves a point's context only. The availability mask still determines
    # which families can be used in a particular comparison; it creates no time
    # denominator and attributes no contribution amount to the sampled build.
    # UNKNOWN_CONTEXT is represented by the independent family mask (and the
    # explicit unknown buff origin). It does not negate a verified association,
    # point clock or configuration, nor make an unavailable family comparable.
    return comparison.point_context_verified(value)


def _comparison_value(value):
    # Exact raw values remain in the same public point. Avoid duplicating them
    # in the bounded payload while retaining every independent dimension status.
    classified = comparison.classify_point(value)
    return dict(classification_version=classified["classification_version"], catalog_digest=classified["catalog_digest"],
        dimensions={name: {field: cell[field] for field in ("status", "usable_for_matching")}
            for name, cell in classified["dimensions"].items()})


def _result_qualification(value):
    event_fatal = (battle.QUALITY_KNOWN & ~(1 | 4 | 64 | 256)) | identity.UTC_ATTRIBUTION_FLAGS | (
        history.ROLLUP_QUALITY_PROCESS_GAP | history.ROLLUP_QUALITY_INCIDENT_GAP | history.ROLLUP_QUALITY_INCIDENT_INVENTORY_UNKNOWN)
    qualified = (value["bout_kind"] <= 6 and value["native_source_status"] == "supported" and
        value["clock_status"] == value["configuration_status"] == "verified" and
        not value["event_quality_flags"] & event_fatal)
    if value["bout_kind"] == 4:
        qualified = qualified and value["chain_status"] == value["target_link_status"] == "verified"
    if value["bout_kind"] == 6:
        qualified = qualified and value["objective_status"] == "verified_commit"
    context_fatal = (battle.QUALITY_KNOWN & ~(1 | 64 | 256)) | event_fatal
    context = (qualified and value["target_link_status"] == "verified" and value["observed_pre_roster"] is not None and
        not value["publication_quality_flags"] & context_fatal)
    return bool(qualified), bool(context)


def _validate_result_value(value):
    _require(type(value) is dict and set(value) == set(RESULT_FIELDS), "battle_public_result_fields")
    raw = {name: value[name] for name in source.SOURCE_COLUMNS[14]}
    operation = raw["bout_operation_id"]
    _require(type(operation) is str and re.fullmatch("[0-9a-f]{32}", operation) is not None, "battle_public_result_operation")
    raw["bout_operation_id"] = bytes.fromhex(operation)
    results.validate_raw_observation(raw)
    _require(type(raw["ingest_id"]) is int and raw["ingest_id"] > 0 and type(raw["ingested_utc_usec"]) is int and
        -(1 << 63) <= raw["ingested_utc_usec"] < (1 << 63), "battle_public_result_arrival")
    for prefix in ("source", "target"):
        root = value[prefix + "_canonical_battle"]
        # Objective receipts may borrow only the exact request's historical
        # association; their raw receipt remains explicitly sessionless.
        inherited = value["reference_origin"] == "accepted_objective_request"
        ref = results.association(raw, prefix)
        _require(root is None or isinstance(root, (tuple, list)) and len(root) == 3 and
            all(type(item) is int and 0 < item <= battle.UINT64_MAX for item in root) and
            (inherited or tuple(root[:2]) == results.observation_key(raw)[:2]), "battle_public_result_reference")
        _require(inherited or (root is None) == (not any(ref)), "battle_public_result_optional_reference")
        _require(value[prefix + "_link_status"] in {"verified", "missing_packet", "partial_history", "outside_battle",
            "actor_not_in_packet", "context_changed", "stale_association", "outside_observed_prefix"}, "battle_public_result_link")
        account, controller, association = (value[prefix + "_" + name] for name in CONTROL_IDENTITY_FIELDS[:3])
        _require(all(item is None or type(item) is int and 0 < item <= battle.UINT64_MAX for item in
            (account, controller, association)), "battle_public_result_identity")
        actor = results.actor_context(raw, prefix)
        _require(account is None or actor["battle_actor_kind"] == 1 and actor["battle_actor_pid"] == actor["battle_actor_id"] ==
            actor["battle_actor_owner_subject_id"] and all(actor["battle_actor_" + name] for name in observations.SESSION),
            "battle_public_result_authenticated_identity")
        _require(controller is None or account is not None and association is not None and value["registry_version"] is not None and
            value[prefix + "_linkage_status"] == "confirmed", "battle_public_result_controller_authority")
        _require(value[prefix + "_linkage_status"] in {"unknown_account", "confirmed", "unknown", "clock_unknown",
            "outside_reviewed_window", "no_reviewed_mapping", "not_player_actor", "unproven_owner_identity", "missing_session"} and
            value[prefix + "_attribution_status"] in {"point_identity", "uniform_observed_identity", "identity_changes_inside_prefix",
                "identity_unavailable", "not_player_actor", "unproven_owner_identity", "missing_session"}, "battle_public_result_identity_status")
        source._quality(value[prefix + "_attribution_quality_flags"])
    parent = value["parent_event_key"]
    _require(parent is None or isinstance(parent, (tuple, list)) and len(parent) == 3 and
        all(type(item) is int and 0 < item <= battle.UINT64_MAX for item in parent), "battle_public_result_parent")
    _require(value["clock_status"] in {"verified", "unknown", "unverified", "mismatch", "discontinuous"} and
        value["chain_status"] in {"not_applicable", "verified", "missing_parent", "censored", "reengaged", "partial_history"} and
        value["objective_status"] in {"not_applicable", "requested", "unresolved", "recovered_claim", "unsupported_authority",
            "missing_request", "verified_commit", "duplicate_commit", "configuration_changed", "cross_producer_request"} and
        value["reference_origin"] in {"native_pre_teardown", "native_escape_parent", "accepted_objective_request"} and
        value["native_source_status"] in {"supported", "unsupported"} and
        value["configuration_status"] in {"verified", "unknown", "unavailable"} and
        (value["configuration_status"] == "unavailable") == (raw["bout_config_id"] == 0), "battle_public_result_status")
    roster = value["observed_pre_roster"]
    fields = ("observed_pre_roster_count", "observed_pre_pc_count", "observed_pre_owner_count", "observed_pre_own_side_owners",
        "observed_pre_opposing_owners", "observed_pre_mode", "observed_pre_side_status")
    _require(roster is None or type(roster) is dict and set(roster) == set(fields) and
        all(item is None and name in fields[3:5] or type(item) is int and 0 <= item <= battle.MAX_ACTORS
            for name, item in roster.items()), "battle_public_result_roster")
    source._quality(value["event_quality_flags"])
    _require(not value["bout_quality_flags"] & ~value["event_quality_flags"] and
        not value["event_quality_flags"] & ~value["publication_quality_flags"], "battle_public_result_quality_erasure")
    _require(type(value["event_evidence_qualified"]) is bool and type(value["battle_context_qualified"]) is bool and
        (value["event_evidence_qualified"], value["battle_context_qualified"]) == _result_qualification(value) and
        all(value[name] is False for name in ("whole_battle_victory_implied", "full_zone_clear_implied", "complete_population_coverage_implied")),
        "battle_public_result_qualification")
    _require(value["registry_version"] is None or type(value["registry_version"]) is int and value["registry_version"] > 0,
        "battle_public_result_registry")
    _quality(value)


def _control_duration_mask(value):
    # The observed prefix can be censored at a later decision. Its recorded end
    # is retained, never extended into that tail. Status duration is not action
    # restriction time; the native v1 contract carries no action-gate evidence.
    fatal = (battle.QUALITY_KNOWN & ~64) | history.ROLLUP_QUALITY_PROCESS_GAP | identity.UTC_ATTRIBUTION_FLAGS | (
        history.ROLLUP_QUALITY_INCIDENT_GAP | history.ROLLUP_QUALITY_INCIDENT_INVENTORY_UNKNOWN)
    qualified = (value["ctl_kind"] == 3 and value["chain_status"] == value["target_link_status"] ==
        value["clock_status"] == value["configuration_status"] == "verified" and
        not value["publication_quality_flags"] & fatal)
    return value["ctl_duration_coverage"] if qualified else 0


def _validate_control_value(kind, value):
    _require(type(value) is dict and set(value) == set(CONTROL_FIELDS), "battle_public_control_fields")
    raw = {name: value[name] for name in source.SOURCE_COLUMNS[13]}
    controls.validate_raw_observation(raw)
    _require(type(raw["ingest_id"]) is int and raw["ingest_id"] > 0 and type(raw["ingested_utc_usec"]) is int and
        -(1 << 63) <= raw["ingested_utc_usec"] < (1 << 63), "battle_public_control_arrival")
    _require((kind == 7) == (raw["ctl_kind"] == 1), "battle_public_control_row_family")
    link_states = {"verified", "missing_packet", "partial_history", "outside_battle", "context_changed",
        "stale_association", "outside_observed_prefix", "missing_lifecycle", "lifecycle_mismatch"}
    for prefix in ("source", "target"):
        ref = controls._association(raw, prefix)
        root = value[prefix + "_canonical_battle"]
        _require(root is None if not any(ref) else isinstance(root, (tuple, list)) and len(root) == 3 and
            all(type(item) is int and 0 < item <= battle.UINT64_MAX for item in root) and
            tuple(root[:2]) == controls.observation_key(raw)[:2], "battle_public_control_reference")
        _require(value[prefix + "_link_status"] in link_states and
            (value[prefix + "_link_status"] == "outside_battle") == (not any(ref)), "battle_public_control_link")
        account, controller, association = (value[prefix + "_" + name] for name in CONTROL_IDENTITY_FIELDS[:3])
        _require(all(item is None or type(item) is int and 0 < item <= battle.UINT64_MAX for item in
            (account, controller, association)), "battle_public_control_identity")
        actor = controls.actor_context(raw, prefix)
        if account is not None:
            _require(actor["battle_actor_kind"] == 1 and actor["battle_actor_pid"] == actor["battle_actor_id"] ==
                actor["battle_actor_owner_subject_id"] and all(actor["battle_actor_" + name] for name in observations.SESSION),
                "battle_public_control_authenticated_actor")
        _require(controller is None or account is not None and association is not None and value["registry_version"] is not None and
            value[prefix + "_linkage_status"] == "confirmed", "battle_public_control_controller_authority")
        _require(value[prefix + "_linkage_status"] in {"unknown_account", "confirmed", "unknown", "clock_unknown",
            "outside_reviewed_window", "no_reviewed_mapping", "not_player_actor", "unproven_owner_identity", "missing_session", "not_applicable"} and
            value[prefix + "_attribution_status"] in {"point_identity", "uniform_observed_identity", "identity_changes_inside_prefix",
                "identity_unavailable", "not_applicable", "not_player_actor", "unproven_owner_identity", "missing_session"},
            "battle_public_control_attribution_status")
        flags = value[prefix + "_attribution_quality_flags"]
        _require(type(flags) is int and 0 <= flags <= ROLLUP_QUALITY_MASK and flags & ~ROLLUP_QUALITY_MASK == 0,
            "battle_public_control_identity_quality")
    _require(value["registry_version"] is None or type(value["registry_version"]) is int and value["registry_version"] > 0,
        "battle_public_control_registry")
    expected_chain = {1: {"not_applicable"}, 2: {"entry"}, 3: {"verified", "missing_predecessor"}, 4: {"not_applicable"}}
    _require(value["chain_status"] in expected_chain[raw["ctl_kind"]] and
        value["clock_status"] in {"verified", "unknown", "mismatch", "discontinuous"} and
        value["configuration_status"] in {"verified", "unknown", "unavailable"} and
        (value["configuration_status"] == "unavailable") == (raw["ctl_config_id"] == 0), "battle_public_control_status")
    _require(type(value["accepted_application_count"]) is int and value["accepted_application_count"] ==
        int(raw["ctl_kind"] == raw["ctl_result"] == 1), "battle_public_control_accepted_count")
    elapsed = raw["ctl_at_usec"] - raw["ctl_start_usec"] if raw["ctl_kind"] == 3 else None
    mask = _control_duration_mask(value)
    _require(value["observed_prefix_usec"] == elapsed and type(value["qualified_duration_mask"]) is int and
        value["qualified_duration_mask"] == mask and isinstance(value["qualified_status_usec"], (tuple, list)) and
        list(value["qualified_status_usec"]) == [(elapsed if raw["ctl_before_mask"] & (1 << bit) else 0)
            if mask & (1 << bit) else None for bit in range(8)] and
        value["proven_action_restriction_usec"] is None and value["caster_attributed_duration_usec"] is None,
        "battle_public_control_duration")
    _quality(value)


def _control_identity(row, prefix, ownership, registry, coverage, budget):
    actor = controls.actor_context(row, prefix)
    result = dict(account_token=None, controller_token=None, association_id=None, linkage_status="unknown_account",
        attribution_status="identity_unavailable", attribution_quality_flags=row["publication_quality_flags"])
    if prefix == "source" and row["ctl_kind"] != 1:
        return dict(result, linkage_status="not_applicable", attribution_status="not_applicable", attribution_quality_flags=0)
    session = tuple(actor["battle_actor_" + name] for name in observations.SESSION)
    if actor["battle_actor_kind"] != 1 or not all(session):
        status = "not_player_actor" if actor["battle_actor_kind"] == 3 else "unproven_owner_identity" if actor["battle_actor_kind"] == 2 else "missing_session"
        return dict(result, linkage_status=status, attribution_status=status)
    samples = ownership[(session, actor["battle_actor_id"], actor["battle_actor_pid"])]
    reservation = _piece_reservation(samples, registry, coverage)
    budget.reserve(reservation)
    try:
        if prefix == "target" and row["ctl_kind"] == 3 and row["ctl_at_usec"] > row["ctl_start_usec"]:
            shim = dict(actor, source_battle=(*controls.observation_key(row)[:2], row["ctl_target_battle_seq"]),
                battle_environment_id=row["ctl_environment_id"], battle_season_id=row["ctl_season_id"],
                battle_config_id=row["ctl_config_id"], start_record_seq=row["record_seq"],
                start_monotonic_usec=row["ctl_start_usec"], observed_through_monotonic_usec=row["ctl_at_usec"],
                start_utc_usec=row["ctl_start_utc_usec"], observed_through_utc_usec=row["ctl_at_utc_usec"],
                quality_flags=row["publication_quality_flags"])
            pieces = _pieces(shim, samples, registry, coverage, budget)
            identities = {tuple(piece[4:8]) for piece in pieces}
            if len(identities) == 1:
                account, controller, association, status = next(iter(identities))
                result.update(account_token=account, controller_token=controller, association_id=association,
                    linkage_status=status, attribution_status="uniform_observed_identity" if account is not None else "identity_unavailable")
            else:
                result["attribution_status"] = "identity_changes_inside_prefix"
            for piece in pieces:
                result["attribution_quality_flags"] |= piece[8]
            return result
        at, utc = row["ctl_decision_usec"], row["ctl_decision_utc_usec"]
        ordered = identity.ordered_ownership((row["ctl_environment_id"], row["ctl_season_id"]), session,
            actor["battle_actor_id"], actor["battle_actor_pid"], controls.observation_key(row)[:2], samples)
        before = [sample for sample in ordered if sample.at_monotonic_usec <= at]
        sample = before[-1] if before else None
        account = None if sample is None else sample.account_token
        quality = row["publication_quality_flags"] | (sample.quality_flags if sample else 0)
        if sample is not None and sample.at_utc_usec is not None and utc != controls.UTC_UNKNOWN and (
                sample.at_utc_usec + at - sample.at_monotonic_usec != utc):
            quality |= identity.ROLLUP_QUALITY_UTC_MISMATCH
        shim = dict(zip(observations.SESSION, session, strict=True), environment_id=row["ctl_environment_id"],
            season_id=row["ctl_season_id"], boot_id=row["ctl_boot_id"], process_id=row["ctl_process_id"],
            subject_id=actor["battle_actor_id"], pid=actor["battle_actor_pid"], at_monotonic_usec=at,
            at_utc_usec=utc, projection_quality=quality)
        windows = identity_publication._gap_windows(shim, coverage, samples)
        if windows and (utc == controls.UTC_UNKNOWN or any((first is None or first <= utc) and
                (last is None or utc < last) for first, last in windows)):
            account = None
            quality |= history.ROLLUP_QUALITY_INCIDENT_GAP
        controller, association, status = identity.linkage_at(registry, account,
            None if utc == controls.UTC_UNKNOWN or quality & identity.UTC_ATTRIBUTION_FLAGS else utc)
        return dict(result, account_token=account, controller_token=controller, association_id=association,
            linkage_status=status, attribution_status="point_identity" if account is not None else "identity_unavailable",
            attribution_quality_flags=quality)
    finally:
        budget.used -= reservation


def build_publication(window: source.VerifiedSource, registry: identity.Registry | None, coverage: Mapping[str, Any], *,
                      max_total_bytes: int = source.DEFAULT_BYTE_LIMIT, max_output_rows: int = 2_000,
                      check_deadline: Callable[[], None] = lambda: None) -> Publication:
    _require(type(window) is source.VerifiedSource and type(max_total_bytes) is int and max_total_bytes > 0 and
        type(max_output_rows) is int and 0 < max_output_rows <= MAX_OUTPUT_ROWS and callable(check_deadline), "battle_public_capacity")
    scope = source._header(window.header)
    _require(window.header["publication_complete"] == 0 and len(window.facts) == window.header["source_fact_count"] ==
        len(window.projection_qualities), "battle_public_building_source")
    _require(registry is None or type(registry) is identity.Registry and (registry.environment_id, registry.season_id) == scope[2:],
        "battle_public_registry_scope")
    incident_version = incident.generation_schema(scope[0])
    _require(coverage["registry_schema_version"] == incident_version, "battle_public_incident_schema")
    budget = _Budget(max_total_bytes, max_output_rows, check_deadline)
    budget.reserve(source.HEADER_BYTE_BOUND + len(window.facts) * source.PUBLICATION_INPUT_BYTE_BOUND + REVIEW_BYTE_BOUND)
    digest, previous, counts = source.seed_digest(scope, window.header["input_origin"]), window.header["input_origin"], dict.fromkeys(source.counts(scope).values(), 0)
    _require(not window.configurations or len(window.configurations) == len(window.facts), "battle_public_configuration_count")
    budget.reserve(source.PUBLICATION_INPUT_BYTE_BOUND)
    for index, (row, flags) in enumerate(zip(window.facts, window.projection_qualities, strict=True)):
        check_deadline()
        _require(previous < row["ingest_id"] <= window.header["input_watermark"], "battle_public_source_cursor")
        previous = row["ingest_id"]
        retained = source.retain_input(row, scope, flags, configuration=window.configurations[index] if window.configurations else None)
        digest = source.advance_digest(digest, retained)
        counts[source.counts(scope)[row["record_kind"]]] += 1
    budget.used -= source.PUBLICATION_INPUT_BYTE_BOUND
    _require(digest == window.header["source_digest"] and all(counts[name] == window.header[name] for name in counts), "battle_public_source_changed")
    reduced = history.build_history([row for row in window.facts if row["record_kind"] in (10, 11, 12, 13, 14)], scope[2:],
        incident_coverage=coverage, incident_schema_version=incident_version, max_output_rows=max_output_rows, max_total_bytes=budget.limit - budget.used,
        check_deadline=check_deadline)
    budget.reserve(reduced.summary["reserved_bytes"])
    ownership = _ownership(window)
    output, keys = [], set()
    header = dict(window.header, snapshot_digest=b"\0" * 32, **dict.fromkeys(coverage_columns(scope)[len(source.header_columns(scope)) + 1:], 0))
    header.update(publication_complete=1, quality_flags=window.header["quality_flags"] | coverage["quality_flags"])

    def append(kind, value):
        budget.output()
        row = retain_row(scope, kind, value)
        key = kind, row["row_key"]
        _require(key not in keys, "battle_public_duplicate_cell")
        keys.add(key)
        output.append(row)
        header[count_fields(scope)[kind - 1]] += 1
        header["quality_flags"] |= row["quality_flags"]

    for value in reduced.battles:
        append(1, dict(value))
    for value in reduced.actors:
        append(2, dict(value))
    for value in reduced.contributions:
        append(3, _metric_identity(value, ownership, registry, coverage, budget))
    build_source = {builds.observation_key(row): (row, configuration, flags) for row, configuration, flags in zip(window.facts,
        window.configurations or (None,) * len(window.facts), window.projection_qualities, strict=True) if row["record_kind"] == 12}
    for value in reduced.builds:
        raw, configuration, flags = build_source[builds.observation_key(value)]
        row = dict(value, ingest_id=raw["ingest_id"], ingested_utc_usec=raw["ingested_utc_usec"],
            configuration_status="unavailable" if not value["bctx_config_id"] else
                "unknown" if configuration is None else "verified")
        if scope[0] == source.RESULT_DEFINITION_VERSION:
            row["publication_quality_flags"] |= flags
        row["point_context_verified"] = _build_verified(row)
        for name in builds.BYTE_FIELDS:
            row[name] = row[name].hex()
        if scope[0] == source.RESULT_DEFINITION_VERSION:
            row["comparison"] = _comparison_value(row)
            for cell in row["comparison"]["dimensions"].values():
                header["comparison_" + cell["status"] + "_cells"] += 1
                header["comparison_matching_cells"] += int(cell["usable_for_matching"])
        append(6, row)
        header["qualified_build_points"] += int(row["point_context_verified"])
        header["unavailable_build_points"] += int(row["bctx_status"] == 2)
        header["configuration_unknown_build_points"] += int(row["configuration_status"] != "verified")
    control_source = {controls.observation_key(row): (row, configuration, flags) for row, configuration, flags in zip(window.facts,
        window.configurations or (None,) * len(window.facts), window.projection_qualities, strict=True) if row["record_kind"] == 13}
    for value in reduced.controls:
        raw, configuration, flags = control_source[controls.observation_key(value)]
        row = dict(value, ingest_id=raw["ingest_id"], ingested_utc_usec=raw["ingested_utc_usec"],
            configuration_status="unavailable" if not value["ctl_config_id"] else "unknown" if configuration is None else "verified",
            accepted_application_count=int(value["ctl_kind"] == value["ctl_result"] == 1),
            proven_action_restriction_usec=None, caster_attributed_duration_usec=None,
            registry_version=None if registry is None else registry.registry_version)
        if scope[0] == source.RESULT_DEFINITION_VERSION:
            row["publication_quality_flags"] |= flags
        row["qualified_duration_mask"] = _control_duration_mask(row)
        row["qualified_status_usec"] = [(row["observed_prefix_usec"] if row["ctl_before_mask"] & (1 << bit) else 0)
            if row["qualified_duration_mask"] & (1 << bit) else None for bit in range(8)]
        for prefix in ("source", "target"):
            attributed = _control_identity(row, prefix, ownership, registry, coverage, budget)
            row.update({prefix + "_" + name: item for name, item in attributed.items()})
        append(7 if row["ctl_kind"] == 1 else 8, row)
        header["control_accepted_applications"] += row["accepted_application_count"]
        for control_kind, field in ((2, "control_entry_count"), (3, "control_interval_count"), (4, "control_gap_count")):
            header[field] += int(row["ctl_kind"] == control_kind)
        header["verified_control_target_links" if row["target_link_status"] == "verified" else "partial_control_target_links"] += 1
        header["verified_control_chains"] += int(row["chain_status"] == "verified")
        header["qualified_control_prefixes"] += int(row["qualified_duration_mask"] != 0)
        header["configuration_unknown_control_points"] += int(row["configuration_status"] != "verified")
    result_source = {results.observation_key(row): (row, configuration, flags) for row, configuration, flags in zip(window.facts,
        window.configurations or (None,) * len(window.facts), window.projection_qualities, strict=True) if row["record_kind"] == 14}
    for value in reduced.results:
        raw, configuration, flags = result_source[results.observation_key(value)]
        parent = result_source.get(value["parent_event_key"])
        row = dict(value, ingest_id=raw["ingest_id"], ingested_utc_usec=raw["ingested_utc_usec"],
            configuration_status="unavailable" if not value["bout_config_id"] else
                "unknown" if configuration is None or parent is not None and parent[1] is None else "verified",
            registry_version=None if registry is None else registry.registry_version)
        row["event_quality_flags"] |= flags | (parent[2] if parent is not None else 0)
        row["publication_quality_flags"] |= row["event_quality_flags"]
        row["event_evidence_qualified"], row["battle_context_qualified"] = _result_qualification(row)
        for prefix in ("source", "target"):
            shim = {name.replace("bout_", "ctl_", 1): item for name, item in row.items()}
            shim.update(ctl_kind=3 if prefix == "target" and row["bout_kind"] == 4 else 1,
                ctl_decision_usec=row["bout_at_usec"], ctl_decision_utc_usec=row["bout_at_utc_usec"])
            attributed = _control_identity(shim, prefix, ownership, registry, coverage, budget)
            row.update({prefix + "_" + name: item for name, item in attributed.items()})
        row["bout_operation_id"] = row["bout_operation_id"].hex()
        append(9, row)
        header["observed_" + RESULT_KINDS[row["bout_kind"] - 1] + "_count"] += 1
        header["qualified_result_evidence_count"] += int(row["event_evidence_qualified"])
        header["unknown_result_evidence_count"] += int(not row["event_evidence_qualified"])
        header["qualified_result_context_count"] += int(row["battle_context_qualified"])
        header["configuration_unknown_result_count"] += int(row["configuration_status"] != "verified")
        header["recovered_objective_count"] += int(row["objective_status"] == "recovered_claim")
        header["duplicate_objective_count"] += int(row["objective_status"] == "duplicate_commit")
        header["qualified_objective_commit_count"] += int(row["bout_kind"] == 6 and row["event_evidence_qualified"])
    for value, flags in zip(window.facts, window.projection_qualities, strict=True):
        if value["record_kind"] == 10:
            append(5, dict(value, projection_quality_flags=flags))
    for value in reduced.exposures:
        session = tuple(value["battle_actor_" + name] for name in observations.SESSION)
        samples = ownership[(session, value["battle_actor_id"], value["battle_actor_pid"])]
        reservation = _piece_reservation(samples, registry, coverage)
        budget.reserve(reservation)
        pieces = _pieces(value, samples, registry, coverage, budget)
        elapsed = value["observed_through_monotonic_usec"] - value["start_monotonic_usec"]
        _require(all(value[name] in (0, elapsed) for name in battle.EFFORT), "battle_public_source_exposure_conservation")
        for first, last, utc_first, utc_last, account, controller, association, status, flags in pieces:
            row = dict(value, start_monotonic_usec=first, observed_through_monotonic_usec=last,
                start_utc_usec=utc_first, observed_through_utc_usec=utc_last,
                utc_day=None if utc_first is None or identity_publication._day(utc_first) == UNKNOWN_DAY else identity_publication._day(utc_first).isoformat(),
                registry_version=None if registry is None else registry.registry_version, account_token=account,
                controller_token=controller, association_id=association, linkage_status=status, quality_flags=flags)
            row.update({name: last - first if value[name] else 0 for name in battle.EFFORT})
            append(4, row)
            header["observed_present_usec"] = observations.checked_add(header["observed_present_usec"], last - first, "battle_presence")
            if value["battle_actor_kind"] == 1:
                for field in ("observed_pc_present_usec", "unknown_account_pc_present_usec" if account is None else "owned_pc_present_usec",
                        "unlinked_controller_pc_present_usec" if controller is None else "confirmed_controller_pc_present_usec"):
                    header[field] = observations.checked_add(header[field], last - first, field)
            else:
                header["non_pc_present_usec"] = observations.checked_add(header["non_pc_present_usec"], last - first, "non_pc_presence")
        _require(sum(piece[1] - piece[0] for piece in pieces) == elapsed, "battle_public_attribution_conservation")
        budget.used -= reservation
    for field in ("complete_packet_count", "incomplete_packet_count", "alias_count", "canonical_battle_count",
            "verified_contribution_links", "partial_contribution_links"):
        header[field] = reduced.summary[field]
    if scope[0] in source.BUILD_DEFINITION_VERSIONS:
        header.update({field: reduced.summary[field] for field in ("verified_build_links", "partial_build_links")})
    header["snapshot_digest"] = snapshot_digest(scope, sorted(output, key=lambda item: (item["row_kind"], item["row_key"])),
        check_deadline=check_deadline)
    _validate_header(header)
    _require(header["observed_present_usec"] == reduced.summary["verified_exposure_present_usec"], "battle_public_history_conservation")
    return Publication(header, tuple(output), budget.used)


def _validate_header(row):
    _require(isinstance(row, Mapping) and set(SCOPE) <= set(row), "battle_public_header_fields")
    scope = tuple(row[name] for name in SCOPE)
    _require(set(row) == set(coverage_columns(scope)), "battle_public_header_fields")
    source._header({name: row[name] for name in source.header_columns(scope)})
    _require(row["publication_complete"] == 1 and type(row["snapshot_digest"]) is bytes and len(row["snapshot_digest"]) == 32,
        "battle_public_header_incomplete")
    for name in coverage_columns(scope)[len(source.header_columns(scope)) + 1:]:
        _require(type(row[name]) is int and 0 <= row[name] <= battle.UINT64_MAX, "battle_public_header_scalar")
    _require(sum(row[name] for name in count_fields(scope)) <= MAX_OUTPUT_ROWS and
        row["association_row_count"] == row["association_count"] and
        row["contribution_row_count"] == row["contribution_count"] == row["verified_contribution_links"] + row["partial_contribution_links"] and
        row["observed_present_usec"] == row["observed_pc_present_usec"] + row["non_pc_present_usec"] and
        row["observed_pc_present_usec"] == row["owned_pc_present_usec"] + row["unknown_account_pc_present_usec"] ==
        row["confirmed_controller_pc_present_usec"] + row["unlinked_controller_pc_present_usec"], "battle_public_header_conservation")
    if scope[0] in source.BUILD_DEFINITION_VERSIONS:
        _require(row["build_count"] == row["build_row_count"] == row["verified_build_links"] + row["partial_build_links"] and
            row["qualified_build_points"] <= row["verified_build_links"] and
            row["qualified_build_points"] + row["unavailable_build_points"] <= row["build_count"] and
            row["configuration_unknown_build_points"] <= row["build_count"],
            "battle_public_build_conservation")
    if scope[0] in source.CONTROL_DEFINITION_VERSIONS:
        _require(row["control_count"] == row["control_operation_count"] + row["control_state_count"] ==
            row["verified_control_target_links"] + row["partial_control_target_links"] and
            row["control_state_count"] == row["control_entry_count"] + row["control_interval_count"] + row["control_gap_count"] and
            row["control_accepted_applications"] <= row["control_operation_count"] and
            row["qualified_control_prefixes"] <= row["verified_control_chains"] <= row["control_interval_count"] and
            row["configuration_unknown_control_points"] <= row["control_count"], "battle_public_control_conservation")
    if scope[0] == source.RESULT_DEFINITION_VERSION:
        _require(row["result_count"] == row["result_row_count"] ==
            sum(row["observed_" + name + "_count"] for name in RESULT_KINDS) ==
            row["qualified_result_evidence_count"] + row["unknown_result_evidence_count"] and
            row["qualified_result_context_count"] <= row["qualified_result_evidence_count"] and
            row["configuration_unknown_result_count"] <= row["result_count"] and
            row["qualified_objective_commit_count"] + row["recovered_objective_count"] + row["duplicate_objective_count"] <=
                row["observed_objective_commit_count"] and
            sum(row["comparison_" + name + "_cells"] for name in ("observed", "partial", "unavailable", "unclassified")) ==
                row["build_count"] * len(comparison.DIMENSIONS) and
            row["comparison_matching_cells"] <= row["comparison_observed_cells"], "battle_public_result_conservation")


def public_header(row, reservation):
    _validate_header(row)
    _require(tuple(row[name] for name in SCOPE) == tuple(reservation[name] for name in SCOPE), "battle_public_identity_scope")
    metadata = identity.public_generation(reservation)
    metadata.update(status="published_reviewed_version" if reservation["registry_version"] is not None else "published_unknown_identity",
        balance_report_published=True)
    result = dict(row, source_digest=row["source_digest"].hex(), snapshot_digest=row["snapshot_digest"].hex(),
        identity=metadata, source_input_retention_complete=True,
        presence_is_input_activity=False, alternative_projections_additive=False, metrics_apportioned_across_identity_boundaries=False,
        decisive_outcomes_available=False, complete_metric_coverage_implied=False, complete_controller_population_implied=False,
        zero_activity_implied=False, economic_rewards_included=False, rates_computed=False)
    if row["definition_version"] in source.BUILD_DEFINITION_VERSIONS:
        result.update(builds_are_point_observations=True, continuous_build_exposure_implied=False,
            contributions_attributed_to_builds=False, raw_cleared_values_require_available_family=True,
            build_actor_identity_inferred=False, arena_roster_is_match_result=False)
    if row["definition_version"] in source.CONTROL_DEFINITION_VERSIONS:
        result.update(control_operations_and_status_prefixes_are_separate=True, configured_ticks_are_elapsed_time=False,
            status_family_order=("blindness", "stun", "major_paralysis", "minor_paralysis", "slow", "sleep", "silence", "binding"),
            overlapping_status_families_are_additive=False, caster_attributed_duration_available=False,
            proven_action_restriction_duration_available=False, complete_control_attempt_coverage_implied=False)
    if row["definition_version"] == source.RESULT_DEFINITION_VERSION:
        result.update(comparison_classification_version=comparison.CLASSIFICATION_VERSION, comparison_catalog_digest=comparison.CATALOG_DIGEST,
            comparison_dimensions=comparison.DIMENSIONS, level_is_combat_strength=False,
            result_kind_order=RESULT_KINDS, escape_minimum_usec=results.ESCAPE_MIN_USEC,
            event_evidence_and_battle_context_are_separate=True, whole_battle_victory_implied=False, full_zone_clear_implied=False,
            objective_receipt_is_economic_amount=False, recovered_claims_are_new_objectives=False,
            complete_outcome_population_implied=False, comparison_reports_are_same_point_rows=True)
    return result
