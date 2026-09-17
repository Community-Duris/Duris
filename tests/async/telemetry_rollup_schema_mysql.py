#!/usr/bin/env python3
"""#268 disposable MariaDB/MySQL full-schema and 0017 contract test.

This is deliberately narrower than an application regression suite.  It runs the
same fresh-bootstrap plus immutable-migration setup used by
``run_runtime_compatibility_mysql.sh`` on the task-owned MariaDB container and a
new no-network MySQL 8 container.  It then exercises only the new rollup tables,
the runtime compatibility verifier, and the generated contract fingerprints.

No credential value is printed.  The configured task database is never used as a
test target; each engine receives a newly named ``duris_268_*test`` database.
"""
from __future__ import annotations

import argparse
import copy
import json
import os
import re
import secrets
import shlex
import subprocess
import sys
import tempfile
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable, Sequence


ROOT = Path(__file__).resolve().parents[2]
BOOTSTRAP = ROOT / "migrations/bootstrap_multithread_safe.sql"
RUNTIME_MANIFEST = ROOT / "migrations/runtime_compatibility_manifest.json"
RUNTIME_VERIFY = ROOT / "migrations/verify_runtime_compatibility.sh"
VALIDATOR = ROOT / "scripts/validate_runtime_compatibility.py"

BOOTSTRAP_TABLE_COUNT = 179
RUNTIME_TABLE_COUNT = 193

NEW_TABLES = ("telemetry_cohort_member", "telemetry_rollup_session")
SESSION_TABLE = "telemetry_rollup_session"
COHORT_TABLE = "telemetry_cohort_member"

SESSION_PROBE = (
    1, 101, 7, 9, 1001, 2002, 3, 4242, 123,
    7, 100, 80, 10, 1, 100, 20, 100, 80, 10, 1, 100, 20,
    100, 5, 1, 0, 2, 0, 36, 1,
)
COHORT_PROBE = (
    1, 101, 7, 9, "2026-09-14", 50, 2, 3, 4, 1234, 7001, 2, 2,
    4242, 1001, 2002, 3, 100, 90, 4, 0, 36,
)

SESSION_COLUMNS = (
    "definition_version", "generation", "environment_id", "season_id",
    "session_boot_id", "session_process_id", "session_seq", "subject_id",
    "pid", "latest_checkpoint_revision", "connected_usec", "active_usec",
    "idle_usec", "unknown_usec", "resident_usec", "linkdead_usec",
    "covered_connected_usec", "covered_active_usec", "covered_idle_usec",
    "covered_unknown_usec", "covered_resident_usec", "covered_linkdead_usec",
    "attributable_usec", "observed_intervals", "entered", "exited",
    "end_reason", "quality_flags", "input_watermark", "provisional",
)
COHORT_COLUMNS = (
    "definition_version", "generation", "environment_id", "season_id",
    "utc_day", "level_band", "class_id", "race_id", "faction_id",
    "zone_vnum", "config_id", "category", "membership_kind", "subject_id",
    "session_boot_id", "session_process_id", "session_seq", "duration_usec",
    "attributable_usec", "observed_intervals", "quality_flags", "input_watermark",
)

CANONICAL_PRIMARY = (
    "definition_version,generation,environment_id,season_id,"
    "session_boot_id,session_process_id,session_seq"
)
CANONICAL_SECONDARY = (
    "definition_version,generation,environment_id,season_id,subject_id,"
    "session_boot_id,session_process_id,session_seq"
)
CHECK_KIND_NAME = "chk_telemetry_cohort_member_kind"
CHECK_SUBJECT_NAME = "chk_telemetry_cohort_member_subject"
CHECK_KIND_CANONICAL = (
    "(membership_kind = 1 and session_boot_id = 0 and session_process_id = 0 "
    "and session_seq = 0) or (membership_kind = 2 and session_boot_id <> 0 "
    "and session_process_id <> 0 and session_seq <> 0)"
)
CHECK_SUBJECT_CANONICAL = "subject_id <> 0"
CHECK_KIND_MYSQL = (
    "(((membership_kind = 1) and (session_boot_id = 0) and "
    "(session_process_id = 0) and (session_seq = 0)) or ((membership_kind = 2) "
    "and (session_boot_id <> 0) and (session_process_id <> 0) and "
    "(session_seq <> 0)))"
)
CHECK_KIND_MARIADB = (
    "membership_kind = 1 and session_boot_id = 0 and session_process_id = 0 "
    "and session_seq = 0 or membership_kind = 2 and session_boot_id <> 0 "
    "and session_process_id <> 0 and session_seq <> 0"
)
CHECK_SUBJECT_MYSQL = "(subject_id <> 0)"
CHECK_SUBJECT_MARIADB = "subject_id <> 0"
ORIGINAL17_KEY_TABLE = "telemetry_cohort_member_original17_probe"
ORIGINAL17_KEY_DDL = f"""
CREATE TABLE {ORIGINAL17_KEY_TABLE} (
    definition_version INT UNSIGNED NOT NULL,
    generation BIGINT UNSIGNED NOT NULL,
    environment_id BIGINT UNSIGNED NOT NULL,
    season_id BIGINT UNSIGNED NOT NULL,
    utc_day DATE NOT NULL,
    level_band SMALLINT UNSIGNED NOT NULL,
    class_id SMALLINT UNSIGNED NOT NULL,
    race_id SMALLINT UNSIGNED NOT NULL,
    faction_id SMALLINT UNSIGNED NOT NULL,
    zone_vnum INT NOT NULL,
    config_id BIGINT UNSIGNED NOT NULL,
    category TINYINT UNSIGNED NOT NULL,
    membership_kind TINYINT UNSIGNED NOT NULL,
    subject_id BIGINT UNSIGNED NOT NULL,
    session_boot_id BIGINT UNSIGNED NOT NULL,
    session_process_id BIGINT UNSIGNED NOT NULL,
    session_seq BIGINT UNSIGNED NOT NULL,
    PRIMARY KEY (definition_version,generation,environment_id,season_id,utc_day,
        level_band,class_id,race_id,faction_id,zone_vnum,config_id,category,
        membership_kind,subject_id,session_boot_id,session_process_id,session_seq)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
""".strip()


class SchemaTestFailure(RuntimeError):
    """A focused test contract failed."""


SECRETS: list[str] = []


def redact(value: str) -> str:
    for secret in SECRETS:
        if secret:
            value = value.replace(secret, "<redacted>")
    return value


def safe_command(args: Iterable[object]) -> str:
    values = [str(value) for value in args]
    return " ".join(
        "MYSQL_PWD=<redacted>" if value.startswith("MYSQL_PWD=") else
        "MYSQL_ROOT_PASSWORD=<redacted>" if value.startswith("MYSQL_ROOT_PASSWORD=") else
        "DB_PASSWD=<redacted>" if value.startswith("DB_PASSWD=") else
        value
        for value in values
    )


