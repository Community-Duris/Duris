#!/usr/bin/env python3
"""Runtime shop revision and bounded payload-builder regression."""

import pathlib
import subprocess
import tempfile


ROOT = pathlib.Path(__file__).resolve().parents[2]
SOURCE = (ROOT / "src/shop_trade_runtime.c").read_text()

for token in (
    "player_item_snapshot_tree_capture(selected",
    "player_item_snapshot_list_encode(snapshots",
    "item_ownership_runtime_lookup(snapshot.object_uid",
    "runtime.root_item_uid != selected->obj_uid",
    "runtime.parent_item_uid != parent_uid",
    "std::sort(built.items.begin()",
    "ITEM_TRANSFER_ABSENT_REVISION",
    "item_ownership_runtime_lookup(stock->obj_uid",
    "shop_trade_command_encode_payload(built",
):
    if token not in SOURCE:
        raise SystemExit(f"shop trade payload builder is missing {token}")
for mutation in ("obj_from_char(", "obj_to_char(", "extract_obj(", "ADD_MONEY(", "SUB_MONEY("):
    if mutation in SOURCE:
        raise SystemExit(f"shop trade payload builder mutates live state through {mutation}")

HARNESS = r'''
#include "shop_trade_runtime.h"

#include <cassert>
#include <cstdint>
#include <limits>

int main()
{
	shop_trade_runtime_reset_for_tests();
	uint64_t revision = 0;
	assert(!shop_trade_runtime_revision(0, &revision));
	flatfile_shopkeeper_record first = {};
	first.shop_id = 0;
	first.revision = 4;
	flatfile_shopkeeper_record second = {};
	second.shop_id = 7;
	second.revision = 9;
	assert(shop_trade_runtime_replace_revisions({ first, second }));
	assert(shop_trade_runtime_revision(0, &revision) && revision == 4);
	assert(shop_trade_runtime_can_advance(0, 4, 5));
	assert(!shop_trade_runtime_can_advance(0, 3, 4));
	assert(shop_trade_runtime_advance(0, 4, 5));
	assert(shop_trade_runtime_revision(0, &revision) && revision == 5);
	assert(!shop_trade_runtime_advance(0, 4, 5));
	assert(!shop_trade_runtime_replace_revisions({ first, first }));
	assert(shop_trade_runtime_revision(0, &revision) && revision == 5);
	first.revision = std::numeric_limits<uint64_t>::max();
	assert(shop_trade_runtime_replace_revisions({ first }));
	assert(!shop_trade_runtime_can_advance(0, first.revision, 0));
}
'''

with tempfile.TemporaryDirectory(prefix="duris-shop-trade-runtime-") as temporary:
    source = pathlib.Path(temporary) / "shop_trade_runtime_test.cpp"
    binary = pathlib.Path(temporary) / "shop_trade_runtime_test"
    source.write_text(HARNESS)
    compiled = subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Wpedantic",
            "-Werror",
            "-ffunction-sections",
            "-fdata-sections",
            "-Isrc",
            str(source),
            "src/shop_trade_runtime.c",
            "-Wl,--gc-sections",
            "-o",
            str(binary),
        ],
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    if compiled.returncode:
        raise SystemExit(compiled.stdout)
    run = subprocess.run(
        [str(binary)],
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    if run.returncode:
        raise SystemExit(run.stdout)

print("shop trade runtime revision and payload-builder contracts passed")
