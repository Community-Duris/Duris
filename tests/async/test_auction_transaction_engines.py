from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
bootstrap = (ROOT / "migrations/bootstrap_multithread_safe.sql").read_text()
legacy = (ROOT / "src/duris.sql").read_text()
tables = ("auction_bid_history", "auction_item_pickups", "auction_money_pickups", "auctions")

for text, name in ((bootstrap, "bootstrap"), (legacy, "legacy schema")):
    for table in tables:
        start = text.find(f"CREATE TABLE IF NOT EXISTS `{table}`")
        if start == -1:
            start = text.find(f"CREATE TABLE `{table}`")
        assert start != -1, f"{table} missing from {name}"
        end = text.find(";", start)
        block = text[start:end]
        assert "ENGINE=InnoDB" in block or "engine=innodb" in block.lower(), f"{table} is not InnoDB in {name}"

contract = (ROOT / "migrations/persistence_contract.sql").read_text()
for table in tables:
    assert f"ALTER TABLE {table} ENGINE=InnoDB" in contract, f"{table} conversion missing from canonical contract"

authoritative = (ROOT / "migrations/run_migration.sh").read_text()
root_entrypoint = (ROOT / "run_migration.sh").read_text()
assert '"$SCRIPT_DIR/persistence_contract.sql"' in authoritative
assert 'exec "$SCRIPT_DIR/migrations/run_migration.sh" "$@"' in root_entrypoint

print("auction transaction-engine checks passed")
