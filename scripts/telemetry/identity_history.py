#!/usr/bin/env python3
"""Dated reviewed identity and exact observed effort, without identity inference.

Ownership inputs come from authenticated capture, not current account tables.
Restricted SQL registration authenticates a provisioned database reviewer and
issued account tokens; generation reservations freeze reviewed versions. The
bounded pure seams implement dated
review/correction, same-producer ownership cuts, and independent account and
confirmed-controller effort. Names, email, IP and devices are not inputs.
"""
from __future__ import annotations

import argparse
from dataclasses import dataclass
import hashlib
import json
from pathlib import Path
import re
from typing import Any, Mapping, Sequence

try:
    from .incident import IncidentError, load_evidence_packet
    from .rollup_definitions import (UTC_UNKNOWN, UINT64_MAX, ROLLUP_QUALITY_MASK,
        ROLLUP_QUALITY_UTC_UNKNOWN, ROLLUP_QUALITY_UTC_MISMATCH,
        ROLLUP_QUALITY_UTC_BACKWARD, ROLLUP_QUALITY_UTC_FANOUT)
except ImportError:
    from incident import IncidentError, load_evidence_packet
    from rollup_definitions import (UTC_UNKNOWN, UINT64_MAX, ROLLUP_QUALITY_MASK,
        ROLLUP_QUALITY_UTC_UNKNOWN, ROLLUP_QUALITY_UTC_MISMATCH,
        ROLLUP_QUALITY_UTC_BACKWARD, ROLLUP_QUALITY_UTC_FANOUT)

REGISTRY_SCHEMA_VERSION = 1
MAX_ASSOCIATIONS = 1_024
MAX_PACKET_BYTES = 1_048_576
MAX_OWNERSHIP_OBSERVATIONS = 1_024
MAX_EFFORT_SLICES = 16_384
CLOCK_DISCONTINUITY = 1 << 7
UTC_ATTRIBUTION_FLAGS = (CLOCK_DISCONTINUITY | ROLLUP_QUALITY_UTC_UNKNOWN |
    ROLLUP_QUALITY_UTC_MISMATCH | ROLLUP_QUALITY_UTC_BACKWARD | ROLLUP_QUALITY_UTC_FANOUT)
PROVENANCES = frozenset({"account_owner_confirmation", "staff_review", "historical_review"})
STATUSES = frozenset({"confirmed", "unknown", "withdrawn"})
OWNERSHIP_SOURCES = frozenset({"authenticated_login", "authenticated_reconnect",
                             "authenticated_copyover", "authenticated_change", "unavailable"})
CATEGORIES = frozenset({"active", "idle", "unknown", "resident_linkdead", "presence"})
PACKET_KEYS = frozenset({"registry_schema_version", "environment_id", "season_id",
    "registry_version", "previous_registry_version", "previous_packet_digest",
    "reviewed_from_utc_usec", "reviewed_through_utc_usec", "reviewed_at_utc_usec",
    "reviewer_token", "review_evidence_digest", "associations"})
ASSOCIATION_KEYS = frozenset({"association_id", "account_token", "controller_token",
    "valid_from_utc_usec", "valid_through_utc_usec", "status", "provenance", "evidence_digest"})
STATUS_CODES = {"confirmed": 1, "unknown": 2, "withdrawn": 3}
PROVENANCE_CODES = {"account_owner_confirmation": 1, "staff_review": 2, "historical_review": 3}
META_COLUMNS = ("environment_id", "season_id", "registry_version", "previous_registry_version",
    "previous_packet_digest", "reviewed_from_utc_usec", "reviewed_through_utc_usec",
    "reviewed_at_utc_usec", "reviewer_token", "review_evidence_digest", "packet_digest",
    "association_count", "database_principal", "registered_at_utc_usec")
ASSOCIATION_COLUMNS = ("association_id", "account_token", "controller_token",
    "valid_from_utc_usec", "valid_through_utc_usec", "status", "provenance", "evidence_digest")
GENERATION_COLUMNS = ("definition_version", "generation", "environment_id", "season_id",
    "registry_version", "registry_digest", "reviewed_from_utc_usec", "reviewed_through_utc_usec",
    "reviewed_at_utc_usec", "association_count")


class IdentityError(ValueError):
    """Payload-free identity contract failure."""


