#!/usr/bin/env python3
"""Modeled per-PID XP observations, not decoder/SQL/native execution."""

import copy
import unittest

from capture_quest_cut import MAX_ROWS
import quest_cut_checks as checks
import quest_reward_xp_checks as oracle
from test_quest_cut_checks import empty_cut

OPERATION = "34" * 16
OTHER = "56" * 16


def pack(*, version=5, legacy=False, pids=(42, 43, 44), slots=(2, 5)):
    # Explicit manual amounts, not an effective-XP formula or production recipe.
    amounts = ((200, 300), (2000, 1500), (100, 300))
    terms = dict(version=version, pid=pids[0], mobile_vnum=1234, level=1,
                 party_size=len(pids), xp_awards=[])
    cuts = []
    for recipient, pid in enumerate(pids):
        cut = empty_cut("QP06")
        cut["meta"].update(pid=pid, authority="native")
        cut["player"][0]["pid"] = pid
        for position, slot in enumerate(slots):
            amount = amounts[recipient][position]
            terms["xp_awards"].append(dict(pid=pid, index=slot, amount=amount))
            cut["xp_entitlements"].append(dict(offering_operation_id=OPERATION,
                recipient_pid=pid, reward_index=slot, amount=amount,
                applied=int(recipient == 2 or position == recipient)))
        if legacy:
            cut["meta"].update(authority="legacy-no-epoch", lineage=None, epoch=None, mobile_instance_ids=[])
            cut["epochs"] = []
        cuts.append(cut)
    owner = cuts[0]
    owner["obligations"] = [dict(offering_operation_id=OPERATION, player_pid=pids[0],
        continuation="aa" * 40, xp_applied_mask=1 << slots[0], acknowledged=0)]
    return [OPERATION, terms, owner, cuts[1:]]


def all_owner_applied(inputs):
    inputs[2]["obligations"][0].update(xp_applied_mask=sum(1 << row["reward_index"]
        for row in inputs[2]["xp_entitlements"]), acknowledged=1)
    for row in inputs[2]["xp_entitlements"]:
        row["applied"] = 1


