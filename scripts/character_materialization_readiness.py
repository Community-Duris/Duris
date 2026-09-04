#!/usr/bin/env python3
"""Fail-closed character materialization audit and exact manifest repair."""

from __future__ import annotations

import argparse
import gzip
import hashlib
import hmac
import os
import re
import stat
import subprocess
import sys
import time
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path
from typing import Callable

from chaos_eq_analyze import area_file_names, parse_defines, reconcile_area_objects
from import_legacy_dump import (
    LegacyImportError,
    process_environment,
    read_env_file,
)


ROOT = Path(__file__).resolve().parents[1]
MANIFEST_HEADER = "# duris-character-materialization-repair-v1"
MANIFEST_COLUMNS = (
    "kind\towner_pid\trow_id\tvnum\tcurrent_a\texpected_a\tcurrent_b\t"
    "expected_b\tchild_count"
)
MAX_ARTIFACT_BYTES = 1024 * 1024
MAX_PETS = 32
MAX_BACKUP_AGE_SECONDS = 2 * 60 * 60
LOOPBACK_HOSTS = {"127.0.0.1", "localhost", "::1"}
PRODUCTION_NAME = re.compile(r"(^|[_-])prod(?:uction)?($|[_-])", re.IGNORECASE)


class ReadinessError(Exception):
    """A fail-closed readiness error safe to show to an operator."""


def connection_arguments(config: dict[str, str]) -> list[str]:
    """Build transport arguments that verify every remote database identity."""
    socket_path = config.get("DB_SOCKET", "")
    if socket_path:
        return ["--protocol=socket", f"--socket={socket_path}"]
    arguments = ["--host", config["DB_HOST"], "--port", config.get("DB_PORT", "3306")]
    if config["DB_HOST"] in LOOPBACK_HOSTS:
        return ["--skip-ssl", *arguments]
    ca_path = Path(config.get("DB_SSL_CA", ""))
    if config.get("DB_TLS", "").upper() != "TRUE" or not ca_path.is_absolute() or \
            ca_path.is_symlink() or not ca_path.is_file():
        raise ReadinessError("remote readiness checks require TLS and a CA file")
    help_text = subprocess.run(
        ["mysql", "--help"], capture_output=True, text=True, check=False).stdout
    if "--ssl-mode" in help_text:
        return ["--ssl-mode=VERIFY_IDENTITY", f"--ssl-ca={ca_path}", *arguments]
    if "--ssl-verify-server-cert" in help_text:
        return [f"--ssl-ca={ca_path}", "--ssl-verify-server-cert", *arguments]
    raise ReadinessError("database client cannot verify the remote server identity")


def mysql_command(config: dict[str, str]) -> list[str]:
    """Build the noninteractive MySQL client command for readiness queries."""
    return [
        "mysql", *connection_arguments(config), "--user", config["DB_USER"],
        "--batch", "--skip-column-names", "--raw", "--max-allowed-packet=1G",
        config["DB_NAME"],
    ]


def run_mysql(config: dict[str, str], statement: str) -> str:
    """Run one database statement and expose only a bounded diagnostic on failure."""
    result = subprocess.run(
        [*mysql_command(config), "--execute", statement], capture_output=True,
        text=True, env=process_environment(config), check=False)
    if result.returncode:
        lines = [line.strip() for line in result.stderr.splitlines() if line.strip()]
        raise ReadinessError(
            "database command failed: " + (" | ".join(lines[:8]) or "no diagnostic"))
    return result.stdout.strip()


def active_connections(config: dict[str, str]) -> int:
    """Count other sessions using the selected database."""
    output = run_mysql(
        config, "SELECT COUNT(*) FROM information_schema.processlist "
        "WHERE db=DATABASE() AND id<>CONNECTION_ID();")
    try:
        return int(output)
    except ValueError as error:
        raise ReadinessError("active connection check returned malformed data") from error


@dataclass(frozen=True)
class Character:
    """Selectable character state needed by the materialization audit."""

    pid: int
    room_vnum: int


@dataclass(frozen=True)
class Item:
    """Persisted player or pet item topology needed by the audit."""

    row_id: int
    owner_pid: int
    vnum: int
    parent_id: int | None
    item_type: int | None
    domain: str = "player"
    scope_id: int = 0


