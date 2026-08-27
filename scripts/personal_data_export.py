#!/usr/bin/env python3
"""Authenticated, manifest-driven personal-data export packaging contracts.

The canonical shared-disclosure policy is pending, so the command-line surface is
inspection-only. Tests exercise approved synthetic policy and records without reading
real accounts, credentials, or configured databases.
"""

from __future__ import annotations

import argparse
import hashlib
import hmac
import json
import os
import re
import secrets
import stat
import sys
import tempfile
from dataclasses import dataclass, field
from enum import Enum
from pathlib import Path
from typing import Callable

sys.path.insert(0, str(Path(__file__).resolve().parent))
import lifecycle_archive  # noqa: E402
import validate_data_lifecycle as lifecycle_policy  # noqa: E402


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_MANIFEST = ROOT / "migrations" / "data_lifecycle_manifest.json"
MAX_SECTION_ROWS = 256
MAX_SECTION_BYTES = 1024 * 1024
MAX_BUNDLE_ROWS = 10000
MAX_BUNDLE_BYTES = 16 * 1024 * 1024
MAX_AUTH_FAILURES = 5
AUTH_WINDOW_SECONDS = 900
REQUEST_COOLDOWN_SECONDS = 3600
FORBIDDEN_FIELDS = {
    "password", "password_hash", "acct_password", "confirmation_code",
    "delivery_token", "delivery_token_hash", "private_key", "secret",
    "command_payload", "result_payload", "keys_hash", "command_hash",
    "raw_security_events",
}


class ExportContractError(Exception):
    pass


class RequestStatus(str, Enum):
    PENDING = "pending"
    COLLECTING = "collecting"
    PACKAGED = "packaged"
    RELEASED = "released"
    CANCELLED = "cancelled"
    FAILED = "failed"
    EXPIRED = "expired"
    BLOCKED = "blocked"


def canonical_json(value: object) -> bytes:
    return json.dumps(value, sort_keys=True, separators=(",", ":"),
                      ensure_ascii=False).encode("utf-8")


def digest_hex(value: bytes) -> str:
    return hashlib.sha256(value).hexdigest()


def normalize_account(account_name: str) -> str:
    if not isinstance(account_name, str) or \
            not re.fullmatch(r"[A-Za-z][A-Za-z0-9_-]{1,49}", account_name):
        raise ExportContractError("invalid account identity")
    return account_name.casefold()


@dataclass(frozen=True)
class ExportPolicySnapshot:
    manifest: dict
    entries: dict[str, dict]
    checksum: str


def load_policy(path: Path = DEFAULT_MANIFEST) -> ExportPolicySnapshot:
    snapshot = lifecycle_archive.load_policy(path)
    return ExportPolicySnapshot(snapshot.manifest, snapshot.entries, snapshot.checksum)


def validate_export_ready(snapshot: ExportPolicySnapshot) -> None:
    policy = snapshot.manifest["export_policy"]
    if policy["status"] != "approved" or not policy["shared_disclosure_enabled"] or \
            policy["reference"].startswith("PENDING"):
        raise ExportContractError("shared-record export policy is not approved")
    pending = sorted(
        entry_id for entry_id, entry in snapshot.entries.items()
        if entry["export_rule"]["disposition"] == "pending"
    )
    if pending:
        raise ExportContractError("lifecycle stores still have pending export rules")


@dataclass
class AuthRateState:
    window_started: int
    failures: int = 0
    last_request_at: int = 0


class ReauthenticationGate:
    def __init__(self, server_secret: bytes):
        if len(server_secret) < 32:
            raise ExportContractError("reauthentication secret is too short")
        self._secret = bytes(server_secret)
        self._rates: dict[str, AuthRateState] = {}

    def scope_hash(self, account_name: str) -> str:
        normalized = normalize_account(account_name)
        return hmac.new(self._secret, normalized.encode(), hashlib.sha256).hexdigest()

    def reauthenticate(
        self,
        account_name: str,
        password: bytearray,
        now: int,
        verifier: Callable[[str, bytes], bool],
    ) -> str:
        scope = self.scope_hash(account_name)
        state = self._rates.setdefault(scope, AuthRateState(now))
        if now < state.window_started or now - state.window_started >= AUTH_WINDOW_SECONDS:
            state.window_started = now
            state.failures = 0
        if state.failures >= MAX_AUTH_FAILURES:
            raise ExportContractError("reauthentication rate limit reached")
        try:
            verified = verifier(account_name, bytes(password))
        finally:
            for index in range(len(password)):
                password[index] = 0
        if not verified:
            state.failures += 1
            raise ExportContractError("reauthentication failed")
        state.failures = 0
        return scope

    def claim_request(self, scope_hash: str, now: int) -> None:
        state = self._rates.setdefault(scope_hash, AuthRateState(now))
        if state.last_request_at and now - state.last_request_at < REQUEST_COOLDOWN_SECONDS:
            raise ExportContractError("export request cooldown is active")
        state.last_request_at = now


