#!/usr/bin/env python3
"""Modeled captured agreement controls; never export authenticity or execution."""

import copy
import hashlib
import unittest

from test_quest_cut_checks import empty_cut, item, root
import native_quest_pair_checks as adapter
import quest_cut_checks as checks


def modeled_pair(held=False):
    raw = b"x" * 48
    receipt = dict(present=True, outcome=0, durable_revision=12, error_code=0,
                   failure_stage=0, result_size=len(raw), result_sha256=hashlib.sha256(raw).hexdigest(),
                   result_array_sha256=hashlib.sha256(raw + bytes(4096 - len(raw))).hexdigest())
    pair = dict(scope="original-pair value correlation only", pair_checked=not held, pair_valid=not held,
                external_proof_required=list(adapter.EXTERNAL),
                required_xp_mask=0, required_economic_mask=3,
                continuation_sha256=hashlib.sha256(b"modeled terms").hexdigest())
    for role, tag in (("parent", "06"), ("child", "10")):
        pair[role] = dict(operation=tag + "00" * 15, command_sha256=tag * 32,
                          attachment_sha256="aa" * 32, command_bytes=16000, attachment_bytes=24000,
                          revision=11 if role == "parent" else 7, phase=2, payload_version=12,
                          pid=42, instance=9000, birth="01" + "00" * 15, source_sha256="44" * 32,
                          save_revision=23, publication_stage=2, handoff=2 if role == "parent" else 0,
                          branch=0, publication_steps=[2] * 6, give_messages=[2] * 3,
                          give_hooks=[2] * 9 if role == "parent" else [0] * 9,
                          consumed_steps=[] if role == "parent" else [2] * 3, receipt=copy.deepcopy(receipt))
    if held:
        pair["child"].update(phase=1, publication_stage=1, publication_steps=[1, 0, 0, 0, 0, 0])
    return pair


def modeled_cut(pair, held=False):
    cut = empty_cut("QP03")
    cut["meta"].update(authority="native", mobile_instance_ids=[9000],
                       source_commit="df0570c5456d4d747ca1320ce958c1db52bb08fd")
    cut.update(epochs=[], player_affects=[], birth_origins=[])
    for role in ("parent", "child"):
        value = pair[role]
        root(cut, value["operation"])
        cut["inbox"][-1].update(command_hash=value["command_sha256"], payload_version=12,
                                  durable_revision=12, result_payload=(b"x" * 48).hex())
    cut["obligations"] = [dict(offering_operation_id=pair["child"]["operation"], player_pid=42,
                               continuation=b"modeled terms".hex(), acknowledged=0 if held else 1,
                               xp_applied_mask=0)]
    # Reward already moved by an ordinary owner before pair cleanup.
    cut["items"] = [item(200, 16075, owner=3, revision=4)]
    return cut


def binding(cut, pair):
    return dict(pair=copy.deepcopy(pair), meta={key: cut["meta"][key] for key in adapter.PINS},
                process_generation=5, coordinator_generation=9)


def held_owner(cut, pair):
    owner = binding(cut, pair)
    child = pair["child"]
    owner.update(hold=dict(held=True, operation=child["operation"], pid=42, generation=14,
                           save_revision=23, command_sha256=child["command_sha256"]),
                 physical_released=False, retired=False, uncertain=True, poisoned=False)
    return owner


def attempt(cut, pair, sequence=1, result="ok"):
    owner = binding(cut, pair)
    owner["publication"] = {}
    owner["verification"] = {}
    for role in ("parent", "child"):
        value = pair[role]
        owner["publication"][role] = dict(operation=value["operation"], command_sha256=value["command_sha256"],
            receipt=copy.deepcopy(value["receipt"]), checkpoint_result="ok", hold_generation=14,
            hold_consumed=True, physical_released=True, native_ack_uncertain=False,
            publication_checkpointing=False, physical_proof_sha256="55" * 32, census_sha256="66" * 32)
        owner["verification"][role] = dict(command_sha256=value["command_sha256"],
                                             receipt=dict(value["receipt"], outcome=1))
    owner["verification"]["obligation"] = dict(operation=pair["child"]["operation"],
        continuation_sha256=pair["continuation_sha256"], acknowledged=True, reader_result="ok", error_code=0,
        required_xp_mask=0, xp_applied_mask=0, required_economic_mask=3, economic_applied_mask=3, complete_xp_set=True)
    owner.update(cleanup=dict(original_session=8, verified_session=8, after_session=8,
                              rollback_confirmed=True, cleanup_error=0, disposition="idle_verified"),
                 parent_live=False, successor=None, terminal_pair_attempted=True, sequence=sequence,
                 verification_origin_sequence=sequence,
                 postimage=dict(bytes=12000, sha256="33" * 32, complete=True), kind="attempt",
                 journal_result=result, coordinator_return=result == "ok", retired=result == "ok",
                 parent_present=result != "ok", child_present=result != "ok", context_uncertain=result == "append_uncertain")
    return owner


