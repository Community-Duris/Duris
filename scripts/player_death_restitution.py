#!/usr/bin/env python3
"""Inspect, plan, apply, and verify SQL death-item restitution.

This tool deliberately has no file-authority implementation. It only operates
against the MySQL/MariaDB ownership authority after the caller proves a stopped
server and drained writers. Protected JSON artifacts contain identifiers and
must not be copied to ordinary logs.
"""

from __future__ import annotations

import argparse
import base64
import datetime as dt
import hashlib
import json
import os
from pathlib import Path
import re
import shlex
import shutil
import stat
import subprocess
import sys
import tempfile
import time
from typing import Any, Iterable, Mapping

ROOT = Path(__file__).resolve().parents[1]
SCRIPTS = ROOT / "scripts"
if str(SCRIPTS) not in sys.path:
    sys.path.insert(0, str(SCRIPTS))
from player_death_restitution_reconciliation import (  # noqa: E402
    ARTIFACT_UNIQUE,
    REAL_ARTIFACT_FLAG,
    classify_item,
    final_domain_expectation,
    reconcile_artifact_authority,
)
from player_death_restitution_backup import create_backup  # noqa: E402
from player_death_restitution_target import (  # noqa: E402
    BACKUP_RECEIPT_FORMAT,
    TARGET_INFO_FORMAT,
    TargetError,
    TargetPolicy,
    protected_file_digest,
    validate_maintenance_record,
    validate_target_record,
    verify_backup_receipt,
)

CODEC_SOURCE = ROOT / "scripts" / "player_death_restitution_codec.cpp"
CODEC_BINARY = ROOT / "bin" / "tools" / "player_death_restitution_codec"

TOOL_VERSION = 3
DEATH_SCHEMA_VERSION = 6
# All supported death encodings carry the same exact item representation.
# Wire 3/4 added pet restoration fields; wire 5/6 added output preferences.
# Let the native codec validate the bytes, then require a death schema.
DEATH_WIRE_VERSIONS = {2, 4, 6}
ITEM_MONEY = 20
VOBJ_COINS = 3
ITEM_ARTIFACT = REAL_ARTIFACT_FLAG
OWNER_UNKNOWN = 0
OWNER_PLAYER = 1
STATE_ACTIVE = 1
STATE_DESTROYED = 2
STATE_QUARANTINED = 3
ITEM_STATE_ABSENT = 0

# Receipt state values are part of the operator contract, not server guesses.
RECEIPT_APPROVED = 1
RECEIPT_APPLIED = 2
RECEIPT_VERIFIED = 3
QUIESCENCE_PROOF_FORMAT = "duris-death-restitution-quiescence-v3"
QUIESCENCE_BOUNDARY = "mysql-advisory-exclusion"
RUNTIME_EXCLUSION_LOCK_PREFIX = "duris.player.death.restitution"
RUNTIME_EXCLUSION_LOCK_EXPRESSION = "CONCAT('duris.player.death.restitution.',DATABASE())"

DISPOSITION = {
    "recoverable_exact": 1,
    "already_delivered": 2,
    "missing_payload": 3,
    "owner_conflict": 4,
    "currency_refused": 5,
    "destroyed": 6,
    "cross_death_payload_conflict": 7,
    "ambiguous_topology": 8,
    "payload_without_custody": 9,
    "custody_absent": 10,
    "artifact_authority_missing": 11,
    "artifact_identity_unbound": 12,
    "artifact_binding_conflict": 13,
    "stale_owner_revision": 14,
    "projection_without_authority": 15,
    "owner_missing": 16,
    "parent_not_recoverable": 17,
    "evidence_inconsistent": 18,
    "recipient_mismatch": 19,
    "recoverable_topology_reconciled": 20,
    "legal_transfer_or_destroyed": 21,
    "owner_vnum_conflict": 22,
    "artifact_legacy_conflict": 23,
    "artifact_competing_instance": 24,
    "recoverable_artifact_reconciled": 25,
    "artifact_baseline_missing": 26,
    "artifact_timing_missing": 27,
    "artifact_timing_expired_at_loss": 28,
}
MAX_ARTIFACT_COMPENSATION_SECONDS = 10 * 365 * 24 * 60 * 60


class ToolError(Exception):
    """An operator-facing error without SQL, credentials, or player data."""


# ---------- safe local files and canonical digests ----------


def canonical_json(value: Any) -> bytes:
    return json.dumps(value, ensure_ascii=True, sort_keys=True, separators=(",", ":")).encode()


def digest_json(value: Any) -> str:
    return hashlib.sha256(canonical_json(value)).hexdigest()


def digest_bytes(value: bytes) -> str:
    return hashlib.sha256(value).hexdigest()


def atomic_write_json(path: Path, value: dict[str, Any], overwrite: bool = False) -> None:
    path = path.expanduser()
    parent = path.parent
    parent.mkdir(mode=0o700, parents=True, exist_ok=True)
    if path.exists() and not overwrite:
        raise ToolError("refusing to overwrite an existing protected artifact")
    if path.is_symlink():
        raise ToolError("refusing a symlink artifact path")
    fd, temporary = tempfile.mkstemp(prefix=f".{path.name}.", dir=parent, text=True)
    temporary_path = Path(temporary)
    try:
        os.fchmod(fd, 0o600)
        with os.fdopen(fd, "w", encoding="utf-8") as stream:
            stream.write(json.dumps(value, ensure_ascii=True, indent=2) + "\n")
            stream.flush()
            os.fsync(stream.fileno())
        if not overwrite and path.exists():
            raise ToolError("refusing to overwrite an existing protected artifact")
        os.replace(temporary_path, path)
        os.chmod(path, 0o600)
    finally:
        try:
            temporary_path.unlink()
        except FileNotFoundError:
            pass


def read_protected_json(path: Path) -> dict[str, Any]:
    path = path.expanduser()
    try:
        info = path.lstat()
    except FileNotFoundError as exc:
        raise ToolError("protected artifact was not found") from exc
    if stat.S_ISLNK(info.st_mode) or not stat.S_ISREG(info.st_mode):
        raise ToolError("protected artifact must be a regular file")
    if info.st_uid != os.getuid() or info.st_mode & 0o077:
        raise ToolError("protected artifact must be owner-readable only")
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise ToolError("protected artifact could not be read") from exc
    if not isinstance(value, dict):
        raise ToolError("protected artifact has an invalid root")
    return value


def verify_artifact_digest(value: dict[str, Any], field: str) -> str:
    supplied = value.get(field)
    if not isinstance(supplied, str) or not re.fullmatch(r"[0-9a-f]{64}", supplied):
        raise ToolError(f"protected artifact has no valid {field}")
    body = dict(value)
    del body[field]
    actual = digest_json(body)
    if actual != supplied:
        raise ToolError(f"protected artifact {field} does not match its contents")
    return supplied


def int_value(value: Any, label: str, minimum: int | None = None,
              maximum: int | None = None) -> int:
    if isinstance(value, bool):
        raise ToolError(f"invalid {label}")
    try:
        number = int(value)
    except (TypeError, ValueError) as exc:
        raise ToolError(f"invalid {label}") from exc
    if minimum is not None and number < minimum:
        raise ToolError(f"invalid {label}")
    if maximum is not None and number > maximum:
        raise ToolError(f"invalid {label}")
    return number


def row_value(row: list[str], index: int) -> str | None:
    if index >= len(row) or row[index] == r"\N":
        return None
    return row[index]


def row_int(row: list[str], index: int, label: str, default: int | None = None) -> int | None:
    value = row_value(row, index)
    if value is None:
        return default
    return int_value(value, label)


def hex_bytes(value: str | None, label: str = "blob") -> bytes:
    if value is None:
        raise ToolError(f"missing {label}")
    if len(value) % 2 or not re.fullmatch(r"[0-9a-fA-F]*", value):
        raise ToolError(f"invalid {label}")
    try:
        return bytes.fromhex(value)
    except ValueError as exc:
        raise ToolError(f"invalid {label}") from exc


def hex_digest(value: str | None, label: str = "digest") -> str:
    if value is None or not re.fullmatch(r"[0-9a-fA-F]{64}", value):
        raise ToolError(f"invalid {label}")
    return value.lower()


def b64(value: bytes) -> str:
    return base64.b64encode(value).decode("ascii")


# ---------- SQL connection and target safety ----------


def load_env_file(path: str | Path | None) -> None:
    if not path:
        return
    env_path = Path(path).expanduser()
    try:
        lines = env_path.read_text(encoding="utf-8").splitlines()
    except OSError as exc:
        raise ToolError("database environment file could not be read") from exc
    for line in lines:
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        if line.startswith("export "):
            line = line[7:].lstrip()
        if "=" not in line:
            continue
        key, raw = line.split("=", 1)
        key = key.strip()
        if not re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", key):
            continue
        try:
            value = shlex.split(raw, comments=True)[0] if raw.strip() else ""
        except ValueError as exc:
            raise ToolError("database environment file has invalid quoting") from exc
        os.environ.setdefault(key, value)


def validate_target(
    *, confirm_production_target: str | None = None,
    expected_fingerprint: str | None = None,
    maintenance_kind: str | None = None,
    maintenance_id: str | None = None,
) -> TargetPolicy:
    """Build the target policy used by every database-facing command."""
    try:
        return TargetPolicy.from_environment(
            os.environ,
            confirm_production_target=confirm_production_target,
            expected_fingerprint=expected_fingerprint,
            maintenance_kind=maintenance_kind,
            maintenance_id=maintenance_id,
        )
    except TargetError as exc:
        raise ToolError(str(exc)) from exc


def target_error(action: str, callback: Any) -> Any:
    """Translate target-module failures without exposing command output."""
    try:
        return callback()
    except TargetError as exc:
        raise ToolError(str(exc)) from exc


def target_record(value: Any, *, label: str = "target") -> dict[str, Any]:
    return target_error(label, lambda: validate_target_record(value, label=label))


def maintenance_record(value: Any, *, label: str = "maintenance boundary") -> dict[str, str]:
    return target_error(label, lambda: validate_maintenance_record(value, label=label))


def target_matches(actual: Mapping[str, Any], expected: Mapping[str, Any], *, label: str = "target") -> None:
    actual_record = target_record(dict(actual), label="current target")
    expected_record = target_record(dict(expected), label=f"approved {label}")
    if actual_record != expected_record:
        raise ToolError(f"current database identity does not match the approved {label}")


class Mysql:
    def __init__(self, policy: TargetPolicy | None = None) -> None:
        self.policy = policy or validate_target()
        binary = os.environ.get("MYSQL_BIN") or shutil.which("mysql")
        if not binary:
            raise ToolError("mysql client is required; file-authority recovery is not implemented")
        self.binary = binary
        self.env = os.environ.copy()
        if self.env.get("DB_PASSWD") and not self.env.get("MYSQL_PWD"):
            self.env["MYSQL_PWD"] = self.env["DB_PASSWD"]
        self.ssl = self._ssl_option()
        self.base = [
            self.binary,
            *self.ssl,
            "--protocol=tcp",
            "--connect-timeout=10",
            "-h", self.env["DB_HOST"],
            "-P", self.env.get("DB_PORT", "3306"),
            "-u", self.env["DB_USER"],
            "-N", "-B", "--raw", "--binary-mode",
        ]
        self.database = self.env["DB_NAME"]

    def _ssl_option(self) -> list[str]:
        try:
            help_result = subprocess.run(
                [self.binary, "--help"], stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                text=True, timeout=10, check=False,
            )
        except (OSError, subprocess.SubprocessError) as exc:
            raise ToolError("mysql client could not be queried") from exc
        return ["--ssl-mode=PREFERRED"] if "--ssl-mode" in help_result.stdout else ["--skip-ssl"]

    def run(self, sql: str, database: bool = True) -> list[list[str]]:
        command = list(self.base)
        if database:
            command.append(self.database)
        try:
            result = subprocess.run(
                command, input=sql + "\n", text=True, stdout=subprocess.PIPE,
                stderr=subprocess.PIPE, env=self.env, timeout=120, check=False,
            )
        except (OSError, subprocess.SubprocessError) as exc:
            raise ToolError("mysql client could not execute the request") from exc
        if result.returncode:
            # Do not return stderr: it can contain SQL literals or connection details.
            raise ToolError("database request failed")
        rows: list[list[str]] = []
        for line in result.stdout.splitlines():
            if line:
                rows.append(line.split("\t"))
        return rows

    def scalar(self, sql: str) -> str:
        rows = self.run(sql)
        if not rows or not rows[0]:
            raise ToolError("database returned no scalar result")
        return rows[0][0]


def load_target_info(path: Path) -> dict[str, Any]:
    """Load and authenticate a read-only target probe artifact."""
    value = read_protected_json(path)
    if value.get("kind") != TARGET_INFO_FORMAT or value.get("artifact_version") != TOOL_VERSION:
        raise ToolError("not a supported restitution target-info artifact")
    verify_artifact_digest(value, "target_info_digest")
    target = target_record(value.get("target"), label="target-info")
    boundary = value.get("maintenance_boundary")
    if boundary is not None:
        value["maintenance_boundary"] = maintenance_record(boundary, label="target-info maintenance boundary")
    value["target"] = target
    return value


def load_backup_receipt(path: Path) -> dict[str, Any]:
    """Load a receipt as a protected object; bind it to a target later."""
    value = read_protected_json(path)
    if value.get("format") != BACKUP_RECEIPT_FORMAT:
        raise ToolError("not a supported restitution backup receipt")
    if not isinstance(value.get("target"), dict):
        raise ToolError("backup receipt has no target identity")
    target_record(value["target"], label="backup receipt target")
    maintenance_record(value.get("maintenance_boundary"), label="backup receipt maintenance boundary")
    return value


def policy_for_command(
    args: argparse.Namespace, *, expected_target: Mapping[str, Any] | None = None,
    require_maintenance: bool = False,
) -> tuple[TargetPolicy, dict[str, Any] | None]:
    """Construct a policy and enforce the exact target carried by an artifact."""
    target_info: dict[str, Any] | None = None
    target_info_path = getattr(args, "target_info", None)
    if target_info_path is not None:
        target_info = load_target_info(target_info_path)
        info_target = target_info["target"]
        if expected_target is not None:
            target_matches(info_target, expected_target, label="target-info target")
        expected_target = info_target

    expected_record = target_record(expected_target, label="approved target") if expected_target is not None else None
    supplied_fingerprint = getattr(args, "expected_fingerprint", None)
    if expected_record is not None:
        approved_fingerprint = expected_record["server_fingerprint"]
        if supplied_fingerprint is not None and supplied_fingerprint != approved_fingerprint:
            raise ToolError("supplied server fingerprint differs from the approved target")
        supplied_fingerprint = approved_fingerprint

    maintenance_kind = getattr(args, "maintenance_kind", None)
    maintenance_id = getattr(args, "maintenance_id", None)
    if (maintenance_kind is None) != (maintenance_id is None):
        raise ToolError("maintenance kind and immutable maintenance identity must be supplied together")
    policy = validate_target(
        confirm_production_target=getattr(args, "confirm_production_target", None),
        expected_fingerprint=supplied_fingerprint,
        maintenance_kind=maintenance_kind,
        maintenance_id=maintenance_id,
    )
    current_fields = {
        "host": policy.host,
        "port": policy.port,
        "database": policy.database,
        "production": policy.production,
    }
    if expected_record is not None and any(
        current_fields[key] != expected_record[key] for key in current_fields
    ):
        raise ToolError("current database connection does not match the approved exact target")
    if require_maintenance and policy.production and (maintenance_kind is None or maintenance_id is None):
        raise ToolError("production mutation requires an explicit maintenance boundary")
    return policy, target_info


def identify_target(
    db: Mysql, policy: TargetPolicy, expected_target: Mapping[str, Any] | None = None,
    *, probe: bool = False,
) -> dict[str, Any]:
    """Read the server identity and optionally compare an approved pin."""
    actual = target_error("database identity", lambda: policy.identity(db, probe=probe))
    if expected_target is not None:
        target_matches(actual, expected_target)
    return actual


def require_policy_maintenance(policy: TargetPolicy) -> dict[str, str]:
    return target_error("maintenance boundary", policy.require_maintenance)


def validate_plan_target_controls(
    db: Mysql, plan: Mapping[str, Any], policy: TargetPolicy,
    *, require_maintenance: bool = False,
    target_info: Mapping[str, Any] | None = None,
) -> tuple[dict[str, Any] | None, dict[str, str] | None]:
    """Revalidate target, lifecycle boundary, and backup before a DB write."""
    expected = plan.get("target")
    if expected is None:
        if policy.production:
            raise ToolError("production operation requires a target-pinned plan")
        return None, None
    expected_record = target_record(expected, label="plan target")
    if target_info is not None:
        info_target = target_record(target_info.get("target"), label="target-info target")
        if info_target != expected_record:
            raise ToolError("target-info does not match the approved plan target")
        info_boundary = target_info.get("maintenance_boundary")
        if info_boundary is not None and plan.get("maintenance_boundary") != info_boundary:
            raise ToolError("target-info maintenance boundary differs from the approved plan")
    actual = identify_target(db, policy, expected_record)
    boundary: dict[str, str] | None = None
    if require_maintenance or policy.production:
        boundary = require_policy_maintenance(policy)
        expected_boundary = plan.get("maintenance_boundary")
        if expected_boundary is None:
            raise ToolError("production plan has no approved maintenance boundary")
        if boundary != maintenance_record(expected_boundary, label="plan maintenance boundary"):
            raise ToolError("current maintenance boundary differs from the approved plan")
    return actual, boundary


def validate_plan_backup(
    plan: Mapping[str, Any], target: Mapping[str, Any],
    maintenance: Mapping[str, Any] | None,
) -> None:
    """Verify the exact receipt and opened dump bound into a plan."""
    receipt = plan.get("backup_receipt")
    if not isinstance(receipt, dict):
        raise ToolError("production plan has no approved native backup receipt")
    receipt_target = receipt.get("target")
    if receipt_target != dict(target):
        raise ToolError("backup receipt target differs from the approved plan target")
    expected_boundary = plan.get("maintenance_boundary")
    if maintenance is None or expected_boundary is None:
        raise ToolError("production plan has no approved maintenance-bound backup")
    if receipt.get("maintenance_boundary") != dict(expected_boundary):
        raise ToolError("backup receipt is not bound to the approved plan boundary")
    try:
        verify_backup_receipt(receipt, target, maintenance)
    except TargetError as exc:
        raise ToolError(str(exc)) from exc


def sql_num(value: Any, label: str, minimum: int | None = None,
            maximum: int | None = None) -> str:
    return str(int_value(value, label, minimum, maximum))


def sql_blob(value: bytes | str, label: str = "blob") -> str:
    if isinstance(value, str):
        value = hex_bytes(value, label)
    return "UNHEX('" + value.hex() + "')"


def sql_nullable_blob(value: bytes | str | None, label: str = "blob") -> str:
    return "NULL" if value is None else sql_blob(value, label)


def sql_text(value: str, label: str = "text") -> str:
    if not isinstance(value, str) or len(value) > 4096:
        raise ToolError(f"invalid {label}")
    return sql_blob(value.encode("utf-8"), label)


def sql_list(values: Iterable[int]) -> str:
    checked = [sql_num(value, "item identity", 1, 2**64 - 1) for value in values]
    if not checked:
        return "(0)"
    return "(" + ",".join(checked) + ")"


def table_exists(db: Mysql, table: str) -> bool:
    if not re.fullmatch(r"[a-z_]+", table):
        raise ToolError("invalid schema table name")
    return db.scalar(
        "SELECT COUNT(*) FROM information_schema.tables "
        f"WHERE table_schema=DATABASE() AND table_name='{table}'"
    ) == "1"


def require_schema(db: Mysql) -> None:
    required = {
        "item_current_owner", "item_owner_revision", "player_death_disposition", "player_death_custody",
        "player_items", "player_item_affects", "player_item_extra_descr",
        "player_death_restitution_receipt", "player_death_restitution_item",
        "player_death_restitution_delivery", "player_death_restitution_runtime",
    }
    rows = db.run(
        "SELECT table_name FROM information_schema.tables WHERE table_schema=DATABASE() "
        "AND table_name IN (" + ",".join("'" + name + "'" for name in sorted(required)) + ")"
    )
    found = {row[0] for row in rows if row}
    if found != required:
        raise ToolError("required SQL death/ownership/restitution schema is incomplete")


# ---------- source evidence reads ----------


def fetch_death(db: Mysql, pid: int, revision: int) -> dict[str, Any] | None:
    rows = db.run(
        "SELECT HEX(operation_id),corpse_item_uid,corpse_room_vnum,wallet_revision,"
        "wallet_copper,wallet_silver,wallet_gold,wallet_platinum,wallet_pile_uid,HEX(payload),"
        "FLOOR(UNIX_TIMESTAMP(recorded_at)) FROM player_death_disposition WHERE pid=" + sql_num(pid, "pid", 1, 2**31 - 1) +
        " AND save_revision=" + sql_num(revision, "death revision", 1, 2**64 - 1)
    )
    if not rows:
        return None
    row = rows[0]
    if len(row) != 11:
        raise ToolError("death disposition row has an unexpected shape")
    payload_hex = row_value(row, 9)
    if payload_hex is None:
        raise ToolError("death disposition payload is missing")
    loss_epoch = row_int(row, 10, "death recorded epoch", 1)
    if loss_epoch is None:
        raise ToolError("death disposition recorded timestamp is missing")
    return {
        "pid": pid,
        "save_revision": revision,
        "operation_id_hex": (row_value(row, 0) or "").lower(),
        "corpse_item_uid": row_int(row, 1, "corpse UID", 1),
        "corpse_room_vnum": row_int(row, 2, "corpse room", 1),
        "wallet_revision": row_int(row, 3, "wallet revision", 1),
        "wallet_before": [row_int(row, index, "wallet amount", 0) for index in range(4, 8)],
        "wallet_pile_uid": row_int(row, 8, "wallet pile UID", 0),
        "payload_hex": payload_hex.lower(),
        "payload_digest": digest_bytes(hex_bytes(payload_hex, "death payload")),
        "loss_epoch": loss_epoch,
    }


def fetch_custody(db: Mysql, pid: int, revision: int) -> list[dict[str, Any]]:
    rows = db.run(
        "SELECT item_uid,root_item_uid,COALESCE(parent_item_uid,0),item_revision,vnum,state,"
        "owner_type,owner_id,owner_context_id,owner_revision FROM player_death_custody "
        "WHERE pid=" + sql_num(pid, "pid", 1, 2**31 - 1) +
        " AND save_revision=" + sql_num(revision, "death revision", 1, 2**64 - 1) +
        " ORDER BY item_uid"
    )
    result: list[dict[str, Any]] = []
    for row in rows:
        if len(row) != 10:
            raise ToolError("death custody row has an unexpected shape")
        result.append({
            "pid": pid,
            "save_revision": revision,
            "item_uid": row_int(row, 0, "item UID", 1),
            "root_item_uid": row_int(row, 1, "root item UID", 1),
            "parent_item_uid": row_int(row, 2, "parent item UID", 0),
            "expected_item_revision": row_int(row, 3, "item revision", 0),
            "vnum": row_int(row, 4, "item vnum", 1),
            "expected_state": row_int(row, 5, "item state", 0),
            "owner_type": row_int(row, 6, "owner type", 0),
            "owner_id": row_int(row, 7, "owner ID", 0),
            "owner_context_id": row_int(row, 8, "owner context", 0),
            "owner_revision": row_int(row, 9, "owner revision", 0),
        })
    return result


