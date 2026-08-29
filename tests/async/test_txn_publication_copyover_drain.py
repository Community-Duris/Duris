#!/usr/bin/env python3
"""Source-contract checks for transaction ownership and commit ordering fixes."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
locker = (root / "src/storage_lockers.c").read_text()
auction = (root / "src/auction_houses.c").read_text()
auction_repository = (root / "src/auction_repository.c").read_text()
critical_repository = (root / "src/critical_command_repository.c").read_text()
auction_transaction = (root / "src/auction_transaction.c").read_text()
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

# Active offer/bid commands submit immutable typed requests without destroying
# live objects. Durable database mutation, inbox/result, and outbox publication
# share one coordinator-owned transaction.
offer_start = auction.rindex("bool auction_offer(")
offer = auction[offer_start : auction.index("bool auction_offer_legacy(", offer_start)]
assert "payload.action = auction_action::list" in offer
assert "auction_transaction_submit" in offer
assert "obj_from_char" not in offer and "extract_obj" not in offer

bid_start = auction.rindex("bool auction_bid(")
bid = auction[bid_start : auction.index("bool auction_bid_legacy(", bid_start)]
assert "payload.action = auction_action::bid" in bid
assert "auction_transaction_submit" in bid

apply_start = critical_repository.index("critical_apply_result critical_command_repository_apply")
apply = critical_repository[apply_start:]
auction_execute = apply.index("auction_repository_execute")
auction_commit = apply.index('execute(connection, "COMMIT")', auction_execute)
assert apply.index('execute(connection, "START TRANSACTION")') < auction_execute
assert auction_execute < apply.index("insert_outbox", auction_execute) < auction_commit

repository_bid = auction_repository[
    auction_repository.index("else if (payload.action == auction_action::bid)"):
    auction_repository.index("else if (payload.action == auction_action::finalize")
]
assert "lock_auction" in repository_bid and "apply_wallet_delta" in repository_bid

# Live inventory destruction and external broadcast/notification happen only
# after a committed completion or a committed outbox delivery.
list_completion = auction[
    auction.rindex("void auction_list_completed"):
    auction.rindex("void auction_bid_completed")
]
assert list_completion.index("if (!committed)") < list_completion.index("obj_from_char")
assert list_completion.index("if (!committed)") < list_completion.index("extract_obj")
assert "critical_apply_outcome::applied" in auction_transaction
assert "auction_transaction_publish_outbox" in auction_transaction
publisher = auction[
    auction.rindex("bool auction_publish_committed_event"):
    auction.rindex("// syntax: auction offer")
]
assert "ws_broadcast_auction_bid" in publisher
assert "send_to_pid" in publisher

# Copyover uses a forced drain which ignores retry_after and aborts on failure.
drain = ships[ships.index("bool drain_pending_ship_saves") : ships.index("int read_ships()")]
assert "save_retry_after" not in drain
assert "if (!write_ship(ship))" in drain
assert "drained = false;" in drain
assert "if (!drain_pending_ship_saves())" in copyover
assert "notify_copyover_failure" in copyover[copyover.index("if (!drain_pending_ship_saves())") :]
assert "locker_async_drain" in copyover

print("locker ownership, auction commit ordering, and copyover ship drain checks passed")