def run_process(
    args: list[str], *, input_bytes: bytes | None = None, check: bool = True,
) -> subprocess.CompletedProcess[bytes]:
    result = subprocess.run(
        args, input=input_bytes, capture_output=True, check=False,
    )
    if check and result.returncode:
        detail = redact(
            (result.stderr or result.stdout).decode("utf-8", "replace").strip()
        )
        raise SchemaTestFailure(
            f"command failed ({result.returncode}): {safe_command(args)}"
            + (f"; {detail[-2500:]}" if detail else "")
        )
    return result


def json_from_container(name: str) -> dict:
    raw = run_process(["docker", "inspect", name]).stdout
    try:
        return json.loads(raw)[0]
    except (IndexError, json.JSONDecodeError) as error:
        raise SchemaTestFailure(f"docker inspect returned invalid JSON for {name}") from error


def assert_task_mariadb(name: str) -> None:
    inspected = json_from_container(name)
    labels = inspected["Config"].get("Labels", {})
    if inspected["Config"].get("Image") != "mariadb:10.11":
        raise SchemaTestFailure(f"unexpected image for task MariaDB container {name}")
    if labels.get("hermes.task") != "duris-268":
        raise SchemaTestFailure(f"container {name} is not labeled for task duris-268")
    if inspected["HostConfig"].get("NetworkMode") != "none":
        raise SchemaTestFailure(f"task MariaDB container {name} is not network-isolated")


def mysql_args(engine: "Engine", database: str | None, sql: str | None) -> list[str]:
    args = [
        "docker", "exec", "-i", "-e", f"MYSQL_PWD={engine.password}",
        engine.container, "mysql", "--protocol=tcp", "-h127.0.0.1", "-P3306",
        "-uroot", "-N", "-B", "--raw",
    ]
    if database is not None:
        args.append(database)
    if sql is not None:
        args.extend(["-e", sql])
    return args


@dataclass
class Engine:
    label: str
    container: str
    password: str
    database: str
    server_version: str = ""
    bootstrap_shape: tuple[str, ...] = ()
    applied_shape: tuple[str, ...] = ()
    probe_before_replay: dict[str, str] | None = None
    probe_after_replay: dict[str, str] | None = None
    member_constraint_outcomes: dict[str, dict[str, object]] | None = None
    measured_fingerprint: str = ""
    history_checksum: str = ""
    rollup_verifier_path: str = ""
    rollup_drift_outcomes: dict[str, dict[str, object]] | None = None
    drift_outcomes: dict[str, dict[str, object]] | None = None

    def sql(
        self, statement: str, *, database: str | None = None, check: bool = True,
    ) -> str:
        result = run_process(mysql_args(self, database, statement), check=check)
        return result.stdout.decode("utf-8", "replace").strip()

    def sql_file(self, path: Path) -> None:
        run_process(
            mysql_args(self, self.database, None),
            input_bytes=path.read_bytes(),
        )

    def copy(self, source: Path, destination: str) -> None:
        run_process(["docker", "cp", str(source), f"{self.container}:{destination}"])

    def exec(self, args: list[str], *, check: bool = True) -> subprocess.CompletedProcess[bytes]:
        # Docker exec options precede the container; the command follows it.
        options: list[str] = []
        command = list(args)
        while command and command[0] == "-e":
            if len(command) < 2:
                raise SchemaTestFailure("docker exec environment option is incomplete")
            options.extend(command[:2])
            command = command[2:]
        return run_process(["docker", "exec", *options, self.container, *command], check=check)

    def create_database(self) -> None:
        if not re.fullmatch(r"duris_268_[a-z0-9_]*test", self.database):
            raise SchemaTestFailure(f"generated database name is outside task scope: {self.database}")
        self.sql(
            f"CREATE DATABASE `{self.database}` CHARACTER SET utf8mb4 "
            "COLLATE utf8mb4_unicode_ci;",
            database=None,
        )

    def drop_database(self) -> None:
        self.sql(f"DROP DATABASE IF EXISTS `{self.database}`;", database=None)

    def wait_ready(self) -> None:
        deadline = time.monotonic() + 120
        last_error = ""
        while time.monotonic() < deadline:
            result = run_process(
                mysql_args(self, None, "SELECT VERSION();"), check=False,
            )
            if result.returncode == 0:
                self.server_version = result.stdout.decode("utf-8", "replace").strip()
                return
            last_error = redact(result.stderr.decode("utf-8", "replace").strip())
            time.sleep(1)
        raise SchemaTestFailure(
            f"{self.label} database did not become ready: {last_error[-1000:]}"
        )


def new_database_name(label: str) -> str:
    token = secrets.token_hex(4)
    SECRETS.append(token)  # the token is not secret, but keeps defensive redaction simple.
    return f"duris_268_schema_{label}_{os.getpid()}_{token}test"


def new_container_name() -> str:
    return f"duris-268-rollup-mysql8-schema-{os.getpid()}-{secrets.token_hex(4)}"


def start_mysql8() -> Engine:
    name = new_container_name()
    password = "schema-test-" + secrets.token_urlsafe(24)
    SECRETS.append(password)
    run_process([
        "docker", "run", "-d", "--name", name,
        "--label", "hermes.task=duris-268",
        "--label", "hermes.purpose=issue-268-schema-test",
        "--network", "none",
        "-e", f"MYSQL_ROOT_PASSWORD={password}",
        "mysql:8.0",
    ])
    return Engine("MySQL 8.0", name, password, new_database_name("mysql8"))


def start_mariadb(credentials: dict) -> Engine:
    container = credentials["container"]
    assert_task_mariadb(container)
    password = credentials["root"]
    if not isinstance(password, str) or not password:
        raise SchemaTestFailure("credential file does not contain a MariaDB root handle")
    SECRETS.append(password)
    return Engine("MariaDB 10.11", container, password, new_database_name("mariadb10_11"))


def original17_key_limit_probe(engine: Engine) -> dict[str, object]:
    engine.sql(f"DROP TABLE IF EXISTS {ORIGINAL17_KEY_TABLE};", database=engine.database)
    result = run_process(
        mysql_args(engine, engine.database, ORIGINAL17_KEY_DDL),
        check=False,
    )
    output = redact((result.stdout + result.stderr).decode("utf-8", "replace")).strip()
    try:
        if engine.label == "MySQL 8.0":
            if result.returncode == 0 or "Too many key parts" not in output:
                raise SchemaTestFailure(
                    f"{engine.label} did not reject the original 17-key-part DDL: {output[-1000:]}"
                )
            return {
                "key_parts": 17,
                "rejected": True,
                "reason": "MySQL maximum is 16 key parts",
                "failure_excerpt": output[-500:],
            }
        if result.returncode:
            raise SchemaTestFailure(
                f"{engine.label} unexpectedly rejected the original 17-key-part DDL: {output[-1000:]}"
            )
        return {"key_parts": 17, "accepted": True, "reason": "MariaDB accepts 17 key parts"}
    finally:
        engine.sql(f"DROP TABLE IF EXISTS {ORIGINAL17_KEY_TABLE};", database=engine.database)


