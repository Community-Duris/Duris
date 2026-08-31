#!/usr/bin/env python3
"""Fail-closed validation for the Duris technical data-lifecycle inventory."""

from __future__ import annotations

import argparse
import json
import os
import re
import stat
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_MANIFEST = ROOT / "migrations" / "data_lifecycle_manifest.json"
DEFAULT_REDIS_REGISTRY = ROOT / "src/redis/redis_key_registry.def"
DEFAULT_SCHEMA_FILES = (
    ROOT / "migrations" / "bootstrap_multithread_safe.sql",
    ROOT / "migrations" / "bootstrap_legacy_baseline.sql",
    ROOT / "migrations" / "immutable" / "0001_lookup_dataset_state.sql",
    ROOT / "migrations" / "immutable" / "0003_season_reset_state.sql",
    ROOT / "migrations" / "immutable" / "0004_server_reboots.sql",
)

ROOT_FIELDS = {
    "schema_version", "policy_id", "status", "technical_owner_default",
    "decision_semantics", "allowed_actions", "destructive_actions",
    "controller_approval", "export_policy", "entries",
}
ENTRY_FIELDS = {
    "id", "kind", "locator", "data_category", "data_subject_key",
    "technical_purpose", "controller_decision", "season_action",
    "active_retention", "archive_retention", "terminal_action", "exception",
    "protected_record", "dependencies", "export_rule",
}
EXPORT_RULE_FIELDS = {
    "disposition", "subject_route", "decision", "excluded_fields", "shared_fields",
}
EXPORT_DISPOSITIONS = {"include", "exclude", "shared_redacted", "pending"}
EXPORT_ROUTES = {
    "direct_account", "direct_player", "dependency", "operation_domain", "aggregate",
    "not_subject_scoped", "non_database", "lifecycle_metadata", "shared_security",
    "archive_source",
}
APPROVAL_FIELDS = {"status", "reference"}
GLOBAL_APPROVAL_FIELDS = {"status", "reference", "destructive_rules_enabled"}
EXPORT_POLICY_FIELDS = {
    "status", "reference", "shared_disclosure_enabled", "bundle_ttl_seconds",
    "row_budget", "byte_budget",
}
SEASON_ACTIONS = {"retain", "deactivate", "reset_delete", "reset_update", "regenerate"}
ALLOWED_ACTIONS = {
    "retain", "regenerate", "deactivate", "reset_delete", "reset_update",
    "archive", "purge", "pseudonymize", "cascade", "restore_tombstone",
}
DESTRUCTIVE_ACTIONS = {
    "archive", "purge", "pseudonymize", "cascade", "restore_tombstone",
}
REQUIRED_NON_DATABASE_STORES = {
    "file:player_save_journal": ("journal", "PLAYER_SAVE_JOURNAL_DIR/player-save.journal"),
    "file:player_save_quarantine": (
        "quarantine", "PLAYER_SAVE_JOURNAL_DIR/player-save.journal.quarantine",
    ),
    "file:critical_command_journal": ("journal", "CRITICAL_COMMAND_JOURNAL_DIR"),
    "file:persistence_fallback": ("fallback", "legacy persistence fallback file"),
    "file:persistence_fallback_quarantine": (
        "quarantine", "legacy persistence fallback quarantine",
    ),
    "file:runtime_pfiles": ("runtime_file", "lib/players"),
    "file:runtime_accounts": ("runtime_file", "lib/accounts"),
    "file:player_logs": ("log", "logs/player-log"),
    "file:server_logs": ("log", "logs"),
    "file:statistics_history": ("report", "lib/statistics/statistics_general*"),
    "file:web_status": ("report", "lib/reports/status"),
    "file:maintenance_scheduler_state": ("recovery_state", "MAINTENANCE_STATE_FILE"),
    "file:export_spool": ("export_spool", "operator-configured export spool"),
    "backup:database": ("backup", "mysqldump backup class"),
    "backup:pfiles": ("backup", "Players/Backup"),
    "backup:conversion": ("backup", "*.preconvert and *.backup conversion artifacts"),
}
MAX_MANIFEST_BYTES = 2 * 1024 * 1024
MAX_SCHEMA_BYTES = 8 * 1024 * 1024
MAX_REDIS_REGISTRY_BYTES = 64 * 1024
PROTECTED_EXCEPTIONS = {
    "protected_reconciliation_or_replay_horizon",
    "protected_recovery_replay_or_restore_horizon",
}
REQUIRED_SECRET_EXCLUSIONS = {
    "database:accounts": {"password", "confirmation_code"},
    "database:private_chests": {"password_hash"},
    "database:critical_operation_inbox": {"command_hash", "keys_hash", "result_payload"},
    "database:critical_outbox": {"payload"},
    "database:personal_data_export_requests": {"delivery_token_hash"},
    "file:runtime_accounts": {"password", "confirmation_code"},
    "file:server_logs": {"raw_security_events"},
    "file:player_logs": {"raw_security_events"},
    "file:critical_command_journal": {"command_payload"},
}


