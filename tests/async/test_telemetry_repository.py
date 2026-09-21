#!/usr/bin/env python3
"""Compile repository boundary tests and optionally execute disposable SQL.

SQL execution requires an explicitly disposable loopback MariaDB/MySQL fixture,
TELEMETRY_REPOSITORY_DISPOSABLE=1, and a unique database name beginning with
``duris_telemetry_test_``.  The fixture loads the authoritative bootstrap,
adopts its sealed baseline, and applies/verifies the complete immutable migration
manifest before running the repository.  No checkout credentials are used.
"""
from __future__ import annotations

import argparse
import copy
import json
import os
from pathlib import Path
import re
import shlex
import subprocess
import sys
import tempfile

from test_telemetry_contract_fixtures import canonical_config_fingerprint

ROOT = Path(__file__).resolve().parents[2]
FIXTURES = ROOT / "tests/async/fixtures/telemetry/contract"
ENUMS = {
    "kind": "telemetry_record_kind", "category": "telemetry_interval_category",
    "context": "telemetry_activity_context", "context_quality": "telemetry_context_quality",
    "lifecycle": "telemetry_lifecycle_kind", "end_reason": "telemetry_session_end_reason",
    "reason": "telemetry_gap_reason", "backend": "telemetry_storage_backend",
}
PAYLOADS = {"session_lifecycle": "lifecycle", "session_checkpoint": "checkpoint", "coverage_gap": "gap"}
DATABASE_PATTERN = re.compile(r"duris_telemetry_test_[a-z0-9_]{12,80}\Z")


def function_body(source: str, start: str, end: str) -> str:
    return source[source.index(start) : source.index(end, source.index(start))]


def mapped_columns(source: str) -> list[str]:
    columns = []
    for line in source.splitlines():
        explicit = re.search(r'number\(values, "([a-z0-9_]+)"', line)
        inferred = re.search(r'FIELD\(values, [^,]+, ([a-z0-9_]+)\)', line)
        if explicit:
            columns.append(explicit.group(1))
        elif inferred:
            columns.append(inferred.group(1))
    return columns


def migration_columns(name: str) -> list[str]:
    migration = (ROOT / "migrations" / "immutable" / name).read_text()
    return re.findall(r"ADD COLUMN ([a-z0-9_]+)", migration)


def repository_mapping_contract() -> None:
    repository = (ROOT / "src" / "telemetry" / "telemetry_repository.c").read_text()
    encounter = function_body(repository, "void encounter_fields", "void combat_summary_fields")
    combat = function_body(repository, "void combat_summary_fields", "fields counter_fields")
    progression = function_body(
        repository,
        "case telemetry_record_kind::progression:",
        "case telemetry_record_kind::encounter:",
    )

    engine = os.environ.get("TELEMETRY_REPOSITORY_DB_IMAGE", "source-contract")
    progression_schema = migration_columns("0022_telemetry_progression.sql")
    progression_mapped = mapped_columns(progression)
    missing = sorted(set(progression_schema) - set(progression_mapped))
    generic = sorted(set(column.removeprefix("progression_") for column in progression_schema) &
                     set(progression_mapped))
    assert not missing, (f"engine={engine} migration=0022_telemetry_progression "
                         f"record_kind=6 missing_columns={','.join(missing)}")
    assert not generic, (f"engine={engine} migration=0022_telemetry_progression "
                         f"record_kind=6 generic_columns={','.join(generic)}")

    encounter_schema = migration_columns("0024_telemetry_encounters.sql")
    encounter_mapped = mapped_columns(encounter)
    assert encounter_mapped == encounter_schema, (
        f"engine={engine} migration=0024_telemetry_encounters record_kind=7 "
        f"columns_mapped={encounter_mapped!r} columns_schema={encounter_schema!r}")
    generic = sorted({"start_monotonic_usec", "start_utc_usec", "quality_flags"} &
                     set(encounter_mapped))
    assert not generic, (f"engine={engine} migration=0024_telemetry_encounters "
                         f"record_kind=7 generic_columns={','.join(generic)}")

    combat_schema = migration_columns("0025_telemetry_combat_summaries.sql")
    combat_mapped = mapped_columns(combat)
    assert combat_mapped == combat_schema, (
        f"engine={engine} migration=0025_telemetry_combat_summaries record_kind=8 "
        f"columns_mapped={combat_mapped!r} columns_schema={combat_schema!r}")
    assert "FIELD(values, summary" not in combat, (
        f"engine={engine} migration=0025_telemetry_combat_summaries "
        "record_kind=8 unprefixed_FIELD_mapping")