def table_shape(engine: Engine) -> tuple[str, ...]:
    queries = (
        "SELECT table_name,engine,table_collation FROM information_schema.tables "
        "WHERE table_schema=DATABASE() AND table_type='BASE TABLE' "
        "AND table_name IN ('telemetry_cohort_member','telemetry_rollup_session') "
        "ORDER BY table_name;",
        "SELECT table_name,column_name,ordinal_position,data_type,column_type,is_nullable,"
        "COALESCE(column_default,'<NULL>'),COALESCE(extra,''),COALESCE(character_maximum_length,0),"
        "COALESCE(numeric_precision,0),COALESCE(numeric_scale,0),COALESCE(datetime_precision,0) "
        "FROM information_schema.columns WHERE table_schema=DATABASE() "
        "AND table_name IN ('telemetry_cohort_member','telemetry_rollup_session') "
        "ORDER BY table_name,ordinal_position;",
        "SELECT table_name,index_name,non_unique,seq_in_index,column_name,COALESCE(sub_part,0) "
        "FROM information_schema.statistics WHERE table_schema=DATABASE() "
        "AND table_name IN ('telemetry_cohort_member','telemetry_rollup_session') "
        "ORDER BY table_name,BINARY index_name,seq_in_index;",
        "SELECT k.table_name,k.constraint_name,k.column_name,k.referenced_table_name,"
        "k.referenced_column_name,k.ordinal_position,r.update_rule,r.delete_rule "
        "FROM information_schema.key_column_usage k JOIN information_schema.referential_constraints r "
        "ON r.constraint_schema=k.constraint_schema AND r.constraint_name=k.constraint_name "
        "WHERE k.constraint_schema=DATABASE() AND k.table_name IN "
        "('telemetry_cohort_member','telemetry_rollup_session') "
        "AND k.referenced_table_name IS NOT NULL ORDER BY k.table_name,k.constraint_name,k.ordinal_position;",
        "SELECT table_name,constraint_name,constraint_type FROM information_schema.table_constraints "
        "WHERE constraint_schema=DATABASE() AND table_name IN "
        "('telemetry_cohort_member','telemetry_rollup_session') AND constraint_type='CHECK' "
        "ORDER BY table_name,BINARY constraint_name;",
    )
    return tuple(
        f"q{index}:{engine.sql(query, database=engine.database)}"
        for index, query in enumerate(queries, 1)
    )


def normalize_check_clause(clause: str) -> str:
    return re.sub(r"\s+", " ", clause.replace("`", "").lower()).strip()


def canonical_check_clause(name: str, clause: str) -> str:
    if name == CHECK_KIND_NAME and clause in (CHECK_KIND_MYSQL, CHECK_KIND_MARIADB):
        return CHECK_KIND_CANONICAL
    if name == CHECK_SUBJECT_NAME and clause in (CHECK_SUBJECT_MYSQL, CHECK_SUBJECT_MARIADB):
        return CHECK_SUBJECT_CANONICAL
    raise SchemaTestFailure(f"{name} has an unexpected normalized CHECK expression")


def member_check_expression_contract(engine: Engine) -> dict[str, object]:
    if "MariaDB" in engine.server_version:
        query = (
            "SELECT c.table_name,c.constraint_name,c.check_clause,'UNAVAILABLE' "
            "FROM information_schema.check_constraints c JOIN information_schema.table_constraints t "
            "ON t.constraint_schema=c.constraint_schema AND t.constraint_name=c.constraint_name "
            "WHERE c.constraint_schema=DATABASE() AND c.table_name='telemetry_cohort_member' "
            "AND t.constraint_type='CHECK' ORDER BY BINARY c.constraint_name;"
        )
    else:
        query = (
            "SELECT t.table_name,c.constraint_name,c.check_clause,t.enforced "
            "FROM information_schema.check_constraints c JOIN information_schema.table_constraints t "
            "ON t.constraint_schema=c.constraint_schema AND t.constraint_name=c.constraint_name "
            "WHERE c.constraint_schema=DATABASE() AND t.table_name='telemetry_cohort_member' "
            "AND t.constraint_type='CHECK' ORDER BY BINARY c.constraint_name;"
        )
    rows = engine.sql(query, database=engine.database).splitlines()
    expressions: dict[str, str] = {}
    enforcement: dict[str, str] = {}
    for row in rows:
        table, name, clause, enforced = row.split("\t", 3)
        if table != COHORT_TABLE:
            raise SchemaTestFailure(f"{engine.label} returned CHECK metadata for {table}")
        expressions[name] = canonical_check_clause(name, normalize_check_clause(clause))
        enforcement[name] = enforced
    expected = {
        CHECK_KIND_NAME: CHECK_KIND_CANONICAL,
        CHECK_SUBJECT_NAME: CHECK_SUBJECT_CANONICAL,
    }
    if expressions != expected:
        raise SchemaTestFailure(
            f"{engine.label} normalized CHECK expressions differ: {expressions}"
        )
    if "MariaDB" not in engine.server_version and set(enforcement.values()) != {"YES"}:
        raise SchemaTestFailure(
            f"{engine.label} CHECK enforcement metadata is not YES: {enforcement}"
        )
    return {
        "normalized_expressions": expressions,
        "enforcement": enforcement,
        "cross_engine_canonical": True,
    }


def probe_rows(engine: Engine) -> dict[str, str]:
    session = (
        "SELECT CONCAT_WS('|'," + ",".join(SESSION_COLUMNS) + ") "
        "FROM telemetry_rollup_session WHERE definition_version=1 AND generation=101 "
        "AND environment_id=7 AND season_id=9 AND session_boot_id=1001 "
        "AND session_process_id=2002 AND session_seq=3;"
    )
    cohort = (
        "SELECT CONCAT_WS('|'," + ",".join(COHORT_COLUMNS) + ") "
        "FROM telemetry_cohort_member WHERE definition_version=1 AND generation=101 "
        "AND environment_id=7 AND season_id=9 AND utc_day='2026-09-14' "
        "AND subject_id=4242 AND session_boot_id=1001 AND session_process_id=2002 "
        "AND session_seq=3;"
    )
    return {
        "telemetry_rollup_session": engine.sql(session, database=engine.database),
        "telemetry_cohort_member": engine.sql(cohort, database=engine.database),
    }


