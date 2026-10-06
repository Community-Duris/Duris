#!/usr/bin/env python3
"""Counterfactual SQL-shaped cuts test the oracle, never native authority."""

import copy
import hashlib
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import struct
import unittest

from case_data import ACCOUNTING_PIN, blocks
import quest_cut_checks as checks
from test_reconcile_economy_accounting import source_identity, key
from test_economic_sql_canonical_audit import native_mobile_image, native_mobile_stock


def empty_cut(case_id):
    cut = {name: [] for name in ("items", "ownership_events", "currency", "history", "mobiles",
                                "operations", "inbox", "postings", "source_claims", "item_references",
                                "obligations", "xp_entitlements", "migrations", "effects")}
    cut["meta"] = dict(case=case_id, pid=42, source_commit=ACCOUNTING_PIN,
                       binary_sha256="ab" * 32, schema_manifest_sha256="cd" * 32,
                       lineage="11" * 16, epoch="22" * 16,
                       mobile_instance_ids=[900], watched_vnums=[])
    cut["player"] = [dict(pid=42, copper=10000, silver=0, gold=0, platinum=0, exp=1000,
                          quest_active=0, quest_started=100, quest_mob_vnum=50, quest_map_bought=0)]
    return cut


def item(uid, vnum, owner=1, revision=1, state=1):
    return dict(item_uid=uid, vnum=vnum, item_revision=revision, root_item_uid=uid,
                parent_item_uid=None, owner_type=owner, owner_id=42 if owner == 1 else 0,
                owner_context_id=0, state=state, equipment_slot=0)


def root(cut, operation_id, *, reason=33, amounts=()):
    source = source_identity(kind=1, identity=operation_id[:2]) if reason != 32 else None
    cut["operations"].append(dict(operation_id=operation_id, lineage=cut["meta"]["lineage"],
        epoch=cut["meta"]["epoch"], outcome=1, result_code=0, source_event=source,
        original_operation_id=None, reason=reason, posting_count=len(amounts)))
    cut["inbox"].append(dict(operation_id=operation_id, status=1, result_code=0, failure_stage=0,
                             committed_at_present=1, result_payload="unit-receipt-not-native-proof"))
    if source:
        cut["source_claims"].append(dict(operation_id=operation_id, lineage=cut["meta"]["lineage"],
                                          source_event=source, outcome=1))
    for index, amount in enumerate(amounts):
        cut["postings"].append(dict(operation_id=operation_id, line_index=index,
            delta_copper=amount, delta_silver=0, delta_gold=0, delta_platinum=0, copper_value=amount))


def event(cut, uid, operation, event_index, before_owner, after_owner, revision):
    cut["ownership_events"].append(dict(operation_id=operation, event_index=event_index,
        item_uid=uid, from_owner_type=before_owner, to_owner_type=after_owner,
        to_owner_id=42 if after_owner == 1 else 29217 if after_owner == 3 else 0, item_revision=revision))
    cut["item_references"].append(dict(operation_id=operation, legacy_operation_id=operation,
        legacy_event_index=event_index, item_uid=uid, before_revision=revision - 1, after_revision=revision))


def complete_cuts(case_id, reward_vnum):
    terms = next(term for term in blocks(case_id) if ("I", reward_vnum) in term["receive"])
    before = empty_cut(case_id)
    input_vnums = [number for kind, number in terms["give"] if kind == "I"]
    reward_vnums = [number for kind, number in terms["receive"] if kind == "I"]
    selected = list(range(100, 100 + len(input_vnums)))
    rewards = list(range(200, 200 + len(reward_vnums)))
    before["items"] = [item(uid, vnum) for uid, vnum in zip(selected, input_vnums)] + [item(150, input_vnums[0])]
    after = copy.deepcopy(before)
    consume, create = "33" * 16, "44" * 16
    root(after, consume, reason=34)
    for index, entry in enumerate(after["items"][:-1]):
        entry.update(owner_type=8, owner_id=0, item_revision=2, state=2)
        event(after, entry["item_uid"], consume, index, 1, 8, 2)
    net = (sum(number for kind, number in terms["receive"] if kind == "C") -
           sum(number for kind, number in terms["give"] if kind == "C"))
    root(after, create, amounts=(net, -net) if net else ())
    for index, (uid, vnum) in enumerate(zip(rewards, reward_vnums)):
        after["items"].append(item(uid, vnum))
        event(after, uid, create, index, 7, 1, 1)
    if net:
        after["player"][0]["copper"] += net
        after["currency"].append(dict(operation_id=create, wallet_delta_copper=net,
            wallet_delta_silver=0, wallet_delta_gold=0, wallet_delta_platinum=0))
    if case_id == "QP06":
        after["player"][0]["exp"] += 100
        after["xp_entitlements"].append(dict(offering_operation_id=consume, recipient_pid=42,
                                              reward_index=0, amount=100, applied=1))
    return before, after, selected, rewards


