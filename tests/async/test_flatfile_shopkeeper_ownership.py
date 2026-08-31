#!/usr/bin/env python3
"""Exact shopkeeper aggregate/global-item ownership reconciliation regression."""

from _paths import rel
import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
HARNESS = r'''
#include "flatfile_shopkeeper_ownership.h"

#include <cassert>

flatfile_item_repository_result flatfile_item_repository_load_owner(
	const std::string &, const item_owner_identity &, uint64_t *,
	std::vector<flatfile_item_ownership_record> *, std::string *)
{
	return flatfile_item_repository_result::io_error;
}

int main()
{
	const item_owner_identity zero = flatfile_shopkeeper_item_owner(0);
	const item_owner_identity high = flatfile_shopkeeper_item_owner(UINT32_MAX);
	assert(zero.type == item_owner_type::shopkeeper && zero.id == 1 && zero.context_id == 0);
	assert(high.id == UINT64_C(4294967296));
	assert(item_owner_identity_valid(zero));

	flatfile_shopkeeper_record record = {};
	record.shop_id = 0;
	record.revision = 7;
	player_item_snapshot root = {};
	root.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	root.object_uid = 100;
	root.vnum = 10;
	player_item_snapshot child = {};
	child.parent_index = 0;
	child.object_uid = 101;
	child.vnum = 11;
	record.items = { root, child };
	std::vector<flatfile_item_ownership_record> custody = {
		{ 100, 100, 0, zero, 3, 10, item_custody_state::active },
		{ 101, 100, 100, zero, 4, 11, item_custody_state::active },
	};
	std::vector<player_load_item_identity> identities;
	assert(flatfile_shopkeeper_reconcile_item_ownership(record, 9, custody, &identities) ==
	       flatfile_shopkeeper_ownership_result::ok);
	assert(identities.size() == 2);
	assert(identities[0].database_id == 1 && identities[0].serialized_parent_id == 0);
	assert(identities[1].database_id == 2 && identities[1].serialized_parent_id == 1);
	assert(identities[1].item_uid == 101 && identities[1].root_item_uid == 100 &&
	       identities[1].parent_item_uid == 100 && identities[1].item_revision == 4 &&
	       identities[1].owner_revision == 9 &&
	       identities[1].override_mask == PLAYER_LOAD_ITEM_OVERRIDE_ALL);

	const auto original = identities;
	custody[1].parent_item_uid = 0;
	assert(flatfile_shopkeeper_reconcile_item_ownership(record, 9, custody, &identities) ==
	       flatfile_shopkeeper_ownership_result::invalid);
	assert(identities.size() == original.size() && identities[1].parent_item_uid == 100);
	custody[1].parent_item_uid = 100;
	custody[1].owner = { item_owner_type::player, 1, 0 };
	assert(flatfile_shopkeeper_reconcile_item_ownership(record, 9, custody, &identities) ==
	       flatfile_shopkeeper_ownership_result::invalid);
	return 0;
}
'''


with tempfile.TemporaryDirectory(prefix="duris-shopkeeper-ownership-") as temp_dir:
    source = Path(temp_dir) / "shopkeeper_ownership_test.cpp"
    binary = Path(temp_dir) / "shopkeeper_ownership_test"
    source.write_text(HARNESS)
    subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Wpedantic",
            "-Werror",
            "-Isrc",
            str(source),
            rel("flatfile_shopkeeper_ownership.c"),
            rel("item_transfer_command.c"),
            rel("critical_command.c"),
            "-lcrypto",
            "-o",
            str(binary),
        ],
        cwd=ROOT,
        check=True,
        capture_output=True,
        text=True,
    )
    subprocess.run([str(binary)], check=True)

print("flat-file shopkeeper ownership reconciliation passed")
