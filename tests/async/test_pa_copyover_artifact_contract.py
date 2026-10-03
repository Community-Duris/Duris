#!/usr/bin/env python3
"""Client-free checks for frozen copyover artifacts and manual registration."""
from __future__ import annotations

import contextlib
import hashlib
import io
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

TEST_DIRECTORY = Path(__file__).resolve().parent
TESTS_ROOT = TEST_DIRECTORY.parent
sys.path.insert(0, str(TEST_DIRECTORY))
sys.path.insert(0, str(TESTS_ROOT))

import test_pa_copyover_sql as copyover_sql  # noqa: E402
import test_pa_copyover_account_authority as account_authority  # noqa: E402
import run_regression_tests as regression_runner  # noqa: E402

SOURCE_SHA = "a" * 40
BINARY_CONTENT = b"temporary frozen MariaDB game binary fixture\n"


class FrozenArtifactContractTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="pa-copyover-artifact-")
        self.directory = Path(self.temporary.name)
        self.binary = self.directory / "dms-db-test"
        self.binary.write_bytes(BINARY_CONTENT)
        self.binary.chmod(0o755)
        self.digest = hashlib.sha256(BINARY_CONTENT).hexdigest()
        self.descriptor_path = self.directory / "batch-build.json"
        self.descriptor = {
            "head": SOURCE_SHA,
            "status": copyover_sql.PASS_STATUS,
            "checks": [
                {"name": "build", "exit": 0},
                {"name": "client-free-contracts", "exit": 0},
            ],
            "binary": str(self.binary),
            "binary_sha256": self.digest,
            "backend": "mariadb",
            "profile": "development/TEST_MUD",
        }
        self.write_descriptor()

    def tearDown(self):
        self.temporary.cleanup()

    def write_descriptor(self):
        self.descriptor_path.write_text(json.dumps(self.descriptor), encoding="utf-8")

    def load(self):
        return copyover_sql.load_frozen_binary(
            descriptor_path=self.descriptor_path,
            expected_source_sha=SOURCE_SHA,
        )

    def rejects(self, mutate, message=None):
        mutate()
        self.write_descriptor()
        with self.assertRaises(AssertionError, msg=message):
            self.load()

    def test_accepts_parent_frozen_descriptor_and_hashes_real_temporary_binary(self):
        binary, digest, descriptor = self.load()
        self.assertEqual(binary, self.binary)
        self.assertEqual(digest, hashlib.sha256(self.binary.read_bytes()).hexdigest())
        self.assertEqual(descriptor, self.descriptor)

    def test_account_cli_rejects_raw_binary_provenance_claims(self):
        argv = [
            "test_pa_copyover_account_authority.py",
            "--binary", str(self.binary),
            "--expected-sha256", self.digest,
            "--provenance-sha", SOURCE_SHA,
        ]
        with patch.object(sys, "argv", argv), \
                patch.object(account_authority, "run") as run_fixture, \
                contextlib.chdir(copyover_sql.ROOT), \
                contextlib.redirect_stdout(io.StringIO()), \
                contextlib.redirect_stderr(io.StringIO()):
            with self.assertRaises(SystemExit) as error:
                account_authority.main()
        self.assertEqual(error.exception.code, 2)
        run_fixture.assert_not_called()

    def test_account_cli_validates_descriptor_before_invoking_journey(self):
        with patch.object(account_authority, "run") as run_fixture, \
                contextlib.chdir(copyover_sql.ROOT), \
                contextlib.redirect_stdout(io.StringIO()):
            result = account_authority.main([
                "--descriptor", str(self.descriptor_path),
                "--source-sha", SOURCE_SHA,
            ])
        self.assertEqual(result, 0)
        run_fixture.assert_called_once_with(self.binary, self.digest, SOURCE_SHA)

    def test_account_gate_rejects_mismatched_or_unqualified_descriptor_evidence(self):
        baseline = dict(self.descriptor)
        invalid_updates = (
            {"head": "b" * 40},
            {"backend": "flatfile"},
            {"profile": "production"},
            {"status": "BUILD_FAILED"},
            {"checks": [{"name": "build", "exit": False}]},
            {"binary_sha256": "0" * 64},
        )
        for updates in invalid_updates:
            with self.subTest(updates=updates):
                self.descriptor = {**baseline, **updates}
                self.write_descriptor()
                with self.assertRaises(AssertionError):
                    account_authority.parent_artifact(self.descriptor_path, SOURCE_SHA)
        with self.assertRaises(AssertionError):
            account_authority.parent_artifact(self.binary, SOURCE_SHA)

    def test_account_cli_requires_both_descriptor_and_source(self):
        for argv in (["--descriptor", str(self.descriptor_path)],
                     ["--source-sha", SOURCE_SHA]):
            with self.subTest(argv=argv), \
                    patch.object(account_authority, "run") as run_fixture, \
                    contextlib.redirect_stderr(io.StringIO()):
                with self.assertRaises(SystemExit) as error:
                    account_authority.main(argv)
                self.assertEqual(error.exception.code, 2)
                run_fixture.assert_not_called()

    def test_explicit_cli_inputs_validate_then_invoke_the_real_fixture_entrypoint(self):
        with patch.object(copyover_sql.pa_copyover_fixture, "run") as run_fixture:
            output = io.StringIO()
            with contextlib.redirect_stdout(output), contextlib.chdir(copyover_sql.ROOT):
                result = copyover_sql.main([
                    "--descriptor", str(self.descriptor_path),
                    "--source-sha", SOURCE_SHA,
                ])
        self.assertEqual(result, 0)
        run_fixture.assert_called_once_with(self.binary, self.digest)
        self.assertIn(f"base={SOURCE_SHA}", output.getvalue())

    def test_legacy_no_argument_defaults_remain_available(self):
        args = copyover_sql.parse_args([])
        self.assertIsNone(args.descriptor)
        self.assertIsNone(args.source_sha)
        self.assertEqual(copyover_sql.DESCRIPTOR.name, "base-build.json")
        self.assertEqual(copyover_sql.BASE_SHA, "23d086e3a76f2bd424aa4034c5879ceba0d5d594")

    def test_cli_rejects_incomplete_descriptor_source_pair(self):
        with self.assertRaises(SystemExit) as error:
            copyover_sql.parse_args(["--descriptor", str(self.descriptor_path)])
        self.assertEqual(error.exception.code, 2)

    def test_rejects_descriptor_source_mismatch(self):
        self.rejects(lambda: self.descriptor.update(head="b" * 40))

    def test_rejects_invalid_source_sha_format(self):
        with self.assertRaises(AssertionError):
            copyover_sql.load_frozen_binary(self.descriptor_path, "not-a-commit")

    def test_rejects_unpassed_status_backend_and_profile(self):
        baseline = dict(self.descriptor)
        cases = (
            {"status": "BUILD_FAILED"},
            {"backend": "mysql"},
            {"profile": "development/OTHER"},
        )
        for updates in cases:
            with self.subTest(updates=updates):
                self.descriptor = {**baseline, **updates}
                self.write_descriptor()
                with self.assertRaises(AssertionError):
                    self.load()

    def test_rejects_missing_empty_malformed_or_nonzero_check_evidence(self):
        invalid_checks = (None, [], ["exit 0"], [{"name": "build"}],
                          [{"name": "build", "exit": 1}],
                          [{"name": "build", "exit": False}],
                          [{"name": "build", "exit": "0"}])
        for checks in invalid_checks:
            with self.subTest(checks=checks):
                self.descriptor["checks"] = checks
                self.write_descriptor()
                with self.assertRaises(AssertionError):
                    self.load()
        self.descriptor.pop("checks")
        self.write_descriptor()
        with self.assertRaises(AssertionError):
            self.load()

    def test_rejects_missing_or_bad_binary_hash_and_non_sha256_claims(self):
        self.rejects(lambda: self.descriptor.update(binary_sha256="0" * 64))
        self.descriptor["binary_sha256"] = "not-a-sha256"
        self.write_descriptor()
        with self.assertRaises(AssertionError):
            self.load()
        self.descriptor.pop("binary_sha256")
        self.write_descriptor()
        with self.assertRaises(AssertionError):
            self.load()

    def test_rejects_missing_relative_symlink_directory_nonexecutable_and_writable_binary(self):
        missing = self.directory / "missing-binary"
        self.descriptor["binary"] = str(missing)
        self.write_descriptor()
        with self.assertRaises(AssertionError):
            self.load()

        self.descriptor["binary"] = "relative-binary"
        self.write_descriptor()
        with self.assertRaises(AssertionError):
            self.load()

        symlink = self.directory / "binary-link"
        symlink.symlink_to(self.binary)
        self.descriptor["binary"] = str(symlink)
        self.write_descriptor()
        with self.assertRaises(AssertionError):
            self.load()

        self.descriptor["binary"] = str(self.directory)
        self.write_descriptor()
        with self.assertRaises(AssertionError):
            self.load()

        self.descriptor["binary"] = str(self.binary)
        self.binary.chmod(0o644)
        with self.assertRaises(AssertionError):
            self.load()
        self.binary.chmod(0o755)
        self.binary.chmod(0o775)
        with self.assertRaises(AssertionError):
            self.load()

    def test_rejects_non_object_or_invalid_json_descriptor(self):
        self.descriptor_path.write_text("[]", encoding="utf-8")
        with self.assertRaises(AssertionError):
            self.load()
        self.descriptor_path.write_text("{invalid", encoding="utf-8")
        with self.assertRaises(AssertionError):
            self.load()