@dataclass(frozen=True)
class Pet:
    """Persisted pet bounds and placement needed by the audit."""

    row_id: int
    owner_pid: int
    mob_vnum: int
    order: int
    hit: int
    max_hit: int
    mana: int
    max_mana: int
    vitality: int
    max_vitality: int
    charm_duration: int
    room_vnum: int


@dataclass(frozen=True, order=True)
class Finding:
    """One exact materialization failure and its optional safe repair."""

    kind: str
    owner_pid: int
    row_id: int
    vnum: int
    current_a: int
    expected_a: int
    current_b: int = 0
    expected_b: int = 0
    child_count: int = 0
    repairable: bool = False

    def manifest_row(self) -> str:
        """Serialize this finding into the protected manifest format."""
        values = (
            self.kind, self.owner_pid, self.row_id, self.vnum, self.current_a,
            self.expected_a, self.current_b, self.expected_b, self.child_count,
        )
        return "\t".join(str(value) for value in values)


@dataclass(frozen=True)
class Snapshot:
    """Database rows required to predict selectable-character materialization."""

    characters: tuple[Character, ...]
    items: tuple[Item, ...]
    pets: tuple[Pet, ...]
    pet_items: tuple[Item, ...]


def active_mobile_vnums() -> set[int]:
    """Load every active mobile vnum and fail when the area manifest is incomplete."""
    result: set[int] = set()
    try:
        paths = area_file_names(
            ROOT / "areas/mob",
            ROOT / "areas/AREA",
            extension=".mob",
            require_all=True,
        )
    except FileNotFoundError as error:
        raise ReadinessError("active mobile prototype files are missing") from error
    for path in paths:
        for source_line in path.read_text(
                encoding="utf-8", errors="replace").splitlines():
            match = re.fullmatch(r"#(\d+)", source_line.strip())
            if match and int(match.group(1)) != 9999999:
                result.add(int(match.group(1)))
    return result


def active_object_types() -> dict[int, int]:
    """Resolve unambiguous object types from the active area manifest."""
    constants = parse_defines(ROOT / "src/core/defines.h")
    paths = area_file_names(ROOT / "areas/obj", ROOT / "areas/AREA")
    objects, diagnostics = reconcile_area_objects(paths, {}, constants)
    if diagnostics["parse_errors"]:
        raise ReadinessError("active object prototypes cannot be parsed")
    if any(prototype.ambiguous for prototype in objects.values()):
        raise ReadinessError("active object prototypes contain an ambiguous vnum")
    return {vnum: prototype.object_type for vnum, prototype in objects.items()}


def _integer(value: str) -> int:
    """Parse a database integer or raise a readiness-safe error."""
    try:
        return int(value)
    except ValueError as error:
        raise ReadinessError("database readiness query returned malformed data") from error


def _nullable_integer(value: str) -> int | None:
    """Parse a nullable database integer in raw MySQL output form."""
    return None if value == "NULL" else _integer(value)


def _rows(output: str, width: int) -> list[list[str]]:
    """Split tabular MySQL output while enforcing its exact column count."""
    rows = [line.split("\t") for line in output.splitlines()] if output else []
    if any(len(row) != width for row in rows):
        raise ReadinessError("database readiness query returned malformed data")
    return rows


