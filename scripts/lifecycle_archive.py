#!/usr/bin/env python3
"""Bounded lifecycle archive planning and state-machine contracts.

The checked-in policy has no approved destructive rule, so this command is dry-run and
metadata-only. The state machine produces a finalize authorization only after exact
verification; a database adapter must still recheck that authorization transactionally.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import sys
import tempfile
from dataclasses import dataclass, field
from enum import Enum
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import validate_data_lifecycle as lifecycle_policy  # noqa: E402


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_MANIFEST = ROOT / "migrations" / "data_lifecycle_manifest.json"
DEFAULT_SCHEMA_FILES = (
    ROOT / "migrations" / "bootstrap_multithread_safe.sql",
    ROOT / "migrations" / "bootstrap_legacy_baseline.sql",
    ROOT / "migrations" / "immutable" / "0001_lookup_dataset_state.sql",
)
MAX_ROW_BUDGET = 256
MAX_BYTE_BUDGET = 1024 * 1024
MAX_TIME_BUDGET_USEC = 500000
MAX_KEY_BYTES = 512
STATE_FIELDS = {
    "state_version", "job_id", "job_key", "policy_id", "policy_schema_version",
    "manifest_checksum", "store_id", "action", "status", "resume_status",
    "cursor", "upper_bound", "cutoff", "row_budget", "byte_budget",
    "time_budget_usec", "dry_run", "reason_codes",
}
REASON_CODES = {
    "unknown_store", "protected_store", "policy_action_mismatch",
    "entry_approval_missing", "global_approval_disabled",
}


class ArchiveContractError(Exception):
    pass


class BatchStatus(str, Enum):
    PLANNED = "planned"
    PAUSED = "paused"
    COPYING = "copying"
    COPIED = "copied"
    VERIFIED = "verified"
    FINALIZING = "finalizing"
    COMPLETED = "completed"
    FAILED = "failed"
    BLOCKED = "blocked"


@dataclass(frozen=True)
class Budget:
    rows: int = 64
    bytes: int = 256 * 1024
    time_usec: int = 25000

    def validate(self) -> None:
        if type(self.rows) is not int or not 1 <= self.rows <= MAX_ROW_BUDGET:
            raise ArchiveContractError("row budget outside fixed bound")
        if type(self.bytes) is not int or not 1 <= self.bytes <= MAX_BYTE_BUDGET:
            raise ArchiveContractError("byte budget outside fixed bound")
        if type(self.time_usec) is not int or \
                not 1 <= self.time_usec <= MAX_TIME_BUDGET_USEC:
            raise ArchiveContractError("time budget outside fixed bound")


@dataclass(frozen=True)
class PolicySnapshot:
    manifest: dict
    entries: dict[str, dict]
    checksum: str


@dataclass(frozen=True)
class LifecyclePlan:
    job_id: str
    job_key: str
    policy_id: str
    policy_schema_version: int
    manifest_checksum: str
    store_id: str
    action: str
    cutoff: str
    cursor: str
    upper_bound: str
    budget: Budget
    dry_run: bool
    status: BatchStatus
    approval_reference: str
    reason_codes: tuple[str, ...] = ()

    def redacted_report(self) -> dict:
        return {
            "job_id": self.job_id,
            "policy_id": self.policy_id,
            "policy_schema_version": self.policy_schema_version,
            "manifest_checksum": self.manifest_checksum,
            "store_id": self.store_id,
            "action": self.action,
            "status": self.status.value,
            "dry_run": self.dry_run,
            "row_budget": self.budget.rows,
            "byte_budget": self.budget.bytes,
            "time_budget_usec": self.budget.time_usec,
            "reason_codes": list(self.reason_codes),
        }


@dataclass(frozen=True)
class ArchiveRow:
    source_key: bytes
    payload: bytes

    def validate(self) -> None:
        if not self.source_key or len(self.source_key) > MAX_KEY_BYTES:
            raise ArchiveContractError("source key outside fixed bound")
        if not self.payload or len(self.payload) > MAX_BYTE_BUDGET:
            raise ArchiveContractError("row payload outside fixed bound")

    @property
    def checksum(self) -> bytes:
        return hashlib.sha256(self.payload).digest()


@dataclass(frozen=True)
class FinalizeAuthorization:
    job_id: str
    batch_id: str
    manifest_checksum: str
    approval_reference: str
    source_count: int
    source_checksum: str
    source_keys_checksum: str
    token: str


@dataclass(frozen=True)
class ExecutionAuthorization:
    job_id: str
    manifest_checksum: str
    approval_reference: str
    target_environment: str
    database_host: str
    lifecycle_role: str
    token: str


def _length_prefixed(digest: "hashlib._Hash", value: bytes) -> None:
    digest.update(len(value).to_bytes(8, "big"))
    digest.update(value)


def fingerprint(rows: list[ArchiveRow]) -> str:
    digest = hashlib.sha256()
    seen: set[bytes] = set()
    for row in sorted(rows, key=lambda candidate: candidate.source_key):
        row.validate()
        if row.source_key in seen:
            raise ArchiveContractError("duplicate source identity")
        seen.add(row.source_key)
        _length_prefixed(digest, row.source_key)
        _length_prefixed(digest, row.checksum)
    return digest.hexdigest()


def stable_identity(*values: str) -> tuple[str, str]:
    digest = hashlib.sha256()
    for value in values:
        _length_prefixed(digest, value.encode("utf-8"))
    raw = digest.digest()
    return raw[:16].hex(), raw.hex()


def load_policy(path: Path = DEFAULT_MANIFEST,
                schema_files: tuple[Path, ...] = DEFAULT_SCHEMA_FILES) -> PolicySnapshot:
    manifest = lifecycle_policy.load_manifest(path)
    tables = lifecycle_policy.schema_tables(schema_files)
    dependencies = lifecycle_policy.schema_dependencies(schema_files)
    entries = lifecycle_policy.validate_manifest(manifest, tables, dependencies)
    canonical = json.dumps(manifest, sort_keys=True, separators=(",", ":")).encode()
    return PolicySnapshot(manifest, entries, hashlib.sha256(canonical).hexdigest())


def dependency_order(snapshot: PolicySnapshot, store_ids: list[str],
                     finalization: bool = False) -> list[str]:
    requested = set(store_ids)
    if len(requested) != len(store_ids) or not requested <= set(snapshot.entries):
        raise ArchiveContractError("dependency order contains duplicate or unknown store")
    ordered: list[str] = []
    visiting: set[str] = set()
    visited: set[str] = set()

    def visit(entry_id: str) -> None:
        if entry_id in visiting:
            raise ArchiveContractError("dependency cycle")
        if entry_id in visited:
            return
        visiting.add(entry_id)
        for dependency in snapshot.entries[entry_id]["dependencies"]:
            if dependency in requested:
                visit(dependency)
        visiting.remove(entry_id)
        visited.add(entry_id)
        ordered.append(entry_id)

    for store_id in store_ids:
        visit(store_id)
    return list(reversed(ordered)) if finalization else ordered


def build_plan(snapshot: PolicySnapshot, store_id: str, action: str, cutoff: str,
               cursor: str, upper_bound: str, budget: Budget, dry_run: bool = True) -> LifecyclePlan:
    budget.validate()
    if not re.fullmatch(r"[a-z_]+:[a-z0-9_*.-]+", store_id):
        raise ArchiveContractError("invalid store id")
    if action not in lifecycle_policy.DESTRUCTIVE_ACTIONS:
        raise ArchiveContractError("action is not a declared lifecycle action")
    if not cutoff or not upper_bound:
        raise ArchiveContractError("cutoff and stable upper bound are required")
    entry = snapshot.entries.get(store_id)
    reasons: list[str] = []
    approval_reference = "PENDING-CONTROLLER-DECISION"
    if entry is None:
        reasons.append("unknown_store")
    else:
        approval_reference = entry["controller_decision"]["reference"]
        if entry["protected_record"]:
            reasons.append("protected_store")
        if entry["terminal_action"] != action:
            reasons.append("policy_action_mismatch")
        decision = entry["controller_decision"]
        if decision["status"] != "approved" or approval_reference.startswith("PENDING"):
            reasons.append("entry_approval_missing")
    global_approval = snapshot.manifest["controller_approval"]
    if global_approval["status"] != "approved" or \
            not global_approval["destructive_rules_enabled"]:
        reasons.append("global_approval_disabled")
    status = BatchStatus.BLOCKED if reasons else BatchStatus.PLANNED
    job_id, job_key = stable_identity(
        snapshot.manifest["policy_id"], str(snapshot.manifest["schema_version"]),
        snapshot.checksum, store_id, action, cutoff, cursor, upper_bound,
    )
    return LifecyclePlan(
        job_id=job_id,
        job_key=job_key,
        policy_id=snapshot.manifest["policy_id"],
        policy_schema_version=snapshot.manifest["schema_version"],
        manifest_checksum=snapshot.checksum,
        store_id=store_id,
        action=action,
        cutoff=cutoff,
        cursor=cursor,
        upper_bound=upper_bound,
        budget=budget,
        dry_run=dry_run,
        status=status,
        approval_reference=approval_reference,
        reason_codes=tuple(sorted(set(reasons))),
    )


def authorize_execution(snapshot: PolicySnapshot, plan: LifecyclePlan,
                        target_environment: str, database_host: str,
                        lifecycle_role: str) -> ExecutionAuthorization:
    if plan.dry_run or plan.status is not BatchStatus.PLANNED:
        raise ArchiveContractError("dry-run or blocked plan cannot be authorized")
    if target_environment.lower() not in {"local", "development", "dev", "test"}:
        raise ArchiveContractError("execution target must be non-production")
    if database_host.lower() not in {"127.0.0.1", "localhost", "::1"}:
        raise ArchiveContractError("execution database must be loopback")
    if lifecycle_role != "lifecycle-admin":
        raise ArchiveContractError("execution requires lifecycle-admin role")
    global_approval = snapshot.manifest["controller_approval"]
    if global_approval["status"] != "approved" or \
            not global_approval["destructive_rules_enabled"] or \
            global_approval["reference"].startswith("PENDING"):
        raise ArchiveContractError("global execution approval is missing")
    entry = snapshot.entries.get(plan.store_id)
    if entry is None or entry["protected_record"] or entry["terminal_action"] != plan.action:
        raise ArchiveContractError("execution rule changed or targets a protected store")
    if snapshot.checksum != plan.manifest_checksum or \
            entry["controller_decision"]["reference"] != plan.approval_reference:
        raise ArchiveContractError("execution policy or approval identity changed")
    _, token = stable_identity(
        plan.job_key, plan.manifest_checksum, plan.approval_reference,
        target_environment.lower(), database_host.lower(), lifecycle_role,
    )
    return ExecutionAuthorization(
        plan.job_id, plan.manifest_checksum, plan.approval_reference,
        target_environment.lower(), database_host.lower(), lifecycle_role, token,
    )


@dataclass
class ArchiveBatchMachine:
    plan: LifecyclePlan
    sequence_number: int
    cursor_start: bytes
    upper_bound: bytes
    authorization: ExecutionAuthorization | None = None
    status: BatchStatus = BatchStatus.PLANNED
    resume_status: BatchStatus | None = None
    archived: dict[bytes, ArchiveRow] = field(default_factory=dict)
    source_count: int = 0
    source_bytes: int = 0
    source_checksum: str = ""
    archive_checksum: str = ""
    reconciliation_before: bool = False
    reconciliation_after: bool = False

    def __post_init__(self) -> None:
        if self.plan.status is BatchStatus.BLOCKED:
            self.status = BatchStatus.BLOCKED
        if not self.plan.dry_run and self.plan.status is BatchStatus.PLANNED:
            expected_token = None
            if self.authorization is not None:
                _, expected_token = stable_identity(
                    self.plan.job_key, self.plan.manifest_checksum,
                    self.plan.approval_reference, self.authorization.target_environment,
                    self.authorization.database_host, self.authorization.lifecycle_role,
                )
            if self.authorization is None or self.authorization.job_id != self.plan.job_id or \
                    self.authorization.manifest_checksum != self.plan.manifest_checksum or \
                    self.authorization.approval_reference != self.plan.approval_reference or \
                    self.authorization.target_environment not in {
                        "local", "development", "dev", "test",
                    } or self.authorization.database_host not in {
                        "127.0.0.1", "localhost", "::1",
                    } or self.authorization.lifecycle_role != "lifecycle-admin" or \
                    self.authorization.token != expected_token:
                raise ArchiveContractError("mutation-capable batch lacks exact authorization")
        if self.sequence_number < 0 or not self.upper_bound:
            raise ArchiveContractError("invalid batch identity or upper bound")

    @property
    def batch_id(self) -> str:
        return stable_identity(self.plan.job_key, str(self.sequence_number),
                               self.cursor_start.hex(), self.upper_bound.hex())[0]

    def pause(self) -> None:
        if self.status in {BatchStatus.COMPLETED, BatchStatus.BLOCKED, BatchStatus.FAILED}:
            raise ArchiveContractError("terminal or blocked batch cannot be paused")
        self.resume_status = self.status
        self.status = BatchStatus.PAUSED

    def resume(self) -> None:
        if self.status is not BatchStatus.PAUSED or self.resume_status is None:
            raise ArchiveContractError("batch is not paused")
        self.status = self.resume_status
        self.resume_status = None

    def copy(self, rows: list[ArchiveRow], run_usec: int) -> None:
        if self.status not in {BatchStatus.PLANNED, BatchStatus.COPYING, BatchStatus.COPIED}:
            raise ArchiveContractError("copy is invalid in current state")
        if self.plan.dry_run:
            raise ArchiveContractError("dry-run plan cannot copy archive rows")
        if run_usec < 0 or run_usec > self.plan.budget.time_usec:
            raise ArchiveContractError("copy exceeded wall-time budget")
        if len(rows) > self.plan.budget.rows:
            raise ArchiveContractError("copy exceeded row budget")
        self.status = BatchStatus.COPYING
        ordered = sorted(rows, key=lambda row: row.source_key)
        if ordered and (ordered[0].source_key <= self.cursor_start or
                        ordered[-1].source_key > self.upper_bound):
            raise ArchiveContractError("source identity outside stable cursor window")
        if sum(len(row.payload) for row in ordered) > self.plan.budget.bytes:
            raise ArchiveContractError("copy exceeded byte budget")
        fingerprint(ordered)
        incoming = {row.source_key: row for row in ordered}
        if self.archived and set(self.archived) != set(incoming):
            raise ArchiveContractError("retry source set differs from stable batch")
        for row in ordered:
            existing = self.archived.get(row.source_key)
            if existing and existing.payload != row.payload:
                raise ArchiveContractError("retry conflicts with archived source identity")
        for row in ordered:
            self.archived[row.source_key] = row
        self.source_count = len(ordered)
        self.source_bytes = sum(len(row.payload) for row in ordered)
        self.source_checksum = fingerprint(ordered)
        self.archive_checksum = fingerprint(list(self.archived.values()))
        if len(self.archived) != self.source_count:
            raise ArchiveContractError("retry source set differs from stable batch")
        self.status = BatchStatus.COPIED

    def verify(self, source_rows: list[ArchiveRow], reconciliation_before: bool) -> None:
        if self.status is not BatchStatus.COPIED:
            raise ArchiveContractError("verification requires copied state")
        source_checksum = fingerprint(source_rows)
        archive_checksum = fingerprint(list(self.archived.values()))
        if len(source_rows) != len(self.archived) or source_checksum != archive_checksum or \
                source_checksum != self.source_checksum:
            self.status = BatchStatus.FAILED
            raise ArchiveContractError("archive count or checksum mismatch")
        if not reconciliation_before:
            self.status = BatchStatus.FAILED
            raise ArchiveContractError("pre-finalization reconciliation failed")
        self.reconciliation_before = True
        self.archive_checksum = archive_checksum
        self.status = BatchStatus.VERIFIED

    def finalize(self, manifest_checksum: str,
                 approval_reference: str) -> FinalizeAuthorization:
        if self.status not in {BatchStatus.VERIFIED, BatchStatus.FINALIZING}:
            raise ArchiveContractError("finalization requires verified state")
        if self.plan.dry_run or self.plan.status is BatchStatus.BLOCKED:
            raise ArchiveContractError("dry-run or blocked plan cannot finalize")
        if manifest_checksum != self.plan.manifest_checksum or \
                approval_reference != self.plan.approval_reference:
            raise ArchiveContractError("policy or approval identity changed")
        if not self.reconciliation_before:
            raise ArchiveContractError("pre-finalization reconciliation is incomplete")
        self.status = BatchStatus.FINALIZING
        keys = sorted(self.archived)
        key_digest = hashlib.sha256()
        for key in keys:
            _length_prefixed(key_digest, key)
        source_keys_checksum = key_digest.hexdigest()
        _, token = stable_identity(
            self.plan.job_id, self.batch_id, self.plan.manifest_checksum,
            self.plan.approval_reference, str(self.source_count), self.source_checksum,
            source_keys_checksum,
        )
        return FinalizeAuthorization(
            job_id=self.plan.job_id,
            batch_id=self.batch_id,
            manifest_checksum=self.plan.manifest_checksum,
            approval_reference=self.plan.approval_reference,
            source_count=self.source_count,
            source_checksum=self.source_checksum,
            source_keys_checksum=source_keys_checksum,
            token=token,
        )

    def complete_finalize(self, authorization: FinalizeAuthorization,
                          affected_count: int, remaining_count: int,
                          reconciliation_after: bool) -> None:
        expected = self.finalize(
            self.plan.manifest_checksum, self.plan.approval_reference
        )
        if authorization != expected:
            raise ArchiveContractError("finalization acknowledgment identity mismatch")
        if affected_count != self.source_count or remaining_count != 0:
            raise ArchiveContractError("finalization count verification failed")
        if not reconciliation_after:
            raise ArchiveContractError("post-finalization reconciliation failed")
        self.reconciliation_after = True
        self.status = BatchStatus.COMPLETED

    def restore(self) -> list[ArchiveRow]:
        rows = sorted(self.archived.values(), key=lambda row: row.source_key)
        if fingerprint(rows) != self.archive_checksum:
            raise ArchiveContractError("archive corruption prevents restore")
        return rows

    def redacted_report(self) -> dict:
        return {
            "job_id": self.plan.job_id,
            "batch_id": self.batch_id,
            "sequence_number": self.sequence_number,
            "status": self.status.value,
            "source_count": self.source_count,
            "source_bytes": self.source_bytes,
            "reconciliation_before": self.reconciliation_before,
            "reconciliation_after": self.reconciliation_after,
        }


def state_from_plan(plan: LifecyclePlan) -> dict:
    return {
        "state_version": 1,
        "job_id": plan.job_id,
        "job_key": plan.job_key,
        "policy_id": plan.policy_id,
        "policy_schema_version": plan.policy_schema_version,
        "manifest_checksum": plan.manifest_checksum,
        "store_id": plan.store_id,
        "action": plan.action,
        "status": plan.status.value,
        "resume_status": None,
        "cursor": plan.cursor,
        "upper_bound": plan.upper_bound,
        "cutoff": plan.cutoff,
        "row_budget": plan.budget.rows,
        "byte_budget": plan.budget.bytes,
        "time_budget_usec": plan.budget.time_usec,
        "dry_run": plan.dry_run,
        "reason_codes": list(plan.reason_codes),
    }


def read_state(path: Path) -> dict:
    raw = lifecycle_policy.read_regular_text(path, 65536, "lifecycle state")
    def unique_object(pairs: list[tuple[str, object]]) -> dict:
        value = {}
        for key, item in pairs:
            if key in value:
                raise ArchiveContractError(f"state contains duplicate field: {key}")
            value[key] = item
        return value

    try:
        state = json.loads(raw, object_pairs_hook=unique_object)
    except json.JSONDecodeError as error:
        raise ArchiveContractError(f"state parse failed: {error}") from error
    if not isinstance(state, dict) or set(state) != STATE_FIELDS:
        raise ArchiveContractError("state fields differ")
    string_fields = {
        "job_id", "job_key", "policy_id", "manifest_checksum", "store_id",
        "action", "status", "cursor", "upper_bound", "cutoff",
    }
    if type(state["state_version"]) is not int or state["state_version"] != 1 or \
            type(state["policy_schema_version"]) is not int or \
            state["policy_schema_version"] != 1 or \
            not all(isinstance(state[field], str) and state[field]
                    for field in string_fields) or \
            not re.fullmatch(r"[0-9a-f]{32}", state["job_id"]) or \
            not re.fullmatch(r"[0-9a-f]{64}", state["job_key"]) or \
            not re.fullmatch(r"[0-9a-f]{64}", state["manifest_checksum"]):
        raise ArchiveContractError("state identity fields are invalid")
    if state["status"] not in {status.value for status in BatchStatus} or \
            (state["resume_status"] is not None and
             state["resume_status"] not in {status.value for status in BatchStatus}) or \
            not isinstance(state["dry_run"], bool) or \
            not isinstance(state["reason_codes"], list) or \
            not all(isinstance(reason, str) for reason in state["reason_codes"]) or \
            not set(state["reason_codes"]) <= REASON_CODES or not state["dry_run"]:
        raise ArchiveContractError("state status fields are invalid")
    Budget(state["row_budget"], state["byte_budget"],
           state["time_budget_usec"]).validate()
    expected_job_id, expected_job_key = stable_identity(
        state["policy_id"], str(state["policy_schema_version"]),
        state["manifest_checksum"], state["store_id"], state["action"],
        state["cutoff"], state["cursor"], state["upper_bound"],
    )
    if state["job_id"] != expected_job_id or state["job_key"] != expected_job_key:
        raise ArchiveContractError("state stable identity mismatch")
    return state


def write_state(path: Path, state: dict) -> None:
    if path.is_symlink():
        raise ArchiveContractError("state path must not be a symlink")
    path.parent.mkdir(mode=0o700, parents=True, exist_ok=True)
    descriptor, temporary_name = tempfile.mkstemp(prefix=path.name + ".", dir=path.parent)
    temporary = Path(temporary_name)
    try:
        os.fchmod(descriptor, 0o600)
        payload = (json.dumps(state, sort_keys=True) + "\n").encode()
        with os.fdopen(descriptor, "wb", closefd=True) as output:
            output.write(payload)
            output.flush()
            os.fsync(output.fileno())
        os.replace(temporary, path)
    except Exception as error:
        try:
            os.close(descriptor)
        except OSError:
            pass
        temporary.unlink(missing_ok=True)
        if isinstance(error, ArchiveContractError):
            raise
        raise ArchiveContractError(f"state write failed: {error}") from error


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--manifest", type=Path, default=DEFAULT_MANIFEST)
    subcommands = parser.add_subparsers(dest="command", required=True)

    inspect = subcommands.add_parser("inspect")
    inspect.add_argument("--json", action="store_true")

    plan = subcommands.add_parser("plan")
    plan.add_argument("--store", required=True)
    plan.add_argument("--action", required=True)
    plan.add_argument("--cutoff", required=True)
    plan.add_argument("--cursor", default="0")
    plan.add_argument("--upper-bound", required=True)
    plan.add_argument("--row-budget", type=int, default=64)
    plan.add_argument("--byte-budget", type=int, default=256 * 1024)
    plan.add_argument("--time-budget-usec", type=int, default=25000)
    plan.add_argument("--state-file", type=Path)

    for command in ("pause", "resume", "report"):
        control = subcommands.add_parser(command)
        control.add_argument("--state-file", type=Path, required=True)
    return parser.parse_args()


def main() -> int:
    arguments = parse_args()
    try:
        snapshot = load_policy(arguments.manifest)
        if arguments.command == "inspect":
            report = {
                "policy_id": snapshot.manifest["policy_id"],
                "policy_schema_version": snapshot.manifest["schema_version"],
                "manifest_checksum": snapshot.checksum,
                "stores": len(snapshot.entries),
                "approved_destructive_rules": sum(
                    entry["terminal_action"] in lifecycle_policy.DESTRUCTIVE_ACTIONS
                    for entry in snapshot.entries.values()
                ),
                "destructive_rules_enabled": snapshot.manifest["controller_approval"]
                ["destructive_rules_enabled"],
                "scheduler_state": "blocked_by_policy",
            }
        elif arguments.command == "plan":
            plan = build_plan(
                snapshot, arguments.store, arguments.action, arguments.cutoff,
                arguments.cursor, arguments.upper_bound,
                Budget(arguments.row_budget, arguments.byte_budget,
                       arguments.time_budget_usec),
            )
            report = plan.redacted_report()
            if arguments.state_file:
                write_state(arguments.state_file, state_from_plan(plan))
        else:
            state = read_state(arguments.state_file)
            if state["manifest_checksum"] != snapshot.checksum:
                raise ArchiveContractError("state policy checksum is stale")
            if arguments.command == "pause":
                if state["status"] not in {"planned", "copying", "copied", "verified"}:
                    raise ArchiveContractError("state cannot be paused")
                state["resume_status"] = state["status"]
                state["status"] = "paused"
                write_state(arguments.state_file, state)
            elif arguments.command == "resume":
                if state["status"] != "paused" or not state["resume_status"]:
                    raise ArchiveContractError("state is not paused")
                state["status"] = state["resume_status"]
                state["resume_status"] = None
                write_state(arguments.state_file, state)
            report = {key: state[key] for key in (
                "job_id", "policy_id", "policy_schema_version", "manifest_checksum",
                "store_id", "action", "status", "row_budget", "byte_budget",
                "time_budget_usec", "dry_run", "reason_codes",
            )}
        print(json.dumps(report, sort_keys=True))
        return 0
    except (ArchiveContractError, lifecycle_policy.ValidationError) as error:
        print(f"lifecycle archive blocked: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
