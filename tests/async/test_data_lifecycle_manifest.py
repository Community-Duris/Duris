#!/usr/bin/env python3
"""Focused fail-closed regressions for the lifecycle manifest contract."""

from __future__ import annotations

from _paths import SRC
import json
import importlib.util
import os
import subprocess
import tempfile
import unittest
from pathlib import Path
from unittest import mock


ROOT = Path(__file__).resolve().parents[2]
VALIDATOR = ROOT / "scripts" / "validate_data_lifecycle.py"
MANIFEST = ROOT / "migrations" / "data_lifecycle_manifest.json"
REDIS_REGISTRY = SRC / "redis_key_registry.def"
SCHEMA_FILES = (
    ROOT / "migrations" / "bootstrap_multithread_safe.sql",
    ROOT / "migrations" / "bootstrap_legacy_baseline.sql",
    ROOT / "migrations" / "immutable" / "0001_lookup_dataset_state.sql",
    ROOT / "migrations" / "immutable" / "0003_season_reset_state.sql",
    ROOT / "migrations" / "immutable" / "0004_server_reboots.sql",
    ROOT / "migrations" / "immutable" / "0006_kingdom_realms.sql",
)
VALIDATOR_SPEC = importlib.util.spec_from_file_location("validate_data_lifecycle", VALIDATOR)
VALIDATOR_MODULE = importlib.util.module_from_spec(VALIDATOR_SPEC)
assert VALIDATOR_SPEC.loader is not None
VALIDATOR_SPEC.loader.exec_module(VALIDATOR_MODULE)


