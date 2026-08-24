from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
paths = [
    ROOT / "migrations/bootstrap_multithread_safe.sql",
    ROOT / "migrations/persistence_contract.sql",
]
for path in paths:
    text = path.read_text()
    assert "persistence_scalar_events" in text
    assert "dedupe_key" in text
authoritative = (ROOT / "migrations/run_migration.sh").read_text()
assert '"$SCRIPT_DIR/persistence_contract.sql"' in authoritative
sql = (ROOT / "src/sql.c").read_text()
assert "ON DUPLICATE KEY UPDATE id=id" in sql
assert "dedupe_key" in sql
print("scalar persistence idempotency checks passed")
