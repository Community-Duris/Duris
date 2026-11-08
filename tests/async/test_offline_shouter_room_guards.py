#!/usr/bin/env python3
"""A shout from a character in no room reads no room.

poof_artifact() has an artifact's owner shout even when the owner is a dummy loaded off
the loop, which is in no room (NOWHERE). do_shout() hands that owner to act() for each
listener, and act() names it through PERS() and CAN_SEE(). ac_can_see() asked
IS_MAP_ROOM(obj->in_room) before checking the room was real, and PERS() asked
CAN_NIGHTPEOPLE_SEE()/CAN_DAYPEOPLE_SEE() of the shouter's room unchecked: both read
world[-1]. IS_TWILIGHT_ROOM() and IS_OUTDOORS() now refuse an index outside the world too.
"""
import re

from _paths import source

utility = source("utility.c").read_text()
actcomm = source("actcomm.c").read_text()
boards = source("boards.c").read_text()


def body(text, signature):
    start = text.index(signature)
    return text[start:text.index("\n}\n", start)]


see = body(utility, "bool ac_can_see(P_char sub, P_char obj, bool")
assert "if (oroom >= 0 && oroom <= top_of_world && !IS_MAP_ROOM(oroom))" in see

pers = body(utility, "char *PERS(P_char ch, P_char vict, int short_d, bool noansi)")
guard = pers.index("else if (ch->in_room >= 0 && ch->in_room <= top_of_world)")
assert guard < pers.index("CAN_NIGHTPEOPLE_SEE(ch->in_room)")
assert guard < pers.index("CAN_DAYPEOPLE_SEE(ch->in_room)")

for name in ("bool IS_TWILIGHT_ROOM(int r)", "bool IS_OUTDOORS(int r)"):
    fn = body(utility, name)
    assert fn.index("if (r < 0 || r > top_of_world)") < fn.index("world[r]"), name

shout = body(actcomm, "void do_shout(P_char ch, char *argument, int")
assert re.search(r"ch->in_room >= 0 && ch->in_room <= top_of_world &&\s*\(IS_ROOM\(ch->in_room",
                 shout)

find = body(boards, "int find_board(P_obj obj)\n")
assert find.index("if (!obj)") < find.index("obj->R_num")

print("a shout from a character in no room reads no room")
