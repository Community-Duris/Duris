#!/usr/bin/env python3
"""Reviewed, append-only incident inventories; never reconstruct missing facts."""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re
import sys
from typing import Any, Mapping, Sequence

REGISTRY_SCHEMA_VERSION = 1
MAX_INCIDENTS = 64
MAX_PACKET_BYTES = 131_072
INCIDENT_ROW_BYTE_BOUND = 2_048
UINT64_MAX = (1 << 64) - 1
UTC_UNKNOWN = -(1 << 63)
KNOWN_KINDS = (1 << 9) - 2
# Sealed v1 inventories retain families 1..8; definition 3 retains independent
# v2 ownership history. Definition 4 uses v3 shared-battle facts; definitions
# 5 requires v4 contribution evidence; build observations require an independent
# v5 inventory. Earlier reviewed inventories stay sealed.
REGISTRY_SCHEMAS = {
    1: (KNOWN_KINDS, 8, "telemetry_incident_registry", "telemetry_incident"),
    2: ((1 << 10) - 2, 9, "telemetry_incident_registry_v2", "telemetry_incident_v2"),
    3: ((1 << 11) - 2, 10, "telemetry_incident_registry_v3", "telemetry_incident_v3"),
    4: ((1 << 12) - 2, 11, "telemetry_incident_registry_v4", "telemetry_incident_v4"),
    5: ((1 << 13) - 2, 12, "telemetry_incident_registry_v5", "telemetry_incident_v5"),
    6: ((1 << 14) - 2, 13, "telemetry_incident_registry_v6", "telemetry_incident_v6"),
}
QUALITY_INCIDENT_GAP = 1 << 27
QUALITY_INVENTORY_UNKNOWN = 1 << 28
BACKLOG = {"unknown": 0, "none": 1, "delivered": 2, "abandoned": 3,
           "mixed": 4, "retained_unresolved": 5}
PROVENANCE = {"original_observation": 1, "unavailable": 2,
              "reconstructed_separately": 3}
STATUS = {"active": 1, "withdrawn": 2}
RELATIONS = {1: "overlaps_occurrence_window", 2: "possible_overlap",
             3: "outside_occurrence_window"}
META_COLUMNS = (
    "environment_id", "season_id", "registry_version", "previous_registry_version",
    "reviewed_from_utc_usec", "reviewed_through_utc_usec", "reviewer_token",
    "review_evidence_digest", "packet_digest", "incident_count",
)
INCIDENT_COLUMNS = (
    "incident_id", "producer_boot_id", "producer_process_id", "start_utc_usec",
    "end_utc_usec", "record_kind_mask", "first_record_seq", "last_record_seq",
    "fix_reference_digest", "verified_boot_id", "verified_process_id",
    "verified_record_seq", "verified_record_kind", "verified_occurrence_utc_usec",
    "backlog_disposition", "observation_provenance", "evidence_digest", "status",
)
PUBLICATION_COLUMNS = (
    "definition_version", "generation", "environment_id", "season_id",
    "registry_version", "registry_digest", "reviewed_from_utc_usec",
    "reviewed_through_utc_usec", "incident_count", "relevant_incident_count",
    "unknown_end_count", "unresolved_backlog_count", "quality_flags",
)
PACKET_KEYS = frozenset(META_COLUMNS) - {"packet_digest", "incident_count"} | {
    "registry_schema_version", "incidents"}
INPUT_INCIDENT_KEYS = frozenset(INCIDENT_COLUMNS) - {
    "verified_boot_id", "verified_process_id", "verified_record_seq",
    "verified_record_kind", "verified_occurrence_utc_usec"} | {"first_verified_postfix"}
VERIFY_KEYS = frozenset({"boot_id", "process_id", "record_seq", "record_kind",
                         "occurrence_utc_usec"})


class IncidentError(ValueError):
    """A payload-free evidence/contract failure."""


def schema_contract(version: Any) -> tuple[int, int, str, str]:
    if type(version) is not int or version not in REGISTRY_SCHEMAS:
        raise IncidentError("unsupported_registry_version")
    return REGISTRY_SCHEMAS[version]


def generation_schema(definition_version: int) -> int:
    if type(definition_version) is not int or not 1 <= definition_version < (1 << 32):
        raise IncidentError("invalid_definition_version")
    return 6 if definition_version >= 7 else 5 if definition_version == 6 else 4 if definition_version == 5 else 3 if definition_version == 4 else 2 if definition_version == 3 else 1


def _stored_digest(value: Any, *, nullable: bool = False) -> bytes | None:
    if nullable and value is None:
        return None
    if type(value) is not bytes or len(value) != 32:
        raise IncidentError("invalid_stored_digest")
    return value


