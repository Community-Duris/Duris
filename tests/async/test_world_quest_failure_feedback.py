#!/usr/bin/env python3
"""Regression contract for max-level bartender quest feedback."""

from _paths import SRC
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SOURCE = (SRC / "specs.mobile.c").read_text()

start = SOURCE.index("int world_quest(")
end = SOURCE.index("int newbie_quest(", start)
world_quest = SOURCE[start:end]
deferred_start = SOURCE.index("static void world_quest_report_creation_failure(")
deferred = SOURCE[deferred_start:end]

failure = deferred.index("else if (GET_LEVEL(pl) >= MAXLVLMORTAL)")
refund = deferred.index("world_quest_refund_payment(pl, payment.fee);", failure)
feedback = deferred[failure:refund]

assert "someone of your experience" in feedback
assert "grab a few levels and come back" in feedback
assert "You need to reach level 11" in world_quest
level_gate = world_quest.index("if (GET_LEVEL(pl) < WORLD_QUEST_MIN_LEVEL)")
quota_check = world_quest.index("if (sql_world_quest_can_do_another(pl) < 1)")
fee_calculation = world_quest.index('get_property("world.quest.cost.per.level", 20.000)')
fee_deduction = world_quest.index("currency_transaction_submit_wallet_value(", fee_calculation)
assert level_gate < quota_check < fee_calculation < fee_deduction

assert "SUB_MONEY(pl, temp, 0);" not in world_quest
assert "currency_transaction_submit_wallet_value" in world_quest
assert "createQuestForGiverVnum" in deferred

context = SOURCE[SOURCE.index("struct world_quest_payment_context"):SOURCE.index(
    "static_assert(sizeof(world_quest_payment_context)",
)]
assert "P_char" not in context
assert "CURRENCY_PENDING_CONTEXT_MAX_BYTES" in SOURCE

rejection = deferred[deferred.index("if (!committed)"):deferred.index("switch (payment.action)")]
assert "createQuestForGiverVnum" not in rejection
assert "quest_buy_map" not in rejection
assert "resetQuest" not in rejection

print("world quest failure feedback contract passed")
