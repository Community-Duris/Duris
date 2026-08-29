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
	const item_transfer_result extended = { 100, 1, 11, 1, 6, 7 };
	std::array<uint8_t, ITEM_TRANSFER_RESULT_BYTES> encoded = {};
	assert(item_transfer_command_encode_result(extended, &encoded));
	item_transfer_result decoded = {};
	assert(item_transfer_command_decode_result(encoded.data(), encoded.size(), &decoded));
	assert(decoded.corpse_revision == 7);
	assert(item_transfer_command_decode_result(encoded.data(),
					   ITEM_TRANSFER_LEGACY_RESULT_BYTES, &decoded));
	assert(decoded.corpse_revision == 0 && decoded.max_item_revision == 6);
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
	const item_transfer_result committed = { 100, 1, 11, 1, 6, 0 };
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
	item_transfer_payload creation = {};
	creation.from_owner = { item_owner_type::system, 0, 0 };
	creation.to_owner = player;
	creation.selected_item_uid = 200;
	creation.target_root_item_uid = 101;
	creation.target_parent_item_uid = 101;
	creation.expected_target_parent_revision = 3;
	creation.item_count = 1;
	creation.items[0] = { 200, 200, 0, ITEM_TRANSFER_ABSENT_REVISION, 9,
			      item_custody_state::absent };
	assert(item_ownership_runtime_apply(creation, { 200, 1, 1, 12, 1, 0 }));
	item_ownership_runtime_entry created = {};
	assert(item_ownership_runtime_lookup(200, &created));
	assert(created.root_item_uid == 101 && created.parent_item_uid == 101 &&
	       created.owner_revision == 12);
	creation.selected_item_uid = 201;
	creation.items[0].item_uid = 201;
	creation.expected_target_parent_revision = 2;
	assert(!item_ownership_runtime_apply(creation, { 201, 1, 2, 13, 1, 0 }));

	const item_ownership_runtime_entry stale = {
		101, 101, 0, player, 3, 10, 8, item_custody_state::active
	};
	assert(!item_ownership_runtime_hydrate_batch(&stale, 1));

	const item_owner_identity locker = { item_owner_type::locker, 77, 0 };
	const item_ownership_runtime_entry rejected_atomic[] = {
		{ 202, 202, 0, room, 1, 1, 9, item_custody_state::active },
		{ 101, 101, 0, player, 3, 10, 8, item_custody_state::active },
	};
	assert(!item_ownership_runtime_hydrate_many_atomic(rejected_atomic, 2));
	item_ownership_runtime_entry absent = {};
	assert(!item_ownership_runtime_lookup(202, &absent));

	const item_ownership_runtime_entry accepted_atomic[] = {
		{ 202, 202, 0, room, 1, 1, 9, item_custody_state::active },
		{ 203, 203, 0, locker, 2, 4, 10, item_custody_state::active },
	};
	assert(item_ownership_runtime_hydrate_many_atomic(accepted_atomic, 2));
	assert(item_ownership_runtime_lookup(202, &absent));
	assert(absent.owner.type == item_owner_type::room && absent.owner.id == 1200);
	assert(item_ownership_runtime_lookup(203, &absent));
	assert(absent.owner.type == item_owner_type::locker && absent.owner.id == 77);

	item_ownership_runtime_reset();
	const item_owner_identity corpse = {
		item_owner_type::corpse, item_corpse_owner_id(42, 20), 0
	};
	const item_owner_identity release_room = { item_owner_type::room, 500, 0 };
	const item_ownership_runtime_entry corpse_items[] = {
		{ 300, 300, 0, corpse, 4, 2, 20, item_custody_state::active },
		{ 301, 300, 300, corpse, 7, 2, 21, item_custody_state::active },
	};
	assert(item_ownership_runtime_hydrate_batch(corpse_items, 2));
	assert(item_ownership_runtime_hydrate_owner(release_room, 4));
	corpse_lifecycle_result released = {};
	released.owner_pid = 42;
	released.save_id = 20;
	released.action = corpse_lifecycle_action::release;
	released.catalog_revision = 10;
	released.corpse_owner_revision = 3;
	released.room_owner_revision = 5;
	released.max_item_revision = 8;
	released.item_count = 2;
	corpse_lifecycle_result mismatched_release = released;
	mismatched_release.max_item_revision = 9;
	assert(!item_ownership_runtime_apply_corpse_release(42, 20, 500,
							   mismatched_release));
	assert(item_ownership_runtime_lookup(300, &absent) && absent.item_revision == 4 &&
	       item_owner_identity_equal(absent.owner, corpse));
	assert(item_ownership_runtime_apply_corpse_release(42, 20, 500, released));
	assert(item_ownership_runtime_lookup(300, &absent));
	assert(item_owner_identity_equal(absent.owner, release_room) &&
	       absent.item_revision == 5 && absent.owner_revision == 5);
	assert(item_ownership_runtime_lookup(301, &absent));
	assert(item_owner_identity_equal(absent.owner, release_room) &&
	       absent.item_revision == 8 && absent.parent_item_uid == 300);
	uint64_t owner_revision = 0;
	assert(item_ownership_runtime_owner_revision(corpse, &owner_revision) &&
	       owner_revision == 3);
	assert(item_ownership_runtime_owner_revision(release_room, &owner_revision) &&
	       owner_revision == 5);
	assert(!item_ownership_runtime_apply_corpse_release(42, 20, 500, released));

	item_ownership_runtime_reset();
	released.owner_pid = 43;
	released.save_id = 21;
	released.corpse_owner_revision = 1;
	released.room_owner_revision = 1;
	released.max_item_revision = 0;
	released.item_count = 0;
	assert(item_ownership_runtime_apply_corpse_release(43, 21, 501, released));
	const item_owner_identity empty_room = { item_owner_type::room, 501, 0 };
	assert(item_ownership_runtime_owner_revision(empty_room, &owner_revision) &&
	       owner_revision == 1);

	item_ownership_runtime_reset();
	const item_owner_identity destroyed_corpse = {
		item_owner_type::corpse, item_corpse_owner_id(50, 30), 0
	};
	const item_ownership_runtime_entry destroyed_corpse_items[] = {
		{ 400, 400, 0, destroyed_corpse, 2, 4, 30, item_custody_state::active },
		{ 401, 400, 400, destroyed_corpse, 6, 4, 31, item_custody_state::active },
	};
	assert(item_ownership_runtime_hydrate_batch(destroyed_corpse_items, 2));
	corpse_lifecycle_result destroyed = {};
	destroyed.owner_pid = 50;
	destroyed.save_id = 30;
	destroyed.action = corpse_lifecycle_action::destroy;
	destroyed.catalog_revision = 11;
	destroyed.corpse_owner_revision = 5;
	destroyed.room_owner_revision = 1;
	destroyed.max_item_revision = 7;
	destroyed.item_count = 2;
	assert(item_ownership_runtime_apply_corpse_destruction(50, 30, destroyed));
	assert(item_ownership_runtime_lookup(400, &absent) &&
	       absent.owner.type == item_owner_type::destruction &&
	       absent.state == item_custody_state::destroyed && absent.item_revision == 3 &&
	       absent.owner_revision == 1);
	assert(item_ownership_runtime_lookup(401, &absent) &&
	       absent.owner.type == item_owner_type::destruction &&
	       absent.state == item_custody_state::destroyed && absent.item_revision == 7);
	const item_owner_identity destruction = { item_owner_type::destruction, 0, 0 };
	assert(item_ownership_runtime_owner_revision(destruction, &owner_revision) &&
	       owner_revision == 1);
	assert(!item_ownership_runtime_apply_corpse_destruction(50, 30, destroyed));
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