def _keys(value: Any, expected: frozenset[str]) -> Mapping[str, Any]:
    if not isinstance(value, dict) or set(value) != expected:
        raise IncidentError("invalid_fields")
    return value


def _integer(value: Any, *, maximum: int = UINT64_MAX, nullable: bool = False,
             allow_zero: bool = False) -> int | None:
    if nullable and value is None:
        return None
    if type(value) is not int or not (0 if allow_zero else 1) <= value <= maximum:
        raise IncidentError("invalid_integer")
    return value


def _utc(value: Any) -> int | None:
    if value is None:
        return None
    if type(value) is not int or not UTC_UNKNOWN < value < (1 << 63):
        raise IncidentError("invalid_utc")
    return value


def _digest(value: Any, *, nullable: bool = False) -> bytes | None:
    if nullable and value is None:
        return None
    if not isinstance(value, str) or not re.fullmatch(r"[0-9a-f]{64}", value):
        raise IncidentError("invalid_digest")
    return bytes.fromhex(value)


def _enum(value: Any, values: Mapping[str, int]) -> int:
    if not isinstance(value, str) or value not in values:
        raise IncidentError("invalid_enum")
    return values[value]


def _range(first: int | None, last: int | None) -> None:
    if first is not None and last is not None and first > last:
        raise IncidentError("reversed_range")


def validate_packet(packet: Any) -> tuple[dict[str, Any], tuple[dict[str, Any], ...]]:
    p = _keys(packet, PACKET_KEYS)
    known_kinds, max_kind, _registry_table, _incident_table = schema_contract(p["registry_schema_version"])
    meta = {name: _integer(p[name]) for name in ("environment_id", "season_id", "registry_version")}
    previous = _integer(p["previous_registry_version"], allow_zero=True)
    if previous == UINT64_MAX or meta["registry_version"] != previous + 1:
        raise IncidentError("nonconsecutive_registry_version")
    meta["previous_registry_version"] = previous
    for name in ("reviewed_from_utc_usec", "reviewed_through_utc_usec"):
        meta[name] = _utc(p[name])
    _range(meta["reviewed_from_utc_usec"], meta["reviewed_through_utc_usec"])
    for name in ("reviewer_token", "review_evidence_digest"):
        meta[name] = _digest(p[name])
    incidents = p["incidents"]
    if not isinstance(incidents, list) or len(incidents) > MAX_INCIDENTS:
        raise IncidentError("incident_capacity")
    rows = []
    ids: set[int] = set()
    for item in incidents:
        i = _keys(item, INPUT_INCIDENT_KEYS)
        row = {"incident_id": _integer(i["incident_id"])}
        if row["incident_id"] in ids:
            raise IncidentError("duplicate_incident")
        ids.add(row["incident_id"])
        for name in ("producer_boot_id", "producer_process_id", "first_record_seq", "last_record_seq"):
            row[name] = _integer(i[name], nullable=True)
        if (row["producer_boot_id"] is None) != (row["producer_process_id"] is None):
            raise IncidentError("partial_producer")
        if row["producer_boot_id"] is None and any(row[n] is not None for n in ("first_record_seq", "last_record_seq")):
            raise IncidentError("sequence_without_producer")
        _range(row["first_record_seq"], row["last_record_seq"])
        row["start_utc_usec"] = _utc(i["start_utc_usec"])
        row["end_utc_usec"] = _utc(i["end_utc_usec"])
        _range(row["start_utc_usec"], row["end_utc_usec"])
        row["record_kind_mask"] = _integer(i["record_kind_mask"])
        if row["record_kind_mask"] & ~known_kinds:
            raise IncidentError("unknown_record_family")
        for name in ("evidence_digest", "fix_reference_digest"):
            row[name] = _digest(i[name], nullable=name == "fix_reference_digest")
        row["backlog_disposition"] = _enum(i["backlog_disposition"], BACKLOG)
        row["observation_provenance"] = _enum(i["observation_provenance"], PROVENANCE)
        row["status"] = _enum(i["status"], STATUS)
        v = i["first_verified_postfix"]
        for name in INCIDENT_COLUMNS[9:14]:
            row[name] = None
        if v is not None:
            v = _keys(v, VERIFY_KEYS)
            if row["fix_reference_digest"] is None:
                raise IncidentError("verification_without_fix")
            for source, dest in (("boot_id", "verified_boot_id"), ("process_id", "verified_process_id"),
                                 ("record_seq", "verified_record_seq"), ("record_kind", "verified_record_kind")):
                row[dest] = _integer(v[source], maximum=max_kind if source == "record_kind" else UINT64_MAX)
            row["verified_occurrence_utc_usec"] = _utc(v["occurrence_utc_usec"])
            if not row["record_kind_mask"] & (1 << row["verified_record_kind"]):
                raise IncidentError("verification_family_mismatch")
        rows.append(row)
    rows.sort(key=lambda row: row["incident_id"])
    # List order is not identity. The digest covers every reviewed field, not
    # formatting, and makes an exact retry distinguishable from a correction.
    canonical = dict(p)
    canonical["incidents"] = sorted(incidents, key=lambda row: row["incident_id"])
    encoded = json.dumps(canonical, sort_keys=True, separators=(",", ":"), allow_nan=False).encode("utf-8")
    if len(encoded) > MAX_PACKET_BYTES:
        raise IncidentError("packet_capacity")
    meta["packet_digest"] = hashlib.sha256(encoded).digest()
    meta["incident_count"] = len(rows)
    return meta, tuple(rows)


