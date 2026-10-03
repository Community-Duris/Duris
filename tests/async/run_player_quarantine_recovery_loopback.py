#!/usr/bin/env python3
"""Qualify the native stopped recovery owner on an explicitly disposable SQL server.

Reuses the maintained item-transfer harness and immutable migration runner.
Never reads checkout configuration; creates and removes only its own schema.
"""
import os
from pathlib import Path
import secrets
import subprocess

ROOT = Path(__file__).resolve().parents[2]


def main():
    if (os.environ.get("TEST_DB_DISPOSABLE") != "1" or
            os.environ.get("PLAYER_QUARANTINE_RECOVERY_DISPOSABLE_SERVER") != "1" or
            os.environ.get("DB_HOST") != "127.0.0.1" or os.environ.get("DB_SOCKET")):
        raise RuntimeError("explicit disposable loopback server authorization is required")
    port = os.environ.get("DB_PORT", "")
    if not port.isdigit() or not 1 <= int(port) <= 65535:
        raise RuntimeError("explicit disposable server port is required")
    if not os.environ.get("DB_USER") or not os.environ.get("DB_PASSWD"):
        raise RuntimeError("explicit disposable server credentials are required")
    name = "economic_schema_test_recovery_" + secrets.token_hex(8)
    env = dict(os.environ, DB_NAME=name, MYSQL_PWD=os.environ["DB_PASSWD"],
               DB_ALLOWED_TARGETS="127.0.0.1/" + name, DB_TLS="FALSE", ENVIRONMENT="test",
               PLAYER_QUARANTINE_RECOVERY_TEST="1")
    client = ["mysql", "--no-defaults", "--protocol=tcp", "-h127.0.0.1",
              "-P" + port, "-u" + env["DB_USER"], "-N", "-B", "--raw"]

    def sql(statement, selected=True):
        return subprocess.check_output(client + ([name] if selected else []),
                                       input=statement, env=env, text=True).strip()

    version = sql("SELECT VERSION()", False)
    if not (version.startswith("8.0.") and "MariaDB" not in version or
            version.startswith("10.11.") and "MariaDB" in version):
        raise RuntimeError("only disposable MySQL 8.0 or MariaDB 10.11 is qualified")
    print("PLAYER_RECOVERY_LOOPBACK version=" + version, flush=True)
    sql("CREATE DATABASE " + name, False)
    try:
        sql((ROOT / "migrations/bootstrap_multithread_safe.sql").read_text())
        for arguments in (("adopt", "--kind", "fresh_bootstrap"), ("run",), ("run",)):
            subprocess.run(["python3", "scripts/migration_runner.py", *arguments],
                           cwd=ROOT, env=env, check=True)
        subprocess.run(["bash", "tests/async/run_item_transfer_schema_mysql.sh"],
                       cwd=ROOT, env=env, check=True)
    finally:
        sql("DROP DATABASE " + name, False)


if __name__ == "__main__":
    main()