def insert_probe_rows(engine: Engine) -> None:
    session_columns = ",".join(SESSION_COLUMNS)
    session_values = ",".join(str(value) for value in SESSION_PROBE)
    cohort_columns = ",".join(COHORT_COLUMNS)
    cohort_values = ",".join(
        f"'{value}'" if isinstance(value, str) else str(value)
        for value in COHORT_PROBE
    )
    engine.sql(
        f"INSERT INTO {SESSION_TABLE} ({session_columns}) VALUES ({session_values}); "
        f"INSERT INTO {COHORT_TABLE} ({cohort_columns}) VALUES ({cohort_values});",
        database=engine.database,
    )


def verify_member_identity_constraints(engine: Engine) -> dict[str, dict[str, object]]:
    def insert_statement(values: Sequence[object]) -> str:
        rendered = ",".join(
            f"'{value}'" if isinstance(value, str) else str(value)
            for value in values
        )
        return (
            f"INSERT INTO {COHORT_TABLE} ({','.join(COHORT_COLUMNS)}) "
            f"VALUES ({rendered});"
        )

    invalid_rows: dict[str, list[object]] = {
        "kind_one_with_session_ids": list(COHORT_PROBE),
        "kind_two_without_session_ids": list(COHORT_PROBE),
        "zero_subject_id": list(COHORT_PROBE),
    }
    invalid_rows["kind_one_with_session_ids"][12] = 1
    invalid_rows["kind_two_without_session_ids"][12] = 2
    invalid_rows["kind_two_without_session_ids"][14:17] = [0, 0, 0]
    invalid_rows["zero_subject_id"][13] = 0

    outcomes: dict[str, dict[str, object]] = {}
    for name, values in invalid_rows.items():
        result = run_process(
            mysql_args(engine, engine.database, insert_statement(values)),
            check=False,
        )
        output = redact((result.stdout + result.stderr).decode("utf-8", "replace")).strip()
        if result.returncode == 0:
            raise SchemaTestFailure(
                f"{engine.label} accepted invalid cohort member identity {name}"
            )
        outcomes[name] = {"rejected": True, "failure_excerpt": output[-500:]}
    return outcomes


def copy_migration_verifiers(engine: Engine, steps: Iterable[object]) -> dict[str, str]:
    paths = {}
    for step in steps:
        destination = f"/tmp/duris268_{step.verify_path.name}"
        engine.copy(step.verify_path, destination)
        engine.exec(["chmod", "+x", destination])
        paths[step.migration_id] = destination
    return paths


def run_migration_verifier(
    engine: Engine, path: str, *, check: bool = True,
) -> subprocess.CompletedProcess[bytes]:
    args = [
        "-e", "ENVIRONMENT=test",
        "-e", "DB_HOST=127.0.0.1",
        "-e", "DB_PORT=3306",
        "-e", "DB_USER=root",
        "-e", f"DB_PASSWD={engine.password}",
        "-e", f"DB_NAME={engine.database}",
        path,
    ]
    return engine.exec(args, check=check)


def run_rollup_verifier(
    engine: Engine, *, check: bool = True,
) -> subprocess.CompletedProcess[bytes]:
    if not engine.rollup_verifier_path:
        raise SchemaTestFailure(f"{engine.label} has no copied 0017 verifier")
    return run_migration_verifier(engine, engine.rollup_verifier_path, check=check)


def drop_rollup_check(engine: Engine, name: str) -> None:
    keyword = "DROP CONSTRAINT" if "MariaDB" in engine.server_version else "DROP CHECK"
    engine.sql(
        f"ALTER TABLE {COHORT_TABLE} {keyword} {name};",
        database=engine.database,
    )


def add_rollup_check(engine: Engine, name: str, expression: str) -> None:
    engine.sql(
        f"ALTER TABLE {COHORT_TABLE} ADD CONSTRAINT {name} CHECK ({expression});",
        database=engine.database,
    )


def replace_rollup_check(engine: Engine, name: str, expression: str) -> None:
    drop_rollup_check(engine, name)
    add_rollup_check(engine, name, expression)


