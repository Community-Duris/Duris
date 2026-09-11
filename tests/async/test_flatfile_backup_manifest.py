#!/usr/bin/env python3
"""Managed flat-file capture covers locks, transaction state and tamper refusal.

Native replay and service boot are in test_persistence_backup_integration.py.
"""
from unittest import mock
import unittest
from test_persistence_backup import Fixture, backup


class FlatfileManifestTests(Fixture):
    def test_manifest_covers_authority_and_pending_transaction(self):
        live = self.base / "live"
        (live / "domains/.critical-authority-transaction").write_bytes(b"synthetic-pending-transaction")
        for relative in ("identities/names/.identity.lock", "identities/accounts/.accounts.lock",
                         "domains/.critical-authority.lock"):
            (live / relative).touch(mode=0o600)
        generation = self.create()
        meta = backup.verify(generation)
        self.assertTrue(meta["pending_transaction"])
        self.assertEqual(backup.inventory(live, exclude_locks=True), backup.inventory(generation / "state"))
        self.assertIn("state/domains/locker_catalog", meta["files"])
        self.assertIn("state/domains/.critical-authority-transaction", meta["files"])
        self.assertFalse(any(key.split("/")[-1] in backup.LOCKS for key in meta["files"]))
        (generation / "state/domains/locker_catalog").write_bytes(b"corrupted")
        with self.assertRaisesRegex(backup.BackupError, "checksum"):
            backup.verify(generation)

    def test_authority_writer_overlap_does_not_publish(self):
        path = self.base / "live/domains/.critical-authority.lock"
        real_lock = backup.lock
        # Keep real flock exclusion, but eliminate the operational 120-second
        # timeout from this deterministic unit test.
        with real_lock(path):
            with mock.patch.object(backup, "lock", lambda path, wait=0: real_lock(path, wait=0)):
                with self.assertRaisesRegex(backup.BackupError, "authority_busy"):
                    self.create()
        self.assertFalse(backup.generations(self.p["root"]))

    def test_writer_change_during_copy_refuses_mixed_generation(self):
        original = backup.shutil.copytree
        def changed_copy(source, target, *args, **kwargs):
            result = original(source, target, *args, **kwargs)
            if source == self.base / "live":
                (source / "players/42").write_bytes(b"changed-by-synthetic-writer")
            return result
        with mock.patch.object(backup.shutil, "copytree", changed_copy):
            with self.assertRaisesRegex(backup.BackupError, "generation_changed"):
                self.create()
        self.assertFalse(backup.generations(self.p["root"]))


if __name__ == "__main__":
    unittest.main()
