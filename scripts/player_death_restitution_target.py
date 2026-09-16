"""Explicit production-target and maintenance checks for death restitution.

This module never stops/masks a service, stops a container, or changes a DB.
The caller must still use the native SQL exclusion/quiescence transaction.
A maintenance check alone is not permission to apply a recovery.
"""
from __future__ import annotations

from dataclasses import dataclass
import hashlib
import ipaddress
import json
import os
from pathlib import Path
import re
import stat
import subprocess
from typing import Any, Mapping


class TargetError(Exception):
    """Sanitized, operator-facing preflight failure."""


TARGET_INFO_FORMAT = "duris-death-restitution-target-v1"
BACKUP_RECEIPT_FORMAT = "duris-death-restitution-backup-v1"


def _run_json(command: list[str]) -> Any:
    try:
        result = subprocess.run(command, capture_output=True, text=True, timeout=20, check=False)
        if result.returncode:
            raise TargetError("maintenance boundary could not be inspected")
        return json.loads(result.stdout)
    except (OSError, subprocess.SubprocessError, ValueError) as exc:
        raise TargetError("maintenance boundary could not be inspected") from exc


def _canonical_digest(value: Any) -> str:
    data = json.dumps(value, sort_keys=True, ensure_ascii=True, separators=(",", ":")).encode()
    return hashlib.sha256(data).hexdigest()


def server_fingerprint(db: Any) -> str:
    """Read actual server identity, not just user-supplied connection labels."""
    rows = db.run("SELECT @@hostname,@@port,@@server_id,DATABASE(),VERSION(),CURRENT_USER()")
    if len(rows) != 1 or len(rows[0]) != 6 or rows[0][3] != db.database:
        raise TargetError("database identity could not be verified")
    uuid_rows = db.run("SHOW VARIABLES LIKE 'server_uuid'")
    if len(uuid_rows) > 1 or any(len(row) != 2 for row in uuid_rows):
        raise TargetError("database UUID response was invalid")
    return _canonical_digest({"server": rows[0], "uuid": uuid_rows})


def validate_target_record(value: Any, *, label: str = "target") -> dict[str, Any]:
    """Validate the non-secret target identity carried by an approval artifact."""
    if not isinstance(value, dict):
        raise TargetError(f"{label} identity is missing")
    required = {"host", "port", "database", "production", "server_fingerprint"}
    if set(value) != required:
        raise TargetError(f"{label} identity has an unexpected shape")
    if any(not isinstance(value[key], str) for key in ("host", "port", "database", "server_fingerprint")):
        raise TargetError(f"{label} identity has invalid text fields")
    if type(value["production"]) is not bool:
        raise TargetError(f"{label} identity has an invalid production flag")
    if not re.fullmatch(r"[0-9]{1,5}", value["port"]) or not 1 <= int(value["port"]) <= 65535:
        raise TargetError(f"{label} identity has an invalid port")
    if not value["host"] or not value["database"]:
        raise TargetError(f"{label} identity has an empty host or database")
    if not re.fullmatch(r"[0-9a-f]{64}", value["server_fingerprint"]):
        raise TargetError(f"{label} identity has an invalid server fingerprint")
    return dict(value)


def validate_maintenance_record(value: Any, *, label: str = "maintenance boundary") -> dict[str, str]:
    """Validate a captured lifecycle boundary without treating it as permission."""
    if not isinstance(value, dict) or not isinstance(value.get("kind"), str):
        raise TargetError(f"{label} is missing")
    kind = value["kind"]
    if kind == "native-sql-exclusion-only":
        if set(value) != {"kind"}:
            raise TargetError(f"{label} has an unexpected native shape")
        return {"kind": kind}
    if kind not in {"docker", "systemd"}:
        raise TargetError(f"{label} has an unsupported kind")
    if not isinstance(value.get("id"), str) or not value["id"]:
        raise TargetError(f"{label} has no identity")
    if any(not isinstance(key, str) or not isinstance(item, str) for key, item in value.items()):
        raise TargetError(f"{label} has non-text fields")
    return {str(key): str(item) for key, item in value.items()}