def rollup_verifier_drift_matrix(engine: Engine) -> dict[str, dict[str, object]]:
    scenarios = [
        (
            "wrong_unsigned_type",
            lambda: engine.sql(
                "ALTER TABLE telemetry_rollup_session MODIFY generation BIGINT NOT NULL;",
                database=engine.database,
            ),
            lambda: engine.sql(
                "ALTER TABLE telemetry_rollup_session MODIFY generation BIGINT UNSIGNED NOT NULL;",
                database=engine.database,
            ),
        ),
        (
            "wrong_default",
            lambda: engine.sql(
                "ALTER TABLE telemetry_rollup_session MODIFY input_watermark BIGINT UNSIGNED NOT NULL DEFAULT 1;",
                database=engine.database,
            ),
            lambda: engine.sql(
                "ALTER TABLE telemetry_rollup_session MODIFY input_watermark BIGINT UNSIGNED NOT NULL DEFAULT 0;",
                database=engine.database,
            ),
        ),
        (
            "missing_primary_index",
            lambda: engine.sql(
                "ALTER TABLE telemetry_rollup_session DROP PRIMARY KEY;",
                database=engine.database,
            ),
            lambda: engine.sql(
                "ALTER TABLE telemetry_rollup_session ADD PRIMARY KEY (" + CANONICAL_PRIMARY + ");",
                database=engine.database,
            ),
        ),
        (
            "wrong_primary_index",
            lambda: engine.sql(
                "ALTER TABLE telemetry_rollup_session DROP PRIMARY KEY, ADD PRIMARY KEY "
                "(definition_version,generation,environment_id,season_id,session_boot_id,session_process_id);",
                database=engine.database,
            ),
            lambda: engine.sql(
                "ALTER TABLE telemetry_rollup_session DROP PRIMARY KEY, ADD PRIMARY KEY ("
                + CANONICAL_PRIMARY + ");",
                database=engine.database,
            ),
        ),
        (
            "missing_secondary_index",
            lambda: engine.sql(
                "ALTER TABLE telemetry_rollup_session DROP INDEX idx_rollup_session_subject;",
                database=engine.database,
            ),
            lambda: engine.sql(
                "ALTER TABLE telemetry_rollup_session ADD KEY idx_rollup_session_subject ("
                + CANONICAL_SECONDARY + ");",
                database=engine.database,
            ),
        ),
        (
            "wrong_secondary_index",
            lambda: engine.sql(
                "ALTER TABLE telemetry_rollup_session DROP INDEX idx_rollup_session_subject, ADD KEY "
                "idx_rollup_session_subject (definition_version,generation,environment_id,season_id,"
                "subject_id,session_boot_id,session_process_id);",
                database=engine.database,
            ),
            lambda: engine.sql(
                "ALTER TABLE telemetry_rollup_session DROP INDEX idx_rollup_session_subject, ADD KEY "
                "idx_rollup_session_subject (" + CANONICAL_SECONDARY + ");",
                database=engine.database,
            ),
        ),
        (
            "wrong_engine",
            lambda: engine.sql(
                "ALTER TABLE telemetry_rollup_session ENGINE=MyISAM;",
                database=engine.database,
            ),
            lambda: engine.sql(
                "ALTER TABLE telemetry_rollup_session ENGINE=InnoDB;",
                database=engine.database,
            ),
        ),
        (
            "wrong_collation",
            lambda: engine.sql(
                "ALTER TABLE telemetry_rollup_session DEFAULT CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci;",
                database=engine.database,
            ),
            lambda: engine.sql(
                "ALTER TABLE telemetry_rollup_session DEFAULT CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;",
                database=engine.database,
            ),
        ),
        (
            "weakened_same_named_kind_check",
            lambda: replace_rollup_check(engine, CHECK_KIND_NAME, "membership_kind IN (1,2)"),
            lambda: replace_rollup_check(
                engine,
                CHECK_KIND_NAME,
                "(membership_kind = 1 AND session_boot_id = 0 AND session_process_id = 0 AND session_seq = 0) "
                "OR (membership_kind = 2 AND session_boot_id <> 0 AND session_process_id <> 0 AND session_seq <> 0)",
            ),
        ),
        (
            "missing_subject_check",
            lambda: drop_rollup_check(engine, CHECK_SUBJECT_NAME),
            lambda: add_rollup_check(engine, CHECK_SUBJECT_NAME, "subject_id <> 0"),
        ),
    ]
    if "MariaDB" not in engine.server_version:
        scenarios.append(
            (
                "not_enforced_kind_check",
                lambda: engine.sql(
                    f"ALTER TABLE {COHORT_TABLE} ALTER CHECK {CHECK_KIND_NAME} NOT ENFORCED;",
                    database=engine.database,
                ),
                lambda: engine.sql(
                    f"ALTER TABLE {COHORT_TABLE} ALTER CHECK {CHECK_KIND_NAME} ENFORCED;",
                    database=engine.database,
                ),
            )
        )
    outcomes: dict[str, dict[str, object]] = {}
    for name, change, restore in scenarios:
        change()
        try:
            result = run_rollup_verifier(engine, check=False)
            output = redact((result.stdout + result.stderr).decode("utf-8", "replace")).strip()
            if result.returncode == 0:
                raise SchemaTestFailure(
                    f"{engine.label} 0017 verifier accepted synthetic drift {name}"
                )
            outcomes[name] = {"rejected": True, "failure_excerpt": output[-500:]}
        finally:
            restore()
        clean = run_rollup_verifier(engine, check=False)
        if clean.returncode:
            detail = redact((clean.stdout + clean.stderr).decode("utf-8", "replace"))
            raise SchemaTestFailure(
                f"{engine.label} 0017 verifier failed to restore synthetic drift {name}: {detail[-1500:]}"
            )
        after_restore = probe_rows(engine)
        if after_restore != engine.probe_before_replay:
            raise SchemaTestFailure(
                f"{engine.label} synthetic 0017 drift {name} did not preserve probe rows"
            )
        outcomes[name]["restored"] = True
        outcomes[name]["probe_rows_preserved"] = True
    if "MariaDB" in engine.server_version:
        outcomes["not_enforced_kind_check"] = {
            "supported": False,
            "reason": "MariaDB 10.11 exposes no ENFORCED metadata or ALTER CHECK enforcement state",
        }
    engine.rollup_drift_outcomes = outcomes
    return outcomes


def history_sql(manifest: object) -> tuple[str, str]:
    # Importing the existing runner keeps this setup's history contract identical
    # to the supported run_runtime_compatibility_mysql.sh plan.
    import migration_runner as runner  # type: ignore

    applied = [
        runner.AppliedMigration(
            step.migration_id, step.sequence, step.description,
            step.apply_checksum, step.verify_checksum, step.compatibility,
            manifest.runner_version,
        )
        for step in manifest.migrations
    ]
    checksum = runner.history_checksum(applied)

    def quoted(value: str) -> str:
        return "CONVERT(UNHEX('" + value.encode("utf-8").hex() + "') USING utf8mb4)"

    statements = [
        "INSERT INTO mud_schema_baselines"
        "(baseline_id,baseline_kind,schema_fingerprint,manifest_version,runner_version) VALUES("
        + quoted(manifest.baseline_id) + ",'fresh_bootstrap',UNHEX('"
        + manifest.required_table_fingerprint + "'),"
        + str(manifest.version) + "," + str(manifest.runner_version) + ");"
    ]
    for step in manifest.migrations:
        statements.append(
            "INSERT INTO mud_schema_history"
            "(migration_id,sequence_number,description,apply_checksum,verify_checksum,"
            "compatibility,runner_version) VALUES("
            + quoted(step.migration_id) + "," + str(step.sequence) + ","
            + quoted(step.description) + ",UNHEX('" + step.apply_checksum + "'),UNHEX('"
            + step.verify_checksum + "')," + quoted(step.compatibility) + ","
            + str(manifest.runner_version) + ");"
        )
    statements.append(
        "UPDATE mud_schema_migration_state SET applied_count="
        + str(len(applied)) + ",history_checksum=UNHEX('" + checksum
        + "') WHERE state_id=1;"
    )
    return " ".join(statements), checksum


def seed_history(engine: Engine, manifest: object) -> str:
    sql, checksum = history_sql(manifest)
    engine.sql(sql, database=engine.database)
    count = engine.sql("SELECT COUNT(*) FROM mud_schema_history;", database=engine.database)
    state = engine.sql(
        "SELECT applied_count,LOWER(HEX(history_checksum)) "
        "FROM mud_schema_migration_state WHERE state_id=1;",
        database=engine.database,
    )
    if count != str(len(manifest.migrations)) or state != f"{len(manifest.migrations)}\t{checksum}":
        raise SchemaTestFailure(
            f"{engine.label} migration history contract was not seeded exactly"
        )
    engine.history_checksum = checksum
    return checksum