@dataclass(frozen=True)
class ExportAuditEvent:
    event: str
    status: str
    section_count: int = 0
    record_count: int = 0
    byte_count: int = 0
    error_code: int = 0


@dataclass
class ExportRequest:
    request_id: str
    request_key: str
    account_scope_hash: str
    policy_id: str
    policy_schema_version: int
    manifest_checksum: str
    snapshot_id: str
    created_at: int
    expires_at: int
    owner_token_hash: str
    status: RequestStatus = RequestStatus.PENDING
    sections: dict[str, list[dict]] = field(default_factory=dict)
    section_bytes: dict[str, int] = field(default_factory=dict)
    completed_sections: set[str] = field(default_factory=set)
    audit: list[ExportAuditEvent] = field(default_factory=list)
    package_checksum: str = ""
    package_path: Path | None = None

    def owns(self, owner_token: bytes) -> bool:
        return hmac.compare_digest(self.owner_token_hash, digest_hex(owner_token))

    def redacted_status(self) -> dict:
        return {
            "request_id": self.request_id,
            "policy_id": self.policy_id,
            "policy_schema_version": self.policy_schema_version,
            "manifest_checksum": self.manifest_checksum,
            "status": self.status.value,
            "expected_sections": len(self.sections),
            "completed_sections": len(self.completed_sections),
            "record_count": sum(len(rows) for rows in self.sections.values()),
            "package_checksum": self.package_checksum or None,
            "expires_at": self.expires_at,
        }


def create_request(snapshot: ExportPolicySnapshot, gate: ReauthenticationGate,
                   account_name: str, password: bytearray, idempotency_key: str,
                   now: int, verifier: Callable[[str, bytes], bool]) -> tuple[ExportRequest, bytes]:
    validate_export_ready(snapshot)
    if not re.fullmatch(r"[A-Za-z0-9._-]{8,128}", idempotency_key):
        raise ExportContractError("invalid idempotency key")
    scope = gate.reauthenticate(account_name, password, now, verifier)
    gate.claim_request(scope, now)
    request_id, request_key = lifecycle_archive.stable_identity(
        snapshot.manifest["policy_id"], str(snapshot.manifest["schema_version"]),
        snapshot.checksum, scope, idempotency_key,
    )
    owner_token = secrets.token_bytes(32)
    ttl = snapshot.manifest["export_policy"]["bundle_ttl_seconds"]
    sections = {entry_id: [] for entry_id in sorted(snapshot.entries)}
    request = ExportRequest(
        request_id=request_id,
        request_key=request_key,
        account_scope_hash=scope,
        policy_id=snapshot.manifest["policy_id"],
        policy_schema_version=snapshot.manifest["schema_version"],
        manifest_checksum=snapshot.checksum,
        snapshot_id=secrets.token_hex(16),
        created_at=now,
        expires_at=now + ttl,
        owner_token_hash=digest_hex(owner_token),
        sections=sections,
        section_bytes={entry_id: 0 for entry_id in sections},
    )
    request.audit.append(ExportAuditEvent("created", request.status.value))
    return request, owner_token


def _contains_forbidden_field(value: object) -> bool:
    if isinstance(value, dict):
        for key, nested in value.items():
            if not isinstance(key, str):
                raise ExportContractError("export object keys must be strings")
            if key.casefold() in FORBIDDEN_FIELDS or _contains_forbidden_field(nested):
                return True
    elif isinstance(value, list):
        return any(_contains_forbidden_field(item) for item in value)
    return False


