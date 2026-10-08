#!/usr/bin/env python3
"""Modeled four-cut task sensitivity; no SQL, service owner or native journey."""

import copy
import unittest

from capture_quest_cut import TASK_COLUMNS
import quest_cut_checks as checks
import world_quest_task_checks as oracle
from test_quest_cut_checks import empty_cut


def cuts(*, same_target=False, legacy=False):
    """Extend the maintained narrow builder with actual capture task columns."""
    request = empty_cut("QP07")
    request["meta"]["authority"] = "native"
    request["player"][0].update(
        quest_active=1, quest_mob_vnum=1708, quest_type=1, quest_accomplished=0,
        quest_started=100, quest_zone_number=17, quest_giver=1709, quest_level=11,
        quest_receiver=42, quest_shares_left=2, quest_kill_how_many=1,
        quest_kill_original=8, quest_map_room=0, quest_map_bought=0)
    request["history"] = [dict(id=7, quest_giver=1709, quest_target=1710, reward_vnum=1711)]
    reset = copy.deepcopy(request)
    reset["player"][0].update({name: -1 if name == "quest_zone_number" else 0
                               for name in TASK_COLUMNS if name != "quest_started"})
    replacement = copy.deepcopy(reset)
    replacement["player"][0].update(
        quest_active=1, quest_mob_vnum=1708 if same_target else 1707, quest_type=1,
        quest_started=101, quest_zone_number=17, quest_giver=1709, quest_level=11,
        quest_receiver=43, quest_shares_left=0, quest_kill_how_many=2,
        quest_kill_original=7)
    completed = copy.deepcopy(replacement)
    # Identical copper value, different denomination projection after restitution.
    completed["player"][0].update(copper=0, platinum=10)
    result = [request, reset, replacement, completed]
    if legacy:
        for cut in result:
            cut["meta"].update(authority="legacy-no-epoch", lineage=None, epoch=None, mobile_instance_ids=[])
            cut["epochs"] = []
    return result


