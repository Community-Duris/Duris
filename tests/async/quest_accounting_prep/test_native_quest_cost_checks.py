#!/usr/bin/env python3
"""Modeled oracle sensitivity only; no SQL/native/mapping/projection authority."""

import copy
import hashlib
import struct
import unittest

import native_quest_cost_checks as oracle
import quest_cut_checks as checks
from test_quest_cut_checks import empty_cut, item, root
from test_economic_sql_canonical_audit import native_mobile_image, native_mobile_stock
from test_reconcile_economy_accounting import key, source_identity


OP = "66" * 16
BIRTH = "46" + "00" * 15


def image(instance, cash, *, charged=False, uids=()):
    # Reuse maintained modeled wire fixture; these are not native observations.
    forest = bytearray(struct.pack('<I', len(uids)))
    for uid in uids:
        obj = bytearray(native_mobile_stock()[4:])
        struct.pack_into('<ihQ', obj, 0, -1, 0, uid)
        struct.pack_into('<i', obj, 22, 19006 if uid != 150 else 19010)
        forest.extend(obj)
    data = bytearray(native_mobile_image(identity=instance, items=bytes(forest)))
    struct.pack_into('<H', data, 56, 2)  # modeled npc_generation
    data[76:92] = data[60:76]  # real reset source shape: one original invocation
    struct.pack_into('<Q', data, 92, 0)
    struct.pack_into('<iiiQQ', data, 104, 19005, 19008, 190, 3 if charged else 2, 4 if charged else 3)
    struct.pack_into('<Q4q', data, 180, 5 if charged else 4, *cash)
    if charged:
        data[164:180] = bytes.fromhex(OP)
    data[132:164] = hashlib.sha256(data[16:132]).digest()
    data[-32:] = hashlib.sha256(data[:-32]).digest()
    return dict(mobile_instance_id=instance, mobile_revision=3 if charged else 2,
                stock_revision=4 if charged else 3, lifetime_state=1, canonical_image=data.hex())


