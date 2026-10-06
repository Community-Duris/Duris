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
import re
import subprocess
import sys
import tempfile
import unittest

from test_telemetry_gameplay_adapters import ROOT, compile_gameplay

sys.path.insert(0, str(ROOT))
from scripts.telemetry import battle_history as history
from scripts.telemetry import battle_contract as battle
from scripts.telemetry import battle_contribution_contract as contribution
from scripts.telemetry import battle_build_contract as builds
from scripts.telemetry import battle_comparison as comparison
from scripts.telemetry import control_contract as controls
from scripts.telemetry import battle_result_contract as results
from scripts.telemetry import battle_source as source
from scripts.telemetry import battle_publication as publication, identity_history as identity, incident
from scripts.telemetry.rollup_engine import build_page_contributions, BoundsExceeded, SemanticError
from test_telemetry_observations import ownership
from test_telemetry_battle_contribution_contract import ControlContractTests
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


def qualify_native_control_history(rows, *, expanded=False):
    """Actual helper/runtime capture and its independent canonical SQL readback."""
    scope = next((row["battle_environment_id"], row["battle_season_id"])
                 for row in rows if row["record_kind"] == 10)
    result = history.build_history(rows, scope)
    applications = 17 if expanded else 8
    attacker, target, rejected = (8971, 8972, 8973) if expanded else (8961, 8962, 8963)
    canonical = [row for row in result.battles if row["canonical"]]
    assert len(canonical) == 1 and canonical[0]["available_metric_mask"] == 31
    assert canonical[0]["control_applications"] == canonical[0]["control_received"] == applications
    assert result.summary["verified_contribution_links"] == result.summary["contribution_count"] > 0
    assert result.summary["partial_contribution_links"] == 0
    assert all(row["packet_history_complete"] and row["outcome"] is None and
        not row["complete_metric_coverage_implied"] for row in result.battles)
    assert all(row["battle_actor_id"] != rejected for row in result.actors)
    segments = result.contributions
    assert sum(row["bc_control_applications"] for row in segments) == applications
    assert sum(row["bc_control_received"] for row in segments) == applications
    assert any(row["bc_actor_kind"] == 2 and row["bc_actor_owner_subject_id"] == attacker and
        row["bc_control_applications"] == 1 for row in segments)
    assert any(row["bc_actor_kind"] == 3 and row["bc_control_received"] == 1 and
        row["bc_control_applications"] == (0 if expanded else 1) for row in segments)
    assert any(row["bc_actor_id"] == target and row["bc_modifier_flags"] & 128 and
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
        expanded_controls = cls.path / "expanded-controls.jsonl"
        typed_controls = cls.path / "typed-controls.jsonl"
        subprocess.run([str(executable), "--native-expanded-control-capture"], cwd=ROOT,
            env=dict(os.environ, TELEMETRY_BATTLE_CAPTURE_EXPORT=str(expanded_controls),
                     TELEMETRY_CONTROL_CAPTURE_EXPORT=str(typed_controls)),
            text=True, capture_output=True, check=True, timeout=30)
        cls.expanded_control_rows = [json.loads(line) for line in expanded_controls.read_text(encoding="utf-8").splitlines()]
        cls.typed_control_rows = [json.loads(line) for line in typed_controls.read_text(encoding="utf-8").splitlines()]
        build_points, build_battles = cls.path / "build-points.jsonl", cls.path / "build-battles.jsonl"
        subprocess.run([str(executable), "--native-build-capture"], cwd=ROOT,
            env=dict(os.environ, TELEMETRY_BUILD_CAPTURE_EXPORT=str(build_points), TELEMETRY_BATTLE_CAPTURE_EXPORT=str(build_battles)),
            capture_output=True, check=True, timeout=30)
        cls.build_rows = [json.loads(line) for path in (build_points, build_battles) for line in path.read_text().splitlines()]
        for row in cls.build_rows:
            if row["record_kind"] == 12:
                for name in builds.BYTE_FIELDS:
                    row[name] = bytes.fromhex(row[name])
                builds.validate_raw_observation(row)
        result_files = [cls.path / name for name in ("results.jsonl", "result-battles.jsonl", "result-builds.jsonl")]
        subprocess.run([str(executable), "--native-result-capture"], cwd=ROOT,
            env=dict(os.environ, TELEMETRY_RESULT_CAPTURE_EXPORT=str(result_files[0]),
                TELEMETRY_BATTLE_CAPTURE_EXPORT=str(result_files[1]), TELEMETRY_BUILD_CAPTURE_EXPORT=str(result_files[2])),
            capture_output=True, check=True, timeout=30)
        cls.result_rows = [json.loads(line) for path in result_files for line in path.read_text().splitlines()]
        for row in cls.result_rows:
            for name in results.BYTE_FIELDS if row["record_kind"] == 14 else builds.BYTE_FIELDS if row["record_kind"] == 12 else ():
                row[name] = bytes.fromhex(row[name])
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
        ControlContractTests.setUpClass()
        try:
            associations, control_export = cls.path / "control-associations", cls.path / "control-prefixes.jsonl"
            subprocess.run([str(ControlContractTests.native), "--export-control", str(control_export)],
                env=dict(os.environ, TELEMETRY_CONTROL_ASSOCIATION_EXPORT=str(associations)),
                check=True, capture_output=True, timeout=30)
            cls.control_prefix_rows = []
            for wire in associations.read_text(encoding="ascii").splitlines():
                value = battle.decode_fact(bytes.fromhex(wire))
                cls.control_prefix_rows.append(dict(value, boot_id=value["battle_boot_id"], process_id=value["battle_process_id"],
                    record_seq=len(cls.control_prefix_rows) + 1, schema_version=1, record_kind=10,
                    occurrence_utc_usec=value["battle_at_utc_usec"]))
            for line in control_export.read_text(encoding="ascii").splitlines():
                item = json.loads(line)
                if item["case"] != 2:
                    continue
                value = item["fields"]
                cls.control_prefix_rows.append(dict(value, boot_id=value["ctl_boot_id"], process_id=value["ctl_process_id"],
                    record_seq=len(cls.control_prefix_rows) + 1, schema_version=1, record_kind=13,
                    occurrence_utc_usec=value["ctl_decision_utc_usec"]))
            cls.control_prefix_rows.sort(key=lambda row: (row["occurrence_utc_usec"], row["record_kind"], row["record_seq"]))
            for index, row in enumerate(cls.control_prefix_rows, 1):
                row["record_seq"] = index
        finally:
            ControlContractTests.tearDownClass()

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

    @staticmethod
    def _result_window(rows, *, capture_config=True, projection_quality=0):
        scope = next((row["bout_environment_id"], row["bout_season_id"]) for row in rows if row["record_kind"] == 14)
        target = (source.RESULT_DEFINITION_VERSION, 1, *scope)
        ordered = [dict(row, ingest_id=index) for index, row in enumerate(sorted(rows,
            key=lambda row: (row["boot_id"], row["process_id"], row["record_seq"])), 1)]
        inputs = []
        for row in ordered:
            kind = row["record_kind"]
            prefix = {12: "bctx_", 13: "ctl_", 14: "bout_"}.get(kind)
            config = {name: row[prefix + name] for name in source.configuration_columns(kind)} if (
                capture_config and prefix and row[prefix + "config_id"]) else None
            inputs.append(source.retain_input(row, target, projection_quality, configuration=config))
        header = source.advance_header(source.initial_header(target), inputs, len(ordered))
        return source.verify_source(header, inputs, expected_scope=target, expected_watermark=len(ordered)), inputs

    @staticmethod
    def _result_coverage(rows):
        scope = next((row["bout_environment_id"], row["bout_season_id"]) for row in rows if row["record_kind"] == 14)
        clocks = [row["occurrence_utc_usec"] for row in rows if row["occurrence_utc_usec"] != results.UTC_UNKNOWN]
        packet = dict(incident.template(7), incidents=[], environment_id=scope[0], season_id=scope[1],
            reviewer_token="a" * 64, review_evidence_digest="b" * 64,
            reviewed_from_utc_usec=min(clocks) - 1, reviewed_through_utc_usec=max(clocks) + 1)
        meta, details = incident.validate_packet(packet)
        summary = incident.publication_summary((8, 1, *scope), meta, details)
        return incident.public_coverage(summary, details, registry_schema_version=7)

    def test_result_catalog_requires_current_review_without_rewriting_older_definitions(self):
        catalogs = {version: {item["name"]: item for item in report_catalog(version)} for version in (6, 7, 8)}
        self.assertIn("schema-5", catalogs[6]["battle_build_points"]["denominator"])
        self.assertIn("schema-5", catalogs[7]["battle_build_points"]["denominator"])
        self.assertIn("schema-6", catalogs[7]["battle_control_states"]["denominator"])
        for name in ("battle_build_points", "battle_control_operations", "battle_control_states", "battle_build_comparisons"):
            self.assertIn("schema-7", catalogs[8][name]["denominator"])
            self.assertNotIn("schema-5", catalogs[8][name]["denominator"])
            self.assertNotIn("schema-6", catalogs[8][name]["denominator"])

    def test_native_results_retain_exact_pre_teardown_parent_and_objective_identities(self):
        rows = self.result_rows
        coverage = self._result_coverage(rows)
        window, retained = self._result_window(rows)
        output = publication.build_publication(window, None, coverage)
        values = [publication.decode_row((8, 1, *self.scope), row) for row in output.rows if row["row_kind"] == 9]
        self.assertEqual(output.header["result_count"], 18)
        self.assertEqual(output.header["observed_death_count"], 1)
        self.assertEqual(output.header["observed_escape_count"], 1)
        self.assertEqual(output.header["observed_objective_commit_count"], 2)
        self.assertEqual(output.header["recovered_objective_count"], 1)
        self.assertEqual(output.header["observed_censored_count"], 4)
        self.assertEqual(sum(value["event_evidence_qualified"] for value in values), output.header["qualified_result_evidence_count"])
        death = next(value for value in values if value["bout_kind"] == 1)
        self.assertEqual(death["target_link_status"], "verified")
        self.assertTrue(death["battle_context_qualified"])
        escape = next(value for value in values if value["bout_kind"] == 4)
        self.assertEqual(escape["chain_status"], "verified")
        self.assertEqual(escape["reference_origin"], "native_escape_parent")
        self.assertEqual(escape["parent_event_key"][-1], escape["bout_parent_sequence"])
        for value in values:
            self.assertFalse(value["whole_battle_victory_implied"])
            self.assertFalse(value["full_zone_clear_implied"])
        commits = [value for value in values if value["bout_kind"] == 6]
        self.assertEqual({value["bout_operation_id"] for value in commits}, {bytes(range(1, 17)).hex()})
        self.assertTrue(all(value["target_account_token"] is None for value in commits))
        self.assertEqual(sum(value["event_evidence_qualified"] for value in commits), 1)
        self.assertTrue(all(source.decode_input(value, (8, 1, *self.scope)) for value in retained))
        points = [publication.decode_row((8, 1, *self.scope), row) for row in output.rows if row["row_kind"] == 6]
        self.assertTrue(points)
        self.assertTrue(all(tuple(point["comparison"]["dimensions"]) == tuple(sorted(comparison.DIMENSIONS)) for point in points))
        self.assertTrue(all(point["comparison"]["dimensions"]["support_origin"]["status"] in ("unclassified", "unavailable") for point in points))

    def test_result_missing_parent_config_review_and_source_remain_unknown(self):
        rows = self.result_rows
        scope = (8, 1, *self.scope)
        for capture_config, coverage in ((False, self._result_coverage(rows)), (True, incident.public_coverage(
                None, (), registry_schema_version=7))):
            window, _ = self._result_window(rows, capture_config=capture_config)
            output = publication.build_publication(window, None, coverage)
            self.assertEqual(output.header["qualified_result_evidence_count"], 0)
        without = [row for row in rows if row["record_kind"] != 10]
        window, _ = self._result_window(without)
        output = publication.build_publication(window, None, self._result_coverage(rows))
        self.assertEqual(output.header["qualified_result_context_count"], 0)
        escape = next(row for row in rows if row.get("bout_kind") == 4)
        missing = [row for row in rows if row.get("bout_sequence") != escape["bout_parent_sequence"]]
        window, _ = self._result_window(missing)
        output = publication.build_publication(window, None, self._result_coverage(rows))
        item = next(publication.decode_row(scope, row) for row in output.rows if row["row_kind"] == 9 and
            publication.decode_row(scope, row)["bout_kind"] == 4)
        self.assertEqual(item["chain_status"], "missing_parent")
        self.assertFalse(item["event_evidence_qualified"])

    def test_result_escape_parent_conflict_refused_and_earlier_definitions_sealed(self):
        rows = deepcopy(self.result_rows)
        escape = next(row for row in rows if row.get("bout_kind") == 4)
        escape["bout_target_level_band"] += 1
        with self.assertRaisesRegex(history.HistoryError, "escape_parent_conflict"):
            history.build_history(rows, self.scope, incident_schema_version=7)
        native = next(row for row in self.result_rows if row["record_kind"] == 14)
        for definition in (5, 6, 7):
            with self.assertRaises(source.SourceError):
                source.retain_input(dict(native, ingest_id=1), (definition, 1, *self.scope), 0)
        for schema in (4, 5, 6):
            with self.assertRaises(history.HistoryError):
                history.build_history(self.result_rows, self.scope, incident_schema_version=schema)

    def test_result_projection_quality_is_retained_and_cannot_be_erased(self):
        rows = self.result_rows
        window, _ = self._result_window(rows, projection_quality=ROLLUP_QUALITY_PROCESS_GAP)
        output = publication.build_publication(window, None, self._result_coverage(rows))
        self.assertEqual(output.header["qualified_result_evidence_count"], 0)
        self.assertEqual(output.header["qualified_build_points"], 0)
        scope = (8, 1, *self.scope)
        values = [publication.decode_row(scope, row) for row in output.rows if row["row_kind"] == 9]
        self.assertTrue(all(row["event_quality_flags"] & ROLLUP_QUALITY_PROCESS_GAP for row in values))
        point = next(row for row in values if row["bout_quality_flags"])
        with self.assertRaisesRegex(publication.PublicationError, "quality_erasure"):
            publication.retain_row(scope, 9, dict(point, event_quality_flags=0))
        with self.assertRaisesRegex(publication.PublicationError, "quality_erasure"):
            publication.retain_row(scope, 9, dict(point, publication_quality_flags=0))

    def test_result_clocks_loss_stale_reference_and_public_conclusions(self):
        scope = (8, 1, *self.scope)
        rows = deepcopy(self.result_rows)
        death = next(row for row in rows if row.get("bout_kind") == 1)
        death["bout_at_utc_usec"] += 1
        death["occurrence_utc_usec"] = death["bout_at_utc_usec"]
        window, _ = self._result_window(rows)
        output = publication.build_publication(window, None, self._result_coverage(rows))
        values = [publication.decode_row(scope, row) for row in output.rows if row["row_kind"] == 9]
        point = next(row for row in values if row["bout_kind"] == 1)
        self.assertEqual(point["clock_status"], "mismatch")
        self.assertFalse(point["event_evidence_qualified"])
        for change in ({"whole_battle_victory_implied": True}, {"full_zone_clear_implied": True},
                {"event_evidence_qualified": True}):
            with self.assertRaises(publication.PublicationError):
                publication.retain_row(scope, 9, dict(point, **change))
        with self.assertRaisesRegex(history.HistoryError, "history_output_capacity"):
            publication.build_publication(window, None, self._result_coverage(rows), max_output_rows=1)
        rows = deepcopy(self.result_rows)
        death = next(row for row in rows if row.get("bout_target_association_revision", 0) > 1 and
            any(basis["record_kind"] == 10 and basis["battle_fact_kind"] == 4 and
                basis["battle_seq"] == row["bout_target_battle_seq"] and
                basis["battle_revision"] < row["bout_target_association_revision"] for basis in rows))
        older = [row for row in rows if row["record_kind"] == 10 and row["battle_fact_kind"] == 4 and
            row["battle_seq"] == death["bout_target_battle_seq"] and row["battle_revision"] < death["bout_target_association_revision"]]
        self.assertTrue(older)
        death["bout_target_association_revision"] = older[-1]["battle_revision"]
        death["bout_target_association_fact_sequence"] = older[-1]["battle_fact_sequence"]
        reduced = history.build_history(rows, self.scope, incident_schema_version=7)
        self.assertEqual(next(row for row in reduced.results if row["bout_sequence"] == death["bout_sequence"])["target_link_status"], "stale_association")
        rows = self.result_rows
        coverage = self._result_coverage(rows)
        escape = next(row for row in rows if row.get("bout_kind") == 4)
        loss = dict(incident.template(7)["incidents"][0], producer_boot_id=escape["boot_id"],
            producer_process_id=escape["process_id"], record_kind_mask=1 << 14,
            start_utc_usec=escape["bout_start_utc_usec"], end_utc_usec=escape["bout_at_utc_usec"],
            first_record_seq=None, last_record_seq=None)
        coverage = dict(coverage, incidents=[loss])
        window, _ = self._result_window(rows)
        output = publication.build_publication(window, None, coverage)
        point = next(publication.decode_row(scope, row) for row in output.rows if row["row_kind"] == 9 and
            publication.decode_row(scope, row)["bout_kind"] == 4)
        self.assertFalse(point["event_evidence_qualified"])
        self.assertTrue(point["event_quality_flags"] & ROLLUP_QUALITY_INCIDENT_GAP)

    def test_result_objective_missing_duplicate_unsupported_and_conflicting_receipts(self):
        scope = (8, 1, *self.scope)
        original = deepcopy(self.result_rows)
        commit = next(row for row in original if row.get("bout_kind") == 6 and not row["bout_flags"] & 256)
        rows = [row for row in original if row.get("bout_kind") != 5]
        window, _ = self._result_window(rows)
        output = publication.build_publication(window, None, self._result_coverage(original))
        item = next(publication.decode_row(scope, row) for row in output.rows if row["row_kind"] == 9 and
            publication.decode_row(scope, row)["bout_sequence"] == commit["bout_sequence"])
        self.assertEqual(item["objective_status"], "missing_request")
        self.assertFalse(item["event_evidence_qualified"])
        rows = deepcopy(original)
        duplicate = dict(commit, record_seq=max(row["record_seq"] for row in rows)+1,
            bout_sequence=max(row.get("bout_sequence", 0) for row in rows)+1)
        rows.append(duplicate)
        window, _ = self._result_window(rows)
        output = publication.build_publication(window, None, self._result_coverage(rows))
        self.assertEqual(output.header["duplicate_objective_count"], 1)
        self.assertEqual(output.header["qualified_objective_commit_count"], 1)
        for updates, expected in (({"bout_authority": 8}, "unsupported_authority"),
                ({"bout_content_version": commit["bout_content_version"]+1}, "configuration_changed")):
            rows = [dict(row, **updates) if row is commit else row for row in original]
            window, _ = self._result_window(rows)
            output = publication.build_publication(window, None, self._result_coverage(rows))
            item = next(publication.decode_row(scope, row) for row in output.rows if row["row_kind"] == 9 and
                publication.decode_row(scope, row)["bout_sequence"] == commit["bout_sequence"])
            self.assertEqual(item["objective_status"], expected)
            self.assertFalse(item["event_evidence_qualified"])
        rows = [dict(row, bout_participant_count=row["bout_participant_count"]+1) if row is commit else row for row in original]
        with self.assertRaisesRegex(history.HistoryError, "objective_receipt_conflict"):
            history.build_history(rows, self.scope, incident_schema_version=7)
        uncertain = next(row for row in original if row.get("bout_kind") == 7 and row["bout_reason"] == 12)
        rows = [*original, dict(uncertain, bout_reason=6, record_seq=max(row["record_seq"] for row in original)+1)]
        with self.assertRaisesRegex(history.HistoryError, "history_logical_receipt_conflict"):
            history.build_history(rows, self.scope, incident_schema_version=7)

    @staticmethod
    def _build_window(rows, *, capture_config=True):
        scope = next((row["bctx_environment_id"], row["bctx_season_id"]) for row in rows if row["record_kind"] == 12)
        target = (source.BUILD_DEFINITION_VERSION, 1, *scope)
        ordered = [dict(row, ingest_id=index) for index, row in enumerate(sorted(rows,
            key=lambda row: (row["boot_id"], row["process_id"], row["record_seq"])), 1)]
        inputs = [source.retain_input(row, target, 0, configuration={name: row["bctx_" + name] for name in source.CONFIG_COLUMNS}
            if capture_config and row["record_kind"] == 12 and row["bctx_config_id"] else None) for row in ordered]
        header = source.advance_header(source.initial_header(target), inputs, len(ordered))
        return source.verify_source(header, inputs, expected_scope=target, expected_watermark=len(ordered)), inputs

    def _dated_build_rows(self):
        # Coherent labels are an explicit clock fixture, separate from the native
        # points. No continuous build interval or damage attribution is created.
        rows = deepcopy(self.build_rows)
        epoch = 4 * 86_400_000_000
        for row in rows:
            prefix = {10: "battle_", 11: "bc_", 12: "bctx_"}[row["record_kind"]]
            for clock in ("at", "observed_through") if row["record_kind"] == 10 else (
                    ("start", "observed_through", "decision") if row["record_kind"] == 11 else ("at",)):
                row[prefix + clock + "_utc_usec"] = epoch + row[prefix + clock + "_monotonic_usec"]
            row["occurrence_utc_usec"] = row[prefix + ("decision" if row["record_kind"] == 11 else "at") + "_utc_usec"]
        packet = dict(incident.template(5), incidents=[], environment_id=rows[0]["bctx_environment_id"],
            season_id=rows[0]["bctx_season_id"], reviewer_token="a" * 64, review_evidence_digest="b" * 64,
            reviewed_from_utc_usec=min(row["occurrence_utc_usec"] for row in rows) - 1,
            reviewed_through_utc_usec=max(row["occurrence_utc_usec"] for row in rows) + 1)
        meta, details = incident.validate_packet(packet)
        summary = incident.publication_summary((6, 1, packet["environment_id"], packet["season_id"]), meta, details)
        return rows, incident.public_coverage(summary, details, registry_schema_version=5)

    @staticmethod
    def _control_window(rows, *, capture_config=True):
        scope = next((row["ctl_environment_id"], row["ctl_season_id"]) for row in rows if row["record_kind"] == 13)
        target = (source.CONTROL_DEFINITION_VERSION, 1, *scope)
        ordered = [dict(row, ingest_id=index) for index, row in enumerate(sorted(rows,
            key=lambda row: (row["boot_id"], row["process_id"], row["record_seq"])), 1)]
        inputs = [source.retain_input(row, target, 0, configuration={name: row["ctl_" + name]
            for name in source.CONTROL_CONFIG_COLUMNS} if capture_config and row["record_kind"] == 13 and row["ctl_config_id"] else None)
            for row in ordered]
        header = source.advance_header(source.initial_header(target), inputs, len(ordered))
        return source.verify_source(header, inputs, expected_scope=target, expected_watermark=len(ordered)), inputs

    def test_native_typed_control_exact_source_and_versioned_publication(self):
        rows = [*self.expanded_control_rows, *self.typed_control_rows]
        window, retained = self._control_window(rows)
        scope = tuple(window.header[name] for name in source.SCOPE)
        output = publication.build_publication(window, None, incident.public_coverage(None, [], registry_schema_version=6))
        values = [publication.decode_row(scope, row) for row in output.rows if row["row_kind"] in (7, 8)]
        self.assertEqual(len(values), len(self.typed_control_rows))
        self.assertEqual({controls.observation_key(row): {name: row[name] for name in controls.FIELDS} for row in values},
            {controls.observation_key(row): {name: row[name] for name in controls.FIELDS} for row in self.typed_control_rows})
        self.assertEqual(sum(row["accepted_application_count"] for row in values), 18)
        self.assertEqual(output.header["control_operation_count"], 56)
        self.assertEqual(output.header["control_interval_count"], sum(row["ctl_kind"] == 3 for row in values))
        self.assertTrue(all(row["qualified_duration_mask"] == 0 and row["qualified_status_usec"] == [None] * 8 and
            row["proven_action_restriction_usec"] is None and row["caster_attributed_duration_usec"] is None for row in values))
        self.assertTrue(all(source.decode_input(row, scope).configuration is not None for row in retained if row["record_kind"] == 13))
        self.assertEqual({row["name"] for row in report_catalog(7)}, set(publication.CONTROL_ROW_KINDS))
        self.assertEqual({row["name"] for row in report_catalog(6)}, set(publication.BUILD_ROW_KINDS))

    def test_control_missing_source_configuration_and_loss_remain_explicit(self):
        rows = [*self.expanded_control_rows, *self.typed_control_rows]
        window, retained = self._control_window(rows, capture_config=False)
        scope = tuple(window.header[name] for name in source.SCOPE)
        coverage = incident.public_coverage(None, [], registry_schema_version=6)
        output = publication.build_publication(window, None, coverage)
        self.assertEqual(output.header["configuration_unknown_control_points"], len(self.typed_control_rows))
        self.assertTrue(all(publication.decode_row(scope, row)["configuration_status"] == "unknown"
            for row in output.rows if row["row_kind"] in (7, 8)))
        without_associations = [row for row in rows if row["record_kind"] != 10]
        missing, _ = self._control_window(without_associations)
        output = publication.build_publication(missing, None, coverage)
        self.assertEqual(output.header["verified_control_target_links"], 0)
        self.assertEqual(output.header["partial_control_target_links"], len(self.typed_control_rows))
        with self.assertRaisesRegex(publication.PublicationError, "incident_schema"):
            publication.build_publication(window, None, incident.public_coverage(None, [], registry_schema_version=5))
        modified = deepcopy(retained)
        selected = next(row for row in modified if row["record_kind"] == 13)
        selected["payload"] = selected["payload"].replace(b'"ctl_configured_ticks":', b'"changed_ticks":')
        with self.assertRaisesRegex(source.SourceError, "digest"):
            source.verify_source(window.header, modified, expected_scope=scope, expected_watermark=window.header["input_watermark"])

    def test_control_public_tampering_and_bounded_publication_refuse(self):
        window, _ = self._control_window([*self.expanded_control_rows, *self.typed_control_rows])
        scope = tuple(window.header[name] for name in source.SCOPE)
        coverage = incident.public_coverage(None, [], registry_schema_version=6)
        output = publication.build_publication(window, None, coverage)
        original = next(publication.decode_row(scope, row) for row in output.rows if row["row_kind"] == 7)
        for changes in ({"proven_action_restriction_usec": 1}, {"caster_attributed_duration_usec": 1},
                {"accepted_application_count": 99}, {"qualified_duration_mask": 255},
                {"qualified_status_usec": [0] * 8}, {"source_account_token": 999, "source_controller_token": 999}):
            with self.subTest(changes=changes), self.assertRaises(publication.PublicationError):
                publication.retain_row(scope, 7, dict(original, **changes))
        with self.assertRaisesRegex((publication.PublicationError, history.HistoryError), "capacity"):
            publication.build_publication(window, None, coverage, max_output_rows=1)
        with self.assertRaisesRegex(publication.PublicationError, "byte_capacity"):
            publication.build_publication(window, None, coverage, max_total_bytes=1)

    def _control_prefix_publication(self, rows=None, *, registry=None, losses=()):
        rows = deepcopy(self.control_prefix_rows if rows is None else rows)
        window, _ = self._control_window(rows)
        scope = tuple(window.header[name] for name in source.SCOPE)
        packet = dict(incident.template(6), incidents=list(losses), environment_id=scope[2], season_id=scope[3],
            reviewer_token="a" * 64, review_evidence_digest="b" * 64,
            reviewed_from_utc_usec=min(row["occurrence_utc_usec"] for row in rows) - 1,
            reviewed_through_utc_usec=max(row["occurrence_utc_usec"] for row in rows) + 1)
        meta, details = incident.validate_packet(packet)
        details = tuple(dict(row, occurrence_relation=incident.occurrence_relation(row,
            packet["reviewed_from_utc_usec"], packet["reviewed_through_utc_usec"])) for row in details)
        coverage = incident.public_coverage(incident.publication_summary(scope, meta, details), details, registry_schema_version=6)
        output = publication.build_publication(window, registry, coverage)
        values = [publication.decode_row(scope, row) for row in output.rows if row["row_kind"] == 8]
        return output, values

    def test_control_native_accumulator_prefixes_qualify_disjoint_overlap(self):
        output, values = self._control_prefix_publication()
        prefixes = [row for row in values if row["ctl_kind"] == 3]
        self.assertEqual(output.header["qualified_control_prefixes"], 5)
        self.assertTrue(all(row["qualified_duration_mask"] == 255 for row in prefixes))
        self.assertEqual(sum(row["qualified_status_usec"][2] for row in prefixes), 200)
        self.assertEqual(sum(row["qualified_status_usec"][3] for row in prefixes), 200)
        self.assertEqual(sum(row["observed_prefix_usec"] for row in prefixes if row["ctl_before_mask"]), 300)
        self.assertEqual((prefixes[-1]["ctl_at_usec"], prefixes[-1]["ctl_decision_usec"]), (550, 600))
        self.assertEqual(prefixes[-1]["observed_prefix_usec"], 50)
        self.assertTrue(all(row["proven_action_restriction_usec"] is None and
            row["caster_attributed_duration_usec"] is None for row in values))

    def test_control_missing_predecessor_and_partial_family_do_not_invent_zero(self):
        rows = deepcopy(self.control_prefix_rows)
        entry = next(row for row in rows if row.get("ctl_kind") == 2)
        rows.remove(entry)
        _, values = self._control_prefix_publication(rows)
        first = next(row for row in values if row["ctl_previous_state_sequence"] == entry["ctl_sequence"])
        self.assertEqual(first["chain_status"], "missing_predecessor")
        self.assertEqual(first["qualified_status_usec"], [None] * 8)
        rows = deepcopy(self.control_prefix_rows)
        for row in rows:
            if row["record_kind"] == 13:
                row["ctl_duration_coverage"] = 12
                row["ctl_quality_flags"] |= 1  # Native CONTEXT_UNKNOWN marks partial families.
        _, values = self._control_prefix_publication(rows)
        self.assertTrue(all(row["qualified_status_usec"] == [None] * 8 for row in values))

    def test_control_clock_source_loss_and_configuration_cuts_censor_duration(self):
        for updates in ({"ctl_quality_flags": battle.QUALITY_CLOCK_DISCONTINUITY},
                {"ctl_at_utc_usec": controls.UTC_UNKNOWN},
                {"ctl_at_utc_usec": 1, "ctl_quality_flags": battle.QUALITY_CLOCK_DISCONTINUITY}):
            rows = deepcopy(self.control_prefix_rows)
            selected = next(row for row in rows if row.get("ctl_before_mask") == 12)
            selected.update(updates)
            # A point used as the next predecessor must carry the same clock;
            # select its own public prefix in an independently truncated stream.
            rows = [row for row in rows if row["record_seq"] <= selected["record_seq"]]
            _, values = self._control_prefix_publication(rows)
            found = next(row for row in values if row["ctl_sequence"] == selected["ctl_sequence"])
            self.assertEqual(found["qualified_duration_mask"], 0)
            self.assertEqual(found["qualified_status_usec"], [None] * 8)
        selected = next(row for row in self.control_prefix_rows if row.get("ctl_before_mask") == 12)
        for family in (10, 13):
            loss = dict(incident.template(6)["incidents"][0], producer_boot_id=selected["boot_id"],
                producer_process_id=selected["process_id"], record_kind_mask=1 << family,
                start_utc_usec=selected["ctl_start_utc_usec"], end_utc_usec=selected["ctl_at_utc_usec"],
                first_record_seq=None, last_record_seq=None, evidence_digest="c" * 64)
            _, values = self._control_prefix_publication(losses=[loss])
            found = next(row for row in values if row["ctl_sequence"] == selected["ctl_sequence"])
            self.assertEqual(found["qualified_duration_mask"], 0)
            self.assertTrue(found["publication_quality_flags"] & ROLLUP_QUALITY_INCIDENT_GAP)

    def test_control_conflicting_chain_and_overlapping_target_prefixes_refuse(self):
        rows = deepcopy(self.control_prefix_rows)
        selected = next(row for row in rows if row.get("ctl_before_mask") == 12)
        selected["ctl_before_mask"] = 4
        with self.assertRaisesRegex(history.HistoryError, "chain_conflict"):
            self._control_prefix_publication(rows)
        rows = deepcopy(self.control_prefix_rows)
        duplicate = dict(next(row for row in rows if row.get("ctl_before_mask") == 12),
            ctl_sequence=1000, record_seq=1000)
        rows.append(duplicate)
        with self.assertRaisesRegex(history.HistoryError, "target_overlap"):
            self._control_prefix_publication(rows)

    def test_control_missing_departure_or_close_keeps_lifecycle_unknown(self):
        rows = [row for row in deepcopy(self.control_prefix_rows) if row["record_kind"] != 10 or
            row["battle_at_monotonic_usec"] != 600]
        _, values = self._control_prefix_publication(rows)
        last = next(row for row in values if row["ctl_boundary"] == 5)
        self.assertEqual(last["target_link_status"], "missing_lifecycle")
        self.assertEqual(last["qualified_status_usec"], [None] * 8)
        last = next(row for row in rows if row.get("ctl_boundary") == 5)
        last["ctl_boundary"] = 6
        _, values = self._control_prefix_publication(rows)
        self.assertEqual(next(row for row in values if row["ctl_boundary"] == 6)["target_link_status"], "missing_lifecycle")

    def test_control_departure_clock_must_match_actual_lifecycle(self):
        rows = deepcopy(self.control_prefix_rows)
        for row in rows:
            if row["record_kind"] == 13 and row["ctl_boundary"] == 5:
                row["ctl_decision_utc_usec"] += 1
                row["occurrence_utc_usec"] = row["ctl_decision_utc_usec"]
        _output, values = self._control_prefix_publication(rows)
        last = next(row for row in values if row["ctl_boundary"] == 5)
        self.assertEqual(last["target_link_status"], "lifecycle_mismatch")
        self.assertEqual(last["qualified_status_usec"], [None] * 8)

    def test_control_dated_identity_never_borrows_a_later_owner(self):
        rows = deepcopy(self.control_prefix_rows)
        entry = next(row for row in rows if row.get("ctl_kind") == 2)
        for index, (at, token) in enumerate(((100, 101), (350, 102)), len(rows) + 1):
            rows.append(ownership(boot_id=entry["boot_id"], process_id=entry["process_id"], record_seq=index,
                environment_id=11, season_id=22, config_id=33, classifier_version=4, policy_version=5,
                subject_id=72, pid=72, session_boot_id=101, session_process_id=202, session_seq=72,
                connection_boot_id=101, connection_process_id=202, connection_seq=index,
                at_monotonic_usec=at, at_utc_usec=entry["ctl_at_utc_usec"] + at - 100,
                occurrence_utc_usec=entry["ctl_at_utc_usec"] + at - 100, ownership_account_token=token))
        registry = identity.Registry(11, 22, 1, 0, None, entry["ctl_at_utc_usec"] - 1,
            entry["ctl_at_utc_usec"] + 1000, entry["ctl_at_utc_usec"] + 1001, "a" * 64, "b" * 64,
            tuple(identity.Association(index, token, controller, entry["ctl_at_utc_usec"] - 1, None,
                "confirmed", "staff_review", "c" * 64) for index, (token, controller) in enumerate(((101, 701), (102, 702)), 1)))
        _, values = self._control_prefix_publication(rows, registry=registry)
        early = next(row for row in values if row["ctl_kind"] == 3 and row["ctl_at_usec"] == 300)
        crossing = next(row for row in values if row["ctl_kind"] == 3 and row["ctl_at_usec"] == 400)
        late = next(row for row in values if row["ctl_kind"] == 3 and row["ctl_at_usec"] == 500)
        self.assertEqual((early["target_account_token"], early["target_controller_token"]), (101, 701))
        self.assertEqual(crossing["target_attribution_status"], "identity_changes_inside_prefix")
        self.assertIsNone(crossing["target_account_token"])
        self.assertEqual((late["target_account_token"], late["target_controller_token"]), (102, 702))

    def _comparison_point(self):
        rows, coverage = self._dated_build_rows()
        window, _ = self._build_window(rows)
        scope = tuple(window.header[name] for name in source.SCOPE)
        output = publication.build_publication(window, None, coverage)
        points = [publication.decode_row(scope, row) for row in output.rows if row["row_kind"] == 6]
        qualified = [row for row in points if row["point_context_verified"] and row["bctx_actor_kind"] == 1]
        self.assertTrue(qualified)
        return max(qualified, key=lambda row: row["bctx_available"].bit_count())

    def test_comparison_catalog_uses_native_masks_and_active_spec_cells(self):
        defines = (ROOT / "src/core/defines.h").read_text()
        bits = [int(value) for value in re.findall(r"^#define CLASS_\w+ BIT_(\d+)\s*$", defines, re.MULTILINE)]
        self.assertEqual(bits, list(range(1, 31)))
        self.assertEqual(comparison.CLASS_MASK, sum(1 << (bit - 1) for bit in bits))
        common = (ROOT / "src/core/common.c").read_text()
        block = common.split("const char *specdata[][MAX_SPEC] = {", 1)[1].split("};", 1)[0]
        active = tuple(tuple(index for index, value in enumerate(re.findall(r'"([^"\\]*)"', row), 1)
            if value and value != "Not Used") for row in re.findall(r"\{([^{}]*)\}", block))
        self.assertEqual(active, comparison.SPECIALIZATIONS)

    def test_comparison_preserves_source_point_and_separate_dimensions(self):
        point = self._comparison_point()
        saved = deepcopy(point)
        result = comparison.classify_point(point)
        self.assertEqual(result["point_key"], list(builds.observation_key(point)))
        self.assertEqual(result["source_battle"], [point[name] for name in builds.BATTLE])
        self.assertEqual(result["canonical_battle"], list(point["canonical_battle"]))
        self.assertEqual(result["association"], [point["bctx_association_revision"], point["bctx_association_fact_sequence"]])
        self.assertEqual(tuple(result["dimensions"]), comparison.DIMENSIONS)
        for name in ("level", "base_setup", "effective_setup"):
            self.assertTrue(result["dimensions"][name]["usable_for_matching"])
        self.assertEqual(result["dimensions"]["level"]["values"], {"bctx_level": point["bctx_level"]})
        self.assertEqual(result["dimensions"]["base_setup"]["values"]["bctx_base_str"], point["bctx_base_str"])
        self.assertFalse(result["level_is_combat_strength"])
        self.assertFalse(result["applied_equipment_effects_implied"])
        self.assertFalse(result["continuous_build_exposure_implied"])
        self.assertFalse(result["complete_intrinsic_setup_implied"])
        self.assertFalse(result["contributions_attributed_to_builds"])
        self.assertLessEqual(len(json.dumps(result, sort_keys=True, separators=(",", ":")).encode()), publication.MAX_PAYLOAD_BYTES)
        self.assertEqual(point, saved)

    def test_comparison_requires_selected_dimensions_and_same_configuration(self):
        point = self._comparison_point()
        same = comparison.compare_points(point, point, ("level", "base_setup"))
        self.assertTrue(same["selected_observed_dimensions_match"])
        self.assertEqual(same["matched_dimensions"], ["level", "base_setup"])
        self.assertFalse(same["combat_strength_equivalence_implied"])
        changed = dict(point, bctx_base_str=point["bctx_base_str"] - 1 if point["bctx_base_str"] == 32767 else point["bctx_base_str"] + 1)
        result = comparison.compare_points(point, changed, ("level", "base_setup"))
        self.assertEqual((result["matched_dimensions"], result["different_dimensions"]), (["level"], ["base_setup"]))
        self.assertFalse(result["selected_observed_dimensions_match"])
        for field in ("bctx_environment_id", "bctx_season_id", "bctx_config_id", "bctx_build_version", "bctx_content_version"):
            with self.subTest(field=field):
                result = comparison.compare_points(point, dict(point, **{field: point[field] + 1}), ("level",))
                self.assertFalse(result["configuration_matches"])
                self.assertEqual(result["unknown_dimensions"], ["level"])
        for selected in ((), ("level", "level"), ("power",), "level", (1,)):
            with self.subTest(selected=selected), self.assertRaisesRegex(comparison.ComparisonError, "dimension_selection"):
                comparison.compare_points(point, point, selected)

    def test_comparison_stale_missing_clock_config_and_loss_never_match(self):
        point = self._comparison_point()
        for field, value in (("link_status", "stale_association"), ("link_status", "missing_packet"),
                ("link_status", "partial_history"), ("link_status", "outside_observed_prefix"),
                ("point_clock_status", "unknown"), ("point_clock_status", "mismatch"),
                ("point_clock_status", "discontinuous"), ("configuration_status", "unknown"),
                ("configuration_status", "unavailable"), ("publication_quality_flags", ROLLUP_QUALITY_PROCESS_GAP),
                ("publication_quality_flags", ROLLUP_QUALITY_INCIDENT_GAP),
                ("publication_quality_flags", ROLLUP_QUALITY_INCIDENT_INVENTORY_UNKNOWN)):
            with self.subTest(field=field, value=value):
                changed = dict(point, point_context_verified=False, **{field: value})
                result = comparison.classify_point(changed)
                self.assertFalse(result["point_context_verified"])
                self.assertEqual(result["dimensions"]["level"]["values"], {"bctx_level": point["bctx_level"]})
                self.assertFalse(result["dimensions"]["level"]["usable_for_matching"])
                match = comparison.compare_points(point, changed, ("level",))
                self.assertEqual(match["unknown_dimensions"], ["level"])
                self.assertFalse(match["selected_observed_dimensions_match"])

    def test_comparison_unavailable_equipment_and_partial_effects_are_explicit(self):
        point = self._comparison_point()
        absent = dict(point, bctx_available=point["bctx_available"] & ~32,
            bctx_context_quality=point["bctx_context_quality"] | 2)
        for field in builds.EQUIPMENT_FIELDS:
            absent[field] = "0" * 64 if field in builds.BYTE_FIELDS else 0
        dimensions = comparison.classify_point(absent)["dimensions"]
        self.assertEqual(dimensions["equipment"], {"status": "unavailable", "values": None, "usable_for_matching": False})
        self.assertTrue(dimensions["base_setup"]["usable_for_matching"])
        self.assertEqual(comparison.compare_points(absent, absent, ("equipment",))["unknown_dimensions"], ["equipment"])
        partial = dict(point, bctx_available=point["bctx_available"] | 128, bctx_affect_nodes=64,
            bctx_affects_complete=0, bctx_context_quality=(point["bctx_context_quality"] & ~8) | 4)
        result = comparison.classify_point(partial)
        self.assertEqual(result["dimensions"]["listed_effects"]["status"], "partial")
        self.assertEqual(result["dimensions"]["listed_effects"]["values"]["bctx_affect_nodes"], 64)
        self.assertFalse(result["dimensions"]["listed_effects"]["usable_for_matching"])
        for name in ("temporary_effect_origin", "support_origin"):
            self.assertEqual(result["dimensions"][name], {"status": "unclassified", "values": None, "usable_for_matching": False})
            self.assertEqual(comparison.compare_points(point, point, (name,))["unknown_dimensions"], [name])

    def test_comparison_unknown_class_and_retired_spec_preserve_raw_values(self):
        point = self._comparison_point()
        unknown = dict(point, bctx_primary_class_mask=1 << 31, bctx_specialization=0)
        result = comparison.classify_point(unknown)
        self.assertEqual(result["dimensions"]["classes"]["status"], "unclassified")
        self.assertEqual(result["dimensions"]["classes"]["values"]["bctx_primary_class_mask"], 1 << 31)
        self.assertTrue(result["dimensions"]["level"]["usable_for_matching"])
        for primary, spec in (((1 << 12), 3), (1 << 4, 4), (3, 1), (1 << 13, 1)):
            with self.subTest(primary=primary, spec=spec):
                changed = dict(point, bctx_primary_class_mask=primary, bctx_specialization=spec)
                family = comparison.classify_point(changed)["dimensions"]["specialization"]
                self.assertEqual(family["status"], "unclassified")
                self.assertFalse(family["usable_for_matching"])
                self.assertEqual(family["values"]["bctx_specialization"], spec)
        rogue_archer = dict(point, bctx_primary_class_mask=1 << 12, bctx_specialization=4)
        self.assertTrue(comparison.classify_point(rogue_archer)["dimensions"]["specialization"]["usable_for_matching"])

    def test_comparison_refuses_forged_qualification_digest_and_canonical_identity(self):
        point = self._comparison_point()
        for changes in ({"point_context_verified": False}, {"bctx_equipment_digest": "f" * 63},
                {"bctx_equipment_digest": "F" * 64}, {"publication_quality_flags": 1 << 63},
                {"canonical_battle": [point["bctx_battle_boot_id"] + 1, point["bctx_battle_process_id"], point["bctx_battle_seq"]]},
                {"canonical_battle": [*point["canonical_battle"][:2], point["bctx_battle_seq"] + 1]}):
            with self.subTest(changes=changes), self.assertRaises((comparison.ComparisonError, builds.BuildContractError)):
                comparison.classify_point(dict(point, **changes))

    def test_native_build_points_retain_all_fields_and_publish_separately(self):
        rows, coverage = self._dated_build_rows()
        window, inputs = self._build_window(rows)
        scope = tuple(window.header[name] for name in source.SCOPE)
        self.assertEqual(window.header["build_count"], 20)
        for raw, decoded in zip(sorted(rows, key=lambda row: (row["boot_id"], row["process_id"], row["record_seq"])), window.facts, strict=True):
            self.assertTrue(all(decoded[name] == value for name, value in raw.items()))
        output = publication.build_publication(window, None, coverage)
        points = [publication.decode_row(scope, row) for row in output.rows if row["row_kind"] == 6]
        self.assertEqual(len(points), 20)
        self.assertEqual(output.header["verified_build_links"] + output.header["partial_build_links"], 20)
        self.assertGreater(output.header["qualified_build_points"], 0, [(row["bctx_sequence"], row["link_status"],
            row["point_clock_status"], row["configuration_status"], row["publication_quality_flags"]) for row in points])
        self.assertGreater(output.header["partial_build_links"], 0)
        self.assertTrue(any(row["link_status"] == "outside_observed_prefix" for row in points))
        self.assertTrue(all(not row["continuous_build_exposure_implied"] for row in points))
        original = {builds.observation_key(row): row for row in rows if row["record_kind"] == 12}
        for row in points:
            expected = original[builds.observation_key(row)]
            self.assertTrue(all(row[name] == (expected[name].hex() if name in builds.BYTE_FIELDS else expected[name]) for name in builds.FIELDS))
        self.assertEqual(output.header["unavailable_build_points"], sum(row["bctx_status"] == 2 for row in points))
        self.assertEqual({row["name"] for row in report_catalog(6)}, set(publication.BUILD_ROW_KINDS))
        from scripts.telemetry.rollup_definitions import report_definition
        with self.assertRaises(ValueError):
            report_definition("battle_build_points", 5)
        # All sealed generations continue over the valid new family without
        # retaining it, changing their coverage or creating any output amount.
        for version in (1, 2, 3, 5):
            page = build_page_contributions([dict(row, ingest_id=index) for index, row in enumerate(original.values(), 1)],
                RollupTarget(version, 2, *scope[2:]))
            self.assertEqual((page.battle_inputs, page.coverage_start_utc_usec, page.coverage_end_utc_usec, page.state_quality_flags), ([], None, None, 0))

    def test_build_configuration_is_retained_and_unknown_remains_unqualified(self):
        rows, coverage = self._dated_build_rows()
        window, _ = self._build_window(rows, capture_config=False)
        output = publication.build_publication(window, None, coverage)
        self.assertEqual(output.header["qualified_build_points"], 0)
        self.assertEqual(output.header["configuration_unknown_build_points"], 20)
        point = next(row for row in window.facts if row["record_kind"] == 12 and row["bctx_config_id"])
        config = {name: point["bctx_" + name] for name in source.CONFIG_COLUMNS}
        with self.assertRaisesRegex(source.SourceError, "configuration_conflict"):
            source.retain_input(point, tuple(window.header[name] for name in source.SCOPE), 0,
                configuration=dict(config, content_version=config["content_version"] + 1))
        with self.assertRaisesRegex(publication.PublicationError, "incident_schema"):
            publication.build_publication(window, None, incident.public_coverage(None, [], registry_schema_version=4))

    def test_build_reference_loss_and_changed_clocks_stay_visible(self):
        rows, coverage = self._dated_build_rows()
        points = [row for row in rows if row["record_kind"] == 12]
        window, _ = self._build_window(points)
        output = publication.build_publication(window, None, coverage)
        self.assertEqual(output.header["partial_build_links"], 20)
        self.assertEqual(output.header["qualified_build_points"], 0)
        self.assertTrue(all(publication.decode_row(tuple(window.header[name] for name in source.SCOPE), row)["link_status"] == "missing_packet"
            for row in output.rows if row["row_kind"] == 6))
        point = next(row for row in rows if row["record_kind"] == 12 and row["bctx_status"] == 1)
        point["bctx_at_utc_usec"] += 123
        point["occurrence_utc_usec"] = point["bctx_at_utc_usec"]
        changed = history.build_history(rows, (point["bctx_environment_id"], point["bctx_season_id"]),
            incident_coverage=coverage, incident_schema_version=5)
        result = next(row for row in changed.builds if builds.observation_key(row) == builds.observation_key(point))
        self.assertEqual(result["point_clock_status"], "mismatch")
        self.assertTrue(result["publication_quality_flags"] & ROLLUP_QUALITY_UTC_MISMATCH)
        point["bctx_at_monotonic_usec"] = 0
        with self.assertRaisesRegex(history.HistoryError, "build_reference_clock"):
            history.build_history(rows, (point["bctx_environment_id"], point["bctx_season_id"]), incident_schema_version=5)

    def test_build_logical_conflicts_tampering_and_bounds_refuse(self):
        rows, coverage = self._dated_build_rows()
        window, inputs = self._build_window(rows)
        scope = tuple(window.header[name] for name in source.SCOPE)
        duplicate = dict(next(row for row in rows if row["record_kind"] == 12), record_seq=max(row["record_seq"] for row in rows) + 1)
        with self.assertRaisesRegex(history.HistoryError, "logical_receipt_conflict"):
            history.build_history([*rows, duplicate], scope[2:], incident_schema_version=5)
        saved = deepcopy(window)
        with self.assertRaisesRegex((publication.PublicationError, history.HistoryError), "output_capacity"):
            publication.build_publication(window, None, coverage, max_output_rows=1)
        with self.assertRaisesRegex(source.SourceError, "byte_capacity"):
            source.verify_source(window.header, inputs, expected_scope=scope, expected_watermark=window.header["input_watermark"], max_total_bytes=1)
        output = publication.build_publication(window, None, coverage)
        raw = next(row for row in output.rows if row["row_kind"] == 6)
        value = publication.decode_row(scope, raw)
        for field, item in (("continuous_build_exposure_implied", True), ("point_context_verified", not value["point_context_verified"]),
                ("bctx_equipment_digest", "0" * 64), ("point_clock_status", "invented")):
            changed = dict(value, **{field: item})
            with self.subTest(field=field), self.assertRaises((publication.PublicationError, builds.BuildContractError)):
                publication.retain_row(scope, 6, changed)
        for changes in ({"build_row_count": 21}, {"verified_build_links": 0, "partial_build_links": 20},
                {"unavailable_build_points": 20}, {"configuration_unknown_build_points": 21}):
            with self.subTest(header=changes), self.assertRaisesRegex(publication.PublicationError, "build_conservation"):
                publication._validate_header(dict(output.header, **changes))
        self.assertEqual(window, saved)

    def test_build_and_association_loss_reviews_are_independent(self):
        rows, coverage = self._dated_build_rows()
        window, _ = self._build_window(rows)
        scope = tuple(window.header[name] for name in source.SCOPE)
        baseline = publication.build_publication(window, None, coverage)
        point = next(publication.decode_row(scope, row) for row in baseline.rows if row["row_kind"] == 6 and
            publication.decode_row(scope, row)["point_context_verified"])
        for kind in (10, 12):
            packet = dict(incident.template(5), environment_id=scope[2], season_id=scope[3], reviewer_token="a" * 64,
                review_evidence_digest="b" * 64, reviewed_from_utc_usec=coverage["reviewed_from_utc_usec"],
                reviewed_through_utc_usec=coverage["reviewed_through_utc_usec"])
            packet["incidents"][0].update(producer_boot_id=point["boot_id"], producer_process_id=point["process_id"],
                start_utc_usec=point["bctx_at_utc_usec"], end_utc_usec=point["bctx_at_utc_usec"], record_kind_mask=1 << kind,
                evidence_digest="c" * 64)
            meta, details = incident.validate_packet(packet)
            details = [dict(row, occurrence_relation=incident.occurrence_relation(row, coverage["reviewed_from_utc_usec"],
                coverage["reviewed_through_utc_usec"])) for row in details]
            reviewed = incident.public_coverage(incident.publication_summary(scope, meta, details), details, registry_schema_version=5)
            output = publication.build_publication(window, None, reviewed)
            changed = next(publication.decode_row(scope, row) for row in output.rows if row["row_kind"] == 6 and
                builds.observation_key(publication.decode_row(scope, row)) == builds.observation_key(point))
            self.assertFalse(changed["point_context_verified"])
            self.assertTrue(changed["publication_quality_flags"] & ROLLUP_QUALITY_INCIDENT_GAP)
            if kind == 12:
                self.assertEqual([row for row in output.rows if row["row_kind"] != 6], [row for row in baseline.rows if row["row_kind"] != 6])

    def test_build_alias_lineage_preserves_original_reference_and_rejects_late_receipt(self):
        donor = next(row for row in self.baseline.battles if row["retired_by_alias"])
        facts = [dict(row, record_seq=row["record_seq"] * 2) for row in self.rows if row["record_kind"] == 10]
        cut = next(row for row in facts if battle.battle_id(row) == donor["battle"] and row["battle_fact_kind"] == 4)
        actor = next(row for row in facts if battle.battle_id(row) == donor["battle"] and row["battle_fact_kind"] == 2 and
            row["battle_actor_kind"] == 1)
        point = deepcopy(next(row for row in self.build_rows if row["record_kind"] == 12 and row["bctx_status"] == row["bctx_actor_kind"] == 1))
        point.update(boot_id=cut["boot_id"], process_id=cut["process_id"], record_seq=cut["record_seq"] + 1,
            bctx_battle_boot_id=cut["battle_boot_id"], bctx_battle_process_id=cut["battle_process_id"], bctx_battle_seq=cut["battle_seq"],
            bctx_environment_id=cut["battle_environment_id"], bctx_season_id=cut["battle_season_id"], bctx_config_id=cut["battle_config_id"],
            bctx_actor_id=actor["battle_actor_id"], bctx_association_revision=cut["battle_revision"],
            bctx_association_fact_sequence=cut["battle_fact_sequence"], bctx_at_monotonic_usec=cut["battle_at_monotonic_usec"],
            bctx_at_utc_usec=cut["battle_at_utc_usec"], occurrence_utc_usec=cut["battle_at_utc_usec"])
        result = history.build_history([*facts, point], self.scope, incident_schema_version=5)
        self.assertEqual(result.builds[0]["canonical_battle"], donor["canonical_battle"])
        self.assertEqual(result.builds[0]["link_status"], "verified")
        self.assertEqual(tuple(result.builds[0][name] for name in builds.BATTLE), donor["battle"])
        point["record_seq"] = max(row["record_seq"] for row in facts) + 1
        late = history.build_history([*facts, point], self.scope, incident_schema_version=5)
        self.assertEqual(late.builds[0]["link_status"], "outside_observed_prefix")

    def test_build_stale_reference_does_not_borrow_later_roster(self):
        rows, coverage = self._dated_build_rows()
        first = next(row for row in rows if row["record_kind"] == 12 and row["bctx_status"] == 1)
        # Retain the verified earlier packet, but advance this point to a later
        # admitted association boundary while preserving its old reference.
        identity = tuple(first[name] for name in builds.BATTLE)
        cuts = [row for row in rows if row["record_kind"] == 10 and battle.battle_id(row) == identity and
            row["battle_fact_kind"] == 4 and row["battle_revision"] > first["bctx_association_revision"]]
        cut = min(cuts, key=lambda row: row["record_seq"])
        first.update(record_seq=cut["record_seq"] + 1, bctx_at_monotonic_usec=cut["battle_at_monotonic_usec"],
            bctx_at_utc_usec=cut["battle_at_utc_usec"], occurrence_utc_usec=cut["battle_at_utc_usec"])
        # Only this point is needed; another raw family's receipt does not change
        # the association state, and the fixture sequence is made unique.
        facts = [dict(row, record_seq=row["record_seq"] * 2) for row in rows if row["record_kind"] == 10]
        first["record_seq"] = cut["record_seq"] * 2 + 1
        result = history.build_history([*facts, first], (first["bctx_environment_id"], first["bctx_season_id"]),
            incident_coverage=coverage, incident_schema_version=5)
        self.assertEqual(result.builds[0]["link_status"], "stale_association")

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

    def test_expanded_native_control_survives_history_and_publication(self):
        result = qualify_native_control_history(self.expanded_control_rows, expanded=True)
        window = self._publication_window(self.expanded_control_rows)
        scope = tuple(window.header[name] for name in source.SCOPE)
        output = publication.build_publication(window, None, incident.public_coverage(None, [], registry_schema_version=4))
        self.assertEqual(output.header["verified_contribution_links"], result.summary["contribution_count"])
        metrics = [publication.decode_row(scope, row) for row in output.rows if row["row_kind"] == 3]
        self.assertEqual(sum(row["bc_control_applications"] for row in metrics), 17)
        self.assertEqual(sum(row["bc_control_received"] for row in metrics), 17)
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
        self.assertEqual(SUPPORTED_DEFINITION_VERSIONS, {1, 2, 3, 5, 6, 7, 8, 9})

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
