#!/usr/bin/env python3
"""Source contracts for restored saved-world-item routing."""

from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"


class SavedItemFlatfileRoutingTests(unittest.TestCase):
    def test_historical_entry_points_do_not_reach_sql_in_flat_primary(self):
        files = (SRC / "files.c").read_text()
        write = files[files.index("void writeSavedItem") :]
        write = write[: write.index("void restoreSavedItems")]
        restore = files[files.index("void restoreSavedItems") :]
        restore = restore[: restore.index("void PurgeSavedItemFile")]
        purge = files[files.index("void PurgeSavedItemFile") :]
        purge = purge[: purge.index("int writeShopKeeper")]
        self.assertLess(write.index("PERSISTENCE_MODE_FLATFILE_PRIMARY"),
                        write.index("sql_save_saved_item"))
        self.assertLess(restore.index("PERSISTENCE_MODE_FLATFILE_PRIMARY"),
                        restore.index("sql_restore_saved_items"))
        self.assertLess(purge.index("PERSISTENCE_MODE_FLATFILE_PRIMARY"),
                        purge.index("sql_delete_saved_item"))
        self.assertIn("item_ownership_runtime_lookup", write)
        self.assertIn("item_custody_state::destroyed", purge)

    def test_storage_admin_mutations_wait_for_item_transfer_ack(self):
        actwiz = (SRC / "actwiz.c").read_text()
        storage = actwiz[actwiz.index("enum class flat_storage_action") :]
        storage = storage[: storage.index("void newb_spellup")]
        completion = storage[storage.index("void flat_storage_completion") :]
        completion = completion[: completion.index("bool submit_flat_storage_destroy")]
        self.assertIn("submit_flat_storage_establish", storage)
        self.assertIn("submit_flat_storage_destroy", storage)
        self.assertIn("submit_flat_storage_remove_next", storage)
        self.assertIn("item_transfer_reason::operator_repair", storage)
        self.assertIn("item_transfer_reason::destruction", storage)
        self.assertLess(completion.index("if (!committed)"),
                        completion.index("obj_to_room(storage"))
        self.assertLess(completion.index("if (!committed)"),
                        completion.index("obj_from_obj(item)"))
        self.assertLess(completion.index("if (!committed)"),
                        completion.index("extract_obj(storage"))

    def test_room_repository_admits_only_the_bounded_storage_transfers(self):
        repository = (SRC / "flatfile_item_repository.c").read_text()
        room = repository[repository.index("bool room_transfer") :]
        room = room[: room.index("bool generic_transfer_supported")]
        self.assertIn("item_owner_type::system", room)
        self.assertIn("item_owner_type::destruction", room)
        self.assertIn("item_transfer_reason::operator_repair", room)
        movement = (SRC / "item_movement_transaction.c").read_text()
        self.assertIn("adoption_only", movement)
        self.assertLess(movement.index("if (entry.adoption_only)"),
                        movement.index("item_movement_transaction_submit(actor, root"))


if __name__ == "__main__":
    unittest.main()