def fetch_custody_pairs(
    db: Mysql, pairs: Iterable[tuple[int, int]]
) -> dict[tuple[int, int], list[dict[str, Any]]]:
    checked = sorted({
        (
            int_value(pid, "related custody pid", 1, 2**31 - 1),
            int_value(revision, "related custody revision", 1, 2**64 - 1),
        )
        for pid, revision in pairs
    })
    if not checked:
        return {}
    clauses = [
        "(pid=" + sql_num(pid, "related custody pid", 1, 2**31 - 1) +
        " AND save_revision=" + sql_num(revision, "related custody revision", 1, 2**64 - 1) + ")"
        for pid, revision in checked
    ]
    rows = db.run(
        "SELECT pid,save_revision,item_uid,root_item_uid,COALESCE(parent_item_uid,0),"
        "item_revision,vnum,state,owner_type,owner_id,owner_context_id,owner_revision "
        "FROM player_death_custody WHERE " + " OR ".join(clauses) +
        " ORDER BY pid,save_revision,item_uid"
    )
    result: dict[tuple[int, int], list[dict[str, Any]]] = {}
    for row in rows:
        if len(row) != 12:
            raise ToolError("related death custody row has an unexpected shape")
        pid = row_int(row, 0, "related custody pid", 1)
        revision = row_int(row, 1, "related custody revision", 1)
        if pid is None or revision is None:
            raise ToolError("related death custody identity is missing")
        result.setdefault((pid, revision), []).append({
            "pid": pid,
            "save_revision": revision,
            "item_uid": row_int(row, 2, "related custody item UID", 1),
            "root_item_uid": row_int(row, 3, "related custody root UID", 1),
            "parent_item_uid": row_int(row, 4, "related custody parent UID", 0),
            "expected_item_revision": row_int(row, 5, "related custody item revision", 0),
            "vnum": row_int(row, 6, "related custody vnum", 1),
            "expected_state": row_int(row, 7, "related custody state", 0),
            "owner_type": row_int(row, 8, "related custody owner type", 0),
            "owner_id": row_int(row, 9, "related custody owner ID", 0),
            "owner_context_id": row_int(row, 10, "related custody context", 0),
            "owner_revision": row_int(row, 11, "related custody owner revision", 0),
        })
    return result


def fetch_current_owners(db: Mysql, uids: Iterable[int]) -> dict[str, dict[str, Any]]:
    values = sorted({int_value(uid, "item UID", 1, 2**64 - 1) for uid in uids})
    if not values:
        return {}
    rows = db.run(
        "SELECT own.item_uid,own.root_item_uid,COALESCE(own.parent_item_uid,0),"
        "own.owner_type,own.owner_id,own.owner_context_id,own.item_revision,own.vnum,own.state,"
        "COALESCE(revision.revision,0) FROM item_current_owner own LEFT JOIN item_owner_revision revision "
        "ON revision.owner_type=own.owner_type AND revision.owner_id=own.owner_id "
        "AND revision.owner_context_id=own.owner_context_id WHERE own.item_uid IN " + sql_list(values) +
        " ORDER BY own.item_uid"
    )
    result: dict[str, dict[str, Any]] = {}
    for row in rows:
        if len(row) != 10:
            raise ToolError("current ownership row has an unexpected shape")
        uid = row_int(row, 0, "item UID", 1)
        result[str(uid)] = {
            "item_uid": uid,
            "root_item_uid": row_int(row, 1, "root item UID", 1),
            "parent_item_uid": row_int(row, 2, "parent item UID", 0),
            "owner_type": row_int(row, 3, "owner type", 0),
            "owner_id": row_int(row, 4, "owner ID", 0),
            "owner_context_id": row_int(row, 5, "owner context", 0),
            "item_revision": row_int(row, 6, "item revision", 0),
            "vnum": row_int(row, 7, "item vnum", 0),
            "state": row_int(row, 8, "item state", 0),
            "owner_revision": row_int(row, 9, "owner revision", 0),
        }
    return result


def fetch_player_projections(db: Mysql, uids: Iterable[int]) -> list[dict[str, Any]]:
    values = sorted({int_value(uid, "item UID", 1, 2**64 - 1) for uid in uids})
    if not values:
        return []
    rows = db.run(
        "SELECT id,pid,obj_uid,vnum,equip_slot,COALESCE(container_id,0) FROM player_items "
        "WHERE obj_uid IN " + sql_list(values) + " ORDER BY id"
    )
    result = []
    for row in rows:
        if len(row) != 6:
            raise ToolError("player item projection has an unexpected shape")
        result.append({
            "id": row_int(row, 0, "player item row", 1),
            "pid": row_int(row, 1, "player item owner", 1),
            "item_uid": row_int(row, 2, "item UID", 1),
            "vnum": row_int(row, 3, "item vnum", 1),
            "equip_slot": row_int(row, 4, "equipment slot", -32768),
            "container_id": row_int(row, 5, "container ID", 0),
        })
    return result


def fetch_recipient_uids(db: Mysql, pid: int) -> list[int]:
    rows = db.run(
        "SELECT obj_uid FROM player_items WHERE pid=" + sql_num(pid, "pid", 1, 2**31 - 1) +
        " AND obj_uid IS NOT NULL ORDER BY obj_uid"
    )
    result: list[int] = []
    for row in rows:
        value = row_int(row, 0, "recipient item UID", 1)
        if value is not None:
            result.append(value)
    return result


def fetch_deliveries(db: Mysql, uids: Iterable[int]) -> dict[str, dict[str, Any]]:
    values = sorted({int_value(uid, "item UID", 1, 2**64 - 1) for uid in uids})
    if not values:
        return {}
    rows = db.run(
        "SELECT item_uid,HEX(restitution_id),source_pid,death_revision,recipient_pid,"
        "source_item_revision,delivered_item_revision,delivered_item_id,HEX(metadata_digest),"
        "HEX(original_payload) FROM player_death_restitution_delivery WHERE item_uid IN " + sql_list(values) +
        " ORDER BY item_uid"
    )
    result: dict[str, dict[str, Any]] = {}
    for row in rows:
        if len(row) != 10:
            raise ToolError("restitution delivery row has an unexpected shape")
        uid = row_int(row, 0, "delivery UID", 1)
        result[str(uid)] = {
            "item_uid": uid,
            "restitution_id_hex": (row_value(row, 1) or "").lower(),
            "source_pid": row_int(row, 2, "delivery source", 1),
            "death_revision": row_int(row, 3, "delivery death revision", 1),
            "recipient_pid": row_int(row, 4, "delivery recipient", 1),
            "source_item_revision": row_int(row, 5, "delivery source revision", 0),
            "delivered_item_revision": row_int(row, 6, "delivery item revision", 0),
            "delivered_item_id": row_int(row, 7, "delivered database row", 1),
            "metadata_digest": hex_digest(row_value(row, 8), "delivery metadata digest"),
            "original_payload_hex": (row_value(row, 9) or "").lower(),
        }
    return result


def fetch_artifacts(db: Mysql, vnums: Iterable[int]) -> dict[str, Any]:
    values = sorted({int_value(vnum, "artifact vnum", 1, 2**31 - 1) for vnum in vnums})
    result: dict[str, Any] = {
        "domain": {}, "domain_table_present": False, "baseline": {}, "baseline_table_present": False,
        "bind": {}, "bind_table_present": False, "mortal": {}, "god": {}, "competitors": {},
    }
    if not values:
        return result
    if table_exists(db, "artifact_domain_state"):
        result["domain_table_present"] = True
        rows = db.run(
            "SELECT vnum,owned,loc_type,location,timer_epoch,artifact_type,bind_owner_pid,"
            "bind_timer_epoch,item_uid,COALESCE(item_revision,0),revision "
            "FROM artifact_domain_state WHERE vnum IN " + sql_list(values) + " ORDER BY vnum"
        )
        for row in rows:
            if len(row) != 11:
                raise ToolError("artifact domain row has an unexpected shape")
            vnum = row_int(row, 0, "artifact vnum", 1)
            result["domain"][str(vnum)] = {
                "vnum": vnum, "owned": row_int(row, 1, "artifact owned", 0),
                "loc_type": row_int(row, 2, "artifact location type", 0),
                "location": row_int(row, 3, "artifact location", 0),
                "timer_epoch": row_int(row, 4, "artifact timer", 0),
                "artifact_type": row_int(row, 5, "artifact type", 0),
                "bind_owner_pid": row_int(row, 6, "artifact bind owner", 0),
                "bind_timer_epoch": row_int(row, 7, "artifact bind timer", 0),
                "item_uid": None if row_value(row, 8) is None else row_int(row, 8, "artifact item UID", 0),
                "item_revision": row_int(row, 9, "artifact item revision", 0),
                "revision": row_int(row, 10, "artifact revision", 0),
            }
    if table_exists(db, "artifact_domain_baseline"):
        result["baseline_table_present"] = True
        rows = db.run(
            "SELECT vnum,opening_timer_epoch,opening_bind_owner_pid,opening_bind_timer_epoch,opening_revision "
            "FROM artifact_domain_baseline WHERE vnum IN " + sql_list(values) + " ORDER BY vnum"
        )
        for row in rows:
            if len(row) != 5:
                raise ToolError("artifact baseline row has an unexpected shape")
            vnum = row_int(row, 0, "artifact baseline vnum", 1)
            result["baseline"][str(vnum)] = {
                "vnum": vnum,
                "opening_timer_epoch": row_int(row, 1, "artifact opening timer", 0),
                "opening_bind_owner_pid": row_int(row, 2, "artifact opening bind owner", -1),
                "opening_bind_timer_epoch": row_int(row, 3, "artifact opening bind timer", 0),
                "opening_revision": row_int(row, 4, "artifact opening revision", 0),
            }
    if table_exists(db, "artifact_bind"):
        result["bind_table_present"] = True
        rows = db.run(
            "SELECT vnum,COALESCE(owner_pid,-1),COALESCE(timer,0) FROM artifact_bind "
            "WHERE vnum IN " + sql_list(values) + " ORDER BY vnum"
        )
        for row in rows:
            if len(row) != 3:
                raise ToolError("artifact binding row has an unexpected shape")
            vnum = row_int(row, 0, "artifact vnum", 1)
            result["bind"][str(vnum)] = {
                "vnum": vnum, "owner_pid": row_int(row, 1, "artifact bind owner", -1),
                "timer": row_int(row, 2, "artifact bind timer", 0),
            }
    for table, key in (("artifacts_mortal", "mortal"), ("artifacts", "god")):
        if not table_exists(db, table):
            continue
        rows = db.run(
            "SELECT vnum,COALESCE(owned,''),COALESCE(locType,0),COALESCE(location,0),"
            "COALESCE(UNIX_TIMESTAMP(timer),0),COALESCE(type,0) FROM " + table +
            " WHERE vnum IN " + sql_list(values) + " ORDER BY vnum"
        )
        for row in rows:
            if len(row) != 6:
                raise ToolError("legacy artifact row has an unexpected shape")
            vnum = row_int(row, 0, "artifact vnum", 1)
            result[key][str(vnum)] = {
                "vnum": vnum, "owned": row_value(row, 1) or "", "loc_type": row_int(row, 2, "artifact location type", 0),
                "location": row_int(row, 3, "artifact location", 0), "timer": row_int(row, 4, "artifact timer", 0),
                "artifact_type": row_int(row, 5, "artifact type", 0),
            }
    # A second live UID with the same vnum is a surviving competing instance,
    # even when the selected UID's canonical row is missing.
    rows = db.run(
        "SELECT item_uid,vnum,state,owner_type,owner_id,owner_context_id,item_revision "
        "FROM item_current_owner WHERE vnum IN " + sql_list(values) + " ORDER BY vnum,item_uid"
    )
    for row in rows:
        if len(row) != 7:
            raise ToolError("artifact competitor row has an unexpected shape")
        vnum = row_int(row, 1, "artifact competitor vnum", 1)
        result["competitors"].setdefault(str(vnum), []).append({
            "item_uid": row_int(row, 0, "artifact competitor UID", 1),
            "vnum": vnum,
            "state": row_int(row, 2, "artifact competitor state", 0),
            "owner_type": row_int(row, 3, "artifact competitor owner type", 0),
            "owner_id": row_int(row, 4, "artifact competitor owner ID", 0),
            "owner_context_id": row_int(row, 5, "artifact competitor context", 0),
            "item_revision": row_int(row, 6, "artifact competitor revision", 0),
        })
    # Historical custody is evidence too. A newer explicit destruction in the
    # current-owner table retires it; absent/nonterminal authority does not.
    rows = db.run(
        "SELECT c.item_uid,c.vnum,c.state,c.owner_type,c.owner_id,c.owner_context_id,"
        "c.item_revision,c.pid,c.save_revision FROM player_death_custody c "
        "LEFT JOIN item_current_owner resolved ON resolved.item_uid=c.item_uid "
        "WHERE c.vnum IN " + sql_list(values) +
        " AND c.state<>2 AND (resolved.item_uid IS NULL OR resolved.state<>2) "
        "ORDER BY c.vnum,c.item_uid,c.pid,c.save_revision"
    )
    for row in rows:
        if len(row) != 9:
            raise ToolError("artifact custody competitor row has an unexpected shape")
        vnum = row_int(row, 1, "artifact custody competitor vnum", 1)
        result["competitors"].setdefault(str(vnum), []).append({
            "source_table": "player_death_custody",
            "item_uid": row_int(row, 0, "artifact custody competitor UID", 1),
            "vnum": vnum,
            "state": row_int(row, 2, "artifact custody competitor state", 0),
            "owner_type": row_int(row, 3, "artifact custody owner type", 0),
            "owner_id": row_int(row, 4, "artifact custody owner ID", 0),
            "owner_context_id": row_int(row, 5, "artifact custody context", 0),
            "item_revision": row_int(row, 6, "artifact custody revision", 0),
            "pid": row_int(row, 7, "artifact custody PID", 1),
            "save_revision": row_int(row, 8, "artifact custody death revision", 0),
        })
    return result


def normalize_payload_items(items: Any) -> tuple[list[dict[str, Any]], dict[str, int], dict[str, int], set[int], list[str]]:
    """Remove synthetic corpse/index identity from comparable item evidence."""
    if not isinstance(items, list) or not items:
        raise ToolError("death payload has no item list")
    by_index: dict[int, dict[str, Any]] = {}
    parent_indexes: dict[int, int] = {}
    seen_uids: set[int] = set()
    for index, item in enumerate(items):
        if not isinstance(item, dict):
            raise ToolError("death payload item has an invalid shape")
        uid = int_value(item.get("object_uid"), "payload item UID", 1, 2**64 - 1)
        if uid in seen_uids:
            raise ToolError("death payload contains a duplicate item UID")
        seen_uids.add(uid)
        parent_index = int_value(
            item.get("parent_index"), "payload parent index", -1, len(items) - 1
        )
        if index == 0:
            if parent_index != -1:
                raise ToolError("synthetic corpse root has a parent")
        elif parent_index < 0 or parent_index >= index:
            raise ToolError("death payload item ancestry is invalid")
        by_index[index] = item
        parent_indexes[index] = parent_index

    parent: dict[str, int] = {}
    root: dict[str, int] = {}
    normalized: list[dict[str, Any]] = []
    order: list[str] = []
    for index in range(1, len(items)):
        item = by_index[index]
        uid = int_value(item["object_uid"], "payload item UID", 1, 2**64 - 1)
        parent_index = parent_indexes[index]
        if parent_index == 0:
            parent_uid = 0
            root_uid = uid
        else:
            parent_uid = int_value(
                by_index[parent_index].get("object_uid"), "payload parent UID", 1, 2**64 - 1
            )
            root_uid = root[str(parent_uid)]
        canonical = dict(item)
        canonical.pop("parent_index", None)
        canonical["object_uid"] = uid
        canonical["parent_item_uid"] = parent_uid
        canonical["root_item_uid"] = root_uid
        normalized.append(canonical)
        order.append(str(uid))
        parent[str(uid)] = parent_uid
        root[str(uid)] = root_uid
    return normalized, parent, root, seen_uids - {int_value(items[0]["object_uid"], "corpse UID", 1)}, order


def related_death_pairs(db: Mysql, source_pid: int, revision: int, uids: Iterable[int]) -> list[tuple[int, int]]:
    values = sorted({int_value(uid, "item UID", 1, 2**64 - 1) for uid in uids})
    if not values:
        return [(source_pid, revision)]
    rows = db.run(
        "SELECT DISTINCT pid,save_revision FROM player_death_custody WHERE item_uid IN " + sql_list(values)
    )
    pairs = {(source_pid, revision)}
    for row in rows:
        if len(row) != 2:
            raise ToolError("related custody row has an unexpected shape")
        pairs.add((row_int(row, 0, "related death pid", 1), row_int(row, 1, "related death revision", 1)))
    return sorted(pairs)


def fetch_related_deaths(db: Mysql, source_pid: int, revision: int, uids: Iterable[int],
                         decode: Any) -> list[dict[str, Any]]:
    pairs = related_death_pairs(db, source_pid, revision, uids)
    custody_by_pair = fetch_custody_pairs(db, pairs)
    clauses = [
        "(pid=" + sql_num(pid, "related death pid", 1, 2**31 - 1) +
        " AND save_revision=" + sql_num(rev, "related death revision", 1, 2**64 - 1) + ")"
        for pid, rev in pairs
    ]
    rows = db.run(
        "SELECT pid,save_revision,HEX(operation_id),HEX(payload),"
        "FLOOR(UNIX_TIMESTAMP(recorded_at)) FROM player_death_disposition WHERE " +
        " OR ".join(clauses) + " ORDER BY pid,save_revision"
    )
    result: list[dict[str, Any]] = []
    for row in rows:
        if len(row) != 5:
            raise ToolError("related death row has an unexpected shape")
        pid = row_int(row, 0, "related death pid", 1)
        rev = row_int(row, 1, "related death revision", 1)
        if pid is None or rev is None:
            raise ToolError("related death identity is missing")
        operation_id_hex = (row_value(row, 2) or "").lower()
        payload_hex = row_value(row, 3)
        entry: dict[str, Any] = {
            "pid": pid,
            "save_revision": rev,
            "operation_id_hex": operation_id_hex,
            "loss_epoch": row_int(row, 4, "related death recorded epoch", 1),
            "payload_uids": [],
            "payload_items": [],
            "payload_order": [],
            "custody": custody_by_pair.get((pid, rev), []),
        }
        try:
            payload = hex_bytes(payload_hex, "related death payload")
            entry["payload_digest"] = digest_bytes(payload)
            decoded = decode(payload)
            if not isinstance(decoded, dict) or not isinstance(decoded.get("death"), dict):
                raise ToolError("related death payload has an invalid death object")
            death = decoded["death"]
            normalized, _, _, _, order = normalize_payload_items(death.get("corpse"))
            entry["payload_items"] = normalized
            entry["payload_uids"] = [item["object_uid"] for item in normalized]
            entry["payload_order"] = order
            entry["normalized_payload_digest"] = digest_json(normalized)
            identity_matches = (
                decoded.get("pid") == pid
                and decoded.get("revision") == rev
                and str(death.get("operation_id_hex", "")).lower() == operation_id_hex
            )
            if not identity_matches:
                entry["identity_error"] = "related death payload identity does not match its SQL row"
            else:
                entry["wire_version"] = decoded["wire_version"]
                entry["schema_version"] = decoded["schema_version"]
        except ToolError:
            entry["decode_error"] = "codec rejected related death payload"
        result.append(entry)
    return result


# ---------- supported player snapshot codec bridge ----------


def ensure_codec() -> Path:
    CODEC_BINARY.parent.mkdir(mode=0o755, parents=True, exist_ok=True)
    if CODEC_BINARY.exists() and CODEC_BINARY.stat().st_mtime_ns >= CODEC_SOURCE.stat().st_mtime_ns:
        return CODEC_BINARY
    command = [
        "g++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-Isrc",
        str(CODEC_SOURCE), str(ROOT / "src/player/player_snapshot_codec.c"),
        "-o", str(CODEC_BINARY),
    ]
    try:
        result = subprocess.run(command, cwd=ROOT, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                timeout=120, check=False)
    except (OSError, subprocess.SubprocessError) as exc:
        raise ToolError("could not build the supported death snapshot codec bridge") from exc
    if result.returncode:
        raise ToolError("supported death snapshot codec bridge failed to build")
    os.chmod(CODEC_BINARY, 0o755)
    return CODEC_BINARY


def decode_payload(payload: bytes) -> dict[str, Any]:
    binary = ensure_codec()
    try:
        result = subprocess.run([str(binary), "decode-death"], input=payload,
                                stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=120, check=False)
    except (OSError, subprocess.SubprocessError) as exc:
        raise ToolError("supported death snapshot codec bridge could not run") from exc
    if result.returncode:
        raise ToolError("death payload was rejected by player_snapshot_codec")
    try:
        decoded = json.loads(result.stdout.decode("utf-8"))
    except (UnicodeDecodeError, json.JSONDecodeError) as exc:
        raise ToolError("codec bridge returned invalid structured output") from exc
    if not isinstance(decoded, dict):
        raise ToolError("codec bridge returned an invalid death object")
    if decoded.get("wire_version") not in DEATH_WIRE_VERSIONS or decoded.get("schema_version") != DEATH_SCHEMA_VERSION:
        raise ToolError("death payload is not a supported schema-6 death encoding")
    return decoded


# ---------- inspection and plan ----------


def compare_custody(decoded: dict[str, Any], custody: list[dict[str, Any]],
                    allow_custody_only: bool = False) -> list[str]:
    expected = {str(row["item_uid"]): row for row in decoded["death"]["custody"]}
    actual = {str(row["item_uid"]): row for row in custody}
    errors: list[str] = []
    if (not allow_custody_only and set(expected) != set(actual)) or (
            allow_custody_only and not set(actual).issuperset(expected)):
        errors.append("payload and SQL custody UID sets differ")
    fields = (
        "root_item_uid", "parent_item_uid", "expected_item_revision", "vnum", "expected_state",
        "owner_type", "owner_id", "owner_context_id", "owner_revision",
    )
    for uid in sorted(set(expected) & set(actual), key=int):
        for field in fields:
            left = expected[uid][field]
            right = actual[uid][field]
            if left != right:
                errors.append("payload and SQL custody metadata differ")
                break
    return sorted(set(errors))


def is_unique_item(item: dict[str, Any]) -> bool:
    try:
        name = bytes.fromhex(item.get("name_hex", "")).decode("ascii", "ignore").lower()
    except ValueError:
        name = ""
    return bool(re.search(r"(?:^|[ _-])unique(?:$|[ _-])", name)) and "powerunique" not in name