def filter_record(record: dict, export_rule: dict) -> dict:
    if not isinstance(record, dict):
        raise ExportContractError("export record must be an object")
    excluded = set(export_rule["excluded_fields"]) | FORBIDDEN_FIELDS
    disposition = export_rule["disposition"]
    if disposition == "shared_redacted":
        allowed = set(export_rule["shared_fields"])
        filtered = {key: value for key, value in record.items()
                    if key in allowed and key not in excluded}
    elif disposition == "include":
        filtered = {key: value for key, value in record.items() if key not in excluded}
    else:
        raise ExportContractError("records supplied for non-exportable store")
    if _contains_forbidden_field(filtered):
        raise ExportContractError("secret field survived export filtering")
    return filtered


def add_section_batch(snapshot: ExportPolicySnapshot, request: ExportRequest,
                      owner_token: bytes, store_id: str, records: list[dict],
                      snapshot_id: str, final: bool) -> None:
    if not request.owns(owner_token):
        raise ExportContractError("request ownership mismatch")
    if request.status in {
        RequestStatus.CANCELLED, RequestStatus.FAILED, RequestStatus.EXPIRED,
        RequestStatus.RELEASED,
    }:
        raise ExportContractError("request is terminal")
    if snapshot.checksum != request.manifest_checksum or \
            snapshot_id != request.snapshot_id:
        request.status = RequestStatus.FAILED
        raise ExportContractError("policy or consistent snapshot identity changed")
    if store_id not in request.sections:
        request.status = RequestStatus.FAILED
        raise ExportContractError("unknown lifecycle store")
    rule = snapshot.entries[store_id]["export_rule"]
    disposition = rule["disposition"]
    if disposition in {"exclude", "pending"}:
        if records:
            request.status = RequestStatus.FAILED
            raise ExportContractError("excluded or pending store supplied records")
        request.completed_sections.add(store_id)
        request.status = RequestStatus.COLLECTING
        return
    if store_id in request.completed_sections:
        raise ExportContractError("completed section cannot accept another batch")
    if len(records) > MAX_SECTION_ROWS:
        raise ExportContractError("section row batch exceeds bound")
    filtered = [filter_record(record, rule) for record in records]
    encoded = sum(len(canonical_json(record)) for record in filtered)
    if encoded > MAX_SECTION_BYTES:
        raise ExportContractError("section byte batch exceeds bound")
    if request.section_bytes[store_id] + encoded > MAX_BUNDLE_BYTES:
        raise ExportContractError("section cumulative bytes exceed bundle bound")
    request.sections[store_id].extend(filtered)
    request.section_bytes[store_id] += encoded
    if sum(len(rows) for rows in request.sections.values()) > MAX_BUNDLE_ROWS or \
            sum(request.section_bytes.values()) > MAX_BUNDLE_BYTES:
        request.status = RequestStatus.FAILED
        raise ExportContractError("export bundle exceeds fixed bound")
    request.status = RequestStatus.COLLECTING
    if final:
        request.sections[store_id].sort(key=canonical_json)
        request.completed_sections.add(store_id)


def build_bundle(snapshot: ExportPolicySnapshot, request: ExportRequest,
                 owner_token: bytes) -> bytes:
    if not request.owns(owner_token):
        raise ExportContractError("request ownership mismatch")
    if request.status is not RequestStatus.COLLECTING or \
            request.completed_sections != set(request.sections):
        raise ExportContractError("incomplete request cannot be packaged")
    section_output = []
    total_records = 0
    for store_id in sorted(request.sections):
        rule = snapshot.entries[store_id]["export_rule"]
        records = request.sections[store_id]
        disposition = rule["disposition"]
        payload = canonical_json(records)
        total_records += len(records)
        section_output.append({
            "store_id": store_id,
            "disposition": disposition,
            "status": "excluded" if disposition == "exclude" else "included",
            "subject_route": rule["subject_route"],
            "record_count": len(records),
            "checksum": digest_hex(payload),
            "excluded_fields": sorted(rule["excluded_fields"]),
            "records": records,
        })
    unsigned = {
        "bundle_version": 1,
        "request_id": request.request_id,
        "policy_id": request.policy_id,
        "policy_schema_version": request.policy_schema_version,
        "manifest_checksum": request.manifest_checksum,
        "snapshot_id": request.snapshot_id,
        "generated_at": request.created_at,
        "expires_at": request.expires_at,
        "section_count": len(section_output),
        "record_count": total_records,
        "field_guide": {
            "excluded": "Store is not subject-scoped under the approved export rule.",
            "included": "Fields were filtered by the versioned lifecycle export rule.",
            "checksum": "SHA-256 of canonical section record JSON.",
        },
        "sections": section_output,
    }
    checksum = digest_hex(canonical_json(unsigned))
    bundle = dict(unsigned)
    bundle["package_checksum"] = checksum
    encoded = canonical_json(bundle)
    if len(encoded) > MAX_BUNDLE_BYTES:
        request.status = RequestStatus.FAILED
        raise ExportContractError("package exceeds fixed byte bound")
    request.package_checksum = checksum
    request.status = RequestStatus.PACKAGED
    request.audit.append(ExportAuditEvent(
        "packaged", request.status.value, len(section_output), total_records, len(encoded)
    ))
    return encoded