def sql_environment() -> tuple[dict[str, str], list[str], str]:
    required = {
        "TELEMETRY_REPOSITORY_HOST": "DB_HOST",
        "TELEMETRY_REPOSITORY_PORT": "DB_PORT",
        "TELEMETRY_REPOSITORY_USER": "DB_USER",
        "TELEMETRY_REPOSITORY_PASSWORD": "DB_PASSWD",
        "TELEMETRY_REPOSITORY_DATABASE": "DB_NAME",
    }
    missing = [name for name in required if not os.environ.get(name)]
    if missing:
        raise RuntimeError("disposable SQL fixture is missing: " + ", ".join(missing))
    host = os.environ["TELEMETRY_REPOSITORY_HOST"]
    database = os.environ["TELEMETRY_REPOSITORY_DATABASE"]
    if not DATABASE_PATTERN.fullmatch(database):
        raise RuntimeError("disposable SQL fixture database name is not uniquely test-scoped")
    if host != "127.0.0.1":
        raise RuntimeError("disposable SQL fixture host must be explicit TCP loopback")
    try:
        port = int(os.environ["TELEMETRY_REPOSITORY_PORT"])
    except ValueError as error:
        raise RuntimeError("disposable SQL fixture port is not numeric") from error
    if not 1 <= port <= 65535:
        raise RuntimeError("disposable SQL fixture port is outside 1..65535")
    environment = dict(os.environ)
    environment.pop("DB_SOCKET", None)
    environment.pop("MYSQL_UNIX_PORT", None)
    environment.update({
        "ENVIRONMENT": "test",
        "DB_HOST": host,
        "DB_PORT": str(port),
        "DB_USER": os.environ["TELEMETRY_REPOSITORY_USER"],
        "DB_PASSWD": os.environ["TELEMETRY_REPOSITORY_PASSWORD"],
        "DB_NAME": database,
        "DB_ALLOWED_TARGETS": f"{host}/{database}",
        "DB_TLS": "FALSE",
        "MYSQL_PWD": os.environ["TELEMETRY_REPOSITORY_PASSWORD"],
    })
    command = ["mysql", "--protocol=tcp", "-h", host, "-P", str(port), "-u",
               os.environ["TELEMETRY_REPOSITORY_USER"], "-N", "-B", "--raw"]
    return environment, command, database


def prepare_sql_fixture() -> tuple[dict[str, str], list[str], str]:
    environment, command, database = sql_environment()
    engine = os.environ.get("TELEMETRY_REPOSITORY_DB_IMAGE", "unknown-engine")
    print(f"Preparing disposable SQL fixture: engine={engine} database={database}", flush=True)
    existing = subprocess.check_output(
        command + ["-e", ("SELECT COUNT(*) FROM information_schema.schemata WHERE "
                           f"schema_name='{database}'")],
        text=True, env=environment).strip()
    if existing != "0":
        raise RuntimeError(f"engine={engine} disposable database already exists: {database}")
    subprocess.run(command + ["-e", (f"CREATE DATABASE `{database}` CHARACTER SET utf8mb4 "
                                           "COLLATE utf8mb4_unicode_ci")],
                   check=True, env=environment)
    try:
        subprocess.run(command + [database],
                       input=(ROOT / "migrations/bootstrap_multithread_safe.sql").read_bytes(),
                       check=True, env=environment)
        subprocess.run([sys.executable, "scripts/migration_runner.py", "adopt", "--kind",
                        "fresh_bootstrap"], cwd=ROOT, env=environment, check=True)
        subprocess.run([sys.executable, "scripts/migration_runner.py", "run"], cwd=ROOT,
                       env=environment, check=True)
        manifest = json.loads((ROOT / "migrations/migration_manifest.json").read_text())
        expected_count = len(manifest["migrations"])
        expected_head = manifest["migrations"][-1]["id"]
        actual = subprocess.check_output(
            command + [database, "-e", ("SELECT COUNT(*),COALESCE(MAX(migration_id),'') "
                                         "FROM mud_schema_history")],
            text=True, env=environment).strip()
        if actual != f"{expected_count}\t{expected_head}":
            raise RuntimeError(
                f"engine={engine} migration=manifest-head expected="
                f"{expected_count}:{expected_head} actual={actual!r}")
        environment["TELEMETRY_REPOSITORY_MIGRATION_COUNT"] = str(expected_count)
        print(f"Immutable migration chain: PASS ({engine}, {expected_count} steps through "
              f"{expected_head})", flush=True)
        return environment, command, database
    except Exception:
        subprocess.run(command + ["-e", f"DROP DATABASE IF EXISTS `{database}`"],
                       check=False, env=environment)
        raise