def item_kind(item: dict[str, Any], artifacts: dict[str, dict[str, Any]]) -> str:
    if item["object_uid"] == 0:
        return "invalid"
    if item["type"] == ITEM_MONEY or item["vnum"] == VOBJ_COINS:
        return "currency"
    return classify_item(item, artifacts)["kind"]


def artifact_reconciliation(item: dict[str, Any], pid: int, artifacts: dict[str, Any],
                            current: dict[str, Any], loss_epoch: int | None = None) -> dict[str, Any]:
    """Adapter kept in the runner so plan code has one auditable authority call."""
    competitors = artifacts.get("competitors", {}).get(str(item.get("vnum")), [])
    return reconcile_artifact_authority(item, pid, current, artifacts, competitors, loss_epoch)


def artifact_authority_fence(item: dict[str, Any], pid: int, artifacts: dict[str, Any],
                             current: dict[str, Any]) -> tuple[str | None, str, dict[str, Any] | None, dict[str, dict[str, Any]]]:
    """Backward-compatible view of the richer reconciliation decision."""
    # Older source-contract tests call this narrow helper with only the
    # domain/item-revision fields. Keep that diagnostic surface stable; real
    # plan candidates always carry the complete current-owner fence.
    if "owner_type" not in current:
        return _legacy_artifact_authority_fence(item, pid, artifacts, current)
    decision = artifact_reconciliation(item, pid, artifacts, current)
    if not decision["ok"]:
        return decision["classification"], decision["note"], None, decision["legacy_before"]
    domain = decision.get("domain_before") or decision.get("domain_seed")
    return None, decision["note"], domain, decision["legacy_before"]


def _legacy_artifact_authority_fence(item: dict[str, Any], pid: int, artifacts: dict[str, Any],
                             current: dict[str, Any]) -> tuple[str | None, str, dict[str, Any] | None, dict[str, dict[str, Any]]]:
    vnum_text = str(item["vnum"])
    domain = artifacts.get("domain", {}).get(vnum_text)
    if domain is None:
        return (
            "artifact_authority_missing",
            "artifact identity authority is unavailable; no mint or registry reset",
            None,
            {},
        )
    if domain.get("item_uid") != item["object_uid"] or domain.get("owned") != 1:
        return (
            "artifact_identity_unbound",
            "artifact authority does not bind this original UID",
            None,
            {},
        )
    if domain.get("loc_type") not in {3, 5} or domain.get("location") != pid:
        return (
            "artifact_legacy_conflict",
            "artifact authority location is not the source player's PC/corpse location",
            None,
            {},
        )
    if domain.get("bind_owner_pid") != pid:
        return (
            "artifact_binding_conflict",
            "artifact domain binding does not belong to the source player",
            None,
            {},
        )
    if domain.get("item_revision") != current.get("item_revision"):
        return (
            "artifact_binding_conflict",
            "artifact domain item revision differs from current ownership",
            None,
            {},
        )
    bind = artifacts.get("bind", {}).get(vnum_text)
    if bind is not None and (
            bind.get("owner_pid") not in {pid, 0, -1} or
            bind.get("timer") != domain.get("bind_timer_epoch")):
        return (
            "artifact_binding_conflict",
            "artifact binding authority conflicts with the domain fence",
            None,
            {},
        )

    legacy_rows: dict[str, dict[str, Any]] = {}
    legacy_signature: tuple[Any, ...] | None = None
    for legacy_name in ("mortal", "god"):
        legacy = artifacts.get(legacy_name, {}).get(vnum_text)
        if legacy is None:
            continue
        legacy_rows[legacy_name] = legacy
        owned = str(legacy.get("owned", "")).upper()
        signature = (
            legacy.get("loc_type"), legacy.get("location"), legacy.get("timer"),
            legacy.get("artifact_type"),
        )
        if owned != "Y":
            return (
                "artifact_legacy_conflict",
                "legacy artifact authority is not in an owned state",
                None,
                {},
            )
        if legacy.get("loc_type") not in {3, 5} or legacy.get("location") != pid:
            return (
                "artifact_legacy_conflict",
                "legacy artifact location is not the source player's PC/corpse location",
                None,
                {},
            )
        if (legacy.get("artifact_type") != domain.get("artifact_type") or
                legacy.get("timer") != domain.get("timer_epoch")):
            return (
                "artifact_legacy_conflict",
                "legacy artifact authority does not match the domain fence",
                None,
                {},
            )
        if legacy_signature is not None and legacy_signature != signature:
            return (
                "artifact_legacy_conflict",
                "legacy artifact authorities disagree",
                None,
                {},
            )
        legacy_signature = signature
    if not legacy_rows:
        return (
            "artifact_authority_missing",
            "no source-supported legacy artifact authority row exists",
            None,
            {},
        )
    return None, "", domain, legacy_rows


def topology_for(items: list[dict[str, Any]]) -> tuple[dict[str, int], dict[str, int], set[int]]:
    _, parent, root, asset_uids, _ = normalize_payload_items(items)
    return parent, root, asset_uids


def build_inspection(
    db: Mysql, pid: int, revision: int, recipient_pid: int,
    target: Mapping[str, Any] | None = None,
) -> dict[str, Any]:
    require_schema(db)
    death = fetch_death(db, pid, revision)
    if death is None:
        raise ToolError("selected death record was not found")
    payload = hex_bytes(death["payload_hex"], "death payload")
    decoded = decode_payload(payload)
    errors: list[str] = []
    if decoded.get("pid") != pid or decoded.get("revision") != revision:
        errors.append("payload identity does not match selected death record")
    if decoded["death"].get("operation_id_hex") != death["operation_id_hex"]:
        errors.append("payload operation does not match death record")
    for field in ("corpse_room_vnum", "wallet_revision", "wallet_pile_uid"):
        if decoded["death"].get(field) != death[field]:
            errors.append("payload death metadata does not match SQL row")
    if decoded["death"].get("wallet_before") != death["wallet_before"]:
        errors.append("payload wallet evidence does not match SQL row")
    if decoded["death"]["corpse"][0]["object_uid"] != death["corpse_item_uid"]:
        errors.append("payload corpse identity does not match SQL row")

    custody = fetch_custody(db, pid, revision)
    errors.extend(compare_custody(decoded, custody, allow_custody_only=True))
    payload_items = decoded["death"]["corpse"]
    all_uids = [item["object_uid"] for item in payload_items]
    all_uids.extend(row["item_uid"] for row in custody)
    owners = fetch_current_owners(db, all_uids)
    projections = fetch_player_projections(db, all_uids)
    recipient_uids = fetch_recipient_uids(db, recipient_pid)
    related = fetch_related_deaths(db, pid, revision, all_uids, decode_payload)
    related_pairs = related_death_pairs(db, pid, revision, all_uids)
    related_custody_by_pair = fetch_custody_pairs(db, related_pairs)
    related_custody = [
        row
        for pair in sorted(related_custody_by_pair)
        for row in related_custody_by_pair[pair]
    ]
    item_loss_epochs: dict[str, int] = {}
    for item in payload_items[1:]:
        item_loss_epochs[str(item["object_uid"])] = death["loss_epoch"]
    for related_death in related:
        related_loss_epoch = related_death.get("loss_epoch")
        if related_loss_epoch is None:
            continue
        for item in related_death.get("payload_items", []):
            uid_text = str(item["object_uid"])
            prior_loss_epoch = item_loss_epochs.get(uid_text)
            if prior_loss_epoch is not None and prior_loss_epoch != related_loss_epoch:
                errors.append("same UID has conflicting death-loss timestamps")
            else:
                item_loss_epochs[uid_text] = related_loss_epoch
    vnums = {item["vnum"] for item in payload_items[1:]}
    for related_death in related:
        for item in related_death.get("payload_items", []):
            vnums.add(item["vnum"])
    artifacts = fetch_artifacts(db, vnums)
    deliveries = fetch_deliveries(db, all_uids)

    body: dict[str, Any] = {
        "artifact_version": TOOL_VERSION,
        "kind": "death_restitution_inspection",
        "backend": "sql",
        "source": {
            "pid": pid,
            "death_revision": revision,
            "operation_id_hex": death["operation_id_hex"],
            "corpse_item_uid": death["corpse_item_uid"],
            "corpse_room_vnum": death["corpse_room_vnum"],
            "wallet_revision": death["wallet_revision"],
            "wallet_before": death["wallet_before"],
            "wallet_pile_uid": death["wallet_pile_uid"],
            "loss_epoch": death["loss_epoch"],
        },
        "recipient_pid": recipient_pid,
        "payload_hex": death["payload_hex"],
        "payload_digest": death["payload_digest"],
        "decoded": decoded,
        "custody_db": custody,
        "current_owners": owners,
        "player_projections": projections,
        "recipient_existing_uids": sorted(set(recipient_uids)),
        "related_deaths": related,
        "related_custody": related_custody,
        "item_loss_epochs": item_loss_epochs,
        "deliveries": deliveries,
        "artifacts": artifacts,
        "consistency_errors": sorted(set(errors)),
    }
    if target is not None:
        body["target"] = target_record(target, label="inspection target")
    body["evidence_digest"] = digest_json(body)
    return body


def load_inspection(path: Path) -> dict[str, Any]:
    value = read_protected_json(path)
    if value.get("kind") != "death_restitution_inspection" or value.get("artifact_version") != TOOL_VERSION:
        raise ToolError("not a supported restitution inspection artifact")
    verify_artifact_digest(value, "evidence_digest")
    return value


def payload_evidence_maps(inspection: dict[str, Any]) -> tuple[
        dict[str, dict[str, Any]], dict[str, int], dict[str, int], list[str], set[str]]:
    selected_items = inspection["decoded"]["death"]["corpse"]
    normalized, _, _, _, selected_order = normalize_payload_items(selected_items)
    item_by_uid: dict[str, dict[str, Any]] = {}
    payload_parent: dict[str, int] = {}
    payload_root: dict[str, int] = {}
    payload_order: list[str] = []
    fingerprints: dict[str, str] = {}
    conflicts: set[str] = set()

    def add_evidence(items: Any, order: Any, normalized_digest: Any = None) -> None:
        if items is None and order is None:
            raise ToolError("related death payload evidence is missing")
        if not isinstance(items, list) or not isinstance(order, list):
            raise ToolError("related death payload evidence has an invalid shape")
        if len(items) != len(order) or len(set(str(uid) for uid in order)) != len(order):
            raise ToolError("related death payload evidence has an invalid order")
        if normalized_digest is not None and normalized_digest != digest_json(items):
            raise ToolError("related death payload normalized digest does not match")
        evidence_uids = {str(int_value(item.get("object_uid"), "payload item UID", 1, 2**64 - 1))
                         for item in items if isinstance(item, dict)}
        if len(evidence_uids) != len(items):
            raise ToolError("related death payload evidence has duplicate or invalid items")
        for item, ordered_uid in zip(items, order):
            if not isinstance(item, dict):
                raise ToolError("related death payload evidence has an invalid item")
            uid = int_value(item.get("object_uid"), "payload item UID", 1, 2**64 - 1)
            uid_text = str(uid)
            if str(ordered_uid) != uid_text:
                raise ToolError("related death payload evidence identity is inconsistent")
            parent_uid = int_value(item.get("parent_item_uid"), "payload parent UID", 0, 2**64 - 1)
            root_uid = int_value(item.get("root_item_uid"), "payload root UID", 1, 2**64 - 1)
            if (parent_uid == 0 and root_uid != uid) or (
                    parent_uid != 0 and str(parent_uid) not in evidence_uids
            ) or str(root_uid) not in evidence_uids:
                raise ToolError("related death payload evidence ancestry is inconsistent")
            fingerprint = digest_json(item)
            if uid_text in fingerprints:
                if fingerprints[uid_text] != fingerprint:
                    conflicts.add(uid_text)
                continue
            fingerprints[uid_text] = fingerprint
            item_by_uid[uid_text] = item
            payload_parent[uid_text] = parent_uid
            payload_root[uid_text] = root_uid
            payload_order.append(uid_text)

    add_evidence(normalized, selected_order, digest_json(normalized))
    for related in inspection.get("related_deaths", []):
        if not isinstance(related, dict):
            raise ToolError("related death evidence has an invalid record")
        if related.get("identity_error"):
            payload_uids = related.get("payload_uids")
            if not isinstance(payload_uids, list):
                raise ToolError("identity-mismatched related death has no UID evidence")
            conflicts.update(
                str(int_value(uid, "related payload item UID", 1, 2**64 - 1))
                for uid in payload_uids
            )
            continue
        if related.get("decode_error"):
            continue
        add_evidence(
            related.get("payload_items"),
            related.get("payload_order"),
            related.get("normalized_payload_digest"),
        )
    return item_by_uid, payload_parent, payload_root, payload_order, conflicts


def payload_item_maps(inspection: dict[str, Any]) -> tuple[dict[str, dict[str, Any]], dict[str, dict[str, Any]],
                                                             dict[str, int], dict[str, int]]:
    item_by_uid, payload_parent, payload_root, _, _ = payload_evidence_maps(inspection)
    custody_by_uid = {str(row["item_uid"]): row for row in inspection["custody_db"]}
    return item_by_uid, custody_by_uid, payload_parent, payload_root


def normalize_artifact_timing_compensations(
    value: Mapping[Any, Any] | None,
) -> dict[str, dict[str, Any]]:
    if value is None:
        return {}
    if not isinstance(value, Mapping):
        raise ToolError("artifact timing compensations must be a UID-keyed mapping")
    result: dict[str, dict[str, Any]] = {}
    for raw_uid, raw_compensation in value.items():
        uid = int_value(raw_uid, "artifact compensation UID", 1, 2**64 - 1)
        if not isinstance(raw_compensation, Mapping):
            raise ToolError("artifact compensation must contain seconds and approval")
        seconds = int_value(
            raw_compensation.get("seconds"),
            "artifact compensation lifetime",
            1,
            MAX_ARTIFACT_COMPENSATION_SECONDS,
        )
        approval = raw_compensation.get("approval")
        if not isinstance(approval, str) or not re.fullmatch(
            r"[A-Za-z0-9_.:/()\[\], -]{1,255}", approval.strip()
        ):
            raise ToolError("artifact timing compensation requires a non-secret approval reference")
        uid_text = str(uid)
        if uid_text in result:
            raise ToolError("duplicate artifact timing compensation UID")
        result[uid_text] = {"seconds": seconds, "approval": approval.strip()}
    return result


def parse_artifact_timing_compensation_specs(
    specs: Iterable[str],
) -> dict[str, dict[str, Any]]:
    parsed: dict[str, dict[str, Any]] = {}
    for spec in specs:
        if not isinstance(spec, str) or "=" not in spec or ":" not in spec:
            raise ToolError("artifact timing compensation must be UID=SECONDS:APPROVAL")
        uid_text, remainder = spec.split("=", 1)
        seconds_text, approval = remainder.split(":", 1)
        uid = int_value(uid_text, "artifact compensation UID", 1, 2**64 - 1)
        if str(uid) in parsed:
            raise ToolError("duplicate artifact timing compensation UID")
        parsed[str(uid)] = {"seconds": seconds_text, "approval": approval}
    return normalize_artifact_timing_compensations(parsed)


