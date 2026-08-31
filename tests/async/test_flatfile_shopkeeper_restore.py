#!/usr/bin/env python3
"""Source contract for catalog-wide shopkeeper staging and publication."""

from _paths import SRC
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SOURCE = (SRC / "flatfile_shopkeeper_restore.c").read_text()

for token in (
    "flatfile_shopkeeper_list(root, &records, error)",
    "mobile_vnums.insert(record.mob_vnum)",
    "shop_index[record.shop_id].keeper != mobile_rnum",
    "number_items_produced",
    "PLAYER_SNAPSHOT_NO_PARENT",
    "flatfile_shopkeeper_materialize(root, record",
    "staged.push_back(materialized)",
    "char_to_room(staged[index].character, staged[index].room_rnum, 0)",
    "staged[index].character->in_room != staged[index].room_rnum",
    "discard_staged(&staged, records)",
    "item_ownership_runtime_forget(item.object_uid)",
    "shop_trade_runtime_replace_revisions(records)",
    "replacements.find(existing) == replacements.end()",
    "extract_char(existing)",
    "shop_index[record.shop_id].dirty = 1",
):
    if token not in SOURCE:
        raise SystemExit(f"shopkeeper catalog restore contract is missing {token}")

stage = SOURCE.index("flatfile_shopkeeper_materialize(root, record")
place = SOURCE.index("char_to_room(staged[index].character")
replace = SOURCE.index("extract_char(existing)")
revisions = SOURCE.index("shop_trade_runtime_replace_revisions(records)")
if not stage < place < revisions < replace:
    raise SystemExit("shopkeepers are not fully staged and placed before incumbent replacement")

print("flat-file shopkeeper catalog restore contract passed")
