#!/usr/bin/env python3
"""Regression: valid player input must clear idle state before queue delay."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
comm = (root / "src/comm.c").read_text()

start = comm.index("static void process_line(")
end = comm.index("\n}\n\n/*\n * ****************************************************************", start)
body = comm[start:end]

# Input that survived validation is player activity even when combat/wait state
# delays get_from_q().  The reset must happen before the line enters the queue.
activity = body.find("note_player_input_activity(t, out)")
queue = body.find("write_to_q(out, &t->input, 0)")
assert activity != -1, "process_line must record valid player input activity"
assert queue != -1 and activity < queue, "activity must be recorded before queueing"

helper_start = comm.index("static void note_player_input_activity(")
helper_end = comm.index("\n}\n", helper_start) + 3
helper = comm[helper_start:helper_end]
assert "t->connected != CON_PLAYING" in helper
assert "IS_PC(t->character)" in helper
assert "|| !*input" in helper
assert "t->character->specials.timer = 0;" in helper
assert "REMOVE_BIT(t->character->specials.act, PLR_AFK);" in helper

print("AFK input-activity reset ordering: ok")
