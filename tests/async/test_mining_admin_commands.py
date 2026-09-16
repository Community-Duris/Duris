#!/usr/bin/env python3
"""Regression contract for the trusted mining administration command."""

import re

from _paths import SRC
from _source_contract import function_body


source = (SRC / "economy" / "mining.c").read_text()
do_mine = function_body(source, r"\bvoid\s+do_mine\s*\(")
assert do_mine is not None, "do_mine definition not found"

# half_chop puts the subcommand in arg1 and the complete target portion in
# arg2. The admin paths must not compare the original two-word command with a
# region abbreviation, and purge must not read an unrelated/uninitialised
# buffer.
assert "half_chop(arg, arg1, arg2);" in do_mine
assert 'if (!strcmp(arg1, "load") && IS_TRUSTED(ch))' in do_mine
assert 'if (!strcmp(arg1, "purge") && IS_TRUSTED(ch))' in do_mine
assert "isname(arg2, mine_data[i].abbrev)" in do_mine
assert 'const bool purge_all = !strcmp(arg2, "all");' in do_mine
assert "char buff" not in do_mine
assert 'strcmp(arg, mine_data[i].abbrev)' not in do_mine
assert 'strcmp(arg, "all")' not in do_mine

# Purge must validate the selected region before indexing mine_data and must
# save the successor before extraction so empty, one-node, and tail nodes are
# all handled safely.
region_guard = "if (!purge_all && region < 0)"
assert region_guard in do_mine
assert do_mine.index(region_guard) < do_mine.index("mine_data[region].type")
assert "for (P_obj tobj = object_list; tobj;)" in do_mine
assert "P_obj next = tobj->next;" in do_mine
assert "tobj = next;" in do_mine
assert "for (; tobj && next;" not in do_mine
assert "P_obj next = object_list->next" not in do_mine

# loc.room is a union member; only inspect it after the object is known to be
# in a valid room. Matching the configured type/range also covers gem mines
# and keeps purge aligned with load_mines().
assert "IS_SET(tobj->loc_p, LOC_ROOM)" in do_mine
assert "tobj->loc.room <= top_of_world" in do_mine
assert "OBJ_VNUM(tobj) == mine_data[i].type" in do_mine
assert re.search(
    r'mining_config_region_value\(\s*i,\s*"start",\s*mine_data\[i\]\.start\)',
    do_mine,
)
assert re.search(
    r'mining_config_region_value\(\s*i,\s*"end",\s*mine_data\[i\]\.end\)',
    do_mine,
)

print("mining admin command regression contract passed")
