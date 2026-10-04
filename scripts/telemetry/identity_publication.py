"""Retained inputs and atomic identity effort/XP publication for definition 3.

This module computes observations, not durable economic rewards or human census.
The existing cursor transaction retains canonical selected source facts. Its
publication transaction owns the reviewed identity and incident snapshots and
all outputs. No current ownership or latest identity review is consulted here.
"""
from __future__ import annotations

from collections import defaultdict
from dataclasses import replace
from datetime import date, timedelta
import hashlib
import json
from typing import Any, Callable, Mapping, Sequence

try:
    from . import identity_history as identity, incident, observation_semantics as observations
    from .rollup_definitions import (RollupTarget, UNKNOWN_DAY, UTC_UNKNOWN, UINT64_MAX,
        ROLLUP_QUALITY_MASK, ROLLUP_QUALITY_UTC_UNKNOWN, ROLLUP_QUALITY_UTC_MISMATCH,
        ROLLUP_QUALITY_UTC_FANOUT, ROLLUP_QUALITY_CONTEXT_UNAVAILABLE,
        ROLLUP_QUALITY_INCIDENT_GAP)
except ImportError:
    import identity_history as identity
    import incident
    import observation_semantics as observations
    from rollup_definitions import (RollupTarget, UNKNOWN_DAY, UTC_UNKNOWN, UINT64_MAX,
        ROLLUP_QUALITY_MASK, ROLLUP_QUALITY_UTC_UNKNOWN, ROLLUP_QUALITY_UTC_MISMATCH,
        ROLLUP_QUALITY_UTC_FANOUT, ROLLUP_QUALITY_CONTEXT_UNAVAILABLE,
        ROLLUP_QUALITY_INCIDENT_GAP)

DEFINITION_VERSION = 3
MAX_INPUTS = 16_384
MAX_PAYLOAD_BYTES = 4_096
INPUT_ROW_BYTE_BOUND = 6_144
PUBLICATION_INPUT_BYTE_BOUND = 16_384
DAY_USEC = 86_400_000_000
MAX_DAY_SLICES = 32
SCOPE = observations.SCOPE
REPLAY = ("boot_id", "process_id", "record_seq")
INPUT_COLUMNS = (*SCOPE, "ingest_id", *REPLAY, "subject_id", "pid", "record_kind", "payload", "payload_digest")
HEADER_COLUMNS = (*SCOPE, "input_watermark", "source_fact_count", "source_digest", "publication_complete",
    "interval_count", "ownership_count", "progression_count", "effort_row_count", "portfolio_row_count",
    "effort_slice_count", "observed_character_usec", "owned_character_usec", "confirmed_controller_character_usec",
    "unknown_account_character_usec", "unlinked_controller_character_usec", "incident_affected_character_usec", "quality_flags")
CELL_DIMENSIONS = ("utc_day", "partition_kind", "basis", "identity_token", "config_id", "classifier_version",
    "policy_version", "faction_id", "level_band", "group_context_mode")
EFFORT_COLUMNS = (*SCOPE, "cell_digest", *CELL_DIMENSIONS, "category", "character_usec",
    "utc_covered_character_usec", "unknown_clock_character_usec", "covered_union_usec", "union_usec",
    "distinct_characters", "distinct_accounts", "quality_flags")
XP_DIMENSIONS = (*CELL_DIMENSIONS, "source", "reason", "observation_status", "modifier_flags")
XP_COLUMNS = (*SCOPE, "cell_digest", *XP_DIMENSIONS, *observations.XP_METRICS,
    "linked_observations", "clock_unknown_observations", "incident_affected_observations", "quality_flags")
BASES = {"character": 1, "account": 2, "controller": 3, "unknown_account": 4, "unknown_controller": 5}
CATEGORIES = {"unknown": 0, "idle": 1, "active": 2, "resident_linkdead": 3, "presence": 4}
OWNERSHIP_SOURCES = {1: "authenticated_login", 2: "authenticated_reconnect", 3: "authenticated_copyover",
                     4: "authenticated_change", 5: "unavailable"}
