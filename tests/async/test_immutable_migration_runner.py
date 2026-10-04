#!/usr/bin/env python3
"""Focused immutable migration manifest and success-last runner regressions."""

from __future__ import annotations

import gzip
import json
import os
import subprocess
import sys
import tempfile
import unittest
from dataclasses import replace
from pathlib import Path
from unittest import mock


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))
import migration_runner as runner  # noqa: E402
import qualify_database_restore as restore_qualifier  # noqa: E402


class FakeExecutor:
    def __init__(self, applied=None, fail_apply=False, fail_verify=False, fail_record=False):
        self.rows = list(applied or [])
        self.fail_apply = fail_apply
        self.fail_verify = fail_verify
        self.fail_record = fail_record
        self.events = []

    def acquire_lock(self): self.events.append("lock")
    def release_lock(self): self.events.append("unlock")
    def require_baseline(self, manifest): self.events.append("baseline")
    def applied(self): return list(self.rows)
    def apply(self, migration):
        """Trace an apply, or raise when the test asked this stage to fail."""
        self.events.append(f"apply:{migration.migration_id}")
        if self.fail_apply: raise runner.MigrationContractError("synthetic apply failure")
    def verify(self, migration):
        """Trace a verify, or raise when the test asked this stage to fail."""
        self.events.append(f"verify:{migration.migration_id}")
        if self.fail_verify: raise runner.MigrationContractError("synthetic verify failure")
    def record(self, migration, version):
        """Record one applied migration, tracing the call for order assertions."""
        self.events.append(f"record:{migration.migration_id}")
        if self.fail_record: raise runner.MigrationContractError("synthetic record failure")
        self.rows.append(runner.AppliedMigration(
            migration.migration_id, migration.sequence, migration.description,
            migration.apply_checksum, migration.verify_checksum,
            migration.compatibility, version,
        ))


