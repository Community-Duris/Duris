#!/usr/bin/env python3
"""Fail-closed account-erasure and restore-tombstone contracts.

Canonical destructive policy is disabled. The CLI is inspection-only; tests use an
explicit synthetic approval snapshot and never touch configured data.
"""

from __future__ import annotations

import argparse
import hashlib
import hmac
import json
import re
import secrets
import sys
from dataclasses import dataclass, field
from enum import Enum
from pathlib import Path
from typing import Callable

sys.path.insert(0, str(Path(__file__).resolve().parent))
import lifecycle_archive  # noqa: E402
import personal_data_export as personal_export  # noqa: E402
import validate_data_lifecycle as lifecycle_policy  # noqa: E402


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_MANIFEST = ROOT / "migrations" / "data_lifecycle_manifest.json"
RESTORE_SOURCES = {"database_backup", "pfile_backup", "conversion_backup",
                   "journal_replay", "cache_rebuild", "export_spool"}
VALUE_DOMAINS = {"currency", "wallet", "bank", "item", "auction", "locker",
                 "reward", "ownership", "ledger"}
REQUIRED_DIRECT_ACTIONS = {
    "database:accounts", "database:player_data", "file:runtime_accounts",
    "file:runtime_pfiles",
}


class ErasureContractError(Exception):
    pass


class Status(str, Enum):
    PENDING = "pending"
    FENCED = "fenced"
    DRAINED = "drained"
    APPLYING = "applying"
    VERIFYING = "verifying"
    TOMBSTONED = "tombstoned"
    COMPLETE = "complete"
    CANCELLED = "cancelled"
    FAILED = "failed"
    BLOCKED = "blocked"


@dataclass(frozen=True)
class ErasurePolicySnapshot:
    manifest: dict
    entries: dict[str, dict]
    checksum: str


def load_policy(path: Path = DEFAULT_MANIFEST) -> ErasurePolicySnapshot:
    snapshot = lifecycle_archive.load_policy(path)
    return ErasurePolicySnapshot(snapshot.manifest, snapshot.entries, snapshot.checksum)


def validate_ready(snapshot: ErasurePolicySnapshot) -> None:
    approval = snapshot.manifest["controller_approval"]
    if approval["status"] != "approved" or not approval["destructive_rules_enabled"] or \
            approval["reference"].startswith("PENDING"):
        raise ErasureContractError("canonical destructive lifecycle policy is disabled")
    pending = [entry_id for entry_id, entry in snapshot.entries.items()
               if entry["data_subject_key"] != "not_applicable" and
               entry["controller_decision"]["status"] != "approved"]
    if pending:
        raise ErasureContractError("subject stores still have pending erasure decisions")
    retained_direct = sorted(
        entry_id for entry_id in REQUIRED_DIRECT_ACTIONS
        if snapshot.entries[entry_id]["terminal_action"] == "retain"
    )
    if retained_direct:
        raise ErasureContractError("direct identity stores lack an erasure action")


def ordered_actions(snapshot: ErasurePolicySnapshot) -> list[tuple[str, str]]:
    ordered = lifecycle_archive.dependency_order(
        lifecycle_archive.PolicySnapshot(snapshot.manifest, snapshot.entries,
                                         snapshot.checksum),
        sorted(snapshot.entries), finalization=True,
    )
    return [(entry_id, snapshot.entries[entry_id]["terminal_action"])
            for entry_id in ordered]


def subject_token(secret: bytes, scope_hash: str) -> str:
    if len(secret) < 32 or not re.fullmatch(r"[0-9a-f]{64}", scope_hash):
        raise ErasureContractError("invalid tombstone token input")
    return hmac.new(secret, b"duris-erasure-v1\0" + bytes.fromhex(scope_hash),
                    hashlib.sha256).hexdigest()


@dataclass
class StoreEvidence:
    action: str
    affected: int = 0
    remaining_direct_identifiers: int = 0
    reconciled: bool = False
    complete: bool = False