def database_snapshot(query: Callable[[str], str]) -> Snapshot:
    """Read all selectable character, item, and pet materialization inputs."""
    eligibility = (
        "pd.active=1 AND ac.deleted_at IS NULL AND ac.blocked=0 AND "
        "COALESCE(a.blocked,0)=0"
    )
    joins = (
        " JOIN account_characters ac ON ac.pid=pd.pid"
        " JOIN accounts a ON a.account_name=ac.account_name"
    )
    characters = tuple(
        Character(_integer(row[0]), _integer(row[1]))
        for row in _rows(query(
            "SELECT DISTINCT pd.pid,pd.last_room FROM player_data pd" + joins +
            " WHERE " + eligibility + " ORDER BY pd.pid;"), 2)
    )
    items = tuple(
        Item(_integer(row[0]), _integer(row[1]), _integer(row[2]),
             _nullable_integer(row[3]), _nullable_integer(row[4]))
        for row in _rows(query(
            "SELECT DISTINCT pi.id,pi.pid,pi.vnum,parent.id,pi.item_type "
            "FROM player_items pi JOIN item_current_owner own ON own.item_uid=pi.obj_uid "
            "AND own.owner_type=1 AND own.owner_id=pi.pid AND own.owner_context_id=0 "
            "AND own.state=1 AND own.vnum=pi.vnum LEFT JOIN item_current_owner parent_own "
            "ON parent_own.item_uid=own.parent_item_uid AND parent_own.owner_type=1 "
            "AND parent_own.owner_id=pi.pid AND parent_own.owner_context_id=0 "
            "AND parent_own.state=1 LEFT JOIN player_items parent "
            "ON parent.obj_uid=parent_own.item_uid AND parent.pid=pi.pid "
            "JOIN player_data pd ON pd.pid=pi.pid" + joins +
            " WHERE " + eligibility + " ORDER BY pi.id;"), 5)
    )
    pets = tuple(
        Pet(*(_integer(value) for value in row))
        for row in _rows(query(
            "SELECT DISTINCT pp.id,pp.owner_pid,pp.mob_vnum,pp.pet_order,pp.hit,pp.max_hit,"
            "pp.mana,pp.max_mana,pp.vitality,pp.max_vitality,pp.charm_duration,"
            "pp.room_vnum FROM player_pets pp JOIN player_data pd "
            "ON pd.pid=pp.owner_pid" + joins + " WHERE " + eligibility +
            " ORDER BY pp.id;"), 12)
    )
    pet_items = tuple(
        Item(_integer(row[0]), _integer(row[1]), _integer(row[2]),
             _nullable_integer(row[3]), _nullable_integer(row[4]), "pet",
             _integer(row[5]))
        for row in _rows(query(
            "SELECT DISTINCT ppi.id,pp.owner_pid,ppi.vnum,parent.id,"
            "ppi.item_type,ppi.pet_id "
            "FROM player_pet_items ppi JOIN player_pets pp ON pp.id=ppi.pet_id "
            "JOIN item_current_owner own ON own.item_uid=ppi.obj_uid "
            "AND own.owner_type=1 AND own.owner_id=pp.owner_pid "
            "AND own.owner_context_id=0 AND own.state=1 AND own.vnum=ppi.vnum "
            "LEFT JOIN item_current_owner parent_own "
            "ON parent_own.item_uid=own.parent_item_uid AND parent_own.owner_type=1 "
            "AND parent_own.owner_id=pp.owner_pid AND parent_own.owner_context_id=0 "
            "AND parent_own.state=1 LEFT JOIN player_pet_items parent "
            "ON parent.obj_uid=parent_own.item_uid AND parent.pet_id=ppi.pet_id "
            "JOIN player_data pd ON pd.pid=pp.owner_pid" + joins +
            " WHERE " + eligibility + " ORDER BY ppi.id;"), 6)
    )
    return Snapshot(characters, items, pets, pet_items)


def _item_findings(items: tuple[Item, ...], object_types: dict[int, int],
                   allowed_parent_types: set[int]) -> list[Finding]:
    """Find unknown prototypes and item trees the runtime would reject."""
    findings: list[Finding] = []
    by_id = {item.row_id: item for item in items}
    children: dict[int, int] = {}
    for item in items:
        if item.parent_id is not None:
            children[item.parent_id] = children.get(item.parent_id, 0) + 1
        if item.vnum not in object_types:
            findings.append(Finding(
                f"{item.domain}_unknown_object", item.owner_pid, item.row_id,
                item.vnum, -1, -1))
    reported: set[int] = set()
    for child in items:
        if child.parent_id is None or child.parent_id in reported:
            continue
        parent = by_id.get(child.parent_id)
        if parent is None or parent.owner_pid != child.owner_pid or \
                parent.scope_id != child.scope_id:
            findings.append(Finding(
                f"{child.domain}_invalid_parent", child.owner_pid, child.row_id,
                child.vnum, child.parent_id, -1))
            continue
        canonical = object_types.get(parent.vnum)
        if canonical is None:
            continue
        effective = parent.item_type if parent.item_type is not None else canonical
        if effective in allowed_parent_types:
            continue
        repairable = (parent.item_type is not None and
                      canonical in allowed_parent_types)
        kind = ("player_item_type" if parent.domain == "player" else
                "pet_item_type") if repairable else f"{parent.domain}_invalid_nesting"
        findings.append(Finding(
            kind, parent.owner_pid, parent.row_id, parent.vnum, effective,
            canonical, child_count=children[parent.row_id], repairable=repairable))
        reported.add(parent.row_id)
    return findings


