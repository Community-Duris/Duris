#!/usr/bin/env python3
"""Modeled optional-join sensitivity only; no SQL/native/owner authentication."""

import copy
import struct
import unittest

import native_wallet_mapping_checks as oracle
import quest_cut_checks as checks
import native_quest_cost_checks as paid_oracle
import native_quest_retirement_checks as temporal_oracle
from case_data import ROOT
from reconcile_economy_accounting import account_key
from test_capture_quest_cut import mapping
from test_native_quest_cost_checks import cuts as paid_cuts
from test_native_quest_retirement_checks import cuts as temporal_cuts, rehash
from test_economic_sql_canonical_audit import native_mobile_image
from test_reconcile_economy_accounting import key


def observe(cut, selections):
    """Add modeled literal optional-reader rows to existing maintained cut builders."""
    rows = []
    for instance, wallet in selections:
        lineage, _, mapping_id, _ = account_key(wallet)
        native = checks.mobile(cut, instance)
        row = mapping(mapping_id, native_id=instance, lineage=lineage, locator=7,
                      active=native["row"]["lifetime_state"] == 1)
        row["creating_operation_id"] = native["birth"]
        row["lineage"] = lineage
        row["retiring_operation_id"] = None if row["active_native_id"] else native["transition"]
        rows.append(row)
    cut["account_mappings"] = sorted(rows, key=lambda row: row["mapping_id"])
    cut["lineage_head"] = [dict(lineage=cut["meta"]["lineage"], active_epoch=cut["meta"]["epoch"], revision=9)]
    cut["meta"].update(watched_mapping_ids=sorted(row["mapping_id"] for row in rows),
                       missing_mapping_ids=[], mapping_observation_authenticated=False)
    return cut


def modeled(mapping_id=7777):
    before, _, args = paid_cuts(mapping=mapping_id)
    return observe(before, [(900, args["native_wallet"])]), dict(original_instance=900, native_wallet=args["native_wallet"])


def change_image(cut, offset, fmt, value):
    row = cut["mobiles"][0]
    image = bytearray.fromhex(row["canonical_image"])
    struct.pack_into(fmt, image, offset, value)
    row["canonical_image"] = rehash(image)


