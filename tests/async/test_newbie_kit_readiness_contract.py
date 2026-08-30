"""Contracts for withholding commands until a durable starter kit is complete."""

from pathlib import Path

from contract_text import contains


ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"

movement = (SRC / "item_movement_transaction.c").read_text()
header = (SRC / "item_movement_transaction.h").read_text()
nanny = (SRC / "nanny.c").read_text()
comm = (SRC / "comm.c").read_text()

queue = movement.split("struct creation_grant_queue", 1)[1].split("};", 1)[0]
assert contains(queue, "bool blocks_actor_commands = false")
assert "item_creation_grant_mark_blocking" in header
assert "item_creation_grant_blocks_commands" in header

newbie_kit = nanny.split("void load_obj_to_newbies(P_char ch)", 1)[1]
newbie_kit = newbie_kit.split("/* check for a legal player name", 1)[0]
assert newbie_kit.rindex("item_creation_grant_mark_blocking(ch)") > newbie_kit.rindex(
    "obj_to_char(shield, ch)"
)

command_gate = comm.split("casting_input =", 1)[1].split("PROFILE_END(commands)", 1)[0]
assert contains(command_gate, "item_creation_grant_blocks_commands(t_ch)")
assert command_gate.index("!creation_grant_input") < command_gate.index("get_from_q(")

output_gate = comm.split("int process_output(P_desc t)", 1)[1].split(
    "int process_input(P_desc t)", 1
)[0]
assert contains(output_gate, "item_creation_grant_blocks_commands(realChar)")
assert output_gate.index("item_creation_grant_blocks_commands(realChar)") < output_gate.index(
    "make_prompt(t)"
)

creation_flow = movement.split("bool creation_grant_request_valid", 1)[1]
creation_flow = creation_flow.split("void account_health", 1)[0]
assert contains(creation_flow, "Your starter kit is ready")
