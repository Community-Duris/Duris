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
    """Exercise audit parity and protected repair guarantees."""

    object_types = {100: 15, 101: 9}
    mobiles = {200}
    allowed = {15, 24, 30, 35}

    def snapshot(self, *, override: int | None = 9, pet_room: int = 50,
                 pet_hit: int = 10, owner_room: int = 50) -> readiness.Snapshot:
        """Build a minimal selectable-character snapshot."""
        return readiness.Snapshot(
            (readiness.Character(1, owner_room), readiness.Character(2, 60)),
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
        """Evaluate a fixture against the active-type test catalog."""
        return readiness.evaluate(
            snapshot, self.object_types, self.mobiles, self.allowed)

    def test_container_override_uses_effective_type_and_clearing_repairs_it(self):
        """Clear only a stale override that turns a container into armor."""
        findings = self.evaluate(self.snapshot())
        self.assertEqual(len(findings), 1)
        self.assertEqual(findings[0].kind, "player_item_type")
        self.assertEqual(findings[0].child_count, 1)
        self.assertTrue(findings[0].repairable)
        self.assertEqual(self.evaluate(self.snapshot(override=None)), [])

    def test_pet_room_and_hit_repairs_match_runtime_bounds(self):
        """Repair pet placement and hit bounds to match runtime acceptance."""
        findings = self.evaluate(self.snapshot(
            override=None, pet_room=49, pet_hit=21))
        self.assertEqual(len(findings), 1)
        self.assertEqual(findings[0].kind, "pet_state")
        self.assertEqual(
            (findings[0].current_a, findings[0].expected_a,
             findings[0].current_b, findings[0].expected_b),
            (49, 50, 21, 20))
        self.assertEqual(self.evaluate(self.snapshot(override=None)), [])

    def test_non_positive_owner_room_is_an_unrepairable_finding(self):
        """Reject a pet whose owner has no positive room to copy."""
        findings = self.evaluate(self.snapshot(override=None, owner_room=0))
        self.assertEqual(len(findings), 1)
        self.assertEqual(findings[0].kind, "pet_invalid_bounds")
        self.assertFalse(findings[0].repairable)

    def test_unrepairable_unknown_prototype_blocks_manifest(self):
        """Block manifests when an unknown object prototype has no safe repair."""
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
        """Authenticate manifests and keep their identifying rows private."""
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

    def test_secure_destination_rejects_unsafe_or_existing_paths(self):
        """Reject every unsafe receipt destination before artifact creation."""
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            protected = root / "protected"
            protected.mkdir(mode=0o700)
            existing = protected / "existing.receipt"
            existing.write_bytes(b"existing")
            open_parent = root / "open"
            open_parent.mkdir(mode=0o755)
            os.chmod(open_parent, 0o755)
            linked_parent = root / "linked"
            linked_parent.symlink_to(protected, target_is_directory=True)
            cases = (
                (Path("relative.receipt"), "absolute"),
                (open_parent / "receipt", "owner-only"),
                (linked_parent / "receipt", "symbolic links"),
                (existing, "already exists"),
            )
            for path, message in cases:
                with self.subTest(path=path), self.assertRaisesRegex(
                        readiness.ReadinessError, message):
                    readiness._validate_secure_create_destination(
                        path, "repair receipt")

    def test_invalid_receipt_blocks_repair_transaction(self):
        """Leave the database untouched when the receipt cannot be created."""
        findings = self.evaluate(self.snapshot())
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            os.chmod(root, 0o700)
            receipt = root / "receipt.txt"
            receipt.write_bytes(b"existing")
            arguments = readiness.argparse.Namespace(
                env_file=root / "env",
                write_manifest=None,
                apply=True,
                manifest=root / "manifest",
                manifest_sha256="1" * 64,
                backup=root / "backup.sql.gz",
                backup_sha256="2" * 64,
                confirm_database="duris_test",
                authorize_production_repair=False,
                receipt=receipt,
            )
            config = {
                "DB_NAME": "duris_test",
                "ENVIRONMENT": "development",
            }
            constants = {
                "ITEM_CONTAINER": 15,
                "ITEM_QUIVER": 24,
                "ITEM_STORAGE": 30,
                "ITEM_CORPSE": 35,
            }
            with mock.patch.object(readiness, "parse_arguments", return_value=arguments), \
                    mock.patch.object(readiness, "read_env_file", return_value=config), \
                    mock.patch.object(readiness, "active_object_types",
                                      return_value=self.object_types), \
                    mock.patch.object(readiness, "active_mobile_vnums",
                                      return_value=self.mobiles), \
                    mock.patch.object(readiness, "parse_defines", return_value=constants), \
                    mock.patch.object(readiness, "database_snapshot",
                                      return_value=self.snapshot()), \
                    mock.patch.object(readiness, "read_manifest",
                                      return_value=findings), \
                    mock.patch.object(readiness, "validate_backup", return_value="2" * 64), \
                    mock.patch.object(readiness, "active_connections", return_value=0), \
                    mock.patch.object(readiness, "apply_repair") as apply_repair, \
                    mock.patch("builtins.print"):
                self.assertEqual(readiness.main(), 2)
                apply_repair.assert_not_called()

    def test_targeted_transaction_guards_unaffected_row_fingerprints(self):
        """Guard every unaffected materialization row inside the repair transaction."""
        findings = self.evaluate(self.snapshot(pet_room=49, pet_hit=21))
        columns = iter(("pid\nlast_room", "id\npid\nvnum\nitem_type",
                        "id\nowner_pid\nroom_vnum\nhit", "id\npet_id\nvnum"))
        statements: list[str] = []

        def fake_mysql(_config, statement):
            """Capture generated DML while returning deterministic metadata."""
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
        """Require verifiable TLS for every remote readiness target."""
        config = {"DB_HOST": "database.internal", "DB_PORT": "3306"}
        with self.assertRaisesRegex(readiness.ReadinessError, "require TLS"):
            readiness.connection_arguments(config)

    def test_area_file_names_supports_strict_mobile_manifests(self):
        """Share manifest parsing while making missing mobile files fatal."""
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            area_root = root / "mob"
            area_root.mkdir()
            area_list = root / "AREA"
            area_list.write_text("alpha\nbeta\n", encoding="utf-8")
            (area_root / "alpha.mob").write_text("#200\n", encoding="utf-8")
            with self.assertRaises(FileNotFoundError):
                readiness.area_file_names(
                    area_root, area_list, extension=".mob", require_all=True)
            self.assertEqual(
                readiness.area_file_names(
                    area_root, area_list, extension=".mob", require_all=False),
                [area_root / "alpha.mob"],
            )


if __name__ == "__main__":
    unittest.main()
