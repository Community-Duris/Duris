"""Player-facing contracts for the transition from death to the account menu."""

from _paths import SRC
from pathlib import Path

from contract_text import contains


ROOT = Path(__file__).resolve().parents[2]
fight = (SRC / "fight.c").read_text()
comm = (SRC / "comm.c").read_text()
interp = (SRC / "interp.c").read_text()

die = fight.split("void die(P_char ch, P_char killer)", 1)[1]
die = die.split("void kill_gain(P_char ch, P_char victim)", 1)[0]
assert contains(die, "Your wounds claim you at last. Your spirit slips free")
assert die.index("Your wounds claim you at last") < die.index("make_corpse(ch, loss)")

output = comm.split("int process_output(P_desc t)", 1)[1]
output = output.split("int process_input(P_desc t)", 1)[0]
assert contains(output, "GET_STAT(realChar) == STAT_DEAD")
assert output.index("GET_STAT(realChar) == STAT_DEAD") < output.index("make_prompt(t)")

commands = interp.split("void command_interpreter(", 1)[1]
assert contains(commands, "the afterlife is drawing you onward")
assert "Lie still; you are DEAD!!!" not in commands

print("player death transition contracts passed")
