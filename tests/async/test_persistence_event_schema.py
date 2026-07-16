from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
paths = (
    ROOT / "migrations/bootstrap_multithread_safe.sql",
    ROOT / "migrations/run_migration.sh",
    ROOT / "run_migration.sh",
    ROOT / "src/duris.sql",
)
required = {
    "persistence_item_events": ("item_uid", "target", "ts_usec", "dedupe_key", "uq_item_dedupe"),
    "persistence_scalar_events": ("event_type", "event_key", "zone_number", "created_at", "dedupe_key", "uq_scalar_dedupe"),
}

for path in paths:
    text = path.read_text()
    if path != ROOT / "src/duris.sql":
        assert "deleted_at" in text, f"account_characters.deleted_at missing from {path}"
    for table, columns in required.items():
        assert table in text, f"{table} missing from {path}"
        for column in columns:
            assert column in text, f"{column} missing from {table} in {path}"

for path in paths[1:3]:
    text = path.read_text()
    assert "information_schema.columns" in text
    assert "ADD COLUMN dedupe_key" in text
    assert "CREATE UNIQUE INDEX uq_item_dedupe" in text
    assert "CREATE UNIQUE INDEX uq_scalar_dedupe" in text
    assert "repair persistence event schema drift" in text
    for required_name in ("id", "idx_item_uid_ts", "idx_event_type_created", "idx_scalar_event_key", "idx_scalar_zone_time"):
        assert required_name in text, f"{required_name} drift repair missing from {path}"

# In the clean bootstrap, collations are part of the CREATE TABLE statements,
# not migration ALTER statements. Verify the final schema is correct.
bootstrap = (ROOT / "migrations/bootstrap_multithread_safe.sql").read_text()
# player_data should have utf8mb4_0900_ai_ci (preserved from original migration)
assert "utf8mb4_0900_ai_ci" in bootstrap, "player_data utf8mb4_0900_ai_ci collation missing from bootstrap"
# account_characters should have utf8mb4_general_ci
assert "utf8mb4_general_ci" in bootstrap, "utf8mb4_general_ci collation missing from bootstrap"

print("persistence event schema coverage checks passed")
