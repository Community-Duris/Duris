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
SYSTEMD_CGROUP_ROOT = Path("/sys/fs/cgroup")
SYSTEMD_RUNTIME_ROOT = Path("/run/user")
SYSTEMD_PROC_ROOT = Path("/proc")


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
    if kind == "systemd-user":
        required = {
            "kind", "id", "state", "owner_uid", "manager_id", "manager_pid",
            "manager_control_pid", "manager_uid", "manager_state", "manager_sub_state",
            "manager_control_group", "runtime_dir", "manager_socket",
            "control_group", "main_pid", "control_pid", "cgroup_processes",
        }
        if set(value) != required:
            raise TargetError(f"{label} has an unexpected user-manager shape")
        if any(not isinstance(key, str) or not isinstance(item, str) for key, item in value.items()):
            raise TargetError(f"{label} has non-text fields")
        owner_uid = value["owner_uid"]
        if not re.fullmatch(r"[1-9][0-9]*", owner_uid):
            raise TargetError(f"{label} has an invalid user-manager owner")
        manager_id = f"user@{owner_uid}.service"
        if value["manager_id"] != manager_id or value["manager_uid"] != owner_uid:
            # manager_uid is checked below; keeping this branch separate makes
            # an omitted/extra identity field fail closed rather than being
            # silently inferred from the manager unit name.
            raise TargetError(f"{label} is not bound to the declared user manager")
        if value["id"] != "duris-mud-production.service":
            raise TargetError(f"{label} names an unsupported user service")
        if value["state"] != "masked-inactive" or value["manager_state"] != "active" \
                or value["manager_sub_state"] != "running":
            raise TargetError(f"{label} does not describe a stopped unit and live manager")
        if (not re.fullmatch(r"[1-9][0-9]*", value["manager_pid"])
                or value["manager_control_pid"] != "0"
                or value["main_pid"] != "0" or value["control_pid"] != "0"
                or value["cgroup_processes"] != "0"):
            raise TargetError(f"{label} has unexpected user-manager or service PIDs")
        expected_runtime = str(SYSTEMD_RUNTIME_ROOT / owner_uid)
        if value["runtime_dir"] != expected_runtime \
                or value["manager_socket"] != expected_runtime + "/systemd/private":
            raise TargetError(f"{label} is not bound to the declared user runtime directory")
        expected_group = f"/user.slice/user-{owner_uid}.slice/user@{owner_uid}.service"
        if value["manager_control_group"] != expected_group:
            raise TargetError(f"{label} has an unexpected user-manager cgroup")
        control_group = value["control_group"]
        if control_group and not control_group.startswith(expected_group + "/"):
            raise TargetError(f"{label} has an unexpected user-manager cgroup")
        # A masked inactive unit may have no cgroup after systemd removes the
        # dead unit's scope.  Preserve the observed empty identity; never
        # manufacture a path for a group that does not exist.
        return {str(key): str(item) for key, item in value.items()}
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
    maintenance_owner: str | None = None

    @classmethod
    def from_environment(
        cls, env: Mapping[str, str], *, confirm_production_target: str | None = None,
        expected_fingerprint: str | None = None, maintenance_kind: str | None = None,
        maintenance_id: str | None = None, maintenance_owner: str | None = None,
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
        if maintenance_kind == "systemd-user":
            if maintenance_owner is None or not re.fullmatch(r"[1-9][0-9]*", maintenance_owner):
                raise TargetError("user-systemd maintenance requires an explicit owner UID")
        elif maintenance_owner is not None:
            raise TargetError("maintenance owner is only valid for a user-systemd boundary")
        return cls(environment, host, port, name, production, expected_fingerprint,
                   maintenance_kind, maintenance_id, maintenance_owner)

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
        if self.maintenance_kind == "systemd-user":
            return self._systemd_user_maintenance()
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
        values = self._systemd_properties("--system", unit, properties)
        if (values["Id"] != unit
                or values["LoadState"] != "masked" or values["ActiveState"] != "inactive"
                or values["SubState"] != "dead" or values["MainPID"] != "0"
                or values["ControlPID"] != "0"
                or values["UnitFileState"] not in {"masked", "masked-runtime"}):
            raise TargetError("production service is not both masked and fully stopped")
        expected_group = "/system.slice/" + unit
        if values["ControlGroup"] not in {"", expected_group}:
            raise TargetError("unexpected production service control group")
        if not (SYSTEMD_CGROUP_ROOT / "cgroup.controllers").is_file():
            raise TargetError("supported cgroup-v2 process visibility is unavailable")
        group = SYSTEMD_CGROUP_ROOT / expected_group.lstrip("/")
        try:
            if group.exists():
                files = list(group.rglob("cgroup.procs"))
                if not files or any(path.read_text(encoding="ascii").strip() for path in files):
                    raise TargetError("production control group is not demonstrably empty")
        except (OSError, UnicodeError) as exc:
            raise TargetError("production control group could not be inspected") from exc
        return {"kind": "systemd", "id": unit, "state": "masked-inactive"}

    @staticmethod
    def _systemd_properties(
        scope: str, unit: str | None, properties: tuple[str, ...],
        *, environment: Mapping[str, str] | None = None,
    ) -> dict[str, str]:
        command = ["systemctl", scope, "show"]
        if unit is not None:
            command.append(unit)
        command.extend(["--no-pager", "--no-legend"])
        for prop in properties:
            command.extend(["-p", prop])
        try:
            result = subprocess.run(
                command, capture_output=True, text=True, timeout=20, check=False, env=environment,
            )
        except (OSError, subprocess.SubprocessError) as exc:
            raise TargetError("systemd maintenance boundary could not be inspected") from exc
        values: dict[str, str] = {}
        for line in result.stdout.splitlines():
            if "=" in line:
                key, value = line.split("=", 1)
                if key in values:
                    raise TargetError("systemd maintenance response was ambiguous")
                values[key] = value
        if result.returncode or set(values) != set(properties):
            raise TargetError("systemd maintenance response was incomplete")
        return values

    @staticmethod
    def _cgroup_directory(control_group: str, *, label: str) -> Path:
        if not control_group.startswith("/") or "\x00" in control_group \
                or "/../" in control_group or control_group.endswith("/.."):
            raise TargetError(f"{label} has an invalid cgroup path")
        try:
            root = SYSTEMD_CGROUP_ROOT.resolve(strict=True)
            controllers = root / "cgroup.controllers"
            if not controllers.is_file():
                raise TargetError(f"{label} cgroup process visibility is unavailable")
            group = (SYSTEMD_CGROUP_ROOT / control_group.lstrip("/")).resolve(strict=True)
        except OSError as exc:
            raise TargetError(f"{label} cgroup could not be inspected") from exc
        if group == root or root not in group.parents or not group.is_dir():
            raise TargetError(f"{label} cgroup is outside the supported hierarchy")
        return group

    @classmethod
    def _cgroup_pids(cls, control_group: str, *, label: str) -> set[int]:
        group = cls._cgroup_directory(control_group, label=label)
        try:
            files = [path for path in group.rglob("cgroup.procs") if path.is_file()]
        except OSError as exc:
            raise TargetError(f"{label} process visibility is unavailable") from exc
        if not files:
            raise TargetError(f"{label} has no visible cgroup process file")
        pids: set[int] = set()
        try:
            for path in files:
                for value in path.read_text(encoding="ascii").split():
                    if not value.isdigit() or int(value) <= 0:
                        raise TargetError(f"{label} cgroup process inventory is invalid")
                    pids.add(int(value))
        except (OSError, UnicodeError) as exc:
            raise TargetError(f"{label} process visibility is unavailable") from exc
        return pids

    @staticmethod
    def _proc_real_uid(pid: int, *, label: str) -> int:
        try:
            status = (SYSTEMD_PROC_ROOT / str(pid) / "status").read_text(encoding="ascii")
        except (OSError, UnicodeError) as exc:
            raise TargetError(f"{label} process identity is not visible") from exc
        for line in status.splitlines():
            if line.startswith("Uid:"):
                fields = line.split()
                if len(fields) >= 2 and fields[1].isdigit():
                    return int(fields[1])
        raise TargetError(f"{label} process identity is not visible")

    @staticmethod
    def _proc_text(pid: int, filename: str, *, label: str) -> str:
        try:
            return (SYSTEMD_PROC_ROOT / str(pid) / filename).read_text(encoding="ascii")
        except (OSError, UnicodeError) as exc:
            raise TargetError(f"{label} process identity is not visible") from exc

    @classmethod
    def _user_manager_pid(cls, pids: set[int], owner_uid: int) -> int:
        candidates = []
        for pid in sorted(pids):
            if cls._proc_real_uid(pid, label="user-systemd manager") != owner_uid:
                raise TargetError("user-systemd manager cgroup contains an unexpected owner")
            comm = cls._proc_text(pid, "comm", label="user-systemd manager").strip()
            cmdline = cls._proc_text(pid, "cmdline", label="user-systemd manager")
            if comm == "systemd" and "--user" in cmdline.split("\x00"):
                candidates.append(pid)
        if len(candidates) != 1:
            raise TargetError("user-systemd manager PID is not uniquely visible")
        return candidates[0]

    @staticmethod
    def _user_runtime(owner_uid: int) -> tuple[Path, Path]:
        runtime = SYSTEMD_RUNTIME_ROOT / str(owner_uid)
        try:
            info = runtime.lstat()
            private = runtime / "systemd" / "private"
            socket_info = private.lstat()
        except OSError as exc:
            raise TargetError("user-systemd runtime directory or manager socket is unavailable") from exc
        if (not stat.S_ISDIR(info.st_mode) or info.st_uid != owner_uid or info.st_mode & 0o077
                or not stat.S_ISSOCK(socket_info.st_mode) or socket_info.st_uid != owner_uid):
            raise TargetError("user-systemd runtime directory or manager socket is not owner-bound")
        if os.environ.get("XDG_RUNTIME_DIR") != str(runtime):
            raise TargetError("XDG_RUNTIME_DIR is not bound to the approved user manager")
        return runtime, private

    def _systemd_user_maintenance(self) -> dict[str, str]:
        unit = self.maintenance_id
        owner_text = self.maintenance_owner
        if unit != "duris-mud-production.service":
            raise TargetError("unsupported production user service boundary")
        if owner_text is None or not re.fullmatch(r"[1-9][0-9]*", owner_text):
            raise TargetError("user-systemd maintenance requires an explicit owner UID")
        owner_uid = int(owner_text)
        if owner_uid != os.getuid():
            raise TargetError("user-systemd manager owner is not the current operator")
        runtime, private = self._user_runtime(owner_uid)
        systemd_environment = os.environ.copy()
        systemd_environment["XDG_RUNTIME_DIR"] = str(runtime)
        # Do not let a caller-supplied session/system bus redirect the proof to
        # a different manager. systemctl --user will use the owner runtime.
        systemd_environment.pop("SYSTEMD_BUS_ADDRESS", None)
        systemd_environment.pop("DBUS_SESSION_BUS_ADDRESS", None)
        manager_properties = ("SystemState",)
        manager = self._systemd_properties(
            "--user", None, manager_properties, environment=systemd_environment,
        )
        expected_manager_id = f"user@{owner_uid}.service"
        if manager["SystemState"] != "running":
            raise TargetError("user-systemd manager is not the active owner manager")
        manager_group = f"/user.slice/user-{owner_uid}.slice/{expected_manager_id}"
        expected_group = f"/user.slice/user-{owner_uid}.slice/user@{owner_uid}.service"
        if manager_group != expected_group:
            raise TargetError("user-systemd manager cgroup is not owner-bound")
        manager_pids = self._cgroup_pids(manager_group, label="user-systemd manager")
        manager_pid = self._user_manager_pid(manager_pids, owner_uid)

        unit_properties = (
            "Id", "LoadState", "ActiveState", "SubState", "MainPID", "ControlPID",
            "UnitFileState", "ControlGroup",
        )
        service = self._systemd_properties(
            "--user", unit, unit_properties, environment=systemd_environment,
        )
        if (service["Id"] != unit or service["LoadState"] not in {"masked", "loaded"}
                or service["ActiveState"] != "inactive" or service["SubState"] != "dead"
                or service["MainPID"] != "0" or service["ControlPID"] != "0"
                or service["UnitFileState"] not in {"masked", "masked-runtime"}):
            raise TargetError("production user service is not masked, inactive, and fully stopped")
        control_group = service["ControlGroup"]
        if control_group:
            if not control_group.startswith(manager_group + "/"):
                raise TargetError("production user service cgroup is outside its manager")
            if self._cgroup_pids(control_group, label="production user service"):
                raise TargetError("production user service cgroup is not empty")
        elif service["LoadState"] != "masked":
            # A loaded unit without a cgroup is not an approved stopped
            # boundary.  The empty form is supported only for a unit that
            # systemd reports as masked and fully inactive.
            raise TargetError("production user service has no verifiable cgroup")
        # Keep an empty ControlGroup empty in the evidence.  It means the
        # masked unit's removed cgroup was observed as absent; do not invent a
        # manager-descendant path for it.
        return {
            "kind": "systemd-user",
            "id": unit,
            "state": "masked-inactive",
            "owner_uid": owner_text,
            "manager_id": expected_manager_id,
            "manager_uid": owner_text,
            "manager_pid": str(manager_pid),
            "manager_control_pid": "0",
            "manager_state": "active",
            "manager_sub_state": "running",
            "manager_control_group": manager_group,
            "runtime_dir": str(runtime),
            "manager_socket": str(private),
            "control_group": control_group,
            "main_pid": service["MainPID"],
            "control_pid": service["ControlPID"],
            "cgroup_processes": "0",
        }


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
