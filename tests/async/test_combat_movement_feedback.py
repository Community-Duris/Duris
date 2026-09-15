#!/usr/bin/env python3
"""Verify combat direction refusals tell players how to escape."""

from _paths import SRC
from _source_contract import function_body


INTERP = (SRC / "interp.c").read_text(encoding="utf-8", errors="replace")
MOVEMENT = function_body(INTERP, r"static bool\s+is_normal_movement_command\s*\(")
COMMAND = function_body(
    INTERP, r"void\s+command_interpreter\s*\(\s*P_char\s+ch"
)

assert "CMD_NORTH && cmd <= CMD_DOWN" in MOVEMENT
assert "CMD_NORTHWEST && cmd <= CMD_SE" in MOVEMENT
assert "You cannot move normally while fighting; use 'flee' to escape." in COMMAND
assert "is_normal_movement_command(cmd) ?" in COMMAND
assert "Sorry, you aren't allowed to do that in combat." in COMMAND

print("combat movement feedback: direction refusals explain flee")
