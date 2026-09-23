#!/usr/bin/env python3
"""Static contract checks for the additive #268 rollup support schema."""

from __future__ import annotations

import hashlib
import json
import re
import sys
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
MIGRATION = ROOT / "migrations/immutable/0017_telemetry_rollup_support.sql"
BOOTSTRAP = ROOT / "migrations/bootstrap_multithread_safe.sql"
MIGRATIONS = ROOT / "migrations/migration_manifest.json"
RUNTIME = ROOT / "migrations/runtime_compatibility_manifest.json"
LIFECYCLE = ROOT / "migrations/data_lifecycle_manifest.json"
VERIFIER = ROOT / "migrations/immutable/0017_telemetry_rollup_support.sh"
sys.path.insert(0, str(ROOT / "scripts"))
import validate_data_lifecycle as lifecycle  # noqa: E402


TABLES = {"telemetry_rollup_session", "telemetry_cohort_member"}

SESSION_COLUMNS = (
    "definition_version int unsigned not null",
    "generation bigint unsigned not null",
    "environment_id bigint unsigned not null",
    "season_id bigint unsigned not null",
    "session_boot_id bigint unsigned not null",
    "session_process_id bigint unsigned not null",
    "session_seq bigint unsigned not null",
    "subject_id bigint unsigned not null",
    "pid int not null",
    "latest_checkpoint_revision bigint unsigned not null default 0",
    "connected_usec bigint unsigned not null default 0",
    "active_usec bigint unsigned not null default 0",
    "idle_usec bigint unsigned not null default 0",
    "unknown_usec bigint unsigned not null default 0",
    "resident_usec bigint unsigned not null default 0",
    "linkdead_usec bigint unsigned not null default 0",
    "covered_connected_usec bigint unsigned not null default 0",
    "covered_active_usec bigint unsigned not null default 0",
    "covered_idle_usec bigint unsigned not null default 0",
    "covered_unknown_usec bigint unsigned not null default 0",
    "covered_resident_usec bigint unsigned not null default 0",
    "covered_linkdead_usec bigint unsigned not null default 0",
    "attributable_usec bigint unsigned not null default 0",
    "observed_intervals bigint unsigned not null default 0",
    "entered tinyint unsigned not null default 0",
    "exited tinyint unsigned not null default 0",
    "end_reason tinyint unsigned not null default 0",
    "quality_flags int unsigned not null default 0",
    "input_watermark bigint unsigned not null default 0",
    "provisional tinyint unsigned not null default 1",
)
COHORT_COLUMNS = (
    "definition_version int unsigned not null",
    "generation bigint unsigned not null",
    "environment_id bigint unsigned not null",
    "season_id bigint unsigned not null",
    "utc_day date not null",
    "level_band smallint unsigned not null",
    "class_id smallint unsigned not null",
    "race_id smallint unsigned not null",
    "faction_id smallint unsigned not null",
    "zone_vnum int not null",
    "config_id bigint unsigned not null",
    "category tinyint unsigned not null",
    "membership_kind tinyint unsigned not null",
    "subject_id bigint unsigned not null",
    "session_boot_id bigint unsigned not null",
    "session_process_id bigint unsigned not null",
    "session_seq bigint unsigned not null",
    "duration_usec bigint unsigned not null default 0",
    "attributable_usec bigint unsigned not null default 0",
    "observed_intervals bigint unsigned not null default 0",
    "quality_flags int unsigned not null default 0",
    "input_watermark bigint unsigned not null default 0",
)
PRIMARY_SESSION = (
    "primary key (definition_version,generation,environment_id,season_id,"
    "session_boot_id,session_process_id,session_seq)"
)
PRIMARY_COHORT = (
    "primary key (definition_version,generation,environment_id,season_id,utc_day,"
    "level_band,class_id,race_id,faction_id,zone_vnum,config_id,category,"
    "subject_id,session_boot_id,session_process_id,session_seq)"
)
COHORT_CHECKS = (
    "constraint chk_telemetry_cohort_member_kind check ((membership_kind = 1 and "
    "session_boot_id = 0 and session_process_id = 0 and session_seq = 0) or "
    "(membership_kind = 2 and session_boot_id <> 0 and session_process_id <> 0 and "
    "session_seq <> 0))",
    "constraint chk_telemetry_cohort_member_subject check (subject_id <> 0)",
)
SESSION_INDEX = (
    "key idx_rollup_session_subject (definition_version,generation,environment_id,"
    "season_id,subject_id,session_boot_id,session_process_id,session_seq)"
)