def setup_full_schema(engine: Engine, manifest: object) -> dict[str, object]:
    engine.wait_ready()
    engine.create_database()
    original17_outcome = original17_key_limit_probe(engine)
    engine.sql_file(BOOTSTRAP)
    engine.bootstrap_shape = table_shape(engine)
    bootstrap_table_count = len(engine.sql(
        "SELECT table_name FROM information_schema.tables WHERE table_schema=DATABASE() "
        "AND table_type='BASE TABLE' ORDER BY table_name;", database=engine.database
    ).splitlines())
    if bootstrap_table_count != BOOTSTRAP_TABLE_COUNT:
        raise SchemaTestFailure(
            f"{engine.label} fresh bootstrap did not produce {BOOTSTRAP_TABLE_COUNT} "
            f"pre-immutable tables (found {bootstrap_table_count})"
        )

    # Make the new table creation observable: bootstrap supplies the reference
    # shape, then only these two disposable tables are removed before 0017.
    engine.sql(
        "DROP TABLE telemetry_cohort_member,telemetry_rollup_session;",
        database=engine.database,
    )
    verifier_paths = copy_migration_verifiers(engine, manifest.migrations)
    engine.rollup_verifier_path = verifier_paths["0017_telemetry_rollup_support"]
    first_apply_shape = None
    check_expression_contract = None
    for replay in (1, 2):
        for step in manifest.migrations:
            engine.sql_file(step.apply_path)
            result = run_migration_verifier(engine, verifier_paths[step.migration_id])
            if result.returncode:
                raise SchemaTestFailure(
                    f"{engine.label} migration verifier failed for {step.migration_id}"
                )
            if step.migration_id == "0017_telemetry_rollup_support":
                if replay == 1:
                    first_apply_shape = table_shape(engine)
                    if first_apply_shape != engine.bootstrap_shape:
                        raise SchemaTestFailure(
                            f"{engine.label} bootstrap/0017 table metadata parity differs"
                        )
                    engine.applied_shape = first_apply_shape
                    check_expression_contract = member_check_expression_contract(engine)
                    engine.member_constraint_outcomes = verify_member_identity_constraints(engine)
                    insert_probe_rows(engine)
                    engine.probe_before_replay = probe_rows(engine)
                else:
                    engine.probe_after_replay = probe_rows(engine)
                    if engine.probe_after_replay != engine.probe_before_replay:
                        raise SchemaTestFailure(
                            f"{engine.label} 0017 replay changed synthetic probe rows"
                        )
    if engine.probe_before_replay is None or engine.probe_after_replay is None:
        raise SchemaTestFailure(f"{engine.label} did not execute both 0017 passes")
    checksum = seed_history(engine, manifest)
    full_table_count = len(engine.sql(
        "SELECT table_name FROM information_schema.tables WHERE table_schema=DATABASE() "
        "AND table_type='BASE TABLE' ORDER BY table_name;", database=engine.database
    ).splitlines())
    if full_table_count != RUNTIME_TABLE_COUNT:
        raise SchemaTestFailure(
            f"{engine.label} full immutable setup did not produce {RUNTIME_TABLE_COUNT} tables "
            f"(found {full_table_count})"
        )
    return {
        "server_version": engine.server_version,
        "database": engine.database,
        "bootstrap_table_count": bootstrap_table_count,
        "full_table_count": full_table_count,
        "bootstrap_new_table_shape": list(engine.bootstrap_shape),
        "applied_new_table_shape": list(engine.applied_shape),
        "bootstrap_0017_exact_parity": engine.bootstrap_shape == engine.applied_shape,
        "new0017_apply_replay": True,
        "original17_key_limit": original17_outcome,
        "member_check_expression_contract": check_expression_contract,
        "member_identity_checks": engine.member_constraint_outcomes,
        "probe_rows_preserved": engine.probe_before_replay == engine.probe_after_replay,
        "history_checksum": checksum,
    }


def install_runtime_verifier(engine: Engine, manifest_path: Path) -> str:
    verifier_path = "/tmp/duris268_verify_runtime_compatibility.sh"
    manifest_destination = "/tmp/duris268_runtime_compatibility_manifest.json"
    engine.copy(RUNTIME_VERIFY, verifier_path)
    engine.exec(["chmod", "+x", verifier_path])
    engine.copy(manifest_path, manifest_destination)
    return manifest_destination


def compatibility_result(
    engine: Engine, verifier_path: str, manifest_path: str,
) -> subprocess.CompletedProcess[bytes]:
    return run_migration_verifier(
        engine,
        verifier_path,
        check=False,
    ) if manifest_path == "" else engine.exec([
        "-e", "ENVIRONMENT=test",
        "-e", "DB_HOST=127.0.0.1",
        "-e", "DB_PORT=3306",
        "-e", "DB_USER=root",
        "-e", f"DB_PASSWD={engine.password}",
        "-e", f"DB_NAME={engine.database}",
        "-e", f"RUNTIME_COMPATIBILITY_MANIFEST={manifest_path}",
        verifier_path,
    ], check=False)


def measure_fingerprint(engine: Engine, runtime_value: dict) -> str:
    zero_manifest = copy.deepcopy(runtime_value)
    zero_manifest["normalized_metadata_fingerprints"] = {
        "mysql8": "0" * 64,
        "mariadb10_11": "0" * 64,
    }
    with tempfile.TemporaryDirectory(prefix="duris268-runtime-") as directory:
        local_manifest = Path(directory) / "runtime_compatibility_manifest.json"
        local_manifest.write_text(json.dumps(zero_manifest, indent=2) + "\n")
        manifest_path = install_runtime_verifier(engine, local_manifest)
        result = compatibility_result(
            engine, "/tmp/duris268_verify_runtime_compatibility.sh", manifest_path,
        )
        output = redact(
            (result.stdout + result.stderr).decode("utf-8", "replace")
        )
        match = re.search(
            r"normalized metadata fingerprint mismatch: expected=[0-9a-f]{64} actual=([0-9a-f]{64})",
            output,
        )
        if result.returncode == 0 or match is None:
            raise SchemaTestFailure(
                f"{engine.label} exact runtime metadata query did not expose a measured "
                f"fingerprint: {output[-2500:]}"
            )
        return match.group(1)


