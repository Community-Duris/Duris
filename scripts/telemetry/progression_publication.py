"""Bounded retained progression inputs for independent definition 9.

These are mutable-state observations, not committed rewards or character saves.
Definition 9 uses the existing cursor and publication transactions. Earlier
definitions do not select these inputs or consult this contract.
"""
from __future__ import annotations

from dataclasses import dataclass, replace
from collections import defaultdict
from bisect import bisect_right
import hashlib
import json
from typing import Any, Callable, Mapping

try:
    from . import identity_history as identity, identity_publication, incident
    from . import observation_semantics as observations, progression_context_contract as context
    from .rollup_definitions import ROLLUP_QUALITY_MASK, UINT64_MAX, UTC_UNKNOWN
except ImportError:
    import identity_history as identity
    import identity_publication
    import incident
    import observation_semantics as observations
    import progression_context_contract as context
    from rollup_definitions import ROLLUP_QUALITY_MASK, UINT64_MAX, UTC_UNKNOWN

DEFINITION_VERSION = 9
SCOPE = observations.SCOPE
REPLAY = identity_publication.REPLAY
MAX_INPUTS = 16_384
MAX_PAYLOAD_BYTES = 8_192
INPUT_ROW_BYTE_BOUND = 12_288
PUBLICATION_INPUT_BYTE_BOUND = 32_768
HEADER_BYTE_BOUND = 4_096
DEFAULT_BYTE_LIMIT = 32 * 1024 * 1024
INPUT_COLUMNS = (*SCOPE, "ingest_id", *REPLAY, "record_kind", "payload", "payload_digest")
COUNTS = {1: "interval_count", 2: "lifecycle_count", 6: "progression_count", 9: "ownership_count",
          15: "context_count", 16: "configuration_chunk_count"}
HEADER_COLUMNS = (*SCOPE, "input_origin", "input_watermark", "source_fact_count", "source_digest",
                  *COUNTS.values(), "reference_count", "reference_digest", "quality_flags", "publication_complete")
RAW_HEADER = ("schema_version", "record_kind", "occurrence_utc_usec")
SOURCE_COLUMNS = {
    **{kind: (*identity_publication.SOURCE_COLUMNS[kind], "ingested_utc_usec") for kind in (6, 9)},
    1: (*identity_publication.SOURCE_COLUMNS[1], "connection_boot_id", "connection_process_id",
        "connection_seq", "ingested_utc_usec"),
    2: (*identity_publication.COMMON_SOURCE, *identity_publication.POINT_SOURCE,
        "lifecycle_kind", "end_reason", "ingested_utc_usec"),
    15: ("ingest_id", *REPLAY, *RAW_HEADER, "ingested_utc_usec", *context.FIELDS),
    16: ("ingest_id", *REPLAY, *RAW_HEADER, "ingested_utc_usec", *(name for name, _, _ in context.CONFIGURATION_CHUNK_LAYOUT)),
}
SOURCE_SCOPE = {kind: ("environment_id", "season_id") for kind in (1, 2, 6, 9)} | {
    15: ("pctx_environment_id", "pctx_season_id"), 16: ("pcfg_environment_id", "pcfg_season_id")}
CONFIG_COLUMNS = ("config_id", "environment_id", "season_id", "build_version", "content_version",
                  "classifier_version", "policy_version")


class PublicationError(ValueError):
    """Payload-free refusal of conflicting, changed or excessive evidence."""


def _require(condition, reason):
    if not condition:
        raise PublicationError(reason)


def _integer(value, *, lower=0, upper=UINT64_MAX):
    _require(type(value) is int and lower <= value <= upper, "progression_integer_range")
    return value


def _scope(scope):
    identity.generation_scope(scope)
    _require(scope[0] == DEFINITION_VERSION, "progression_definition")
    return scope


def _canonical(value):
    return json.dumps(value, sort_keys=True, separators=(",", ":"), allow_nan=False).encode("utf-8")


def _quality(value):
    _integer(value)
    _require(not value & ~ROLLUP_QUALITY_MASK, "progression_projection_quality")
    return value


def _digest(value):
    _require(type(value) is bytes and len(value) == 32, "progression_digest_shape")
    return value


def seed_digest(scope, origin=0):
    return hashlib.sha256(b"duris-progression-source-v1:" + _canonical(_scope(scope)) +
                          _integer(origin).to_bytes(8, "big")).digest()


def advance_digest(digest, row):
    return hashlib.sha256(_digest(digest) + _integer(row["ingest_id"], lower=1).to_bytes(8, "big") +
                          _digest(row["payload_digest"])).digest()


def _source(row, scope, *, exact):
    kind = row.get("record_kind")
    _require(type(kind) is int and kind in SOURCE_COLUMNS, "progression_source_family")
    if not exact and kind == 2 and "lifecycle" in row:
        # Native SQL names the family-2 field lifecycle. Retained definition-9
        # values use lifecycle_kind; decoding remains strictly canonical.
        _require("lifecycle_kind" not in row, "progression_lifecycle_alias_conflict")
        row = dict(row)
        row["lifecycle_kind"] = row.pop("lifecycle")
    if exact:
        _require(set(row) == set(SOURCE_COLUMNS[kind]), "progression_source_columns")
    else:
        _require(all(value is None or name in SOURCE_COLUMNS[kind] for name, value in row.items()),
                 "progression_inactive_source_fields")
    source = {name: row.get(name) for name in SOURCE_COLUMNS[kind]}
    source["ingested_utc_usec"] = UTC_UNKNOWN if row.get("ingested_utc_usec") is None else row["ingested_utc_usec"]
    _require(all(type(value) is int for value in source.values()), "progression_source_field_type")
    for name in ("ingest_id", *REPLAY):
        _integer(source[name], lower=1)
    for name, value in source.items():
        _integer(value, lower=-(1 << 63))
    _require(source["schema_version"] == 1 and tuple(source[name] for name in SOURCE_SCOPE[kind]) == scope[2:],
             "progression_source_scope_or_schema")
    try:
        if kind in (15, 16):
            context.validate_raw_observation(row)
        elif kind in (6, 9):
            observations.validate_observation(source)
        else:
            for name in ("environment_id", "season_id", *observations.SESSION, "subject_id", "pid", "config_id",
                         "classifier_version", "policy_version"):
                _integer(source[name], lower=1)
            _integer(source["pid"], lower=1, upper=(1 << 31) - 1)
            for name in ("classifier_version", "policy_version"):
                _integer(source[name], lower=1, upper=(1 << 32) - 1)
            for name in observations.DIMENSIONS:
                _integer(source[name], lower=-1 if name == "zone_vnum" else 0,
                         upper=(1 << (31 if name == "zone_vnum" else 32 if name == "group_size" else 16)) - 1)
            _integer(source["occurrence_utc_usec"], lower=-(1 << 63), upper=(1 << 63) - 1)
            _integer(source["quality_flags"], upper=1023)
            if kind == 1:
                connection = tuple(source["connection_" + name] for name in ("boot_id", "process_id", "seq"))
                _require(all(value > 0 for value in connection) or connection == (0, 0, 0),
                         "progression_interval_connection")
                for name in ("start_monotonic_usec", "end_monotonic_usec"):
                    _integer(source[name])
                for name in ("start_utc_usec", "end_utc_usec"):
                    _integer(source[name], lower=-(1 << 63), upper=(1 << 63) - 1)
                _require(source["category"] in (0, 1, 2, 3) and source["context"] in range(9) and
                    source["context_quality"] in range(5) and source["duration_usec"] > 0 and
                    source["end_monotonic_usec"] - source["start_monotonic_usec"] == source["duration_usec"],
                    "progression_interval_shape")
            else:
                _require(source["lifecycle_kind"] in (1, 2, 3, 4) and source["end_reason"] in range(6),
                         "progression_lifecycle_shape")
                _integer(source["at_monotonic_usec"])
                _integer(source["at_utc_usec"], lower=-(1 << 63), upper=(1 << 63) - 1)
                connection = tuple(source["connection_" + name] for name in ("boot_id", "process_id", "seq"))
                _require(all(value > 0 for value in connection) or connection == (0, 0, 0),
                         "progression_lifecycle_connection")
                _require((source["lifecycle_kind"] == 2 and source["end_reason"] != 0) or
                         (source["lifecycle_kind"] != 2 and source["end_reason"] == 0 and connection != (0, 0, 0)),
                         "progression_lifecycle_reason")
    except (context.ContextContractError, observations.ObservationError) as error:
        raise PublicationError(str(error)) from error
    return source


def _configuration(configuration, source):
    if configuration is None:
        return None
    kind = source["record_kind"]
    _require(kind in (15, 16) and set(configuration) == set(CONFIG_COLUMNS), "progression_configuration_columns")
    value = {name: _integer(configuration[name], lower=1) for name in CONFIG_COLUMNS}
    prefix = "pctx_" if kind == 15 else "pcfg_"
    _require(all(value[name] == source[prefix + name] for name in CONFIG_COLUMNS), "progression_configuration_conflict")
    return value


def retain_input(row, scope, quality, *, configuration=None):
    source = _source(row, _scope(scope), exact=False)
    packet = {"source": source, "projection_quality": _quality(quality)}
    if source["record_kind"] in (15, 16):
        packet["configuration"] = _configuration(configuration, source)
    else:
        _require(configuration is None, "progression_configuration_family")
    payload = _canonical(packet)
    _require(len(payload) <= MAX_PAYLOAD_BYTES, "progression_source_payload_capacity")
    return dict(zip(SCOPE, scope, strict=True), **{name: source[name] for name in ("ingest_id", *REPLAY, "record_kind")},
                payload=payload, payload_digest=hashlib.sha256(payload).digest())


@dataclass(frozen=True, slots=True)
class DecodedInput:
    source: Mapping[str, Any]
    projection_quality: int
    configuration: Mapping[str, int] | None


def decode_input(row, scope):
    _scope(scope)
    _require(set(row) == set(INPUT_COLUMNS) and tuple(row[name] for name in SCOPE) == scope,
             "progression_retained_scope_or_columns")
    payload = row["payload"]
    _require(type(payload) is bytes and 0 < len(payload) <= MAX_PAYLOAD_BYTES and
             hashlib.sha256(payload).digest() == _digest(row["payload_digest"]), "progression_retained_payload_digest")
    try:
        packet = incident.decode_evidence_packet(payload, max_bytes=MAX_PAYLOAD_BYTES)
    except incident.IncidentError as error:
        raise PublicationError("progression_retained_json") from error
    fields = {"source", "projection_quality"} | ({"configuration"} if row["record_kind"] in (15, 16) else set())
    _require(type(packet) is dict and set(packet) == fields and type(packet["source"]) is dict and
             _canonical(packet) == payload, "progression_retained_canonical_columns")
    source = _source(packet["source"], scope, exact=True)
    _require(all(row[name] == source[name] for name in ("ingest_id", *REPLAY, "record_kind")),
             "progression_retained_identity_conflict")
    return DecodedInput(source, _quality(packet["projection_quality"]), _configuration(packet.get("configuration"), source))