def _integer(value: Any, name: str, *, zero: bool = False, nullable: bool = False) -> int | None:
    if nullable and value is None:
        return None
    if type(value) is not int or not (0 if zero else 1) <= value <= UINT64_MAX:
        raise IdentityError("invalid_" + name)
    return value


def _utc(value: Any, name: str, *, nullable: bool = False) -> int | None:
    if nullable and value is None:
        return None
    if type(value) is not int or not UTC_UNKNOWN < value < (1 << 63):
        raise IdentityError("invalid_" + name)
    return value


def _digest(value: Any, name: str, *, nullable: bool = False) -> str | None:
    if nullable and value is None:
        return None
    if type(value) is not str or not re.fullmatch("[0-9a-f]{64}", value) or value == "0" * 64:
        raise IdentityError("invalid_" + name)
    return value


def _stored_digest(value: Any, name: str, *, nullable: bool = False) -> str | None:
    if nullable and value is None:
        return None
    if type(value) is not bytes or len(value) != 32:
        raise IdentityError("invalid_stored_" + name)
    return _digest(value.hex(), name)


def _keys(value: Any, keys: frozenset[str]) -> Mapping[str, Any]:
    if type(value) is not dict or set(value) != keys:
        raise IdentityError("invalid_fields")
    return value


def _identity_tuple(value: Any, size: int, name: str) -> None:
    if type(value) is not tuple or len(value) != size:
        raise IdentityError("invalid_" + name)
    for item in value:
        _integer(item, name)


@dataclass(frozen=True, slots=True)
class Association:
    association_id: int
    account_token: int
    controller_token: int | None
    valid_from_utc_usec: int
    valid_through_utc_usec: int | None
    status: str
    provenance: str
    evidence_digest: str

    def __post_init__(self) -> None:
        for name in ("association_id", "account_token"):
            _integer(getattr(self, name), name)
        _integer(self.controller_token, "controller_token", nullable=True)
        _utc(self.valid_from_utc_usec, "valid_from")
        _utc(self.valid_through_utc_usec, "valid_through", nullable=True)
        if self.valid_through_utc_usec is not None and self.valid_through_utc_usec <= self.valid_from_utc_usec:
            raise IdentityError("empty_or_reversed_association")
        if type(self.status) is not str or type(self.provenance) is not str or self.status not in STATUSES or self.provenance not in PROVENANCES:
            raise IdentityError("invalid_association_enum")
        if (self.status == "confirmed" and self.controller_token is None) or (
            self.status == "unknown" and self.controller_token is not None):
            raise IdentityError("association_status_identity")
        _digest(self.evidence_digest, "evidence_digest")

    def input_dict(self) -> dict[str, Any]:
        return {name: getattr(self, name) for name in ASSOCIATION_KEYS}


