#!/usr/bin/env python3
"""Linux filesystem/unit regressions; all authority is disposable synthetic data.

No database, Redis, live game, or external transport. Capture stubs below model
MariaDB dump bytes only; the separate integration suite exercises real MariaDB.
Run: python3 tests/async/test_persistence_backup.py
"""
import contextlib
from dataclasses import replace
import gzip
import io
import json
import os
from pathlib import Path
import stat
import sys
import tempfile
import time
import unittest
from unittest import mock

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))
import persistence_backup as backup
import persistence_restore as restore
import migration_runner as migrations
import qualify_database_restore as database_restore


def policy(base):
    return dict(version=1, approved=True, custodian="synthetic-test", schedule_seconds=3600,
                rpo_seconds=7200, hourly=48, daily=14, weekly=8, max_bytes=20 * 1024**3,
                min_free_bytes=0, drill_seconds=604800, root=base / "backups",
                restore_root=base / "restore", live_roots=[base / "live"], replica_root=None,
                journal_roots={"players": base / "journals/players",
                               "critical": base / "journals/critical"})


def provision(root):
    for directory in ("identities/names", "identities/accounts", "players", "domains"):
        (root / directory).mkdir(parents=True, mode=0o700, exist_ok=True)
    journal_root = root.parent / "journals"
    for directory in ("players", "critical"):
        (journal_root / directory).mkdir(parents=True, mode=0o700, exist_ok=True)
    for relative in ("identities/names/catalog.identity", "identities/accounts/synthetic.acct",
                     "players/42", "domains/player_42", "domains/locker_catalog"):
        (root / relative).write_bytes(("synthetic:" + relative).encode())
    for path in [root, *root.rglob("*"), journal_root, *journal_root.rglob("*")]:
        path.chmod(0o700 if path.is_dir() else 0o600)


def fake_database_capture(stage, unused, capacity_base=None):
    tables = json.loads((stage / "runtime-schema.json").read_text())
    with gzip.open(stage / "database.sql.gz", "wb") as stream:
        for table in tables["runtime_table_sql_list"].replace("'", "").split(","):
            stream.write(f"CREATE TABLE `{table}` (synthetic INT);\n".encode())
    return {"database": "synthetic"}


class Fixture(unittest.TestCase):
    def setUp(self):
        self.old_umask = os.umask(0o077)
        self.addCleanup(os.umask, self.old_umask)
        self.temp = tempfile.TemporaryDirectory(prefix="duris-backup-unit-")
        self.addCleanup(self.temp.cleanup)
        self.base = Path(self.temp.name)
        self.p = policy(self.base)
        provision(self.base / "live")
        self.env = mock.patch.dict(os.environ, {
            "FLATFILE_STATE_DIR": str(self.base / "live"),
            "PLAYER_SAVE_JOURNAL_DIR": str(self.base / "journals/players"),
            "CRITICAL_COMMAND_JOURNAL_DIR": str(self.base / "journals/critical"),
        })
        self.env.start()
        self.addCleanup(self.env.stop)
        self.capture = mock.patch.object(backup, "mariadb_capture", fake_database_capture)
        self.capture.start()
        self.addCleanup(self.capture.stop)
        self.schema_check = mock.patch.object(backup, "verify_database_schema")
        self.schema_check.start()
        self.addCleanup(self.schema_check.stop)

    def create(self, mode="flatfile-primary", created=None):
        with contextlib.ExitStack() as stack:
            if created is not None:
                stack.enter_context(mock.patch.object(backup.time, "time", return_value=created))
            result = backup.backup(self.p, mode)
        return self.p["root"] / result["generation"]

    def baseline(self, mode):
        now = int(time.time())
        # Deliberately outside every retention bucket; only the newest two are
        # protected, so a subsequent successful generation can prune one.
        paths = [self.create(mode, now - 90 * 86400 + offset) for offset in (0, 1)]
        return {path: backup.inventory(path) for path in paths}

    def assert_preserved(self, before):
        for path, contents in before.items():
            self.assertEqual(backup.inventory(path), contents)
            backup.verify(path)

    def scheduled(self):
        stdout, stderr = io.StringIO(), io.StringIO()
        with mock.patch.object(backup, "policy_load", return_value=self.p), \
             mock.patch.object(sys, "argv", ["backup", "--policy", "/synthetic/policy", "schedule"]), \
             contextlib.redirect_stdout(stdout), contextlib.redirect_stderr(stderr):
            result = backup.main()
        return result, stdout.getvalue(), stderr.getvalue()

    def finalized(self):
        stdout, stderr = io.StringIO(), io.StringIO()
        with mock.patch.object(backup, "policy_load", return_value=self.p), \
             mock.patch.object(sys, "argv", ["backup", "--policy", "/synthetic/policy", "finalize"]), \
             contextlib.redirect_stdout(stdout), contextlib.redirect_stderr(stderr):
            result = backup.main()
        return result, stdout.getvalue(), stderr.getvalue()

    def ledger(self, **changes):
        value = dict(version=1, captured_at=int(time.time()),
                     policy_sha256=backup.digest(ROOT / "migrations/data_lifecycle_manifest.json"),
                     tombstones=[])
        value.update(changes)
        path = self.base / "independent-tombstones.json"
        backup.write_json(path, value)
        return path


class PolicyTests(Fixture):
    def load(self, **changes):
        value = dict(self.p)
        value.update(changes)
        path = self.base / "policy.json"
        path.write_text(json.dumps(value, default=str))
        path.chmod(0o600)
        return backup.policy_load(path)

    def test_approved_defaults(self):
        loaded = self.load()
        expected = dict(self.p,
                        live_roots=self.p["live_roots"] + list(self.p["journal_roots"].values()),
                        resume_published=False, blocked_retry_seconds=3600)
        self.assertEqual(loaded, expected)

    def test_restore_database_engine_is_explicit_and_bounded(self):
        self.assertEqual(self.load().get("restore_database_engine", "mariadb"), "mariadb")
        for engine in ("mariadb", "mysql"):
            self.assertEqual(self.load(restore_database_engine=engine)["restore_database_engine"], engine)
        for engine in ("", "mysql8", "postgres", True, None, 1, []):
            with self.subTest(engine=engine), self.assertRaisesRegex(
                    backup.BackupError, "invalid_restore_database_engine"):
                self.load(restore_database_engine=engine)

    def test_recovery_policy_is_opt_in_and_retry_window_is_bounded(self):
        self.assertFalse(self.load()["resume_published"])
        self.assertTrue(self.load(resume_published=True)["resume_published"])
        for seconds in (59, 604801, True):
            with self.subTest(seconds=seconds), self.assertRaises(backup.BackupError):
                self.load(blocked_retry_seconds=seconds)

    def test_invalid_policy_values(self):
        for changes in ({"approved": False}, {"custodian": "SET_BY_OPERATOR"},
                        {"hourly": 1}, {"hourly": 8761}, {"daily": 3651}, {"weekly": 521},
                        {"max_bytes": 0}, {"max_bytes": True}, {"min_free_bytes": -1},
                        {"schedule_seconds": 59}, {"rpo_seconds": 3599},
                        {"rpo_seconds": 604801}, {"drill_seconds": 3599},
                        {"drill_seconds": 2678401}, {"live_roots": []}, {"unknown": 1}):
            with self.subTest(changes=changes), self.assertRaises(backup.BackupError):
                self.load(**changes)

    def test_overlap_in_both_directions_and_replica(self):
        for changes in ({"root": self.base / "live"}, {"root": self.base},
                        {"root": self.base / "live/child"},
                        {"restore_root": self.p["root"] / "child"},
                        {"replica_root": self.p["root"]},
                        {"replica_root": self.base / "live/replica"}):
            with self.subTest(changes=changes), self.assertRaises(backup.BackupError):
                self.load(**changes)

    def test_unsafe_paths_permissions_links_and_owner(self):
        for path in (Path("relative"), self.base / ".." / "escape"):
            with self.assertRaisesRegex(backup.BackupError, "unsafe_path"):
                backup.secure_path(path)
        file = self.base / "file"
        file.write_bytes(b"fixture")
        file.chmod(0o644)
        with self.assertRaisesRegex(backup.BackupError, "require_owner_only"):
            backup.secure_path(file, False)
        file.chmod(0o600)
        link = self.base / "link"
        link.symlink_to(file)
        with self.assertRaisesRegex(backup.BackupError, "symlink_rejected"):
            backup.secure_path(link)
        hardlink = self.base / "hardlink"
        os.link(file, hardlink)
        with self.assertRaisesRegex(backup.BackupError, "unexpected_file_type"):
            backup.secure_path(file, False)
        hardlink.unlink()
        real_lstat = Path.lstat
        def changed_owner(path):
            info = real_lstat(path)
            if path == file:
                fields = list(info)
                fields[stat.ST_UID] = os.getuid() + 10001
                return os.stat_result(fields)
            return info
        with mock.patch.object(Path, "lstat", changed_owner):
            with self.assertRaisesRegex(backup.BackupError, "unexpected_owner"):
                backup.secure_path(file, False)

    def test_retention_bucket_edges_and_always_two(self):
        now = 200 * 604800 + 12 * 3600
        offsets = [0, 1, 3599, 3600, 7199, 7200, 86400, 172800, 604800, 1209600]
        items = [(Path(str(index)), {"created": now - offset}) for index, offset in enumerate(offsets)]
        p = dict(self.p, hourly=2, daily=2, weekly=2)
        self.assertEqual(backup.retained(items, p, now), {"0", "1", "6"})
        self.assertEqual(backup.retained(items, dict(p, hourly=0, daily=0, weekly=0), now), {"0", "1"})


    def test_each_retention_tier_includes_previous_bucket_and_excludes_boundary(self):
        now = 200 * 604800 + 3 * 86400 + 12 * 3600 + 1800
        for tier, seconds in (("hourly", 3600), ("daily", 86400), ("weekly", 604800)):
            with self.subTest(tier=tier):
                p = dict(self.p, hourly=0, daily=0, weekly=0)
                p[tier] = 2
                items = [(Path("latest"), {"created": now}),
                         (Path("second"), {"created": now - 1}),
                         (Path("previous"), {"created": now - seconds}),
                         (Path("outside"), {"created": now - 2 * seconds})]
                self.assertEqual(backup.retained(items, p, now), {"latest", "second", "previous"})

