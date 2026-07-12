from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
bootstrap = (ROOT / "migrations/bootstrap_multithread_safe.sql").read_text()
legacy = (ROOT / "src/duris.sql").read_text()

for text, name in ((bootstrap, "bootstrap"), (legacy, "legacy schema")):
    for table in ("auction_bid_history", "auction_item_pickups", "auction_money_pickups", "auctions"):
        start = text.find(f"CREATE TABLE IF NOT EXISTS `{table}`")
        if start == -1:
            start = text.find(f"CREATE TABLE `{table}`")
        assert start != -1, f"{table} missing from {name}"
        end = text.find(";", start)
        block = text[start:end]
        assert "ENGINE=InnoDB" in block or "engine=innodb" in block.lower(), f"{table} is not InnoDB in {name}"

# In the clean bootstrap, tables are created with ENGINE=InnoDB directly (no ALTER needed).
# The ALTER TABLE ... ENGINE=InnoDB statements exist only in the migration runner for upgrades.
for path in (ROOT / "migrations/run_migration.sh", ROOT / "run_migration.sh"):
    text = path.read_text()
    assert "ALTER TABLE auctions ENGINE=InnoDB" in text, f"auctions conversion missing from {path}"
    assert "ALTER TABLE auction_bid_history ENGINE=InnoDB" in text, f"bid history conversion missing from {path}"

print("auction transaction-engine checks passed")