@dataclass(frozen=True, slots=True)
class Registry:
    environment_id: int
    season_id: int
    registry_version: int
    previous_registry_version: int
    previous_packet_digest: str | None
    reviewed_from_utc_usec: int
    reviewed_through_utc_usec: int
    reviewed_at_utc_usec: int
    reviewer_token: str
    review_evidence_digest: str
    associations: tuple[Association, ...]

    def __post_init__(self) -> None:
        for name in ("environment_id", "season_id", "registry_version"):
            _integer(getattr(self, name), name)
        _integer(self.previous_registry_version, "previous_registry_version", zero=True)
        if self.registry_version != self.previous_registry_version + 1:
            raise IdentityError("nonconsecutive_registry_version")
        _digest(self.previous_packet_digest, "previous_packet_digest", nullable=True)
        if (self.previous_registry_version == 0) != (self.previous_packet_digest is None):
            raise IdentityError("previous_digest_identity")
        for name in ("reviewed_from_utc_usec", "reviewed_through_utc_usec", "reviewed_at_utc_usec"):
            _utc(getattr(self, name), name)
        if not self.reviewed_from_utc_usec < self.reviewed_through_utc_usec <= self.reviewed_at_utc_usec:
            raise IdentityError("invalid_review_window")
        _digest(self.reviewer_token, "reviewer_token")
        _digest(self.review_evidence_digest, "review_evidence_digest")
        if type(self.associations) is not tuple or len(self.associations) > MAX_ASSOCIATIONS or any(
            type(row) is not Association for row in self.associations):
            raise IdentityError("association_capacity_or_type")
        ids: set[int] = set()
        latest_end: dict[int, int | None] = {}
        for row in sorted(self.associations, key=lambda row: (row.account_token, row.valid_from_utc_usec, row.association_id)):
            if row.association_id in ids:
                raise IdentityError("duplicate_association_id")
            ids.add(row.association_id)
            if row.valid_from_utc_usec >= self.reviewed_through_utc_usec or (
                row.valid_through_utc_usec is not None and row.valid_through_utc_usec > self.reviewed_at_utc_usec):
                raise IdentityError("unreviewed_future_association")
            if row.status == "withdrawn":
                continue
            if row.account_token in latest_end and (latest_end[row.account_token] is None or
                row.valid_from_utc_usec < latest_end[row.account_token]):
                raise IdentityError("overlapping_account_associations")
            latest_end[row.account_token] = row.valid_through_utc_usec
        # Canonical size is bounded even when constructed directly, before
        # hashing or retaining it as a candidate reviewed version.
        if len(self.canonical_bytes()) > MAX_PACKET_BYTES:
            raise IdentityError("packet_capacity")

    @classmethod
    def from_packet(cls, packet: Any) -> Registry:
        p = _keys(packet, PACKET_KEYS)
        if type(p["registry_schema_version"]) is not int or p["registry_schema_version"] != REGISTRY_SCHEMA_VERSION:
            raise IdentityError("unsupported_registry_schema")
        rows = p["associations"]
        if type(rows) is not list or len(rows) > MAX_ASSOCIATIONS:
            raise IdentityError("association_capacity")
        return cls(**{name: p[name] for name in PACKET_KEYS - {"registry_schema_version", "associations"}},
                   associations=tuple(Association(**_keys(row, ASSOCIATION_KEYS)) for row in rows))

    def input_dict(self) -> dict[str, Any]:
        result = {name: getattr(self, name) for name in PACKET_KEYS - {"registry_schema_version", "associations"}}
        result.update(registry_schema_version=REGISTRY_SCHEMA_VERSION,
                      associations=[row.input_dict() for row in sorted(self.associations, key=lambda row: row.association_id)])
        return result

    def canonical_bytes(self) -> bytes:
        return json.dumps(self.input_dict(), sort_keys=True, separators=(",", ":"), allow_nan=False).encode("utf-8")

    @property
    def packet_digest(self) -> str:
        return hashlib.sha256(self.canonical_bytes()).hexdigest()

    def linkage_at(self, account_token: int | None, at_utc_usec: int | None) -> tuple[int | None, int | None, str]:
        _integer(account_token, "account_token", nullable=True)
        _utc(at_utc_usec, "point", nullable=True)
        if account_token is None:
            return None, None, "unknown_account"
        if at_utc_usec is None:
            return None, None, "clock_unknown"
        if not self.reviewed_from_utc_usec <= at_utc_usec < self.reviewed_through_utc_usec:
            return None, None, "outside_reviewed_window"
        for row in self.associations:
            if row.status != "withdrawn" and row.account_token == account_token and row.valid_from_utc_usec <= at_utc_usec and (
                row.valid_through_utc_usec is None or at_utc_usec < row.valid_through_utc_usec):
                return row.controller_token, row.association_id, row.status
        return None, None, "no_reviewed_mapping"


def validate_successor(previous: Registry | None, current: Registry) -> str:
    """An exact retry is allowed; corrections retain earlier complete versions."""
    if type(current) is not Registry or (previous is not None and type(previous) is not Registry):
        raise IdentityError("invalid_registry_type")
    if previous is None:
        if current.previous_registry_version != 0:
            raise IdentityError("missing_registry_history")
        return "new_version"
    if (previous.environment_id, previous.season_id) != (current.environment_id, current.season_id):
        raise IdentityError("registry_scope_mismatch")
    if previous.registry_version == current.registry_version:
        if previous.packet_digest != current.packet_digest:
            raise IdentityError("registry_version_conflict")
        return "already_registered"
    if current.previous_registry_version != previous.registry_version or current.previous_packet_digest != previous.packet_digest:
        raise IdentityError("registry_history_conflict")
    if current.reviewed_at_utc_usec < previous.reviewed_at_utc_usec:
        raise IdentityError("review_time_regressed")
    new = {row.association_id: row for row in current.associations}
    for old in previous.associations:
        if old.association_id not in new:
            raise IdentityError("association_removed_without_withdrawal")
        if old.account_token != new[old.association_id].account_token:
            raise IdentityError("association_account_changed")
    return "new_version"


