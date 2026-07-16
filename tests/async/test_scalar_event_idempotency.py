from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
paths = [
    ROOT / "migrations/bootstrap_multithread_safe.sql",
    ROOT / "migrations/run_migration.sh",
    ROOT / "run_migration.sh",
]
for path in paths:
    text = path.read_text()
    assert "persistence_scalar_events" in text
    assert "dedupe_key" in text
sql = (ROOT / "src/sql.c").read_text()
assert "ON DUPLICATE KEY UPDATE id=id" in sql
assert "dedupe_key" in sql
print("scalar persistence idempotency checks passed")
