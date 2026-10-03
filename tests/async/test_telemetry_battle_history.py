#!/usr/bin/env python3
"""Native shared history, conservative linkage and bounded publication input."""
from __future__ import annotations

from collections import defaultdict
from copy import deepcopy
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

from test_telemetry_gameplay_adapters import ROOT, compile_gameplay

sys.path.insert(0, str(ROOT))
from scripts.telemetry import battle_history as history
from scripts.telemetry import battle_contract as battle
from scripts.telemetry import battle_contribution_contract as contribution
from scripts.telemetry.rollup_definitions import (
    ROLLUP_QUALITY_PROCESS_GAP, ROLLUP_QUALITY_INCIDENT_GAP,
    ROLLUP_QUALITY_INCIDENT_INVENTORY_UNKNOWN, ROLLUP_QUALITY_UTC_UNKNOWN, ROLLUP_QUALITY_UTC_BACKWARD)


def qualify_native_history(rows):
    """Also called with actual canonical SQL rows by the private writer journey."""
    scope = next((row["battle_environment_id"], row["battle_season_id"])
                 for row in rows if row["record_kind"] == 10)
    result = history.build_history(rows, scope)
    assert result.summary["source_fact_count"] == 123
    assert result.summary["complete_packet_count"] == 38
    assert result.summary["incomplete_packet_count"] == 0
    assert result.summary["contribution_count"] == 28
    assert result.summary["alias_count"] == 1
    assert result.summary["verified_contribution_links"] == 28
    assert result.summary["partial_contribution_links"] == 0
    canonical = [row for row in result.battles if row["canonical"]]
    assert sum(row["damage_dealt"] or 0 for row in canonical) == 112
    assert sum(row["damage_taken"] or 0 for row in canonical) == 112
    assert sum(row["effective_healing"] or 0 for row in canonical) == 15
    assert sum(row["healing_received"] or 0 for row in canonical) == 15
    assert sum(row["casting_attempts"] or 0 for row in canonical) == 6
    assert sum(row["casting_completions"] or 0 for row in canonical) == 1
    assert sum(row["casting_aborts"] or 0 for row in canonical) == 1
    assert sum(row["casting_unresolved"] or 0 for row in canonical) == 4
    assert all(row["control_applications"] is None and row["control_received"] is None for row in result.battles)
    assert all(row["outcome"] is None and not row["complete_metric_coverage_implied"] for row in result.battles)
    assert all(row["packet_history_complete"] for row in result.battles)
    assert {row["close_reason"] for row in canonical} == {1, 2, 3}
    assert all(row["end_censored"] for row in canonical)
    assert sum(row["retired_by_alias"] for row in result.battles) == 1
    assert result.summary["reserved_bytes"] <= history.DEFAULT_BYTE_LIMIT
    return result


class BattleHistoryTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        artifacts = ROOT / "bin/tests"
        artifacts.mkdir(parents=True, exist_ok=True)
        cls.directory = tempfile.TemporaryDirectory(prefix="telemetry-battle-history-", dir=artifacts)
        cls.path = Path(cls.directory.name)
        executable, exported = cls.path / "capture", cls.path / "capture.jsonl"
        compile_gameplay(executable)
        completed = subprocess.run([str(executable)], cwd=ROOT,
            env=dict(os.environ, TELEMETRY_BATTLE_CAPTURE_EXPORT=str(exported)),
            text=True, capture_output=True, check=True, timeout=30)
        assert "telemetry gameplay adapter paths passed" in completed.stdout
        cls.rows = [json.loads(line) for line in exported.read_text(encoding="utf-8").splitlines()]
        cls.scope = next((row["battle_environment_id"], row["battle_season_id"])
                         for row in cls.rows if row["record_kind"] == 10)
        cls.baseline = qualify_native_history(cls.rows)
        print(json.dumps(dict(cls.baseline.summary, native_history=True, running_server=False), sort_keys=True), flush=True)
        cls.packets = defaultdict(list)
        for row in cls.rows:
            if row["record_kind"] == 10:
                cls.packets[(*battle.battle_id(row), row["battle_revision"])].append(row)
        cls.verified = next(row for row in cls.baseline.contributions if row["link_status"] == "verified")
        pure, exported = cls.path / "association", cls.path / "association.jsonl"
        subprocess.run(["g++", "-std=c++20", "-Wall", "-Wextra", "-Werror", "-pedantic", "-I", str(ROOT / "src"),
            str(ROOT / "tests/async/telemetry_battle_harness.cc"), str(ROOT / "src/telemetry/telemetry_battle.c"),
            str(ROOT / "src/telemetry/telemetry_battle_contract.c"), "-o", str(pure)], check=True, timeout=120)
        subprocess.run([str(pure), "--export", str(exported)], check=True, capture_output=True, timeout=30)
        cls.association_cases = defaultdict(list)
        for line in exported.read_text(encoding="ascii").splitlines():
            item = json.loads(line)
            value = battle.decode_fact(bytes.fromhex(item["wire"]))
            index = len(cls.association_cases[item["case"]]) + 1
            cls.association_cases[item["case"]].append(dict(value, boot_id=value["battle_boot_id"],
                process_id=value["battle_process_id"], record_seq=index, schema_version=1,
                record_kind=10, occurrence_utc_usec=value["battle_at_utc_usec"]))

    @classmethod
    def tearDownClass(cls):
        cls.directory.cleanup()

    def test_actual_native_histories_and_disjoint_amounts(self):
        self.assertEqual(qualify_native_history(self.rows).summary, self.baseline.summary)
        retired = next(row for row in self.baseline.battles if row["retired_by_alias"])
        self.assertFalse(retired["canonical"])
        self.assertEqual(retired["contribution_count"], 0)
        self.assertIsNone(retired["damage_dealt"])
        owned = [row for row in self.baseline.contributions if row["bc_actor_kind"] == 2]
        unowned = [row for row in self.baseline.contributions if row["bc_actor_kind"] == 3 and row["bc_damage_dealt"]]
        self.assertTrue(owned and unowned)
        self.assertTrue(set(row["bc_actor_id"] for row in owned) & set(row["bc_actor_id"] for row in unowned))
        self.assertFalse({8804, 8805} & {row["bc_actor_id"] for row in self.baseline.contributions})
        for actor in self.baseline.actors:
            self.assertEqual(actor["battle_present_usec"], sum(actor["battle_" + mode + "_usec"]
                for mode in ("pve", "pvp", "mixed", "unknown_mode")))

    def test_reordered_arrival_and_identical_receipts_do_not_duplicate(self):
        reordered = history.build_history(list(reversed(self.rows)), self.scope)
        self.assertEqual(reordered, self.baseline)
        retry = history.build_history(self.rows + [deepcopy(self.rows[0])], self.scope)
        self.assertEqual(retry.battles, self.baseline.battles)
        self.assertEqual(retry.actors, self.baseline.actors)
        self.assertEqual(retry.contributions, self.baseline.contributions)
        self.assertEqual(retry.summary["identical_receipt_retries"], 1)

    def test_arrival_reorder_cannot_rewrite_source_transport_order(self):
        rows = deepcopy(self.rows)
        start = next(row for row in rows if row.get("battle_fact_kind") == 1)
        packet = sorted((row for row in rows if row["record_kind"] == 10 and
            battle.battle_id(row) == battle.battle_id(start) and row["battle_revision"] == 1),
            key=lambda row: row["battle_fact_index"])
        packet[0]["record_seq"], packet[-1]["record_seq"] = packet[-1]["record_seq"], packet[0]["record_seq"]
        with self.assertRaisesRegex(history.HistoryError, "packet_transport_order"):
            history.build_history(rows, self.scope)

    def test_existing_pure_association_journeys_reconcile_history(self):
        self.assertGreater(len(self.association_cases), 20)
        count = 0
        for case, rows in self.association_cases.items():
            with self.subTest(case=case):
                result = history.build_history(rows, (11, 22), max_total_bytes=128 * 1024 * 1024,
                    max_output_rows=2_000)
                count += result.summary["source_fact_count"]
                self.assertFalse(result.summary["decisive_outcomes_available"])
                self.assertTrue(all(row["damage_dealt"] is None for row in result.battles))
                for row in result.battles:
                    if row["incomplete_packet_count"]:
                        self.assertFalse(row["packet_history_complete"])
        self.assertGreater(count, 4_000)
        print(json.dumps({"pure_association_cases": len(self.association_cases), "source_facts": count}, sort_keys=True), flush=True)

    def test_conflicting_transport_and_second_logical_receipt_are_refused(self):
        original = next(row for row in self.rows if row["record_kind"] == 11 and row["bc_damage_dealt"])
        changed = dict(original, bc_damage_dealt=original["bc_damage_dealt"] + 1)
        with self.assertRaisesRegex(history.HistoryError, "transport_conflict"):
            history.build_history(self.rows + [changed], self.scope)
        changed = dict(original, record_seq=max(row["record_seq"] for row in self.rows) + 1)
        with self.assertRaisesRegex(history.HistoryError, "logical_receipt_conflict"):
            history.build_history(self.rows + [changed], self.scope)

    def test_missing_frames_and_whole_mutation_preserve_partial_history(self):
        packet = next(rows for rows in self.packets.values() if len(rows) >= 3 and rows[0]["battle_fact_kind"] == 1)
        for omitted in ([packet[0]], [packet[1]], [packet[-1]], packet):
            with self.subTest(omitted=len(omitted), index=omitted[0]["battle_fact_index"]):
                result = history.build_history([row for row in self.rows if row not in omitted], self.scope)
                self.assertEqual(result.summary["contribution_count"], 28)
                self.assertGreater(result.summary["partial_contribution_links"], 0)
                self.assertTrue(any(not row["packet_history_complete"] for row in result.battles))
                self.assertTrue(any(row["quality_flags"] & ROLLUP_QUALITY_PROCESS_GAP for row in result.battles))
                self.assertFalse(result.summary["complete_metric_coverage_implied"])
                self.assertEqual(sum(row["damage_dealt"] or 0 for row in result.battles if row["canonical"]), 112)

    def test_missing_alias_cannot_resolve_lineage(self):
        alias = next(row for row in self.rows if row.get("battle_fact_kind") == 5)
        result = history.build_history([row for row in self.rows if row is not alias], self.scope)
        self.assertEqual(result.summary["alias_count"], 0)
        self.assertTrue(any(not row["packet_history_complete"] for row in result.battles))
        self.assertEqual(sum(row["damage_dealt"] or 0 for row in result.battles if row["canonical"]), 112)

    def test_alias_requires_a_bridge_between_the_two_components(self):
        rows = deepcopy(self.association_cases[4])
        alias = next(row for row in rows if row["battle_fact_kind"] == 5)
        donor = tuple(alias[name] for name in battle.RELATED_BATTLE)
        target = next(row for row in rows if battle.battle_id(row) == donor and
                      row["battle_fact_kind"] == 2 and row["battle_actor_kind"] == 3)
        packet = [row for row in rows if battle.battle_id(row) == battle.battle_id(alias) and
                  row["battle_revision"] == alias["battle_revision"]]
        for row in packet:
            row.update(battle_side_status=3, battle_actor_side=0, battle_mode=1)
            if row["battle_fact_kind"] == 3:
                row.update(battle_related_actor_id=target["battle_actor_id"], battle_related_actor_kind=3)
        # Intrinsic packet validity alone cannot prove a cross-component bridge.
        battle.validate_packet([{name: row[name] for name in battle.FIELDS} for row in packet])
        with self.assertRaisesRegex(history.HistoryError, "alias_bridge_conflict"):
            history.build_history(rows, (11, 22))

    def test_later_packets_cannot_erase_latched_source_quality(self):
        rows = deepcopy(self.association_cases[4])
        for row in rows:
            if row["battle_fact_kind"] in (6, 7):
                row["battle_quality_flags"] &= ~1
                battle.validate_raw_fact(row)
        with self.assertRaisesRegex(history.HistoryError, "packet_quality_regression"):
            history.build_history(rows, (11, 22))

    def test_no_association_does_not_manufacture_rosters_or_zeros(self):
        result = history.build_history([row for row in self.rows if row["record_kind"] == 11], self.scope)
        self.assertEqual(result.summary["verified_contribution_links"], 0)
        self.assertEqual(result.summary["partial_contribution_links"], 28)
        self.assertFalse(result.actors)
        self.assertTrue(all(not row["start_seen"] and not row["close_seen"] and row["declared_actor_count"] is None
                            for row in result.battles))
        self.assertTrue(all(row["control_applications"] is None for row in result.battles))
        empty = history.build_history([], self.scope)
        self.assertFalse(empty.battles or empty.actors or empty.contributions)
        self.assertFalse(empty.summary["complete_metric_coverage_implied"])

    def test_complete_packet_references_require_effective_actor_context(self):
        key = contribution.segment_key(self.verified)
        source = next(row for row in self.rows if row["record_kind"] == 11 and contribution.segment_key(row) == key)
        changed = dict(source, bc_actor_class_id=source["bc_actor_class_id"] + 1)
        with self.assertRaisesRegex(history.HistoryError, "context_conflict"):
            history.build_history([changed if row is source else row for row in self.rows], self.scope)
        changed = dict(source, bc_first_association_fact_sequence=source["bc_first_association_fact_sequence"] - 1)
        result = history.build_history([changed if row is source else row for row in self.rows], self.scope)
        selected = next(row for row in result.contributions if contribution.segment_key(row) == key)
        self.assertEqual(selected["link_status"], "missing_packet")

    def test_context_change_cannot_hide_between_retained_references(self):
        # Extend a measured prefix just beyond an actual context-change packet.
        # Its first/last reference values still match; history must expose the
        # intervening boundary instead of blessing the extended old context.
        for source in self.rows:
            if source["record_kind"] != 11 or source["bc_end_reason"] != 1:
                continue
            matched = next(row for row in self.baseline.contributions if
                contribution.segment_key(row) == contribution.segment_key(source))
            if matched["link_status"] != "verified":
                continue
            changed = dict(source,
                bc_observed_through_monotonic_usec=source["bc_observed_through_monotonic_usec"] + 1,
                bc_decision_monotonic_usec=source["bc_decision_monotonic_usec"] + 1)
            try:
                history.build_history([changed if row is source else row for row in self.rows], self.scope)
            except history.HistoryError as error:
                if str(error) == "history_contribution_crosses_context_boundary":
                    return
        self.fail("native fixture must expose a changed context inside an illegally extended contribution")

    def test_overlap_cannot_add_a_second_copy_of_measured_time(self):
        source = next(row for row in self.rows if row["record_kind"] == 11 and
            row["bc_observed_through_monotonic_usec"] > row["bc_start_monotonic_usec"])
        changed = dict(source, record_seq=max(row["record_seq"] for row in self.rows) + 1,
            bc_segment_seq=max(row["bc_segment_seq"] for row in self.rows if row["record_kind"] == 11) + 1)
        with self.assertRaisesRegex(history.HistoryError, "contribution_overlap"):
            history.build_history(self.rows + [changed], self.scope)

    def test_alias_and_close_boundaries_refuse_later_measured_amounts(self):
        alias = next(row for row in self.rows if row.get("battle_fact_kind") == 5)
        donor = tuple(alias[name] for name in battle.RELATED_BATTLE)
        source = next(row for row in self.rows if row["record_kind"] == 11 and
            tuple(row[name] for name in contribution.BATTLE) == donor)
        through = alias["battle_at_monotonic_usec"] + 1
        decision = max(through, source["bc_decision_monotonic_usec"])
        changed = dict(source, bc_observed_through_monotonic_usec=through,
            bc_decision_monotonic_usec=decision)
        with self.assertRaisesRegex(history.HistoryError, "contribution_after_alias"):
            history.build_history([changed if row is source else row for row in self.rows], self.scope)
        source = next(row for row in self.rows if row["record_kind"] == 11 and row["bc_end_reason"] == 3)
        changed = dict(source, bc_decision_monotonic_usec=source["bc_decision_monotonic_usec"] + 1)
        with self.assertRaisesRegex(history.HistoryError, "close_boundary"):
            history.build_history([changed if row is source else row for row in self.rows], self.scope)
        close = next(row for row in self.rows if row.get("battle_fact_kind") == 7 and
            battle.battle_id(row) == tuple(source[name] for name in contribution.BATTLE))
        result = history.build_history([row for row in self.rows if row is not close], self.scope)
        selected = next(row for row in result.contributions if contribution.segment_key(row) == contribution.segment_key(source))
        self.assertEqual(selected["link_status"], "missing_lifecycle")

    def test_inactivity_decision_does_not_extend_measured_effort(self):
        inactivity = [row for row in self.baseline.battles if row["close_reason"] == 1]
        self.assertEqual(len(inactivity), 2)
        for row in inactivity:
            self.assertLess(row["observed_through_monotonic_usec"], row["decision_monotonic_usec"])
            actors = [actor for actor in self.baseline.actors if actor["canonical_battle"] == row["canonical_battle"]]
            self.assertEqual(len(actors), 2)
            self.assertTrue(all(actor["effort_replay_verified"] for actor in actors))

    def test_actor_leave_requires_an_observed_leave_boundary(self):
        source = next(row for row in self.rows if row["record_kind"] == 11 and row["bc_end_reason"] == 2 and
            row["bc_observed_through_monotonic_usec"] > row["bc_start_monotonic_usec"])
        changed = dict(source, bc_observed_through_monotonic_usec=source["bc_observed_through_monotonic_usec"] - 1)
        # Remove bounded duration metrics if this deliberately shortened prefix
        # would otherwise violate the intrinsic contract before history review.
        changed.update(bc_casting_elapsed_usec=0, bc_engaged_target_usec=0)
        result = history.build_history([changed if row is source else row for row in self.rows], self.scope)
        selected = next(row for row in result.contributions if contribution.segment_key(row) == contribution.segment_key(source))
        self.assertEqual(selected["link_status"], "missing_lifecycle")

    def test_absolute_actor_effort_conflict_is_refused(self):
        source = next(row for row in self.rows if row.get("battle_fact_kind") == 6 and
            row["battle_present_usec"] > 0 and not row["battle_quality_flags"] & battle.INCOMPLETE_GRAPH)
        mode = next(name for name in ("pve", "pvp", "mixed", "unknown_mode") if source["battle_" + name + "_usec"])
        changed = dict(source, battle_present_usec=source["battle_present_usec"] - 1,
            **{"battle_" + mode + "_usec": source["battle_" + mode + "_usec"] - 1})
        for name, maximum in (("contributor", "battle_present_usec"), ("unknown_side", "battle_present_usec"),
                              ("outnumbered_owner", "battle_pvp_usec")):
            field = "battle_" + name + "_usec"
            changed[field] = min(changed[field], changed[maximum])
        battle.validate_raw_fact(changed)
        with self.assertRaisesRegex(history.HistoryError, "cumulative_effort_conflict"):
            history.build_history([changed if row is source else row for row in self.rows], self.scope)

    def test_family_and_producer_specific_incident_coverage(self):
        coverage = {"registry_schema_version": 4, "status": "reviewed_inventory",
            "reviewed_from_utc_usec": -(1 << 63) + 1, "reviewed_through_utc_usec": (1 << 63) - 1, "incidents": []}
        covered = history.build_history(self.rows, self.scope, incident_coverage=coverage)
        self.assertTrue(all(not row["publication_quality_flags"] & ROLLUP_QUALITY_INCIDENT_INVENTORY_UNKNOWN
            for row in covered.contributions if not row["publication_quality_flags"] &
            (ROLLUP_QUALITY_UTC_UNKNOWN | ROLLUP_QUALITY_UTC_BACKWARD)))
        selected = self.verified
        item = {"status": "active", "record_kind_mask": 1 << 11,
            "producer_boot_id": selected["boot_id"], "producer_process_id": selected["process_id"],
            "first_record_seq": selected["record_seq"], "last_record_seq": selected["record_seq"],
            "start_utc_usec": None, "end_utc_usec": None}
        affected = history.build_history(self.rows, self.scope, incident_coverage=dict(coverage, incidents=[item]))
        rows = [row for row in affected.contributions if row["publication_quality_flags"] & ROLLUP_QUALITY_INCIDENT_GAP]
        self.assertEqual(len(rows), 1)
        self.assertEqual(contribution.segment_key(rows[0]), contribution.segment_key(selected))
        other = dict(item, producer_process_id=item["producer_process_id"] + 100000)
        unrelated = history.build_history(self.rows, self.scope, incident_coverage=dict(coverage, incidents=[other]))
        self.assertTrue(all(not row["publication_quality_flags"] & ROLLUP_QUALITY_INCIDENT_GAP for row in unrelated.contributions))
        with self.assertRaisesRegex(history.HistoryError, "incident_schema"):
            history.build_history(self.rows, self.scope, incident_coverage=dict(coverage, registry_schema_version=3))
        with self.assertRaisesRegex(history.HistoryError, "incident_review_range"):
            history.build_history(self.rows, self.scope, incident_coverage=dict(coverage,
                reviewed_from_utc_usec=True))

    def test_unknown_utc_preserves_measured_amount_and_prevents_clock_claims(self):
        source = next(row for row in self.rows if row["record_kind"] == 11 and row["bc_damage_dealt"])
        changed = dict(source, bc_start_utc_usec=contribution.UTC_UNKNOWN)
        result = history.build_history([changed if row is source else row for row in self.rows], self.scope)
        selected = next(row for row in result.contributions if contribution.segment_key(row) == contribution.segment_key(source))
        self.assertTrue(selected["publication_quality_flags"] & ROLLUP_QUALITY_UTC_UNKNOWN)
        self.assertEqual(selected["bc_damage_dealt"], source["bc_damage_dealt"])
        self.assertFalse(selected["complete_metric_coverage_implied"])

    def test_bounds_and_deadline_refuse_without_mutating_source(self):
        saved = deepcopy(self.rows)
        with self.assertRaisesRegex(history.HistoryError, "byte_capacity"):
            history.build_history(self.rows, self.scope, max_total_bytes=len(self.rows) * history.INPUT_BYTE_BOUND - 1)
        with self.assertRaisesRegex(history.HistoryError, "output_capacity"):
            history.build_history(self.rows, self.scope, max_output_rows=1)
        with self.assertRaisesRegex(history.HistoryError, "input_capacity"):
            history.build_history([self.rows[0]] * (history.MAX_INPUTS + 1), self.scope)
        def deadline():
            raise TimeoutError("fixture deadline")
        with self.assertRaisesRegex(TimeoutError, "fixture deadline"):
            history.build_history(self.rows, self.scope, check_deadline=deadline)
        self.assertEqual(self.rows, saved)
        result = history.build_history(self.rows, self.scope)
        result.contributions[0]["bc_damage_dealt"] += 1
        self.assertEqual(self.rows, saved)


if __name__ == "__main__":
    unittest.main()