COMMON_SOURCE = ("ingest_id", *REPLAY, "schema_version", "record_kind", "occurrence_utc_usec",
    "environment_id", "season_id", *observations.SESSION, "subject_id", "pid", "config_id",
    "classifier_version", "policy_version", *observations.DIMENSIONS, "quality_flags")
POINT_SOURCE = ("connection_boot_id", "connection_process_id", "connection_seq", "at_monotonic_usec", "at_utc_usec")
SOURCE_COLUMNS = {
    1: (*COMMON_SOURCE, "duration_usec", "start_monotonic_usec", "end_monotonic_usec", "start_utc_usec",
        "end_utc_usec", "category", "context", "context_quality"),
    6: (*COMMON_SOURCE, *POINT_SOURCE, *observations.PROGRESSION_RAW_COLUMNS),
    9: (*COMMON_SOURCE, *POINT_SOURCE, *observations.OWNERSHIP_RAW_COLUMNS),
}


class PublicationError(ValueError):
    """A payload-free projection, retention, attribution or capacity failure."""


def _integer(value: Any, name: str, *, lower=0, upper=UINT64_MAX) -> int:
    if type(value) is not int or not lower <= value <= upper:
        raise PublicationError(name + "_range")
    return value


def _bytes(value: Any, size: int, name: str) -> bytes:
    if type(value) is not bytes or len(value) != size:
        raise PublicationError(name + "_shape")
    return value


def _canonical(value: Any) -> bytes:
    return json.dumps(value, sort_keys=True, separators=(",", ":"), allow_nan=False).encode()


def seed_digest(scope: tuple[int, int, int, int]) -> bytes:
    identity.generation_scope(scope)
    return hashlib.sha256(b"duris-identity-source-v1:" + _canonical(scope)).digest()


def advance_digest(digest: bytes, row: Mapping[str, Any]) -> bytes:
    return hashlib.sha256(_bytes(digest, 32, "source_digest") +
        _integer(row["ingest_id"], "ingest_id", lower=1).to_bytes(8, "big") +
        _bytes(row["payload_digest"], 32, "payload_digest")).digest()


def retain_input(row: Mapping[str, Any], target: RollupTarget, quality: int) -> dict[str, Any]:
    """Called only after the existing raw family validator accepted this fact."""
    if target.definition_version != DEFINITION_VERSION or row["record_kind"] not in SOURCE_COLUMNS:
        raise PublicationError("unsupported_identity_input")
    quality = _integer(quality, "projection_quality")
    if quality & ~ROLLUP_QUALITY_MASK:
        raise PublicationError("unknown_projection_quality")
    source = {name: row.get(name) for name in SOURCE_COLUMNS[row["record_kind"]]}
    if (source["environment_id"], source["season_id"]) != target.scope_tuple[2:]:
        raise PublicationError("input_scope_mismatch")
    for name, value in source.items():
        if type(value) is not int:
            raise PublicationError("input_field_type")
        _integer(value, name, lower=-(1 << 63))
    payload = _canonical({"projection_quality": quality, "source": source})
    if len(payload) > MAX_PAYLOAD_BYTES:
        raise PublicationError("input_payload_capacity")
    result = dict(zip(SCOPE, target.scope_tuple, strict=True))
    result.update({name: row[name] for name in ("ingest_id", *REPLAY, "subject_id", "pid", "record_kind")})
    result.update(payload=payload, payload_digest=hashlib.sha256(payload).digest())
    return result


