#!/usr/bin/env python3
"""Regression: valid player input must clear idle state before queue delay."""
from pathlib import Path
from contract_text import contains, find, index

root = Path(__file__).resolve().parents[2]
comm = (root / "src/comm.c").read_text()

start = index(comm, "static void process_line(P_desc t, char *in)\n{")
end = index(comm, "\n}\n\n/*\n * ****************************************************************", start)
body = comm[start:end]

# Input that survived validation is player activity even when combat/wait state
# delays get_from_q().  The reset must happen before the line enters the queue.
activity = find(body, "note_player_input_activity(t, out)")
queue = find(body, "write_to_q(out, &t->input, 0)")
assert activity != -1, "process_line must record valid player input activity"
assert queue != -1 and activity < queue, "activity must be recorded before queueing"

helper_start = index(comm, "static void note_player_input_activity(P_desc t, const char *input)\n{")
helper_end = comm.index("\n}\n", helper_start) + 3
helper = comm[helper_start:helper_end]
assert contains(helper, "t->connected != CON_PLAYING")
assert contains(helper, "IS_PC(t->character)")
assert contains(helper, "|| !*input")
assert contains(helper, "t->character->specials.timer = 0;")
assert contains(helper, "REMOVE_BIT(t->character->specials.act, PLR_AFK);")

print("AFK input-activity reset ordering: ok")