def load_evidence_packet(path: Path, *, max_bytes: int = MAX_PACKET_BYTES) -> Any:
    """Shared bounded JSON decoding for reviewed evidence packet families."""
    if type(max_bytes) is not int or not 1 <= max_bytes <= 1_048_576:
        raise IncidentError("invalid_packet_bound")
    with path.open("rb") as stream:
        data = stream.read(max_bytes + 1)
    return decode_evidence_packet(data, max_bytes=max_bytes)


def decode_evidence_packet(data: bytes, *, max_bytes: int = MAX_PACKET_BYTES) -> Any:
    """Decode bounded file evidence or a retained canonical projection input."""
    if type(max_bytes) is not int or not 1 <= max_bytes <= 1_048_576:
        raise IncidentError("invalid_packet_bound")
    if type(data) is not bytes or len(data) > max_bytes:
        raise IncidentError("packet_capacity")
    def integer(text: str) -> int:
        # Evidence families have only signed/unsigned 64-bit integer fields.
        # Reserve at most 20 magnitude digits before int(), independently of
        # the host Python version's optional decimal conversion limit.
        if len(text.removeprefix("-")) > 20:
            raise IncidentError("invalid_number")
        return int(text)
    def invalid_number(_text: str) -> Any:
        raise IncidentError("invalid_number")
    def unique_pairs(pairs: Sequence[tuple[str, Any]]) -> dict[str, Any]:
        result: dict[str, Any] = {}
        for key, value in pairs:
            if key in result:
                raise IncidentError("duplicate_field")
            result[key] = value
        return result
    try:
        packet = json.loads(data, object_pairs_hook=unique_pairs, parse_int=integer,
                            parse_float=invalid_number, parse_constant=invalid_number)
        return packet
    except IncidentError:
        raise
    except (UnicodeError, ValueError, RecursionError):
        raise IncidentError("invalid_json") from None


def load_packet(path: Path) -> Any:
    packet = load_evidence_packet(path)
    validate_packet(packet)
    return packet


def occurrence_relation(row: Mapping[str, Any], start: int | None, end: int | None) -> int:
    first, last = row["start_utc_usec"], row["end_utc_usec"]
    if (last is not None and start is not None and last < start) or (
        first is not None and end is not None and first > end):
        return 3
    if all(value is not None for value in (first, last, start, end)):
        return 1
    return 2


def _input_row(row: Mapping[str, Any]) -> dict[str, Any]:
    try:
        values = {name: row[name] for name in INPUT_INCIDENT_KEYS - {"first_verified_postfix"}}
        for name in ("fix_reference_digest", "evidence_digest"):
            value = _stored_digest(values[name], nullable=name == "fix_reference_digest")
            values[name] = None if value is None else value.hex()
        values["backlog_disposition"] = {v: k for k, v in BACKLOG.items()}[row["backlog_disposition"]]
        values["observation_provenance"] = {v: k for k, v in PROVENANCE.items()}[row["observation_provenance"]]
        values["status"] = {v: k for k, v in STATUS.items()}[row["status"]]
        verify = tuple(row[name] for name in INCIDENT_COLUMNS[9:14])
        values["first_verified_postfix"] = None if all(v is None for v in verify) else dict(zip(
            ("boot_id", "process_id", "record_seq", "record_kind", "occurrence_utc_usec"), verify, strict=True))
        return values
    except (KeyError, TypeError, ValueError):
        raise IncidentError("invalid_stored_inventory") from None


