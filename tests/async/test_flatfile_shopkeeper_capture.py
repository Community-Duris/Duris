#!/usr/bin/env python3
"""Source contract for bounded, non-mutating shopkeeper capture."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
HEADER = (ROOT / "src/flatfile_shopkeeper_capture.h").read_text()
SOURCE = (ROOT / "src/flatfile_shopkeeper_capture.c").read_text()

for token in (
    "flatfile_shopkeeper_capture",
    "IS_PC(shopkeeper)",
    "IS_SHOPKEEPER(shopkeeper)",
    "GET_RNUM(shopkeeper)",
    "shopkeeper->in_room",
    "AFFTYPE_NOSAVE",
    "std::unordered_set<const struct affected_type *>",
    "player_item_snapshot_list_capture(",
    "shopkeeper, true, true, false",
    "!item.object_uid",
    "*record_out = std::move(record);",
):
    if token not in SOURCE:
        raise SystemExit(f"shopkeeper capture contract is missing {token}")

if SOURCE.index("player_item_snapshot_list_capture(") > SOURCE.index("*record_out = std::move(record);"):
    raise SystemExit("shopkeeper capture publishes before item traversal succeeds")

for mutation in (
    "equip_char(",
    "unequip_char(",
    "extract_obj(",
    "affect_remove(",
    "obj_from_char(",
    "sql_",
    "db_query(",
):
    if mutation in SOURCE:
        raise SystemExit(f"shopkeeper capture mutates or performs I/O through {mutation}")

if "P_char shopkeeper" not in HEADER or "flatfile_shopkeeper_record *record_out" not in HEADER:
    raise SystemExit("shopkeeper capture header does not expose the typed adapter")

print("flat-file shopkeeper capture contract passed")
