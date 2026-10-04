#!/usr/bin/env python3
"""Native shared history, conservative linkage and bounded publication input."""
from __future__ import annotations

from collections import defaultdict
from copy import deepcopy
from dataclasses import replace
import hashlib
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
from scripts.telemetry import battle_source as source
from scripts.telemetry import battle_publication as publication, identity_history as identity, incident
from scripts.telemetry.rollup_engine import build_page_contributions, BoundsExceeded, SemanticError
from test_telemetry_observations import ownership
from scripts.telemetry.rollup_definitions import (
    ROLLUP_QUALITY_PROCESS_GAP, ROLLUP_QUALITY_INCIDENT_GAP,
    ROLLUP_QUALITY_INCIDENT_INVENTORY_UNKNOWN, ROLLUP_QUALITY_UTC_UNKNOWN,
    ROLLUP_QUALITY_UTC_BACKWARD, ROLLUP_QUALITY_UTC_MISMATCH)
from scripts.telemetry.rollup_definitions import RollupTarget, ROLLUP_QUALITY_LATE_INPUT, report_catalog


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
    assert all(row["control_applications"] == row["control_received"] ==
               (0 if row["available_metric_mask"] & 4 else None) for row in result.battles)
    assert all(row["outcome"] is None and not row["complete_metric_coverage_implied"] for row in result.battles)
    assert all(row["packet_history_complete"] for row in result.battles)
    assert {row["close_reason"] for row in canonical} == {1, 2, 3}
    assert all(row["end_censored"] for row in canonical)
    assert sum(row["retired_by_alias"] for row in result.battles) == 1
    assert result.exposures and result.summary["exposure_count"] == len(result.exposures)
    exposures = defaultdict(lambda: dict.fromkeys(battle.EFFORT, 0))
    for row in result.exposures:
        assert row["history_verified"] and not row["complete_population_coverage_implied"]
        assert row["battle_present_usec"] == row["observed_through_monotonic_usec"] - row["start_monotonic_usec"] > 0
        assert sum(row["battle_" + mode + "_usec"] for mode in ("pve", "pvp", "mixed", "unknown_mode")) == row["battle_present_usec"]
        cell = exposures[(row["canonical_battle"], row["battle_actor_id"])]
        for name in battle.EFFORT:
            cell[name] += row[name]
    for actor in result.actors:
        assert exposures[(actor["canonical_battle"], actor["battle_actor_id"])] == {name: actor[name] for name in battle.EFFORT}
    assert result.summary["verified_exposure_present_usec"] == sum(row["battle_present_usec"] for row in result.actors)
    assert result.summary["reserved_bytes"] <= history.DEFAULT_BYTE_LIMIT
    return result


def qualify_native_control_history(rows):
    """Actual helper/runtime capture and its independent canonical SQL readback."""
    scope = next((row["battle_environment_id"], row["battle_season_id"])
                 for row in rows if row["record_kind"] == 10)
    result = history.build_history(rows, scope)
    canonical = [row for row in result.battles if row["canonical"]]
    assert len(canonical) == 1 and canonical[0]["available_metric_mask"] == 31
    assert canonical[0]["control_applications"] == canonical[0]["control_received"] == 8
    assert result.summary["verified_contribution_links"] == result.summary["contribution_count"] > 0
    assert result.summary["partial_contribution_links"] == 0
    assert all(row["packet_history_complete"] and row["outcome"] is None and
        not row["complete_metric_coverage_implied"] for row in result.battles)
    assert all(row["battle_actor_id"] != 8963 for row in result.actors)
    segments = result.contributions
    assert sum(row["bc_control_applications"] for row in segments) == 8
    assert sum(row["bc_control_received"] for row in segments) == 8
    assert any(row["bc_actor_kind"] == 2 and row["bc_actor_owner_subject_id"] == 8961 and
        row["bc_control_applications"] == 1 for row in segments)
    assert any(row["bc_actor_kind"] == 3 and row["bc_control_applications"] ==
        row["bc_control_received"] == 1 for row in segments)
    assert any(row["bc_actor_id"] == 8962 and row["bc_modifier_flags"] & 128 and
        row["bc_control_applications"] == 1 for row in segments)
    return result