def evaluate(snapshot: Snapshot, object_types: dict[int, int], mobile_vnums: set[int],
             allowed_parent_types: set[int]) -> list[Finding]:
    """Predict materializer refusals and identify only exact safe repairs."""
    findings = _item_findings(snapshot.items, object_types, allowed_parent_types)
    findings.extend(_item_findings(snapshot.pet_items, object_types, allowed_parent_types))
    rooms = {character.pid: character.room_vnum for character in snapshot.characters}
    pet_orders: set[tuple[int, int]] = set()
    for pet in snapshot.pets:
        owner_room = rooms.get(pet.owner_pid)
        if pet.mob_vnum not in mobile_vnums:
            findings.append(Finding(
                "pet_unknown_mobile", pet.owner_pid, pet.row_id, pet.mob_vnum,
                -1, -1))
        duplicate_order = (pet.owner_pid, pet.order) in pet_orders
        pet_orders.add((pet.owner_pid, pet.order))
        valid_other_bounds = (
            0 <= pet.order < MAX_PETS and not duplicate_order and pet.max_hit > 0 and
            pet.hit >= 0 and pet.max_mana >= 0 and 0 <= pet.mana <= pet.max_mana and
            pet.max_vitality >= 0 and 0 <= pet.vitality <= pet.max_vitality and
            pet.charm_duration >= -1 and pet.room_vnum > 0
        )
        repairable_hit = pet.hit > pet.max_hit
        room_mismatch = owner_room is not None and pet.room_vnum != owner_room
        if valid_other_bounds and owner_room is not None and owner_room > 0 and \
                (repairable_hit or room_mismatch):
            findings.append(Finding(
                "pet_state", pet.owner_pid, pet.row_id, pet.mob_vnum,
                pet.room_vnum, owner_room if owner_room is not None else pet.room_vnum,
                pet.hit, min(pet.hit, pet.max_hit), repairable=True))
        elif not valid_other_bounds or pet.hit > pet.max_hit or owner_room is None or \
                owner_room <= 0:
            findings.append(Finding(
                "pet_invalid_bounds", pet.owner_pid, pet.row_id, pet.mob_vnum,
                pet.room_vnum, owner_room if owner_room is not None else -1,
                pet.hit, pet.max_hit))
    return sorted(set(findings))


def summarize(snapshot: Snapshot, findings: list[Finding]) -> str:
    """Return an identifier-free aggregate audit summary."""
    affected = len({finding.owner_pid for finding in findings})
    repairable = sum(finding.repairable for finding in findings)
    return (
        f"characters={len(snapshot.characters)} items={len(snapshot.items)} "
        f"pets={len(snapshot.pets)} pet_items={len(snapshot.pet_items)} "
        f"findings={len(findings)} affected_characters={affected} "
        f"repairable_rows={repairable}"
    )


def _validate_secure_create_destination(path: Path, label: str) -> None:
    """Validate that a protected artifact can be created at an unused path."""
    if not path.is_absolute():
        raise ReadinessError(f"{label} path must be absolute")
    try:
        parent = path.parent.resolve(strict=True)
        metadata = parent.stat()
    except OSError as error:
        raise ReadinessError(f"cannot inspect {label} directory: {error}") from error
    if not stat.S_ISDIR(metadata.st_mode) or metadata.st_uid != os.getuid() or \
            stat.S_IMODE(metadata.st_mode) & 0o077:
        raise ReadinessError(f"{label} directory must be owner-only")
    if path.parent != parent:
        raise ReadinessError(f"{label} directory must not traverse symbolic links")
    try:
        path.lstat()
    except FileNotFoundError:
        return
    except OSError as error:
        raise ReadinessError(f"cannot inspect {label} destination: {error}") from error
    raise ReadinessError(f"{label} destination already exists")


