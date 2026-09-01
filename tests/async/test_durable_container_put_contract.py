#!/usr/bin/env python3
"""Source contracts for the durable single-item put into an owned container.

Nesting is ledger state.  `item_current_owner` carries each item's parent and
root, `capture()` refuses a subtree whose ledger nesting disagrees with the live
object tree, and player load rebuilds nesting from the ledger rather than from
the saved rows.  A put that moves an item into a container without submitting a
transfer therefore strands the container: every later give or drop of it fails
preflight, and the contents un-nest on the next login.  These contracts pin the
single-item put to the same durable path `put all` already uses, and pin the
refusal diagnostics that made the original report unattributable.
"""

from _paths import SRC
import re
import unittest


def function_body(text, signature, terminator):
    start = text.index(signature)
    return text[start:text.index(terminator, start)]


class DurableContainerPutContractTests(unittest.TestCase):
    def test_put_defers_to_the_ownership_pipeline_for_a_same_owner_container(self):
        actobj = (SRC / "actobj.c").read_text()
        self.assertIn("bool defer_durable_put(", actobj)
        self.assertNotIn("defer_cross_owner_put", actobj)
        body = function_body(actobj, "bool defer_durable_put(", "\nbool submit_player_drop(")
        # The locker branch keeps its same-owner shortcut: a chest is an owner,
        # not a parent, so an item already owned by the chest records nothing.
        locker = body[:body.index("if (!item_ownership_runtime_lookup(container->obj_uid")]
        container = body[body.index("if (!item_ownership_runtime_lookup(container->obj_uid"):]
        self.assertIn("item_owner_identity_equal(source, locker_destination)", locker)
        # The container branch must not short-circuit on a matching owner; the
        # parent linkage still has to reach the ledger.
        self.assertNotIn("item_owner_identity_equal(source, container_runtime.owner)",
                         container)
        self.assertIn("item_transfer_reason::player_put", container)
        # The container itself is the ownership target, which is what carries
        # target_parent_item_uid / target_root_item_uid into the payload.
        submit = container[container.index("item_movement_transaction_submit("):]
        self.assertIn("actor, object, container, source, destination", submit)

    def test_single_item_put_records_the_same_topology_as_bulk_put(self):
        actobj = (SRC / "actobj.c").read_text()
        movement = (SRC / "item_movement_transaction.c").read_text()
        bulk = function_body(actobj, "void start_bulk_put(", "\nvoid do_put(")
        self.assertIn("P_obj ownership_target = container;", bulk)
        # Both paths hand the container to the same payload field.
        self.assertIn(".target_parent_item_uid = target_container ? target_container->obj_uid : 0,",
                      movement)

    def test_capture_names_both_sides_of_a_topology_mismatch(self):
        movement = (SRC / "item_movement_transaction.c").read_text()
        capture = function_body(movement, "bool capture(P_obj object", "\nbool capture_absent(")
        self.assertIn("outcome=topology_mismatch", capture)
        self.assertIn("cause=missing_ledger_row", capture)
        self.assertIn("ledger(root=", capture)
        self.assertIn("live(root=", capture)

    def test_every_submission_refusal_carries_a_reason(self):
        header = (SRC / "item_movement_transaction.h").read_text()
        movement = (SRC / "item_movement_transaction.c").read_text()
        for reason in ("invalid_request", "queue_saturated", "pending_conflict",
                       "owner_mismatch", "missing_owner_revision", "topology_mismatch",
                       "snapshot_failure", "allocation_failure", "command_build_failure",
                       "coordinator_rejected"):
            self.assertIn(f"\t{reason},", header)
            self.assertIn(f"item_movement_reject::{reason}", movement)
        self.assertIn("item_movement_reject *reject = NULL", header)
        for signature, terminator in (
                ("bool item_movement_transaction_submit(P_char actor",
                 "\nconst char *item_movement_reject_name"),
                ("bool item_movement_transaction_submit_batch(",
                 "\nconst char *item_movement_reject_name")):
            body = function_body(movement, signature, terminator)
            body = body[:body.index("\n}\n")] if "\n}\n" in body else body
            self.assertNotIn("return false;", body,
                             f"{signature} still refuses without naming a reason")

    def test_busy_and_unreconciled_failures_read_differently(self):
        movement = (SRC / "item_movement_transaction.c").read_text()
        actobj = (SRC / "actobj.c").read_text()
        transient = function_body(movement, "bool item_movement_reject_is_transient",
                                  "\nbool item_creation_grant_submit_to_player")
        self.assertIn("item_movement_reject::pending_conflict", transient)
        self.assertIn("item_movement_reject::queue_saturated", transient)
        self.assertNotIn("item_movement_reject::topology_mismatch", transient)
        self.assertNotIn("item_movement_reject::owner_mismatch", transient)
        # The single collapsed sentence is gone from every command.
        self.assertNotIn("busy or lacks authoritative ownership", actobj)
        self.assertNotIn("busy or its ownership changed", actobj)
        report = function_body(actobj, "void report_movement_reject(",
                               "\nvoid report_batch_movement_reject(")
        self.assertIn("item_movement_reject_is_transient(reason)", report)
        self.assertIn("item_movement_reject_name(reason)", report)

    def test_audited_commands_report_the_reason_they_were_refused(self):
        actobj = (SRC / "actobj.c").read_text()
        for command in ("give", "get", "put", "drop"):
            self.assertRegex(actobj, re.compile(
                r'report_(batch_)?movement_reject\([^;]*"' + command + r'"', re.S),
                f"{command} does not report a refusal reason")
        # submit_player_drop hands its reason back rather than swallowing it.
        self.assertIn("bool submit_player_drop(P_char ch, P_obj object, "
                      "item_movement_reject *reject)", actobj)


if __name__ == "__main__":
    unittest.main()
