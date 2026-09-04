#!/usr/bin/env python3
"""Classify frozen legacy item evidence and plan only proven recoveries."""

from __future__ import annotations

import argparse
import hashlib
import hmac
import json
import os
import re
import stat
import sys
from collections import Counter
from dataclasses import dataclass
from pathlib import Path


MAX_ARTIFACT_BYTES = 64 * 1024 * 1024
MAX_DEPTH = 32
MAX_UID = (1 << 64) - 1
EXPECTED_QUARANTINE_ROWS = 38_257
DIGEST = re.compile(r"[0-9a-f]{64}")
SOURCE_TABLES = {"player_items", "locker_items"}
PROTOTYPE_STATES = {"current", "missing", "artifact"}
DECISIONS = {"hold", "discard", "recover_new_uid", "recover_descendant"}
PRIMARY_ORDER = (
    "artifact", "conflicted_owner", "unknown_prototype", "uid_collision",
    "insufficient_metadata", "missing_ancestor", "cross_owner", "cycle",
    "depth_exceeded",
)


class QuarantineError(Exception):
    """A fail-closed error safe for aggregate operator output."""


@dataclass(frozen=True)
class EvidenceRow:
    """One protected frozen-stage row and its independent recovery evidence."""

    row_ref: str
    source_table: str
    source_row_id: int
    item_uid: int
    parent_ref: str | None
    owner_ref: str
    owner_proven: bool
    vnum: int
    prototype_state: str
    metadata_fingerprint: str
    metadata_candidates: int
    uid_candidates: int
    live_uid_conflict: bool
    evidence_ref: str


@dataclass(frozen=True)
class Disposition:
    """One explicit operator decision bound to protected evidence."""

    row_ref: str
    decision: str
    evidence_ref: str


@dataclass(frozen=True)
class Classification:
    """One mutually exclusive primary classification with overlapping reasons."""

    row: EvidenceRow
    primary: str
    reasons: frozenset[str]


@dataclass(frozen=True)
class RecoveryRow:
    """One approved row with its planned UID and rewritten ancestry."""

    row: EvidenceRow
    new_uid: int
    new_parent_uid: int | None
    new_root_uid: int


@dataclass(frozen=True)
class Evidence:
    """The complete protected quarantine evidence and UID allocation floors."""

    allocator_next_uid: int
    live_uid_floor: int
    rows: tuple[EvidenceRow, ...]


def _strict_object(pairs: list[tuple[str, object]]) -> dict:
    """Build a JSON object while rejecting duplicate keys."""
    result: dict = {}
    for key, value in pairs:
        if key in result:
            raise QuarantineError("protected artifact has a duplicate JSON key")
        result[key] = value
    return result


def _read_json(path: Path, label: str) -> tuple[dict, str]:
    """Read one bounded owner-only JSON artifact and return its digest."""
    if not path.is_absolute():
        raise QuarantineError(f"{label} path must be absolute")
    flags = os.O_RDONLY | getattr(os, "O_NOFOLLOW", 0)
    try:
        descriptor = os.open(path, flags)
        with os.fdopen(descriptor, "rb") as source:
            metadata = os.fstat(source.fileno())
            if not stat.S_ISREG(metadata.st_mode) or metadata.st_uid != os.getuid() or \
                    stat.S_IMODE(metadata.st_mode) & 0o077:
                raise QuarantineError(f"{label} must be an owner-only regular file")
            payload = source.read(MAX_ARTIFACT_BYTES + 1)
    except OSError as error:
        raise QuarantineError(f"cannot read {label}: {error}") from error
    if len(payload) > MAX_ARTIFACT_BYTES:
        raise QuarantineError(f"{label} exceeds the fixed size limit")
    try:
        value = json.loads(payload, object_pairs_hook=_strict_object)
    except (UnicodeError, json.JSONDecodeError) as error:
        raise QuarantineError(f"{label} is not strict JSON") from error
    if not isinstance(value, dict):
        raise QuarantineError(f"{label} root must be an object")
    return value, hashlib.sha256(payload).hexdigest()


def _keys(value: dict, expected: set[str], label: str) -> None:
    """Require an artifact object to match its schema exactly."""
    if set(value) != expected:
        raise QuarantineError(f"{label} fields do not match the contract")