def plan_from_inspection(
    inspection: dict[str, Any], approve_artifact_reconciliation: bool = False,
    *, target_info: Mapping[str, Any] | None = None,
    backup_receipt: Mapping[str, Any] | None = None,
    approve_production: bool = False,
    artifact_timing_compensations: Mapping[Any, Any] | None = None,
) -> dict[str, Any]:
    inspection_target = inspection.get("target")
    target: dict[str, Any] | None = None
    if inspection_target is not None:
        target = target_record(inspection_target, label="inspection target")
    if target_info is not None:
        info_target = target_record(target_info.get("target"), label="target-info target")
        if target is not None and target != info_target:
            raise ToolError("target-info does not match the inspection target")
        target = info_target
    production = bool(target and target["production"])
    if approve_production and not production:
        raise ToolError("production approval requires a production-classified target")
    timing_compensations = normalize_artifact_timing_compensations(artifact_timing_compensations)
    boundary: dict[str, str] | None = None
    if target_info is not None and target_info.get("maintenance_boundary") is not None:
        boundary = maintenance_record(
            target_info["maintenance_boundary"], label="target-info maintenance boundary"
        )
    receipt: dict[str, Any] | None = None
    if backup_receipt is not None:
        receipt = dict(backup_receipt)
        if receipt.get("format") != BACKUP_RECEIPT_FORMAT:
            raise ToolError("backup receipt has an unsupported format")
        if target is None:
            raise ToolError("a backup receipt requires a target-pinned inspection")
        receipt_target = target_record(receipt.get("target"), label="backup receipt target")
        if receipt_target != target:
            raise ToolError("backup receipt does not identify the inspection target")
        receipt_boundary = maintenance_record(
            receipt.get("maintenance_boundary"), label="backup receipt maintenance boundary"
        )
        if boundary is not None and boundary != receipt_boundary:
            raise ToolError("backup receipt and target-info use different maintenance boundaries")
        boundary = receipt_boundary
        try:
            verify_backup_receipt(receipt, target, boundary)
        except TargetError as exc:
            raise ToolError(str(exc)) from exc
    if production:
        if target_info is None:
            raise ToolError("production planning requires a protected target-info artifact")
        if receipt is None:
            raise ToolError("production planning requires a protected native backup receipt")
        if boundary is None:
            raise ToolError("production planning requires an approved maintenance boundary")
    elif receipt is not None and target is not None:
        # A non-production backup is still checked when supplied, but it does
        # not turn the normal development plan into a production approval.
        try:
            verify_backup_receipt(receipt, target, boundary)
        except TargetError as exc:
            raise ToolError(str(exc)) from exc
    source = inspection["source"]
    pid = int_value(source["pid"], "source pid", 1, 2**31 - 1)
    revision = int_value(source["death_revision"], "death revision", 1, 2**64 - 1)
    recipient = int_value(inspection["recipient_pid"], "recipient pid", 1, 2**31 - 1)
    item_by_uid, payload_parent, payload_root, evidence_order, evidence_conflicts = payload_evidence_maps(inspection)
    custody_by_uid = {str(row["item_uid"]): row for row in inspection["custody_db"]}
    owners = inspection["current_owners"]
    projections_by_uid: dict[str, list[dict[str, Any]]] = {}
    for projection in inspection["player_projections"]:
        projections_by_uid.setdefault(str(projection["item_uid"]), []).append(projection)
    deliveries = inspection["deliveries"]
    artifacts = inspection["artifacts"]
    wallet_uid = int_value(inspection["decoded"]["death"]["wallet_pile_uid"], "wallet UID", 0)
    corpse_uid = int_value(inspection["decoded"]["death"]["corpse"][0]["object_uid"], "corpse UID", 1)

    selected_payload_order = [
        str(item["object_uid"]) for item in inspection["decoded"]["death"]["corpse"][1:]
    ]
    candidate_uids = set(selected_payload_order) | set(custody_by_uid)
    payload_order = [uid for uid in evidence_order if uid in candidate_uids]
    payload_order.extend(uid for uid in selected_payload_order if uid not in payload_order)
    custody_only = sorted(set(custody_by_uid) - set(payload_order), key=int)
    ordered_uids = payload_order + custody_only
    plans: list[dict[str, Any]] = []
    used_timing_compensations: set[str] = set()
    global_errors = list(inspection.get("consistency_errors", []))
    for uid_text in ordered_uids:
        item = item_by_uid.get(uid_text)
        custody = custody_by_uid.get(uid_text)
        owner = owners.get(uid_text)
        projections = projections_by_uid.get(uid_text, [])
        candidate: dict[str, Any] = {
            "item_uid": int(uid_text),
            "eligible": False,
            "classification": "missing_payload" if item is None else "evidence_inconsistent",
            "disposition": DISPOSITION["missing_payload"] if item is None else DISPOSITION["evidence_inconsistent"],
            "kind": "unknown" if item is None else item_kind(item, artifacts),
            "source_root_item_uid": custody["root_item_uid"] if custody else 0,
            "source_parent_item_uid": custody["parent_item_uid"] if custody else 0,
            "delivered_root_item_uid": 0,
            "delivered_parent_item_uid": 0,
            "source_item_revision": 0,
            "custody_expected_item_revision": 0,
            "custody_expected_state": 0,
            "custody_owner_type": 0,
            "custody_owner_id": 0,
            "custody_owner_context_id": 0,
            "custody_owner_revision": 0,
            "expected_current": None,
            "note": "",
        }
        if item is not None:
            candidate["vnum"] = item["vnum"]
            candidate["metadata"] = item
            candidate["metadata_digest"] = digest_bytes(hex_bytes(item["item_payload_hex"], "item payload"))
            candidate["metadata_payload_hex"] = item["item_payload_hex"]
        if custody is None:
            if item is not None:
                candidate["classification"] = "payload_without_custody"
                candidate["disposition"] = DISPOSITION["payload_without_custody"]
                candidate["note"] = "serialized payload has no matching custody authority"
            plans.append(candidate)
            continue
        candidate["source_item_revision"] = custody["expected_item_revision"]
        candidate["custody_expected_item_revision"] = custody["expected_item_revision"]
        candidate["custody_expected_state"] = custody["expected_state"]
        candidate["custody_owner_type"] = custody["owner_type"]
        candidate["custody_owner_id"] = custody["owner_id"]
        candidate["custody_owner_context_id"] = custody["owner_context_id"]
        candidate["custody_owner_revision"] = custody["owner_revision"]
        candidate["source_root_item_uid"] = custody["root_item_uid"]
        candidate["source_parent_item_uid"] = custody["parent_item_uid"]
        if item is None:
            candidate["classification"] = "missing_payload"
            candidate["disposition"] = DISPOSITION["missing_payload"]
            candidate["note"] = "custody is retained but no reconstructable payload exists"
            plans.append(candidate)
            continue
        if item["object_uid"] == corpse_uid:
            candidate["classification"] = "evidence_inconsistent"
            candidate["disposition"] = DISPOSITION["evidence_inconsistent"]
            candidate["note"] = "corpse lifecycle identity is not an inventory entitlement"
            plans.append(candidate)
            continue
        if item["object_uid"] == wallet_uid or candidate["kind"] == "currency":
            candidate["classification"] = "currency_refused"
            candidate["disposition"] = DISPOSITION["currency_refused"]
            candidate["note"] = "currency restitution is excluded; reconcile wallet authority separately"
            plans.append(candidate)
            continue
        if custody["vnum"] != item["vnum"]:
            candidate["classification"] = "evidence_inconsistent"
            candidate["disposition"] = DISPOSITION["evidence_inconsistent"]
            candidate["note"] = "custody and payload vnums differ"
            plans.append(candidate)
            continue
        if custody["expected_state"] == ITEM_STATE_ABSENT:
            candidate["classification"] = "custody_absent"
            candidate["disposition"] = DISPOSITION["custody_absent"]
            candidate["note"] = "custody explicitly records an absent identity"
            plans.append(candidate)
            continue
        if uid_text in evidence_conflicts:
            candidate["classification"] = "cross_death_payload_conflict"
            candidate["disposition"] = DISPOSITION["cross_death_payload_conflict"]
            candidate["note"] = "same physical UID has conflicting normalized payload evidence"
            plans.append(candidate)
            continue
        if uid_text in deliveries:
            candidate["classification"] = "already_delivered"
            candidate["disposition"] = DISPOSITION["already_delivered"]
            candidate["note"] = "global UID delivery receipt already exists"
            plans.append(candidate)
            continue
        if recipient != pid:
            candidate["classification"] = "recipient_mismatch"
            candidate["disposition"] = DISPOSITION["recipient_mismatch"]
            candidate["note"] = "SQL first slice only restores to the source character"
            plans.append(candidate)
            continue
        if owner is None:
            candidate["classification"] = "owner_missing"
            candidate["disposition"] = DISPOSITION["owner_missing"]
            candidate["note"] = "current ownership authority is absent"
            plans.append(candidate)
            continue
        if projections:
            candidate["classification"] = "projection_without_authority"
            candidate["disposition"] = DISPOSITION["projection_without_authority"]
            candidate["note"] = "a player projection already uses the UID before repair"
            plans.append(candidate)
            continue
        if owner["owner_type"] != OWNER_PLAYER or owner["owner_id"] != pid or owner["owner_context_id"] != 0:
            candidate["classification"] = "owner_conflict"
            candidate["disposition"] = DISPOSITION["owner_conflict"]
            candidate["note"] = "current owner is not the quarantined source player"
            plans.append(candidate)
            continue
        if owner["state"] == STATE_DESTROYED:
            candidate["classification"] = "destroyed"
            candidate["disposition"] = DISPOSITION["destroyed"]
            candidate["note"] = "ownership authority records destruction"
            plans.append(candidate)
            continue
        if owner["state"] != STATE_QUARANTINED:
            candidate["classification"] = "legal_transfer_or_destroyed"
            candidate["disposition"] = DISPOSITION["legal_transfer_or_destroyed"]
            candidate["note"] = "current ownership is not quarantined"
            plans.append(candidate)
            continue
        if owner["vnum"] != item["vnum"]:
            candidate["classification"] = "owner_vnum_conflict"
            candidate["disposition"] = DISPOSITION["owner_vnum_conflict"]
            candidate["note"] = "current ownership vnum differs from evidence"
            plans.append(candidate)
            continue
        expected_current_revision = custody["expected_item_revision"] + (
            1 if custody["expected_state"] == STATE_ACTIVE else 0
        )
        expected_owner_revision = custody["owner_revision"] + (
            1 if custody["expected_state"] == STATE_ACTIVE else 0
        )
        candidate["expected_current"] = {
            "root_item_uid": owner["root_item_uid"],
            "parent_item_uid": owner["parent_item_uid"],
            "item_revision": owner["item_revision"],
            "state": owner["state"],
            "owner_revision": owner["owner_revision"],
        }
        candidate["source_item_revision"] = owner["item_revision"]
        if owner["item_revision"] != expected_current_revision or owner["owner_revision"] != expected_owner_revision:
            candidate["classification"] = "stale_owner_revision"
            candidate["disposition"] = DISPOSITION["stale_owner_revision"]
            candidate["note"] = "current ownership revision is not the death quarantine revision"
            plans.append(candidate)
            continue
        if owner["root_item_uid"] != custody["root_item_uid"]:
            candidate["classification"] = "ambiguous_topology"
            candidate["disposition"] = DISPOSITION["ambiguous_topology"]
            candidate["note"] = "current root differs from the retained custody root"
            plans.append(candidate)
            continue
        if str(item["object_uid"]) not in payload_parent or str(item["object_uid"]) not in payload_root:
            candidate["classification"] = "ambiguous_topology"
            candidate["disposition"] = DISPOSITION["ambiguous_topology"]
            candidate["note"] = "payload ancestry cannot be reconstructed"
            plans.append(candidate)
            continue
        delivered_parent = payload_parent[str(item["object_uid"])]
        delivered_root = payload_root[str(item["object_uid"])]
        if delivered_root != custody["root_item_uid"]:
            candidate["classification"] = "ambiguous_topology"
            candidate["disposition"] = DISPOSITION["ambiguous_topology"]
            candidate["note"] = "payload root differs from custody root"
            plans.append(candidate)
            continue
        if delivered_parent == 0:
            if custody["parent_item_uid"] != 0 or owner["parent_item_uid"] != 0:
                candidate["classification"] = "ambiguous_topology"
                candidate["disposition"] = DISPOSITION["ambiguous_topology"]
                candidate["note"] = "a payload root has a non-root retained parent"
                plans.append(candidate)
                continue
            topology_reconciled = False
        else:
            def same_payload_root(parent_uid: int) -> bool:
                return parent_uid != 0 and (
                    parent_uid == delivered_root or payload_root.get(str(parent_uid)) == delivered_root
                )

            if not same_payload_root(custody["parent_item_uid"]) or not same_payload_root(owner["parent_item_uid"]):
                candidate["classification"] = "ambiguous_topology"
                candidate["disposition"] = DISPOSITION["ambiguous_topology"]
                candidate["note"] = "retained and live parents are not in the captured payload tree"
                plans.append(candidate)
                continue
            topology_reconciled = (
                custody["parent_item_uid"] != delivered_parent or owner["parent_item_uid"] != delivered_parent
            )
        if candidate["kind"] == "artifact":
            reconciliation = artifact_reconciliation(
                item, pid, artifacts, owner,
                inspection.get("item_loss_epochs", {}).get(uid_text, inspection["source"].get("loss_epoch"))
            )
            if not reconciliation["ok"]:
                candidate["classification"] = reconciliation["classification"]
                candidate["disposition"] = DISPOSITION[reconciliation["classification"]]
                candidate["note"] = reconciliation["note"]
                plans.append(candidate)
                continue
            candidate["artifact_reconciliation"] = reconciliation
            candidate["artifact_before"] = reconciliation.get("domain_before")
            candidate["artifact_bind_before"] = reconciliation.get("bind_before")
            candidate["artifact_legacy_before"] = reconciliation.get("legacy_before", {})
            if reconciliation.get("reconciliation_required"):
                candidate["artifact_reconciliation_required"] = True
                candidate["artifact_reconciliation_mode"] = reconciliation["mode"]
                candidate["artifact_legacy_unknown"] = reconciliation["legacy_unknown"]
            else:
                candidate["artifact_reconciliation_required"] = False
            timing_status = reconciliation.get("timing_status")
            if timing_status == "historical":
                timing_basis = "historical_loss_remainder"
                usable_lifetime = int_value(
                    reconciliation.get("usable_lifetime_seconds"),
                    "artifact usable lifetime",
                    1,
                    MAX_ARTIFACT_COMPENSATION_SECONDS,
                )
                compensation_reference = ""
            elif uid_text in timing_compensations:
                compensation = timing_compensations[uid_text]
                timing_basis = "approved_compensation"
                usable_lifetime = int_value(
                    compensation["seconds"],
                    "artifact compensation lifetime",
                    1,
                    MAX_ARTIFACT_COMPENSATION_SECONDS,
                )
                compensation_reference = compensation["approval"]
                used_timing_compensations.add(uid_text)
            else:
                classification = (
                    "artifact_timing_expired_at_loss"
                    if timing_status == "expired_at_loss" else "artifact_timing_missing"
                )
                candidate["classification"] = classification
                candidate["disposition"] = DISPOSITION[classification]
                candidate["note"] = (
                    "absolute artifact poof epoch had no usable lifetime at loss; explicit approved compensation is required"
                    if timing_status == "expired_at_loss" else
                    "historical loss or absolute poof timing evidence is missing; explicit approved compensation is required"
                )
                plans.append(candidate)
                continue
            candidate["artifact_timing"] = {
                "loss_epoch": int_value(reconciliation.get("loss_epoch"), "artifact loss epoch", 0),
                "source_timer_epoch": int_value(
                    reconciliation.get("source_timer_epoch"), "artifact source timer epoch", 0
                ),
                "usable_lifetime_seconds": usable_lifetime,
                "basis": timing_basis,
                "compensation_reference": compensation_reference,
            }
        candidate["delivered_root_item_uid"] = delivered_root
        candidate["delivered_parent_item_uid"] = delivered_parent
        candidate["destination"] = "player_inventory"
        candidate["equipment_slot"] = 0
        candidate["eligible"] = True
        if candidate.get("artifact_reconciliation_required"):
            candidate["classification"] = "recoverable_artifact_reconciled"
            candidate["note"] = candidate["artifact_reconciliation"]["note"]
        else:
            candidate["classification"] = (
                "recoverable_topology_reconciled" if topology_reconciled else "recoverable_exact"
            )
            candidate["note"] = (
                "payload topology is used explicitly; retained custody parent is preserved as evidence"
                if topology_reconciled else "original UID and payload-backed state are recoverable"
            )
        candidate["disposition"] = DISPOSITION[candidate["classification"]]
        if candidate.get("kind") == "artifact":
            candidate["artifact_after"] = final_domain_expectation(candidate, recipient)
        plans.append(candidate)

    # A child is never delivered without its proven parent. This is per-tree fail
    # closed, while independent exact roots can still be delivered alongside
    # missing-payload custody rows.
    changed = True
    while changed:
        changed = False
        eligible = {str(row["item_uid"]) for row in plans if row.get("eligible")}
        for row in plans:
            if not row.get("eligible") or not row.get("delivered_parent_item_uid"):
                continue
            parent_uid = str(row["delivered_parent_item_uid"])
            if parent_uid not in eligible:
                row["eligible"] = False
                row["classification"] = "parent_not_recoverable"
                row["disposition"] = DISPOSITION["parent_not_recoverable"]
                row["note"] = "a nested item cannot be delivered without its recoverable container"
                changed = True

    blocking = {
        "evidence_inconsistent", "owner_conflict", "owner_missing", "projection_without_authority",
        "stale_owner_revision", "ambiguous_topology", "payload_without_custody", "owner_vnum_conflict",
        "recipient_mismatch", "parent_not_recoverable", "artifact_authority_missing",
        "artifact_identity_unbound", "artifact_binding_conflict", "cross_death_payload_conflict",
        "artifact_legacy_conflict", "artifact_competing_instance", "artifact_baseline_missing",
        "artifact_timing_missing", "artifact_timing_expired_at_loss",
    }
    eligible_count = sum(1 for row in plans if row.get("eligible"))
    refusal_counts: dict[str, int] = {}
    for row in plans:
        if not row.get("eligible"):
            refusal_counts[row["classification"]] = refusal_counts.get(row["classification"], 0) + 1
    reconciliation_items = [
        row for row in plans if row.get("eligible") and row.get("artifact_reconciliation_required")
    ]
    timing_compensation_items = [
        row for row in plans
        if row.get("eligible") and row.get("artifact_timing", {}).get("basis") == "approved_compensation"
    ]
    used_timing_compensations = {str(row["item_uid"]) for row in timing_compensation_items}
    unused_timing_compensations = set(timing_compensations) - used_timing_compensations
    if unused_timing_compensations:
        raise ToolError(
            "artifact timing compensation supplied for a UID without missing/expired timing: " +
            ",".join(sorted(unused_timing_compensations, key=int))
        )
    body: dict[str, Any] = {
        "artifact_version": TOOL_VERSION,
        "kind": "death_restitution_plan",
        "backend": "sql",
        "source": source,
        "recipient_pid": recipient,
        "destination": "player_inventory",
        "evidence_digest": inspection["evidence_digest"],
        "payload_digest": inspection["payload_digest"],
        "items": plans,
        "candidate_count": len(plans),
        "eligible_count": eligible_count,
        "unresolved_count": len(plans) - eligible_count,
        "refusal_counts": refusal_counts,
        "blocking_classifications_present": sorted(set(row["classification"] for row in plans) & blocking),
        "artifact_reconciliation_uids": [row["item_uid"] for row in reconciliation_items],
        "artifact_reconciliation_count": len(reconciliation_items),
        "artifact_reconciliation_approved": bool(approve_artifact_reconciliation),
        "artifact_timing_compensation_count": len(timing_compensation_items),
        "artifact_timing_compensations": (
            {uid: timing_compensations[uid] for uid in sorted(used_timing_compensations, key=int)}
            if timing_compensation_items else {}
        ),
        "artifact_timing_compensation_approved": bool(timing_compensation_items),
        "production_approval_required": production,
        "production_approved": bool(approve_production),
        "applyable": bool(
            eligible_count and not global_errors and recipient == pid and
            (not reconciliation_items or approve_artifact_reconciliation) and
            (not production or (approve_production and receipt is not None and boundary is not None))
        ),
        "consistency_errors": global_errors,
        "recipient_existing_uids": inspection.get("recipient_existing_uids", []),
        "custody_evidence": inspection.get("custody_db", []),
        "related_custody": inspection.get("related_custody", []),
        "related_deaths": inspection.get("related_deaths", []),
        "item_loss_epochs": inspection.get("item_loss_epochs", {}),
    }
    if target is not None:
        body["target"] = target
    if boundary is not None:
        body["maintenance_boundary"] = boundary
    if receipt is not None:
        body["backup_receipt"] = receipt
    if reconciliation_items and approve_artifact_reconciliation:
        body["artifact_reconciliation_approval"] = (
            "explicit operator approval recorded at plan creation; apply must repeat --approve-artifact-reconciliation"
        )
    if timing_compensation_items:
        body["artifact_timing_compensation_approval_record"] = (
            "explicit approved compensation; apply must repeat --approve-artifact-timing-compensation"
        )
    body["plan_digest"] = digest_json(body)
    material = (
        b"duris-player-death-restitution-v1\0" + hex_bytes(body["evidence_digest"], "evidence digest") +
        recipient.to_bytes(8, "little") + hex_bytes(body["plan_digest"], "plan digest")
    )
    body["restitution_id_hex"] = hashlib.sha256(material).digest()[:16].hex()
    return body


def load_plan(path: Path) -> dict[str, Any]:
    value = read_protected_json(path)
    if value.get("kind") != "death_restitution_plan" or value.get("artifact_version") != TOOL_VERSION:
        raise ToolError("not a supported restitution plan artifact")
    supplied_plan_digest = value.get("plan_digest")
    if not isinstance(supplied_plan_digest, str) or not re.fullmatch(r"[0-9a-f]{64}", supplied_plan_digest):
        raise ToolError("protected artifact has no valid plan_digest")
    digest_body = dict(value)
    del digest_body["plan_digest"]
    digest_body.pop("restitution_id_hex", None)
    if digest_json(digest_body) != supplied_plan_digest:
        raise ToolError("protected artifact plan_digest does not match its contents")
    evidence = value.get("evidence_digest")
    if not isinstance(evidence, str) or not re.fullmatch(r"[0-9a-f]{64}", evidence):
        raise ToolError("plan has no valid evidence digest")
    rid = value.get("restitution_id_hex")
    if not isinstance(rid, str) or not re.fullmatch(r"[0-9a-f]{32}", rid):
        raise ToolError("plan has no valid restitution identity")
    material = (
        b"duris-player-death-restitution-v1\0" + hex_bytes(evidence, "evidence digest") +
        int_value(value["recipient_pid"], "recipient pid", 1, 2**31 - 1).to_bytes(8, "little") +
        hex_bytes(supplied_plan_digest, "plan digest")
    )
    if hashlib.sha256(material).digest()[:16].hex() != rid:
        raise ToolError("protected artifact restitution identity does not match its plan")
    validate_artifact_timing_compensations(value)
    plan_target = value.get("target")
    if plan_target is not None:
        plan_target = target_record(plan_target, label="plan target")
        if value.get("production_approval_required") is not plan_target["production"]:
            raise ToolError("plan production classification is inconsistent with its target")
        if plan_target["production"]:
            boundary = maintenance_record(
                value.get("maintenance_boundary"), label="plan maintenance boundary"
            )
            receipt = value.get("backup_receipt")
            if not isinstance(receipt, dict):
                raise ToolError("production plan has no approved native backup receipt")
            if receipt.get("target") != plan_target or receipt.get("maintenance_boundary") != boundary:
                raise ToolError("production plan backup is not bound to its target and boundary")
            try:
                verify_backup_receipt(receipt, plan_target, boundary)
            except TargetError as exc:
                raise ToolError(str(exc)) from exc
            if type(value.get("production_approved")) is not bool:
                raise ToolError("production plan approval is invalid")
        elif value.get("production_approved") is True:
            raise ToolError("non-production plan cannot carry production approval")
    return value


def validate_artifact_timing_compensations(plan: Mapping[str, Any]) -> None:
    raw = plan.get("artifact_timing_compensations", {})
    compensations = normalize_artifact_timing_compensations(raw)
    count = int_value(plan.get("artifact_timing_compensation_count", 0),
                      "artifact timing compensation count", 0)
    if count != len(compensations):
        raise ToolError("artifact timing compensation count does not match its UID map")
    items = plan.get("items")
    if not isinstance(items, list):
        raise ToolError("plan has no item candidates")
    approved_uids: set[str] = set()
    for item in items:
        if not isinstance(item, dict) or not item.get("eligible") or item.get("kind") != "artifact":
            continue
        uid = str(int_value(item.get("item_uid"), "planned artifact UID", 1, 2**64 - 1))
        timing = item.get("artifact_timing")
        if not isinstance(timing, dict):
            continue
        if timing.get("basis") != "approved_compensation":
            continue
        approved_uids.add(uid)
        compensation = compensations.get(uid)
        if compensation is None:
            raise ToolError("approved artifact timing is missing its UID compensation")
        if (
            compensation["seconds"] != int_value(
                timing.get("usable_lifetime_seconds"), "artifact usable lifetime", 1,
                MAX_ARTIFACT_COMPENSATION_SECONDS,
            ) or compensation["approval"] != timing.get("compensation_reference")
        ):
            raise ToolError("artifact timing compensation does not match the planned UID")
    if approved_uids != set(compensations):
        raise ToolError("artifact timing compensation UID map does not match planned artifacts")


# ---------- quiescence and apply ----------


def parse_expiry(value: str) -> float:
    try:
        parsed = dt.datetime.fromisoformat(value.replace("Z", "+00:00"))
    except ValueError as exc:
        raise ToolError("offline proof expiry is invalid") from exc
    if parsed.tzinfo is None:
        raise ToolError("offline proof expiry must include UTC timezone")
    return parsed.timestamp()


def find_trusted_command(name: str) -> str | None:
    command = shutil.which(name)
    if not command:
        return None
    try:
        path = Path(command).resolve(strict=True)
        info = path.stat()
    except (OSError, RuntimeError):
        return None
    if not stat.S_ISREG(info.st_mode) or info.st_uid != 0 or info.st_mode & 0o022:
        return None
    return str(path)


def run_quiescence_command(command: list[str], label: str) -> str:
    try:
        result = subprocess.run(
            command,
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
            text=True,
            timeout=10,
            check=False,
        )
    except (OSError, subprocess.SubprocessError) as exc:
        raise ToolError(f"offline boundary dependency could not run: {label}") from exc
    if result.returncode:
        raise ToolError(f"offline boundary check failed: {label}")
    return result.stdout


def check_process_inventory() -> None:
    ps = find_trusted_command("ps")
    if not ps:
        raise ToolError("offline boundary requires ps for live process inventory")
    output = run_quiescence_command(
        [ps, "-eo", "pid=,args="],
        "live process inventory",
    )
    runtime_binaries = {
        (ROOT / "bin" / "server" / "dms").resolve(),
        (ROOT / "bin" / "server" / "dms_new").resolve(),
    }
    managed_scripts = {
        str(ROOT / "scripts" / "cycle_mud.sh"),
        str(ROOT / "scripts" / "start_mud.sh"),
    }
    for line in output.splitlines():
        fields = line.strip().split(None, 1)
        if len(fields) != 2:
            continue
        try:
            pid = int(fields[0])
        except ValueError:
            continue
        if pid == os.getpid():
            continue
        arguments = fields[1]
        executable: Path | None = None
        try:
            executable = Path(f"/proc/{pid}/exe").resolve()
        except (OSError, RuntimeError):
            pass
        cwd: Path | None = None
        try:
            cwd = Path(f"/proc/{pid}/cwd").resolve()
        except (OSError, RuntimeError):
            pass
        argument_tokens = arguments.split()
        relative_managed_script = cwd == ROOT and any(
            Path(token).name == Path(script).name
            for token in argument_tokens
            for script in managed_scripts
        )
        if (
            executable in runtime_binaries
            or any(script in arguments for script in managed_scripts)
            or relative_managed_script
        ):
            raise ToolError("Duris runtime process is still present")


def database_visibility_sql() -> str:
    # Require a direct global grant. Inherited-role-only privileges are refused
    # rather than guessing role expansion across MySQL and MariaDB versions.
    return (
        "SELECT COUNT(*) FROM information_schema.USER_PRIVILEGES "
        "WHERE PRIVILEGE_TYPE='PROCESS' AND GRANTEE=CONCAT(QUOTE(SUBSTRING(CURRENT_USER(),1,"
        "CHAR_LENGTH(CURRENT_USER())-CHAR_LENGTH(SUBSTRING_INDEX(CURRENT_USER(),'@',-1))-1)),"
        "'@',QUOTE(SUBSTRING_INDEX(CURRENT_USER(),'@',-1)))"
    )


def check_database_quiescence(db: Mysql) -> None:
    if db.scalar(database_visibility_sql()) != "1":
        raise ToolError("recovery account requires a direct global PROCESS grant for session/transaction visibility")
    for sample in range(2):
        other_connections = int_value(
            db.scalar(
                "SELECT COUNT(*) FROM information_schema.processlist "
                "WHERE ID<>CONNECTION_ID()"
            ),
            "other database connection count",
            0,
        )
        if other_connections:
            raise ToolError("other database sessions are present; refusing restitution")
        active_writers = int_value(
            db.scalar(
                "SELECT COUNT(*) FROM information_schema.processlist "
                "WHERE ID<>CONNECTION_ID() AND COMMAND<>'Sleep'"
            ),
            "active database writer count",
            0,
        )
        if active_writers:
            raise ToolError("active database writers are present; refusing restitution")
        open_transactions = int_value(
            db.scalar(
                "SELECT COUNT(*) FROM information_schema.innodb_trx "
                "WHERE trx_mysql_thread_id<>CONNECTION_ID()"
            ),
            "open database transaction count",
            0,
        )
        if open_transactions:
            raise ToolError("open database transactions are present; refusing restitution")
        if sample == 0:
            time.sleep(0.05)


