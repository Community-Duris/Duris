#!/usr/bin/env python3
"""Validate synchronization of runtime, migration, lifecycle, and compiled contracts."""

from __future__ import annotations

import json
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import migration_runner  # noqa: E402
import validate_data_lifecycle as lifecycle  # noqa: E402


ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "migrations/runtime_compatibility_manifest.json"
HEADER = ROOT / "src/runtime_compatibility_contract.h"
FIELDS = {
    "manifest_version", "baseline_id", "baseline_table_count",
    "baseline_table_fingerprint", "current_table_count",
    "runtime_table_sql_list", "normalized_metadata_fingerprints", "migration_head",
    "connection", "lookup",
}
HEAD_FIELDS = {"id", "sequence", "apply_checksum", "verify_checksum",
               "history_checksum"}
CONNECTION_FIELDS = {
    "character_set", "time_zone", "isolation", "required_sql_modes",
    "connect_timeout_seconds", "read_timeout_seconds", "write_timeout_seconds",
    "remote_tls_required",
}
EXPECTED_CONNECTION = {
    "character_set": "utf8mb4",
    "time_zone": "+00:00",
    "isolation": "READ-COMMITTED",
    "required_sql_modes": [
        "STRICT_TRANS_TABLES", "ERROR_FOR_DIVISION_BY_ZERO",
        "NO_ENGINE_SUBSTITUTION",
    ],
    "connect_timeout_seconds": 10,
    "read_timeout_seconds": 10,
    "write_timeout_seconds": 10,
    "remote_tls_required": True,
}


def load() -> dict:
    raw = lifecycle.read_regular_text(MANIFEST, 1024 * 1024,
                                      "runtime compatibility manifest")
    value = json.loads(raw, object_pairs_hook=migration_runner.strict_object)
    if not isinstance(value, dict) or set(value) != FIELDS:
        raise migration_runner.MigrationContractError(
            "runtime compatibility manifest fields differ"
        )
    if value["manifest_version"] != 1 or value["baseline_table_count"] != 170 or \
            value["current_table_count"] != 173:
        raise migration_runner.MigrationContractError("runtime manifest version/count drift")
    if not isinstance(value["runtime_table_sql_list"], str) or not re.fullmatch(
            r"'[A-Za-z0-9_]+'(?:,'[A-Za-z0-9_]+')*",
            value["runtime_table_sql_list"]):
        raise migration_runner.MigrationContractError(
            "runtime table inventory is invalid")
    for field in ("baseline_table_fingerprint",):
        if not isinstance(value[field], str) or not re.fullmatch(r"[0-9a-f]{64}",
                                                                 value[field]):
            raise migration_runner.MigrationContractError("runtime fingerprint is invalid")
    fingerprints = value["normalized_metadata_fingerprints"]
    if not isinstance(fingerprints, dict) or set(fingerprints) != {
            "mysql8", "mariadb10_11"} or any(
                not isinstance(item, str) or not re.fullmatch(r"[0-9a-f]{64}", item)
                for item in fingerprints.values()):
        raise migration_runner.MigrationContractError(
            "runtime metadata fingerprints are invalid")
    if not isinstance(value["migration_head"], dict) or \
            set(value["migration_head"]) != HEAD_FIELDS or \
            not isinstance(value["connection"], dict) or \
            set(value["connection"]) != CONNECTION_FIELDS or \
            value["connection"] != EXPECTED_CONNECTION or value["lookup"] != {
                "dataset_name": "race_class", "dataset_version": 1
            }:
        raise migration_runner.MigrationContractError("runtime sub-contract fields differ")
    return value