def load_registry(path: Path) -> Registry:
    try:
        return Registry.from_packet(load_evidence_packet(path, max_bytes=MAX_PACKET_BYTES))
    except IncidentError as error:
        raise IdentityError(str(error)) from None


def storage_rows(registry: Registry, principal: str, registered_at: int) -> tuple[dict[str, Any], list[dict[str, Any]]]:
    if type(registry) is not Registry or type(principal) is not str or not 0 < len(principal) <= 384:
        raise IdentityError("invalid_registration_authority")
    _utc(registered_at, "registered_at")
    if registered_at < registry.reviewed_at_utc_usec:
        raise IdentityError("unreviewed_future_registration")
    meta = {name: getattr(registry, name) for name in META_COLUMNS[:-4]}
    for name in ("previous_packet_digest", "reviewer_token", "review_evidence_digest"):
        meta[name] = None if meta[name] is None else bytes.fromhex(meta[name])
    meta.update(packet_digest=bytes.fromhex(registry.packet_digest),
                association_count=len(registry.associations), database_principal=principal,
                registered_at_utc_usec=registered_at)
    rows = []
    for association in sorted(registry.associations, key=lambda row: row.association_id):
        row = association.input_dict()
        row.update(status=STATUS_CODES[association.status], provenance=PROVENANCE_CODES[association.provenance],
                   evidence_digest=bytes.fromhex(association.evidence_digest))
        rows.append(row)
    return meta, rows


def registry_from_storage(meta: Mapping[str, Any], rows: Sequence[Mapping[str, Any]]) -> Registry:
    """Verify retained SQL bytes before using or retrying a reviewed version."""
    try:
        if len(rows) > MAX_ASSOCIATIONS or type(meta["association_count"]) is not int or len(rows) != meta["association_count"]:
            raise IdentityError("stored_registry_incomplete")
        if type(meta["database_principal"]) is not str or not 0 < len(meta["database_principal"]) <= 384:
            raise IdentityError("invalid_registration_authority")
        reverse_status = {value: key for key, value in STATUS_CODES.items()}
        reverse_provenance = {value: key for key, value in PROVENANCE_CODES.items()}
        packet = {name: meta[name] for name in PACKET_KEYS - {"registry_schema_version", "associations"}}
        for name in ("previous_packet_digest", "reviewer_token", "review_evidence_digest"):
            packet[name] = _stored_digest(packet[name], name, nullable=name == "previous_packet_digest")
        associations = []
        for source in rows:
            row = {name: source[name] for name in ASSOCIATION_COLUMNS}
            if type(row["status"]) is not int or type(row["provenance"]) is not int:
                raise IdentityError("invalid_stored_association_enum")
            row.update(status=reverse_status[row["status"]], provenance=reverse_provenance[row["provenance"]],
                       evidence_digest=_stored_digest(row["evidence_digest"], "evidence_digest"))
            associations.append(row)
        packet.update(registry_schema_version=REGISTRY_SCHEMA_VERSION, associations=associations)
        registry = Registry.from_packet(packet)
        _utc(meta["registered_at_utc_usec"], "registered_at")
        if meta["registered_at_utc_usec"] < registry.reviewed_at_utc_usec:
            raise IdentityError("unreviewed_future_registration")
        if _stored_digest(meta["packet_digest"], "packet_digest") != registry.packet_digest:
            raise IdentityError("stored_registry_digest_mismatch")
        return registry
    except (KeyError, TypeError, ValueError, OverflowError) as error:
        if isinstance(error, IdentityError):
            raise
        raise IdentityError("invalid_stored_registry") from None


def generation_scope(scope: Any) -> tuple[int, int, int, int]:
    _identity_tuple(scope, 4, "generation_scope")
    if not 3 <= scope[0] < (1 << 32):
        raise IdentityError("identity_requires_balance_definition")
    return scope


def generation_row(scope: tuple[int, int, int, int], registry: Registry | None) -> dict[str, Any]:
    generation_scope(scope)
    if registry is not None and (registry.environment_id, registry.season_id) != scope[2:]:
        raise IdentityError("registry_scope_mismatch")
    return dict(zip(GENERATION_COLUMNS, (*scope,
        None if registry is None else registry.registry_version,
        None if registry is None else bytes.fromhex(registry.packet_digest),
        None if registry is None else registry.reviewed_from_utc_usec,
        None if registry is None else registry.reviewed_through_utc_usec,
        None if registry is None else registry.reviewed_at_utc_usec,
        0 if registry is None else len(registry.associations)), strict=True))


