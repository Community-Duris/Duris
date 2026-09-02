#!/usr/bin/env python3
"""Source regression for safe non-leader autopilot group removal."""

from _paths import SRC
from contract_text import contains


source = (SRC / "ships/ship_auto.c").read_text()
start = source.index("int shipgroupremove(")
end = source.index("int shipgroupadd(", start)
body = source[start:end]
member = body[body.rindex("\ttmpgroup2 = autopilot->group;") :]

# A member node must be located through its predecessor and unlinked before it
# is freed.  This also prevents the old loop from advancing through a null or
# freed cursor after autopilot->group is cleared.
assert contains(member, "while (tmpgroup && tmpgroup->next != tmpgroup2)")
splice = member.index("tmpgroup->next = tmpgroup2->next;")
release = member.index("FREE(tmpgroup2);")
clear = member.index("autopilot->group = NULL;", release)
assert splice < release < clear
assert not contains(member, "if ((tmpgroup = autopilot->group))")

print("autopilot group member removal unlinks before release")