def unit_mobile(instance, *, retired=False, birth="46", transition="49", source_identity_byte="47"):
    # Reuse maintained modeled wire fixture solely to test the oracle. This never
    # supplies a native birth or touches SQL/journals; no runtime authority claimed.
    image = bytearray(native_mobile_image(state=2 if retired else 1, identity=instance,
                                         items=bytes(4) if retired else native_mobile_stock()))
    image[40:56] = bytes.fromhex(birth + "00" * 15)
    image[60] = int(source_identity_byte, 16)
    struct.pack_into("<ii", image, 104, 16006, 16077)
    if retired:
        struct.pack_into("<QQ", image, 116, 3, 4)
        struct.pack_into("<Q", image, 180, 5)
    image[132:164] = hashlib.sha256(image[16:132]).digest()
    image[164:180] = bytes.fromhex(transition + "00" * 15)
    if not retired:
        struct.pack_into("<i", image, 250, 16016)
    image[-32:] = hashlib.sha256(image[:-32]).digest()
    return dict(mobile_instance_id=instance, mobile_revision=3 if retired else 2,
                stock_revision=4 if retired else 3, lifetime_state=2 if retired else 1,
                canonical_image=image.hex())


def d_cuts():
    before = empty_cut("QP03")
    before["meta"]["mobile_instance_ids"] = [900, 901]
    before["mobiles"] = [unit_mobile(900), unit_mobile(901, birth="56", source_identity_byte="57")]
    # Both modeled forests use UID81; change the replacement's validated UID to82.
    replacement = bytearray.fromhex(before["mobiles"][1]["canonical_image"])
    struct.pack_into("<Q", replacement, 234, 82)
    replacement[-32:] = hashlib.sha256(replacement[:-32]).digest()
    before["mobiles"][1]["canonical_image"] = replacement.hex()
    for entry in before["mobiles"]:
        image = bytes.fromhex(entry["canonical_image"])
        birth = image[40:56].hex()
        root(before, birth, reason=38)
        before["operations"][-1]["source_event"] = image[56:104].hex()
        before["source_claims"][-1]["source_event"] = image[56:104].hex()
    before["items"] = [item(81, 16016, owner=12), item(82, 16016, owner=12)]
    before["items"][0]["owner_id"] = 900
    before["items"][1]["owner_id"] = 901
    after = copy.deepcopy(before)
    after["mobiles"][0] = unit_mobile(900, retired=True, transition="66")
    operation = "66" + "00" * 15
    after["items"][0].update(owner_type=8, owner_id=0, state=2, item_revision=2)
    root(after, operation, reason=34, amounts=(-8765, 8765))
    event(after, 81, operation, 0, 12, 8, 2)
    # This key deliberately differs from the NPC UID, as native mapping requires.
    account = key(1, 7777, 12)
    account = before["meta"]["lineage"] + account[32:]
    after["effects"].append(dict(operation_id=operation, account_index=0, account_key=account,
        before_copper=5, before_silver=6, before_gold=7, before_platinum=8,
        after_copper=0, after_silver=0, after_gold=0, after_platinum=0,
        before_revision=4, after_revision=5))
    after["postings"][0]["account_index"] = 0
    after["postings"][1]["account_index"] = 1
    return before, after, account


