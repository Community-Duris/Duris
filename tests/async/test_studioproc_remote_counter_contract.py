#!/usr/bin/env python3
"""Contracts for studioproc remote counter writes."""
from pathlib import Path

from contract_text import contains, index

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / "src/studioproc.c").read_text()
header = (ROOT / "src/studioproc.h").read_text()
guide = (ROOT / "docs/legacy/src/howto_trg.txt").read_text()

assert contains(header, "#define SP_A_RSET 22")
assert contains(header, "#define SP_A_RADD 23")

mob_start = index(source, "static P_char sp_mob_instance(")
obj_start = index(source, "static P_obj sp_obj_instance(", mob_start)
context_start = index(source, "struct sp_ctx", obj_start)
mob_lookup = source[mob_start:obj_start]
obj_lookup = source[obj_start:context_start]
assert contains(mob_lookup, "IS_NPC(ch) && IS_ALIVE(ch) && GET_VNUM(ch) == vnum")
assert contains(obj_lookup, "obj->R_num >= 0")
assert contains(obj_lookup, "obj_index[obj->R_num].virtual_number == vnum")

remote_exec = index(source, "case SP_A_RSET:")
exit_exec = index(source, "case SP_A_EXIT:", remote_exec)
executor = source[remote_exec:exit_exec]
assert contains(executor, "if (a->state == SP_T_MOB)")
assert contains(executor, "sp_char_set(m, SP_TAG_COUNTER, a->slot, n);")
assert contains(executor, "sp_obj_set(o, SP_TAG_COUNTER, a->slot, n);")

local_parse = index(source, 'else if (!strcmp(word, "set") || !strcmp(word, "add"))')
remote_parse = index(source, 'else if (!strcmp(word, "rset") || !strcmp(word, "radd"))')
oneof_parse = index(source, 'else if (!strcmp(word, "oneof"))', remote_parse)
assert local_parse < remote_parse < oneof_parse
parser = source[remote_parse:oneof_parse]
assert contains(parser, "real_mobile(n) >= 0")
assert contains(parser, "real_object(n) >= 0")
assert contains(parser, "a->state = SP_T_MOB;")
assert contains(parser, "a->state = SP_T_OBJ;")
assert "silent no-op" in guide

print("studioproc remote counter contracts passed")
