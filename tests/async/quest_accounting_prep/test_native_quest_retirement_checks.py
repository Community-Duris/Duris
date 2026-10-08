#!/usr/bin/env python3
"""Modeled sensitivity only; no SQL, reset chronology or native D execution."""

import copy
import hashlib
import struct
import unittest

import native_quest_retirement_checks as oracle
import quest_cut_checks as checks
from test_quest_cut_checks import empty_cut, item, root
from test_economic_sql_canonical_audit import native_mobile_image, native_mobile_stock
from test_reconcile_economy_accounting import key, source_identity


A_BIRTH = "46" + "00" * 15
B_BIRTH = "76" + "00" * 15
TERMINAL = "66" * 16
A_CASH = (5, 6, 7, 8)
B_CASH = (3, 2, 1, 4)
A_STOCK = ((81, 16016, -1, 7), (82, 16015, -1, 0), (83, 999, 1, 0))
B_STOCK = ((91, 16016, -1, 7), (92, 16015, -1, 0))


def rehash(data):
    data[132:164] = hashlib.sha256(data[16:132]).digest()
    data[-32:] = hashlib.sha256(data[:-32]).digest()
    return data.hex()


def image(instance, *, retired=False, stock=None, cash=None):
    replacement = instance == 901
    stock = stock if stock is not None else (() if retired else B_STOCK if replacement else A_STOCK)
    forest = bytearray(struct.pack('<I', len(stock)))
    for uid, vnum, parent, slot in stock:
        obj = bytearray(native_mobile_stock()[4:])
        struct.pack_into('<ihQ', obj, 0, parent, slot, uid)
        struct.pack_into('<i', obj, 22, vnum)
        forest.extend(obj)
    data = bytearray(native_mobile_image(identity=instance, state=2 if retired else 1, items=bytes(forest)))
    data[40:56] = bytes.fromhex(B_BIRTH if replacement else A_BIRTH)
    data[56:104] = bytes.fromhex(source_identity(kind=2, identity="77" if replacement else "47", slot=258))
    data[76:92] = data[60:76]  # actual reset source/generation shape
    mobile_rev, stock_rev = (1, 1) if replacement else (3, 4) if retired else (2, 3)
    struct.pack_into('<iiiQQ', data, 104, 16006, 16077, 160, mobile_rev, stock_rev)
    cash = cash if cash is not None else (0,) * 4 if retired else B_CASH if replacement else A_CASH
    cash_rev = 1 if replacement else 5 if retired else 4
    struct.pack_into('<Q4q', data, 180, cash_rev, *cash)
    data[164:180] = bytes.fromhex(B_BIRTH if replacement else TERMINAL if retired else "49" + "00" * 15)
    return dict(mobile_instance_id=instance, mobile_revision=mobile_rev, stock_revision=stock_rev,
                lifetime_state=2 if retired else 1, canonical_image=rehash(data))


def holdings(instance, stock):
    result = []
    for idx, (uid, vnum, parent, slot) in enumerate(stock):
        entry = dict(item(uid, vnum, owner=12), owner_id=instance, equipment_slot=slot)
        if parent >= 0:
            entry.update(root_item_uid=stock[parent][0], parent_item_uid=stock[parent][0])
        result.append(entry)
    return result


def birth(cut, instance, *, historical=False):
    observed = checks.mobile(cut, instance)
    root(cut, observed["birth"], reason=6)
    cut["operations"][-1].update(source_event=observed["source"], item_event_count=0, child_count=0)
    if historical:
        cut["operations"][-1]["epoch"] = "88" * 16
    cut["source_claims"][-1]["source_event"] = observed["source"]
    cut["birth_origins"].append(dict(mobile_instance_id=instance, birth_operation=observed["birth"],
                                     publication_revision=1, canonical_origin="aa"))


