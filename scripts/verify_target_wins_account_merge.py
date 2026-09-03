#!/usr/bin/env python3
"""Verify exceptional target-wins account attachment dispositions."""

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
from datetime import datetime, timezone
from pathlib import Path


MAX_ARTIFACT_BYTES = 1024 * 1024
DIGEST = re.compile(r"[0-9a-f]{64}")
ACTIONS = {"attach", "quarantine", "remap"}
DECISIONS = {"same_owner", "quarantine", "remap"}


class MergeVerificationError(Exception):
    """A fail-closed error that contains no protected identity."""


@dataclass(frozen=True)
class Parent:
    account_ref: str
    password_fingerprint: str
    email_fingerprint: str
    created_fingerprint: str

    @property
    def auth_metadata(self) -> tuple[str, str, str]:
        return (
            self.password_fingerprint, self.email_fingerprint,
            self.created_fingerprint,
        )


@dataclass(frozen=True)
class Child:
    child_ref: str
    account_ref: str
    action: str
    target_ref: str | None


@dataclass(frozen=True)
class Decision:
    account_ref: str
    decision: str
    evidence_ref: str
    target_ref: str | None


@dataclass(frozen=True)
class Plan:
    source: dict[str, Parent]
    target: dict[str, Parent]
    children: tuple[Child, ...]


@dataclass(frozen=True)
class Verification:
    status_by_account: dict[str, str]
    child_count: int
    blocked_children: int

    @property
    def valid(self) -> bool:
        return "unverified" not in self.status_by_account.values() and \
            self.blocked_children == 0

    def summary(self) -> dict[str, int | str]:
        counts = Counter(self.status_by_account.values())
        return {
            "status": "ready" if self.valid else "blocked",
            "source_accounts": len(self.status_by_account),
            "new_parents": counts["new_parent"],
            "identical_parents": counts["identical_parent"],
            "approved_semantic_matches": counts["approved_same_owner"],
            "quarantined_parents": counts["quarantine"],
            "remapped_parents": counts["remap"],
            "unverified_collisions": counts["unverified"],
            "children": self.child_count,
            "blocked_children": self.blocked_children,
        }


def _read_protected_json(path: Path, label: str) -> tuple[dict, str]:
    if not path.is_absolute():
        raise MergeVerificationError(f"{label} path must be absolute")
    flags = os.O_RDONLY | getattr(os, "O_NOFOLLOW", 0)
    try:
        descriptor = os.open(path, flags)
        metadata = os.fstat(descriptor)
        if not stat.S_ISREG(metadata.st_mode) or metadata.st_uid != os.getuid() or \
                stat.S_IMODE(metadata.st_mode) & 0o077:
            raise MergeVerificationError(
                f"{label} must be an owner-only regular file")
        with os.fdopen(descriptor, "rb") as source:
            payload = source.read(MAX_ARTIFACT_BYTES + 1)
    except OSError as error:
        raise MergeVerificationError(f"cannot read {label}: {error}") from error
    if len(payload) > MAX_ARTIFACT_BYTES:
        raise MergeVerificationError(f"{label} exceeds the fixed size limit")
    try:
        value = json.loads(payload, object_pairs_hook=_strict_object)
    except (UnicodeError, json.JSONDecodeError) as error:
        raise MergeVerificationError(f"{label} is not strict JSON") from error
    if not isinstance(value, dict):
        raise MergeVerificationError(f"{label} root must be an object")
    return value, hashlib.sha256(payload).hexdigest()


def _strict_object(pairs: list[tuple[str, object]]) -> dict:
    result: dict = {}
    for key, value in pairs:
        if key in result:
            raise MergeVerificationError("protected artifact has a duplicate JSON key")
        result[key] = value
    return result


def _exact_keys(value: dict, expected: set[str], label: str) -> None:
    if set(value) != expected:
        raise MergeVerificationError(f"{label} fields do not match the contract")


def _digest(value: object, label: str) -> str:
    if not isinstance(value, str) or DIGEST.fullmatch(value) is None:
        raise MergeVerificationError(f"{label} must be a lowercase SHA-256 reference")
    return value


