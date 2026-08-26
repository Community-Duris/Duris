from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / "src/sql.c").read_text()

assert "static bool sql_verify_boot_database(void)" in source
assert "information_schema.columns" in source
assert "player_revision_probe" in source
assert "column_name='save_revision'" in source
assert "column_type='bigint unsigned'" in source
assert "player save revision schema is missing or incompatible at boot" in source
assert "persistence_item_events" in source
assert "persistence_scalar_events" in source
assert "expected 25 required columns" in source
assert "SELECT DISTINCT table_name, index_name" in source
assert "AS required_indexes" in source
assert "expected 8 entries" in source
assert "idx_item_uid_ts" in source
assert "idx_event_type_created" in source
assert "idx_scalar_event_key" in source
assert "idx_scalar_zone_time" in source
assert "uq_item_dedupe" in source
assert "uq_scalar_dedupe" in source
assert "auction_engine_probe" in source
assert "COUNT(DISTINCT table_name)" in source
assert "transactional auction tables are not all InnoDB" in source

print("boot persistence schema preflight checks passed")
