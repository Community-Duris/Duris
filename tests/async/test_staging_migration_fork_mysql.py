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
import tempfile
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
    if isinstance(engine, LoopbackEngine):
        return engine.qa(command)
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


class LoopbackEngine(schema.Engine):
    """Native transport restricted to an explicitly disposable loopback server.

    The caller owns the server lifecycle. Only uniquely generated test databases
    and this object's private temporary files are created by this transport.
    """
    def __init__(self, label: str, container: str, password: str, database: str):
        super().__init__(label, container, password, database)
        check(os.environ.get("TEST_DB_DISPOSABLE") == "1" and
              os.environ.get("STAGING_FORK_DISPOSABLE_SERVER") == "1" and
              os.environ.get("DB_HOST") == "127.0.0.1" and
              not os.environ.get("DB_SOCKET"),
              "native staging qualification requires an explicitly disposable loopback server")
        self.port = os.environ.get("DB_PORT", "")
        check(self.port.isdigit() and 1 <= int(self.port) <= 65535,
              "explicit disposable server port is required")
        self.user = os.environ.get("DB_USER", "")
        check(bool(self.user), "explicit disposable server user is required")
        self.directory = tempfile.TemporaryDirectory(prefix="duris-staging-verifiers-")
        self.copies = {}

    def environment(self):
        return dict(os.environ, ENVIRONMENT="test", DB_HOST="127.0.0.1",
                    DB_PORT=self.port, DB_USER=self.user, DB_PASSWD=self.password,
                    DB_NAME=self.database, TEST_DB_DISPOSABLE="1", MYSQL_PWD=self.password)

    def mysql_args(self, database=None, sql=None):
        args = ["mysql", "--no-defaults", "--protocol=tcp", "-h127.0.0.1",
                "-P" + self.port, "-u" + self.user, "-N", "-B", "--raw"]
        if database is not None:
            args.append(database)
        if sql is not None:
            args.extend(["-e", sql])
        return args

    def sql(self, statement, *, database=None, check=True):
        result = subprocess.run(self.mysql_args(database, statement),
                                env=self.environment(), capture_output=True, text=True)
        if check and result.returncode:
            raise schema.SchemaTestFailure(schema.redact(result.stderr))
        return result.stdout.strip()

    def sql_file(self, path):
        result = subprocess.run(self.mysql_args(self.database), input=path.read_bytes(),
                                env=self.environment(), capture_output=True)
        check(result.returncode == 0, schema.redact(result.stderr.decode()))

    def wait_ready(self):
        self.server_version = self.sql("SELECT VERSION();")
        valid = (self.server_version.startswith("8.0.") and "MariaDB" not in self.server_version
                 if self.label == "mysql8" else
                 self.server_version.startswith("10.11.") and "MariaDB" in self.server_version)
        check(valid, "disposable server version does not match the selected engine")

    def copy(self, source, destination):
        target = Path(self.directory.name) / Path(destination).name
        target.write_bytes(source.read_bytes().replace(b"\r\n", b"\n"))
        self.copies[destination] = str(target)

    def exec(self, args, *, check=True):
        environment = self.environment()
        command = list(args)
        while command and command[0] == "-e":
            key, value = command[1].split("=", 1)
            environment[key] = self.copies.get(value, value)
            command = command[2:]
        environment.update(DB_PORT=self.port, DB_USER=self.user)
        command = [self.copies.get(value, value) for value in command]
        result = subprocess.run(command, env=environment, capture_output=True)
        if check and result.returncode:
            raise schema.SchemaTestFailure(schema.redact(result.stderr.decode()))
        return result

    def qa(self, command):
        result = subprocess.run(command, cwd=ROOT, env=self.environment(),
                                capture_output=True, text=True)
        result.stdout = schema.redact(result.stdout)
        result.stderr = schema.redact(result.stderr)
        return result


def migrate(engine: schema.Engine, manifest_name: str, success: bool = True) -> str:
    result = qa(engine, ["python3", "scripts/migration_runner.py", "--manifest",
                         "migrations/" + manifest_name, "run"])
    check((result.returncode == 0) == success,
          f"migration outcome mismatch: {result.stdout} {result.stderr}")
    return result.stdout + result.stderr


def migrate_prefix(engine: schema.Engine, manifest_name: str, count: int) -> None:
    """Create an actual immutable prefix, retaining the runner's receipts."""
    result = qa(engine, ["python3", "-c",
        "import sys; from dataclasses import replace; sys.path.insert(0,'scripts'); "
        "import migration_runner as m; "
        "manifest=m.load_manifest(m.ROOT/'migrations'/sys.argv[1]); "
        "prefix=replace(manifest,migrations=manifest.migrations[:int(sys.argv[2])]); "
        "print(m.run_pending(prefix,m.MysqlExecutor(prefix)))", manifest_name, str(count)])
    check(result.returncode == 0,
          "immutable prefix failed: " + result.stdout + result.stderr)