class ImmutableMigrationRunnerTest(unittest.TestCase):
    def test_restore_accepts_all_complete_supported_histories(self):
        for path in (runner.DEFAULT_MANIFEST,
                     ROOT / "migrations/migration_manifest.staging_0045.json",
                     ROOT / "migrations/migration_manifest.master_0031.json"):
            manifest = runner.load_manifest(path)
            rows = [runner.AppliedMigration(
                step.migration_id, step.sequence, step.description,
                step.apply_checksum, step.verify_checksum, step.compatibility,
                manifest.runner_version) for step in manifest.migrations]
            with self.subTest(manifest=path.name):
                restore_qualifier.require_completed_history(rows)

    def test_restore_rejects_partial_mixed_and_edited_histories(self):
        manifest = runner.load_manifest()
        stage = runner.load_manifest(
            ROOT / "migrations/migration_manifest.staging_0045.json")
        master = runner.load_manifest(
            ROOT / "migrations/migration_manifest.master_0031.json")
        def receipts(selected):
            return [runner.AppliedMigration(
                step.migration_id, step.sequence, step.description,
                step.apply_checksum, step.verify_checksum, step.compatibility,
                selected.runner_version) for step in selected.migrations]
        canonical, staging = receipts(manifest), receipts(stage)
        upgraded_master = receipts(master)
        mixed = list(canonical)
        mixed[44] = staging[44]
        mixed_master = list(canonical)
        mixed_master[30] = upgraded_master[30]
        for label, rows in (
            ("empty", []), ("common_prefix", canonical[:44]),
            ("unmigrated_stage", staging[:45]), ("missing_head", canonical[:-1]),
            ("mixed", mixed),
            ("unmigrated_master", upgraded_master[:31]),
            ("mixed_master", mixed_master),
            ("master_missing_head", upgraded_master[:-1]),
            ("master_edited_receipt", upgraded_master[:30] +
             [replace(upgraded_master[30], description="edited")] + upgraded_master[31:]),
            ("edited_old_receipt", [replace(canonical[0], description="edited")] + canonical[1:]),
            ("extra_receipt", staging + [staging[-1]]),
        ):
            with self.subTest(history=label), self.assertRaisesRegex(
                    RuntimeError, "incomplete_or_unknown"):
                restore_qualifier.require_completed_history(rows)

    def test_restore_closes_session_on_success_and_history_refusal(self):
        manifest = runner.load_manifest()
        rows = [runner.AppliedMigration(
            step.migration_id, step.sequence, step.description,
            step.apply_checksum, step.verify_checksum, step.compatibility,
            manifest.runner_version) for step in manifest.migrations]
        for complete in (True, False):
            executor = FakeExecutor(rows if complete else rows[:-1])
            executor.sql = mock.Mock(return_value="0")
            with self.subTest(complete=complete), mock.patch.dict(os.environ, {
                    "DB_NAME": "duris_restore", "DB_SOCKET": "/tmp/disposable.sock"}), \
                    mock.patch.object(runner, "MysqlExecutor", return_value=executor):
                if complete:
                    restore_qualifier.main()
                    self.assertGreater(executor.sql.call_count, 0)
                else:
                    with self.assertRaisesRegex(RuntimeError, "incomplete_or_unknown"):
                        restore_qualifier.main()
                    executor.sql.assert_not_called()
            self.assertEqual(executor.events, ["baseline", "unlock"])

    def make_production_backup(self, directory: Path, database: str = "duris") -> Path:
        path = directory / "production.sql.gz"
        payload = (
            f"CREATE DATABASE `{database}`;\nUSE `{database}`;\n"
            "CREATE TABLE `accounts` (id INT);\n"
            "CREATE TABLE `player_data` (id INT);\n"
            "CREATE TABLE `ships` (id INT);\n"
        ).encode()
        with gzip.open(path, "wb") as backup:
            backup.write(payload)
        path.chmod(0o600)
        return path

    def make_manifest(self, directory: Path, count: int = 2) -> Path:
        """Build a synthetic migration manifest with real files and checksums.

        Lets the rejection tests mutate one field at a time against a manifest that
        would otherwise load cleanly.
        """
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

    def test_canonical_verifiers_are_executable(self):
        """The real runner executes verifier paths directly, not through bash."""
        for migration in runner.load_manifest().migrations:
            with self.subTest(migration=migration.migration_id):
                self.assertTrue(os.access(migration.verify_path, os.X_OK),
                                f"verifier is not executable: {migration.verify_path}")

    def test_canonical_manifest_keeps_baseline_and_orders_immutable_steps(self):
        """The shipped manifest still describes the sealed baseline and head.

        The 170-table Session 11 baseline and its fingerprint must not move, and the
        immutable migrations must stay in recorded order. Tables created by immutable
        migrations are excluded before comparing against that baseline inventory.
        """
        manifest = runner.load_manifest()
        self.assertEqual(manifest.required_table_count, 170)
        self.assertEqual(len(manifest.required_tables), 170)
        self.assertEqual(len(manifest.migrations), 65)
        self.assertEqual(manifest.migrations[-1].migration_id,
                         "0065_telemetry_battle_builds")
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
        self.assertEqual(manifest.migrations[6].migration_id,
                         "0007_pkill_event_stamp_contract")
        self.assertEqual(manifest.migrations[7].migration_id,
                         "0008_statistics_date_index")
        self.assertEqual(manifest.migrations[8].migration_id,
                         "0009_kingdom_garrison")
        self.assertEqual(len(manifest.required_table_fingerprint), 64)
        lifecycle = json.loads(
            (ROOT / "migrations/data_lifecycle_manifest.json").read_text()
        )
        tables = [entry["locator"] for entry in lifecycle["entries"]
                  if entry["kind"] == "database_table"]
        import validate_data_lifecycle as inventory
        post_baseline = inventory.schema_tables(tuple(
            item.apply_path for item in manifest.migrations)) - set(manifest.required_tables)
        self.assertEqual(set(tables), set(manifest.required_tables) | post_baseline)
        baseline_tables = [table for table in tables if table not in post_baseline]
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

    def test_stage_failures_identify_migration_and_never_record_success(self):
        with tempfile.TemporaryDirectory() as temporary:
            manifest = runner.load_manifest(self.make_manifest(Path(temporary)))
            for flag, stage in (("fail_apply", "apply"), ("fail_verify", "verify"),
                                ("fail_record", "history record")):
                with self.subTest(stage=stage):
                    executor = FakeExecutor(**{flag: True})
                    with self.assertRaisesRegex(runner.MigrationContractError,
                                                f"migration 0001_synthetic_step {stage} failed"):
                        runner.run_pending(manifest, executor)
                    self.assertEqual(executor.rows, [])
                    self.assertEqual(executor.events[-1], "unlock")

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

    def test_staging_sequence_45_fork_refuses_before_schema_changes(self):
        manifest = runner.load_manifest()
        self.assertEqual(manifest.migrations[44].migration_id, "0045_quest_reward_obligation")
        rows = [runner.AppliedMigration(
            item.migration_id, item.sequence, item.description,
            item.apply_checksum, item.verify_checksum, item.compatibility,
            manifest.runner_version,
        ) for item in manifest.migrations[:44]]
        rows.append(runner.AppliedMigration(
            "0045_item_extra_description_fulltext_unique", 45,
            "verified staging fork fixture",
            runner.checksum(b"synthetic full-text uniqueness apply"),
            runner.checksum(b"synthetic full-text uniqueness verify"),
            manifest.migrations[44].compatibility, manifest.runner_version,
        ))
        # A consistent history count and digest do not make this a candidate prefix.
        runner.validate_history_state(rows, 45, runner.history_checksum(rows))
        executor = FakeExecutor(rows)
        with self.assertRaisesRegex(runner.MigrationContractError, "edited or reordered"):
            runner.run_pending(manifest, executor)
        self.assertEqual(executor.events, ["lock", "baseline", "unlock"])
        self.assertEqual(executor.rows, rows)

    def test_explicit_staging_manifest_appends_without_rewriting_history(self):
        staging = runner.load_manifest(ROOT / "migrations/migration_manifest.staging_0045.json")
        rows = [runner.AppliedMigration(
            item.migration_id, item.sequence, item.description, item.apply_checksum,
            item.verify_checksum, item.compatibility, staging.runner_version,
        ) for item in staging.migrations[:45]]
        executor = FakeExecutor(rows)
        self.assertEqual(runner.run_pending(staging, executor),
                         [item.migration_id for item in staging.migrations[45:]])
        self.assertEqual(executor.rows[:45], rows)
        self.assertEqual(len(executor.rows), 65)
        replay = FakeExecutor(executor.rows)
        self.assertEqual(runner.run_pending(staging, replay), [])
        self.assertEqual(replay.events, ["lock", "baseline", "unlock"])
        with self.assertRaisesRegex(runner.MigrationContractError, "edited or reordered"):
            runner.run_pending(runner.load_manifest(), FakeExecutor(rows))

    def test_master_upgrade_preserves_sealed_prefix_resumes_and_replays(self):
        master = runner.load_manifest(
            ROOT / "migrations/migration_manifest.master_0031.json")
        prefix = [runner.AppliedMigration(
            item.migration_id, item.sequence, item.description, item.apply_checksum,
            item.verify_checksum, item.compatibility, master.runner_version,
        ) for item in master.migrations[:31]]
        # Measured from master's actual manifest at 1862a5fb, including all metadata.
        self.assertEqual(runner.history_checksum(prefix),
                         "30be02f71b0cb9d0e70812c5762b112b1636f9ceb07f95e08d94743a5e971120")
        self.assertEqual(prefix[-1].migration_id, "0031_player_item_runtime_state")
        refused = FakeExecutor(prefix)
        with self.assertRaisesRegex(runner.MigrationContractError, "edited or reordered"):
            runner.run_pending(runner.load_manifest(), refused)
        self.assertEqual(refused.events, ["lock", "baseline", "unlock"])
        self.assertEqual(refused.rows, prefix)

        interrupted = FakeExecutor(prefix, fail_verify=True)
        with self.assertRaisesRegex(runner.MigrationContractError, "verify failed"):
            runner.run_pending(master, interrupted)
        self.assertEqual(interrupted.rows, prefix)
        self.assertEqual(interrupted.events[-1], "unlock")
        resumed = FakeExecutor(interrupted.rows)
        self.assertEqual(runner.run_pending(master, resumed),
                         [item.migration_id for item in master.migrations[31:]])
        self.assertEqual(resumed.rows[:31], prefix)
        self.assertEqual(len(resumed.rows), 65)
        self.assertEqual(resumed.rows[-1].migration_id,
                         "0065_telemetry_battle_builds")
        replay = FakeExecutor(resumed.rows)
        self.assertEqual(runner.run_pending(master, replay), [])
        self.assertEqual(replay.events, ["lock", "baseline", "unlock"])
        restore_qualifier.require_completed_history(replay.rows)

    def test_runtime_history_query_is_bounded_and_covers_every_receipt_field(self):
        sql = runner.runtime_history_sql(51)
        for field in ("migration_id", "sequence_number", "description", "apply_checksum",
                      "verify_checksum", "compatibility", "runner_version"):
            self.assertIn(field, sql)
        self.assertEqual(sql.count("UNHEX(LPAD(HEX(OCTET_LENGTH("), 7)
        self.assertTrue(sql.endswith("ORDER BY sequence_number LIMIT 51"))
        for invalid in (0, -1, True, "51", 10001):
            with self.assertRaises(runner.MigrationContractError):
                runner.runtime_history_sql(invalid)

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
        production = {"ENVIRONMENT": "production", "DB_HOST": "127.0.0.1",
                      "DB_NAME": "duris", "DB_USER": "x", "DB_PASSWD": "x"}
        with mock.patch.dict(os.environ, production, clear=True):
            with self.assertRaisesRegex(runner.MigrationContractError, "not confirmed"):
                runner.MysqlExecutor(manifest)

        cases = (
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

    def test_production_run_requires_exact_target_backup_allowlist_and_quiescence(self):
        manifest = runner.load_manifest()
        with tempfile.TemporaryDirectory() as temporary:
            backup = self.make_production_backup(Path(temporary))
            environment = {
                "ENVIRONMENT": "production", "DB_HOST": "127.0.0.1",
                "DB_NAME": "duris", "DB_USER": "x", "DB_PASSWD": "x",
                "DB_ALLOWED_TARGETS": "127.0.0.1/duris",
            }
            with mock.patch.dict(os.environ, environment, clear=True):
                executor = runner.MysqlExecutor(
                    manifest, "127.0.0.1/duris", backup)
                self.assertTrue(executor.production)
                with mock.patch.object(executor, "sql", return_value="0"):
                    executor.require_quiescent()
                with mock.patch.object(executor, "sql", return_value="1"), \
                        self.assertRaisesRegex(runner.MigrationContractError,
                                              "every other database connection"):
                    executor.require_quiescent()

                with self.assertRaisesRegex(runner.MigrationContractError, "not confirmed"):
                    runner.MysqlExecutor(manifest, "127.0.0.1/other", backup)
                with self.assertRaisesRegex(runner.MigrationContractError,
                                            "validated backup"):
                    runner.MysqlExecutor(manifest, "127.0.0.1/duris")

                environment["DB_ALLOWED_TARGETS"] = "127.0.0.1/other"
                with mock.patch.dict(os.environ, environment, clear=True), \
                        self.assertRaisesRegex(runner.MigrationContractError,
                                              "not allow-listed"):
                    runner.MysqlExecutor(manifest, "127.0.0.1/duris", backup)

    def test_production_backup_rejects_unsafe_mode_and_wrong_database(self):
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            backup = self.make_production_backup(directory)
            backup.chmod(0o644)
            with self.assertRaisesRegex(runner.MigrationContractError, "owner-only"):
                runner.validate_production_backup(backup, "duris")
            backup.chmod(0o600)
            with self.assertRaisesRegex(runner.MigrationContractError,
                                        "does not identify"):
                runner.validate_production_backup(backup, "other")

    def test_socket_adapter_keeps_no_defaults_first_and_overrides_routing(self):
        with tempfile.TemporaryDirectory() as temporary:
            client = Path(temporary) / "mysql-arguments"
            client.write_text('#!/bin/sh\nprintf "%s\\n" "$@"\n')
            client.chmod(0o700)
            environment = dict(os.environ, DURIS_REAL_MYSQL_CLIENT=str(client),
                               DB_SOCKET="/tmp/synthetic-restore.sock")
            for defaults in ([], ["--no-defaults"]):
                with self.subTest(defaults=defaults):
                    result = subprocess.run([
                        "bash", str(ROOT / "scripts/mysql_socket_bin/mysql"),
                        *defaults, "-h", "ignored-host", "-P3307", "--protocol=tcp",
                        "--socket=/tmp/ignored.sock", "--user=restore", "-N", "-B",
                        "duris_restore", "-e", "SELECT 1",
                    ], env=environment, capture_output=True, text=True, check=True)
                    self.assertEqual(result.stdout.splitlines(), [
                        *defaults, "--protocol=socket", "--socket=/tmp/synthetic-restore.sock",
                        "--user=restore", "-N", "-B", "duris_restore", "-e", "SELECT 1",
                    ])

    def test_local_unix_socket_is_explicit_and_reaches_sealed_verifiers(self):
        manifest = runner.load_manifest()
        environment = {
            "ENVIRONMENT": "local", "DB_HOST": "127.0.0.1",
            "DB_NAME": "duris_dev", "DB_USER": "duris", "DB_PASSWD": "secret",
            "DB_SOCKET": "/run/mysqld/mysqld.sock",
        }
        with mock.patch.dict(os.environ, environment, clear=True):
            executor = runner.MysqlExecutor(manifest)
            self.assertIn("--skip-reconnect", executor.command)
            self.assertIn("--unbuffered", executor.command)
            self.assertEqual(executor.command[1], "--no-defaults")
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

    def test_receipt_record_rolls_back_a_failed_state_compare(self):
        manifest = runner.load_manifest()
        environment = {"ENVIRONMENT": "test", "DB_HOST": "127.0.0.1",
                       "DB_NAME": "migration_test", "DB_USER": "test", "DB_PASSWD": "test"}
        with mock.patch.dict(os.environ, environment, clear=True):
            executor = runner.MysqlExecutor(manifest)
        with mock.patch.object(executor, "applied", return_value=[]), \
                mock.patch.object(executor, "sql", side_effect=["0", ""]) as sql:
            with self.assertRaisesRegex(runner.MigrationContractError, "head changed"):
                executor.record(manifest.migrations[0], manifest.runner_version)
            self.assertIn("START TRANSACTION", sql.call_args_list[0].args[0])
            self.assertNotIn("COMMIT", sql.call_args_list[0].args[0])
            self.assertEqual(sql.call_args_list[1].args[0], "ROLLBACK;")
        with mock.patch.object(executor, "applied", return_value=[]), \
                mock.patch.object(executor, "sql", side_effect=["1", ""]) as sql:
            executor.record(manifest.migrations[0], manifest.runner_version)
            self.assertEqual(sql.call_args_list[1].args[0], "COMMIT;")

    def test_closed_transport_cannot_reconnect_or_run_a_statement(self):
        manifest = runner.load_manifest()
        environment = {"ENVIRONMENT": "test", "DB_HOST": "127.0.0.1",
                       "DB_NAME": "migration_test", "DB_USER": "test", "DB_PASSWD": "test"}
        with mock.patch.dict(os.environ, environment, clear=True):
            executor = runner.MysqlExecutor(manifest)
        executor._close_session()
        with mock.patch.object(runner.subprocess, "Popen") as spawn:
            with self.assertRaisesRegex(runner.MigrationContractError, "session is closed"):
                executor.sql("SELECT 1;")
            spawn.assert_not_called()
        executor.release_lock()

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
        self.assertIn("TOTAL=150", legacy)

    def test_legacy_upgrade_verifies_schema_before_imported_character_baselines(self):
        adoption = (ROOT / "migrations/adopt_migration_baseline.sh").read_text()
        verifier = (ROOT / "migrations/verify_runtime_compatibility.sh").read_text()
        importer = (ROOT / "scripts/import_legacy_dump.py").read_text()
        self.assertIn('verify_runtime_compatibility.sh" --schema-only', adoption)
        self.assertIn('"$SCHEMA_ONLY" == 0', verifier)
        self.assertIn("establish_character_baselines(config)", importer)
        self.assertIn('verifier = ROOT / "migrations/verify_runtime_compatibility.sh"', importer)


if __name__ == "__main__":
    unittest.main()
