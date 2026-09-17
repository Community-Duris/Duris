#!/usr/bin/env python3
"""Compile repository boundary tests; --sql-fixture resets ONLY duris_telemetry_test.

SQL execution requires an explicitly disposable loopback MariaDB/MySQL fixture
and TELEMETRY_REPOSITORY_DISPOSABLE=1. TELEMETRY_REPOSITORY_PORT permits only 3306
(default MariaDB fixture) or 3307 (MySQL 8.4 fixture). No game, Redis, pool, or production credentials
are used. Default execution runs the SQL-free harness and only builds SQL tests.
"""
from __future__ import annotations

import argparse
import copy
import json
import os
from pathlib import Path
import shlex
import subprocess
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
    if os.environ.get("TELEMETRY_REPOSITORY_PORT", "3306") not in {"3306", "3307"}:
        parser.error("TELEMETRY_REPOSITORY_PORT must be 3306 or 3307 for the disposable fixtures")
    compiler = shlex.split(os.environ.get("CXX", "g++"))
    output_root = ROOT / "bin/tests"
    output_root.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="telemetry-repository-", dir=output_root) as tmp:
        tmp = Path(tmp)
        (tmp / "telemetry_repository_golden.inc").write_text(generate())
        common = compiler + ["-std=c++20", "-Wall", "-Wextra", "-Werror", "-pedantic", "-pthread",
                             "-I", str(ROOT / "src"), "-I", str(tmp)]
        source = str(ROOT / "src/telemetry/telemetry_repository.c")
        harness = str(ROOT / "tests/async/telemetry_repository_harness.cc")
        no_sql = tmp / "no_mysql"
        subprocess.run(common + ["-D__NO_MYSQL__", source, harness, "-o", str(no_sql)], check=True)
        subprocess.run([str(no_sql)], check=True, timeout=30)
        mysql = shlex.split(subprocess.check_output(["mysql_config", "--cflags", "--libs"], text=True)) + ["-lcrypto"]
        sql = tmp / "sql"
        subprocess.run(common + [source, harness, "-Wl,--wrap=mysql_real_query", "-Wl,--wrap=mysql_errno", "-Wl,--wrap=_Znwm",
                                  "-o", str(sql)] + mysql, check=True)
        print("SQL repository harness compile: PASS", flush=True)
        if args.sql_fixture:
            subprocess.run([str(sql), str(ROOT / "migrations/immutable/0014_telemetry_storage.sql"),
                            str(ROOT / "migrations/immutable/0020_telemetry_progression.sql")],
                           check=True, timeout=120)
        else:
            print("SQL runtime: SKIPPED (use --sql-fixture with disposable fixture acknowledgement)")


if __name__ == "__main__":
    main()
