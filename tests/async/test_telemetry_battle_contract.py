#!/usr/bin/env python3
"""Cross-language battle packet, replay, loss and numeric wire qualification."""
from __future__ import annotations

from collections import defaultdict
from copy import deepcopy
from dataclasses import asdict
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
from scripts.telemetry import battle_contract as contract
from scripts.telemetry.db_access import RAW_COLUMNS
from scripts.telemetry.rollup_definitions import RollupTarget
from scripts.telemetry.rollup_engine import BoundsExceeded, SemanticError, build_page_contributions
sys.path.insert(0, str(ROOT / "tests/async"))
from telemetry_rollup_fixtures import FIXTURE_DIR, golden_rows


def raw_wire(row):
    """Test malformed semantic values while preserving the declared wire widths."""
    return b"".join(row[name].to_bytes(width, "big", signed=signed)
                    for name, width, signed in contract.FIELD_LAYOUT)


class BattleContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        artifacts = ROOT / "bin/tests"
        artifacts.mkdir(parents=True, exist_ok=True)
        cls.directory = tempfile.TemporaryDirectory(prefix="telemetry-battle-contract-", dir=artifacts)
        cls.path = Path(cls.directory.name)
        cls.native = cls.path / "battle-contract"
        compiler = ["g++", "-std=c++20", "-Wall", "-Wextra", "-Werror", "-pedantic", "-I", str(ROOT / "src")]
        common = str(ROOT / "src/telemetry/telemetry_battle_contract.c")
        subprocess.run(compiler + [str(ROOT / "tests/async/telemetry_battle_contract_harness.cc"), common, "-o", str(cls.native)],
                       check=True, timeout=120)
        source = cls.path / "battle-source"
        subprocess.run(compiler + [str(ROOT / "tests/async/telemetry_battle_harness.cc"),
                                  str(ROOT / "src/telemetry/telemetry_battle.c"), common, "-o", str(source)], check=True, timeout=120)
        exported = cls.path / "source.jsonl"
        subprocess.run([str(source), "--export", str(exported)], check=True, timeout=30)
        cls.rows, cls.packets = [], defaultdict(list)
        for line in exported.read_text(encoding="ascii").splitlines():
            item = json.loads(line)
            wire = bytes.fromhex(item["wire"])
            row = contract.decode_fact(wire)
            cls.rows.append((row, wire))
            cls.packets[(item["case"], *contract.battle_id(row), row["battle_revision"])].append(row)
        cls.start = next(rows for rows in cls.packets.values() if len(rows) == 5 and rows[0]["battle_fact_kind"] == 1)
        cls.terminal = next(rows for rows in cls.packets.values() if len(rows) == 3 and rows[-1]["battle_fact_kind"] == 7)

    @classmethod
    def tearDownClass(cls):
        cls.directory.cleanup()

    def native_results(self, packets):
        path = self.path / "verify.bin"
        with path.open("wb") as output:
            for rows in packets:
                wire = b"".join(raw_wire(row) for row in rows)
                output.write(len(wire).to_bytes(4, "big"))
                output.write(wire)
        result = subprocess.run([str(self.native), str(path)], capture_output=True, text=True, check=True, timeout=30)
        return [line == "1" for line in result.stdout.splitlines()]

    @staticmethod
    def python_result(rows):
        try:
            contract.validate_packet(rows)
        except contract.BattleContractError:
            return False
        return True

    @staticmethod
    def raw_fact(value, ingest_id=1):
        row = dict.fromkeys(RAW_COLUMNS)
        row.update(value)
        row.update(ingest_id=ingest_id, boot_id=value["battle_boot_id"],
                   process_id=value["battle_process_id"], record_seq=ingest_id,
                   schema_version=1, record_kind=10,
                   occurrence_utc_usec=value["battle_at_utc_usec"], ingested_utc_usec=0)
        return row

    def test_every_native_source_fact_survives_the_raw_sql_contract(self):
        for index, (value, _) in enumerate(self.rows, 1):
            self.assertEqual(contract.validate_raw_fact(self.raw_fact(value, index)), value)
        raw = self.raw_fact(self.start[1])
        for name, replacement in (("boot_id", raw["boot_id"] + 1),
                                  ("process_id", raw["process_id"] + 1),
                                  ("occurrence_utc_usec", raw["occurrence_utc_usec"] + 1),
                                  ("record_kind", 9), ("schema_version", 2),
                                  ("record_seq", True), ("battle_fact_count", None),
                                  ("combat_damage_dealt", 1), ("duration_usec", 1)):
            with self.subTest(name=name):
                with self.assertRaises(contract.BattleContractError):
                    contract.validate_raw_fact(dict(raw, **{name: replacement}))

    def test_mixed_battles_preserve_all_three_existing_report_definitions(self):
        _, legacy = golden_rows(FIXTURE_DIR / "normal_interval.json")
        interval = next(row for row in legacy if row["record_kind"] == 1)
        maximum = max(row["ingest_id"] for row in legacy)
        raw = [self.raw_fact(value, maximum + index) for index, value in enumerate(self.start, 1)]
        for version in (1, 2, 3):
            target = RollupTarget(version, 1, interval["environment_id"], interval["season_id"])
            before = build_page_contributions(legacy, target, max_page_bytes=1_000_000)
            after = build_page_contributions(legacy + raw, target, max_page_bytes=1_000_000)
            self.assertTrue(before.player_days)
            first, second = asdict(before), asdict(after)
            for metadata in ("page_last_ingest_id", "fetched_rows", "estimated_bytes"):
                first.pop(metadata)
                second.pop(metadata)
            self.assertEqual(first, second)
            self.assertEqual(after.cursor, raw[-1]["ingest_id"])
            self.assertEqual(after.fetched_rows, before.fetched_rows + 5)
            self.assertGreater(after.estimated_bytes, before.estimated_bytes)
            malformed = dict(raw[-1], battle_fact_count=None)
            with self.assertRaises(SemanticError):
                build_page_contributions(legacy + raw[:-1] + [malformed], target,
                                         max_page_bytes=1_000_000)
            with self.assertRaises(BoundsExceeded):
                build_page_contributions(legacy + raw, target,
                                         max_page_bytes=before.estimated_bytes)

    def test_layout_is_exact_and_definition_is_immutable(self):
        descriptor = (ROOT / "src/telemetry/telemetry_battle_fields.inc").read_text(encoding="utf-8")
        fields = re.findall(r"TELEMETRY_BATTLE_FIELD\(\s*(\w+),\s*([\w.]+),\s*(\d),\s*(true|false)\s*\)", descriptor)
        self.assertEqual(tuple((name, int(width), signed == "true") for name, member, width, signed in fields), contract.FIELD_LAYOUT)
        self.assertEqual((len(fields), contract.WIRE_BYTES), (70, 366))
        self.assertEqual(len(contract.FIELDS), len(set(contract.FIELDS)))
        normalized = json.dumps(contract.FIELD_LAYOUT, separators=(",", ":")).encode("ascii")
        self.assertEqual(hashlib.sha256(normalized).hexdigest(), "b6be021524e627a8162de6eca6aad660ae5219d1b3fed8ac0666c2acab9c5284")
        self.assertEqual(raw_wire(self.start[0])[:24], (101).to_bytes(8, "big") + (202).to_bytes(8, "big") + (1).to_bytes(8, "big"))

    def test_all_native_source_values_round_trip_exactly(self):
        self.assertGreater(len(self.rows), 4000)
        self.assertEqual([contract.encode_fact(row) for row, _ in self.rows], [wire for _, wire in self.rows])
        self.assertTrue(any(row["battle_at_utc_usec"] == -(1 << 63) for row, _ in self.rows))
        self.assertTrue(any(row["battle_actor_pid"] == -1 for row, _ in self.rows))
        self.assertTrue(any(row["battle_actor_kind"] == 2 for row, _ in self.rows))
        self.assertTrue(any(row["battle_actor_count"] == 64 for row, _ in self.rows))
        self.assertTrue(any(row["battle_fact_kind"] == 5 for row, _ in self.rows))

    def test_native_and_python_agree_on_every_real_packet_and_loss(self):
        packets = list(self.packets.values())
        expected = [self.python_result(rows) for rows in packets]
        self.assertEqual(self.native_results(packets), expected)
        self.assertGreater(sum(expected), 2000)
        self.assertIn(False, expected)  # Accepted source frames from the rejected-sink journey retain a missing cut.
        full_rosters = [rows for rows in packets if len(rows) == 65]
        self.assertTrue(full_rosters)
        self.assertTrue(all(self.python_result(rows) for rows in full_rosters))

    def test_reorder_replay_and_missing_cut_never_complete_early(self):
        buffer = contract.PacketBuffer()
        for row in reversed(self.start[1:]):
            self.assertEqual(buffer.receive(row), "pending")
            self.assertEqual(buffer.receive(row), "duplicate_identical")
        self.assertFalse(buffer.complete)
        with self.assertRaises(contract.BattleContractError):
            buffer.packet()
        self.assertEqual(buffer.receive(self.start[0]), "complete")
        self.assertEqual(buffer.packet(), tuple(self.start))
        returned = buffer.packet()
        returned[0]["battle_config_id"] += 1
        self.assertEqual(buffer.packet(), tuple(self.start))
        buffer.reset()
        for row in self.start[:-1]:
            self.assertEqual(buffer.receive(row), "pending")
        self.assertFalse(buffer.complete)
        self.assertEqual(buffer.received, 4)
        self.assertEqual(buffer.receive(self.start[-1]), "complete")

    def test_conflicting_replay_latches_and_other_packet_preserves_pending(self):
        buffer = contract.PacketBuffer()
        row = self.start[1]
        self.assertEqual(buffer.receive(row), "pending")
        other = dict(row, battle_seq=2)
        self.assertEqual(buffer.receive(other), "other_packet")
        self.assertEqual(buffer.received, 1)
        changed = dict(row, battle_actor_power_band=row["battle_actor_power_band"] + 1)
        self.assertEqual(contract.fact_key(row), contract.fact_key(changed))
        self.assertEqual(buffer.receive(changed), "duplicate_conflict")
        self.assertEqual(buffer.receive(row), "duplicate_conflict")
        self.assertFalse(buffer.complete)
        buffer.reset()
        self.assertEqual(buffer.receive(row), "pending")
        self.assertEqual(buffer.receive(dict(row, battle_fact_sequence=row["battle_fact_sequence"] + 1)), "duplicate_conflict")

    def test_semantic_corruption_is_refused_in_both_languages(self):
        mutations = {
            "battle_boot_id": 0, "battle_scope_zone_vnum": 0, "battle_scope_group_key": 1,
            "battle_definition_version": 2, "battle_revision": 0, "battle_fact_sequence": 1,
            "battle_fact_index": 5, "battle_fact_count": 0, "battle_fact_kind": 99,
            "battle_actor_kind": 99, "battle_mode": 99, "battle_actor_id": 0, "battle_actor_pid": -1,
            "battle_actor_owner_subject_id": 7, "battle_actor_session_seq": 0,
            "battle_actor_encounter_boot_id": 999, "battle_actor_group_key": contract.GENERATION_TAG,
            "battle_actor_group_revision": 1, "battle_actor_context_version": 0, "battle_actor_zone_vnum": -2,
            "battle_actor_quality_flags": 1, "battle_quality_flags": 1 << 10, "battle_actor_roles": 8,
            "battle_actor_active": 2, "battle_actor_side": 3, "battle_actor_count": 65,
            "battle_active_actor_count": 65, "battle_observed_owner_count": 65,
            "battle_present_usec": 1, "battle_contributor_usec": 1, "battle_pve_usec": 1,
            "battle_outnumbered_owner_usec": 1, "battle_unknown_side_usec": 1,
            "battle_start_monotonic_usec": 1, "battle_inactivity_grace_usec": 0,
            "battle_end_censored": 1, "battle_close_reason": 1, "battle_related_seq": 2,
            "battle_related_actor_id": 2, "battle_dropped_actor_count": 1,
        }
        packets = []
        for name, value in mutations.items():
            self.assertNotEqual(self.start[1][name], value, name)
            rows = deepcopy(self.start)
            rows[1][name] = value
            packets.append(rows)
        self.assertEqual(self.native_results(packets), [False] * len(packets))
        self.assertEqual([self.python_result(rows) for rows in packets], [False] * len(packets))

    def test_packet_only_corruption_is_refused(self):
        packets = [[], self.start[:-1], self.start[1:], list(reversed(self.start))]
        for name, value in (("battle_config_id", 99), ("battle_fact_sequence", 7), ("battle_observed_owner_count", 1)):
            rows = deepcopy(self.start)
            rows[-1][name] = value
            packets.append(rows)
        rows = deepcopy(self.start)
        rows[3]["battle_actor_power_band"] += 1
        packets.append(rows)
        rows = deepcopy(self.start)
        for row in rows:
            row["battle_observed_owner_count"] = 1
        packets.append(rows)
        rows = deepcopy(self.terminal)
        for name in contract.ACTOR_VALUES:
            rows[1][name] = rows[0][name]
        packets.append(rows)
        rows = deepcopy(self.start)
        rows[2]["battle_actor_id"] = rows[2]["battle_actor_pid"] = rows[2]["battle_actor_owner_subject_id"] = 1
        packets.append(rows)
        self.assertEqual(self.native_results(packets), [False] * len(packets))
        self.assertEqual([self.python_result(rows) for rows in packets], [False] * len(packets))

    def test_quality_can_increase_within_a_packet_and_cannot_disappear(self):
        rows = deepcopy(self.start)
        for row in rows[2:]:
            row["battle_quality_flags"] |= contract.QUALITY_QUEUE_DROP
            row["battle_side_status"] = 3
            row["battle_actor_side"] = 0
        self.assertTrue(self.python_result(rows))
        self.assertEqual(self.native_results([rows]), [True])
        rows[-1]["battle_quality_flags"] &= ~contract.QUALITY_QUEUE_DROP
        self.assertFalse(self.python_result(rows))
        self.assertEqual(self.native_results([rows]), [False])

    def test_exact_widths_unknown_fields_boolean_and_length_refusals(self):
        row = self.start[1]
        for value in (raw_wire(row)[:-1], raw_wire(row) + b"\0", b"", "invalid"):
            with self.assertRaises(contract.BattleContractError):
                contract.decode_fact(value)
        for name, width, signed in contract.FIELD_LAYOUT:
            for value in (True, "1", None, 1 << (width * 8 - int(signed))):
                with self.subTest(name=name, value=value):
                    with self.assertRaises(contract.BattleContractError):
                        contract.encode_fact(dict(row, **{name: value}))
        with self.assertRaises(contract.BattleContractError):
            contract.encode_fact(dict(row, unknown_field=0))
        self.assertEqual(contract.PacketBuffer().receive({}), "invalid")

    def test_max_packet_reordering_and_input_is_copied(self):
        rows = next(rows for rows in self.packets.values() if len(rows) == 65)
        buffer = contract.PacketBuffer()
        for index, row in enumerate(reversed(rows)):
            source = dict(row)
            self.assertEqual(buffer.receive(source), "complete" if index == 64 else "pending")
            source["battle_config_id"] += 1
        self.assertEqual(buffer.packet(), tuple(rows))
        self.assertEqual(buffer.received, 65)


if __name__ == "__main__":
    unittest.main()
