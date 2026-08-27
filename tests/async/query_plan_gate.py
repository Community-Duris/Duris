#!/usr/bin/env python3
"""Fail-closed, aggregate-only query-plan qualification for a local clone."""

import argparse
import json
import os
import re
import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
MANIFEST = ROOT / "tests/async/query_plan_manifest.json"
SAFE_ENVIRONMENTS = {"dev", "development", "local", "test"}
LOOPBACK_HOSTS = {"localhost", "127.0.0.1", "::1"}
IDENTIFIER = re.compile(r"^[a-z][a-z0-9_]*$")


def read_env(path):
    values = {}
    for raw in path.read_text().splitlines():
        line = raw.strip()
        if not line or line.startswith("#") or "=" not in line:
            continue
        key, value = line.split("=", 1)
        values[key.strip()] = value.strip().strip("\"'")
    return values


def classify(values):
    environment = values.get("ENVIRONMENT", "").lower()
    host = values.get("DB_HOST", "").lower()
    return environment in SAFE_ENVIRONMENTS and host in LOOPBACK_HOSTS


def load_manifest(path=MANIFEST):
    manifest = json.loads(path.read_text())
    assert manifest["schema_version"] == 1
    ids = [entry["id"] for entry in manifest["queries"]]
    assert len(ids) == len(set(ids))
    assert all(re.fullmatch(r"[A-Z][A-Z0-9_]{3,31}", query_id) for query_id in ids)
    for entry in manifest["queries"]:
        assert entry["parameter_types"] and entry["ordering"] and entry["acceptance"]
        assert all(IDENTIFIER.fullmatch(table) for table in entry["tables"])
        assert all(isinstance(limit, int) and limit > 0 for limit in entry["tables"].values())
    return manifest


def mysql_query(values, sql):
    child_env = os.environ.copy()
    child_env["MYSQL_PWD"] = values["DB_PASSWD"]
    command = [
        "mysql", "--batch", "--skip-column-names", "--raw",
        "--host", values["DB_HOST"], "--port", values["DB_PORT"],
        "--user", values["DB_USER"], values["DB_NAME"], "--execute", sql,
    ]
    return subprocess.check_output(command, cwd=ROOT, env=child_env, text=True, timeout=30)


def qualify(values, manifest):
    tables = sorted({table for entry in manifest["queries"] for table in entry["tables"]})
    sql = " UNION ALL ".join(
        f"SELECT '{table}',COUNT(*) FROM `{table}`" for table in tables
    )
    counts = {}
    for line in mysql_query(values, sql).splitlines():
        table, count = line.split("\t", 1)
        counts[table] = int(count)
    results = []
    for entry in manifest["queries"]:
        missing = {
            table: {"observed": counts.get(table, 0), "required": required}
            for table, required in entry["tables"].items()
            if counts.get(table, 0) < required
        }
        results.append({
            "id": entry["id"],
            "status": "unmeasured" if missing else "qualified",
            "qualification": missing or "passed",
            "candidate": entry["candidate"],
        })
    return counts, results


PLAN_KEYS = {
    "access_type", "key", "possible_keys", "rows", "filtered",
    "using_filesort", "using_temporary_table", "table_name",
}


def sanitize_plan(value):
    if isinstance(value, dict):
        return {
            key: sanitize_plan(item)
            for key, item in value.items()
            if key in PLAN_KEYS or isinstance(item, (dict, list))
        }
    if isinstance(value, list):
        return [sanitize_plan(item) for item in value]
    return value


def capture_qualified_plans(values, manifest, results, output_dir):
    output_dir.mkdir(parents=True, exist_ok=True)
    by_id = {entry["id"]: entry for entry in manifest["queries"]}
    for result in results:
        if result["status"] != "qualified":
            continue
        raw = mysql_query(values, "EXPLAIN FORMAT=JSON " + by_id[result["id"]]["explain_sql"])
        plan = sanitize_plan(json.loads(raw))
        (output_dir / f"{result['id']}.sanitized.json").write_text(
            json.dumps(plan, indent=2, sort_keys=True) + "\n"
        )
        result["status"] = "baseline-measured"


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", default="tmp/query-plan-gate/result.json")
    args = parser.parse_args()
    values = read_env(ROOT / ".env")
    if not classify(values):
        raise SystemExit("query-plan gate refused: target is not non-production loopback")
    manifest = load_manifest()
    counts, results = qualify(values, manifest)
    output = ROOT / args.output
    output.parent.mkdir(parents=True, exist_ok=True)
    capture_qualified_plans(values, manifest, results, output.parent)
    payload = {
        "schema_version": 1,
        "target": "non-production-loopback",
        "aggregate_table_counts": counts,
        "queries": results,
        "migration_allowed": all(item["status"] == "accepted" for item in results),
    }
    output.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n")
    print("query-plan gate completed; aggregate evidence written to ignored output")


if __name__ == "__main__":
    main()