def _digest(value: object, label: str) -> str:
    """Validate one lowercase SHA-256 opaque reference."""
    if not isinstance(value, str) or DIGEST.fullmatch(value) is None:
        raise QuarantineError(f"{label} must be a lowercase SHA-256 reference")
    return value


def _positive(value: object, label: str, *, zero: bool = False) -> int:
    """Validate one bounded unsigned integer field."""
    minimum = 0 if zero else 1
    if not isinstance(value, int) or isinstance(value, bool) or \
            not minimum <= value <= MAX_UID:
        raise QuarantineError(f"{label} is outside its numeric contract")
    return value


def read_evidence(path: Path) -> tuple[Evidence, str]:
    """Read and validate a protected frozen-stage evidence artifact."""
    value, digest = _read_json(path, "quarantine evidence")
    _keys(value, {"version", "allocator_next_uid", "live_uid_floor", "rows"},
          "quarantine evidence")
    if value["version"] != 1 or not isinstance(value["rows"], list):
        raise QuarantineError("quarantine evidence contract is invalid")
    rows: list[EvidenceRow] = []
    references: set[str] = set()
    evidence_references: set[str] = set()
    source_keys: set[tuple[str, int]] = set()
    fields = {
        "row_ref", "source_table", "source_row_id", "item_uid", "parent_ref",
        "owner_ref", "owner_proven", "vnum", "prototype_state",
        "metadata_fingerprint", "metadata_candidates", "uid_candidates",
        "live_uid_conflict", "evidence_ref",
    }
    for raw in value["rows"]:
        if not isinstance(raw, dict):
            raise QuarantineError("quarantine evidence row must be an object")
        _keys(raw, fields, "quarantine evidence row")
        source_table = raw["source_table"] if isinstance(raw["source_table"], str) else ""
        prototype = raw["prototype_state"] \
            if isinstance(raw["prototype_state"], str) else ""
        parent = None if raw["parent_ref"] is None else \
            _digest(raw["parent_ref"], "parent_ref")
        if source_table not in SOURCE_TABLES or prototype not in PROTOTYPE_STATES or \
                not isinstance(raw["owner_proven"], bool) or \
                not isinstance(raw["live_uid_conflict"], bool):
            raise QuarantineError("quarantine evidence row has an invalid enum or boolean")
        row = EvidenceRow(
            _digest(raw["row_ref"], "row_ref"), source_table,
            _positive(raw["source_row_id"], "source_row_id"),
            _positive(raw["item_uid"], "item_uid"), parent,
            _digest(raw["owner_ref"], "owner_ref"), raw["owner_proven"],
            _positive(raw["vnum"], "vnum", zero=True), prototype,
            _digest(raw["metadata_fingerprint"], "metadata_fingerprint"),
            _positive(raw["metadata_candidates"], "metadata_candidates", zero=True),
            _positive(raw["uid_candidates"], "uid_candidates", zero=True),
            raw["live_uid_conflict"], _digest(raw["evidence_ref"], "evidence_ref"),
        )
        source_key = (row.source_table, row.source_row_id)
        if row.row_ref in references or source_key in source_keys or \
                row.evidence_ref in evidence_references:
            raise QuarantineError(
                "quarantine evidence contains a duplicate row or evidence reference")
        references.add(row.row_ref)
        evidence_references.add(row.evidence_ref)
        source_keys.add(source_key)
        rows.append(row)
    if not rows:
        raise QuarantineError("quarantine evidence contains no rows")
    evidence = Evidence(
        _positive(value["allocator_next_uid"], "allocator_next_uid"),
        _positive(value["live_uid_floor"], "live_uid_floor"), tuple(rows))
    return evidence, digest


