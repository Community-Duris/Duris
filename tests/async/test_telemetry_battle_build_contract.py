#!/usr/bin/env python3
"""Qualify the selected build point contract in C++ and Python."""
from __future__ import annotations

import argparse
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
from scripts.telemetry import battle_build_contract as contract

SANITIZE = False


def compile_harness(executable: Path, *, sanitize=False):
    command = ["g++", "-std=c++20", "-Wall", "-Wextra", "-Werror", "-pedantic", "-I", str(ROOT / "src"),
               str(ROOT / "tests/async/telemetry_battle_build_observation_harness.cc"),
               str(ROOT / "src/telemetry/telemetry_battle_build_observation.c"), "-o", str(executable)]
    if sanitize:
        command += ["-g", "-fno-omit-frame-pointer", "-fsanitize=address,undefined"]
    subprocess.run(command, check=True, timeout=120)


def raw_wire(row):
    return b"".join(row[name] if signed is None else row[name].to_bytes(width, "big", signed=signed)
                    for name, width, signed in contract.FIELD_LAYOUT)


class BuildContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        artifacts = ROOT / "bin/tests"
        artifacts.mkdir(parents=True, exist_ok=True)
        cls.directory = tempfile.TemporaryDirectory(prefix="telemetry-build-contract-", dir=artifacts)
        cls.path = Path(cls.directory.name)
        cls.native = cls.path / "build-observations"
        compile_harness(cls.native, sanitize=SANITIZE)
        result = subprocess.run([str(cls.native)], check=True, capture_output=True, text=True, timeout=30)
        exported = [json.loads(line.removeprefix("BUILD_OBSERVATION_JSON "))
                    for line in result.stdout.splitlines() if line.startswith("BUILD_OBSERVATION_JSON ")]
        assert len(exported) == 1
        cls.source = exported[0]
        cls.value = dict(cls.source["fields"])
        for name in contract.BYTE_FIELDS:
            cls.value[name] = bytes.fromhex(cls.value[name])
        cls.wire = bytes.fromhex(cls.source["wire"])
        print("native factory/clearing/partial-family assertions passed")

    @classmethod
    def tearDownClass(cls):
        cls.directory.cleanup()

    def native_results(self, wires):
        path = self.path / "verify.bin"
        with path.open("wb") as output:
            for wire in wires:
                output.write(len(wire).to_bytes(4, "big"))
                output.write(wire)
        result = subprocess.run([str(self.native), "--verify", str(path)], check=True,
                                capture_output=True, text=True, timeout=30)
        values = [line == "1" for line in result.stdout.splitlines()]
        self.assertEqual(len(values), len(wires))
        return values

    @staticmethod
    def python_result(row):
        try:
            contract.validate_observation(row)
        except contract.BuildContractError:
            return False
        return True

    def agrees(self, rows):
        expected = [self.python_result(row) for row in rows]
        actual = self.native_results([raw_wire(row) for row in rows])
        for index, (native, python) in enumerate(zip(actual, expected)):
            with self.subTest(index=index):
                self.assertEqual(native, python)
        return expected

    def test_native_values_round_trip_every_field(self):
        self.assertEqual(contract.decode_observation(self.wire), self.value)
        self.assertEqual(contract.encode_observation(self.value), self.wire)
        self.assertEqual(len(self.wire), 447)
        self.assertEqual(self.value["bctx_base_hit"], 70000)
        self.assertEqual(self.value["bctx_effective_hit"], 123456)
        self.assertEqual(self.value["bctx_current_hit"], -2)
        self.assertEqual(self.value["bctx_effective_armor"], -15)
        self.assertEqual(self.value["bctx_saving_para"], -128)
        self.assertEqual(self.value["bctx_at_utc_usec"], contract.UTC_UNKNOWN)
        self.assertEqual(self.value["bctx_equipment_flags_5"], (1 << 64) - 1)
        self.assertEqual(self.value["bctx_equipment_digest"], bytes(range(1, 33)))
        self.assertTrue(all(self.agrees([self.value])))

    def test_layout_is_exact_independent_and_sealed(self):
        descriptor = (ROOT / "src/telemetry/telemetry_battle_build_fields.inc").read_text()
        actual = []
        for kind, name, width, signed in re.findall(
            r"TELEMETRY_BUILD_(FIELD|BYTES)\((\w+),\s*[^,]+,\s*(\d+)(?:,\s*(true|false))?\)", descriptor):
            actual.append((name, int(width), None if kind == "BYTES" else signed == "true"))
        self.assertEqual(tuple(actual), contract.FIELD_LAYOUT)
        self.assertEqual(len(actual), 110)
        self.assertEqual(hashlib.sha256(json.dumps(actual, separators=(",", ":")).encode()).hexdigest(),
                         "059f0e9236f3d30340feacf7329d8c0857cee23a2e94c327eb60bb2a8c51b5a5")

    def test_semantic_corruption_has_identical_refusal(self):
        changes = {
            "bctx_battle_boot_id": 0, "bctx_battle_process_id": 0, "bctx_battle_seq": 0,
            "bctx_sequence": 0, "bctx_actor_id": 0, "bctx_actor_kind": 4,
            "bctx_environment_id": 0, "bctx_season_id": 0, "bctx_config_id": 0,
            "bctx_build_version": 0, "bctx_content_version": 0,
            "bctx_association_revision": 0, "bctx_association_fact_sequence": 0,
            "bctx_definition_version": 2, "bctx_native_context_version": 2,
            "bctx_boundary": 6, "bctx_status": 2, "bctx_available": 1024,
            "bctx_context_quality": 0, "bctx_quality_flags": 1024,
            "bctx_equipment_occupied_slots_count": 4, "bctx_equipment_digest": bytes(32),
            "bctx_epic_learned_skills": 3, "bctx_epic_catalog_skills": 310,
            "bctx_epic_digest": bytes(32), "bctx_affect_nodes": 0,
            "bctx_affects_complete": 0, "bctx_arena_membership": 0,
            "bctx_arena_room": 2, "bctx_arena_enabled": 2,
            "bctx_arena_type": 6, "bctx_arena_stage": 6, "bctx_arena_team": 1,
            "bctx_arena_player_flags": 1,
        }
        rows = [dict(self.value, **{name: value}) for name, value in changes.items()]
        self.assertEqual(self.agrees(rows), [False] * len(rows))

    def test_numeric_and_digest_boundaries_agree(self):
        rows = []
        for name, width, signed in contract.FIELD_LAYOUT:
            if signed is None:
                candidates = (bytes(width), bytes([255]) * width, bytes(range(width)))
            else:
                limit = 1 << (width * 8 - int(signed))
                candidates = sorted({0, 1, limit - 1, -limit if signed else 0})
            rows.extend(dict(self.value, **{name: value}) for value in candidates)
        self.agrees(rows)

    def test_independent_unavailable_families_and_partial_prefix(self):
        rows = []
        for bit, names in ((1, contract.BASE_FIELDS), (2, contract.EFFECTIVE_FIELDS),
                           (4, contract.RESOURCE_FIELDS), (8, contract.SAVE_FIELDS),
                           (16, contract.FLAG_FIELDS), (32, contract.EQUIPMENT_FIELDS),
                           (64, contract.EPIC_FIELDS), (128, contract.AFFECT_FIELDS)):
            row = dict(self.value, bctx_available=self.value["bctx_available"] & ~bit)
            for name in names:
                row[name] = bytes(32) if name in contract.BYTE_FIELDS else 0
            rows.append(row)
        rows.extend([
            dict(self.value, bctx_available=1023 & ~256, bctx_context_quality=128 | 64),
            dict(self.value, bctx_available=1023 & ~512, bctx_context_quality=128 | 16, bctx_arena_membership=0),
            dict(self.value, bctx_available=1023 & ~512, bctx_context_quality=128 | 32, bctx_arena_membership=3),
            dict(self.value, bctx_affect_nodes=64, bctx_affects_complete=0, bctx_context_quality=128 | 4),
            dict(self.value, bctx_affects_complete=0, bctx_context_quality=128 | 8),
            dict(self.value, bctx_arena_membership=2, bctx_arena_team=3, bctx_arena_player_flags=-1),
        ])
        self.assertEqual(self.agrees(rows), [True] * len(rows))
        for row in rows:
            self.assertEqual(contract.decode_observation(contract.encode_observation(row)), row)
        self.assertFalse(self.agrees([dict(self.value, bctx_affect_nodes=0, bctx_unapplied_nodes=0,
            bctx_offensive_modifier_nodes=0, bctx_armor_modifier_nodes=0,
            bctx_affects_complete=0, bctx_context_quality=128 | 8)])[0])

    def test_gaps_never_copy_a_preceding_profile(self):
        gap = {name: self.value[name] if name in contract.METADATA_FIELDS else
               bytes(width) if signed is None else 0 for name, width, signed in contract.FIELD_LAYOUT}
        gap.update(bctx_sequence=2, bctx_status=2, bctx_boundary=6)
        rows = [dict(gap, bctx_boundary=boundary) for boundary in (6, 7)]
        rows.append(dict(gap, bctx_boundary=8, bctx_config_id=0, bctx_build_version=0, bctx_content_version=0))
        self.assertEqual(self.agrees(rows), [True, True, True])
        stale = [dict(gap, **{name: self.value[name]}) for name in contract.FIELDS
                 if name not in contract.METADATA_FIELDS and self.value[name] not in (0, bytes(32))]
        self.assertEqual(self.agrees(stale), [False] * len(stale))
        self.assertFalse(self.agrees([dict(gap, bctx_quality_flags=0)])[0])

    def test_npc_builds_keep_live_generation_and_no_pc_epics(self):
        rows = []
        for kind in (2, 3):
            row = dict(self.value, bctx_actor_kind=kind, bctx_actor_id=(1 << 63) | 42,
                       bctx_available=1023 & ~64, bctx_context_quality=128 | 1,
                       bctx_epic_catalog_skills=0, bctx_epic_learned_skills=0, bctx_epic_digest=bytes(32))
            rows.append(row)
        self.assertEqual(self.agrees(rows), [True, True])
        self.assertEqual(self.agrees([dict(row, bctx_actor_id=42) for row in rows]), [False, False])
        self.assertNotEqual(contract.observation_key(self.value), contract.observation_key(dict(self.value, bctx_sequence=2)))
        self.assertEqual(contract.observation_key(self.value), contract.observation_key(dict(self.value, bctx_battle_seq=999)))

    def test_strict_shapes_widths_and_wire_lengths(self):
        for row in (None, [], {}, dict(self.value, unexpected=0)):
            with self.assertRaises(contract.BuildContractError):
                contract.validate_observation(row)
        for name, width, signed in contract.FIELD_LAYOUT:
            if signed is None:
                candidates = (None, "00" * width, bytearray(width), bytes(width - 1), bytes(width + 1))
            else:
                limit = 1 << (width * 8 - int(signed))
                candidates = (None, True, "0", 1.0, limit, -limit - 1 if signed else -1)
            for value in candidates:
                with self.subTest(name=name, value=value):
                    with self.assertRaises(contract.BuildContractError):
                        contract.validate_observation(dict(self.value, **{name: value}))
        malformed = [b"", self.wire[:-1], self.wire + b"\0"]
        self.assertEqual(self.native_results(malformed), [False] * len(malformed))
        for wire in malformed + [None, [], memoryview(self.wire)]:
            with self.assertRaises(contract.BuildContractError):
                contract.decode_observation(wire)

    def test_raw_receipt_bindings_and_inactive_families(self):
        from scripts.telemetry.db_access import RAW_COLUMNS
        row = dict.fromkeys(RAW_COLUMNS)
        row.update(self.value, ingest_id=5, boot_id=101, process_id=201, record_seq=22,
                   record_kind=12, schema_version=1, occurrence_utc_usec=contract.UTC_UNKNOWN,
                   ingested_utc_usec=123456)
        self.assertEqual(contract.validate_raw_observation(row), self.value)
        for name, value in (("boot_id", 102), ("process_id", 202), ("record_seq", 0),
                            ("record_kind", 11), ("schema_version", 2), ("occurrence_utc_usec", 0),
                            ("duration_usec", 0), ("bc_damage_dealt", 0), ("ownership_account_token", 0)):
            with self.subTest(name=name):
                with self.assertRaises(contract.BuildContractError):
                    contract.validate_raw_observation(dict(row, **{name: value}))

    def test_sealed_report_definitions_skip_builds_after_validation(self):
        from scripts.telemetry.rollup_definitions import RollupTarget
        from scripts.telemetry.rollup_engine import build_page_contributions, SemanticError
        row = dict(self.value, ingest_id=5, boot_id=101, process_id=201, record_seq=22,
                   record_kind=12, schema_version=1, occurrence_utc_usec=contract.UTC_UNKNOWN)
        for definition in (1, 2, 3, 5):
            with self.subTest(definition=definition):
                page = build_page_contributions([row], RollupTarget(definition, 1, 1, 2))
                self.assertEqual(page.page_last_ingest_id, 5)
                self.assertEqual(page.fetched_rows, 1)
                self.assertEqual(page.battle_inputs, [])
                self.assertEqual(page.identity_inputs, [])
                self.assertEqual(page.state_quality_flags, 0)
                with self.assertRaises(SemanticError):
                    build_page_contributions([dict(row, bctx_association_revision=0)],
                                             RollupTarget(definition, 1, 1, 2))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sanitize", action="store_true")
    arguments, remaining = parser.parse_known_args()
    SANITIZE = arguments.sanitize
    unittest.main(argv=[sys.argv[0], *remaining])
