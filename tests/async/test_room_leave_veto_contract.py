#!/usr/bin/env python3
"""Regression contract for room-hook leave veto and duplicate room insertion."""
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[2]
handler = (ROOT / "src/handler.c").read_text()
actmove = (ROOT / "src/actmove.c").read_text()
lockers = (ROOT / "src/storage_lockers.c").read_text()
interp = (ROOT / "src/interp.h").read_text()
rooms = (ROOT / "src/specs.room.c").read_text()

assert "#define CMD_FROMROOM" in interp
assert "#define ROOM_PROC_LEAVE_VETO" in interp

char_from_start = handler.index("void char_from_room(P_char ch)")
char_to_start = handler.index("bool char_to_room(P_char ch, int room, int dir)")
char_from = handler[char_from_start:char_to_start]
char_to = handler[char_to_start:handler.index("void obj_from_room", char_to_start)]

# Legacy room procedures commonly return TRUE for handled synthetic events.
warning_start = rooms.index("int fw_warning_room(")
warning_end = rooms.index("\n}\n", warning_start) + 3
warning = rooms[warning_start:warning_end]
assert "return TRUE;" in warning

# Only the explicit veto result may block removal; ordinary TRUE must not.
assert "== ROOM_PROC_LEAVE_VETO" in char_from
assert "CMD_FROMROOM" in char_from
assert "if ((*world[ch->in_room].funct)(ch->in_room, ch, (-75), NULL))" not in char_from

# The locker failure path is the sole explicit leave-veto producer.
leave_start = lockers.index("if (cmd == CMD_FROMROOM)")
leave = lockers[leave_start:leave_start + 220]
assert "return ROOM_PROC_LEAVE_VETO;" in leave

# The ordinary movement path must not insert after a vetoed/failed unlink.
move_start = actmove.index("int do_simple_move_skipping_procs")
move_end = actmove.index("int do_simple_move(", move_start)
move = actmove[move_start:move_end]
to_pos = move.rfind("char_to_room(ch, new_room")
assert to_pos != -1
from_pos = move.rfind("char_from_room(ch);", 0, to_pos)
assert from_pos != -1
between = move[from_pos:to_pos]
assert re.search(r"ch->in_room\s*!=\s*NOWHERE", between)
assert "return FALSE;" in between

# Shared boundary: no caller may insert a character still linked to a room.
pre_insert = char_to[:char_to.index("ch->next_in_room   = world[room].people;")]
assert re.search(r"ch->in_room\s*!=\s*NOWHERE", pre_insert)
assert "return FALSE;" in pre_insert

print("room leave-veto and duplicate-insertion contract passed")