def _secure_create(path: Path, payload: bytes, label: str) -> None:
    """Create a new owner-only artifact without following symbolic links."""
    _validate_secure_create_destination(path, label)
    flags = os.O_WRONLY | os.O_CREAT | os.O_EXCL | getattr(os, "O_NOFOLLOW", 0)
    try:
        descriptor = os.open(path, flags, 0o600)
        with os.fdopen(descriptor, "wb") as destination:
            destination.write(payload)
    except OSError as error:
        raise ReadinessError(f"cannot create {label}: {error}") from error


def write_manifest(path: Path, database: str, findings: list[Finding]) -> str:
    """Write an exact protected manifest for fully repairable findings."""
    if not findings:
        raise ReadinessError("no materialization findings require a repair manifest")
    if any(not finding.repairable for finding in findings):
        raise ReadinessError("unrepairable findings prevent exact manifest generation")
    lines = [MANIFEST_HEADER, f"# database={database}", MANIFEST_COLUMNS]
    lines.extend(finding.manifest_row() for finding in findings)
    payload = ("\n".join(lines) + "\n").encode()
    _secure_create(path, payload, "repair manifest")
    return hashlib.sha256(payload).hexdigest()


def read_manifest(path: Path, expected_digest: str, database: str) -> list[Finding]:
    """Read and authenticate a protected manifest for the selected database."""
    if not path.is_absolute() or not re.fullmatch(r"[0-9a-f]{64}", expected_digest):
        raise ReadinessError("repair manifest path or digest is invalid")
    flags = os.O_RDONLY | getattr(os, "O_NOFOLLOW", 0)
    try:
        descriptor = os.open(path, flags)
        metadata = os.fstat(descriptor)
        if not stat.S_ISREG(metadata.st_mode) or metadata.st_uid != os.getuid() or \
                stat.S_IMODE(metadata.st_mode) & 0o077:
            raise ReadinessError("repair manifest must be an owner-only regular file")
        with os.fdopen(descriptor, "rb") as source:
            payload = source.read(MAX_ARTIFACT_BYTES + 1)
    except OSError as error:
        raise ReadinessError(f"cannot read repair manifest: {error}") from error
    if len(payload) > MAX_ARTIFACT_BYTES or not hmac.compare_digest(
            hashlib.sha256(payload).hexdigest(), expected_digest):
        raise ReadinessError("repair manifest digest does not match")
    lines = payload.decode("utf-8").splitlines()
    if lines[:3] != [MANIFEST_HEADER, f"# database={database}", MANIFEST_COLUMNS]:
        raise ReadinessError("repair manifest header does not match this database")
    result: list[Finding] = []
    for line in lines[3:]:
        fields = line.split("\t")
        if len(fields) != 9 or fields[0] not in {
                "player_item_type", "pet_item_type", "pet_state"}:
            raise ReadinessError("repair manifest contains an invalid row")
        try:
            numbers = [int(value) for value in fields[1:]]
        except ValueError as error:
            raise ReadinessError("repair manifest contains an invalid number") from error
        result.append(Finding(fields[0], *numbers, repairable=True))
    if not result or len(set(result)) != len(result):
        raise ReadinessError("repair manifest must contain unique affected rows")
    return sorted(result)


