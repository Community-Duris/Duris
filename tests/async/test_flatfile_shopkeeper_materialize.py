#!/usr/bin/env python3
"""Source contract for fail-closed detached shopkeeper materialization."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
HEADER = (ROOT / "src/flatfile_shopkeeper_materialize.h").read_text()
SOURCE = (ROOT / "src/flatfile_shopkeeper_materialize.c").read_text()

for token in (
    "real_mobile(record.mob_vnum)",
    "real_room(record.room_vnum)",
    "flatfile_shopkeeper_load_item_ownership(",
    "read_mobile(mobile_rnum, REAL)",
    "IS_SHOPKEEPER(character)",
    "GET_BIRTHPLACE(character) = record.room_vnum",
    "affect_to_char(character, &affect)",
    "player_load_item_graph_materialize_for_owner(",
    "flatfile_shopkeeper_item_owner(record.shop_id)",
    "owner_revision, true, true",
    "staged.release()",
    "*materialized = output",
):
    if token not in SOURCE:
        raise SystemExit(f"shopkeeper materializer contract is missing {token}")

if SOURCE.index("flatfile_shopkeeper_load_item_ownership(") > SOURCE.index(
    "read_mobile(mobile_rnum, REAL)"
):
    raise SystemExit("shopkeeper is allocated before authority reconciliation succeeds")
if SOURCE.index("player_load_item_graph_materialize_for_owner(") > SOURCE.index(
    "staged.release()"
):
    raise SystemExit("shopkeeper is released before item materialization succeeds")
if SOURCE.index("staged.release()") > SOURCE.index("*materialized = output"):
    raise SystemExit("shopkeeper output is published before staged ownership is released")
if "extract_char(character)" not in SOURCE or "~staged_shopkeeper()" not in SOURCE:
    raise SystemExit("shopkeeper materializer lacks failure cleanup")

for token in (
    "flatfile_shopkeeper_materialize_result",
    "flatfile_materialized_shopkeeper",
    "player_load_item_materialize_metrics *item_metrics",
):
    if token not in HEADER:
        raise SystemExit(f"shopkeeper materializer header is missing {token}")

print("flat-file detached shopkeeper materialization contract passed")
