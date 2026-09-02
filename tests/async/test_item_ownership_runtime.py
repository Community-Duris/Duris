#!/usr/bin/env python3
"""Runtime regressions for the live item ownership cache."""

from _paths import rel
import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]

HARNESS = r'''
#include "item/item_ownership_runtime.h"

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

	const item_ownership_runtime_entry second_inventory = {
		204, 204, 0, player, 4, 12, 11, item_custody_state::active
	};
	assert(item_ownership_runtime_hydrate_batch(&second_inventory, 1));
	item_transfer_payload batch = {};
	batch.from_owner = player;
	batch.to_owner = room;
	batch.reason = item_transfer_reason::player_drop;
	batch.expected_from_revision = 12;
	batch.expected_to_revision = 1;
	batch.multi_root = true;
	batch.item_count = 2;
	batch.items[0] = { 101, 101, 0, 3, 8, item_custody_state::active };
	batch.items[1] = { 204, 204, 0, 4, 11, item_custody_state::active };
	assert(item_ownership_runtime_apply(batch, { 101, 2, 13, 2, 5, 0 }));
	assert(item_ownership_runtime_lookup(101, &absent) &&
	       item_owner_identity_equal(absent.owner, room) && absent.root_item_uid == 101 &&
	       absent.parent_item_uid == 0 && absent.item_revision == 4);
	assert(item_ownership_runtime_lookup(204, &absent) &&
	       item_owner_identity_equal(absent.owner, room) && absent.root_item_uid == 204 &&
	       absent.parent_item_uid == 0 && absent.item_revision == 5);

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

	item_ownership_runtime_reset();
	const item_owner_identity resurrected_corpse = {
		item_owner_type::corpse, item_corpse_owner_id(60, 40), 0
	};
	const item_owner_identity resurrected_player = { item_owner_type::player, 70, 0 };
	const item_owner_identity old_room = { item_owner_type::room, 600, 0 };
	const item_ownership_runtime_entry resurrected_items[] = {
		{ 500, 500, 0, resurrected_corpse, 3, 4, 40, item_custody_state::active },
		{ 501, 500, 500, resurrected_corpse, 5, 4, 41, item_custody_state::active },
	};
	assert(item_ownership_runtime_hydrate_batch(resurrected_items, 2));
	assert(item_ownership_runtime_hydrate_owner(resurrected_player, 6));
	assert(item_ownership_runtime_hydrate_owner(old_room, 8));
	corpse_lifecycle_result resurrected = {};
	resurrected.owner_pid = 60;
	resurrected.save_id = 40;
	resurrected.action = corpse_lifecycle_action::resurrect;
	resurrected.catalog_revision = 12;
	resurrected.corpse_owner_revision = 5;
	resurrected.room_owner_revision = 9;
	resurrected.player_owner_revision = 7;
	resurrected.wallet_revision = 2;
	resurrected.max_item_revision = 6;
	resurrected.item_count = 2;
	resurrected.wallet = { 1, 2, 3, 4 };
	assert(item_ownership_runtime_apply_corpse_resurrection(60, 40, 70, 600,
							       resurrected));
	assert(item_ownership_runtime_lookup(500, &absent) &&
	       item_owner_identity_equal(absent.owner, resurrected_player) &&
	       absent.item_revision == 4 && absent.owner_revision == 7);
	assert(item_ownership_runtime_lookup(501, &absent) &&
	       item_owner_identity_equal(absent.owner, resurrected_player) &&
	       absent.item_revision == 6 && absent.parent_item_uid == 500);
	assert(item_ownership_runtime_owner_revision(resurrected_corpse, &owner_revision) &&
	       owner_revision == 5);
	assert(item_ownership_runtime_owner_revision(resurrected_player, &owner_revision) &&
	       owner_revision == 7);
	assert(item_ownership_runtime_owner_revision(old_room, &owner_revision) &&
	       owner_revision == 9);
	assert(!item_ownership_runtime_apply_corpse_resurrection(60, 40, 70, 600,
								resurrected));

	item_ownership_runtime_reset();
	const item_owner_identity raised_corpse = {
		item_owner_type::corpse, item_corpse_owner_id(61, 41), 0
	};
	const item_owner_identity raising_player = { item_owner_type::player, 71, 0 };
	const item_ownership_runtime_entry raised_items[] = {
		{ 510, 510, 0, raised_corpse, 2, 3, 42, item_custody_state::active },
		{ 511, 510, 510, raised_corpse, 4, 3, 43, item_custody_state::active },
	};
	assert(item_ownership_runtime_hydrate_batch(raised_items, 2));
	assert(item_ownership_runtime_hydrate_owner(raising_player, 5));
	corpse_lifecycle_result raised = {};
	raised.owner_pid = 61;
	raised.save_id = 41;
	raised.action = corpse_lifecycle_action::raise_follower;
	raised.catalog_revision = 13;
	raised.corpse_owner_revision = 4;
	raised.player_owner_revision = 6;
	raised.wallet_revision = 3;
	raised.max_item_revision = 5;
	raised.item_count = 2;
	raised.wallet = { 5, 6, 7, 8 };
	assert(item_ownership_runtime_apply_corpse_raise(61, 41, 71, raised));
	assert(item_ownership_runtime_lookup(510, &absent) &&
	       item_owner_identity_equal(absent.owner, raising_player) &&
	       absent.item_revision == 3 && absent.owner_revision == 6);
	assert(item_ownership_runtime_lookup(511, &absent) &&
	       item_owner_identity_equal(absent.owner, raising_player) &&
	       absent.item_revision == 5 && absent.parent_item_uid == 510);
	assert(item_ownership_runtime_owner_revision(raised_corpse, &owner_revision) &&
	       owner_revision == 4);
	assert(item_ownership_runtime_owner_revision(raising_player, &owner_revision) &&
	       owner_revision == 6);
	assert(!item_ownership_runtime_apply_corpse_raise(61, 41, 71, raised));

	item_ownership_runtime_reset();
	const item_owner_identity nested_corpse = {
		item_owner_type::corpse, item_corpse_owner_id(62, 42), 0
	};
	const item_owner_identity nested_room = { item_owner_type::room, 700, 0 };
	const item_ownership_runtime_entry nested_items[] = {
		{ 600, 600, 0, nested_room, 9, 5, 50, item_custody_state::active },
		{ 610, 610, 0, nested_corpse, 2, 3, 51, item_custody_state::active },
		{ 611, 610, 610, nested_corpse, 4, 3, 52, item_custody_state::active },
	};
	assert(item_ownership_runtime_hydrate_many_atomic(nested_items, 3));
	corpse_lifecycle_result nested = {};
	nested.owner_pid = 62;
	nested.save_id = 42;
	nested.action = corpse_lifecycle_action::release_nested;
	nested.catalog_revision = 14;
	nested.corpse_owner_revision = 4;
	nested.room_owner_revision = 6;
	nested.max_item_revision = 5;
	nested.item_count = 2;
	assert(!item_ownership_runtime_apply_corpse_nested_release(
		62, 42, nested_room, 600, 600, 8, nested));
	assert(item_ownership_runtime_apply_corpse_nested_release(
		62, 42, nested_room, 600, 600, 9, nested));
	assert(item_ownership_runtime_lookup(610, &absent) &&
	       item_owner_identity_equal(absent.owner, nested_room) &&
	       absent.root_item_uid == 600 && absent.parent_item_uid == 600 &&
	       absent.item_revision == 3 && absent.owner_revision == 6);
	assert(item_ownership_runtime_lookup(611, &absent) &&
	       absent.root_item_uid == 600 && absent.parent_item_uid == 610 &&
	       absent.item_revision == 5 && absent.owner_revision == 6);
	assert(item_ownership_runtime_owner_revision(nested_corpse, &owner_revision) &&
	       owner_revision == 4);
	assert(item_ownership_runtime_owner_revision(nested_room, &owner_revision) &&
	       owner_revision == 6);

	item_ownership_runtime_reset();
	const item_owner_identity deleted_player = { item_owner_type::player, 80, 0 };
	const item_owner_identity deleted_corpse = {
		item_owner_type::corpse, item_corpse_owner_id(80, 50), 0
	};
	const item_owner_identity retained_player = { item_owner_type::player, 81, 0 };
	const item_ownership_runtime_entry deletion_domain[] = {
		{ 700, 700, 0, deleted_player, 1, 2, 60, item_custody_state::active },
		{ 701, 701, 0, deleted_corpse, 1, 3, 61, item_custody_state::active },
		{ 702, 702, 0, retained_player, 1, 4, 62, item_custody_state::active },
	};
	assert(item_ownership_runtime_hydrate_many_atomic(deletion_domain, 3));
	item_ownership_runtime_forget_player_domain(80);
	assert(!item_ownership_runtime_lookup(700, &absent));
	assert(!item_ownership_runtime_lookup(701, &absent));
	/* Missing valid owners are rehydrated at revision zero by the lookup API. */
	assert(item_ownership_runtime_owner_revision(deleted_player, &owner_revision) &&
	       owner_revision == 0);
	assert(item_ownership_runtime_owner_revision(deleted_corpse, &owner_revision) &&
	       owner_revision == 0);
	assert(item_ownership_runtime_lookup(702, &absent) &&
	       item_owner_identity_equal(absent.owner, retained_player));
	assert(item_ownership_runtime_owner_revision(retained_player, &owner_revision) &&
	       owner_revision == 4);
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
			rel("item_ownership_runtime.c"),
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

print("[PASS] authoritative reload and multi-owner hydration are transactional")
