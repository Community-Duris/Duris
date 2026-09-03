#!/usr/bin/env python3
"""Focused immutable migration manifest and success-last runner regressions."""

from __future__ import annotations

import json
import os
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))
import migration_runner as runner  # noqa: E402


class FakeExecutor:
    def __init__(self, applied=None, fail_apply=False, fail_verify=False):
        self.rows = list(applied or [])
        self.fail_apply = fail_apply
        self.fail_verify = fail_verify
        self.events = []

    def acquire_lock(self): self.events.append("lock")
    def release_lock(self): self.events.append("unlock")
    def require_baseline(self, manifest): self.events.append("baseline")
    def applied(self): return list(self.rows)
    def apply(self, migration):
        self.events.append(f"apply:{migration.migration_id}")
        if self.fail_apply: raise runner.MigrationContractError("synthetic apply failure")
    def verify(self, migration):
        self.events.append(f"verify:{migration.migration_id}")
        if self.fail_verify: raise runner.MigrationContractError("synthetic verify failure")
    def record(self, migration, version):
        self.events.append(f"record:{migration.migration_id}")
        self.rows.append(runner.AppliedMigration(
            migration.migration_id, migration.sequence, migration.description,
            migration.apply_checksum, migration.verify_checksum,
            migration.compatibility, version,
        ))