def read_dispositions(path: Path | None) -> tuple[dict[str, Disposition], str | None]:
    """Read optional protected operator dispositions and their digest."""
    if path is None:
        return {}, None
    value, digest = _read_json(path, "quarantine dispositions")
    _keys(value, {"version", "dispositions"}, "quarantine dispositions")
    if value["version"] != 1 or not isinstance(value["dispositions"], list):
        raise QuarantineError("quarantine disposition contract is invalid")
    result: dict[str, Disposition] = {}
    for raw in value["dispositions"]:
        if not isinstance(raw, dict):
            raise QuarantineError("quarantine disposition row must be an object")
        _keys(raw, {"row_ref", "decision", "evidence_ref"}, "disposition row")
        decision = raw["decision"] if isinstance(raw["decision"], str) else ""
        disposition = Disposition(
            _digest(raw["row_ref"], "row_ref"), decision,
            _digest(raw["evidence_ref"], "evidence_ref"))
        if decision not in DECISIONS or disposition.row_ref in result:
            raise QuarantineError("quarantine disposition is invalid or duplicated")
        result[disposition.row_ref] = disposition
    return result, digest


def _graph_reasons(rows: dict[str, EvidenceRow], maximum_depth: int) \
        -> dict[str, set[str]]:
    """Classify missing, cross-owner, cyclic, and over-depth ancestry."""
    reasons: dict[str, set[str]] = {reference: set() for reference in rows}
    for reference, row in rows.items():
        if row.parent_ref is not None and row.parent_ref not in rows:
            reasons[reference].add("missing_ancestor")
        elif row.parent_ref is not None and rows[row.parent_ref].owner_ref != row.owner_ref:
            reasons[reference].add("cross_owner")
    for start in rows:
        visited: set[str] = set()
        current = start
        depth = 0
        while current is not None and current in rows:
            if current in visited:
                reasons[start].add("cycle")
                break
            visited.add(current)
            depth += 1
            if depth > maximum_depth:
                reasons[start].add("depth_exceeded")
                break
            current = rows[current].parent_ref
    return reasons


def classify(evidence: Evidence, maximum_depth: int = MAX_DEPTH) \
        -> list[Classification]:
    """Assign one primary rejection reason while retaining overlapping evidence."""
    rows = {row.row_ref: row for row in evidence.rows}
    graph = _graph_reasons(rows, maximum_depth)
    direct: dict[str, set[str]] = {}
    for reference, row in rows.items():
        reasons = set(graph[reference])
        if row.prototype_state == "artifact":
            reasons.add("artifact")
        if not row.owner_proven:
            reasons.add("conflicted_owner")
        if row.prototype_state == "missing":
            reasons.add("unknown_prototype")
        if row.uid_candidates != 1 or row.live_uid_conflict:
            reasons.add("uid_collision")
        if row.metadata_candidates != 1:
            reasons.add("insufficient_metadata")
        direct[reference] = reasons
    result: list[Classification] = []
    for reference, row in rows.items():
        reasons = set(direct[reference])
        if not reasons:
            current = row.parent_ref
            visited: set[str] = set()
            while current is not None and current in rows and current not in visited:
                visited.add(current)
                if direct[current]:
                    reasons.add("dependent_ancestry")
                    break
                current = rows[current].parent_ref
        if not reasons:
            primary = "recoverable"
        elif "dependent_ancestry" in reasons:
            primary = "dependent_ancestry"
        else:
            primary = next(name for name in PRIMARY_ORDER if name in reasons)
        result.append(Classification(row, primary, frozenset(reasons)))
    return sorted(result, key=lambda item: (item.primary, item.row.source_table,
                                            item.row.source_row_id))


def classification_summary(classifications: list[Classification]) -> dict[str, int]:
    """Return aggregate-only mutually exclusive classification counts."""
    counts = Counter(item.primary for item in classifications)
    return {
        "rows": len(classifications),
        "recoverable": counts["recoverable"],
        "artifact": counts["artifact"],
        "conflicted_owner": counts["conflicted_owner"],
        "unknown_prototype": counts["unknown_prototype"],
        "uid_collision": counts["uid_collision"],
        "insufficient_metadata": counts["insufficient_metadata"],
        "missing_ancestor": counts["missing_ancestor"],
        "cross_owner": counts["cross_owner"],
        "cycle": counts["cycle"],
        "depth_exceeded": counts["depth_exceeded"],
        "dependent_children": counts["dependent_ancestry"],
    }


