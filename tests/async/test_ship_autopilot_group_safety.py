#!/usr/bin/env python3
"""Source regressions for ship-audit review fixes."""

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

# Projection must clamp to getmap()'s populated 0..99 range without changing
# the map's established 100-y coordinate inversion.
assert contains(source, "#define AUTOPILOT_MAP_MAX 99")
assert contains(source, "int map_y = 100 - (int)(ydist + ship->y);")

npc_source = (SRC / "ships/ship_npc.c").read_text()
start = npc_source.index("void setup_npc_caravel_03(")
end = npc_source.index("void setup_npc_caravel_04(", start)
caravel = npc_source[start:end]
for slot in (1, 2, 3):
    assert contains(caravel, f"set_weapon(ship, {slot}, W_MEDIUM_BAL, SIDE_PORT);")
assert caravel.count("W_MEDIUM_BAL, SIDE_PORT") == 3

shop_source = (SRC / "ships/ship_shop.c").read_text()
assert not contains(shop_source, "a rejected transaction refunds through")
assert not contains(shop_source, "you cannot re-hull")
assert contains(shop_source, "wartime hull changes take five times longer")

print("ship audit review regressions passed")
