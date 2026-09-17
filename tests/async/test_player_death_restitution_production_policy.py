#!/usr/bin/env python3
"""Execute the production-policy restitution CLI against a disposable target.

The target is deliberately production-named, but is an isolated Docker database
with no published ports and a synthetic fixture.  The test proves the command
path, not just TargetPolicy helpers: target probe, native backup, target-pinned
plan, no-write refusals, guarded apply, exact verification, and mark-verified.
"""
from __future__ import annotations

import json
import os
from pathlib import Path
import secrets
import subprocess
import tempfile
import time
import unittest


ROOT = Path(__file__).resolve().parents[2]
CLI = ROOT / "scripts" / "player_death_restitution.py"
SCHEMA = ROOT / "tests" / "async" / "player_death_restitution_test_schema.sql"
MIGRATION = ROOT / "migrations" / "immutable" / "0020_player_death_restitution.sql"
FIXTURE_SOURCE = ROOT / "tests" / "async" / "player_death_restitution_fixture.cpp"
CODEC_SOURCE = ROOT / "src" / "player" / "player_snapshot_codec.c"
SEED = ROOT / "tests" / "async" / "player_death_restitution_seed.py"
PASSWORD = "issue331-production-policy-test-only"
DATABASE = "duris_prod_policy_fixture"


