#!/usr/bin/env python3
"""Immutable post-baseline migration manifest and success-last runner."""

from __future__ import annotations

import argparse
import hashlib
import hmac
import json
import os
import re
import shutil
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Protocol


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_MANIFEST = ROOT / "migrations" / "migration_manifest.json"
MAX_MANIFEST_BYTES = 1024 * 1024
MAX_MIGRATION_BYTES = 8 * 1024 * 1024
MANIFEST_FIELDS = {"manifest_version", "runner_version", "baseline", "migrations"}
BASELINE_FIELDS = {"id", "required_table_count", "required_table_fingerprint"}
MIGRATION_FIELDS = {
    "id", "sequence", "description", "apply", "apply_checksum", "verify",
    "verify_checksum", "compatibility",
}


class MigrationContractError(Exception):
    pass


def strict_object(pairs: list[tuple[str, object]]) -> dict:
    result = {}
    for key, value in pairs:
        if key in result:
            raise MigrationContractError(f"duplicate JSON key: {key}")
        result[key] = value
    return result


def read_regular(path: Path, maximum: int, label: str) -> bytes:
    flags = os.O_RDONLY | getattr(os, "O_CLOEXEC", 0) | getattr(os, "O_NOFOLLOW", 0)
    try:
        descriptor = os.open(path, flags)
        with os.fdopen(descriptor, "rb") as source:
            payload = source.read(maximum + 1)
    except OSError as error:
        raise MigrationContractError(f"cannot read {label}: {error}") from error
    if len(payload) > maximum:
        raise MigrationContractError(f"{label} exceeds fixed bound")
    return payload


def checksum(payload: bytes) -> str:
    return hashlib.sha256(payload).hexdigest()


@dataclass(frozen=True)
class Migration:
    migration_id: str
    sequence: int
    description: str
    apply_path: Path
    apply_checksum: str
    verify_path: Path
    verify_checksum: str
    compatibility: str


@dataclass(frozen=True)
class Manifest:
    version: int
    runner_version: int
    baseline_id: str
    required_table_count: int
    required_table_fingerprint: str
    migrations: tuple[Migration, ...]


@dataclass(frozen=True)
class AppliedMigration:
    migration_id: str
    sequence: int
    description: str
    apply_checksum: str
    verify_checksum: str
    compatibility: str
    runner_version: int


def load_manifest(path: Path = DEFAULT_MANIFEST) -> Manifest:
    try:
        raw = read_regular(path, MAX_MANIFEST_BYTES, "migration manifest")
        value = json.loads(raw, object_pairs_hook=strict_object)
    except (UnicodeDecodeError, json.JSONDecodeError) as error:
        raise MigrationContractError(f"migration manifest parse failed: {error}") from error
    if not isinstance(value, dict) or set(value) != MANIFEST_FIELDS:
        raise MigrationContractError("migration manifest fields differ")
    if type(value["manifest_version"]) is not int or value["manifest_version"] != 1 or \
            type(value["runner_version"]) is not int or value["runner_version"] != 1:
        raise MigrationContractError("unsupported manifest or runner version")
    baseline = value["baseline"]
    if not isinstance(baseline, dict) or set(baseline) != BASELINE_FIELDS or \
            not re.fullmatch(r"[a-z0-9][a-z0-9._-]{7,63}", baseline.get("id", "")) or \
            type(baseline.get("required_table_count")) is not int or \
            baseline["required_table_count"] < 1 or \
            not re.fullmatch(r"[0-9a-f]{64}", baseline.get(
                "required_table_fingerprint", "")):
        raise MigrationContractError("baseline contract is invalid")
    items = value["migrations"]
    if not isinstance(items, list):
        raise MigrationContractError("migrations must be a list")
    directory = path.resolve().parent
    migrations = []
    seen_ids = set()
    for expected_sequence, item in enumerate(items, 1):
        if not isinstance(item, dict) or set(item) != MIGRATION_FIELDS:
            raise MigrationContractError("migration fields differ")
        migration_id = item["id"]
        if not isinstance(migration_id, str) or \
                not re.fullmatch(r"[0-9]{4}_[a-z0-9][a-z0-9_]{2,54}", migration_id):
            raise MigrationContractError("migration id is invalid")
        if migration_id in seen_ids:
            raise MigrationContractError("duplicate migration id")
        seen_ids.add(migration_id)
        if item["sequence"] != expected_sequence:
            raise MigrationContractError("migration sequence is missing or reordered")
        paths = []
        for field, extension in (("apply", "sql"), ("verify", "sh")):
            relative = item[field]
            if not isinstance(relative, str) or not re.fullmatch(
                    rf"immutable/[0-9]{{4}}_[a-z0-9_]+\.{extension}", relative):
                raise MigrationContractError(f"migration {field} path is invalid")
            candidate = directory / relative
            if candidate.resolve().parent != (directory / "immutable").resolve():
                raise MigrationContractError("migration path escapes immutable directory")
            paths.append(candidate)
        apply_payload = read_regular(paths[0], MAX_MIGRATION_BYTES, "migration apply")
        verify_payload = read_regular(paths[1], MAX_MIGRATION_BYTES, "migration verifier")
        if item["apply_checksum"] != checksum(apply_payload) or \
                item["verify_checksum"] != checksum(verify_payload):
            raise MigrationContractError("migration content checksum mismatch")
        if not isinstance(item["description"], str) or \
                not re.fullmatch(r"[A-Za-z0-9][A-Za-z0-9 ._:/+()-]{2,254}",
                                 item["description"]) or \
                not isinstance(item["compatibility"], str) or not re.fullmatch(
                    r"[A-Za-z0-9][A-Za-z0-9._+-]{2,63}", item["compatibility"]):
            raise MigrationContractError("migration metadata is invalid")
        migrations.append(Migration(migration_id, expected_sequence, item["description"],
                                    paths[0], item["apply_checksum"], paths[1],
                                    item["verify_checksum"], item["compatibility"]))
    return Manifest(value["manifest_version"], value["runner_version"], baseline["id"],
                    baseline["required_table_count"],
                    baseline["required_table_fingerprint"], tuple(migrations))


