#!/usr/bin/env python3
"""Regression contract for room-hook leave veto and duplicate room insertion."""
from pathlib import Path
import re
from contract_text import contains, count, find, index

ROOT = Path(__file__).resolve().parents[2]
handler = (ROOT / "src/handler.c").read_text()
actmove = (ROOT / "src/actmove.c").read_text()
lockers = (ROOT / "src/storage_lockers.c").read_text()
interp = (ROOT / "src/interp.h").read_text()
rooms = (ROOT / "src/specs.room.c").read_text()

assert contains(interp, "#define CMD_FROMROOM")
assert contains(interp, "#define ROOM_PROC_LEAVE_VETO")

char_from_start = index(handler, "void char_from_room(P_char ch)")
char_to_start = index(handler, "bool char_to_room(P_char ch, int room, int dir)")
char_from = handler[char_from_start:char_to_start]
char_to = handler[char_to_start:index(handler, "void obj_from_room", char_to_start)]

# Legacy room procedures commonly return TRUE for handled synthetic events.
warning_start = index(rooms, "int fw_warning_room(")
warning_end = rooms.index("\n}\n", warning_start) + 3
warning = rooms[warning_start:warning_end]
assert contains(warning, "return TRUE;")

# Only the explicit veto result may block removal; ordinary TRUE must not.
assert contains(char_from, "== ROOM_PROC_LEAVE_VETO")
assert contains(char_from, "CMD_FROMROOM")
assert not contains(char_from, "if ((*world[ch->in_room].funct)(ch->in_room, ch, (-75), NULL))")

# The locker failure path is the sole explicit leave-veto producer.
leave_start = index(lockers, "if (cmd == CMD_FROMROOM)")
leave = lockers[leave_start:leave_start + 220]
assert contains(leave, "return ROOM_PROC_LEAVE_VETO;")

leave_handler_start = index(lockers, "static bool locker_handle_leave")
leave_handler_end = lockers.index("\n}\n", leave_handler_start) + 3
leave_handler = lockers[leave_handler_start:leave_handler_end]
assert contains(leave_handler, "P_char nextChar = NULL;")
assert contains(leave_handler, "nextChar = tmpChar->next_in_room;")
assert contains(leave_handler, "if (tmpChar != ch)")
assert contains(leave_handler, "tmpChar = nextChar;")
# The list head is re-read exactly once, at the top of the walk.
assert count(leave_handler, "tmpChar = world[ch->in_room].people;") == 1

# The ordinary movement path must not insert after a vetoed/failed unlink.
move_start = index(actmove, "int do_simple_move_skipping_procs")
move_end = index(actmove, "int do_simple_move(", move_start)
move = actmove[move_start:move_end]
to_pos = move.rfind("char_to_room(ch, new_room")
assert to_pos != -1
from_pos = move.rfind("char_from_room(ch);", 0, to_pos)
assert from_pos != -1
between = move[from_pos:to_pos]
assert re.search(r"ch->in_room\s*!=\s*NOWHERE", between)
assert contains(between, "return FALSE;")

# Shared boundary: no caller may insert a character still linked to a room.
pre_insert = char_to[:index(char_to, "ch->next_in_room   = world[room].people;")]
assert re.search(r"ch->in_room\s*!=\s*NOWHERE", pre_insert)
assert contains(pre_insert, "return FALSE;")

print("room leave-veto and duplicate-insertion contract passed")