def initial_header(scope, watermark=0):
    result = dict(zip(SCOPE, _scope(scope), strict=True))
    result.update({name: 0 for name in HEADER_COLUMNS[4:]})
    result.update(input_origin=_integer(watermark), input_watermark=watermark, source_digest=seed_digest(scope, watermark),
                  reference_digest=_reference_digest(scope, ()))
    return result


def validate_header(header):
    _require(set(header) == set(HEADER_COLUMNS), "progression_source_header_columns")
    scope = _scope(tuple(header[name] for name in SCOPE))
    _integer(header["input_origin"])
    _integer(header["input_watermark"])
    for name in ("source_fact_count", *COUNTS.values()):
        _integer(header[name], upper=MAX_INPUTS)
    _quality(header["quality_flags"])
    _integer(header["publication_complete"], upper=1)
    _digest(header["source_digest"])
    _integer(header["reference_count"], upper=MAX_INPUTS)
    _digest(header["reference_digest"])
    _require(header["source_fact_count"] + header["reference_count"] <= MAX_INPUTS, "progression_source_reference_capacity")
    _require(header["input_origin"] <= header["input_watermark"] and
             sum(header[name] for name in COUNTS.values()) == header["source_fact_count"] <=
             header["input_watermark"] - header["input_origin"], "progression_source_header_count")
    return scope


def advance_header(header, inputs, watermark, *, max_total_bytes=DEFAULT_BYTE_LIMIT, check_deadline=lambda: None):
    scope = validate_header(header)
    _require(header["publication_complete"] == 0, "progression_published_source_immutable")
    _require(len(inputs) <= MAX_INPUTS - header["source_fact_count"] - header["reference_count"] and
             HEADER_BYTE_BOUND + len(inputs) * (INPUT_ROW_BYTE_BOUND + PUBLICATION_INPUT_BYTE_BOUND) <= max_total_bytes,
             "progression_source_capacity")
    result, previous = dict(header), header["input_watermark"]
    watermark = _integer(watermark)
    _require(watermark >= previous, "progression_cursor_backward")
    for row in inputs:
        check_deadline()
        decode_input(row, scope)
        _require(previous < row["ingest_id"] <= watermark, "progression_input_order_or_cursor")
        previous = row["ingest_id"]
        result["source_digest"] = advance_digest(result["source_digest"], row)
        result[COUNTS[row["record_kind"]]] += 1
        result["source_fact_count"] += 1
    result["input_watermark"] = watermark
    validate_header(result)
    return result


@dataclass(frozen=True, slots=True)
class VerifiedSource:
    scope: tuple[int, int, int, int]
    header: Mapping[str, Any]
    inputs: tuple[DecodedInput, ...]
    by_receipt: Mapping[tuple[int, int, int], DecodedInput]
    reference_receipts: frozenset[tuple[int, int, int]] = frozenset()


def _reference_digest(scope, references):
    digest = hashlib.sha256(b"duris-progression-references-v1:" + _canonical(_scope(scope))).digest()
    previous = 0
    for row in references:
        _require(previous < row["ingest_id"], "progression_reference_order_or_replay")
        previous = row["ingest_id"]
        digest = advance_digest(digest, row)
    return digest


def _reference_targets(selected):
    receipts, roots = {}, set()
    for item in selected:
        row = item.source
        if row["record_kind"] != 15:
            continue
        for prefix, kind in (("pctx_source_", row["pctx_source_kind"]), ("pctx_ownership_", 9)):
            key = _key(row, prefix)
            if key != (0, 0, 0):
                _require(key not in receipts or receipts[key] == kind, "progression_reference_family_conflict")
                receipts[key] = kind
        if row["pctx_configuration_record_seq"]:
            roots.add((row["pctx_boot_id"], row["pctx_process_id"], row["pctx_configuration_record_seq"]))
    return receipts, roots


def bind_references(header, inputs, references, *, max_total_bytes=DEFAULT_BYTE_LIMIT, check_deadline=lambda: None):
    """Bind exact requested receipts without advancing the ingestion cursor.

    References can predate a rebuild origin or arrive after its selected cursor.
    They are independently retained/countable inputs, never synthetic cursor
    rows or substituted current configuration/ownership.
    """
    _require(header["publication_complete"] == 0 and header["reference_count"] == 0, "progression_reference_binding_state")
    result = dict(header, reference_count=len(references), reference_digest=_reference_digest(validate_header(header), references))
    verify_source(result, inputs, references=references, expected_scope=tuple(header[name] for name in SCOPE),
                  max_total_bytes=max_total_bytes, check_deadline=check_deadline)
    return result


def verify_source(header, inputs, *, references=(), expected_scope, max_total_bytes=DEFAULT_BYTE_LIMIT, check_deadline=lambda: None):
    scope = validate_header(header)
    _require(scope == expected_scope and len(inputs) == header["source_fact_count"] <= MAX_INPUTS,
             "progression_retained_source_incomplete")
    _require(len(references) == header["reference_count"] and len(inputs) + len(references) <= MAX_INPUTS,
             "progression_retained_reference_count_or_capacity")
    _require(HEADER_BYTE_BOUND + (len(inputs) + len(references)) * (INPUT_ROW_BYTE_BOUND + PUBLICATION_INPUT_BYTE_BOUND) <= max_total_bytes,
             "progression_retained_source_capacity")
    digest, previous = seed_digest(scope, header["input_origin"]), header["input_origin"]
    decoded, receipts = [], {}
    counts = dict.fromkeys(COUNTS, 0)
    for row in inputs:
        check_deadline()
        item = decode_input(row, scope)
        _require(previous < row["ingest_id"] <= header["input_watermark"], "progression_retained_input_order")
        previous = row["ingest_id"]
        replay = tuple(row[name] for name in REPLAY)
        _require(replay not in receipts, "progression_retained_receipt_replay")
        receipts[replay] = item
        decoded.append(item)
        counts[row["record_kind"]] += 1
        digest = advance_digest(digest, row)
    _require(digest == header["source_digest"] and all(counts[kind] == header[name] for kind, name in COUNTS.items()),
             "progression_retained_source_digest_or_counts")
    _require(_reference_digest(scope, references) == header["reference_digest"], "progression_retained_reference_digest")
    targets, roots = _reference_targets(decoded)
    reference_keys = set()
    for row in references:
        check_deadline()
        item = decode_input(row, scope)
        source, key = item.source, _key(item.source)
        _require(key not in reference_keys, "progression_reference_replay")
        reference_keys.add(key)
        allowed = targets.get(key) == source["record_kind"] or source["record_kind"] == 16 and (
            source["pcfg_boot_id"], source["pcfg_process_id"], source["pcfg_root_record_seq"]) in roots
        _require(allowed, "progression_reference_not_requested")
        if key in receipts:
            _require(receipts[key] == item, "progression_reference_cursor_conflict")
        else:
            receipts[key] = item
            decoded.append(item)
    return VerifiedSource(scope, dict(header), tuple(decoded), receipts, frozenset(reference_keys))


@dataclass(frozen=True, slots=True)
class ResolvedContext:
    receipt: tuple[int, int, int]
    context: Mapping[str, int]
    source: Mapping[str, int] | None
    ownership: Mapping[str, int] | None
    configuration: Mapping[str, Any] | None
    unknown: tuple[str, ...]


def _key(row, prefix=""):
    return tuple(row[prefix + name] for name in ("boot_id", "process_id", "record_seq"))


def _matching_subject(value, source):
    return all(value["pctx_" + name] == source[name] for name in
               (*observations.SESSION, "subject_id", "pid", "environment_id", "season_id", "config_id",
                "classifier_version", "policy_version"))