def _optional_digest(value: object, label: str) -> str | None:
    if value is None:
        return None
    return _digest(value, label)


def _parents(values: object, label: str) -> dict[str, Parent]:
    if not isinstance(values, list):
        raise MergeVerificationError(f"{label} must be a list")
    result: dict[str, Parent] = {}
    expected = {
        "account_ref", "password_fingerprint", "email_fingerprint",
        "created_fingerprint",
    }
    for value in values:
        if not isinstance(value, dict):
            raise MergeVerificationError(f"{label} row must be an object")
        _exact_keys(value, expected, f"{label} row")
        parent = Parent(*(_digest(value[field], field) for field in (
            "account_ref", "password_fingerprint", "email_fingerprint",
            "created_fingerprint")))
        if parent.account_ref in result:
            raise MergeVerificationError(f"{label} contains a duplicate account")
        result[parent.account_ref] = parent
    return result


def read_plan(path: Path) -> tuple[Plan, str]:
    value, digest = _read_protected_json(path, "merge plan")
    _exact_keys(value, {"version", "source_accounts", "target_accounts", "children"},
                "merge plan")
    if value["version"] != 1:
        raise MergeVerificationError("merge plan version is unsupported")
    source = _parents(value["source_accounts"], "source_accounts")
    target = _parents(value["target_accounts"], "target_accounts")
    if not source:
        raise MergeVerificationError("merge plan has no source accounts")
    children_value = value["children"]
    if not isinstance(children_value, list):
        raise MergeVerificationError("children must be a list")
    children: list[Child] = []
    child_refs: set[str] = set()
    for value in children_value:
        if not isinstance(value, dict):
            raise MergeVerificationError("child row must be an object")
        _exact_keys(value, {"child_ref", "account_ref", "action", "target_ref"},
                    "child row")
        child = Child(
            _digest(value["child_ref"], "child_ref"),
            _digest(value["account_ref"], "account_ref"),
            value["action"] if isinstance(value["action"], str) else "",
            _optional_digest(value["target_ref"], "target_ref"),
        )
        if child.action not in ACTIONS or child.account_ref not in source:
            raise MergeVerificationError("child row references an invalid action or parent")
        if child.child_ref in child_refs:
            raise MergeVerificationError("merge plan contains a duplicate child")
        child_refs.add(child.child_ref)
        children.append(child)
    return Plan(source, target, tuple(children)), digest


def read_decisions(path: Path | None) -> tuple[dict[str, Decision], str | None]:
    if path is None:
        return {}, None
    value, digest = _read_protected_json(path, "disposition record")
    _exact_keys(value, {"version", "decisions"}, "disposition record")
    if value["version"] != 1 or not isinstance(value["decisions"], list):
        raise MergeVerificationError("disposition record contract is invalid")
    result: dict[str, Decision] = {}
    for value in value["decisions"]:
        if not isinstance(value, dict):
            raise MergeVerificationError("decision row must be an object")
        _exact_keys(value, {"account_ref", "decision", "evidence_ref", "target_ref"},
                    "decision row")
        decision = Decision(
            _digest(value["account_ref"], "account_ref"),
            value["decision"] if isinstance(value["decision"], str) else "",
            _digest(value["evidence_ref"], "evidence_ref"),
            _optional_digest(value["target_ref"], "target_ref"),
        )
        if decision.decision not in DECISIONS or decision.account_ref in result:
            raise MergeVerificationError("disposition record has an invalid decision")
        if decision.decision == "remap" and decision.target_ref is None:
            raise MergeVerificationError("remap decision requires a target reference")
        if decision.decision != "remap" and decision.target_ref is not None:
            raise MergeVerificationError("only remap decisions may name a target")
        result[decision.account_ref] = decision
    return result, digest


