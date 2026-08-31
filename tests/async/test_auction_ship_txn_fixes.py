#!/usr/bin/env python3
"""Regression checks for auction bid transaction leak, cargo commit rollback, and ship db_id reset."""
from _paths import SRC
from pathlib import Path
from contract_text import contains

root = Path(__file__).resolve().parents[2]

# 1. Auction bid failure paths must rollback owned transactions
auction = (SRC / "auction_houses.c").read_text()
assert contains(auction, "if (own_txn) sql_rollback();")
assert auction.count("sql_rollback();") >= 10

# 2. Cargo commit failure must attempt rollback
cargo = (SRC / "ships/ship_cargo.c").read_text()
assert contains(cargo, "logit(LOG_DEBUG, \"write_cargo(): commit failed\");\n\t\tsql_rollback();")

# 3. Ship db_id must be reset to -1 on post-INSERT failure paths
sql_player = (SRC / "sql_player.c").read_text()
# At least 4 reset points: query failure, no row, armor/crew/slot failure, batch failure
assert sql_player.count("ship->db_id = -1;") >= 4

# 4. Ship UPDATE must include owner_name so rename_ship_owner persists
assert contains(sql_player, "update ships set owner_name='%s', ship_name='%s'")

# 4. Auction finalization still has the atomic claim
assert contains(auction, "UPDATE auctions SET status = %d WHERE id = '%d' AND status <> %d")
assert contains(auction, "mysql_affected_rows(DB) != 1")

print("auction bid leak, cargo rollback, and ship db_id reset checks passed")