def validate_stored(meta: Mapping[str, Any], rows: Sequence[Mapping[str, Any]], *,
                    registry_schema_version: int = REGISTRY_SCHEMA_VERSION) -> None:
    schema_contract(registry_schema_version)
    try:
        packet = {name: meta[name] for name in PACKET_KEYS - {"registry_schema_version", "incidents"}}
        for name in ("reviewer_token", "review_evidence_digest"):
            packet[name] = _stored_digest(packet[name]).hex()
        packet.update(registry_schema_version=registry_schema_version, incidents=[_input_row(row) for row in rows])
        computed, _ = validate_packet(packet)
        if len(rows) != meta["incident_count"] or computed["packet_digest"] != _stored_digest(meta["packet_digest"]):
            raise IncidentError("stored_inventory_digest_mismatch")
    except (KeyError, TypeError):
        raise IncidentError("invalid_stored_inventory") from None


def occurrence_window(state: Mapping[str, Any]) -> tuple[int | None, int | None]:
    # Rollup-owned UTC unknown/mismatch/backward/fanout bits 16..19 mean
    # observed labels cannot prove that an incident lies outside this report.
    if int(state["quality_flags"]) & (15 << 16):
        return None, None
    start, end = state["coverage_start_utc_usec"], state["coverage_end_utc_usec"]
    if start is not None and end is not None and start > end:
        return None, None
    return start, end


def publication_summary(scope: tuple[int, int, int, int], meta: Mapping[str, Any] | None,
                        rows: Sequence[Mapping[str, Any]]) -> dict[str, Any]:
    if len(rows) > MAX_INCIDENTS or (meta is not None and len(rows) != int(meta["incident_count"])):
        raise IncidentError("incomplete_inventory")
    relevant = [r for r in rows if int(r["status"]) == STATUS["active"] and int(r["occurrence_relation"]) != 3]
    return dict(zip(PUBLICATION_COLUMNS, (
        *scope, 0 if meta is None else int(meta["registry_version"]),
        None if meta is None else meta["packet_digest"],
        None if meta is None else meta["reviewed_from_utc_usec"],
        None if meta is None else meta["reviewed_through_utc_usec"], len(rows), len(relevant),
        sum(r["end_utc_usec"] is None for r in relevant),
        sum(int(r["backlog_disposition"]) in (0, 5) for r in relevant),
        QUALITY_INVENTORY_UNKNOWN if meta is None else (QUALITY_INCIDENT_GAP if relevant else 0),
    ), strict=True))


def public_coverage(meta: Mapping[str, Any] | None, rows: Sequence[Mapping[str, Any]], *,
                    occurrence_window: tuple[int | None, int | None] | None = None,
                    registry_schema_version: int = REGISTRY_SCHEMA_VERSION) -> dict[str, Any]:
    schema_contract(registry_schema_version)
    if meta is None:
        if rows:
            raise IncidentError("incomplete_publication")
        return {"registry_schema_version": registry_schema_version, "status": "not_published",
                "registry_version": None, "quality_flags": QUALITY_INVENTORY_UNKNOWN,
                "incident_inventory_complete": False, "zero_activity_implied": False, "incidents": []}
    if len(rows) > MAX_INCIDENTS or len(rows) != int(meta["incident_count"]):
        raise IncidentError("incomplete_publication")
    # Existing v1 generations can advance their input watermark incrementally.
    # Keep the reviewed version frozen, but classify its gaps against the same
    # current occurrence window as the report's other coverage metadata.
    rows = [dict(row) for row in rows]
    if occurrence_window is not None:
        for row in rows:
            row["occurrence_relation"] = occurrence_relation(row, *occurrence_window)
    reverse_backlog = {value: key for key, value in BACKLOG.items()}
    reverse_provenance = {value: key for key, value in PROVENANCE.items()}
    reverse_status = {value: key for key, value in STATUS.items()}
    public = []
    for source in rows:
        # Reuse the packet field contract for materialized rows. A projection
        # cannot bless an invalid enum, sentinel UTC or half-present key.
        check = template(registry_schema_version)
        check.update(reviewer_token="00" * 32, review_evidence_digest="00" * 32,
                     incidents=[_input_row(source)])
        validate_packet(check)
        row = {name: source[name] for name in INCIDENT_COLUMNS}
        for name in ("evidence_digest", "fix_reference_digest"):
            value = _stored_digest(row[name], nullable=name == "fix_reference_digest")
            row[name] = None if value is None else value.hex()
        row["backlog_disposition"] = reverse_backlog[int(row["backlog_disposition"])]
        row["observation_provenance"] = reverse_provenance[int(row["observation_provenance"])]
        row["status"] = reverse_status[int(row["status"])]
        row["occurrence_relation"] = RELATIONS[int(source["occurrence_relation"])]
        row["gap_remains_after_reconstruction"] = row["observation_provenance"] == "reconstructed_separately"
        public.append(row)
    version = int(meta["registry_version"])
    relevant = [r for r in rows if int(r["status"]) == STATUS["active"] and int(r["occurrence_relation"]) != 3]
    if not version and (rows or meta["registry_digest"] is not None):
        raise IncidentError("invalid_publication")
    return {
        "registry_schema_version": registry_schema_version,
        "status": "reviewed_inventory" if version else "not_registered",
        "registry_version": version or None,
        "registry_digest": None if meta["registry_digest"] is None else _stored_digest(meta["registry_digest"]).hex(),
        "reviewed_from_utc_usec": meta["reviewed_from_utc_usec"],
        "reviewed_through_utc_usec": meta["reviewed_through_utc_usec"],
        "incident_count": int(meta["incident_count"]),
        "relevant_incident_count": len(relevant),
        "unknown_end_count": sum(r["end_utc_usec"] is None for r in relevant),
        "unresolved_backlog_count": sum(int(r["backlog_disposition"]) in (0, 5) for r in relevant),
        "quality_flags": QUALITY_INVENTORY_UNKNOWN if not version else (QUALITY_INCIDENT_GAP if relevant else 0),
        "incident_inventory_complete": False,
        "zero_activity_implied": False,
        "incidents": public,
    }


