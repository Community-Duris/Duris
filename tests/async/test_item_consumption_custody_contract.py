#!/usr/bin/env python3
"""Custody boundaries for junk and donation item removal."""

from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[2]
ACTOBJ = (ROOT / "src/cmd/actobj.c").read_text(encoding="utf-8")
ACTOTH = (ROOT / "src/cmd/actoth.c").read_text(encoding="utf-8")
MOVEMENT = (ROOT / "src/item/item_movement_transaction.c").read_text(
    encoding="utf-8"
)
JANITOR = (ROOT / "src/specs/specs.winterhaven.c").read_text(encoding="utf-8")
HANDLER = (ROOT / "src/world/handler.c").read_text(encoding="utf-8")


def body(source: str, signature: str) -> str:
    start = source.index(signature)
    brace = source.index("{", start)
    depth = 0
    for pos in range(brace, len(source)):
        if source[pos] == "{":
            depth += 1
        elif source[pos] == "}":
            depth -= 1
            if depth == 0:
                return source[start : pos + 1]
    raise AssertionError(f"unterminated function: {signature}")


class ItemConsumptionCustodyContract(unittest.TestCase):
    def test_junk_batch_commits_before_live_removal_or_reward(self) -> None:
        command = body(ACTOBJ, "void do_junk(")
        submit = body(ACTOBJ, "bool submit_junk_batch(")
        publish = body(ACTOBJ, "bool junk_publication(")
        completed = body(ACTOBJ, "void junk_completed(")
        self.assertIn("item_command_uses_durable_ownership(object)", command)
        self.assertIn("junk_has_durable_child(object)", command)
        self.assertIn("item_ownership_runtime_lookup(selected.uid, &runtime)", submit)
        self.assertIn("item_movement_transaction_submit_batch(", submit)
        self.assertIn("item_transfer_reason::destruction", submit)
        self.assertIn("junk_publication", submit)
        self.assertIn("junk_completed", submit)
        self.assertNotIn("extract_obj(", command)
        self.assertNotIn("ADD_MONEY(", command)
        self.assertIn("if (!committed)", publish)
        self.assertIn("publish_junk_live(actor, object, selected)", publish)
        self.assertNotIn("ADD_MONEY(", publish)
        self.assertIn("junk_batches.erase(found);", completed)
        self.assertLess(completed.index("junk_batches.erase(found);"),
                        completed.index("ADD_MONEY(actor, reward);"))

    def test_donation_uses_room_owned_well_and_post_ack_chain(self) -> None:
        submit = body(ACTOTH, "bool submit_donation(P_char ch,")
        publish = body(ACTOTH, "bool donation_publication(")
        completed = body(ACTOTH, "void donation_completed(")
        self.assertIn("well_owner.owner.type != item_owner_type::room", submit)
        self.assertIn("well_owner.root_item_uid != sub_object->obj_uid", submit)
        self.assertIn("item_movement_transaction_submit(", submit)
        self.assertIn("donation_publication", submit)
        self.assertIn("donation_completed", submit)
        self.assertIn("if (!committed)", publish)
        self.assertIn("publish_donation_live(actor, item, well, context.destroy)",
                      publish)
        self.assertIn("continue_donation_batch(actor)", completed)
        adapter = body(MOVEMENT, "void publish(")
        self.assertIn("critical_command_coordinator_acknowledge_publication(", adapter)
        self.assertLess(
            adapter.index("pending.erase(current);"),
            adapter.index("completion_fn(actor, committed, result, error_code,"),
        )

    def test_quaff_removes_owned_potion_before_post_ack_spell_effects(self) -> None:
        command = body(ACTOTH, "void do_quaff(")
        publish = body(ACTOTH, "bool quaff_publication(")
        completed = body(ACTOTH, "void quaff_completed(")
        effects = body(ACTOTH, "void apply_quaff_effects(")
        self.assertIn("item_command_uses_durable_ownership(bottle)", command)
        self.assertIn("item_tree_has_durable_ownership(bottle)", command)
        self.assertIn("context.local_item = NULL", command)
        self.assertIn("item_transfer_reason::destruction", command)
        self.assertIn("quaff_completed", command)
        self.assertIn("quaff_publication", command)
        self.assertNotIn("extract_obj(bottle)", command)
        self.assertIn("if (!committed)", publish)
        self.assertIn("extract_obj(bottle)", publish)
        self.assertNotIn("spell_damage(", publish)
        self.assertIn("apply_quaff_effects(actor, context)", completed)
        self.assertIn("spell_damage(", effects)
        self.assertIn("context.values[index]", effects)

    def test_janitor_leaves_active_room_and_carried_forests_alone(self) -> None:
        janitor = body(JANITOR, "int wh_janitor(")
        self.assertLess(janitor.index("item_tree_has_active_custody(o)"),
                        janitor.index("obj_from_room(o);"))
        self.assertIn("!IS_ARTIFACT(o) && !item_tree_has_active_custody(o)", janitor)

    def test_room_leaf_decay_commits_destruction_before_extraction(self) -> None:
        decay = body(HANDLER, "void Decay(P_obj obj)")
        publish = body(HANDLER, "bool publish_room_leaf_decay(")
        eligible = body(HANDLER, "bool ordinary_room_leaf_decay_eligible(")
        self.assertLess(decay.index("if (item_tree_has_active_custody(obj))"),
                        decay.index("if (OBJ_ROOM(obj))"))
        self.assertIn("item_movement_transaction_submit(", decay)
        self.assertIn("item_transfer_reason::destruction", decay)
        self.assertIn("publish_room_leaf_decay", decay)
        self.assertIn("ordinary_room_leaf_decay_eligible(obj, obj->loc.room)", decay)
        self.assertIn("OBJ_IN_ROOM(item, room)", eligible)
        self.assertIn("!item->contains", eligible)
        self.assertLess(publish.index("!ordinary_room_leaf_decay_eligible(item, context.room)"),
                        publish.index("extract_obj(item, TRUE)"))


if __name__ == "__main__":
    unittest.main()
