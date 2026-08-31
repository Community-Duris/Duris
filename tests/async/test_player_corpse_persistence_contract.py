#!/usr/bin/env python3
"""Contracts for player-corpse SQL identity reconstruction."""
from _paths import SRC
from pathlib import Path

from contract_text import contains, index

ROOT = Path(__file__).resolve().parents[2]
sql_player = (SRC / "sql_player.c").read_text()
migration = (ROOT / "migrations/corpse_persistence_state.sql").read_text()
runner = (ROOT / "migrations/run_migration.sh").read_text()
bootstrap = (ROOT / "migrations/bootstrap_multithread_safe.sql").read_text()
combined = (ROOT / "migrations/pfile_to_db_combined_migration.sql").read_text()
makefile = (ROOT / "Makefile").read_text()
mysql_schema_test = ROOT / "tests/async/run_corpse_persistence_schema_mysql.sh"


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
saver = body(sql_player, "bool sql_save_corpse(P_obj corpse)", last=True)
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

outer_columns = ("name", "weight", "value0", "value1", "value2", "value3", "value4", "value5", "value7")
for column in outer_columns:
    assert contains(migration, f"table_name = 'corpses' AND column_name = '{column}'")
    assert contains(bootstrap, f"`{column}`")
    assert contains(combined, column)

assert contains(runner, '"$SCRIPT_DIR/corpse_persistence_state.sql"')
assert mysql_schema_test.exists(), "database-backed corpse schema regression is missing"
assert contains(makefile, "tests/async/run_corpse_persistence_schema_mysql.sh")
assert contains(saver, "name, weight, ")
assert contains(saver, "value0, value1, value2, value3, value4, value5, value7")
for value_index in range(6):
    assert contains(saver, f"corpse->value[{value_index}]")
assert contains(saver, "corpse->value[7]")
assert contains(loader, "c.short_descr, c.description, c.name, c.weight")
assert contains(loader, "c.value0, c.value1, c.value2, c.value3, c.value4, c.value5, c.value7")
assert contains(loader, "row[CORPSE_COL_WEIGHT]")
assert contains(loader, "row[CORPSE_COL_VALUE0 + value_index]")
assert contains(loader, "row[CORPSE_COL_VALUE7]")
assert contains(loader, "cur_corpse->value[CORPSE_SAVEID] = save_id;")

# Legacy repair is intentionally narrow and does not invent unknown death-time state.
assert contains(migration, "TRIM(short_descr) REGEXP '^[0-9]+$'")
assert contains(migration, "LOWER(TRIM(description)) = LOWER(CONCAT('the corpse of ', player_name))")
assert contains(migration, "WHERE value1 IS NULL")
assert not contains(migration, "SET value2 =")
assert not contains(migration, "SET value4 =")

print("player corpse persistence contracts passed")
