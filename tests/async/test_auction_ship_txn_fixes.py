#!/usr/bin/env python3
"""Regression checks for auction bid transaction leak, cargo commit rollback, and ship db_id reset."""
from _paths import SRC
from pathlib import Path
import re
from contract_text import contains

root = Path(__file__).resolve().parents[2]

# 1. Auction bid failure paths must rollback owned transactions
auction = (SRC / "auction_houses.c").read_text()
assert contains(auction, "if (own_txn) sql_rollback();")
assert auction.count("sql_rollback();") >= 10

# 2. Cargo commit failure must attempt rollback
cargo = (SRC / "ships/ship_cargo.c").read_text()
assert contains(cargo, "logit(LOG_DEBUG, \"write_cargo(): commit failed\");\n\t\tsql_rollback();")

# 3. A failed ship save resets db_id only when that save inserted the row: the
# batch, batch-helper and commit failures are all guarded.  An existing ship
# keeps its id (behaviour: test_ship_save_failure_keeps_db_id.py).
sql_player = (SRC / "sql_player.c").read_text()
save_start = sql_player.index("#define SHIP_SQL_BATCH_SIZE")
save_ship = sql_player[save_start:sql_player.index("static bool sql_load_ship_armor(", save_start)]
assert contains(save_ship, "ship->db_id = atoi(row[0]);\n\t\tinserted = true;")
assert len(re.findall(r"if \(inserted\)\s*ship->db_id = -1;", save_ship)) == 3

# 4. Ship UPDATE must include owner_name so rename_ship_owner persists
assert contains(sql_player, "update ships set owner_name='%s', ship_name='%s'", literal=True)

# 4. Auction finalization still has the atomic claim
assert contains(auction, "UPDATE auctions SET status = %d WHERE id = '%d' AND status <> %d", literal=True)
assert contains(auction, "mysql_affected_rows(DB) != 1")

print("auction bid leak, cargo rollback, and ship db_id reset checks passed")