def check_quiescence(db: Mysql, proof_path: Path) -> None:
    proof_path = proof_path.expanduser()
    try:
        info = proof_path.lstat()
    except FileNotFoundError as exc:
        raise ToolError("apply requires an offline/quiescence proof file") from exc
    if (
        stat.S_ISLNK(info.st_mode)
        or not stat.S_ISREG(info.st_mode)
        or info.st_uid != os.getuid()
        or info.st_mode & 0o077
        or info.st_size > 8192
    ):
        raise ToolError("offline proof must be a small owner-only regular file")
    try:
        lines = proof_path.read_text(encoding="utf-8").splitlines()
    except OSError as exc:
        raise ToolError("offline proof could not be read") from exc
    values: dict[str, str] = {}
    for line in lines:
        if not line.strip():
            continue
        if "=" not in line:
            raise ToolError("offline proof contains an invalid line")
        key, value = line.split("=", 1)
        key = key.strip()
        if not re.fullmatch(r"[a-z_]+", key) or key in values:
            raise ToolError("offline proof contains duplicate or invalid fields")
        values[key] = value.strip()
    required = {
        "format": QUIESCENCE_PROOF_FORMAT,
        "database": db.database,
        "boundary": QUIESCENCE_BOUNDARY,
        "guard": RUNTIME_EXCLUSION_LOCK_PREFIX,
    }
    if any(values.get(key) != expected for key, expected in required.items()):
        raise ToolError("offline proof does not identify the supported quiescent boundary")
    expiry = values.get("expires_at")
    if not expiry or parse_expiry(expiry) <= time.time() + 5:
        raise ToolError("offline proof is expired or too close to expiry")
    check_process_inventory()
    check_database_quiescence(db)


def eligible_items(plan: dict[str, Any]) -> list[dict[str, Any]]:
    validate_artifact_timing_compensations(plan)
    rows = [row for row in plan.get("items", []) if row.get("eligible") is True]
    if not rows:
        raise ToolError("plan contains no exact-state recoverable items")
    seen: set[int] = set()
    for row in rows:
        uid = int_value(row.get("item_uid"), "planned item UID", 1, 2**64 - 1)
        if uid in seen:
            raise ToolError("plan contains duplicate item identities")
        seen.add(uid)
        if not isinstance(row.get("metadata_payload_hex"), str):
            raise ToolError("eligible item has no supported metadata payload")
        if row.get("kind") == "artifact":
            timing = row.get("artifact_timing")
            if not isinstance(timing, dict):
                raise ToolError("eligible artifact has no timing evidence")
            basis = timing.get("basis")
            if basis not in {"historical_loss_remainder", "approved_compensation"}:
                raise ToolError("eligible artifact has an unsupported timing basis")
            int_value(
                timing.get("usable_lifetime_seconds"),
                "artifact usable lifetime",
                1,
                MAX_ARTIFACT_COMPENSATION_SECONDS,
            )
            if basis == "approved_compensation" and not isinstance(
                timing.get("compensation_reference"), str
            ):
                raise ToolError("approved artifact compensation has no durable reference")
    return rows