class ValidationError(Exception):
    pass


def require_exact_fields(value: object, expected: set[str], label: str) -> None:
    if not isinstance(value, dict):
        raise ValidationError(f"{label} must be an object")
    actual = set(value)
    if actual != expected:
        raise ValidationError(
            f"{label} fields differ: missing={sorted(expected - actual)} "
            f"unknown={sorted(actual - expected)}"
        )


def read_regular_text(path: Path, maximum_bytes: int, label: str) -> str:
    flags = os.O_RDONLY | getattr(os, "O_CLOEXEC", 0) | getattr(os, "O_NOFOLLOW", 0)
    try:
        descriptor = os.open(path, flags)
    except OSError as error:
        raise ValidationError(
            f"{label} must be a regular non-symlink file: {path}: {error.strerror}"
        ) from error
    try:
        metadata = os.fstat(descriptor)
        if not stat.S_ISREG(metadata.st_mode) or metadata.st_size > maximum_bytes:
            raise ValidationError(
                f"{label} must be a bounded regular non-symlink file: {path}"
            )
        chunks: list[bytes] = []
        total = 0
        while True:
            chunk = os.read(descriptor, min(65536, maximum_bytes + 1 - total))
            if not chunk:
                break
            chunks.append(chunk)
            total += len(chunk)
            if total > maximum_bytes:
                raise ValidationError(f"{label} exceeds size limit: {path}")
    finally:
        os.close(descriptor)
    try:
        return b"".join(chunks).decode("utf-8")
    except UnicodeDecodeError as error:
        raise ValidationError(f"{label} is not valid UTF-8: {path}") from error


def redis_registry_inventory(
    path: Path = DEFAULT_REDIS_REGISTRY,
) -> tuple[dict[str, tuple[str, str]], int]:
    text = read_regular_text(path, MAX_REDIS_REGISTRY_BYTES, "Redis key registry")
    store_pattern = re.compile(
        r'^REDIS_STORE\([A-Z0-9_]+, "([^"]+)", "([^"]+)", "([^"]+)"\)$'
    )
    surface_pattern = re.compile(
        r'^REDIS_SURFACE\(([A-Z0-9_]+), "[^"]+", "[^"]+", '
        r'([A-Z0-9_]+), "[^"]+", "(?:active|cleanup_only)"\)$'
    )
    owned_pattern = re.compile(r'^REDIS_OWNED_PATTERN\([A-Z0-9_]+, "[^"]+"\)$')
    stores: dict[str, tuple[str, str]] = {}
    store_symbols: dict[str, str] = {}
    surfaces: set[str] = set()
    surface_stores: set[str] = set()
    for number, raw_line in enumerate(text.splitlines(), 1):
        line = raw_line.strip()
        if not line:
            continue
        store = store_pattern.fullmatch(line)
        if store:
            lifecycle_id, locator, kind = store.groups()
            symbol = line.split("(", 1)[1].split(",", 1)[0]
            if lifecycle_id in stores or symbol in store_symbols:
                raise ValidationError(f"duplicate Redis registry store at line {number}")
            stores[lifecycle_id] = (kind, locator)
            store_symbols[symbol] = lifecycle_id
            continue
        surface = surface_pattern.fullmatch(line)
        if surface:
            name, store_symbol = surface.groups()
            if name in surfaces:
                raise ValidationError(f"duplicate Redis registry surface at line {number}")
            surfaces.add(name)
            surface_stores.add(store_symbol)
            continue
        if owned_pattern.fullmatch(line):
            continue
        raise ValidationError(f"invalid Redis registry declaration at line {number}")
    unknown_stores = surface_stores - set(store_symbols)
    unused_stores = set(store_symbols) - surface_stores
    if not stores or not surfaces or unknown_stores or unused_stores:
        raise ValidationError(
            f"Redis registry coverage mismatch: unknown={sorted(unknown_stores)} "
            f"unused={sorted(unused_stores)}"
        )
    return stores, len(surfaces)