def decode_input(row: Mapping[str, Any], scope: tuple[int, int, int, int]) -> dict[str, Any]:
    if set(row) != set(INPUT_COLUMNS) or tuple(row[name] for name in SCOPE) != scope:
        raise PublicationError("retained_input_scope_or_columns")
    for name in ("ingest_id", *REPLAY, "subject_id"):
        _integer(row[name], name, lower=1)
    _integer(row["pid"], "pid", lower=1, upper=(1 << 31) - 1)
    kind = _integer(row["record_kind"], "record_kind", lower=1, upper=9)
    if kind not in SOURCE_COLUMNS or type(row["payload"]) is not bytes or len(row["payload"]) > MAX_PAYLOAD_BYTES:
        raise PublicationError("invalid_retained_payload")
    if hashlib.sha256(row["payload"]).digest() != _bytes(row["payload_digest"], 32, "payload_digest"):
        raise PublicationError("retained_payload_digest_mismatch")
    packet = incident.decode_evidence_packet(row["payload"], max_bytes=MAX_PAYLOAD_BYTES)
    if type(packet) is not dict or set(packet) != {"projection_quality", "source"} or type(packet["source"]) is not dict:
        raise PublicationError("retained_payload_fields")
    source = packet["source"]
    if set(source) != set(SOURCE_COLUMNS[kind]) or any(type(value) is not int for value in source.values()):
        raise PublicationError("retained_source_fields")
    if _canonical(packet) != row["payload"]:
        raise PublicationError("retained_payload_not_canonical")
    if (source["environment_id"], source["season_id"]) != scope[2:] or any(
        source[name] != row[name] for name in ("ingest_id", *REPLAY, "subject_id", "pid", "record_kind")):
        raise PublicationError("retained_source_identity_conflict")
    _integer(source["schema_version"], "schema_version", lower=1, upper=1)
    _integer(source["config_id"], "config_id", lower=1)
    for name in ("classifier_version", "policy_version"):
        _integer(source[name], name, lower=1, upper=(1 << 32) - 1)
    for name in observations.DIMENSIONS:
        _integer(source[name], name, lower=-1 if name == "zone_vnum" else 0,
            upper=(1 << (31 if name == "zone_vnum" else 32 if name == "group_size" else 16)) - 1)
    _integer(source["quality_flags"], "raw_quality", upper=1023)
    quality = _integer(packet["projection_quality"], "projection_quality")
    if quality & ~ROLLUP_QUALITY_MASK:
        raise PublicationError("unknown_projection_quality")
    return dict(source, projection_quality=quality)


def initial_header(scope: tuple[int, int, int, int], watermark: int) -> dict[str, Any]:
    header = dict(zip(SCOPE, scope, strict=True))
    header.update({name: 0 for name in HEADER_COLUMNS[4:]})
    header.update(input_watermark=_integer(watermark, "watermark"), source_digest=seed_digest(scope))
    return header


def _session(row: Mapping[str, Any]) -> tuple[int, int, int]:
    return tuple(row[name] for name in observations.SESSION)


def _ownership_key(row: Mapping[str, Any]) -> tuple[Any, ...]:
    return _session(row), row["subject_id"], row["pid"]


def _group_mode(size: int) -> int:
    return 0 if size == 0 else 1 if size == 1 else 2


def _point(row):
    return observations.validate_observation({name: value for name, value in row.items() if name != "projection_quality"})


