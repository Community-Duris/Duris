#!/usr/bin/env python3
"""Disposable Linux/MariaDB/native recovery integration.

Build bin/server/dms_new with make -C src, and bin/server/dms_restore_flatfile
with PERSISTENCE_BACKEND=flatfile and an explicit DMS_BINARY path. Then run
DURIS_RUN_BACKUP_INTEGRATION=1 python3 tests/async/test_persistence_backup_integration.py.
Requires g++, libcrypto,
MariaDB server/client tools, bash, openssl, iproute2 and unshare permission (CAP_SYS_ADMIN
in the validation container). Uses no existing DB, runtime .env, Redis or game.
Every daemon has its own new datadir and Unix socket with TCP disabled; candidate
game processes boot in their own network namespaces and are terminated afterward.
All filesystem deletion is limited to this test's TemporaryDirectory. Startup
timeouts (60s) may need investigation on heavily contended machines.
"""
import json
import hashlib
import struct
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import time
import unittest
from unittest import mock

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))
import persistence_backup as backup
import persistence_restore as restore
import build_restore_qualifier as native
from _restore_fixture import build as build_fixture
from test_flatfile_restore_economic_authority import build_fixture as build_economic_fixture
import migration_runner as migrations
from test_persistence_backup import policy


def sql(env, query=None, payload=None):
    args = ["mysql", "--no-defaults", "--protocol=socket", "--socket=" + env["DB_SOCKET"],
            "--user=restore", "-N", "-B", "duris_restore"]
    if query is not None:
        args += ["-e", query]
    return backup.run(args, env=env, input=payload).decode().strip()


@unittest.skipUnless(os.environ.get("DURIS_RUN_BACKUP_INTEGRATION") == "1",
                     "requires explicit disposable Linux integration invocation")