def schema_tables(paths: tuple[Path, ...]) -> set[str]:
    tables: set[str] = set()
    statement = re.compile(
        r"(CREATE|DROP)\s+TABLE\s+(?:IF\s+(?:NOT\s+)?EXISTS\s+)?[`\"]?(\w+)",
        re.IGNORECASE,
    )
    for path in paths:
        source = read_regular_text(path, MAX_SCHEMA_BYTES, "schema source")
        for operation, table in statement.findall(source):
            if operation.upper() == "CREATE":
                tables.add(table.lower())
            else:
                tables.discard(table.lower())
    if not tables:
        raise ValidationError("schema inventory is empty")
    return tables


def schema_dependencies(paths: tuple[Path, ...]) -> dict[str, set[str]]:
    """Extract declared foreign-key parent tables from authoritative bootstrap SQL."""
    dependencies: dict[str, set[str]] = {}
    create_table = re.compile(
        r"CREATE\s+TABLE\s+(?:IF\s+NOT\s+EXISTS\s+)?[`\"]?(\w+)[`\"]?"
        r"\s*\((.*?)\)\s*(?:ENGINE|;)",
        re.IGNORECASE | re.DOTALL,
    )
    reference = re.compile(r"REFERENCES\s+[`\"]?(\w+)", re.IGNORECASE)
    for path in paths:
        source = read_regular_text(path, MAX_SCHEMA_BYTES, "schema source")
        for match in create_table.finditer(source):
            table = match.group(1).lower()
            parents = {parent.lower() for parent in reference.findall(match.group(2))}
            parents.discard(table)
            if parents:
                dependencies[table] = parents
    return dependencies


def load_manifest(path: Path) -> dict:
    def unique_object(pairs: list[tuple[str, object]]) -> dict:
        value = {}
        for key, item in pairs:
            if key in value:
                raise ValidationError(f"manifest contains duplicate field: {key}")
            value[key] = item
        return value

    try:
        value = json.loads(
            read_regular_text(path, MAX_MANIFEST_BYTES, "manifest"),
            object_pairs_hook=unique_object,
        )
    except json.JSONDecodeError as error:
        raise ValidationError(f"manifest parse failed: {error}") from error
    if not isinstance(value, dict):
        raise ValidationError("manifest root must be an object")
    return value


def validate_dependency_graph(entries: dict[str, dict]) -> None:
    visiting: set[str] = set()
    visited: set[str] = set()

    def visit(entry_id: str) -> None:
        if entry_id in visiting:
            raise ValidationError(f"dependency cycle includes {entry_id}")
        if entry_id in visited:
            return
        visiting.add(entry_id)
        for dependency in entries[entry_id]["dependencies"]:
            if dependency not in entries:
                raise ValidationError(f"{entry_id} has missing dependency {dependency}")
            visit(dependency)
        visiting.remove(entry_id)
        visited.add(entry_id)

    for entry_id in entries:
        visit(entry_id)


