from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
schema_paths = (
    ROOT / "migrations/bootstrap_multithread_safe.sql",
    ROOT / "migrations/persistence_contract.sql",
    ROOT / "migrations/bootstrap_legacy_baseline.sql",
)
required = {
    "persistence_item_events": ("item_uid", "target", "ts_usec", "dedupe_key", "uq_item_dedupe"),
    "persistence_scalar_events": ("event_type", "event_key", "zone_number", "created_at", "dedupe_key", "uq_scalar_dedupe"),
}

for path in schema_paths:
    text = path.read_text()
    for table, columns in required.items():
        assert table in text, f"{table} missing from {path}"
        for column in columns:
            assert column in text, f"{column} missing from {table} in {path}"

bootstrap = schema_paths[0].read_text()
contract = schema_paths[1].read_text()
assert "information_schema.columns" in contract
assert "ADD COLUMN dedupe_key" in contract
assert "CREATE UNIQUE INDEX uq_item_dedupe" in contract
assert "CREATE UNIQUE INDEX uq_scalar_dedupe" in contract
for required_name in ("id", "idx_item_uid_ts", "idx_event_type_created", "idx_scalar_event_key", "idx_scalar_zone_time"):
    assert required_name in contract, f"{required_name} drift repair missing from canonical contract"

authoritative = (ROOT / "migrations/run_migration.sh").read_text()
assert "run_sql_file()" in authoritative
assert '"$SCRIPT_DIR/persistence_contract.sql"' in authoritative

print("persistence event schema coverage checks passed")
