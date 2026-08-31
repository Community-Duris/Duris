#!/usr/bin/env python3
"""Registration and safety contracts for the read-only builder zone audit."""
from _paths import SRC
from pathlib import Path
import re

from contract_text import contains, index

ROOT = Path(__file__).resolve().parents[2]
source = (SRC / "zcheck.c").read_text()
interp = (SRC / "interp.c").read_text()
interp_h = (SRC / "interp.h").read_text()
config = (SRC / "config.h").read_text()
makefile = (SRC / "Makefile").read_text()
prototypes = (SRC / "prototypes.h").read_text()

table = re.search(r"const char \*command\[MAX_CMD\] = \{(.*?)\n\};", interp, re.DOTALL)
assert table is not None
entries = re.findall(r'"((?:\\.|[^"\\])*)"', table.group(1))
assert entries[-1] == "\\n"
assert int(re.search(r"#define MAX_CMD\s+(\d+)", config).group(1)) == len(entries)
cmd = int(re.search(r"#define CMD_ZCHECK\s+(\d+)", interp_h).group(1))
assert entries[cmd - 1] == "zcheck"
assert contains(interp, "CMD_Y(CMD_ZCHECK, STAT_DEAD + POS_PRONE, do_zcheck, IMMORTAL, FALSE);")
assert contains(prototypes, "void do_zcheck(P_char, char *, int);")
assert "zcheck.o" in makefile

body_start = index(source, "void do_zcheck(")
body = source[body_start:]
for required in (
	"UNREACHABLE ROOMS",
	"ONE-WAY TRAPS",
	"RECIPROCITY GAPS",
	"LOCKED DOORS / BLOCKED WALLS",
	"RESET CHOREOGRAPHY",
	"Read-only report: nothing was changed.",
):
	assert required in body

# Prototype-less runtime objects may exist; never index obj_index with R_num -1.
switch_scan = index(body, "for (P_obj o = object_list;")
lock_section = index(body, "4. LOCKED DOORS", switch_scan)
switches = body[switch_scan:lock_section]
assert contains(switches, "o->type != ITEM_SWITCH || o->R_num < 0")
assert index(switches, "o->R_num < 0") < index(switches, "obj_index[o->R_num]")

# The command audits loaded state; it must not invoke mutation or file APIs.
for forbidden in (
	"reset_zone(",
	"char_to_room(",
	"char_from_room(",
	"extract_obj(",
	"extract_char(",
	"fopen(",
	"fwrite(",
):
	assert not contains(body, forbidden)

print("zcheck registration and read-only contracts passed")
