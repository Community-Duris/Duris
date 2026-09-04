#!/usr/bin/env python3
"""Coin commands publish world state only after a durable wallet debit."""

from _paths import SRC
import re


ACTOBJ = (SRC / "actobj.c").read_text(encoding="utf-8", errors="replace")


def body(signature: str) -> str:
    """Return one C++ function body selected by its signature."""
    start = ACTOBJ.index(signature)
    opening = ACTOBJ.index("{", start)
    depth = 0
    for position in range(opening, len(ACTOBJ)):
        if ACTOBJ[position] == "{":
            depth += 1
        elif ACTOBJ[position] == "}":
            depth -= 1
            if depth == 0:
                return ACTOBJ[start : position + 1]
    raise AssertionError(f"unterminated function: {signature}")


drop_all = body("void do_dropalldot(")
drop = body("void do_drop(")
put = body("void do_put(")
give = body("void do_give(")
submit = body("bool submit_coin_debit(")
completion = body("void coin_debit_completion(")
publish_drop = body("void publish_coin_drop(")
publish_put = body("bool publish_coin_put(")
give_credit = body("bool begin_coin_give_credit(")
give_completion = body("void coin_give_credit_completion(")

for name, command in (
    ("drop all.coins", drop_all),
    ("drop coins", drop),
    ("put coins", put),
    ("give coins", give),
):
    assert "submit_coin_debit(ch, context)" in command, name

assert "currency_transaction_submit_wallet_value(" in submit
assert "actor, -value, currency_reason_type::wallet_spend" in submit
assert "critical_source_site::command" in submit
assert "coin_debit_completion" in submit

failed = completion.index("if (!committed)")
dispatch = completion.index("switch (context.action)")
assert failed < dispatch
assert "The coin debit did not commit; nothing changed." in completion
assert "publish_coin_drop(actor, context)" in completion
assert "publish_coin_put(actor, context)" in completion
assert "begin_coin_give_credit(actor, recipient, context, true)" in completion

assert "create_money(" in publish_drop
assert "obj_to_room(" in publish_drop
assert "create_money(" in publish_put
assert "coin_put_destination_custody(" in publish_put
assert "item_movement_transaction_submit(" in publish_put
assert "coin_put_custody_completion" in publish_put
assert "extract_obj(old_money)" not in publish_put
assert "currency_transaction_submit_wallet_value(" in give_credit
assert "recipient, value, currency_reason_type::wallet_reward" in give_credit
assert "uint8_t debit_committed;" in ACTOBJ
assert "static_cast<uint8_t>(debit_committed)" in give_credit
assert "if (context.debit_committed)" in give_completion
assert "refund_committed_coin_debit(sender, context.value, context.room)" in give_completion
assert "begin_coin_give_credit(ch, vict, context, false)" in give

direct_cash_write = re.compile(r"points\.cash\[[^]]+\]\s*(?:[+\-]=|=(?!=))")
assert not direct_cash_write.search(ACTOBJ)

for command in (drop_all, drop, put, give):
    assert "create_money(" not in command

print("coin drop, put, and give wait for durable debit ACK before publication")
