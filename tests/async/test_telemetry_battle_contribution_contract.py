#!/usr/bin/env python3
"""Cross-language qualification of actual sealed contribution segments."""
from __future__ import annotations

import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))
from scripts.telemetry import battle_contribution_contract as contract
from scripts.telemetry import control_contract as control
from test_telemetry_battle_contributions import compile_harness


def raw_wire(row):
    """Preserve widths without enforcing semantic validity."""
    return b"".join(row[name].to_bytes(width, "big", signed=signed)
                    for name, width, signed in contract.FIELD_LAYOUT)


class ContributionContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        artifacts = ROOT / "bin/tests"
        artifacts.mkdir(parents=True, exist_ok=True)
        cls.directory = tempfile.TemporaryDirectory(prefix="telemetry-contribution-contract-", dir=artifacts)
        cls.path = Path(cls.directory.name)
        cls.native = cls.path / "contributions"
        compile_harness(cls.native)
        exported = cls.path / "source.jsonl"
        subprocess.run([str(cls.native), "--export", str(exported)], check=True, timeout=60)
        cls.sources = tuple(json.loads(line) for line in exported.read_text(encoding="ascii").splitlines())
        cls.wires = tuple(bytes.fromhex(item["wire"]) for item in cls.sources)
        cls.rows = tuple(contract.decode_segment(wire) for wire in cls.wires)
        cls.value = next(row for row in cls.rows if row["bc_damage_dealt"] == 1812)

    @classmethod
    def tearDownClass(cls):
        cls.directory.cleanup()

    def native_results(self, wires):
        path = self.path / "verify.bin"
        with path.open("wb") as output:
            for wire in wires:
                output.write(len(wire).to_bytes(4, "big"))
                output.write(wire)
        result = subprocess.run([str(self.native), "--verify", str(path)], capture_output=True,
                                text=True, check=True, timeout=30)
        values = [line == "1" for line in result.stdout.splitlines()]
        self.assertEqual(len(values), len(wires))
        return values

    @staticmethod
    def python_result(value):
        try:
            contract.validate_segment(value)
        except contract.ContributionContractError:
            return False
        return True

    def test_native_source_segments_round_trip_every_field(self):
        self.assertTrue(self.rows)
        self.assertEqual(self.native_results(self.wires), [True] * len(self.wires))
        keys = set()
        for source, row, wire in zip(self.sources, self.rows, self.wires):
            self.assertEqual(contract.encode_segment(row), wire)
            key = source["case"], contract.segment_key(row)
            self.assertNotIn(key, keys)
            keys.add(key)
        self.assertTrue(any(row["bc_actor_kind"] == 2 for row in self.rows))
        self.assertTrue(any(row["bc_actor_kind"] == 3 for row in self.rows))
        self.assertTrue(any(row["bc_casting_unresolved"] for row in self.rows))
        self.assertTrue(any(row["bc_quality_flags"] & (1 << 4) for row in self.rows))
        self.assertTrue(any(row["bc_damage_dealt"] == (1 << 64) - 1 for row in self.rows))

    def test_canonical_layout_matches_native_descriptor(self):
        descriptor = (ROOT / "src/telemetry/telemetry_battle_contribution_fields.inc").read_text()
        actual = tuple((name, int(width), signed == "true") for name, width, signed in re.findall(
            r"TELEMETRY_BC_FIELD\((\w+),\s*[\w.]+,\s*(\d+),\s*(true|false)\)", descriptor))
        self.assertEqual(actual, contract.FIELD_LAYOUT)
        self.assertEqual(len(actual), 65)
        self.assertLessEqual(contract.WIRE_BYTES + 40, 512)
        # A deliberate contract version change is required to alter this layout.
        self.assertEqual(hashlib.sha256(json.dumps(actual, separators=(",", ":")).encode()).hexdigest(),
                         "9790a6518be5a3989470decdc3ec7ea8f2ef1cf92134e19da1075c5ab5997965")

    def test_semantic_corruption_is_refused_in_both_languages(self):
        changes = {
            "bc_battle_boot_id": 0, "bc_segment_seq": 0, "bc_definition_version": 2,
            "bc_end_reason": 0, "bc_environment_id": 0, "bc_config_id": 0,
            "bc_scope_zone_vnum": 700, "bc_scope_group_key": 1,
            "bc_first_association_revision": 0, "bc_last_association_fact_sequence": 0,
            "bc_available_metrics": 0, "bc_actor_kind": 4, "bc_actor_pid": -1,
            "bc_actor_owner_subject_id": 999, "bc_actor_id": 0,
            "bc_actor_context_version": 2, "bc_actor_zone_vnum": -2,
            "bc_actor_encounter_boot_id": 0, "bc_actor_session_seq": 0,
            "bc_actor_group_key": 1 << 63, "bc_actor_group_revision": 1,
            "bc_actor_quality_flags": 1, "bc_quality_flags": 1 << 10,
            "bc_modifier_flags": 1 << 8, "bc_side_status": 0, "bc_actor_side": 0,
            "bc_mode": 4, "bc_effective_healing": 11, "bc_overhealing": 7,
            "bc_casting_completions": 1, "bc_casting_unresolved": 1,
            "bc_casting_elapsed_usec": 1, "bc_control_applications": 1,
            "bc_observed_through_monotonic_usec": 100, "bc_decision_monotonic_usec": 1299,
            "bc_start_utc_usec": self.value["bc_decision_utc_usec"] + 1,
        }
        values = [dict(self.value, **{name: value}) for name, value in changes.items()]
        for name, row in zip(changes, values):
            with self.subTest(name=name):
                self.assertFalse(self.python_result(row))
        self.assertEqual(self.native_results([raw_wire(row) for row in values]), [False] * len(values))

    def test_numeric_boundaries_have_identical_semantic_acceptance(self):
        values = []
        for name, width, signed in contract.FIELD_LAYOUT:
            limit = 1 << (width * 8 - int(signed))
            for candidate in sorted({0, 1, limit - 1, -limit if signed else 0}):
                values.append(dict(self.value, **{name: candidate}))
        expected = [self.python_result(value) for value in values]
        self.assertEqual(self.native_results([raw_wire(value) for value in values]), expected)

    def test_unknown_signed_clocks_and_context_are_preserved(self):
        values = []
        unknown = dict(self.value)
        for name in ("bc_start_utc_usec", "bc_observed_through_utc_usec", "bc_decision_utc_usec"):
            unknown[name] = contract.UTC_UNKNOWN
        values.append(unknown)
        values.append(dict(self.value, bc_start_utc_usec=-20, bc_observed_through_utc_usec=-10, bc_decision_utc_usec=-5))
        values.append(dict(self.value, bc_side_status=3, bc_actor_side=0, bc_context_quality_flags=16, bc_quality_flags=16))
        values.append(dict(self.value, bc_mode=0))
        # Copyover can retain a logical session's original producer.
        values.append(dict(self.value, bc_actor_session_boot_id=999))
        unavailable = dict(self.value, bc_available_metrics=0)
        for _, names in contract.COUNTER_FAMILIES:
            for name in names:
                unavailable["bc_" + name] = 0
        values.append(unavailable)
        self.assertTrue(all(self.python_result(row) for row in values))
        wires = [contract.encode_segment(row) for row in values]
        self.assertEqual(self.native_results(wires), [True] * len(values))
        for row, wire in zip(values, wires):
            self.assertEqual(contract.decode_segment(wire), row)

    def test_strict_python_shapes_types_widths_and_wire_lengths(self):
        for value in (None, [], {}, dict(self.value, surprise=0)):
            with self.assertRaises(contract.ContributionContractError):
                contract.validate_segment(value)
        for name, width, signed in contract.FIELD_LAYOUT:
            limit = 1 << (width * 8 - int(signed))
            for value in (None, True, "0", 1.0, limit, -limit - 1 if signed else -1):
                with self.subTest(name=name, value=value):
                    with self.assertRaises(contract.ContributionContractError):
                        contract.validate_segment(dict(self.value, **{name: value}))
        wire = contract.encode_segment(self.value)
        malformed = [b"", wire[:-1], wire + b"\0"]
        self.assertEqual(self.native_results(malformed), [False] * len(malformed))
        for value in malformed + [None, [], memoryview(wire)]:
            with self.assertRaises(contract.ContributionContractError):
                contract.decode_segment(value)

    def test_domain_key_is_independent_of_battle_actor_and_receipt(self):
        value = self.value
        self.assertEqual(contract.segment_key(value), contract.segment_key(dict(value, bc_battle_seq=999, bc_actor_id=999)))
        self.assertNotEqual(contract.segment_key(value), contract.segment_key(dict(value, bc_segment_seq=value["bc_segment_seq"] + 1)))
        self.assertNotEqual(contract.segment_key(value), contract.segment_key(dict(value, bc_battle_process_id=999)))
        copied = contract.validate_segment(value)
        copied["bc_damage_dealt"] += 1
        self.assertNotEqual(copied, value)

    @staticmethod
    def raw_segment(value, sequence):
        return dict(value, ingest_id=sequence, boot_id=value["bc_battle_boot_id"],
                    process_id=value["bc_battle_process_id"], record_seq=sequence,
                    record_kind=11, schema_version=1,
                    occurrence_utc_usec=value["bc_decision_utc_usec"], ingested_utc_usec=1)

    def test_raw_rows_bind_header_and_exclude_inactive_families(self):
        for value in self.rows:
            raw = self.raw_segment(value, 12345)
            self.assertEqual(contract.validate_raw_segment(raw), value)
            self.assertEqual(contract.validate_raw_segment(dict(raw, combat_damage_dealt=None)), value)
            for name, wrong in (("record_kind", 10), ("schema_version", 2), ("record_seq", 0),
                                ("boot_id", raw["boot_id"] + 1), ("process_id", True),
                                ("occurrence_utc_usec", raw["occurrence_utc_usec"] + 1),
                                ("combat_damage_dealt", 0), ("battle_boot_id", raw["boot_id"])):
                with self.subTest(name=name):
                    with self.assertRaises(contract.ContributionContractError):
                        contract.validate_raw_segment(dict(raw, **{name: wrong}))

    def test_earlier_rollup_definitions_validate_without_changing_amounts(self):
        from scripts.telemetry.rollup_definitions import RollupTarget
        from scripts.telemetry.rollup_engine import build_page_contributions, SemanticError
        raw = self.raw_segment(self.value, 12345)
        for version in (1, 2, 3):
            target = RollupTarget(version, 1, self.value["bc_environment_id"], self.value["bc_season_id"])
            result = build_page_contributions([raw], target, max_page_bytes=1000000)
            self.assertEqual(result.page_last_ingest_id, 12345)
            self.assertEqual(result.sessions, {})
            self.assertEqual(result.player_days, {})
            self.assertEqual(result.identity_inputs, [])
            with self.assertRaises(SemanticError):
                build_page_contributions([dict(raw, bc_effective_healing=21)], target)


class ControlContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        artifacts = ROOT / "bin/tests"
        artifacts.mkdir(parents=True, exist_ok=True)
        cls.directory = tempfile.TemporaryDirectory(prefix="telemetry-control-contract-", dir=artifacts)
        cls.path = Path(cls.directory.name)
        cls.native = cls.path / "controls"
        compile_harness(cls.native)
        exported = cls.path / "controls.jsonl"
        subprocess.run([str(cls.native), "--export-control", str(exported)], check=True, timeout=60)
        cls.sources = tuple(json.loads(line) for line in exported.read_text(encoding="ascii").splitlines())
        cls.rows = tuple(item["fields"] for item in cls.sources)
        cls.value = next(row for row in cls.rows if row["ctl_kind"] == 1 and
                         row["ctl_family"] == 3 and row["ctl_result"] == 1)
        cls.interval = next(item["fields"] for item in cls.sources if item["case"] == 2 and
                            item["fields"]["ctl_before_mask"] == 12)
        cls.gap = next(row for row in cls.rows if row["ctl_kind"] == 4)

    @classmethod
    def tearDownClass(cls):
        cls.directory.cleanup()

    def test_exact_raw_binding_and_inactive_families(self):
        row = dict(self.value, boot_id=self.value["ctl_boot_id"],
            process_id=self.value["ctl_process_id"], record_seq=55, record_kind=13,
            schema_version=1, occurrence_utc_usec=self.value["ctl_decision_utc_usec"],
            ingest_id=1, ingested_utc_usec=100, bc_damage_dealt=None)
        self.assertEqual(control.validate_raw_observation(row), self.value)
        for field, value in (("boot_id", row["boot_id"] + 1), ("process_id", 0),
            ("record_seq", False), ("record_kind", 12), ("schema_version", 2),
            ("occurrence_utc_usec", 0), ("bc_damage_dealt", 0),
            ("bctx_equipment_digest", bytes(32)), ("ctl_after_mask", None)):
            with self.subTest(field=field), self.assertRaises(control.ControlContractError):
                control.validate_raw_observation(dict(row, **{field: value}))

    def native_results(self, rows=None, wires=None):
        if wires is None:
            wires = [b"".join(row[name].to_bytes(width, "big", signed=signed)
                     for name, width, signed in control.FIELD_LAYOUT) for row in rows]
        path = self.path / "verify-control.bin"
        with path.open("wb") as output:
            for wire in wires:
                output.write(len(wire).to_bytes(4, "big"))
                output.write(wire)
        result = subprocess.run([str(self.native), "--verify-control", str(path)],
            capture_output=True, text=True, check=True, timeout=30)
        values = [line == "1" for line in result.stdout.splitlines()]
        self.assertEqual(len(values), len(wires))
        return values

    def agrees(self, rows):
        expected = []
        for row in rows:
            try:
                control.validate_observation(row)
            except control.ControlContractError:
                expected.append(False)
            else:
                expected.append(True)
        self.assertEqual(self.native_results(rows), expected)
        return expected

    def test_every_actual_accumulator_field_round_trips(self):
        self.assertTrue(self.sources)
        keys = set()
        for item in self.sources:
            row, wire = item["fields"], bytes.fromhex(item["wire"])
            self.assertEqual(control.decode_observation(wire), row)
            self.assertEqual(control.encode_observation(row), wire)
            key = item["case"], control.observation_key(row)
            self.assertNotIn(key, keys)
            keys.add(key)
        self.assertEqual(self.native_results(list(self.rows)), [True] * len(self.rows))
        self.assertEqual(self.value["ctl_configured_ticks"], -10)
        self.assertTrue(any(row["ctl_sequence"] == (1 << 64) - 1 for row in self.rows))

    def test_independent_named_layout_and_exact_width(self):
        descriptor = (ROOT / "src/telemetry/telemetry_control_fields.inc").read_text()
        actual = tuple((name, int(width), signed == "true") for name, width, signed in re.findall(
            r"TELEMETRY_CONTROL_FIELD\((\w+),\s*[\w.]+,\s*(\d+),\s*(true|false)\)", descriptor))
        self.assertEqual(actual, control.FIELD_LAYOUT)
        self.assertEqual(len(actual), 76)
        self.assertEqual(control.WIRE_BYTES, 368)
        self.assertLessEqual(control.WIRE_BYTES + 40, 512)

    def test_application_units_overlap_refresh_and_censored_prefix(self):
        resolutions = [item["fields"] for item in self.sources if item["case"] == 1]
        self.assertEqual(len(resolutions), 112)
        self.assertEqual(sum(row["ctl_result"] == 1 for row in resolutions), 8)
        self.assertTrue(all(row["ctl_duration_coverage"] == 0 and
            row["ctl_start_usec"] == row["ctl_at_usec"] == row["ctl_decision_usec"] for row in resolutions))
        intervals = [item["fields"] for item in self.sources if item["case"] == 2 and
                     item["fields"]["ctl_kind"] == 3]
        union = sum(row["ctl_at_usec"] - row["ctl_start_usec"] for row in intervals if row["ctl_before_mask"])
        major = sum(row["ctl_at_usec"] - row["ctl_start_usec"] for row in intervals if row["ctl_before_mask"] & 4)
        minor = sum(row["ctl_at_usec"] - row["ctl_start_usec"] for row in intervals if row["ctl_before_mask"] & 8)
        self.assertEqual((union, major, minor), (300, 200, 200))
        self.assertEqual((intervals[-1]["ctl_at_usec"], intervals[-1]["ctl_decision_usec"]), (550, 600))
        self.assertTrue(all(row["ctl_source_actor_id"] == 0 and row["ctl_family"] == 0 for row in intervals))
        refresh = [item["fields"] for item in self.sources if item["case"] == 3]
        self.assertEqual(len(refresh), 2)
        self.assertEqual((refresh[-1]["ctl_start_usec"], refresh[-1]["ctl_at_usec"]), (100, 400))
        cuts = [item["fields"] for item in self.sources if item["case"] == 4]
        self.assertEqual((cuts[1]["ctl_at_usec"], cuts[1]["ctl_decision_usec"], cuts[2]["ctl_start_usec"]),
                         (200, 400, 400))

    def test_semantic_corruption_and_state_source_confusion_refuse(self):
        changes = {
            "ctl_boot_id": 0, "ctl_sequence": 0, "ctl_environment_id": 0,
            "ctl_config_id": 0, "ctl_build_version": 0, "ctl_content_version": 0,
            "ctl_scope_zone_vnum": 0, "ctl_scope_group_key": 1,
            "ctl_definition_version": 2, "ctl_producer_version": 2,
            "ctl_source_actor_pid": -1, "ctl_target_actor_id": 0,
            "ctl_target_owner_subject_id": 999, "ctl_target_actor_kind": 3,
            "ctl_target_context_version": 0, "ctl_target_zone_vnum": -2,
            "ctl_source_session_boot_id": 0, "ctl_target_session_seq": 0,
            "ctl_target_association_revision": 0, "ctl_last_target_association_revision": 0,
            "ctl_target_group_key": 1 << 63, "ctl_target_group_revision": 1,
            "ctl_quality_flags": 1024, "ctl_target_quality_flags": 1,
            "ctl_state_available": 0, "ctl_duration_coverage": 255,
            "ctl_before_mask": 256, "ctl_after_mask": 0, "ctl_flags": 64,
            "ctl_family": 0, "ctl_result": 0, "ctl_kind": 5, "ctl_boundary": 1,
            "ctl_previous_state_sequence": 1, "ctl_start_usec": self.value["ctl_at_usec"] + 1,
        }
        rows = [dict(self.value, **{name: value}) for name, value in changes.items()]
        self.assertEqual(self.agrees(rows), [False] * len(rows))
        interval_rows = [dict(self.interval, **{name: value}) for name, value in {
            "ctl_previous_state_sequence": 0, "ctl_flags": 32, "ctl_family": 1,
            "ctl_result": 1, "ctl_source_actor_id": 42, "ctl_configured_ticks": 1,
            "ctl_last_target_association_revision": 0, "ctl_duration_coverage": 0,
        }.items()]
        self.assertEqual(self.agrees(interval_rows), [False] * len(interval_rows))

    def test_known_gates_bypasses_and_unassociated_operations(self):
        rows = [dict(self.value, ctl_result=result, ctl_after_mask=0, ctl_configured_ticks=0)
                for result in range(2, 15)]
        rows.append(dict(self.value, ctl_flags=1 | 2 | 4))
        rows.append(dict(self.value, ctl_family=2, ctl_after_mask=2, ctl_flags=16))
        rows.append(dict(self.value, ctl_family=6, ctl_before_mask=32, ctl_after_mask=32, ctl_flags=8 | 4))
        rows.extend(dict(self.value, ctl_family=8, ctl_after_mask=mask) for mask in (8, 128))
        unassociated = dict(self.value)
        for prefix in ("source", "target"):
            for name in ("battle_seq", "association_revision", "association_fact_sequence"):
                unassociated["ctl_" + prefix + "_" + name] = 0
        unassociated["ctl_last_target_association_revision"] = 0
        unassociated["ctl_last_target_association_fact_sequence"] = 0
        rows.append(unassociated)
        self.assertEqual(self.agrees(rows), [True] * len(rows))
        self.assertEqual(self.agrees([dict(self.value, ctl_flags=8), dict(self.value, ctl_flags=16),
            dict(rows[0], ctl_configured_ticks=10), dict(rows[0], ctl_flags=4),
            dict(self.value, ctl_flags=32)]), [False] * 5)

    def test_gap_recovery_unknown_clocks_partial_duration_and_live_actors(self):
        gap = dict(self.gap, ctl_boundary=8)
        for name in ("config_id", "classifier_version", "policy_version", "build_version", "content_version"):
            gap["ctl_" + name] = 0
        partial = dict(self.interval, ctl_duration_coverage=12, ctl_quality_flags=1)
        unknown = dict(self.interval)
        for name in ("start", "at", "decision"):
            unknown["ctl_" + name + "_utc_usec"] = control.UTC_UNKNOWN
        npc = dict(self.value, ctl_target_actor_id=(1 << 63) | 999,
            ctl_target_actor_pid=-1, ctl_target_actor_kind=3, ctl_target_owner_subject_id=0,
            ctl_target_session_boot_id=0, ctl_target_session_process_id=0, ctl_target_session_seq=0)
        pet = dict(npc, ctl_target_actor_kind=2, ctl_target_owner_subject_id=42)
        self_effect = dict(self.value, ctl_flags=32)
        for name in control.FIELDS:
            if name.startswith("ctl_source_"):
                self_effect[name] = self.value[name.replace("ctl_source_", "ctl_target_", 1)]
        self.assertEqual(self.agrees([gap, partial, unknown, npc, pet, self_effect]), [True] * 6)
        self.assertEqual(self.agrees([dict(gap, ctl_before_mask=4), dict(gap, ctl_after_mask=4),
            dict(gap, ctl_state_available=255), dict(gap, ctl_content_version=1),
            dict(self.gap, ctl_quality_flags=0), dict(npc, ctl_target_actor_id=999),
            dict(pet, ctl_target_owner_subject_id=0)]), [False] * 7)
        recovery = [item["fields"] for item in self.sources if item["case"] == 5]
        self.assertEqual([row["ctl_kind"] for row in recovery], [4, 2, 3])
        self.assertEqual(recovery[1]["ctl_previous_state_sequence"], 0)
        self.assertEqual(recovery[-1]["ctl_at_usec"] - recovery[-1]["ctl_start_usec"], 100)

    def test_numeric_boundaries_and_strict_wire_shapes_agree(self):
        rows = []
        for name, width, signed in control.FIELD_LAYOUT:
            limit = 1 << (width * 8 - int(signed))
            for candidate in sorted({0, 1, limit - 1, -limit if signed else 0}):
                rows.append(dict(self.value, **{name: candidate}))
            for invalid in (True, None, "0", 1.0, limit, -limit - 1 if signed else -1):
                with self.assertRaises(control.ControlContractError):
                    control.validate_observation(dict(self.value, **{name: invalid}))
        self.agrees(rows)
        wire = control.encode_observation(self.value)
        malformed = [b"", wire[:-1], wire + b"\0"]
        self.assertEqual(self.native_results(wires=malformed), [False] * 3)
        for data in malformed + [None, [], memoryview(wire)]:
            with self.assertRaises(control.ControlContractError):
                control.decode_observation(data)
        for row in (None, [], {}, dict(self.value, extra=0)):
            with self.assertRaises(control.ControlContractError):
                control.validate_observation(row)


if __name__ == "__main__":
    unittest.main(verbosity=2)