def drop_sql_fixture(environment: dict[str, str], command: list[str], database: str) -> None:
    subprocess.run(command + ["-e", f"DROP DATABASE IF EXISTS `{database}`"],
                   check=True, env=environment)


def assignments(value, target):
    lines = []
    for key, item in value.items():
        field = f"{target}.{PAYLOADS.get(key, key) if target.endswith('.payload') else key}"
        if isinstance(item, dict):
            lines += assignments(item, field)
        elif key == "fingerprint":
            lines += [f"{field}[{i}] = {byte}U;" for i, byte in enumerate(bytes.fromhex(item))]
        elif isinstance(item, list):
            lines += [f"{field}[{i}] = {x}U;" for i, x in enumerate(item)]
        elif isinstance(item, str):
            lines.append(f"{field} = {ENUMS[key]}::{item};")
        else:
            lines.append(f"{field} = {item}{'LL' if item < 0 else 'ULL'};")
    return lines


def record_expr(record):
    return "[] { telemetry_record r{}; " + " ".join(assignments(record, "r")) + " return r; }()"


def config_record(config, index):
    return {"header": {"schema_version": 1, "kind": "configuration", "reserved": 0,
                      "key": {"producer": {"boot_id": 900000, "process_id": 900001},
                              "record_seq": index + 1}, "occurrence_utc_usec": config["effective_utc_usec"]},
            "payload": {"configuration": {"config": config}}}