@dataclass(frozen=True)
class TargetPolicy:
    environment: str
    host: str
    port: str
    database: str
    production: bool
    expected_fingerprint: str | None = None
    maintenance_kind: str | None = None
    maintenance_id: str | None = None

    @classmethod
    def from_environment(
        cls, env: Mapping[str, str], *, confirm_production_target: str | None = None,
        expected_fingerprint: str | None = None, maintenance_kind: str | None = None,
        maintenance_id: str | None = None,
    ) -> TargetPolicy:
        environment = env.get("ENVIRONMENT", "").strip().lower()
        if environment not in {"test", "dev", "development", "local", "production", "prod"}:
            raise TargetError("an explicit supported ENVIRONMENT is required")
        host, name, user = (env.get(key, "") for key in ("DB_HOST", "DB_NAME", "DB_USER"))
        if not host or not name or not user:
            raise TargetError("DB_HOST, DB_NAME and DB_USER are required")
        backend = env.get("PERSISTENCE_BACKEND", env.get("DB_BACKEND", "sql")).strip().lower()
        if backend not in {"sql", "mysql", "mariadb"}:
            raise TargetError("only SQL/MySQL/MariaDB recovery is supported")
        port = env.get("DB_PORT", "3306")
        if not re.fullmatch(r"[0-9]{1,5}", port) or not 1 <= int(port) <= 65535:
            raise TargetError("invalid database port")
        production = (environment in {"production", "prod"} or name.lower() == "duris"
                      or any(word in (host + " " + name).lower() for word in ("prod", "live")))
        if production:
            if confirm_production_target != name:
                raise TargetError("production recovery requires exact database-name confirmation")
            # Current production's supported operator path runs on the DB host.
            # Do not reuse the development client's optional/disabled remote TLS.
            try:
                loopback = ipaddress.ip_address(host).is_loopback
            except ValueError:
                loopback = False
            if not loopback:
                raise TargetError("production recovery requires a literal loopback database address")
        elif confirm_production_target is not None:
            raise TargetError("production confirmation does not match a production-classified target")
        if expected_fingerprint is not None and not re.fullmatch(r"[0-9a-f]{64}", expected_fingerprint):
            raise TargetError("invalid expected database fingerprint")
        return cls(environment, host, port, name, production, expected_fingerprint,
                   maintenance_kind, maintenance_id)

    def identity(self, db: Any, *, probe: bool = False) -> dict[str, Any]:
        actual = server_fingerprint(db)
        if self.production and not probe and self.expected_fingerprint is None:
            raise TargetError("production operations require a pinned database fingerprint")
        if self.expected_fingerprint is not None and self.expected_fingerprint != actual:
            raise TargetError("database fingerprint does not match the approved target")
        return {"host": self.host, "port": self.port, "database": self.database,
                "production": self.production, "server_fingerprint": actual}

    def require_maintenance(self) -> dict[str, str]:
        """Read a real lifecycle boundary; never alter its state automatically."""
        if self.maintenance_kind == "docker":
            return self._docker_maintenance()
        if self.maintenance_kind == "systemd":
            return self._systemd_maintenance()
        if self.production:
            raise TargetError("production mutation requires an explicit stopped maintenance boundary")
        return {"kind": "native-sql-exclusion-only"}

    def _docker_maintenance(self) -> dict[str, str]:
        identity = self.maintenance_id or ""
        if not re.fullmatch(r"[0-9a-f]{64}", identity):
            raise TargetError("maintenance container must use its complete immutable ID")
        records = _run_json(["docker", "inspect", "--type", "container", identity])
        if not isinstance(records, list) or len(records) != 1 or not isinstance(records[0], dict):
            raise TargetError("maintenance container response is invalid")
        record = records[0]
        state = record.get("State", {})
        restart = record.get("HostConfig", {}).get("RestartPolicy", {})
        if (record.get("Id") != identity or state.get("Status") != "exited"
                or state.get("Running") is not False or state.get("Restarting") is not False
                or state.get("Paused") is not False or state.get("Dead") is not False
                or state.get("Pid") != 0 or restart.get("Name") != "no"):
            raise TargetError("maintenance container is not stopped with automatic restart disabled")
        configured = {}
        for entry in record.get("Config", {}).get("Env", []):
            if isinstance(entry, str) and "=" in entry:
                key, value = entry.split("=", 1)
                if key in {"DB_HOST", "DB_NAME"}:
                    if key in configured:
                        raise TargetError("maintenance container has ambiguous database configuration")
                    configured[key] = value
        if configured != {"DB_HOST": self.host, "DB_NAME": self.database}:
            raise TargetError("maintenance container does not name the approved database target")
        return {"kind": "docker", "id": identity, "state": "exited", "restart": "no"}

    def _systemd_maintenance(self) -> dict[str, str]:
        # This is the repo-installed system unit, NOT a guessed user service.
        unit = self.maintenance_id
        if unit != "duris-mud-production.service":
            raise TargetError("unsupported production service boundary")
        properties = ("Id", "LoadState", "ActiveState", "SubState", "MainPID", "ControlPID",
                      "UnitFileState", "ControlGroup")
        command = ["systemctl", "--system", "show", unit, "--no-pager"]
        for prop in properties:
            command.extend(["-p", prop])
        try:
            result = subprocess.run(command, capture_output=True, text=True, timeout=20, check=False)
        except (OSError, subprocess.SubprocessError) as exc:
            raise TargetError("system maintenance boundary could not be inspected") from exc
        values = {}
        for line in result.stdout.splitlines():
            if "=" in line:
                key, value = line.split("=", 1)
                if key in values:
                    raise TargetError("system maintenance response was ambiguous")
                values[key] = value
        if (result.returncode or set(values) != set(properties) or values["Id"] != unit
                or values["LoadState"] != "masked" or values["ActiveState"] != "inactive"
                or values["SubState"] != "dead" or values["MainPID"] != "0"
                or values["ControlPID"] != "0"
                or values["UnitFileState"] not in {"masked", "masked-runtime"}):
            raise TargetError("production service is not both masked and fully stopped")
        expected_group = "/system.slice/" + unit
        if values["ControlGroup"] not in {"", expected_group}:
            raise TargetError("unexpected production service control group")
        if not Path("/sys/fs/cgroup/cgroup.controllers").is_file():
            raise TargetError("supported cgroup-v2 process visibility is unavailable")
        group = Path("/sys/fs/cgroup") / expected_group.lstrip("/")
        try:
            if group.exists():
                files = list(group.rglob("cgroup.procs"))
                if not files or any(path.read_text().strip() for path in files):
                    raise TargetError("production control group is not demonstrably empty")
        except OSError as exc:
            raise TargetError("production control group could not be inspected") from exc
        return {"kind": "systemd", "id": unit, "state": "masked-inactive"}


