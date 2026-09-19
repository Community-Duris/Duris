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
import socket
import shlex
import subprocess
import sys
import tempfile
import time
import unittest
from unittest import mock

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

    def test_user_systemd_boundary_binds_owner_manager_unit_and_cgroup(self):
        uid = os.getuid()
        db_name = "duris_user_manager_fixture"
        policy_env = self.env(ENVIRONMENT="production", DB_NAME=db_name)
        with tempfile.TemporaryDirectory(prefix="duris-504-user-systemd-") as directory:
            root = Path(directory)
            runtime_root = root / "run" / "user"
            runtime = runtime_root / str(uid)
            (runtime / "systemd").mkdir(parents=True, mode=0o700)
            runtime.chmod(0o700)
            private = runtime / "systemd" / "private"
            manager_socket = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
            manager_socket.bind(str(private))

            cgroup_root = root / "cgroup"
            cgroup_root.mkdir()
            (cgroup_root / "cgroup.controllers").write_text("cpu\n")
            manager_group = cgroup_root / "user.slice" / f"user-{uid}.slice" / f"user@{uid}.service"
            unit_group = manager_group / "app.slice" / "duris-mud-production.service"
            unit_group.mkdir(parents=True)
            (manager_group / "cgroup.procs").write_text("123\n")
            (unit_group / "cgroup.procs").write_text("")

            proc_root = root / "proc"
            (proc_root / "123").mkdir(parents=True)
            (proc_root / "123" / "status").write_text(
                f"Name:\tsystemd\nUid:\t{uid}\t{uid}\t{uid}\t{uid}\n"
            )
            (proc_root / "123" / "comm").write_text("systemd\n")
            (proc_root / "123" / "cmdline").write_bytes(b"/usr/lib/systemd/systemd\x00--user\x00")
            manager_output = "SystemState=running\n"
            unit_output = "\n".join([
                "Id=duris-mud-production.service", "LoadState=loaded",
                "ActiveState=inactive", "SubState=dead", "MainPID=0", "ControlPID=0",
                f"UnitFileState=masked", f"ControlGroup=/user.slice/user-{uid}.slice/user@{uid}.service/app.slice/duris-mud-production.service",
            ]) + "\n"

            def systemctl(command, **_kwargs):
                output = unit_output if "duris-mud-production.service" in command else manager_output
                return subprocess.CompletedProcess(command, 0, output, "")

            with mock.patch.object(target_module, "SYSTEMD_CGROUP_ROOT", cgroup_root), \
                    mock.patch.object(target_module, "SYSTEMD_RUNTIME_ROOT", runtime_root), \
                    mock.patch.object(target_module, "SYSTEMD_PROC_ROOT", proc_root), \
                    mock.patch.dict(os.environ, {"XDG_RUNTIME_DIR": str(runtime)}, clear=False), \
                    mock.patch.object(os, "getuid", return_value=uid), \
                    mock.patch.object(subprocess, "run", side_effect=systemctl):
                policy = TargetPolicy.from_environment(
                    policy_env, confirm_production_target=db_name,
                    maintenance_kind="systemd-user",
                    maintenance_id="duris-mud-production.service",
                    maintenance_owner=str(uid),
                )
                boundary = policy.require_maintenance()
                self.assertEqual(boundary["kind"], "systemd-user")
                self.assertEqual(boundary["owner_uid"], str(uid))
                self.assertEqual(boundary["manager_id"], f"user@{uid}.service")
                self.assertEqual(boundary["main_pid"], "0")
                self.assertEqual(target_module.validate_maintenance_record(boundary), boundary)
                stale = dict(boundary)
                stale["manager_pid"] = "124"
                self.assertNotEqual(stale, boundary)

                # A masked user unit can have no retained cgroup at all.  The
                # empty ControlGroup is the observed state; do not synthesize
                # a path merely to make the old cgroup check pass.
                (unit_group / "cgroup.procs").unlink()
                unit_group.rmdir()
                unit_group.parent.rmdir()
                unit_output = "\n".join([
                    "Id=duris-mud-production.service", "LoadState=masked",
                    "ActiveState=inactive", "SubState=dead", "MainPID=0", "ControlPID=0",
                    "UnitFileState=masked", "ControlGroup=",
                ]) + "\n"
                boundary = policy.require_maintenance()
                self.assertEqual(boundary["control_group"], "")
                self.assertEqual(boundary["cgroup_processes"], "0")
                self.assertEqual(target_module.validate_maintenance_record(boundary), boundary)

                # Do not turn the absent-group compatibility state into a
                # generic loaded-unit bypass.
                unit_output = unit_output.replace("LoadState=masked", "LoadState=loaded")
                with self.assertRaisesRegex(TargetError, "no verifiable cgroup"):
                    policy.require_maintenance()
            manager_socket.close()

    def test_systemd_boundary_keeps_system_scope_and_cgroup_gate(self):
        policy = TargetPolicy.from_environment(
            self.env(), maintenance_kind="systemd", maintenance_id="duris-mud-production.service"
        )
        properties = "\n".join([
            "Id=duris-mud-production.service", "LoadState=masked", "ActiveState=inactive",
            "SubState=dead", "MainPID=0", "ControlPID=0", "UnitFileState=masked",
            "ControlGroup=/system.slice/duris-mud-production.service",
        ]) + "\n"
        with tempfile.TemporaryDirectory(prefix="duris-504-systemd-") as directory:
            cgroup_root = Path(directory) / "cgroup"
            group = cgroup_root / "system.slice" / "duris-mud-production.service"
            group.mkdir(parents=True)
            (cgroup_root / "cgroup.controllers").write_text("cpu\n")
            (group / "cgroup.procs").write_text("")

            def systemctl(command, **_kwargs):
                return subprocess.CompletedProcess(command, 0, properties, "")

            with mock.patch.object(target_module, "SYSTEMD_CGROUP_ROOT", cgroup_root), \
                    mock.patch.object(subprocess, "run", side_effect=systemctl) as run:
                self.assertEqual(
                    policy.require_maintenance(),
                    {"kind": "systemd", "id": "duris-mud-production.service", "state": "masked-inactive"},
                )
                self.assertIn("--system", run.call_args.args[0])

            (group / "cgroup.procs").write_text("987\n")
            with mock.patch.object(target_module, "SYSTEMD_CGROUP_ROOT", cgroup_root), \
                    mock.patch.object(subprocess, "run", side_effect=systemctl), \
                    self.assertRaisesRegex(TargetError, "control group"):
                policy.require_maintenance()

            (group / "cgroup.procs").write_text("")
            (cgroup_root / "cgroup.controllers").unlink()
            with mock.patch.object(target_module, "SYSTEMD_CGROUP_ROOT", cgroup_root), \
                    mock.patch.object(subprocess, "run", side_effect=systemctl), \
                    self.assertRaisesRegex(TargetError, "visibility"):
                policy.require_maintenance()

        live = properties.replace("ActiveState=inactive", "ActiveState=active").replace(
            "SubState=dead", "SubState=running").replace("MainPID=0", "MainPID=987", 1)
        with mock.patch.object(subprocess, "run", return_value=subprocess.CompletedProcess(
                ["systemctl"], 0, live, "")), self.assertRaisesRegex(TargetError, "masked"):
            policy.require_maintenance()

    def test_user_systemd_boundary_refuses_owner_state_and_visibility_bypasses(self):
        uid = os.getuid()
        db_name = "duris_user_manager_fixture"
        env = self.env(ENVIRONMENT="production", DB_NAME=db_name)
        with self.assertRaises(TargetError):
            TargetPolicy.from_environment(
                env, confirm_production_target=db_name,
                maintenance_kind="systemd-user", maintenance_id="duris-mud-production.service",
            )
        policy = TargetPolicy.from_environment(
            env, confirm_production_target=db_name,
            maintenance_kind="systemd-user", maintenance_id="duris-mud-production.service",
            maintenance_owner=str(uid),
        )
        with mock.patch.object(os, "getuid", return_value=uid + 1), \
                self.assertRaisesRegex(TargetError, "owner"):
            policy.require_maintenance()

        with self.assertRaises(TargetError):
            replace(policy, maintenance_id="duris-mud.service").require_maintenance()

        manager = {"SystemState": "running"}
        unit = {
            "Id": "duris-mud-production.service", "LoadState": "loaded",
            "ActiveState": "active", "SubState": "running", "MainPID": "456",
            "ControlPID": "0", "UnitFileState": "masked",
            "ControlGroup": f"/user.slice/user-{uid}.slice/user@{uid}.service/app.slice/duris-mud-production.service",
        }
        with mock.patch.object(os, "getuid", return_value=uid), \
                mock.patch.object(TargetPolicy, "_user_runtime", return_value=(Path(f"/run/user/{uid}"), Path(f"/run/user/{uid}/systemd/private"))), \
                mock.patch.object(TargetPolicy, "_systemd_properties", side_effect=[manager, unit]), \
                mock.patch.object(TargetPolicy, "_user_manager_pid", return_value=123), \
                mock.patch.object(TargetPolicy, "_cgroup_pids", return_value={123}), \
                self.assertRaisesRegex(TargetError, "stopped"):
            policy.require_maintenance()

        bad_manager = {"SystemState": "degraded"}
        with mock.patch.object(os, "getuid", return_value=uid), \
                mock.patch.object(TargetPolicy, "_user_runtime", return_value=(Path(f"/run/user/{uid}"), Path(f"/run/user/{uid}/systemd/private"))), \
                mock.patch.object(TargetPolicy, "_systemd_properties", side_effect=[bad_manager, unit]), \
                self.assertRaisesRegex(TargetError, "manager"):
            policy.require_maintenance()

        unit["ActiveState"] = "inactive"
        unit["SubState"] = "dead"
        unit["MainPID"] = "0"
        with mock.patch.object(os, "getuid", return_value=uid), \
                mock.patch.object(TargetPolicy, "_user_runtime", return_value=(Path(f"/run/user/{uid}"), Path(f"/run/user/{uid}/systemd/private"))), \
                mock.patch.object(TargetPolicy, "_systemd_properties", side_effect=[manager, unit]), \
                mock.patch.object(TargetPolicy, "_user_manager_pid", return_value=123), \
                mock.patch.object(TargetPolicy, "_cgroup_pids", side_effect=[{123}, {456}]), \
                self.assertRaisesRegex(TargetError, "not empty"):
            policy.require_maintenance()

        with mock.patch.object(os, "getuid", return_value=uid), \
                mock.patch.object(TargetPolicy, "_user_runtime", return_value=(Path(f"/run/user/{uid}"), Path(f"/run/user/{uid}/systemd/private"))), \
                mock.patch.object(TargetPolicy, "_systemd_properties", side_effect=[manager, unit]), \
                mock.patch.object(TargetPolicy, "_user_manager_pid", return_value=123), \
                mock.patch.object(TargetPolicy, "_cgroup_pids", side_effect=TargetError("process visibility is unavailable")), \
                self.assertRaisesRegex(TargetError, "visibility"):
            policy.require_maintenance()

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