def table_fingerprint(table_names: list[str]) -> str:
    if len(set(table_names)) != len(table_names) or any(
            not re.fullmatch(r"[A-Za-z0-9_]+", name) for name in table_names):
        raise MigrationContractError("schema table inventory is invalid")
    return checksum(("\n".join(sorted(table_names)) + "\n").encode())


def history_checksum(rows: list[AppliedMigration]) -> str:
    digest = hashlib.sha256()
    for row in rows:
        for value in (row.migration_id, str(row.sequence), row.description,
                      row.apply_checksum, row.verify_checksum, row.compatibility,
                      str(row.runner_version)):
            encoded = value.encode("utf-8")
            digest.update(len(encoded).to_bytes(8, "big"))
            digest.update(encoded)
    return digest.hexdigest()


def validate_history_state(rows: list[AppliedMigration], applied_count: int,
                           expected_checksum: str) -> None:
    if type(applied_count) is not int or applied_count != len(rows) or \
            not re.fullmatch(r"[0-9a-f]{64}", expected_checksum) or \
            not hmac.compare_digest(history_checksum(rows), expected_checksum):
        raise MigrationContractError("migration history head/count mismatch")


def validate_applied_prefix(manifest: Manifest,
                            applied: list[AppliedMigration]) -> tuple[Migration, ...]:
    if len(applied) > len(manifest.migrations):
        raise MigrationContractError("applied history extends beyond manifest")
    for index, row in enumerate(applied):
        expected = manifest.migrations[index]
        actual = (row.migration_id, row.sequence, row.description, row.apply_checksum,
                  row.verify_checksum, row.compatibility, row.runner_version)
        wanted = (expected.migration_id, expected.sequence, expected.description,
                  expected.apply_checksum, expected.verify_checksum,
                  expected.compatibility, manifest.runner_version)
        if actual != wanted:
            raise MigrationContractError("applied migration history was edited or reordered")
    return manifest.migrations[len(applied):]


class Executor(Protocol):
    def acquire_lock(self) -> None: ...
    def release_lock(self) -> None: ...
    def require_baseline(self, manifest: Manifest) -> None: ...
    def applied(self) -> list[AppliedMigration]: ...
    def apply(self, migration: Migration) -> None: ...
    def verify(self, migration: Migration) -> None: ...
    def record(self, migration: Migration, runner_version: int) -> None: ...


def run_pending(manifest: Manifest, executor: Executor) -> list[str]:
    executor.acquire_lock()
    completed = []
    try:
        executor.require_baseline(manifest)
        pending = validate_applied_prefix(manifest, executor.applied())
        for migration in pending:
            executor.apply(migration)
            executor.verify(migration)
            executor.record(migration, manifest.runner_version)
            completed.append(migration.migration_id)
        return completed
    finally:
        executor.release_lock()