def generate():
    lines = ["// Generated from reviewed contract JSON fixtures; not a second oracle."]
    fixtures = [json.loads(p.read_text()) for p in sorted(FIXTURES.glob("*.json"))]
    for fixture in fixtures:
        name = fixture["fixture_id"]
        lines += [f"static const std::vector<telemetry_record> {name}_configs = {{"]
        lines += [record_expr(config_record(c, i)) + "," for i, c in enumerate(fixture["configurations"])]
        lines += ["};", f"static const telemetry_record {name}_records[] = {{"]
        lines += [record_expr(r) + "," for r in fixture["records"]]
        lines += ["};"]
    normal_config = next(f for f in fixtures if f["fixture_id"] == "normal_interval")["configurations"][0]
    for index, field in enumerate(("environment_id", "season_id", "interval_usec")):
        config = copy.deepcopy(normal_config)
        config[field] += 1
        config["config_id"] += 1000 + index
        config["fingerprint"] = canonical_config_fingerprint(config)
        lines += [f"static const telemetry_record changed_{field}_config = " + record_expr(config_record(config, 100 + index)) + ";"]
    lines += ["static void golden_tests() {"]
    for fixture in fixtures:
        name = fixture["fixture_id"]
        lines += ["{", f'case_name = "golden:{name}";', "reset_fixture();",
                  f"for (const auto &r : {name}_configs) expect_one(r, telemetry_apply_outcome::applied);"]
        expected = {e["name"]: e for e in fixture["expected"]["operations"]}
        for operation in fixture["operations"]:
            exp = expected[operation["name"]]
            indexes = operation["record_indexes"]
            lines += ["{", "const telemetry_record batch[] = {" + ",".join(f"{name}_records[{i}]" for i in indexes) + "};"]
            if operation.get("acknowledgement") == "ambiguous":
                lines += ["fault = fault_kind::commit_lost_committed;"]
            lines += [f'case_name = "golden:{name}:{operation["name"]}";',
                      "const auto result = telemetry_repository_apply(batch, std::size(batch));",
                      f"CHECK(result.outcome == telemetry_batch_outcome::{exp['batch_outcome']});",
                      f"CHECK(result.result_count == {len(indexes)}U);"]
            for i, outcome in enumerate(exp["record_outcomes"]):
                lines += [f"CHECK(result.results[{i}].outcome == telemetry_apply_outcome::{outcome});"]
            lines += ["}"]
        metrics = fixture["expected"]["metrics"]
        if "conservation" in metrics:
            total = metrics["conservation"]["interval_total_usec"]
            lines += [f'CHECK(scalar("SELECT COALESCE(SUM(duration_usec),0) FROM telemetry_interval WHERE record_kind=1") == {total}ULL);']
        checkpoint = metrics.get("checkpoint", {})
        if "latest_revision" in checkpoint:
            lines += [f'CHECK(scalar("SELECT MAX(latest_revision) FROM telemetry_session") == {checkpoint["latest_revision"]}ULL);']
            for field, total in checkpoint["latest_cumulative"].items():
                lines += [f'CHECK(scalar("SELECT SUM({field}) FROM telemetry_session") == {total}ULL);']
        lines += ['CHECK(scalar("SELECT COUNT(*) FROM telemetry_player_day") == 0U);',
                  'CHECK(scalar("SELECT COUNT(*) FROM telemetry_cohort_day") == 0U);',
                  'CHECK(scalar("SELECT COUNT(*) FROM telemetry_rollup_state") == 0U);',
                  "}"]
    lines += ["}"]
    return "\n".join(lines) + "\n"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sql-fixture", action="store_true")
    args = parser.parse_args()
    if args.sql_fixture and os.environ.get("TELEMETRY_REPOSITORY_DISPOSABLE") != "1":
        parser.error("--sql-fixture requires TELEMETRY_REPOSITORY_DISPOSABLE=1; database reset is destructive")
    repository_mapping_contract()
    compiler = shlex.split(os.environ.get("CXX", "g++"))
    output_root = ROOT / "bin/tests"
    output_root.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="telemetry-repository-", dir=output_root) as tmp:
        tmp = Path(tmp)
        (tmp / "telemetry_repository_golden.inc").write_text(generate())
        common = compiler + ["-std=c++20", "-Wall", "-Wextra", "-Werror", "-pedantic", "-pthread",
                             "-I", str(ROOT / "src"), "-I", str(tmp)]
        source = str(ROOT / "src/telemetry/telemetry_repository.c")
        failure_source = str(ROOT / "src/telemetry/telemetry_failure.c")
        harness = str(ROOT / "tests/async/telemetry_repository_harness.cc")
        no_sql = tmp / "no_mysql"
        subprocess.run(common + ["-D__NO_MYSQL__", failure_source, source, harness,
                                 "-o", str(no_sql)], check=True)
        subprocess.run([str(no_sql)], check=True, timeout=30)
        mysql = shlex.split(subprocess.check_output(["mysql_config", "--cflags", "--libs"], text=True)) + ["-lcrypto"]
        sql = tmp / "sql"
        subprocess.run(common + [failure_source, source, harness, "-Wl,--wrap=mysql_real_query", "-Wl,--wrap=mysql_errno", "-Wl,--wrap=_Znwm",
                                  "-o", str(sql)] + mysql, check=True)
        print("SQL repository harness compile: PASS", flush=True)
        if args.sql_fixture:
            environment, command, database = prepare_sql_fixture()
            try:
                subprocess.run([str(sql)], check=True, timeout=180, env=environment)
            finally:
                drop_sql_fixture(environment, command, database)
        else:
            print("SQL runtime: NOT REQUESTED (the required workflow uses --sql-fixture)")


if __name__ == "__main__":
    main()