def update_contract(measured: dict[str, str], migration_manifest: object) -> dict[str, object]:
    value = json.loads(RUNTIME_MANIFEST.read_text())
    import migration_runner as runner  # type: ignore

    applied = [
        runner.AppliedMigration(
            step.migration_id, step.sequence, step.description,
            step.apply_checksum, step.verify_checksum, step.compatibility,
            migration_manifest.runner_version,
        )
        for step in migration_manifest.migrations
    ]
    head = migration_manifest.migrations[-1]
    history = runner.history_checksum(applied)
    value["normalized_metadata_fingerprints"] = {
        "mysql8": measured["mysql8"],
        "mariadb10_11": measured["mariadb10_11"],
    }
    value["migration_head"] = {
        "id": head.migration_id,
        "sequence": head.sequence,
        "apply_checksum": head.apply_checksum,
        "verify_checksum": head.verify_checksum,
        "history_checksum": history,
    }
    RUNTIME_MANIFEST.write_text(json.dumps(value, indent=2) + "\n")

    header_path = ROOT / "src/core/runtime_compatibility_contract.h"
    header = header_path.read_text()

    def replace_multiline(name: str, replacement: str) -> None:
        nonlocal header
        pattern = rf'(constexpr const char \*{re.escape(name)} =\n\t")[^"]*(";)'
        header, count = re.subn(pattern, rf"\g<1>{replacement}\g<2>", header)
        if count != 1:
            raise SchemaTestFailure(f"expected one multiline header constant: {name}")

    def replace_inline(name: str, replacement: str) -> None:
        nonlocal header
        pattern = rf'(constexpr const char \*{re.escape(name)} = ")[^"]*(";)'
        header, count = re.subn(pattern, rf"\g<1>{replacement}\g<2>", header)
        if count != 1:
            raise SchemaTestFailure(f"expected one inline header constant: {name}")

    def replace_unsigned(name: str, replacement: str) -> None:
        nonlocal header
        pattern = rf'(constexpr unsigned {re.escape(name)} = )[0-9]+(;)'
        header, count = re.subn(pattern, rf"\g<1>{replacement}\g<2>", header)
        if count != 1:
            raise SchemaTestFailure(f"expected one unsigned header constant: {name}")

    replace_multiline("RUNTIME_MYSQL8_METADATA_FINGERPRINT", measured["mysql8"])
    replace_multiline("RUNTIME_MARIADB10_11_METADATA_FINGERPRINT", measured["mariadb10_11"])
    replace_inline("RUNTIME_MIGRATION_HEAD_ID", head.migration_id)
    replace_unsigned("RUNTIME_MIGRATION_HEAD_SEQUENCE", str(head.sequence))
    replace_multiline("RUNTIME_MIGRATION_APPLY_CHECKSUM", head.apply_checksum)
    replace_multiline("RUNTIME_MIGRATION_VERIFY_CHECKSUM", head.verify_checksum)
    replace_multiline("RUNTIME_MIGRATION_HISTORY_CHECKSUM", history)
    header_path.write_text(header)
    return {
        "runtime_manifest": str(RUNTIME_MANIFEST),
        "runtime_header": str(header_path),
        "migration_head": value["migration_head"],
    }


def run_clean_verifier(engine: Engine, runtime_manifest: Path) -> str:
    manifest_path = install_runtime_verifier(engine, runtime_manifest)
    result = compatibility_result(
        engine, "/tmp/duris268_verify_runtime_compatibility.sh", manifest_path,
    )
    output = redact((result.stdout + result.stderr).decode("utf-8", "replace")).strip()
    if result.returncode:
        raise SchemaTestFailure(
            f"{engine.label} clean runtime compatibility verification failed: {output[-2500:]}"
        )
    return output


def drift_matrix(engine: Engine, runtime_manifest: Path) -> dict[str, dict[str, object]]:
    install_runtime_verifier(engine, runtime_manifest)
    verifier = "/tmp/duris268_verify_runtime_compatibility.sh"
    drifts = (
        (
            "wrong_unsigned_type",
            "ALTER TABLE telemetry_rollup_session MODIFY generation BIGINT NOT NULL;",
            "ALTER TABLE telemetry_rollup_session MODIFY generation BIGINT UNSIGNED NOT NULL;",
        ),
        (
            "wrong_default",
            "ALTER TABLE telemetry_rollup_session MODIFY input_watermark BIGINT UNSIGNED NOT NULL DEFAULT 1;",
            "ALTER TABLE telemetry_rollup_session MODIFY input_watermark BIGINT UNSIGNED NOT NULL DEFAULT 0;",
        ),
        (
            "missing_primary_index",
            "ALTER TABLE telemetry_rollup_session DROP PRIMARY KEY;",
            "ALTER TABLE telemetry_rollup_session ADD PRIMARY KEY (" + CANONICAL_PRIMARY + ");",
        ),
        (
            "wrong_primary_index",
            "ALTER TABLE telemetry_rollup_session DROP PRIMARY KEY, ADD PRIMARY KEY "
            "(definition_version,generation,environment_id,season_id,session_boot_id,session_process_id);",
            "ALTER TABLE telemetry_rollup_session DROP PRIMARY KEY, ADD PRIMARY KEY ("
            + CANONICAL_PRIMARY + ");",
        ),
        (
            "missing_secondary_index",
            "ALTER TABLE telemetry_rollup_session DROP INDEX idx_rollup_session_subject;",
            "ALTER TABLE telemetry_rollup_session ADD KEY idx_rollup_session_subject ("
            + CANONICAL_SECONDARY + ");",
        ),
        (
            "wrong_secondary_index",
            "ALTER TABLE telemetry_rollup_session DROP INDEX idx_rollup_session_subject, ADD KEY "
            "idx_rollup_session_subject (definition_version,generation,environment_id,season_id,"
            "subject_id,session_boot_id,session_process_id);",
            "ALTER TABLE telemetry_rollup_session DROP INDEX idx_rollup_session_subject, ADD KEY "
            "idx_rollup_session_subject (" + CANONICAL_SECONDARY + ");",
        ),
        (
            "wrong_engine",
            "ALTER TABLE telemetry_rollup_session ENGINE=MyISAM;",
            "ALTER TABLE telemetry_rollup_session ENGINE=InnoDB;",
        ),
        (
            "wrong_collation",
            "ALTER TABLE telemetry_rollup_session DEFAULT CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci;",
            "ALTER TABLE telemetry_rollup_session DEFAULT CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;",
        ),
    )
    outcomes: dict[str, dict[str, object]] = {}
    for name, change, restore in drifts:
        engine.sql(change, database=engine.database)
        try:
            result = compatibility_result(engine, verifier, "/tmp/duris268_runtime_compatibility_manifest.json")
            output = redact((result.stdout + result.stderr).decode("utf-8", "replace")).strip()
            if result.returncode == 0:
                raise SchemaTestFailure(
                    f"{engine.label} verifier accepted synthetic drift {name}"
                )
            outcomes[name] = {
                "rejected": True,
                "failure_excerpt": output[-500:],
            }
        finally:
            engine.sql(restore, database=engine.database)
        clean = compatibility_result(engine, verifier, "/tmp/duris268_runtime_compatibility_manifest.json")
        if clean.returncode:
            detail = redact((clean.stdout + clean.stderr).decode("utf-8", "replace"))
            raise SchemaTestFailure(
                f"{engine.label} failed to restore synthetic drift {name}: {detail[-1500:]}"
            )
        after_restore = probe_rows(engine)
        if after_restore != engine.probe_before_replay:
            raise SchemaTestFailure(
                f"{engine.label} synthetic drift {name} did not preserve probe rows"
            )
        outcomes[name]["restored"] = True
        outcomes[name]["probe_rows_preserved"] = True
    engine.drift_outcomes = outcomes
    return outcomes


def validate_local_contract() -> str:
    result = subprocess.run(
        [sys.executable, str(VALIDATOR)],
        cwd=ROOT, capture_output=True, check=False,
    )
    output = redact((result.stdout + result.stderr).decode("utf-8", "replace")).strip()
    if result.returncode:
        raise SchemaTestFailure(
            f"validate_runtime_compatibility.py failed: {output[-2500:]}"
        )
    return output


