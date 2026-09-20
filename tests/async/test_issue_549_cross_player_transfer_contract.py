#!/usr/bin/env python3
"""Contracts for issue 549's cross-player Soulbind and Slip handoffs.

Both commands have command-specific side effects, so a durable ownership commit
must precede live publication and those side effects must remain deferred while
the exact UID is retryable after a disconnect.
"""

import unittest

from _paths import source
from _source_contract import function_body


class CrossPlayerTransferContractTests(unittest.TestCase):
    def test_soulbind_and_slip_are_distinct_player_transfer_reasons(self):
        header = source("item/item_transfer_command.h").read_text()
        command = source("item/item_transfer_command.c").read_text()

        self.assertIn("soulbind", header)
        self.assertIn("slip", header)
        self.assertIn("case item_transfer_reason::soulbind:", command)
        self.assertIn("case item_transfer_reason::slip:", command)

    def test_soulbind_admits_before_any_cross_player_mutation(self):
        magic = source("magic/magic.c").read_text()
        soulbind = function_body(magic, r"void do_soulbind\(")
        cross_player = soulbind[soulbind.index("if (ch != victim)") :]

        self.assertIn("item_ownership_runtime_lookup", cross_player)
        self.assertIn("ownership.state != item_custody_state::active", cross_player)
        self.assertIn("item_movement_transaction_submit(", cross_player)
        self.assertIn("item_transfer_reason::soulbind", cross_player)
        self.assertIn("soulbind_transfer_completion", cross_player)
        self.assertIn("total_carried_weight(victim)", cross_player)
        self.assertIn("ch->in_room", cross_player)
        self.assertIn("victim->in_room", cross_player)
        self.assertNotIn("obj_from_char", soulbind)
        self.assertNotIn("obj_to_char", soulbind)
        self.assertLess(
            cross_player.index("item_movement_transaction_submit("),
            cross_player.index("apply_soulbind_metadata(victim, obj)"),
        )

    def test_soulbind_completion_defers_metadata_until_commit_and_publication(self):
        magic = source("magic/magic.c").read_text()
        completion = function_body(
            magic, r"static void soulbind_transfer_completion\("
        )

        self.assertIn("find_soulbind_item(context.item_uid)", completion)
        self.assertIn("find_soulbind_player(context.victim_pid)", completion)
        self.assertIn("OBJ_CARRIED_BY(object, victim)", completion)
        self.assertLess(completion.index("if (!committed)"), completion.index("obj_from_char"))
        self.assertLess(completion.index("OBJ_CARRIED_BY(object, victim)"),
                        completion.index("apply_soulbind_metadata(victim, object)"))
        self.assertLess(completion.index("remove_soulbind(victim)"),
                        completion.index("apply_soulbind_metadata(victim, object)"))
        self.assertIn("mark_player_dirty_components", completion)

    def test_slip_admits_before_detachment_and_defers_success_effects(self):
        rogues = source("classes/rogues.c").read_text()
        slip = function_body(rogues, r"void do_slip\(")
        success = slip[slip.index("if (success)") :]
        durable = success[success.index("if (IS_PC(ch) && IS_PC(vict)") :]
        durable = durable[: durable.index("notch_skill(ch")]

        self.assertIn("item_ownership_runtime_lookup", durable)
        self.assertIn("ownership.state != item_custody_state::active", durable)
        self.assertIn("item_movement_transaction_submit(", durable)
        self.assertIn("item_transfer_reason::slip", durable)
        self.assertIn("slip_transfer_completion", durable)
        self.assertIn("ch->in_room", durable)
        self.assertIn("vict->in_room", durable)
        self.assertNotIn("obj_from_char", durable)
        self.assertLess(
            durable.index("item_movement_transaction_submit("),
            durable.index("The Slip transfer is pending"),
        )

        completion = function_body(
            rogues, r"static void slip_transfer_completion\("
        )
        self.assertIn("find_slip_item(context.item_uid)", completion or "")
        self.assertIn("find_slip_player(context.victim_pid)", completion or "")
        self.assertIn("already_published", completion or "")
        self.assertIn("OBJ_NOWHERE(object)", completion or "")
        self.assertIn("ITEM2_CRUMBLELOOT", completion or "")
        self.assertLess(completion.index("if (!committed)"), completion.index("obj_from_char"))
        self.assertLess(completion.index("OBJ_CARRIED_BY(object, victim)"),
                        completion.index("notch_skill(source"))
        self.assertLess(completion.index("OBJ_CARRIED_BY(object, victim)"),
                        completion.index("writeCharacter(source"))

    def test_committed_player_publication_remains_retryable_for_disconnects(self):
        movement = source("item/item_movement_transaction.c").read_text()
        publish = function_body(movement, r"void publish\(")

        self.assertIn("retained_player_transfer_reason", movement)
        self.assertIn("player_transfer_live_ready", movement)
        self.assertIn("retain_player_transfer_publication", movement)
        self.assertIn("item_transfer_reason::soulbind", movement)
        self.assertIn("item_transfer_reason::slip", movement)
        self.assertIn("destination_ready", function_body(
            movement, r"void item_movement_transaction_player_ready\("
        ))
        self.assertIn("pending.find(pending_key)", publish)
        self.assertLess(
            publish.index("!retain_creation_grant && !retain_trusted_steal && !retain_player_transfer"),
            publish.index("completion_fn(actor, committed && registry_applied"),
        )

    def test_payload_scope_requires_distinct_live_player_owners(self):
        command = source("item/item_transfer_command.c").read_text()
        validate = function_body(command, r"bool validate_payload\(")

        self.assertIn("const bool soulbind", validate)
        self.assertIn("const bool slip", validate)
        self.assertIn("payload.from_owner.type != item_owner_type::player", validate)
        self.assertIn("payload.to_owner.type != item_owner_type::player", validate)
        self.assertIn("payload.from_owner.id == payload.to_owner.id", validate)
        self.assertIn("payload.multi_root", validate)
        self.assertIn("payload.reason_id", validate)

    def test_sabotage_replacing_soulbind_reason_is_detected(self):
        magic = source("magic/magic.c").read_text()
        marker = "item_transfer_reason::soulbind, GET_PID(ch)"
        self.assertIn(marker, magic)
        sabotaged = magic.replace(
            marker, "item_transfer_reason::player_give, GET_PID(ch)", 1
        )
        self.assertNotIn(marker, sabotaged)


if __name__ == "__main__":
    unittest.main()