def validate_backup(path: Path, expected_digest: str, database: str) -> str:
    """Authenticate a fresh owner-only dump containing required database markers."""
    if not path.is_absolute() or not re.fullmatch(r"[0-9a-f]{64}", expected_digest):
        raise ReadinessError("backup path or digest is invalid")
    flags = os.O_RDONLY | getattr(os, "O_NOFOLLOW", 0)
    digest = hashlib.sha256()
    markers = {
        f"USE `{database}`;".encode(), b"CREATE TABLE `accounts`",
        b"CREATE TABLE `player_data`", b"CREATE TABLE `ships`",
    }
    try:
        descriptor = os.open(path, flags)
        metadata = os.fstat(descriptor)
        if not stat.S_ISREG(metadata.st_mode) or metadata.st_uid != os.getuid() or \
                stat.S_IMODE(metadata.st_mode) & 0o077:
            raise ReadinessError("backup must be an owner-only regular file")
        age = time.time() - metadata.st_mtime
        if age < -300 or age > MAX_BACKUP_AGE_SECONDS:
            raise ReadinessError("backup must be fresh")
        with os.fdopen(descriptor, "rb") as source:
            while payload := source.read(1024 * 1024):
                digest.update(payload)
        descriptor = os.open(path, flags)
        with os.fdopen(descriptor, "rb") as raw:
            source = gzip.GzipFile(fileobj=raw) if path.suffix == ".gz" else raw
            tail = b""
            while payload := source.read(1024 * 1024):
                window = tail + payload
                markers = {marker for marker in markers if marker not in window}
                tail = window[-512:]
    except (OSError, EOFError) as error:
        raise ReadinessError(f"cannot validate backup: {error}") from error
    actual = digest.hexdigest()
    if not hmac.compare_digest(actual, expected_digest) or markers:
        raise ReadinessError("backup digest or database markers do not match")
    return actual


def _fingerprint_expression(columns: list[str]) -> str:
    """Build the stable SQL expression used for unaffected-row fingerprints."""
    encoded = ",".join(f"IFNULL(HEX(`{column}`),'NULL')" for column in columns)
    return f"COALESCE(BIT_XOR(CRC32(CONCAT_WS(CHAR(31),{encoded}))),0)"