def cleanup_engine(engine: Engine, keep: bool) -> str:
    if keep:
        return "kept"
    actions = []
    try:
        engine.drop_database()
        actions.append("database_removed")
    except Exception as error:  # pragma: no cover - cleanup is reported, not hidden
        actions.append("database_cleanup_failed: " + redact(str(error))[-500:])
    if engine.container.startswith("duris-268-rollup-mysql8-schema-"):
        result = run_process(["docker", "rm", "-f", engine.container], check=False)
        if result.returncode == 0:
            actions.append("container_removed")
        else:
            actions.append("container_cleanup_failed")
    return "; ".join(actions)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--credentials", type=Path, required=True,
        help="private task settings JSON (container, database, root); never committed or printed",
    )
    parser.add_argument("--report", type=Path, default=ROOT / "tests/async/telemetry_rollup_schema_mysql.report.json")
    parser.add_argument(
        "--update-contract", action="store_true",
        help="write measured fingerprints and current migration head to the owned manifest/header",
    )
    parser.add_argument("--keep", action="store_true", help="keep disposable DBs/container for inspection")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    credentials = json.loads(args.credentials.read_text())
    protected_database = credentials.get("database")
    protected_container = credentials.get("container")
    if not isinstance(protected_database, str) or not isinstance(protected_container, str):
        raise SchemaTestFailure("disposable DB credential file has an invalid scope")

    sys.path.insert(0, str(ROOT / "scripts"))
    import migration_runner as runner  # type: ignore

    manifest = runner.load_manifest()
    runtime_value = json.loads(RUNTIME_MANIFEST.read_text())
    if protected_database != "duris_268_test":
        raise SchemaTestFailure("unexpected configured task database scope")

    mariadb = start_mariadb(credentials)
    mysql8 = start_mysql8()
    engines = [mariadb, mysql8]
    engine_reports: dict[str, dict[str, object]] = {}
    report: dict[str, object] = {
        "status": "running",
        "protected_task_container": protected_container,
        "protected_task_database": protected_database,
        "engines": engine_reports,
        "commands": {
            "invocation": shlex.join(["python3", "tests/async/telemetry_rollup_schema_mysql.py", *sys.argv[1:]]),
            "bootstrap": "docker exec -i -e MYSQL_PWD=<root-password> <container> mysql --protocol=tcp -h127.0.0.1 -P3306 -uroot -N -B --raw <database> < migrations/bootstrap_multithread_safe.sql",
            "immutable_apply": "docker exec -i -e MYSQL_PWD=<root-password> <container> mysql --protocol=tcp -h127.0.0.1 -P3306 -uroot -N -B --raw <database> < migrations/immutable/NNNN_*.sql",
            "immutable_verify": "docker exec -e ENVIRONMENT=test -e DB_HOST=127.0.0.1 -e DB_PORT=3306 -e DB_USER=root -e DB_PASSWD=<root-password> -e DB_NAME=<database> <container> /tmp/duris268_NNNN_*.sh",
            "runtime_verify": "docker exec -e ENVIRONMENT=test -e DB_HOST=127.0.0.1 -e DB_PORT=3306 -e DB_USER=root -e DB_PASSWD=<root-password> -e DB_NAME=<database> -e RUNTIME_COMPATIBILITY_MANIFEST=/tmp/duris268_runtime_compatibility_manifest.json <container> /tmp/duris268_verify_runtime_compatibility.sh",
            "validator": "python3 scripts/validate_runtime_compatibility.py",
        },
        "artifact": str(Path(__file__).resolve()),
        "contract_update_requested": args.update_contract,
    }
    try:
        measured: dict[str, str] = {}
        cross_engine_check_contract: dict[str, str] | None = None
        for engine in engines:
            engine_reports[engine.label] = setup_full_schema(engine, manifest)
            check_contract = engine_reports[engine.label]["member_check_expression_contract"]
            if not isinstance(check_contract, dict):
                raise SchemaTestFailure(f"{engine.label} did not report CHECK expression metadata")
            normalized = check_contract.get("normalized_expressions")
            if not isinstance(normalized, dict):
                raise SchemaTestFailure(f"{engine.label} did not report normalized CHECK expressions")
            if cross_engine_check_contract is None:
                cross_engine_check_contract = normalized
            elif normalized != cross_engine_check_contract:
                raise SchemaTestFailure(
                    f"dual-engine normalized CHECK expressions differ: {normalized} vs {cross_engine_check_contract}"
                )
            fingerprint_key = "mysql8" if engine.label == "MySQL 8.0" else "mariadb10_11"
            engine.measured_fingerprint = measure_fingerprint(engine, runtime_value)
            measured[fingerprint_key] = engine.measured_fingerprint
            engine_reports[engine.label]["measured_fingerprint"] = engine.measured_fingerprint
        report["cross_engine_normalized_check_expressions"] = cross_engine_check_contract
        report["measured_normalized_metadata_fingerprints"] = measured
        expected_head = None
        if args.update_contract:
            expected_head = update_contract(measured, manifest)
        else:
            current = json.loads(RUNTIME_MANIFEST.read_text())
            if current["normalized_metadata_fingerprints"] != measured:
                raise SchemaTestFailure(
                    "measured fingerprints differ from the checked-in manifest; rerun with --update-contract"
                )
        report["contract_update"] = expected_head or {
            "runtime_manifest": str(RUNTIME_MANIFEST),
            "runtime_header": str(ROOT / "src/core/runtime_compatibility_contract.h"),
            "migration_head": json.loads(RUNTIME_MANIFEST.read_text())["migration_head"],
        }
        # The updated manifest must be copied only after both engine measurements
        # are complete, so each clean verifier checks the same two-engine contract.
        runtime_manifest = RUNTIME_MANIFEST
        for engine in engines:
            engine_reports[engine.label]["0017_verifier_drift_outcomes"] = rollup_verifier_drift_matrix(engine)
            clean_output = run_clean_verifier(engine, runtime_manifest)
            engine_reports[engine.label]["clean_runtime_verifier"] = clean_output
            engine_reports[engine.label]["drift_outcomes"] = drift_matrix(engine, runtime_manifest)
        report["validation"] = validate_local_contract()
        report["status"] = "passed"
    except Exception as error:
        report["status"] = "failed"
        report["error"] = redact(str(error))
        raise
    finally:
        cleanup = {}
        for engine in engines:
            cleanup[engine.label] = {
                "container": engine.container,
                "database": engine.database,
                "action": cleanup_engine(engine, args.keep),
            }
        report["cleanup"] = cleanup
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n")
        print(json.dumps(report, indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