def resolve_contexts(window: VerifiedSource, *, check_deadline: Callable[[], None] = lambda: None):
    """Resolve only exact retained receipts; absent inputs remain unknown.

    A complete configuration inventory is checked independently of queue
    admission. This function never substitutes a current owner or catalogue for
    the packet's original references and never certifies an XP/save commit.
    """
    groups, ownership = defaultdict(list), defaultdict(list)
    for item in window.inputs:
        check_deadline()
        row = item.source
        if row["record_kind"] == 16:
            root = (row["pcfg_boot_id"], row["pcfg_process_id"], row["pcfg_root_record_seq"])
            groups[root].append(item)
        elif row["record_kind"] == 9:
            ownership[tuple(row[name] for name in (*observations.SESSION, "subject_id", "pid", "boot_id", "process_id"))].append(row)
    ownership_times = {}
    for key, samples in ownership.items():
        samples.sort(key=lambda sample: (sample["at_monotonic_usec"], sample["record_seq"]))
        ownership_times[key] = [(sample["at_monotonic_usec"], sample["record_seq"]) for sample in samples]
    catalogs, catalog_general = {}, {}
    meta_fields = tuple(name for name, _, _ in context.CONFIGURATION_CHUNK_LAYOUT if
                        not name.startswith("pcfg_value_") and name not in ("pcfg_chunk_index", "pcfg_value_count"))
    for root, packets in groups.items():
        check_deadline()
        ordinals = [item.source["pcfg_chunk_index"] for item in packets]
        _require(len(set(ordinals)) == len(ordinals) and len(packets) <= context.CONFIGURATION_CHUNKS,
                 "progression_catalog_ordinal_replay")
        common = tuple(packets[0].source[name] for name in meta_fields)
        _require(all(tuple(item.source[name] for name in meta_fields) == common for item in packets),
                 "progression_catalog_mixed_metadata")
        complete = len(packets) == context.CONFIGURATION_CHUNKS
        try:
            catalogs[root] = context.reconcile_configuration_chunks(
                [{name: item.source[name] for name, _, _ in context.CONFIGURATION_CHUNK_LAYOUT} for item in packets],
                [_key(item.source) for item in packets]) if complete else None
        except context.ContextContractError as error:
            raise PublicationError(str(error)) from error
        catalog_general[root] = complete and all(item.configuration is not None for item in packets)
    resolved, seen_sources, context_sequences = [], set(), set()
    for item in window.inputs:
        check_deadline()
        row = item.source
        if row["record_kind"] != 15:
            continue
        value = {name: row[name] for name in context.FIELDS}
        sequence = (value["pctx_boot_id"], value["pctx_process_id"], value["pctx_sequence"])
        _require(sequence not in context_sequences, "progression_context_sequence_replay")
        context_sequences.add(sequence)
        unknown = []
        if item.projection_quality or value["pctx_quality_flags"]:
            unknown.append("context_quality")
        source_key = _key(value, "pctx_source_")
        source_item = window.by_receipt.get(source_key)
        source = None if source_item is None else source_item.source
        if source_key != (0, 0, 0):
            _require(source_key not in seen_sources, "progression_context_source_replay")
            seen_sources.add(source_key)
            if source is None:
                unknown.append("missing_source")
            else:
                _require(source["record_kind"] == value["pctx_source_kind"] and _matching_subject(value, source),
                         "progression_context_source_subject_or_scope")
                _require(all(value["pctx_connection_" + name] == source["connection_" + name]
                    for name in ("boot_id", "process_id", "seq")), "progression_context_source_connection")
                if source["record_kind"] == 1:
                    _require(all(value["pctx_" + dst] == source[src] for dst, src in (
                        ("start_usec", "start_monotonic_usec"), ("at_usec", "end_monotonic_usec"),
                        ("start_utc_usec", "start_utc_usec"), ("at_utc_usec", "end_utc_usec"))),
                        "progression_context_interval_clocks")
                else:
                    _require(value["pctx_at_usec"] == source["at_monotonic_usec"] and
                        value["pctx_at_utc_usec"] == source["at_utc_usec"], "progression_context_point_clocks")
                if source["record_kind"] == 6:
                    _require((value["pctx_boundary"] == 3) == (source["progression_kind"] == 1) and
                        value["pctx_current_level"] == source["progression_after_level"],
                        "progression_context_XP_or_level_boundary")
                    if source["progression_kind"] == 1:
                        _require(value["pctx_current_exp"] == source["progression_after_exp"],
                                 "progression_context_applied_state_conflict")
                    if source["progression_observation_status"] != 1:
                        unknown.append("unsupported_progression_durability")
                if source["record_kind"] == 2:
                    _require(source["lifecycle_kind"] == 2, "progression_context_lifecycle_cut")
                if source_item.projection_quality or source.get("quality_flags", 0):
                    unknown.append("source_quality")
                if source["occurrence_utc_usec"] != value["pctx_at_utc_usec"]:
                    unknown.append("clock_ambiguity")
        owner_key = _key(value, "pctx_ownership_")
        owner_item = window.by_receipt.get(owner_key)
        owner = None if owner_item is None else owner_item.source
        if owner is None:
            unknown.append("missing_ownership")
        else:
            _require(owner["record_kind"] == 9 and all(value["pctx_" + name] == owner[name] for name in
                (*observations.SESSION, "subject_id", "pid", "environment_id", "season_id")) and
                value["pctx_account_token"] == owner["ownership_account_token"] and
                all(value["pctx_connection_" + name] == owner["connection_" + name] for name in
                    ("boot_id", "process_id", "seq")) and owner["at_monotonic_usec"] <= value["pctx_at_usec"],
                "progression_context_ownership_receipt_conflict")
            if not owner["ownership_account_token"]:
                unknown.append("unknown_account")
            subject = tuple(owner[name] for name in (*observations.SESSION, "subject_id", "pid", "boot_id", "process_id"))
            latest = ownership[subject][bisect_right(ownership_times[subject], (value["pctx_at_usec"], UINT64_MAX)) - 1]
            _require(_key(latest) == owner_key, "progression_context_ownership_not_latest")
            if owner_item.projection_quality or owner["quality_flags"]:
                unknown.append("ownership_quality")
            if owner["at_utc_usec"] == UTC_UNKNOWN or value["pctx_at_utc_usec"] == UTC_UNKNOWN or (
                owner["at_utc_usec"] + value["pctx_at_usec"] - owner["at_monotonic_usec"] != value["pctx_at_utc_usec"]):
                unknown.append("clock_ambiguity")
        root = (value["pctx_boot_id"], value["pctx_process_id"], value["pctx_configuration_record_seq"])
        configuration = catalogs.get(root)
        if configuration is None or not value["pctx_flags"] & context.CONFIG_CATALOG_KNOWN:
            unknown.append("incomplete_configuration")
            configuration = None
        else:
            _require(all(configuration[name] == value["pctx_" + name] for name in
                ("config_id", "build_version", "content_version", "classifier_version", "policy_version")),
                "progression_context_configuration_versions")
            digest = b"".join(value["pctx_configuration_digest_" + str(index)].to_bytes(8, "big") for index in range(4)).hex()
            _require(context.configuration_digest(configuration) == digest, "progression_context_configuration_digest")
            if value["pctx_flags"] & context.THRESHOLD_KNOWN:
                threshold = next((entry for entry in configuration["values"] if entry["id"] == value["pctx_threshold_level"]), None)
                _require(threshold is not None and threshold["kind"] == 2 and
                    int.from_bytes(threshold["bits"].to_bytes(8, "big"), "big", signed=True) == value["pctx_next_threshold_xp"],
                    "progression_context_actual_threshold_conflict")
            values = {entry["id"]: entry["bits"] for entry in configuration["values"]}
            if value["pctx_flags"] & context.DECISION_POLICY_KNOWN:
                _require(all(value[field] == int.from_bytes(values[source_id].to_bytes(8, "big"), "big", signed=True) for field, source_id in (
                    ("pctx_decision_good_assistance_gap", 604), ("pctx_decision_evil_assistance_gap", 605),
                    ("pctx_decision_max_exp_level", 606))), "progression_context_actual_policy_conflict")
            _require(bool(value["pctx_flags"] & context.AUTOMATIC_RESTED) == bool(values[608]),
                     "progression_context_actual_rested_policy_conflict")
            if source is not None and source["record_kind"] == 6 and source["progression_kind"] == 2 and source["progression_reason"] == 4:
                _require(source["progression_after_level"] in values and source["progression_threshold_xp"] == values[source["progression_after_level"]],
                         "progression_context_consumed_threshold_conflict")
        if item.configuration is None or not catalog_general.get(root, False):
            unknown.append("unretained_general_configuration")
        if value["pctx_at_utc_usec"] == UTC_UNKNOWN or value["pctx_quality_flags"] & (1 << 7):
            unknown.append("clock_ambiguity")
        resolved.append(ResolvedContext(_key(row), value, source, owner, configuration, tuple(sorted(set(unknown)))))
    return tuple(resolved)


MAX_OUTPUT_ROWS = 4_096
MAX_SLICES = identity.MAX_EFFORT_SLICES
ROW_FETCH_BYTE_BOUND = INPUT_ROW_BYTE_BOUND
ROW_VALUE_BYTE_BOUND = PUBLICATION_INPUT_BYTE_BOUND
SNAPSHOT_ROW_BYTE_BOUND = 512
REVIEW_BYTE_BOUND = 2 * identity.MAX_PACKET_BYTES + 2 * incident.MAX_PACKET_BYTES + (
    incident.MAX_INCIDENTS + 1) * incident.INCIDENT_ROW_BYTE_BOUND
REPORT_METADATA_BYTE_BOUND = 3 * HEADER_BYTE_BOUND + 2 * incident.MAX_PACKET_BYTES + (
    incident.MAX_INCIDENTS + 1) * incident.INCIDENT_ROW_BYTE_BOUND + (MAX_OUTPUT_ROWS + 1) * SNAPSHOT_ROW_BYTE_BOUND
_STAGE_BUILD = ("primary_class_mask", "secondary_class_mask", "specialization", "race", "faction",
                *("base_stat_" + str(index) for index in range(10)),
                *("effective_stat_" + str(index) for index in range(10)))


def _session(row):
    return tuple(row[name] for name in observations.SESSION)


def _utc(value):
    return None if value == UTC_UNKNOWN else value


def _sum(values, *, signed=False):
    result = 0
    for value in values:
        result = observations.checked_add(result, value, "progression_total", signed=signed)
    return result


def _union(windows):
    result, through = 0, None
    for first, last in sorted(windows):
        _require(first < last, "progression_union_window")
        result = _sum((result, last - first if through is None or first > through else max(0, last - through)))
        through = last if through is None else max(through, last)
    return result


def _review_unknown(coverage, producer, first, last, kinds):
    """A reviewed empty inventory qualifies only its actual dated range."""
    if coverage.get("registry_schema_version") != 8 or coverage.get("status") != "reviewed_inventory":
        return ("unreviewed_loss_inventory",)
    if first is None or last is None or not (coverage["reviewed_from_utc_usec"] <= first <= last <=
                                           coverage["reviewed_through_utc_usec"]):
        return ("loss_review_clock_or_range",)
    mask = sum(1 << kind for kind in kinds)
    affected = any(row["status"] == "active" and row["record_kind_mask"] & mask and
        (row["producer_boot_id"], row["producer_process_id"]) in ((None, None), producer) and
        (row["start_utc_usec"] is None or row["start_utc_usec"] <= last) and
        (row["end_utc_usec"] is None or first <= row["end_utc_usec"]) for row in coverage["incidents"])
    return ("reviewed_source_loss",) if affected else ()


def _ownership_samples(window):
    result = defaultdict(list)
    for item in window.inputs:
        row = item.source
        if row["record_kind"] == 9:
            sample = identity.OwnershipObservation(window.scope[2:], _session(row), _key(row), row["subject_id"], row["pid"],
                row["at_monotonic_usec"], row["ownership_account_token"] or None,
                identity_publication.OWNERSHIP_SOURCES[row["ownership_source"]], _utc(row["at_utc_usec"]),
                item.projection_quality | row["quality_flags"])
            result[(sample.session, sample.subject_id, sample.pid)].append(sample)
    return result


def _attributed_intervals(window, registry, coverage, check_deadline):
    samples = _ownership_samples(window)
    pieces, rows = [], {}
    for item in window.inputs:
        check_deadline()
        row = item.source
        if row["record_kind"] != 1:
            continue
        quality = item.projection_quality | row["quality_flags"]
        review = _review_unknown(coverage, _key(row)[:2], _utc(row["start_utc_usec"]), _utc(row["end_utc_usec"]), (1, 9))
        if review:
            quality |= identity_publication.ROLLUP_QUALITY_INCIDENT_GAP
        interval = identity.ObservedInterval(window.scope[2:], _session(row), _key(row), row["subject_id"], row["pid"],
            row["config_id"], {0: "unknown", 1: "idle", 2: "active", 3: "resident_linkdead"}[row["category"]],
            row["start_monotonic_usec"], row["end_monotonic_usec"], _utc(row["start_utc_usec"]),
            _utc(row["end_utc_usec"]), quality)
        source = dict(row, projection_quality=quality)
        selected = samples.get((_session(row), row["subject_id"], row["pid"]), ())
        windows = identity_publication._gap_windows(source, coverage, selected)
        if review and not windows:
            windows = [(None, None)]
        attributed = identity.attribute_interval(interval, selected, registry,
            ownership_gap_windows=identity_publication._monotonic_gaps(interval, windows))
        _require(len(pieces) + len(attributed) <= MAX_SLICES, "progression_effort_slice_capacity")
        pieces.extend(attributed)
        rows[_key(row)] = source
    identity.union_effort(pieces)  # Reject overlap before grouping can hide it.
    return tuple(pieces), rows, samples


