#!/usr/bin/env python3
"""Source contracts for the live ownership ACK boundary."""

from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"


class LiveItemMovementContractTests(unittest.TestCase):
    def test_runtime_hydrates_authoritative_rows_not_relative_events(self):
        sql = (SRC / "sql.c").read_text()
        owner_check = sql[sql.rindex("bool sql_persistence_item_owner_matches_identity"):]
        owner_check = owner_check[:owner_check.index("bool sql_hydrate_item_owner_revisions")]
        self.assertIn("FROM item_current_owner", owner_check)
        self.assertIn("item_owner_revision", owner_check)
        self.assertNotIn("persistence_item_events", owner_check)
        self.assertIn("item_ownership_runtime_hydrate", owner_check)

    def test_pending_adapter_retains_scalars_and_publishes_after_commit(self):
        movement = (SRC / "item_movement_transaction.c").read_text()
        pending = movement[movement.index("struct pending_movement"):]
        pending = pending[:pending.index("};")]
        self.assertNotIn("P_obj", pending)
        self.assertNotIn("P_char", pending)
        self.assertIn("critical_command_coordinator_submit", movement)
        self.assertIn("item_ownership_runtime_apply", movement)
        self.assertIn("movement_conflicts(from_owner, to_owner)", movement)
        self.assertLess(movement.index("movement_conflicts(from_owner, to_owner)"),
                        movement.index("critical_command_coordinator_submit"))
        self.assertLess(movement.index("const bool committed"),
                        movement.index("item_ownership_runtime_apply"))

    def test_audited_commands_defer_pointer_mutation(self):
        actobj = (SRC / "actobj.c").read_text()
        for reason in ("player_get", "player_drop", "player_put", "player_give",
                       "corpse_loot"):
            self.assertIn(f"item_transfer_reason::{reason}", actobj)
        self.assertIn("item_movement_transaction_submit", actobj)
        self.assertIn("item_get_ack_publication", actobj)
        self.assertIn("item_put_ack_publication", actobj)
        self.assertIn("start_container_bulk_get", actobj)
        self.assertIn("continue_bulk_get(actor, true)", actobj)
        self.assertIn("start_bulk_drop", actobj)
        self.assertIn("continue_bulk_drop(actor, true)", actobj)
        self.assertIn("start_bulk_put", actobj)
        self.assertIn("continue_bulk_put(actor, stored)", actobj)
        # every bulk command now serialises its transactions instead of
        # refusing the command outright.
        self.assertNotIn("Durable container items must be collected one at a time", actobj)
        self.assertNotIn("Durable items must be dropped one at a time", actobj)
        self.assertNotIn("Durable items must be put away one at a time", actobj)

    def test_death_items_are_chained_after_ack_and_failure_is_preserved(self):
        fight = (SRC / "fight.c").read_text()
        make_corpse = fight[fight.index("P_obj make_corpse"):]
        self.assertIn("submit_next_corpse_item", fight)
        self.assertIn("item_transfer_reason::corpse_create", fight)
        self.assertIn("failed_preserved", fight)
        self.assertNotIn("sql_delete_player_items", make_corpse)
        completion = fight[fight.index("void corpse_item_completion"):]
        self.assertLess(completion.index("if (!committed)"), completion.index("obj_from_char"))

    def test_corpse_identity_and_floor_hints_are_non_authoritative(self):
        migration = (ROOT / "migrations/live_item_movement_cutover.sql").read_text()
        baseline = (ROOT / "migrations/baseline_item_ownership.sh").read_text()
        self.assertIn("c.save_id", migration)
        self.assertIn("CAST(pd.pid AS UNSIGNED) << 32", baseline)
        self.assertIn("CAST(c.save_id AS UNSIGNED)", baseline)
        actobj = (SRC / "actobj.c").read_text()
        self.assertNotIn("redis_check_floor_pickup", actobj)
        self.assertNotIn("redis_check_floor_drop", actobj)
        recovery = (SRC / "world_recovery_pipeline.c").read_text()
        self.assertIn("sql_persistence_reconcile_world_recovery_items", recovery)
        self.assertIn("item_ownership_runtime_hydrate_many_atomic", recovery)
        self.assertNotIn("redis_check_floor_pickup", recovery)
        self.assertNotIn("redis_check_floor_drop", recovery)

    def test_reconnect_replays_retained_completion(self):
        nanny = (SRC / "nanny.c").read_text()
        self.assertGreaterEqual(nanny.count("item_movement_transaction_player_ready"), 2)

    def test_give_completion_can_publish_to_a_linkdead_recipient(self):
        actobj = (SRC / "actobj.c").read_text()
        helper = actobj[actobj.index("P_char find_live_player_pid"):]
        helper = helper[:helper.index("void item_get_completion")]
        self.assertIn("character_list", helper)
        give = actobj[actobj.index("void item_give_completion"):]
        give = give[:give.index("void item_put_completion")]
        self.assertIn("find_live_player_pid(context.recipient_pid)", give)
        self.assertNotIn("find_player_by_pid(context.recipient_pid)", give)

    def test_same_owner_reparenting_is_authoritative(self):
        command = (SRC / "item_transfer_command.c").read_text()
        repository = (SRC / "item_transfer_repository.c").read_text()
        movement = (SRC / "item_movement_transaction.c").read_text()
        self.assertIn("target_root_item_uid", command)
        self.assertIn("target_parent_item_uid", command)
        self.assertIn("const bool same_owner", repository)
        self.assertIn("SET root_item_uid=?", repository)
        self.assertIn("target_container", movement)


if __name__ == "__main__":
    unittest.main()
