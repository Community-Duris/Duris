#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
contract_path = ROOT / "migrations/persistence_contract.sql"
verify_path = ROOT / "migrations/verify_persistence_contract.sh"
apply_path = ROOT / "migrations/apply_persistence_contract.sh"
assert contract_path.exists(), "canonical persistence migration contract is missing"
assert verify_path.exists(), "read-only persistence contract verifier is missing"
assert apply_path.exists(), "scoped persistence contract runner is missing"

contract = contract_path.read_text()
verifier = verify_path.read_text()
apply_runner = apply_path.read_text()
runner_path = ROOT / "migrations/run_migration.sh"
runner = runner_path.read_text()

item_columns = (
    "id", "ts_usec", "event_type", "item_uid", "vnum", "item", "actor",
    "actor_id", "source", "target", "note", "dedupe_key", "created_at",
)
scalar_columns = (
    "id", "event_type", "event_key", "boot_time", "touched_at", "zone_number",
    "toucher_pid", "group_size", "epic_value", "alignment_delta", "dedupe_key",
    "created_at",
)
indexes = (
    "idx_item_uid_ts", "idx_event_type_created", "uq_item_dedupe",
    "idx_scalar_event_key", "idx_scalar_zone_time", "uq_scalar_dedupe",
)
auctions = (
    "auction_bid_history", "auction_item_pickups", "auction_money_pickups", "auctions",
)

assert "CREATE TABLE IF NOT EXISTS persistence_item_events" in contract
assert "CREATE TABLE IF NOT EXISTS persistence_scalar_events" in contract
for table, columns in (("persistence_item_events", item_columns), ("persistence_scalar_events", scalar_columns)):
    for column in columns:
        needle = f"table_name = '{table}' AND column_name = '{column}'"
        assert needle in contract, f"independent repair guard missing for {table}.{column}"
for index in indexes:
    assert f"index_name = '{index}'" in contract, f"index guard missing for {index}"
for table in auctions:
    assert f"ALTER TABLE {table} ENGINE=InnoDB" in contract

assert "FLUSHDB" not in contract
assert "FLUSHALL" not in contract
assert "SELECT COUNT(*) FROM information_schema.columns" in verifier
assert "SELECT COUNT(DISTINCT table_name)" in verifier
assert "expected 25 required columns" in verifier
assert "expected 25 exact column definitions" in verifier
assert "expected 8 required indexes" in verifier
assert "expected 8 exact index definitions" in verifier
assert "expected 2 exact InnoDB/utf8mb4 persistence event tables" in verifier
assert "expected 4 InnoDB auction tables" in verifier
assert "MODIFY COLUMN ts_usec BIGINT UNSIGNED NOT NULL AFTER id" in contract
assert "GROUP_CONCAT(CONCAT(column_name, ':', non_unique)" in contract
assert "id itself is missing" in contract
assert "ALTER TABLE persistence_item_events DROP PRIMARY KEY" in contract
assert "ALTER TABLE persistence_scalar_events DROP PRIMARY KEY" in contract
assert "FLUSHDB" not in verifier
assert "FLUSHALL" not in verifier
assert "--confirm-db" in apply_runner
assert '"$SCRIPT_DIR/persistence_contract.sql"' in apply_runner
assert '"$SCRIPT_DIR/verify_persistence_contract.sh"' in apply_runner
assert "FLUSHDB" not in apply_runner
assert "FLUSHALL" not in apply_runner

assert "run_sql_file()" in runner, "run_sql_file helper missing from authoritative runner"
assert '"$SCRIPT_DIR/persistence_contract.sql"' in runner
assert "CREATE TABLE IF NOT EXISTS mud_schema_migrations" in runner
assert "account_locker_copy_v1" in runner
assert 'run_sql "repair item persistence schema drift"' in runner
assert "CREATE TABLE IF NOT EXISTS locker_item_extra_descr" in runner
assert '"$SCRIPT_DIR/legacy_schema_convergence.sql"' in runner
convergence = (ROOT / "migrations/legacy_schema_convergence.sql").read_text()
assert "MODIFY COLUMN id INT UNSIGNED NOT NULL AUTO_INCREMENT FIRST" in convergence
assert runner.index('"$SCRIPT_DIR/legacy_schema_convergence.sql"') < runner.index(
    '"$SCRIPT_DIR/adopt_migration_baseline.sh"'
)
assert 'PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"' in runner
assert 'source "$PROJECT_ROOT/.env"' in runner

print("persistence migration contract coverage passed")
