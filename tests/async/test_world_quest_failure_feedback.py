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
assert feedback.index("someone of your experience") < feedback.index("else")

print("world quest failure feedback contract passed")