def public_generation(row: Mapping[str, Any]) -> dict[str, Any]:
    try:
        generation_scope(tuple(row[name] for name in GENERATION_COLUMNS[:4]))
        version = _integer(row["registry_version"], "registry_version", nullable=True)
        count = row["association_count"]
        if type(count) is not int or not 0 <= count <= MAX_ASSOCIATIONS:
            raise IdentityError("invalid_generation_identity")
        digest = _stored_digest(row["registry_digest"], "registry_digest", nullable=True)
        if version is None:
            if digest is not None or count or any(row[name] is not None for name in GENERATION_COLUMNS[6:9]):
                raise IdentityError("invalid_generation_identity")
        else:
            _digest(digest, "registry_digest")
            for name in GENERATION_COLUMNS[6:9]:
                _utc(row[name], name)
            if not row["reviewed_from_utc_usec"] < row["reviewed_through_utc_usec"] <= row["reviewed_at_utc_usec"]:
                raise IdentityError("invalid_generation_identity")
        return {**{name: row[name] for name in GENERATION_COLUMNS if name != "registry_digest"},
            "registry_digest": digest, "registry_schema_version": REGISTRY_SCHEMA_VERSION,
            "status": "reserved_reviewed_version" if version is not None else "reserved_unknown_identity",
            "balance_report_published": False, "complete_identity_coverage_implied": False}
    except (KeyError, TypeError, ValueError, OverflowError) as error:
        if isinstance(error, IdentityError):
            raise
        raise IdentityError("invalid_generation_identity") from None


@dataclass(frozen=True, slots=True)
class OwnershipObservation:
    scope: tuple[int, int]  # environment, season
    session: tuple[int, int, int]  # immutable original session
    replay: tuple[int, int, int]  # observation producer and record sequence
    subject_id: int
    pid: int
    at_monotonic_usec: int
    account_token: int | None
    source: str

    def __post_init__(self) -> None:
        _identity_tuple(self.scope, 2, "scope")
        _identity_tuple(self.session, 3, "session")
        _identity_tuple(self.replay, 3, "replay")
        _integer(self.subject_id, "subject_id")
        if type(self.pid) is not int or not 0 < self.pid < (1 << 31):
            raise IdentityError("invalid_pid")
        _integer(self.at_monotonic_usec, "at_monotonic_usec", zero=True)
        _integer(self.account_token, "account_token", nullable=True)
        if type(self.source) is not str or self.source not in OWNERSHIP_SOURCES or (self.source == "unavailable") != (self.account_token is None):
            raise IdentityError("ownership_source_identity")


@dataclass(frozen=True, slots=True)
class ObservedInterval:
    scope: tuple[int, int]
    session: tuple[int, int, int]
    replay: tuple[int, int, int]
    subject_id: int
    pid: int
    config_id: int
    category: str
    start_monotonic_usec: int
    end_monotonic_usec: int
    start_utc_usec: int | None
    end_utc_usec: int | None
    quality_flags: int = 0

    def __post_init__(self) -> None:
        _identity_tuple(self.scope, 2, "scope")
        _identity_tuple(self.session, 3, "session")
        _identity_tuple(self.replay, 3, "replay")
        for name in ("subject_id", "config_id"):
            _integer(getattr(self, name), name)
        if type(self.pid) is not int or not 0 < self.pid < (1 << 31):
            raise IdentityError("invalid_pid")
        for name in ("start_monotonic_usec", "end_monotonic_usec"):
            _integer(getattr(self, name), name, zero=True)
        if self.end_monotonic_usec < self.start_monotonic_usec or type(self.category) is not str or self.category not in CATEGORIES:
            raise IdentityError("invalid_interval")
        for name in ("start_utc_usec", "end_utc_usec"):
            _utc(getattr(self, name), name, nullable=True)
        _integer(self.quality_flags, "quality_flags", zero=True)
        if self.quality_flags & ~ROLLUP_QUALITY_MASK:
            raise IdentityError("unknown_interval_quality")

    @property
    def comparable_utc(self) -> bool:
        return not self.attribution_quality & UTC_ATTRIBUTION_FLAGS

    @property
    def attribution_quality(self) -> int:
        quality = self.quality_flags
        if self.start_utc_usec is None or self.end_utc_usec is None:
            return quality | ROLLUP_QUALITY_UTC_UNKNOWN
        if self.end_utc_usec - self.start_utc_usec != self.end_monotonic_usec - self.start_monotonic_usec:
            return quality | ROLLUP_QUALITY_UTC_MISMATCH
        return quality