def validate() -> dict:
    value = load()
    migration = migration_runner.load_manifest()
    if value["baseline_id"] != migration.baseline_id or \
            value["baseline_table_count"] != migration.required_table_count or \
            value["baseline_table_fingerprint"] != migration.required_table_fingerprint or \
            not migration.migrations:
        raise migration_runner.MigrationContractError("runtime and migration baseline drift")
    head = migration.migrations[-1]
    applied = [migration_runner.AppliedMigration(
        item.migration_id, item.sequence, item.description, item.apply_checksum,
        item.verify_checksum, item.compatibility, migration.runner_version,
    ) for item in migration.migrations]
    expected_head = {
        "id": head.migration_id, "sequence": head.sequence,
        "apply_checksum": head.apply_checksum, "verify_checksum": head.verify_checksum,
        "history_checksum": migration_runner.history_checksum(applied),
    }
    if value["migration_head"] != expected_head:
        raise migration_runner.MigrationContractError("runtime migration head drift")
    lifecycle_manifest = lifecycle.load_manifest(
        ROOT / "migrations/data_lifecycle_manifest.json"
    )
    tables = [entry["locator"] for entry in lifecycle_manifest["entries"]
              if entry["kind"] == "database_table"]
    if len(tables) != value["current_table_count"] or \
            "lookup_dataset_state" not in tables or "season_reset_state" not in tables or \
            "server_reboots" not in tables or \
            migration_runner.table_fingerprint(
                [table for table in tables if table not in {
                    "lookup_dataset_state", "season_reset_state", "server_reboots"
                }]
            ) != value["baseline_table_fingerprint"]:
        raise migration_runner.MigrationContractError("runtime lifecycle table drift")
    baseline_tables = [table for table in tables if table not in {
        "lookup_dataset_state", "season_reset_state", "server_reboots"
    }]
    if set(baseline_tables) != set(migration.required_tables):
        raise migration_runner.MigrationContractError(
            "migration baseline table inventory drift")
    header = HEADER.read_text()
    table_list_match = re.search(
        r"RUNTIME_TABLE_SQL_LIST\s*=\s*((?:\"[^\"]*\"\s*)+);", header)
    if table_list_match is None:
        raise migration_runner.MigrationContractError(
            "compiled runtime table inventory is absent")
    compiled_table_list = "".join(
        json.loads(literal)
        for literal in re.findall(r'"[^\"]*"', table_list_match.group(1))
    )
    expected_table_list = ",".join(f"'{table}'" for table in sorted(tables))
    if value["runtime_table_sql_list"] != expected_table_list:
        raise migration_runner.MigrationContractError(
            "runtime manifest table inventory drift")
    if compiled_table_list != expected_table_list:
        raise migration_runner.MigrationContractError(
            "compiled runtime table inventory drift")
    required_literals = (
        str(value["manifest_version"]), value["baseline_id"],
        value["baseline_table_fingerprint"], str(value["baseline_table_count"]),
        str(value["current_table_count"]),
        value["normalized_metadata_fingerprints"]["mysql8"],
        value["normalized_metadata_fingerprints"]["mariadb10_11"],
        head.migration_id, str(head.sequence), head.apply_checksum,
        head.verify_checksum, value["migration_head"]["history_checksum"],
        value["lookup"]["dataset_name"],
        str(value["lookup"]["dataset_version"]),
    )
    if any(literal not in header for literal in required_literals):
        raise migration_runner.MigrationContractError("compiled compatibility header drift")
    for constant in (
            "RUNTIME_DB_CHARACTER_SET", "RUNTIME_DB_TIME_ZONE",
            "RUNTIME_DB_ISOLATION", "RUNTIME_DB_SQL_MODE",
            "RUNTIME_DB_TIMEOUT_SECONDS", "RUNTIME_DB_REMOTE_TLS_REQUIRED",
            "RUNTIME_METADATA_MAX_BYTES"):
        if constant not in header:
            raise migration_runner.MigrationContractError(
                "compiled connection contract drift")
    return {"baseline_id": value["baseline_id"],
            "current_table_count": value["current_table_count"],
            "migration_head": head.migration_id,
            "normalized_metadata_fingerprints":
                value["normalized_metadata_fingerprints"], "status": "valid"}


if __name__ == "__main__":
    try:
        print(json.dumps(validate(), sort_keys=True))
    except (json.JSONDecodeError, lifecycle.ValidationError,
            migration_runner.MigrationContractError) as error:
        print(f"runtime compatibility validation failed: {error}", file=sys.stderr)
        raise SystemExit(2)
