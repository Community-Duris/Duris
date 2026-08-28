#!/usr/bin/env python3
"""Runtime regressions for the live item ownership cache."""

import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]

HARNESS = r'''
#include "item_ownership_runtime.h"

#include <cassert>

int main()
{
	item_ownership_runtime_reset();
	const item_owner_identity player = { item_owner_type::player, 42, 0 };
	const item_owner_identity room = { item_owner_type::room, 1200, 0 };
	const item_ownership_runtime_entry inventory[] = {
		{ 100, 100, 0, player, 5, 10, 7, item_custody_state::active },
		{ 101, 101, 0, player, 3, 10, 8, item_custody_state::active },
	};
	assert(item_ownership_runtime_hydrate_batch(inventory, 2));

	item_transfer_payload move = {};
	move.from_owner = player;
	move.to_owner = room;
	move.selected_item_uid = 100;
	move.target_root_item_uid = 100;
	move.item_count = 1;
	move.items[0] = { 100, 100, 0, 5, 7, item_custody_state::active };
	const item_transfer_result committed = { 100, 1, 11, 1, 6 };
	assert(item_ownership_runtime_apply(move, committed));

	item_ownership_runtime_entry untouched = {};
	assert(item_ownership_runtime_lookup(101, &untouched));
	assert(untouched.item_revision == 3 && untouched.owner_revision == 10);

	const item_ownership_runtime_entry authoritative = {
		101, 101, 0, player, 3, 11, 8, item_custody_state::active
	};
	assert(item_ownership_runtime_hydrate_batch(&authoritative, 1));
	assert(item_ownership_runtime_lookup(101, &untouched));
	assert(untouched.owner_revision == 11);

	const item_ownership_runtime_entry stale = {
		101, 101, 0, player, 3, 10, 8, item_custody_state::active
	};
	assert(!item_ownership_runtime_hydrate_batch(&stale, 1));

	const item_owner_identity locker = { item_owner_type::locker, 77, 0 };
	const item_ownership_runtime_entry rejected_atomic[] = {
		{ 200, 200, 0, room, 1, 1, 9, item_custody_state::active },
		{ 101, 101, 0, player, 3, 10, 8, item_custody_state::active },
	};
	assert(!item_ownership_runtime_hydrate_many_atomic(rejected_atomic, 2));
	item_ownership_runtime_entry absent = {};
	assert(!item_ownership_runtime_lookup(200, &absent));

	const item_ownership_runtime_entry accepted_atomic[] = {
		{ 200, 200, 0, room, 1, 1, 9, item_custody_state::active },
		{ 201, 201, 0, locker, 2, 4, 10, item_custody_state::active },
	};
	assert(item_ownership_runtime_hydrate_many_atomic(accepted_atomic, 2));
	assert(item_ownership_runtime_lookup(200, &absent));
	assert(absent.owner.type == item_owner_type::room && absent.owner.id == 1200);
	assert(item_ownership_runtime_lookup(201, &absent));
	assert(absent.owner.type == item_owner_type::locker && absent.owner.id == 77);
	return 0;
}
'''


with tempfile.TemporaryDirectory(prefix="duris-item-ownership-runtime-") as temp_dir:
	source = Path(temp_dir) / "item_ownership_runtime_test.cpp"
	binary = Path(temp_dir) / "item_ownership_runtime_test"
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
			"src/item_ownership_runtime.c",
			"src/item_transfer_command.c",
			"src/critical_command.c",
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

print("[PASS] authoritative reload and multi-owner hydration are transactional")