@dataclass(frozen=True, slots=True)
class AttributedSlice:
    interval: ObservedInterval
    registry_version: int
    start_monotonic_usec: int
    end_monotonic_usec: int
    start_utc_usec: int | None
    end_utc_usec: int | None
    account_token: int | None
    controller_token: int | None
    association_id: int | None
    linkage_status: str

    def __post_init__(self) -> None:
        if type(self.interval) is not ObservedInterval:
            raise IdentityError("invalid_effort_interval")
        _integer(self.registry_version, "registry_version")
        for name in ("start_monotonic_usec", "end_monotonic_usec"):
            _integer(getattr(self, name), name, zero=True)
        for name in ("start_utc_usec", "end_utc_usec"):
            _utc(getattr(self, name), name, nullable=True)
        for name in ("account_token", "controller_token", "association_id"):
            _integer(getattr(self, name), name, nullable=True)
        i = self.interval
        if not i.start_monotonic_usec <= self.start_monotonic_usec < self.end_monotonic_usec <= i.end_monotonic_usec:
            raise IdentityError("invalid_effort_slice")
        if i.comparable_utc:
            if self.start_utc_usec != i.start_utc_usec + self.start_monotonic_usec - i.start_monotonic_usec or (
                self.end_utc_usec != i.start_utc_usec + self.end_monotonic_usec - i.start_monotonic_usec):
                raise IdentityError("effort_clock_conflict")
        elif self.start_utc_usec is not None or self.end_utc_usec is not None:
            raise IdentityError("uncovered_effort_clock")
        status = self.linkage_status
        if self.account_token is None:
            valid = status == "unknown_account" and self.controller_token is None and self.association_id is None
        elif not i.comparable_utc:
            valid = status == "clock_unknown" and self.controller_token is None and self.association_id is None
        elif status in ("confirmed", "unknown"):
            valid = self.association_id is not None and (self.controller_token is not None) == (status == "confirmed")
        else:
            valid = status in ("outside_reviewed_window", "no_reviewed_mapping") and self.controller_token is None and self.association_id is None
        if not valid:
            raise IdentityError("invalid_linkage_status")

    @property
    def duration_usec(self) -> int:
        return self.end_monotonic_usec - self.start_monotonic_usec


def attribute_interval(interval: ObservedInterval, ownership: Sequence[OwnershipObservation], registry: Registry) -> tuple[AttributedSlice, ...]:
    """Split at observed ownership and reviewed linkage boundaries, conserving time.

    Monotonic clocks are usable only within the interval's producer incarnation.
    An inherited original session ID does not make a previous process's clock
    comparable. Ownership before the first matching observation stays unknown.
    """
    if type(interval) is not ObservedInterval or type(registry) is not Registry or len(ownership) > MAX_OWNERSHIP_OBSERVATIONS:
        raise IdentityError("invalid_or_oversized_attribution")
    if interval.scope != (registry.environment_id, registry.season_id):
        raise IdentityError("registry_scope_mismatch")
    by_replay: dict[tuple[int, int, int], OwnershipObservation] = {}
    observations = []
    for row in ownership:
        if type(row) is not OwnershipObservation:
            raise IdentityError("invalid_ownership_type")
        if row.scope != interval.scope or row.session != interval.session or (row.subject_id, row.pid) != (interval.subject_id, interval.pid):
            raise IdentityError("ownership_session_mismatch")
        if row.replay in by_replay and by_replay[row.replay] != row:
            raise IdentityError("ownership_replay_conflict")
        by_replay[row.replay] = row
    for row in by_replay.values():
        if row.replay[:2] == interval.replay[:2]:
            observations.append(row)
    observations.sort(key=lambda row: (row.at_monotonic_usec, row.replay[2]))
    accounts_at: dict[int, int | None] = {}
    for row in observations:
        if row.at_monotonic_usec in accounts_at and accounts_at[row.at_monotonic_usec] != row.account_token:
            raise IdentityError("ambiguous_ownership_boundary")
        accounts_at[row.at_monotonic_usec] = row.account_token
    start, end = interval.start_monotonic_usec, interval.end_monotonic_usec
    cuts = {start, end}
    cuts.update(point for point in accounts_at if start < point < end)
    if interval.comparable_utc:
        review_cuts = {registry.reviewed_from_utc_usec, registry.reviewed_through_utc_usec}
        relevant_accounts = set(accounts_at.values()) - {None}
        for row in registry.associations:
            if row.status != "withdrawn" and row.account_token in relevant_accounts:
                review_cuts.add(row.valid_from_utc_usec)
                if row.valid_through_utc_usec is not None:
                    review_cuts.add(row.valid_through_utc_usec)
        cuts.update(start + point - interval.start_utc_usec for point in review_cuts
                    if interval.start_utc_usec < point < interval.end_utc_usec)
    sorted_cuts = sorted(cuts)
    result = []
    observation_index = 0
    account = None
    for first, last in zip(sorted_cuts, sorted_cuts[1:]):
        while observation_index < len(observations) and observations[observation_index].at_monotonic_usec <= first:
            account = observations[observation_index].account_token
            observation_index += 1
        utc_first = interval.start_utc_usec + first - start if interval.comparable_utc else None
        utc_last = interval.start_utc_usec + last - start if interval.comparable_utc else None
        controller, association, status = registry.linkage_at(account, utc_first)
        result.append(AttributedSlice(interval, registry.registry_version, first, last, utc_first, utc_last,
                                      account, controller, association, status))
    if sum(row.duration_usec for row in result) != end - start:
        raise IdentityError("attribution_duration_conflict")
    return tuple(result)