def build_apply_sql(plan: dict[str, Any], actor: str, reason: str) -> str:
    source_pid = int_value(plan["source"]["pid"], "source pid", 1, 2**31 - 1)
    death_revision = int_value(plan["source"]["death_revision"], "death revision", 1, 2**64 - 1)
    recipient_pid = int_value(plan["recipient_pid"], "recipient pid", 1, 2**31 - 1)
    rid = hex_bytes(plan["restitution_id_hex"], "restitution ID")
    evidence_digest = hex_bytes(plan["evidence_digest"], "evidence digest")
    plan_digest = hex_bytes(plan["plan_digest"], "plan digest")
    payload_digest = hex_bytes(plan["payload_digest"], "payload digest")
    death_operation = hex_bytes(plan["source"]["operation_id_hex"], "death operation")
    eligible = eligible_items(plan)
    all_items = plan.get("items", [])
    if not isinstance(all_items, list) or not all_items:
        raise ToolError("plan has no candidate rows")
    if len(all_items) > 3000 or len(eligible) > 3000:
        raise ToolError("plan exceeds the supported restitution item bound")
    n = len(eligible)
    actor = actor.strip()
    reason = reason.strip()
    if not re.fullmatch(r"[A-Za-z0-9_.@:/ -]{1,128}", actor):
        raise ToolError("actor must be a short non-secret operator label")
    if not re.fullmatch(r"[A-Za-z0-9_.@:/()\[\], -]{1,255}", reason):
        raise ToolError("reason contains unsupported characters")

    custody_evidence = plan.get("custody_evidence", [])
    related_custody = plan.get("related_custody", [])
    related_deaths = plan.get("related_deaths", [])
    if not isinstance(custody_evidence, list) or not isinstance(related_custody, list) or not isinstance(related_deaths, list):
        raise ToolError("plan is missing structured custody/death evidence")
    if len(custody_evidence) > 3000 or len(related_custody) > 10000 or len(related_deaths) > 1000:
        raise ToolError("plan exceeds the supported evidence bound")
    expected_owner_revisions = {
        int_value(row["expected_current"]["owner_revision"], "current owner revision", 0, 2**64 - 1)
        for row in eligible
    }
    if len(expected_owner_revisions) != 1:
        raise ToolError("eligible items do not share one source owner revision")
    expected_owner_revision = next(iter(expected_owner_revisions))

    lines = [
        "SET autocommit=0;",
        "START TRANSACTION;",
        "CREATE TEMPORARY TABLE restitution_apply_items ("
        "item_uid BIGINT UNSIGNED NOT NULL PRIMARY KEY,"
        "delivered_root_item_uid BIGINT UNSIGNED NOT NULL,"
        "delivered_parent_item_uid BIGINT UNSIGNED NOT NULL DEFAULT 0,"
        "expected_root_item_uid BIGINT UNSIGNED NOT NULL,"
        "expected_parent_item_uid BIGINT UNSIGNED NOT NULL DEFAULT 0,"
        "expected_item_revision BIGINT UNSIGNED NOT NULL,"
        "expected_owner_revision BIGINT UNSIGNED NOT NULL,"
        "expected_state TINYINT UNSIGNED NOT NULL,"
        "custody_item_revision BIGINT UNSIGNED NOT NULL,"
        "custody_state TINYINT UNSIGNED NOT NULL,"
        "custody_owner_type TINYINT UNSIGNED NOT NULL,"
        "custody_owner_id BIGINT UNSIGNED NOT NULL,"
        "custody_owner_context_id BIGINT UNSIGNED NOT NULL,"
        "custody_owner_revision BIGINT UNSIGNED NOT NULL,"
        "vnum INT NOT NULL,metadata_digest BINARY(32) NOT NULL,"
        "metadata_payload MEDIUMBLOB NOT NULL) ENGINE=InnoDB;",
        "CREATE TEMPORARY TABLE restitution_existing_inventory ("
        "item_uid BIGINT UNSIGNED NOT NULL PRIMARY KEY) ENGINE=InnoDB;",
    ]
    temp_values: list[str] = []
    for row in eligible:
        current = row.get("expected_current")
        if not isinstance(current, dict):
            raise ToolError("eligible item has no current ownership fence")
        temp_values.append("(" + ",".join([
            sql_num(row["item_uid"], "item UID", 1, 2**64 - 1),
            sql_num(row["delivered_root_item_uid"], "delivery root", 1, 2**64 - 1),
            sql_num(row.get("delivered_parent_item_uid", 0), "delivery parent", 0, 2**64 - 1),
            sql_num(current["root_item_uid"], "current root", 1, 2**64 - 1),
            sql_num(current.get("parent_item_uid", 0), "current parent", 0, 2**64 - 1),
            sql_num(current["item_revision"], "current item revision", 0, 2**64 - 1),
            sql_num(current["owner_revision"], "current owner revision", 0, 2**64 - 1),
            sql_num(current["state"], "current item state", 0, 255),
            sql_num(row.get("custody_expected_item_revision", current["item_revision"]), "custody item revision", 0, 2**64 - 1),
            sql_num(row.get("custody_expected_state", current["state"]), "custody state", 0, 255),
            sql_num(row.get("custody_owner_type", OWNER_PLAYER), "custody owner type", 0, 255),
            sql_num(row.get("custody_owner_id", source_pid), "custody owner ID", 0, 2**64 - 1),
            sql_num(row.get("custody_owner_context_id", 0), "custody owner context", 0, 2**64 - 1),
            sql_num(row.get("custody_owner_revision", current["owner_revision"]), "custody owner revision", 0, 2**64 - 1),
            sql_num(row["vnum"], "item vnum", 1, 2**31 - 1),
            sql_blob(row["metadata_digest"], "metadata digest"),
            sql_blob(row["metadata_payload_hex"], "metadata payload"),
        ]) + ")")
    lines.append(
        "INSERT INTO restitution_apply_items(item_uid,delivered_root_item_uid,delivered_parent_item_uid,"
        "expected_root_item_uid,expected_parent_item_uid,expected_item_revision,expected_owner_revision,expected_state,"
        "custody_item_revision,custody_state,custody_owner_type,custody_owner_id,custody_owner_context_id,"
        "custody_owner_revision,vnum,metadata_digest,metadata_payload) VALUES " + ",".join(temp_values) + ";"
    )
    existing_uids = sorted({
        int_value(uid, "existing recipient UID", 1, 2**64 - 1)
        for uid in plan.get("recipient_existing_uids", [])
    })
    if existing_uids:
        lines.append(
            "INSERT INTO restitution_existing_inventory(item_uid) VALUES " +
            ",".join("(" + sql_num(uid, "existing recipient UID", 1, 2**64 - 1) + ")" for uid in existing_uids) + ";"
        )
    lines.append(
        f"SELECT GET_LOCK({RUNTIME_EXCLUSION_LOCK_EXPRESSION},0) INTO @restitution_lock;"
    )
    lines.append(
        f"SET @restitution_owner=(IFNULL(IS_USED_LOCK({RUNTIME_EXCLUSION_LOCK_EXPRESSION}),0)=CONNECTION_ID());"
    )
    lines.append("SET @restitution_ok=(@restitution_lock=1 AND @restitution_owner=1);")
    lines.append(
        "SELECT pid,save_revision FROM player_death_disposition WHERE pid=" + str(source_pid) +
        " AND save_revision=" + str(death_revision) + " FOR UPDATE;"
    )
    lines.append(
        "SET @death_lock_ok=((SELECT COUNT(*) FROM player_death_disposition WHERE pid=" + str(source_pid) +
        " AND save_revision=" + str(death_revision) + ")=1);"
    )
    lines.append(
        "SELECT pid,save_revision,item_uid FROM player_death_custody WHERE pid=" + str(source_pid) +
        " AND save_revision=" + str(death_revision) + " FOR UPDATE;"
    )
    lines.append(
        "SET @source_custody_ok=((SELECT COUNT(*) FROM player_death_custody WHERE pid=" +
        str(source_pid) + " AND save_revision=" + str(death_revision) + ")=" +
        str(len(custody_evidence)) + ");"
    )
    for evidence in custody_evidence:
        if not isinstance(evidence, dict):
            raise ToolError("source custody evidence has an invalid row")
        lines.append(
            "SET @source_custody_ok=@source_custody_ok AND (SELECT COUNT(*) FROM player_death_custody WHERE "
            "pid=" + sql_num(evidence["pid"], "custody pid", 1, 2**31 - 1) +
            " AND save_revision=" + sql_num(evidence["save_revision"], "custody revision", 1, 2**64 - 1) +
            " AND item_uid=" + sql_num(evidence["item_uid"], "custody UID", 1, 2**64 - 1) +
            " AND root_item_uid=" + sql_num(evidence["root_item_uid"], "custody root", 1, 2**64 - 1) +
            " AND parent_item_uid=" + sql_num(evidence.get("parent_item_uid", 0), "custody parent", 0, 2**64 - 1) +
            " AND item_revision=" + sql_num(evidence["expected_item_revision"], "custody item revision", 0, 2**64 - 1) +
            " AND vnum=" + sql_num(evidence["vnum"], "custody vnum", 1, 2**31 - 1) +
            " AND state=" + sql_num(evidence["expected_state"], "custody state", 0, 255) +
            " AND owner_type=" + sql_num(evidence["owner_type"], "custody owner type", 0, 255) +
            " AND owner_id=" + sql_num(evidence["owner_id"], "custody owner ID", 0, 2**64 - 1) +
            " AND owner_context_id=" + sql_num(evidence["owner_context_id"], "custody context", 0, 2**64 - 1) +
            " AND owner_revision=" + sql_num(evidence["owner_revision"], "custody owner revision", 0, 2**64 - 1) + ")=1;"
        )
    related_pairs: set[tuple[int, int]] = {(source_pid, death_revision)}
    for evidence in related_custody:
        if not isinstance(evidence, dict):
            raise ToolError("related custody evidence has an invalid row")
        related_pairs.add((
            int_value(evidence["pid"], "related custody pid", 1, 2**31 - 1),
            int_value(evidence["save_revision"], "related custody revision", 1, 2**64 - 1),
        ))
    for evidence in related_deaths:
        if not isinstance(evidence, dict):
            raise ToolError("related death evidence has an invalid row")
        related_pairs.add((
            int_value(evidence["pid"], "related death pid", 1, 2**31 - 1),
            int_value(evidence["save_revision"], "related death revision", 1, 2**64 - 1),
        ))
    related_pair_clauses = [
        "(pid=" + sql_num(pid, "related custody pid", 1, 2**31 - 1) +
        " AND save_revision=" + sql_num(revision, "related custody revision", 1, 2**64 - 1) + ")"
        for pid, revision in sorted(related_pairs)
    ]
    lines.append(
        "SELECT pid,save_revision,item_uid FROM player_death_custody WHERE " +
        " OR ".join(related_pair_clauses) + " FOR UPDATE;"
    )
    unique_related_custody = {
        (int(row["pid"]), int(row["save_revision"]), int(row["item_uid"]))
        for row in related_custody
    }
    lines.append(
        "SET @related_custody_ok=((SELECT COUNT(*) FROM player_death_custody WHERE " +
        " OR ".join(related_pair_clauses) + ")=" + str(len(unique_related_custody)) + ");"
    )
    for evidence in related_custody:
        lines.append(
            "SET @related_custody_ok=@related_custody_ok AND (SELECT COUNT(*) FROM player_death_custody WHERE "
            "pid=" + sql_num(evidence["pid"], "related custody pid", 1, 2**31 - 1) +
            " AND save_revision=" + sql_num(evidence["save_revision"], "related custody revision", 1, 2**64 - 1) +
            " AND item_uid=" + sql_num(evidence["item_uid"], "related custody UID", 1, 2**64 - 1) +
            " AND root_item_uid=" + sql_num(evidence["root_item_uid"], "related custody root", 1, 2**64 - 1) +
            " AND parent_item_uid=" + sql_num(evidence.get("parent_item_uid", 0), "related custody parent", 0, 2**64 - 1) +
            " AND item_revision=" + sql_num(evidence["expected_item_revision"], "related custody item revision", 0, 2**64 - 1) +
            " AND vnum=" + sql_num(evidence["vnum"], "related custody vnum", 1, 2**31 - 1) +
            " AND state=" + sql_num(evidence["expected_state"], "related custody state", 0, 255) +
            " AND owner_type=" + sql_num(evidence["owner_type"], "related custody owner type", 0, 255) +
            " AND owner_id=" + sql_num(evidence["owner_id"], "related custody owner ID", 0, 2**64 - 1) +
            " AND owner_context_id=" + sql_num(evidence["owner_context_id"], "related custody context", 0, 2**64 - 1) +
            " AND owner_revision=" + sql_num(evidence["owner_revision"], "related custody owner revision", 0, 2**64 - 1) + ")=1;"
        )
    if related_deaths:
        death_pair_clauses = []
        for evidence in related_deaths:
            if not isinstance(evidence, dict):
                raise ToolError("related death evidence has an invalid row")
            death_pair_clauses.append(
                "(pid=" + sql_num(evidence["pid"], "related death pid", 1, 2**31 - 1) +
                " AND save_revision=" + sql_num(evidence["save_revision"], "related death revision", 1, 2**64 - 1) + ")"
            )
        lines.append(
            "SELECT pid,save_revision FROM player_death_disposition WHERE " +
            " OR ".join(sorted(set(death_pair_clauses))) + " FOR UPDATE;"
        )
        lines.append("SET @related_death_ok=1;")
        for evidence in related_deaths:
            digest = evidence.get("payload_digest")
            if not isinstance(digest, str):
                raise ToolError("related death evidence has no payload digest")
            lines.append(
                "SET @related_death_ok=@related_death_ok AND (SELECT COUNT(*) FROM player_death_disposition WHERE "
                "pid=" + sql_num(evidence["pid"], "related death pid", 1, 2**31 - 1) +
                " AND save_revision=" + sql_num(evidence["save_revision"], "related death revision", 1, 2**64 - 1) +
                " AND operation_id=" + sql_blob(evidence["operation_id_hex"], "related death operation") +
                " AND SHA2(payload,256)=UPPER('" + digest.lower() + "') AND FLOOR(UNIX_TIMESTAMP(recorded_at))=" +
                sql_num(evidence["loss_epoch"], "related death loss epoch", 1, 2**63 - 1) + ")=1;"
            )
    else:
        lines.append("SET @related_death_ok=1;")
    lines.append(
        "SELECT item_uid FROM item_current_owner WHERE item_uid IN " +
        sql_list([row["item_uid"] for row in eligible]) + " FOR UPDATE;"
    )
    lines.append(
        "SELECT revision FROM item_owner_revision WHERE owner_type=1 AND owner_id=" + str(source_pid) +
        " AND owner_context_id=0 FOR UPDATE;"
    )
    if existing_uids:
        lines.append(
            "SELECT pi.obj_uid FROM player_items pi JOIN restitution_existing_inventory e "
            "ON e.item_uid=pi.obj_uid WHERE pi.pid=" + str(recipient_pid) + " FOR UPDATE;"
        )
        lines.append(
            "SET @recipient_inventory_ok=((SELECT COUNT(*) FROM player_items pi JOIN restitution_existing_inventory e "
            "ON e.item_uid=pi.obj_uid WHERE pi.pid=" + str(recipient_pid) + ")=" + str(len(existing_uids)) + ");"
        )
    else:
        lines.append("SET @recipient_inventory_ok=1;")
    lines.append(
        "SELECT restitution_id FROM player_death_restitution_receipt WHERE restitution_id=" +
        sql_blob(rid) + " FOR UPDATE;"
    )
    lines.append("SET @restitution_completed=(SELECT COUNT(*) FROM player_death_restitution_receipt "
                 "WHERE restitution_id=" + sql_blob(rid) + " AND plan_digest=" + sql_blob(plan_digest) +
                 " AND evidence_digest=" + sql_blob(evidence_digest) + " AND status IN (2,3));")
    lines.append(
        "SET @database_quiescent=((" + database_visibility_sql() + ")=1 AND "
        "(SELECT COUNT(*) FROM information_schema.processlist "
        "WHERE ID<>CONNECTION_ID())=0 AND (SELECT COUNT(*) FROM information_schema.innodb_trx "
        "WHERE trx_mysql_thread_id<>CONNECTION_ID())=0);"
    )
    lines.append(
        "SET @restitution_ok=@restitution_ok AND @database_quiescent AND @death_lock_ok="
        "1 AND @source_custody_ok=1 AND @related_custody_ok=1 AND @related_death_ok=1 "
        "AND @recipient_inventory_ok=1;"
    )
    artifact_vnums = sorted({
        int_value(row["vnum"], "artifact vnum", 1, 2**31 - 1)
        for row in eligible if row.get("kind") == "artifact"
    })
    for artifact_vnum in artifact_vnums:
        vnum = sql_num(artifact_vnum, "artifact vnum", 1, 2**31 - 1)
        # Lock every authority row, including the empty unique-key gap.  The
        # subsequent predicates remain conditional fences for READ COMMITTED
        # engines and make the expected row shape explicit.
        artifact_tables = {"artifact_domain_state"}
        for artifact_row in eligible:
            if artifact_row.get("kind") != "artifact" or int_value(artifact_row["vnum"], "artifact vnum", 1) != artifact_vnum:
                continue
            artifact_decision = artifact_row.get("artifact_reconciliation") or {}
            if artifact_row.get("artifact_reconciliation_required") or isinstance(
                artifact_decision.get("baseline_before"), dict
            ):
                artifact_tables.add("artifact_domain_baseline")
            if artifact_row.get("artifact_reconciliation_required") or isinstance(
                artifact_row.get("artifact_bind_before"), dict
            ):
                artifact_tables.add("artifact_bind")
            artifact_tables.update(
                "artifacts_mortal" if name == "mortal" else "artifacts"
                for name in artifact_row.get("artifact_legacy_before", {})
            )
        for table in sorted(artifact_tables):
            lines.append("SELECT vnum FROM " + table + " WHERE vnum=" + vnum + " FOR UPDATE;")
        lines.append("SELECT item_uid FROM item_current_owner WHERE vnum=" + vnum + " FOR UPDATE;")
        lines.append("SELECT item_uid FROM player_death_custody WHERE vnum=" + vnum + " AND state<>2 FOR UPDATE;")
    lines.append("SET @artifact_ok=1;")
    for row in eligible:
        if row.get("kind") != "artifact":
            continue
        item = row["metadata"]
        vnum = sql_num(item["vnum"], "artifact vnum", 1, 2**31 - 1)
        uid = sql_num(row["item_uid"], "artifact UID", 1, 2**64 - 1)
        reconciliation = row.get("artifact_reconciliation")
        if not isinstance(reconciliation, dict):
            raise ToolError("eligible artifact has no reconciliation evidence")
        domain = row.get("artifact_before")
        domain_fence = ""
        if isinstance(domain, dict):
            domain_uid = domain.get("item_uid")
            domain_uid_fence = "item_uid IS NULL" if domain_uid is None else (
                "item_uid=" + sql_num(domain_uid, "artifact domain UID", 0, 2**64 - 1)
            )
            domain_fence = (
                " AND " + domain_uid_fence +
                " AND owned=1 AND loc_type=" + sql_num(domain["loc_type"], "artifact location type", 0) +
                " AND location=" + sql_num(domain["location"], "artifact location", -2**31, 2**31 - 1) +
                " AND artifact_type=" + sql_num(domain["artifact_type"], "artifact type", 0) +
                " AND revision=" + sql_num(domain["revision"], "artifact revision", 0) +
                " AND timer_epoch=" + sql_num(domain["timer_epoch"], "artifact timer", -2**63, 2**63 - 1) +
                " AND bind_owner_pid=" + sql_num(domain["bind_owner_pid"], "artifact bind owner", -2**31, 2**31 - 1) +
                " AND bind_timer_epoch=" + sql_num(domain["bind_timer_epoch"], "artifact bind timer", -2**63, 2**63 - 1) +
                " AND COALESCE(item_revision,0)=" + sql_num(domain["item_revision"], "artifact item revision", 0)
            )
            lines.append(
                "SET @artifact_ok=@artifact_ok AND (@restitution_completed=1 OR (SELECT COUNT(*) "
                "FROM artifact_domain_state WHERE vnum=" + vnum + domain_fence + ")=1);"
            )
        else:
            # Missing-domain reconciliation is additive only.  A row appearing
            # after plan creation is a race/competing authority, not a reason
            # to overwrite it.
            lines.append(
                "SET @artifact_ok=@artifact_ok AND (@restitution_completed=1 OR "
                "(SELECT COUNT(*) FROM artifact_domain_state WHERE vnum=" + vnum + ")=0);"
            )
        lines.append(
            "SET @artifact_ok=@artifact_ok AND (@restitution_completed=1 OR "
            "(SELECT COUNT(*) FROM item_current_owner WHERE vnum=" + vnum +
            " AND item_uid<>" + uid + " AND state<>2)=0);"
        )
        lines.append(
            "SET @artifact_ok=@artifact_ok AND (@restitution_completed=1 OR "
            "(SELECT COUNT(*) FROM player_death_custody c LEFT JOIN item_current_owner resolved "
            "ON resolved.item_uid=c.item_uid WHERE c.vnum=" + vnum +
            " AND c.item_uid<>" + uid + " AND c.state<>2 "
            "AND (resolved.item_uid IS NULL OR resolved.state<>2))=0);"
        )
        baseline = reconciliation.get("baseline_before")
        baseline_seed = reconciliation.get("baseline_seed")
        if not isinstance(baseline_seed, dict):
            raise ToolError("eligible artifact has no baseline preservation evidence")
        if isinstance(baseline, dict):
            lines.append(
                "SET @artifact_ok=@artifact_ok AND (@restitution_completed=1 OR (SELECT COUNT(*) "
                "FROM artifact_domain_baseline WHERE vnum=" + vnum +
                " AND opening_timer_epoch=" + sql_num(baseline["opening_timer_epoch"], "artifact opening timer", -2**63, 2**63 - 1) +
                " AND opening_bind_owner_pid=" + sql_num(baseline["opening_bind_owner_pid"], "artifact opening bind owner", -2**31, 2**31 - 1) +
                " AND opening_bind_timer_epoch=" + sql_num(baseline["opening_bind_timer_epoch"], "artifact opening bind timer", -2**63, 2**63 - 1) +
                " AND opening_revision=" + sql_num(baseline["opening_revision"], "artifact opening revision", 0, 2**64 - 1) +
                ")=1);"
            )
        elif row.get("artifact_reconciliation_required"):
            lines.append(
                "SET @artifact_ok=@artifact_ok AND (@restitution_completed=1 OR "
                "(SELECT COUNT(*) FROM artifact_domain_baseline WHERE vnum=" + vnum + ")=0);"
            )
        bind = row.get("artifact_bind_before")
        if isinstance(bind, dict):
            lines.append(
                "SET @artifact_ok=@artifact_ok AND (@restitution_completed=1 OR (SELECT COUNT(*) FROM artifact_bind WHERE vnum=" + vnum +
                " AND COALESCE(owner_pid,-1)=" + sql_num(bind["owner_pid"], "artifact bind owner", -1, 2**31 - 1) +
                " AND COALESCE(timer,0)=" + sql_num(bind["timer"], "artifact bind timer", -2**31, 2**31 - 1) + ")=1);"
            )
        elif row.get("artifact_reconciliation_required"):
            lines.append(
                "SET @artifact_ok=@artifact_ok AND (@restitution_completed=1 OR "
                "(SELECT COUNT(*) FROM artifact_bind WHERE vnum=" + vnum + ")=0);"
            )
        for legacy_name, legacy in row.get("artifact_legacy_before", {}).items():
            table = "artifacts_mortal" if legacy_name == "mortal" else "artifacts"
            lines.append(
                "SET @artifact_ok=@artifact_ok AND (@restitution_completed=1 OR (SELECT COUNT(*) FROM " + table + " WHERE vnum=" + vnum+
                " AND owned='Y' AND locType=" + sql_num(legacy["loc_type"], "legacy artifact location type", 0) +
                " AND location=" + sql_num(legacy["location"], "legacy artifact location", -2**31, 2**31 - 1) +
                " AND COALESCE(UNIX_TIMESTAMP(timer),0)=" + sql_num(legacy["timer"], "legacy artifact timer", 0) +
                " AND type=" + sql_num(legacy["artifact_type"], "legacy artifact type", 0) + ")=1);"
            )
    lines.extend([
        "SET @restitution_collision=(SELECT COUNT(*) FROM player_death_restitution_receipt "
        "WHERE restitution_id=" + sql_blob(rid) + " AND "
        "(plan_digest<>" + sql_blob(plan_digest) + " OR evidence_digest<>" + sql_blob(evidence_digest) + "));",
        "SET @restitution_ok=@restitution_ok AND @restitution_collision=0;",
        "SET @death_ok=(SELECT COUNT(*) FROM player_death_disposition WHERE pid=" + str(source_pid) +
        " AND save_revision=" + str(death_revision) + " AND operation_id=" + sql_blob(death_operation) +
        " AND corpse_item_uid=" + sql_num(plan["source"].get("corpse_item_uid", 1), "corpse UID", 1, 2**64 - 1) +
        " AND corpse_room_vnum=" + sql_num(plan["source"].get("corpse_room_vnum", 1), "corpse room", 1, 2**31 - 1) +
        " AND wallet_revision=" + sql_num(plan["source"].get("wallet_revision", 1), "wallet revision", 1, 2**64 - 1) +
        " AND wallet_pile_uid=" + sql_num(plan["source"].get("wallet_pile_uid", 0), "wallet UID", 0, 2**64 - 1) +
        " AND wallet_copper=" + sql_num(plan["source"].get("wallet_before", [0, 0, 0, 0])[0], "wallet copper") +
        " AND wallet_silver=" + sql_num(plan["source"].get("wallet_before", [0, 0, 0, 0])[1], "wallet silver") +
        " AND wallet_gold=" + sql_num(plan["source"].get("wallet_before", [0, 0, 0, 0])[2], "wallet gold") +
        " AND wallet_platinum=" + sql_num(plan["source"].get("wallet_before", [0, 0, 0, 0])[3], "wallet platinum") +
        " AND SHA2(payload,256)=UPPER('" + payload_digest.hex() + "') AND FLOOR(UNIX_TIMESTAMP(recorded_at))=" +
        sql_num(plan["source"].get("loss_epoch", 1), "source loss epoch", 1, 2**63 - 1) + ");",
        "SET @owner_ok=(SELECT COUNT(*) FROM item_current_owner own JOIN restitution_apply_items p "
        "ON p.item_uid=own.item_uid LEFT JOIN item_owner_revision r ON r.owner_type=own.owner_type "
        "AND r.owner_id=own.owner_id AND r.owner_context_id=own.owner_context_id WHERE "
        "own.owner_type=1 AND own.owner_id=" + str(source_pid) + " AND own.owner_context_id=0 "
        "AND own.root_item_uid=p.expected_root_item_uid AND COALESCE(own.parent_item_uid,0)=p.expected_parent_item_uid "
        "AND own.item_revision=p.expected_item_revision AND own.state=p.expected_state "
        "AND COALESCE(r.revision,0)=p.expected_owner_revision AND own.vnum=p.vnum);",
        "SET @parent_ok=(SELECT COUNT(*) FROM restitution_apply_items p WHERE p.delivered_parent_item_uid=0 OR "
        "EXISTS(SELECT 1 FROM item_current_owner parent JOIN restitution_apply_items pp "
        "ON pp.item_uid=p.delivered_parent_item_uid WHERE parent.item_uid=pp.item_uid "
        "AND parent.owner_type=1 AND parent.owner_id=" + str(source_pid) +
        " AND parent.owner_context_id=0 AND parent.root_item_uid=pp.expected_root_item_uid "
        "AND COALESCE(parent.parent_item_uid,0)=pp.expected_parent_item_uid "
        "AND parent.item_revision=pp.expected_item_revision AND parent.state=pp.expected_state "
        "AND pp.expected_state=3));",
        "SET @revision_ok=(SELECT COUNT(*) FROM item_owner_revision WHERE owner_type=1 AND owner_id=" +
        str(source_pid) + " AND owner_context_id=0 AND revision=" + str(expected_owner_revision) + ");",
        "SELECT d.item_uid FROM player_death_restitution_delivery d JOIN restitution_apply_items p "
        "ON p.item_uid=d.item_uid FOR UPDATE;",
        "SET @delivery_ok=(SELECT COUNT(*) FROM player_death_restitution_delivery d JOIN restitution_apply_items p "
        "ON p.item_uid=d.item_uid);",
        "SELECT pi.id,pi.obj_uid FROM player_items pi JOIN restitution_apply_items p "
        "ON p.item_uid=pi.obj_uid FOR UPDATE;",
        "SET @projection_ok=(SELECT COUNT(*) FROM player_items pi JOIN restitution_apply_items p "
        "ON p.item_uid=pi.obj_uid);",
        "SET @restitution_ok=@restitution_ok AND @artifact_ok=1 AND (@restitution_completed=1 OR "
        "(@death_ok=1 AND @owner_ok=" + str(n) + " AND @parent_ok=" + str(n) +
        " AND @revision_ok=1 AND @delivery_ok=0 AND @projection_ok=0));",
        "SET @receipt_inserted=0,@receipt_items_inserted=0,@player_rows_inserted=0,"
        "@delivery_rows_inserted=0,@runtime_rows_inserted=0,@affect_rows_inserted=0,"
        "@description_rows_inserted=0,@owner_revision_updated=0,@owner_rows_updated=0,"
        "@artifact_domain_rows_updated=0,@artifact_domain_rows_inserted=0,"
        "@artifact_baseline_rows_inserted=0,@artifact_legacy_rows_updated=0;",
        # Capture the delivery epoch only after every blocking validation/lock
        # above and before the first persistent receipt/projection mutation. It
        # anchors delivery, not admission or the later COMMIT; the small,
        # unavoidable mutation-to-COMMIT gap remains real elapsed lifetime.
        "SET @restitution_delivery_epoch=FLOOR(UNIX_TIMESTAMP(CURRENT_TIMESTAMP(6)));",
        "INSERT INTO player_death_restitution_receipt(restitution_id,source_pid,death_revision,recipient_pid,"
        "death_operation_id,evidence_digest,plan_digest,status,actor,reason,candidate_count,delivered_count,"
        "unresolved_count,applied_at) SELECT " + sql_blob(rid) + "," + str(source_pid) + "," +
        str(death_revision) + "," + str(recipient_pid) + "," + sql_blob(death_operation) + "," +
        sql_blob(evidence_digest) + "," + sql_blob(plan_digest) + ",2," + sql_text(actor, "actor") + "," +
        sql_text(reason, "reason") + "," + str(len(all_items)) + "," + str(n) + "," +
        str(len(all_items) - n) + ",CURRENT_TIMESTAMP(6) WHERE @restitution_ok=1 AND @restitution_completed=0;",
        "SET @receipt_inserted=@receipt_inserted+ROW_COUNT();",
    ])

    for row in all_items:
        uid = int_value(row["item_uid"], "item UID", 1, 2**64 - 1)
        payload_hex = row.get("metadata_payload_hex")
        metadata_digest = row.get("metadata_digest")
        payload_expr = sql_blob(payload_hex, "metadata payload") if payload_hex else "NULL"
        digest_expr = sql_blob(metadata_digest, "metadata digest") if metadata_digest else "NULL"
        artifact_timing = row.get("artifact_timing") if row.get("kind") == "artifact" else {}
        if not isinstance(artifact_timing, dict):
            evidence = row.get("artifact_reconciliation")
            artifact_timing = evidence if isinstance(evidence, dict) else {}
        artifact_loss_epoch = sql_num(
            artifact_timing.get("loss_epoch", 0), "artifact loss epoch", 0, 2**63 - 1
        )
        artifact_source_timer_epoch = sql_num(
            artifact_timing.get("source_timer_epoch", 0), "artifact source timer epoch", 0, 2**63 - 1
        )
        artifact_lifetime = sql_num(
            artifact_timing.get("usable_lifetime_seconds", 0),
            "artifact usable lifetime",
            0,
            MAX_ARTIFACT_COMPENSATION_SECONDS,
        )
        artifact_basis = sql_text(str(artifact_timing.get("basis", "")), "artifact timing basis")
        artifact_compensation_reference = sql_text(
            str(artifact_timing.get("compensation_reference", "")),
            "artifact compensation reference",
        )
        artifact_delivered_timer = (
            "(@restitution_delivery_epoch+" + artifact_lifetime + ")"
            if row.get("eligible") and row.get("kind") == "artifact" else "0"
        )
        delivered_root = sql_num(row.get("delivered_root_item_uid", 0), "delivery root", 0, 2**64 - 1)
        delivered_parent = sql_num(row.get("delivered_parent_item_uid", 0), "delivery parent", 0, 2**64 - 1)
        source_root = sql_num(row.get("source_root_item_uid", 0), "source root", 0, 2**64 - 1)
        source_parent = sql_num(row.get("source_parent_item_uid", 0), "source parent", 0, 2**64 - 1)
        source_revision = sql_num(row.get("source_item_revision", 0), "source item revision", 0, 2**64 - 1)
        delivered_revision = sql_num(
            int_value(row.get("expected_current", {}).get("item_revision", 0), "current item revision", 0, 2**64 - 2) + 1,
            "delivered item revision", 1, 2**64 - 1,
        ) if row.get("eligible") else "0"
        lines.append(
            "INSERT INTO player_death_restitution_item(restitution_id,item_uid,source_root_item_uid,"
            "source_parent_item_uid,delivered_root_item_uid,delivered_parent_item_uid,source_item_revision,"
            "delivered_item_revision,vnum,artifact_vnum,disposition,classification,metadata_digest,metadata_payload,note,"
            "artifact_loss_epoch,artifact_source_timer_epoch,artifact_usable_lifetime_seconds,"
            "artifact_delivered_timer_epoch,artifact_timing_basis,artifact_compensation_reference) "
            "SELECT " + sql_blob(rid) + "," + str(uid) + "," + source_root + "," + source_parent + "," +
            delivered_root + "," + delivered_parent + "," + source_revision + "," + delivered_revision + "," +
            sql_num(row.get("vnum", 0), "item vnum", 0, 2**31 - 1) + "," +
            sql_num(row.get("vnum", 0) if row.get("kind") == "artifact" else 0, "artifact vnum", 0, 2**31 - 1) + "," +
            sql_num(row["disposition"], "disposition", 1, 255) + "," + sql_text(row["classification"], "classification") + "," +
            digest_expr + "," + payload_expr + "," + sql_text(row.get("note", ""), "note") + "," +
            artifact_loss_epoch + "," + artifact_source_timer_epoch + "," + artifact_lifetime + "," +
            artifact_delivered_timer + "," + artifact_basis + "," + artifact_compensation_reference +
            " WHERE @restitution_ok=1 AND @restitution_completed=0;"
        )
        lines.append("SET @receipt_items_inserted=@receipt_items_inserted+ROW_COUNT();")

    lines.extend([
        "UPDATE item_owner_revision SET revision=revision+1 WHERE @restitution_ok=1 AND @restitution_completed=0 "
        "AND owner_type=1 AND owner_id=" + str(source_pid) + " AND owner_context_id=0 "
        "AND revision=" + str(expected_owner_revision) + ";",
        "SET @owner_revision_updated=ROW_COUNT();",
        "UPDATE item_current_owner own JOIN restitution_apply_items p ON p.item_uid=own.item_uid SET "
        "own.root_item_uid=p.delivered_root_item_uid,own.parent_item_uid=IF(p.delivered_parent_item_uid=0,NULL,p.delivered_parent_item_uid),"
        "own.owner_type=1,own.owner_id=" + str(recipient_pid) + ",own.owner_context_id=0,own.item_revision=own.item_revision+1,own.state=1 "
        "WHERE @restitution_ok=1 AND @restitution_completed=0 AND own.owner_type=1 AND own.owner_id=" + str(source_pid) +
        " AND own.owner_context_id=0 AND own.root_item_uid=p.expected_root_item_uid "
        "AND COALESCE(own.parent_item_uid,0)=p.expected_parent_item_uid AND own.item_revision=p.expected_item_revision "
        "AND own.state=p.expected_state AND own.vnum=p.vnum;",
        "SET @owner_rows_updated=ROW_COUNT();",
    ])

    variables: dict[int, str] = {}
    for row in eligible:
        uid = int_value(row["item_uid"], "item UID", 1, 2**64 - 1)
        variables[uid] = "@restitution_item_" + str(uid)
        item = row["metadata"]
        parent_uid = int_value(row.get("delivered_parent_item_uid", 0), "delivery parent", 0, 2**64 - 1)
        container_expr = "NULL" if parent_uid == 0 else variables.get(parent_uid, "")
        if not container_expr:
            raise ToolError("eligible plan is not in payload parent order")
        masks = {"name_hex": 1, "short_description_hex": 4, "description_hex": 2, "action_description_hex": 8}
        strings = []
        for field, mask in masks.items():
            strings.append(sql_blob(item[field], field) if item["string_mask"] & mask else "NULL")
        values = [sql_num(value, "item value", -2**31, 2**31 - 1) for value in item["values"]]
        bitvectors = [sql_num(value, "item bitvector", 0, 2**64 - 1) for value in item["bitvectors"]]
        columns = (
            "pid,vnum,equip_slot,container_id,quantity,weight,cost,timer,extra_flags,wear_flags,item_type,"
            "value0,value1,value2,value3,value4,value5,value6,value7,name,short_descr,description,action_descr,"
            "bitvector1,bitvector2,bitvector3,bitvector4,bitvector5,item_material,obj_uid,item_condition"
        )
        row_values = [
            str(recipient_pid), sql_num(item["vnum"], "item vnum", 1, 2**31 - 1), "0", container_expr, "1",
            sql_num(item["weight"], "item weight", -2**31, 2**31 - 1), sql_num(item["cost"], "item cost", -2**31, 2**31 - 1),
            sql_num(item["timers"][0], "item timer", -2**63, 2**63 - 1), sql_num(item["extra_flags"], "extra flags", 0, 2**32 - 1),
            sql_num(item["wear_flags"], "wear flags", 0, 2**32 - 1), sql_num(item["type"], "item type", -128, 127),
            *values, *strings, *bitvectors, sql_num(item["material"], "item material", -128, 127),
            sql_num(uid, "item UID", 1, 2**64 - 1), sql_num(item["condition"], "item condition", -32768, 32767),
        ]
        lines.append(
            "INSERT INTO player_items(" + columns + ") SELECT " + ",".join(row_values) +
            " WHERE @restitution_ok=1 AND @restitution_completed=0;"
        )
        lines.append("SET @player_rows_inserted=@player_rows_inserted+ROW_COUNT();")
        lines.append("SET " + variables[uid] + "=LAST_INSERT_ID();")
        seen_affects: set[tuple[int, int]] = set()
        for affect in item["affects"]:
            pair = (int_value(affect[0], "affect location", -32768, 32767),
                    int_value(affect[1], "affect modifier", -32768, 32767))
            if pair == (0, 0) or pair in seen_affects:
                continue
            seen_affects.add(pair)
            lines.append(
                "INSERT INTO player_item_affects(item_id,location,modifier) SELECT " + variables[uid] + "," +
                str(pair[0]) + "," + str(pair[1]) + " WHERE @restitution_ok=1 AND @restitution_completed=0;"
            )
            lines.append("SET @affect_rows_inserted=@affect_rows_inserted+ROW_COUNT();")
        seen_descriptions: set[tuple[bytes, bytes]] = set()
        for description in item["extra_descriptions"]:
            keyword = hex_bytes(description["keyword_hex"], "extra-description keyword")
            text = hex_bytes(description["description_hex"], "extra-description text")
            if description.get("spellbook"):
                keyword = b"SPELLBOOK"
                text = ("[" + ",".join(str(int_value(value, "spell ID", -2**31, 2**31 - 1))
                                            for value in description.get("spell_ids", [])) + "]").encode()
            pair = (keyword, text)
            if not keyword or pair in seen_descriptions:
                continue
            seen_descriptions.add(pair)
            lines.append(
                "INSERT INTO player_item_extra_descr(item_id,keyword,description) SELECT " + variables[uid] + "," +
                sql_blob(keyword, "extra-description keyword") + "," + sql_blob(text, "extra-description text") +
                " WHERE @restitution_ok=1 AND @restitution_completed=0;"
            )
            lines.append("SET @description_rows_inserted=@description_rows_inserted+ROW_COUNT();")
        payload = hex_bytes(item["item_payload_hex"], "item payload")
        metadata_digest = hex_bytes(row["metadata_digest"], "metadata digest")
        lines.append(
            "INSERT INTO player_death_restitution_delivery(item_uid,restitution_id,source_pid,death_revision,"
            "recipient_pid,source_item_revision,delivered_item_revision,delivered_item_id,metadata_digest,original_payload) "
            "SELECT " + str(uid) + "," + sql_blob(rid) + "," + str(source_pid) + "," + str(death_revision) + "," +
            str(recipient_pid) + "," + sql_num(row["source_item_revision"], "source item revision", 0, 2**64 - 1) + "," +
            sql_num(int_value(row["expected_current"]["item_revision"], "current item revision", 0, 2**64 - 2) + 1,
                    "delivered item revision", 1, 2**64 - 1) + "," + variables[uid] + "," + sql_blob(metadata_digest) + "," +
            sql_blob(payload) + " WHERE @restitution_ok=1 AND @restitution_completed=0;"
        )
        lines.append("SET @delivery_rows_inserted=@delivery_rows_inserted+ROW_COUNT();")
        lines.append(
            "INSERT INTO player_death_restitution_runtime(item_uid,recipient_pid,state_payload,state_digest) SELECT " +
            str(uid) + "," + str(recipient_pid) + "," + sql_blob(payload) + "," + sql_blob(metadata_digest) +
            " WHERE @restitution_ok=1 AND @restitution_completed=0;"
        )
        lines.append("SET @runtime_rows_inserted=@runtime_rows_inserted+ROW_COUNT();")
        if row.get("kind") == "artifact":
            vnum = sql_num(item["vnum"], "artifact vnum", 1, 2**31 - 1)
            reconciliation = row["artifact_reconciliation"]
            domain = row.get("artifact_before")
            delivered_revision = str(int_value(row["expected_current"]["item_revision"], "current item revision", 0, 2**64 - 2) + 1)
            timing = row.get("artifact_timing")
            if not isinstance(timing, dict):
                raise ToolError("eligible artifact has no delivery lifetime evidence")
            artifact_timer_expr = "(@restitution_delivery_epoch+" + sql_num(
                timing.get("usable_lifetime_seconds"),
                "artifact usable lifetime",
                1,
                MAX_ARTIFACT_COMPENSATION_SECONDS,
            ) + ")"
            if row.get("artifact_reconciliation_required") and not isinstance(domain, dict):
                seed = reconciliation["domain_seed"]
                baseline_seed = reconciliation["baseline_seed"]
                if not isinstance(reconciliation.get("baseline_before"), dict):
                    lines.append(
                        "INSERT INTO artifact_domain_baseline(vnum,opening_timer_epoch,opening_bind_owner_pid,"
                        "opening_bind_timer_epoch,opening_revision) SELECT " + vnum + "," +
                        sql_num(baseline_seed["opening_timer_epoch"], "artifact opening timer", -2**63, 2**63 - 1) + "," +
                        sql_num(baseline_seed["opening_bind_owner_pid"], "artifact opening bind owner", -2**31, 2**31 - 1) + "," +
                        sql_num(baseline_seed["opening_bind_timer_epoch"], "artifact opening bind timer", -2**63, 2**63 - 1) + "," +
                        sql_num(baseline_seed["opening_revision"], "artifact opening revision", 0, 2**64 - 1) +
                        " WHERE @restitution_ok=1 AND @restitution_completed=0 AND (SELECT COUNT(*) FROM artifact_domain_baseline WHERE vnum=" + vnum + ")=0;"
                    )
                    lines.append("SET @artifact_baseline_rows_inserted=@artifact_baseline_rows_inserted+ROW_COUNT();")
                lines.append(
                    "INSERT INTO artifact_domain_state(vnum,owned,loc_type,location,timer_epoch,artifact_type,"
                    "bind_owner_pid,bind_timer_epoch,item_uid,item_revision,revision) SELECT " + vnum +
                    ",1,3," + str(recipient_pid) + "," +
                    artifact_timer_expr + "," +
                    sql_num(seed["artifact_type"], "artifact type", 0, 255) + "," +
                    sql_num(seed["bind_owner_pid"], "artifact bind owner", -2**31, 2**31 - 1) + "," +
                    sql_num(seed["bind_timer_epoch"], "artifact bind timer", -2**63, 2**63 - 1) + "," +
                    str(uid) + "," + delivered_revision + "," +
                    str(int_value(seed.get("revision"), "artifact revision", 0, 2**64 - 2) + 1) +
                    " WHERE @restitution_ok=1 AND @restitution_completed=0 AND (SELECT COUNT(*) FROM artifact_domain_state WHERE vnum=" + vnum + ")=0;"
                )
                lines.append("SET @artifact_domain_rows_inserted=@artifact_domain_rows_inserted+ROW_COUNT();")
            else:
                if not isinstance(domain, dict):
                    raise ToolError("eligible artifact has no canonical state evidence")
                old_uid = domain.get("item_uid")
                old_uid_fence = "item_uid IS NULL" if old_uid is None else (
                    "item_uid=" + sql_num(old_uid, "artifact domain UID", 0, 2**64 - 1)
                )
                lines.append(
                    "UPDATE artifact_domain_state SET loc_type=3,location=" + str(recipient_pid) +
                    ",timer_epoch=" + artifact_timer_expr +
                    ",item_uid=" + str(uid) + ",item_revision=" + delivered_revision + ",revision=revision+1 WHERE @restitution_ok=1 "
                    "AND @restitution_completed=0 AND vnum=" + vnum + domain_fence + ";"
                )
                lines.append("SET @artifact_domain_rows_updated=@artifact_domain_rows_updated+ROW_COUNT();")
            for legacy_name in row.get("artifact_legacy_before", {}):
                table = "artifacts_mortal" if legacy_name == "mortal" else "artifacts"
                legacy = row["artifact_legacy_before"][legacy_name]
                lines.append(
                    "UPDATE " + table + " SET locType=3,location=" + str(recipient_pid) +
                    ",timer=FROM_UNIXTIME(" + artifact_timer_expr + "),lastUpdate=DATE_ADD(CURRENT_TIMESTAMP,INTERVAL 1 SECOND) WHERE @restitution_ok=1 AND @restitution_completed=0 "
                    "AND vnum=" + vnum + " AND owned='Y' AND locType=" +
                    sql_num(legacy["loc_type"], "legacy artifact location type", 0) +
                    " AND location=" + sql_num(legacy["location"], "legacy artifact location", -2**31, 2**31 - 1) +
                    " AND COALESCE(UNIX_TIMESTAMP(timer),0)=" + sql_num(legacy["timer"], "legacy artifact timer", 0) +
                    " AND type=" + sql_num(legacy["artifact_type"], "legacy artifact type", 0) + ";"
                )
                lines.append("SET @artifact_legacy_rows_updated=@artifact_legacy_rows_updated+ROW_COUNT();")
    expected_affects = 0
    expected_descriptions = 0
    for row in eligible:
        item = row["metadata"]
        expected_affects += len({
            (int_value(affect[0], "affect location", -32768, 32767),
             int_value(affect[1], "affect modifier", -32768, 32767))
            for affect in item["affects"]
            if (int_value(affect[0], "affect location", -32768, 32767),
                int_value(affect[1], "affect modifier", -32768, 32767)) != (0, 0)
        })
        description_pairs: set[tuple[bytes, bytes]] = set()
        for description in item["extra_descriptions"]:
            keyword = hex_bytes(description["keyword_hex"], "extra-description keyword")
            text = hex_bytes(description["description_hex"], "extra-description text")
            if description.get("spellbook"):
                keyword = b"SPELLBOOK"
                text = ("[" + ",".join(str(int_value(value, "spell ID", -2**31, 2**31 - 1))
                                            for value in description.get("spell_ids", [])) + "]").encode()
            if keyword:
                description_pairs.add((keyword, text))
        expected_descriptions += len(description_pairs)
    artifact_rows = [row for row in eligible if row.get("kind") == "artifact"]
    expected_domain_inserts = sum(
        1 for row in artifact_rows
        if row.get("artifact_reconciliation_required") and not isinstance(row.get("artifact_before"), dict)
    )
    expected_domain_updates = len(artifact_rows) - expected_domain_inserts
    expected_baseline_inserts = sum(
        1 for row in artifact_rows
        if row.get("artifact_reconciliation_required") and
        not isinstance((row.get("artifact_reconciliation") or {}).get("baseline_before"), dict)
    )
    expected_legacy_updates = sum(len(row.get("artifact_legacy_before", {})) for row in artifact_rows)
    lines.extend([
        f"SET @restitution_owner=(IFNULL(IS_USED_LOCK({RUNTIME_EXCLUSION_LOCK_EXPRESSION}),0)=CONNECTION_ID());",
        "SET @restitution_ok=@restitution_ok AND @restitution_owner=1;",
        "SET @restitution_ok=@restitution_ok AND (@restitution_completed=1 OR ("
        "@receipt_inserted=1 AND @receipt_items_inserted=" + str(len(all_items)) +
        " AND @player_rows_inserted=" + str(n) + " AND @delivery_rows_inserted=" + str(n) +
        " AND @runtime_rows_inserted=" + str(n) + " AND @affect_rows_inserted=" + str(expected_affects) +
        " AND @description_rows_inserted=" + str(expected_descriptions) +
        " AND @owner_revision_updated=1 AND @owner_rows_updated=" + str(n) +
        " AND @artifact_domain_rows_inserted=" + str(expected_domain_inserts) +
        " AND @artifact_domain_rows_updated=" + str(expected_domain_updates) +
        " AND @artifact_baseline_rows_inserted=" + str(expected_baseline_inserts) +
        " AND @artifact_legacy_rows_updated=" + str(expected_legacy_updates) + "));",
        "SET @restitution_decision=IF(@restitution_ok=1,'COMMIT','ROLLBACK');",
        "PREPARE restitution_decision_stmt FROM @restitution_decision;",
        "EXECUTE restitution_decision_stmt;",
        "DEALLOCATE PREPARE restitution_decision_stmt;",
        "SELECT CONCAT('DURIS_RESULT|',IFNULL(@restitution_ok,0),'|',IFNULL(@restitution_completed,0),'|',"
        "IFNULL(@restitution_lock,0),'|'," + str(n) + ");",
        f"DO RELEASE_LOCK({RUNTIME_EXCLUSION_LOCK_EXPRESSION});",
    ])
    return "\n".join(lines)


