#!/usr/bin/env python3
"""Regression contracts for studioproc commands and movement lifetime safety."""
from pathlib import Path

from contract_text import contains, index

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / "src/studioproc.c").read_text()
proclib = (ROOT / "src/studioproclib.c").read_text()

command_start = index(source, "static int sp_do_command(")
command_end = index(source, "static int sp_execute(", command_start)
command = source[command_start:command_end]
assert contains(command, "if (!MIN_POS(ch, cmd_info[cmd].minimum_position))")
assert not contains(command, "GET_POS(ch) < cmd_info[cmd].minimum_position")

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
