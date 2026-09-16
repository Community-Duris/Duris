#!/usr/bin/env python3
"""Focused production-preflight/backup tests; no live target is used.

Set DURIS_TARGET_NATIVE_DB_IMAGE to mysql:8.0 or mariadb:10.11 for the real
Docker lifecycle + native dump/restore test. It creates/removes only its own
network-isolated containers and uses a synthetic production-named database.
"""
from __future__ import annotations

from dataclasses import replace
import hashlib
import os
from pathlib import Path
import secrets
import shlex
import subprocess
import sys
import tempfile
import time
import unittest

import importlib
sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "scripts"))
target_module = importlib.import_module("player_death_restitution_target")
backup_module = importlib.import_module("player_death_restitution_backup")
TargetError = target_module.TargetError
TargetPolicy = target_module.TargetPolicy
protected_file_digest = target_module.protected_file_digest
server_fingerprint = target_module.server_fingerprint
verify_backup_receipt = target_module.verify_backup_receipt
create_backup = backup_module.create_backup


class TargetTests(unittest.TestCase):
    def env(self, **updates):
        result = {"ENVIRONMENT": "test", "DB_HOST": "127.0.0.1", "DB_PORT": "3306",
                  "DB_USER": "fixture", "DB_NAME": "duris_fixture"}
        result.update(updates)
        return result

    def test_nonproduction_default(self):
        policy = TargetPolicy.from_environment(self.env())
        self.assertFalse(policy.production)
        self.assertEqual(policy.require_maintenance(), {"kind": "native-sql-exclusion-only"})

    def test_production_requires_literal_confirmation(self):
        for name in ("duris", "duris_prod", "duris_live_fixture"):
            with self.subTest(name=name), self.assertRaises(TargetError):
                TargetPolicy.from_environment(self.env(DB_NAME=name))
        with self.assertRaises(TargetError):
            TargetPolicy.from_environment(self.env(DB_NAME="duris_prod"),
                                          confirm_production_target="DURIS_PROD")

    def test_environment_alone_identifies_production(self):
        with self.assertRaises(TargetError):
            TargetPolicy.from_environment(self.env(ENVIRONMENT="production"))
        policy = TargetPolicy.from_environment(self.env(ENVIRONMENT="production"),
                                               confirm_production_target="duris_fixture")
        self.assertTrue(policy.production)
        with self.assertRaises(TargetError):
            policy.require_maintenance()

    def test_remote_production_transport_refused(self):
        for host in ("localhost", "db.example.invalid", "192.0.2.1"):
            with self.subTest(host=host), self.assertRaises(TargetError):
                TargetPolicy.from_environment(self.env(DB_HOST=host, ENVIRONMENT="production"),
                                              confirm_production_target="duris_fixture")

    def test_unsupported_backend_and_environment(self):
        for update in ({"PERSISTENCE_BACKEND": "file"}, {"ENVIRONMENT": ""}, {"DB_PORT": "0"}):
            with self.subTest(update=update), self.assertRaises(TargetError):
                TargetPolicy.from_environment(self.env(**update))

    def test_boundary_identity_is_not_a_container_name(self):
        policy = TargetPolicy.from_environment(self.env())
        with self.assertRaises(TargetError):
            replace(policy, maintenance_kind="docker", maintenance_id="a-convenient-name").require_maintenance()
        with self.assertRaises(TargetError):
            replace(policy, maintenance_kind="systemd", maintenance_id="duris-mud.service").require_maintenance()

    def test_backup_file_security_and_mutation(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "dump.sql"
            path.write_bytes(b"protected fixture backup\n")
            path.chmod(0o600)
            expected = hashlib.sha256(path.read_bytes()).hexdigest()
            self.assertEqual(protected_file_digest(path), expected)
            target = {"database": "fixture", "server_fingerprint": "a" * 64}
            receipt = {"format": "duris-death-restitution-backup-v1", "target": target,
                       "dump_path": str(path), "sha256": expected, "dump_exit_code": 0, "complete": True}
            verify_backup_receipt(receipt, target)
            with self.assertRaises(TargetError):
                verify_backup_receipt({**receipt, "dump_exit_code": False}, target)
            with self.assertRaises(TargetError):
                verify_backup_receipt(receipt, {"database": "other"})
            path.write_bytes(b"changed\n")
            with self.assertRaises(TargetError):
                verify_backup_receipt(receipt, target)
            path.chmod(0o644)
            with self.assertRaises(TargetError):
                protected_file_digest(path)
            link = Path(tmp) / "link.sql"
            link.symlink_to(path)
            with self.assertRaises(TargetError):
                protected_file_digest(link)

    @unittest.skipUnless(os.environ.get("DURIS_TARGET_NATIVE_DB_IMAGE"), "native DB image not requested")
    def test_real_maintenance_and_native_backup_round_trip(self):
        image = os.environ["DURIS_TARGET_NATIVE_DB_IMAGE"]
        self.assertIn(image, {"mysql:8.0", "mariadb:10.11"})
        db_name = "duris_prod_recovery_fixture"
        suffix = secrets.token_hex(5)
        created = []

        def run(args, *, data=None, timeout=40, check=True):
            result = subprocess.run(args, input=data, text=True, capture_output=True,
                                    timeout=timeout, check=False)
            if check and result.returncode:
                raise RuntimeError("disposable fixture command failed: " + result.stderr[-1000:])
            return result

        try:
            maria = image.startswith("mariadb:")
            auth = "MARIADB_ALLOW_EMPTY_ROOT_PASSWORD=1" if maria else "MYSQL_ALLOW_EMPTY_PASSWORD=1"
            database_env = ("MARIADB_DATABASE=" if maria else "MYSQL_DATABASE=") + db_name
            db_id = run(["docker", "create", "--name", "duris-331-target-db-" + suffix,
                         "--network", "none", "-e", auth, "-e", database_env,
                         image, "--event-scheduler=OFF"]).stdout.strip()
            created.append(db_id)
            run(["docker", "start", db_id])
            client = ["docker", "exec", "-i", db_id, "mysql", "--no-defaults",
                      "--protocol=TCP", "-h", "127.0.0.1", "-uroot", "-N", "-B", "--raw", db_name]
            deadline = time.monotonic() + 90
            while True:
                ready = run(client, data="SELECT 1;\n", check=False)
                if ready.returncode == 0 and ready.stdout.strip() == "1":
                    break
                if time.monotonic() >= deadline:
                    self.fail("disposable SQL target did not become authenticated-ready")
                time.sleep(0.5)
            runtime_id = run(["docker", "create", "--name", "duris-331-target-runtime-" + suffix,
                              "--network", "none", "--restart=no", "-e", "DB_HOST=127.0.0.1",
                              "-e", "DB_NAME=" + db_name, "duris-issue-213-tools:latest",
                              "sleep", "600"]).stdout.strip()
            created.append(runtime_id)
            env = self.env(ENVIRONMENT="production", DB_NAME=db_name, DB_USER="root")
            policy = TargetPolicy.from_environment(env, confirm_production_target=db_name,
                                                   maintenance_kind="docker", maintenance_id=runtime_id)
            with self.assertRaises(TargetError):
                policy.require_maintenance()  # merely created is not a stopped runtime
            run(["docker", "start", runtime_id])
            with self.assertRaises(TargetError):
                policy.require_maintenance()
            run(["docker", "stop", "--time", "1", runtime_id])
            self.assertEqual(policy.require_maintenance()["id"], runtime_id)
            with self.assertRaises(TargetError):
                replace(policy, database="wrong_database").require_maintenance()

            class DB:
                database = db_name

                def __init__(self):
                    self.env = {key: value for key, value in os.environ.items()
                                if not key.startswith(("DB_", "MYSQL"))}
                    self.env.update(env)

                def run(self, query):
                    return [line.split("\t") for line in run(client, data=query + "\n").stdout.splitlines() if line]

            db = DB()
            fingerprint = server_fingerprint(db)
            with self.assertRaises(TargetError):
                policy.identity(db)
            policy = replace(policy, expected_fingerprint=fingerprint)
            target = policy.identity(db)
            self.assertEqual(target["database"], db_name)
            with self.assertRaises(TargetError):
                replace(policy, expected_fingerprint="0" * 64).identity(db)
            db.run("CREATE TABLE backup_probe(value INT NOT NULL) ENGINE=InnoDB; INSERT INTO backup_probe VALUES(17)")

            def quiescent():
                rows = db.run("SELECT COUNT(*) FROM information_schema.PROCESSLIST WHERE ID<>CONNECTION_ID() AND USER NOT IN ('system user','event_scheduler','mysql.session')")
                if rows != [["0"]]:
                    raise TargetError("disposable target has another SQL client")

            with tempfile.TemporaryDirectory(prefix="duris-331-backup-") as tmp:
                wrapper = Path(tmp) / "native-dump"
                wrapper.write_text("#!/bin/sh\nexec docker exec " + shlex.quote(db_id) + " mysqldump --no-defaults \"$@\"\n")
                wrapper.chmod(0o700)
                path = Path(tmp) / "backup.json"
                receipt = create_backup(db, policy, path, quiescent, dump_binary=str(wrapper))
                self.assertEqual(receipt["target"], target)
                self.assertEqual(path.stat().st_mode & 0o777, 0o600)
                verify_backup_receipt(receipt, target)
                with self.assertRaises(TargetError):
                    create_backup(db, policy, path, quiescent, dump_binary=str(wrapper))
                db.run("UPDATE backup_probe SET value=99")
                self.assertEqual(db.run("SELECT value FROM backup_probe"), [["99"]])
                # Restore only this disposable fixture to prove the native dump is usable.
                db.run(Path(receipt["dump_path"]).read_text())
                self.assertEqual(db.run("SELECT value FROM backup_probe"), [["17"]])
                with Path(receipt["dump_path"]).open("ab") as output:
                    output.write(b"\n-- changed after approval\n")
                with self.assertRaises(TargetError):
                    verify_backup_receipt(receipt, target)
            print("real lifecycle refusal/acceptance, pinned SQL identity, and native backup/restore verified:", image)
        finally:
            for identity in reversed(created):
                run(["docker", "rm", "-f", identity], check=False)


if __name__ == "__main__":
    unittest.main()