def _point_identity(value, samples, registry, coverage):
    v = value.context
    producer = (v["pctx_boot_id"], v["pctx_process_id"])
    selected = samples.get((tuple(v["pctx_" + name] for name in observations.SESSION), v["pctx_subject_id"], v["pctx_pid"]), ())
    ordered = identity.ordered_ownership((v["pctx_environment_id"], v["pctx_season_id"]),
        tuple(v["pctx_" + name] for name in observations.SESSION), v["pctx_subject_id"], v["pctx_pid"], producer, selected)
    sample = next((sample for sample in reversed(ordered) if sample.at_monotonic_usec <= v["pctx_at_usec"]), None)
    first = _utc(v["pctx_at_utc_usec"])
    unknown = set(value.unknown) | set(_review_unknown(coverage, producer, first, first, (6, 9, 15, 16)))
    source = {"environment_id": v["pctx_environment_id"], "season_id": v["pctx_season_id"],
        **{name: v["pctx_" + name] for name in observations.SESSION}, "subject_id": v["pctx_subject_id"],
        "pid": v["pctx_pid"], "boot_id": producer[0], "process_id": producer[1], "at_utc_usec": v["pctx_at_utc_usec"],
        "at_monotonic_usec": v["pctx_at_usec"], "projection_quality": v["pctx_quality_flags"]}
    windows = identity_publication._gap_windows(source, coverage, selected)
    if any(first is None or (begin is None or begin <= first) and (end is None or first < end) for begin, end in windows):
        unknown.add("ownership_loss_until_fresh_anchor")
    account = None if sample is None or value.ownership is None or _key(value.ownership) != sample.replay or (
        any(name in unknown for name in ("missing_ownership", "unknown_account", "ownership_quality", "clock_ambiguity",
                                         "ownership_loss_until_fresh_anchor", "unreviewed_loss_inventory",
                                         "loss_review_clock_or_range", "reviewed_source_loss"))) else sample.account_token
    controller, association, status = identity.linkage_at(registry, account, first if "clock_ambiguity" not in unknown else None)
    return dict(registry_version=None if registry is None else registry.registry_version, account_token=account,
                controller_token=controller, association_id=association, linkage_status=status,
                unknown=sorted(unknown))


def _stratum(v):
    flags = v["pctx_flags"]
    return dict(level=v["pctx_current_level"], config_id=v["pctx_config_id"], build_version=v["pctx_build_version"],
        content_version=v["pctx_content_version"], classifier_version=v["pctx_classifier_version"],
        policy_version=v["pctx_policy_version"], configuration_digest="".join(
            v["pctx_configuration_digest_" + str(index)].to_bytes(8, "big").hex() for index in range(4)),
        **{name: v["pctx_" + name] if flags & context.BUILD_KNOWN else None for name in _STAGE_BUILD},
        rested_selection=v["pctx_rested_selection"] if flags & context.SELECTION_KNOWN else None,
        rested_application=v["pctx_rested_application"] if flags & context.APPLICATION_KNOWN else None,
        assistance=v["pctx_assistance"] if flags & context.ASSISTANCE_KNOWN else None,
        formal_group_size=v["pctx_formal_group_size"] if flags & context.GROUP_ROSTER_KNOWN else None,
        eligible_group_size=v["pctx_eligible_group_size"] if flags & context.GROUP_ELIGIBILITY_KNOWN else None,
        highest_eligible_group_level=v["pctx_highest_group_level"] if flags & context.GROUP_ELIGIBILITY_KNOWN else None,
        assistance_level=v["pctx_assistance_level"] if flags & context.ASSISTANCE_KNOWN else None,
        alive=bool(flags & context.ALIVE),
        **{name: v["pctx_" + name] if flags & context.DECISION_POLICY_KNOWN else None for name in (
            "decision_level_cap", "decision_good_assistance_gap", "decision_evil_assistance_gap", "decision_max_exp_level")},
        hardcore=bool(flags & context.HARDCORE) if flags & context.DECISION_POLICY_KNOWN else None,
        hardcore_bypass=bool(flags & context.HARDCORE_BYPASS) if flags & context.DECISION_POLICY_KNOWN else None,
        storage_gate_passed=bool(flags & context.STORAGE_GATE_PASSED) if flags & context.STORAGE_GATE_KNOWN else None,
        point_context_only=not bool(flags & context.CONTIGUOUS_EXPOSURE))


def _milestone_measure(first, first_utc, through, intervals, unknown, check_deadline, *, configuration=None):
    end = through
    connected, active, idle, uncertain_activity, linkdead, coverage = 0, 0, 0, 0, 0, []
    source_count, source_digest, clock_unknown = 0, hashlib.sha256(b"duris-progression-stage-interval-v1").digest(), False
    for row in intervals:
        check_deadline()
        low, high = max(first, row["start_monotonic_usec"]), min(end[0], row["end_monotonic_usec"])
        if low >= high:
            continue
        coverage.append((low, high))
        source_count += 1
        source_digest = hashlib.sha256(source_digest + b"".join(number.to_bytes(8, "big") for number in (*_key(row), low, high))).digest()
        length = high - low
        if row["category"] == 3:
            linkdead = _sum((linkdead, length))
        else:
            connected = _sum((connected, length))
            if row["category"] == 2:
                active = _sum((active, length))
            elif row["category"] == 1:
                idle = _sum((idle, length))
            else:
                uncertain_activity = _sum((uncertain_activity, length))
        if row["projection_quality"] or row["quality_flags"]:
            unknown.add("interval_quality")
        if configuration is not None and any(row[name] != configuration[name] for name in
                ("config_id", "classifier_version", "policy_version")):
            unknown.add("interval_configuration_change")
        if row["start_utc_usec"] == UTC_UNKNOWN or row["end_utc_usec"] == UTC_UNKNOWN or (
            row["start_utc_usec"] - row["start_monotonic_usec"] != first_utc - first or
            row["end_utc_usec"] - row["end_monotonic_usec"] != first_utc - first):
            clock_unknown = True
    covered = _union(coverage)
    elapsed = None if first_utc == UTC_UNKNOWN or end[1] == UTC_UNKNOWN or clock_unknown or (
        end[1] - first_utc != end[0] - first) else end[0] - first
    missing = end[0] - first - covered
    if missing:
        unknown.add("missing_intervals")
    if elapsed is None:
        unknown.add("clock_ambiguity")
    if uncertain_activity:
        unknown.add("unknown_activity_classification")
    return dict(observed_connected_usec=connected, observed_heuristic_active_usec=active, observed_idle_usec=idle,
        unknown_activity_usec=uncertain_activity, resident_linkdead_usec=linkdead, covered_interval_usec=covered,
        missing_interval_usec=missing, observed_elapsed_usec=elapsed, interval_source_count=source_count,
        interval_source_digest=source_digest.hex())