@dataclass(frozen=True, slots=True)
class EffortTotals:
    scope: tuple[int, int]
    registry_version: int
    config_id: int
    category: str
    basis: str
    token: int | None
    character_usec: int
    covered_character_usec: int
    unknown_clock_character_usec: int
    covered_union_usec: int | None
    union_usec: int | None
    distinct_characters: int
    distinct_accounts: int
    quality_flags: int


def _union(windows: Sequence[tuple[int, int]]) -> int:
    total = 0
    last_end: int | None = None
    for first, last in sorted(windows):
        total += last - first if last_end is None or first > last_end else max(0, last - last_end)
        last_end = last if last_end is None else max(last_end, last)
    return total


def _sum(values: Sequence[int]) -> int:
    total = sum(values)
    if total > UINT64_MAX:
        raise IdentityError("effort_overflow")
    return total


def union_effort(slices: Sequence[AttributedSlice], *, category: str = "active") -> tuple[EffortTotals, ...]:
    """Account and confirmed-controller union time in compatible cells.

    Results describe exactly the supplied covered input. The report caller must
    publish input/retention completeness. Presence never becomes input-derived
    activity, and an unknown-controller population has no combined human clock.
    """
    if type(category) is not str or category not in CATEGORIES or len(slices) > MAX_EFFORT_SLICES:
        raise IdentityError("invalid_effort_category_or_capacity")
    seen: dict[tuple[Any, ...], AttributedSlice] = {}
    versions: dict[tuple[int, int], int] = {}
    for row in slices:
        if type(row) is not AttributedSlice or type(row.interval) is not ObservedInterval:
            raise IdentityError("invalid_effort_type")
        interval = row.interval
        if interval.scope in versions and versions[interval.scope] != row.registry_version:
            raise IdentityError("mixed_registry_versions")
        versions[interval.scope] = row.registry_version
        key = (interval.scope, interval.replay, row.start_monotonic_usec, row.end_monotonic_usec)
        if key in seen and seen[key] != row:
            raise IdentityError("effort_replay_conflict")
        seen[key] = row
    # Conflicting/overlapping cuts under one original raw key cannot create
    # extra character effort, including rows built outside attribute_interval.
    by_input: dict[tuple[Any, ...], list[AttributedSlice]] = {}
    for row in seen.values():
        by_input.setdefault((row.interval.scope, row.interval.replay), []).append(row)
    for rows in by_input.values():
        ordered = sorted(rows, key=lambda row: row.start_monotonic_usec)
        for previous, current in zip(ordered, ordered[1:]):
            if previous.interval != current.interval or previous.end_monotonic_usec > current.start_monotonic_usec:
                raise IdentityError("overlapping_or_conflicting_input_slices")
    by_clock: dict[tuple[Any, ...], list[AttributedSlice]] = {}
    for row in seen.values():
        i = row.interval
        # Presence and input-derived activity are independent measurements;
        # active/idle/unknown/linkdead within activity are exclusive.
        family = "presence" if i.category == "presence" else "activity"
        by_clock.setdefault((i.scope, i.session, i.replay[:2], family), []).append(row)
    for rows in by_clock.values():
        ordered = sorted(rows, key=lambda row: row.start_monotonic_usec)
        for previous, current in zip(ordered, ordered[1:]):
            if previous.end_monotonic_usec > current.start_monotonic_usec:
                raise IdentityError("source_interval_overlap")
    cells: dict[tuple[Any, ...], list[AttributedSlice]] = {}
    for row in seen.values():
        i = row.interval
        if i.category != category:
            continue
        identities = (("account", row.account_token), ("controller", row.controller_token))
        for basis, token in identities:
            if token is None:
                basis = "unknown_" + basis
            key = (i.scope, row.registry_version, i.config_id, i.category, basis, token)
            cells.setdefault(key, []).append(row)
    result = []
    for key, rows in sorted(cells.items(), key=lambda item: item[0]):
        covered = [row for row in rows if row.start_utc_usec is not None]
        unknown = [row for row in rows if row.start_utc_usec is None]
        known_identity = key[4] in ("account", "controller")
        covered_union = _union([(row.start_utc_usec, row.end_utc_usec) for row in covered]) if known_identity else None
        character = _sum([row.duration_usec for row in rows])
        quality = 0
        for row in rows:
            quality |= row.interval.attribution_quality
        result.append(EffortTotals(*key, character, _sum([row.duration_usec for row in covered]),
            _sum([row.duration_usec for row in unknown]), covered_union, covered_union if known_identity and not unknown else None,
            len({row.interval.subject_id for row in rows}), len({row.account_token for row in rows if row.account_token is not None}), quality))
    return tuple(result)


