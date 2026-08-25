#!/usr/bin/env python3
"""Regression contract for the object proclib transporter and the displaced-proc chain.

Two defects are pinned here.

1.  proclibobj_transporter() tested char_to_room() inverted.  char_to_room() is
    bool and returns TRUE on success (handler.c), so the arrival act and the
    look only ran when the move had FAILED.  On the failure path that meant
    announcing an arrival and looking while the character sat in NOWHERE.

2.  proclibObj_add() refused to install the bridge on a vnum that already had
    an object proc.  That protected the incumbent but made every instance
    proclib on such a vnum permanently unreachable, silently.  The bridge is
    now always installed and the displaced proc is called first.
"""
from pathlib import Path
import re
from contract_text import contains, find, index

ROOT = Path(__file__).resolve().parents[2]
handler = (ROOT / "src/handler.c").read_text()
proclib = (ROOT / "src/studioproclib.c").read_text()
proclib_h = (ROOT / "src/studioproclib.h").read_text()
library = (ROOT / "src/specs.library.c").read_text()


def slice_fn(text, start_marker, *, end_marker=None):
    start = text.index(start_marker)
    end = text.index(end_marker, start) if end_marker else text.index("\n}\n", start) + 3
    return text[start:end]


# ---------------------------------------------------------------------------
# The boundary the transporter depends on: char_to_room() is bool, TRUE means
# the character made it in, and every FALSE it can return to a dir < 0 caller
# is raised BEFORE the character is placed.
# ---------------------------------------------------------------------------
char_to = slice_fn(handler, "bool char_to_room(P_char ch, int room, int dir)",
                   end_marker="void obj_from_room")
assert contains(handler, "// Returns TRUE iff char made it into the room.")
placed = index(char_to, "ch->in_room = room;")
pre_insert = char_to[:index(char_to, "ch->next_in_room   = world[room].people;")]
assert contains(pre_insert, "if (!IS_ALIVE(ch))")
assert re.search(r"ch->in_room\s*!=\s*NOWHERE", pre_insert)
assert contains(pre_insert, "return FALSE;")
# dir < 0 returns TRUE straight after placement, so no post-placement FALSE
# can reach the transporter's call site.
early_true = index(char_to, "if (dir < 0) /* flag value, skip aggro checks */")
assert early_true > placed
assert contains(char_to[early_true:early_true + 120], "return TRUE;")

# ---------------------------------------------------------------------------
# 1. The transporter tests the return the right way round, and recovers from a
#    refusal by testing the STATE rather than the return.
# ---------------------------------------------------------------------------
transporter = slice_fn(proclib, "int proclibobj_transporter(P_obj obj, P_char ch, int cmd, char *argument)")

# was_in must be read while the character is still in the departure room.
was_in_pos = index(transporter, "was_in = ch->in_room;")
from_pos = index(transporter, "char_from_room(ch);")
assert was_in_pos < from_pos

# The success test is not inverted, and the arrival act plus the look sit
# inside it.
success = index(transporter, "if (char_to_room(ch, rnum, -1))", from_pos)
assert not contains(transporter, "if (!char_to_room(")
recovery = index(transporter, "else if", success)
success_body = transporter[success:recovery]
assert contains(success_body, "$n arrives in a swirl of mist.")
assert contains(success_body, "do_look(ch, empty, CMD_LOOK);")

# The recovery branch keys off ch->in_room == NOWHERE, not off the return
# value, because handler.c also returns FALSE from points where the character
# IS already in the destination; re-inserting there would be a duplicate.
recovery_body = transporter[recovery:index(transporter, "return TRUE;", recovery)]
assert re.search(r"ch->in_room\s*==\s*NOWHERE", recovery_body)
assert contains(recovery_body, "IS_ALIVE(ch)")
assert contains(recovery_body, "char_to_room(ch, was_in, -1);")
assert not contains(recovery_body, "$n arrives in a swirl of mist.")
assert not contains(recovery_body, "do_look(")

# ---------------------------------------------------------------------------
# 2. proclibObj_add() installs the bridge even when the slot is taken, and
#    hands the incumbent to the chain instead of standing down.
# ---------------------------------------------------------------------------
add = slice_fn(library, "int proclibObj_add(P_obj obj, char *procName, char *args)")
# The old refuse-if-occupied guard is gone.
assert not contains(add, "!obj_index[obj->R_num].func.obj)")
assert contains(add, "obj_index[obj->R_num].func.obj != proclib_obj_cmd_bridge")
chain_call = index(add, "proclib_chain_install(obj->R_num, obj_index[obj->R_num].func.obj);")
install = index(add, "obj_index[obj->R_num].func.obj = proclib_obj_cmd_bridge;")
assert chain_call < install, "the incumbent must be remembered before it is overwritten"
assert contains(proclib_h, "void proclib_chain_install(int rnum, int (*prev)(P_obj, P_char, int, char *));")

# The chain never wraps itself and never records the same vnum twice.
chain_install = slice_fn(proclib, "void proclib_chain_install(int rnum, int (*prev)(P_obj, P_char, int, char *))")
assert contains(chain_install, "prev == proclib_obj_cmd_bridge")
dedupe = chain_install[:index(chain_install, "if (proclib_chain_top == proclib_chain_cap)")]
assert contains(dedupe, "proclib_chain[i].rnum == rnum")
assert contains(dedupe, "return;")

# ---------------------------------------------------------------------------
# 3. Dispatch order: the displaced proc runs first and a TRUE from it wins.
# ---------------------------------------------------------------------------
bridge = slice_fn(proclib, "int proclib_obj_cmd_bridge(P_obj obj, P_char ch, int cmd, char *argument)")
prev_call = index(bridge, "if (prev(obj, ch, cmd, argument))")
assert contains(bridge[prev_call:bridge.index("\n", bridge.index("\n", prev_call) + 1) + 40], "return TRUE;")
dispatch = index(bridge, "return proclib_obj_proc(obj, ch, cmd, argument);")
assert prev_call < dispatch, "the incumbent must be consulted before any proclib"
# The bridge's own cmd filter must not gate the incumbent: the displaced proc
# keeps its original semantics, including the cmd values the bridge ignores.
assert prev_call < index(bridge, "if (cmd <= 0 || cmd == CMD_SAY)")

print("proclib transporter and displaced-proc chain contract passed")