def history(engine: schema.Engine, limit: int | None = None) -> str:
    suffix = f" LIMIT {limit}" if limit is not None else ""
    return engine.sql("SELECT HEX(CONCAT(migration_id,CHAR(0),sequence_number,CHAR(0),"
                      "description,CHAR(0),HEX(apply_checksum),CHAR(0),HEX(verify_checksum),"
                      "CHAR(0),compatibility,CHAR(0),runner_version,CHAR(0),applied_at)) "
                      "FROM mud_schema_history ORDER BY sequence_number" + suffix + ";",
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
    if isinstance(engine, LoopbackEngine):
        result = subprocess.run(engine.mysql_args(engine.database),
                                input=step.apply_path.read_bytes(), env=engine.environment(),
                                capture_output=True)
    else:
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


def run(update: bool, lock_only: bool = False, loopback_engine: str | None = None,
        master_bootstrap: Path | None = None) -> dict:
    canonical = runner.load_manifest()
    staging = runner.load_manifest(ROOT / "migrations/migration_manifest.staging_0045.json")
    master = runner.load_manifest(ROOT / "migrations/migration_manifest.master_0031.json")
    retained_telemetry = (
        ("telemetry_0067_migration_head", "migration_manifest.telemetry_0067.json"),
        ("telemetry_0067_staging_0045_migration_head",
         "migration_manifest.telemetry_0067_staging_0045.json"),
        ("telemetry_0067_master_0031_migration_head",
         "migration_manifest.telemetry_0067_master_0031.json"),
    )
    unique_description_step = next(step for step in canonical.migrations
                                   if step.migration_id ==
                                   "0050_item_extra_description_fulltext_unique")
    report = {}
    engines = []
    token = secrets.token_hex(5)
    containers = []
    native_engines = []
    def engine_factory(*args):
        engine = LoopbackEngine(*args) if loopback_engine else schema.Engine(*args)
        if loopback_engine:
            native_engines.append(engine)
        return engine
    try:
        for label, image, password_key in (
                ("mysql8", "mysql:8.0.46", "MYSQL_ROOT_PASSWORD"),
                ("mariadb10_11", "mariadb:10.11", "MARIADB_ROOT_PASSWORD")):
            if loopback_engine and label != loopback_engine:
                continue
            name = f"duris-staging-fork-{token}-{label}"
            password = os.environ.get("DB_PASSWD", "") if loopback_engine else secrets.token_urlsafe(30)
            check(bool(password), "explicit disposable server password is required")
            schema.SECRETS.append(password)
            if not loopback_engine:
                schema.run_process(["docker", "run", "-d", "--network", "none", "--name", name,
                                "--label", "codex.task=duris-staging-fork", "-e",
                                f"{password_key}={password}", image])
                containers.append(name)
            if lock_only:
                engine = engine_factory(label, name, password, f"duris_268_{token}_locktest")
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
            normal = engine_factory(label, name, password, f"duris_268_{token}_canonicaltest")
            fork = engine_factory(label, name, password, f"duris_268_{token}_stagingtest")
            from_master = engine_factory(label, name, password, f"duris_268_{token}_mastertest")
            print(f"{label}: building canonical history with the real runner", flush=True)
            setup(normal, canonical)
            migrate_prefix(normal, "migration_manifest.json", 56)
            canonical_prefix = history(normal)
            boot(normal, False)
            migrate_prefix(normal, "migration_manifest.json", 75)
            progression_prefixes = {"migration_head": history(normal)}
            boot(normal, False)
            migrate(normal, "migration_manifest.json")
            check(progression_prefixes["migration_head"] == history(normal, 75),
                  "canonical reward append rewrote progression receipts")
            check(canonical_prefix == history(normal, 56),
                  "canonical append rewrote the recorded accounting prefix")
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
            migrate_prefix(fork, "migration_manifest.staging_0045.json", 56)
            staging_prefix = history(fork)
            boot(fork, False)
            migrate_prefix(fork, "migration_manifest.staging_0045.json", 75)
            progression_prefixes["staging_0045_migration_head"] = history(fork)
            boot(fork, False)
            migrate(fork, "migration_manifest.staging_0045.json")
            check(progression_prefixes["staging_0045_migration_head"] == history(fork, 75),
                  "staging reward append rewrote progression receipts")
            check(staging_prefix == history(fork, 56),
                  "staging append rewrote the recorded accounting prefix")
            check(before == history(fork, 45), "transition rewrote staging receipts")
            check(rows == description_rows(fork), "transition changed protected descriptions")
            after = history(fork)
            migrate(fork, "migration_manifest.staging_0045.json")
            check(after == history(fork), "staging rerun rewrote migration receipts")
            check(fork.sql("SELECT COUNT(*) FROM mud_schema_history;", database=fork.database)
                  == str(len(staging.migrations)), "staging transition has an incomplete history")

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
            migrate_prefix(from_master, "migration_manifest.master_0031.json", 56)
            master_prefix = history(from_master)
            migrate_prefix(from_master, "migration_manifest.master_0031.json", 75)
            progression_prefixes["master_0031_migration_head"] = history(from_master)
            boot(from_master, False)
            migrate(from_master, "migration_manifest.master_0031.json")
            check(progression_prefixes["master_0031_migration_head"] == history(from_master, 75),
                  "master reward append rewrote progression receipts")
            check(master_prefix == history(from_master, 56),
                  "master append rewrote the recorded accounting prefix")
            check(master_before == history(from_master, 31), "upgrade rewrote master receipts")
            check(payload_before == from_master.sql(payload_query, database=from_master.database),
                  "master upgrade changed retained item runtime payloads")
            master_after = history(from_master)
            migrate(from_master, "migration_manifest.master_0031.json")
            check(master_after == history(from_master), "master rerun rewrote migration receipts")
            check(from_master.sql("SELECT COUNT(*) FROM mud_schema_history;",
                                  database=from_master.database) == str(len(master.migrations)),
                  "master transition has an incomplete history")
            check(payload_before == from_master.sql(payload_query, database=from_master.database),
                  "master rerun changed retained item runtime payloads")
            runtime = json.loads(schema.RUNTIME_MANIFEST.read_text())
            current_histories = [(normal, "migration_head"),
                                (fork, "staging_0045_migration_head"),
                                (from_master, "master_0031_migration_head")]
            preserved_prefixes = {}
            for number, (key, manifest_name) in enumerate(retained_telemetry):
                print(f"{label}: appending accounting changes to recorded {key}", flush=True)
                telemetry_manifest = runner.load_manifest(ROOT / "migrations" / manifest_name)
                current = engine_factory(label, name, password,
                    f"duris_268_{token}_telemetry{number}test")
                setup(current, telemetry_manifest)
                migrate_prefix(current, manifest_name, 67)
                old_prefix = history(current)
                boot(current, False)
                refusal = migrate(current, "migration_manifest.json", False)
                check("edited or reordered" in refusal,
                      "canonical accepted a divergent recorded telemetry history")
                check(old_prefix == history(current), "refusal changed telemetry receipts")
                migrate_prefix(current, manifest_name, 75)
                progression_prefixes[key] = history(current)
                boot(current, False)
                migrate(current, manifest_name)
                check(progression_prefixes[key] == history(current, 75),
                      "retained telemetry reward append rewrote progression receipts")
                check(old_prefix == history(current, 67),
                      "append rewrote the recorded telemetry prefix")
                complete = history(current)
                migrate(current, manifest_name)
                check(complete == history(current), "telemetry rerun rewrote receipts")
                check(current.sql("SELECT COUNT(*) FROM mud_schema_history;",
                                  database=current.database) == str(len(telemetry_manifest.migrations)),
                      "telemetry append has an incomplete history")
                preserved_prefixes[key] = {
                    "prefix_count": 67, "append_count": len(telemetry_manifest.migrations) - 67,
                    "prefix_sha256": hashlib.sha256(old_prefix.encode()).hexdigest(),
                    "rerun_preserved_receipts": True,
                }
                current_histories.append((current, key))
            for current, key in current_histories:
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
            for current, key in current_histories[1:]:
                check(schema.measure_fingerprint(current, runtime) == measured,
                      f"canonical and {key} schema metadata differ")
            report[label] = {"server_version": normal.server_version,
                             "fingerprint": measured, "staging_prefix_preserved": True,
                             "first_45_receipt_sha256": hashlib.sha256(before.encode()).hexdigest(),
                             "staging_append_count": len(canonical.migrations) - 45, "reruns_preserved_receipts": True,
                             "description_rows_preserved": True,
                             "master_prefix_preserved": True, "master_append_count": len(canonical.migrations) - 31,
                             "first_31_receipt_sha256": hashlib.sha256(master_before.encode()).hexdigest(),
                             "master_runtime_payload_preserved": True,
                             "master_bootstrap": "supplied" if master_bootstrap else "current",
                             "accounting_56_prefixes_preserved": True,
                             "accounting_append_count": len(canonical.migrations) - 56,
                             "retained_telemetry_histories": preserved_prefixes,
                             "converged_history_count": len(current_histories),
                             "complete_sequence_count": len(canonical.migrations),
                             "progression_75_prefixes_preserved": {
                                 key: {"prefix_count": 75, "append_count": 3,
                                       "prefix_sha256": hashlib.sha256(value.encode()).hexdigest()}
                                 for key, value in progression_prefixes.items()},
                             "runtime_table_count": runtime["current_table_count"]}
            duplicate_guard(engine_factory(label, name, password,
                                          f"duris_268_{token}_duplicatetest"),
                            unique_description_step)
            report[label]["duplicates_refuse_before_ddl_without_deleting_evidence"] = True
            lock_engine = engine_factory(label, name, password, f"duris_268_{token}_locktest")
            lock_engine.create_database()
            lock_engine.sql_file(ROOT / "migrations/immutable_migration_ledger.sql")
            lock_result = qa(lock_engine, ["python3", "tests/async/test_staging_migration_fork_mysql.py",
                                          "--native-lock-fixture"])
            check(lock_result.returncode == 0,
                  "native lock/receipt fault test failed: " + lock_result.stdout + lock_result.stderr)
            report[label]["native_migration_session"] = json.loads(lock_result.stdout)
            print(f"{label}: native lock/session/receipt faults passed", flush=True)
            engines.append((label, current_histories))
            print(f"{label}: six complete histories converge at {measured}", flush=True)
        if update:
            value = json.loads(schema.RUNTIME_MANIFEST.read_text())
            header_path = ROOT / "src/core/runtime_compatibility_contract.h"
            header = header_path.read_text()
            for label in report:
                old = value["normalized_metadata_fingerprints"][label]
                new = report[label]["fingerprint"]
                constant = {"mysql8": "RUNTIME_MYSQL8_METADATA_FINGERPRINT",
                            "mariadb10_11": "RUNTIME_MARIADB10_11_METADATA_FINGERPRINT"}[label]
                expression = re.compile(r"(" + constant + r'\s*=\s*")' + old + r'(")')
                header, replacements = expression.subn(lambda match: match[1] + new + match[2], header)
                check(replacements == 1, "compiled fingerprint is ambiguous")
                value["normalized_metadata_fingerprints"][label] = new
            schema.RUNTIME_MANIFEST.write_text(json.dumps(value, indent=2) + "\n", newline="\n")
            header_path.write_text(header, newline="\n")
        for label, current_histories in engines:
            for engine, key in current_histories:
                print(f"{label}/{engine.database.rsplit('_', 1)[-1]}: {boot(engine)}", flush=True)
                if key == "migration_head":
                    engine.sql("ALTER TABLE telemetry_reward_generation_v2 MODIFY reserved_bytes INT NOT NULL;",
                               database=engine.database)
                    boot(engine, False)
                    engine.sql("ALTER TABLE telemetry_reward_generation_v2 MODIFY reserved_bytes INT UNSIGNED NOT NULL;",
                               database=engine.database)
                    boot(engine)
                    engine.sql("DROP TRIGGER telemetry_reward2_source_insert;", database=engine.database)
                    boot(engine, False)
                    engine.sql_file(ROOT / "migrations/immutable/0073_telemetry_canonical_reward_retention.sql")
                    boot(engine)
                    report[label]["canonical_reward_type_and_trigger_tamper_rejected"] = True
                # An old row tampered while the head/state remain unchanged must
                # fail in the shell and the actual compiled boot predicate.
                engine.sql("UPDATE mud_schema_history SET description=CONCAT(description,'!') "
                           "WHERE sequence_number=3;", database=engine.database)
                boot(engine, False)
                engine.sql("UPDATE mud_schema_history SET description=LEFT(description,"
                           "CHAR_LENGTH(description)-1) WHERE sequence_number=3;",
                           database=engine.database)
                other = json.loads(schema.RUNTIME_MANIFEST.read_text())[
                    "staging_0045_migration_head" if key == "migration_head" else "migration_head"][
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
        for engine in native_engines:
            engine.drop_database()
            engine.directory.cleanup()
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
    parser.add_argument("--disposable-loopback", choices=("mysql8", "mariadb10_11"),
                        help="use an explicitly guarded task-owned native server; caller owns cleanup")
    arguments = parser.parse_args()
    if arguments.update_contract and (arguments.lock_only or arguments.native_lock_fixture):
        parser.error("contract updates require the complete schema qualification")
    if arguments.update_contract and arguments.disposable_loopback:
        parser.error("contract updates require both engines in one qualification")
    result = native_lock_fixture() if arguments.native_lock_fixture else run(
        arguments.update_contract, arguments.lock_only, arguments.disposable_loopback,
        arguments.master_bootstrap)
    text = json.dumps(result, indent=2) + "\n"
    if arguments.report:
        arguments.report.write_text(text)
    print(text)