def apply_repair(config: dict[str, str], findings: list[Finding]) -> str:
    """Apply exact manifested repairs in one guarded transaction."""
    item_findings = [finding for finding in findings if finding.kind.endswith("item_type")]
    pet_findings = [finding for finding in findings if finding.kind == "pet_state"]
    columns: dict[str, list[str]] = {}
    for table in ("player_data", "player_items", "player_pets", "player_pet_items"):
        output = run_mysql(
            config, "SELECT column_name FROM information_schema.columns "
            f"WHERE table_schema=DATABASE() AND table_name='{table}' "
            "ORDER BY ordinal_position;")
        columns[table] = output.splitlines()
        if not columns[table] or any(re.fullmatch(r"[A-Za-z0-9_]+", name) is None
                                     for name in columns[table]):
            raise ReadinessError("cannot fingerprint unaffected rows")
    player_item_ids = ",".join(
        str(finding.row_id) for finding in item_findings
        if finding.kind == "player_item_type") or "0"
    pet_item_ids = ",".join(
        str(finding.row_id) for finding in item_findings
        if finding.kind == "pet_item_type") or "0"
    pet_ids = ",".join(str(finding.row_id) for finding in pet_findings) or "0"
    exclusions = {
        "player_data": "",
        "player_items": f" WHERE id NOT IN ({player_item_ids})",
        "player_pets": f" WHERE id NOT IN ({pet_ids})",
        "player_pet_items": f" WHERE id NOT IN ({pet_item_ids})",
    }
    before = []
    after_guards = []
    for table in columns:
        fingerprint = _fingerprint_expression(columns[table])
        before.append(
            "INSERT INTO _materialization_unaffected_before VALUES "
            f"('{table}',(SELECT COUNT(*) FROM `{table}`{exclusions[table]}),"
            f"(SELECT {fingerprint} FROM `{table}`{exclusions[table]}));")
        after_guards.append(
            "INSERT INTO _materialization_guard(ok) SELECT 1 FROM "
            f"_materialization_unaffected_before b WHERE b.table_name='{table}' AND "
            f"(b.row_count<>(SELECT COUNT(*) FROM `{table}`{exclusions[table]}) OR "
            f"b.row_fingerprint<>(SELECT {fingerprint} FROM `{table}`{exclusions[table]}));")
    item_predicates = [
        f"(id={row.row_id} AND pid={row.owner_pid} AND vnum={row.vnum} "
        f"AND item_type={row.current_a} AND EXISTS(SELECT 1 FROM item_current_owner own "
        f"WHERE own.item_uid=player_items.obj_uid AND own.owner_type=1 "
        f"AND own.owner_id={row.owner_pid} AND own.owner_context_id=0 "
        f"AND own.state=1 AND own.vnum={row.vnum}))" for row in item_findings
        if row.kind == "player_item_type"
    ]
    pet_item_predicates = [
        f"(id={row.row_id} AND vnum={row.vnum} AND item_type={row.current_a} "
        f"AND EXISTS(SELECT 1 FROM player_pets pp JOIN item_current_owner own "
        f"ON own.item_uid=player_pet_items.obj_uid WHERE pp.id=player_pet_items.pet_id "
        f"AND pp.owner_pid={row.owner_pid} AND own.owner_type=1 "
        f"AND own.owner_id={row.owner_pid} AND own.owner_context_id=0 "
        f"AND own.state=1 AND own.vnum={row.vnum}))"
        for row in item_findings if row.kind == "pet_item_type"
    ]
    pet_predicates = [
        f"(id={row.row_id} AND owner_pid={row.owner_pid} AND mob_vnum={row.vnum} "
        f"AND room_vnum={row.current_a} AND hit={row.current_b} "
        f"AND EXISTS(SELECT 1 FROM player_data pd WHERE pd.pid={row.owner_pid} "
        f"AND pd.last_room={row.expected_a}))" for row in pet_findings
    ]
    pet_room_cases = " ".join(
        f"WHEN {row.row_id} THEN {row.expected_a}" for row in pet_findings)
    pet_hit_cases = " ".join(
        f"WHEN {row.row_id} THEN {row.expected_b}" for row in pet_findings)
    statements = [
        "START TRANSACTION;",
        "CREATE TEMPORARY TABLE _materialization_guard(ok INT PRIMARY KEY);",
        "INSERT INTO _materialization_guard VALUES(1);",
        "CREATE TEMPORARY TABLE _materialization_unaffected_before("
        "table_name VARCHAR(32) PRIMARY KEY,row_count BIGINT UNSIGNED NOT NULL,"
        "row_fingerprint BIGINT UNSIGNED NOT NULL);",
        *before,
    ]
    if item_predicates:
        statements.extend([
            "UPDATE player_items SET item_type=NULL WHERE " + " OR ".join(item_predicates) + ";",
            "SET @player_item_rows=ROW_COUNT();",
        ])
    else:
        statements.append("SET @player_item_rows=0;")
    if pet_item_predicates:
        statements.extend([
            "UPDATE player_pet_items SET item_type=NULL WHERE " +
            " OR ".join(pet_item_predicates) + ";",
            "SET @pet_item_rows=ROW_COUNT();",
        ])
    else:
        statements.append("SET @pet_item_rows=0;")
    if pet_predicates:
        statements.extend([
            f"UPDATE player_pets SET room_vnum=CASE id {pet_room_cases} END,"
            f"hit=CASE id {pet_hit_cases} END WHERE " + " OR ".join(pet_predicates) + ";",
            "SET @pet_rows=ROW_COUNT();",
        ])
    else:
        statements.append("SET @pet_rows=0;")
    statements.extend([
        "INSERT INTO _materialization_guard(ok) SELECT 1 WHERE "
        f"@player_item_rows+@pet_item_rows<>{len(item_findings)} OR "
        f"@pet_rows<>{len(pet_findings)};",
        *after_guards,
        "SELECT @player_item_rows,@pet_item_rows,@pet_rows;",
        "COMMIT;",
    ])
    try:
        return run_mysql(config, "\n".join(statements))
    except LegacyImportError as error:
        raise ReadinessError(str(error)) from error


def parse_arguments() -> argparse.Namespace:
    """Parse audit, manifest, and guarded-repair command-line options."""
    parser = argparse.ArgumentParser(
        description="Check selectable character snapshots against current prototypes.")
    parser.add_argument("--env-file", type=Path, default=ROOT / ".env")
    parser.add_argument("--write-manifest", type=Path)
    parser.add_argument("--apply", action="store_true")
    parser.add_argument("--manifest", type=Path)
    parser.add_argument("--manifest-sha256")
    parser.add_argument("--backup", type=Path)
    parser.add_argument("--backup-sha256")
    parser.add_argument("--confirm-database")
    parser.add_argument("--authorize-production-repair", action="store_true")
    parser.add_argument("--receipt", type=Path)
    return parser.parse_args()


