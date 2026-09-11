#!/usr/bin/env python3
"""Regression contracts for the review findings on the backup/recovery slice."""

from __future__ import annotations

import contextlib
import fcntl
import io
import json
import multiprocessing
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import threading
import time
import unittest
from unittest import mock

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))

import persistence_backup as backup  # noqa: E402
import persistence_restore as restore  # noqa: E402
from test_persistence_backup import Fixture, policy, provision  # noqa: E402


class BackupReviewRemediationTests(Fixture):
    def setUp(self):
        super().setUp()
        self.journal_environment = mock.patch.dict(os.environ, {
            "PLAYER_SAVE_JOURNAL_DIR": "",
            "CRITICAL_COMMAND_JOURNAL_DIR": "",
        })
        self.journal_environment.start()
        self.addCleanup(self.journal_environment.stop)

    def test_old_empty_tombstone_evidence_is_rejected(self):
        now = int(time.time())
        path = self.base / "erasure-current.json"
        backup.write_json(path, {
            "version": 1,
            "captured_at": now - 301,
            "policy_sha256": backup.digest(ROOT / "migrations/data_lifecycle_manifest.json"),
            "tombstones": [],
        })
        with self.assertRaisesRegex(backup.BackupError, "tombstone_evidence_stale"):
            restore.tombstone_preflight(path, self.p, now - self.p["rpo_seconds"])

    def test_policy_requires_both_journal_roots(self):
        value = policy(self.base)
        value["journal_roots"] = {
            "players": self.base / "player-journal",
            "critical": self.base / "critical-journal",
        }
        for path in value["journal_roots"].values():
            path.mkdir(mode=0o700)
        config = self.base / "policy.json"
        config.write_text(json.dumps(value, default=str))
        config.chmod(0o600)
        for roots in ({"players": value["journal_roots"]["players"]},
                      {"critical": value["journal_roots"]["critical"]}, {}):
            with self.subTest(roots=roots), self.assertRaisesRegex(backup.BackupError,
                                                                    "invalid_journal_roots"):
                broken = dict(value, journal_roots=roots)
                config.write_text(json.dumps(broken, default=str))
                backup.policy_load(config)

    def test_journal_temp_file_never_enters_a_generation(self):
        players = self.base / "player-journal"
        critical = self.base / "critical-journal"
        players.mkdir(mode=0o700)
        critical.mkdir(mode=0o700)
        (players / "player-save.journal.tmp").write_bytes(b"unfinished")
        stage = self.base / "stage"
        stage.mkdir(mode=0o700)
        value = dict(self.p, journal_roots={"players": players, "critical": critical})
        with self.assertRaisesRegex(backup.BackupError, "journal_filename"):
            backup.journal_capture(stage, value)

    def test_journal_capture_preserves_locker_receipts_and_empty_service_lock(self):
        critical = self.p["journal_roots"]["critical"]
        store = critical / "locker-identification"
        store.mkdir(mode=0o700)
        (store / ".service-lock").touch(mode=0o600)
        (store / "42.receipt").write_bytes(b"synthetic bounded receipt; native validation is separate")
        stage = self.base / "stage"
        stage.mkdir(mode=0o700)
        captured = backup.journal_capture(stage, self.p)
        self.assertEqual(backup.inventory(critical), captured["critical"])
        self.assertEqual(backup.inventory(stage / "journals/critical"), captured["critical"])

    def test_journal_capture_rejects_invalid_locker_entries(self):
        store = self.p["journal_roots"]["critical"] / "locker-identification"
        store.mkdir(mode=0o700)
        for index, (name, payload, code) in enumerate((
                (".service-lock", b"data", "journal_service_lock_nonempty"),
                ("0.receipt", b"data", "journal_filename"),
                ("2147483648.receipt", b"data", "journal_receipt_pid"),
                ("42.receipt", b"", "journal_receipt_size"),
                ("42.receipt", b"x" * (70 * 1024), "journal_receipt_size"),
                ("unexpected", b"data", "journal_filename"))):
            with self.subTest(name=name, code=code):
                entry = store / name
                entry.write_bytes(payload)
                stage = self.base / f"stage-{index}"
                stage.mkdir(mode=0o700)
                with self.assertRaisesRegex(backup.BackupError, code):
                    backup.journal_capture(stage, self.p)
                entry.unlink()

    def test_expired_generations_are_pruned_only_after_successful_capture(self):
        now = int(time.time())
        old = [self.create("flatfile-primary", now - 90 * 86400 + offset)
               for offset in (0, 1, 2)]
        retained_size = sum(backup.total_size(path) for path in old[:2])
        self.p["max_bytes"] = retained_size + backup.total_size(self.base / "live") + 512 * 1024
        before = {path: backup.inventory(path) for path in old}
        with mock.patch.object(backup, "checkpoint", side_effect=lambda stage:
                              (_ for _ in ()).throw(OSError("capture_failed"))
                              if stage == "before_capture" else None), \
             self.assertRaisesRegex(OSError, "capture_failed"):
            self.create("flatfile-primary")
        self.assertEqual({path: backup.inventory(path) for path in old}, before)
        newest = self.create("flatfile-primary")
        self.assertTrue(newest.is_dir())
        generations = {path for path, _ in backup.generations(self.p["root"])}
        self.assertIn(newest, generations)
        self.assertNotIn(old[0], generations)

    def test_replication_failure_keeps_local_generation_and_status(self):
        first = self.create("flatfile-primary")
        replica = self.base / "replica"
        replica.mkdir(mode=0o700)
        self.p["replica_root"] = replica
        with mock.patch.object(backup, "replicate",
                               side_effect=backup.BackupError("replication_failed")):
            result = backup.backup(self.p, "flatfile-primary")
        self.assertEqual(result["result"], "replication_pending")
        self.assertEqual(result["replica"], "pending")
        status = json.loads((self.p["root"] / "status.json").read_text())
        self.assertEqual(status["generation"], result["generation"])
        self.assertEqual(status["replica"], "pending")
        self.assertEqual(status["result"], "replication_pending")
        self.assertTrue(first.is_dir())
        self.assertEqual(len(backup.generations(self.p["root"])), 2)
        with mock.patch.object(backup, "replicate",
                               return_value="transport_and_readback_verified"):
            retried = backup.backup(self.p, "flatfile-primary")
        self.assertEqual(retried["result"], "ok")
        self.assertEqual(retried["replica"], "transport_and_readback_verified")
        status = json.loads((self.p["root"] / "status.json").read_text())
        self.assertEqual(status["result"], "ok")
        self.assertEqual(status["replica"], "transport_and_readback_verified")

    def test_replication_stage_is_removed_when_publication_fails(self):
        source = self.create("flatfile-primary")
        replica = self.base / "replica"
        replica.mkdir(mode=0o700)
        value = dict(self.p, replica_root=replica)
        with mock.patch.object(backup, "run", return_value=b"fuse.sshfs"), \
             mock.patch.object(backup, "sync_tree", side_effect=backup.BackupError("sync_failed")), \
             self.assertRaisesRegex(backup.BackupError, "sync_failed"):
            backup.replicate(source, value)
        self.assertFalse(list(replica.glob(".staging-*")))

    def test_bounded_dump_output_checks_disk_capacity_periodically(self):
        stream = io.BytesIO()
        output = backup.BoundedOutput(stream, self.base, 128 * 1024 * 1024, 0)
        with mock.patch.object(backup.shutil, "disk_usage",
                               return_value=mock.Mock(free=128 * 1024 * 1024)) as usage:
            for _ in range(40):
                output.write(b"x" * (1024 * 1024))
        self.assertLessEqual(usage.call_count, 3)
        self.assertEqual(len(stream.getvalue()), 40 * 1024 * 1024)
        guarded = backup.BoundedOutput(io.BytesIO(), self.base, 1024, 1)
        with mock.patch.object(backup.shutil, "disk_usage",
                               return_value=mock.Mock(free=1)):
            with self.assertRaisesRegex(backup.BackupError, "low_free_capacity"):
                guarded.write(b"x")

    def test_operational_status_waits_for_a_separate_job_lock_holder(self):
        self.create("flatfile-primary")
        lock_path = self.p["root"] / ".job.lock"
        ready = multiprocessing.Event()
        release = multiprocessing.Event()

        def hold(path, ready_event, release_event):
            fd = os.open(path, os.O_RDWR | os.O_CREAT, 0o600)
            try:
                fcntl.flock(fd, fcntl.LOCK_EX)
                ready_event.set()
                release_event.wait(5)
                fcntl.flock(fd, fcntl.LOCK_UN)
            finally:
                os.close(fd)

        context = multiprocessing.get_context("fork")
        process = context.Process(target=hold, args=(str(lock_path), ready, release))
        process.start()
        try:
            self.assertTrue(ready.wait(5))
            with mock.patch.object(backup, "LOCK_WAIT_SECONDS", 1):
                timer = threading.Timer(0.25, release.set)
                timer.start()
                started = time.monotonic()
                result = backup.status(self.p)
                elapsed = time.monotonic() - started
                timer.cancel()
            self.assertEqual(result["result"], "ok")
            self.assertGreaterEqual(elapsed, 0.20)
        finally:
            release.set()
            process.join(5)
            if process.is_alive():
                process.terminate()
                process.join(5)
        self.assertEqual(process.exitcode, 0)

    def test_restore_uses_candidate_owned_tmpdir(self):
        candidate = self.base / "candidate"
        candidate.mkdir(mode=0o700)
        environment = restore.clean_environment(candidate)
        self.assertEqual(environment["TMPDIR"], str(candidate / "tmp"))
        self.assertTrue((candidate / "tmp").is_dir())

    def test_restore_wrapper_without_policy_is_controlled(self):
        tombstones = self.base / "tombstones.json"
        tombstones.write_text("{}")
        result = subprocess.run(
            ["bash", str(ROOT / "scripts/restore_flatfile_backup.sh"), "generation", str(tombstones)],
            env={"PATH": os.environ.get("PATH", "/usr/bin:/bin")},
            text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=30,
        )
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("backup_policy_required", result.stdout)
        self.assertNotIn("unbound variable", result.stdout)

    def test_systemd_drill_and_cycle_coordinate_backup_contention(self):
        drill = (ROOT / "deploy/systemd/duris-backup-drill.service").read_text()
        backup_unit = (ROOT / "deploy/systemd/duris-backup-backup.service").read_text()
        health_unit = (ROOT / "deploy/systemd/duris-backup-health.service").read_text()
        cycle = (ROOT / "scripts/cycle_mud.sh").read_text()
        self.assertIn("PrivateTmp=true", drill)
        self.assertIn("--tmpdir=", (ROOT / "scripts/persistence_restore.py").read_text())
        self.assertIn("Conflicts=duris-backup-health.service duris-backup-drill.service", backup_unit)
        self.assertIn("Conflicts=duris-backup-backup.service duris-backup-drill.service", health_unit)
        self.assertIn("Conflicts=duris-backup-backup.service duris-backup-health.service", drill)
        self.assertIn("job_overlap_or_authority_busy", cycle)
        self.assertIn("retry", cycle.lower())

    def test_database_qualification_keeps_generation_tombstone_invariant(self):
        qualifier = (ROOT / "scripts/qualify_database_restore.py").read_text()
        self.assertIn("SELECT COUNT(*) FROM account_erasure_tombstones", qualifier)
        self.assertIn("tombstone_preflight", qualifier)


if __name__ == "__main__":
    unittest.main()
