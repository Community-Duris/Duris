#!/usr/bin/env python3
"""Focused source regressions for locker, auction, and ship follow-up fixes."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
locker = (root / "src/storage_lockers.c").read_text()
auction = (root / "src/auction_houses.c").read_text()
ship = (root / "src/ships/ship_base.c").read_text()
handler = (root / "src/handler.c").read_text()

# Locker chest strings must be assembled within their fixed-size buffers.
chest_start = locker.index("LockerChest::CreateChestObject")
chest_end = locker.index("ComboChest::FillExtraDescBuf", chest_start)
chest_body = locker[chest_start:chest_end]
assert 'snprintf(GBuf1, sizeof(GBuf1), "chest %s", this->m_chestKeyword);' in chest_body
assert 'snprintf(GBuf1, sizeof(GBuf1), "&+yAn ornate chest bearing items %s&+y.&n", this->m_chestDescText);' in chest_body
assert "strcat(GBuf1, this->m_chestDescText);" not in chest_body

# The mutable bid state must be read under a row lock after transaction start.
bid_start = auction.index("bool auction_bid(")
bid_end = auction.index("bool auction_pickup(", bid_start)
bid_body = auction[bid_start:bid_end]
begin_pos = bid_body.index("sql_begin_transaction()")
select_pos = bid_body.index("SELECT a.cur_price")
assert begin_pos < select_pos
assert "a.status = %d FOR UPDATE" in bid_body
assert "status = %d FOR UPDATE" in bid_body

# A failed shutdown commit must rollback and terminate through the corruption path.
shutdown_start = ship.index("void shutdown_ships()")
shutdown_end = ship.index("struct ShipData *new_ship", shutdown_start)
shutdown_body = ship[shutdown_start:shutdown_end]
assert 'panic_corruption("shutdown_ships", "commit failed after rollback")' in shutdown_body

# Room-exit callbacks can veto removal, and locker save failure uses that contract.
assert "if ((*world[ch->in_room].funct)(ch->in_room, ch, (-75), NULL))\n\t\t\treturn;" in handler
assert "static bool locker_handle_leave" in locker
assert "if (!locker_handle_leave(ch, pLocker, room, troom))\n\t\t\treturn TRUE;" in locker

print("locker bounds, auction row lock, ship commit failure, and locker exit veto checks passed")
