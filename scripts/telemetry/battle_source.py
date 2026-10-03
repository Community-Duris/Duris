"""Exact bounded retained inputs for atomic shared-battle publication.

This is the value contract consumed by the publication transaction. It validates
canonical kinds 9/10/11, ties selected inputs to an ingestion checkpoint and
refuses changed, missing or conflicting source. It performs no SQL, authorizes
no private review and does not activate a report definition or imply publication.
"""
from __future__ import annotations

from dataclasses import dataclass
import hashlib
import json
from typing import Any, Callable, Mapping, Sequence

try:
    from . import battle_contract as battle, battle_contribution_contract as contribution
    from . import identity_history as identity, identity_publication, observation_semantics as observations, incident
    from .battle_history import HEADER
    from .rollup_definitions import PREPARATION_DEFINITION_VERSION, ROLLUP_QUALITY_MASK
except ImportError:
    import battle_contract as battle
    import battle_contribution_contract as contribution
    import identity_history as identity
    import identity_publication
    import observation_semantics as observations
    import incident
    from battle_history import HEADER
    from rollup_definitions import PREPARATION_DEFINITION_VERSION, ROLLUP_QUALITY_MASK

DEFINITION_VERSION = PREPARATION_DEFINITION_VERSION  # Independent incident schema 4.
SCOPE = observations.SCOPE
REPLAY = identity_publication.REPLAY
MAX_INPUTS = 16_384
MAX_PAYLOAD_BYTES = 8_192
INPUT_ROW_BYTE_BOUND = 12_288
PUBLICATION_INPUT_BYTE_BOUND = 32_768
HEADER_BYTE_BOUND = 4_096
DEFAULT_BYTE_LIMIT = 32 * 1024 * 1024
INPUT_COLUMNS = (*SCOPE, "ingest_id", *REPLAY, "record_kind", "payload", "payload_digest")
HEADER_COLUMNS = (*SCOPE, "input_origin", "input_watermark", "source_fact_count", "source_digest",
    "ownership_count", "association_count", "contribution_count", "quality_flags", "publication_complete")
SOURCE_COLUMNS = {
    9: (*identity_publication.SOURCE_COLUMNS[9], "ingested_utc_usec"),
    10: ("ingest_id", *HEADER, "ingested_utc_usec", *battle.FIELDS),
    11: ("ingest_id", *HEADER, "ingested_utc_usec", *contribution.FIELDS),
}
SOURCE_SCOPE = {9: ("environment_id", "season_id"),
    10: ("battle_environment_id", "battle_season_id"), 11: ("bc_environment_id", "bc_season_id")}
COUNTS = {9: "ownership_count", 10: "association_count", 11: "contribution_count"}


class SourceError(ValueError):
    """A payload-free retained-source, cursor or capacity failure."""


def _require(condition: bool, reason: str) -> None:
    if not condition:
        raise SourceError(reason)


def _integer(value: Any, *, lower=0, upper=battle.UINT64_MAX) -> int:
    _require(type(value) is int and lower <= value <= upper, "battle_source_integer_range")
    return value


def _scope(value: Any) -> tuple[int, int, int, int]:
    try:
        identity.generation_scope(value)
    except identity.IdentityError as error:
        raise SourceError("battle_source_generation_scope") from error
    _require(value[0] == DEFINITION_VERSION, "battle_source_definition")
    return value


def _bytes(value: Any, size: int) -> bytes:
    _require(type(value) is bytes and len(value) == size, "battle_source_digest_shape")
    return value


def _quality(value: Any) -> int:
    _integer(value)
    _require(value & ~ROLLUP_QUALITY_MASK == 0, "battle_source_projection_quality")
    return value


def _canonical(value: Any) -> bytes:
    return json.dumps(value, sort_keys=True, separators=(",", ":"), allow_nan=False).encode("utf-8")


def seed_digest(scope: tuple[int, int, int, int], origin: int = 0) -> bytes:
    return hashlib.sha256(b"duris-battle-source-v2:" + _canonical(_scope(scope)) + _integer(origin).to_bytes(8, "big")).digest()


def advance_digest(digest: bytes, row: Mapping[str, Any]) -> bytes:
    return hashlib.sha256(_bytes(digest, 32) + _integer(row["ingest_id"], lower=1).to_bytes(8, "big") +
        _bytes(row["payload_digest"], 32)).digest()