def plan_recovery(evidence: Evidence, classifications: list[Classification],
                  dispositions: dict[str, Disposition]) \
        -> tuple[list[RecoveryRow], int, int]:
    """Plan only complete approved graphs with deterministic replacement UIDs."""
    rows = {row.row_ref: row for row in evidence.rows}
    by_ref = {item.row.row_ref: item for item in classifications}
    expected = {item.row.row_ref: item for item in classify(evidence)}
    if len(by_ref) != len(classifications) or by_ref != expected:
        raise QuarantineError("classifications do not exactly match the evidence")
    if set(dispositions) - set(by_ref):
        raise QuarantineError("disposition references a row outside the evidence")
    for reference, disposition in dispositions.items():
        if not hmac.compare_digest(
                disposition.evidence_ref, rows[reference].evidence_ref):
            raise QuarantineError("disposition is stale or mismatched to the evidence")
    approved: set[str] = set()
    descendant_candidates: set[str] = set()
    undispositioned = 0
    for reference, item in by_ref.items():
        disposition = dispositions.get(reference)
        if item.primary == "recoverable":
            if disposition is None:
                approved.add(reference)
            elif disposition.decision not in {"hold", "discard"}:
                raise QuarantineError("recoverable row has an invalid override disposition")
        elif item.reasons == {"uid_collision"} and \
                (item.row.uid_candidates > 1 or item.row.live_uid_conflict) and \
                disposition is not None and \
                disposition.decision == "recover_new_uid":
            approved.add(reference)
        elif item.reasons == {"dependent_ancestry"} and disposition is not None and \
                disposition.decision == "recover_descendant":
            descendant_candidates.add(reference)
        elif disposition is None:
            undispositioned += 1
        elif disposition.decision not in {"hold", "discard"}:
            raise QuarantineError("unsafe row has an invalid recovery disposition")
    if undispositioned:
        return [], evidence.allocator_next_uid, undispositioned
    while descendant_candidates:
        progressed = False
        for reference in sorted(
                descendant_candidates,
                key=lambda candidate: (
                    rows[candidate].source_table,
                    rows[candidate].source_row_id,
                    candidate,
                )):
            parent = rows[reference].parent_ref
            if parent in approved:
                approved.add(reference)
                descendant_candidates.remove(reference)
                progressed = True
        if not progressed:
            raise QuarantineError(
                "descendant recovery requires every ancestor to be approved")
    changed = True
    while changed:
        changed = False
        for reference in tuple(approved):
            parent = rows[reference].parent_ref
            if parent is not None and parent not in approved:
                approved.remove(reference)
                disposition = dispositions.get(reference)
                if disposition is not None and disposition.decision in {
                        "recover_new_uid", "recover_descendant"}:
                    raise QuarantineError(
                        "recovery disposition requires every ancestor to be approved")
                if disposition is None:
                    undispositioned += 1
                changed = True
    if undispositioned:
        return [], evidence.allocator_next_uid, undispositioned
    maximum_evidence_uid = max(row.item_uid for row in evidence.rows)
    evidence_uid_floor = MAX_UID if maximum_evidence_uid == MAX_UID \
        else maximum_evidence_uid + 1
    next_uid = max(
        evidence.allocator_next_uid, evidence.live_uid_floor, evidence_uid_floor)
    assigned: dict[str, int] = {}
    pending = set(approved)
    while pending:
        progressed = False
        for reference in sorted(
                pending,
                key=lambda candidate: (
                    rows[candidate].source_table,
                    rows[candidate].source_row_id,
                    candidate,
                )):
            row = rows[reference]
            if row.parent_ref is not None and row.parent_ref not in assigned:
                continue
            collision = by_ref[reference].reasons == {"uid_collision"}
            if collision:
                if next_uid >= MAX_UID:
                    raise QuarantineError("recovery UID allocation overflowed")
                assigned[reference] = next_uid
                next_uid += 1
            else:
                assigned[reference] = row.item_uid
            pending.remove(reference)
            progressed = True
        if not progressed:
            raise QuarantineError("approved recovery set is not an acyclic complete graph")
    recovery: list[RecoveryRow] = []
    for reference in approved:
        row = rows[reference]
        parent_uid = assigned.get(row.parent_ref) if row.parent_ref else None
        root_ref = reference
        while rows[root_ref].parent_ref is not None:
            root_ref = rows[root_ref].parent_ref
        recovery.append(RecoveryRow(row, assigned[reference], parent_uid, assigned[root_ref]))
    recovery.sort(key=lambda item: (item.row.source_table, item.row.source_row_id))
    return recovery, next_uid, 0