def protected_file_digest(path: Path) -> str:
    """Hash the opened owner-only regular file, not a symlink-followed pathname."""
    try:
        fd = os.open(path, os.O_RDONLY | os.O_NOFOLLOW | os.O_NONBLOCK)
    except OSError as exc:
        raise TargetError("backup could not be opened securely") from exc
    with os.fdopen(fd, "rb") as stream:
        before = os.fstat(stream.fileno())
        if (not stat.S_ISREG(before.st_mode) or before.st_uid != os.getuid()
                or before.st_mode & 0o077 or before.st_size <= 0):
            raise TargetError("backup must be a nonempty owner-only regular file")
        digest = hashlib.sha256()
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
        after = os.fstat(stream.fileno())
        if (before.st_size, before.st_mtime_ns, before.st_ctime_ns) != (
                after.st_size, after.st_mtime_ns, after.st_ctime_ns):
            raise TargetError("backup changed during verification")
    return digest.hexdigest()


def verify_backup_receipt(
    receipt: Mapping[str, Any], target: Mapping[str, Any],
    maintenance: Mapping[str, Any] | None = None,
) -> None:
    """The caller must securely load the receipt and bind it into the approved plan.

    A hash/receipt is backup integrity evidence, not proof of quiescence and
    not independent authorization to write a production database.
    """
    if (receipt.get("format") != BACKUP_RECEIPT_FORMAT
            or receipt.get("target") != dict(target)
            or type(receipt.get("dump_exit_code")) is not int
            or receipt.get("dump_exit_code") != 0
            or receipt.get("complete") is not True):
        raise TargetError("backup receipt does not describe the approved target and completed backup")
    if maintenance is not None:
        try:
            expected_maintenance = validate_maintenance_record(maintenance)
            actual_maintenance = validate_maintenance_record(receipt.get("maintenance_boundary"))
        except TargetError as exc:
            raise TargetError("backup receipt has no valid maintenance boundary") from exc
        if actual_maintenance != expected_maintenance:
            raise TargetError("backup receipt is not bound to the approved maintenance boundary")
    path, expected = receipt.get("dump_path"), receipt.get("sha256")
    if not isinstance(path, str) or not Path(path).is_absolute():
        raise TargetError("backup receipt requires an absolute dump path")
    if not isinstance(expected, str) or not re.fullmatch(r"[0-9a-f]{64}", expected):
        raise TargetError("backup receipt has no valid content digest")
    if protected_file_digest(Path(path)) != expected:
        raise TargetError("backup no longer matches its receipt")