class GenerationTests(Fixture):
    def test_explicit_deployed_schema_survives_checkout_schema_change(self):
        schema = json.loads((ROOT / "migrations/runtime_compatibility_manifest.json").read_text())
        schema["runtime_table_sql_list"] = "'accounts','player_data','ships'"
        selected = self.base / "deployed-schema.json"
        backup.write_json(selected, schema)
        with mock.patch.dict(os.environ, RUNTIME_COMPATIBILITY_MANIFEST=str(selected)):
            generation = self.create("mariadb-primary")
        selected.unlink()
        self.assertEqual(json.loads((generation / "runtime-schema.json").read_text()), schema)
        self.assertEqual(backup.verify(generation)["mode"], "mariadb-primary")

    def test_schema_mismatch_before_or_after_capture_preserves_generations(self):
        before = self.baseline("mariadb-primary")
        for checks in ([backup.BackupError("schema_mismatch")],
                       [None, backup.BackupError("schema_mismatch")]):
            with self.subTest(checks=len(checks)), \
                 mock.patch.object(backup, "verify_database_schema", side_effect=checks), \
                 self.assertRaisesRegex(backup.BackupError, "schema_mismatch"):
                self.create("mariadb-primary")
            self.assert_preserved(before)
            self.assertEqual(set(self.p["root"].glob("[0-9]*")), set(before))
            self.assertFalse(list(self.p["root"].glob(".staging-*")))

    def test_fallback_mode_captures_database_authority(self):
        generation = self.create("mariadb-primary-flatfile-fallback")
        self.assertEqual(backup.verify(generation)["mode"], "mariadb-primary")
        self.assertTrue((generation / "database.sql.gz").is_file())
        self.assertFalse((generation / "state").exists())

    def test_status_capture_age_is_nonnegative_and_within_rpo(self):
        now = int(time.time())
        for mode in sorted(backup.MODES):
            self.p["root"] = self.base / ("age-" + mode)
            generation = self.create(mode, now)
            backup.write_json(self.p["root"] / "drill.json",
                              dict(result="qualified", completed=now))
            before = backup.inventory(generation)
            receipt = backup.digest(self.p["root"] / "status.json")
            for age in (-300, -1, 0, 1, self.p["rpo_seconds"], self.p["rpo_seconds"] + 1):
                for require_drill in (False, True):
                    with self.subTest(mode=mode, age=age, require_drill=require_drill), \
                         mock.patch.object(backup.time, "time", return_value=now + age):
                        self.assertEqual(backup.verify(generation)["created"], now)
                        valid = 0 <= age <= self.p["rpo_seconds"]
                        if valid:
                            result = backup.status(self.p, require_drill)
                            self.assertEqual(result["result"], "ok")
                            self.assertEqual(result["age_seconds"], age)
                        else:
                            with self.assertRaisesRegex(backup.BackupError, "^rpo_exceeded$"):
                                backup.status(self.p, require_drill)
                        stdout, stderr = io.StringIO(), io.StringIO()
                        command = ["backup", "--policy", "/synthetic/policy", "status"]
                        if require_drill:
                            command.append("--require-drill")
                        with mock.patch.object(backup, "policy_load", return_value=self.p), \
                             mock.patch.object(sys, "argv", command), \
                             contextlib.redirect_stdout(stdout), contextlib.redirect_stderr(stderr):
                            self.assertEqual(backup.main(), 0 if valid else 1)
                        if valid:
                            self.assertEqual(json.loads(stdout.getvalue())["age_seconds"], age)
                            self.assertEqual(stderr.getvalue(), "")
                        else:
                            self.assertEqual(stdout.getvalue(), "")
                            self.assertEqual(json.loads(stderr.getvalue()),
                                             dict(event="status", result="failed", code="rpo_exceeded"))
                        self.assertEqual(backup.inventory(generation), before)
                        self.assertEqual(backup.digest(self.p["root"] / "status.json"), receipt)

    def test_rpo_measures_from_capture_start(self):
        started = int(time.time())
        clock = [started]
        def delayed_capture(stage, p, capacity_base=None):
            clock[0] += 300
            return fake_database_capture(stage, p, capacity_base)
        with mock.patch.object(backup.time, "time", side_effect=lambda: clock[0]), \
             mock.patch.object(backup, "mariadb_capture", delayed_capture):
            self.create("mariadb-primary")
            self.assertEqual(backup.status(self.p)["age_seconds"], 300)
            clock[0] = started + self.p["rpo_seconds"] + 1
            with self.assertRaisesRegex(backup.BackupError, "rpo_exceeded"):
                backup.status(self.p)

    def test_required_drill_receipt_is_current_and_qualified(self):
        self.create()
        with self.assertRaisesRegex(backup.BackupError, "restore_drill_missing_or_overdue"):
            backup.status(self.p, require_drill=True)
        receipt = self.p["root"] / "drill.json"
        for result, age in (("failed", 0), ("qualified", self.p["drill_seconds"] + 1)):
            backup.write_json(receipt, {"result": result, "completed": int(time.time()) - age})
            with self.assertRaisesRegex(backup.BackupError, "restore_drill_missing_or_overdue"):
                backup.status(self.p, require_drill=True)
        backup.write_json(receipt, {"result": "qualified", "completed": int(time.time())})
        self.assertEqual(backup.status(self.p, require_drill=True)["result"], "ok")


    def test_required_drill_receipt_rejects_malformed_completion(self):
        now = int(time.time())
        with mock.patch.object(backup.time, "time", return_value=now):
            generation = self.create()
            before = backup.inventory(generation)
            receipt = self.p["root"] / "drill.json"
            malformed = [None, False, 1, "private-drill-alias", [], {},
                         {"result": "qualified"}]
            malformed += [dict(result="qualified", completed=value) for value in
                          (None, True, False, float(now), "1", [], {}, -1, now + 1,
                           float("nan"), float("inf"))]
            for value in malformed:
                with self.subTest(receipt=value):
                    backup.write_json(receipt, value)
                    receipt_hash = backup.digest(receipt)
                    with self.assertRaisesRegex(backup.BackupError,
                                                "^restore_drill_missing_or_overdue$"):
                        backup.status(self.p, require_drill=True)
                    self.assertIsNone(backup.status(self.p)["drill_age_seconds"])
                    self.assertEqual(backup.digest(receipt), receipt_hash)
                    self.assertEqual(backup.inventory(generation), before)
            for age in (0, self.p["drill_seconds"]):
                backup.write_json(receipt, dict(result="qualified", completed=now - age))
                self.assertEqual(backup.status(self.p, require_drill=True)["drill_age_seconds"], age)

    def test_manifest_requires_an_exact_version_one_object(self):
        for mode in sorted(backup.MODES):
            generation = self.create(mode)
            manifest = generation / "manifest.json"
            valid = backup.read_json(manifest)
            self.assertEqual(backup.verify(generation), valid)
            malformed = [None, False, 1, 1.0, "private-manifest-alias", [], sorted(valid)]
            malformed += [dict(valid, version=value)
                          for value in (True, False, 1.0, "1", None, 0, 2, [], {})]
            malformed += [{key: value for key, value in valid.items() if key != "version"}]
            for value in malformed:
                with self.subTest(mode=mode, manifest=value):
                    backup.write_json(manifest, value)
                    before = backup.inventory(generation)
                    with self.assertRaisesRegex(backup.BackupError, "^invalid_generation_manifest$"):
                        backup.verify(generation)
                    self.assertEqual(backup.inventory(generation), before)
            backup.write_json(manifest, valid)
            self.assertEqual(backup.verify(generation), valid)

    def test_full_manifest_and_checksum_tamper_both_modes(self):
        for mode in sorted(backup.MODES):
            with self.subTest(mode=mode):
                path = self.create(mode)
                meta = backup.verify(path)
                self.assertEqual(meta["mode"], mode)
                self.assertIn("runtime-schema.json", meta["files"])
                data = path / ("state/players/42" if mode == "flatfile-primary" else "database.sql.gz")
                data.write_bytes(data.read_bytes() + b"corruption")
                with self.assertRaisesRegex(backup.BackupError, "checksum"):
                    backup.verify(path)
                # Restore the fixture for the next mode; no invalid generation
                # should be silently bypassed by backup discovery.
                data.write_bytes(data.read_bytes()[:-len(b"corruption")])

    def test_failures_preserve_live_and_last_two_generations(self):
        for mode in sorted(backup.MODES):
            for failed in ("before_capture", "after_capture", "before_sync", "before_publish",
                           "after_publish", "after_verify", "before_rotation", "before_prune"):
                with self.subTest(mode=mode, stage=failed):
                    self.p["root"] = self.base / (mode + "-" + failed)
                    before = self.baseline(mode)
                    live = backup.inventory(self.base / "live", exclude_locks=True)
                    def interrupt(stage):
                        if stage == failed:
                            raise OSError("synthetic injected failure")
                    with mock.patch.object(backup, "checkpoint", interrupt), self.assertRaises(OSError):
                        self.create(mode)
                    self.assert_preserved(before)
                    self.assertEqual(backup.inventory(self.base / "live", exclude_locks=True), live)
                    self.assertFalse(list(self.p["root"].glob(".staging-*")))

    def test_fsync_verify_and_publication_failures_preserve_previous(self):
        for mode in sorted(backup.MODES):
            for failure in ("sync_tree", "verify", "rename"):
                with self.subTest(mode=mode, operation=failure):
                    self.p["root"] = self.base / (mode + "-" + failure)
                    before = self.baseline(mode)
                    if failure == "verify":
                        original = backup.verify
                        def verify(path):
                            if path not in before:
                                raise backup.BackupError("synthetic_verify_failed")
                            return original(path)
                        patch = mock.patch.object(backup, "verify", verify)
                    else:
                        patch = mock.patch.object(backup if failure == "sync_tree" else backup.os,
                                                  failure, side_effect=OSError("synthetic failure"))
                    with patch, self.assertRaises((OSError, backup.BackupError)):
                        self.create(mode)
                    self.assert_preserved(before)

    def test_overlap_fails_promptly(self):
        backup.mkdir(self.p["root"])
        with backup.lock(self.p["root"] / ".job.lock"):
            with mock.patch.object(backup, "LOCK_WAIT_SECONDS", 0):
                with self.assertRaisesRegex(backup.BackupError, "job_overlap"):
                    self.create()
        self.assertFalse(backup.generations(self.p["root"]))

    def test_hard_capacity_does_not_publish_or_remove_prior(self):
        for mode in sorted(backup.MODES):
            with self.subTest(mode=mode):
                self.p = policy(self.base)
                self.p["root"] = self.base / (mode + "-budget")
                before = self.baseline(mode)
                size = sum(backup.total_size(path) for path in before)
                self.p["max_bytes"] = size + 64
                with self.assertRaises(backup.BackupError):
                    self.create(mode)
                self.assert_preserved(before)
                self.assertEqual(set(path for path, _ in backup.generations(self.p["root"])), set(before))

    def test_generation_and_free_space_limits(self):
        self.p["max_bytes"] = 1
        with self.assertRaisesRegex(backup.BackupError, "capacity"):
            self.create()
        self.assertFalse(backup.generations(self.p["root"]))
        self.p["max_bytes"] = 20 * 1024**3
        self.p["min_free_bytes"] = 1
        with mock.patch.object(backup.shutil, "disk_usage", return_value=mock.Mock(free=0)):
            with self.assertRaisesRegex(backup.BackupError, "low_free_capacity"):
                self.create()

    def test_prune_interruption_preserves_newest_and_trash_for_inspection(self):
        before = self.baseline("flatfile-primary")
        def interrupt(stage):
            if stage == "prune_renamed":
                raise OSError("synthetic interruption")
        with mock.patch.object(backup, "checkpoint", interrupt), self.assertRaises(OSError):
            self.create()
        self.assertEqual(len(list(self.p["root"].glob(".trash-*"))), 1)
        self.assertEqual(len(backup.generations(self.p["root"])), 2)
        self.assertTrue(list(before)[1].exists())
        with self.assertRaises(backup.BackupError):
            backup.status(self.p)
        with self.assertRaises(backup.BackupError):
            self.create()

    def test_current_completion_receipt_is_valid_before_health_or_capture(self):
        now = int(time.time())
        with mock.patch.object(backup.time, "time", return_value=now):
            for mode in sorted(backup.MODES):
                self.p["root"] = self.base / ("completion-" + mode)
                generation = self.create(mode)
                meta = backup.verify(generation)
                path = self.p["root"] / "status.json"
                valid = backup.read_json(path)
                before = backup.inventory(generation)
                malformed = [dict(valid, version=value) for value in
                             (None, True, False, 1.0, "1", 0, 2, [], {})]
                malformed += [{key: value for key, value in valid.items() if key != "version"}]
                malformed += [dict(valid, completed=value) for value in
                              (None, True, False, float(now), "1", [], {}, -1,
                               meta["created"] - 1, now + 1, float("nan"), float("inf"))]
                malformed += [{key: value for key, value in valid.items() if key != "completed"}]
                malformed += [dict(valid, replica=value) for value in
                              (None, True, [], {}, "pending", "private-replica-alias")]
                malformed += [{key: value for key, value in valid.items() if key != "replica"},
                              dict(valid, result="replication_pending")]
                items = [(Path("newest"), {"created": now}), (generation, meta)]
                for value in malformed:
                    for consumer in ("status", "backup", "previous"):
                        with self.subTest(mode=mode, receipt=value, consumer=consumer):
                            backup.write_json(path, value)
                            receipt_hash = backup.digest(path)
                            with mock.patch.object(backup, "remember_blocked"), \
                                 mock.patch.object(backup, "checkpoint", side_effect=
                                      backup.BackupError("synthetic_capture_started")) as capture, \
                                 mock.patch.object(backup, "complete_generation", side_effect=
                                      backup.BackupError("synthetic_completion_started")) as complete:
                                if consumer == "status":
                                    with self.assertRaisesRegex(backup.BackupError,
                                                                "^backup_or_rotation_incomplete$"):
                                        backup.status(self.p)
                                elif consumer == "backup":
                                    with self.assertRaisesRegex(backup.BackupError,
                                                                "^prior_backup_requires_finalization$"):
                                        backup.backup(self.p, mode)
                                else:
                                    self.assertFalse(backup.previous_completion_matches(items, value, self.p))
                                capture.assert_not_called()
                                complete.assert_not_called()
                            self.assertEqual(backup.digest(path), receipt_hash)
                            self.assertEqual(backup.inventory(generation), before)
                            self.assertFalse(list(self.p["root"].glob(".staging-*")))
                backup.write_json(path, valid)
                self.assertEqual(backup.status(self.p)["result"], "ok")
                self.assertTrue(backup.previous_completion_matches(items, valid, self.p))
                self.assertEqual(backup.inventory(generation), before)

    def test_stale_or_incomplete_status(self):
        self.create()
        self.assertEqual(backup.status(self.p)["result"], "ok")
        with mock.patch.object(backup.time, "time", return_value=time.time() + 7201):
            with self.assertRaisesRegex(backup.BackupError, "rpo_exceeded"):
                backup.status(self.p)
        backup.write_json(self.p["root"] / "status.json", {"generation": "missing", "result": "ok"})
        with self.assertRaisesRegex(backup.BackupError, "incomplete"):
            backup.status(self.p)


    def test_actual_file_fsync_failure_preserves_previous_generations(self):
        before = self.baseline("flatfile-primary")
        real_fsync = backup.os.fsync
        def failed_sync(fd):
            if ".staging-" in os.readlink(f"/proc/self/fd/{fd}"):
                raise OSError("synthetic durable file sync failure")
            return real_fsync(fd)
        with mock.patch.object(backup.os, "fsync", failed_sync), self.assertRaises(OSError):
            self.create()
        self.assert_preserved(before)
        self.assertFalse(list(self.p["root"].glob(".staging-*")))

    def test_finalize_after_publication_failure_is_idempotent(self):
        self.baseline("flatfile-primary")
        def interrupt(stage):
            if stage == "after_publish":
                raise OSError("synthetic publication interruption")
        with mock.patch.object(backup, "checkpoint", interrupt), self.assertRaises(OSError):
            self.create()
        newest = backup.generations(self.p["root"])[0][0]
        contents = backup.inventory(newest)
        with self.assertRaises(backup.BackupError):
            self.create()
        with mock.patch.object(backup, "policy_load", return_value=self.p), \
             mock.patch.object(sys, "argv", ["backup", "--policy", "/synthetic/policy", "finalize"]), \
             contextlib.redirect_stdout(io.StringIO()):
            self.assertEqual(backup.main(), 0)
            self.assertEqual(backup.main(), 0)
        self.assertEqual(backup.status(self.p)["result"], "ok")
        self.assertEqual(backup.inventory(newest), contents)
        self.assertEqual(len(backup.generations(self.p["root"])), 2)

    def test_schedule_requires_nonfuture_integer_completion(self):
        now = int(time.time())
        with mock.patch.object(backup.time, "time", return_value=now):
            generation = self.create()
            before = backup.inventory(generation)
            receipt = backup.digest(self.p["root"] / "status.json")
            path = self.p["root"] / "schedule.json"
            malformed = [None, False, 1, 1.0, "private-schedule-alias", [], {},
                         {"unexpected": now}]
            malformed += [dict(completed=value) for value in
                          (None, True, False, float(now), "1", [], {}, -1, now + 1,
                           float("nan"), float("inf"))]
            for value in malformed:
                with self.subTest(receipt=value):
                    backup.write_json(path, value)
                    schedule_hash = backup.digest(path)
                    with mock.patch.object(backup, "backup", return_value=dict(result="ok")) as capture, \
                         mock.patch.object(backup, "status", return_value=dict(result="ok")) as health:
                        result, stdout, stderr = self.scheduled()
                        self.assertEqual(result, 1)
                        self.assertEqual(stdout, "")
                        self.assertEqual(json.loads(stderr),
                                         dict(event="schedule", result="failed", code="invalid_schedule_receipt"))
                        capture.assert_not_called()
                        health.assert_not_called()
                    self.assertEqual(backup.digest(path), schedule_hash)
                    self.assertEqual(backup.digest(self.p["root"] / "status.json"), receipt)
                    self.assertEqual(backup.inventory(generation), before)

    def test_schedule_preserves_due_boundary_and_failure_receipt(self):
        now = int(time.time())
        for mode in sorted(backup.MODES):
            with mock.patch.dict(os.environ, {"PERSISTENCE_MODE": mode}), \
                 mock.patch.object(backup.time, "time", return_value=now):
                for index, (completed, due) in enumerate(((None, True), (0, True),
                        (now - self.p["schedule_seconds"], True),
                        (now - self.p["schedule_seconds"] + 1, False), (now, False))):
                    with self.subTest(mode=mode, completed=completed):
                        self.p["root"] = self.base / ("cadence-" + mode + "-" + str(index))
                        generation = self.create(mode)
                        before = backup.inventory(generation)
                        path = self.p["root"] / "schedule.json"
                        if completed is not None:
                            backup.write_json(path, dict(completed=completed))
                        result, stdout, stderr = self.scheduled()
                        self.assertEqual((result, stderr), (0, ""))
                        self.assertEqual(json.loads(stdout)["result"], "ok")
                        self.assertEqual(len(backup.generations(self.p["root"])), 2 if due else 1)
                        self.assertEqual(backup.read_json(path)["completed"], now if due else completed)
                        self.assertEqual(backup.inventory(generation), before)
                self.p["root"] = self.base / ("failed-cadence-" + mode)
                generation = self.create(mode)
                before = backup.inventory(generation)
                path = self.p["root"] / "schedule.json"
                backup.write_json(path, dict(completed=now - self.p["schedule_seconds"]))
                schedule_hash = backup.digest(path)
                with mock.patch.object(backup, "backup", side_effect=backup.BackupError("synthetic_capture_failure")):
                    result, stdout, stderr = self.scheduled()
                self.assertEqual((result, stdout), (1, ""))
                self.assertEqual(json.loads(stderr),
                                 dict(event="schedule", result="failed", code="synthetic_capture_failure"))
                self.assertEqual(backup.digest(path), schedule_hash)
                self.assertEqual(backup.inventory(generation), before)

    def test_next_scheduled_call_resumes_only_verified_publications_then_captures_again(self):
        for failed_stage in ("after_publish", "after_verify", "before_rotation", "prune_complete",
                             "status_write"):
            with self.subTest(stage=failed_stage):
                self.p["root"] = self.base / ("scheduled-resume-" + failed_stage)
                self.p["resume_published"] = True
                self.p["schedule_seconds"] = 0
                self.baseline("flatfile-primary")
                def interrupt(stage):
                    if stage == failed_stage:
                        raise OSError("synthetic interruption")
                if failed_stage == "status_write":
                    original = backup.write_json
                    def fail_status(path, value):
                        if path.name == "status.json":
                            raise OSError("synthetic status publication failure")
                        return original(path, value)
                    failure = mock.patch.object(backup, "write_json", fail_status)
                else:
                    failure = mock.patch.object(backup, "checkpoint", interrupt)
                with failure:
                    result, unused_out, unused_error = self.scheduled()
                self.assertEqual(result, 1)
                published = backup.generations(self.p["root"])[0][0]
                result, unused_out, error = self.scheduled()
                self.assertEqual((result, error), (0, ""))
                self.assertEqual(backup.status(self.p)["result"], "ok")
                self.assertEqual(backup.generations(self.p["root"])[0][0], published)
                result, unused_out, error = self.scheduled()
                self.assertEqual((result, error), (0, ""))
                self.assertNotEqual(backup.generations(self.p["root"])[0][0], published)
                self.assertEqual(backup.status(self.p)["result"], "ok")

    def test_scheduled_replication_pending_retries_then_allows_fresh_capture(self):
        self.p["root"] = self.base / "scheduled-replication"
        self.p["replica_root"] = self.base / "replica"
        self.p["replica_root"].mkdir(mode=0o700)
        self.p["schedule_seconds"] = 0
        with mock.patch.object(backup, "replicate", return_value="transport_and_readback_verified"):
            self.baseline("flatfile-primary")
        self.p["resume_published"] = True
        def interrupt(stage):
            if stage == "after_publish":
                raise OSError("synthetic publication interruption")
        with mock.patch.object(backup, "checkpoint", interrupt):
            result, unused_out, unused_error = self.scheduled()
        self.assertEqual(result, 1)
        def failed_replication(*unused):
            raise backup.BackupError("replica_mount_missing")
        with mock.patch.object(backup, "replicate", side_effect=failed_replication):
            result, unused_out, unused_error = self.scheduled()
        self.assertEqual(result, 1)
        self.assertEqual(backup.read_json(self.p["root"] / "status.json")["result"],
                         "replication_pending")
        with self.assertRaisesRegex(backup.BackupError, "replica_not_verified"):
            backup.status(self.p)
        with mock.patch.object(backup, "replicate", return_value="transport_and_readback_verified"):
            result, unused_out, error = self.scheduled()
            self.assertEqual((result, error), (0, ""))
            self.assertEqual(backup.status(self.p)["result"], "ok")
            newest = backup.generations(self.p["root"])[0][0]
            result, unused_out, error = self.scheduled()
        self.assertEqual((result, error), (0, ""))
        self.assertNotEqual(backup.generations(self.p["root"])[0][0], newest)

    def test_blocked_unfinalized_state_is_backed_off_without_trusting_later_corruption(self):
        self.p["root"] = self.base / "scheduled-blocked"
        self.p["schedule_seconds"] = 0
        self.baseline("flatfile-primary")
        def interrupt(stage):
            if stage == "after_publish":
                raise OSError("synthetic publication interruption")
        with mock.patch.object(backup, "checkpoint", interrupt):
            self.assertEqual(self.scheduled()[0], 1)
        result, unused_out, error = self.scheduled()
        self.assertEqual(result, 1)
        self.assertIn("published_generation_requires_finalize_command", error)
        with self.assertRaisesRegex(backup.BackupError,
                                   "published_generation_requires_finalize_command"):
            backup.status(self.p)
        with mock.patch.object(backup, "verify", wraps=backup.verify) as verify:
            result, unused_out, error = self.scheduled()
            self.assertEqual(result, 1)
            self.assertIn("published_generation_requires_finalize_command", error)
            verify.assert_not_called()
        newest = backup.generations(self.p["root"])[0][0]
        data = newest / "database.sql.gz"
        data.write_bytes(data.read_bytes() + b"changed after backoff")
        with mock.patch.object(backup, "verify", wraps=backup.verify) as verify:
            result, unused_out, error = self.scheduled()
            self.assertEqual(result, 1)
            self.assertIn("generation_checksum_mismatch", error)
            verify.assert_called()

    def test_corrupt_published_generation_is_never_auto_resumed(self):
        self.p["root"] = self.base / "scheduled-corrupt"
        self.p["resume_published"] = True
        self.p["schedule_seconds"] = 0
        self.baseline("flatfile-primary")
        def interrupt(stage):
            if stage == "after_publish":
                raise OSError("synthetic publication interruption")
        with mock.patch.object(backup, "checkpoint", interrupt):
            self.assertEqual(self.scheduled()[0], 1)
        items = backup.generations(self.p["root"])
        old_status = backup.read_json(self.p["root"] / "status.json")
        newest = items[0][0]
        data = newest / "database.sql.gz"
        data.write_bytes(data.read_bytes() + b"corrupt before continuation")
        result, unused_out, error = self.scheduled()
        self.assertEqual(result, 1)
        self.assertIn("generation_checksum_mismatch", error)
        self.assertEqual(backup.read_json(self.p["root"] / "status.json"), old_status)
        self.assertEqual({path.name for path in self.p["root"].iterdir()
                          if backup.GENERATION.fullmatch(path.name)},
                         {path.name for path, _ in items})

    def test_scheduled_rotation_remnant_is_actionable_and_unchanged_retry_is_bounded(self):
        self.p["root"] = self.base / "scheduled-trash"
        self.p["resume_published"] = True
        self.p["schedule_seconds"] = 0
        self.baseline("flatfile-primary")
        def interrupt(stage):
            if stage == "prune_renamed":
                raise OSError("synthetic rotation interruption")
        with mock.patch.object(backup, "checkpoint", interrupt):
            self.assertEqual(self.scheduled()[0], 1)
        self.assertEqual(len(list(self.p["root"].glob(".trash-*"))), 1)
        trash = next(self.p["root"].glob(".trash-*"))
        trash_before = backup.inventory(trash)
        before = backup.read_json(self.p["root"] / "status.json")
        result, unused_out, error = self.scheduled()
        self.assertEqual(result, 1)
        self.assertIn("interrupted_job_requires_inspection", error)
        with mock.patch.object(backup, "verify", wraps=backup.verify) as verify:
            result, unused_out, error = self.scheduled()
            self.assertEqual(result, 1)
            self.assertIn("interrupted_job_requires_inspection", error)
            verify.assert_not_called()
        self.assertEqual(backup.read_json(self.p["root"] / "status.json"), before)
        self.assertEqual(len(list(self.p["root"].glob(".trash-*"))), 1)
        result, unused_out, error = self.finalized()
        self.assertEqual(result, 1)
        self.assertIn("interrupted_job_requires_inspection", error)
        self.assertEqual(backup.read_json(self.p["root"] / "status.json"), before)
        self.assertEqual(backup.inventory(trash), trash_before)

    def test_auto_resume_preserves_capacity_and_capture_age_gates(self):
        self.p["root"] = self.base / "scheduled-capacity"
        self.p["resume_published"] = True
        self.p["schedule_seconds"] = 0
        self.baseline("flatfile-primary")
        def interrupt(stage):
            if stage == "after_publish":
                raise OSError("synthetic publication interruption")
        with mock.patch.object(backup, "checkpoint", interrupt):
            self.assertEqual(self.scheduled()[0], 1)
        self.p["min_free_bytes"] = 1
        with mock.patch.object(backup.shutil, "disk_usage", return_value=mock.Mock(free=0)):
            result, unused_out, error = self.scheduled()
        self.assertEqual(result, 1)
        self.assertIn("low_free_capacity", error)
        self.p["min_free_bytes"] = 0
        self.p["max_bytes"] = 1
        before = {path: backup.inventory(path) for path, _ in backup.generations(self.p["root"])}
        result, unused_out, error = self.scheduled()
        self.assertEqual(result, 1)
        self.assertIn("retention_exceeds_capacity", error)
        self.assertEqual({path: backup.inventory(path) for path, _ in backup.generations(self.p["root"])}, before)

        self.p = policy(self.base)
        self.p["root"] = self.base / "scheduled-rpo"
        self.p["resume_published"] = True
        self.p["schedule_seconds"] = 0
        self.baseline("flatfile-primary")
        old_capture_time = int(time.time()) - self.p["rpo_seconds"] - 10
        with mock.patch.object(backup.time, "time", return_value=old_capture_time), \
             mock.patch.object(backup, "checkpoint", interrupt):
            with self.assertRaises(OSError):
                self.create()
        result, unused_out, error = self.scheduled()
        self.assertEqual(result, 1)
        self.assertIn("published_generation_rpo_exceeded", error)
        previous = backup.read_json(self.p["root"] / "status.json")
        self.assertEqual(previous["result"], "ok")
        with mock.patch.object(backup, "verify", wraps=backup.verify) as verify:
            result, unused_out, error = self.scheduled()
            self.assertEqual(result, 1)
            self.assertIn("published_generation_rpo_exceeded", error)
            verify.assert_not_called()
        result, unused_out, error = self.finalized()
        self.assertEqual(result, 1)
        self.assertIn("published_generation_rpo_exceeded", error)
        self.assertEqual(backup.read_json(self.p["root"] / "status.json"), previous)

    def test_malformed_completion_replica_is_a_protected_refusal(self):
        self.p["root"] = self.base / "malformed-completion"
        self.p["resume_published"] = True
        self.p["schedule_seconds"] = 0
        self.baseline("flatfile-primary")
        def interrupt(stage):
            if stage == "after_publish":
                raise OSError("synthetic publication interruption")
        with mock.patch.object(backup, "checkpoint", interrupt):
            self.assertEqual(self.scheduled()[0], 1)
        receipt = backup.read_json(self.p["root"] / "status.json")
        receipt["replica"] = []
        backup.write_json(self.p["root"] / "status.json", receipt)
        result, unused_out, error = self.scheduled()
        self.assertEqual(result, 1)
        self.assertIn("published_generation_requires_finalize_command", error)
        with mock.patch.object(backup, "verify", wraps=backup.verify) as verify:
            self.assertEqual(self.scheduled()[0], 1)
            verify.assert_not_called()
        self.assertEqual(backup.read_json(self.p["root"] / "status.json"), receipt)

    def test_scheduled_resume_keeps_job_lock_exclusive(self):
        self.p["root"] = self.base / "scheduled-lock"
        self.p["resume_published"] = True
        self.p["schedule_seconds"] = 0
        self.baseline("flatfile-primary")
        def interrupt(stage):
            if stage == "after_publish":
                raise OSError("synthetic publication interruption")
        with mock.patch.object(backup, "checkpoint", interrupt):
            self.assertEqual(self.scheduled()[0], 1)
        newest = backup.generations(self.p["root"])[0][0]
        with backup.lock(self.p["root"] / ".job.lock"):
            with mock.patch.object(backup, "LOCK_WAIT_SECONDS", 0):
                result, unused_out, error = self.scheduled()
        self.assertEqual(result, 1)
        self.assertIn("job_overlap_or_authority_busy", error)
        self.assertEqual(backup.generations(self.p["root"])[0][0], newest)
        result, unused_out, error = self.scheduled()
        self.assertEqual((result, error), (0, ""))
        self.assertEqual(backup.status(self.p)["result"], "ok")

    def test_manual_finalize_refuses_staging_remnants_without_claiming_success(self):
        self.create()
        before = backup.read_json(self.p["root"] / "status.json")
        orphan = self.p["root"] / ".staging-orphan"
        orphan.mkdir(mode=0o700)
        result, unused_out, error = self.finalized()
        self.assertEqual(result, 1)
        self.assertIn("interrupted_job_requires_inspection", error)
        self.assertEqual(backup.read_json(self.p["root"] / "status.json"), before)
        self.assertTrue(orphan.is_dir())

