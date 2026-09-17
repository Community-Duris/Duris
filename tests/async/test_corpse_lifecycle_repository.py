#!/usr/bin/env python3
"""Offline contracts for atomic SQL corpse lifecycle disposal."""

from __future__ import annotations

import hashlib
import json
import stat
import unittest
from pathlib import Path

from _paths import SRC


ROOT = Path(__file__).resolve().parents[2]
MIGRATION = ROOT / "migrations/immutable/0019_corpse_lifecycle_authority.sql"
VERIFIER = ROOT / "migrations/immutable/0019_corpse_lifecycle_authority.sh"
RUNNER = ROOT / "tests/async/run_corpse_lifecycle_repository_schema_mysql.sh"
HARNESS = ROOT / "tests/async/corpse_lifecycle_repository_mysql_harness.cpp"


class CorpseLifecycleRepositoryTest(unittest.TestCase):
    def test_authority_migration_is_sealed_registered_and_in_bootstrap(self) -> None:
        manifest = json.loads((ROOT / "migrations/migration_manifest.json").read_text())
        step = next(item for item in manifest["migrations"]
                    if item["id"] == "0019_corpse_lifecycle_authority")
        self.assertEqual((step["id"], step["sequence"]),
                         ("0019_corpse_lifecycle_authority", 19))
        self.assertEqual(step["apply_checksum"],
                         hashlib.sha256(MIGRATION.read_bytes()).hexdigest())
        self.assertEqual(step["verify_checksum"],
                         hashlib.sha256(VERIFIER.read_bytes()).hexdigest())
        migration = MIGRATION.read_text()
        bootstrap = (ROOT / "migrations/bootstrap_multithread_safe.sql").read_text()
        for token in ("corpse_revision", "idx_corpse_owner_save",
                      "corpse_catalog_state", "catalog_revision"):
            self.assertIn(token, migration)
            self.assertIn(token, bootstrap)
        self.assertIn("singleton_rows", VERIFIER.read_text())

    def test_runtime_and_lifecycle_contracts_include_corpse_authority(self) -> None:
        runtime = json.loads(
            (ROOT / "migrations/runtime_compatibility_manifest.json").read_text()
        )
        lifecycle = json.loads(
            (ROOT / "migrations/data_lifecycle_manifest.json").read_text()
        )
        lifecycle_entries = {entry["id"]: entry for entry in lifecycle["entries"]}
        self.assertEqual(runtime["current_table_count"], 198)
        self.assertIn("'corpse_catalog_state'", runtime["runtime_table_sql_list"])
        self.assertEqual(runtime["migration_head"]["id"],
                         "0022_telemetry_progression")
        entry = lifecycle_entries["database:corpse_catalog_state"]
        self.assertEqual(entry["data_category"], "reconciliation_or_replay_record")
        self.assertEqual(entry["export_rule"]["disposition"], "exclude")
        self.assertTrue(entry["protected_record"])

    def test_repository_keeps_every_domain_under_one_savepoint(self) -> None:
        source = (SRC / "corpse_lifecycle_repository.c").read_text()
        for token in (
            "FOR UPDATE",
            "SAVEPOINT corpse_lifecycle_domain",
            "ROLLBACK TO SAVEPOINT corpse_lifecycle_domain",
            "item_transfer_repository_execute",
            "collector_repository_apply_item_boundary",
            "currency_repository_execute",
            "artifact_domain_state",
            "materialize_items",
            "DELETE FROM corpses",
            "UPDATE corpse_catalog_state",
        ):
            self.assertIn(token, source)
        execute = source[source.index("bool corpse_lifecycle_repository_execute(") :]
        self.assertLess(execute.index("SAVEPOINT corpse_lifecycle_domain"),
                        execute.index("item_transfer_repository_execute"))
        self.assertLess(execute.index("item_transfer_repository_execute"),
                        execute.index("finish_corpse(connection"))

    def test_generic_dispatcher_journals_receipt_and_outbox_before_commit(self) -> None:
        source = (SRC / "critical_command_repository.c").read_text()
        branch = source[source.index("if (corpse_command)") :]
        self.assertIn("corpse_lifecycle_repository_execute", branch)
        self.assertIn("CORPSE_LIFECYCLE_RESULT_BYTES", branch)
        self.assertIn("collector_events", branch)
        self.assertLess(branch.index("insert_outbox"), branch.index("finish_inbox"))
        self.assertLess(branch.index("finish_inbox"),
                        branch.index('execute(connection, "COMMIT")'))

    def test_disposable_dual_engine_journey_is_wired(self) -> None:
        runner = RUNNER.read_text()
        makefile = (ROOT / "Makefile").read_text()
        self.assertTrue(RUNNER.stat().st_mode & stat.S_IXUSR)
        self.assertIn("never reads .env", runner)
        self.assertIn("CORPSE_LIFECYCLE_REPOSITORY_DB_IMAGE", runner)
        self.assertIn("bootstrap_multithread_safe.sql", runner)
        self.assertIn("corpse_lifecycle_repository_mysql_harness.cpp", runner)
        self.assertIn("src/persistence/corpse_lifecycle_command.c", runner)
        self.assertIn("src/persistence/corpse_lifecycle_repository.c", runner)
        self.assertIn("-Wpedantic -Werror", runner)
        self.assertIn("run_corpse_lifecycle_repository_schema_mysql.sh", makefile)

    def test_existing_dispatcher_harnesses_link_the_corpse_branch(self) -> None:
        runners = ROOT.glob("tests/async/run*_mysql.sh")
        linked = []
        for path in runners:
            source = path.read_text()
            if "src/persistence/critical_command_repository.c" not in source:
                continue
            linked.append(path.name)
            self.assertIn("src/persistence/corpse_lifecycle_command.c", source, path.name)
            self.assertIn("src/persistence/corpse_lifecycle_repository.c", source, path.name)
        self.assertGreaterEqual(len(linked), 10)

    def test_journey_covers_all_terminal_paths_replay_and_rollback(self) -> None:
        harness = HARNESS.read_text()
        for token in (
            "corpse_lifecycle_action::release",
            "corpse_lifecycle_action::destroy",
            "corpse_lifecycle_action::resurrect",
            "corpse_lifecycle_action::raise_follower",
            "corpse_lifecycle_action::release_nested",
            "critical_apply_outcome::already_applied",
            "critical_apply_outcome::terminal_failure",
            "collector_listings",
            "currency_ledger",
            "artifact_domain_state",
            "saved_item_affects",
            "player_items",
            "critical_outbox",
        ):
            self.assertIn(token, harness)


if __name__ == "__main__":
    unittest.main()