def _milestones(window, contexts, raw_intervals, coverage, check_deadline, max_output_rows):
    """Per-producer observed stage segments, including left/right censoring."""
    groups, intervals, raw_progression, exits = (defaultdict(list) for _ in range(4))
    for value in contexts:
        v = value.context
        if v["pctx_boundary"] == 2:
            continue  # An interval's context describes its span, not a later level point.
        groups[(v["pctx_subject_id"], tuple(v["pctx_" + name] for name in observations.SESSION), value.receipt[:2])].append(value)
    for row in raw_intervals.values():
        intervals[(row["subject_id"], _session(row), _key(row)[:2])].append(row)
    for item in window.inputs:
        row = item.source
        if row["record_kind"] == 6:
            raw_progression[(row["subject_id"], _session(row), _key(row)[:2])].append(row)
        elif row["record_kind"] == 2 and row["lifecycle_kind"] == 2:
            exits[(row["subject_id"], _session(row), _key(row)[:2])].append(row)
    result = []
    for key, points in sorted(groups.items()):
        check_deadline()
        points.sort(key=lambda value: (value.context["pctx_at_usec"], value.receipt[2]))
        stage_intervals = sorted(intervals[key], key=lambda row: row["start_monotonic_usec"])
        for previous, current in zip(stage_intervals, stage_intervals[1:]):
            _require(previous["end_monotonic_usec"] <= current["start_monotonic_usec"], "progression_milestone_interval_overlap")
        start, left_censored, complete_sources = None, True, set()

        def emit(through, status, end_context=None, cut=None):
            nonlocal start
            _require(len(result) < max_output_rows, "progression_milestone_output_capacity")
            v = start.context
            unknown = set(start.unknown)
            if not v["pctx_starting_level"]:
                unknown.add("missing_collection_baseline")
            if start is points[0] and v["pctx_boundary"] != 1:
                unknown.add("missing_baseline")
            if end_context is not None:
                unknown.update(end_context.unknown)
            if cut:
                unknown.add(cut)
            unknown.update(_review_unknown(coverage, key[2], _utc(v["pctx_at_utc_usec"]), _utc(through[1]), (1, 2, 6, 15, 16)))
            missing_context = [row for row in raw_progression[key] if v["pctx_at_usec"] <= row["at_monotonic_usec"] <= through[0]
                and _key(row) not in complete_sources]
            if missing_context:
                unknown.add("missing_progression_context")
            if any(v["pctx_at_usec"] < row["at_monotonic_usec"] < through[0] for row in exits[key]):
                unknown.add("lifecycle_cut")
            measured = _milestone_measure(v["pctx_at_usec"], v["pctx_at_utc_usec"], through, stage_intervals, unknown, check_deadline,
                configuration={name: v["pctx_" + name] for name in ("config_id", "classifier_version", "policy_version")})
            qualified = status == "observed_completion" and not left_censored and not unknown
            result.append(dict(subject_id=key[0], session=list(key[1]), producer=list(key[2]),
                start_context_key=list(start.receipt), end_context_key=None if end_context is None else list(end_context.receipt),
                starting_level=v["pctx_starting_level"] or None, stage_level=v["pctx_current_level"], milestone_level=v["pctx_current_level"] + 1,
                already_past_through_level=v["pctx_current_level"] if v["pctx_boundary"] == 1 or start is points[0] else None,
                start_monotonic_usec=v["pctx_at_usec"], through_monotonic_usec=through[0],
                start_utc_usec=_utc(v["pctx_at_utc_usec"]), through_utc_usec=_utc(through[1]),
                status=status, left_censored=left_censored, right_censored=status != "observed_completion", unknown=sorted(unknown),
                start_anchor_kind="retained_context", first_progression_source_key=None, last_progression_source_key=None,
                stratum=_stratum(v), **measured, full_stage_connected_usec=measured["observed_connected_usec"] if qualified else None,
                full_stage_heuristic_active_usec=measured["observed_heuristic_active_usec"] if qualified else None,
                full_stage_elapsed_usec=measured["observed_elapsed_usec"] if qualified else None,
                activity_method="existing_input_activity_heuristic", character_save_committed=False,
                complete_population_coverage_implied=False, time_to_level_median=None))

        complete_sources = {_key(value.source) for value in points if value.source is not None}
        for value in points:
            check_deadline()
            v = value.context
            if start is None:
                if v["pctx_boundary"] == 5:
                    continue
                start, left_censored = value, True
                continue
            s = start.context
            source = value.source
            level_boundary = source is not None and source["record_kind"] == 6 and source["progression_kind"] in (2, 3)
            ordinary_completion = level_boundary and source["progression_kind"] == 2 and source["progression_reason"] == 4 and (
                (1 <= source["progression_source"] <= 10 or source["progression_source"] == 12)) and source["progression_before_level"] == s["pctx_current_level"] and (
                source["progression_after_level"] == s["pctx_current_level"] + 1)
            same_configuration = all(v["pctx_" + name] == s["pctx_" + name] for name in (
                "config_id", "build_version", "content_version", "classifier_version", "policy_version",
                *("configuration_digest_" + str(index) for index in range(4))))
            same_build = all(v["pctx_" + name] == s["pctx_" + name] for name in _STAGE_BUILD)
            same_rested = v["pctx_rested_selection"] == s["pctx_rested_selection"]
            same_group_size = v["pctx_formal_group_size"] == s["pctx_formal_group_size"]
            cut = "configuration_change" if not same_configuration else "observed_build_change" if not same_build else (
                "observed_rested_selection_change" if not same_rested else "observed_formal_group_size_change" if not same_group_size else
                "lifecycle_cut" if v["pctx_boundary"] == 5 else "level_state_change" if level_boundary or (
                    v["pctx_current_level"] != s["pctx_current_level"]) else None)
            same_stage = same_configuration and same_build and same_rested and same_group_size
            if ordinary_completion or cut:
                emit((v["pctx_at_usec"], v["pctx_at_utc_usec"]), "observed_completion" if ordinary_completion else "censored", value,
                     None if ordinary_completion and same_stage else cut)
                start = None if v["pctx_boundary"] == 5 else value
                left_censored = not ordinary_completion or not same_stage or bool(value.unknown)
        if start is not None:
            last = points[-1].context
            through = (last["pctx_at_usec"], last["pctx_at_utc_usec"])
            later = [row for row in stage_intervals if row["end_monotonic_usec"] >= through[0]]
            if later:
                last_interval = max(later, key=lambda row: row["end_monotonic_usec"])
                through = last_interval["end_monotonic_usec"], last_interval["end_utc_usec"]
            lifecycle = [row for row in exits[key] if start.context["pctx_at_usec"] <= row["at_monotonic_usec"] <= through[0]]
            if lifecycle:
                close = min(lifecycle, key=lambda row: (row["at_monotonic_usec"], row["record_seq"]))
                emit((close["at_monotonic_usec"], close["at_utc_usec"]), "censored", cut="missing_lifecycle_context")
            else:
                emit(through, "unfinished")
    for key in sorted(raw_progression.keys() - groups.keys()):
        check_deadline()
        _require(len(result) < max_output_rows, "progression_milestone_output_capacity")
        points = sorted(raw_progression[key], key=lambda row: (row["at_monotonic_usec"], row["record_seq"]))
        first, last = points[0], points[-1]
        through = last["at_monotonic_usec"], last["at_utc_usec"]
        selected = sorted(intervals[key], key=lambda row: row["start_monotonic_usec"])
        if selected and selected[-1]["end_monotonic_usec"] > through[0]:
            through = selected[-1]["end_monotonic_usec"], selected[-1]["end_utc_usec"]
        unknown = {"missing_context", "missing_baseline"}
        unknown.update(_review_unknown(coverage, key[2], _utc(first["at_utc_usec"]), _utc(through[1]), (1, 2, 6, 15, 16)))
        measured = _milestone_measure(first["at_monotonic_usec"], first["at_utc_usec"], through, selected, unknown, check_deadline)
        level = first["progression_before_level"] or None
        result.append(dict(subject_id=key[0], session=list(key[1]), producer=list(key[2]), start_context_key=None,
            end_context_key=None, starting_level=None, stage_level=level, milestone_level=None if level is None else level+1,
            already_past_through_level=level, start_monotonic_usec=first["at_monotonic_usec"], through_monotonic_usec=through[0],
            start_utc_usec=_utc(first["at_utc_usec"]), through_utc_usec=_utc(through[1]), status="unknown_context",
            left_censored=True, right_censored=True, unknown=sorted(unknown), stratum=None, **measured,
            start_anchor_kind="first_retained_progression_point", first_progression_source_key=list(_key(first)),
            last_progression_source_key=list(_key(last)), full_stage_connected_usec=None, full_stage_heuristic_active_usec=None,
            full_stage_elapsed_usec=None, activity_method="existing_input_activity_heuristic", character_save_committed=False,
            complete_population_coverage_implied=False, time_to_level_median=None))
    return tuple(result)


@dataclass(frozen=True, slots=True)
class EvidenceValues:
    """Private publication values; the SQL transaction still owns publication."""
    contexts: tuple[Mapping[str, Any], ...]
    milestones: tuple[Mapping[str, Any], ...]
    rotations: tuple[Mapping[str, Any], ...]
    effort: tuple[Mapping[str, Any], ...]
    portfolio: tuple[Mapping[str, Any], ...]


def _rotations(pieces, rows, check_deadline, max_output_rows, *, category):
    groups = defaultdict(list)
    for piece in pieces:
        if (category == "heuristic_active" and piece.interval.category != "active") or (
            category == "connected" and piece.interval.category == "resident_linkdead"):
            continue
        row = rows[piece.interval.replay]
        for basis, token in (("character", piece.interval.subject_id), ("account", piece.account_token),
                             ("controller", piece.controller_token)):
            groups[(row["config_id"], row["classifier_version"], row["policy_version"], basis if token else "unknown_" + basis,
                    token or 0)].append(piece)
    result = []
    for key, values in sorted(groups.items()):
        check_deadline()
        _require(len(result) < max_output_rows, "progression_rotation_output_capacity")
        known = key[3] in ("character", "account", "controller")
        comparable = [piece for piece in values if piece.start_utc_usec is not None and piece.end_utc_usec is not None and
            not piece.attribution_quality]
        events = defaultdict(list)
        for piece in comparable:
            session = (piece.interval.session, piece.interval.replay[:2], piece.interval.subject_id)
            events[piece.start_utc_usec].append((1, session))
            events[piece.end_utc_usec].append((-1, session))
        active, concurrent, simultaneous_characters, maximum, regions = {}, 0, 0, 0, []
        previous, region_start, region_characters = None, None, set()
        for at, changes in sorted(events.items()):
            check_deadline()
            if previous is not None and active:
                duration = at - previous
                if len(active) > 1:
                    concurrent = _sum((concurrent, duration))
                if len({session[2] for session in active}) > 1:
                    simultaneous_characters = _sum((simultaneous_characters, duration))
            previous_characters = {session[2] for session in active}
            for direction, session in sorted(changes):
                if direction < 0:
                    _require(session in active, "progression_rotation_end_without_start")
                    del active[session]
                else:
                    _require(session not in active, "progression_rotation_session_overlap")
                    active[session] = True
            current_characters = {session[2] for session in active}
            if previous_characters and current_characters and previous_characters.isdisjoint(current_characters):
                regions.append((region_start, at, set(region_characters)))
                region_start, region_characters = None, set()
            if active:
                if region_start is None:
                    region_start = at
                region_characters.update(session[2] for session in active)
                maximum = max(maximum, len(active))
            elif region_start is not None:
                regions.append((region_start, at, set(region_characters)))
                region_start, region_characters = None, set()
            previous = at
        switches, ambiguous = [], 0
        for before, after in zip(regions, regions[1:]):
            if len(before[2]) != 1 or len(after[2]) != 1:
                ambiguous += 1
            elif before[2] != after[2]:
                _require(len(switches) < MAX_OUTPUT_ROWS, "progression_switch_capacity")
                switches.append(dict(from_character=next(iter(before[2])), to_character=next(iter(after[2])),
                    previous_observed_through_utc_usec=before[1], next_observed_from_utc_usec=after[0],
                    elapsed_gap_usec=after[0] - before[1], gap_activity_observed=False))
        unknown_clock = _sum([piece.duration_usec for piece in values]) - _sum([piece.duration_usec for piece in comparable])
        result.append(dict(config_id=key[0], classifier_version=key[1], policy_version=key[2], basis=key[3],
            identity_token=key[4] or None, registry_version=values[0].registry_version, effort_category=category,
            summed_character_usec=_sum([piece.duration_usec for piece in values]),
            covered_union_usec=_union([(piece.start_utc_usec, piece.end_utc_usec) for piece in comparable]) if known else None,
            unknown_clock_or_quality_character_usec=unknown_clock,
            union_usec=_union([(piece.start_utc_usec, piece.end_utc_usec) for piece in comparable]) if known and not unknown_clock else None,
            observed_characters=len({piece.interval.subject_id for piece in values}),
            observed_simultaneous_session_usec=concurrent if known else None,
            observed_simultaneous_character_usec=simultaneous_characters if known else None,
            maximum_observed_simultaneous_sessions=maximum if known else None,
            sequential_switches=switches if known else None, ambiguous_region_transitions=ambiguous if known else None,
            sequential_switch_coverage_complete=known and not unknown_clock and not ambiguous,
            activity_method="existing_input_activity_heuristic" if category == "heuristic_active" else "observed_connected_session_intervals",
            all_character_switches_observed=False, human_population_implied=False,
            causal_rotation_advantage_implied=False))
    return tuple(result)


def _rate_stratum(v):
    point = _stratum(v)
    return {name: value for name, value in point.items() if name not in (
        "rested_application", "assistance", "eligible_group_size", "highest_eligible_group_level", "assistance_level",
        "decision_level_cap", "decision_good_assistance_gap", "decision_evil_assistance_gap", "decision_max_exp_level",
        "hardcore", "hardcore_bypass", "storage_gate_passed", "point_context_only")}


def _piece_identity(piece, basis):
    return piece.interval.subject_id if basis == "character" else piece.account_token if basis == "account" else piece.controller_token


