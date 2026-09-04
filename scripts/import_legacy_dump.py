#!/usr/bin/env python3
"""Replace a local Duris database from a guarded legacy MySQL dump."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import shlex
import stat
import subprocess
import sys
import tempfile
from datetime import datetime, timezone
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SAFE_ENVIRONMENTS = {"local", "development", "dev", "test"}
LOOPBACK_HOSTS = {"127.0.0.1", "localhost", "::1"}
REQUIRED_CONFIG = {
    "ENVIRONMENT", "DB_HOST", "DB_USER", "DB_PASSWD", "DB_NAME",
    "DB_ALLOWED_TARGETS",
}
SAFE_IDENTIFIER = re.compile(r"^[A-Za-z0-9_]+$")
PRODUCTION_NAME = re.compile(r"(^|[_-])prod(?:uction)?($|[_-])", re.IGNORECASE)
DATABASE_DIRECTIVE = re.compile(
    rb"^\s*(?:/\*!\d*\s*)?(?:(?:CREATE|DROP)\s+(?:DATABASE|SCHEMA)\b|USE\s+)",
    re.IGNORECASE)
MYSQL8_COLLATION = b"utf8mb4_0900_ai_ci"
PORTABLE_COLLATION = b"utf8mb4_unicode_ci"
DEFINER = re.compile(rb"DEFINER=`(?:``|[^`])+`@`(?:``|[^`])+`")


class LegacyImportError(Exception):
    """A fail-closed import error safe to show to an operator."""


def read_env_file(path: Path) -> dict[str, str]:
    try:
        metadata = path.lstat()
    except OSError as error:
        raise LegacyImportError(f"cannot inspect environment file: {error}") from error
    if not stat.S_ISREG(metadata.st_mode) or path.is_symlink():
        raise LegacyImportError("environment file must be a regular non-symlink file")
    if stat.S_IMODE(metadata.st_mode) & 0o077:
        raise LegacyImportError("environment file must have mode 0600 or stricter")
    values: dict[str, str] = {}
    try:
        lines = path.read_text(encoding="utf-8").splitlines()
    except (OSError, UnicodeError) as error:
        raise LegacyImportError(f"cannot read environment file: {error}") from error
    for line_number, raw in enumerate(lines, 1):
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        if line.startswith("export "):
            line = line[7:].lstrip()
        if "=" not in line:
            raise LegacyImportError(
                f"environment file line {line_number} is not a KEY=value assignment")
        key, value = line.split("=", 1)
        key = key.strip()
        if not re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", key):
            raise LegacyImportError(f"environment file line {line_number} has an invalid key")
        value = value.strip()
        if value.startswith(("'", '"')):
            try:
                parsed = shlex.split(value, comments=True, posix=True)
            except ValueError as error:
                raise LegacyImportError(
                    f"environment file line {line_number} has invalid quoting") from error
            if len(parsed) != 1:
                raise LegacyImportError(
                    f"environment file line {line_number} has an invalid value")
            value = parsed[0]
        values[key] = value
    missing = sorted(key for key in REQUIRED_CONFIG if not values.get(key))
    if missing:
        raise LegacyImportError("missing import configuration: " + ", ".join(missing))
    return values


def validate_target(config: dict[str, str]) -> None:
    environment = config["ENVIRONMENT"].casefold()
    host = config["DB_HOST"]
    database = config["DB_NAME"]
    if environment not in SAFE_ENVIRONMENTS or host not in LOOPBACK_HOSTS or \
            PRODUCTION_NAME.search(database):
        raise LegacyImportError("legacy import target must be loopback non-production")
    if not SAFE_IDENTIFIER.fullmatch(database):
        raise LegacyImportError("database name is invalid")
    port = config.get("DB_PORT", "3306")
    if not port.isdigit() or not 1 <= int(port) <= 65535:
        raise LegacyImportError("database port must be between 1 and 65535")
    allowed = {target.strip() for target in config["DB_ALLOWED_TARGETS"].split(",")}
    if f"{host}/{database}" not in allowed:
        raise LegacyImportError("legacy import target is not allow-listed")
    socket_path = config.get("DB_SOCKET", "")
    if socket_path and not Path(socket_path).is_absolute():
        raise LegacyImportError("database socket must be an absolute path")


def validate_dump(path: Path) -> str:
    try:
        metadata = path.lstat()
    except OSError as error:
        raise LegacyImportError(f"cannot inspect dump: {error}") from error
    if not stat.S_ISREG(metadata.st_mode) or path.is_symlink():
        raise LegacyImportError("legacy dump must be a regular non-symlink file")
    if stat.S_IMODE(metadata.st_mode) & 0o077:
        raise LegacyImportError("legacy dump must have mode 0600 or stricter")
    digest = hashlib.sha256()
    header = b""
    try:
        with path.open("rb") as source:
            for line_number, line in enumerate(source, 1):
                if line_number == 1:
                    header = line
                if DATABASE_DIRECTIVE.match(line):
                    raise LegacyImportError(
                        f"legacy dump contains a database-selection directive at line {line_number}")
                digest.update(line)
    except OSError as error:
        raise LegacyImportError(f"cannot read legacy dump: {error}") from error
    if not header.startswith(b"-- MySQL dump"):
        raise LegacyImportError("legacy dump does not have a recognized mysqldump header")
    return digest.hexdigest()


def process_environment(config: dict[str, str]) -> dict[str, str]:
    environment = dict(os.environ)
    environment.update(config)
    environment["MYSQL_PWD"] = config["DB_PASSWD"]
    return environment


def connection_arguments(config: dict[str, str]) -> list[str]:
    socket_path = config.get("DB_SOCKET", "")
    if socket_path:
        return ["--protocol=socket", f"--socket={socket_path}"]
    return ["--skip-ssl", "--host", config["DB_HOST"],
            "--port", config.get("DB_PORT", "3306")]


def mysql_command(config: dict[str, str], *, database: bool = True) -> list[str]:
    command = ["mysql", *connection_arguments(config), "--user", config["DB_USER"],
               "--batch", "--skip-column-names", "--raw", "--max-allowed-packet=1G"]
    if database:
        command.append(config["DB_NAME"])
    return command


def run_mysql(config: dict[str, str], statement: str) -> str:
    result = subprocess.run(
        [*mysql_command(config), "--execute", statement],
        capture_output=True, text=True, env=process_environment(config), check=False)
    if result.returncode:
        raise LegacyImportError("database command failed: " + summarize_error(result.stderr))
    return result.stdout.strip()


def summarize_error(stderr: str | bytes) -> str:
    if isinstance(stderr, bytes):
        stderr = stderr.decode("utf-8", errors="replace")
    lines = [line.strip() for line in stderr.splitlines() if line.strip()]
    return " | ".join(lines[:8]) or "no diagnostic was returned"


def normalize_dump_line(line: bytes) -> bytes:
    if line.startswith(b"INSERT INTO "):
        return line
    line = line.replace(MYSQL8_COLLATION, PORTABLE_COLLATION)
    return DEFINER.sub(b"DEFINER=CURRENT_USER", line)


def target_objects(config: dict[str, str], table_type: str) -> list[str]:
    output = run_mysql(
        config,
        "SELECT table_name FROM information_schema.tables "
        f"WHERE table_schema=DATABASE() AND table_type='{table_type}' ORDER BY table_name;")
    names = output.splitlines() if output else []
    if any(SAFE_IDENTIFIER.fullmatch(name) is None for name in names):
        raise LegacyImportError("target contains an object with an unsafe identifier")
    return names


def target_routines(config: dict[str, str]) -> list[tuple[str, str]]:
    output = run_mysql(
        config,
        "SELECT routine_type,routine_name FROM information_schema.routines "
        "WHERE routine_schema=DATABASE() ORDER BY routine_type,routine_name;")
    routines = []
    for row in output.splitlines() if output else []:
        fields = row.split("\t")
        if len(fields) != 2 or fields[0] not in {"FUNCTION", "PROCEDURE"} or \
                SAFE_IDENTIFIER.fullmatch(fields[1]) is None:
            raise LegacyImportError("target contains a routine with an unsafe identifier")
        routines.append((fields[0], fields[1]))
    return routines


def target_events(config: dict[str, str]) -> list[str]:
    output = run_mysql(
        config,
        "SELECT event_name FROM information_schema.events "
        "WHERE event_schema=DATABASE() ORDER BY event_name;")
    names = output.splitlines() if output else []
    if any(SAFE_IDENTIFIER.fullmatch(name) is None for name in names):
        raise LegacyImportError("target contains an event with an unsafe identifier")
    return names


def active_connections(config: dict[str, str]) -> int:
    output = run_mysql(
        config,
        "SELECT COUNT(*) FROM information_schema.processlist "
        "WHERE db=DATABASE() AND id<>CONNECTION_ID();")
    try:
        return int(output)
    except ValueError as error:
        raise LegacyImportError("active database connection check returned invalid data") from error


def wipe_target(config: dict[str, str]) -> None:
    events = target_events(config)
    routines = target_routines(config)
    views = target_objects(config, "VIEW")
    tables = target_objects(config, "BASE TABLE")
    statements = [f"DROP EVENT IF EXISTS `{name}`" for name in events]
    statements.extend(
        f"DROP {routine_type} IF EXISTS `{name}`" for routine_type, name in routines)
    statements.append("SET FOREIGN_KEY_CHECKS=0")
    if views:
        statements.append("DROP VIEW IF EXISTS " + ",".join(f"`{name}`" for name in views))
    if tables:
        statements.append("DROP TABLE IF EXISTS " + ",".join(f"`{name}`" for name in tables))
    statements.append("SET FOREIGN_KEY_CHECKS=1")
    run_mysql(config, ";".join(statements) + ";")


def create_backup(config: dict[str, str], backup_path: Path) -> None:
    backup_path.parent.mkdir(mode=0o700, parents=True, exist_ok=True)
    os.chmod(backup_path.parent, 0o700)
    descriptor = os.open(backup_path, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
    command = [
        "mysqldump", *connection_arguments(config), "--user", config["DB_USER"],
        "--single-transaction", "--skip-lock-tables", "--routines", "--events",
        "--triggers", "--hex-blob", "--add-drop-table", config["DB_NAME"],
    ]
    with os.fdopen(descriptor, "wb") as destination:
        result = subprocess.run(
            command, stdout=destination, stderr=subprocess.PIPE,
            env=process_environment(config), check=False)
    if result.returncode:
        backup_path.unlink(missing_ok=True)
        raise LegacyImportError("database backup failed: " + summarize_error(result.stderr))


def import_stream(config: dict[str, str], source_path: Path, *, normalize: bool) -> None:
    with tempfile.TemporaryFile() as diagnostic:
        process = subprocess.Popen(
            mysql_command(config), stdin=subprocess.PIPE, stdout=subprocess.DEVNULL,
            stderr=diagnostic, env=process_environment(config))
        assert process.stdin is not None
        try:
            with source_path.open("rb") as source:
                for line in source:
                    if normalize:
                        line = normalize_dump_line(line)
                    process.stdin.write(line)
            process.stdin.close()
            return_code = process.wait()
        except BaseException as error:
            try:
                process.stdin.close()
            except (BrokenPipeError, OSError):
                pass
            if process.poll() is None:
                process.terminate()
            process.wait()
            diagnostic.seek(0)
            stderr = diagnostic.read()
            if isinstance(error, (BrokenPipeError, OSError)):
                raise LegacyImportError(
                    "dump restore failed: " + summarize_error(stderr)) from error
            raise
        diagnostic.seek(0)
        stderr = diagnostic.read()
        if return_code:
            raise LegacyImportError("dump restore failed: " + summarize_error(stderr))


def table_counts(config: dict[str, str]) -> dict[str, int]:
    counts: dict[str, int] = {}
    for table in target_objects(config, "BASE TABLE"):
        output = run_mysql(config, f"SELECT COUNT(*) FROM `{table}`;")
        try:
            counts[table] = int(output)
        except ValueError as error:
            raise LegacyImportError(f"row count for {table} is invalid") from error
    return counts


def runtime_tables() -> set[str]:
    try:
        manifest = json.loads(
            (ROOT / "migrations/data_lifecycle_manifest.json").read_text(encoding="utf-8"))
        tables = {entry["locator"] for entry in manifest["entries"]
                  if entry["kind"] == "database_table"}
    except (OSError, UnicodeError, json.JSONDecodeError, KeyError, TypeError) as error:
        raise LegacyImportError("runtime table inventory cannot be read") from error
    if not tables or any(SAFE_IDENTIFIER.fullmatch(table) is None for table in tables):
        raise LegacyImportError("runtime table inventory is invalid")
    return tables


def verify_source_rows(source: dict[str, int], final: dict[str, int],
                       runtime: set[str]) -> None:
    missing = sorted(set(source) - set(final))
    if missing:
        raise LegacyImportError("migrated database lost source tables: " + ", ".join(missing[:8]))
    archive_tables = {
        "account_characters": "legacy_import_account_characters",
        "player_item_affects": "legacy_import_player_item_affects",
        "player_item_extra_descr": "legacy_import_player_item_extra_descr",
        "player_pet_item_affects": "legacy_import_player_pet_item_affects",
        "player_pet_item_extra_descr": "legacy_import_player_pet_item_extra_descr",
    }
    reduced = sorted(
        table for table, count in source.items()
        if final[table] < count and table not in archive_tables)
    if reduced:
        raise LegacyImportError("migrated database lost source rows from: " + ", ".join(reduced[:8]))
    incomplete_archives = []
    for table, archive in archive_tables.items():
        if table not in source or final[table] >= source[table]:
            continue
        required = (source[table] - final[table]
                    if table == "account_characters" else source[table])
        if final.get(archive, 0) < required:
            incomplete_archives.append(table)
    if incomplete_archives:
        raise LegacyImportError(
            "deduplicated source rows were not archived for: " +
            ", ".join(sorted(incomplete_archives)[:8]))
    changed_extensions = sorted(
        table for table, count in source.items()
        if table not in runtime and final[table] != count)
    if changed_extensions:
        raise LegacyImportError(
            "migration changed extension-table row counts: " +
            ", ".join(changed_extensions[:8]))


def establish_character_baselines(config: dict[str, str]) -> None:
    """Create only unambiguous opening state before imported characters can load."""
    run_mysql(
        config,
        "START TRANSACTION;"
        "INSERT INTO currency_wallet_baseline(pid,opening_copper,opening_silver,"
        "opening_gold,opening_platinum,opening_revision) "
        "SELECT p.pid,p.copper,p.silver,p.gold,p.platinum,p.wallet_revision "
        "FROM player_data p LEFT JOIN currency_wallet_baseline b ON b.pid=p.pid "
        "WHERE b.pid IS NULL AND p.wallet_revision=0 AND NOT EXISTS "
        "(SELECT 1 FROM currency_ledger l WHERE l.pid=p.pid);"
        "INSERT INTO epic_balance_baseline(pid,opening_balance,opening_revision) "
        "SELECT p.pid,p.epics,p.epic_revision FROM player_data p "
        "LEFT JOIN epic_balance_baseline b ON b.pid=p.pid "
        "WHERE b.pid IS NULL AND p.epic_revision=0 AND NOT EXISTS "
        "(SELECT 1 FROM epic_ledger l WHERE l.pid=p.pid);"
        "INSERT INTO combat_frag_baseline(pid,opening_frags,opening_revision) "
        "SELECT p.pid,p.frags,p.frag_revision FROM player_data p "
        "LEFT JOIN combat_frag_baseline b ON b.pid=p.pid "
        "WHERE b.pid IS NULL AND p.frag_revision=0 AND NOT EXISTS "
        "(SELECT 1 FROM combat_frag_ledger l WHERE l.pid=p.pid);"
        "COMMIT;",
    )
    readiness = run_mysql(
        config,
        "SELECT COALESCE(SUM(wallet.pid IS NULL),0),"
        "COALESCE(SUM(epic.pid IS NULL),0),"
        "COALESCE(SUM(combat.pid IS NULL),0) FROM ("
        "SELECT DISTINCT p.pid FROM player_data p JOIN account_characters ac ON ac.pid=p.pid "
        "WHERE p.active=1 AND ac.deleted_at IS NULL AND ac.blocked=0) eligible "
        "LEFT JOIN currency_wallet_baseline wallet ON wallet.pid=eligible.pid "
        "LEFT JOIN epic_balance_baseline epic ON epic.pid=eligible.pid "
        "LEFT JOIN combat_frag_baseline combat ON combat.pid=eligible.pid;",
    )
    fields = readiness.split("\t")
    if len(fields) != 3 or any(not field.isdigit() for field in fields):
        raise LegacyImportError("character baseline readiness returned invalid data")
    if any(int(field) for field in fields):
        raise LegacyImportError(
            "imported character baseline readiness failed; ledger history requires review")


def run_migrations(config: dict[str, str], env_path: Path) -> None:
    """Run schema convergence, establish safe baselines, and verify readiness."""
    environment = process_environment(config)
    environment["MIGRATION_ENV_FILE"] = str(env_path)
    commands = (
        [str(ROOT / "migrations/run_migration.sh")],
        [sys.executable, str(ROOT / "scripts/migration_runner.py"), "run"],
    )
    for command in commands:
        result = subprocess.run(command, cwd=ROOT, env=environment, check=False)
        if result.returncode:
            raise LegacyImportError(f"migration command failed: {Path(command[0]).name}")
    establish_character_baselines(config)
    verifier = ROOT / "migrations/verify_runtime_compatibility.sh"
    result = subprocess.run([str(verifier)], cwd=ROOT, env=environment, check=False)
    if result.returncode:
        raise LegacyImportError(f"migration command failed: {verifier.name}")


def run_materialization_readiness(config: dict[str, str], env_path: Path) -> None:
    """Reject imported snapshots that current runtime materializers cannot load."""
    result = subprocess.run(
        [sys.executable, str(ROOT / "scripts/character_materialization_readiness.py"),
         "--env-file", str(env_path)],
        cwd=ROOT, env=process_environment(config), check=False)
    if result.returncode:
        raise LegacyImportError("character materialization readiness failed")


def restore_backup(config: dict[str, str], backup_path: Path) -> None:
    wipe_target(config)
    import_stream(config, backup_path, normalize=False)


def import_legacy_dump(config: dict[str, str], env_path: Path, dump_path: Path,
                       backup_path: Path) -> tuple[str, int, int]:
    """Replace a guarded local target and restore its backup on any failure."""
    dump_checksum = validate_dump(dump_path)
    if active_connections(config):
        raise LegacyImportError("target database has active connections; stop Duris before import")
    create_backup(config, backup_path)
    mutation_started = False
    try:
        mutation_started = True
        wipe_target(config)
        import_stream(config, dump_path, normalize=True)
        source_counts = table_counts(config)
        run_migrations(config, env_path)
        final_counts = table_counts(config)
        verify_source_rows(source_counts, final_counts, runtime_tables())
        run_materialization_readiness(config, env_path)
    except BaseException as error:
        if mutation_started:
            try:
                restore_backup(config, backup_path)
            except BaseException as restore_error:
                raise LegacyImportError(
                    f"import failed and automatic restore failed; recover from {backup_path}: "
                    f"{restore_error}") from error
        if isinstance(error, LegacyImportError):
            raise
        if isinstance(error, Exception):
            raise LegacyImportError(f"legacy import failed: {error}") from error
        raise
    return dump_checksum, len(source_counts), sum(source_counts.values())


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Back up and replace an allow-listed local Duris database from a legacy dump.")
    parser.add_argument("dump", type=Path)
    parser.add_argument("--env-file", type=Path, default=ROOT / ".env")
    parser.add_argument("--backup-dir", type=Path,
                        default=ROOT / "tmp/legacy-import-backups")
    parser.add_argument(
        "--replace", action="store_true",
        help="required acknowledgment that the configured database will be replaced")
    return parser.parse_args()


def main() -> int:
    arguments = parse_arguments()
    try:
        if not arguments.replace:
            raise LegacyImportError("refusing replacement without --replace")
        env_path = arguments.env_file.resolve()
        dump_path = arguments.dump.resolve()
        config = read_env_file(env_path)
        validate_target(config)
        timestamp = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ")
        backup_path = arguments.backup_dir.resolve() / f"{config['DB_NAME']}-{timestamp}.sql"
        checksum, tables, rows = import_legacy_dump(
            config, env_path, dump_path, backup_path)
        print(
            f"legacy import complete: source_tables={tables} source_rows={rows} "
            f"dump_sha256={checksum} backup={backup_path}")
        return 0
    except LegacyImportError as error:
        print(f"legacy import blocked: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