class FrozenXPTests(unittest.TestCase):
    def reject(self, inputs, **kwargs):
        original = copy.deepcopy(inputs)
        with self.assertRaises(checks.CutError):
            oracle.assert_frozen_xp_agreement(*inputs, **kwargs)
        self.assertEqual(inputs, original)

    def test_three_recipients_multiple_slots_manual_amounts_and_mixed_flags(self):
        inputs = pack()
        before = copy.deepcopy(inputs)
        result = oracle.assert_frozen_xp_agreement(*inputs)
        self.assertEqual(len(result["expected_awards"]), 6)
        self.assertEqual(result["expected_awards"][2], dict(pid=43, index=2, amount=2000))
        self.assertEqual(result["observed_owner_mask"], 4)
        self.assertEqual(result["observations"][1]["applications"],
                         [dict(index=2, applied=0), dict(index=5, applied=1)])
        self.assertEqual(inputs, before)

    def test_two_recipients_and_versions5_6(self):
        for version in (5, 6):
            result = oracle.assert_frozen_xp_agreement(*pack(version=version, pids=(42, 43)))
            self.assertEqual(result["version"], version)
            self.assertEqual(len(result["observations"]), 2)
            self.assertFalse(result["owner_authenticated"])

    def test_old_unapplied_peer_and_new_owner_ack_do_not_imply_shared_time(self):
        inputs = pack()
        all_owner_applied(inputs)
        inputs[2]["meta"]["observed_at"] = "newer"
        inputs[3][0]["meta"]["observed_at"] = "older"
        self.assertEqual(inputs[3][0]["xp_entitlements"][0]["applied"], 0)
        result = oracle.assert_frozen_xp_agreement(*inputs)
        self.assertEqual(result["observed_owner_acknowledged"], 1)
        self.assertFalse(result["common_time_snapshot_proven"])
        self.assertFalse(result["ack_order_proven"])

    def test_coherent_forgery_and_coherence_labels_prove_only_declared_agreement(self):
        inputs = pack()
        # Every row and decoded value is invented, including a non-decoder blob.
        all_owner_applied(inputs)
        for cut in [inputs[2], *inputs[3]]:
            cut["meta"].update(common_cut="caller-claims-authentic", schedule_authenticated=True)
        result = oracle.assert_frozen_xp_agreement(*inputs)
        self.assertEqual(result["scope"], "captured frozen XP entitlement agreement only")
        for name in ("owner_authenticated", "decoded_terms_authenticated", "common_time_snapshot_proven",
                     "ack_order_proven", "effective_xp_proven", "save_completion_proven",
                     "native_journey_proven", "retirement_proven"):
            self.assertIs(result[name], False)
        # The export lacks reward_count/types. Removing a whole slot coherently
        # cannot prove an omitted production XP slot exists; decoder is external.
        inputs[1]["xp_awards"] = [a for a in inputs[1]["xp_awards"] if a["index"] == 2]
        for cut in [inputs[2], *inputs[3]]:
            cut["xp_entitlements"] = [r for r in cut["xp_entitlements"] if r["reward_index"] == 2]
        inputs[2]["obligations"][0]["xp_applied_mask"] = 4
        result = oracle.assert_frozen_xp_agreement(*inputs)
        self.assertFalse(result["decoded_terms_authenticated"])

    def test_original_operation_exact_hex_and_wrong_selected_operation(self):
        for invalid in (None, True, 34, bytes.fromhex(OPERATION), "", "0" * 32,
                        "A" * 32, "g" * 32, "1" * 31, "1" * 33, OTHER):
            inputs = pack()
            inputs[0] = invalid
            self.reject(inputs)

    def test_relevant_wrong_operation_loses_required_evidence(self):
        for target in ("obligation", "entitlement"):
            inputs = pack()
            row = inputs[2]["obligations"][0] if target == "obligation" else inputs[3][0]["xp_entitlements"][0]
            row["offering_operation_id"] = OTHER
            self.reject(inputs)

    def test_foreign_operations_coexist_without_other_domain_validation(self):
        inputs = pack()
        for cut in [inputs[2], *inputs[3]]:
            # A legitimate unrelated operation may have a different slot/amount
            # set. Only its operation identity is needed to exclude it here.
            cut["obligations"].append(dict(offering_operation_id=OTHER, player_pid=cut["meta"]["pid"]))
            cut["xp_entitlements"].append(dict(offering_operation_id=OTHER,
                recipient_pid=cut["meta"]["pid"], reward_index=1, amount=27, applied=1))
        original = copy.deepcopy(inputs)
        self.assertEqual(len(oracle.assert_frozen_xp_agreement(*inputs)["expected_awards"]), 6)
        self.assertEqual(inputs, original)

    def test_missing_extra_duplicate_conflicting_selected_entitlements_every_pid(self):
        for pid_index in range(3):
            for action in ("missing", "duplicate", "conflict", "extra"):
                inputs = pack()
                cut = [inputs[2], *inputs[3]][pid_index]
                rows = cut["xp_entitlements"]
                if action == "missing":
                    rows.pop()
                else:
                    extra = dict(rows[0])
                    if action == "conflict":
                        extra["amount"] += 1
                    if action == "extra":
                        extra["reward_index"] = 9
                    rows.append(extra)
                with self.subTest(pid_index=pid_index, action=action):
                    self.reject(inputs)

    def test_exact_types_ranges_and_foreign_pid_slot_amount_at_every_cut(self):
        for cut_index in range(3):
            for field, invalids in (
                ("recipient_pid", (True, False, None, "42", 42.0, 0, -1, 2**32, 99)),
                ("reward_index", (True, None, "2", 2.0, -1, 64, 7)),
                ("amount", (True, None, "200", 200.0, 0, -1, 2**31, 201)),
                ("applied", (True, False, None, "1", 1.0, -1, 2))):
                for invalid in invalids:
                    inputs = pack()
                    [inputs[2], *inputs[3]][cut_index]["xp_entitlements"][0][field] = invalid
                    with self.subTest(cut=cut_index, field=field, invalid=invalid):
                        self.reject(inputs)

    def test_export_missing_extra_duplicate_conflict_and_nonrectangular_awards(self):
        for action in ("missing", "duplicate", "conflict", "extra", "nonrectangular"):
            inputs = pack()
            awards = inputs[1]["xp_awards"]
            if action == "missing":
                awards.pop()
            else:
                extra = dict(awards[0])
                if action == "conflict":
                    extra["amount"] += 1
                if action == "extra":
                    extra["pid"] = 99
                if action == "nonrectangular":
                    awards[0]["index"] = 9
                else:
                    awards.append(extra)
            self.reject(inputs)

    def test_export_exact_fields_versions_group_cardinality_and_domains(self):
        fields = ("version", "pid", "mobile_vnum", "level", "party_size", "xp_awards")
        for field in fields:
            inputs = pack()
            del inputs[1][field]
            self.reject(inputs)
        inputs = pack()
        inputs[1]["credited_count"] = 3  # not an actual adapter export
        self.reject(inputs)
        for field, invalids in (
            ("version", (True, None, "5", 5.0, 0, 4, 7)),
            ("pid", (True, None, "42", 42.0, 0, -1, 2**32, 43)),
            ("mobile_vnum", (True, None, "1234", 1234.0, 0, -1, 2**31)),
            ("level", (True, None, "1", 1.0, -1, 2**31)),
            ("party_size", (True, None, "3", 3.0, 0, 1, 2, 65)),
            ("xp_awards", (None, {}, (), [], [None], [dict(pid=42)]))):
            for invalid in invalids:
                inputs = pack()
                inputs[1][field] = invalid
                self.reject(inputs)
        for field, invalids in (
            ("pid", (True, None, "42", 42.0, 0, -1, 2**32)),
            ("index", (True, None, "2", 2.0, -1, 64)),
            ("amount", (True, None, "200", 200.0, 0, -1, 2**31))):
            for invalid in invalids:
                inputs = pack()
                inputs[1]["xp_awards"][0][field] = invalid
                self.reject(inputs)

    def test_bounded64_full_awards_slot63_mask_and_signed_amount_max(self):
        inputs = pack(pids=(42, 43), slots=(0, 63))
        for award in inputs[1]["xp_awards"]:
            award["amount"] = 2**31 - 1
        for cut in [inputs[2], *inputs[3]]:
            for entry in cut["xp_entitlements"]:
                entry.update(amount=2**31 - 1, applied=1)
        inputs[2]["obligations"][0].update(xp_applied_mask=(1 << 63) | 1, acknowledged=1)
        oracle.assert_frozen_xp_agreement(*inputs)
        # Sixty-four total tuples is a shared array bound, not64 per recipient.
        inputs = pack(pids=(42, 43), slots=(0,))
        inputs[1]["xp_awards"] = [dict(pid=pid, index=slot, amount=1)
            for pid in (42, 43) for slot in range(32)]
        for cut in [inputs[2], *inputs[3]]:
            pid = cut["meta"]["pid"]
            cut["xp_entitlements"] = [dict(offering_operation_id=OPERATION, recipient_pid=pid,
                reward_index=slot, amount=1, applied=0) for slot in range(32)]
        inputs[2]["obligations"][0]["xp_applied_mask"] = 0
        self.assertEqual(len(oracle.assert_frozen_xp_agreement(*inputs)["expected_awards"]), 64)
        inputs[1]["xp_awards"].append(dict(pid=42, index=32, amount=1))
        self.reject(inputs)

    def test_sql_uint_pid_bound_is_not_native_actor_authentication(self):
        inputs = pack(pids=(42, 2**32 - 1))
        result = oracle.assert_frozen_xp_agreement(*inputs)
        self.assertEqual(result["observations"][1]["pid"], 2**32 - 1)
        self.assertFalse(result["owner_authenticated"])

    def test_owner_obligation_unique_identity_required_and_peer_cannot_export_it(self):
        for action in ("missing", "duplicate", "foreign_owner", "peer"):
            inputs = pack()
            if action == "missing":
                inputs[2]["obligations"].clear()
            elif action == "duplicate":
                inputs[2]["obligations"].append(dict(inputs[2]["obligations"][0]))
            elif action == "foreign_owner":
                inputs[2]["obligations"][0]["player_pid"] = 43
            else:
                inputs[3][0]["obligations"].append(dict(inputs[2]["obligations"][0]))
            self.reject(inputs)

    def test_owner_mask_type_range_unknown_bits_and_local_agreement(self):
        for invalid in (True, False, None, "4", 4.0, -1, 2**64, 0, 1, 4 | (1 << 63)):
            inputs = pack()
            inputs[2]["obligations"][0]["xp_applied_mask"] = invalid
            self.reject(inputs)
        inputs = pack()
        inputs[2]["xp_entitlements"][0]["applied"] = 0
        inputs[2]["obligations"][0]["xp_applied_mask"] = 0
        oracle.assert_frozen_xp_agreement(*inputs)

    def test_ack_flag_exact_and_only_owner_local_application_implication(self):
        for invalid in (True, False, None, "1", 1.0, -1, 2, 1):
            inputs = pack()
            inputs[2]["obligations"][0]["acknowledged"] = invalid
            self.reject(inputs)
        inputs = pack()
        all_owner_applied(inputs)
        oracle.assert_frozen_xp_agreement(*inputs)
        inputs[2]["obligations"][0]["acknowledged"] = 0
        oracle.assert_frozen_xp_agreement(*inputs)

    def test_one_cut_per_peer_exact_selection_no_duplicates_or_extra_missing(self):
        for action in ("missing", "extra", "duplicate", "owner", "foreign", "tuple"):
            inputs = pack()
            if action == "missing":
                inputs[3].pop()
            elif action == "extra":
                inputs[3].append(copy.deepcopy(inputs[3][0]))
            elif action == "duplicate":
                inputs[3][1] = copy.deepcopy(inputs[3][0])
            elif action == "owner":
                inputs[3][1] = copy.deepcopy(inputs[2])
            elif action == "foreign":
                inputs[3][1]["meta"]["pid"] = 99
                inputs[3][1]["player"][0]["pid"] = 99
            else:
                inputs[3] = tuple(inputs[3])
            self.reject(inputs)

    def test_selected_pid_strict_typed_same_metadata_and_one_player_row(self):
        for cut_index in range(3):
            for invalid in (True, False, None, "42", 42.0, 0, -1, 2**32):
                inputs = pack()
                cut = [inputs[2], *inputs[3]][cut_index]
                cut["player"][0]["pid"] = invalid
                self.reject(inputs)
            for invalid in (True, "42", 42.0, 99):
                inputs = pack()
                [inputs[2], *inputs[3]][cut_index]["meta"]["pid"] = invalid
                self.reject(inputs)
        for rows in ([], [dict(pid=42), dict(pid=42)]):
            inputs = pack()
            inputs[2]["player"] = rows
            self.reject(inputs)

    def test_candidate_binding_drift_every_recipient_and_typed_migrations(self):
        for cut_index in (1, 2):
            for field, invalid in (("source_commit", "ef" * 20), ("binary_sha256", "ef" * 32),
                                  ("schema_manifest_sha256", "ef" * 32), ("lineage", "33" * 16),
                                  ("epoch", "44" * 16), ("case", "QP01"),
                                  ("mobile_instance_ids", [901]), ("watched_vnums", [1234]),
                                  ("authority", "legacy-no-epoch")):
                inputs = pack()
                [inputs[2], *inputs[3]][cut_index]["meta"][field] = invalid
                self.reject(inputs)
        inputs = pack()
        inputs[2]["migrations"] = [dict(sequence=1)]
        for peer in inputs[3]:
            peer["migrations"] = [dict(sequence=True)]
        self.reject(inputs)
        inputs = pack()
        for cut in [inputs[2], *inputs[3]]:
            cut["meta"]["watched_vnums"] = [1]
        inputs[3][0]["meta"]["watched_vnums"] = [True]
        self.reject(inputs)

    def test_missing_tables_columns_malformed_rows_and_bounded_observations(self):
        for cut_index in range(3):
            for table in ("player", "obligations", "xp_entitlements", "migrations"):
                inputs = pack()
                del [inputs[2], *inputs[3]][cut_index][table]
                self.reject(inputs)
        for table in ("obligations", "xp_entitlements"):
            for invalid in (None, {}, [None], [dict(offering_operation_id=None)]):
                inputs = pack()
                inputs[2][table] = invalid
                self.reject(inputs)
        for field in ("continuation", "xp_applied_mask", "acknowledged", "player_pid"):
            inputs = pack()
            del inputs[2]["obligations"][0][field]
            self.reject(inputs)
        for field in ("recipient_pid", "reward_index", "amount", "applied"):
            inputs = pack()
            del inputs[3][0]["xp_entitlements"][0][field]
            self.reject(inputs)
        inputs = pack()
        inputs[2]["xp_entitlements"] += [dict(offering_operation_id=OTHER)] * MAX_ROWS
        self.reject(inputs)

    def test_continuation_required_bounded_hex_but_no_new_decoder(self):
        for invalid in (None, True, b"literal", "", "a", "zz", "AA", "aa" * 8193):
            inputs = pack()
            inputs[2]["obligations"][0]["continuation"] = invalid
            self.reject(inputs)
        inputs = pack()
        inputs[2]["obligations"][0]["continuation"] = "00" * 8192
        self.assertFalse(oracle.assert_frozen_xp_agreement(*inputs)["decoded_terms_authenticated"])

    def test_explicit_legacy_mode_retains_limited_group_agreement_no_native_promotion(self):
        inputs = pack(legacy=True)
        self.reject(inputs)
        result = oracle.assert_frozen_xp_agreement(*inputs, legacy=True)
        self.assertFalse(result["native_journey_proven"])
        for invalid in (1, "true", None):
            self.reject(pack(), legacy=invalid)
        inputs[3][0]["epochs"] = [dict(epoch="22" * 16)]
        self.reject(inputs, legacy=True)

    def test_order_of_awards_peers_and_rows_does_not_mutate_or_change_agreement(self):
        inputs = pack()
        expected = oracle.assert_frozen_xp_agreement(*inputs)
        inputs[1]["xp_awards"].reverse()
        inputs[3].reverse()
        for cut in [inputs[2], *inputs[3]]:
            cut["xp_entitlements"].reverse()
        before = copy.deepcopy(inputs)
        self.assertEqual(oracle.assert_frozen_xp_agreement(*inputs), expected)
        self.assertEqual(inputs, before)


if __name__ == "__main__":
    unittest.main()
