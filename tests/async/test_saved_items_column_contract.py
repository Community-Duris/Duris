"""Source contract: every saved_items query names a column the schema defines.

sql_restore_saved_items() and sql_load_saved_item_contents() once selected
item_condition from saved_items.  That column exists on player_items,
corpse_items, locker_items and player_pet_items, but has never been part of
saved_items in any migration, so MySQL rejected the SELECT with error 1054
(SQLSTATE 42S22).  db_query() returns NULL on failure and the restore function
returns early, so a single stray column silently stopped every saved ground
item from being restored at boot.
"""

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]

# --- the canonical saved_items definition -----------------------------------
# bootstrap_multithread_safe.sql is the authoritative full-schema bootstrap.
bootstrap = (ROOT / "migrations/bootstrap_multithread_safe.sql").read_text()
create = re.search(
    r"CREATE TABLE `saved_items` \((.*?)\n\)", bootstrap, re.S
)
assert create, "saved_items CREATE TABLE not found in bootstrap_multithread_safe.sql"

schema_columns = set(re.findall(r"^\s+`([a-z_0-9]+)`", create.group(1), re.M))
assert "item_key" in schema_columns and "obj_uid" in schema_columns
# Guard the specific drift that caused the outage.
assert "item_condition" not in schema_columns

# Later guarded migrations add columns to saved_items; accept those too.
migrations = ROOT / "migrations"
for path in migrations.rglob("*"):
    if path.is_file() and path.suffix in {".sql", ".sh"}:
        schema_columns.update(
            re.findall(
                r"ALTER TABLE saved_items ADD COLUMN ([a-z_0-9]+)",
                path.read_text(errors="ignore"),
            )
        )

# --- every column named in a saved_items query must exist -------------------
sql_player = (ROOT / "src/sql_player.c").read_text()

# Grab each C string-concatenated statement that touches saved_items.
for match in re.finditer(
    r'(?:"[^"]*"\s*)*"[^"]*\bsaved_items\b[^"]*"(?:\s*"[^"]*")*', sql_player
):
    statement = " ".join(re.findall(r'"([^"]*)"', match.group(0)))
    if "saved_item_affects" in statement or "saved_item_extra_descr" in statement:
        continue

    select = re.search(r"SELECT\s+(?:DISTINCT\s+)?(.*?)\s+FROM\s+saved_items", statement, re.I)
    insert = re.search(r"INSERT INTO saved_items\s*\((.*?)\)", statement, re.I | re.S)
    column_list = (select or insert).group(1) if (select or insert) else None
    if not column_list:
        continue

    for raw in column_list.split(","):
        column = raw.strip().split()[-1] if raw.strip() else ""
        column = column.split(".")[-1].strip("`")
        if not column or not re.fullmatch(r"[a-z_0-9]+", column):
            continue  # %s placeholders, literals, COALESCE(...) fragments
        assert column in schema_columns, (
            f"saved_items query names unknown column {column!r}; "
            f"schema defines {sorted(schema_columns)}"
        )

print("saved items column contract passed")