def cash_rows(cut, op, wallet, old, new, revisions, *, issuance=False):
    counterpart = cut["meta"]["lineage"] + key(7 if issuance else 8, 123, 0)[32:]
    for idx, account, start, finish, revs in ((0, wallet, old, new, revisions),
                                            (1, counterpart, (0,) * 4, (0,) * 4, (0, 0))):
        effect = dict(operation_id=op, account_index=idx, account_key=account,
                      before_revision=revs[0], after_revision=revs[1])
        effect.update({"before_" + unit: value for unit, value in zip(oracle.UNITS, start)})
        effect.update({"after_" + unit: value for unit, value in zip(oracle.UNITS, finish)})
        cut["effects"].append(effect)
        delta = tuple((new - old) * (1 if idx == 0 else -1) for old, new in zip(old, new))
        posting = dict(operation_id=op, line_index=idx, event_index=idx, account_index=idx, child_index=0)
        posting.update({"delta_" + unit: value for unit, value in zip(oracle.UNITS, delta)})
        posting["copper_value"] = checks.value(posting, "delta_")
        cut["postings"].append(posting)
    cut["operations"][-1]["posting_count"] = 2


def stock_events(cut, stock, instance, op, *, creation=False):
    for idx, (uid, _, _, _) in enumerate(stock):
        cut["ownership_events"].append(dict(operation_id=op, event_index=idx, item_uid=uid,
            from_owner_type=7 if creation else 12, from_owner_id=0 if creation else instance,
            to_owner_type=12 if creation else 8, to_owner_id=instance if creation else 0,
            item_revision=1 if creation else 2))
        cut["item_references"].append(dict(operation_id=op, line_index=idx, item_uid=uid,
            legacy_operation_id=op, legacy_event_index=idx, before_revision=0 if creation else 1,
            after_revision=1 if creation else 2))
    cut["operations"][-1]["item_event_count"] = len(stock)


def cuts(mapping=7777):
    before = empty_cut("QP03")
    before["meta"]["authority"] = "native"
    before["epochs"] = [dict(lineage=before["meta"]["lineage"], epoch=before["meta"]["epoch"])]
    before["player_affects"] = []
    before["birth_origins"] = []
    before["mobiles"] = [image(900)]
    before["items"] = holdings(900, A_STOCK) + [item(200, 16075)]
    before["history"] = [dict(quest_id=16006, completed=1)]
    birth(before, 900, historical=True)
    wallet = before["meta"]["lineage"] + key(1, mapping, 12)[32:]
    replacement_wallet = before["meta"]["lineage"] + key(1, 8888, 12)[32:]
    terminal = copy.deepcopy(before)
    terminal["mobiles"] = [image(900, retired=True)]
    for entry in terminal["items"][:3]:
        entry.update(owner_type=8, owner_id=0, state=2, item_revision=2)
    root(terminal, TERMINAL, reason=34)
    terminal["operations"][-1].update(item_event_count=3, child_count=0)
    stock_events(terminal, A_STOCK, 900, TERMINAL)
    cash_rows(terminal, TERMINAL, wallet, A_CASH, (0,) * 4, (4, 5))
    born = copy.deepcopy(terminal)
    born["meta"]["mobile_instance_ids"] = [900, 901]
    born["mobiles"].append(image(901))
    born["items"].extend(holdings(901, B_STOCK))
    birth(born, 901)
    stock_events(born, B_STOCK, 901, B_BIRTH, creation=True)
    cash_rows(born, B_BIRTH, replacement_wallet, (0,) * 4, B_CASH, (0, 1), issuance=True)
    args = dict(original_instance=900, replacement_instance=901, original_wallet=wallet,
                replacement_wallet=replacement_wallet, terminal_operation=TERMINAL)
    return [before, terminal, born, copy.deepcopy(born), copy.deepcopy(born)], args


def change_image(cut, instance, offset, fmt, value):
    entry = next(row for row in cut["mobiles"] if row["mobile_instance_id"] == instance)
    data = bytearray.fromhex(entry["canonical_image"])
    struct.pack_into(fmt, data, offset, value)
    entry["canonical_image"] = rehash(data)
    if offset in (116, 124):
        entry["mobile_revision" if offset == 116 else "stock_revision"] = value


