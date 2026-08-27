#!/usr/bin/env python3
"""Source and schema contracts for authoritative item ownership."""

from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"


class ItemOwnershipContractTests(unittest.TestCase):
    def test_schema_and_guarded_tools_are_wired(self):
        migration = (ROOT / "migrations/item_ownership_ledger.sql").read_text()
        bootstrap = (ROOT / "migrations/bootstrap_multithread_safe.sql").read_text()
        runner = (ROOT / "migrations/run_migration.sh").read_text()
        for token in (
            "item_uid_allocator", "item_owner_revision", "item_current_owner",
            "item_ownership_baseline", "item_ownership_quarantine",
            "item_ownership_ledger", "uq_item_ledger_item_revision",
        ):
            self.assertIn(token, migration)
            self.assertIn(token, bootstrap)
        self.assertIn("item_ownership_ledger.sql", runner)
        for script in ("baseline_item_ownership.sh", "reconcile_item_ownership.sh",
                       "verify_item_ownership_schema.sh"):
            self.assertTrue((ROOT / "migrations" / script).stat().st_mode & 0o111)

    def test_command_is_fixed_bounded_typed_and_revisioned(self):
        header = (SRC / "item_transfer_command.h").read_text()
        implementation = (SRC / "item_transfer_command.c").read_text()
        self.assertIn("ITEM_TRANSFER_MAX_ITEMS = 12", header)
        self.assertIn("ITEM_TRANSFER_PAYLOAD_BYTES", header)
        for owner in ("player", "container", "room", "corpse", "locker", "auction",
                      "system", "destruction"):
            self.assertIn(owner, header)
        self.assertIn("expected_item_revision", header)
        self.assertIn("critical_entity_key_less", implementation)

    def test_repository_locks_complete_root_and_commits_all_authorities(self):
        repository = (SRC / "item_transfer_repository.c").read_text()
        for token in (
            "item_owner_revision", "ORDER BY item_uid FOR UPDATE",
            "WHERE root_item_uid=?", "item_current_owner SET",
            "INSERT INTO item_ownership_ledger", "update_owner_revision",
        ):
            self.assertIn(token, repository)
        critical = (SRC / "critical_command_repository.c").read_text()
        branch = critical[critical.index("if (item_command)"):]
        commit = branch.index('execute(connection, "COMMIT")')
        self.assertLess(branch.index("insert_outbox"), commit)
        self.assertLess(branch.index("finish_inbox"), commit)

    def test_uid_allocator_is_sql_reserved_before_world_boot(self):
        allocator = (SRC / "item_uid_allocator.c").read_text()
        sql = (SRC / "sql.c").read_text()
        db = (SRC / "db.c").read_text()
        redis = (SRC / "redis.c").read_text()
        self.assertIn("FOR UPDATE", allocator)
        self.assertIn("item_uid_allocator_reserve(DB", sql)
        self.assertLess(sql.index("item_uid_allocator_reserve(DB"), sql.index("sql_pool_init"))
        self.assertIn("persistence_next_item_uid()", db)
        self.assertNotIn("next_obj_uid = loaded", redis)

    def test_snapshot_repositories_do_not_write_owner_authority(self):
        for name in ("player_snapshot_repository.c", "sql_player.c", "files.c"):
            source = (SRC / name).read_text()
            self.assertNotIn("UPDATE item_current_owner", source)
            self.assertNotIn("DELETE FROM item_current_owner", source)
            self.assertNotIn("UPDATE item_owner_revision", source)

    def test_synthetic_adapter_is_pointer_free_and_coordinator_backed(self):
        header = (SRC / "item_transfer_synthetic.h").read_text()
        implementation = (SRC / "item_transfer_synthetic.c").read_text()
        self.assertNotIn("P_obj", header)
        self.assertNotIn("P_char", header)
        self.assertIn("critical_command_coordinator_submit", implementation)
        self.assertIn("item_transfer_command_decode_result", implementation)


if __name__ == "__main__":
    unittest.main()