class ImmutableMigrationRunnerTest(unittest.TestCase):
    def make_manifest(self, directory: Path, count: int = 2) -> Path:
        immutable = directory / "immutable"
        immutable.mkdir(parents=True)
        items = []
        for sequence in range(1, count + 1):
            migration_id = f"{sequence:04d}_synthetic_step"
            apply_name = f"immutable/{migration_id}.sql"
            verify_name = f"immutable/{migration_id}.sh"
            apply_path = directory / apply_name
            verify_path = directory / verify_name
            apply_path.write_text(f"SELECT {sequence};\n")
            verify_path.write_text("#!/usr/bin/env bash\nexit 0\n")
            items.append({
                "id": migration_id, "sequence": sequence,
                "description": f"synthetic step {sequence}", "apply": apply_name,
                "apply_checksum": runner.checksum(apply_path.read_bytes()),
                "verify": verify_name,
                "verify_checksum": runner.checksum(verify_path.read_bytes()),
                "compatibility": "mysql8-mariadb10",
            })
        manifest = {
            "manifest_version": 1, "runner_version": 1,
            "baseline": {"id": "test-baseline-0001", "required_table_count": 2,
                         "required_table_fingerprint":
                             runner.table_fingerprint(["alpha", "beta"]),
                         "required_tables": ["alpha", "beta"]},
            "migrations": items,
        }
        path = directory / "migration_manifest.json"
        path.write_text(json.dumps(manifest))
        return path

    def test_canonical_manifest_keeps_baseline_and_orders_immutable_steps(self):
        """The shipped manifest still describes the sealed baseline and head.

        The 170-table Session 11 baseline and its fingerprint must not move, and the
        immutable migrations must stay in recorded order. Tables created by immutable
        migrations are excluded before comparing against that baseline inventory.
        """
        manifest = runner.load_manifest()
        self.assertEqual(manifest.required_table_count, 170)
        self.assertEqual(len(manifest.required_tables), 170)
        self.assertEqual(len(manifest.migrations), 6)
        self.assertEqual(manifest.migrations[0].migration_id,
                         "0001_lookup_dataset_state")
        self.assertEqual(manifest.migrations[1].migration_id,
                         "0002_player_item_metadata_uniqueness")
        self.assertEqual(manifest.migrations[2].migration_id,
                         "0003_season_reset_state")
        self.assertEqual(manifest.migrations[3].migration_id,
                         "0004_server_reboots")
        self.assertEqual(manifest.migrations[4].migration_id,
                         "0005_level_cap_singleton")
        self.assertEqual(manifest.migrations[5].migration_id,
                         "0006_kingdom_realms")
        self.assertEqual(len(manifest.required_table_fingerprint), 64)
        lifecycle = json.loads(
            (ROOT / "migrations/data_lifecycle_manifest.json").read_text()
        )
        tables = [entry["locator"] for entry in lifecycle["entries"]
                  if entry["kind"] == "database_table"]
        baseline_tables = [table for table in tables if table not in {
            "lookup_dataset_state", "season_reset_state", "server_reboots",
            "kingdom_realms"
        }]
        self.assertEqual(len(baseline_tables), manifest.required_table_count)
        self.assertEqual(runner.table_fingerprint(baseline_tables),
                         manifest.required_table_fingerprint)
        self.assertEqual(set(baseline_tables), set(manifest.required_tables))

    def test_manifest_rejects_duplicate_reorder_checksum_and_symlink(self):
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            path = self.make_manifest(directory)
            value = json.loads(path.read_text())

            duplicate = dict(value)
            duplicate["migrations"] = value["migrations"] + [value["migrations"][0]]
            path.write_text(json.dumps(duplicate))
            with self.assertRaisesRegex(runner.MigrationContractError,
                                        "duplicate|sequence"):
                runner.load_manifest(path)

            path.write_text(json.dumps(value))
            value["migrations"][1]["sequence"] = 3
            path.write_text(json.dumps(value))
            with self.assertRaisesRegex(runner.MigrationContractError, "reordered"):
                runner.load_manifest(path)

            path = self.make_manifest(directory := Path(temporary) / "hash")
            value = json.loads(path.read_text())
            value["migrations"][0]["apply_checksum"] = "0" * 64
            path.write_text(json.dumps(value))
            with self.assertRaisesRegex(runner.MigrationContractError, "checksum"):
                runner.load_manifest(path)

            path = self.make_manifest(directory := Path(temporary) / "link")
            target = directory / "immutable/0001_synthetic_step.sql"
            real = directory / "real.sql"
            target.rename(real)
            target.symlink_to(real)
            with self.assertRaisesRegex(runner.MigrationContractError,
                                        "cannot read|escapes"):
                runner.load_manifest(path)

    def test_apply_verify_record_order_failure_resume_and_noop(self):
        with tempfile.TemporaryDirectory() as temporary:
            manifest = runner.load_manifest(self.make_manifest(Path(temporary)))
            failing = FakeExecutor(fail_verify=True)
            with self.assertRaisesRegex(runner.MigrationContractError, "verify"):
                runner.run_pending(manifest, failing)
            self.assertNotIn("record:0001_synthetic_step", failing.events)
            self.assertEqual(failing.events[-1], "unlock")

            resumed = FakeExecutor()
            self.assertEqual(runner.run_pending(manifest, resumed),
                             ["0001_synthetic_step", "0002_synthetic_step"])
            self.assertLess(resumed.events.index("apply:0001_synthetic_step"),
                            resumed.events.index("verify:0001_synthetic_step"))
            self.assertLess(resumed.events.index("verify:0001_synthetic_step"),
                            resumed.events.index("record:0001_synthetic_step"))
            replay = FakeExecutor(resumed.rows)
            self.assertEqual(runner.run_pending(manifest, replay), [])
            self.assertFalse(any(event.startswith("apply:") for event in replay.events))

    def test_applied_history_edit_and_reorder_fail_before_apply(self):
        with tempfile.TemporaryDirectory() as temporary:
            manifest = runner.load_manifest(self.make_manifest(Path(temporary)))
            first = manifest.migrations[0]
            edited = runner.AppliedMigration(first.migration_id, first.sequence,
                                             "edited description", first.apply_checksum,
                                             first.verify_checksum, first.compatibility, 1)
            executor = FakeExecutor([edited])
            with self.assertRaisesRegex(runner.MigrationContractError, "edited"):
                runner.run_pending(manifest, executor)
            self.assertFalse(any(event.startswith("apply:") for event in executor.events))

    def test_history_head_detects_trailing_deletion(self):
        row = runner.AppliedMigration("0001_test", 1, "test", "1" * 64,
                                      "2" * 64, "mysql8", 1)
        head = runner.history_checksum([row])
        runner.validate_history_state([row], 1, head)
        with self.assertRaisesRegex(runner.MigrationContractError, "head/count"):
            runner.validate_history_state([], 1, head)

    def test_table_fingerprint_is_exact_and_order_independent(self):
        self.assertEqual(runner.table_fingerprint(["beta", "alpha"]),
                         runner.table_fingerprint(["alpha", "beta"]))
        with self.assertRaisesRegex(runner.MigrationContractError, "inventory"):
            runner.table_fingerprint(["alpha", "alpha"])
        with self.assertRaisesRegex(runner.MigrationContractError, "inventory"):
            runner.table_fingerprint(["bad-name"])

    def test_target_safety_rejects_production_before_mysql(self):
        manifest = runner.load_manifest()
        cases = (
            {"ENVIRONMENT": "production", "DB_HOST": "127.0.0.1",
             "DB_NAME": "duris", "DB_USER": "x", "DB_PASSWD": "x"},
            {"ENVIRONMENT": "test", "DB_HOST": "database.internal",
             "DB_NAME": "duris", "DB_USER": "x", "DB_PASSWD": "x"},
            {"ENVIRONMENT": "test", "DB_HOST": "127.0.0.1",
             "DB_NAME": "duris_prod", "DB_USER": "x", "DB_PASSWD": "x"},
        )
        for environment in cases:
            with mock.patch.dict(os.environ, environment, clear=True):
                with self.assertRaisesRegex(runner.MigrationContractError,
                                            "non-production"):
                    runner.MysqlExecutor(manifest)

    def test_local_unix_socket_is_explicit_and_reaches_sealed_verifiers(self):
        manifest = runner.load_manifest()
        environment = {
            "ENVIRONMENT": "local", "DB_HOST": "127.0.0.1",
            "DB_NAME": "duris_dev", "DB_USER": "duris", "DB_PASSWD": "secret",
            "DB_SOCKET": "/run/mysqld/mysqld.sock",
        }
        with mock.patch.dict(os.environ, environment, clear=True):
            executor = runner.MysqlExecutor(manifest)
            self.assertIn("--protocol=socket", executor.command)
            self.assertIn("--socket=/run/mysqld/mysqld.sock", executor.command)
            with mock.patch.object(runner.shutil, "which", return_value="/usr/bin/mysql"), \
                    mock.patch.object(runner.subprocess, "run") as process:
                process.return_value.returncode = 0
                executor.verify(manifest.migrations[0])
                verify_environment = process.call_args.kwargs["env"]
                self.assertEqual(verify_environment["DURIS_REAL_MYSQL_CLIENT"],
                                 "/usr/bin/mysql")
                self.assertTrue(verify_environment["PATH"].startswith(
                    str(ROOT / "scripts/mysql_socket_bin") + os.pathsep))

        environment["DB_SOCKET"] = "relative/socket"
        with mock.patch.dict(os.environ, environment, clear=True):
            with self.assertRaisesRegex(runner.MigrationContractError, "absolute"):
                runner.MysqlExecutor(manifest)

    def test_legacy_data_markers_are_not_recast_as_complete_history(self):
        ledger = (ROOT / "migrations/immutable_migration_ledger.sql").read_text()
        legacy = (ROOT / "migrations/run_migration.sh").read_text()
        self.assertIn("mud_schema_baselines", ledger)
        self.assertIn("mud_schema_history", ledger)
        self.assertIn("mud_schema_migration_state", ledger)
        self.assertNotIn("DROP TABLE mud_schema_migrations", ledger)
        adoption = (ROOT / "migrations/adopt_migration_baseline.sh").read_text()
        self.assertIn("verified_legacy_adoption", adoption)
        self.assertIn("migration_runner.py\" run", adoption)
        self.assertIn("verify_runtime_compatibility.sh", adoption)
        self.assertIn("TOTAL=143", legacy)


if __name__ == "__main__":
    unittest.main()