def verify_bundle(payload: bytes) -> dict:
    def strict_object(pairs: list[tuple[str, object]]) -> dict:
        result = {}
        for key, value in pairs:
            if key in result:
                raise ExportContractError(f"duplicate bundle key: {key}")
            result[key] = value
        return result

    try:
        bundle = json.loads(payload, object_pairs_hook=strict_object)
    except (UnicodeDecodeError, json.JSONDecodeError, ExportContractError) as error:
        raise ExportContractError(f"bundle parse failed: {error}") from error
    required = {
        "bundle_version", "request_id", "policy_id", "policy_schema_version",
        "manifest_checksum", "snapshot_id", "generated_at", "expires_at",
        "section_count", "record_count", "field_guide", "sections",
        "package_checksum",
    }
    if not isinstance(bundle, dict) or set(bundle) != required or \
            not isinstance(bundle.get("sections"), list):
        raise ExportContractError("bundle shape is invalid")
    checksum = bundle.pop("package_checksum")
    if not isinstance(checksum, str) or not hmac.compare_digest(
        checksum, digest_hex(canonical_json(bundle))
    ):
        raise ExportContractError("bundle checksum mismatch")
    bundle["package_checksum"] = checksum
    try:
        if bundle["section_count"] != len(bundle["sections"]) or \
                bundle["record_count"] != sum(
                    section["record_count"] for section in bundle["sections"]
                ):
            raise ExportContractError("bundle count verification failed")
        for section in bundle["sections"]:
            if not isinstance(section, dict) or not isinstance(section["records"], list):
                raise ExportContractError("section shape is invalid")
            if section["record_count"] != len(section["records"]):
                raise ExportContractError("section record count mismatch")
            if section["checksum"] != digest_hex(canonical_json(section["records"])):
                raise ExportContractError("section checksum mismatch")
            if _contains_forbidden_field(section["records"]):
                raise ExportContractError("bundle contains a forbidden field")
    except (KeyError, TypeError) as error:
        raise ExportContractError("section shape is invalid") from error
    return bundle


