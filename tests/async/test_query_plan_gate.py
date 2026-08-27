#!/usr/bin/env python3
"""Safety and manifest contracts for the production-clone query-plan gate."""

import importlib.util
import json
import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
MODULE_PATH = ROOT / "tests/async/query_plan_gate.py"
spec = importlib.util.spec_from_file_location("query_plan_gate", MODULE_PATH)
gate = importlib.util.module_from_spec(spec)
spec.loader.exec_module(gate)

manifest = gate.load_manifest()
assert len(manifest["queries"]) >= 8
assert gate.classify({"ENVIRONMENT": "development", "DB_HOST": "127.0.0.1"})
assert not gate.classify({"ENVIRONMENT": "production", "DB_HOST": "127.0.0.1"})
assert not gate.classify({"ENVIRONMENT": "development", "DB_HOST": "db.example"})

text = MODULE_PATH.read_text()
assert "MYSQL_PWD" in text
assert "--password" not in text
assert "DROP TABLE" not in text and "ALTER TABLE" not in text
assert "SELECT '{table}',COUNT(*)" in text
assert "EXPLAIN FORMAT=JSON" in text
assert gate.sanitize_plan({"query_block": {"table": {"table_name": "t", "rows": 2,
                                                       "attached_condition": "secret"}}}) == {
    "query_block": {"table": {"table_name": "t", "rows": 2}}
}
assert "tmp/query-plan-gate" in (ROOT / ".gitignore").read_text()

bootstrap = (ROOT / "migrations/bootstrap_multithread_safe.sql").read_text()
for entry in manifest["queries"]:
    for table in entry["tables"]:
        assert f"CREATE TABLE `{table}`" in bootstrap

tracked = set(subprocess.check_output(["git", "ls-files"], cwd=ROOT, text=True).splitlines())
assert not any(path.startswith("tmp/query-plan-gate/") for path in tracked)
assert json.loads((ROOT / "tests/async/query_plan_manifest.json").read_text()) == manifest

print("query-plan gate safety and manifest contracts passed")
