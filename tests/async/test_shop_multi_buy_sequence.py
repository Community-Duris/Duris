#!/usr/bin/env python3
"""Regression contracts for completion-driven produced-item shop purchases (#537)."""

from _paths import SRC, extract_function


SHOP = (SRC / "economy" / "shop.c").read_text(encoding="utf-8", errors="replace")
MOVEMENT_H = (SRC / "item" / "item_movement_transaction.h").read_text(
    encoding="utf-8", errors="replace"
)
BUY = extract_function("economy/shop.c", "void shopping_buy(")


def last_function(signature: str) -> str:
    """Extract a definition whose forward declaration uses the same signature."""
    start = SHOP.rindex(signature)
    depth = 0
    for index in range(SHOP.index("{", start), len(SHOP)):
        if SHOP[index] == "{":
            depth += 1
        elif SHOP[index] == "}":
            depth -= 1
            if depth == 0:
                return SHOP[start : index + 1]
    raise AssertionError(f"unbalanced function: {signature}")


SUBMIT = last_function("static bool shop_creation_submit_produced_continuation(")
GRANT = last_function("static bool shop_creation_submit_grant(")
COMPLETE = last_function("static void shop_creation_grant_completion(")
PAYMENT = last_function("static void shop_creation_payment_completion(")
COUNT = extract_function("economy/shop.c", "static bool shop_purchase_count(")

# The regression was caused by recursively entering BUY while the preceding
# ownership grant was still pending. Continuation now belongs to final grant
# completion, never to shopping_buy itself.
assert "shop_keeper(keeper, ch, CMD_BUY" not in BUY
assert "shop_creation_submit_produced_continuation(ch, sequence)" in COMPLETE
assert "item_creation_grant_submit_to_player_with_completion" in GRANT
assert "current_item_uid" in SUBMIT and "current_item_uid != item_uid" in COMPLETE

# Invalid, signed/zero, oversized, or trailing quantity input cannot silently
# degrade into a single inventory purchase.
assert "strtol" in COUNT
assert "parsed < 1 || parsed > 50" in COUNT
assert "*end" in COUNT
assert "nothing was purchased" in BUY
assert "delivered to your inventory" not in BUY

# Payment commits before a held grant only after cloning/capacity checks, and
# every terminal grant failure refunds that exact unit before ending the sequence.
assert SUBMIT.index("read_object(") < SUBMIT.index("shop_trade_container_accepts(")
assert SUBMIT.index("shop_trade_container_accepts(") < SUBMIT.index(
    "currency_transaction_submit_wallet_value("
)
assert "shop_creation_submit_grant(ch, sequence)" in PAYMENT
assert "payment_pending" in SUBMIT and "payment_committed" in PAYMENT
assert "ch->in_room == keeper->in_room" in PAYMENT
assert "shop_trade_container_accepts(ch, selected, destination)" in GRANT
assert "IS_CARRYING_N(ch) + 1 <= CAN_CARRY_N(ch)" in GRANT
failure = COMPLETE[COMPLETE.index("if (!committed)") :]
assert failure.index("shop_creation_refund(") < failure.index(
    "produced_purchase_sequences.erase("
)
assert "transact(" not in SUBMIT

# Player success is emitted only after durable ownership publication, and the
# next unit sees the already-published container/carry weight of prior units.
assert "You now have" not in SUBMIT
assert "You now have" not in GRANT
assert "You now have" in COMPLETE
assert "shop_trade_container_accepts(ch, selected, destination)" in SUBMIT
assert "++sequence.completed" in COMPLETE and "--sequence.remaining" in COMPLETE
assert "the remaining %d were not charged" in SHOP

# Starter/pre-entry grants remain callback-free; the callback API is limited to
# ordinary player grants whose actor is in normal command processing.
assert "item_creation_grant_submit_to_player_with_completion" in MOVEMENT_H
assert "before_entry_with_completion" not in MOVEMENT_H

print("completion-driven multi-buy contracts passed")