@dataclass
class ErasureRequest:
    request_id: str
    request_key: str
    account_scope_hash: str
    subject_token: str
    manifest_checksum: str
    status: Status
    stores: dict[str, StoreEvidence]
    owner_token_hash: str
    fence_revision: int | None = None
    confirmed: bool = False
    audit: list[dict] = field(default_factory=list)

    def owns(self, token: bytes) -> bool:
        return hmac.compare_digest(self.owner_token_hash,
                                   hashlib.sha256(token).hexdigest())

    def report(self) -> dict:
        return {
            "request_id": self.request_id,
            "manifest_checksum": self.manifest_checksum,
            "status": self.status.value,
            "expected_stores": len(self.stores),
            "completed_stores": sum(item.complete for item in self.stores.values()),
            "fenced": self.fence_revision is not None,
        }


def create_request(snapshot: ErasurePolicySnapshot,
                   gate: personal_export.ReauthenticationGate, token_secret: bytes,
                   account_name: str, password: bytearray, idempotency_key: str,
                   now: int, verifier: Callable[[str, bytes], bool]) -> tuple[ErasureRequest, bytes]:
    validate_ready(snapshot)
    if not re.fullmatch(r"[A-Za-z0-9._-]{8,128}", idempotency_key):
        raise ErasureContractError("invalid idempotency key")
    try:
        scope = gate.reauthenticate(account_name, password, now, verifier)
        gate.claim_request(scope, now)
    except personal_export.ExportContractError as error:
        raise ErasureContractError(str(error)) from error
    request_id, request_key = lifecycle_archive.stable_identity(
        snapshot.manifest["policy_id"], str(snapshot.manifest["schema_version"]),
        snapshot.checksum, scope, idempotency_key, "account-erasure",
    )
    owner = secrets.token_bytes(32)
    stores = {store_id: StoreEvidence(action) for store_id, action
              in ordered_actions(snapshot)}
    request = ErasureRequest(
        request_id, request_key, scope, subject_token(token_secret, scope),
        snapshot.checksum, Status.PENDING, stores, hashlib.sha256(owner).hexdigest(),
    )
    request.audit.append({"event": "created", "status": request.status.value,
                          "store_count": len(stores)})
    return request, owner


class ErasureCoordinator:
    def __init__(self, request: ErasureRequest, owner: bytes):
        if not request.owns(owner):
            raise ErasureContractError("request ownership mismatch")
        self.request = request

    def cancel(self) -> None:
        if self.request.status is not Status.PENDING:
            raise ErasureContractError("cancellation is closed after fencing")
        self.request.status = Status.CANCELLED

    def confirm(self, phrase: str) -> None:
        expected = f"ERASE {self.request.request_id[:8]}"
        if self.request.status is not Status.PENDING or not hmac.compare_digest(
                phrase.encode("utf-8"), expected.encode("utf-8")):
            raise ErasureContractError("explicit erasure confirmation mismatch")
        self.request.confirmed = True

    def fence(self, revision: int, descriptors_closed: bool) -> None:
        if self.request.status is not Status.PENDING or not self.request.confirmed or \
                revision < 1 or not descriptors_closed:
            raise ErasureContractError("identity mutation fence is incomplete")
        self.request.fence_revision = revision
        self.request.status = Status.FENCED

    def drain(self, pending_snapshots: int, pending_commands: int,
              value_domains_reconciled: bool) -> None:
        if self.request.status is not Status.FENCED or pending_snapshots or \
                pending_commands or not value_domains_reconciled:
            raise ErasureContractError("pending work or value reconciliation blocks erasure")
        self.request.status = Status.DRAINED

    def apply(self, store_id: str, affected: int, remaining: int,
              reconciled: bool, via_domain_command: bool = True) -> None:
        if self.request.status not in {Status.DRAINED, Status.APPLYING}:
            raise ErasureContractError("erasure action requires drained fence")
        evidence = self.request.stores.get(store_id)
        if evidence is None or min(affected, remaining) < 0:
            raise ErasureContractError("invalid erasure store evidence")
        locator = store_id.casefold()
        if any(name in locator for name in VALUE_DOMAINS) and not via_domain_command:
            raise ErasureContractError("value domain requires transactional disposition")
        if evidence.action == "retain" and affected:
            raise ErasureContractError("retained store cannot be mutated")
        if evidence.action != "retain" and remaining:
            raise ErasureContractError("direct identifiers remain after action")
        if evidence.complete:
            if (evidence.affected, evidence.remaining_direct_identifiers,
                    evidence.reconciled) == (affected, remaining, reconciled):
                return
            raise ErasureContractError("conflicting erasure retry evidence")
        evidence.affected = affected
        evidence.remaining_direct_identifiers = remaining
        evidence.reconciled = reconciled
        evidence.complete = reconciled and remaining == 0
        if not evidence.complete:
            raise ErasureContractError("store evidence did not reconcile")
        self.request.status = Status.APPLYING

    def verify(self) -> None:
        if self.request.status is not Status.APPLYING or \
                not all(item.complete and item.reconciled and
                        item.remaining_direct_identifiers == 0
                        for item in self.request.stores.values()):
            raise ErasureContractError("all stores must reconcile before tombstone")
        self.request.status = Status.VERIFYING