class QuestCutTests(unittest.TestCase):
    def test_static_contracts_require_exact_roots_and_row_links(self):
        for case_id, reward in (("QP01", 44192), ("QP02", 19009), ("QP03", 16075),
                                ("QP05", 16048), ("QP05", 16050), ("QP06", 29237)):
            with self.subTest(case=case_id, reward=reward):
                before, after, selected, rewards = complete_cuts(case_id, reward)
                checks.bind(before, after, case_id)
                checks.static_complete(before, after, case_id, selected, rewards, [150], reward,
                                       expected_xp=100 if case_id == "QP06" else None)

    def test_qp01_lookalike_shortage_duplicate_spare_and_extra_reward_refuse(self):
        before, after, selected, rewards = complete_cuts("QP01", 44192)
        changes = (
            lambda b, a: b["items"][1].update(vnum=43703),
            lambda b, a: a["items"][0].update(state=1),
            lambda b, a: a["items"][4].update(item_revision=2),
            lambda b, a: a["items"].append(item(999, 44192)),
            lambda b, a: a["items"][-1].update(item_uid=100),
            lambda b, a: a["item_references"].pop(),
            lambda b, a: a["source_claims"].clear(),
            lambda b, a: a["inbox"][0].update(committed_at_present=0),
        )
        for change in changes:
            b, a = copy.deepcopy(before), copy.deepcopy(after)
            change(b, a)
            with self.subTest(change=changes.index(change)), self.assertRaises(checks.CutError):
                checks.static_complete(b, a, "QP01", selected, rewards, [150], 44192)
        with self.assertRaises(checks.CutError):
            checks.static_complete(before, after, "QP01", selected[:-1], rewards, [150], 44192)
        with self.assertRaises(checks.CutError):
            checks.static_complete(before, after, "QP01", selected + [selected[0]], rewards, [150], 44192)

    def test_original_npc_roots_cannot_borrow_replacement_stock(self):
        before, after, selected, rewards = complete_cuts("QP03", 16075)
        before["meta"]["mobile_instance_ids"] = after["meta"]["mobile_instance_ids"] = [900, 901]
        for entry in before["items"][:-1]:
            entry.update(owner_type=12, owner_id=900)
        checks.static_complete(before, after, "QP03", selected, rewards, [150], 16075, original_mobile=900)
        # Same-VNUM independently born reset stock is not a third quest reward.
        operation = "99" * 16
        root(after, operation, reason=38)
        source = source_identity(kind=10, identity="99")
        after["operations"][-1]["source_event"] = source
        after["source_claims"][-1]["source_event"] = source
        reset_stock = item(999, 16015, owner=12)
        reset_stock["owner_id"] = 901
        after["items"].append(reset_stock)
        event(after, 999, operation, 0, 7, 12, 1)
        after["ownership_events"][-1]["to_owner_id"] = 901
        checks.static_complete(before, after, "QP03", selected, rewards, [150], 16075, original_mobile=900)
        before["items"][0]["owner_id"] = 901
        with self.assertRaises(checks.CutError):
            checks.static_complete(before, after, "QP03", selected, rewards, [150], 16075, original_mobile=900)

    def test_qp02_all_four_costs_are_preserved(self):
        for reward, fee in ((19007, 1000), (19008, 2000), (19009, 0), (19010, 10000)):
            before, after, selected, rewards = complete_cuts("QP02", reward)
            with self.subTest(reward=reward):
                self.assertEqual(checks.value(after["player"][0]) - checks.value(before["player"][0]), -fee)
                checks.static_complete(before, after, "QP02", selected, rewards, [150], reward)
                if fee:
                    after["player"][0]["copper"] += 1
                    with self.assertRaises(checks.CutError):
                        checks.static_complete(before, after, "QP02", selected, rewards, [150], reward)

    def test_qp05_shortage_wrong_kind_and_same_uid_do_not_satisfy_counts(self):
        for reward in (16048, 16050):
            before, after, selected, rewards = complete_cuts("QP05", reward)
            with self.subTest(reward=reward), self.assertRaises(checks.CutError):
                checks.static_complete(before, after, "QP05", selected[:-1], rewards, [150], reward)
            before["items"][0]["root_item_uid"] = 999
            with self.assertRaises(checks.CutError):
                checks.static_complete(before, after, "QP05", selected, rewards, [150], reward)

    def test_qp06_unbalanced_missing_currency_or_duplicate_money_refuse(self):
        before, after, selected, rewards = complete_cuts("QP06", 29237)
        for mutation in (lambda a: a["postings"][-1].update(copper_value=-2999),
                         lambda a: a["currency"].clear(),
                         lambda a: a["currency"].append(copy.deepcopy(a["currency"][0]))):
            a = copy.deepcopy(after)
            mutation(a)
            with self.assertRaises(checks.CutError):
                checks.static_complete(before, a, "QP06", selected, rewards, [150], 29237, expected_xp=100)

    def test_refusal_and_replay_require_no_partial_effect(self):
        before = empty_cut("QP02")
        checks.unchanged(before, copy.deepcopy(before))
        for table, entry in (("history", dict(id=1)), ("items", item(1, 19006)),
                             ("obligations", dict(offering_operation_id="44" * 16))):
            after = copy.deepcopy(before)
            after[table].append(entry)
            with self.subTest(table=table), self.assertRaises(checks.CutError):
                checks.unchanged(before, after)
        before, after, _, _ = complete_cuts("QP06", 29237)
        checks.replay(after, copy.deepcopy(after))
        replayed = copy.deepcopy(after)
        replayed["player"][0]["copper"] += 3000
        with self.assertRaises(checks.CutError):
            checks.replay(after, replayed)

    def test_qp06_frozen_xp_is_not_nominal_and_cannot_repeat(self):
        before, after, selected, rewards = complete_cuts("QP06", 29237)
        with self.assertRaises(checks.CutError):
            checks.static_complete(before, after, "QP06", selected, rewards, [150], 29237)
        for change in (lambda a: a["player"][0].update(exp=1200),
                       lambda a: a["xp_entitlements"][0].update(applied=0),
                       lambda a: a["xp_entitlements"][0].update(amount=2500)):
            invalid = copy.deepcopy(after)
            change(invalid)
            with self.assertRaises(checks.CutError):
                checks.static_complete(before, invalid, "QP06", selected, rewards, [150], 29237, expected_xp=100)
        after["player"][0]["platinum"] = 2**63 - 1
        with self.assertRaises(checks.CutError):
            checks.static_complete(before, after, "QP06", selected, rewards, [150], 29237, expected_xp=100)

    def test_later_move_and_second_boot_preserve_original_reward(self):
        _, before, _, rewards = complete_cuts("QP06", 29237)
        after = copy.deepcopy(before)
        after["items"][-1].update(owner_type=3, owner_id=29217, item_revision=2)
        operation = "55" * 16
        root(after, operation, reason=32)
        event(after, rewards[0], operation, 0, 1, 3, 2)
        checks.later_move(before, after, rewards[0])
        checks.replay(after, copy.deepcopy(after))
        reset = copy.deepcopy(after)
        reset["items"][-1].update(owner_type=1, owner_id=42, item_revision=3)
        with self.assertRaises(checks.CutError):
            checks.replay(after, reset)

    def test_dynamic_refund_needs_exact_debit_and_once_only_restitution(self):
        for case_id, fee in (("QP04", 220), ("QP07", 110), ("QP07", 1331)):
            before = empty_cut(case_id)
            before["player"][0].update(quest_active=1, quest_started=200, quest_mob_vnum=60)
            after = copy.deepcopy(before)
            for operation, amount in (("66" * 16, -fee), ("77" * 16, fee)):
                root(after, operation, reason=20, amounts=(amount, -amount))
                after["currency"].append(dict(operation_id=operation, wallet_delta_copper=amount,
                    wallet_delta_silver=0, wallet_delta_gold=0, wallet_delta_platinum=0))
            after["operations"][-1]["original_operation_id"] = "66" * 16
            checks.refunded(before, after, fee)
            for change in (lambda a: a["player"][0].update(quest_map_bought=1),
                           lambda a: a["history"].append(dict(id=1)),
                           lambda a: a["currency"].pop(),
                           lambda a: a["operations"][-1].update(original_operation_id="99" * 16)):
                invalid = copy.deepcopy(after)
                change(invalid)
                with self.subTest(case=case_id, fee=fee), self.assertRaises(checks.CutError):
                    checks.refunded(before, invalid, fee)

    def test_original_d_requires_stock_cash_birth_and_untouched_replacement(self):
        before, after, account = d_cuts()
        checks.retired(before, after, 900, 901, account)
        mutations = (
            lambda a: a["mobiles"].pop(0),
            lambda a: a["mobiles"][0].update(canonical_image=before["mobiles"][0]["canonical_image"]),
            lambda a: a["mobiles"][1].update(stock_revision=4),
            lambda a: a["items"][0].update(state=1),
            lambda a: a["items"][1].update(item_revision=2),
            lambda a: a["operations"].pop(0),
            lambda a: a["effects"].clear(),
            lambda a: a["effects"][0].update(before_copper=4),
            lambda a: a["postings"][0].update(copper_value=-1),
            lambda a: a["player"][0].update(exp=1001),
        )
        for change in mutations:
            invalid = copy.deepcopy(after)
            change(invalid)
            with self.subTest(change=mutations.index(change)), self.assertRaises((checks.CutError, ValueError)):
                checks.retired(before, invalid, 900, 901, account)
        # Historical v1 is explicitly unknown cash, never an inferred zero.
        invalid = copy.deepcopy(before)
        old = native_mobile_image(version=1, identity=900)
        invalid["mobiles"][0]["canonical_image"] = old.hex()
        with self.assertRaises(checks.CutError):
            checks.mobile(invalid, 900)

    def test_started_unreturned_holds_original_without_authorizing_replay(self):
        cut = empty_cut("QP03")
        operation = "88" * 16
        root(cut, operation)
        cut["obligations"].append(dict(offering_operation_id=operation, acknowledged=0, continuation="unit-only"))
        checks.held(cut, copy.deepcopy(cut), operation)
        for change in (lambda a: a["obligations"].clear(),
                       lambda a: a["obligations"][0].update(acknowledged=1),
                       lambda a: a["player"][0].update(exp=1001),
                       lambda a: a["inbox"].clear()):
            invalid = copy.deepcopy(cut)
            change(invalid)
            with self.assertRaises(checks.CutError):
                checks.held(cut, invalid, operation)

    def test_collector_refuses_unsafe_target_before_binary_or_sql_access(self):
        # Actual CLI refusal only; no mocked successful SQL, epoch or activation.
        with tempfile.TemporaryDirectory(prefix="quest-prep-guard-") as directory:
            output = Path(directory) / "must-not-exist.json"
            command = [sys.executable, str(Path(__file__).with_name("capture_quest_cut.py")),
                       "--case", "QP03", "--database", "quest_journey_test_" + "a" * 12,
                       "--pid", "42", "--lineage", "11" * 16, "--epoch", "22" * 16,
                       "--server", str(Path(directory) / "missing-binary"),
                       "--server-sha256", "ab" * 32, "--source-commit", ACCOUNTING_PIN,
                       "--output", str(output)]
            for disposable, host, database in (("0", "127.0.0.1", command[5]),
                                                ("1", "remote.invalid", command[5]),
                                                ("1", "127.0.0.1", "production")):
                arguments = command.copy()
                arguments[5] = database
                result = subprocess.run(arguments, env=dict(os.environ, TEST_DB_DISPOSABLE=disposable,
                    TEST_DB_HOST=host), stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, timeout=10)
                self.assertEqual(result.returncode, 1)
                self.assertIn("refused", result.stderr)
                self.assertNotIn("missing-binary", result.stderr)
                self.assertFalse(output.exists())

    def test_historical_ack_never_follows_from_absent_frames_or_actor(self):
        cut = empty_cut("QP06")
        operation = "88" * 16
        with self.assertRaises(checks.CutError):
            checks.acknowledged(cut, operation)
        root(cut, operation)
        cut["obligations"].append(dict(offering_operation_id=operation, acknowledged=1, continuation="unit-only"))
        cut["xp_entitlements"].append(dict(offering_operation_id=operation, recipient_pid=42,
                                            reward_index=0, amount=100, applied=1))
        self.assertEqual(checks.acknowledged(cut, operation), 100)
        cut["xp_entitlements"][0]["applied"] = 0
        with self.assertRaises(checks.CutError):
            checks.acknowledged(cut, operation)


if __name__ == "__main__":
    unittest.main()