class MysqlExecutor:
    def __init__(self, manifest: Manifest):
        environment = os.environ.get("ENVIRONMENT", "").casefold()
        host = os.environ.get("DB_HOST", "")
        database = os.environ.get("DB_NAME", "")
        socket_path = os.environ.get("DB_SOCKET", "")
        if environment not in {"local", "development", "dev", "test"} or \
                host not in {"127.0.0.1", "localhost", "::1"} or \
                re.search(r"(^|[_-])prod(uction)?($|[_-])", database, re.I):
            raise MigrationContractError("migration target must be loopback non-production")
        if socket_path and not os.path.isabs(socket_path):
            raise MigrationContractError("DB_SOCKET must be an absolute path")
        for name in ("DB_USER", "DB_PASSWD", "DB_NAME"):
            if not os.environ.get(name):
                raise MigrationContractError(f"missing migration credential: {name}")
        self.manifest = manifest
        self.socket_path = socket_path
        connection = (["--protocol=socket", f"--socket={socket_path}"] if socket_path else
                      ["-h", host, "-P", os.environ.get("DB_PORT", "3306")])
        self.command = ["mysql", *connection, "-u", os.environ["DB_USER"],
                        "-N", "-B", database]

    def sql(self, statement: str, input_payload: bytes | None = None) -> str:
        environment = dict(os.environ)
        environment["MYSQL_PWD"] = os.environ["DB_PASSWD"]
        result = subprocess.run(self.command + (["-e", statement] if input_payload is None
                                                else []), input=input_payload,
                                capture_output=True, env=environment, check=False)
        if result.returncode:
            raise MigrationContractError("database migration command failed")
        return result.stdout.decode().strip()

    def acquire_lock(self) -> None:
        if self.sql("SELECT GET_LOCK('duris_immutable_migration',5);") != "1":
            raise MigrationContractError("migration lock unavailable")

    def release_lock(self) -> None:
        try:
            self.sql("SELECT RELEASE_LOCK('duris_immutable_migration');")
        except MigrationContractError:
            pass

    def live_tables(self) -> list[str]:
        output = self.sql("SELECT table_name FROM information_schema.tables WHERE "
                          "table_schema=DATABASE() AND table_type='BASE TABLE' ORDER BY table_name;")
        return output.splitlines() if output else []

    def require_baseline(self, manifest: Manifest) -> None:
        row = self.sql("SELECT baseline_id,HEX(schema_fingerprint),manifest_version,"
                       "runner_version FROM mud_schema_baselines ORDER BY adopted_at LIMIT 1;")
        expected = (f"{manifest.baseline_id}\t"
                    f"{manifest.required_table_fingerprint.upper()}\t"
                    f"{manifest.version}\t{manifest.runner_version}")
        if row != expected:
            raise MigrationContractError("verified migration baseline is absent or stale")

    def applied(self) -> list[AppliedMigration]:
        output = self.sql("SELECT migration_id,sequence_number,description,"
                          "LOWER(HEX(apply_checksum)),LOWER(HEX(verify_checksum)),"
                          "compatibility,runner_version FROM mud_schema_history "
                          "ORDER BY sequence_number;")
        rows = []
        for line in output.splitlines() if output else []:
            fields = line.split("\t")
            if len(fields) != 7:
                raise MigrationContractError("migration history row shape is invalid")
            rows.append(AppliedMigration(fields[0], int(fields[1]), fields[2], fields[3],
                                         fields[4], fields[5], int(fields[6])))
        state = self.sql("SELECT applied_count,LOWER(HEX(history_checksum)) FROM "
                         "mud_schema_migration_state WHERE state_id=1;").split("\t")
        if len(state) != 2:
            raise MigrationContractError("migration history state is absent")
        try:
            validate_history_state(rows, int(state[0]), state[1])
        except ValueError as error:
            raise MigrationContractError("migration history state is invalid") from error
        return rows

    def apply(self, migration: Migration) -> None:
        self.sql("", read_regular(migration.apply_path, MAX_MIGRATION_BYTES,
                                  "migration apply"))

    def verify(self, migration: Migration) -> None:
        environment = dict(os.environ)
        if self.socket_path:
            # Immutable verifier files are checksum-sealed and carry explicit
            # TCP flags. Route only their mysql executable through a narrow
            # adapter instead of rewriting those historical artifacts.
            real_mysql = shutil.which("mysql")
            if not real_mysql:
                raise MigrationContractError("mysql client is unavailable")
            environment["DURIS_REAL_MYSQL_CLIENT"] = real_mysql
            environment["PATH"] = (str(ROOT / "scripts/mysql_socket_bin") +
                                   os.pathsep + environment.get("PATH", ""))
        result = subprocess.run([str(migration.verify_path)], capture_output=True,
                                env=environment, check=False)
        if result.returncode:
            raise MigrationContractError("migration verifier failed")

    def record(self, migration: Migration, runner_version: int) -> None:
        description_hex = migration.description.encode("utf-8").hex()
        compatibility_hex = migration.compatibility.encode("utf-8").hex()
        existing = self.applied()
        old_checksum = history_checksum(existing)
        new_row = AppliedMigration(migration.migration_id, migration.sequence,
                                   migration.description, migration.apply_checksum,
                                   migration.verify_checksum, migration.compatibility,
                                   runner_version)
        new_checksum = history_checksum(existing + [new_row])
        result = self.sql("START TRANSACTION; INSERT INTO mud_schema_history("
                 "migration_id,sequence_number,description,apply_checksum,verify_checksum,"
                 "compatibility,runner_version) VALUES("+
                 f"'{migration.migration_id}',{migration.sequence},"
                 f"CONVERT(UNHEX('{description_hex}') USING utf8mb4),"
                 f"UNHEX('{migration.apply_checksum}'),UNHEX('{migration.verify_checksum}'),"
                 f"CONVERT(UNHEX('{compatibility_hex}') USING utf8mb4),{runner_version}); "
                 "UPDATE mud_schema_migration_state "
                 f"SET applied_count={migration.sequence},history_checksum=UNHEX('{new_checksum}') "
                 f"WHERE state_id=1 AND applied_count={len(existing)} AND "
                 f"history_checksum=UNHEX('{old_checksum}'); SELECT ROW_COUNT(); COMMIT;")
        if result.splitlines()[-1:] != ["1"]:
            raise MigrationContractError("migration history head changed during record")

    def adopt(self, kind: str) -> None:
        tables = self.live_tables()
        if len(tables) != self.manifest.required_table_count or \
                table_fingerprint(tables) != self.manifest.required_table_fingerprint:
            raise MigrationContractError("live schema does not match required baseline fingerprint")
        if kind not in {"fresh_bootstrap", "verified_legacy_adoption"}:
            raise MigrationContractError("invalid baseline kind")
        self.sql("INSERT INTO mud_schema_baselines(baseline_id,baseline_kind,"
                 "schema_fingerprint,manifest_version,runner_version) VALUES("+
                 f"'{self.manifest.baseline_id}','{kind}',"
                 f"UNHEX('{self.manifest.required_table_fingerprint}'),"
                 f"{self.manifest.version},{self.manifest.runner_version}) "
                 "ON DUPLICATE KEY UPDATE baseline_id=IF(baseline_id=VALUES(baseline_id) "
                 "AND schema_fingerprint=VALUES(schema_fingerprint),baseline_id,NULL);")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--manifest", type=Path, default=DEFAULT_MANIFEST)
    parser.add_argument("command", choices=("inspect", "adopt", "run"))
    parser.add_argument("--kind", choices=("fresh_bootstrap", "verified_legacy_adoption"))
    arguments = parser.parse_args()
    try:
        manifest = load_manifest(arguments.manifest)
        if arguments.command == "inspect":
            print(json.dumps({"baseline_id": manifest.baseline_id,
                              "required_table_count": manifest.required_table_count,
                              "required_table_fingerprint":
                                  manifest.required_table_fingerprint,
                              "migration_count": len(manifest.migrations),
                              "manifest_version": manifest.version,
                              "runner_version": manifest.runner_version}, sort_keys=True))
            return 0
        executor = MysqlExecutor(manifest)
        if arguments.command == "adopt":
            if arguments.kind is None:
                raise MigrationContractError("adopt requires --kind")
            executor.acquire_lock()
            try:
                executor.adopt(arguments.kind)
            finally:
                executor.release_lock()
        else:
            run_pending(manifest, executor)
        return 0
    except MigrationContractError as error:
        print(f"migration runner blocked: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