def _portfolio(window, contexts, points, pieces, check_deadline, max_output_rows):
    """Source-separated mutable XP with exposure-qualified denominators.

    Assistance and rested application are award decisions. They remain numerator
    dimensions and never become an inferred duration. The denominator names its
    actual level/build/config/rested-selection/formal-group-size exposure scope.
    """
    by_source = {_key(value.source): (value, point) for value, point in zip(contexts, points, strict=True) if value.source is not None}
    exposures = {}
    required = context.CONTIGUOUS_EXPOSURE | context.BUILD_KNOWN | context.SELECTION_KNOWN | context.GROUP_ROSTER_KNOWN
    for value, point in zip(contexts, points, strict=True):
        if value.source is not None and value.source["record_kind"] == 1 and not point["unknown"] and (
            value.context["pctx_flags"] & required == required):
            exposures[_key(value.source)] = _rate_stratum(value.context)
    groups = {}
    for item in window.inputs:
        check_deadline()
        row = item.source
        if row["record_kind"] != 6:
            continue
        linked = by_source.get(_key(row))
        point = None if linked is None else linked[1]
        stratum = None if linked is None else _stratum(linked[0].context)
        rate_stratum = None if linked is None else _rate_stratum(linked[0].context)
        for basis, token in (("character", row["subject_id"]), ("account", None if point is None else point["account_token"]),
                             ("controller", None if point is None else point["controller_token"])):
            key = (basis if token else "unknown_" + basis, token or 0, row["config_id"], row["progression_before_level"],
                   row["progression_source"], row["progression_reason"], row["progression_observation_status"], _canonical(stratum))
            if key not in groups:
                _require(len(groups) < max_output_rows, "progression_portfolio_output_capacity")
                groups[key] = dict(basis=key[0], identity_token=token, config_id=row["config_id"],
                    level_stage=row["progression_before_level"], source=row["progression_source"], reason=row["progression_reason"],
                    observation_status=row["progression_observation_status"], stratum=stratum, rate_stratum=rate_stratum,
                    source_keys=[], unknown=set(), **dict.fromkeys(("requested_xp", "computed_xp", "observed_applied_xp",
                        "observed_earned_positive_xp", "death_loss_xp", "restored_positive_xp", "threshold_consumed_xp",
                        "administrative_positive_xp", "administrative_negative_xp", "system_positive_xp", "system_negative_xp",
                        "uncategorized_positive_xp", "uncategorized_negative_xp", "XP_observations", "level_advances", "level_losses"), 0))
            cell = groups[key]
            cell["source_keys"].append(list(_key(row)))
            cell["unknown"].update(("missing_progression_context",) if point is None else point["unknown"])
            if row["progression_observation_status"] != 1:
                cell["unknown"].add("unsupported_progression_durability")
            if row["quality_flags"] or item.projection_quality:
                cell["unknown"].add("source_quality")
            amounts = {}
            kind, reason, applied = row["progression_kind"], row["progression_reason"], row["progression_applied_xp"]
            if kind == 1:
                amounts.update(XP_observations=1, requested_xp=row["progression_requested_xp"],
                    computed_xp=row["progression_computed_xp"], observed_applied_xp=applied)
                family = "observed_earned" if reason == 1 and 1 <= row["progression_source"] <= 10 else (
                    "administrative" if reason == 5 else "system" if reason == 6 else "uncategorized")
                if reason == 2:
                    amounts["death_loss_xp"] = max(0, -applied)
                elif reason == 3:
                    amounts["restored_positive_xp"] = max(0, applied)
                elif reason == 4:
                    amounts["threshold_consumed_xp"] = max(0, -applied)
                elif family == "observed_earned":
                    amounts["observed_earned_positive_xp"] = max(0, applied)
                    amounts["uncategorized_negative_xp"] = max(0, -applied)
                else:
                    amounts[family + "_positive_xp"] = max(0, applied)
                    amounts[family + "_negative_xp"] = max(0, -applied)
            else:
                amounts["level_advances" if kind == 2 else "level_losses"] = 1
                if reason == 4:
                    amounts["threshold_consumed_xp"] = row["progression_threshold_xp"]
            for name, amount in amounts.items():
                cell[name] = _sum((cell[name], amount), signed=name in ("requested_xp", "computed_xp", "observed_applied_xp"))
    result = []
    for cell in groups.values():
        check_deadline()
        basis = cell["basis"].removeprefix("unknown_")
        relevant = [piece for piece in pieces if piece.interval.config_id == cell["config_id"] and
            _piece_identity(piece, basis) == cell["identity_token"] and piece.interval.category != "resident_linkdead"]
        selected = [piece for piece in relevant if exposures.get(piece.interval.replay) == cell["rate_stratum"] and cell["rate_stratum"] is not None]
        unclassified = [piece for piece in relevant if piece.interval.replay not in exposures]
        unknown = cell["unknown"]
        if not selected:
            unknown.add("no_contiguous_stratum_exposure")
        if unclassified:
            unknown.add("unclassified_stratum_exposure")
        if any(piece.attribution_quality for piece in selected):
            unknown.add("exposure_quality")
        for source_key in cell["source_keys"]:
            row = window.by_receipt[tuple(source_key)].source
            if row["progression_kind"] == 1 and not any(piece.interval.session == _session(row) and
                piece.interval.subject_id == row["subject_id"] and piece.interval.replay[:2] == _key(row)[:2] and
                piece.start_monotonic_usec <= row["at_monotonic_usec"] <= piece.end_monotonic_usec for piece in selected):
                unknown.add("award_outside_retained_exposure")
        known_identity = cell["basis"] in ("character", "account", "controller")
        clock_covered = all(piece.start_utc_usec is not None for piece in selected)
        connected_sum = _sum([piece.duration_usec for piece in selected])
        active = [piece for piece in selected if piece.interval.category == "active"]
        connected_union = _union([(piece.start_utc_usec, piece.end_utc_usec) for piece in selected]) if known_identity and clock_covered else None
        active_union = _union([(piece.start_utc_usec, piece.end_utc_usec) for piece in active]) if known_identity and clock_covered else None
        qualified = known_identity and not unknown
        cell.update(unknown=sorted(unknown), summed_character_connected_usec=connected_sum,
            summed_character_heuristic_active_usec=_sum([piece.duration_usec for piece in active]),
            covered_connected_union_usec=connected_union, covered_heuristic_active_union_usec=active_union,
            unclassified_connected_character_usec=_sum([piece.duration_usec for piece in unclassified]),
            observed_earned_xp_per_connected_hour=None if not qualified or not connected_union else (
                dict(numerator_xp=cell["observed_earned_positive_xp"], denominator_usec=connected_union, scale_usec_per_hour=3_600_000_000)),
            observed_earned_xp_per_heuristic_active_hour=None if not qualified or not active_union else (
                dict(numerator_xp=cell["observed_earned_positive_xp"], denominator_usec=active_union, scale_usec_per_hour=3_600_000_000)),
            denominator_scope="observed_level_build_configuration_rested_selection_formal_group_size_exposure",
            assistance_duration_implied=False, rested_application_duration_implied=False,
            character_save_committed=False, XP_award_committed=False, causal_advantage_implied=False,
            other_reward_units_available=False, economic_authority_dependency_issue=487,
            universal_progression_score=None, activity_method="existing_input_activity_heuristic")
        result.append(cell)
    return tuple(result)