class ManualRegistrationTests(unittest.TestCase):
    def test_all_rollout_database_journeys_require_explicit_batch_invocation(self):
        leased_journeys = {
            "test_pa_runtime_sql.py",
            "test_pa_copyover_sql.py",
            "test_pa_copyover_account_authority.py",
            "test_pa_necromancy_sql.py",
            "test_pa_item_creation_sql.py",
            "test_pa_item_flags_sql.py",
            "test_pa_atm_publication_sql.py",
            "test_pa_coin_sql.py",
            "test_pa_web_recovery_sql.py",
        }
        self.assertLessEqual(leased_journeys, regression_runner.MANUAL_ONLY_TEST_NAMES)
        discovered_names = {path.name for path in regression_runner.discover_tests(None)}
        self.assertFalse(leased_journeys & discovered_names)
        self.assertIn("test_pa_copyover_artifact_contract.py", discovered_names)

    def test_rollout_sql_and_account_authority_journeys_are_manual_only(self):
        manual = regression_runner.MANUAL_ONLY_TEST_NAMES
        self.assertIn("test_pa_copyover_sql.py", manual)
        self.assertIn("test_pa_copyover_account_authority.py", manual)
        discovered_names = {path.name for path in regression_runner.discover_tests(None)}
        self.assertNotIn("test_pa_copyover_sql.py", discovered_names)
        self.assertNotIn("test_pa_copyover_account_authority.py", discovered_names)
        self.assertIn("test_pa_copyover_artifact_contract.py", discovered_names)


if __name__ == "__main__":
    unittest.main(verbosity=2)
