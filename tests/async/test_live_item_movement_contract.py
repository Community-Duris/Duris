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

    def test_transfer_captures_exact_snapshot_before_submission(self):
        movement = (SRC / "item_movement_transaction.c").read_text()
        capture = movement.index("player_item_snapshot_tree_capture")
        encode = movement.index("player_item_snapshot_list_encode")
        build = movement.index("item_transfer_command_build")
        submit = movement.index("critical_command_coordinator_submit")
        self.assertLess(capture, encode)
        self.assertLess(encode, build)
        self.assertLess(build, submit)
        self.assertIn("payload.item_blob_size", movement)

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
        self.assertIn("corpse->weight = GET_WEIGHT(ch);", fight)
        self.assertIn("int contents_weight = total_carried_weight(ch);", fight)
        self.assertIn("sizeof(context), corpse", fight)
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

    def test_flat_corpse_creation_and_loot_are_composite(self):
        repository = (SRC / "flatfile_item_repository.c").read_text()
        world = (SRC / "flatfile_world_item_repository.c").read_text()
        artifact = (SRC / "flatfile_artifact_repository.c").read_text()
        movement = (SRC / "item_movement_transaction.c").read_text()
        self.assertIn("flatfile_world_item_prepare_corpse_transfer", world)
        self.assertIn("flatfile_artifact_prepare_corpse_transfer", artifact)
        self.assertIn("capture_corpse_metadata", movement)
        self.assertIn("corpse_loot_transfer(payload)", repository)
        self.assertIn("corpse_create_transfer(payload)", repository)
        apply = repository[repository.index(
            "critical_apply_result flatfile_item_repository_apply") :]
        prepare = apply.index("flatfile_world_item_prepare_corpse_transfer")
        artifact_prepare = apply.index("flatfile_artifact_prepare_corpse_transfer", prepare)
        image = apply.index("corpse.after_image", prepare)
        artifact_image = apply.index("corpse_artifacts.after_image", artifact_prepare)
        commit = apply.index("flatfile_authority_transaction_commit", image)
        self.assertLess(prepare, image)
        self.assertLess(artifact_prepare, artifact_image)
        self.assertLess(image, commit)
        self.assertLess(artifact_image, commit)
        supported = repository[repository.index("bool generic_transfer_supported") :]
        supported = supported[:supported.index("bool locker_custody_matches")]
        self.assertIn("corpse_loot_transfer(payload)", supported)
        self.assertIn("corpse_create_transfer(payload)", supported)
        self.assertIn("ITEM_TRANSFER_PAYLOAD_VERSION", supported)

    def test_flat_room_item_moves_are_composite(self):
        repository = (SRC / "flatfile_item_repository.c").read_text()
        world = (SRC / "flatfile_world_item_repository.c").read_text()
        artifact = (SRC / "flatfile_artifact_repository.c").read_text()
        self.assertIn("flatfile_world_item_prepare_room_transfer", world)
        self.assertIn("flatfile_artifact_prepare_room_transfer", artifact)
        supported = repository[repository.index("bool generic_transfer_supported") :]
        supported = supported[:supported.index("bool locker_custody_matches")]
        self.assertIn("room_transfer(payload)", supported)
        apply = repository[repository.index(
            "critical_apply_result flatfile_item_repository_apply") :]
        prepare = apply.index("flatfile_world_item_prepare_room_transfer")
        artifact_prepare = apply.index("flatfile_artifact_prepare_room_transfer", prepare)
        image = apply.index("room.after_image", prepare)
        artifact_image = apply.index("room_artifacts.after_image", artifact_prepare)
        commit = apply.index("flatfile_authority_transaction_commit", image)
        self.assertLess(prepare, image)
        self.assertLess(artifact_prepare, artifact_image)
        self.assertLess(image, commit)
        self.assertLess(artifact_image, commit)


if __name__ == "__main__":
    unittest.main()
