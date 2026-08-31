#!/usr/bin/env python3
"""Focused source regressions for locker, auction, and ship follow-up fixes."""
from _paths import SRC
from pathlib import Path
from contract_text import contains

root = Path(__file__).resolve().parents[2]
locker = (SRC / "storage_lockers.c").read_text()
auction = (SRC / "auction_houses.c").read_text()
auction_repository = (SRC / "auction_repository.c").read_text()
critical_repository = (SRC / "critical_command_repository.c").read_text()
ship = (SRC / "ships/ship_base.c").read_text()
handler = (SRC / "handler.c").read_text()

# Locker chest strings must be assembled within their fixed-size buffers.
chest_start = locker.index("LockerChest::CreateChestObject")
chest_end = locker.index("ComboChest::FillExtraDescBuf", chest_start)
chest_body = locker[chest_start:chest_end]
assert contains(chest_body, 'snprintf(GBuf1, sizeof(GBuf1), "chest %s", this->m_chestKeyword);')
assert contains(chest_body, 'snprintf(GBuf1, sizeof(GBuf1), "&+yAn ornate chest bearing items %s&+y.&n", this->m_chestDescText);')
assert not contains(chest_body, "strcat(GBuf1, this->m_chestDescText);")

# The active bid path must submit the typed command, whose MariaDB repository
# reads mutable auction state under a row lock inside the coordinator transaction.
bid_start = auction.rindex("bool auction_bid(")
bid_end = auction.index("bool auction_bid_legacy(", bid_start)
bid_body = auction[bid_start:bid_end]
assert contains(bid_body, "payload.action = auction_action::bid")
assert contains(bid_body, "auction_transaction_submit")

lock_start = auction_repository.index("bool lock_auction(")
lock_end = auction_repository.index("bool ensure_owner_revision", lock_start)
lock_body = auction_repository[lock_start:lock_end]
assert contains(lock_body, "WHERE a.id=")
assert contains(lock_body, "FOR UPDATE")

apply_start = critical_repository.index("critical_apply_result critical_command_repository_apply")
apply_body = critical_repository[apply_start:]
assert apply_body.index('execute(connection, "START TRANSACTION")') < apply_body.index(
    "auction_repository_execute"
)
assert apply_body.index("auction_repository_execute") < apply_body.index(
    'execute(connection, "COMMIT")', apply_body.index("auction_repository_execute")
)

# A failed shutdown commit must rollback and terminate through the corruption path.
shutdown_start = ship.index("void shutdown_ships()")
shutdown_end = ship.index("struct ShipData *new_ship", shutdown_start)
shutdown_body = ship[shutdown_start:shutdown_end]
assert contains(shutdown_body, 'panic_corruption("shutdown_ships", "commit failed after rollback")')

# Room-exit callbacks may veto only with the explicit sentinel; ordinary legacy
# TRUE results must remain notifications rather than removal vetoes.
assert contains(handler, "CMD_FROMROOM, NULL) == ROOM_PROC_LEAVE_VETO")
assert contains(locker, "static bool locker_handle_leave")
assert contains(locker, "if (!locker_handle_leave(ch, pLocker, room, troom))\n\t\t\treturn ROOM_PROC_LEAVE_VETO;")

# An unpaid locker entry must resolve its bank exit before char_from_room runs
# the leave hook and frees the temporary locker room. The leave hook owns the
# locker cleanup, and a veto must not duplicate the character into the bank.
entry_start = locker.index("int storage_locker_room_hook(")
entry_end = locker.index("int guild_locker_room_hook(", entry_start)
entry_body = locker[entry_start:entry_end]
payment_start = entry_body.index("..but you don't have the money")
payment_end = entry_body.index("// End Money hack", payment_start)
payment_failure = entry_body[payment_start:payment_end]
exit_lookup = payment_failure.index("const int exit_room = locker_exit_room(ch, room);")
unlink = payment_failure.index("char_from_room(ch);")
reinsert = payment_failure.index("char_to_room(ch, exit_room, 0);")
assert exit_lookup < unlink < reinsert
assert contains(payment_failure, "if (ch->in_room == NOWHERE)")
assert not contains(payment_failure, "free_locker(room);")
assert not contains(payment_failure, "extract_char(chLocker);")

print("locker bounds, auction row lock, ship commit failure, and locker exit veto checks passed")