def _source(row: Mapping[str, Any], scope: tuple[int, int, int, int], *, exact: bool) -> dict[str, int]:
    _require(isinstance(row, Mapping), "battle_source_mapping")
    kind = _integer(row.get("record_kind"), lower=9, upper=11)
    if exact:
        _require(set(row) == set(SOURCE_COLUMNS[kind]), "battle_source_exact_fields")
    try:
        if kind == 10:
            battle.validate_raw_fact(row)
        elif kind == 11:
            contribution.validate_raw_segment(row)
        else:
            observations.validate_observation(row)
    except (battle.BattleContractError, contribution.ContributionContractError, observations.ObservationError) as error:
        raise SourceError(str(error)) from error
    _integer(row.get("ingest_id"), lower=1)
    _require(tuple(row[name] for name in SOURCE_SCOPE[kind]) == scope[2:], "battle_source_environment_season")
    value = {name: row.get(name) for name in SOURCE_COLUMNS[kind]}
    # Export-only fixtures do not have a durable ingestion timestamp. Preserve
    # that absence explicitly; never turn the producer's UTC label into arrival.
    ingested = row.get("ingested_utc_usec")
    value["ingested_utc_usec"] = contribution.UTC_UNKNOWN if ingested is None else _integer(
        ingested, lower=-(1 << 63), upper=(1 << 63) - 1)
    _require(all(type(item) is int for item in value.values()), "battle_source_field_types")
    return value


def retain_input(row: Mapping[str, Any], scope: tuple[int, int, int, int], quality: int) -> dict[str, Any]:
    scope = _scope(scope)
    source = _source(row, scope, exact=False)
    payload = _canonical({"projection_quality": _quality(quality), "source": source})
    _require(len(payload) <= MAX_PAYLOAD_BYTES, "battle_source_payload_capacity")
    result = dict(zip(SCOPE, scope, strict=True))
    result.update({name: source[name] for name in ("ingest_id", *REPLAY, "record_kind")})
    result.update(payload=payload, payload_digest=hashlib.sha256(payload).digest())
    return result


@dataclass(frozen=True, slots=True)
class DecodedInput:
    source: Mapping[str, int]
    projection_quality: int


def decode_input(row: Mapping[str, Any], scope: tuple[int, int, int, int]) -> DecodedInput:
    scope = _scope(scope)
    _require(isinstance(row, Mapping) and set(row) == set(INPUT_COLUMNS) and
        tuple(row[name] for name in SCOPE) == scope, "battle_source_retained_scope_or_columns")
    for name in ("ingest_id", *REPLAY):
        _integer(row[name], lower=1)
    _integer(row["record_kind"], lower=9, upper=11)
    payload = row["payload"]
    _require(type(payload) is bytes and 0 < len(payload) <= MAX_PAYLOAD_BYTES, "battle_source_payload_capacity")
    _require(hashlib.sha256(payload).digest() == _bytes(row["payload_digest"], 32), "battle_source_payload_digest")
    try:
        packet = incident.decode_evidence_packet(payload, max_bytes=MAX_PAYLOAD_BYTES)
    except incident.IncidentError as error:
        raise SourceError("battle_source_payload_json") from error
    _require(type(packet) is dict and set(packet) == {"projection_quality", "source"} and
        type(packet["source"]) is dict, "battle_source_payload_fields")
    _require(_canonical(packet) == payload, "battle_source_payload_canonical")
    source = _source(packet["source"], scope, exact=True)
    _require(all(source[name] == row[name] for name in ("ingest_id", *REPLAY, "record_kind")), "battle_source_payload_identity")
    return DecodedInput(source, _quality(packet["projection_quality"]))


def initial_header(scope: tuple[int, int, int, int], watermark: int = 0) -> dict[str, Any]:
    result = dict(zip(SCOPE, _scope(scope), strict=True))
    result.update({name: 0 for name in HEADER_COLUMNS[4:]})
    result.update(input_origin=_integer(watermark), input_watermark=watermark, source_digest=seed_digest(scope, watermark))
    return result


def _header(header: Mapping[str, Any]) -> tuple[int, int, int, int]:
    _require(isinstance(header, Mapping) and set(header) == set(HEADER_COLUMNS), "battle_source_header_fields")
    scope = _scope(tuple(header[name] for name in SCOPE))
    _integer(header["input_origin"])
    _integer(header["input_watermark"])
    _integer(header["source_fact_count"], upper=MAX_INPUTS)
    for name in COUNTS.values():
        _integer(header[name], upper=MAX_INPUTS)
    _quality(header["quality_flags"])
    _integer(header["publication_complete"], upper=1)
    _bytes(header["source_digest"], 32)
    _require(header["input_origin"] <= header["input_watermark"] and sum(header[name] for name in COUNTS.values()) ==
        header["source_fact_count"] <= header["input_watermark"] - header["input_origin"],
        "battle_source_header_count")
    return scope