def validate_manifest(manifest: dict, expected_tables: set[str],
                      foreign_keys: dict[str, set[str]],
                      redis_registry: Path = DEFAULT_REDIS_REGISTRY) -> dict[str, dict]:
    require_exact_fields(manifest, ROOT_FIELDS, "manifest")
    if type(manifest["schema_version"]) is not int or manifest["schema_version"] != 1 or \
            manifest["policy_id"] != "duris-lifecycle-v1":
        raise ValidationError("unsupported or stale lifecycle policy version")
    if manifest["status"] != "technical_inventory_destructive_rules_disabled":
        raise ValidationError("manifest status must not claim legal approval or enable destruction")
    for field in ("technical_owner_default", "decision_semantics"):
        if not isinstance(manifest[field], str) or not manifest[field]:
            raise ValidationError(f"manifest {field} must be a non-empty string")
    allowed_actions = manifest["allowed_actions"]
    destructive_actions = manifest["destructive_actions"]
    if not isinstance(allowed_actions, list) or \
            not all(isinstance(action, str) for action in allowed_actions) or \
            set(allowed_actions) != ALLOWED_ACTIONS or \
            len(allowed_actions) != len(ALLOWED_ACTIONS):
        raise ValidationError("allowed_actions must match the canonical action set")
    if not isinstance(destructive_actions, list) or \
            not all(isinstance(action, str) for action in destructive_actions) or \
            set(destructive_actions) != DESTRUCTIVE_ACTIONS or \
            len(destructive_actions) != len(DESTRUCTIVE_ACTIONS):
        raise ValidationError("destructive_actions must match the canonical destructive set")
    require_exact_fields(manifest["controller_approval"], GLOBAL_APPROVAL_FIELDS,
                         "controller_approval")
    global_approval = manifest["controller_approval"]
    if global_approval["status"] != "pending" or \
            global_approval["reference"] != "PENDING-CONTROLLER-DECISION" or \
            global_approval["destructive_rules_enabled"] is not False:
        raise ValidationError("controller approval must keep destructive rules disabled")
    require_exact_fields(manifest["export_policy"], EXPORT_POLICY_FIELDS,
                         "export_policy")
    export_policy = manifest["export_policy"]
    if export_policy["status"] != "pending" or \
            export_policy["reference"] != "PENDING-SHARED-DISCLOSURE-DECISION" or \
            export_policy["shared_disclosure_enabled"] is not False or \
            type(export_policy["bundle_ttl_seconds"]) is not int or \
            not 300 <= export_policy["bundle_ttl_seconds"] <= 86400 or \
            type(export_policy["row_budget"]) is not int or \
            not 1 <= export_policy["row_budget"] <= 256 or \
            type(export_policy["byte_budget"]) is not int or \
            not 1 <= export_policy["byte_budget"] <= 1048576:
        raise ValidationError("export policy must remain bounded and disclosure-disabled")
    raw_entries = manifest["entries"]
    if not isinstance(raw_entries, list) or not raw_entries:
        raise ValidationError("entries must be a non-empty list")

    entries: dict[str, dict] = {}
    database_tables: set[str] = set()
    non_database_ids: set[str] = set()
    required_non_database_stores = dict(REQUIRED_NON_DATABASE_STORES)
    redis_stores, _ = redis_registry_inventory(redis_registry)
    required_non_database_stores.update(redis_stores)
    for index, entry in enumerate(raw_entries):
        if not isinstance(entry, dict):
            raise ValidationError(f"entry {index} must be an object")
        require_exact_fields(entry, ENTRY_FIELDS, f"entry {index}")
        entry_id = entry["id"]
        if not isinstance(entry_id, str) or not re.fullmatch(r"[a-z_]+:[a-z0-9_*.-]+", entry_id):
            raise ValidationError(f"entry {index} has invalid id")
        if entry_id in entries:
            raise ValidationError(f"duplicate store id: {entry_id}")
        entries[entry_id] = entry
        for field in ("kind", "locator", "data_category", "data_subject_key",
                      "technical_purpose", "active_retention", "archive_retention",
                      "terminal_action", "exception"):
            if not isinstance(entry[field], str) or not entry[field]:
                raise ValidationError(f"{entry_id} has invalid {field}")
        require_exact_fields(entry["controller_decision"], APPROVAL_FIELDS,
                             f"{entry_id}.controller_decision")
        if not isinstance(entry["controller_decision"]["status"], str) or \
                entry["controller_decision"]["status"] not in {
                    "approved", "pending", "not_required",
                }:
            raise ValidationError(f"{entry_id} has unknown approval status")
        if not isinstance(entry["controller_decision"]["reference"], str) or \
                not entry["controller_decision"]["reference"]:
            raise ValidationError(f"{entry_id} is missing an approval reference")
        export_rule = entry["export_rule"]
        require_exact_fields(export_rule, EXPORT_RULE_FIELDS, f"{entry_id}.export_rule")
        require_exact_fields(export_rule["decision"], APPROVAL_FIELDS,
                             f"{entry_id}.export_rule.decision")
        if not isinstance(export_rule["disposition"], str) or \
                export_rule["disposition"] not in EXPORT_DISPOSITIONS or \
                not isinstance(export_rule["subject_route"], str) or \
                export_rule["subject_route"] not in EXPORT_ROUTES:
            raise ValidationError(f"{entry_id} has unknown export disposition or route")
        if not isinstance(export_rule["decision"]["status"], str) or \
                export_rule["decision"]["status"] not in {
            "approved", "pending", "not_required",
        } or not isinstance(export_rule["decision"]["reference"], str) or \
                not export_rule["decision"]["reference"]:
            raise ValidationError(f"{entry_id} has invalid export decision")
        for export_field in ("excluded_fields", "shared_fields"):
            values = export_rule[export_field]
            if not isinstance(values, list) or \
                not all(isinstance(value, str) and value for value in values) or \
                    not all(re.fullmatch(r"[a-z0-9_.*-]+", value) for value in values) or \
                    len(set(values)) != len(values):
                raise ValidationError(f"{entry_id} has invalid {export_field}")
        if export_rule["disposition"] in {"include", "shared_redacted"} and \
                export_rule["decision"]["status"] != "approved":
            raise ValidationError(f"{entry_id} has unapproved export disclosure rule")
        if export_rule["disposition"] == "pending" and \
                export_rule["decision"]["status"] != "pending":
            raise ValidationError(f"{entry_id} pending export rule lacks pending decision")
        if export_rule["disposition"] == "exclude" and \
                export_rule["decision"]["status"] != "not_required":
            raise ValidationError(f"{entry_id} excluded export rule has invalid decision")
        if set(export_rule["excluded_fields"]) & set(export_rule["shared_fields"]):
            raise ValidationError(f"{entry_id} shares an explicitly excluded field")
        required_secrets = REQUIRED_SECRET_EXCLUSIONS.get(entry_id, set())
        if not required_secrets <= set(export_rule["excluded_fields"]):
            raise ValidationError(
                f"{entry_id} omits secret exclusions {sorted(required_secrets - set(export_rule['excluded_fields']))}"
            )
        if not isinstance(entry["season_action"], str) or \
                entry["season_action"] not in SEASON_ACTIONS:
            raise ValidationError(f"{entry_id} has unknown season action")
        if entry["terminal_action"] not in allowed_actions:
            raise ValidationError(f"{entry_id} has unknown terminal action")
        if not isinstance(entry["dependencies"], list) or \
                not all(isinstance(dependency, str) for dependency in entry["dependencies"]) or \
                len(set(entry["dependencies"])) != len(entry["dependencies"]):
            raise ValidationError(f"{entry_id} dependencies must be a unique list")
        if not isinstance(entry["protected_record"], bool):
            raise ValidationError(f"{entry_id} protected_record must be boolean")
        if entry["protected_record"] and entry["exception"] not in PROTECTED_EXCEPTIONS:
            raise ValidationError(f"{entry_id} protected record lacks a recognized exception")
        if entry["terminal_action"] in destructive_actions:
            if entry["protected_record"]:
                raise ValidationError(f"protected store {entry_id} cannot use destructive action")
            decision = entry["controller_decision"]
            if decision["status"] != "approved" or decision["reference"].startswith("PENDING"):
                raise ValidationError(f"unapproved destructive rule for {entry_id}")
        if entry["kind"] == "database_table":
            if not entry_id.startswith("database:") or entry["locator"] != entry_id.split(":", 1)[1]:
                raise ValidationError(f"database entry mismatch for {entry_id}")
            database_tables.add(entry["locator"])
        else:
            expected_store = required_non_database_stores.get(entry_id)
            if expected_store != (entry["kind"], entry["locator"]):
                raise ValidationError(f"non-database entry mismatch for {entry_id}")
            non_database_ids.add(entry_id)

    missing_tables = expected_tables - database_tables
    unknown_tables = database_tables - expected_tables
    if missing_tables or unknown_tables:
        raise ValidationError(
            f"schema coverage mismatch: missing={sorted(missing_tables)} "
            f"unknown={sorted(unknown_tables)}"
        )
    required_non_database_ids = set(required_non_database_stores)
    if non_database_ids != required_non_database_ids:
        raise ValidationError(
            f"non-database coverage mismatch: "
            f"missing={sorted(required_non_database_ids - non_database_ids)} "
            f"unknown={sorted(non_database_ids - required_non_database_ids)}"
        )
    for table, parents in foreign_keys.items():
        entry_id = f"database:{table}"
        required = {f"database:{parent}" for parent in parents}
        declared = set(entries[entry_id]["dependencies"])
        if not required <= declared:
            raise ValidationError(
                f"{entry_id} omits schema dependencies {sorted(required - declared)}"
            )
    validate_dependency_graph(entries)
    return entries