def verify(plan: Plan, decisions: dict[str, Decision]) -> Verification:
    unknown_decisions = set(decisions) - set(plan.source)
    if unknown_decisions:
        raise MergeVerificationError("disposition record references an unknown source parent")
    children: dict[str, list[Child]] = {reference: [] for reference in plan.source}
    for child in plan.children:
        children[child.account_ref].append(child)
    status: dict[str, str] = {}
    blocked = 0
    for reference, source in plan.source.items():
        target = plan.target.get(reference)
        decision = decisions.get(reference)
        if target is None:
            expected_status = "new_parent"
            valid_children = all(
                child.action == "attach" and child.target_ref == reference
                for child in children[reference])
        elif source.auth_metadata == target.auth_metadata:
            expected_status = "identical_parent"
            valid_children = all(
                child.action == "attach" and child.target_ref == reference
                for child in children[reference])
        elif decision is None:
            expected_status = "unverified"
            valid_children = False
        elif decision.decision == "same_owner":
            expected_status = "approved_same_owner"
            valid_children = all(
                child.action == "attach" and child.target_ref == reference
                for child in children[reference])
        elif decision.decision == "quarantine":
            expected_status = "quarantine"
            valid_children = all(
                child.action == "quarantine" and child.target_ref is None
                for child in children[reference])
        else:
            expected_status = "remap"
            valid_target = decision.target_ref in plan.target and \
                decision.target_ref != reference
            valid_children = valid_target and all(
                child.action == "remap" and child.target_ref == decision.target_ref
                for child in children[reference])
        status[reference] = expected_status
        if not valid_children:
            blocked += len(children[reference]) or 1
    return Verification(status, len(plan.children), blocked)


def _secure_write(path: Path, payload: bytes) -> None:
    if not path.is_absolute():
        raise MergeVerificationError("receipt path must be absolute")
    try:
        parent = path.parent.resolve(strict=True)
        metadata = parent.stat()
    except OSError as error:
        raise MergeVerificationError(f"cannot inspect receipt directory: {error}") from error
    if parent != path.parent or metadata.st_uid != os.getuid() or \
            not stat.S_ISDIR(metadata.st_mode) or stat.S_IMODE(metadata.st_mode) & 0o077:
        raise MergeVerificationError("receipt directory must be owner-only without symlinks")
    flags = os.O_WRONLY | os.O_CREAT | os.O_EXCL | getattr(os, "O_NOFOLLOW", 0)
    try:
        descriptor = os.open(path, flags, 0o600)
        with os.fdopen(descriptor, "wb") as destination:
            destination.write(payload)
    except OSError as error:
        raise MergeVerificationError(f"cannot create receipt: {error}") from error


def write_receipt(path: Path, plan_digest: str, decision_digest: str | None,
                  verification: Verification) -> str:
    payload = json.dumps({
        "version": 1,
        "verified_at": datetime.now(timezone.utc).isoformat(),
        "plan_sha256": plan_digest,
        "dispositions_sha256": decision_digest,
        "summary": verification.summary(),
    }, sort_keys=True, separators=(",", ":")).encode() + b"\n"
    _secure_write(path, payload)
    return hashlib.sha256(payload).hexdigest()


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Verify an exceptional target-wins account merge plan.")
    parser.add_argument("--plan", type=Path, required=True)
    parser.add_argument("--dispositions", type=Path)
    parser.add_argument("--receipt", type=Path)
    return parser.parse_args()


def main() -> int:
    arguments = parse_arguments()
    try:
        plan, plan_digest = read_plan(arguments.plan)
        decisions, decision_digest = read_decisions(arguments.dispositions)
        verification = verify(plan, decisions)
        summary = verification.summary()
        rendered = " ".join(f"{key}={value}" for key, value in summary.items())
        if not verification.valid:
            print("target-wins account preflight: " + rendered)
            return 2
        if arguments.receipt is None:
            raise MergeVerificationError("a protected receipt is required for a ready plan")
        digest = write_receipt(
            arguments.receipt, plan_digest, decision_digest, verification)
        print("target-wins account preflight: " + rendered + f" receipt_sha256={digest}")
        return 0
    except MergeVerificationError as error:
        print(f"target-wins account preflight blocked: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
