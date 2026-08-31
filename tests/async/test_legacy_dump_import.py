#!/usr/bin/env python3
"""Focused safety and portability regressions for legacy dump replacement."""

from __future__ import annotations

import hashlib
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


if __name__ == "__main__":
    unittest.main()