class LifecycleManifestTest(unittest.TestCase):
    def setUp(self) -> None:
        self.manifest = json.loads(MANIFEST.read_text())

    def run_validator(
        self,
        manifest: Path = MANIFEST,
        schema_files: tuple[Path, ...] = SCHEMA_FILES,
        preflight: tuple[str, str] | None = None,
        redis_registry: Path = REDIS_REGISTRY,
    ) -> subprocess.CompletedProcess[str]:
        command = ["python3", str(VALIDATOR), "--manifest", str(manifest),
                   "--redis-registry", str(redis_registry), "--json"]
        for schema_file in schema_files:
            command.extend(("--schema-file", str(schema_file)))
        if preflight:
            command.extend(("--destructive-preflight", *preflight))
        environment = {
            "PATH": os.environ.get("PATH", ""),
            "ENVIRONMENT": "test",
            "DB_HOST": "127.0.0.1",
            "LIFECYCLE_ROLE": "lifecycle-admin",
        }
        return subprocess.run(command, capture_output=True, text=True, env=environment,
                              check=False)

    def write_manifest(self, directory: Path, value: dict) -> Path:
        path = directory / "manifest.json"
        path.write_text(json.dumps(value))
        return path

    def entry(self, entry_id: str) -> dict:
        return next(entry for entry in self.manifest["entries"] if entry["id"] == entry_id)

    def assert_rejected(self, result: subprocess.CompletedProcess[str], message: str) -> None:
        self.assertEqual(result.returncode, 2, result.stdout)
        self.assertIn(message, result.stderr)

    def test_canonical_inventory_passes_and_reports_only_counts(self) -> None:
        result = self.run_validator()
        self.assertEqual(result.returncode, 0, result.stderr)
        report = json.loads(result.stdout)
        self.assertEqual(report["database_tables"], 174)
        self.assertEqual(report["non_database_stores"], 21)
        self.assertEqual(report["redis_surfaces"], 42)
        self.assertFalse(report["destructive_rules_enabled"])

    def test_missing_duplicate_unknown_and_stale_rules_fail_closed(self) -> None:
        mutations = []

        missing = json.loads(json.dumps(self.manifest))
        missing["entries"] = [entry for entry in missing["entries"]
                              if entry["id"] != "database:accounts"]
        mutations.append((missing, "schema coverage mismatch"))

        duplicate = json.loads(json.dumps(self.manifest))
        duplicate["entries"].append(duplicate["entries"][0])
        mutations.append((duplicate, "duplicate store id"))

        unknown_action = json.loads(json.dumps(self.manifest))
        unknown_action["entries"][0]["season_action"] = "vacuum_everything"
        mutations.append((unknown_action, "unknown season action"))

        stale = json.loads(json.dumps(self.manifest))
        stale["schema_version"] = 0
        mutations.append((stale, "unsupported or stale lifecycle policy version"))

        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            for index, (manifest, message) in enumerate(mutations):
                path = directory / f"manifest-{index}.json"
                path.write_text(json.dumps(manifest))
                self.assert_rejected(self.run_validator(path), message)

    def test_dependency_drift_and_cycles_fail_closed(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            omitted = json.loads(json.dumps(self.manifest))
            target = next(entry for entry in omitted["entries"]
                          if entry["id"] == "database:account_banks")
            target["dependencies"] = []
            self.assert_rejected(
                self.run_validator(self.write_manifest(directory, omitted)),
                "omits schema dependencies",
            )

            missing = json.loads(json.dumps(self.manifest))
            missing_target = next(entry for entry in missing["entries"]
                                  if entry["id"] == "database:critical_test_state")
            missing_target["dependencies"] = ["database:not_a_table"]
            self.assert_rejected(
                self.run_validator(self.write_manifest(directory, missing)),
                "missing dependency",
            )

            cycle = json.loads(json.dumps(self.manifest))
            first = next(entry for entry in cycle["entries"]
                         if entry["id"] == "database:critical_test_state")
            second = next(entry for entry in cycle["entries"]
                          if entry["id"] == "database:mud_info")
            first["dependencies"] = [second["id"]]
            second["dependencies"] = [first["id"]]
            self.assert_rejected(
                self.run_validator(self.write_manifest(directory, cycle)),
                "dependency cycle",
            )

    def test_unapproved_and_protected_destructive_rules_fail_closed(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            unapproved = json.loads(json.dumps(self.manifest))
            target = next(entry for entry in unapproved["entries"]
                          if entry["id"] == "database:accounts")
            target["terminal_action"] = "purge"
            self.assert_rejected(
                self.run_validator(self.write_manifest(directory, unapproved)),
                "unapproved destructive rule",
            )

            protected = json.loads(json.dumps(self.manifest))
            target = next(entry for entry in protected["entries"] if entry["protected_record"])
            target["terminal_action"] = "purge"
            target["controller_decision"] = {
                "status": "approved",
                "reference": "TEST-APPROVAL",
            }
            self.assert_rejected(
                self.run_validator(self.write_manifest(directory, protected)),
                "protected store",
            )

    def test_destructive_preflight_requires_global_approval(self) -> None:
        approved_entry = json.loads(json.dumps(self.manifest))
        target = next(entry for entry in approved_entry["entries"]
                      if entry["id"] == "database:accounts")
        target["terminal_action"] = "purge"
        target["controller_decision"] = {
            "status": "approved",
            "reference": "TEST-APPROVAL",
        }
        with tempfile.TemporaryDirectory() as temporary:
            path = self.write_manifest(Path(temporary), approved_entry)
            self.assert_rejected(
                self.run_validator(path, preflight=("database:accounts", "purge")),
                "controller approval does not enable destructive rules",
            )

    def test_destructive_preflight_rejects_environment_host_and_role(self) -> None:
        manifest = json.loads(json.dumps(self.manifest))
        manifest["controller_approval"] = {
            "status": "approved",
            "reference": "TEST-GLOBAL-APPROVAL",
            "destructive_rules_enabled": True,
        }
        target = self.entry("database:accounts")
        target["terminal_action"] = "purge"
        target["controller_decision"] = {
            "status": "approved",
            "reference": "TEST-ENTRY-APPROVAL",
        }
        entries = {entry["id"]: entry for entry in self.manifest["entries"]}
        cases = (
            ({"ENVIRONMENT": "production", "DB_HOST": "127.0.0.1",
              "LIFECYCLE_ROLE": "lifecycle-admin"}, "non-production"),
            ({"ENVIRONMENT": "test", "DB_HOST": "database.internal",
              "LIFECYCLE_ROLE": "lifecycle-admin"}, "loopback"),
            ({"ENVIRONMENT": "test", "DB_HOST": "127.0.0.1",
              "LIFECYCLE_ROLE": "operator"}, "lifecycle-admin"),
        )
        for environment, message in cases:
            with mock.patch.dict(os.environ, environment, clear=True):
                with self.assertRaisesRegex(VALIDATOR_MODULE.ValidationError, message):
                    VALIDATOR_MODULE.destructive_preflight(
                        manifest, entries, "database:accounts", "purge"
                    )

    def test_schema_drift_and_symlink_manifest_fail_closed(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            schema = directory / "schema.sql"
            schema.write_text(SCHEMA_FILES[0].read_text() + "\nCREATE TABLE lifecycle_drift(id INT);\n")
            self.assert_rejected(
                self.run_validator(schema_files=(schema, SCHEMA_FILES[1])),
                "schema coverage mismatch",
            )

            link = directory / "manifest-link.json"
            link.symlink_to(MANIFEST)
            self.assert_rejected(self.run_validator(link), "regular non-symlink file")

    def test_schema_scan_ignores_prose_in_whole_line_comments(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            schema = directory / "commented.sql"
            schema.write_text(
                "-- CREATE TABLE IF NOT EXISTS is a no-op on an existing table, and\n"
                "-- DROP TABLE lifecycle_commented would remove it.\n"
                "CREATE TABLE lifecycle_commented (\n"
                "  id INT NOT NULL,\n"
                "  -- REFERENCES lifecycle_ghost is only described here\n"
                "  PRIMARY KEY (id)\n"
                ") ENGINE=InnoDB;\n"
            )
            self.assertEqual(VALIDATOR_MODULE.schema_tables((schema,)),
                             {"lifecycle_commented"})
            self.assertEqual(VALIDATOR_MODULE.schema_dependencies((schema,)), {})

    def test_redis_registry_drives_manifest_coverage_and_fails_closed(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            registry = REDIS_REGISTRY.read_text(encoding="ascii")
            omitted_store = directory / "omitted-store.def"
            omitted_store.write_text(
                "\n".join(
                    line for line in registry.splitlines()
                    if "REDIS_STORE(CONTENT_CACHE" not in line
                ) + "\n",
                encoding="ascii",
            )
            self.assert_rejected(
                self.run_validator(redis_registry=omitted_store),
                "Redis registry coverage mismatch",
            )

            added_store = directory / "added-store.def"
            added_store.write_text(
                registry +
                'REDIS_STORE(TEST_ONLY, "redis:test_only", "test-only Redis store", '
                '"redis_keyspace")\n'
                'REDIS_SURFACE(TEST_ONLY, "test:key", "test:key", TEST_ONLY, "key", '
                '"active")\n',
                encoding="ascii",
            )
            self.assert_rejected(
                self.run_validator(redis_registry=added_store),
                "non-database coverage mismatch",
            )

    def test_personal_and_protected_records_have_required_evidence(self) -> None:
        personal = [entry for entry in self.manifest["entries"]
                    if entry["data_subject_key"] != "not_applicable"]
        self.assertTrue(personal)
        for entry in personal:
            self.assertTrue(entry["technical_purpose"])
            self.assertTrue(entry["controller_decision"]["reference"])
            self.assertTrue(entry["active_retention"])
            self.assertTrue(entry["archive_retention"])
        for entry in self.manifest["entries"]:
            if entry["protected_record"]:
                self.assertEqual(entry["terminal_action"], "retain")
                self.assertIn("replay", entry["exception"])

    def test_export_rules_are_exact_and_secret_exclusions_fail_closed(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            missing = json.loads(json.dumps(self.manifest))
            del missing["entries"][0]["export_rule"]
            self.assert_rejected(
                self.run_validator(self.write_manifest(directory, missing)),
                "export_rule",
            )

            secret = json.loads(json.dumps(self.manifest))
            accounts = next(entry for entry in secret["entries"]
                            if entry["id"] == "database:accounts")
            accounts["export_rule"]["excluded_fields"].remove("password")
            self.assert_rejected(
                self.run_validator(self.write_manifest(directory, secret)),
                "omits secret exclusions",
            )

            overlap = json.loads(json.dumps(self.manifest))
            accounts = next(entry for entry in overlap["entries"]
                            if entry["id"] == "database:accounts")
            accounts["export_rule"]["shared_fields"] = ["password"]
            self.assert_rejected(
                self.run_validator(self.write_manifest(directory, overlap)),
                "shares an explicitly excluded field",
            )


if __name__ == "__main__":
    unittest.main()