def apply_plan(
    db: Mysql, plan: dict[str, Any], proof: Path, actor: str, reason: str,
    approve_artifact_reconciliation: bool = False, *, policy: TargetPolicy | None = None,
    target_info: Mapping[str, Any] | None = None,
    approve_artifact_timing_compensation: bool = False,
) -> str:
    if not plan.get("applyable"):
        raise ToolError("plan is not applyable; refused classifications or missing reconciliation approval remain")
    validate_artifact_timing_compensations(plan)
    if policy is None:
        policy = getattr(db, "policy", None)
    plan_target = plan.get("target")
    actual_target: dict[str, Any] | None = None
    boundary: dict[str, str] | None = None
    if plan_target is not None:
        if policy is None:
            raise ToolError("target-pinned apply requires an explicit target policy")
        actual_target, boundary = validate_plan_target_controls(
            db, plan, policy, require_maintenance=policy.production, target_info=target_info
        )
        if actual_target is None:
            raise ToolError("approved plan target could not be read back")
        if actual_target["production"]:
            if plan.get("production_approved") is not True:
                raise ToolError("production apply requires a plan approved for production")
            validate_plan_backup(plan, actual_target, boundary)
    elif policy is not None and policy.production:
        raise ToolError("production apply requires a target-pinned plan")
    reconciliation_count = int_value(plan.get("artifact_reconciliation_count", 0), "artifact reconciliation count", 0)
    if reconciliation_count:
        if not plan.get("artifact_reconciliation_approved") or not approve_artifact_reconciliation:
            raise ToolError(
                "artifact identity reconciliation requires explicit approval in both plan and apply"
            )
    timing_compensation_count = int_value(
        plan.get("artifact_timing_compensation_count", 0),
        "artifact timing compensation count",
        0,
    )
    if timing_compensation_count:
        if (not plan.get("artifact_timing_compensation_approved") or
                not approve_artifact_timing_compensation):
            raise ToolError(
                "artifact timing compensation requires explicit approval in both plan and apply"
            )
    check_quiescence(db, proof)
    existing = fetch_receipt(db, plan["restitution_id_hex"])
    if existing is not None:
        if (existing["source_pid"] != plan["source"]["pid"] or
                existing["death_revision"] != plan["source"]["death_revision"] or
                existing["recipient_pid"] != plan["recipient_pid"] or
                existing["death_operation_id_hex"] != plan["source"]["operation_id_hex"] or
                existing["evidence_digest"] != plan["evidence_digest"] or
                existing["plan_digest"] != plan["plan_digest"]):
            raise ToolError("restitution identity collides with a different receipt")
        if existing["status"] in {RECEIPT_APPLIED, RECEIPT_VERIFIED}:
            return "already applied"
    # Re-read the selected death and all evidence before opening the mutation transaction.
    source_pid = int_value(plan["source"]["pid"], "source pid", 1, 2**31 - 1)
    revision = int_value(plan["source"]["death_revision"], "death revision", 1, 2**64 - 1)
    fresh = build_inspection(
        db, source_pid, revision, int_value(plan["recipient_pid"], "recipient pid", 1), actual_target
    )
    if fresh["evidence_digest"] != plan["evidence_digest"]:
        raise ToolError("plan is stale: evidence or expected current revisions changed")
    fresh_plan = plan_from_inspection(
        fresh,
        bool(plan.get("artifact_reconciliation_approved")),
        target_info=(
            {"target": plan["target"], "maintenance_boundary": plan.get("maintenance_boundary")}
            if plan_target is not None else None
        ),
        backup_receipt=plan.get("backup_receipt"),
        approve_production=bool(plan.get("production_approved")),
        artifact_timing_compensations=plan.get("artifact_timing_compensations"),
    )
    if fresh_plan["plan_digest"] != plan["plan_digest"] or fresh_plan["restitution_id_hex"] != plan["restitution_id_hex"]:
        raise ToolError("plan is stale: classification or selection changed")
    sql = build_apply_sql(plan, actor, reason)
    rows = db.run(sql)
    marker = next((row[0] for row in reversed(rows) if row and row[0].startswith("DURIS_RESULT|")), None)
    if marker is None:
        raise ToolError("apply transaction returned no durable result")
    parts = marker.split("|")
    if len(parts) != 5:
        raise ToolError("apply transaction returned an invalid durable result")
    ok, completed, lock_ok = (int_value(parts[index], "apply result", 0, 1) for index in (1, 2, 3))
    if not lock_ok:
        raise ToolError("database restitution lock could not be acquired")
    if not ok:
        raise ToolError("apply refused: current ownership or evidence fence did not match")
    receipt_after = fetch_receipt(db, plan["restitution_id_hex"])
    if receipt_after is None or receipt_after["status"] not in {RECEIPT_APPLIED, RECEIPT_VERIFIED}:
        raise ToolError("apply transaction did not persist an applied restitution receipt")
    if receipt_after["delivered_count"] != len(eligible_items(plan)):
        raise ToolError("apply transaction receipt delivered count is inconsistent")
    if len(fetch_delivery_rows(db, plan["restitution_id_hex"])) != len(eligible_items(plan)):
        raise ToolError("apply transaction delivery rows are incomplete")
    if completed:
        return "already applied"
    return "applied"


# ---------- exact post-apply verification ----------


def fetch_receipt(db: Mysql, rid_hex: str) -> dict[str, Any] | None:
    rows = db.run(
        "SELECT HEX(restitution_id),source_pid,death_revision,recipient_pid,HEX(death_operation_id),"
        "HEX(evidence_digest),HEX(plan_digest),status,actor,reason,candidate_count,delivered_count,"
        "unresolved_count FROM player_death_restitution_receipt WHERE restitution_id=" + sql_blob(rid_hex)
    )
    if not rows:
        return None
    row = rows[0]
    if len(row) != 13:
        raise ToolError("restitution receipt has an unexpected shape")
    return {
        "restitution_id_hex": (row_value(row, 0) or "").lower(), "source_pid": row_int(row, 1, "receipt source", 1),
        "death_revision": row_int(row, 2, "receipt revision", 1), "recipient_pid": row_int(row, 3, "receipt recipient", 1),
        "death_operation_id_hex": (row_value(row, 4) or "").lower(), "evidence_digest": hex_digest(row_value(row, 5)),
        "plan_digest": hex_digest(row_value(row, 6)), "status": row_int(row, 7, "receipt status", 1),
        "actor": "redacted", "reason": "redacted", "candidate_count": row_int(row, 10, "receipt candidate count", 0),
        "delivered_count": row_int(row, 11, "receipt delivered count", 0),
        "unresolved_count": row_int(row, 12, "receipt unresolved count", 0),
    }


def fetch_delivery_rows(db: Mysql, rid_hex: str) -> list[dict[str, Any]]:
    rows = db.run(
        "SELECT item_uid,source_pid,death_revision,recipient_pid,source_item_revision,"
        "delivered_item_revision,delivered_item_id,HEX(metadata_digest),HEX(original_payload) "
        "FROM player_death_restitution_delivery WHERE restitution_id=" + sql_blob(rid_hex) + " ORDER BY item_uid"
    )
    result = []
    for row in rows:
        if len(row) != 9:
            raise ToolError("delivery readback has an unexpected shape")
        result.append({
            "item_uid": row_int(row, 0, "delivery UID", 1), "source_pid": row_int(row, 1, "delivery source", 1),
            "death_revision": row_int(row, 2, "delivery revision", 1), "recipient_pid": row_int(row, 3, "delivery recipient", 1),
            "source_item_revision": row_int(row, 4, "delivery source revision", 0),
            "delivered_item_revision": row_int(row, 5, "delivery item revision", 1),
            "delivered_item_id": row_int(row, 6, "delivery database row", 1),
            "metadata_digest": hex_digest(row_value(row, 7), "delivery metadata digest"),
            "original_payload_hex": (row_value(row, 8) or "").lower(),
        })
    return result


def fetch_restitution_item_rows(db: Mysql, rid_hex: str) -> dict[str, dict[str, Any]]:
    rows = db.run(
        "SELECT item_uid,artifact_vnum,artifact_loss_epoch,artifact_source_timer_epoch,"
        "artifact_usable_lifetime_seconds,artifact_delivered_timer_epoch,artifact_timing_basis,"
        "artifact_compensation_reference FROM player_death_restitution_item WHERE restitution_id=" +
        sql_blob(rid_hex) + " ORDER BY item_uid"
    )
    result: dict[str, dict[str, Any]] = {}
    for row in rows:
        if len(row) != 8:
            raise ToolError("restitution item readback has an unexpected shape")
        uid = row_int(row, 0, "restitution item UID", 1)
        if uid is None:
            raise ToolError("restitution item readback has no UID")
        result[str(uid)] = {
            "item_uid": uid,
            "artifact_vnum": row_int(row, 1, "receipt artifact vnum", 0),
            "artifact_loss_epoch": row_int(row, 2, "receipt artifact loss epoch", 0),
            "artifact_source_timer_epoch": row_int(row, 3, "receipt artifact source timer", 0),
            "artifact_usable_lifetime_seconds": row_int(row, 4, "receipt artifact lifetime", 0),
            "artifact_delivered_timer_epoch": row_int(row, 5, "receipt artifact delivery timer", 0),
            "artifact_timing_basis": row_value(row, 6) or "",
            "artifact_compensation_reference": row_value(row, 7) or "",
        }
    return result


def fetch_player_rows(db: Mysql, pid: int, uids: Iterable[int]) -> dict[str, dict[str, Any]]:
    values = sorted({int_value(uid, "item UID", 1, 2**64 - 1) for uid in uids})
    if not values:
        return {}
    rows = db.run(
        "SELECT id,pid,vnum,equip_slot,COALESCE(container_id,0),quantity,weight,cost,timer,extra_flags,"
        "wear_flags,item_type,value0,value1,value2,value3,value4,value5,value6,value7,HEX(name),HEX(short_descr),"
        "HEX(description),HEX(action_descr),bitvector1,bitvector2,bitvector3,bitvector4,"
        "bitvector5,item_material,obj_uid,item_condition FROM player_items WHERE pid=" + str(pid) +
        " AND obj_uid IN " + sql_list(values) + " ORDER BY id"
    )
    result: dict[str, dict[str, Any]] = {}
    for row in rows:
        if len(row) != 32:
            raise ToolError("player item readback has an unexpected shape")
        uid = row_int(row, 30, "player UID", 1)
        if uid is None:
            raise ToolError("player item readback has no UID")
        result[str(uid)] = {
            "id": row_int(row, 0, "player row", 1), "pid": row_int(row, 1, "player pid", 1),
            "vnum": row_int(row, 2, "player vnum", 1), "equip_slot": row_int(row, 3, "equipment slot", -32768),
            "container_id": row_int(row, 4, "container ID", 0), "quantity": row_int(row, 5, "quantity", 1),
            "weight": row_int(row, 6, "weight", 0), "cost": row_int(row, 7, "cost", 0),
            "timer": row_int(row, 8, "timer", 0), "extra_flags": row_int(row, 9, "extra flags", 0),
            "wear_flags": row_int(row, 10, "wear flags", 0), "type": row_int(row, 11, "item type", 0),
            "values": [row_int(row, index, "item value", 0) for index in range(12, 20)],
            "name_hex": (row_value(row, 20) or "").lower() or None,
            "short_description_hex": (row_value(row, 21) or "").lower() or None,
            "description_hex": (row_value(row, 22) or "").lower() or None,
            "action_description_hex": (row_value(row, 23) or "").lower() or None,
            "bitvectors": [row_int(row, index, "item bitvector", 0) if row_value(row, index) is not None else None
                           for index in range(24, 29)],
            "material": row_int(row, 29, "item material", 0),
            "object_uid": uid, "condition": row_int(row, 31, "item condition", 0),
        }
    return result


def fetch_item_metadata(db: Mysql, uids: Iterable[int]) -> tuple[dict[int, set[tuple[int, int]]], dict[int, set[tuple[bytes, bytes]]]]:
    values = sorted({int_value(uid, "item UID", 1, 2**64 - 1) for uid in uids})
    if not values:
        return {}, {}
    rows = db.run(
        "SELECT ia.item_id,ia.location,ia.modifier FROM player_item_affects ia JOIN player_items pi ON "
        "pi.id=ia.item_id WHERE pi.obj_uid IN " + sql_list(values) + " ORDER BY ia.item_id,ia.id"
    )
    affects: dict[int, set[tuple[int, int]]] = {}
    item_ids: dict[int, int] = {}
    player_rows = fetch_player_rows_for_ids(db, values)
    for uid, row in player_rows.items():
        item_ids[row["id"]] = uid
    for row in rows:
        item_id = row_int(row, 0, "affect item", 1)
        if item_id not in item_ids:
            continue
        affects.setdefault(item_ids[item_id], set()).add((row_int(row, 1, "affect location", 0), row_int(row, 2, "affect modifier", 0)))
    rows = db.run(
        "SELECT ed.item_id,HEX(ed.keyword),HEX(ed.description) FROM player_item_extra_descr ed JOIN player_items pi "
        "ON pi.id=ed.item_id WHERE pi.obj_uid IN " + sql_list(values) + " ORDER BY ed.item_id,ed.id"
    )
    descriptions: dict[int, set[tuple[bytes, bytes]]] = {}
    for row in rows:
        item_id = row_int(row, 0, "description item", 1)
        if item_id not in item_ids:
            continue
        descriptions.setdefault(item_ids[item_id], set()).add((hex_bytes(row_value(row, 1), "keyword"), hex_bytes(row_value(row, 2), "description")))
    return affects, descriptions


def fetch_player_rows_for_ids(db: Mysql, uids: Iterable[int]) -> dict[int, dict[str, Any]]:
    values = sorted({int_value(uid, "item UID", 1, 2**64 - 1) for uid in uids})
    if not values:
        return {}
    rows = db.run("SELECT id,obj_uid FROM player_items WHERE obj_uid IN " + sql_list(values))
    result = {}
    for row in rows:
        result[row_int(row, 1, "item UID", 1)] = {"id": row_int(row, 0, "item row", 1)}
    return result


def fetch_runtime_state(db: Mysql, uids: Iterable[int]) -> dict[str, str]:
    values = sorted({int_value(uid, "item UID", 1, 2**64 - 1) for uid in uids})
    if not values:
        return {}
    rows = db.run(
        "SELECT item_uid,HEX(state_payload) FROM player_death_restitution_runtime WHERE item_uid IN " + sql_list(values)
    )
    return {str(row_int(row, 0, "runtime UID", 1)): (row_value(row, 1) or "").lower() for row in rows}


