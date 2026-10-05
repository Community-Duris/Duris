#!/usr/bin/env python3
"""Qualify SQL lifecycle ownership on an explicitly disposable loopback server.

The caller owns the server lifetime. This runner creates and removes only its
unique fixture database, never reads .env, and executes the same sanitizer
harnesses and schema-drift probes as the Docker runner.
"""

from __future__ import annotations

import json
import os
from pathlib import Path
import re
import secrets
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def main() -> None:
    if os.environ.get("TEST_DB_DISPOSABLE") != "1" or \
            os.environ.get("ECONOMIC_SQL_LIFECYCLE_DISPOSABLE_SERVER") != "1" or \
            os.environ.get("DB_HOST") != "127.0.0.1" or os.environ.get("DB_SOCKET"):
        raise RuntimeError("explicit disposable loopback server authorization is required")
    port = os.environ.get("DB_PORT", "")
    if not port.isdigit() or not 1 <= int(port) <= 65535:
        raise RuntimeError("explicit valid disposable server port is required")
    if not os.environ.get("DB_USER") or not os.environ.get("DB_PASSWD"):
        raise RuntimeError("explicit disposable server credentials are required")
    name = "economic_lifecycle_test_" + secrets.token_hex(8)
    assert re.fullmatch(r"economic_lifecycle_test_[0-9a-f]{16}", name)
    env = dict(os.environ, DB_NAME=name, MYSQL_PWD=os.environ["DB_PASSWD"],
               ECONOMIC_SQL_LIFECYCLE_DISPOSABLE_SCHEMA="1", ENVIRONMENT="test")
    client = ["mysql", "--no-defaults", "--protocol=tcp", "-h127.0.0.1",
              "-P" + port, "-u" + env["DB_USER"], "-N", "-B", "--raw"]

    def sql(statement: str, database: bool = True) -> str:
        result = subprocess.run(client + ([name] if database else []) + ["-e", statement],
                                env=env, capture_output=True, text=True, check=True)
        return result.stdout.strip()

    def apply(path: str) -> None:
        with (ROOT / path).open("rb") as stream:
            subprocess.run(client + [name], stdin=stream, env=env, check=True)

    def verify() -> subprocess.CompletedProcess:
        return subprocess.run(["bash", str(ROOT / "migrations/immutable/0033_economic_sql_lifecycle_owner.sh")],
                              cwd=ROOT, env=env, capture_output=True, text=True)

    def require_schema() -> None:
        result = verify()
        if result.returncode:
            raise RuntimeError(result.stdout + result.stderr)

    version = sql("SELECT VERSION()", False)
    if not (version.startswith("8.0.") and "MariaDB" not in version or
            version.startswith("10.11.") and "MariaDB" in version):
        raise RuntimeError("only disposable MySQL 8.0 or MariaDB 10.11 is qualified")
    print("SQL_LIFECYCLE_LOOPBACK version=" + version, flush=True)
    sql(f"CREATE DATABASE `{name}` CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci", False)
    try:
        apply("migrations/bootstrap_multithread_safe.sql")
        apply("migrations/immutable/0043_shopkeeper_item_condition.sql")
        require_schema()
        sql("DROP TABLE economic_sql_global_activation; DROP TABLE economic_sql_lifecycle_installation")
        apply("migrations/immutable/0033_economic_sql_lifecycle_owner.sql")
        require_schema()
        apply("migrations/economic_sql_lifecycle_owner.sql")
        require_schema()
        for description, damage, repair in (
            ("extra-index", "CREATE INDEX lifecycle_probe_extra ON economic_sql_lifecycle_installation(wallet_count)",
             "DROP INDEX lifecycle_probe_extra ON economic_sql_lifecycle_installation"),
            ("wallet-count-type", "ALTER TABLE economic_sql_lifecycle_installation MODIFY wallet_count BIGINT UNSIGNED NOT NULL",
             "ALTER TABLE economic_sql_lifecycle_installation MODIFY wallet_count INT UNSIGNED NOT NULL"),
        ):
            sql(damage)
            result = verify()
            if not result.returncode or "schema metadata fingerprint mismatch" not in result.stdout + result.stderr:
                raise RuntimeError("schema drift was not specifically rejected: " + description)
            print("SQL_LIFECYCLE_SCHEMA_REJECTED drift=" + description, flush=True)
            sql(repair)
            require_schema()
        apply("migrations/immutable/0040_economic_sql_global_activation.sql")
        apply("migrations/immutable/0040_economic_sql_global_activation.sql")
        subprocess.run(["bash", "migrations/immutable/0040_economic_sql_global_activation.sh"],
                       cwd=ROOT, env=env, check=True)
        # Historical schema replay/drift probes above retain their original cut.
        # Current native witnesses require the complete sealed EAB2 schema head.
        manifest = json.loads((ROOT / "migrations/migration_manifest.json").read_text())
        history = manifest["migrations"]
        if len(history) != 61 or history[-1]["sequence"] != 61 or \
                history[-1]["id"] != "0061_economic_baseline_equipment":
            raise RuntimeError("current lifecycle qualification requires sealed canonical schema61")
        migration_env = dict(env, RUNTIME_COMPATIBILITY_MANIFEST=str(
            ROOT / "migrations/runtime_compatibility_manifest.json"))
        subprocess.run(["python3", "scripts/migration_runner.py", "adopt", "--kind", "fresh_bootstrap"],
                       cwd=ROOT, env=migration_env, check=True)
        subprocess.run(["python3", "scripts/migration_runner.py", "run"],
                       cwd=ROOT, env=migration_env, check=True)
        if sql("SELECT COUNT(*),MAX(sequence_number),SUM(sequence_number=61 AND "
               "migration_id='0061_economic_baseline_equipment') FROM mud_schema_history") != "61\t61\t1":
            raise RuntimeError("current lifecycle qualification did not reach the original schema61 head")
        subprocess.run(["bash", "migrations/verify_runtime_compatibility.sh"],
                       cwd=ROOT, env=migration_env, check=True)
        # Reuse the maintained compile/run section verbatim, including the
        # composed lease-transfer faults and all sanitizer settings.
        runner = (ROOT / "tests/async/run_economic_sql_lifecycle_owner_mysql.sh").read_text()
        marker = 'read -r -a MYSQL_CFLAGS <<< "$(mysql_config --cflags)"'
        if runner.count(marker) != 1:
            raise RuntimeError("Docker runner compile boundary changed; review the native runner")
        with tempfile.TemporaryDirectory(prefix="duris-native-lifecycle-") as temporary:
            env["TEMP"] = temporary
            subprocess.run(["bash", "-c", "set -euo pipefail\n" + runner[runner.index(marker):]],
                           cwd=ROOT, env=env, check=True)
        print("SQL lifecycle owner and owned cutover sanitizer harnesses passed", flush=True)
    finally:
        sql(f"DROP DATABASE `{name}`", False)


if __name__ == "__main__":
    main()
