#!/usr/bin/env python3
"""Contracts for numeric, ACK-gated locker ownership custody."""

from _paths import SRC
from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[2]
def function_body(source: str, signature: str, next_signature: str) -> str:
    start = source.index(signature)
    end = source.index(next_signature, start)
    return source[start:end]


class LockerOwnershipCutoverTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.lockers = (SRC / "storage_lockers.c").read_text()
        cls.lockers_h = (SRC / "storage_lockers.h").read_text()
        cls.actobj = (SRC / "actobj.c").read_text()
        cls.sql = (SRC / "sql.c").read_text()
        cls.sql_player = (SRC / "sql_player.c").read_text()
        cls.movement = (SRC / "item_movement_transaction.c").read_text()
        cls.snapshot = (SRC / "locker_async.c").read_text()
        cls.copyover = (SRC / "copyover.c").read_text()
        cls.flat_items = (SRC / "flatfile_item_repository.c").read_text()
        cls.flat_lockers = (SRC / "flatfile_locker_repository.c").read_text()

    def test_live_identity_uses_stable_locker_and_chest_ids(self):
        for token in (
            "locker_owner_for_room",
            "locker_owner_for_container",
            "GetLockerId()",
            "GetPublicChestId()",
            "GetChestId()",
            "item_owner_type::locker",
        ):
            self.assertIn(token, self.lockers)
        self.assertIn("m_publicChestId", self.lockers_h)
        self.assertIn("SetPublicChestId(sql_get_or_create_public_chest(locker_id))",
                      self.lockers)

    def test_deposit_and_withdraw_are_ack_gated(self):
        self.assertIn("locker_owner_for_room(ch, &destination)", self.actobj)
        self.assertIn("locker_owner_for_container(actor, container, &locker_destination)",
                      self.actobj)
        self.assertIn("item_transfer_reason::locker_deposit", self.actobj)
        self.assertIn("locker_deposit ?", self.actobj)
        self.assertIn("item_transfer_reason::locker_withdraw", self.actobj)
        self.assertIn("actor, object, NULL, source, locker_destination", self.actobj)
        self.assertIn("locker_deposit ? 0 : 1", self.actobj)
        self.assertIn("locker_deposit ? 0 :", self.actobj)
        self.assertIn("world[ch->in_room].number", self.actobj)

    def test_terminal_teardown_and_snapshot_wait_for_ack(self):
        leave = function_body(self.lockers, "static bool locker_handle_leave(",
                              "static int locker_handle_save_hook(")
        self.assertLess(leave.index("item_movement_transaction_player_busy(ch)"),
                        leave.index("LockerToPFile()"))
        self.assertLess(leave.index("LockerToPFile()"), leave.index("free_locker(room)"))
        save = function_body(self.lockers, "static int save_locker_char(P_char ch, int bTerminal)",
                             "bool StorageLocker::LockerToPFile")
        self.assertLess(save.index("item_movement_transaction_player_busy(ch)"),
                        save.index("LockerToPFile()"))
        self.assertIn("std::any_of", self.movement)
        critical_drain = self.copyover.index("critical_command_coordinator_drain(3000)")
        player_drain = self.copyover.index("player_save_pipeline_drain(3000)", critical_drain)
        final_locker_drain = self.copyover.index("locker_async_drain(3000)", player_drain)
        world_drain = self.copyover.index("redis_world_recovery_drain(3000)", player_drain)
        self.assertLess(critical_drain, player_drain)
        self.assertLess(player_drain, final_locker_drain)
        self.assertLess(final_locker_drain, world_drain)

    def test_restore_checks_exact_numeric_owner_identity(self):
        self.assertIn("sql_persistence_item_owner_matches_identity", self.sql)
        self.assertIn("static_cast<unsigned long long>(locker_id)", self.sql_player)
        self.assertIn("static_cast<unsigned long long>(chest_id)", self.sql_player)
        self.assertNotIn('sql_persistence_item_owner_matches(obj->obj_uid, "locker"',
                         self.sql_player)
        locker_restore = function_body(
            self.sql_player,
            "static P_obj sql_load_locker_items(int locker_id",
            "bool sql_locker_exists(",
        )
        self.assertNotIn("owner_ref", locker_restore)

    def test_flat_transfer_composes_locker_and_ownership_after_images(self):
        self.assertIn("flatfile_locker_prepare_item_transfer", self.flat_lockers)
        apply = self.flat_items[self.flat_items.index(
            "critical_apply_result flatfile_item_repository_apply") :]
        prepare = apply.index("flatfile_locker_prepare_item_transfer")
        locker_image = apply.index("locker.after_image", prepare)
        commit = apply.index("flatfile_authority_transaction_commit", locker_image)
        self.assertLess(prepare, locker_image)
        self.assertLess(locker_image, commit)
        self.assertIn("item_transfer_reason::locker_deposit", self.flat_items)
        self.assertIn("item_transfer_reason::locker_withdraw", self.flat_items)

    def test_snapshot_is_not_ownership_authority_and_worker_is_pointer_free(self):
        builder = function_body(self.snapshot, "static char *build_locker_snapshot_sql(",
                                "/* ---------------- worker ---------------- */")
        for forbidden in (
            "item_current_owner",
            "item_owner_revision",
            "item_ownership_ledger",
            "item_ownership_baseline",
        ):
            self.assertNotIn(forbidden, builder)
        job = function_body(self.snapshot, "struct locker_async_job", "struct locker_async_result")
        self.assertNotIn("P_obj", job)
        self.assertNotIn("P_char", job)
        self.assertIn("char *sql", job)

    def test_cutover_is_guarded_rerunnable_and_non_destructive(self):
        migration = (ROOT / "migrations/locker_ownership_cutover.sql").read_text()
        wrapper = (ROOT / "migrations/apply_locker_ownership_cutover.sh").read_text()
        runner = (ROOT / "migrations/run_migration.sh").read_text()
        baseline = (ROOT / "migrations/baseline_item_ownership.sh").read_text()
        self.assertIn("INSERT IGNORE INTO private_chests", migration)
        self.assertIn("WHERE item.chest_id IS NULL", migration)
        self.assertNotIn("DELETE FROM locker_items", migration)
        self.assertNotIn("DELETE owner_revision", migration)
        self.assertIn("environment is not development/local/test", wrapper)
        self.assertIn("database name is not development/local/test", wrapper)
        self.assertIn("locker_ownership_cutover.sql", runner)
        self.assertIn("COALESCE(i.chest_id,public_chest.id,0)", baseline)


if __name__ == "__main__":
    unittest.main()