@unittest.skipUnless(
    os.environ.get("DURIS_PRODUCTION_POLICY_DB_IMAGE"),
    "set DURIS_PRODUCTION_POLICY_DB_IMAGE to run the disposable production-policy journey",
)
class ProductionPolicyCliJourney(unittest.TestCase):
    """Use only task-owned Docker resources and synthetic SQL rows."""

    def run_command(
        self,
        command: list[str], *, data: str | bytes | None = None,
        env: dict[str, str] | None = None, check: bool = True,
        timeout: int = 180,
    ) -> subprocess.CompletedProcess[str]:
        result = subprocess.run(
            command,
            input=data,
            text=isinstance(data, str) or data is None,
            capture_output=True,
            env=env,
            timeout=timeout,
            check=False,
        )
        if check and result.returncode:
            detail = (result.stderr or result.stdout)[-2000:]
            raise AssertionError(
                "command failed ({}): {}\n{}".format(
                    result.returncode, " ".join(command[:8]), detail
                )
            )
        return result

    def sql(self, wrapper: Path, query: str) -> str:
        result = self.run_command(
            [str(wrapper), "-uroot", "-N", "-B", DATABASE, "-e", query],
            env=self.db_env,
        )
        return result.stdout.strip()

    def cli(
        self, args: list[str], *, check: bool = True,
        environment: dict[str, str] | None = None,
    ) -> subprocess.CompletedProcess[str]:
        return self.run_command(
            ["python3", str(CLI), *args],
            env=environment or self.cli_env,
            check=check,
        )

    def snapshot(self, wrapper: Path) -> str:
        return self.sql(
            wrapper,
            "SELECT CONCAT("
            "(SELECT COUNT(*) FROM player_death_restitution_receipt),'|',"
            "(SELECT COUNT(*) FROM player_death_restitution_delivery),'|',"
            "(SELECT COUNT(*) FROM player_death_restitution_item),'|',"
            "(SELECT COUNT(*) FROM player_items WHERE obj_uid BETWEEN 1000 AND 1004),'|',"
            "(SELECT COUNT(*) FROM player_death_restitution_runtime),'|',"
            "(SELECT COALESCE(MAX(value),-1) FROM production_policy_backup_probe),'|',"
            "(SELECT COALESCE(MAX(status),-1) FROM player_death_restitution_receipt)"
            ")"
        )

    def assert_refused_without_db_write(
        self, wrapper: Path, args: list[str], *, label: str,
        environment: dict[str, str] | None = None,
    ) -> None:
        before = self.snapshot(wrapper)
        result = self.cli(args, check=False, environment=environment)
        self.assertNotEqual(result.returncode, 0, label)
        self.assertEqual(self.snapshot(wrapper), before, label + " changed the database")

    def setUp(self) -> None:
        self.image = os.environ["DURIS_PRODUCTION_POLICY_DB_IMAGE"]
        self.assertIn(self.image, {"mysql:8.0", "mariadb:10.11"})
        token = secrets.token_hex(6)
        self.db_name = "duris-331-prod-policy-db-" + token
        self.runtime_name = "duris-331-prod-policy-runtime-" + token
        self.created: list[str] = []
        self.temp = tempfile.TemporaryDirectory(prefix="duris-331-production-policy-")
        self.base = Path(self.temp.name)
        self.addCleanup(self.cleanup_resources)
        self.wrapper = self.base / "mysql"
        self.dump_wrapper = self.base / "mysqldump"
        self.fixture_bin = self.base / "fixture"
        self.target_info = self.base / "target-info.json"
        self.receipt = self.base / "backup.json"
        self.inspect_artifact = self.base / "inspect.json"
        self.unapproved_plan = self.base / "unapproved.plan.json"
        self.plan = self.base / "approved.plan.json"
        self.proof = self.base / "quiescence.proof"
        self.db_env = os.environ.copy()
        self.db_env.update({"MYSQL_PWD": PASSWORD})
        self.make_database()
        self.cli_env = os.environ.copy()
        self.cli_env.update(
            {
                "ENVIRONMENT": "production",
                "DB_HOST": "127.0.0.1",
                "DB_PORT": "3306",
                "DB_USER": "root",
                "DB_PASSWD": PASSWORD,
                "DB_NAME": DATABASE,
                "MYSQL_BIN": str(self.wrapper),
                "MYSQLDUMP_BIN": str(self.dump_wrapper),
                "PYTHONDONTWRITEBYTECODE": "1",
            }
        )

    def cleanup_resources(self) -> None:
        for identity in reversed(self.created):
            self.run_command(["docker", "rm", "-f", identity], check=False)
        self.temp.cleanup()

    def make_database(self) -> None:
        root_variable = "MARIADB_ROOT_PASSWORD" if self.image.startswith("mariadb:") else "MYSQL_ROOT_PASSWORD"
        database_variable = "MARIADB_DATABASE" if self.image.startswith("mariadb:") else "MYSQL_DATABASE"
        db_id = self.run_command(
            [
                "docker", "create", "--name", self.db_name, "--network", "none",
                "-e", root_variable + "=" + PASSWORD,
                "-e", database_variable + "=" + DATABASE,
                self.image, "--event-scheduler=OFF",
            ]
        ).stdout.strip()
        self.created.append(db_id)
        self.run_command(["docker", "start", db_id])
        self.wrapper.write_text(
            "#!/bin/sh\n"
            "set -eu\n"
            "exec docker exec -e MYSQL_PWD=\"${MYSQL_PWD:-}\" -i "
            + self.db_name
            + " mysql --no-defaults --protocol=TCP -h127.0.0.1 -P3306 \"$@\"\n",
            encoding="utf-8",
        )
        self.wrapper.chmod(0o700)
        self.dump_wrapper.write_text(
            "#!/bin/sh\n"
            "set -eu\n"
            "exec docker exec -e MYSQL_PWD=\"${MYSQL_PWD:-}\" -i "
            + self.db_name
            + " mysqldump --no-defaults \"$@\"\n",
            encoding="utf-8",
        )
        self.dump_wrapper.chmod(0o700)
        deadline = time.monotonic() + 120
        while True:
            ready = self.run_command(
                [str(self.wrapper), "-uroot", "-N", "-B", "-e", "SELECT 1"],
                env={"MYSQL_PWD": PASSWORD},
                check=False,
            )
            if ready.returncode == 0 and ready.stdout.strip() == "1":
                break
            if time.monotonic() >= deadline:
                self.fail("disposable production-named database did not become ready")
            time.sleep(0.5)
        schema_sql = SCHEMA.read_text(encoding="utf-8").replace("duris_issue_331_test", DATABASE)
        self.run_command(
            [str(self.wrapper), "-uroot", DATABASE], data=schema_sql, env=self.db_env
        )
        self.run_command(
            [str(self.wrapper), "-uroot", DATABASE], data=MIGRATION.read_text(), env=self.db_env
        )
        self.run_command(
            ["g++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-Isrc",
             str(FIXTURE_SOURCE), str(CODEC_SOURCE), "-o", str(self.fixture_bin)],
            timeout=180,
        )
        payloads = self.run_command([str(self.fixture_bin)]).stdout.splitlines()
        self.assertEqual(len(payloads), 2)
        seed_sql = self.run_command(["python3", str(SEED), payloads[0], payloads[1]]).stdout
        self.run_command([str(self.wrapper), "-uroot", DATABASE], data=seed_sql, env=self.db_env)
        self.run_command(
            [
                str(self.wrapper), "-uroot", DATABASE, "-e",
                "CREATE TABLE production_policy_backup_probe(value INT NOT NULL) ENGINE=InnoDB;"
                "INSERT INTO production_policy_backup_probe VALUES(17);",
            ], env=self.db_env,
        )
        runtime_id = self.run_command(
            [
                "docker", "create", "--name", self.runtime_name, "--network", "none",
                "--restart=no", "-e", "DB_HOST=127.0.0.1", "-e", "DB_NAME=" + DATABASE,
                "duris-issue-213-tools:latest", "sleep", "600",
            ]
        ).stdout.strip()
        self.created.append(runtime_id)
        self.runtime_id = runtime_id
        self.run_command(["docker", "start", self.runtime_id])
        self.run_command(["docker", "stop", "--time", "1", self.runtime_id])

    def production_flags(self, fingerprint: str) -> list[str]:
        return [
            "--confirm-production-target", DATABASE,
            "--expected-fingerprint", fingerprint,
            "--maintenance-kind", "docker",
            "--maintenance-id", self.runtime_id,
        ]

    def test_production_policy_cli_recovery_journey(self) -> None:
        # target-info is a read-only probe of both SQL identity and the actual
        # stopped runtime boundary.  It is not an authorization artifact alone.
        initial = self.snapshot(self.wrapper)
        target_result = self.cli(
            [
                "target-info", "--artifact", str(self.target_info),
                "--confirm-production-target", DATABASE,
                "--maintenance-kind", "docker", "--maintenance-id", self.runtime_id,
            ]
        )
        self.assertEqual(self.snapshot(self.wrapper), initial)
        target = json.loads(self.target_info.read_text(encoding="utf-8"))
        self.assertTrue(target["target"]["production"])
        self.assertEqual(target["target"]["database"], DATABASE)
        self.assertEqual(target["maintenance_boundary"]["id"], self.runtime_id)
        fingerprint = target["target"]["server_fingerprint"]
        self.assertIn(fingerprint, target_result.stdout)

        # The backup subcommand calls the real check_quiescence callback and
        # native mysqldump, while retaining an exact target/boundary receipt.
        backup_args = [
            "backup", "--target-info", str(self.target_info),
            "--receipt", str(self.receipt), "--offline-proof", str(self.proof),
            *self.production_flags(fingerprint),
        ]
        self.proof.write_text(
            "format=duris-death-restitution-quiescence-v3\n"
            "database=" + DATABASE + "\n"
            "boundary=mysql-advisory-exclusion\n"
            "guard=duris.player.death.restitution\n"
            "expires_at=2099-01-01T00:00:00Z\n",
            encoding="utf-8",
        )
        self.proof.chmod(0o600)
        self.cli(backup_args)
        receipt = json.loads(self.receipt.read_text(encoding="utf-8"))
        self.assertEqual(receipt["target"], target["target"])
        self.assertEqual(receipt["maintenance_boundary"], target["maintenance_boundary"])
        dump_path = Path(receipt["dump_path"])
        self.assertTrue(dump_path.is_file())
        self.assertEqual(self.snapshot(self.wrapper), initial)

        # Prove the generated native dump is usable, then restore the same
        # synthetic fixture before planning.  No live or repository DB is used.
        dump_bytes = dump_path.read_bytes()
        self.run_command(
            [str(self.wrapper), "-uroot", DATABASE, "-e",
             "UPDATE production_policy_backup_probe SET value=99;"],
            env=self.db_env,
        )
        self.run_command(
            [str(self.wrapper), "-uroot", DATABASE],
            data=dump_bytes.decode("utf-8"), env=self.db_env,
        )
        self.assertEqual(self.sql(self.wrapper, "SELECT value FROM production_policy_backup_probe"), "17")

        self.cli(
            [
                "inspect", "--target-info", str(self.target_info), "--artifact", str(self.inspect_artifact),
                "--pid", "42", "--death-revision", "7", "--recipient-pid", "42",
                "--confirm-production-target", DATABASE, "--server-fingerprint", fingerprint,
            ]
        )
        common_plan = [
            "plan", "--inspect", str(self.inspect_artifact), "--target-info", str(self.target_info),
            "--backup-receipt", str(self.receipt),
        ]
        self.cli([*common_plan, "--artifact", str(self.unapproved_plan)])
        unapproved = json.loads(self.unapproved_plan.read_text(encoding="utf-8"))
        self.assertFalse(unapproved["applyable"])
        self.assertFalse(unapproved["production_approved"])
        apply_base = [
            "apply", "--plan", str(self.unapproved_plan), "--offline-proof", str(self.proof),
            "--approve", "--actor", "production-policy-test", "--reason", "unapproved-plan-fence",
            "--target-info", str(self.target_info), *self.production_flags(fingerprint),
        ]
        self.assert_refused_without_db_write(self.wrapper, apply_base, label="unapproved plan")

        self.cli([*common_plan, "--approve-production", "--artifact", str(self.plan)])
        approved = json.loads(self.plan.read_text(encoding="utf-8"))
        self.assertTrue(approved["applyable"])
        self.assertTrue(approved["production_approved"])
        approved_apply = [
            "apply", "--plan", str(self.plan), "--offline-proof", str(self.proof),
            "--approve", "--actor", "production-policy-test", "--reason", "synthetic-production-recovery",
            "--target-info", str(self.target_info), *self.production_flags(fingerprint),
        ]

        # Every negative case is checked against the complete authority-count
        # snapshot, not merely the command's exit code.
        self.assert_refused_without_db_write(
            self.wrapper,
            [*approved_apply[:-len(self.production_flags(fingerprint))],
             "--confirm-production-target", DATABASE, "--server-fingerprint", "0" * 64,
             "--maintenance-kind", "docker", "--maintenance-id", self.runtime_id],
            label="wrong server fingerprint",
        )
        self.assert_refused_without_db_write(
            self.wrapper,
            [*approved_apply[:-len(self.production_flags(fingerprint))],
             "--confirm-production-target", DATABASE + "-wrong",
             "--expected-fingerprint", fingerprint,
             "--maintenance-kind", "docker", "--maintenance-id", self.runtime_id],
            label="wrong exact target",
        )
        dump_path.write_bytes(dump_bytes + b"\n-- changed after approval\n")
        self.assert_refused_without_db_write(self.wrapper, approved_apply, label="changed backup")
        dump_path.write_bytes(dump_bytes)

        self.run_command(["docker", "start", self.runtime_id])
        self.assert_refused_without_db_write(self.wrapper, approved_apply, label="live runtime")
        self.run_command(["docker", "stop", "--time", "1", self.runtime_id])

        self.cli(approved_apply)
        applied_snapshot = self.snapshot(self.wrapper)
        self.assertEqual(applied_snapshot.split("|"), ["1", "5", "7", "5", "5", "17", "2"])

        verify_flags = [
            "--target-info", str(self.target_info), *self.production_flags(fingerprint),
        ]
        self.cli(["verify", "--plan", str(self.plan), *verify_flags])
        self.cli([
            "verify", "--plan", str(self.plan), "--mark-verified", "--approve",
            "--offline-proof", str(self.proof), *verify_flags,
        ])
        self.assertEqual(
            self.sql(self.wrapper, "SELECT status FROM player_death_restitution_receipt"), "3"
        )
        replay_before = self.snapshot(self.wrapper)
        replay = self.cli(approved_apply)
        self.assertIn("already applied", replay.stdout)
        self.assertEqual(self.snapshot(self.wrapper), replay_before)
        print("production-policy CLI target/backup/apply/readback journey verified")


if __name__ == "__main__":
    unittest.main()
