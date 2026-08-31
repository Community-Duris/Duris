"""Source contracts for the saved_items restore statements.

Two defects motivate these checks.

1. sql_restore_saved_items() and sql_load_saved_item_contents() selected
   item_condition from saved_items.  That column exists on player_items,
   corpse_items, locker_items and player_pet_items but has never been part of
   saved_items, so MySQL rejected the statement with error 1054 (SQLSTATE
   42S22).  db_query() returns NULL on failure and the restore function
   returns early, so one stray column silently stopped every saved ground
   item from being restored at boot.

2. The root-item SELECT omitted wear_flags, item_type, item_material and
   bitvector1..5 while the container-child SELECT read them back.  saved_items
   stores those as v19 "diff" columns where NULL means keep the read_object()
   prototype value, so a top-level ground item quietly reverted them on every
   reboot while the same item inside a container kept them.
"""

from _paths import SRC
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]

INSERT_MARKER = "INSERT INTO saved_items ("
ROOT_MARKER = "FROM saved_items WHERE container_id IS NULL"
CHILD_MARKER = "FROM saved_items WHERE item_key='%s' AND container_id="

sql_player = (SRC / "sql_player.c").read_text()
SQL_LINES = sql_player.splitlines()


def statement_at(marker, back=6, forward=40):
    """Join the C string fragments of the statement containing marker."""
    index = next(n for n, line in enumerate(SQL_LINES) if marker in line)
    window = SQL_LINES[max(0, index - back):min(len(SQL_LINES), index + forward)]
    return " ".join(re.findall(r'"([^"]*)"', "\n".join(window)))


def select_columns(marker):
    statement = statement_at(marker, back=12, forward=2)
    body = re.search(
        r"SELECT\s+(?:DISTINCT\s+)?(.*?)\s+FROM saved_items", statement, re.I
    )
    assert body, f"could not parse the SELECT ending at {marker!r}"
    return [c.strip().split(".")[-1].strip("`") for c in body.group(1).split(",")]


def insert_columns():
    statement = statement_at(INSERT_MARKER, back=2, forward=12)
    body = re.search(r"INSERT INTO saved_items\s*\((.*?)\)", statement, re.I | re.S)
    assert body, "could not parse the saved_items INSERT column list"
    return [c.strip().strip("`") for c in body.group(1).split(",")]


def row_indices(marker, span=130):
    index = next(n for n, line in enumerate(SQL_LINES) if marker in line)
    body = "\n".join(SQL_LINES[index:index + span])
    used = {int(m) for m in re.findall(r"row\[(\d+)\]", body)}
    assert used, f"no row[] uses found after {marker!r}"
    return used


# --- the canonical saved_items column set -----------------------------------
bootstrap = (ROOT / "migrations/bootstrap_multithread_safe.sql").read_text()
create = re.search(r"CREATE TABLE `saved_items` \((.*?)\n\)", bootstrap, re.S)
assert create, "saved_items CREATE TABLE not found in bootstrap_multithread_safe.sql"

schema_columns = set(re.findall(r"^\s+`([a-z_0-9]+)`", create.group(1), re.M))
assert {"item_key", "obj_uid"} <= schema_columns
assert "item_condition" not in schema_columns  # the drift that caused the outage

for path in (ROOT / "migrations").rglob("*"):
    if path.is_file() and path.suffix in {".sql", ".sh"}:
        schema_columns.update(
            re.findall(
                r"ALTER TABLE saved_items ADD COLUMN ([a-z_0-9]+)",
                path.read_text(errors="ignore"),
            )
        )

inserted = insert_columns()
root = select_columns(ROOT_MARKER)
child = select_columns(CHILD_MARKER)

# --- every column named in a saved_items statement must exist ---------------
for name, columns in (("INSERT", inserted), ("root SELECT", root),
                      ("child SELECT", child)):
    for column in columns:
        assert re.fullmatch(r"[a-z_0-9]+", column), f"{name}: unparsed {column!r}"
        assert column in schema_columns, (
            f"saved_items {name} names unknown column {column!r}; "
            f"schema defines {sorted(schema_columns)}"
        )

# --- what the insert persists, the root restore must read back --------------
# container_id is the predicate that splits the two restore queries; quantity
# is bookkeeping the restore path never applies to the object.
for column in set(inserted) - {"container_id", "quantity"}:
    assert column in root, (
        f"saved_items INSERT persists {column!r} but the root-item restore "
        "never reads it back, so it silently reverts to the prototype"
    )

# --- both restore paths must rebuild the same item state --------------------
state_columns = {
    "wear_flags", "item_type", "item_material",
    "bitvector1", "bitvector2", "bitvector3", "bitvector4", "bitvector5",
}
assert state_columns <= set(root), sorted(state_columns - set(root))
assert state_columns <= set(child), sorted(state_columns - set(child))

# --- row[] indices must stay inside each SELECT's column list ---------------
for marker, columns, name in (
    (ROOT_MARKER, root, "sql_restore_saved_items"),
    (CHILD_MARKER, child, "sql_load_saved_item_contents"),
):
    used = row_indices(marker)
    assert max(used) < len(columns), (
        f"{name} reads row[{max(used)}] but its SELECT has only "
        f"{len(columns)} columns"
    )
    # obj_uid is the final selected column in both paths and is always read.
    assert columns[-1] == "obj_uid", f"{name}: obj_uid is no longer last"
    assert max(used) == len(columns) - 1, f"{name}: obj_uid index drifted"

print("saved items column contract passed")
