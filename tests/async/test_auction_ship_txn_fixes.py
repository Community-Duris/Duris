#!/usr/bin/env python3
"""Regression checks for auction bid transaction leak, cargo commit rollback, and ship db_id reset."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]

# 1. Auction bid failure paths must rollback owned transactions
auction = (root / "src/auction_houses.c").read_text()
assert "if (own_txn) sql_rollback();" in auction
assert auction.count("sql_rollback();") >= 10

# 2. Cargo commit failure must attempt rollback
cargo = (root / "src/ships/ship_cargo.c").read_text()
assert "logit(LOG_DEBUG, \"write_cargo(): commit failed\");\n\t\tsql_rollback();" in cargo

# 3. Ship db_id must be reset to -1 on post-INSERT failure paths
sql_player = (root / "src/sql_player.c").read_text()
# At least 4 reset points: query failure, no row, armor/crew/slot failure, batch failure
assert sql_player.count("ship->db_id = -1;") >= 4

# 4. Ship UPDATE must include owner_name so rename_ship_owner persists
assert "update ships set owner_name='%s', ship_name='%s'" in sql_player

# 4. Auction finalization still has the atomic claim
assert "UPDATE auctions SET status = %d WHERE id = '%d' AND status <> %d" in auction
assert "mysql_affected_rows(DB) != 1" in auction

print("auction bid leak, cargo rollback, and ship db_id reset checks passed")