class TemporalRetirementTests(unittest.TestCase):
    def reject(self, mutate, *, argument=None):
        stages, args = cuts()
        mutate(stages)
        if argument:
            args.update(argument)
        with self.assertRaises(checks.CutError):
            oracle.assert_temporal_qp03(*stages, **args)

    def test_temporal_stage_agreement_and_historical_epoch_without_mutation(self):
        for mapping in (7777, 900):
            with self.subTest(mapping=mapping):
                stages, args = cuts(mapping)
                original = copy.deepcopy(stages)
                result = oracle.assert_temporal_qp03(*stages, **args)
                self.assertEqual(stages, original)
                self.assertEqual(result["original_residual_uids"], [81, 82, 83])
                self.assertEqual(result["replacement_stock_uids"], [91, 92])
                self.assertEqual(result["stable_intervals"], 2)
                self.assertIn("unsupported", result["epoch_rotation"])

    def test_incomplete_reversed_or_simultaneous_stages_refuse(self):
        stages, args = cuts()
        with self.assertRaises(checks.CutError):
            oracle.assert_temporal_qp03(*stages[:3], **args)
        with self.assertRaises(checks.CutError):
            oracle.assert_temporal_qp03(stages[0], stages[2], stages[1], stages[3], **args)
        self.reject(lambda s: s[0]["meta"].update(mobile_instance_ids=[900, 901]))
        self.reject(lambda s: s[1]["mobiles"].append(copy.deepcopy(s[2]["mobiles"][-1])))
        self.reject(lambda s: s[1]["birth_origins"].append(copy.deepcopy(s[2]["birth_origins"][-1])))
        self.reject(lambda s: s[1]["operations"].append(copy.deepcopy(s[2]["operations"][-1])))
        self.reject(lambda s: s[1]["items"].append(copy.deepcopy(s[2]["items"][-1])))

    def test_selected_absence_and_labels_do_not_authenticate_chronology(self):
        stages, args = cuts()
        stages[0]["meta"]["label"] = "B globally absent, trustworthy reset chronology"
        stages[2]["meta"]["label"] = "genuinely later B"
        result = oracle.assert_temporal_qp03(*stages, **args)
        self.assertEqual(result["scope"], "captured temporal-QP03 agreement only")
        self.assertIn("selected absence is insufficient", result["external_proof_required"][0])
        self.assertIn("real quest-D owner", result["external_proof_required"][1])
        self.assertIn("pair ACK", result["external_proof_required"][3])

    def test_wrong_identity_candidate_schema_or_stage_ids_refuse(self):
        for arg in (dict(original_instance=True), dict(replacement_instance=900), dict(replacement_instance=902),
                    dict(terminal_operation="00" * 16), dict(original_wallet=key(1, 7777, 0)),
                    dict(replacement_wallet=key(8, 8888, 12))):
            with self.subTest(argument=arg):
                self.reject(lambda s: None, argument=arg)
        for field, value in (("source_commit", "99" * 20), ("binary_sha256", "99" * 32),
                             ("schema_manifest_sha256", "99" * 32), ("lineage", "99" * 16),
                             ("epoch", "99" * 16), ("watched_vnums", [999]),
                             ("watched_item_uids", [91]), ("mobile_instance_ids", [900, 900])):
            with self.subTest(field=field):
                self.reject(lambda s: s[2]["meta"].update({field: value}))
        self.reject(lambda s: s[2]["migrations"].append(dict(version=999)))
        self.reject(lambda s: s[2]["epochs"].clear())
        self.reject(lambda s: s[2]["meta"].update(authority="legacy-no-epoch"))

    def test_missing_explicit_terminal_image_cash_or_clocks_refuse(self):
        self.reject(lambda s: s[1]["mobiles"].clear())
        self.reject(lambda s: s[1]["mobiles"].__setitem__(0, copy.deepcopy(s[0]["mobiles"][0])))
        for offset, fmt, value in ((32, '<Q', 901), (40, '<Q', 999), (60, '<Q', 999), (104, '<i', 19005),
                                   (116, '<Q', 2), (124, '<Q', 3), (180, '<Q', 6), (188, '<q', 1), (164, '<Q', 999)):
            with self.subTest(offset=offset):
                self.reject(lambda s: change_image(s[1], 900, offset, fmt, value))
        self.reject(lambda s: s[1]["mobiles"][0].update(canonical_image=native_mobile_image(version=1, identity=900).hex()))

    def test_reused_or_wrong_replacement_birth_source_and_carrier_refuse(self):
        for offset, fmt, value in ((32, '<Q', 900), (40, '<Q', 0x46), (60, '<Q', 0x47),
                                   (104, '<i', 19005), (108, '<i', 19008), (116, '<Q', 2),
                                   (124, '<Q', 2), (180, '<Q', 2), (164, '<Q', 0x49)):
            with self.subTest(offset=offset):
                self.reject(lambda s: change_image(s[2], 901, offset, fmt, value))
        for table in ("operations", "inbox", "source_claims", "birth_origins"):
            with self.subTest(table=table):
                self.reject(lambda s: s[2][table].pop())
        self.reject(lambda s: s[2]["birth_origins"][-1].update(canonical_origin=""))
        self.reject(lambda s: s[2]["birth_origins"][-1].update(birth_operation=A_BIRTH))
        self.reject(lambda s: s[2]["inbox"][-1].update(committed_at_present=0))

    def test_retained_a_history_cannot_change_or_disappear_across_birth_or_replay(self):
        for stage in (1, 2, 3, 4):
            for table in ("operations", "inbox", "source_claims", "birth_origins"):
                with self.subTest(stage=stage, table=table):
                    self.reject(lambda s: s[stage][table].pop(0))
            self.reject(lambda s: s[stage]["operations"][0].update(epoch="99" * 16))
        for table in ("effects", "postings", "ownership_events", "item_references"):
            with self.subTest(terminal_history=table):
                self.reject(lambda s: s[2][table].pop(0))
        self.reject(lambda s: s[2]["mobiles"][0].update(stock_revision=9))
        self.reject(lambda s: s[2]["items"][0].update(item_revision=9))

    def test_residual_stock_topology_and_exact_events_refuse_damage(self):
        for field, value in (("state", 1), ("owner_type", 12), ("owner_id", 900), ("vnum", 16075),
                             ("item_revision", 1), ("root_item_uid", 83), ("parent_item_uid", None),
                             ("owner_context_id", 99), ("equipment_slot", 7)):
            with self.subTest(field=field):
                self.reject(lambda s: s[1]["items"][2].update({field: value}))
        self.reject(lambda s: s[0]["items"].pop(2))  # foreign-VNUM nested residual must be watched
        self.reject(lambda s: s[1]["items"].pop(2))
        self.reject(lambda s: s[1]["ownership_events"][2].update(from_owner_id=901))
        self.reject(lambda s: s[1]["item_references"][2].update(before_revision=0))
        self.reject(lambda s: s[1]["item_references"][2].update(after_revision=3))
        self.reject(lambda s: s[1]["operations"][-1].update(item_event_count=2))
        self.reject(lambda s: s[1]["operations"][-1].update(child_count=1))

    def test_cash_effect_posting_exact_vectors_and_receipt_sensitivity(self):
        for stage in (1, 2):
            effect_index = 0 if stage == 1 else 2
            for field, value in (("before_revision", 9), ("after_revision", 9), ("after_copper", 9),
                                 ("account_key", key(1, 9999, 12)), ("operation_id", "99" * 16)):
                with self.subTest(stage=stage, field=field):
                    self.reject(lambda s: s[stage]["effects"][effect_index].update({field: value}))
            self.reject(lambda s: s[stage]["postings"][effect_index].update(delta_copper=5, delta_silver=-7))
            self.reject(lambda s: s[stage]["postings"][effect_index].update(account_index=5))
            self.reject(lambda s: s[stage]["effects"].append(copy.deepcopy(s[stage]["effects"][effect_index])))
            self.reject(lambda s: s[stage]["inbox"][-1].update(result_payload=""))
            self.reject(lambda s: s[stage]["source_claims"][-1].update(operation_id="99" * 16))
        self.reject(lambda s: s[1]["postings"][0].update(delta_copper=5, delta_silver=-7))  # scalar-equal damage
        self.reject(lambda s: s[1]["effects"][1].update(account_key=key(1, 8888, 12)))

    def test_borrowed_stock_and_extra_birth_reward_or_charge_refuse(self):
        self.reject(lambda s: s[2]["items"][-1].update(item_uid=82))
        self.reject(lambda s: s[2]["ownership_events"][-1].update(from_owner_type=12, from_owner_id=900))
        self.reject(lambda s: s[2]["items"][-1].update(owner_id=900))
        self.reject(lambda s: s[2]["operations"].append(copy.deepcopy(s[2]["operations"][-1])))
        self.reject(lambda s: s[2]["player"][0].update(exp=1001))
        self.reject(lambda s: s[2]["currency"].append(dict(operation_id=B_BIRTH)))
        self.reject(lambda s: s[2]["items"].append(item(500, 16075)))
        self.reject(lambda s: s[2]["obligations"].append(dict(offering_operation_id=B_BIRTH)))

    def test_offsetting_extra_cash_postings_or_sink_effects_refuse(self):
        def repeated(stages, *, new_accounts):
            cut = stages[1]
            for idx, sign in ((2, 1), (3, -1)):
                posting = copy.deepcopy(cut["postings"][1])
                posting.update(line_index=idx, event_index=idx, account_index=idx if new_accounts else 1,
                               delta_copper=10 * sign, delta_silver=0, delta_gold=0, delta_platinum=0,
                               copper_value=10 * sign)
                cut["postings"].append(posting)
                if new_accounts:
                    effect = copy.deepcopy(cut["effects"][1])
                    effect.update(account_index=idx, account_key=cut["meta"]["lineage"] + key(8, idx + 123)[32:])
                    cut["effects"].append(effect)
            cut["operations"][-1]["posting_count"] = 4
        self.reject(lambda s: repeated(s, new_accounts=False))
        self.reject(lambda s: repeated(s, new_accounts=True))

    def test_stale_and_recovery_cuts_preserve_b_cash_stock_lifetime_birth_and_a_terminal(self):
        for stage in (3, 4):
            for offset, fmt, value in ((40, '<Q', 999), (60, '<Q', 999), (116, '<Q', 2),
                                       (124, '<Q', 2), (188, '<q', 4)):
                with self.subTest(stage=stage, offset=offset):
                    self.reject(lambda s: change_image(s[stage], 901, offset, fmt, value))
            self.reject(lambda s: s[stage]["mobiles"].pop())
            self.reject(lambda s: s[stage]["items"][-1].update(state=2, owner_type=8))
            self.reject(lambda s: s[stage]["items"][0].update(item_revision=3))
            for table in ("operations", "effects", "postings", "ownership_events", "item_references"):
                with self.subTest(stage=stage, repeated=table):
                    self.reject(lambda s: s[stage][table].append(copy.deepcopy(s[stage][table][-1])))
            self.reject(lambda s: s[stage]["player"][0].update(copper=20000))
            self.reject(lambda s: s[stage]["player_affects"].append(dict(affect_type=99)))

    def test_missing_tables_duplicate_rows_and_opaque_bytes_refuse(self):
        for stage in (0, 1, 2, 3, 4):
            for table in ("mobiles", "birth_origins", "operations", "inbox", "postings", "effects", "items", "player_affects"):
                with self.subTest(stage=stage, missing=table):
                    self.reject(lambda s: s[stage].pop(table))
        self.reject(lambda s: s[2]["mobiles"].append(copy.deepcopy(s[2]["mobiles"][-1])))
        self.reject(lambda s: s[1]["birth_origins"][0].update(canonical_origin="zz"))


if __name__ == "__main__":
    unittest.main()