def template(registry_schema_version: int = REGISTRY_SCHEMA_VERSION) -> dict[str, Any]:
    known_kinds, _max_kind, _registry_table, _incident_table = schema_contract(registry_schema_version)
    return {"registry_schema_version": registry_schema_version, "environment_id": 1, "season_id": 1,
            "registry_version": 1, "previous_registry_version": 0,
            "reviewed_from_utc_usec": None, "reviewed_through_utc_usec": None,
            "reviewer_token": "REQUIRED-OPAQUE-SHA256", "review_evidence_digest": "REQUIRED-SHA256",
            "incidents": [{"incident_id": 1, "producer_boot_id": None, "producer_process_id": None,
                           "start_utc_usec": None, "end_utc_usec": None, "record_kind_mask": known_kinds,
                           "first_record_seq": None, "last_record_seq": None,
                           "fix_reference_digest": None, "first_verified_postfix": None,
                           "backlog_disposition": "unknown", "observation_provenance": "unavailable",
                           "evidence_digest": "REQUIRED-SHA256", "status": "active"}]}


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("packet", nargs="?", type=Path)
    parser.add_argument("--template", action="store_true")
    parser.add_argument("--registry-schema-version", type=int, choices=tuple(REGISTRY_SCHEMAS),
                        help="Template schema; registration uses the packet's explicit schema.")
    parser.add_argument("--register", action="store_true", help="append reviewed inventory through TELEMETRY_INCIDENT_DB_* credentials")
    args = parser.parse_args(argv)
    try:
        if args.template:
            if args.packet is not None or args.register:
                raise IncidentError("invalid_command")
            print(json.dumps(template(args.registry_schema_version or REGISTRY_SCHEMA_VERSION), indent=2, sort_keys=True))
            return 0
        if args.packet is None:
            raise IncidentError("packet_required")
        if args.registry_schema_version is not None:
            raise IncidentError("schema_option_requires_template")
        packet = load_packet(args.packet)
        meta, _rows = validate_packet(packet)
        if not args.register:
            print(json.dumps({"status": "valid_packet", "registry_schema_version": packet["registry_schema_version"], "registry_version": meta["registry_version"],
                              "incident_count": meta["incident_count"], "packet_digest": meta["packet_digest"].hex()}))
            return 0
        # Lazy imports keep validation/template operations connection-free.
        try:
            from .db_access import ConnectionSettings, PyMySQLConnectionFactory, PyMySQLRollupDatabase
        except ImportError:
            from db_access import ConnectionSettings, PyMySQLConnectionFactory, PyMySQLRollupDatabase
        database = PyMySQLRollupDatabase(PyMySQLConnectionFactory(ConnectionSettings.from_env("TELEMETRY_INCIDENT_DB_")))
        try:
            print(json.dumps(dict(database.register_incident_packet(packet)), sort_keys=True))
        finally:
            database.close()
        return 0
    except IncidentError as error:
        print(json.dumps({"status": "refused", "reason": str(error)}))
        return 2
    except Exception:
        # Connector diagnostics may contain credentials, paths or statement text.
        print(json.dumps({"status": "refused", "reason": "storage_or_contract_failure"}))
        return 2


if __name__ == "__main__":
    sys.exit(main())
