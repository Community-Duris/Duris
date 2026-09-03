#!/usr/bin/env python3
"""Focused safety and portability regressions for legacy dump replacement."""

from __future__ import annotations

import hashlib
import io
import importlib.util
import os
import tempfile
import unittest
from pathlib import Path
from unittest import mock


ROOT = Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location(
    "import_legacy_dump", ROOT / "scripts/import_legacy_dump.py")
assert SPEC is not None and SPEC.loader is not None
legacy = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(legacy)


class LegacyDumpImportTest(unittest.TestCase):
    def write_private(self, path: Path, payload: str) -> None:
        path.write_text(payload, encoding="utf-8")
        os.chmod(path, 0o600)

    def valid_config(self) -> dict[str, str]:
        return {
            "ENVIRONMENT": "test",
            "DB_HOST": "127.0.0.1",
            "DB_PORT": "3306",
            "DB_USER": "duris",
            "DB_PASSWD": "secret",
            "DB_NAME": "duris_import_test",
            "DB_ALLOWED_TARGETS": "127.0.0.1/duris_import_test",
        }

    def test_dump_validation_is_private_mysql_and_target_forced(self):
        with tempfile.TemporaryDirectory() as temporary:
            dump = Path(temporary) / "legacy.sql"
            payload = "-- MySQL dump 10.13\nCREATE TABLE `extra` (`id` int);\n"
            self.write_private(dump, payload)
            self.assertEqual(legacy.validate_dump(dump),
                             hashlib.sha256(payload.encode()).hexdigest())

            self.write_private(dump, payload + "USE other_database;\n")
            with self.assertRaisesRegex(legacy.LegacyImportError,
                                        "database-selection directive"):
                legacy.validate_dump(dump)

            self.write_private(dump, payload)
            os.chmod(dump, 0o644)
            with self.assertRaisesRegex(legacy.LegacyImportError, "mode 0600"):
                legacy.validate_dump(dump)

    def test_dump_validation_rejects_executable_and_schema_directives(self):
        with tempfile.TemporaryDirectory() as temporary:
            dump = Path(temporary) / "legacy.sql"
            for directive in (
                    "/*!40000 DROP DATABASE IF EXISTS `other_database`*/;",
                    "CREATE SCHEMA other_database;"):
                self.write_private(
                    dump, f"-- MySQL dump 10.13\n{directive}\n")
                with self.assertRaisesRegex(
                        legacy.LegacyImportError, "database-selection directive"):
                    legacy.validate_dump(dump)

    def test_unquoted_env_value_preserves_hash_with_leading_space(self):
        with tempfile.TemporaryDirectory() as temporary:
            env_file = Path(temporary) / "import.env"
            config = self.valid_config()
            config["DB_PASSWD"] = "hunter2 #7"
            self.write_private(
                env_file,
                "\n".join(f"{key}={value}" for key, value in config.items()) + "\n")
            self.assertEqual(legacy.read_env_file(env_file)["DB_PASSWD"], "hunter2 #7")

    def test_mysql8_metadata_is_normalized_without_touching_row_data(self):
        ddl = (b"CREATE TABLE t (v varchar(20)) COLLATE=utf8mb4_0900_ai_ci "
               b"/*!50013 DEFINER=`old`@`127.0.0.1` SQL SECURITY DEFINER */;\n")
        normalized = legacy.normalize_dump_line(ddl)
        self.assertIn(b"utf8mb4_unicode_ci", normalized)
        self.assertIn(b"DEFINER=CURRENT_USER", normalized)
        row = b"INSERT INTO `t` VALUES ('utf8mb4_0900_ai_ci');\n"
        self.assertEqual(legacy.normalize_dump_line(row), row)

    def test_target_requires_loopback_nonproduction_exact_allowlist(self):
        config = self.valid_config()
        legacy.validate_target(config)
        for field, value in (("ENVIRONMENT", "production"),
                             ("DB_HOST", "database.internal"),
                             ("DB_NAME", "duris_prod"),
                             ("DB_ALLOWED_TARGETS", "127.0.0.1/another_db")):
            invalid = dict(config)
            invalid[field] = value
            with self.assertRaises(legacy.LegacyImportError):
                legacy.validate_target(invalid)

    def test_wipe_removes_events_routines_views_and_tables(self):
        executed: list[str] = []

        def mysql_result(_config, statement):
            if "information_schema.events" in statement:
                return "stale_event"
            if "information_schema.routines" in statement:
                return "FUNCTION\tstale_function\nPROCEDURE\tstale_procedure"
            if "table_type='VIEW'" in statement:
                return "stale_view"
            if "table_type='BASE TABLE'" in statement:
                return "stale_table"
            executed.append(statement)
            return ""

        with mock.patch.object(legacy, "run_mysql", side_effect=mysql_result):
            legacy.wipe_target(self.valid_config())
        self.assertEqual(executed, [
            "DROP EVENT IF EXISTS `stale_event`;"
            "DROP FUNCTION IF EXISTS `stale_function`;"
            "DROP PROCEDURE IF EXISTS `stale_procedure`;"
            "SET FOREIGN_KEY_CHECKS=0;"
            "DROP VIEW IF EXISTS `stale_view`;"
            "DROP TABLE IF EXISTS `stale_table`;"
            "SET FOREIGN_KEY_CHECKS=1;",
        ])

    def test_import_diagnostics_use_file_backing_instead_of_a_pipe(self):
        with tempfile.TemporaryDirectory() as temporary:
            dump = Path(temporary) / "legacy.sql"
            dump.write_bytes(b"SELECT 1;\n")
            diagnostics = []

            def fake_popen(*_args, **kwargs):
                diagnostics.append(kwargs["stderr"])
                kwargs["stderr"].write(b"synthetic mysql failure\n")
                kwargs["stderr"].flush()
                process = mock.Mock()
                process.stdin = io.BytesIO()
                process.wait.return_value = 1
                return process

            with mock.patch.object(legacy.subprocess, "Popen", side_effect=fake_popen):
                with self.assertRaisesRegex(
                        legacy.LegacyImportError, "synthetic mysql failure"):
                    legacy.import_stream(self.valid_config(), dump, normalize=False)
            self.assertEqual(len(diagnostics), 1)
            self.assertNotEqual(diagnostics[0], legacy.subprocess.PIPE)

    def test_import_interrupt_terminates_mysql_child(self):
        with tempfile.TemporaryDirectory() as temporary:
            dump = Path(temporary) / "legacy.sql"
            dump.write_bytes(b"SELECT 1;\n")
            process = mock.Mock()
            process.stdin = io.BytesIO()
            process.wait.side_effect = [KeyboardInterrupt, -15]
            process.poll.return_value = None
            with mock.patch.object(legacy.subprocess, "Popen", return_value=process):
                with self.assertRaises(KeyboardInterrupt):
                    legacy.import_stream(self.valid_config(), dump, normalize=False)
            process.terminate.assert_called_once_with()
            self.assertEqual(process.wait.call_count, 2)

    def test_source_tables_and_extension_counts_must_survive(self):
        source = {"accounts": 2, "website_posts": 7}
        legacy.verify_source_rows(source, dict(source), {"accounts"})
        with self.assertRaisesRegex(legacy.LegacyImportError, "lost source tables"):
            legacy.verify_source_rows(source, {"accounts": 2}, {"accounts"})
        with self.assertRaisesRegex(legacy.LegacyImportError, "extension-table"):
            legacy.verify_source_rows(
                source, {"accounts": 2, "website_posts": 8}, {"accounts"})
        duplicate_source = {"player_item_extra_descr": 10}
        with self.assertRaisesRegex(legacy.LegacyImportError, "not archived"):
            legacy.verify_source_rows(
                duplicate_source, {"player_item_extra_descr": 4},
                {"player_item_extra_descr"})
        legacy.verify_source_rows(
            duplicate_source,
            {"player_item_extra_descr": 4,
             "legacy_import_player_item_extra_descr": 10},
            {"player_item_extra_descr"})

    def test_failure_path_restores_the_owner_only_backup(self):
        events: list[str] = []
        with mock.patch.object(legacy, "validate_dump", return_value="0" * 64), \
                mock.patch.object(legacy, "active_connections", return_value=0), \
                mock.patch.object(legacy, "create_backup",
                                  side_effect=lambda *_: events.append("backup")), \
                mock.patch.object(legacy, "wipe_target",
                                  side_effect=lambda *_: events.append("wipe")), \
                mock.patch.object(
                    legacy, "import_stream",
                    side_effect=legacy.LegacyImportError("synthetic restore failure")), \
                mock.patch.object(legacy, "restore_backup",
                                  side_effect=lambda *_: events.append("restore")):
            with self.assertRaisesRegex(legacy.LegacyImportError, "synthetic"):
                legacy.import_legacy_dump(
                    self.valid_config(), Path("env"), Path("dump"), Path("backup"))
        self.assertEqual(events, ["backup", "wipe", "restore"])

    def test_interrupt_path_restores_backup_then_reraises_interrupt(self):
        events: list[str] = []
        with mock.patch.object(legacy, "validate_dump", return_value="0" * 64), \
                mock.patch.object(legacy, "active_connections", return_value=0), \
                mock.patch.object(legacy, "create_backup",
                                  side_effect=lambda *_: events.append("backup")), \
                mock.patch.object(legacy, "wipe_target",
                                  side_effect=lambda *_: events.append("wipe")), \
                mock.patch.object(legacy, "import_stream", side_effect=KeyboardInterrupt), \
                mock.patch.object(legacy, "restore_backup",
                                  side_effect=lambda *_: events.append("restore")):
            with self.assertRaises(KeyboardInterrupt):
                legacy.import_legacy_dump(
                    self.valid_config(), Path("env"), Path("dump"), Path("backup"))
        self.assertEqual(events, ["backup", "wipe", "restore"])

    def test_account_bank_migration_ignores_only_duplicate_keys(self):
        migration = (ROOT / "migrations/run_migration.sh").read_text()
        section = migration[
            migration.index('run_sql "migrate player banks to account banks"'):
            migration.index('run_sql "create private_chests table"')]
        self.assertNotIn("INSERT IGNORE INTO account_banks", section)
        self.assertIn("ON DUPLICATE KEY UPDATE account_name = account_name", section)

    def test_import_establishes_all_character_baselines_before_readiness(self):
        statements = []

        def mysql_result(_config, statement):
            statements.append(statement)
            return "" if len(statements) == 1 else "0\t0\t0"

        with mock.patch.object(legacy, "run_mysql", side_effect=mysql_result):
            legacy.establish_character_baselines(self.valid_config())
        transaction, readiness = statements
        self.assertIn("START TRANSACTION", transaction)
        self.assertIn("INSERT INTO currency_wallet_baseline", transaction)
        self.assertIn("INSERT INTO epic_balance_baseline", transaction)
        self.assertIn("INSERT INTO combat_frag_baseline", transaction)
        self.assertIn("p.frag_revision=0", transaction)
        self.assertIn("FROM combat_frag_ledger", transaction)
        self.assertNotIn("INSERT IGNORE", transaction)
        self.assertIn("account_characters", readiness)
        self.assertIn("combat_frag_baseline", readiness)

    def test_import_refuses_to_infer_a_missing_baseline_after_history(self):
        with mock.patch.object(
                legacy, "run_mysql", side_effect=["", "0\t0\t1"]):
            with self.assertRaisesRegex(
                    legacy.LegacyImportError, "ledger history requires review"):
                legacy.establish_character_baselines(self.valid_config())


if __name__ == "__main__":
    unittest.main()