def cuts(reward=19010, mapping=7777):
    fee, quantity, slot = {19010: (10000, 4, 0), 19008: (2000, 2, 2), 19007: (1000, 1, 3)}[reward]
    before = empty_cut("QP02")
    before["meta"]["authority"] = "native"
    before["epochs"] = [dict(lineage=before["meta"]["lineage"], epoch=before["meta"]["epoch"])]
    before["player_affects"] = []
    uids = list(range(101, 101 + quantity))
    before["items"] = [dict(item(uid, 19006, owner=12), owner_id=900) for uid in uids]
    before["items"].append(dict(item(150, 19010, owner=12), owner_id=900))
    cash = (5, 6, 7, 18)
    end_cash = (5, 6, 7, 18 - fee // 1000)  # explicit modeled example, not projector implementation
    before["mobiles"] = [image(900, cash, uids=uids + [150])]
    birth_source = checks.mobile(before, 900)["source"]
    root(before, BIRTH, reason=6)
    before["operations"][0].update(source_event=birth_source, epoch="77" * 16)
    before["source_claims"][0]["source_event"] = birth_source
    before["birth_origins"] = [dict(mobile_instance_id=900, birth_operation=BIRTH,
                                    publication_revision=1, canonical_origin="aa")]
    after = copy.deepcopy(before)
    after["mobiles"] = [image(900, end_cash, charged=True, uids=[150])]
    root(after, OP, reason=44)
    source = bytearray.fromhex(source_identity(kind=19, sequence=3, slot=slot))
    source[4:20] = bytes.fromhex(BIRTH)
    source[20:36] = bytes.fromhex(oracle.decode_source_event(birth_source)[2])
    after["operations"][-1].update(source_event=source.hex(), writer_id=oracle.WRITER,
        policy_version=oracle.POLICY, compiler_version=1, posting_count=2, item_event_count=quantity, child_count=0)
    after["source_claims"][-1]["source_event"] = source.hex()
    wallet = before["meta"]["lineage"] + key(1, mapping, 12)[32:]
    sink = before["meta"]["lineage"] + key(8, oracle.SINK_ID, oracle.SINK_CONTEXT)[32:]
    for idx, account, old, new, rev in ((0, wallet, cash, end_cash, (4, 5)),
                                      (1, sink, (0,) * 4, (0,) * 4, (0, 0))):
        effect = dict(operation_id=OP, account_index=idx, account_key=account,
                      before_revision=rev[0], after_revision=rev[1])
        effect.update({"before_" + unit: value for unit, value in zip(oracle.UNITS, old)})
        effect.update({"after_" + unit: value for unit, value in zip(oracle.UNITS, new)})
        after["effects"].append(effect)
        delta = tuple((new - old) * (1 if idx == 0 else 0) for old, new in zip(cash, end_cash))
        if idx == 1:
            delta = tuple(old - new for old, new in zip(cash, end_cash))
        posting = dict(operation_id=OP, line_index=idx, event_index=idx, account_index=idx, child_index=0,
                       copper_value=(-fee if idx == 0 else fee))
        posting.update({"delta_" + unit: value for unit, value in zip(oracle.UNITS, delta)})
        after["postings"].append(posting)
    for idx, uid in enumerate(uids):
        after["items"][idx].update(owner_type=8, owner_id=0, state=2, item_revision=2)
        after["ownership_events"].append(dict(operation_id=OP, event_index=idx, item_uid=uid,
            from_owner_type=12, from_owner_id=900, to_owner_type=8, to_owner_id=0, item_revision=2))
        after["item_references"].append(dict(operation_id=OP, line_index=idx, item_uid=uid,
            legacy_operation_id=OP, legacy_event_index=idx, before_revision=1, after_revision=2))
    after["obligations"].append(dict(offering_operation_id=OP, player_pid=42, continuation="aa",
                                     xp_applied_mask=0, acknowledged=0))
    args = dict(original_instance=900, native_wallet=wallet, cost_operation=OP, reward_vnum=reward, fee=fee)
    return before, after, args


class NativeQuestCostTests(unittest.TestCase):
    def test_paid_contracts_and_equal_numeric_mapping_instance_are_allowed(self):
        for reward in (19010, 19008, 19007):
            for mapping in (7777, 900):
                with self.subTest(reward=reward, mapping=mapping):
                    before, after, args = cuts(reward, mapping)
                    original = copy.deepcopy((before, after))
                    result = oracle.assert_paid_qp02(before, after, **args)
                    self.assertEqual(result["fee"], args["fee"])
                    self.assertIn("required", result["mapping_projection_publication_authentication"])
                    self.assertEqual((before, after), original)

    def reject(self, mutate, *, argument=None):
        before, after, args = cuts()
        mutate(before, after)
        if argument:
            args.update(argument)
        with self.assertRaises(checks.CutError):
            oracle.assert_paid_qp02(before, after, **args)

    def test_wrong_arguments_and_missing_bindings_refuse(self):
        for change in (dict(original_instance=901), dict(original_instance=True), dict(cost_operation="00" * 16),
                       dict(fee=9999), dict(fee=True), dict(reward_vnum=19009), dict(reward_vnum=16075),
                       dict(native_wallet=key(1, 7777, 0)), dict(native_wallet=key(8, 7777, 12))):
            with self.subTest(change=change):
                self.reject(lambda b, a: None, argument=change)
        for table in ("epochs", "birth_origins", "effects", "operations", "inbox", "postings", "player"):
            with self.subTest(missing=table):
                self.reject(lambda b, a: a.pop(table))
        self.reject(lambda b, a: a["meta"].update(source_commit="99" * 20))
        self.reject(lambda b, a: b["meta"].update(authority="legacy-no-epoch"))

    def test_operation_policy_source_claim_and_receipt_sensitivity(self):
        for field, value in (("reason", 34), ("writer_id", 4), ("policy_version", 2),
                             ("compiler_version", 2), ("operation_id", "88" * 16), ("posting_count", 4)):
            with self.subTest(field=field):
                self.reject(lambda b, a: a["operations"][-1].update({field: value}))
        for offset in (0, 4, 20, 36, 44):
            def damage(b, a):
                source = bytearray.fromhex(a["operations"][-1]["source_event"])
                source[offset] ^= 1
                a["operations"][-1]["source_event"] = source.hex()
                a["source_claims"][-1]["source_event"] = source.hex()
            with self.subTest(source_offset=offset):
                self.reject(damage)
        for table in ("operations", "inbox", "source_claims"):
            with self.subTest(duplicate=table):
                self.reject(lambda b, a: a[table].append(copy.deepcopy(a[table][-1])))
        self.reject(lambda b, a: a["inbox"][-1].update(committed_at_present=0))
        self.reject(lambda b, a: a["inbox"][-1].update(result_payload=""))
        self.reject(lambda b, a: a["source_claims"][-1].update(operation_id="88" * 16))

    def test_cash_revision_identity_and_current_v2_sensitivity(self):
        for offset, fmt, value in ((32, '<Q', 901), (40, '<Q', 999), (104, '<i', 16006),
                                   (116, '<Q', 4), (124, '<Q', 5), (180, '<Q', 6),
                                   (188, '<q', 15), (164, '<Q', 999)):
            def damage(b, a):
                data = bytearray.fromhex(a["mobiles"][0]["canonical_image"])
                struct.pack_into(fmt, data, offset, value)
                data[132:164] = hashlib.sha256(data[16:132]).digest()
                data[-32:] = hashlib.sha256(data[:-32]).digest()
                a["mobiles"][0]["canonical_image"] = data.hex()
                # Match row clocks so a wrong one-step transition reaches the oracle.
                if offset in (116, 124):
                    a["mobiles"][0]["mobile_revision" if offset == 116 else "stock_revision"] = value
            with self.subTest(offset=offset):
                self.reject(damage)
        self.reject(lambda b, a: a["mobiles"][0].update(canonical_image=native_mobile_image(version=1, identity=900).hex()))
        self.reject(lambda b, a: a["mobiles"].append(copy.deepcopy(a["mobiles"][0])))
        self.reject(lambda b, a: a["birth_origins"][0].update(canonical_origin=""))
        self.reject(lambda b, a: a["operations"][0].update(source_event=a["operations"][-1]["source_event"]))

    def test_wallet_sink_effects_and_scalar_equal_denomination_damage_refuse(self):
        for idx, field, value in ((0, "account_key", key(1, 7778, 12)), (0, "before_revision", 3),
                                  (0, "after_revision", 6), (0, "before_copper", 6),
                                  (1, "account_key", key(8, 45)), (1, "before_revision", 1),
                                  (1, "after_copper", 1), (0, "operation_id", "88" * 16)):
            with self.subTest(idx=idx, field=field):
                self.reject(lambda b, a: a["effects"][idx].update({field: value}))
        self.reject(lambda b, a: a["effects"].append(copy.deepcopy(a["effects"][0])))
        self.reject(lambda b, a: a["effects"][0].update(after_copper=15, after_silver=5))
        self.reject(lambda b, a: a["postings"][0].update(delta_copper=10, delta_silver=-1))
        for field, value in (("account_index", 1), ("line_index", 2), ("event_index", 1), ("child_index", 1),
                             ("copper_value", -9999), ("operation_id", "88" * 16)):
            with self.subTest(posting=field):
                self.reject(lambda b, a: a["postings"][0].update({field: value}))
        self.reject(lambda b, a: a["postings"][1].update(delta_platinum=9, copper_value=9000))

    def test_offsetting_duplicate_charge_and_player_or_reward_effects_refuse(self):
        def offsets(b, a):
            for idx, factor in ((2, 1), (3, -1)):
                entry = copy.deepcopy(a["postings"][0])
                entry.update(line_index=idx, event_index=idx, delta_platinum=10 * factor, copper_value=10000 * factor)
                a["postings"].append(entry)
            a["operations"][-1]["posting_count"] = 4
        self.reject(offsets)
        self.reject(lambda b, a: a["player"][0].update(copper=0))
        self.reject(lambda b, a: a["player"][0].update(exp=1001))
        self.reject(lambda b, a: a["player"][0].update(quest_active=1))
        self.reject(lambda b, a: a["currency"].append(dict(operation_id=OP)))
        self.reject(lambda b, a: a["items"].append(item(500, 19010)))
        self.reject(lambda b, a: a["obligations"][-1].update(acknowledged=1))
        self.reject(lambda b, a: a["items"][-1].update(owner_type=8))
        self.reject(lambda b, a: a["ownership_events"][0].update(from_owner_id=901))
        self.reject(lambda b, a: a["item_references"][0].update(after_revision=3))
        self.reject(lambda b, a: a["item_references"][0].update(before_revision=0))
        self.reject(lambda b, a: a["operations"][-1].update(item_event_count=3))
        self.reject(lambda b, a: a["operations"][-1].update(child_count=1))

    def test_historical_birth_can_be_older_epoch_but_history_must_survive(self):
        before, after, args = cuts()
        self.assertNotEqual(before["operations"][0]["epoch"], before["meta"]["epoch"])
        oracle.assert_paid_qp02(before, after, **args)
        self.reject(lambda b, a: a["operations"][0].update(epoch="88" * 16))
        self.reject(lambda b, a: a["source_claims"].pop(0))

    def test_explicit_modeled_denomination_change_can_agree(self):
        before, after, args = cuts()
        before["mobiles"][0] = image(900, (9999, 0, 0, 1), uids=[101, 102, 103, 104, 150])
        after["mobiles"][0] = image(900, (9, 9, 9, 0), charged=True, uids=[150])
        for prefix, cash in (("before_", (9999, 0, 0, 1)), ("after_", (9, 9, 9, 0))):
            after["effects"][0].update({prefix + unit: value for unit, value in zip(oracle.UNITS, cash)})
        delta = (-9990, 9, 9, -1)
        for idx, posting in enumerate(after["postings"]):
            posting.update({"delta_" + unit: value * (1 if idx == 0 else -1)
                            for unit, value in zip(oracle.UNITS, delta)})
        self.assertEqual(oracle.assert_paid_qp02(before, after, **args)["wallet_delta"], delta)

    def test_consistent_alternate_vectors_are_not_frozen_projection_authentication(self):
        before, after, args = cuts()
        # Deliberately scalar-equivalent but not a claim about native spend().
        after["mobiles"][0] = image(900, (8765, 0, 0, 0), charged=True, uids=[150])
        effect = after["effects"][0]
        effect.update(after_copper=8765, after_silver=0, after_gold=0, after_platinum=0)
        delta = (8760, -6, -7, -18)
        for idx, posting in enumerate(after["postings"]):
            posting.update({"delta_" + unit: value * (1 if idx == 0 else -1)
                            for unit, value in zip(oracle.UNITS, delta)})
        result = oracle.assert_paid_qp02(before, after, **args)
        self.assertEqual(result["scope"], "captured paid-QP02 agreement only")
        self.assertIn("required", result["mapping_projection_publication_authentication"])


if __name__ == "__main__":
    unittest.main()
