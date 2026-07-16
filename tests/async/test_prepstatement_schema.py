#!/usr/bin/env python3
"""Checks the runtime prep-statement schema contract across schema sources."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
bootstrap = (ROOT / "migrations/bootstrap_multithread_safe.sql").read_text()
runner = (ROOT / "migrations/run_migration.sh").read_text()
root_entrypoint = (ROOT / "run_migration.sh").read_text()
dump = (ROOT / "src/duris.sql").read_text()
sql = (ROOT / "src/sql.c").read_text()

assert "create table `prepstatement_duris_sql`" in bootstrap.lower()
assert "prepstatment_duris_sql" not in bootstrap.lower().replace(
    "-- older deployments may contain the historical prepstatment_duris_sql table with", ""
)
assert "`description` text" in bootstrap.lower()
assert "`sql_code` text" in bootstrap.lower()
assert "CREATE TABLE IF NOT EXISTS prepstatement_duris_sql" in runner
assert "description TEXT DEFAULT NULL" in runner
assert "sql_code TEXT DEFAULT NULL" in runner
assert "CREATE TABLE `prepstatement_duris_sql`" in dump
assert "`description` text" in dump
assert "`sql_code` text" in dump
assert "SELECT id, description FROM prepstatement_duris_sql" in sql
assert "SELECT sql_code FROM prepstatement_duris_sql" in sql
assert "INSERT INTO prepstatement_duris_sql (id, description)" in sql
assert 'exec "$SCRIPT_DIR/migrations/run_migration.sh" "$@"' in root_entrypoint

print("prepstatement schema contract checks passed")
