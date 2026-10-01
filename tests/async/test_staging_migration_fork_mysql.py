#!/usr/bin/env python3
"""Qualify append-only staging/master forks on task-owned MySQL/MariaDB containers.

Uses the real migration runner and both the shell and compiled boot predicates.
Never targets a configured server. --update-contract explicitly seals measured
engine metadata after canonical/fork parity has been established.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import secrets
import subprocess
import sys
import time
from dataclasses import replace
from pathlib import Path
from unittest import mock

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))
import migration_runner as runner
import telemetry_rollup_schema_mysql as schema


def check(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def qa(engine: schema.Engine, command: list[str]) -> subprocess.CompletedProcess:
    environment = dict(os.environ)
    environment.update({"ENVIRONMENT": "test", "DB_HOST": "127.0.0.1", "DB_PORT": "3306",
                        "DB_USER": "root", "DB_PASSWD": engine.password,
                        "DB_NAME": engine.database, "TEST_DB_DISPOSABLE": "1"})
    args = ["docker", "run", "--rm", "--network", f"container:{engine.container}",
            "--mount", f"type=bind,source={ROOT},target=/opt/duris", "-w", "/opt/duris"]
    for key in ("ENVIRONMENT", "DB_HOST", "DB_PORT", "DB_USER", "DB_PASSWD",
                "DB_NAME", "TEST_DB_DISPOSABLE"):
        args.extend(("-e", key))
    result = subprocess.run([*args, "duris-finish-accounting-qa:local", *command],
                            env=environment, capture_output=True, text=True)
    result.stdout = schema.redact(result.stdout)
    result.stderr = schema.redact(result.stderr)
    return result


def migrate(engine: schema.Engine, manifest_name: str, success: bool = True) -> str:
    result = qa(engine, ["python3", "scripts/migration_runner.py", "--manifest",
                         "migrations/" + manifest_name, "run"])
    check((result.returncode == 0) == success,
          f"migration outcome mismatch: {result.stdout} {result.stderr}")
    return result.stdout + result.stderr


def history(engine: schema.Engine, limit: int = 51) -> str:
    return engine.sql("SELECT HEX(CONCAT(migration_id,CHAR(0),sequence_number,CHAR(0),"
                      "description,CHAR(0),HEX(apply_checksum),CHAR(0),HEX(verify_checksum),"
                      "CHAR(0),compatibility,CHAR(0),runner_version,CHAR(0),applied_at)) "
                      f"FROM mud_schema_history ORDER BY sequence_number LIMIT {limit};",
                      database=engine.database)


def boot(engine: schema.Engine, success: bool = True) -> str:
    results = []
    for command in (["bash", "migrations/verify_runtime_compatibility.sh", "--schema-only"],
                    ["python3", "tests/async/runtime_migration_history_fixture.py"]):
        result = qa(engine, command)
        check((result.returncode == 0) == success,
              f"runtime outcome mismatch: {command}: {result.stdout} {result.stderr}")
        results.append((result.stdout + result.stderr).strip())
    return " | ".join(results)


def setup(engine: schema.Engine, manifest: runner.Manifest,
          bootstrap: Path | None = None) -> None:
    engine.wait_ready()
    engine.create_database()
    engine.sql_file(bootstrap or schema.BOOTSTRAP)
    schema.seed_history(engine, replace(manifest, migrations=()))


def descriptions(engine: schema.Engine) -> str:
    values = ["A" * 255 + "one", "A" * 255 + "two", "Case", "case", "café", "cafe"]
    # These synthetic child rows exercise index equivalence only. Own test DB;
    # no fixture writes are made to any actual player or item authority.
    for table in ("player_item_extra_descr", "player_pet_item_extra_descr"):
        rows = ",".join("(4294900001,'probe',CONVERT(UNHEX('" + text.encode().hex() +
                        "') USING utf8mb4))" for text in values)
        engine.sql(f"SET FOREIGN_KEY_CHECKS=0; INSERT INTO {table}"
                   f"(item_id,keyword,description) VALUES {rows};", database=engine.database)
    return description_rows(engine)


def description_rows(engine: schema.Engine) -> str:
    return engine.sql("SELECT 'player',id,item_id,HEX(keyword),HEX(description),"
                      "HEX(description_sha256) FROM player_item_extra_descr UNION ALL "
                      "SELECT 'pet',id,item_id,HEX(keyword),HEX(description),"
                      "HEX(description_sha256) FROM player_pet_item_extra_descr ORDER BY 1,2;",
                      database=engine.database)


def duplicate_guard(engine: schema.Engine, step: runner.Migration) -> None:
    engine.create_database()
    engine.sql("CREATE TABLE player_item_extra_descr (id INT PRIMARY KEY, item_id INT, "
               "keyword VARCHAR(255), description TEXT) ENGINE=InnoDB; "
               "CREATE TABLE player_pet_item_extra_descr LIKE player_item_extra_descr; "
               "INSERT INTO player_item_extra_descr VALUES (1,3,'probe','same'),"
               "(2,3,'probe','same');", database=engine.database)
    before = engine.sql("SELECT * FROM player_item_extra_descr ORDER BY id;",
                        database=engine.database)
    result = schema.run_process(schema.mysql_args(engine, engine.database, None),
                                input_bytes=step.apply_path.read_bytes(), check=False)
    check(result.returncode != 0, "duplicate complete values were accepted")
    check(before == engine.sql("SELECT * FROM player_item_extra_descr ORDER BY id;",
                               database=engine.database), "duplicate guard deleted evidence")
    check(engine.sql("SELECT COUNT(*) FROM information_schema.columns WHERE "
                     "table_schema=DATABASE() AND column_name='description_sha256';",
                     database=engine.database) == "0", "duplicate refusal changed permanent DDL")


def native_lock_fixture() -> dict:
    """Run the actual transport on an explicitly disposable, empty ledger DB."""
    check(os.environ.get("ENVIRONMENT") == "test" and
          os.environ.get("TEST_DB_DISPOSABLE") == "1" and
          os.environ.get("DB_HOST") == "127.0.0.1" and
          re.fullmatch(r"duris_268_[a-z0-9]+_locktest", os.environ.get("DB_NAME", "")),
          "lock fixture requires an isolated disposable DB")
    manifest = runner.load_manifest()
    clients = []

    def client():
        value = runner.MysqlExecutor(manifest)
        clients.append(value)
        return value

    def refuses(action, expected):
        try:
            action()
        except runner.MigrationContractError as error:
            check(expected in str(error), f"unexpected refusal: {error}")
        else:
            raise AssertionError("unsafe SQL operation was accepted")

    try:
        owner = client()
        owner.acquire_lock()
        identity = owner.sql("SELECT CONNECTION_ID();")
        owner.require_quiescent()
        owner.sql("CREATE TEMPORARY TABLE migration_lock_session_probe(value INT); "
                  "INSERT INTO migration_lock_session_probe VALUES(7);")
        check(owner.sql("SELECT value FROM migration_lock_session_probe;") == "7",
              "SQL commands did not retain one session")
        observer = client()
        check(observer.sql("SELECT IS_USED_LOCK('duris_immutable_migration');") == identity,
              "advisory lock vanished between SQL commands")
        refuses(owner.require_quiescent, "every other database connection")
        competitor = client()
        refuses(competitor.acquire_lock, "lock unavailable")
        check(competitor._client is None and competitor._session_closed,
              "failed lock attempt leaked a client")
        observer.release_lock()
        check(owner.sql("SELECT CONNECTION_ID();") == identity, "lock owner reconnected")
        owner.require_quiescent()
        step = manifest.migrations[0]
        owner.apply(step)
        owner.verify(step)
        owner.require_quiescent()
        check(owner.sql("SELECT IS_USED_LOCK('duris_immutable_migration');") == identity,
              "lock was lost across apply/verifier boundaries")

        # Change the stored head after record() reads it, using an independent
        # connection. The failed CAS must not commit its inserted history row.
        corruptor = client()
        original_sql = owner.sql

        def raced_sql(statement, input_payload=None):
            if statement.startswith("START TRANSACTION;"):
                corruptor.sql("UPDATE mud_schema_migration_state SET applied_count=42 "
                              "WHERE state_id=1;")
            return original_sql(statement, input_payload)

        with mock.patch.object(owner, "sql", side_effect=raced_sql):
            refuses(lambda: owner.record(step, manifest.runner_version), "head changed")
        check(corruptor.sql("SELECT COUNT(*) FROM mud_schema_history;") == "0",
              "failed state CAS committed an orphan history receipt")
        corruptor.sql("UPDATE mud_schema_migration_state SET applied_count=0 WHERE state_id=1;")
        corruptor.release_lock()
        owner.record(step, manifest.runner_version)
        check(len(owner.applied()) == 1, "successful receipt/state transaction did not commit")

        # Kill the actual lock-owning server connection during a transaction.
        # Reconnecting must not commit it or run a later statement.
        owner.sql("CREATE TABLE migration_lock_mutation_probe(value INT) ENGINE=InnoDB;")
        owner.sql("START TRANSACTION; INSERT INTO migration_lock_mutation_probe VALUES(1);")
        observer = client()
        observer.sql("KILL CONNECTION " + identity + ";")
        refuses(lambda: owner.sql("COMMIT; INSERT INTO migration_lock_mutation_probe VALUES(2);"),
                "session")
        check(observer.sql("SELECT COUNT(*) FROM migration_lock_mutation_probe;") == "0",
              "session loss committed transaction or ran the following mutation")
        refuses(lambda: owner.sql("INSERT INTO migration_lock_mutation_probe VALUES(3);"),
                "session is closed")
        check(owner._client is None, "lost session restarted a SQL client")
        replacement = client()
        replacement.acquire_lock()
        replacement.release_lock()

        bad_sql = client()
        bad_sql.acquire_lock()
        refuses(lambda: bad_sql.sql("INSERT INTO missing_migration_lock_probe VALUES(1);"),
                "command failed")
        refuses(lambda: bad_sql.sql("SELECT 1;"), "session is closed")

        bounded = client()
        bounded.acquire_lock()
        refuses(lambda: bounded.sql("x" * (runner.MAX_MIGRATION_BYTES + 1)),
                "request exceeds limit")
        check(bounded.sql("SELECT 1;") == "1", "oversized admission lost a valid session")
        refuses(lambda: bounded.sql(f"SELECT REPEAT('x',{runner.MAX_MIGRATION_BYTES + 1});"),
                "result exceeds limit")
        refuses(lambda: bounded.sql("SELECT 1;"), "session is closed")

        timed = client()
        timed.acquire_lock()
        start = time.monotonic()
        with mock.patch.object(runner, "MYSQL_COMMAND_TIMEOUT_SECONDS", 0.15):
            refuses(lambda: timed.sql("SELECT SLEEP(2);"), "timed out")
        check(time.monotonic() - start < 5, "SQL timeout did not bound the client")
        refuses(lambda: timed.sql("SELECT 1;"), "session is closed")
        replacement = client()
        replacement.acquire_lock()
        replacement.release_lock()
        for interruption in (KeyboardInterrupt, MemoryError):
            interrupted = client()
            interrupted.acquire_lock()
            interrupted.sql("START TRANSACTION; INSERT INTO migration_lock_mutation_probe VALUES(4);")
            with mock.patch.object(runner.selectors, "DefaultSelector") as selector:
                selector.return_value.__enter__.return_value.select.side_effect = interruption()
                try:
                    interrupted.sql("COMMIT;")
                except (KeyboardInterrupt, runner.MigrationContractError):
                    pass
                else:
                    raise AssertionError("interrupted transport was accepted")
            refuses(lambda: interrupted.sql("SELECT 1;"), "session is closed")
            check(observer.sql("SELECT COUNT(*) FROM migration_lock_mutation_probe;") == "0",
                  "transport interruption retained a transaction")
        check(observer.sql("SELECT COUNT(*) FROM migration_lock_mutation_probe;") == "0",
              "fault paths changed durable test state")
        check(observer.sql("SELECT COUNT(*) FROM mud_schema_history;") == "1",
              "fault paths changed the committed receipt")
        return {"session_and_lock_retained_across_apply_verify_record": True,
                "concurrent_runner_excluded": True, "quiescence_uses_same_session": True,
                "failed_history_cas_rolled_back": True,
                "killed_session_rolled_back_and_never_reconnected": True,
                "cancellation_and_allocation_abort_close_and_roll_back": True,
                "sql_errors_output_limits_and_timeouts_fence_session": True}
    finally:
        for value in clients:
            value.release_lock()


def run(update: bool, lock_only: bool = False, master_bootstrap: Path | None = None) -> dict:
    canonical = runner.load_manifest()
    staging = runner.load_manifest(ROOT / "migrations/migration_manifest.staging_0045.json")
    master = runner.load_manifest(ROOT / "migrations/migration_manifest.master_0031.json")
    unique_description_step = next(step for step in canonical.migrations
                                   if step.migration_id ==
                                   "0050_item_extra_description_fulltext_unique")
    report = {}
    engines = []
    token = secrets.token_hex(5)
    containers = []
    try:
        for label, image, password_key in (
                ("mysql8", "mysql:8.0.46", "MYSQL_ROOT_PASSWORD"),
                ("mariadb10_11", "mariadb:10.11", "MARIADB_ROOT_PASSWORD")):
            name = f"duris-staging-fork-{token}-{label}"
            password = secrets.token_urlsafe(30)
            schema.SECRETS.append(password)
            schema.run_process(["docker", "run", "-d", "--network", "none", "--name", name,
                                "--label", "codex.task=duris-staging-fork", "-e",
                                f"{password_key}={password}", image])
            containers.append(name)
            if lock_only:
                engine = schema.Engine(label, name, password, f"duris_268_{token}_locktest")
                engine.wait_ready()
                engine.create_database()
                engine.sql_file(ROOT / "migrations/immutable_migration_ledger.sql")
                result = qa(engine, ["python3", "tests/async/test_staging_migration_fork_mysql.py",
                                     "--native-lock-fixture"])
                check(result.returncode == 0, "native session test failed: " + result.stdout + result.stderr)
                report[label] = {"scope": "migration-session-faults",
                                 "server_version": engine.server_version,
                                 "native_migration_session": json.loads(result.stdout)}
                print(f"{label}: native lock/session/receipt/cancellation faults passed", flush=True)
                continue
            normal = schema.Engine(label, name, password, f"duris_268_{token}_canonicaltest")
            fork = schema.Engine(label, name, password, f"duris_268_{token}_stagingtest")
            from_master = schema.Engine(label, name, password, f"duris_268_{token}_mastertest")
            print(f"{label}: building canonical history with the real runner", flush=True)
            setup(normal, canonical)
            migrate(normal, "migration_manifest.json")
            snapshot = history(normal)
            migrate(normal, "migration_manifest.json")
            check(snapshot == history(normal), "canonical rerun rewrote migration receipts")
            normal_rows = descriptions(normal)
            normal.sql_file(unique_description_step.apply_path)
            check(normal_rows == description_rows(normal), "canonical replay changed descriptions")
            print(f"{label}: building immutable staging prefix through 0045", flush=True)
            partial = replace(staging, migrations=staging.migrations[:45])
            setup(fork, staging)
            paths = schema.copy_migration_verifiers(fork, partial.migrations)
            for step in partial.migrations:
                fork.sql_file(step.apply_path)
                result = schema.run_migration_verifier(fork, paths[step.migration_id])
                check(result.returncode == 0, "staging prefix verifier failed")
            # The prefix represents the exact observed historical database; the
            # transition itself uses MysqlExecutor, including success-last receipts.
            fork.sql("DELETE FROM mud_schema_baselines;", database=fork.database)
            schema.seed_history(fork, partial)
            before = history(fork, 45)
            rows = descriptions(fork)
            refusal = migrate(fork, "migration_manifest.json", False)
            check("edited or reordered" in refusal, "canonical did not reject the staging fork")
            check(before == history(fork, 45), "refusal altered staging receipts")
            migrate(fork, "migration_manifest.staging_0045.json")
            check(before == history(fork, 45), "transition rewrote staging receipts")
            check(rows == description_rows(fork), "transition changed protected descriptions")
            after = history(fork)
            migrate(fork, "migration_manifest.staging_0045.json")
            check(after == history(fork), "staging rerun rewrote migration receipts")
            check(fork.sql("SELECT COUNT(*) FROM mud_schema_history;", database=fork.database)
                  == "51", "staging transition did not append exactly six receipts")

            print(f"{label}: upgrading immutable master prefix through 0031", flush=True)
            setup(from_master, master, master_bootstrap)
            prefix_result = qa(from_master, ["python3", "-c",
                "import sys; from dataclasses import replace; sys.path.insert(0,'scripts'); "
                "import migration_runner as m; "
                "manifest=m.load_manifest(m.ROOT/'migrations/migration_manifest.master_0031.json'); "
                "prefix=replace(manifest,migrations=manifest.migrations[:31]); "
                "print(m.run_pending(prefix,m.MysqlExecutor(prefix)))"])
            check(prefix_result.returncode == 0,
                  "master prefix failed: " + prefix_result.stdout + prefix_result.stderr)
            master_before = history(from_master, 31)
            from_master.sql("SET FOREIGN_KEY_CHECKS=0; INSERT INTO player_item_runtime_state "
                            "(item_id,payload) VALUES(4294900002,UNHEX('00017fff804d4153544552'));",
                            database=from_master.database)
            payload_query = "SELECT item_id,HEX(payload) FROM player_item_runtime_state ORDER BY item_id;"
            payload_before = from_master.sql(payload_query, database=from_master.database)
            boot(from_master, False)
            refusal = migrate(from_master, "migration_manifest.json", False)
            check("edited or reordered" in refusal, "canonical accepted master history")
            check(master_before == history(from_master, 31), "refusal altered master receipts")
            migrate(from_master, "migration_manifest.master_0031.json")
            check(master_before == history(from_master, 31), "upgrade rewrote master receipts")
            check(payload_before == from_master.sql(payload_query, database=from_master.database),
                  "master upgrade changed retained item runtime payloads")
            master_after = history(from_master)
            migrate(from_master, "migration_manifest.master_0031.json")
            check(master_after == history(from_master), "master rerun rewrote migration receipts")
            check(from_master.sql("SELECT COUNT(*) FROM mud_schema_history;",
                                  database=from_master.database) == "51",
                  "master transition did not append exactly twenty receipts")
            check(payload_before == from_master.sql(payload_query, database=from_master.database),
                  "master rerun changed retained item runtime payloads")
            runtime = json.loads(schema.RUNTIME_MANIFEST.read_text())
            for current, key in ((normal, "migration_head"),
                                 (fork, "staging_0045_migration_head"),
                                 (from_master, "master_0031_migration_head")):
                serialized = current.sql(runtime["migration_history_sql"],
                                         database=current.database)
                digest = hashlib.sha256(b"".join(bytes.fromhex(line)
                                                for line in serialized.splitlines())).hexdigest()
                state_digest = current.sql("SELECT LOWER(HEX(history_checksum)) FROM "
                                           "mud_schema_migration_state WHERE state_id=1;",
                                           database=current.database)
                print(f"{label}/{key}: framed={digest} state={state_digest}", flush=True)
                check(digest == state_digest == runtime[key]["history_checksum"],
                      "SQL serialization and pinned history state differ")
            measured = schema.measure_fingerprint(normal, runtime)
            check(schema.measure_fingerprint(fork, runtime) == measured,
                  "canonical and staging schema metadata differ")
            check(schema.measure_fingerprint(from_master, runtime) == measured,
                  "canonical and upgraded master schema metadata differ")
            report[label] = {"server_version": normal.server_version,
                             "fingerprint": measured, "staging_prefix_preserved": True,
                             "first_45_receipt_sha256": hashlib.sha256(before.encode()).hexdigest(),
                             "staging_append_count": 6, "reruns_preserved_receipts": True,
                             "description_rows_preserved": True,
                             "master_prefix_preserved": True, "master_append_count": 20,
                             "first_31_receipt_sha256": hashlib.sha256(master_before.encode()).hexdigest(),
                             "master_runtime_payload_preserved": True,
                             "master_bootstrap": "supplied" if master_bootstrap else "current"}
            duplicate_guard(schema.Engine(label, name, password,
                                          f"duris_268_{token}_duplicatetest"),
                            unique_description_step)
            report[label]["duplicates_refuse_before_ddl_without_deleting_evidence"] = True
            lock_engine = schema.Engine(label, name, password, f"duris_268_{token}_locktest")
            lock_engine.create_database()
            lock_engine.sql_file(ROOT / "migrations/immutable_migration_ledger.sql")
            lock_result = qa(lock_engine, ["python3", "tests/async/test_staging_migration_fork_mysql.py",
                                          "--native-lock-fixture"])
            check(lock_result.returncode == 0,
                  "native lock/receipt fault test failed: " + lock_result.stdout + lock_result.stderr)
            report[label]["native_migration_session"] = json.loads(lock_result.stdout)
            print(f"{label}: native lock/session/receipt faults passed", flush=True)
            engines.append((label, normal, fork, from_master))
            print(f"{label}: canonical, staging and master converge at {measured}", flush=True)
        if update:
            value = json.loads(schema.RUNTIME_MANIFEST.read_text())
            header_path = ROOT / "src/core/runtime_compatibility_contract.h"
            header = header_path.read_text()
            for label in report:
                old = value["normalized_metadata_fingerprints"][label]
                new = report[label]["fingerprint"]
                check(header.count(old) == 1, "compiled fingerprint is ambiguous")
                header = header.replace(old, new)
                value["normalized_metadata_fingerprints"][label] = new
            schema.RUNTIME_MANIFEST.write_text(json.dumps(value, indent=2) + "\n", newline="\n")
            header_path.write_text(header, newline="\n")
        for label, normal, fork, from_master in engines:
            for engine in (normal, fork, from_master):
                print(f"{label}/{engine.database.rsplit('_', 1)[-1]}: {boot(engine)}", flush=True)
                # An old row tampered while the head/state remain unchanged must
                # fail in the shell and the actual compiled boot predicate.
                engine.sql("UPDATE mud_schema_history SET description=CONCAT(description,'!') "
                           "WHERE sequence_number=3;", database=engine.database)
                boot(engine, False)
                engine.sql("UPDATE mud_schema_history SET description=LEFT(description,"
                           "CHAR_LENGTH(description)-1) WHERE sequence_number=3;",
                           database=engine.database)
                other = json.loads(schema.RUNTIME_MANIFEST.read_text())[
                    "staging_0045_migration_head" if engine is normal else "migration_head"][
                        "history_checksum"]
                state = engine.sql("SELECT HEX(history_checksum) FROM mud_schema_migration_state "
                                   "WHERE state_id=1;", database=engine.database)
                engine.sql(f"UPDATE mud_schema_migration_state SET history_checksum=UNHEX('{other}') "
                           "WHERE state_id=1;", database=engine.database)
                boot(engine, False)
                engine.sql(f"UPDATE mud_schema_migration_state SET history_checksum=UNHEX('{state}') "
                           "WHERE state_id=1;", database=engine.database)
                # A changed generated expression has the same generic G column
                # fingerprint. The separate exact-expression proof must catch it.
                engine.sql("ALTER TABLE player_item_extra_descr MODIFY description_sha256 "
                           "BINARY(32) GENERATED ALWAYS AS (UNHEX(SHA2(CONCAT(COALESCE("
                           "description,''),'!'),256))) STORED;", database=engine.database)
                boot(engine, False)
                engine.sql("ALTER TABLE player_item_extra_descr MODIFY description_sha256 "
                           "BINARY(32) GENERATED ALWAYS AS (UNHEX(SHA2(COALESCE(description,_utf8mb4''),"
                           "256))) STORED;", database=engine.database)
                boot(engine)
            report[label]["shell_and_compiled_boot"] = True
            report[label]["old_receipt_mixed_state_and_expression_tamper_rejected"] = True
        return report
    finally:
        for name in containers:
            check(name.startswith(f"duris-staging-fork-{token}-"), "unexpected cleanup target")
            subprocess.run(["docker", "rm", "-f", name], capture_output=True)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--update-contract", action="store_true")
    parser.add_argument("--report", type=Path)
    parser.add_argument("--master-bootstrap", type=Path,
                        help="use a captured master bootstrap for the 0031 upgrade fixture")
    parser.add_argument("--native-lock-fixture", action="store_true")
    parser.add_argument("--lock-only", action="store_true",
                        help="run only the native migration-session faults on both engines")
    arguments = parser.parse_args()
    if arguments.update_contract and (arguments.lock_only or arguments.native_lock_fixture):
        parser.error("contract updates require the complete schema qualification")
    result = native_lock_fixture() if arguments.native_lock_fixture else run(
        arguments.update_contract, arguments.lock_only, arguments.master_bootstrap)
    text = json.dumps(result, indent=2) + "\n"
    if arguments.report:
        arguments.report.write_text(text)
    print(text)