def _write_private(path: Path, payload: bytes) -> str:
    """Create a new owner-only recovery plan without following symbolic links."""
    if not path.is_absolute():
        raise QuarantineError("recovery plan path must be absolute")
    try:
        parent = path.parent.resolve(strict=True)
        metadata = parent.stat()
    except OSError as error:
        raise QuarantineError(f"cannot inspect recovery plan directory: {error}") from error
    if parent != path.parent or metadata.st_uid != os.getuid() or \
            not stat.S_ISDIR(metadata.st_mode) or stat.S_IMODE(metadata.st_mode) & 0o077:
        raise QuarantineError("recovery plan directory must be owner-only without symlinks")
    flags = os.O_WRONLY | os.O_CREAT | os.O_EXCL | getattr(os, "O_NOFOLLOW", 0)
    try:
        descriptor = os.open(path, flags, 0o600)
        with os.fdopen(descriptor, "wb") as destination:
            destination.write(payload)
    except OSError as error:
        raise QuarantineError(f"cannot create recovery plan: {error}") from error
    return hashlib.sha256(payload).hexdigest()


def write_plan(path: Path, evidence_digest: str, disposition_digest: str | None,
               recovery: list[RecoveryRow], next_uid: int) -> str:
    """Write a deterministic recovery plan bound to both protected artifacts."""
    payload = json.dumps({
        "version": 1,
        "evidence_sha256": evidence_digest,
        "dispositions_sha256": disposition_digest,
        "next_uid": next_uid,
        "rows": [{
            "row_ref": item.row.row_ref,
            "source_table": item.row.source_table,
            "source_row_id": item.row.source_row_id,
            "old_uid": item.row.item_uid,
            "new_uid": item.new_uid,
            "new_parent_uid": item.new_parent_uid,
            "new_root_uid": item.new_root_uid,
            "owner_ref": item.row.owner_ref,
            "vnum": item.row.vnum,
            "metadata_fingerprint": item.row.metadata_fingerprint,
        } for item in recovery],
    }, sort_keys=True, separators=(",", ":")).encode() + b"\n"
    return _write_private(path, payload)


def parse_arguments() -> argparse.Namespace:
    """Parse protected evidence, disposition, and recovery-plan paths."""
    parser = argparse.ArgumentParser(
        description="Classify protected frozen-stage legacy item quarantine evidence.")
    parser.add_argument("--evidence", type=Path, required=True)
    parser.add_argument("--dispositions", type=Path)
    parser.add_argument("--recovery-plan", type=Path)
    return parser.parse_args()


def main() -> int:
    """Classify the complete retained quarantine and optionally write a plan."""
    arguments = parse_arguments()
    try:
        evidence, evidence_digest = read_evidence(arguments.evidence)
        if len(evidence.rows) != EXPECTED_QUARANTINE_ROWS:
            raise QuarantineError(
                f"quarantine evidence must contain exactly {EXPECTED_QUARANTINE_ROWS} rows")
        dispositions, disposition_digest = read_dispositions(arguments.dispositions)
        classifications = classify(evidence)
        recovery, next_uid, undispositioned = plan_recovery(
            evidence, classifications, dispositions)
        values = classification_summary(classifications)
        values["approved_recovery"] = len(recovery)
        values["undispositioned"] = undispositioned
        rendered = " ".join(f"{key}={value}" for key, value in values.items())
        if arguments.recovery_plan:
            if undispositioned:
                raise QuarantineError("recovery plan requires every unsafe row disposition")
            digest = write_plan(
                arguments.recovery_plan, evidence_digest, disposition_digest,
                recovery, next_uid)
            rendered += f" plan_sha256={digest}"
        print("legacy item quarantine: " + rendered)
        return 1 if undispositioned else 0
    except QuarantineError as error:
        print(f"legacy item quarantine blocked: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
