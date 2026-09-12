#!/usr/bin/env python3
"""Regression contract for max-level bartender quest feedback."""

from _paths import SRC
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SOURCE = (SRC / "specs.mobile.c").read_text()

start = SOURCE.index("int world_quest(")
end = SOURCE.index("int newbie_quest(", start)
world_quest = SOURCE[start:end]

failure = world_quest.index("if (GET_LEVEL(pl) >= MAXLVLMORTAL)")
refund = world_quest.index("ADD_MONEY(pl, temp);", failure)
feedback = world_quest[failure:refund]

assert "someone of your experience" in feedback
assert "grab a few levels and come back" in feedback
assert "You need to reach level 11" in world_quest
level_gate = world_quest.index("if (GET_LEVEL(pl) < WORLD_QUEST_MIN_LEVEL)")
quota_check = world_quest.index("if (sql_world_quest_can_do_another(pl) < 1)")
fee_calculation = world_quest.index('get_property("world.quest.cost.per.level", 20.000)')
fee_deduction = world_quest.index("SUB_MONEY(pl, temp, 0);", fee_calculation)
assert level_gate < quota_check < fee_calculation < fee_deduction

print("world quest failure feedback contract passed")