def main(argv: Sequence[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Validate, register, or reserve one dated reviewed identity version.")
    parser.add_argument("packet", type=Path)
    parser.add_argument("--previous", type=Path, help="Exact retained preceding version; required after version 1.")
    commands = parser.add_mutually_exclusive_group()
    commands.add_argument("--register", action="store_true", help="Register through restricted TELEMETRY_IDENTITY_REVIEW_DB_* credentials.")
    commands.add_argument("--reserve-generation", type=int, help="Pin this exact registered version using TELEMETRY_IDENTITY_ROLLUP_DB_* credentials.")
    parser.add_argument("--definition-version", type=int, default=3, help="Balance definition for a generation reservation; must be at least 3.")
    args = parser.parse_args(argv)
    try:
        current = load_registry(args.packet)
        if args.register or args.reserve_generation is not None:
            if args.previous is not None:
                raise IdentityError("storage_uses_retained_previous_version")
            try:
                from .db_access import ConnectionSettings, PyMySQLConnectionFactory, PyMySQLRollupDatabase
            except ImportError:
                from db_access import ConnectionSettings, PyMySQLConnectionFactory, PyMySQLRollupDatabase
            prefix = "TELEMETRY_IDENTITY_REVIEW_DB_" if args.register else "TELEMETRY_IDENTITY_ROLLUP_DB_"
            database = PyMySQLRollupDatabase(PyMySQLConnectionFactory(ConnectionSettings.from_env(prefix)))
            try:
                result = database.register_identity_packet(current.input_dict()) if args.register else database.reserve_identity_generation(
                    (args.definition_version, args.reserve_generation, current.environment_id, current.season_id),
                    current.registry_version, expected_digest=current.packet_digest)
                print(json.dumps(dict(result), sort_keys=True))
            finally:
                database.close()
            return 0
        previous = load_registry(args.previous) if args.previous is not None else None
        status = validate_successor(previous, current)
        print(json.dumps({"status": status, "registry_schema_version": REGISTRY_SCHEMA_VERSION,
            "environment_id": current.environment_id, "season_id": current.season_id,
            "registry_version": current.registry_version, "packet_digest": current.packet_digest,
            "associations": len(current.associations)}, sort_keys=True))
        return 0
    except (IdentityError, OSError):
        # Paths or source contents may contain private information. CLI errors
        # publish only a stable classification; detailed evidence stays local.
        print(json.dumps({"status": "refused", "reason": "identity_packet_invalid_or_unavailable"}))
        return 1
    except Exception:
        print(json.dumps({"status": "refused", "reason": "identity_storage_or_authority_failure"}))
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