def destructive_preflight(manifest: dict, entries: dict[str, dict], entry_id: str,
                          action: str) -> None:
    if entry_id not in entries:
        raise ValidationError("destructive preflight references an unknown store")
    if action not in manifest["destructive_actions"]:
        raise ValidationError("destructive preflight requires a declared destructive action")
    if entries[entry_id]["terminal_action"] != action:
        raise ValidationError("requested action differs from the versioned store rule")
    if entries[entry_id]["protected_record"]:
        raise ValidationError("protected reconciliation/replay stores cannot be destructive targets")
    global_approval = manifest["controller_approval"]
    if global_approval["status"] != "approved" or not global_approval["destructive_rules_enabled"]:
        raise ValidationError("controller approval does not enable destructive rules")
    environment = os.environ.get("ENVIRONMENT", "").lower()
    role = os.environ.get("LIFECYCLE_ROLE", "")
    host = os.environ.get("DB_HOST", "").lower()
    if environment not in {"local", "development", "dev", "test"}:
        raise ValidationError("destructive preflight is restricted to non-production")
    if host not in {"127.0.0.1", "localhost", "::1"}:
        raise ValidationError("destructive preflight requires a loopback database")
    if role != "lifecycle-admin":
        raise ValidationError("destructive preflight requires lifecycle-admin role")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--manifest", type=Path, default=DEFAULT_MANIFEST)
    parser.add_argument("--schema-file", type=Path, action="append")
    parser.add_argument("--redis-registry", type=Path, default=DEFAULT_REDIS_REGISTRY)
    parser.add_argument("--destructive-preflight", nargs=2, metavar=("STORE_ID", "ACTION"))
    parser.add_argument("--json", action="store_true")
    return parser.parse_args()


def main() -> int:
    arguments = parse_args()
    try:
        manifest = load_manifest(arguments.manifest)
        schema_paths = tuple(arguments.schema_file) if arguments.schema_file else DEFAULT_SCHEMA_FILES
        tables = schema_tables(schema_paths)
        dependencies = schema_dependencies(schema_paths)
        entries = validate_manifest(manifest, tables, dependencies, arguments.redis_registry)
        if arguments.destructive_preflight:
            destructive_preflight(manifest, entries, *arguments.destructive_preflight)
    except ValidationError as error:
        print(f"lifecycle validation failed: {error}", file=sys.stderr)
        return 2
    result = {
        "status": "valid",
        "policy_id": manifest["policy_id"],
        "database_tables": len(tables),
        "non_database_stores": len(entries) - len(tables),
        "redis_surfaces": redis_registry_inventory(arguments.redis_registry)[1],
        "destructive_rules_enabled": False,
    }
    print(json.dumps(result, sort_keys=True) if arguments.json else
          "lifecycle manifest valid: " + " ".join(f"{key}={value}" for key, value in result.items()))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
