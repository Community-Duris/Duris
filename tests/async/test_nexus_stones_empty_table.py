#!/usr/bin/env python3
"""An empty nexus_stones table is a valid world state, not a boot failure.

Both persistence backends must agree: zero stones loads cleanly and still
publishes the stat modifiers. The MariaDB branch used to reject an empty
result set, which logged a spurious load failure on every boot of a world
that simply has no nexus stones yet.
"""
from pathlib import Path
from contract_text import contains, index

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / "src/nexus_stones.c").read_text()

start = index(source, "int load_nexus_stones()")
end = index(source, "bool nexus_stone_info(", start)
loader = source[start:end]

flat, sql = loader.split("#else", 1)

# The MariaDB branch must not treat "no rows" as a failure.
assert not contains(sql, "mysql_num_rows(res)<1"), \
    "empty nexus_stones result set must not be rejected as a load failure"

# Both branches still publish the stat modifiers on the success path.
for name, branch in (("flatfile", flat), ("mariadb", sql)):
    assert contains(branch, "update_nexus_stat_mods();"), \
        f"{name} nexus loader must refresh stat modifiers"
    assert index(branch, "update_nexus_stat_mods();") < index(branch, "returnTRUE;"), \
        f"{name} nexus loader must refresh stat modifiers before succeeding"

# Genuine failures are still rejected.
assert contains(sql, "if(!qry(") and contains(sql, "returnFALSE;")
assert contains(sql, "if(!res)")

print("nexus stone empty-table load contract passed")
