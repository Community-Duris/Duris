#!/usr/bin/env python3
"""Classify legacy association/guild resets and emit a protected repair plan."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import stat
import sys
from collections import Counter
from dataclasses import dataclass
from pathlib import Path


MAX_ARTIFACT_BYTES = 16 * 1024 * 1024
MAX_ID = (1 << 32) - 1
MAX_RANK_INDEX = 255
DIGEST = re.compile(r"[0-9a-f]{64}")
DOMAINS = {"association", "guild"}
DEFINITION_STATES = {"current", "obsolete", "unknown"}
EFFECT_NAMES = {
    "membership", "rank", "association_prestige", "locker_access",
    "profile_display", "forum_acl", "administrative_permissions",
}
EFFECT_STATES = {"preserve", "restore", "not_applicable", "conflict"}
DISPOSITIONS = {"leave_unrestored", "player_support"}


class ReconciliationError(Exception):
    """Fail-closed error whose message does not expose protected identities."""


@dataclass(frozen=True)
class Rank:
    rank_ref: str
    rank_index: int
    administrative: bool


@dataclass(frozen=True)
class Definition:
    definition_ref: str
    domain: str
    numeric_id: int
    active: bool
    ranks: dict[str, Rank]


@dataclass(frozen=True)
class Membership:
    definition_ref: str
    rank_ref: str | None


@dataclass(frozen=True)
class EvidenceRow:
    row_ref: str
    player_ref: str
    domain: str
    legacy_numeric_id: int
    numeric_definition_ref: str | None
    legacy_definition_state: str
    legacy_name_ref: str | None
    canonical_name_matches: frozenset[str]
    membership_history_matches: frozenset[str]
    requested_rank_ref: str | None
    effects: dict[str, str]
    current_authority: tuple[Membership, ...]


@dataclass(frozen=True)
class Evidence:
    source_stage_fingerprint: str
    definitions_fingerprint: str
    authority_snapshot_fingerprint: str
    unchanged_target_fingerprint: str
    cross_repo_contract_ref: str
    definitions: dict[str, Definition]
    rows: tuple[EvidenceRow, ...]


@dataclass(frozen=True)
class Disposition:
    row_ref: str
    decision: str
    evidence_ref: str


@dataclass(frozen=True)
class Classification:
    row: EvidenceRow
    category: str
    target_ref: str | None
    reason: str


def _strict_object(pairs: list[tuple[str, object]]) -> dict:
    result: dict = {}
    for key, value in pairs:
        if key in result:
            raise ReconciliationError("protected artifact has a duplicate JSON key")
        result[key] = value
    return result


def _read_protected_json(path: Path, label: str) -> tuple[dict, str]:
    if not path.is_absolute():
        raise ReconciliationError(f"{label} path must be absolute")
    flags = os.O_RDONLY | getattr(os, "O_NOFOLLOW", 0)
    try:
        descriptor = os.open(path, flags)
        metadata = os.fstat(descriptor)
        if not stat.S_ISREG(metadata.st_mode) or metadata.st_uid != os.getuid() or \
                stat.S_IMODE(metadata.st_mode) & 0o077:
            raise ReconciliationError(f"{label} must be an owner-only regular file")
        with os.fdopen(descriptor, "rb") as source:
            payload = source.read(MAX_ARTIFACT_BYTES + 1)
    except OSError as error:
        raise ReconciliationError(f"cannot read {label}: {error}") from error
    if len(payload) > MAX_ARTIFACT_BYTES:
        raise ReconciliationError(f"{label} exceeds the fixed size limit")
    try:
        value = json.loads(payload, object_pairs_hook=_strict_object)
    except (UnicodeError, json.JSONDecodeError) as error:
        raise ReconciliationError(f"{label} is not strict JSON") from error
    if not isinstance(value, dict):
        raise ReconciliationError(f"{label} root must be an object")
    return value, hashlib.sha256(payload).hexdigest()


def _keys(value: dict, expected: set[str], label: str) -> None:
    if set(value) != expected:
        raise ReconciliationError(f"{label} fields do not match the contract")


def _digest(value: object, label: str) -> str:
    if not isinstance(value, str) or DIGEST.fullmatch(value) is None:
        raise ReconciliationError(f"{label} must be a lowercase SHA-256 reference")
    return value


def _optional_digest(value: object, label: str) -> str | None:
    if value is None:
        return None
    return _digest(value, label)


def _identifier(value: object, label: str, *, zero: bool = False) -> int:
    minimum = 0 if zero else 1
    if not isinstance(value, int) or isinstance(value, bool) or \
            not minimum <= value <= MAX_ID:
        raise ReconciliationError(f"{label} is outside its numeric contract")
    return value


def _digest_set(value: object, label: str) -> frozenset[str]:
    if not isinstance(value, list):
        raise ReconciliationError(f"{label} must be a list")
    result = frozenset(_digest(item, label) for item in value)
    if len(result) != len(value):
        raise ReconciliationError(f"{label} contains a duplicate reference")
    return result


def _read_definitions(values: object) -> dict[str, Definition]:
    if not isinstance(values, list) or not values:
        raise ReconciliationError("current_definitions must be a non-empty list")
    result: dict[str, Definition] = {}
    numeric_keys: set[tuple[str, int]] = set()
    for value in values:
        if not isinstance(value, dict):
            raise ReconciliationError("current definition row must be an object")
        _keys(value, {"definition_ref", "domain", "numeric_id", "active", "ranks"},
              "current definition row")
        reference = _digest(value["definition_ref"], "definition_ref")
        domain = value["domain"] if isinstance(value["domain"], str) else ""
        if domain not in DOMAINS or not isinstance(value["active"], bool) or \
                not isinstance(value["ranks"], list):
            raise ReconciliationError("current definition row has invalid fields")
        ranks: dict[str, Rank] = {}
        indices: set[int] = set()
        for raw_rank in value["ranks"]:
            if not isinstance(raw_rank, dict):
                raise ReconciliationError("current rank row must be an object")
            _keys(raw_rank, {"rank_ref", "rank_index", "administrative"},
                  "current rank row")
            rank = Rank(
                _digest(raw_rank["rank_ref"], "rank_ref"),
                _identifier(raw_rank["rank_index"], "rank_index", zero=True),
                raw_rank["administrative"]
                if isinstance(raw_rank["administrative"], bool) else False,
            )
            if not isinstance(raw_rank["administrative"], bool) or \
                    rank.rank_index > MAX_RANK_INDEX or rank.rank_ref in ranks or \
                    rank.rank_index in indices:
                raise ReconciliationError("current rank row is invalid or duplicated")
            ranks[rank.rank_ref] = rank
            indices.add(rank.rank_index)
        if domain == "association" and ranks:
            raise ReconciliationError("association definitions cannot contain guild ranks")
        definition = Definition(
            reference, domain, _identifier(value["numeric_id"], "numeric_id"),
            value["active"], ranks)
        numeric_key = (domain, definition.numeric_id)
        if reference in result or numeric_key in numeric_keys:
            raise ReconciliationError("current definition is duplicated")
        result[reference] = definition
        numeric_keys.add(numeric_key)
    return result


def read_evidence(path: Path) -> tuple[Evidence, str]:
    value, digest = _read_protected_json(path, "reconciliation evidence")
    _keys(value, {
        "version", "source_stage_fingerprint", "definitions_fingerprint",
        "authority_snapshot_fingerprint", "unchanged_target_fingerprint",
        "cross_repo_contract_ref", "current_definitions", "rows",
    }, "reconciliation evidence")
    if value["version"] != 1 or not isinstance(value["rows"], list):
        raise ReconciliationError("reconciliation evidence contract is invalid")
    definitions = _read_definitions(value["current_definitions"])
    rows: list[EvidenceRow] = []
    row_refs: set[str] = set()
    player_domains: set[tuple[str, str]] = set()
    fields = {
        "row_ref", "player_ref", "domain", "legacy_numeric_id",
        "numeric_definition_ref", "legacy_definition_state", "legacy_name_ref",
        "canonical_name_matches", "membership_history_matches",
        "requested_rank_ref", "effects", "current_authority",
    }
    for raw in value["rows"]:
        if not isinstance(raw, dict):
            raise ReconciliationError("reconciliation evidence row must be an object")
        _keys(raw, fields, "reconciliation evidence row")
        domain = raw["domain"] if isinstance(raw["domain"], str) else ""
        state = raw["legacy_definition_state"] \
            if isinstance(raw["legacy_definition_state"], str) else ""
        if domain not in DOMAINS or state not in DEFINITION_STATES:
            raise ReconciliationError("reconciliation row has an invalid enum")
        effects = raw["effects"]
        if not isinstance(effects, dict) or set(effects) != EFFECT_NAMES or \
                any(status not in EFFECT_STATES for status in effects.values()):
            raise ReconciliationError("effect review is incomplete or invalid")
        authority = raw["current_authority"]
        if not isinstance(authority, list):
            raise ReconciliationError("current authority must be a list")
        memberships: list[Membership] = []
        for membership in authority:
            if not isinstance(membership, dict):
                raise ReconciliationError("current authority row must be an object")
            _keys(membership, {"definition_ref", "rank_ref"},
                  "current authority row")
            memberships.append(Membership(
                _digest(membership["definition_ref"], "authority definition_ref"),
                _optional_digest(membership["rank_ref"], "authority rank_ref")))
        row = EvidenceRow(
            _digest(raw["row_ref"], "row_ref"),
            _digest(raw["player_ref"], "player_ref"), domain,
            _identifier(raw["legacy_numeric_id"], "legacy_numeric_id"),
            _optional_digest(raw["numeric_definition_ref"], "numeric_definition_ref"),
            state, _optional_digest(raw["legacy_name_ref"], "legacy_name_ref"),
            _digest_set(raw["canonical_name_matches"], "canonical_name_matches"),
            _digest_set(raw["membership_history_matches"],
                        "membership_history_matches"),
            _optional_digest(raw["requested_rank_ref"], "requested_rank_ref"),
            dict(effects), tuple(memberships),
        )
        player_domain = (row.player_ref, row.domain)
        if row.row_ref in row_refs or player_domain in player_domains:
            raise ReconciliationError("reconciliation evidence contains a duplicate row")
        row_refs.add(row.row_ref)
        player_domains.add(player_domain)
        rows.append(row)
    if not rows:
        raise ReconciliationError("reconciliation evidence contains no rows")
    evidence = Evidence(
        _digest(value["source_stage_fingerprint"], "source_stage_fingerprint"),
        _digest(value["definitions_fingerprint"], "definitions_fingerprint"),
        _digest(value["authority_snapshot_fingerprint"],
                "authority_snapshot_fingerprint"),
        _digest(value["unchanged_target_fingerprint"],
                "unchanged_target_fingerprint"),
        _digest(value["cross_repo_contract_ref"], "cross_repo_contract_ref"),
        definitions, tuple(rows))
    _validate_references(evidence)
    return evidence, digest


def _validate_references(evidence: Evidence) -> None:
    for row in evidence.rows:
        refs = set(row.canonical_name_matches) | set(row.membership_history_matches)
        refs.update(reference for reference in (
            row.numeric_definition_ref, *(membership.definition_ref
                                          for membership in row.current_authority))
                    if reference is not None)
        if any(reference not in evidence.definitions for reference in refs):
            raise ReconciliationError("row references an unknown current definition")
        if any(evidence.definitions[reference].domain != row.domain
               for reference in refs):
            raise ReconciliationError("row references a definition from another domain")
        if row.canonical_name_matches and row.legacy_name_ref is None:
            raise ReconciliationError("canonical matches require protected legacy name evidence")
        if row.domain == "association":
            if row.requested_rank_ref is not None or \
                    any(membership.rank_ref is not None
                        for membership in row.current_authority):
                raise ReconciliationError("association row contains a guild rank")
        else:
            for membership in row.current_authority:
                if membership.rank_ref not in \
                        evidence.definitions[membership.definition_ref].ranks:
                    raise ReconciliationError(
                        "authority rank is not current for its guild")
        if len({membership.definition_ref for membership in row.current_authority}) != \
                len(row.current_authority):
            raise ReconciliationError("current authority contains a duplicate membership")


def read_dispositions(path: Path | None) -> tuple[dict[str, Disposition], str | None]:
    if path is None:
        return {}, None
    value, digest = _read_protected_json(path, "reconciliation dispositions")
    _keys(value, {"version", "dispositions"}, "reconciliation dispositions")
    if value["version"] != 1 or not isinstance(value["dispositions"], list):
        raise ReconciliationError("reconciliation disposition contract is invalid")
    result: dict[str, Disposition] = {}
    for raw in value["dispositions"]:
        if not isinstance(raw, dict):
            raise ReconciliationError("disposition row must be an object")
        _keys(raw, {"row_ref", "decision", "evidence_ref"}, "disposition row")
        decision = raw["decision"] if isinstance(raw["decision"], str) else ""
        disposition = Disposition(
            _digest(raw["row_ref"], "row_ref"), decision,
            _digest(raw["evidence_ref"], "evidence_ref"))
        if decision not in DISPOSITIONS or disposition.row_ref in result:
            raise ReconciliationError("disposition is invalid or duplicated")
        result[disposition.row_ref] = disposition
    return result, digest


def classify(evidence: Evidence) -> list[Classification]:
    result: list[Classification] = []
    for row in evidence.rows:
        name_matches = row.canonical_name_matches
        history_matches = row.membership_history_matches
        semantic_matches = name_matches & history_matches
        current_definitions = {
            membership.definition_ref for membership in row.current_authority
        }
        current_target = next(iter(current_definitions)) \
            if len(current_definitions) == 1 else None
        target = next(iter(semantic_matches)) if len(semantic_matches) == 1 else None
        category = "insufficient_evidence"
        reason = "semantic name and membership history do not prove one target"
        if row.legacy_definition_state == "obsolete":
            category, reason = "obsolete", "legacy definition has no current successor"
            target = None
        elif "conflict" in row.effects.values():
            category, reason = "conflicted", "downstream effect review contains a conflict"
            target = None
        elif name_matches and history_matches and not semantic_matches:
            category, reason = "conflicted", "semantic evidence points to different targets"
            target = None
        elif len(semantic_matches) > 1:
            category, reason = "conflicted", "semantic evidence has multiple current targets"
            target = None
        elif row.legacy_definition_state != "current":
            category, reason = "insufficient_evidence", \
                "legacy definition state is not proven current"
            target = None
        elif target is not None and not evidence.definitions[target].active:
            category, reason = "obsolete", "proven target definition is inactive"
            target = None
        elif len(current_definitions) > 1:
            category, reason = "conflicted", "current target authority is not unique"
            target = None
        elif target is not None and current_definitions and target not in current_definitions:
            category, reason = "conflicted", "current target authority points elsewhere"
            target = None
        elif target is not None and row.effects["administrative_permissions"] == "restore":
            category, reason = "insufficient_evidence", \
                "legacy administrative privilege is never restored"
            target = None
        elif target is not None and row.effects["association_prestige"] == "restore":
            category, reason = "insufficient_evidence", \
                "current association prestige is authoritative"
            target = None
        elif target is not None and not current_definitions and \
                row.effects["membership"] != "restore":
            category, reason = "insufficient_evidence", \
                "missing membership is not reviewed as a restoration"
            target = None
        elif target is not None and current_target == target and \
                row.effects["membership"] != "preserve":
            category, reason = "conflicted", \
                "existing target membership must be preserved"
            target = None
        elif target is not None and row.domain == "association" and \
                row.effects["rank"] != "not_applicable":
            category, reason = "insufficient_evidence", \
                "association row cannot restore a guild rank"
            target = None
        elif target is not None and row.domain == "guild" and \
                not current_definitions:
            definition = evidence.definitions[target]
            rank = definition.ranks.get(row.requested_rank_ref or "")
            if row.effects["rank"] != "restore":
                category, reason, target = "insufficient_evidence", \
                    "missing guild rank is not reviewed as a restoration", None
            elif rank is None:
                category, reason, target = "insufficient_evidence", \
                    "no current semantic guild rank was proven", None
            elif rank.administrative:
                category, reason, target = "insufficient_evidence", \
                    "legacy administrative guild rank is never restored", None
            else:
                category, reason = "uniquely_mappable", "semantic evidence proves one target"
        elif target is not None:
            if row.domain == "guild" and row.effects["rank"] != "preserve":
                category, reason, target = "conflicted", \
                    "existing target rank must be preserved", None
            else:
                category, reason = "uniquely_mappable", \
                    "semantic evidence proves one target"
        # legacy_numeric_id and numeric_definition_ref are evidence only. They
        # deliberately take no part in classification.
        result.append(Classification(row, category, target, reason))
    return sorted(result, key=lambda item: (item.row.domain, item.category,
                                            item.row.row_ref))


def summary(classifications: list[Classification]) -> dict[str, object]:
    domains: dict[str, dict[str, int]] = {}
    for domain in sorted(DOMAINS):
        rows = [item for item in classifications if item.row.domain == domain]
        counts = Counter(item.category for item in rows)
        domains[domain] = {
            "rows": len(rows),
            "uniquely_mappable": counts["uniquely_mappable"],
            "conflicted": counts["conflicted"],
            "obsolete": counts["obsolete"],
            "insufficient_evidence": counts["insufficient_evidence"],
        }
    return {"rows": len(classifications), "domains": domains}


def build_plan(evidence: Evidence, classifications: list[Classification],
               dispositions: dict[str, Disposition]) -> tuple[list[dict], int]:
    known = {item.row.row_ref for item in classifications}
    if set(dispositions) - known:
        raise ReconciliationError("disposition references a row outside the evidence")
    actions: list[dict] = []
    missing = 0
    for item in classifications:
        row = item.row
        if item.category != "uniquely_mappable":
            if row.row_ref not in dispositions:
                missing += 1
            continue
        if row.row_ref in dispositions:
            raise ReconciliationError("uniquely mappable row cannot have a non-restore disposition")
        assert item.target_ref is not None
        definition = evidence.definitions[item.target_ref]
        if any(membership.definition_ref == item.target_ref
               for membership in row.current_authority):
            action = "keep_current_authority"
            rank_index = None
        elif row.domain == "association":
            action = "set_player_association"
            rank_index = None
        else:
            action = "insert_guild_member"
            rank_index = definition.ranks[row.requested_rank_ref or ""].rank_index
        actions.append({
            "row_ref": row.row_ref,
            "player_ref": row.player_ref,
            "domain": row.domain,
            "action": action,
            "target_definition_ref": item.target_ref,
            "target_numeric_id": definition.numeric_id,
            "target_rank_index": rank_index,
            "prestige_action": "unchanged",
            "legacy_numeric_id_used_as_evidence": False,
            "legacy_guild_status_used_as_evidence": False,
        })
    return sorted(actions, key=lambda value: (value["domain"], value["row_ref"])), missing


def write_plan(path: Path, evidence: Evidence, evidence_digest: str,
               disposition_digest: str | None, classifications: list[Classification],
               dispositions: dict[str, Disposition], actions: list[dict]) -> str:
    if not path.is_absolute():
        raise ReconciliationError("plan output path must be absolute")
    by_ref = {item.row.row_ref: item.category for item in classifications}
    payload = {
        "version": 1,
        "evidence_sha256": evidence_digest,
        "dispositions_sha256": disposition_digest,
        "source_stage_fingerprint": evidence.source_stage_fingerprint,
        "definitions_fingerprint": evidence.definitions_fingerprint,
        "authority_snapshot_fingerprint": evidence.authority_snapshot_fingerprint,
        "unchanged_target_fingerprint": evidence.unchanged_target_fingerprint,
        "cross_repo_contract_ref": evidence.cross_repo_contract_ref,
        "classification_summary": summary(classifications),
        "actions": actions,
        "non_restore_dispositions": [
            {"row_ref": value.row_ref, "classification": by_ref[value.row_ref],
             "decision": value.decision, "evidence_ref": value.evidence_ref}
            for value in sorted(dispositions.values(), key=lambda item: item.row_ref)
        ],
    }
    encoded = (json.dumps(payload, sort_keys=True, separators=(",", ":")) + "\n").encode()
    path.parent.mkdir(mode=0o700, parents=True, exist_ok=True)
    flags = os.O_WRONLY | os.O_CREAT | os.O_EXCL | getattr(os, "O_NOFOLLOW", 0)
    try:
        descriptor = os.open(path, flags, 0o600)
        with os.fdopen(descriptor, "wb") as output:
            output.write(encoded)
    except OSError as error:
        raise ReconciliationError(f"cannot create protected plan: {error}") from error
    return hashlib.sha256(encoded).hexdigest()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--evidence", required=True, type=Path)
    parser.add_argument("--dispositions", type=Path)
    parser.add_argument("--plan-output", type=Path)
    parser.add_argument("--require-association-count", type=int, default=176)
    parser.add_argument("--require-guild-count", type=int, default=190)
    arguments = parser.parse_args()
    try:
        evidence, evidence_digest = read_evidence(arguments.evidence)
        dispositions, disposition_digest = read_dispositions(arguments.dispositions)
        classifications = classify(evidence)
        report = summary(classifications)
        domains = report["domains"]
        assert isinstance(domains, dict)
        counts_match = (
            domains["association"]["rows"] == arguments.require_association_count
            and domains["guild"]["rows"] == arguments.require_guild_count)
        actions, missing = build_plan(evidence, classifications, dispositions)
        report["required_dispositions_missing"] = missing
        report["count_contract_met"] = counts_match
        report["status"] = "ready" if counts_match and missing == 0 else "blocked"
        if arguments.plan_output is not None:
            if report["status"] != "ready":
                raise ReconciliationError("repair plan is blocked by counts or dispositions")
            report["plan_sha256"] = write_plan(
                arguments.plan_output, evidence, evidence_digest, disposition_digest,
                classifications, dispositions, actions)
        print(json.dumps(report, sort_keys=True))
        return 0 if report["status"] == "ready" else 2
    except ReconciliationError as error:
        print(f"reconciliation blocked: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