def qualify_retained_native_source(rows, scope, expected):
    """Actual SQL rows retain their ingestion IDs; export-only fixtures assign IDs."""
    if any("ingest_id" in row for row in rows):
        assert all("ingest_id" in row for row in rows)
        ordered = sorted(rows, key=lambda row: row["ingest_id"])
    else:
        ordered = [dict(row, ingest_id=index) for index, row in enumerate(sorted(rows,
            key=lambda row: (row["boot_id"], row["process_id"], row["record_seq"])), 1)]
    generation = (source.DEFINITION_VERSION, 1, *scope)
    inputs = [source.retain_input(row, generation, 0) for row in ordered]
    watermark = ordered[-1]["ingest_id"]
    header = source.advance_header(source.initial_header(generation), inputs, watermark)
    verified = source.verify_source(header, inputs, expected_scope=generation, expected_watermark=watermark)
    assert header["association_count"] == 123 and header["contribution_count"] == 28 and header["ownership_count"] == 0
    for raw, restored in zip(ordered, verified.facts, strict=True):
        expected_source = {name: raw.get(name) for name in source.SOURCE_COLUMNS[raw["record_kind"]]}
        if expected_source["ingested_utc_usec"] is None:
            expected_source["ingested_utc_usec"] = contribution.UTC_UNKNOWN
        assert restored == expected_source, "retained canonical source or arrival label drift"
    assert history.build_history(verified.facts, scope) == expected
    return verified, inputs


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
        controls = cls.path / "controls.jsonl"
        subprocess.run([str(executable), "--native-control-capture"], cwd=ROOT,
            env=dict(os.environ, TELEMETRY_BATTLE_CAPTURE_EXPORT=str(controls)),
            text=True, capture_output=True, check=True, timeout=30)
        cls.control_rows = [json.loads(line) for line in controls.read_text(encoding="utf-8").splitlines()]
        cls.scope = next((row["battle_environment_id"], row["battle_season_id"])
                         for row in cls.rows if row["record_kind"] == 10)
        cls.baseline = qualify_native_history(cls.rows)
        cls.source_window, cls.retained_inputs = qualify_retained_native_source(cls.rows, cls.scope, cls.baseline)
        cls.source_scope = tuple(cls.source_window.header[name] for name in source.SCOPE)
        print(json.dumps(dict(cls.baseline.summary, native_history=True, running_server=False), sort_keys=True), flush=True)
        print(json.dumps(dict(retained_source_value_contract=True, persisted_source_checkpoint=False,
            source_fact_count=cls.source_window.header["source_fact_count"], source_reserved_bytes=cls.source_window.reserved_bytes), sort_keys=True), flush=True)
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

    @staticmethod
    def _publication_window(rows, generation=1):
        scope = next((row["battle_environment_id"], row["battle_season_id"]) for row in rows if row["record_kind"] == 10)
        target = (source.DEFINITION_VERSION, generation, *scope)
        facts = [dict(row, ingest_id=index) for index, row in enumerate(sorted(rows,
            key=lambda row: (row["boot_id"], row["process_id"], row["record_seq"])), 1)]
        inputs = [source.retain_input(row, target, 0) for row in facts]
        header = source.advance_header(source.initial_header(target), inputs, len(facts))
        return source.verify_source(header, inputs, expected_scope=target, expected_watermark=len(facts))

    def _dated_publication_fixture(self, *, midnight=False):
        # Controlled coherent UTC labels qualify dated boundaries separately
        # from the original real-clock native/SQL readbacks. Measured amounts,
        # complete association references and native actor context stay intact.
        rows = deepcopy(self.rows)
        candidates = [row for row in rows if row["record_kind"] == 11 and row["bc_actor_kind"] == 1 and
            row["bc_observed_through_monotonic_usec"] - row["bc_start_monotonic_usec"] >= 4 and
            all(row["bc_actor_" + name] for name in ("session_boot_id", "session_process_id", "session_seq"))]
        selected = max(candidates, key=lambda row: row["bc_observed_through_monotonic_usec"] - row["bc_start_monotonic_usec"])
        first, last = selected["bc_start_monotonic_usec"], selected["bc_observed_through_monotonic_usec"]
        middle = first + (last - first) // 2
        day = 86_400_000_000
        epoch = day - middle % day if midnight else 2 * day
        for row in rows:
            prefix = "battle_" if row["record_kind"] == 10 else "bc_"
            for clock in ("at", "observed_through") if row["record_kind"] == 10 else ("start", "observed_through", "decision"):
                row[prefix + clock + "_utc_usec"] = epoch + row[prefix + clock + "_monotonic_usec"]
            row["occurrence_utc_usec"] = row[prefix + ("at" if row["record_kind"] == 10 else "decision") + "_utc_usec"]
        producer = selected["bc_battle_boot_id"], selected["bc_battle_process_id"]
        sequence = max(row["record_seq"] for row in rows if (row["boot_id"], row["process_id"]) == producer)
        for index, (at, token) in enumerate(((first, 101), (middle, 102)), 1):
            rows.append(ownership(boot_id=producer[0], process_id=producer[1], record_seq=sequence + index,
                connection_boot_id=producer[0], connection_process_id=producer[1], connection_seq=index,
                environment_id=self.scope[0], season_id=self.scope[1], config_id=selected["bc_config_id"],
                classifier_version=selected["bc_classifier_version"], policy_version=selected["bc_policy_version"],
                subject_id=selected["bc_actor_id"], pid=selected["bc_actor_pid"],
                session_boot_id=selected["bc_actor_session_boot_id"], session_process_id=selected["bc_actor_session_process_id"],
                session_seq=selected["bc_actor_session_seq"], at_monotonic_usec=at, at_utc_usec=epoch + at,
                occurrence_utc_usec=epoch + at, ownership_account_token=token))
        registry = identity.Registry(*self.scope, 1, 0, None, epoch + first - 1, epoch + last + 1,
            epoch + last + 2, "a" * 64, "b" * 64,
            tuple(identity.Association(index, token, controller, epoch + first - 1, None,
                "confirmed", "staff_review", "c" * 64) for index, (token, controller) in enumerate(((101, 701), (102, 702)), 1)))
        return self._publication_window(rows), registry, contribution.segment_key(selected), first, middle, last, epoch

    def test_publication_preserves_native_observations_and_unknown_identity(self):
        saved = deepcopy(self.source_window)
        coverage = incident.public_coverage(None, [], registry_schema_version=4)
        output = publication.build_publication(self.source_window, None, coverage)
        values = [publication.decode_row(self.source_scope, row) for row in output.rows]
        self.assertEqual(output.header["verified_contribution_links"], 28)
        self.assertEqual(output.header["partial_contribution_links"], 0)
        self.assertEqual(output.header["contribution_row_count"], 28)
        self.assertEqual(output.header["observed_pc_present_usec"], output.header["unknown_account_pc_present_usec"])
        self.assertEqual(output.header["confirmed_controller_pc_present_usec"], 0)
        self.assertEqual(output.header["observed_present_usec"], self.baseline.summary["verified_exposure_present_usec"])
        self.assertEqual(sum(row["damage_dealt"] or 0 for row in values if row.get("canonical")), 112)
        metrics = [row for row in values if "bc_segment_seq" in row]
        self.assertTrue(all(row["bc_control_applications"] == row["bc_control_received"] == 0 for row in metrics))
        self.assertTrue(all(row.get("controller_token") is None for row in values))
        self.assertEqual(self.source_window, saved)
        self.assertLessEqual(output.reserved_bytes, source.DEFAULT_BYTE_LIMIT)

    def test_accepted_native_control_survives_history_and_publication(self):
        result = qualify_native_control_history(self.control_rows)
        window = self._publication_window(self.control_rows)
        scope = tuple(window.header[name] for name in source.SCOPE)
        output = publication.build_publication(window, None, incident.public_coverage(None, [], registry_schema_version=4))
        self.assertEqual(output.header["verified_contribution_links"], result.summary["contribution_count"])
        metrics = [publication.decode_row(scope, row) for row in output.rows if row["row_kind"] == 3]
        self.assertEqual(sum(row["bc_control_applications"] for row in metrics), 8)
        self.assertEqual(sum(row["bc_control_received"] for row in metrics), 8)
        self.assertTrue(all(row["account_token"] is None and row["controller_token"] is None and
            not row["complete_metric_coverage_implied"] for row in metrics))

    def test_older_unavailable_control_remains_null_after_publication(self):
        rows = deepcopy(self.rows)
        for row in rows:
            if row["record_kind"] == 11:
                self.assertEqual((row["bc_control_applications"], row["bc_control_received"]), (0, 0))
                row["bc_available_metrics"] &= ~4
        result = history.build_history(rows, self.scope)
        self.assertTrue(all(row["control_applications"] is None and row["control_received"] is None
            for row in result.battles))
        window = self._publication_window(rows)
        output = publication.build_publication(window, None, incident.public_coverage(None, [], registry_schema_version=4))
        scope = tuple(window.header[name] for name in source.SCOPE)
        metrics = [publication.decode_row(scope, row) for row in output.rows if row["row_kind"] == 3]
        self.assertTrue(all(row["bc_control_applications"] is None and row["bc_control_received"] is None for row in metrics))

    def test_publication_dated_transfer_conserves_presence_without_dividing_amounts(self):
        window, registry, selected, first, middle, last, _epoch = self._dated_publication_fixture()
        coverage = incident.public_coverage(None, [], registry_schema_version=4)
        output = publication.build_publication(window, registry, coverage)
        values = [publication.decode_row(self.source_scope, row) for row in output.rows]
        metric = next(row for row in values if "bc_segment_seq" in row and contribution.segment_key(row) == selected)
        self.assertEqual(metric["attribution_status"], "identity_changes_inside_segment")
        self.assertIsNone(metric["account_token"])
        self.assertIsNone(metric["controller_token"])
        original = next(row for row in window.facts if row["record_kind"] == 11 and contribution.segment_key(row) == selected)
        self.assertEqual(metric["bc_damage_dealt"], original["bc_damage_dealt"])
        actor = original["bc_actor_id"]
        pieces = [row for row in values if "utc_day" in row and row["battle_actor_id"] == actor and
            row["source_battle"] == list(selected[:2]) + [original["bc_battle_seq"]] and
            first <= row["start_monotonic_usec"] < last]
        self.assertTrue(pieces)
        before = [row for row in pieces if row["observed_through_monotonic_usec"] <= middle]
        after = [row for row in pieces if row["start_monotonic_usec"] >= middle]
        self.assertTrue(before and after)
        self.assertTrue(all((row["account_token"], row["controller_token"]) == (101, 701) for row in before))
        self.assertTrue(all((row["account_token"], row["controller_token"]) == (102, 702) for row in after))

    def test_publication_midnight_retains_actual_clocks_and_dated_identity(self):
        window, registry, selected, first, middle, last, epoch = self._dated_publication_fixture(midnight=True)
        output = publication.build_publication(window, registry, incident.public_coverage(None, [], registry_schema_version=4))
        values = [publication.decode_row(self.source_scope, row) for row in output.rows if row["row_kind"] == 4]
        actor = next(row["bc_actor_id"] for row in window.facts if row["record_kind"] == 11 and contribution.segment_key(row) == selected)
        selected_rows = [row for row in values if row["battle_actor_id"] == actor and first <= row["start_monotonic_usec"] < last]
        self.assertGreaterEqual(len({row["utc_day"] for row in selected_rows}), 2)
        for row in selected_rows:
            self.assertEqual(row["start_utc_usec"], epoch + row["start_monotonic_usec"])
            self.assertEqual(row["observed_through_utc_usec"], epoch + row["observed_through_monotonic_usec"])
        self.assertTrue(any(row["start_monotonic_usec"] == middle for row in selected_rows))

    def test_publication_review_correction_preserves_reserved_generation(self):
        window, registry, _selected, _first, _middle, _last, _epoch = self._dated_publication_fixture()
        coverage = incident.public_coverage(None, [], registry_schema_version=4)
        old = publication.build_publication(window, registry, coverage)
        saved = deepcopy(old)
        corrected = replace(registry, registry_version=2, previous_registry_version=1,
            previous_packet_digest=registry.packet_digest,
            associations=tuple(replace(row, controller_token=799) if row.account_token == 101 else row
                for row in registry.associations))
        new_window = self._publication_window(window.facts, generation=2)
        new = publication.build_publication(new_window, corrected, coverage)
        old_values = [publication.decode_row(self.source_scope, row) for row in old.rows if row["row_kind"] == 4]
        new_scope = tuple(new_window.header[name] for name in source.SCOPE)
        new_values = [publication.decode_row(new_scope, row) for row in new.rows if row["row_kind"] == 4]
        self.assertTrue(any(row["controller_token"] == 701 for row in old_values))
        self.assertTrue(any(row["controller_token"] == 799 for row in new_values))
        self.assertEqual({row["registry_version"] for row in old_values}, {1})
        self.assertEqual({row["registry_version"] for row in new_values}, {2})
        self.assertEqual(old, saved)
        self.assertEqual(old.header["observed_present_usec"], new.header["observed_present_usec"])

    def test_publication_uniform_account_survives_changing_controller_reviews(self):
        window, registry, selected, first, middle, last, epoch = self._dated_publication_fixture()
        rows = deepcopy(window.facts)
        for row in rows:
            if row["record_kind"] == 9:
                row["ownership_account_token"] = 101
        window = self._publication_window(rows)
        for later_controller, expected_controller, expected_status in (
                (702, None, "mixed_review_linkage"), (701, 701, "confirmed_across_associations")):
            reviewed = replace(registry, associations=(
                identity.Association(1, 101, 701, epoch + first - 1, epoch + middle,
                    "confirmed", "staff_review", "c" * 64),
                identity.Association(2, 101, later_controller, epoch + middle, epoch + last + 1,
                    "confirmed", "staff_review", "d" * 64)))
            output = publication.build_publication(window, reviewed,
                incident.public_coverage(None, [], registry_schema_version=4))
            values = [publication.decode_row(self.source_scope, row) for row in output.rows if row["row_kind"] == 3]
            metric = next(row for row in values if contribution.segment_key(row) == selected)
            self.assertEqual(metric["account_token"], 101)
            self.assertEqual(metric["controller_token"], expected_controller)
            self.assertEqual(metric["linkage_status"], expected_status)
            self.assertIsNone(metric["association_id"])
            original = next(row for row in rows if row["record_kind"] == 11 and contribution.segment_key(row) == selected)
            self.assertEqual(metric["bc_damage_dealt"], original["bc_damage_dealt"])

    def test_publication_ownership_loss_waits_for_a_fresh_observed_anchor(self):
        window, registry, selected, first, middle, last, epoch = self._dated_publication_fixture()
        original = next(row for row in window.facts if row["record_kind"] == 11 and contribution.segment_key(row) == selected)
        begin = first + (middle - first) // 2
        packet = incident.template(4)
        packet.update(environment_id=self.scope[0], season_id=self.scope[1], reviewer_token="a" * 64,
            review_evidence_digest="b" * 64, reviewed_from_utc_usec=epoch + first - 1,
            reviewed_through_utc_usec=epoch + last + 1)
        loss = dict(packet["incidents"][0], producer_boot_id=original["boot_id"],
            producer_process_id=original["process_id"], start_utc_usec=epoch + begin,
            end_utc_usec=epoch + middle, record_kind_mask=1 << 9, evidence_digest="d" * 64)
        for open_end in (False, True):
            packet["incidents"] = [dict(loss, end_utc_usec=None)] if open_end else [loss]
            meta, details = incident.validate_packet(packet)
            details = [dict(row, occurrence_relation=1) for row in details]
            summary = incident.publication_summary(self.source_scope, meta, details)
            covered = incident.public_coverage(summary, details, registry_schema_version=4)
            output = publication.build_publication(window, registry, covered)
            values = [publication.decode_row(self.source_scope, row) for row in output.rows if row["row_kind"] == 4]
            pieces = [row for row in values if row["battle_actor_id"] == original["bc_actor_id"] and
                row["source_battle"] == [original[name] for name in contribution.BATTLE] and
                first <= row["start_monotonic_usec"] < last]
            self.assertTrue(pieces)
            self.assertTrue(all(row["account_token"] is None for row in pieces if
                begin <= row["start_monotonic_usec"] < (last if open_end else middle)))
            recovered = [row for row in pieces if row["start_monotonic_usec"] >= middle]
            self.assertTrue(recovered)
            self.assertTrue(all(row["account_token"] is None if open_end else row["account_token"] == 102
                for row in recovered))
            self.assertEqual(output.header["observed_present_usec"], self.baseline.summary["verified_exposure_present_usec"])
            self.assertTrue(output.header["quality_flags"] & ROLLUP_QUALITY_INCIDENT_GAP)

    def test_publication_retains_full_association_values_and_unproven_pet_identity(self):
        window, registry, _selected, _first, _middle, _last, _epoch = self._dated_publication_fixture()
        output = publication.build_publication(window, registry,
            incident.public_coverage(None, [], registry_schema_version=4))
        decoded = [publication.decode_row(self.source_scope, row) for row in output.rows]
        associations = {battle.fact_key(row): row for row in decoded if "battle_fact_index" in row}
        self.assertEqual(len(associations), 123)
        for row in window.facts:
            if row["record_kind"] == 10:
                self.assertEqual({name: associations[battle.fact_key(row)][name] for name in source.SOURCE_COLUMNS[10]},
                    {name: row[name] for name in source.SOURCE_COLUMNS[10]})
        pets = [row for row in decoded if "utc_day" in row and row["battle_actor_kind"] == 2]
        self.assertTrue(pets)
        self.assertTrue(all(row["account_token"] is None and row["controller_token"] is None and
            row["linkage_status"] == "unproven_owner_identity" for row in pets))

    def test_publication_unknown_clock_keeps_account_but_not_controller(self):
        window, registry, selected, _first, _middle, _last, _epoch = self._dated_publication_fixture()
        rows = deepcopy(window.facts)
        for row in rows:
            if row["record_kind"] == 10:
                for clock in ("at", "observed_through"):
                    row["battle_" + clock + "_utc_usec"] = contribution.UTC_UNKNOWN
            elif row["record_kind"] == 11:
                for clock in ("start", "observed_through", "decision"):
                    row["bc_" + clock + "_utc_usec"] = contribution.UTC_UNKNOWN
            else:
                row["at_utc_usec"] = contribution.UTC_UNKNOWN
            row["occurrence_utc_usec"] = contribution.UTC_UNKNOWN
        changed = self._publication_window(rows)
        output = publication.build_publication(changed, registry, incident.public_coverage(None, [], registry_schema_version=4))
        values = [publication.decode_row(self.source_scope, row) for row in output.rows if row["row_kind"] == 4]
        self.assertTrue(any(row["account_token"] is not None for row in values))
        self.assertTrue(all(row["controller_token"] is None and row["utc_day"] is None for row in values))
        self.assertEqual(output.header["observed_present_usec"], self.baseline.summary["verified_exposure_present_usec"])

    def test_publication_refuses_changed_source_or_incident_schema(self):
        coverage = incident.public_coverage(None, [], registry_schema_version=4)
        changed = deepcopy(self.source_window)
        row = next(row for row in changed.facts if row["record_kind"] == 11)
        row["bc_damage_dealt"] += 1
        with self.assertRaisesRegex(publication.PublicationError, "source_changed"):
            publication.build_publication(changed, None, coverage)
        with self.assertRaisesRegex(publication.PublicationError, "incident_schema"):
            publication.build_publication(self.source_window, None, dict(coverage, registry_schema_version=3))

    def test_publication_payload_scope_key_and_quality_tampering_refuse(self):
        output = publication.build_publication(self.source_window, None, incident.public_coverage(None, [], registry_schema_version=4))
        row = dict(output.rows[0])
        for field, value in (("payload", row["payload"] + b" "), ("row_key", b"x" * 32),
                ("quality_flags", row["quality_flags"] ^ 1), ("row_kind", True), ("generation", 99)):
            with self.subTest(field=field), self.assertRaises(publication.PublicationError):
                publication.decode_row(self.source_scope, dict(row, **{field: value}))

    def test_publication_capacity_deadline_and_values_are_bounded(self):
        saved = deepcopy(self.source_window)
        coverage = incident.public_coverage(None, [], registry_schema_version=4)
        with self.assertRaisesRegex(publication.PublicationError, "byte_capacity"):
            publication.build_publication(self.source_window, None, coverage, max_total_bytes=self.source_window.reserved_bytes)
        with self.assertRaises((publication.PublicationError, history.HistoryError)):
            publication.build_publication(self.source_window, None, coverage, max_output_rows=1)
        def expired():
            raise TimeoutError("publication deadline")
        with self.assertRaises(TimeoutError):
            publication.build_publication(self.source_window, None, coverage, check_deadline=expired)
        self.assertEqual(self.source_window, saved)

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
        self.assertEqual(retry.exposures, self.baseline.exposures)
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
                    max_output_rows=10_000)
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
        rows = deepcopy([row for row in self.rows if row["record_kind"] == 11])
        for row in rows:
            row["bc_available_metrics"] &= ~4  # Explicitly unavailable control producer.
        result = history.build_history(rows, self.scope)
        self.assertEqual(result.summary["verified_contribution_links"], 0)
        self.assertEqual(result.summary["partial_contribution_links"], 28)
        self.assertFalse(result.actors)
        self.assertFalse(result.exposures)
        self.assertTrue(all(not row["start_seen"] and not row["close_seen"] and row["declared_actor_count"] is None
                            for row in result.battles))
        self.assertTrue(all(row["control_applications"] is None for row in result.battles))
        empty = history.build_history([], self.scope)
        self.assertFalse(empty.battles or empty.actors or empty.contributions or empty.exposures)
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

    def test_historical_exposure_keeps_context_and_presence_only_effort(self):
        contexts = defaultdict(set)
        for row in self.baseline.exposures:
            contexts[(row["canonical_battle"], row["battle_actor_id"])].add((
                row["battle_config_id"], row["battle_mode"], row["battle_actor_owner_subject_id"],
                row["battle_actor_group_key"], row["battle_actor_group_revision"]))
        self.assertTrue(any(len(values) > 1 for values in contexts.values()))
        pet = [row for row in self.baseline.exposures if row["battle_actor_kind"] in (2, 3)]
        self.assertTrue(any(row["battle_actor_owner_subject_id"] for row in pet))
        self.assertTrue(any(not row["battle_actor_owner_subject_id"] for row in pet))
        presence = [row for row in self.baseline.exposures if row["battle_actor_id"] in (8804, 8805)]
        self.assertTrue(presence)
        self.assertTrue(all(row["battle_present_usec"] > 0 and row["battle_contributor_usec"] == 0 for row in presence))
        self.assertFalse({8804, 8805} & {row["bc_actor_id"] for row in self.baseline.contributions})

    def test_alias_exposure_conserves_original_prefixes_once(self):
        result = history.build_history(self.association_cases[4], (11, 22))
        retired = next(row for row in result.battles if row["retired_by_alias"])
        alias = next(row for row in self.association_cases[4] if row["battle_fact_kind"] == 5)
        donor = [row for row in result.exposures if row["source_battle"] == retired["battle"]]
        self.assertTrue(donor)
        self.assertTrue(all(row["canonical_battle"] == retired["canonical_battle"] and
            row["observed_through_monotonic_usec"] <= alias["battle_at_monotonic_usec"] for row in donor))
        amounts = defaultdict(lambda: dict.fromkeys(battle.EFFORT, 0))
        for row in result.exposures:
            for name in battle.EFFORT:
                amounts[(row["canonical_battle"], row["battle_actor_id"])][name] += row[name]
        for actor in result.actors:
            self.assertEqual(amounts[(actor["canonical_battle"], actor["battle_actor_id"])],
                {name: actor[name] for name in battle.EFFORT})

    def test_exposure_inactivity_end_uses_observed_clock(self):
        positive = next(rows for rows in self.association_cases.values() if any(row["battle_fact_kind"] == 7 and
            row["battle_close_reason"] == 1 and row["battle_observed_through_monotonic_usec"] > row["battle_start_monotonic_usec"] for row in rows))
        result = history.build_history(positive, (11, 22), max_output_rows=10_000, max_total_bytes=128 * 1024 * 1024)
        closes = [row for row in positive if row["battle_fact_kind"] == 7 and row["battle_close_reason"] == 1]
        for close in closes:
            rows = [row for row in result.exposures if row["source_battle"] == battle.battle_id(close)]
            self.assertTrue(rows)
            self.assertTrue(all(row["observed_through_monotonic_usec"] <= close["battle_observed_through_monotonic_usec"] for row in rows))
            final = [row for row in rows if row["observed_through_monotonic_usec"] == close["battle_observed_through_monotonic_usec"]]
            self.assertTrue(final)
            self.assertTrue(all(row["observed_through_monotonic_usec"] == close["battle_observed_through_monotonic_usec"] and
                row["observed_through_utc_usec"] == close["battle_observed_through_utc_usec"] for row in final))
            self.assertLess(close["battle_observed_through_monotonic_usec"], close["battle_at_monotonic_usec"])

    def test_missing_mutation_cannot_extend_verified_exposure(self):
        def known_before(rows):
            identity, revision = battle.battle_id(rows[0]), rows[0]["battle_revision"]
            through = max(row["battle_at_monotonic_usec"] for row in self.rows if row["record_kind"] == 10 and
                battle.battle_id(row) == identity and row["battle_revision"] < revision)
            return any(row["source_battle"] == identity and row["start_monotonic_usec"] < through
                for row in self.baseline.exposures)
        packet = next(rows for rows in self.packets.values() if rows[0]["battle_fact_kind"] not in (1, 5, 6) and
            rows[0]["battle_revision"] >= 3 and known_before(rows))
        result = history.build_history([row for row in self.rows if row not in packet], self.scope)
        identity, revision = battle.battle_id(packet[0]), packet[0]["battle_revision"]
        through = max(row["battle_at_monotonic_usec"] for row in self.rows if row["record_kind"] == 10 and
            battle.battle_id(row) == identity and row["battle_revision"] < revision)
        before = [row for row in self.baseline.exposures if row["source_battle"] == identity and row["start_monotonic_usec"] < through]
        retained = [row for row in result.exposures if row["source_battle"] == identity]
        self.assertTrue(retained)
        self.assertTrue(all(row["observed_through_monotonic_usec"] <= through for row in retained))
        expected = {name: sum(min(through, row["observed_through_monotonic_usec"]) - row["start_monotonic_usec"]
            for row in before if row[name]) for name in battle.EFFORT}
        self.assertEqual({name: sum(row[name] for row in retained) for name in battle.EFFORT}, expected)
        self.assertTrue(any(not row["packet_history_complete"] for row in result.battles))

    def test_unknown_exposure_clock_preserves_monotonic_effort(self):
        selected = next(row for row in self.baseline.exposures if not row["quality_flags"] & (
            ROLLUP_QUALITY_UTC_UNKNOWN | ROLLUP_QUALITY_UTC_BACKWARD | ROLLUP_QUALITY_UTC_MISMATCH))
        start = selected["start_association"]
        rows = deepcopy(self.rows)
        for row in rows:
            if row["record_kind"] == 10 and battle.battle_id(row) == start[:3] and row["battle_revision"] == start[3]:
                row.update(battle_at_utc_usec=contribution.UTC_UNKNOWN,
                    battle_observed_through_utc_usec=contribution.UTC_UNKNOWN, occurrence_utc_usec=contribution.UTC_UNKNOWN)
        result = history.build_history(rows, self.scope)
        self.assertEqual(result.summary["verified_exposure_present_usec"], self.baseline.summary["verified_exposure_present_usec"])
        affected = [row for row in result.exposures if row["source_battle"] == start[:3] and row["start_association"] == start]
        self.assertTrue(affected)
        self.assertTrue(all(row["quality_flags"] & ROLLUP_QUALITY_UTC_UNKNOWN for row in affected))

    def test_unknown_sides_keep_owner_denominators_unknown(self):
        unknown = [row for row in self.baseline.exposures if row["battle_side_status"] != 1]
        self.assertTrue(unknown)
        self.assertTrue(all(row["observed_side_owners"] is None and row["observed_opposing_owners"] is None and
            row["battle_unknown_side_usec"] == row["battle_present_usec"] for row in unknown))

    def test_equivalent_exposure_coalesces_but_composition_change_is_preserved(self):
        # Complete hostile refresh packets keep two actors' observed context.
        # A later class change on their opponent must split both exposures.
        start = next(rows[:5] for rows in self.association_cases.values() if rows[0]["battle_fact_kind"] == 1)
        first, second, relation, cut = start[1:]
        elapsed = 0
        rows = deepcopy(start)

        def append_packet(template, revision, kind, changed_class=False):
            nonlocal elapsed
            elapsed += 10
            sequence = rows[-1]["battle_fact_sequence"] + 1
            packet = [deepcopy(template), deepcopy(cut)]
            for index, row in enumerate(packet):
                row.update(battle_revision=revision, battle_fact_index=index, battle_fact_count=2,
                    battle_fact_sequence=sequence + index, record_seq=sequence + index,
                    battle_at_monotonic_usec=cut["battle_at_monotonic_usec"] + elapsed,
                    battle_observed_through_monotonic_usec=cut["battle_at_monotonic_usec"] + elapsed,
                    battle_last_engagement_monotonic_usec=cut["battle_at_monotonic_usec"] + elapsed,
                    battle_at_utc_usec=cut["battle_at_utc_usec"] + elapsed,
                    battle_observed_through_utc_usec=cut["battle_at_utc_usec"] + elapsed,
                    occurrence_utc_usec=cut["battle_at_utc_usec"] + elapsed)
                if index == 0:
                    row["battle_fact_kind"] = kind
                    for name in battle.EFFORT:
                        row[name] = elapsed if name in ("battle_present_usec", "battle_contributor_usec", "battle_pvp_usec") else 0
                    if changed_class:
                        row["battle_actor_class_id"] += 1
            battle.validate_packet([{name: row[name] for name in battle.FIELDS} for row in packet])
            rows.extend(packet)

        append_packet(relation, 2, 3)
        append_packet(relation, 3, 3)
        coalesced = history.build_history(rows, (11, 22), max_output_rows=5)
        self.assertEqual(len(coalesced.exposures), 2)
        self.assertTrue(all(row["battle_present_usec"] == 20 and row["start_association"][3] == 1 and
            row["through_association"][3] == 3 for row in coalesced.exposures))
        append_packet(second, 4, 2, changed_class=True)
        append_packet(relation, 5, 3)
        changed = history.build_history(rows, (11, 22))
        own = [row for row in changed.exposures if row["battle_actor_id"] == first["battle_actor_id"]]
        self.assertEqual(len(own), 2)
        self.assertEqual({row["battle_actor_class_id"] for row in own}, {first["battle_actor_class_id"]})
        self.assertEqual([row["battle_present_usec"] for row in own], [30, 10])
        self.assertNotEqual(own[0]["observed_roster_digest"], own[1]["observed_roster_digest"])

    def test_association_only_overlap_cannot_duplicate_actor_exposure(self):
        original = next(row for row in self.rows if row.get("battle_fact_kind") == 1)
        facts = [row for row in self.rows if row["record_kind"] == 10 and battle.battle_id(row) == battle.battle_id(original)]
        high = max(row["record_seq"] for row in self.rows)
        sequence = max(row["battle_seq"] for row in self.rows if row["record_kind"] == 10) + 1
        duplicate = [dict(row, battle_seq=sequence, record_seq=row["record_seq"] + high) for row in facts]
        with self.assertRaisesRegex(history.HistoryError, "actor_exposure_overlap"):
            history.build_history(facts + duplicate, self.scope)

    def test_exposure_incident_coverage_uses_association_family(self):
        coverage = {"registry_schema_version": 4, "status": "reviewed_inventory",
            "reviewed_from_utc_usec": -(1 << 63) + 1, "reviewed_through_utc_usec": (1 << 63) - 1, "incidents": []}
        selected = next(row for row in self.baseline.exposures if not row["quality_flags"] & (
            ROLLUP_QUALITY_UTC_UNKNOWN | ROLLUP_QUALITY_UTC_BACKWARD | ROLLUP_QUALITY_UTC_MISMATCH))
        item = {"status": "active", "record_kind_mask": 1 << 10,
            "producer_boot_id": selected["source_battle"][0], "producer_process_id": selected["source_battle"][1],
            "first_record_seq": selected["start_record_seq"], "last_record_seq": selected["through_record_seq"],
            "start_utc_usec": None, "end_utc_usec": None}
        result = history.build_history(self.rows, self.scope, incident_coverage=dict(coverage, incidents=[item]))
        affected = [row for row in result.exposures if row["quality_flags"] & ROLLUP_QUALITY_INCIDENT_GAP]
        self.assertTrue(affected)
        self.assertTrue(all(row["source_battle"][:2] == selected["source_battle"][:2] for row in affected))
        result = history.build_history(self.rows, self.scope, incident_coverage=dict(coverage,
            incidents=[dict(item, record_kind_mask=1 << 11)]))
        self.assertTrue(all(not row["quality_flags"] & ROLLUP_QUALITY_INCIDENT_GAP for row in result.exposures))

    def test_retained_source_round_trip_and_page_checkpoints(self):
        middle = len(self.retained_inputs) // 2
        header = source.advance_header(source.initial_header(self.source_scope), self.retained_inputs[:middle],
            self.retained_inputs[middle - 1]["ingest_id"])
        header = source.advance_header(header, self.retained_inputs[middle:], self.source_window.header["input_watermark"])
        self.assertEqual(header, self.source_window.header)
        skipped = source.advance_header(header, [], header["input_watermark"] + 7)
        self.assertEqual(skipped["source_digest"], header["source_digest"])
        self.assertEqual(skipped["source_fact_count"], header["source_fact_count"])
        restored = source.verify_source(skipped, self.retained_inputs, expected_scope=self.source_scope,
            expected_watermark=skipped["input_watermark"])
        self.assertEqual(history.build_history(restored.facts, self.scope), self.baseline)

    def test_retained_source_missing_and_reordered_rows_are_refused(self):
        for rows, reason in ((self.retained_inputs[:-1], "missing_or_changed"),
                             (list(reversed(self.retained_inputs)), "retained_order_or_cursor")):
            with self.subTest(reason=reason), self.assertRaisesRegex(source.SourceError, reason):
                source.verify_source(self.source_window.header, rows, expected_scope=self.source_scope,
                    expected_watermark=self.source_window.header["input_watermark"])

    def test_retained_source_payload_and_checkpoint_detect_changed_values(self):
        selected = next(row for row in self.retained_inputs if row["record_kind"] == 11)
        packet = json.loads(selected["payload"])
        packet["source"]["bc_damage_dealt"] += 1
        payload = json.dumps(packet, sort_keys=True, separators=(",", ":")).encode()
        changed = dict(selected, payload=payload)
        with self.assertRaisesRegex(source.SourceError, "payload_digest"):
            source.decode_input(changed, self.source_scope)
        changed["payload_digest"] = hashlib.sha256(payload).digest()
        source.decode_input(changed, self.source_scope)
        with self.assertRaisesRegex(source.SourceError, "missing_or_changed"):
            source.verify_source(self.source_window.header, [changed if row is selected else row for row in self.retained_inputs],
                expected_scope=self.source_scope, expected_watermark=self.source_window.header["input_watermark"])

    def test_retained_source_payload_identity_and_scope_are_pinned(self):
        selected = self.retained_inputs[0]
        for name in ("ingest_id", "boot_id", "process_id", "record_seq"):
            with self.subTest(name=name), self.assertRaisesRegex(source.SourceError, "payload_identity"):
                source.decode_input(dict(selected, **{name: selected[name] + 1}), self.source_scope)
        with self.assertRaisesRegex(source.SourceError, "retained_scope_or_columns"):
            source.decode_input(dict(selected, generation=selected["generation"] + 1), self.source_scope)
        with self.assertRaisesRegex(source.SourceError, "state_checkpoint"):
            source.verify_source(self.source_window.header, self.retained_inputs,
                expected_scope=(5, 2, *self.scope), expected_watermark=self.source_window.header["input_watermark"])
        with self.assertRaisesRegex(source.SourceError, "state_checkpoint"):
            source.verify_source(self.source_window.header, self.retained_inputs,
                expected_scope=self.source_scope, expected_watermark=self.source_window.header["input_watermark"] + 1)

    def test_retained_source_requires_exact_canonical_numeric_payload(self):
        selected = self.retained_inputs[0]
        packet = json.loads(selected["payload"])
        malformed = [json.dumps(packet, indent=2).encode(),
            b'{"projection_quality":0,"projection_quality":0,"source":' +
                json.dumps(packet["source"], sort_keys=True, separators=(",", ":")).encode() + b'}']
        for payload in malformed:
            with self.subTest(payload_length=len(payload)), self.assertRaises(source.SourceError):
                source.decode_input(dict(selected, payload=payload, payload_digest=hashlib.sha256(payload).digest()), self.source_scope)
        for packet in (dict(packet, projection_quality=True),
                       dict(packet, source=dict(packet["source"], private_review_authorized=1))):
            payload = json.dumps(packet, sort_keys=True, separators=(",", ":")).encode()
            with self.assertRaises(source.SourceError):
                source.decode_input(dict(selected, payload=payload, payload_digest=hashlib.sha256(payload).digest()), self.source_scope)

    def test_retained_source_preserves_arrival_clock_without_borrowing_occurrence(self):
        raw = dict(self.source_window.facts[0], ingested_utc_usec=123456789)
        retained = source.retain_input(raw, self.source_scope, 0)
        restored = source.decode_input(retained, self.source_scope).source
        self.assertEqual(restored["ingested_utc_usec"], raw["ingested_utc_usec"])
        self.assertEqual(restored["occurrence_utc_usec"], raw["occurrence_utc_usec"])
        del raw["ingested_utc_usec"]
        restored = source.decode_input(source.retain_input(raw, self.source_scope, 0), self.source_scope).source
        self.assertEqual(restored["ingested_utc_usec"], contribution.UTC_UNKNOWN)
        for clock in (True, 1 << 63):
            with self.assertRaises(source.SourceError):
                source.retain_input(dict(raw, ingested_utc_usec=clock), self.source_scope, 0)
    def test_retained_source_duplicate_receipts_cannot_become_more_inputs(self):
        row = dict(self.source_window.facts[0], ingest_id=self.source_window.header["input_watermark"] + 1)
        repeated = source.retain_input(row, self.source_scope, 0)
        with self.assertRaisesRegex(source.SourceError, "duplicate_receipt"):
            source.advance_header(source.initial_header(self.source_scope), self.retained_inputs + [repeated], row["ingest_id"])
        header = source.advance_header(self.source_window.header, [repeated], row["ingest_id"])
        with self.assertRaisesRegex(source.SourceError, "duplicate_receipt"):
            source.verify_source(header, self.retained_inputs + [repeated],
                expected_scope=self.source_scope, expected_watermark=row["ingest_id"])

    def test_battle_catalog_is_independent_of_existing_report_versions(self):
        target = RollupTarget(*self.source_scope)
        self.assertEqual(target.scope_tuple, self.source_scope)
        for version in (1, 2, 3):
            self.assertTrue(report_catalog(version))
        catalog = report_catalog(source.DEFINITION_VERSION)
        self.assertEqual({row["name"] for row in catalog}, set(publication.ROW_KINDS))
        self.assertEqual({row["name"] for row in catalog if row["account_metrics_available"]},
            {"battle_contributions", "battle_exposure"})
        from scripts.telemetry.rollup_definitions import report_definition
        with self.assertRaises(ValueError):
            report_definition("playtime", source.DEFINITION_VERSION)
        for version in (1, 2, 3, 4):
            with self.assertRaises(ValueError):
                report_definition("battle_exposure", version)

    def test_retained_source_origin_is_bound_to_the_selected_window(self):
        origin = 1000
        facts = [dict(row, ingest_id=row["ingest_id"] + origin) for row in self.source_window.facts]
        inputs = [source.retain_input(row, self.source_scope, 0) for row in facts]
        header = source.advance_header(source.initial_header(self.source_scope, origin), inputs, facts[-1]["ingest_id"])
        restored = source.verify_source(header, inputs, expected_scope=self.source_scope,
            expected_watermark=header["input_watermark"], expected_origin=origin)
        self.assertEqual(restored.facts, tuple(facts))
        with self.assertRaisesRegex(source.SourceError, "state_checkpoint"):
            source.verify_source(header, inputs, expected_scope=self.source_scope,
                expected_watermark=header["input_watermark"], expected_origin=origin - 1)
        with self.assertRaisesRegex(source.SourceError, "missing_or_changed"):
            source.verify_source(dict(header, input_origin=origin - 1), inputs, expected_scope=self.source_scope,
                expected_watermark=header["input_watermark"], expected_origin=origin - 1)

    def test_source_preparation_keeps_exact_selected_values_without_playtime_rows(self):
        target = RollupTarget(*self.source_scope)
        page = build_page_contributions(list(self.source_window.facts), target)
        self.assertEqual(page.cursor, self.source_window.header["input_watermark"])
        self.assertEqual(len(page.battle_inputs), 151)
        self.assertEqual(page.output_fanout, 151)
        self.assertFalse(page.sessions or page.player_days or page.cohorts or page.members or page.identity_inputs)
        self.assertEqual(page.observations.output_fanout, 0)
        restored = tuple(source.decode_input(row, self.source_scope).source for row in page.battle_inputs)
        self.assertEqual(restored, self.source_window.facts)
        self.assertGreaterEqual(page.estimated_bytes,
            source.HEADER_BYTE_BOUND + len(restored) * source.PUBLICATION_INPUT_BYTE_BOUND)

    def test_source_preparation_retains_ownership_and_skips_foreign_scope(self):
        row = ownership(environment_id=self.scope[0], season_id=self.scope[1], ingest_id=1)
        foreign = dict(row, environment_id=row["environment_id"] + 1, ingest_id=2, record_seq=2)
        page = build_page_contributions([row, foreign], RollupTarget(*self.source_scope))
        self.assertEqual(page.cursor, 2)
        self.assertEqual(len(page.battle_inputs), 1)
        self.assertEqual(source.decode_input(page.battle_inputs[0], self.source_scope).source,
            {name: row[name] for name in source.SOURCE_COLUMNS[9]})
        header = source.advance_header(source.initial_header(self.source_scope), page.battle_inputs, page.cursor)
        self.assertEqual((header["source_fact_count"], header["ownership_count"], header["input_watermark"]), (1, 1, 2))

    def test_source_preparation_clock_quality_is_separate_from_source_values(self):
        raw = dict(self.source_window.facts[0])
        target = RollupTarget(*self.source_scope)
        page = build_page_contributions([raw], target, prior_coverage_end_utc_usec=raw["occurrence_utc_usec"] + 1)
        decoded = source.decode_input(page.battle_inputs[0], self.source_scope)
        self.assertTrue(decoded.projection_quality & ROLLUP_QUALITY_LATE_INPUT)
        self.assertEqual(decoded.source, raw)
        owned = ownership(environment_id=self.scope[0], season_id=self.scope[1], ingest_id=1,
            occurrence_utc_usec=contribution.UTC_UNKNOWN, at_utc_usec=contribution.UTC_UNKNOWN)
        page = build_page_contributions([owned], target)
        self.assertTrue(page.state_quality_flags & ROLLUP_QUALITY_UTC_UNKNOWN)
        self.assertIsNone(page.coverage_start_utc_usec)
        self.assertIsNone(page.coverage_end_utc_usec)
        self.assertEqual(source.decode_input(page.battle_inputs[0], self.source_scope).source["occurrence_utc_usec"],
            contribution.UTC_UNKNOWN)

    def test_source_preparation_budgets_and_selected_family_faults_refuse_page(self):
        raw = dict(self.source_window.facts[0])
        original = deepcopy(raw)
        target = RollupTarget(*self.source_scope)
        with self.assertRaises(BoundsExceeded):
            build_page_contributions([raw], target, max_page_bytes=source.PUBLICATION_INPUT_BYTE_BOUND)
        with self.assertRaises(BoundsExceeded):
            build_page_contributions(list(self.source_window.facts[:2]), target, max_output_fanout=1)
        with self.assertRaises(SemanticError):
            build_page_contributions([dict(raw, battle_config_id=0)], target)
        self.assertEqual(raw, original)

    def test_retained_ownership_values_stay_separate_from_battle_metrics(self):
        row = ownership(environment_id=self.scope[0], season_id=self.scope[1],
            ingest_id=self.source_window.header["input_watermark"] + 1)
        retained = source.retain_input(row, self.source_scope, ROLLUP_QUALITY_PROCESS_GAP)
        header = source.advance_header(self.source_window.header, [retained], row["ingest_id"])
        restored = source.verify_source(header, self.retained_inputs + [retained],
            expected_scope=self.source_scope, expected_watermark=row["ingest_id"])
        self.assertEqual((header["ownership_count"], header["association_count"], header["contribution_count"]), (1, 123, 28))
        self.assertTrue(header["quality_flags"] & ROLLUP_QUALITY_PROCESS_GAP)
        owned = restored.facts[-1]
        self.assertEqual(owned, {name: row[name] for name in source.SOURCE_COLUMNS[9]})
        self.assertEqual(restored.projection_qualities[-1], ROLLUP_QUALITY_PROCESS_GAP)
        self.assertEqual(history.build_history(list(restored.facts[:-1]), self.scope), self.baseline)
        with self.assertRaisesRegex(source.SourceError, "header_quality"):
            source.verify_source(dict(header, quality_flags=0), self.retained_inputs + [retained],
                expected_scope=self.source_scope, expected_watermark=row["ingest_id"])

    def test_retained_source_bounds_deadline_and_values_do_not_mutate_inputs(self):
        saved, header = deepcopy(self.retained_inputs), deepcopy(self.source_window.header)
        with self.assertRaisesRegex(source.SourceError, "byte_capacity"):
            source.verify_source(header, self.retained_inputs, expected_scope=self.source_scope,
                expected_watermark=header["input_watermark"], max_total_bytes=self.source_window.reserved_bytes - 1)
        with self.assertRaisesRegex(source.SourceError, "input_capacity"):
            source.verify_source(header, [self.retained_inputs[0]] * (source.MAX_INPUTS + 1),
                expected_scope=self.source_scope, expected_watermark=header["input_watermark"])
        def deadline():
            raise TimeoutError("fixture source deadline")
        with self.assertRaisesRegex(TimeoutError, "fixture source deadline"):
            source.advance_header(header, [], header["input_watermark"], check_deadline=deadline)
        restored = source.verify_source(header, self.retained_inputs, expected_scope=self.source_scope,
            expected_watermark=header["input_watermark"])
        restored.facts[0]["battle_seq"] += 1
        restored.header["quality_flags"] |= ROLLUP_QUALITY_PROCESS_GAP
        self.assertEqual(self.retained_inputs, saved)
        self.assertEqual(header, self.source_window.header)

    def test_retained_source_keeps_published_checkpoints_immutable(self):
        header = dict(self.source_window.header, publication_complete=1)
        source.verify_source(header, self.retained_inputs, expected_scope=self.source_scope,
            expected_watermark=header["input_watermark"])
        with self.assertRaisesRegex(source.SourceError, "published_immutable"):
            source.advance_header(header, [], header["input_watermark"] + 1)
        from scripts.telemetry.rollup_definitions import SUPPORTED_DEFINITION_VERSIONS
        self.assertEqual(SUPPORTED_DEFINITION_VERSIONS, {1, 2, 3, 5})

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
