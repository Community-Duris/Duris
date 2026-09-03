#!/usr/bin/env python3
"""Focused regressions for import-time character materialization readiness."""

from __future__ import annotations

import importlib.util
import os
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))
SPEC = importlib.util.spec_from_file_location(
    "character_materialization_readiness",
    ROOT / "scripts/character_materialization_readiness.py")
assert SPEC is not None and SPEC.loader is not None
readiness = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = readiness
SPEC.loader.exec_module(readiness)


class CharacterMaterializationReadinessTest(unittest.TestCase):
    object_types = {100: 15, 101: 9}
    mobiles = {200}
    allowed = {15, 24, 30, 35}

    def snapshot(self, *, override: int | None = 9, pet_room: int = 50,
                 pet_hit: int = 10) -> readiness.Snapshot:
        return readiness.Snapshot(
            (readiness.Character(1, 50), readiness.Character(2, 60)),
            (
                readiness.Item(10, 1, 100, None, override),
                readiness.Item(11, 1, 101, 10, None),
                readiness.Item(12, 2, 101, None, None),
            ),
            (readiness.Pet(20, 1, 200, 0, pet_hit, 20, 3, 5, 4, 6, 12,
                           pet_room),),
            (),
        )

    def evaluate(self, snapshot: readiness.Snapshot) -> list[readiness.Finding]:
        return readiness.evaluate(
            snapshot, self.object_types, self.mobiles, self.allowed)

    def test_container_override_uses_effective_type_and_clearing_repairs_it(self):
        findings = self.evaluate(self.snapshot())
        self.assertEqual(len(findings), 1)
        self.assertEqual(findings[0].kind, "player_item_type")
        self.assertEqual(findings[0].child_count, 1)
        self.assertTrue(findings[0].repairable)
        self.assertEqual(self.evaluate(self.snapshot(override=None)), [])

    def test_pet_room_and_hit_repairs_match_runtime_bounds(self):
        findings = self.evaluate(self.snapshot(
            override=None, pet_room=49, pet_hit=21))
        self.assertEqual(len(findings), 1)
        self.assertEqual(findings[0].kind, "pet_state")
        self.assertEqual(
            (findings[0].current_a, findings[0].expected_a,
             findings[0].current_b, findings[0].expected_b),
            (49, 50, 21, 20))
        self.assertEqual(self.evaluate(self.snapshot(override=None)), [])

    def test_unrepairable_unknown_prototype_blocks_manifest(self):
        snapshot = self.snapshot(override=None)
        broken = readiness.Snapshot(
            snapshot.characters,
            snapshot.items + (readiness.Item(13, 2, 999999, None, None),),
            snapshot.pets, snapshot.pet_items)
        findings = self.evaluate(broken)
        self.assertEqual(findings[0].kind, "player_unknown_object")
        with tempfile.TemporaryDirectory() as temporary:
            os.chmod(temporary, 0o700)
            with self.assertRaisesRegex(readiness.ReadinessError, "unrepairable"):
                readiness.write_manifest(
                    Path(temporary) / "manifest.tsv", "duris_test", findings)

    def test_manifest_is_owner_only_hashed_and_exact(self):
        findings = self.evaluate(self.snapshot())
        with tempfile.TemporaryDirectory() as temporary:
            os.chmod(temporary, 0o700)
            path = Path(temporary) / "manifest.tsv"
            digest = readiness.write_manifest(path, "duris_test", findings)
            self.assertEqual(os.stat(path).st_mode & 0o777, 0o600)
            self.assertEqual(
                readiness.read_manifest(path, digest, "duris_test"), findings)
            with self.assertRaisesRegex(readiness.ReadinessError, "digest"):
                readiness.read_manifest(path, "0" * 64, "duris_test")

    def test_targeted_transaction_guards_unaffected_row_fingerprints(self):
        findings = self.evaluate(self.snapshot(pet_room=49, pet_hit=21))
        columns = iter(("pid\nlast_room", "id\npid\nvnum\nitem_type",
                        "id\nowner_pid\nroom_vnum\nhit", "id\npet_id\nvnum"))
        statements: list[str] = []

        def fake_mysql(_config, statement):
            if "information_schema.columns" in statement:
                return next(columns)
            statements.append(statement)
            return "1\t0\t0"

        with mock.patch.object(readiness, "run_mysql", side_effect=fake_mysql):
            readiness.apply_repair({}, findings)
        self.assertEqual(len(statements), 1)
        sql = statements[0]
        self.assertIn("START TRANSACTION", sql)
        self.assertIn("UPDATE player_items SET item_type=NULL", sql)
        self.assertIn("id=10 AND pid=1 AND vnum=100 AND item_type=9", sql)
        self.assertIn("UPDATE player_pets SET room_vnum=CASE id", sql)
        self.assertIn("pd.last_room=50", sql)
        self.assertIn("_materialization_unaffected_before", sql)
        self.assertIn("INSERT INTO _materialization_guard(ok) SELECT 1", sql)
        self.assertLess(sql.index("_materialization_guard"), sql.rindex("COMMIT"))

    def test_remote_database_cannot_disable_transport_verification(self):
        config = {"DB_HOST": "database.internal", "DB_PORT": "3306"}
        with self.assertRaisesRegex(readiness.ReadinessError, "require TLS"):
            readiness.connection_arguments(config)


if __name__ == "__main__":
    unittest.main()
