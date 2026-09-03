#!/usr/bin/env python3
"""Source and manifest contracts for pre-mutation boot compatibility."""

from __future__ import annotations

from _paths import SRC
import json
import subprocess
import sys
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))
import validate_runtime_compatibility as runtime  # noqa: E402


class RuntimeBootCompatibilityTest(unittest.TestCase):
    def setUp(self):
        self.sql = (SRC / "sql.c").read_text()
        self.comm = (SRC / "comm.c").read_text()
        self.header = (SRC / "runtime_compatibility_contract.h").read_text()

    def test_manifests_and_compiled_contract_are_synchronized(self):
        report = runtime.validate()
        self.assertEqual(report["current_table_count"], 174)
        self.assertEqual(report["migration_head"],
                         "0006_kingdom_realms")
        self.assertEqual(set(report["normalized_metadata_fingerprints"]),
                         {"mysql8", "mariadb10_11"})
        self.assertIn("RUNTIME_MIGRATION_HISTORY_CHECKSUM", self.header)
        self.assertIn("RUNTIME_MYSQL8_METADATA_FINGERPRINT", self.header)
        self.assertIn("RUNTIME_MARIADB10_11_METADATA_FINGERPRINT", self.header)
        for constant in ("RUNTIME_DB_CHARACTER_SET", "RUNTIME_DB_TIME_ZONE",
                         "RUNTIME_DB_ISOLATION", "RUNTIME_DB_SQL_MODE",
                         "RUNTIME_DB_TIMEOUT_SECONDS",
                         "RUNTIME_DB_REMOTE_TLS_REQUIRED",
                         "RUNTIME_METADATA_MAX_BYTES"):
            self.assertIn(constant, self.header)

    def test_preflight_precedes_every_boot_mutation_and_pool_start(self):
        start = self.sql.index('logit(LOG_STATUS, "Initializing validated MySQL connection.")')
        initialize = self.sql[start:self.sql.index("/* Handle a query", start)]
        verify = initialize.index("sql_verify_boot_database()")
        populate = initialize.index("sql_populate_lookup_tables()")
        allocator = initialize.index("item_uid_allocator_reserve")
        pool = initialize.index("sql_pool_init")
        self.assertLess(verify, populate)
        self.assertLess(populate, allocator)
        self.assertLess(allocator, pool)

    def test_mysql_boundary_precedes_hydration_workers_replay_and_gameplay(self):
        mysql_boundary = (
            "if (persistence_mode_requires_mysql() && initialize_mysql() < 0)"
        )
        main = self.comm[
            self.comm.index(mysql_boundary):
            self.comm.index("return (0);", self.comm.index(mysql_boundary))
        ]
        initialize = main.index("initialize_mysql()")
        hydrate = main.index("sql_hydrate_item_owner_revisions()")
        redis = main.index("redis_init()")
        run_game = main.index("run_the_game(port, sslport)")
        self.assertLess(initialize, hydrate)
        self.assertLess(hydrate, redis)
        self.assertLess(redis, run_game)

        game = self.comm[self.comm.index("void run_the_game(int port, int sslport)"):
                         self.comm.index("void game_loop(int port, int sslport)")]
        for boundary in ("player_load_pipeline_init", "locker_async_init",
                         "player_save_pipeline_init",
                         "critical_command_coordinator_init", "game_loop(port, sslport)"):
            self.assertIn(boundary, game)

        game_loop = self.comm[self.comm.index("void game_loop(int port, int sslport)"):
                              self.comm.index("bool runtime_listener_address")]
        self.assertIn("redis_load_world_state", game_loop)
        self.assertIn("drain_new_connections", game_loop)

    def test_schema_fingerprint_and_redacted_reason_ids_are_enforced(self):
        self.assertIn("sql_verify_metadata_fingerprint", self.sql)
        self.assertIn("RUNTIME_TABLE_SQL_LIST", self.sql)
        self.assertIn("table_name IN (%s)", self.sql)
        self.assertIn("table_type='BASE TABLE'", self.sql)
        self.assertIn("JOIN information_schema.tables t", self.sql)
        self.assertIn("column_type LIKE '%unsigned'", self.sql)
        self.assertIn("k.referenced_table_name IN (", self.sql)
        verifier = (ROOT / "migrations/verify_runtime_compatibility.sh").read_text()
        self.assertIn(
            "k.table_name IN ($runtime_tables) OR "
            "k.referenced_table_name IN ($runtime_tables)", verifier)
        for token in ("information_schema.tables", "information_schema.columns",
                      "information_schema.statistics", "referential_constraints"):
            self.assertIn(token, self.sql)
        self.assertIn("RUNTIME_METADATA_MAX_BYTES - canonical.size()", self.sql)
        for reason in ("COMPAT-E001", "COMPAT-E002", "COMPAT-E003", "COMPAT-E007"):
            self.assertIn(reason, self.sql)
        compatibility_logs = [line for line in self.sql.splitlines() if "COMPAT-E" in line]
        self.assertFalse(any(secret in line.lower() for line in compatibility_logs
                             for secret in ("password", "select ", "insert ", "update ")))

    def test_lookup_unchanged_path_is_before_transaction(self):
        function = self.sql[self.sql.index("bool sql_populate_lookup_tables()"):
                            self.sql.index("/* Resolve the requested database")]
        self.assertLess(function.index("lookup_state_matches"),
                        function.index('"START TRANSACTION"'))
        self.assertIn("publication skipped", function)
        self.assertNotIn("DELETE FROM races\");\n\tqry", function)

    def test_lookup_publication_is_atomic_and_state_advances_last(self):
        function = self.sql[self.sql.index("bool sql_populate_lookup_tables()"):
                            self.sql.index("/* Resolve the requested database")]
        for token in ("START TRANSACTION", "ON DUPLICATE KEY UPDATE",
                      "DELETE FROM races WHERE id NOT IN",
                      "DELETE FROM classes WHERE id NOT IN", "ROLLBACK", "COMMIT"):
            self.assertIn(token, function)
        self.assertLess(function.index("DELETE FROM classes WHERE id NOT IN"),
                        function.index("INSERT INTO lookup_dataset_state"))
        self.assertGreater(function.rfind("lookup_rows_match"),
                           function.index("DELETE FROM classes WHERE id NOT IN"))
        self.assertLess(function.rfind("lookup_rows_match"),
                        function.index("INSERT INTO lookup_dataset_state"))
        self.assertLess(function.index("INSERT INTO lookup_dataset_state"),
                        function.index('"COMMIT"'))

    def test_runtime_validator_cli_is_redacted_and_valid(self):
        result = subprocess.run(
            ["python3", str(ROOT / "scripts/validate_runtime_compatibility.py")],
            capture_output=True, text=True, check=False,
        )
        self.assertEqual(result.returncode, 0, result.stderr)
        report = json.loads(result.stdout)
        self.assertEqual(report["status"], "valid")
        self.assertNotIn("DB_PASSWD", result.stdout + result.stderr)


if __name__ == "__main__":
    unittest.main()