def normalize(line: str) -> str:
    line = line.strip().rstrip(",").replace("`", "").lower()
    line = re.sub(r"default '([0-9]+)'", r"default \1", line)
    line = re.sub(r"\s*,\s*", ",", line)
    return re.sub(r"\s+", " ", line)


def create_tables(source: str) -> dict[str, list[str]]:
    pattern = re.compile(
        r"CREATE TABLE(?: IF NOT EXISTS)?\s+`?(\w+)`?\s*\((.*?)\)\s*"
        r"ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci",
        re.IGNORECASE | re.DOTALL,
    )
    return {
        name.lower(): [normalize(line) for line in body.splitlines() if line.strip()]
        for name, body in pattern.findall(source)
    }


def split_shape(lines: list[str]) -> tuple[list[str], list[str]]:
    marker = next(i for i, line in enumerate(lines) if line.startswith(("primary key", "unique key", "key ")))
    return lines[:marker], lines[marker:]


class TelemetryRollupSchemaTest(unittest.TestCase):
    def setUp(self) -> None:
        self.sql = MIGRATION.read_text()
        self.migration_tables = create_tables(self.sql)
        self.bootstrap_tables = create_tables(BOOTSTRAP.read_text())

    def test_exact_new_store_shapes_and_bootstrap_parity(self) -> None:
        self.assertEqual(set(self.migration_tables), TABLES)
        self.assertEqual(self.sql.upper().count("ENGINE=INNODB"), 2)
        self.assertNotRegex(self.sql.upper(), r"FOREIGN KEY|CREATE TRIGGER|CREATE EVENT|PARTITION BY")
        self.assertNotRegex(self.sql.upper(), r"\b(?:JSON|BLOB|TEXT|VARCHAR)\b")

        expected = {
            "telemetry_rollup_session": (SESSION_COLUMNS, [normalize(PRIMARY_SESSION), normalize(SESSION_INDEX)]),
            "telemetry_cohort_member": (
                COHORT_COLUMNS,
                [normalize(PRIMARY_COHORT), *map(normalize, COHORT_CHECKS)],
            ),
        }
        for table, (columns, indexes) in expected.items():
            self.assertIn(table, self.bootstrap_tables)
            actual_columns, actual_indexes = split_shape(self.migration_tables[table])
            bootstrap_columns, bootstrap_indexes = split_shape(self.bootstrap_tables[table])
            self.assertEqual(actual_columns, list(columns), table)
            self.assertEqual(actual_indexes, indexes, table)
            self.assertEqual(bootstrap_columns, actual_columns, table)
            self.assertEqual(bootstrap_indexes, actual_indexes, table)

    def test_primary_keys_are_replay_identity_and_no_mutable_ddl_is_present(self) -> None:
        self.assertIn("CREATE TABLE IF NOT EXISTS telemetry_rollup_session", self.sql)
        self.assertIn("CREATE TABLE IF NOT EXISTS telemetry_cohort_member", self.sql)
        self.assertIn(normalize(PRIMARY_SESSION), self.migration_tables["telemetry_rollup_session"])
        self.assertIn(normalize(PRIMARY_COHORT), self.migration_tables["telemetry_cohort_member"])
        self.assertNotIn("unique key", self.sql.lower())
        self.assertNotRegex(self.sql, r"(?im)^\s*(?:ALTER|UPDATE|DELETE|DROP)\b")

    def test_verifier_requires_exact_check_expressions_and_mysql_enforcement(self) -> None:
        verifier = VERIFIER.read_text()
        self.assertIn("information_schema.check_constraints", verifier)
        self.assertIn("check_clause", verifier)
        self.assertIn("t.enforced", verifier)
        self.assertIn("CHECK constraints are not enforced", verifier)
        self.assertIn("MariaDB", verifier)
        self.assertIn("KIND_CHECK_CANONICAL", verifier)
        self.assertIn("SUBJECT_CHECK_CANONICAL", verifier)
        self.assertIn("check_check_constraints", verifier)
        self.assertNotIn("check names ONLY", verifier)

    def test_check_expression_contract_preserves_disjunct_grouping(self) -> None:
        normalized = normalize(COHORT_CHECKS[0])
        self.assertIn("(membership_kind = 1 and", normalized)
        self.assertIn(") or (membership_kind = 2 and", normalized)
        self.assertEqual(normalize(COHORT_CHECKS[1]), "constraint chk_telemetry_cohort_member_subject check (subject_id <> 0)")

    def test_registered_manifest_and_computed_inventories(self) -> None:
        migration = json.loads(MIGRATIONS.read_text())
        item = next(entry for entry in migration["migrations"]
                    if entry["id"] == "0017_telemetry_rollup_support")
        self.assertEqual(item["id"], "0017_telemetry_rollup_support")
        self.assertEqual(item["sequence"], 17)
        self.assertEqual(
            item["apply_checksum"], hashlib.sha256(MIGRATION.read_bytes()).hexdigest()
        )
        self.assertEqual(
            item["verify_checksum"], hashlib.sha256(VERIFIER.read_bytes()).hexdigest()
        )

        runtime = json.loads(RUNTIME.read_text())
        runtime_tables = runtime["runtime_table_sql_list"].split(",")
        runtime_tables = [name.strip("'") for name in runtime_tables]
        expected_tables = lifecycle.schema_tables(lifecycle.DEFAULT_SCHEMA_FILES)
        self.assertEqual(len(runtime_tables), runtime["current_table_count"])
        self.assertEqual(len(expected_tables), runtime["current_table_count"])
        self.assertEqual(runtime_tables, sorted(expected_tables))
        self.assertTrue(TABLES <= expected_tables)
        self.assertEqual(runtime["current_table_count"], 203)
        head = migration["migrations"][-1]
        self.assertEqual(runtime["migration_head"]["id"], head["id"])
        self.assertEqual(runtime["migration_head"]["sequence"], head["sequence"])
        self.assertEqual(runtime["migration_head"]["apply_checksum"], head["apply_checksum"])
        self.assertEqual(runtime["migration_head"]["verify_checksum"], head["verify_checksum"])

        lifecycle_manifest = json.loads(LIFECYCLE.read_text())
        entries = {entry["locator"]: entry for entry in lifecycle_manifest["entries"]
                   if entry["kind"] == "database_table"}
        self.assertEqual(set(entries), expected_tables)
        for table in TABLES:
            entry = entries[table]
            self.assertEqual(entry["data_category"], "observational_telemetry")
            self.assertEqual(entry["season_action"], "retain")
            self.assertEqual(entry["terminal_action"], "retain")
            self.assertEqual(entry["controller_decision"]["status"], "pending")
            self.assertEqual(entry["export_rule"]["disposition"], "pending")
            self.assertEqual(entry["dependencies"], [])

    def test_sealed_telemetry_migration_checksum_still_matches_manifest(self) -> None:
        migration = json.loads(MIGRATIONS.read_text())
        prior = {item["id"]: item for item in migration["migrations"]}
        for name in ("0014_telemetry_storage", "0016_artifact_mana"):
            item = prior[name]
            apply_path = ROOT / "migrations" / item["apply"]
            verify_path = ROOT / "migrations" / item["verify"]
            self.assertEqual(item["apply_checksum"], hashlib.sha256(apply_path.read_bytes()).hexdigest())
            self.assertEqual(item["verify_checksum"], hashlib.sha256(verify_path.read_bytes()).hexdigest())


if __name__ == "__main__":
    unittest.main()
