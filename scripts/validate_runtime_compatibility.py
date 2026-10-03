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
HEADER = ROOT / "src/core/runtime_compatibility_contract.h"
FIELDS = {
    "manifest_version", "baseline_id", "baseline_table_count",
    "baseline_table_fingerprint", "current_table_count",
    "runtime_table_sql_list", "normalized_metadata_fingerprints", "migration_head",
    "connection", "lookup", "staging_0045_migration_head", "master_0031_migration_head",
    "migration_history_sql",
    "extra_description_generation_sql",
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
EXTRA_DESCRIPTION_GENERATION_SQL = (
    "SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() "
    "AND table_name IN ('player_item_extra_descr','player_pet_item_extra_descr') "
    "AND column_name='description_sha256' AND data_type='binary' "
    "AND character_maximum_length=32 AND LOWER(extra) LIKE '%stored generated%' "
    "AND LOWER(REPLACE(REPLACE(REPLACE(REPLACE(generation_expression,"
    "CONCAT(CHAR(92),CHAR(39),CHAR(92),CHAR(39)),CONCAT(CHAR(39),CHAR(39))),"
    "CHAR(96),''),' ',''),"
    "'_utf8mb4',''))='unhex(sha2(coalesce(description,''''),256))'"
)


# Explicit offline contract for the post-baseline death schema. This supplements
# engine-measured fingerprints; changing a migration checksum cannot bless a
# dropped column, wrong wallet type, or missing index.
DEATH_SCHEMA_DEFINITIONS = {'player_death_custody': ('pid int not null',
                          'save_revision bigint unsigned not null',
                          'item_uid bigint unsigned not null',
                          'root_item_uid bigint unsigned not null',
                          'parent_item_uid bigint unsigned not null default 0',
                          'item_revision bigint unsigned not null default 0',
                          'vnum int not null default 0',
                          'state tinyint unsigned not null default 0',
                          'owner_type tinyint unsigned not null default 0',
                          'owner_id bigint unsigned not null default 0',
                          'owner_context_id bigint unsigned not null default 0',
                          'owner_revision bigint unsigned not null default 0',
                          'primary key (pid,save_revision,item_uid)',
                          'key idx_player_death_custody_item (item_uid)'),
 'player_death_disposition': ('pid int not null',
                              'save_revision bigint unsigned not null',
                              'operation_id binary(16) not null',
                              'corpse_item_uid bigint unsigned not null',
                              'corpse_room_vnum int not null',
                              'wallet_revision bigint unsigned not null',
                              'wallet_copper int not null default 0',
                              'wallet_silver int not null default 0',
                              'wallet_gold int not null default 0',
                              'wallet_platinum int not null default 0',
                              'wallet_pile_uid bigint unsigned not null default 0',
                              'payload mediumblob not null',
                              'recorded_at timestamp(6) not null default current_timestamp(6)',
                              'primary key (pid,save_revision)',
                              'key idx_player_death_disposition_operation (operation_id)',
                              'key idx_player_death_disposition_corpse (corpse_item_uid)')}


def validate_death_schema(path: Path | None = None) -> None:
    source = lifecycle.read_schema_source(path or (
        ROOT / "migrations/immutable/0011_player_death_disposition.sql"))
    tables = re.findall(
        r"CREATE TABLE IF NOT EXISTS (\w+) \((.*?)\)\s*"
        r"ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;",
        source, re.DOTALL | re.IGNORECASE)
    definitions = {table.lower(): tuple(
        re.sub(r"\s+", " ", line.strip()).rstrip(",").lower()
        for line in body.strip().splitlines()) for table, body in tables}
    if len(tables) != 2 or definitions != DEATH_SCHEMA_DEFINITIONS:
        raise migration_runner.MigrationContractError("offline death schema shape drift")


def validate_death_conflict_schema(path: Path | None = None) -> None:
    """Pin the new evidence-only table offline as well as by engine fingerprints."""
    source = lifecycle.read_schema_source(path or (
        ROOT / "migrations/immutable/0034_player_death_conflict_evidence.sql"))
    tables = re.findall(
        r"CREATE TABLE IF NOT EXISTS (\w+) \((.*?)\)\s*"
        r"ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;",
        source, re.DOTALL | re.IGNORECASE)
    definitions = {table.lower(): tuple(
        re.sub(r"\s+", " ", line.strip()).rstrip(",").lower()
        for line in body.strip().splitlines()) for table, body in tables}
    expected = {"player_death_conflict_evidence": (
        "operation_id binary(16) not null",
        "pid int not null",
        "save_revision bigint unsigned not null",
        "source_revision bigint unsigned not null",
        "corpse_item_uid bigint unsigned not null",
        "request_hash binary(32) not null",
        "payload_hash binary(32) not null",
        "payload mediumblob not null",
        "recorded_at timestamp(6) not null default current_timestamp(6)",
        "primary key (operation_id)",
        "unique key uq_death_conflict_revision (pid,save_revision)",
        "unique key uq_death_conflict_corpse (pid,corpse_item_uid)",
    )}
    if definitions != expected or len(tables) != 1 or re.search(
            r"\b(FOREIGN KEY|REFERENCES|ON DELETE|ON UPDATE)\b", source, re.I):
        raise migration_runner.MigrationContractError(
            "offline death-conflict evidence schema shape drift")


def load() -> dict:
    """Read the runtime compatibility manifest, rejecting any shape drift.

    Every field is pinned: the exact key set, the pinned baseline and current table
    counts, the table inventory grammar, hex-width fingerprints, and the whole
    connection sub-contract. Raises MigrationContractError rather than returning a
    partially trusted manifest.
    """
    raw = lifecycle.read_regular_text(MANIFEST, 1024 * 1024,
                                      "runtime compatibility manifest")
    value = json.loads(raw, object_pairs_hook=migration_runner.strict_object)
    if not isinstance(value, dict) or set(value) != FIELDS:
        raise migration_runner.MigrationContractError(
            "runtime compatibility manifest fields differ"
        )
    if value["manifest_version"] != 1 or value["baseline_table_count"] != 170 or \
            value["current_table_count"] != 236:
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
    if any(not isinstance(value[name], dict) or set(value[name]) != HEAD_FIELDS
           for name in ("migration_head", "staging_0045_migration_head",
                        "master_0031_migration_head")) or \
            not isinstance(value["connection"], dict) or \
            set(value["connection"]) != CONNECTION_FIELDS or \
            value["connection"] != EXPECTED_CONNECTION or value["lookup"] != {
                "dataset_name": "race_class", "dataset_version": 1
            }:
        raise migration_runner.MigrationContractError("runtime sub-contract fields differ")
    return value


def validate() -> dict:
    """Prove the runtime, migration, lifecycle, and compiled contracts agree.

    Cross-checks the manifest against the migration manifest's baseline and head,
    against the lifecycle manifest's database-table inventory, and against the
    constants compiled into runtime_compatibility_contract.h. The tables created by
    immutable migrations are held out of the baseline fingerprint, which covers only
    the sealed Session 11 inventory. Returns the report a caller prints; raises
    MigrationContractError on any drift.
    """
    validate_death_schema()
    validate_death_conflict_schema()
    value = load()
    migration = migration_runner.load_manifest()
    if value["baseline_id"] != migration.baseline_id or \
            value["baseline_table_count"] != migration.required_table_count or \
            value["baseline_table_fingerprint"] != migration.required_table_fingerprint or \
            not migration.migrations:
        raise migration_runner.MigrationContractError("runtime and migration baseline drift")
    staging = migration_runner.load_manifest(
        ROOT / "migrations/migration_manifest.staging_0045.json")
    if len(migration.migrations) != 56 or len(staging.migrations) != 56 or \
            staging.baseline_id != migration.baseline_id or \
            staging.required_tables != migration.required_tables or \
            staging.migrations[:44] != migration.migrations[:44] or \
            staging.migrations[44].migration_id != \
            "0045_item_extra_description_fulltext_unique":
        raise migration_runner.MigrationContractError("unsupported staging migration fork")
    from dataclasses import replace
    if staging.migrations[45:50] != tuple(
            replace(item, sequence=item.sequence + 1)
            for item in migration.migrations[44:49]) or \
            staging.migrations[50:] != migration.migrations[50:]:
        raise migration_runner.MigrationContractError("staging migration append drift")
    master = migration_runner.load_manifest(
        ROOT / "migrations/migration_manifest.master_0031.json")
    # Receipt IDs are immutable names; sequence is the declared application order.
    # Reuse 0051's identical SQL/verifier bytes without renaming master's receipt.
    if len(master.migrations) != 56 or master.baseline_id != migration.baseline_id or \
            master.required_tables != migration.required_tables or \
            master.migrations[:30] != migration.migrations[:30] or \
            master.migrations[30] != replace(migration.migrations[50], sequence=31,
                migration_id="0031_player_item_runtime_state",
                description="Preserve complete crafted item runtime state through SQL save and load") or \
            master.migrations[31:51] != tuple(
                replace(item, sequence=item.sequence + 1)
                for item in migration.migrations[30:50]) or \
            master.migrations[51:] != migration.migrations[51:]:
        raise migration_runner.MigrationContractError("master migration append drift")
    head = migration.migrations[-1]
    for name, contract in (("migration_head", migration),
                           ("staging_0045_migration_head", staging),
                           ("master_0031_migration_head", master)):
        final = contract.migrations[-1]
        applied = [migration_runner.AppliedMigration(
            item.migration_id, item.sequence, item.description, item.apply_checksum,
            item.verify_checksum, item.compatibility, contract.runner_version,
        ) for item in contract.migrations]
        expected_head = {
            "id": final.migration_id, "sequence": final.sequence,
            "apply_checksum": final.apply_checksum,
            "verify_checksum": final.verify_checksum,
            "history_checksum": migration_runner.history_checksum(applied),
        }
        if value[name] != expected_head:
            raise migration_runner.MigrationContractError("runtime migration head drift")
    lifecycle_manifest = lifecycle.load_manifest(
        ROOT / "migrations/data_lifecycle_manifest.json"
    )
    tables = [entry["locator"] for entry in lifecycle_manifest["entries"]
              if entry["kind"] == "database_table"]
    # Discover post-baseline tables from the immutable schema sources instead of
    # maintaining an exclusion list that can hide an unregistered migration.
    immutable_tables = lifecycle.schema_tables(tuple(
        item.apply_path for item in migration.migrations))
    post_baseline_tables = immutable_tables - set(migration.required_tables)
    if set(tables) != set(migration.required_tables) | post_baseline_tables or \
            len(tables) != value["current_table_count"]:
        raise migration_runner.MigrationContractError("runtime lifecycle table drift")
    baseline_tables = [table for table in tables if table not in post_baseline_tables]
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
    for field, constant, expected_sql in (
            ("migration_history_sql", "RUNTIME_MIGRATION_HISTORY_SQL",
             migration_runner.runtime_history_sql(len(migration.migrations) + 1)),
            ("extra_description_generation_sql", "RUNTIME_EXTRA_DESCRIPTION_GENERATION_SQL",
             EXTRA_DESCRIPTION_GENERATION_SQL)):
        match = re.search(rf"{constant}\s*=\s*((?:\"[^\"]*\"\s*)+);", header)
        compiled = "".join(json.loads(literal) for literal in
                           re.findall(r'"[^\"]*"', match.group(1))) if match else None
        if value[field] != expected_sql or compiled != expected_sql:
            raise migration_runner.MigrationContractError("compiled runtime query drift")
    for suffix, field in (("HEAD_ID", "id"), ("HEAD_SEQUENCE", "sequence"),
                          ("APPLY_CHECKSUM", "apply_checksum"),
                          ("VERIFY_CHECKSUM", "verify_checksum"),
                          ("HISTORY_CHECKSUM", "history_checksum")):
        for prefix, name in (("RUNTIME_MIGRATION_", "migration_head"),
                             ("RUNTIME_STAGING_0045_MIGRATION_",
                              "staging_0045_migration_head"),
                             ("RUNTIME_MASTER_0031_MIGRATION_",
                              "master_0031_migration_head")):
            match = re.search(rf'{prefix}{suffix}\s*=\s*("[^\"]*"|[0-9]+);', header)
            if match is None or json.loads(match.group(1)) != value[name][field]:
                raise migration_runner.MigrationContractError("compiled runtime history drift")
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
