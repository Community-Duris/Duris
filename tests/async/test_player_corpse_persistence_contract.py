#!/usr/bin/env python3
"""Contracts for player-corpse SQL identity reconstruction."""
from pathlib import Path

from contract_text import contains, index

ROOT = Path(__file__).resolve().parents[2]
sql_player = (ROOT / "src/sql_player.c").read_text()


def body(text, signature, last=False):
    """Return one function from its signature through its closing brace."""
    start = text.rindex(signature) if last else text.index(signature)
    depth = 0
    opening = text.index("{", start)
    for position in range(opening, len(text)):
        if text[position] == "{":
            depth += 1
        elif text[position] == "}":
            depth -= 1
            if depth == 0:
                return text[start : position + 1]
    raise AssertionError(f"unterminated function: {signature}")


# sql_player.c has a no-database stub first; inspect the real implementation.
loader = body(sql_player, "bool sql_load_all_corpses(void)\n{", last=True)
identity_start = index(sql_player, "static void sql_restore_corpse_identity(")
identity_end = index(sql_player, "bool sql_load_all_corpses(void)", identity_start)
identity = sql_player[identity_start:identity_end]

column_order = (
    "CORPSE_COL_ITEM_UID",
    "CORPSE_COL_ITEM_CONDITION",
    "CORPSE_COL_SHORT_DESCRIPTION",
    "CORPSE_COL_DESCRIPTION",
)
columns_start = index(sql_player, "enum corpse_load_column")
positions = [index(sql_player, name, columns_start) for name in column_order]
assert positions == sorted(positions), "corpse and item result columns must remain in SELECT order"

assert contains(loader, "mysql_num_fields(result) != CORPSE_COL_COUNT")
assert contains(loader, "row[CORPSE_COL_SHORT_DESCRIPTION]")
assert contains(loader, "row[CORPSE_COL_DESCRIPTION]")
assert contains(loader, "row[CORPSE_COL_ITEM_CONDITION]")
assert not contains(loader, "cur_corpse->short_description = str_dup(row[")
assert not contains(loader, "cur_corpse->description = str_dup(row[")

assert contains(identity, "set_keywords(corpse, keywords);")
assert contains(identity, "set_short_description(corpse, short_description);")
assert contains(identity, "set_long_description(corpse, description);")
assert contains(identity, '"%s corpse _pcorpse_"')
assert contains(identity, "corpse->str_mask |= STRUNG_DESC3;")

print("player corpse persistence contracts passed")
