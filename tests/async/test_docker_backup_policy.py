#!/usr/bin/env python3
"""Focused checks for the explicitly approved local Docker backup policy."""
from __future__ import annotations

import json
import os
from pathlib import Path
import stat
import sys
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))

from init_docker_backup_policy import (  # noqa: E402
    APPROVAL_TOKEN,
    PolicyProvisionError,
    provision_policy,
)
from persistence_backup import policy_load  # noqa: E402


class DockerBackupPolicyTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temp_directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp_directory.cleanup)
        self.runtime_directory = Path(self.temp_directory.name) / "runtime"
        self.runtime_directory.mkdir(mode=0o700)
        os.chmod(self.runtime_directory, 0o700)
        self.policy_path = self.runtime_directory / "harness-backup-policy.json"

    def test_policy_requires_explicit_local_volume_approval(self) -> None:
        with self.assertRaisesRegex(PolicyProvisionError, "approval is required"):
            provision_policy(self.policy_path, "", "developer")
        self.assertFalse(self.policy_path.exists())

    def test_approved_policy_is_private_and_accepted_by_runtime_loader(self) -> None:
        created = provision_policy(
            self.policy_path, APPROVAL_TOKEN, "local-developer"
        )

        self.assertTrue(created)
        self.assertEqual(stat.S_IMODE(self.policy_path.stat().st_mode), 0o600)
        policy = json.loads(self.policy_path.read_text(encoding="utf-8"))
        self.assertIs(policy["approved"], True)
        self.assertEqual(policy["custodian"], "local-developer")
        self.assertEqual(policy["root"], "/var/lib/duris/db-backups")
        self.assertEqual(policy_load(self.policy_path)["custodian"], "local-developer")

    def test_existing_policy_is_validated_and_never_overwritten(self) -> None:
        provision_policy(self.policy_path, APPROVAL_TOKEN, "local-developer")
        original = self.policy_path.read_bytes()

        created = provision_policy(self.policy_path, "", "")

        self.assertFalse(created)
        self.assertEqual(self.policy_path.read_bytes(), original)

    def test_invalid_existing_policy_is_preserved_and_rejected(self) -> None:
        invalid_policy = {
            "version": 1,
            "approved": False,
            "custodian": "SET_BY_OPERATOR",
        }
        self.policy_path.write_text(json.dumps(invalid_policy), encoding="utf-8")
        os.chmod(self.policy_path, 0o600)
        original = self.policy_path.read_bytes()

        with self.assertRaisesRegex(PolicyProvisionError, "refusing to replace"):
            provision_policy(self.policy_path, APPROVAL_TOKEN, "local-developer")

        self.assertEqual(self.policy_path.read_bytes(), original)


if __name__ == "__main__":
    unittest.main()
