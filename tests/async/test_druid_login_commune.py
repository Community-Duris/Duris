#!/usr/bin/env python3
"""Regression contract for restarting passive communion on player login."""

from _paths import SRC
from pathlib import Path

from contract_text import contains, index, split_at

ROOT = Path(__file__).resolve().parents[2]
nanny = (SRC / "nanny.c").read_text()

enter_game = split_at(nanny, "void enter_game(P_desc d)", 1)[1]
enter_game = enter_game.split("void nanny(", 1)[0]

schedule = "schedule_pc_events(ch);"
resume = 'do_assimilate(ch, writable_arg("nl"), CMD_COMMUNE);'

assert contains(enter_game, schedule)
assert contains(enter_game, "USES_COMMUNE(ch) && !IS_DRAGOON(ch)")
assert contains(enter_game, resume)
assert index(enter_game, schedule) < index(enter_game, resume)

print("druid login commune regression passed")