class LiveWalletMappingTests(unittest.TestCase):
    def reject(self, mutate, *, argument=None):
        cut, args = modeled()
        mutate(cut)
        if argument:
            args.update(argument)
        unchanged = copy.deepcopy(cut)
        with self.assertRaises(checks.CutError):
            oracle.assert_live_native_wallet(cut, **args)
        self.assertEqual(cut, unchanged)

    def test_public_native_wallet_constants_and_schema_are_distinct_from_generic_fixture(self):
        source = (ROOT / "src/persistence/economic_sql_native_mobile_birth_transaction.h").read_text()
        self.assertRegex(source, r"ECONOMIC_NATIVE_MOBILE_WALLET_LOCATOR = 7;")
        self.assertEqual((oracle.WALLET_KIND, oracle.WALLET_CONTEXT, oracle.SQL_BACKEND, oracle.WALLET_LOCATOR),
                         (1, 12, 1, 7))
        self.assertEqual(mapping(1)["locator_kind"], 12)  # generic cursor fixture is intentionally not native policy

    def test_additive_paid_qp02_before_after_and_numeric_id_independence(self):
        for reward in (19007, 19008, 19010):
            for mapping_id in (7777, 900):
                with self.subTest(reward=reward, mapping_id=mapping_id):
                    before, after, args = paid_cuts(reward, mapping_id)
                    original_result = paid_oracle.assert_paid_qp02(before, after, **args)
                    for cut in (before, after):
                        observe(cut, [(900, args["native_wallet"])])
                        unchanged = copy.deepcopy(cut)
                        result = oracle.assert_live_native_wallet(cut, original_instance=900, native_wallet=args["native_wallet"])
                        self.assertEqual(cut, unchanged)
                        self.assertEqual(result["mapping_id"], mapping_id)
                        self.assertEqual(result["native_instance"], 900)
                        self.assertEqual(result["cash"], checks.mobile(cut, 900)["cash"])
                        self.assertEqual(result["cash_revision"], checks.mobile(cut, 900)["cash_revision"])
                        self.assertEqual(result["historical_birth_epoch"], "77" * 16)
                        self.assertEqual(result["current_epoch"], cut["meta"]["epoch"])
                    self.assertEqual(paid_oracle.assert_paid_qp02(before, after, **args), original_result)

    def test_additive_qp03_pre_D_original_A_and_live_B_keep_retired_row_literal(self):
        stages, args = temporal_cuts()
        original_result = temporal_oracle.assert_temporal_qp03(*stages, **args)
        for stage, cut in enumerate(stages):
            selections = [(900, args["original_wallet"])]
            if stage >= 2:
                selections.append((901, args["replacement_wallet"]))
            observe(cut, selections)
        original = oracle.assert_live_native_wallet(stages[0], original_instance=900, native_wallet=args["original_wallet"])
        replacement = oracle.assert_live_native_wallet(stages[2], original_instance=901, native_wallet=args["replacement_wallet"])
        self.assertEqual(original["historical_birth_epoch"], "88" * 16)
        self.assertEqual(replacement["historical_birth_epoch"], stages[2]["meta"]["epoch"])
        self.assertNotEqual(original["birth_operation"], replacement["birth_operation"])
        unchanged = copy.deepcopy(stages)
        with self.assertRaises(checks.CutError):
            oracle.assert_live_native_wallet(stages[2], original_instance=900, native_wallet=args["original_wallet"])
        self.assertEqual(stages, unchanged)
        self.assertIsNone(stages[2]["account_mappings"][0]["active_native_id"])
        self.assertEqual(stages[2]["account_mappings"][0]["revision"], 1)
        self.assertEqual(temporal_oracle.assert_temporal_qp03(*stages, **args), original_result)

    def test_explicit_key_mutations_refuse_instead_of_falling_back_to_native_id(self):
        cut, args = modeled()
        lineage = cut["meta"]["lineage"]
        for wallet in (None, 7777, "", "00" * 40, args["native_wallet"][:-2],
                       "99" * 16 + args["native_wallet"][32:], lineage + key(2, 7777, 12)[32:],
                       lineage + key(1, 7777, 0)[32:], lineage + key(1, 900, 12)[32:]):
            with self.subTest(wallet=wallet):
                self.reject(lambda cut: None, argument=dict(native_wallet=wallet))
        raw = bytearray.fromhex(args["native_wallet"])
        for offset, value in ((16, 2), (20, 0), (36, 1)):
            changed = bytearray(raw)
            if offset == 20:
                changed[20:28] = bytes(8)
            else:
                changed[offset] = value
            self.reject(lambda cut: None, argument=dict(native_wallet=changed.hex()))

    def test_explicit_instance_type_range_and_mobile_selection_refusals(self):
        for instance in (None, True, 900.0, "900", 0, -1, 901, 2**64 - 1, 2**64):
            with self.subTest(instance=instance):
                self.reject(lambda cut: None, argument=dict(original_instance=instance))
        for ids in ([], [900, 900], [True], [900.0], [900, 2**64 - 1], "900"):
            with self.subTest(ids=ids):
                self.reject(lambda cut: cut["meta"].update(mobile_instance_ids=ids))

    def test_mapping_selection_presence_duplicates_and_observation_labels(self):
        mutations = (
            lambda cut: cut.pop("account_mappings"),
            lambda cut: cut.update(account_mappings=[]),
            lambda cut: cut["account_mappings"].append(copy.deepcopy(cut["account_mappings"][0])),
            lambda cut: cut["meta"].pop("watched_mapping_ids"),
            lambda cut: cut["meta"].update(watched_mapping_ids=[]),
            lambda cut: cut["meta"].update(watched_mapping_ids=[7777, 7777]),
            lambda cut: cut["meta"].update(watched_mapping_ids=[7778, 7777]),
            lambda cut: cut["meta"].update(watched_mapping_ids=[True]),
            lambda cut: cut["meta"].update(watched_mapping_ids=[2**64]),
            lambda cut: cut["meta"].update(watched_mapping_ids=list(range(1, oracle.MAX_ROWS + 2))),
            lambda cut: cut["meta"].update(missing_mapping_ids=[7777]),
            lambda cut: cut["meta"].update(missing_mapping_ids=[8888]),
            lambda cut: cut["meta"].update(missing_mapping_ids=[0]),
            lambda cut: cut["meta"].pop("mapping_observation_authenticated"),
            lambda cut: cut["meta"].update(mapping_observation_authenticated=True),
            lambda cut: cut["meta"].update(mapping_observation_authenticated=0),
            lambda cut: cut["meta"].update(authority="legacy-no-epoch"),
        )
        for index, mutate in enumerate(mutations):
            with self.subTest(index=index):
                self.reject(mutate)

    def test_unrelated_missing_and_foreign_mapping_observations_do_not_invent_a_census(self):
        cut, args = modeled()
        other = copy.deepcopy(cut["account_mappings"][0])
        other.update(mapping_id=8888, lineage="99" * 16, backend_kind=2, locator_kind=123, active_native_id=None,
                     retiring_operation_id="aa" * 16, revision=7)
        cut["account_mappings"].append(other)
        cut["meta"].update(watched_mapping_ids=[7777, 8888, 9999], missing_mapping_ids=[9999])
        unchanged = copy.deepcopy(cut)
        result = oracle.assert_live_native_wallet(cut, **args)
        self.assertFalse(result["owner_authenticated"])
        self.assertEqual(cut, unchanged)

    def test_each_selected_mapping_lifetime_field_is_checked_independently(self):
        fields = dict(mapping_id=7778, lineage="99" * 16, account_kind=2, context_id=0, backend_kind=2,
                      locator_kind=12, native_id=901, active_native_id=901, creating_operation_id="99" * 16,
                      retiring_operation_id="99" * 16, revision=1)
        for field, value in fields.items():
            with self.subTest(field=field):
                self.reject(lambda cut: cut["account_mappings"][0].update({field: value}))
        self.reject(lambda cut: cut["account_mappings"][0].update(active_native_id=None))
        self.reject(lambda cut: cut["account_mappings"][0].pop("retiring_operation_id"))

    def test_selected_mapping_integer_fields_reject_bool_float_string_and_overflow(self):
        for field in ("mapping_id", "account_kind", "context_id", "backend_kind", "locator_kind",
                      "native_id", "active_native_id", "revision"):
            for value in (True, False, 0.0, "0", None, -1, 2**64):
                with self.subTest(field=field, value=value):
                    self.reject(lambda cut: cut["account_mappings"][0].update({field: value}))

    def test_head_current_epoch_and_lineage_binding_refuse_independent_changes(self):
        for head in (None, [], [dict(lineage="11" * 16, active_epoch=None, revision=9)],
                     [dict(lineage="11" * 16, active_epoch="33" * 16, revision=9)],
                     [dict(lineage="99" * 16, active_epoch="22" * 16, revision=9)]):
            with self.subTest(head=head):
                self.reject(lambda cut: cut.update(lineage_head=head))
        self.reject(lambda cut: cut["lineage_head"].append(copy.deepcopy(cut["lineage_head"][0])))
        self.reject(lambda cut: cut.update(epochs=[]))
        self.reject(lambda cut: cut["epochs"][0].update(epoch="33" * 16))
        self.reject(lambda cut: cut["meta"].update(epoch="33" * 16))
        self.reject(lambda cut: cut["meta"].update(lineage="99" * 16))
        for revision in (None, True, 9.0, "9", -1, 2**64):
            with self.subTest(revision=revision):
                self.reject(lambda cut: cut["lineage_head"][0].update(revision=revision))
        for revision in (0, 2**64 - 1):
            cut, args = modeled()
            cut["lineage_head"][0]["revision"] = revision
            self.assertEqual(oracle.assert_live_native_wallet(cut, **args)["lineage_revision"], revision)

    def test_older_or_current_birth_epoch_and_original_root_carrier_links(self):
        for epoch in ("77" * 16, "22" * 16):
            cut, args = modeled()
            cut["operations"][0]["epoch"] = epoch
            self.assertEqual(oracle.assert_live_native_wallet(cut, **args)["historical_birth_epoch"], epoch)
        for field, value in (("lineage", "99" * 16), ("source_event", "99" * 48),
                             ("epoch", None), ("epoch", "00" * 16), ("epoch", "bad")):
            with self.subTest(field=field, value=value):
                self.reject(lambda cut: cut["operations"][0].update({field: value}))
        for table in ("operations", "birth_origins"):
            self.reject(lambda cut: cut.update({table: []}))
            self.reject(lambda cut: cut[table].append(copy.deepcopy(cut[table][0])))
        self.reject(lambda cut: cut["birth_origins"][0].update(birth_operation="99" * 16))

    def test_native_image_missing_duplicate_malformed_unknown_cash_or_nonlive(self):
        self.reject(lambda cut: cut.update(mobiles=[]))
        self.reject(lambda cut: cut["mobiles"].append(copy.deepcopy(cut["mobiles"][0])))
        for value in (None, 7, "bad", "00" * 260):
            self.reject(lambda cut: cut["mobiles"][0].update(canonical_image=value))
        for field in ("mobile_instance_id", "mobile_revision", "stock_revision", "lifetime_state"):
            for value in (True, 1.0, "1", -1, 2**64):
                with self.subTest(field=field, value=value):
                    self.reject(lambda cut: cut["mobiles"][0].update({field: value}))
        self.reject(lambda cut: change_image(cut, 180, '<Q', 0))
        self.reject(lambda cut: change_image(cut, 188, '<q', -1))
        self.reject(lambda cut: change_image(cut, 40, '<Q', 99))
        self.reject(lambda cut: change_image(cut, 60, '<Q', 99))
        cut, args = modeled()
        cut["mobiles"][0]["canonical_image"] = native_mobile_image(version=1, identity=900).hex()
        # The maintained decoder accepts a historical image with no cash, but
        # the reused mobile cut check refuses to treat unknown cash as zero.
        self.assertEqual(checks.decode_native_mobile(bytes.fromhex(cut["mobiles"][0]["canonical_image"])),
                         (900, 2, 3, 1))
        with self.assertRaisesRegex(checks.CutError, "unknown cash"):
            oracle.assert_live_native_wallet(cut, **args)
        stages, args = temporal_cuts()
        observe(stages[1], [(900, args["original_wallet"])])
        # Isolate image lifetime from mapping retirement: an active-looking row
        # cannot make a maintained retired image become LIVE.
        stages[1]["account_mappings"][0].update(active_native_id=900, retiring_operation_id=None, revision=0)
        with self.assertRaises(checks.CutError):
            oracle.assert_live_native_wallet(stages[1], original_instance=900, native_wallet=args["original_wallet"])

    def test_mapping_SQL_upper_bound_does_not_adopt_native_allocator_sentinel(self):
        cut, args = modeled(2**64 - 1)
        self.assertEqual(oracle.assert_live_native_wallet(cut, **args)["mapping_id"], 2**64 - 1)

    def test_coherent_forgery_agrees_but_never_authenticates_or_decodes_opaque_receipts(self):
        cut, args = modeled()
        # Every supplied value is synthetic. Change the complete claimed epoch
        # and mapping consistently; algebraic agreement survives the forgery.
        new_epoch, new_mapping = "aa" * 16, 12345
        cut["meta"].update(epoch=new_epoch, watched_mapping_ids=[new_mapping])
        cut["epochs"][0]["epoch"] = new_epoch
        cut["lineage_head"][0]["active_epoch"] = new_epoch
        cut["account_mappings"][0]["mapping_id"] = new_mapping
        args["native_wallet"] = cut["meta"]["lineage"] + key(1, new_mapping, 12)[32:]
        cut["inbox"][0]["result_payload"] = "opaque-forged-not-a-native-receipt"
        cut["birth_origins"][0]["canonical_origin"] = "opaque-forged-not-a-native-carrier"
        unchanged = copy.deepcopy(cut)
        result = oracle.assert_live_native_wallet(cut, **args)
        self.assertEqual(result["scope"], "captured live native-wallet agreement only")
        self.assertIs(result["owner_authenticated"], False)
        self.assertIs(result["world_publication_proven"], False)
        self.assertEqual(result["external_proof_required"], list(oracle.EXTERNAL_PROOF_REQUIRED))
        self.assertEqual(result["retirement_scope"], "unsupported")
        self.assertEqual(cut, unchanged)


if __name__ == "__main__":
    unittest.main()