def main() -> int:
    """Audit the configured database or apply one authenticated repair manifest."""
    arguments = parse_arguments()
    try:
        config = read_env_file(arguments.env_file.resolve())
        object_types = active_object_types()
        mobile_vnums = active_mobile_vnums()
        constants = parse_defines(ROOT / "src/core/defines.h")
        allowed = {constants[name] for name in (
            "ITEM_CONTAINER", "ITEM_QUIVER", "ITEM_STORAGE", "ITEM_CORPSE")}

        def query(sql: str) -> str:
            """Run a snapshot query against the validated configuration."""
            return run_mysql(config, sql)

        snapshot = database_snapshot(query)
        findings = evaluate(snapshot, object_types, mobile_vnums, allowed)
        summary = summarize(snapshot, findings)
        if arguments.apply:
            required = (
                arguments.manifest, arguments.manifest_sha256, arguments.backup,
                arguments.backup_sha256, arguments.confirm_database, arguments.receipt,
            )
            if not all(required) or arguments.write_manifest:
                raise ReadinessError("apply requires manifest, backup, confirmation, and receipt")
            if arguments.confirm_database != config["DB_NAME"]:
                raise ReadinessError("database confirmation does not match")
            production = (
                config.get("ENVIRONMENT", "").casefold() == "production" or
                PRODUCTION_NAME.search(config["DB_NAME"]) is not None)
            if production and not arguments.authorize_production_repair:
                raise ReadinessError(
                    "production repair requires explicit owner authorization acknowledgment")
            if arguments.authorize_production_repair and not production:
                raise ReadinessError(
                    "production authorization acknowledgment is invalid for this target")
            manifest = read_manifest(
                arguments.manifest, arguments.manifest_sha256, config["DB_NAME"])
            current = sorted(finding for finding in findings if finding.repairable)
            if any(not finding.repairable for finding in findings) or manifest != current:
                raise ReadinessError("current findings do not exactly match the repair manifest")
            backup_digest = validate_backup(
                arguments.backup, arguments.backup_sha256, config["DB_NAME"])
            if active_connections(config):
                raise ReadinessError("database writers are not quiesced")
            _validate_secure_create_destination(arguments.receipt, "repair receipt")
            apply_repair(config, manifest)
            repaired_snapshot = database_snapshot(query)
            repaired = evaluate(repaired_snapshot, object_types, mobile_vnums, allowed)
            if repaired:
                raise ReadinessError("post-repair materialization readiness still fails")
            receipt = (
                "# duris-character-materialization-repair-receipt-v1\n"
                f"timestamp_utc={datetime.now(timezone.utc).isoformat()}\n"
                f"database={config['DB_NAME']}\nmanifest_sha256={arguments.manifest_sha256}\n"
                f"backup_sha256={backup_digest}\n"
                f"item_type_rows={sum(row.kind.endswith('item_type') for row in manifest)}\n"
                f"pet_rows={sum(row.kind == 'pet_state' for row in manifest)}\n"
                "post_repair_findings=0\n"
            ).encode()
            _secure_create(arguments.receipt, receipt, "repair receipt")
            print("materialization repair complete: " + summarize(repaired_snapshot, repaired))
            return 0
        if arguments.manifest or arguments.manifest_sha256 or arguments.backup or \
                arguments.backup_sha256 or arguments.confirm_database or arguments.receipt:
            raise ReadinessError("repair-only arguments require --apply")
        if arguments.authorize_production_repair:
            raise ReadinessError("production authorization acknowledgment requires --apply")
        if arguments.write_manifest:
            digest = write_manifest(
                arguments.write_manifest, config["DB_NAME"], findings)
            print(f"materialization manifest written: {summary} sha256={digest}")
            return 0
        print("materialization readiness: " + summary)
        return 2 if findings else 0
    except (LegacyImportError, ReadinessError, KeyError, OSError, UnicodeError) as error:
        print(f"materialization readiness blocked: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
