#!/usr/bin/env python3
"""Source contract for custody-fenced shopkeeper dirty saves."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SOURCE = (ROOT / "src/flatfile_shopkeeper_save.c").read_text()

for token in (
    "flatfile_shopkeeper_list(root, &records, error)",
    "existing->revision + 1",
    "flatfile_shopkeeper_capture(shopkeeper, shop_id",
    "captured.mob_vnum != existing->mob_vnum",
    "flatfile_shopkeeper_load_item_ownership(",
    "flatfile_shopkeeper_replace(root, captured, existing->revision, error)",
    "flatfile_shopkeeper_result::stale",
    "if (!shop_index[shop_id].dirty)",
    "real_room(shop_index[shop_id].in_room)",
    "GET_RNUM(candidate) == shop_index[shop_id].keeper",
    "shop_index[shop_id].dirty = 0",
):
    if token not in SOURCE:
        raise SystemExit(f"shopkeeper save contract is missing {token}")

capture = SOURCE.index("flatfile_shopkeeper_capture(shopkeeper, shop_id")
custody = SOURCE.index("flatfile_shopkeeper_load_item_ownership(")
replace = SOURCE.index("flatfile_shopkeeper_replace(root, captured")
clear = SOURCE.index("shop_index[shop_id].dirty = 0")
if not capture < custody < replace < clear:
    raise SystemExit("dirty save does not capture, fence custody, publish, then clear")

print("flat-file custody-fenced shopkeeper save contract passed")
