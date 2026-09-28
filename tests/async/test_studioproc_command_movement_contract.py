#!/usr/bin/env python3
"""Regression contracts for studioproc commands and movement lifetime safety."""
from _paths import SRC
from pathlib import Path

from contract_text import contains, index

ROOT = Path(__file__).resolve().parents[2]
source = (SRC / "studioproc.c").read_text()
proclib = (SRC / "studioproclib.c").read_text()

command_start = index(source, "static int sp_do_command(")
command_end = index(source, "static int sp_execute(", command_start)
command = source[command_start:command_end]
assert contains(command, "if (!MIN_POS(ch, cmd_info[cmd].minimum_position))")
assert not contains(command, "GET_POS(ch) < cmd_info[cmd].minimum_position")

execute_start = index(source, "static int sp_execute(")
execute_end = index(source, "static int sp_fire(", execute_start)
execute = source[execute_start:execute_end]
self_membership = index(execute, "if (self && !char_in_list(self))")
self_liveness = index(execute, "!IS_ALIVE(self)", self_membership)
actor_membership = index(execute, "if (actor && !char_in_list(actor))")
actor_liveness = index(execute, "!IS_ALIVE(actor)", actor_membership)
assert self_membership < self_liveness
assert actor_membership < actor_liveness

run_start = index(source, "static int sp_run(")
run_end = index(source, "static int sp_event_for_cmd(", run_start)
run = source[run_start:run_end]
assert contains(
	run,
	"if (cx->self_ch && !char_in_list(cx->self_ch))\n\t\tcx->self_ch = NULL;",
)
assert contains(
	run,
	"if (cx->actor && !char_in_list(cx->actor))\n\t\tcx->actor = NULL;",
)
fire_return = index(run, "one = sp_fire(t, cx);")
self_post_fire_membership = index(run, "!char_in_list(cx->self_ch)", fire_return)
self_post_fire_liveness = index(run, "!IS_ALIVE(cx->self_ch)", fire_return)
actor_post_fire_membership = index(run, "!char_in_list(cx->actor)", fire_return)
actor_post_fire_liveness = index(run, "!IS_ALIVE(cx->actor)", fire_return)
assert self_post_fire_membership < self_post_fire_liveness
assert actor_post_fire_membership < actor_post_fire_liveness

transfer_start = index(source, "case SP_A_TRANSFER:")
goto_start = index(source, "case SP_A_GOTO:", transfer_start)
damage_start = index(source, "case SP_A_DAMAGE:", goto_start)
transfer = source[transfer_start:goto_start]
goto = source[goto_start:damage_start]
assert contains(transfer, "if (char_to_room(actor, rr, -1))")
assert contains(transfer, "else cx->actor = NULL;")
assert not contains(transfer, "if (!char_to_room(actor, rr, -1))")
assert contains(goto, "if (char_to_room(self, rr, -1))")
assert contains(goto, "cx->self_ch = NULL;")
assert contains(goto, "return ret | SP_X_SELFGONE;")
assert not contains(goto, "if (!char_to_room(self, rr, -1))")

transport_start = index(proclib, "int proclibobj_transporter(")
transport_end = index(proclib, "int proclib_obj_cmd_bridge(", transport_start)
transport = proclib[transport_start:transport_end]
assert contains(
	transport,
	"else if (char_in_list(ch) && IS_ALIVE(ch) && ch->in_room == NOWHERE)",
)

print("studioproc command and movement contracts passed")