class CapacityAndInputTests(Fixture):
    def test_metadata_growth_after_size_observation_is_bounded(self):
        path = self.base / "metadata.json"
        path.write_bytes(b"{}")
        path.chmod(0o600)
        original_secure = backup.secure_path
        original_stat, original_fstat = Path.stat, os.fstat
        identity = original_stat(path)
        armed, grown = [], []
        def secure_then_arm(selected, directory=None):
            result = original_secure(selected, directory)
            if selected == path:
                armed.append(True)
            return result
        def grow():
            if armed and not grown:
                with path.open("ab") as writer:
                    writer.write(b" " * (32 * 1024 * 1024 - 1))
                grown.append(True)
        def stat_then_grow(selected, *args, **kwargs):
            result = original_stat(selected, *args, **kwargs)
            if selected == path:
                grow()
            return result
        def fstat_then_grow(fd):
            result = original_fstat(fd)
            if (result.st_dev, result.st_ino) == (identity.st_dev, identity.st_ino):
                grow()
            return result
        with mock.patch.object(backup, "secure_path", secure_then_arm), \
             mock.patch.object(Path, "stat", stat_then_grow), \
             mock.patch.object(os, "fstat", fstat_then_grow):
            with self.assertRaisesRegex(backup.BackupError, "^metadata_too_large$"):
                backup.read_json(path)
        self.assertTrue(grown)
        self.assertEqual(path.read_bytes(), b"{}" + b" " * (32 * 1024 * 1024 - 1))

    def test_metadata_open_rechecks_guards_after_path_validation(self):
        import subprocess
        child = self.base / "metadata-child.py"
        child.write_text("""from pathlib import Path
import json,os,sys
sys.path.insert(0,sys.argv[1]);import persistence_backup as backup
path=Path(sys.argv[2]);case=sys.argv[3];path.write_bytes(b'{}');path.chmod(0o600)
backing=path.with_name('private-backing');backing.write_bytes(b'{}');backing.chmod(0o600)
original=backup.secure_path;changed=[];before=[]
def secure_then_change(selected,directory=None):
    result=original(selected,directory)
    if selected==path and not changed:
        if case=='permissions':path.chmod(0o644)
        elif case=='hardlink':os.link(path,path.with_name('private-hardlink'))
        elif case=='owner':os.chown(path,1,-1)
        else:
            path.unlink()
            if case=='symlink':path.symlink_to(backing)
            elif case=='directory':path.mkdir(mode=0o700)
            elif case=='fifo':os.mkfifo(path,0o600)
        changed.append(True);info=path.lstat()
        before[:]=[(info.st_mode,info.st_ino,info.st_nlink,info.st_size,info.st_uid)]
    return result
backup.secure_path=secure_then_change;code='accepted'
fds=len(list(Path('/proc/self/fd').iterdir()))
try:backup.read_json(path)
except backup.BackupError as error:code=str(error)
except OSError:code='operation_failed'
info=path.lstat()
assert before==[(info.st_mode,info.st_ino,info.st_nlink,info.st_size,info.st_uid)]
assert fds==len(list(Path('/proc/self/fd').iterdir()))
print(json.dumps({'code':code}));sys.exit(0 if code=='accepted' else 2)
""")
        cases = [("permissions", "require_owner_only"), ("hardlink", "unexpected_file_type"),
                 ("symlink", "symlink_rejected"), ("directory", "unexpected_file_type"),
                 ("fifo", "unexpected_file_type")]
        if os.getuid() == 0:
            cases.append(("owner", "unexpected_owner"))
        for case, expected in cases:
            with self.subTest(case=case):
                folder = self.base / case
                folder.mkdir(mode=0o700)
                result = subprocess.run([sys.executable, str(child), str(ROOT / "scripts"),
                                         str(folder / "metadata.json"), case],
                                        capture_output=True, text=True, timeout=2, check=False)
                self.assertEqual(result.returncode, 2, result.stderr)
                self.assertEqual(result.stderr, "")
                self.assertEqual(json.loads(result.stdout), {"code": expected})

    def test_restore_capacity_requires_dedicated_mount(self):
        backup.mkdir(self.p["restore_root"])
        with self.assertRaisesRegex(backup.BackupError, "dedicated_restore_filesystem"):
            restore.restore_capacity(self.p)
        with mock.patch.object(Path, "is_mount", return_value=True):
            with self.assertRaisesRegex(backup.BackupError, "shared_with_authority"):
                restore.restore_capacity(self.p)
        actual_stat = Path.stat
        def separate_device(path, *args, **kwargs):
            info = actual_stat(path, *args, **kwargs)
            if path == self.p["restore_root"]:
                fields = list(info)
                fields[stat.ST_DEV] += 10001
                return os.stat_result(fields)
            return info
        with mock.patch.object(Path, "is_mount", return_value=True), \
             mock.patch.object(Path, "stat", separate_device), \
             mock.patch.object(restore.shutil, "disk_usage", return_value=mock.Mock(total=self.p["max_bytes"] + 1, free=100)):
            with self.assertRaisesRegex(backup.BackupError, "capacity_invalid"):
                restore.restore_capacity(self.p)

    def test_environment_is_literal_and_never_executes_shell(self):
        path = self.base / "literal.env"
        marker = self.base / "must-not-exist"
        path.write_text("export SYNTHETIC_VALUE='$(touch " + str(marker) + ")'\n"
                        "SYNTHETIC_EMPTY=\nSYNTHETIC_QUOTED='two words' # comment\n")
        with mock.patch.dict(os.environ):
            backup.load_environment(path)
            self.assertEqual(os.environ["SYNTHETIC_VALUE"], "$(touch " + str(marker) + ")")
            self.assertEqual(os.environ["SYNTHETIC_EMPTY"], "")
            self.assertEqual(os.environ["SYNTHETIC_QUOTED"], "two words")
        self.assertFalse(marker.exists())
        path.chmod(0o640)
        with self.assertRaises(backup.BackupError):
            backup.load_environment(path)

    def test_stalled_stream_is_killed_and_reaped(self):
        started = time.monotonic()
        with self.assertRaises(backup.BackupError):
            with backup.streaming_process([sys.executable, "-c", "import time; time.sleep(60)"],
                                          env={}, timeout=0.25) as child:
                self.assertEqual(child.stdout.read(), b"")
        self.assertIsNotNone(child.poll())
        self.assertTrue(child.stdout.closed)
        self.assertLess(time.monotonic() - started, 5)

    def test_journals_are_complete_and_churn_blocks_publication(self):
        journal = self.base / "synthetic-player-wal"
        journal.mkdir(mode=0o700)
        (journal / "player-save.journal").write_bytes(b"synthetic-journal-bytes")
        self.p["journal_roots"] = {"players": journal}
        critical = self.base / "critical-journal"
        critical.mkdir(mode=0o700)
        with mock.patch.dict(os.environ, {
            "PLAYER_SAVE_JOURNAL_DIR": str(journal),
            "CRITICAL_COMMAND_JOURNAL_DIR": str(critical),
        }):
            self.p["journal_roots"] = {"players": journal, "critical": critical}
            generation = self.create()
            self.assertEqual(backup.inventory(journal), backup.inventory(generation / "journals/players"))
            captured = backup.inventory(generation)
            original = backup.flatfile_capture
            def changed_capture(stage, p, capacity_base=None):
                result = original(stage, p, capacity_base)
                (journal / "player-save.journal").write_bytes(b"synthetic-new-journal-bytes")
                return result
            with mock.patch.object(backup, "flatfile_capture", changed_capture):
                with self.assertRaisesRegex(backup.BackupError, "journal_changed"):
                    self.create()
            self.assertEqual(backup.inventory(generation), captured)
            self.assertEqual(len(backup.generations(self.p["root"])), 1)