class PersistenceRecoveryIntegration(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        for command in ("g++", "mysql", "mysqldump", "mariadbd", "mariadb-install-db", "unshare", "openssl", "ip"):
            if not shutil.which(command):
                raise RuntimeError("integration prerequisite unavailable: " + command)
        for name in ("dms_new", "dms_restore_flatfile"):
            if not (ROOT / "bin/server" / name).is_file():
                raise RuntimeError("build both backend server binaries before integration")
        cls.old_umask = os.umask(0o077)
        cls.fixture = ROOT / "bin/tools/persistence_restore_fixture"
        cls.native_built = False

    @classmethod
    def build_native_fixture(cls):
        if cls.native_built:
            return
        native.build()
        build_fixture(cls.fixture)
        cls.native_built = True

    @classmethod
    def tearDownClass(cls):
        os.umask(cls.old_umask)

    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="duris-restore-integration-")
        self.addCleanup(self.temp.cleanup)
        self.base = Path(self.temp.name)
        self.p = policy(self.base)
        self.p["journal_roots"] = {}
        self.p["restore_root"].mkdir(mode=0o700)
        subprocess.run(["mount", "-t", "tmpfs", "-o", "size=512M,mode=0700", "tmpfs",
                        str(self.p["restore_root"])], check=True)
        self.addCleanup(subprocess.run, ["umount", str(self.p["restore_root"])], check=True)

    def ledger(self):
        path = self.base / "independent-tombstones.json"
        backup.write_json(path, dict(version=1, captured_at=int(time.time()), tombstones=[],
                                    policy_sha256=backup.digest(ROOT / "migrations/data_lifecycle_manifest.json")))
        return path

    def test_flatfile_economic_record_loss_refuses_before_service_boot(self):
        self.build_native_fixture()
        fixture = build_economic_fixture(ROOT / "bin/tools/flatfile_restore_authority_fixture")
        for mode in ("records", "source-claims"):
            live = self.base / ("live-" + mode)
            self.p["live_roots"] = [live]
            for directory in ("identities/accounts", "identities/names", "players", "domains"):
                (live / directory).mkdir(mode=0o700, parents=True, exist_ok=True)
            backup.run([str(fixture), str(live), mode], env=dict(
                os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
                UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1"))
            proof = self.base / ("intact-economic-proof-" + mode)
            proof.mkdir(mode=0o700)
            backup.write_json(proof / "ISOLATED_RESTORE", {"synthetic": True})
            shutil.copytree(live, proof / "state")
            backup.run([str(ROOT / "bin/tools/qualify_flatfile_restore"),
                        "--state-preflight", str(proof / "state")])
            names = (("bucket-01.eai", "bucket-01-0.eas", "bucket-01-1.eas") if mode == "records"
                     else sorted(path.name for path in (live / "economic-evidence").glob("source-claim-*.bin")))
            self.assertEqual(len(names), 3 if mode == "records" else 2)
            for name in names:
                with self.subTest(mode=mode, missing=name):
                    path = live / "economic-evidence" / name
                    original = path.read_bytes()
                    path.unlink()
                    source_before = backup.inventory(live, exclude_locks=True)
                    with mock.patch.dict(os.environ, {"FLATFILE_STATE_DIR": str(live)}):
                        result = backup.backup(self.p, "flatfile-primary")
                    generation = self.p["root"] / result["generation"]
                    captured = backup.inventory(generation)
                    self.assertNotIn("state/economic-evidence/" + name,
                                     backup.verify(generation)["files"])
                    self.assertIn("state/economic-evidence/authority.eal",
                                  backup.verify(generation)["files"])
                    with mock.patch.object(restore, "service_load") as service:
                        with self.assertRaises(backup.BackupError):
                            restore.restore(self.p, result["generation"], self.ledger())
                        service.assert_not_called()
                    self.assertFalse(list(self.p["restore_root"].glob("candidate-*/QUALIFIED.json")))
                    self.assertEqual(backup.inventory(generation), captured)
                    self.assertEqual(backup.inventory(live, exclude_locks=True), source_before)
                    path.write_bytes(original)

    @unittest.skipUnless(os.geteuid() == 0, "requires root to model a foreign-owned checkout")
    def test_isolated_service_boot_from_private_foreign_owned_checkout(self):
        self.build_native_fixture()
        candidate = self.p["restore_root"] / "candidate-private-checkout"
        candidate.mkdir(mode=0o700)
        backup.write_json(candidate / "ISOLATED_RESTORE", {"generation": "synthetic"})
        backup.run([str(self.fixture), "seed", str(candidate / "state")])
        checkout = self.base / "private-checkout"
        checkout.mkdir(mode=0o700)
        for name in ("areas_mini", "lib"):
            shutil.copytree(ROOT / name, checkout / name)
        for name in ("bin/server/dms_restore_flatfile", "scripts/qualify_service_restore.py"):
            destination = checkout / name
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(ROOT / name, destination)
        # Outer root can read this checkout, but mapped namespace root has no
        # host CAP_DAC_OVERRIDE and cannot traverse it. Do not relax its mode.
        os.chown(checkout, 65534, 65534)
        env = dict(restore.clean_environment(candidate), FLATFILE_STATE_DIR=str(candidate / "state"))
        with mock.patch.object(backup, "ROOT", checkout):
            restore.service_load(candidate, "flatfile-primary", env)
        self.assertEqual(checkout.stat().st_uid, 65534)
        self.assertEqual(checkout.stat().st_mode & 0o777, 0o700)
        for journal in ("players/player-save.journal", "critical/critical-command.journal"):
            self.assertEqual((candidate / "journals" / journal).stat().st_size, 0)

    def seed_wal(self, live, blocked=False):
        backup.run([str(self.fixture), "seed-wal-blocked" if blocked else "seed-wal", str(live)])
        journals = live.parent / "journals"
        self.p["journal_roots"] = {name: journals / name for name in ("players", "critical")}
        self.p["live_roots"] = [live, *self.p["journal_roots"].values()]
        for relative in ("players/player-save.journal", "critical/critical-command.journal"):
            self.assertGreater((journals / relative).stat().st_size, 0)
        return journals
    def test_flatfile_real_pending_replay_account_player_domain_load_and_boot(self):
        self.build_native_fixture()
        live = self.base / "live"
        backup.run([str(self.fixture), "seed", str(live)])
        journals = self.seed_wal(live)
        receipt_relative = "critical/locker-identification/42.receipt"
        backup.run([str(self.fixture), "seed-receipt", str(journals / "critical/locker-identification")])
        receipt_bytes = (journals / receipt_relative).read_bytes()
        journal_before = backup.inventory(journals)
        self.assertTrue((live / "domains/.critical-authority-transaction").exists())
        before = backup.inventory(live, exclude_locks=True)
        with mock.patch.dict(os.environ, {"FLATFILE_STATE_DIR": str(live)}):
            result = backup.backup(self.p, "flatfile-primary")
        generation = self.p["root"] / result["generation"]
        captured = backup.inventory(generation)
        self.assertTrue(backup.verify(generation)["pending_transaction"])
        receipt = restore.restore(self.p, result["generation"], self.ledger())
        self.assertEqual(receipt["result"], "qualified")
        self.assertEqual(receipt["checks"], {"accounts": 1, "identities": 1, "players_loaded": 1, "snapshots": 1})
        candidate = self.p["restore_root"] / receipt["candidate"]
        self.assertEqual((candidate / "journals" / receipt_relative).read_bytes(), receipt_bytes)
        self.assertEqual((generation / "journals" / receipt_relative).read_bytes(), receipt_bytes)
        recovered_before_verify = backup.inventory(candidate / "state", exclude_locks=True)
        backup.run([str(self.fixture), "verify-wal", str(candidate / "state")])
        self.assertEqual(backup.inventory(candidate / "state", exclude_locks=True), recovered_before_verify)
        self.assertEqual(backup.inventory(journals), journal_before)
        for relative in ("players/player-save.journal", "critical/critical-command.journal"):
            self.assertEqual((candidate / "journals" / relative).stat().st_size, 0)
        backup.run([str(ROOT / "bin/tools/qualify_flatfile_restore"), "--journals-drained", str(candidate)])
        # Repeated native verification proves recovery is idempotent.
        recovered = backup.inventory(candidate / "state", exclude_locks=True)
        backup.run([str(ROOT / "bin/tools/qualify_flatfile_restore"), str(candidate / "state")])
        self.assertEqual(backup.inventory(candidate / "state", exclude_locks=True), recovered)
        self.assertEqual(backup.inventory(live, exclude_locks=True), before)
        self.assertEqual(backup.inventory(generation), captured)
        self.assertIn(b"Entering game loop.", (candidate / "service.log").read_bytes())

    def test_spell_receipt_state_qualification(self):
        self.build_native_fixture()
        for case in ("valid", "corrupt", "wrong-operation", "wrong-pid", "public-file",
                     "hardlink", "future-receipt", "missing-player", "retired-owner"):
            with self.subTest(case=case):
                candidate = self.p["restore_root"] / ("spell-" + case)
                candidate.mkdir(mode=0o700)
                backup.write_json(candidate / "ISOLATED_RESTORE", {"synthetic": True})
                state = candidate / "state"
                backup.run([str(self.fixture), "seed", str(state)])
                player = state / "players/42.snapshot"
                prior_player = player.read_bytes()
                backup.run([str(self.fixture), "seed-spell-receipt", str(state)])
                receipt = state / "players/42-a5000000000000000000000000000000.spell"
                self.assertTrue(receipt.is_file())
                if case == "corrupt":
                    receipt.write_bytes(b"corrupt")
                elif case == "wrong-operation":
                    receipt.rename(receipt.with_name("42-b6000000000000000000000000000000.spell"))
                elif case == "wrong-pid":
                    receipt.rename(receipt.with_name("43-a5000000000000000000000000000000.spell"))
                elif case == "public-file":
                    receipt.chmod(0o644)
                elif case == "hardlink":
                    os.link(receipt, candidate / "alias")
                elif case == "future-receipt":
                    player.write_bytes(prior_player)
                elif case == "missing-player":
                    player.unlink()
                elif case == "retired-owner":
                    backup.run([str(self.fixture), "retire-spell-owner", str(state)])
                for phase in ("--state-preflight", None):
                    command = [str(ROOT / "bin/tools/qualify_flatfile_restore")]
                    if phase:
                        command.append(phase)
                    command.append(str(state))
                    if case in ("valid", "retired-owner"):
                        backup.run(command)
                    else:
                        with self.assertRaises(backup.BackupError):
                            backup.run(command)

                if case == "valid":
                    live = self.base / "spell-live"
                    shutil.copytree(state, live)
                    self.p["live_roots"] = [live]
                    before = backup.inventory(live, exclude_locks=True)
                    with mock.patch.dict(os.environ, {"FLATFILE_STATE_DIR": str(live)}):
                        captured = backup.backup(self.p, "flatfile-primary")
                    generation = self.p["root"] / captured["generation"]
                    relative = receipt.relative_to(state)
                    expected = receipt.read_bytes()
                    self.assertEqual((generation / "state" / relative).read_bytes(), expected)
                    restored = restore.restore(self.p, captured["generation"], self.ledger())
                    self.assertEqual(restored["result"], "qualified")
                    restored_candidate = self.p["restore_root"] / restored["candidate"]
                    self.assertEqual((restored_candidate / "state" / relative).read_bytes(), expected)
                    self.assertEqual(backup.inventory(live, exclude_locks=True), before)

    def test_locker_receipt_qualification_rejects_corruption_and_unexpected_entries(self):
        self.build_native_fixture()
        for case in ("valid", "corrupt", "wrong-pid", "zero-pid", "oversized", "extra-file",
                     "nested-directory", "nonempty-lock", "public-file", "public-directory",
                     "symlink-file", "symlink-directory", "hardlink-file"):
            with self.subTest(case=case):
                candidate = self.p["restore_root"] / case
                candidate.mkdir(mode=0o700)
                backup.write_json(candidate / "ISOLATED_RESTORE", {"synthetic": True})
                store = candidate / "journals/critical/locker-identification"
                backup.run([str(self.fixture), "seed-receipt", str(store)])
                receipt = store / "42.receipt"
                lock = store / ".service-lock"
                lock.touch(mode=0o600)
                if case == "corrupt":
                    receipt.write_bytes(b"corrupt")
                elif case in ("wrong-pid", "zero-pid"):
                    receipt.rename(store / ("43.receipt" if case == "wrong-pid" else "0.receipt"))
                elif case == "oversized":
                    receipt.write_bytes(b"x" * (70 * 1024))
                elif case == "extra-file":
                    (store / "unexpected").touch(mode=0o600)
                elif case == "nested-directory":
                    (store / "unexpected").mkdir(mode=0o700)
                elif case == "nonempty-lock":
                    lock.write_bytes(b"not lock metadata")
                elif case == "public-file":
                    receipt.chmod(0o644)
                elif case == "public-directory":
                    store.chmod(0o755)
                elif case == "symlink-file":
                    target = candidate / "target"
                    receipt.rename(target)
                    receipt.symlink_to(target)
                elif case == "symlink-directory":
                    target = candidate / "target"
                    store.rename(target)
                    store.symlink_to(target, target_is_directory=True)
                elif case == "hardlink-file":
                    os.link(receipt, candidate / "alias")
                for phase in ("--journals-preflight", "--journals-drained"):
                    command = [str(ROOT / "bin/tools/qualify_flatfile_restore"), phase, str(candidate)]
                    if case == "valid":
                        backup.run(command)
                    else:
                        with self.assertRaises(backup.BackupError):
                            backup.run(command)

    def test_interrupted_bank_domain_and_legacy_transactions_restore_exactly_once(self):
        self.build_native_fixture()
        for legacy, intent in ((False, ".player-domain-transaction"), (True, ".currency-transaction")):
            with self.subTest(intent=intent):
                case_root = self.base / ("legacy-bank" if legacy else "domain-bank")
                case_root.mkdir(mode=0o700)
                live = case_root / "live"
                backup.run([str(self.fixture), "seed", str(live)])
                player_before = (live / "domains/player-42.domain").read_bytes()
                bank_before = (live / "domains/bank-account-one-0.domain").read_bytes()
                backup.run([str(self.fixture), "seed-legacy-bank-interrupted" if legacy else "seed-bank-interrupted", str(live)])
                self.assertEqual((live / "domains/player-42.domain").read_bytes(), player_before)
                self.assertNotEqual((live / "domains/bank-account-one-0.domain").read_bytes(), bank_before)
                self.assertTrue((live / "domains" / intent).is_file())
                self.p["root"] = case_root / "backups"
                self.p["live_roots"] = [live]
                self.p["journal_roots"] = {}
                before = backup.inventory(live, exclude_locks=True)
                with mock.patch.dict(os.environ, {"FLATFILE_STATE_DIR": str(live)}):
                    result = backup.backup(self.p, "flatfile-primary")
                generation = self.p["root"] / result["generation"]
                manifest = backup.verify(generation)
                self.assertIn("state/domains/" + intent, manifest["files"])
                self.assertTrue(manifest["pending_transaction"])
                captured = backup.inventory(generation)
                receipt = restore.restore(self.p, result["generation"], self.ledger())
                self.assertEqual(receipt["result"], "qualified")
                candidate = self.p["restore_root"] / receipt["candidate"]
                recovered = backup.inventory(candidate / "state", exclude_locks=True)
                backup.run([str(self.fixture), "verify-bank", str(candidate / "state")])
                self.assertEqual(backup.inventory(candidate / "state", exclude_locks=True), recovered)
                self.assertEqual(backup.inventory(live, exclude_locks=True), before)
                self.assertEqual(backup.inventory(generation), captured)
    def test_first_player_snapshot_recovers_from_wal_without_persisted_baseline(self):
        self.build_native_fixture()
        live = self.base / "live"
        backup.run([str(self.fixture), "seed-first-wal", str(live)])
        self.assertFalse(list((live / "players").glob("*.snapshot")))
        journals = self.base / "journals"
        self.assertGreater((journals / "players/player-save.journal").stat().st_size, 0)
        self.assertEqual((journals / "critical/critical-command.journal").stat().st_size, 0)
        self.p["journal_roots"] = {name: journals / name for name in ("players", "critical")}
        self.p["live_roots"] = [live, *self.p["journal_roots"].values()]
        before = backup.inventory(live, exclude_locks=True)
        journal_before = backup.inventory(journals)
        with mock.patch.dict(os.environ, {"FLATFILE_STATE_DIR": str(live)}):
            result = backup.backup(self.p, "flatfile-primary")
        generation = self.p["root"] / result["generation"]
        captured = backup.inventory(generation)
        receipt = restore.restore(self.p, result["generation"], self.ledger())
        self.assertEqual(receipt["result"], "qualified")
        candidate = self.p["restore_root"] / receipt["candidate"]
        backup.run([str(self.fixture), "verify", str(candidate / "state")])
        self.assertEqual((candidate / "journals/players/player-save.journal").stat().st_size, 0)
        self.assertEqual(backup.inventory(live, exclude_locks=True), before)
        self.assertEqual(backup.inventory(journals), journal_before)
        self.assertEqual(backup.inventory(generation), captured)
    def test_corrupt_wal_is_refused_and_unreplayable_player_is_quarantined(self):
        self.build_native_fixture()
        actual_service = restore.service_load
        for case, relative in (("player-corrupt", "players/player-save.journal"),
                               ("critical-corrupt", "critical/critical-command.journal"),
                               ("player-blocked", None)):
            with self.subTest(case=case):
                case_root = self.base / case
                case_root.mkdir(mode=0o700)
                live = case_root / "live"
                backup.run([str(self.fixture), "seed", str(live)])
                journals = self.seed_wal(live, blocked=relative is None)
                player_wal = (journals / "players/player-save.journal").read_bytes()
                # Prove both native records are structurally valid first. The
                # blocked record is a partial update for a missing player PID.
                proof = case_root / "preflight-proof"
                proof.mkdir(mode=0o700)
                backup.write_json(proof / "ISOLATED_RESTORE", {"synthetic": True})
                shutil.copytree(journals, proof / "journals")
                backup.run([str(ROOT / "bin/tools/qualify_flatfile_restore"), "--journals-preflight", str(proof)])
                if relative is not None:
                    path = journals / relative
                    content = bytearray(path.read_bytes())
                    content[-1] ^= 0x5a
                    path.write_bytes(content)
                self.p["root"] = case_root / "backups"
                live_before = backup.inventory(live, exclude_locks=True)
                journal_before = backup.inventory(journals)
                with mock.patch.dict(os.environ, {"FLATFILE_STATE_DIR": str(live)}):
                    result = backup.backup(self.p, "flatfile-primary")
                generation = self.p["root"] / result["generation"]
                captured = backup.inventory(generation)
                backup.verify(generation)
                with mock.patch.object(restore, "service_load", wraps=actual_service) as service:
                    if relative is None:
                        receipt = restore.restore(self.p, result["generation"], self.ledger())
                        self.assertEqual(receipt["result"], "qualified")
                        service.assert_called_once()
                        candidate = self.p["restore_root"] / receipt["candidate"]
                        archive = candidate / "journals/players/player-save.journal.quarantine.archive"
                        self.assertIn(player_wal, archive.read_bytes())
                        self.assertEqual((candidate / "journals/players/player-save.journal").stat().st_size, 0)
                        backup.run([str(self.fixture), "verify-quarantined-wal", str(candidate / "state")])
                    else:
                        with self.assertRaises(backup.BackupError):
                            restore.restore(self.p, result["generation"], self.ledger())
                        service.assert_not_called()
                        self.assertFalse(list(self.p["restore_root"].glob("candidate-*/QUALIFIED.json")))
                self.assertEqual(backup.inventory(live, exclude_locks=True), live_before)
                self.assertEqual(backup.inventory(journals), journal_before)
                self.assertEqual(backup.inventory(generation), captured)
    def test_valid_manifest_with_corrupt_lazy_catalog_never_qualifies(self):
        self.build_native_fixture()
        live = self.base / "live"
        backup.run([str(self.fixture), "seed", str(live)])
        def kingdom_catalog(realm_id):
            # Supported v1 frame: count, four int32 identity/claim fields,
            # four int64 resources, int64 upkeep and two int32 status fields.
            payload = struct.pack("<I4i5q2i", 1, 1, realm_id, 0, 0, 0, 0, 0, 0, 0, 0, 0)
            return b"DURKING\0" + struct.pack("<IIQ", 1, len(payload), 1) + hashlib.sha256(payload).digest() + payload
        (live / "metadata").mkdir(mode=0o700, exist_ok=True)
        (live / "metadata/kingdom_realms").write_bytes(kingdom_catalog(0))

        # A retained, depleted UID is an independent authority, including when
        # its old owner snapshot is no longer present in this restore generation.
        mana_payload = b"DURMANA\x01" + struct.pack("<8Q", 81, 7, 1, 2, 100000, 1, 2, 100)
        (live / "domains/artifact-mana-81").write_bytes(
            mana_payload + hashlib.sha256(mana_payload).digest())
        proof = self.base / "kingdom-format-proof"
        proof.mkdir(mode=0o700)
        backup.write_json(proof / "ISOLATED_RESTORE", {"synthetic": True})
        shutil.copytree(live, proof / "state")
        backup.run([str(ROOT / "bin/tools/qualify_flatfile_restore"), str(proof / "state")])
        for relative in ("domains/corpse_operation_catalog", "domains/shop_trade_operations",
                         "metadata/kingdom_realms", "domains/locker_catalog",
                         "domains/artifact-mana-81"):
            with self.subTest(catalog=relative):
                corrupt = live / relative
                corrupt.parent.mkdir(mode=0o700, parents=True, exist_ok=True)
                original = corrupt.read_bytes() if corrupt.exists() else None
                corrupt.write_bytes(kingdom_catalog(-1) if relative == "metadata/kingdom_realms"
                                    else b"synthetic-corrupt-native-catalog")
                with mock.patch.dict(os.environ, {"FLATFILE_STATE_DIR": str(live)}):
                    result = backup.backup(self.p, "flatfile-primary")
                generation = self.p["root"] / result["generation"]
                self.assertIn("state/" + relative, backup.verify(generation)["files"])
                before = backup.inventory(generation)
                with mock.patch.object(restore, "service_load") as service:
                    with self.assertRaises(backup.BackupError):
                        restore.restore(self.p, result["generation"], self.ledger())
                    service.assert_not_called()
                self.assertFalse(list(self.p["restore_root"].glob("candidate-*/QUALIFIED.json")))
                self.assertEqual(backup.inventory(generation), before)
                if original is None:
                    corrupt.unlink()
                else:
                    corrupt.write_bytes(original)
    def test_mariadb_full_dump_schema_history_values_and_isolated_service_boot(self):
        self.sql_full_dump_restore("mariadb")

    @unittest.skipUnless(os.environ.get("DURIS_RUN_MYSQL_BACKUP_INTEGRATION") == "1",
                         "requires explicit disposable MySQL 8.0 integration invocation")
    def test_mysql_full_dump_schema_history_values_and_isolated_service_boot(self):
        self.sql_full_dump_restore("mysql")

    def sql_full_dump_restore(self, engine):
        self.p["restore_database_engine"] = engine
        self.build_native_fixture()
        source = self.base / "live"
        source.mkdir(mode=0o700)
        def require_selected_engine(env):
            version = sql(env, "SELECT VERSION();")
            if engine == "mysql":
                self.assertTrue(version.startswith("8.0.") and "MariaDB" not in version, version)
            else:
                self.assertIn("MariaDB", version)
            return version
        with restore.private_database(source, engine) as env:
            source_version = require_selected_engine(env)
            self.p["journal_roots"] = {"players": Path(env["PLAYER_SAVE_JOURNAL_DIR"]),
                                       "critical": Path(env["CRITICAL_COMMAND_JOURNAL_DIR"])}
            for journal in self.p["journal_roots"].values():
                journal.mkdir(mode=0o700, parents=True)
            sql(env, payload=(ROOT / "migrations/bootstrap_multithread_safe.sql").read_bytes())
            with mock.patch.dict(os.environ, env, clear=True):
                manifest = migrations.load_manifest()
                executor = migrations.MysqlExecutor(manifest)
                executor.adopt("fresh_bootstrap")
                migrations.run_pending(manifest, executor)
            sql(env, "INSERT INTO accounts(account_name,email,confirmed) VALUES('SyntheticRestore','fixture@example.test',1);"
                     "INSERT INTO player_data(pid,name,account_name,copper,silver,gold,platinum,epics,epic_revision) "
                     "VALUES(42,'SyntheticPlayer','SyntheticRestore',11,12,13,14,15,0);"
                     "INSERT INTO account_characters(account_name,pid,char_name) VALUES('SyntheticRestore',42,'SyntheticPlayer');"
                     "INSERT INTO account_banks(id,account_name,racewar,bank_copper,bank_silver,bank_gold,bank_platinum) "
                     "VALUES(1,'SyntheticRestore',0,21,22,23,24);"
                     "INSERT INTO currency_wallet_baseline(pid,opening_copper,opening_silver,opening_gold,opening_platinum) VALUES(42,11,12,13,14);"
                     "INSERT INTO currency_bank_baseline(bank_id,opening_copper,opening_silver,opening_gold,opening_platinum) VALUES(1,21,22,23,24);"
                     "INSERT INTO epic_balance_baseline(pid,opening_balance) VALUES(42,15);"
                     "INSERT INTO combat_frag_baseline(pid,opening_frags) VALUES(42,0);")
            restore.database_qualify(env)
            query = ("SELECT CONCAT(a.account_name,':',p.pid,':',p.copper,':',p.epics,':',b.bank_gold) "
                     "FROM accounts a JOIN player_data p ON p.account_name=a.account_name "
                     "JOIN account_banks b ON b.account_name=a.account_name WHERE p.pid=42;")
            expected = sql(env, query)
            self.assertEqual(expected, "SyntheticRestore:42:11:15:23")
            store = self.p["journal_roots"]["critical"] / "locker-identification"
            backup.run([str(self.fixture), "seed-receipt", str(store)])
            receipt_bytes = (store / "42.receipt").read_bytes()
            with mock.patch.dict(os.environ, env, clear=True):
                result = backup.backup(self.p, "mariadb-primary")
            generation = self.p["root"] / result["generation"]
            captured = backup.inventory(generation)
            actual_service_load = restore.service_load
            checked = []
            def check_values_then_boot(candidate, mode, restored_env):
                self.assertEqual(require_selected_engine(restored_env), source_version)
                self.assertNotEqual(restored_env["DB_SOCKET"], env["DB_SOCKET"])
                self.assertEqual(sql(restored_env, query), expected)
                checked.append(mode)
                actual_service_load(candidate, mode, restored_env)
            with mock.patch.object(restore, "service_load", check_values_then_boot):
                receipt = restore.restore(self.p, result["generation"], self.ledger())
            self.assertEqual(checked, ["mariadb-primary"])
            self.assertEqual(receipt["result"], "qualified")
            self.assertEqual(receipt["checks"]["database_engine"], engine)
            self.assertEqual(sql(env, query), expected)
            self.assertEqual(backup.inventory(generation), captured)
            candidate = self.p["restore_root"] / receipt["candidate"]
            self.assertEqual((candidate / "journals/critical/locker-identification/42.receipt").read_bytes(),
                             receipt_bytes)
            self.assertIn(b"Entering game loop.", (candidate / "service.log").read_bytes())
            sql(env, "INSERT INTO accounts(account_name,confirmed) VALUES('OtherSynthetic',1);"
                     "UPDATE player_data SET account_name='OtherSynthetic' WHERE pid=42;")
            with self.assertRaises(backup.BackupError):
                restore.database_qualify(env)
            sql(env, "UPDATE player_data SET account_name='SyntheticRestore' WHERE pid=42;")
            # Money authority also needs a witnessed revision, independently
            # of unchanged denomination totals.
            for table, key, identity, column in (
                    ("player_data", "pid", 42, "wallet_revision"),
                    ("account_banks", "id", 1, "bank_revision")):
                sql(env, f"UPDATE {table} SET {column}=9 WHERE {key}={identity};")
                with self.assertRaises(backup.BackupError):
                    restore.database_qualify(env)
                sql(env, f"UPDATE {table} SET {column}=0 WHERE {key}={identity};")
                restore.database_qualify(env)
            # Removing a cancelling money pair preserves every aggregate and
            # the latest after-image, but loses two recoverable native revisions.
            currency_rows = ((1, 1, 12, 20), (2, 2, 14, 18),
                             (3, -2, 12, 20), (4, -1, 11, 21))
            def currency_event(revision, delta, wallet_copper, bank_copper):
                identity = "f1" + f"{revision:030x}"
                sql(env, "INSERT IGNORE INTO critical_operation_inbox(operation_id,command_hash,"
                         "keys_hash,command_type,schema_version,payload_version,status,result_code,"
                         "result_payload,committed_at) VALUES(UNHEX('" + identity + "'),"
                         "UNHEX(REPEAT('11',32)),UNHEX(REPEAT('22',32)),1,1,1,1,0,X'',CURRENT_TIMESTAMP);")
                sql(env, "INSERT INTO currency_ledger(operation_id,pid,bank_id,"
                         "wallet_delta_copper,wallet_delta_silver,wallet_delta_gold,wallet_delta_platinum,"
                         "bank_delta_copper,bank_delta_silver,bank_delta_gold,bank_delta_platinum,"
                         "wallet_after_copper,wallet_after_silver,wallet_after_gold,wallet_after_platinum,"
                         "bank_after_copper,bank_after_silver,bank_after_gold,bank_after_platinum,"
                         "wallet_revision,bank_revision,reason_type,source_site) VALUES(UNHEX('" +
                         identity + "'),42,1," + str(delta) + ",0,0,0," + str(-delta) + ",0,0,0," +
                         str(wallet_copper) + ",12,13,14," + str(bank_copper) + ",22,23,24," +
                         str(revision) + "," + str(revision) + ",1,1);")
            for row in currency_rows:
                currency_event(*row)
            sql(env, "UPDATE player_data SET wallet_revision=4 WHERE pid=42;"
                     "UPDATE account_banks SET bank_revision=4 WHERE id=1;")
            restore.database_qualify(env)
            sql(env, "DELETE FROM currency_ledger WHERE pid=42 AND wallet_revision IN(2,3);")
            self.assertEqual(sql(env, query), expected)
            with self.assertRaises(backup.BackupError):
                restore.database_qualify(env)
            for row in currency_rows[1:3]:
                currency_event(*row)
            restore.database_qualify(env)
            sql(env, "DELETE FROM currency_ledger WHERE pid=42;"
                     "DELETE FROM critical_operation_inbox WHERE LEFT(HEX(operation_id),2)='F1';"
                     "UPDATE player_data SET wallet_revision=0 WHERE pid=42;"
                     "UPDATE account_banks SET bank_revision=0 WHERE id=1;")
            restore.database_qualify(env)
            # A balance alone cannot establish recoverable authority: a future
            # revision without an immutable event would fence legitimate retry.
            sql(env, "UPDATE player_data SET epic_revision=9 WHERE pid=42;")
            with self.assertRaises(backup.BackupError):
                restore.database_qualify(env)
            sql(env, "UPDATE player_data SET epic_revision=0 WHERE pid=42;")
            restore.database_qualify(env)
            # Remove a cancelling pair from an otherwise contiguous history:
            # aggregate value and the latest event remain exactly unchanged.
            event_rows = ((1, 1, 16), (2, 2, 18), (3, -2, 16), (4, 3, 19))
            def epic_event(revision, delta, balance):
                identity = f"{revision:032x}"
                sql(env, "INSERT INTO epic_ledger(operation_id,pid,delta,balance_after,"
                         "epic_revision,reason_type,reason_id,source_site) VALUES(UNHEX('" +
                         identity + "'),42," + str(delta) + "," + str(balance) + "," +
                         str(revision) + ",1,0,1);")
            for revision, delta, balance in event_rows:
                identity = f"{revision:032x}"
                sql(env, "INSERT INTO critical_operation_inbox(operation_id,command_hash,keys_hash,"
                         "command_type,schema_version,payload_version,status,result_payload,committed_at) "
                         "VALUES(UNHEX('" + identity + "'),UNHEX(REPEAT('11',32)),"
                         "UNHEX(REPEAT('22',32)),1,1,1,1,'',CURRENT_TIMESTAMP(6));")
                epic_event(revision, delta, balance)
            sql(env, "UPDATE player_data SET epics=19,epic_revision=4 WHERE pid=42;")
            restore.database_qualify(env)
            sql(env, "DELETE FROM epic_ledger WHERE pid=42 AND epic_revision IN(2,3);")
            with self.assertRaises(backup.BackupError):
                restore.database_qualify(env)
            for revision, delta, balance in event_rows[1:3]:
                epic_event(revision, delta, balance)
            restore.database_qualify(env)
            sql(env, "DELETE FROM epic_ledger WHERE pid=42;"
                     "DELETE FROM critical_operation_inbox WHERE operation_id IN("
                     "UNHEX('00000000000000000000000000000001'),"
                     "UNHEX('00000000000000000000000000000002'),"
                     "UNHEX('00000000000000000000000000000003'),"
                     "UNHEX('00000000000000000000000000000004'));"
                     "UPDATE player_data SET epics=15,epic_revision=0 WHERE pid=42;")
            restore.database_qualify(env)
            self.assertEqual(sql(env, query), expected)
            # Corrupt only the disposable source baseline: the same qualifier
            # that accepted restored values must now reject reconciliation.
            sql(env, "UPDATE currency_wallet_baseline SET opening_copper=999 WHERE pid=42;")
            with self.assertRaises(backup.BackupError):
                restore.database_qualify(env)
            sql(env, "UPDATE currency_wallet_baseline SET opening_copper=11 WHERE pid=42;"
                     "UPDATE mud_schema_history SET apply_checksum=UNHEX(REPEAT('00',32)) WHERE sequence_number=1;")
            with self.assertRaises(backup.BackupError):
                restore.database_qualify(env)


if __name__ == "__main__":
    unittest.main()