class ReplacementTaskTests(unittest.TestCase):
    def reject(self, stages, *, legacy=False):
        original = copy.deepcopy(stages)
        with self.assertRaises(checks.CutError):
            oracle.assert_replacement_task_stable(*stages, legacy=legacy)
        self.assertEqual(stages, original)

    def test_four_cut_agreement_all_fields_and_canonical_wallet_no_financial_pass(self):
        stages = cuts()
        original = copy.deepcopy(stages)
        result = oracle.assert_replacement_task_stable(*stages)
        self.assertEqual(len(result["task_columns"]), 14)
        self.assertEqual(result["task_columns"], list(TASK_COLUMNS))
        self.assertEqual((result["pid"], result["original_attempt"], result["replacement_attempt"]), (42, 100, 101))
        self.assertEqual(checks.value(stages[2]["player"][0]), checks.value(stages[3]["player"][0]))
        self.assertNotEqual(stages[2]["player"][0], stages[3]["player"][0])
        self.assertFalse(result["financial_restitution_proven"])
        self.assertEqual(stages, original)

    def test_same_target_type_requires_newer_attempt_and_is_accepted(self):
        stages = cuts(same_target=True)
        self.assertEqual(stages[0]["player"][0]["quest_mob_vnum"], stages[2]["player"][0]["quest_mob_vnum"])
        oracle.assert_replacement_task_stable(*stages)

    def test_every_final_task_field_change_refuses_including_previously_omitted_fields(self):
        for name in TASK_COLUMNS:
            stages = cuts()
            stages[3]["player"][0][name] += 1
            with self.subTest(field=name):
                self.reject(stages)

    def test_every_field_is_required_at_every_stage(self):
        for stage in range(4):
            for name in TASK_COLUMNS:
                stages = cuts()
                del stages[stage]["player"][0][name]
                with self.subTest(stage=stage, field=name):
                    self.reject(stages)

    def test_every_field_rejects_nonintegers_and_int32_overflow_at_every_stage(self):
        for stage in range(4):
            for name in TASK_COLUMNS:
                for invalid in (True, False, None, "0", 0.0, [], {}, -(2**31)-1, 2**31):
                    stages = cuts()
                    stages[stage]["player"][0][name] = invalid
                    with self.subTest(stage=stage, field=name, value=invalid):
                        self.reject(stages)

    def test_signed_storage_bounds_do_not_invent_universal_nonnegative_rule(self):
        for endpoint in (-(2**31), 2**31-1):
            stages = cuts()
            # Literal storage agreement is not a valid-zone/target authenticity claim.
            for stage in (2, 3):
                stages[stage]["player"][0]["quest_zone_number"] = endpoint
            result = oracle.assert_replacement_task_stable(*stages)
            self.assertFalse(result["owner_authenticated"])
        self.assertEqual(cuts()[1]["player"][0]["quest_zone_number"], -1)

    def test_original_positive_and_reset_watermark_preserved(self):
        for value in (0, -1):
            stages = cuts()
            stages[0]["player"][0]["quest_started"] = value
            stages[1]["player"][0]["quest_started"] = value
            self.reject(stages)
        for value in (0, 99, 101):
            stages = cuts()
            stages[1]["player"][0]["quest_started"] = value
            self.reject(stages)

    def test_reset_requires_each_source_cleared_value(self):
        for name in TASK_COLUMNS:
            if name == "quest_started":
                continue
            stages = cuts()
            stages[1]["player"][0][name] += 1
            with self.subTest(field=name):
                self.reject(stages)

    def test_replacement_must_advance_even_with_same_target_type(self):
        for value in (-1, 0, 99, 100):
            stages = cuts(same_target=True)
            for stage in (2, 3):
                stages[stage]["player"][0]["quest_started"] = value
            self.reject(stages)
        stages = cuts()
        for stage in (2, 3):
            stages[stage]["player"][0]["quest_started"] = 2**31-1
        oracle.assert_replacement_task_stable(*stages)

    def test_original_and_replacement_must_be_unfinished_active_map_tasks(self):
        for stage in (0, 2):
            for name, invalid in (("quest_active", 0), ("quest_active", 2),
                                  ("quest_accomplished", 1), ("quest_type", 0),
                                  ("quest_type", 3), ("quest_map_bought", 1)):
                stages = cuts()
                stages[stage]["player"][0][name] = invalid
                self.reject(stages)
        stages = cuts()
        stages[2]["player"][0]["quest_map_room"] = 1734
        stages[3]["player"][0]["quest_map_room"] = 1734
        self.reject(stages)
        stages = cuts()
        stages[0]["player"][0]["quest_type"] = 2
        oracle.assert_replacement_task_stable(*stages)

    def test_unique_same_player_and_metadata_identity_at_every_stage(self):
        for stage in range(4):
            for wrong in (True, None, 0, -1, 2**31, "42", 42.0, 43):
                stages = cuts()
                stages[stage]["player"][0]["pid"] = wrong
                self.reject(stages)
            for remove in (True, False):
                stages = cuts()
                if remove:
                    stages[stage]["player"] = []
                else:
                    stages[stage]["player"].append(copy.deepcopy(stages[stage]["player"][0]))
                self.reject(stages)
            stages = cuts()
            stages[stage]["meta"]["pid"] = True
            self.reject(stages)

    def test_candidate_schema_authority_and_case_bindings(self):
        for stage in range(4):
            for name in ("case", "source_commit", "binary_sha256", "schema_manifest_sha256", "lineage", "epoch", "authority"):
                stages = cuts()
                stages[stage]["meta"][name] = "wrong"
                self.reject(stages)
            stages = cuts()
            stages[stage]["migrations"] = [dict(migration_id="changed")]
            self.reject(stages)

    def test_xp_complete_typed_and_unchanged_at_B_comparison(self):
        for stage in range(4):
            for invalid in (True, None, 0.0, "1000", -(2**63)-1, 2**63):
                stages = cuts()
                stages[stage]["player"][0]["exp"] = invalid
                self.reject(stages)
            stages = cuts()
            del stages[stage]["player"][0]["exp"]
            self.reject(stages)
        for value in (999, 1001):
            stages = cuts()
            stages[3]["player"][0]["exp"] = value
            self.reject(stages)
        for value in (-(2**63), 2**63-1):
            stages = cuts()
            for stage in (2, 3):
                stages[stage]["player"][0]["exp"] = value
            oracle.assert_replacement_task_stable(*stages)

    def test_history_literal_stability_missing_extra_removed_and_type_changes(self):
        for stage in (2, 3):
            for value in (None, {}, [None]):
                stages = cuts()
                stages[stage]["history"] = value
                self.reject(stages)
            stages = cuts()
            del stages[stage]["history"]
            self.reject(stages)
        for value in ([], [dict(id=8)], cuts()[3]["history"] * 2):
            stages = cuts()
            stages[3]["history"] = value
            self.reject(stages)
        for name in ("id", "quest_giver", "quest_target", "reward_vnum"):
            for value in (True, 7.0, None, "7", 999, float("nan")):
                stages = cuts()
                stages[3]["history"][0][name] = value
                self.reject(stages)
        stages = cuts()
        stages[3]["history"][0] = dict(reversed(list(stages[2]["history"][0].items())))
        oracle.assert_replacement_task_stable(*stages)

    def test_legitimate_A_reset_share_changes_are_not_B_mutations(self):
        stages = cuts()
        stages[1]["player"][0]["exp"] = 1001
        stages[1]["history"] = [dict(id=8)]
        stages[0]["player"][0]["quest_map_room"] = 1734
        oracle.assert_replacement_task_stable(*stages)

    def test_wallet_receipt_items_and_affects_remain_external_and_unmodified(self):
        stages = cuts()
        stages[3]["player"][0].update(copper=123, silver=50, gold=30, platinum=2)
        stages[3]["currency"] = [dict(operation_id="not-authenticated")]
        stages[3]["items"] = [dict(item_uid="unvalidated")]
        stages[3]["player_affects"] = [dict(type=1, duration=5, flags=0)]
        original = copy.deepcopy(stages)
        result = oracle.assert_replacement_task_stable(*stages)
        self.assertFalse(result["financial_restitution_proven"])
        self.assertFalse(result["native_journey_proven"])
        self.assertEqual(stages, original)

    def test_coherent_forgery_cannot_authenticate_runtime_or_finance(self):
        stages = cuts()
        for cut in stages:
            cut["meta"].update(source_commit="aa" * 20, binary_sha256="bb" * 32,
                               schema_manifest_sha256="cc" * 32)
        result = oracle.assert_replacement_task_stable(*stages)
        self.assertFalse(result["owner_authenticated"])
        self.assertFalse(result["native_journey_proven"])
        self.assertFalse(result["financial_restitution_proven"])
        self.assertEqual(result["external_proof_required"], list(oracle.EXTERNAL_PROOF_REQUIRED))
        self.assertIn("delay/release", " ".join(result["external_proof_required"]))
        self.assertIn("hold/ACK/replay/cold", " ".join(result["external_proof_required"]))

    def test_explicit_legacy_mode_cannot_claim_active_native_evidence(self):
        stages = cuts(legacy=True)
        result = oracle.assert_replacement_task_stable(*stages, legacy=True)
        self.assertFalse(result["native_journey_proven"])
        self.reject(stages)
        self.reject(cuts(), legacy=True)
        with self.assertRaises(checks.CutError):
            oracle.assert_replacement_task_stable(*cuts(), legacy=1)

    def test_old_refund_contract_still_requires_full_row_and_financial_proof(self):
        stages = cuts()
        with self.assertRaises(checks.CutError):
            checks.refunded(stages[2], stages[3], 110)
        self.assertFalse(oracle.assert_replacement_task_stable(*stages)["financial_restitution_proven"])


if __name__ == "__main__":
    unittest.main()
