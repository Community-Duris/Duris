#!/usr/bin/env python3
"""Regression checks for the auction finalization claim."""
from pathlib import Path

source = (Path(__file__).resolve().parents[2] / "src/auction_houses.c").read_text()
needle = 'UPDATE auctions SET status = %d WHERE id = \'%d\' AND status <> %d'
assert needle in source
assert "mysql_affected_rows(DB) != 1" in source
assert "UPDATE auctions SET status = %d WHERE id = '%d'\", AUCTION_STATUS_CLOSED" not in source
print("auction finalization claim checks passed")
