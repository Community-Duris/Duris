#!/usr/bin/env python3
"""Source-contract checks for transaction ownership and commit ordering fixes."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
locker = (root / "src/storage_lockers.c").read_text()
auction = (root / "src/auction_houses.c").read_text()
copyover = (root / "src/copyover.c").read_text()
ships = (root / "src/ships/ship_base.c").read_text()

# LockerSave moves inventory via LockerToPFile then hands public SQL to
# the async worker. Sync SQL txn ownership is no longer in this function.
# Private chests still write inside LockerToPFile; public items are queued.
locker_save = locker[locker.index("static int save_locker_char") : locker.index("bool StorageLocker::LockerToPFile")]
assert "LockerToPFile()" in locker_save
assert "locker_async_mark_dirty" in locker_save
assert "save_locker_char-nonterminal" in locker_save
# Fallbacks keep durability if the worker is unavailable.
assert "async unavailable" in locker_save
assert "writeCharacter(chLocker, 0, NOWHERE)" in locker_save or "writeCharacter(chLocker, 3, NOWHERE)" in locker_save

# Offering owns an atomic transaction, saves the detached inventory, commits,
# then destroys detached objects and publishes/notifies.
offer = auction[auction.index("bool auction_offer") : auction.index("bool auction_list")]
assert "sql_in_transaction() || !sql_begin_transaction()" in offer
assert offer.index("obj_from_char(tmp_obj);") < offer.index("writeCharacter(ch, 1, ch->in_room)")
assert offer.index("writeCharacter(ch, 1, ch->in_room)") < offer.index("sql_commit()")
assert offer.index("sql_commit()") < offer.index("extract_obj(removed[--removed_count])")
assert offer.index("sql_commit()") < offer.index("ws_broadcast_auction_new")
assert offer.index("sql_commit()") < offer.rindex("send_to_char(buff, ch)")

# Normal bid success and outbid notifications happen after commit.
bid = auction[auction.index("bool auction_bid") : auction.index("bool auction_pickup")]
commit = bid.rindex("sql_commit()")
assert commit < bid.index("ws_broadcast_auction_bid")
assert commit < bid.index("send_to_pid(buff, winning_bidder_pid)")
assert commit < bid.rindex("send_to_char(buff, ch)")

# Copyover uses a forced drain which ignores retry_after and aborts on failure.
drain = ships[ships.index("bool drain_pending_ship_saves") : ships.index("int read_ships()")]
assert "save_retry_after" not in drain
assert "if (!write_ship(ship))" in drain
assert "drained = false;" in drain
assert "if (!drain_pending_ship_saves())" in copyover
assert "notify_copyover_failure" in copyover[copyover.index("if (!drain_pending_ship_saves())") :]
assert "locker_async_drain" in copyover

print("locker ownership, auction commit ordering, and copyover ship drain checks passed")