def verify_plan(
    db: Mysql, plan: dict[str, Any], policy: TargetPolicy | None = None,
    target_info: Mapping[str, Any] | None = None,
) -> tuple[bool, int, list[str]]:
    if policy is None:
        policy = getattr(db, "policy", None)
    plan_target = plan.get("target")
    if plan_target is not None:
        if policy is None:
            raise ToolError("target-pinned verification requires an explicit target policy")
        actual_target, boundary = validate_plan_target_controls(
            db, plan, policy, require_maintenance=policy.production, target_info=target_info
        )
        if actual_target is None:
            raise ToolError("approved plan target could not be read back")
        if actual_target["production"]:
            validate_plan_backup(plan, actual_target, boundary)
    elif policy is not None and policy.production:
        raise ToolError("production verification requires a target-pinned plan")
    rid = plan["restitution_id_hex"]
    receipt = fetch_receipt(db, rid)
    if receipt is None:
        raise ToolError("restitution receipt was not found")
    failures: list[str] = []
    for field in ("source_pid", "death_revision", "recipient_pid", "death_operation_id_hex", "evidence_digest", "plan_digest"):
        expected = plan["source"].get("pid") if field == "source_pid" else (
            plan["source"].get("death_revision") if field == "death_revision" else (
                plan.get("recipient_pid") if field == "recipient_pid" else (
                    plan["source"].get("operation_id_hex") if field == "death_operation_id_hex" else plan.get(field))))
        if receipt[field] != expected:
            failures.append("receipt identity or digest differs from plan")
    if receipt["candidate_count"] != len(plan["items"]):
        failures.append("receipt candidate count differs")
    eligible = eligible_items(plan)
    if receipt["delivered_count"] != len(eligible):
        failures.append("receipt delivered count differs")
    delivery_rows = fetch_delivery_rows(db, rid)
    receipt_item_rows = fetch_restitution_item_rows(db, rid)
    if len(delivery_rows) != len(eligible):
        failures.append("delivery receipt count differs")
    delivery_by_uid = {str(row["item_uid"]): row for row in delivery_rows}
    uids = [int(row["item_uid"]) for row in eligible]
    players = fetch_player_rows(db, int_value(plan["recipient_pid"], "recipient pid", 1), uids)
    owners = fetch_current_owners(db, uids)
    runtimes = fetch_runtime_state(db, uids)
    affects, descriptions = fetch_item_metadata(db, uids)
    artifact_vnums = sorted({int_value(row["vnum"], "artifact vnum", 1) for row in eligible if row.get("kind") == "artifact"})
    artifacts_after = fetch_artifacts(db, artifact_vnums) if artifact_vnums else None
    id_by_uid = {int(uid): row["id"] for uid, row in players.items()}
    before = {int_value(uid, "existing recipient UID", 1) for uid in plan.get("recipient_existing_uids", [])}
    after = set(fetch_recipient_uids(db, int_value(plan["recipient_pid"], "recipient pid", 1)))
    if not before.issubset(after):
        failures.append("newer recipient inventory was not preserved")
    if not after.issuperset(before | set(uids)):
        failures.append("recipient inventory is missing an expected UID")
    if len(after) < len(before) + len(uids):
        failures.append("recipient inventory contains a duplicate or missing UID")
    player_by_uid = {str(uid): row for uid, row in players.items()}
    for row in eligible:
        uid = int_value(row["item_uid"], "item UID", 1)
        key = str(uid)
        player = player_by_uid.get(key)
        delivery = delivery_by_uid.get(key)
        owner = owners.get(key)
        if player is None or delivery is None or owner is None:
            failures.append("delivered UID is missing from one authority")
            continue
        if delivery["metadata_digest"] != row["metadata_digest"]:
            failures.append("delivery metadata digest differs")
        if digest_bytes(hex_bytes(delivery["original_payload_hex"], "original payload")) != row["metadata_digest"]:
            failures.append("original payload digest differs")
        if owner["owner_type"] != OWNER_PLAYER or owner["owner_id"] != plan["recipient_pid"] or owner["state"] != STATE_ACTIVE:
            failures.append("current owner does not point to the recipient")
        if owner["root_item_uid"] != row["delivered_root_item_uid"] or owner["parent_item_uid"] != row["delivered_parent_item_uid"]:
            failures.append("current ownership topology differs from the plan")
        item = row["metadata"]
        expected_strings = {
            "name_hex": item["name_hex"] if item["string_mask"] & 1 else None,
            "short_description_hex": item["short_description_hex"] if item["string_mask"] & 4 else None,
            "description_hex": item["description_hex"] if item["string_mask"] & 2 else None,
            "action_description_hex": item["action_description_hex"] if item["string_mask"] & 8 else None,
        }
        for field, expected in expected_strings.items():
            if (player.get(field) or None) != (expected or None):
                failures.append("stored item string metadata differs")
        scalar_fields = {
            "vnum": item["vnum"], "equip_slot": 0, "quantity": 1, "weight": item["weight"], "cost": item["cost"],
            "timer": item["timers"][0], "extra_flags": item["extra_flags"], "wear_flags": item["wear_flags"],
            "type": item["type"], "values": item["values"], "condition": item["condition"],
            "object_uid": uid,
        }
        for field, expected in scalar_fields.items():
            if player.get(field) != expected:
                failures.append("stored item scalar metadata differs")
        for index, expected in enumerate(item["bitvectors"]):
            if player.get("bitvectors", [None] * 5)[index] != expected:
                failures.append("stored item bitvector metadata differs")
        parent_uid = int_value(row.get("delivered_parent_item_uid", 0), "delivery parent", 0)
        expected_container = 0 if parent_uid == 0 else id_by_uid.get(parent_uid, -1)
        if player.get("container_id") != expected_container:
            failures.append("player container topology differs")
        expected_affects: set[tuple[int, int]] = set()
        for affect in item["affects"]:
            pair = (int(affect[0]), int(affect[1]))
            if pair != (0, 0):
                expected_affects.add(pair)
        if affects.get(uid, set()) != expected_affects:
            failures.append("item affects differ")
        expected_descriptions: set[tuple[bytes, bytes]] = set()
        for description in item["extra_descriptions"]:
            key_bytes = hex_bytes(description["keyword_hex"], "description keyword")
            text_bytes = hex_bytes(description["description_hex"], "description text")
            if description.get("spellbook"):
                key_bytes = b"SPELLBOOK"
                text_bytes = ("[" + ",".join(str(value) for value in description.get("spell_ids", [])) + "]").encode()
            expected_descriptions.add((key_bytes, text_bytes))
        if descriptions.get(uid, set()) != expected_descriptions:
            failures.append("item extra descriptions differ")
        if runtimes.get(key) != item["item_payload_hex"]:
            failures.append("exact runtime metadata payload differs")
        if row.get("kind") == "artifact":
            receipt_timing = receipt_item_rows.get(key)
            timing = row.get("artifact_timing") or {}
            if not isinstance(receipt_timing, dict):
                failures.append("artifact timing receipt is missing")
            else:
                expected_timing = {
                    "artifact_vnum": int_value(item["vnum"], "artifact vnum", 1),
                    "artifact_loss_epoch": int_value(timing.get("loss_epoch", 0), "artifact loss epoch", 0),
                    "artifact_source_timer_epoch": int_value(timing.get("source_timer_epoch", 0), "artifact source timer", 0),
                    "artifact_usable_lifetime_seconds": int_value(timing.get("usable_lifetime_seconds", 0), "artifact lifetime", 1),
                    "artifact_timing_basis": timing.get("basis", ""),
                    "artifact_compensation_reference": timing.get("compensation_reference", ""),
                }
                for field, expected in expected_timing.items():
                    if receipt_timing.get(field) != expected:
                        failures.append("artifact timing receipt differs from plan")
                        break
                if receipt_timing.get("artifact_delivered_timer_epoch", 0) <= 0:
                    failures.append("artifact delivery timer is missing")
            if not isinstance(artifacts_after, dict):
                failures.append("artifact authority readback is unavailable")
            else:
                actual_domain = artifacts_after["domain"].get(str(item["vnum"]))
                expected_domain = row.get("artifact_after")
                if isinstance(expected_domain, dict) and isinstance(receipt_timing, dict):
                    expected_domain = dict(expected_domain)
                    expected_domain["timer_epoch"] = receipt_timing.get("artifact_delivered_timer_epoch")
                if not isinstance(actual_domain, dict) or not isinstance(expected_domain, dict):
                    failures.append("canonical artifact state is missing after delivery")
                else:
                    for field in (
                        "owned", "loc_type", "location", "timer_epoch", "artifact_type",
                        "bind_owner_pid", "bind_timer_epoch", "item_uid", "item_revision", "revision",
                    ):
                        if actual_domain.get(field) != expected_domain.get(field):
                            failures.append("canonical artifact timer/binding/state differs")
                            break
                reconciliation = row.get("artifact_reconciliation", {})
                if row.get("artifact_reconciliation_required"):
                    baseline_expected = reconciliation.get("baseline_before") or reconciliation.get("baseline_seed")
                    actual_baseline = artifacts_after["baseline"].get(str(item["vnum"]))
                    if not isinstance(actual_baseline, dict) or not isinstance(baseline_expected, dict):
                        failures.append("canonical artifact baseline is missing after reconciliation")
                    else:
                        for field in (
                            "opening_timer_epoch", "opening_bind_owner_pid",
                            "opening_bind_timer_epoch", "opening_revision",
                        ):
                            if actual_baseline.get(field) != baseline_expected.get(field):
                                failures.append("canonical artifact baseline differs")
                                break
                    expected_bind = row.get("artifact_bind_before")
                    actual_bind = artifacts_after["bind"].get(str(item["vnum"]))
                    if isinstance(expected_bind, dict):
                        if not isinstance(actual_bind, dict) or (
                                actual_bind.get("owner_pid") != expected_bind.get("owner_pid") or
                                actual_bind.get("timer") != expected_bind.get("timer")):
                            failures.append("artifact binding metadata changed")
                    elif actual_bind is not None:
                        failures.append("unexpected artifact binding was created")
                for legacy_name in row.get("artifact_legacy_before", {}):
                    legacy_after = artifacts_after.get(legacy_name, {}).get(str(item["vnum"]))
                    if not isinstance(legacy_after, dict):
                        failures.append("legacy artifact tracking row is missing after delivery")
                        continue
                    legacy_before = row["artifact_legacy_before"][legacy_name]
                    if (
                            str(legacy_after.get("owned", "")).upper() != "Y" or
                            legacy_after.get("loc_type") != 3 or
                            legacy_after.get("location") != plan["recipient_pid"] or
                            (receipt_timing is not None and legacy_after.get("timer") != receipt_timing.get("artifact_delivered_timer_epoch")) or
                            legacy_after.get("artifact_type") != legacy_before.get("artifact_type")):
                        failures.append(
                            "legacy artifact timer/type/location metadata differs "
                            f"({legacy_name}: actual={legacy_after}, expected_timer="
                            f"{receipt_timing.get('artifact_delivered_timer_epoch') if receipt_timing else None})"
                        )
    if receipt["status"] not in {RECEIPT_APPLIED, RECEIPT_VERIFIED}:
        failures.append("receipt is not in an applied state")
    # A global UID guard is stronger than the selected death revision.
    duplicate_count = int_value(db.scalar(
        "SELECT COUNT(*)-COUNT(DISTINCT obj_uid) FROM player_items WHERE obj_uid IN " + sql_list(uids)
    ), "duplicate UID count", 0)
    if duplicate_count != 0:
        failures.append("duplicate player projection UID exists")
    return not failures, len(eligible), sorted(set(failures))


def mark_verified(
    db: Mysql, plan: dict[str, Any], proof: Path, *, policy: TargetPolicy | None = None,
    target_info: Mapping[str, Any] | None = None, approve_production: bool = False,
) -> None:
    if policy is None:
        policy = getattr(db, "policy", None)
    plan_target = plan.get("target")
    if plan_target is not None:
        if policy is None:
            raise ToolError("target-pinned verification requires an explicit target policy")
        actual_target, boundary = validate_plan_target_controls(
            db, plan, policy, require_maintenance=policy.production, target_info=target_info
        )
        if actual_target is None:
            raise ToolError("approved plan target could not be read back")
        if actual_target["production"]:
            if plan.get("production_approved") is not True or not approve_production:
                raise ToolError("production mark-verified requires explicit production approval")
            validate_plan_backup(plan, actual_target, boundary)
    elif policy is not None and policy.production:
        raise ToolError("production verification requires a target-pinned plan")
    check_quiescence(db, proof)
    rid = sql_blob(plan["restitution_id_hex"])
    rows = db.run(
        "SET autocommit=0; START TRANSACTION; "
        f"SELECT GET_LOCK({RUNTIME_EXCLUSION_LOCK_EXPRESSION},0) INTO @verify_lock; "
        f"SET @verify_owner=(IFNULL(IS_USED_LOCK({RUNTIME_EXCLUSION_LOCK_EXPRESSION}),0)=CONNECTION_ID()); "
        "SET @verify_ok=(@verify_lock=1 AND @verify_owner=1); "
        "SET @verify_status=0; "
        "SELECT status INTO @verify_status FROM player_death_restitution_receipt WHERE restitution_id=" + rid +
        " FOR UPDATE; "
        "SET @verify_ok=@verify_ok AND @verify_status IN (2,3); "
        "UPDATE player_death_restitution_receipt SET status=3,verified_at=CURRENT_TIMESTAMP(6) "
        "WHERE restitution_id=" + rid + " AND status=2 AND @verify_ok=1; "
        "SET @verify_updated=ROW_COUNT(); "
        f"SET @verify_owner=(IFNULL(IS_USED_LOCK({RUNTIME_EXCLUSION_LOCK_EXPRESSION}),0)=CONNECTION_ID()); "
        "SET @verify_ok=@verify_ok AND @verify_owner=1 AND (@verify_updated=1 OR @verify_status=3); "
        "SET @verify_decision=IF(@verify_ok=1,'COMMIT','ROLLBACK'); "
        "PREPARE verify_decision_stmt FROM @verify_decision; "
        "EXECUTE verify_decision_stmt; "
        "DEALLOCATE PREPARE verify_decision_stmt; "
        "SELECT CONCAT('DURIS_GUARD_RESULT|',IFNULL(@verify_ok,0),'|',IFNULL(@verify_lock,0)); "
        f"DO RELEASE_LOCK({RUNTIME_EXCLUSION_LOCK_EXPRESSION});"
    )
    marker = next((row[0] for row in reversed(rows) if row and row[0].startswith("DURIS_GUARD_RESULT|")), None)
    if marker is None or marker.split("|") != ["DURIS_GUARD_RESULT", "1", "1"]:
        raise ToolError("database restitution lock could not be acquired for verification")
    receipt = fetch_receipt(db, plan["restitution_id_hex"])
    if receipt is None or receipt["status"] != RECEIPT_VERIFIED:
        raise ToolError("receipt verification state could not be read back")


# ---------- command line ----------


def add_policy_arguments(
    command: argparse.ArgumentParser, *, target_info: bool = False,
    maintenance: bool = False,
) -> None:
    if target_info:
        command.add_argument("--target-info", type=Path)
    command.add_argument(
        "--confirm-production-target",
        help="repeat the exact production database name; never a prefix or alias",
    )
    command.add_argument(
        "--expected-fingerprint", "--server-fingerprint", dest="expected_fingerprint",
        help="pin the actual database server fingerprint",
    )
    if maintenance:
        command.add_argument("--maintenance-kind", choices=("docker", "systemd"))
        command.add_argument(
            "--maintenance-id",
            help="complete immutable container ID or duris-mud-production.service",
        )


def make_target_info(target: Mapping[str, Any], boundary: Mapping[str, Any] | None) -> dict[str, Any]:
    body: dict[str, Any] = {
        "artifact_version": TOOL_VERSION,
        "kind": TARGET_INFO_FORMAT,
        "target": target_record(target, label="target-info"),
        "maintenance_boundary": None if boundary is None else maintenance_record(boundary),
    }
    body["target_info_digest"] = digest_json(body)
    return body


def parser() -> argparse.ArgumentParser:
    root = argparse.ArgumentParser(description="SQL-only audited disputed-death item restitution")
    root.add_argument("--env-file", help="explicit database environment file")
    sub = root.add_subparsers(dest="command", required=True)
    target_info = sub.add_parser(
        "target-info", help="read the actual database identity and optional maintenance boundary"
    )
    target_info.add_argument("--artifact", type=Path)
    target_info.add_argument("--overwrite", action="store_true")
    add_policy_arguments(target_info, maintenance=True)
    inspect = sub.add_parser("inspect")
    inspect.add_argument("--pid", required=True, type=int)
    inspect.add_argument("--death-revision", required=True, type=int)
    inspect.add_argument("--recipient-pid", required=True, type=int)
    inspect.add_argument("--artifact", required=True, type=Path)
    inspect.add_argument("--overwrite", action="store_true")
    add_policy_arguments(inspect, target_info=True)
    plan = sub.add_parser("plan")
    plan.add_argument("--inspect", required=True, type=Path)
    plan.add_argument("--artifact", required=True, type=Path)
    plan.add_argument("--overwrite", action="store_true")
    plan.add_argument(
        "--approve-artifact-reconciliation", action="store_true",
        help="explicitly approve evidence-backed canonical artifact identity reconciliation",
    )
    plan.add_argument(
        "--artifact-timing-compensation", action="append", default=[], metavar="UID=SECONDS:APPROVAL",
        help="repeat per-artifact approved remaining lifetime when historical timing is unavailable",
    )
    plan.add_argument(
        "--target-info", required=False, type=Path,
        help="protected target-info probe used for production plan binding",
    )
    plan.add_argument(
        "--backup-receipt", required=False, type=Path,
        help="protected native backup receipt to bind into the plan",
    )
    plan.add_argument(
        "--approve-production", action="store_true",
        help="approve this exact target, boundary, and backup for production apply",
    )
    apply = sub.add_parser("apply")
    apply.add_argument("--plan", required=True, type=Path)
    apply.add_argument("--offline-proof", required=True, type=Path)
    apply.add_argument("--approve", action="store_true")
    apply.add_argument(
        "--approve-artifact-reconciliation", action="store_true",
        help="repeat approval for an artifact identity reconciliation plan",
    )
    apply.add_argument(
        "--approve-artifact-timing-compensation", action="store_true",
        help="repeat approval for explicitly compensated artifact lifetime",
    )
    apply.add_argument("--actor", required=True)
    apply.add_argument("--reason", required=True)
    add_policy_arguments(apply, target_info=True, maintenance=True)
    verify = sub.add_parser("verify")
    verify.add_argument("--plan", required=True, type=Path)
    verify.add_argument("--mark-verified", action="store_true")
    verify.add_argument("--offline-proof", type=Path)
    verify.add_argument(
        "--approve", "--approve-production", dest="approve_production", action="store_true",
        help="explicitly approve a production receipt status transition",
    )
    add_policy_arguments(verify, target_info=True, maintenance=True)
    backup = sub.add_parser("backup", help="create a protected native backup for a pinned target")
    backup.add_argument("--target-info", required=True, type=Path)
    backup.add_argument("--receipt", required=True, type=Path)
    backup.add_argument("--offline-proof", required=True, type=Path)
    add_policy_arguments(backup, maintenance=True)
    return root


def main(argv: list[str]) -> int:
    args = parser().parse_args(argv)
    load_env_file(args.env_file)
    if args.command == "target-info":
        policy, _ = policy_for_command(args)
        db = Mysql(policy)
        target = identify_target(db, policy, probe=True)
        boundary = None
        if args.maintenance_kind is not None:
            boundary = require_policy_maintenance(policy)
        artifact = make_target_info(target, boundary)
        if args.artifact is not None:
            atomic_write_json(args.artifact, artifact, args.overwrite)
            print(
                "target-info written: production=%s server_fingerprint=%s maintenance=%s"
                % (str(target["production"]).lower(), target["server_fingerprint"],
                   boundary["kind"] if boundary else "not-checked")
            )
        else:
            print(json.dumps(artifact, sort_keys=True))
        return 0
    if args.command == "inspect":
        target_info = load_target_info(args.target_info) if args.target_info is not None else None
        expected = target_info["target"] if target_info is not None else None
        policy, _ = policy_for_command(args, expected_target=expected)
        db = Mysql(policy)
        target = identify_target(db, policy, expected)
        pid = int_value(args.pid, "pid", 1, 2**31 - 1)
        revision = int_value(args.death_revision, "death revision", 1, 2**64 - 1)
        recipient = int_value(args.recipient_pid, "recipient pid", 1, 2**31 - 1)
        artifact = build_inspection(db, pid, revision, recipient, target)
        atomic_write_json(args.artifact, artifact, args.overwrite)
        print("inspection written: candidates=%d payload_digest=%s evidence_digest=%s" % (
            len(artifact["custody_db"]), artifact["payload_digest"], artifact["evidence_digest"]))
        return 0
    if args.command == "plan":
        inspection = load_inspection(args.inspect)
        target_info = load_target_info(args.target_info) if args.target_info is not None else None
        backup_receipt = load_backup_receipt(args.backup_receipt) if args.backup_receipt is not None else None
        plan = plan_from_inspection(
            inspection,
            args.approve_artifact_reconciliation,
            target_info=target_info,
            backup_receipt=backup_receipt,
            approve_production=args.approve_production,
            artifact_timing_compensations=parse_artifact_timing_compensation_specs(
                args.artifact_timing_compensation
            ),
        )
        atomic_write_json(args.artifact, plan, args.overwrite)
        print("plan written: candidates=%d eligible=%d unresolved=%d applyable=%s plan_digest=%s" % (
            plan["candidate_count"], plan["eligible_count"], plan["unresolved_count"],
            str(plan["applyable"]).lower(), plan["plan_digest"]))
        return 0
    if args.command == "apply":
        if not args.approve:
            raise ToolError("apply requires explicit --approve")
        plan = load_plan(args.plan)
        expected = plan.get("target")
        policy, target_info = policy_for_command(
            args,
            expected_target=expected,
            require_maintenance=bool(expected and expected.get("production")),
        )
        db = Mysql(policy)
        outcome = apply_plan(
            db, plan, args.offline_proof, args.actor, args.reason,
            args.approve_artifact_reconciliation,
            policy=policy, target_info=target_info,
            approve_artifact_timing_compensation=args.approve_artifact_timing_compensation,
        )
        print("restitution %s: eligible=%d unresolved=%d plan_digest=%s" % (
            outcome, plan["eligible_count"], plan["unresolved_count"], plan["plan_digest"]))
        return 0
    if args.command == "verify":
        plan = load_plan(args.plan)
        expected = plan.get("target")
        production = bool(expected and expected.get("production"))
        if args.mark_verified and production and not args.approve_production:
            raise ToolError("production mark-verified requires explicit --approve")
        policy, target_info = policy_for_command(
            args,
            expected_target=expected,
            require_maintenance=production,
        )
        db = Mysql(policy)
        verified, count, failures = verify_plan(db, plan, policy, target_info)
        if not verified:
            detail = "; ".join(failures) if failures else "no failure detail"
            raise ToolError("verification failed for %d exact delivery checks: %s" % (len(failures), detail))
        if args.mark_verified:
            if args.offline_proof is None:
                raise ToolError("--mark-verified requires --offline-proof")
            mark_verified(
                db, plan, args.offline_proof, policy=policy, target_info=target_info,
                approve_production=args.approve_production,
            )
        print("verification passed: delivered=%d newer_inventory_preserved=true plan_digest=%s" % (
            count, plan["plan_digest"]))
        return 0
    if args.command == "backup":
        target_info = load_target_info(args.target_info)
        expected = target_info["target"]
        policy, _ = policy_for_command(
            args, expected_target=expected, require_maintenance=bool(expected["production"])
        )
        db = Mysql(policy)
        actual = identify_target(db, policy, expected)
        boundary = require_policy_maintenance(policy)
        captured_boundary = target_info.get("maintenance_boundary")
        if captured_boundary is not None and boundary != captured_boundary:
            raise ToolError("current maintenance boundary differs from target-info")
        receipt = target_error(
            "native backup",
            lambda: create_backup(
                db, policy, args.receipt,
                lambda: check_quiescence(db, args.offline_proof),
            ),
        )
        if receipt.get("target") != actual or receipt.get("maintenance_boundary") != boundary:
            raise ToolError("native backup receipt failed exact target/boundary readback")
        print("backup written: dump=%s sha256=%s server_fingerprint=%s" % (
            receipt["dump_path"], receipt["sha256"], actual["server_fingerprint"]))
        return 0
    raise ToolError("unsupported command")


if __name__ == "__main__":
    try:
        raise SystemExit(main(sys.argv[1:]))
    except ToolError as exc:
        print("ERROR: " + str(exc), file=sys.stderr)
        raise SystemExit(2)
