#!/usr/bin/env python3

import os
import pathlib
import shutil
import subprocess
import tempfile


ROOT = pathlib.Path(__file__).resolve().parents[2]
SOURCE = ROOT / "scripts/cycle_mud.sh"


def run(script: pathlib.Path, env: dict[str, str], *arguments: str) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        ["bash", str(script), *arguments],
        env=env,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        timeout=20,
    )


subprocess.run(["bash", "-n", str(SOURCE)], check=True)

with tempfile.TemporaryDirectory(prefix="duris-flatfile-launcher-") as temporary:
    project = pathlib.Path(temporary)
    scripts = project / "scripts"
    scripts.mkdir()
    script = scripts / "cycle_mud.sh"
    shutil.copy2(SOURCE, script)
    shutil.copy2(ROOT / "scripts/backup_pfiles.sh", scripts / "backup_pfiles.sh")

    flat_env = {
        "PATH": os.environ.get("PATH", "/usr/bin:/bin"),
        "ENVIRONMENT": "local",
        "PERSISTENCE_MODE": "flatfile-primary",
        "FLATFILE_STATE_DIR": str(project / "state"),
        "FLATFILE_BACKUP_DIR": str(project / "backups"),
        "REDIS": "1",
    }
    checked = run(script, flat_env, "--check-config")
    if checked.returncode != 0 or "database-independent configuration" not in checked.stdout:
        raise AssertionError("flat-file config check required a database:\n" + checked.stdout)

    missing_root = dict(flat_env)
    del missing_root["FLATFILE_STATE_DIR"]
    rejected = run(script, missing_root, "--check-config")
    if rejected.returncode == 0 or "FLATFILE_STATE_DIR is required" not in rejected.stdout:
        raise AssertionError("flat-file config accepted a missing state root")

    db_env = {
        "PATH": flat_env["PATH"],
        "ENVIRONMENT": "local",
        "PERSISTENCE_MODE": "mariadb-primary",
    }
    rejected = run(script, db_env, "--check-config")
    if rejected.returncode == 0 or "Missing required database field: DB_HOST" not in rejected.stdout:
        raise AssertionError("MariaDB config stopped requiring database credentials")

    valid_db_env = dict(db_env)
    valid_db_env.update(
        {
            "DB_HOST": "127.0.0.1",
            "DB_USER": "test-user",
            "DB_PASSWD": "test-password",
            "DB_NAME": "duris_dev",
            "DB_ALLOWED_TARGETS": "127.0.0.1/duris_dev",
        }
    )
    checked = run(script, valid_db_env, "--check-config")
    if checked.returncode != 0 or "explicit database configuration" not in checked.stdout:
        raise AssertionError("valid MariaDB config was rejected:\n" + checked.stdout)

    fallback_env = dict(flat_env)
    fallback_env["PERSISTENCE_MODE"] = "mariadb-primary-flatfile-fallback"
    rejected = run(script, fallback_env, "--check-config")
    if rejected.returncode == 0 or "Missing required database field: DB_HOST" not in rejected.stdout:
        raise AssertionError("fallback config stopped requiring database credentials")

    invalid_env = dict(flat_env)
    invalid_env["PERSISTENCE_MODE"] = "unknown"
    rejected = run(script, invalid_env, "--check-config")
    if rejected.returncode == 0 or "Invalid PERSISTENCE_MODE" not in rejected.stdout:
        raise AssertionError("invalid persistence mode was accepted")

    (project / "areas_mini").mkdir()
    for name in (
        "mini.mob",
        "mini.obj",
        "mini.qst",
        "mini.wld",
        "mini.zon",
        "world.shp",
        "world.tab",
        "world.weather",
    ):
        (project / "areas_mini" / name).write_text("test\n")
    (project / "bin/server").mkdir(parents=True, exist_ok=True)
    (project / "lib/misc").mkdir(parents=True)
    (project / "logs").mkdir()
    state_record = project / "state/metadata/timer.test"
    state_record.parent.mkdir(parents=True)
    (project / "state").chmod(0o700)
    state_record.write_text("durable state\n")
    state_record.chmod(0o600)
    server = project / "bin/server/dms_new"
    server.write_text("#!/bin/sh\nexit 0\n")
    server.chmod(0o755)

    nested_backup_env = dict(flat_env)
    nested_backup_env["FLATFILE_BACKUP_DIR"] = str(project / "state/backups")
    rejected = run(script, nested_backup_env, "--minimal")
    if rejected.returncode == 0 or "refusing to boot" not in rejected.stdout:
        raise AssertionError("flat-file launcher ignored an unsafe backup target:\n" + rejected.stdout)

    launched = run(script, flat_env, "--minimal")
    if launched.returncode != 0 or "Mud stopped, reason: shutdown [0]" not in launched.stdout:
        raise AssertionError("flat-file launcher did not complete without DB tools:\n" + launched.stdout)
    forbidden = (
        "database migrations",
        "runtime database compatibility",
        "Logged reboot:",
        "DB mode enabled",
        "mysqldump",
    )
    if any(message in launched.stdout for message in forbidden):
        raise AssertionError("flat-file launcher entered a database-only path:\n" + launched.stdout)
    backups = list((project / "backups").glob("*/metadata/timer.test"))
    if len(backups) != 1 or backups[0].read_text() != "durable state\n":
        raise AssertionError("flat-file launcher did not back up its selected state root")
    if backups[0].stat().st_mode & 0o077 or backups[0].parents[1].stat().st_mode & 0o077:
        raise AssertionError("flat-file launcher created a non-private backup")

print("flat-file launcher regression passed")