def _reservation(inputs: Sequence[Mapping[str, Any]], max_total_bytes: int, check_deadline: Callable[[], None]) -> int:
    _require(isinstance(inputs, (tuple, list)) and len(inputs) <= MAX_INPUTS, "battle_source_input_capacity")
    _integer(max_total_bytes, lower=1)
    _require(callable(check_deadline), "battle_source_deadline")
    check_deadline()
    reserved = HEADER_BYTE_BOUND + len(inputs) * PUBLICATION_INPUT_BYTE_BOUND
    _require(reserved <= max_total_bytes, "battle_source_byte_capacity")
    return reserved


def advance_header(header: Mapping[str, Any], inputs: Sequence[Mapping[str, Any]], watermark: int, *,
                   max_total_bytes: int = DEFAULT_BYTE_LIMIT, check_deadline: Callable[[], None] = lambda: None) -> dict[str, Any]:
    """Value update for the transaction retaining selected inputs and its cursor."""
    scope = _header(header)
    _require(header["publication_complete"] == 0, "battle_source_published_immutable")
    _reservation(inputs, max_total_bytes, check_deadline)
    _integer(watermark)
    _require(watermark >= header["input_watermark"] and header["source_fact_count"] + len(inputs) <= MAX_INPUTS,
        "battle_source_cursor_or_capacity")
    result, previous, receipts = dict(header), header["input_watermark"], set()
    for row in inputs:
        check_deadline()
        decoded = decode_input(row, scope)
        _require(previous < row["ingest_id"] <= watermark, "battle_source_page_cursor")
        previous = row["ingest_id"]
        receipt = tuple(row[name] for name in REPLAY)
        _require(receipt not in receipts, "battle_source_duplicate_receipt")
        receipts.add(receipt)
        result["source_digest"] = advance_digest(result["source_digest"], row)
        result[COUNTS[row["record_kind"]]] += 1
        result["quality_flags"] |= decoded.projection_quality
    result.update(input_watermark=watermark, source_fact_count=result["source_fact_count"] + len(inputs))
    _header(result)
    return result


@dataclass(frozen=True, slots=True)
class VerifiedSource:
    header: Mapping[str, Any]
    facts: tuple[Mapping[str, int], ...]
    projection_qualities: tuple[int, ...]
    reserved_bytes: int


def verify_source(header: Mapping[str, Any], inputs: Sequence[Mapping[str, Any]], *,
                  expected_scope: tuple[int, int, int, int], expected_watermark: int, expected_origin: int = 0,
                  max_total_bytes: int = DEFAULT_BYTE_LIMIT,
                  check_deadline: Callable[[], None] = lambda: None) -> VerifiedSource:
    """Verify the exact retained checkpoint against the locked generation state.

    Cursor advancement over unselected raw families does not manufacture source
    facts. A successful value check is not a persisted or atomic publication.
    The SQL caller reserves its buffering fetch before invoking this function.
    """
    scope = _header(header)
    _require(scope == _scope(expected_scope) and header["input_watermark"] == _integer(expected_watermark) and
        header["input_origin"] == _integer(expected_origin),
        "battle_source_state_checkpoint")
    reserved = _reservation(inputs, max_total_bytes, check_deadline)
    digest, previous, receipts = seed_digest(scope, expected_origin), expected_origin, set()
    counts, quality, facts, qualities = dict.fromkeys(COUNTS.values(), 0), 0, [], []
    for row in inputs:
        check_deadline()
        decoded = decode_input(row, scope)
        _require(previous < row["ingest_id"] <= expected_watermark, "battle_source_retained_order_or_cursor")
        previous = row["ingest_id"]
        receipt = tuple(row[name] for name in REPLAY)
        _require(receipt not in receipts, "battle_source_duplicate_receipt")
        receipts.add(receipt)
        counts[COUNTS[row["record_kind"]]] += 1
        quality |= decoded.projection_quality
        facts.append(dict(decoded.source))
        qualities.append(decoded.projection_quality)
        digest = advance_digest(digest, row)
    _require(len(inputs) == header["source_fact_count"] and digest == header["source_digest"] and
        all(counts[name] == header[name] for name in COUNTS.values()), "battle_source_missing_or_changed")
    _require(quality & ~header["quality_flags"] == 0, "battle_source_header_quality")
    return VerifiedSource(dict(header), tuple(facts), tuple(qualities), reserved)