class PairAgreementTests(unittest.TestCase):
    def test_original_phase1_stable_hold_and_lost_reply(self):
        pair = modeled_pair(held=True)
        pair["child"]["receipt"].update(present=False, outcome=0, durable_revision=0, result_size=0,
            result_sha256=hashlib.sha256(b"").hexdigest(), result_array_sha256=hashlib.sha256(bytes(4096)).hexdigest())
        cut = modeled_cut(pair, held=True)
        owner = held_owner(cut, pair)
        result = adapter.assert_qp03_held_pair(cut, cut, pair, pair, owner, owner)
        self.assertEqual(result["external_proof_required"], adapter.EXTERNAL)
        self.assertFalse(pair["pair_checked"])

    def test_hold_replacement_phase2_and_candidate_drift_refuse(self):
        pair = modeled_pair(held=True)
        cut = modeled_cut(pair, held=True)
        owner = held_owner(cut, pair)
        changes = [lambda o: o["hold"].update(generation=15), lambda o: o["hold"].update(save_revision=24),
                   lambda o: o["hold"].update(command_sha256="77" * 32), lambda o: o.update(physical_released=True),
                   lambda o: o["meta"].update(binary_sha256="77" * 32), lambda o: o.update(poisoned=True)]
        for change in changes:
            bad = copy.deepcopy(owner)
            change(bad)
            with self.subTest(change=changes.index(change)), self.assertRaises(checks.CutError):
                adapter.assert_qp03_held_pair(cut, cut, pair, pair, owner, bad)
        bad_pair = copy.deepcopy(pair)
        bad_pair.update(pair_checked=True, pair_valid=True)
        with self.assertRaises(checks.CutError):
            adapter.assert_qp03_held_pair(cut, cut, bad_pair, bad_pair, owner, owner)

    def test_terminal_success_preserves_already_moved_reward(self):
        pair = modeled_pair()
        cut = modeled_cut(pair)
        observed = attempt(cut, pair)
        result = adapter.assert_qp03_pair_retirement(cut, cut, pair, [observed])
        self.assertEqual(result["observations"], 1)
        self.assertEqual(result["external_proof_required"], adapter.EXTERNAL)
        self.assertEqual(cut["items"][0]["owner_type"], 3)

    def test_uncertain_retry_then_guarded_success_and_latched_repeat(self):
        pair = modeled_pair()
        cut = modeled_cut(pair)
        uncertain, success = attempt(cut, pair, result="append_uncertain"), attempt(cut, pair, 2)
        repeat = copy.deepcopy(success)
        repeat.update(sequence=3, kind="latched_repeat", journal_result=None, terminal_pair_attempted=False)
        self.assertEqual(adapter.assert_qp03_pair_retirement(cut, cut, pair, [uncertain, success, repeat])["observations"], 3)
        failure = attempt(cut, pair, result="io_failure")
        adapter.assert_qp03_pair_retirement(cut, cut, pair, [failure, success])

    def test_terminal_success_with_complete_empty_postimage(self):
        pair = modeled_pair()
        cut = modeled_cut(pair)
        observed = attempt(cut, pair)
        observed["postimage"] = dict(bytes=0, sha256=hashlib.sha256(b"").hexdigest(), complete=True)
        result = adapter.assert_qp03_pair_retirement(cut, cut, pair, [observed])
        self.assertEqual(result["observations"], 1)
        self.assertEqual(result["external_proof_required"], adapter.EXTERNAL)

    def test_empty_postimage_uncertain_retry_success_and_latch(self):
        pair = modeled_pair()
        cut = modeled_cut(pair)
        uncertain = attempt(cut, pair, result="append_uncertain")
        success = attempt(cut, pair, 2)
        for observed in (uncertain, success):
            observed["postimage"] = dict(bytes=0, sha256=hashlib.sha256(b"").hexdigest(), complete=True)
        repeat = copy.deepcopy(success)
        repeat.update(sequence=3, kind="latched_repeat", journal_result=None, terminal_pair_attempted=False)
        result = adapter.assert_qp03_pair_retirement(cut, cut, pair, [uncertain, success, repeat])
        self.assertEqual(result["observations"], 3)
        self.assertEqual(result["external_proof_required"], adapter.EXTERNAL)

    def test_empty_postimage_requires_canonical_digest_complete_and_integer_bounds(self):
        pair = modeled_pair()
        cut = modeled_cut(pair)
        for changed in (dict(sha256="33" * 32), dict(sha256="00" * 32), dict(complete=False),
                        dict(bytes=-1), dict(bytes=False), dict(bytes=True), dict(bytes=0.0), dict(bytes=2**64)):
            observed = attempt(cut, pair)
            observed["postimage"] = dict(bytes=0, sha256=hashlib.sha256(b"").hexdigest(), complete=True)
            observed["postimage"].update(changed)
            with self.subTest(changed=changed), self.assertRaisesRegex(checks.CutError, "postimage"):
                adapter.assert_qp03_pair_retirement(cut, cut, pair, [observed])

    def test_empty_postimage_retry_cannot_change_complete_image_in_either_direction(self):
        pair = modeled_pair()
        cut = modeled_cut(pair)
        for empty_first in (True, False):
            uncertain = attempt(cut, pair, result="append_uncertain")
            success = attempt(cut, pair, 2)
            (uncertain if empty_first else success)["postimage"] = dict(
                bytes=0, sha256=hashlib.sha256(b"").hexdigest(), complete=True)
            with self.subTest(empty_first=empty_first), self.assertRaisesRegex(checks.CutError, "changed complete attempted postimage"):
                adapter.assert_qp03_pair_retirement(cut, cut, pair, [uncertain, success])

    def test_exact_original_sql_and_obligation_links_required(self):
        pair = modeled_pair()
        cut = modeled_cut(pair)
        changes = [lambda c: c["inbox"][0].update(command_hash="77" * 32),
                   lambda c: c["inbox"][1].update(durable_revision=13),
                   lambda c: c["inbox"][1].update(result_payload=(b"y" * 48).hex()),
                   lambda c: c["operations"].clear(), lambda c: c["obligations"].clear(),
                   lambda c: c["obligations"][0].update(continuation=b"replacement terms".hex()),
                   lambda c: c["obligations"][0].update(acknowledged=0),
                   lambda c: c["xp_entitlements"].append(dict(offering_operation_id=pair["child"]["operation"],
                       recipient_pid=42, reward_index=0, amount=100, applied=1))]
        for change in changes:
            bad = copy.deepcopy(cut)
            change(bad)
            with self.subTest(change=changes.index(change)), self.assertRaises(checks.CutError):
                adapter.assert_qp03_pair_retirement(bad, bad, pair, [attempt(bad, pair)])

    def test_publication_ack_is_distinct_from_reward_ack_and_carrier(self):
        pair = modeled_pair()
        cut = modeled_cut(pair)
        changes = [lambda o: o["publication"]["child"].update(hold_consumed=False),
                   lambda o: o["publication"]["parent"].update(native_ack_uncertain=True),
                   lambda o: o["verification"]["obligation"].update(acknowledged=False),
                   lambda o: o["verification"]["obligation"].update(economic_applied_mask=1),
                   lambda o: o["verification"]["obligation"].update(complete_xp_set=False),
                   lambda o: o["verification"]["child"]["receipt"].update(outcome=0)]
        for change in changes:
            observed = attempt(cut, pair)
            change(observed)
            with self.subTest(change=changes.index(change)), self.assertRaises(checks.CutError):
                adapter.assert_qp03_pair_retirement(cut, cut, pair, [observed])
        pair["child"]["publication_steps"][0] = 1
        with self.assertRaises(checks.CutError):
            adapter.assert_qp03_pair_retirement(cut, cut, pair, [attempt(cut, pair)])

    def test_session_cleanup_is_same_session_and_confirmed(self):
        pair = modeled_pair()
        cut = modeled_cut(pair)
        for field, value in (("verified_session", 9), ("after_session", 9), ("rollback_confirmed", False),
                             ("cleanup_error", 1), ("disposition", "unknown")):
            observed = attempt(cut, pair)
            observed["cleanup"][field] = value
            with self.subTest(field=field), self.assertRaises(checks.CutError):
                adapter.assert_qp03_pair_retirement(cut, cut, pair, [observed])

    def test_journal_ok_absence_latch_and_ack_alone_are_insufficient(self):
        pair = modeled_pair()
        cut = modeled_cut(pair)
        changes = [lambda o: o.update(coordinator_return=False), lambda o: o.update(retired=False),
                   lambda o: o.update(kind="latched_repeat", journal_result=None),
                   lambda o: o.update(context_uncertain=True), lambda o: o["postimage"].update(complete=False)]
        for change in changes:
            observed = attempt(cut, pair)
            change(observed)
            with self.subTest(change=changes.index(change)), self.assertRaises(checks.CutError):
                adapter.assert_qp03_pair_retirement(cut, cut, pair, [observed])
        with self.assertRaises(checks.CutError):
            adapter.assert_qp03_pair_retirement(cut, cut, pair, [])

    def test_retry_preserves_pair_generation_and_whole_postimage(self):
        pair = modeled_pair()
        cut = modeled_cut(pair)
        changes = [lambda o: o["postimage"].update(sha256="77" * 32), lambda o: o.update(process_generation=6),
                   lambda o: o["pair"]["child"].update(revision=8), lambda o: o.update(sequence=1)]
        for change in changes:
            success = attempt(cut, pair, 2)
            change(success)
            with self.subTest(change=changes.index(change)), self.assertRaises(checks.CutError):
                adapter.assert_qp03_pair_retirement(cut, cut, pair, [attempt(cut, pair, result="append_uncertain"), success])
        with self.assertRaises(checks.CutError):
            adapter.assert_qp03_pair_retirement(cut, cut, pair, [attempt(cut, pair, result="append_uncertain")])

    def test_retired_latch_does_not_attempt_second_mutation(self):
        pair = modeled_pair()
        cut = modeled_cut(pair)
        with self.assertRaises(checks.CutError):
            adapter.assert_qp03_pair_retirement(cut, cut, pair, [attempt(cut, pair), attempt(cut, pair, 2)])
        repeat = attempt(cut, pair, 2)
        repeat.update(kind="latched_repeat", journal_result=None, context_uncertain=True)
        with self.assertRaises(checks.CutError):
            adapter.assert_qp03_pair_retirement(cut, cut, pair, [attempt(cut, pair), repeat])

    def test_live_handoff1_requires_actual_successor(self):
        pair = modeled_pair()
        pair["parent"]["handoff"] = 1
        cut = modeled_cut(pair)
        adapter.assert_qp03_pair_retirement(cut, cut, pair, [attempt(cut, pair)])
        observed = attempt(cut, pair)
        observed["parent_live"] = True
        with self.assertRaises(checks.CutError):
            adapter.assert_qp03_pair_retirement(cut, cut, pair, [observed])

    def test_no_economic_replay_or_reward_rewrite_during_cleanup(self):
        pair = modeled_pair()
        cut = modeled_cut(pair)
        for field in ("items", "player", "player_affects"):
            after = copy.deepcopy(cut)
            if field == "items":
                after[field][0]["item_revision"] += 1
            elif field == "player":
                after[field][0]["copper"] -= 1
            else:
                after[field].append(dict(affect=1))
            with self.subTest(field=field), self.assertRaises(checks.CutError):
                adapter.assert_qp03_pair_retirement(cut, after, pair, [attempt(cut, pair)])

    def test_coherent_forgery_cannot_self_authenticate(self):
        pair = modeled_pair()
        pair["child"]["attachment_sha256"] = "99" * 32
        cut = modeled_cut(pair)
        result = adapter.assert_qp03_pair_retirement(cut, cut, pair, [attempt(cut, pair)])
        self.assertIn("authentic owner export", result["external_proof_required"][0])
        pair["external_proof_required"] = []
        with self.assertRaises(checks.CutError):
            adapter.assert_qp03_pair_retirement(cut, cut, pair, [attempt(cut, pair)])

    def test_boolean_and_float_aliases_do_not_replace_original_integer_values(self):
        pair = modeled_pair()
        cut = modeled_cut(pair)
        changes = [lambda c, o: c["operations"][0].update(outcome=True),
                   lambda c, o: c["inbox"][0].update(status=True),
                   lambda c, o: c["obligations"][0].update(xp_applied_mask=False),
                   lambda c, o: o["pair"]["child"].update(revision=7.0)]
        for change in changes:
            bad = copy.deepcopy(cut)
            observed = attempt(bad, pair)
            change(bad, observed)
            with self.subTest(change=changes.index(change)), self.assertRaises(checks.CutError):
                adapter.assert_qp03_pair_retirement(bad, bad, pair, [observed])


if __name__ == "__main__":
    unittest.main()
