#!/usr/bin/env python3
"""Source contracts for issue 524's trusted-steal custody boundary.

The live game journey needs two player sessions and a running ownership
backend, so these checks pin the parts that must remain true in every build:
admission happens before detachment, the exact UID is published after commit,
and a refused publication cannot destroy an already-authoritative graph.
"""

import unittest

from _paths import source
from _source_contract import function_body


class TrustedStealCustodyContractTests(unittest.TestCase):
    def test_trusted_steal_is_a_distinct_player_to_player_transfer_reason(self):
        header = source("item/item_transfer_command.h").read_text()
        command = source("item/item_transfer_command.c").read_text()
        actoth = source("cmd/actoth.c").read_text()

        self.assertIn("trusted_steal", header)
        self.assertIn("case item_transfer_reason::trusted_steal:", command)
        self.assertIn("item_transfer_reason::trusted_steal", actoth)
        self.assertIn("item_owner_type::player", actoth)

    def test_admission_precedes_both_inventory_and_equipment_detachment(self):
        actoth = source("cmd/actoth.c").read_text()
        submit = function_body(
            source("cmd/actoth.c").read_text(), r"static bool submit_trusted_steal\("
        )
        self.assertIn("item_ownership_runtime_lookup", submit)
        self.assertIn("ownership.state != item_custody_state::active", submit)
        self.assertIn("item_movement_transaction_submit(", submit)
        self.assertNotIn("obj_from_char", submit)
        self.assertNotIn("unequip_char", submit)
        self.assertNotIn("obj_to_char", submit)
        self.assertNotIn("return false;", submit)

        steal = function_body(source("cmd/actoth.c").read_text(), r"void do_steal\(")
        equipped = steal[steal.index("if (roll && (GET_LEVEL(ch) > 40)"):]
        self.assertIn("submit_trusted_steal(ch, victim, obj, TRUE", equipped)
        inventory = steal[steal.index("case 2:"):steal.index("case 3:")]
        self.assertIn("submit_trusted_steal(ch, victim, obj, FALSE", inventory)
        self.assertIn("return;", equipped[equipped.index("submit_trusted_steal"):])
        self.assertIn("return;", inventory[inventory.index("submit_trusted_steal"):])

    def test_completion_uses_the_original_uid_and_only_publishes_after_commit(self):
        actoth = source("cmd/actoth.c").read_text()
        completion = function_body(
            actoth, r"static void trusted_steal_completion\("
        )
        self.assertIn("uint64_t item_uid", actoth)
        self.assertIn("uint32_t victim_pid", actoth)
        self.assertIn("find_trusted_steal_item(context.item_uid)", completion)
        self.assertLess(completion.index("if (!committed)"), completion.index("obj_from_char"))
        self.assertLess(completion.index("if (!committed)"), completion.index("unequip_char"))
        self.assertLess(completion.index("OBJ_CARRIED_BY(object, thief)"),
                        completion.index('send_to_char("Got it!'))
        self.assertIn("OBJ_IN_ROOM(object, context.source_room)", completion)
        self.assertIn("OBJ_NOWHERE(object)", completion)
        self.assertIn("reconnect to recover", completion)

    def test_shared_publication_refusal_does_not_extract_authoritative_graphs(self):
        handler = function_body(
            source("world/handler.c").read_text(), r"void obj_to_char\("
        )
        self.assertIn("has_authoritative_ownership", handler)
        self.assertIn(
            "!has_authoritative_ownership &&\n\t\t\t    item_creation_grant_submit_to_player",
            handler,
        )
        self.assertIn("if (!has_authoritative_ownership)", handler)
        self.assertIn("preserved existing owned graph", handler)

        refused = handler[handler.index("if (!has_authoritative_ownership)"):]
        self.assertIn("extract_obj(object, FALSE)", refused)
        self.assertLess(refused.index("if (!has_authoritative_ownership)"),
                        refused.index("extract_obj(object, FALSE)"))
        self.assertNotIn("extract_obj(object, TRUE)", refused)

    def test_recursive_capture_keeps_a_stolen_container_graph_atomic(self):
        movement = source("item/item_movement_transaction.c").read_text()
        capture = function_body(
            movement, r"bool capture\(P_obj object"
        )
        self.assertIn("for (P_obj child = object->contains", capture)
        self.assertIn("capture(child, root_uid, object->obj_uid, items)", capture)
        self.assertIn("player_item_snapshot_tree_capture(root", movement)
        self.assertIn("item_ownership_runtime_apply(entry.payload, result)", movement)

    def test_trusted_player_gate_remains_in_place(self):
        steal = function_body(source("cmd/actoth.c").read_text(), r"void do_steal\(")
        self.assertIn('if (!IS_TRUSTED(ch))', steal)
        self.assertIn("Steal is temporarily disabled", steal)


if __name__ == "__main__":
    unittest.main()