def _day(utc: int | None) -> date:
    if utc is None:
        return UNKNOWN_DAY
    try:
        value = date(1970, 1, 1) + timedelta(days=utc // DAY_USEC)
        return value if value > UNKNOWN_DAY else UNKNOWN_DAY
    except (ValueError, OverflowError):
        return UNKNOWN_DAY


def _identity_dimensions(row, day, partition, basis, token) -> tuple[Any, ...]:
    return (day, partition, BASES[basis], 0 if token is None else token, row["config_id"],
        row["classifier_version"], row["policy_version"], 0 if partition == 0 else row["faction_id"],
        0 if partition == 0 else row["level_band"], 0 if partition == 0 else _group_mode(row["group_size"]))


def _digest_cell(values: tuple[Any, ...]) -> bytes:
    return hashlib.sha256(_canonical([value.isoformat() if type(value) is date else value for value in values])).digest()


def _row(scope, columns, dimensions, fields, metrics):
    result = dict(zip(SCOPE, scope, strict=True))
    result.update(cell_digest=_digest_cell(dimensions), **dict(zip(fields, dimensions, strict=True)), **metrics)
    if set(result) != set(columns):
        raise PublicationError("publication_row_shape")
    return result


def _gap_windows(row, coverage, known_ownership):
    """A reviewed ownership loss lasts until an actually observed fresh anchor.

    Unknown ends remain unknown. An incident fix or reconstruction is not an
    authenticated ownership observation. Sequence-only/unknown clocks conservatively
    make the selected source uncertain rather than guessing a clock conversion.
    """
    first = row.get("start_utc_usec", row.get("at_utc_usec"))
    last = row.get("end_utc_usec", row.get("at_utc_usec"))
    comparable = (first != UTC_UNKNOWN and last != UTC_UNKNOWN and first is not None and last is not None
        and not row['projection_quality'] & identity.UTC_ATTRIBUTION_FLAGS)
    first_monotonic = row.get('start_monotonic_usec', row.get('at_monotonic_usec'))
    known_ownership = identity.ordered_ownership((row['environment_id'],row['season_id']),
        _session(row),row['subject_id'],row['pid'],(row['boot_id'],row['process_id']),known_ownership)
    result = []
    for item in coverage["incidents"]:
        if item["status"] != "active" or not item["record_kind_mask"] & (1 << 9):
            continue
        producer = item["producer_boot_id"], item["producer_process_id"]
        if producer != (None, None) and producer != (row["boot_id"], row["process_id"]):
            continue
        begin, finish = item["start_utc_usec"], item["end_utc_usec"]
        if comparable and ((begin is not None and begin > last) or (finish is not None and finish < first)):
            # A later anchor can close a known-ended loss, but a reviewed end
            # without a new ownership observation cannot restore attribution.
            if begin is not None and begin > last:
                continue
        recovered = None
        if finish is not None and comparable:
            anchors = [sample.at_utc_usec for sample in known_ownership if sample.account_token is not None and
                sample.replay[:2] == (row["boot_id"], row["process_id"]) and sample.at_utc_usec is not None and
                sample.at_utc_usec >= finish and (begin is None or sample.at_utc_usec >= begin) and
                sample.at_utc_usec == first + sample.at_monotonic_usec - first_monotonic and
                not sample.quality_flags & identity.UTC_ATTRIBUTION_FLAGS]
            recovered = min(anchors) if anchors else None
        if comparable and recovered is not None and recovered <= first:
            continue
        result.append((begin if comparable else None, recovered if comparable else None))
    return result


def _monotonic_gaps(interval, windows):
    result = []
    for first, last in windows:
        if not interval.comparable_utc:
            result.append((interval.start_monotonic_usec, interval.end_monotonic_usec))
            continue
        origin = interval.start_monotonic_usec - interval.start_utc_usec
        begin = interval.start_monotonic_usec if first is None else max(interval.start_monotonic_usec, first + origin)
        end = interval.end_monotonic_usec if last is None else min(interval.end_monotonic_usec, last + origin)
        if begin < end:
            result.append((begin, end))
    return tuple(result)


def _day_slices(row):
    interval = row.interval
    # Day boundaries are physical interval labels. An uncertain ownership
    # clock must not erase an otherwise covered character clock.
    if not interval.comparable_utc or _day(interval.start_utc_usec) == UNKNOWN_DAY or _day(interval.end_utc_usec - 1) == UNKNOWN_DAY:
        return (row,)
    first = interval.start_utc_usec + row.start_monotonic_usec - interval.start_monotonic_usec
    last = first + row.duration_usec
    result = []
    at = first
    while at < last:
        end = min(last, (at // DAY_USEC + 1) * DAY_USEC)
        offset = at - first
        result.append(replace(row, start_monotonic_usec=row.start_monotonic_usec + offset,
            end_monotonic_usec=row.start_monotonic_usec + end - first,
            start_utc_usec=None if row.start_utc_usec is None else at,
            end_utc_usec=None if row.end_utc_usec is None else end))
        at = end
    return tuple(result)


def build_publication(scope: tuple[int, int, int, int], header: Mapping[str, Any],
                      inputs: Sequence[Mapping[str, Any]], registry: identity.Registry | None,
                      incident_coverage: Mapping[str, Any], *, max_output_rows: int = 2_000,
                      check_deadline: Callable[[], None] = lambda: None):
    """Build complete bounded aggregates; any conflict refuses all publication."""
    identity.generation_scope(scope)
    if scope[0] != DEFINITION_VERSION or len(inputs) > MAX_INPUTS or type(max_output_rows) is not int or max_output_rows <= 0:
        raise PublicationError("publication_capacity_or_definition")
    if set(header) != set(HEADER_COLUMNS) or tuple(header[name] for name in SCOPE) != scope or header["publication_complete"] != 0:
        raise PublicationError("invalid_building_header")
    if registry is not None and (registry.environment_id, registry.season_id) != scope[2:]:
        raise PublicationError("registry_scope_mismatch")
    if incident_coverage["registry_schema_version"] != 2:
        raise PublicationError("ownership_incident_schema_required")
    digest, previous = seed_digest(scope), 0
    facts = []
    for source in inputs:
        check_deadline()
        if source["ingest_id"] <= previous or source["ingest_id"] > header["input_watermark"]:
            raise PublicationError("retained_input_order_or_watermark")
        previous = source["ingest_id"]
        facts.append(decode_input(source, scope))
        digest = advance_digest(digest, source)
    if len(inputs) != header["source_fact_count"] or digest != _bytes(header["source_digest"], 32, "source_digest"):
        raise PublicationError("retained_input_incomplete_or_changed")
    ownership = defaultdict(list)
    for row in facts:
        if row["record_kind"] != 9:
            continue
        point = _point(row)
        _day_value, quality, _utc = observations.point_day(point)
        quality |= row["projection_quality"]
        ownership[_ownership_key(row)].append(identity.OwnershipObservation(scope[2:], _session(row),
            tuple(row[name] for name in REPLAY), row["subject_id"], row["pid"], point["at_monotonic_usec"],
            point["account_token"] or None, OWNERSHIP_SOURCES[point["source"]],
            None if point["at_utc_usec"] == UTC_UNKNOWN else point["at_utc_usec"], quality))
    for rows in ownership.values():
        if len(rows) > identity.MAX_OWNERSHIP_OBSERVATIONS:
            raise PublicationError("ownership_capacity")
    slices = []
    rows_by_replay = {}
    counts = {name: 0 for name in HEADER_COLUMNS[8:]}
    counts["quality_flags"] = int(header["quality_flags"]) | int(incident_coverage["quality_flags"])
    xp_cells = {}
    for row in facts:
        check_deadline()
        kind = row["record_kind"]
        rows_by_replay[tuple(row[name] for name in REPLAY)] = row
        counts["quality_flags"] |= row["projection_quality"]
        if kind == 9:
            counts["ownership_count"] += 1
            continue
        samples = ownership[_ownership_key(row)]
        windows = _gap_windows(row, incident_coverage, samples)
        if kind == 1:
            counts["interval_count"] += 1
            duration = _integer(row["duration_usec"], "duration_usec", lower=1)
            if row["end_monotonic_usec"] - row["start_monotonic_usec"] != duration:
                raise PublicationError("interval_duration_conflict")
            category = {0: "unknown", 1: "idle", 2: "active", 3: "resident_linkdead"}.get(row["category"])
            if category is None:
                raise PublicationError("interval_category")
            interval = identity.ObservedInterval(scope[2:], _session(row), tuple(row[name] for name in REPLAY),
                row["subject_id"], row["pid"], row["config_id"], category, row["start_monotonic_usec"], row["end_monotonic_usec"],
                None if row["start_utc_usec"] == UTC_UNKNOWN else row["start_utc_usec"],
                None if row["end_utc_usec"] == UTC_UNKNOWN else row["end_utc_usec"], row["projection_quality"])
            if interval.comparable_utc and (
                _day(interval.start_utc_usec) == UNKNOWN_DAY or _day(interval.end_utc_usec - 1) == UNKNOWN_DAY or
                (interval.end_utc_usec - 1) // DAY_USEC - interval.start_utc_usec // DAY_USEC + 1 > MAX_DAY_SLICES):
                interval = replace(interval, quality_flags=interval.quality_flags | ROLLUP_QUALITY_UTC_FANOUT)
            attributed = identity.attribute_interval(interval, samples, registry,
                ownership_gap_windows=_monotonic_gaps(interval, windows))
            for piece in attributed:
                slices.extend(_day_slices(piece))
                counts["observed_character_usec"] = observations.checked_add(counts["observed_character_usec"], piece.duration_usec, "observed_effort")
                field = "unknown_account_character_usec" if piece.account_token is None else "owned_character_usec"
                counts[field] = observations.checked_add(counts[field], piece.duration_usec, field)
                field = "unlinked_controller_character_usec" if piece.controller_token is None else "confirmed_controller_character_usec"
                counts[field] = observations.checked_add(counts[field], piece.duration_usec, field)
                if piece.quality_flags & ROLLUP_QUALITY_INCIDENT_GAP:
                    counts["incident_affected_character_usec"] = observations.checked_add(counts["incident_affected_character_usec"], piece.duration_usec, "incident_effort")
                counts["quality_flags"] |= piece.attribution_quality
                if len(slices) > identity.MAX_EFFORT_SLICES:
                    raise PublicationError("effort_slice_capacity")
        else:
            counts["progression_count"] += 1
            point = _point(row)
            if point["kind"] != 1:
                continue  # Level threshold consumption is never an XP reward.
            day, quality, utc = observations.point_day(point)
            quality |= row["projection_quality"]
            ordered = identity.ordered_ownership(scope[2:], _session(row), row["subject_id"], row["pid"],
                (row["boot_id"], row["process_id"]), samples)
            before = [sample for sample in ordered if sample.at_monotonic_usec <= point["at_monotonic_usec"]]
            sample = before[-1] if before else None
            account = None if sample is None else sample.account_token
            if sample is not None:
                quality |= sample.quality_flags
                if utc is not None and sample.at_utc_usec is not None and (
                    sample.at_utc_usec + point["at_monotonic_usec"] - sample.at_monotonic_usec != utc):
                    quality |= ROLLUP_QUALITY_UTC_MISMATCH
            affected = bool(windows) and (utc is None or any((first is None or first <= utc) and (last is None or utc < last) for first, last in windows))
            if affected:
                account = None
                quality |= ROLLUP_QUALITY_INCIDENT_GAP
            controller, _association, _status = identity.linkage_at(registry, account,
                None if quality & identity.UTC_ATTRIBUTION_FLAGS else utc)
            counts["quality_flags"] |= quality
            for partition in (0, 1):
                for basis, token in (("character", row["subject_id"]), ("account" if account else "unknown_account", account),
                                     ("controller" if controller else "unknown_controller", controller)):
                    dimensions = (*_identity_dimensions(row, day, partition, basis, token),
                        point["source"], point["reason"], point["observation_status"], point["modifier_flags"])
                    cell = xp_cells.setdefault(dimensions, {name: 0 for name in (*observations.XP_METRICS,
                        "linked_observations", "clock_unknown_observations", "incident_affected_observations", "quality_flags")})
                    amounts = dict(observations=1, zero_applied_observations=int(point["applied_xp"] == 0),
                        negative_applied_observations=int(point["applied_xp"] < 0), requested_xp=point["requested_xp"],
                        computed_xp=point["computed_xp"], applied_xp=point["applied_xp"],
                        earned_positive_xp=max(0, point["applied_xp"]) if point["reason"] == 1 and 1 <= point["source"] <= 10 else 0,
                        death_loss_xp=max(0, -point["applied_xp"]) if point["reason"] == 2 else 0,
                        restored_positive_xp=max(0, point["applied_xp"]) if point["reason"] == 3 else 0,
                        linked_observations=int(controller is not None), clock_unknown_observations=int(bool(quality & identity.UTC_ATTRIBUTION_FLAGS)),
                        incident_affected_observations=int(affected))
                    for name, amount in amounts.items():
                        cell[name] = observations.checked_add(cell[name], amount, name, signed=name in ("requested_xp", "computed_xp", "applied_xp"))
                    cell["quality_flags"] |= quality
            if len(xp_cells) > max_output_rows:
                raise PublicationError("publication_output_capacity")
    # Check source interval overlaps across the entire generation before cells
    # divide the data. Day/config/cohort partitioning cannot hide a conflict.
    identity.union_effort(slices)
    groups = defaultdict(list)
    for piece in slices:
        source = rows_by_replay[piece.interval.replay]
        physical_utc = None if not piece.interval.comparable_utc else (
            piece.interval.start_utc_usec + piece.start_monotonic_usec - piece.interval.start_monotonic_usec)
        day = _day(physical_utc)
        for partition in (0, 1):
            # Identity is grouped by union_effort within each compatible cell.
            key = _identity_dimensions(source, day, partition, "character", 1)
            groups[(key[0], key[1], *key[4:], piece.interval.category)].append(piece)
    effort_rows = []
    for (day, partition, config, classifier, policy, faction, band, mode, category), pieces in groups.items():
        check_deadline()
        for total in identity.union_effort(pieces, category=category, include_characters=True):
            dimensions = (day, partition, BASES[total.basis], total.token or 0, config, classifier, policy, faction, band, mode)
            metrics = dict(category=CATEGORIES[category], character_usec=total.character_usec,
                utc_covered_character_usec=total.covered_character_usec, unknown_clock_character_usec=total.unknown_clock_character_usec,
                covered_union_usec=total.covered_union_usec, union_usec=total.union_usec,
                distinct_characters=total.distinct_characters, distinct_accounts=total.distinct_accounts,
                quality_flags=total.quality_flags)
            # Category is part of the retained cell identity as well as a metric.
            output = _row(scope, EFFORT_COLUMNS, dimensions, CELL_DIMENSIONS, metrics)
            output["cell_digest"] = _digest_cell((*dimensions, CATEGORIES[category]))
            effort_rows.append(output)
        if len(effort_rows) + len(xp_cells) > max_output_rows:
            raise PublicationError("publication_output_capacity")
    xp_rows = [_row(scope, XP_COLUMNS, dimensions, XP_DIMENSIONS, metrics) for dimensions, metrics in xp_cells.items()]
    counts.update(effort_row_count=len(effort_rows), portfolio_row_count=len(xp_rows), effort_slice_count=len(slices))
    counts["quality_flags"] = _integer(counts["quality_flags"], "publication_quality")
    if counts["quality_flags"] & ~ROLLUP_QUALITY_MASK:
        raise PublicationError("unknown_publication_quality")
    result = dict(header, **counts, publication_complete=1)
    if result["source_fact_count"] != result["interval_count"] + result["ownership_count"] + result["progression_count"]:
        raise PublicationError("identity_source_count_conflict")
    if result["observed_character_usec"] != result["owned_character_usec"] + result["unknown_account_character_usec"] or (
        result["observed_character_usec"] != result["confirmed_controller_character_usec"] + result["unlinked_controller_character_usec"]):
        raise PublicationError("identity_coverage_conservation")
    return result, effort_rows, xp_rows


def public_header(row: Mapping[str, Any], reservation: Mapping[str, Any]) -> dict[str, Any]:
    scope = tuple(row[name] for name in SCOPE)
    if set(row) != set(HEADER_COLUMNS) or scope[0] != DEFINITION_VERSION or row["publication_complete"] != 1:
        raise PublicationError("identity_publication_incomplete")
    for name in HEADER_COLUMNS[4:]:
        if name != "source_digest":
            _integer(row[name], name)
    _bytes(row["source_digest"], 32, "source_digest")
    if row["source_fact_count"] != row["interval_count"] + row["ownership_count"] + row["progression_count"] or row["quality_flags"] & ~ROLLUP_QUALITY_MASK:
        raise PublicationError("identity_source_count_or_quality_conflict")
    if row["observed_character_usec"] != row["owned_character_usec"] + row["unknown_account_character_usec"] or (
        row["observed_character_usec"] != row["confirmed_controller_character_usec"] + row["unlinked_controller_character_usec"]):
        raise PublicationError("identity_coverage_conservation")
    metadata = identity.public_generation(reservation)
    if tuple(reservation[name] for name in SCOPE) != scope:
        raise PublicationError("published_identity_scope_conflict")
    metadata.update(status="published_reviewed_version" if reservation["registry_version"] is not None else "published_unknown_identity",
        balance_report_published=True, complete_identity_coverage_implied=False)
    return dict(row, source_digest=row["source_digest"].hex(), identity=metadata,
        source_input_retention_complete=True, controller_population_complete_implied=False,
        economic_rewards_included=False, rates_computed=False,
        partitions={"0": "portfolio_per_configuration_day", "1": "faction_level_band_observed_group_context"},
        union_cells_additive=False)