def _build_evidence(window: VerifiedSource, registry: identity.Registry | None, coverage: Mapping[str, Any], *,
                    max_output_rows=MAX_OUTPUT_ROWS, max_total_bytes=DEFAULT_BYTE_LIMIT, check_deadline=lambda: None):
    """Compute bounded values with dated attribution and explicit censoring.

    The supplied inventory must be the publication transaction's independently
    retained schema-8 review. This private function cannot promote observation
    status or publish a generation. Award-point strata do not establish exposure.
    """
    _require(type(max_output_rows) is int and 0 < max_output_rows <= MAX_OUTPUT_ROWS, "progression_output_capacity")
    # Reserve input, decoded values, bounded review packets and each possible
    # output before attribution or grouping allocates their collections.
    review_bytes = 2 * identity.MAX_PACKET_BYTES + 2 * incident.MAX_PACKET_BYTES + (
        incident.MAX_INCIDENTS + 1) * incident.INCIDENT_ROW_BYTE_BOUND
    reserved = HEADER_BYTE_BOUND + len(window.inputs) * (INPUT_ROW_BYTE_BOUND + PUBLICATION_INPUT_BYTE_BOUND) + review_bytes
    _integer(max_total_bytes)
    _require(max_total_bytes > reserved, "progression_publication_byte_capacity")
    max_output_rows = min(max_output_rows, (max_total_bytes - reserved) // (INPUT_ROW_BYTE_BOUND + PUBLICATION_INPUT_BYTE_BOUND))
    _require(max_output_rows > 0, "progression_publication_byte_capacity")
    _require(coverage.get("registry_schema_version") == 8 and type(coverage.get("incidents")) is list and
             len(coverage["incidents"]) <= incident.MAX_INCIDENTS, "progression_incident_snapshot")
    _require(registry is None or (registry.environment_id, registry.season_id) == window.scope[2:], "progression_registry_scope")
    contexts = resolve_contexts(window, check_deadline=check_deadline)
    pieces, rows, samples = _attributed_intervals(window, registry, coverage, check_deadline)
    points = []
    for value in contexts:
        check_deadline()
        _require(len(points) < max_output_rows, "progression_context_output_capacity")
        source = value.source
        identity_value = _point_identity(value, samples, registry, coverage)
        points.append(dict(receipt=list(value.receipt), **value.context, **identity_value, stratum=_stratum(value.context),
            observed_source_key=None if source is None else list(_key(source)),
            source_progression=None if source is None or source["record_kind"] != 6 else {
                name: source[name] for name in observations.PROGRESSION_RAW_COLUMNS},
            character_save_committed=False, XP_award_committed=False, causal_rested_advantage_implied=False))
    milestones = _milestones(window, contexts, rows, coverage, check_deadline, max_output_rows - len(points))
    rotations = _rotations(pieces, rows, check_deadline, max_output_rows - len(points) - len(milestones), category="heuristic_active")
    rotations += _rotations(pieces, rows, check_deadline, max_output_rows - len(points) - len(milestones) - len(rotations), category="connected")
    portfolio = _portfolio(window, contexts, points, pieces, check_deadline,
                           max_output_rows - len(points) - len(milestones) - len(rotations))
    effort = []
    for category in ("unknown", "idle", "active", "resident_linkdead"):
        for total in identity.union_effort(pieces, category=category, include_characters=True):
            check_deadline()
            _require(len(points) + len(milestones) + len(rotations) + len(portfolio) + len(effort) < max_output_rows, "progression_output_capacity")
            effort.append({name: getattr(total, name) for name in total.__dataclass_fields__})
    result = EvidenceValues(tuple(points), milestones, rotations, tuple(effort), portfolio)
    for values in (result.contexts, result.milestones, result.rotations, result.effort, result.portfolio):
        for row in values:
            check_deadline()
            _require(len(_canonical(row)) <= MAX_PAYLOAD_BYTES, "progression_output_payload_capacity")
    return result


def build_evidence(window: VerifiedSource, registry: identity.Registry | None, coverage: Mapping[str, Any], **bounds):
    try:
        return _build_evidence(window, registry, coverage, **bounds)
    except (identity.IdentityError, observations.ObservationError, incident.IncidentError) as error:
        raise PublicationError(str(error)) from error


# Independent typed row contract. Report readers need only public stores.
ROW_COLUMNS = (*SCOPE, "row_kind", "row_key", "payload", "payload_digest", "quality_flags")
ROW_KINDS = {"progression_context": 1, "progression_milestones": 2, "character_rotation": 3,
             "progression_effort": 4, "progression_portfolio": 5}
_IDENTITY_FIELDS = ("registry_version", "account_token", "controller_token", "association_id", "linkage_status", "unknown")
_CONTEXT_FIELDS = (*context.FIELDS, "receipt", *_IDENTITY_FIELDS, "stratum", "observed_source_key", "source_progression",
                   "character_save_committed", "XP_award_committed", "causal_rested_advantage_implied")
_MILESTONE_FIELDS = ("subject_id", "session", "producer", "start_context_key", "end_context_key", "starting_level",
    "stage_level", "milestone_level", "already_past_through_level", "start_monotonic_usec", "through_monotonic_usec",
    "start_utc_usec", "through_utc_usec", "status", "left_censored", "right_censored", "unknown", "stratum",
    "observed_connected_usec", "observed_heuristic_active_usec", "observed_idle_usec", "unknown_activity_usec",
    "resident_linkdead_usec", "covered_interval_usec", "missing_interval_usec", "observed_elapsed_usec",
    "interval_source_count", "interval_source_digest", "start_anchor_kind", "first_progression_source_key",
    "last_progression_source_key", "full_stage_connected_usec", "full_stage_heuristic_active_usec", "full_stage_elapsed_usec",
    "activity_method", "character_save_committed", "complete_population_coverage_implied", "time_to_level_median")
_ROTATION_FIELDS = ("config_id", "classifier_version", "policy_version", "basis", "identity_token", "registry_version",
    "effort_category", "summed_character_usec", "covered_union_usec", "unknown_clock_or_quality_character_usec", "union_usec",
    "observed_characters", "observed_simultaneous_session_usec", "observed_simultaneous_character_usec",
    "maximum_observed_simultaneous_sessions", "sequential_switches", "ambiguous_region_transitions",
    "sequential_switch_coverage_complete", "activity_method", "all_character_switches_observed", "human_population_implied",
    "causal_rotation_advantage_implied")
_EFFORT_FIELDS = tuple(identity.EffortTotals.__dataclass_fields__)
_PORTFOLIO_FIELDS = ("basis", "identity_token", "config_id", "level_stage", "source", "reason", "observation_status",
    "stratum", "rate_stratum", "source_keys", "unknown", "requested_xp", "computed_xp", "observed_applied_xp",
    "observed_earned_positive_xp", "death_loss_xp", "restored_positive_xp", "threshold_consumed_xp",
    "administrative_positive_xp", "administrative_negative_xp", "system_positive_xp", "system_negative_xp",
    "uncategorized_positive_xp", "uncategorized_negative_xp", "XP_observations", "level_advances", "level_losses",
    "summed_character_connected_usec", "summed_character_heuristic_active_usec", "covered_connected_union_usec",
    "covered_heuristic_active_union_usec", "unclassified_connected_character_usec", "observed_earned_xp_per_connected_hour",
    "observed_earned_xp_per_heuristic_active_hour", "denominator_scope", "assistance_duration_implied",
    "rested_application_duration_implied", "character_save_committed", "XP_award_committed", "causal_advantage_implied",
    "other_reward_units_available", "economic_authority_dependency_issue", "universal_progression_score", "activity_method")
VALUE_FIELDS = {1: _CONTEXT_FIELDS, 2: _MILESTONE_FIELDS, 3: _ROTATION_FIELDS, 4: _EFFORT_FIELDS, 5: _PORTFOLIO_FIELDS}
_FALSE_FIELDS = frozenset(("character_save_committed", "XP_award_committed", "causal_rested_advantage_implied",
    "complete_population_coverage_implied", "all_character_switches_observed", "human_population_implied",
    "causal_rotation_advantage_implied", "assistance_duration_implied", "rested_application_duration_implied",
    "causal_advantage_implied", "other_reward_units_available"))


def _positive_key(value, size=3, *, nullable=False):
    _require(nullable and value is None or type(value) is list and len(value) == size and
             all(type(number) is int and 0 < number <= UINT64_MAX for number in value), "progression_public_key")


def _validate_value(kind, value):
    _require(type(value) is dict and set(value) == set(VALUE_FIELDS[kind]), "progression_public_value_columns")
    for name in _FALSE_FIELDS & value.keys():
        _require(value[name] is False, "progression_unsupported_public_authority")
    for name in ("time_to_level_median", "universal_progression_score"):
        if name in value:
            _require(value[name] is None, "progression_unsupported_distribution_or_score")
    if "unknown" in value:
        _require(type(value["unknown"]) is list and len(value["unknown"]) <= 32 and
            all(type(item) is str and 0 < len(item) <= 80 for item in value["unknown"]) and
            value["unknown"] == sorted(set(value["unknown"])), "progression_public_unknown_shape")
    if kind == 1:
        try:
            packet = context.validate_observation({name: value[name] for name in context.FIELDS})
        except context.ContextContractError as error:
            raise PublicationError(str(error)) from error
        _positive_key(value["receipt"])
        _positive_key(value["observed_source_key"], nullable=True)
        _require(value["observed_source_key"] is None or value["observed_source_key"] == list(_key(packet, "pctx_source_")),
                 "progression_public_source_reference")
        for name in ("registry_version", "account_token", "controller_token", "association_id"):
            if value[name] is not None:
                _integer(value[name], lower=1)
        _require(value["account_token"] is None or value["account_token"] == value["pctx_account_token"],
                 "progression_public_account_reference")
        _require((value["account_token"] is None) == (value["linkage_status"] == "unknown_account") and
            (value["controller_token"] is not None) == (value["linkage_status"] == "confirmed") and
            (value["controller_token"] is None or value["registry_version"] is not None and value["association_id"] is not None),
            "progression_public_dated_linkage")
        _require(value["stratum"] == _stratum(packet), "progression_public_context_stratum")
        _require(value["source_progression"] is None or type(value["source_progression"]) is dict and
            set(value["source_progression"]) == set(observations.PROGRESSION_RAW_COLUMNS) and
            all(type(number) is int for number in value["source_progression"].values()), "progression_public_XP_shape")
        xp = value["source_progression"]
        if xp is not None:
            _require(value["pctx_source_kind"] == 6 and value["pctx_current_level"] == xp["progression_after_level"],
                     "progression_public_XP_context")
            for name, upper in (("kind", 3), ("source", 12), ("reason", 6), ("observation_status", 3),
                ("modifier_flags", 511), ("before_level", 65535), ("after_level", 65535)):
                _integer(xp["progression_" + name], lower=1 if name in ("kind", "observation_status") else 0, upper=upper)
            for name in ("requested_xp", "computed_xp", "applied_xp", "before_exp", "after_exp"):
                _integer(xp["progression_" + name], lower=-(1 << 63), upper=(1 << 63)-1)
            _integer(xp["progression_threshold_xp"])
            if xp["progression_kind"] == 1:
                _require(xp["progression_before_level"] == xp["progression_after_level"] and xp["progression_threshold_xp"] == 0 and
                    xp["progression_after_exp"] - xp["progression_before_exp"] == xp["progression_applied_xp"] == (
                        value["pctx_current_exp"] - xp["progression_before_exp"]), "progression_public_XP_arithmetic")
            else:
                _require(all(xp["progression_" + name] == 0 for name in ("requested_xp", "computed_xp", "applied_xp", "before_exp", "after_exp")),
                         "progression_public_level_reward")
    elif kind == 2:
        for name, size in (("session", 3), ("producer", 2), ("start_context_key", 3), ("end_context_key", 3),
                           ("first_progression_source_key", 3), ("last_progression_source_key", 3)):
            _positive_key(value[name], size, nullable=name not in ("session", "producer"))
        for name in ("start_monotonic_usec", "through_monotonic_usec", "observed_connected_usec", "observed_heuristic_active_usec",
            "observed_idle_usec", "unknown_activity_usec", "resident_linkdead_usec", "covered_interval_usec", "missing_interval_usec",
            "interval_source_count"):
            _integer(value[name])
        _require(value["status"] in ("observed_completion", "censored", "unfinished", "unknown_context") and
            type(value["left_censored"]) is bool and type(value["right_censored"]) is bool and
            value["observed_connected_usec"] == value["observed_heuristic_active_usec"] + value["observed_idle_usec"] + value["unknown_activity_usec"] and
            value["covered_interval_usec"] == value["observed_connected_usec"] + value["resident_linkdead_usec"] and
            value["through_monotonic_usec"] - value["start_monotonic_usec"] == value["covered_interval_usec"] + value["missing_interval_usec"],
            "progression_public_milestone_conservation")
        qualified = value["status"] == "observed_completion" and not value["left_censored"] and not value["unknown"]
        for name, observed in (("full_stage_connected_usec", "observed_connected_usec"),
            ("full_stage_heuristic_active_usec", "observed_heuristic_active_usec"), ("full_stage_elapsed_usec", "observed_elapsed_usec")):
            _require(value[name] == (value[observed] if qualified else None), "progression_public_censored_denominator")
    elif kind == 3:
        _require(value["basis"] in identity_publication.BASES, "progression_public_basis")
        known = value["basis"] in ("character", "account", "controller")
        _require(value["effort_category"] in ("heuristic_active", "connected") and type(value["sequential_switch_coverage_complete"]) is bool,
                 "progression_public_rotation_category")
        for name in ("summed_character_usec", "unknown_clock_or_quality_character_usec", "observed_characters"):
            _integer(value[name])
        for name in ("covered_union_usec", "union_usec", "observed_simultaneous_session_usec", "observed_simultaneous_character_usec",
                     "maximum_observed_simultaneous_sessions", "ambiguous_region_transitions"):
            _require(value[name] is None if not known else value[name] is None and name == "union_usec" or
                     type(value[name]) is int and 0 <= value[name] <= value["summed_character_usec"], "progression_public_rotation_metric")
        _require(value["sequential_switches"] is None if not known else type(value["sequential_switches"]) is list,
                 "progression_public_switch_population")
        for row in value["sequential_switches"] or ():
            _require(type(row) is dict and set(row) == {"from_character", "to_character", "previous_observed_through_utc_usec",
                "next_observed_from_utc_usec", "elapsed_gap_usec", "gap_activity_observed"} and row["gap_activity_observed"] is False and
                row["from_character"] != row["to_character"] and row["elapsed_gap_usec"] == (
                    row["next_observed_from_utc_usec"] - row["previous_observed_through_utc_usec"]) >= 0, "progression_public_switch_gap")
    elif kind == 4:
        _positive_key(value["scope"], 2)
        _require(value["category"] in identity.CATEGORIES and value["basis"] in identity_publication.BASES,
                 "progression_public_effort_category")
        for name in ("character_usec", "covered_character_usec", "unknown_clock_character_usec", "distinct_characters", "distinct_accounts"):
            _integer(value[name])
        _require(value["character_usec"] == value["covered_character_usec"] + value["unknown_clock_character_usec"],
                 "progression_public_effort_conservation")
        _quality(value["quality_flags"])
    else:
        _require(value["basis"] in identity_publication.BASES, "progression_public_basis")
        _integer(value["config_id"], lower=1)
        _integer(value["level_stage"], upper=65535)
        _integer(value["source"], upper=12)
        _integer(value["reason"], upper=6)
        _integer(value["observation_status"], lower=1, upper=3)
        _require(value["economic_authority_dependency_issue"] == 487, "progression_public_economic_dependency")
        _require(value["denominator_scope"] == "observed_level_build_configuration_rested_selection_formal_group_size_exposure",
                 "progression_public_denominator_scope")
        for name in ("requested_xp", "computed_xp", "observed_applied_xp"):
            _integer(value[name], lower=-(1 << 63), upper=(1 << 63)-1)
        for name in ("observed_earned_positive_xp", "death_loss_xp", "restored_positive_xp", "threshold_consumed_xp",
            "administrative_positive_xp", "administrative_negative_xp", "system_positive_xp", "system_negative_xp",
            "uncategorized_positive_xp", "uncategorized_negative_xp", "XP_observations", "level_advances", "level_losses",
            "summed_character_connected_usec", "summed_character_heuristic_active_usec", "unclassified_connected_character_usec"):
            _integer(value[name])
        _require(type(value["source_keys"]) is list and len(value["source_keys"]) <= MAX_INPUTS, "progression_public_XP_sources")
        for key in value["source_keys"]:
            _positive_key(key)
        for rate, denominator in (("observed_earned_xp_per_connected_hour", "covered_connected_union_usec"),
                                  ("observed_earned_xp_per_heuristic_active_hour", "covered_heuristic_active_union_usec")):
            if value[denominator] is not None:
                _integer(value[denominator])
            expected = None if value["basis"] not in ("character", "account", "controller") or value["unknown"] or not value[denominator] else (
                dict(numerator_xp=value["observed_earned_positive_xp"], denominator_usec=value[denominator], scale_usec_per_hour=3_600_000_000))
            _require(value[rate] == expected and (value[rate] is None or type(value[rate]) is dict and
                all(type(number) is int for number in value[rate].values())),
                     "progression_public_rate_denominator")
    return value


def encode_row(scope, kind, value):
    _scope(scope)
    _require(type(kind) is int and kind in VALUE_FIELDS, "progression_public_row_kind")
    # Normalize tuples to the same JSON list shapes a restricted reader sees.
    payload = _canonical(value)
    _require(0 < len(payload) <= MAX_PAYLOAD_BYTES, "progression_output_payload_capacity")
    canonical = incident.decode_evidence_packet(payload, max_bytes=MAX_PAYLOAD_BYTES)
    _validate_value(kind, canonical)
    _require(kind != 1 or tuple(canonical["pctx_" + name] for name in ("environment_id", "season_id")) == scope[2:],
             "progression_public_context_scope")
    _require(kind != 4 or canonical["scope"] == list(scope[2:]), "progression_public_effort_scope")
    digest = hashlib.sha256(payload).digest()
    key = hashlib.sha256(b"duris-progression-row-v1:" + _canonical(scope) + bytes((kind,)) + digest).digest()
    return dict(zip(SCOPE, scope, strict=True), row_kind=kind, row_key=key, payload=payload,
                payload_digest=digest, quality_flags=_quality(value.get("quality_flags", value.get("pctx_quality_flags", 0))))


def decode_row(scope, row):
    _scope(scope)
    _require(set(row) == set(ROW_COLUMNS) and tuple(row[name] for name in SCOPE) == scope, "progression_public_row_scope")
    _require(type(row["row_kind"]) is int and row["row_kind"] in VALUE_FIELDS and type(row["payload"]) is bytes and
        0 < len(row["payload"]) <= MAX_PAYLOAD_BYTES and hashlib.sha256(row["payload"]).digest() == _digest(row["payload_digest"]),
        "progression_public_row_digest")
    try:
        value = incident.decode_evidence_packet(row["payload"], max_bytes=MAX_PAYLOAD_BYTES)
    except incident.IncidentError as error:
        raise PublicationError(str(error)) from error
    expected = encode_row(scope, row["row_kind"], value)
    _require(expected == row, "progression_public_row_key_or_canonical_value")
    return value


def evidence_rows(scope, values: EvidenceValues, *, check_deadline=lambda: None):
    result = []
    for kind, rows in enumerate((values.contexts, values.milestones, values.rotations, values.effort, values.portfolio), 1):
        for value in rows:
            check_deadline()
            _require(len(result) < MAX_OUTPUT_ROWS, "progression_output_capacity")
            result.append(encode_row(scope, kind, value))
    _require(len({(row["row_kind"], row["row_key"]) for row in result}) == len(result), "progression_public_row_replay")
    return tuple(result)


COUNT_FIELDS = ("context_row_count", "milestone_row_count", "rotation_row_count", "effort_row_count", "portfolio_row_count")
COVERAGE_COLUMNS = (*HEADER_COLUMNS, "snapshot_digest", *COUNT_FIELDS, "unknown_context_count", "completed_milestone_count",
    "unfinished_milestone_count", "censored_milestone_count", "qualified_full_stage_count", "qualified_rate_cell_count")
SNAPSHOT_COLUMNS = ("row_kind", "row_key", "payload_digest", "quality_flags")


def snapshot_digest(scope, receipts, *, check_deadline=lambda: None):
    _scope(scope)
    _require(len(receipts) <= MAX_OUTPUT_ROWS, "progression_snapshot_capacity")
    digest = hashlib.sha256(b"duris-progression-snapshot-v1:" + _canonical(scope)).digest()
    previous = None
    for row in receipts:
        check_deadline()
        _require(set(row) == set(SNAPSHOT_COLUMNS) and type(row["row_kind"]) is int and row["row_kind"] in VALUE_FIELDS,
                 "progression_snapshot_columns_or_kind")
        key = row["row_kind"], _digest(row["row_key"])
        _require(previous is None or previous < key, "progression_snapshot_order_or_replay")
        previous = key
        digest = hashlib.sha256(digest + bytes((row["row_kind"],)) + row["row_key"] +
            _digest(row["payload_digest"]) + _quality(row["quality_flags"]).to_bytes(8, "big")).digest()
    return digest


@dataclass(frozen=True, slots=True)
class PublicationValues:
    header: Mapping[str, Any]
    rows: tuple[Mapping[str, Any], ...]


def build_publication(window: VerifiedSource, registry: identity.Registry | None, coverage: Mapping[str, Any], *,
                      check_deadline=lambda: None, **bounds):
    """Compute one immutable output snapshot; the caller owns the atomic SQL TX."""
    _require(window.header["publication_complete"] == 0, "progression_source_already_published")
    values = build_evidence(window, registry, coverage, check_deadline=check_deadline, **bounds)
    rows = tuple(sorted(evidence_rows(window.scope, values, check_deadline=check_deadline), key=lambda row: (row["row_kind"], row["row_key"])))
    receipts = [{name: row[name] for name in SNAPSHOT_COLUMNS} for row in rows]
    counts = {name: sum(row["row_kind"] == kind for row in rows) for kind, name in enumerate(COUNT_FIELDS, 1)}
    quality = window.header["quality_flags"] | coverage.get("quality_flags", 0)
    for row in rows:
        quality |= row["quality_flags"]
    header = dict(window.header, publication_complete=1, quality_flags=_quality(quality), **counts,
        snapshot_digest=snapshot_digest(window.scope, receipts, check_deadline=check_deadline),
        unknown_context_count=sum(bool(value["unknown"]) for value in values.contexts),
        completed_milestone_count=sum(value["status"] == "observed_completion" for value in values.milestones),
        unfinished_milestone_count=sum(value["status"] == "unfinished" for value in values.milestones),
        censored_milestone_count=sum(value["status"] in ("censored", "unknown_context") for value in values.milestones),
        qualified_full_stage_count=sum(value["full_stage_elapsed_usec"] is not None for value in values.milestones),
        qualified_rate_cell_count=sum(value["observed_earned_xp_per_heuristic_active_hour"] is not None for value in values.portfolio))
    _require(set(header) == set(COVERAGE_COLUMNS), "progression_publication_header_columns")
    return PublicationValues(header, rows)


def public_header(header, reservation, receipts, *, check_deadline=lambda: None):
    """Verify published rows through aggregate receipts, without private inputs."""
    _require(set(header) == set(COVERAGE_COLUMNS), "progression_publication_header_columns")
    scope = validate_header({name: header[name] for name in HEADER_COLUMNS})
    _require(header["publication_complete"] == 1 and tuple(reservation[name] for name in SCOPE) == scope,
             "progression_publication_incomplete_or_identity_scope")
    for name in (*COUNT_FIELDS, "unknown_context_count", "completed_milestone_count", "unfinished_milestone_count",
        "censored_milestone_count", "qualified_full_stage_count", "qualified_rate_cell_count"):
        _integer(header[name], upper=MAX_OUTPUT_ROWS)
    _require(sum(header[name] for name in COUNT_FIELDS) == len(receipts) <= MAX_OUTPUT_ROWS and
        all(header[name] == sum(row["row_kind"] == kind for row in receipts) for kind, name in enumerate(COUNT_FIELDS, 1)) and
        snapshot_digest(scope, receipts, check_deadline=check_deadline) == _digest(header["snapshot_digest"]),
        "progression_publication_detail_missing_or_changed")
    _require(header["unknown_context_count"] <= header["context_row_count"] and
        header["completed_milestone_count"] + header["unfinished_milestone_count"] + header["censored_milestone_count"] == header["milestone_row_count"] and
        header["qualified_full_stage_count"] <= header["completed_milestone_count"] and
        header["qualified_rate_cell_count"] <= header["portfolio_row_count"], "progression_publication_coverage_count")
    result = {name: value.hex() if type(value) is bytes else value for name, value in header.items()}
    result.update(identity=identity.public_generation(reservation), character_save_committed=False, XP_award_committed=False,
        complete_population_coverage_implied=False, time_to_level_median=None, universal_progression_score=None,
        economic_authority_dependency_issue=487)
    return result