class RestoreTests(Fixture):
    def test_restore_accepts_exact_schema64_and_six_schema65_histories(self):
        selectors = (
            ("migration_manifest.json", "a0920c9e76246161f9e3dae66a020dbb770ccacbcaaf5a684d4f45fe64d2ab7f"),
            ("migration_manifest.staging_0045.json", "fca49b0d040627d255f43f2170f4af69d9cdd3dc74f4e13461780358ba3cf84e"),
            ("migration_manifest.master_0031.json", "d94d73d733186351a5ed2df7890679ebabe344b92e3a4d14a4ed7e8f29ea28e3"),
            ("migration_manifest.nullable_default_0065.json", "55ab85c769d0f271c0c01b2ec55ac01217548d8062be3f837692bf05987b57ba"),
            ("migration_manifest.staging_0045_nullable_default_0065.json", "9e81f924525c9e089a92559d1429e83f7e1c05cead3ca652a89eb68ec5534f97"),
            ("migration_manifest.master_0031_nullable_default_0065.json", "45b830b94d19c1ab6ff017e33125e0426099a3a2508fb2d0a19b573ece5c0e02"),
        )
        histories = set()
        for selector, checksum in selectors:
            manifest = migrations.load_manifest(ROOT / "migrations" / selector)
            rows = [migrations.AppliedMigration(
                step.migration_id, step.sequence, step.description,
                step.apply_checksum, step.verify_checksum, step.compatibility,
                manifest.runner_version) for step in manifest.migrations]
            self.assertEqual(len(rows), 65)
            self.assertEqual(migrations.history_checksum(rows), checksum)
            for count in (64, 65):
                history = rows[:count]
                with self.subTest(selector=selector, count=count):
                    output = "\n".join("\t".join(map(str, (
                        row.migration_id, row.sequence, row.description,
                        row.apply_checksum, row.verify_checksum, row.compatibility,
                        row.runner_version))) for row in history)
                    executor = object.__new__(migrations.MysqlExecutor)
                    executor.sql = mock.Mock(side_effect=[output,
                        f"{count}\t{migrations.history_checksum(history)}"])
                    database_restore.require_completed_history(executor.applied())
                    self.assertEqual(executor.sql.call_count, 2)
                    histories.add(migrations.history_checksum(history))
        self.assertEqual(len(histories), 9)

    def test_restore_history_rejects_partial_mixed_and_all_receipt_field_edits(self):
        selectors = (
            "migration_manifest.json", "migration_manifest.staging_0045.json",
            "migration_manifest.master_0031.json", "migration_manifest.nullable_default_0065.json",
            "migration_manifest.staging_0045_nullable_default_0065.json",
            "migration_manifest.master_0031_nullable_default_0065.json",
        )
        histories = []
        for selector in selectors:
            manifest = migrations.load_manifest(ROOT / "migrations" / selector)
            histories.append([migrations.AppliedMigration(
                step.migration_id, step.sequence, step.description,
                step.apply_checksum, step.verify_checksum, step.compatibility,
                manifest.runner_version) for step in manifest.migrations])
        edits = {"migration_id": "unknown", "sequence": 2, "description": "edited",
                 "apply_checksum": "0" * 64, "verify_checksum": "0" * 64,
                 "compatibility": "unknown", "runner_version": 2}
        for selector, rows in zip(selectors, histories):
            candidates = [("empty", []), ("partial63", rows[:63]),
                          ("overflow66", rows + [rows[-1]]),
                          ("missing_middle", rows[:30] + rows[31:]),
                          ("reordered", [rows[1], rows[0]] + rows[2:])]
            candidates += [(field, [replace(rows[0], **{field: value})] + rows[1:])
                           for field, value in edits.items()]
            candidates += [("edited_terminal", rows[:-1] +
                            [replace(rows[-1], verify_checksum="0" * 64)])]
            for label, history in candidates:
                with self.subTest(selector=selector, mutation=label):
                    # Even an attacker-recomputed count/hash cannot replace exact receipts.
                    migrations.validate_history_state(history, len(history),
                                                       migrations.history_checksum(history))
                    with self.assertRaisesRegex(RuntimeError, "incomplete_or_unknown"):
                        database_restore.require_completed_history(history)
        mixed = list(histories[0])
        mixed[44] = histories[1][44]
        with self.assertRaisesRegex(RuntimeError, "incomplete_or_unknown"):
            database_restore.require_completed_history(mixed)
        mixed = list(histories[0])
        mixed[30] = histories[2][30]
        with self.assertRaisesRegex(RuntimeError, "incomplete_or_unknown"):
            database_restore.require_completed_history(mixed)

    def test_restore_history_requires_exact_recorded_state(self):
        manifest = migrations.load_manifest()
        rows = [migrations.AppliedMigration(
            step.migration_id, step.sequence, step.description,
            step.apply_checksum, step.verify_checksum, step.compatibility,
            manifest.runner_version) for step in manifest.migrations]
        output = "\n".join("\t".join(map(str, (
            row.migration_id, row.sequence, row.description, row.apply_checksum,
            row.verify_checksum, row.compatibility, row.runner_version))) for row in rows)
        for state in ("", "65", "64\t" + migrations.history_checksum(rows),
                      "65\t" + "0" * 64, "65\t" + migrations.history_checksum(rows[:-1])):
            with self.subTest(state=state):
                executor = object.__new__(migrations.MysqlExecutor)
                executor.sql = mock.Mock(side_effect=[output, state])
                with self.assertRaises(migrations.MigrationContractError):
                    database_restore.require_completed_history(executor.applied())

    def setUp(self):
        super().setUp()
        backup.mkdir(self.p["restore_root"])
        guard = mock.patch.object(restore, "restore_capacity")
        guard.start()
        self.addCleanup(guard.stop)
    def test_drill_only_defers_for_current_qualified_integer_receipt(self):
        now = int(time.time())
        with mock.patch.object(backup.time, "time", return_value=now):
            generation = self.create()
            before = backup.inventory(generation)
            ledger = self.ledger()
            ledger_hash = backup.digest(ledger)
            live = backup.inventory(self.base / "live", exclude_locks=True)
            journals = backup.inventory(self.base / "journals", exclude_locks=True)
            path = self.p["root"] / "drill.json"
            invalid = [None, False, 1, "private-drill-alias", [], {},
                       {"result": "qualified"}, {"completed": now}]
            invalid += [dict(result=value, completed=now) for value in
                        (None, "failed", "not_due", True, [], {})]
            invalid += [dict(result="qualified", completed=value) for value in
                        (None, True, False, float(now), "1", [], {}, -1, now + 1,
                         float("nan"), float("inf"), now - self.p["drill_seconds"],
                         now - self.p["drill_seconds"] - 1)]
            for value in invalid:
                with self.subTest(receipt=value):
                    backup.write_json(path, value)
                    receipt_hash = backup.digest(path)
                    with mock.patch.object(backup, "generations", side_effect=
                                           backup.BackupError("synthetic_generation_check_reached")) as generations, \
                         mock.patch.object(restore, "service_load") as service, \
                         mock.patch.object(restore, "private_database") as database, \
                         mock.patch.object(backup, "run") as run:
                        with self.assertRaisesRegex(backup.BackupError,
                                                    "^synthetic_generation_check_reached$"):
                            restore.restore(self.p, None, ledger, drill=True)
                        self.assertEqual(generations.call_count, 1)
                        stdout, stderr = io.StringIO(), io.StringIO()
                        command = ["backup", "--policy", "/synthetic/policy", "drill",
                                   "--tombstones", str(ledger)]
                        with mock.patch.object(backup, "policy_load", return_value=self.p), \
                             mock.patch.object(sys, "argv", command), \
                             contextlib.redirect_stdout(stdout), contextlib.redirect_stderr(stderr):
                            self.assertEqual(backup.main(), 1)
                        self.assertEqual(json.loads(stderr.getvalue()), dict(event="drill",
                                         result="failed", code="synthetic_generation_check_reached"))
                        self.assertEqual(stdout.getvalue(), "")
                        service.assert_not_called()
                        database.assert_not_called()
                        run.assert_not_called()
                    self.assertEqual(backup.digest(path), receipt_hash)
                    self.assertEqual(backup.inventory(generation), before)
                    self.assertEqual(backup.digest(ledger), ledger_hash)
                    self.assertEqual(backup.inventory(self.base / "live", exclude_locks=True), live)
                    self.assertEqual(backup.inventory(self.base / "journals", exclude_locks=True), journals)
                    self.assertFalse(list(self.p["restore_root"].glob("candidate-*")))
            for age in (0, self.p["drill_seconds"] - 1):
                backup.write_json(path, dict(result="qualified", completed=now - age))
                receipt_hash = backup.digest(path)
                with mock.patch.object(backup, "generations") as generations:
                    self.assertEqual(restore.restore(self.p, None, ledger, drill=True),
                                     dict(event="drill", result="not_due"))
                    generations.assert_not_called()
                self.assertEqual(backup.digest(path), receipt_hash)

    def test_bad_generation_manifest_refuses_before_candidate_or_service(self):
        ledger = self.ledger()
        live = backup.inventory(self.base / "live", exclude_locks=True)
        journals = backup.inventory(self.base / "journals", exclude_locks=True)
        ledger_hash = backup.digest(ledger)
        for mode in sorted(backup.MODES):
            generation = self.create(mode)
            manifest = generation / "manifest.json"
            valid = backup.read_json(manifest)
            malformed = [None, False, 1, 1.0, "private-manifest-alias", [], sorted(valid)]
            malformed += [dict(valid, version=value)
                          for value in (True, False, 1.0, "1", None, 0, 2, [], {})]
            malformed += [{key: value for key, value in valid.items() if key != "version"}]
            for value in malformed:
                with self.subTest(mode=mode, manifest=value):
                    backup.write_json(manifest, value)
                    before = backup.inventory(generation)
                    with mock.patch.object(restore, "service_load") as service, \
                         mock.patch.object(restore, "private_database") as database, \
                         mock.patch.object(backup, "run") as run:
                        with self.assertRaisesRegex(backup.BackupError, "^invalid_generation_manifest$"):
                            restore.restore(self.p, generation.name, ledger)
                        stdout, stderr = io.StringIO(), io.StringIO()
                        command = ["backup", "--policy", "/synthetic/policy", "restore",
                                   "--generation", generation.name, "--tombstones", str(ledger)]
                        with mock.patch.object(backup, "policy_load", return_value=self.p), \
                             mock.patch.object(sys, "argv", command), \
                             contextlib.redirect_stdout(stdout), contextlib.redirect_stderr(stderr):
                            self.assertEqual(backup.main(), 1)
                        self.assertEqual(stdout.getvalue(), "")
                        self.assertEqual(json.loads(stderr.getvalue()),
                                         dict(event="restore", result="failed", code="invalid_generation_manifest"))
                        service.assert_not_called()
                        database.assert_not_called()
                        run.assert_not_called()
                    self.assertFalse(list(self.p["restore_root"].glob("candidate-*")))
                    self.assertEqual(backup.inventory(generation), before)
                    self.assertEqual(backup.inventory(self.base / "live", exclude_locks=True), live)
                    self.assertEqual(backup.inventory(self.base / "journals", exclude_locks=True), journals)
                    self.assertEqual(backup.digest(ledger), ledger_hash)
            backup.write_json(manifest, valid)
            self.assertEqual(backup.verify(generation), valid)

    def test_tombstone_preflight_fails_closed(self):
        captured = int(time.time()) - 10
        restore.tombstone_preflight(self.ledger(), self.p, captured)
        for changes, code in (({"captured_at": captured - 1}, "stale"),
                              ({"captured_at": int(time.time()) + 60}, "stale"),
                              ({"policy_sha256": "invalid"}, "policy_mismatch"),
                              ({"tombstones": [{"account": "synthetic-erased"}]}, "propagation_required")):
            with self.subTest(changes=changes), self.assertRaisesRegex(backup.BackupError, code):
                restore.tombstone_preflight(self.ledger(**changes), self.p, captured)

    def test_restore_requires_a_version_one_tombstone_object(self):
        ledger_path = self.ledger()
        valid = backup.read_json(ledger_path)
        malformed = [None, False, 1, 1.0, "version", [], sorted(valid)]
        malformed += [dict(valid, version=value)
                      for value in (True, False, 1.0, "1", None, 0, 2, [], {})]
        malformed += [{key: value for key, value in valid.items() if key != "version"},
                      dict(valid, unexpected=True)]
        for mode in sorted(backup.MODES):
            generation = self.create(mode)
            before = backup.inventory(generation)
            live = backup.inventory(self.base / "live", exclude_locks=True)
            journals = backup.inventory(self.base / "journals", exclude_locks=True)
            for value in malformed:
                with self.subTest(mode=mode, ledger=value):
                    backup.write_json(ledger_path, value)
                    ledger_hash = backup.digest(ledger_path)
                    with self.assertRaisesRegex(backup.BackupError,
                                                "^invalid_tombstone_evidence$"):
                        restore.tombstone_preflight(ledger_path, self.p,
                                                   backup.read_json(generation / "manifest.json")["created"])
                    with mock.patch.object(restore, "service_load") as service, \
                         mock.patch.object(restore, "private_database") as database, \
                         mock.patch.object(backup, "run") as run:
                        with self.assertRaisesRegex(backup.BackupError,
                                                    "^invalid_tombstone_evidence$"):
                            restore.restore(self.p, generation.name, ledger_path)
                        service.assert_not_called()
                        database.assert_not_called()
                        run.assert_not_called()
                    self.assertFalse(list(self.p["restore_root"].glob("candidate-*")))
                    self.assertEqual(backup.digest(ledger_path), ledger_hash)
                    self.assertEqual(backup.inventory(generation), before)
                    self.assertEqual(backup.inventory(self.base / "live", exclude_locks=True), live)
                    self.assertEqual(backup.inventory(self.base / "journals", exclude_locks=True), journals)

    def test_restore_rejects_tombstones_before_candidate_or_service(self):
        for mode in sorted(backup.MODES):
            with self.subTest(mode=mode):
                self.create(mode)
                ledger = self.ledger(tombstones=[{"account": "synthetic-erased"}])
                with mock.patch.object(restore, "service_load") as service:
                    with self.assertRaisesRegex(backup.BackupError, "propagation_required"):
                        restore.restore(self.p, None, ledger)
                    service.assert_not_called()
                self.assertFalse(list(self.p["restore_root"].glob("candidate-*")))


    def test_failed_service_never_qualifies_and_preserves_generation(self):
        generation = self.create()
        before = backup.inventory(generation)
        live = backup.inventory(self.base / "live", exclude_locks=True)
        with mock.patch.object(backup, "run", return_value=b'{"players_loaded": 1}'), \
             mock.patch.object(restore, "service_load", side_effect=backup.BackupError("synthetic_service_failure")):
            with self.assertRaisesRegex(backup.BackupError, "synthetic_service_failure"):
                restore.restore(self.p, None, self.ledger())
        candidates = list(self.p["restore_root"].glob("candidate-*"))
        self.assertEqual(len(candidates), 1)
        self.assertTrue((candidates[0] / "FAILED.json").is_file())
        self.assertFalse((candidates[0] / "QUALIFIED.json").exists())
        self.assertEqual(backup.inventory(generation), before)
        self.assertEqual(backup.inventory(self.base / "live", exclude_locks=True), live)

    def test_erasure_evidence_changed_during_restore_never_qualifies(self):
        generation = self.create()
        before = backup.inventory(generation)
        ledger = self.ledger()
        def change_evidence(*unused):
            self.ledger(tombstones=[{"account": "synthetic-erased"}])
        with mock.patch.object(backup, "run", return_value=b'{"players_loaded": 1}'), \
             mock.patch.object(restore, "service_load", change_evidence):
            with self.assertRaisesRegex(backup.BackupError, "propagation_required"):
                restore.restore(self.p, None, ledger)
        self.assertFalse(list(self.p["restore_root"].glob("candidate-*/QUALIFIED.json")))
        self.assertEqual(backup.inventory(generation), before)

if __name__ == "__main__":
    unittest.main()