@dataclass(frozen=True)
class Tombstone:
    request_id: str
    account_scope_hash: str
    subject_token: str
    manifest_checksum: str
    completed_at: int


class TombstoneLedger:
    def __init__(self):
        self._by_scope: dict[str, Tombstone] = {}

    def commit(self, request: ErasureRequest, completed_at: int) -> Tombstone:
        existing = self._by_scope.get(request.account_scope_hash)
        if request.status is Status.TOMBSTONED and existing is not None:
            return existing
        if request.status is not Status.VERIFYING:
            raise ErasureContractError("verified request required for tombstone")
        tombstone = Tombstone(request.request_id, request.account_scope_hash,
                              request.subject_token, request.manifest_checksum,
                              completed_at)
        existing = self._by_scope.setdefault(request.account_scope_hash, tombstone)
        if existing != tombstone:
            raise ErasureContractError("conflicting tombstone identity")
        request.status = Status.TOMBSTONED
        return existing

    def restore_preflight(self, source: str, generation: str,
                          records: list[dict]) -> list[dict]:
        if source not in RESTORE_SOURCES or not re.fullmatch(r"[0-9a-f]{64}", generation):
            raise ErasureContractError("invalid restore preflight identity")
        filtered = []
        for record in records:
            if not isinstance(record, dict) or \
                    not isinstance(record.get("account_scope_hash"), str):
                raise ErasureContractError("restored record lacks stable account scope")
            if record["account_scope_hash"] not in self._by_scope:
                filtered.append(record)
        return filtered

    def finalize(self, request: ErasureRequest, credentials_remaining: int,
                 login_loadable: bool) -> None:
        if request.status is not Status.TOMBSTONED or credentials_remaining or login_loadable:
            raise ErasureContractError("credential and login verification blocks completion")
        request.status = Status.COMPLETE


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--manifest", type=Path, default=DEFAULT_MANIFEST)
    parser.add_argument("command", choices=("inspect",))
    arguments = parser.parse_args()
    try:
        snapshot = load_policy(arguments.manifest)
        actions: dict[str, int] = {}
        for _, action in ordered_actions(snapshot):
            actions[action] = actions.get(action, 0) + 1
        try:
            validate_ready(snapshot)
            state = "ready"
        except ErasureContractError:
            state = "blocked_by_policy"
        print(json.dumps({"policy_id": snapshot.manifest["policy_id"],
                          "manifest_checksum": snapshot.checksum,
                          "stores": len(snapshot.entries), "actions": actions,
                          "request_state": state}, sort_keys=True))
        return 0
    except (ErasureContractError, lifecycle_policy.ValidationError) as error:
        print(f"account erasure blocked: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