class ExportSpool:
    def __init__(self, root: Path):
        if not root.is_absolute():
            raise ExportContractError("spool path must be absolute")
        ancestor = root.parent
        while ancestor != ancestor.parent:
            if ancestor.is_symlink():
                raise ExportContractError("spool parent cannot be a symlink")
            ancestor = ancestor.parent
        self.root = root
        if root.exists() and (root.is_symlink() or not root.is_dir()):
            raise ExportContractError("spool must be a real directory")
        try:
            root.mkdir(mode=0o700, exist_ok=True)
        except OSError as error:
            raise ExportContractError(f"spool creation failed: {error}") from error
        os.chmod(root, 0o700)
        metadata = root.stat(follow_symlinks=False)
        if not stat.S_ISDIR(metadata.st_mode) or metadata.st_mode & 0o077:
            raise ExportContractError("spool permissions are not private")

    def _path(self, request_id: str) -> Path:
        if not re.fullmatch(r"[0-9a-f]{32}", request_id):
            raise ExportContractError("invalid request id")
        return self.root / f"{request_id}.json"

    def publish(self, request: ExportRequest, owner_token: bytes, payload: bytes,
                now: int) -> None:
        if not request.owns(owner_token) or request.status is not RequestStatus.PACKAGED:
            raise ExportContractError("package publication ownership or state mismatch")
        if now >= request.expires_at:
            request.status = RequestStatus.EXPIRED
            raise ExportContractError("package expired before publication")
        verified = verify_bundle(payload)
        if verified["request_id"] != request.request_id or \
                verified["package_checksum"] != request.package_checksum:
            raise ExportContractError("package identity mismatch")
        target = self._path(request.request_id)
        if target.exists() or target.is_symlink():
            existing = lifecycle_policy.read_regular_text(
                target, MAX_BUNDLE_BYTES, "existing export package"
            ).encode("utf-8")
            if hmac.compare_digest(existing, payload):
                request.package_path = target
                return
            raise ExportContractError("different package already exists")
        descriptor, temporary_name = tempfile.mkstemp(prefix="export-", dir=self.root)
        temporary = Path(temporary_name)
        try:
            os.fchmod(descriptor, 0o600)
            with os.fdopen(descriptor, "wb", closefd=True) as output:
                output.write(payload)
                output.flush()
                os.fsync(output.fileno())
            os.link(temporary, target, follow_symlinks=False)
            temporary.unlink()
            directory = os.open(self.root, os.O_RDONLY | os.O_DIRECTORY | os.O_CLOEXEC)
            try:
                os.fsync(directory)
            finally:
                os.close(directory)
        except Exception as error:
            try:
                os.close(descriptor)
            except OSError:
                pass
            temporary.unlink(missing_ok=True)
            raise ExportContractError(f"package publication failed: {error}") from error
        request.package_path = target

    def retrieve_once(self, request: ExportRequest, owner_token: bytes, now: int) -> bytes:
        if not request.owns(owner_token):
            raise ExportContractError("request ownership mismatch")
        if now >= request.expires_at:
            self.expire(request, now)
            raise ExportContractError("package expired")
        if request.status is not RequestStatus.PACKAGED or request.package_path is None:
            raise ExportContractError("package is not available")
        path = request.package_path
        raw = lifecycle_policy.read_regular_text(path, MAX_BUNDLE_BYTES, "export package")
        payload = raw.encode("utf-8")
        bundle = verify_bundle(payload)
        if bundle["request_id"] != request.request_id or \
                bundle["package_checksum"] != request.package_checksum:
            raise ExportContractError("retrieval package identity mismatch")
        path.unlink()
        request.package_path = None
        request.status = RequestStatus.RELEASED
        request.audit.append(ExportAuditEvent(
            "released", request.status.value, bundle["section_count"],
            bundle["record_count"], len(payload)
        ))
        return payload

    def cancel(self, request: ExportRequest, owner_token: bytes) -> None:
        if not request.owns(owner_token):
            raise ExportContractError("request ownership mismatch")
        if request.package_path is not None:
            request.package_path.unlink(missing_ok=True)
            request.package_path = None
        request.sections.clear()
        request.status = RequestStatus.CANCELLED
        request.audit.append(ExportAuditEvent("cancelled", request.status.value))

    def expire(self, request: ExportRequest, now: int) -> bool:
        if now < request.expires_at:
            return False
        if request.package_path is not None:
            request.package_path.unlink(missing_ok=True)
            request.package_path = None
        request.sections.clear()
        request.status = RequestStatus.EXPIRED
        request.audit.append(ExportAuditEvent("expired", request.status.value))
        return True


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--manifest", type=Path, default=DEFAULT_MANIFEST)
    parser.add_argument("command", choices=("inspect",))
    return parser.parse_args()


def main() -> int:
    arguments = parse_args()
    try:
        snapshot = load_policy(arguments.manifest)
        dispositions: dict[str, int] = {}
        for entry in snapshot.entries.values():
            disposition = entry["export_rule"]["disposition"]
            dispositions[disposition] = dispositions.get(disposition, 0) + 1
        report = {
            "policy_id": snapshot.manifest["policy_id"],
            "policy_schema_version": snapshot.manifest["schema_version"],
            "manifest_checksum": snapshot.checksum,
            "stores": len(snapshot.entries),
            "dispositions": dispositions,
            "shared_disclosure_enabled": snapshot.manifest["export_policy"]
            ["shared_disclosure_enabled"],
            "request_state": "blocked_by_policy",
        }
        print(json.dumps(report, sort_keys=True))
        return 0
    except (ExportContractError, lifecycle_policy.ValidationError) as error:
        print(f"personal data export blocked: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
