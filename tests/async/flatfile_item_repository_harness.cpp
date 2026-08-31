#include "core/defines.h"
#include "flatfile/flatfile_artifact_repository.h"
#include "flatfile/flatfile_item_repository.h"
#include "flatfile/flatfile_locker_repository.h"
#include "flatfile/flatfile_shop_trade_materialization.h"
#include "player/player_snapshot_codec.h"

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <sys/wait.h>
#include <unistd.h>

namespace fs = std::filesystem;

static void require(bool condition, const std::string &message)
{
	if (!condition)
	{
		std::cerr << message << '\n';
		exit(1);
	}
}

static critical_operation_id operation(uint8_t discriminator)
{
	critical_operation_id id = {};
	id.bytes[0] = 0xa5;
	id.bytes.back() = discriminator;
	return id;
}

static critical_command creation(uint8_t discriminator, int64_t reason_id = 7)
{
	item_transfer_payload payload = {};
	payload.from_owner = { item_owner_type::system, 0, 0 };
	payload.to_owner = { item_owner_type::player, 42, 0 };
	payload.reason = item_transfer_reason::creation;
	payload.reason_id = reason_id;
	payload.expected_from_revision = 0;
	payload.expected_to_revision = 0;
	payload.selected_item_uid = 100;
	payload.target_root_item_uid = 100;
	payload.item_count = 2;
	payload.items[0] = { 100, 100,
			     0,	  ITEM_TRANSFER_ABSENT_REVISION,
			     500, item_custody_state::absent };
	payload.items[1] = { 101, 100,
			     100, ITEM_TRANSFER_ABSENT_REVISION,
			     501, item_custody_state::absent };
	critical_command command = {};
	require(item_transfer_command_build(&command, operation(discriminator), payload,
					    critical_source_site::command,
					    critical_deadline_class::interactive),
		"could not build creation command");
	command.accepted_at_usec = 1;
	return command;
}

static critical_command single_creation(uint8_t discriminator, uint64_t item_uid,
					uint64_t player_pid, uint64_t system_revision)
{
	item_transfer_payload payload = {};
	payload.from_owner = { item_owner_type::system, 0, 0 };
	payload.to_owner = { item_owner_type::player, player_pid, 0 };
	payload.reason = item_transfer_reason::creation;
	payload.reason_id = 11;
	payload.expected_from_revision = system_revision;
	payload.expected_to_revision = 0;
	payload.selected_item_uid = item_uid;
	payload.target_root_item_uid = item_uid;
	payload.item_count = 1;
	payload.items[0] = { item_uid, item_uid,
			     0,	       ITEM_TRANSFER_ABSENT_REVISION,
			     600,      item_custody_state::absent };
	critical_command command = {};
	require(item_transfer_command_build(&command, operation(discriminator), payload,
					    critical_source_site::command,
					    critical_deadline_class::interactive),
		"could not build single-item creation command");
	command.accepted_at_usec = 4;
	return command;
}

static item_transfer_result result_of(const critical_apply_result &applied)
{
	item_transfer_result result = {};
	require(item_transfer_command_decode_result(applied.result_payload.data(),
						    applied.result_size, &result),
		"could not decode item repository result");
	return result;
}

static std::vector<player_item_snapshot> movement_items()
{
	player_item_snapshot root = {};
	root.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	root.equipment_slot = -1;
	root.object_uid = 100;
	root.generated_key = 1100;
	root.vnum = 500;
	root.name = "given container";
	root.short_description = "an exact given container";
	player_item_snapshot child = {};
	child.parent_index = 0;
	child.equipment_slot = -1;
	child.object_uid = 101;
	child.generated_key = 1101;
	child.vnum = 501;
	child.name = "nested gift";
	child.short_description = "an exact nested gift";
	return { root, child };
}

static flatfile_locker_record transfer_locker()
{
	player_item_snapshot stored_root = {};
	stored_root.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	stored_root.equipment_slot = -1;
	stored_root.object_uid = 900;
	stored_root.vnum = 1900;
	stored_root.name = "stored locker container";
	player_item_snapshot stored_child = {};
	stored_child.parent_index = 0;
	stored_child.equipment_slot = -1;
	stored_child.object_uid = 901;
	stored_child.vnum = 1901;
	stored_child.name = "stored locker child";
	flatfile_locker_chest_record chest = {};
	chest.chest_id = 11;
	chest.chest_name = "public";
	chest.is_public = true;
	chest.revision = 1;
	chest.items = { stored_root, stored_child };
	flatfile_locker_record locker = {};
	locker.locker_id = 2;
	locker.locker_name = "transfer.locker";
	locker.owner_pid = 77;
	locker.revision = 1;
	locker.chests = { chest };
	return locker;
}

static std::vector<player_item_snapshot> corpse_items()
{
	player_item_snapshot loot_root = {};
	loot_root.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	loot_root.equipment_slot = -1;
	loot_root.object_uid = 910;
	loot_root.vnum = 1910;
	loot_root.name = "corpse loot container";
	loot_root.short_description = "an exact corpse loot container";
	player_item_snapshot loot_child = {};
	loot_child.parent_index = 0;
	loot_child.equipment_slot = -1;
	loot_child.object_uid = 911;
	loot_child.vnum = 1911;
	loot_child.name = "corpse loot child";
	player_item_snapshot retained_root = {};
	retained_root.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	retained_root.equipment_slot = -1;
	retained_root.object_uid = 920;
	retained_root.vnum = 1920;
	retained_root.name = "retained corpse container";
	player_item_snapshot retained_child = {};
	retained_child.parent_index = 2;
	retained_child.equipment_slot = -1;
	retained_child.object_uid = 921;
	retained_child.vnum = 1921;
	retained_child.name = "retained corpse child";
	player_item_snapshot artifact = {};
	artifact.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	artifact.equipment_slot = -1;
	artifact.object_uid = 930;
	artifact.vnum = 1999;
	artifact.name = "fenced corpse artifact";
	return { loot_root, loot_child, retained_root, retained_child, artifact };
}

static flatfile_corpse_record transfer_corpse()
{
	flatfile_corpse_record corpse = {};
	corpse.owner_pid = 42;
	corpse.owner_name = "corpseowner";
	corpse.save_id = 20;
	corpse.room_vnum = 500;
	corpse.short_description = "the exact transfer corpse";
	corpse.description = "The exact transfer corpse is lying here.";
	corpse.keywords = "corpse corpseowner _pcorpse_";
	corpse.weight = 90;
	corpse.values[3] = 42;
	corpse.values[5] = 1;
	corpse.values[6] = 20;
	corpse.values[7] = 4;
	corpse.revision = 4;
	corpse.items = corpse_items();
	return corpse;
}

static item_corpse_metadata corpse_metadata(const flatfile_corpse_record &corpse,
					    uint8_t actor_racewar, int32_t post_weight)
{
	item_corpse_metadata metadata = {};
	metadata.present = true;
	metadata.room_vnum = corpse.room_vnum;
	metadata.weight = post_weight;
	metadata.actor_racewar = actor_racewar;
	metadata.values = corpse.values;
	metadata.owner_name = corpse.owner_name;
	metadata.short_description = corpse.short_description;
	metadata.description = corpse.description;
	metadata.keywords = corpse.keywords;
	return metadata;
}

static shop_trade_payload shop_trade(shop_trade_action action, uint64_t item_uid,
				     uint64_t item_revision, int32_t vnum, uint64_t stock_uid = 0,
				     uint64_t stock_revision = 0)
{
	shop_trade_payload payload = {};
	payload.action = action;
	payload.player_pid = 42;
	payload.shop_id = 0;
	payload.racewar = 1;
	strcpy(payload.account_name.data(), "ShopTester");
	payload.price = 100;
	payload.expected_wallet_revision = 1;
	payload.expected_bank_revision = 1;
	payload.expected_shop_revision = 1;
	payload.selected_item_uid = item_uid;
	payload.target_root_item_uid = item_uid;
	payload.stock_item_uid = stock_uid;
	payload.expected_stock_item_revision = stock_revision;
	payload.stock_vnum = stock_uid ? vnum : 0;
	payload.item_count = 1;
	payload.items[0] = { item_uid,
			     item_uid,
			     0,
			     item_revision,
			     vnum,
			     action == shop_trade_action::buy_produced ?
				     item_custody_state::absent :
				     item_custody_state::active };
	payload.item_blob_size = 1;
	payload.item_blob[0] = 0xa5;
	return payload;
}

int main(int argc, char **argv)
{
	require(argc == 2, "state root argument required");
	const fs::path root = argv[1];
	const fs::path domains = root / "domains";
	fs::create_directories(domains);
	fs::permissions(root, fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(domains, fs::perms::owner_all, fs::perm_options::replace);
	const item_owner_identity baseline_owner = { item_owner_type::player, 999, 0 };
	std::vector<flatfile_item_ownership_record> baseline = {
		{ 300, 300, 0, baseline_owner, 1, 700, item_custody_state::active },
		{ 301, 300, 300, baseline_owner, 1, 701, item_custody_state::active },
	};
	std::string error;
	require(flatfile_item_repository_establish_owner(root.string(), baseline_owner, baseline,
							 &error) ==
			flatfile_item_baseline_result::applied,
		"owner baseline did not apply: " + error);
	require(flatfile_item_repository_establish_owner(root.string(), baseline_owner, baseline,
							 &error) ==
			flatfile_item_baseline_result::already_applied,
		"owner baseline retry was not idempotent");
	std::vector<flatfile_item_ownership_record> active_player_items;
	require(flatfile_item_repository_list_active_player_items(root.string(),
								  &active_player_items, &error) ==
				flatfile_item_repository_result::ok &&
			active_player_items.size() == 2 && active_player_items[0].item_uid == 300 &&
			active_player_items[1].item_uid == 301 &&
			active_player_items[0].owner.type == item_owner_type::player &&
			active_player_items[0].owner.id == 999,
		"active player item enumeration did not expose the authoritative baseline");
	baseline[1].vnum = 702;
	require(flatfile_item_repository_establish_owner(root.string(), baseline_owner, baseline,
							 &error) ==
			flatfile_item_baseline_result::conflict,
		"conflicting owner baseline was accepted");
	{
		std::fstream legacy(domains / "item_ownership",
				    std::ios::in | std::ios::out | std::ios::binary);
		require(legacy.good(), "could not open item catalog for v1 compatibility fixture");
		const char version[] = { 1, 0, 0, 0 };
		legacy.seekp(8);
		legacy.write(version, sizeof(version));
		legacy.close();
		uint64_t legacy_revision = 0;
		std::vector<flatfile_item_ownership_record> legacy_items;
		require(flatfile_item_repository_load_owner(
				root.string(), baseline_owner, &legacy_revision, &legacy_items,
				&error) == flatfile_item_repository_result::ok &&
				legacy_revision == 1 && legacy_items.size() == 2,
			"v1 ownership catalog without operation results did not remain readable");
	}
	{
		flatfile_authority_lock lock;
		flatfile_authority_operation operation;
		require(lock.acquire(root.string(), &error) &&
				flatfile_item_repository_prepare_player_remove(
					root.string(), lock, 999, &operation, &error) ==
					flatfile_item_repository_result::ok &&
				operation.filename == "item_ownership" && !operation.bytes.empty(),
			"player item destruction was not prepared: " + error);
		require(flatfile_authority_transaction_commit_operations(root.string(), lock,
									 { operation }, &error) ==
				flatfile_authority_transaction_result::ok,
			"prepared player item destruction did not commit: " + error);
	}
	uint64_t removed_revision = 0;
	std::vector<flatfile_item_ownership_record> removed_items;
	require(flatfile_item_repository_load_owner(root.string(), baseline_owner,
						    &removed_revision, &removed_items, &error) ==
			flatfile_item_repository_result::not_found,
		"deleted player item owner remained authoritative");
	{
		flatfile_authority_lock lock;
		flatfile_authority_operation operation;
		require(lock.acquire(root.string(), &error) &&
				flatfile_item_repository_prepare_player_remove(
					root.string(), lock, 999, &operation, &error) ==
					flatfile_item_repository_result::unchanged,
			"prepared player item destruction was not idempotent");
	}

	const critical_command create = creation(1);
	critical_apply_result applied = flatfile_item_repository_apply(root.string(), create);
	require(applied.outcome == critical_apply_outcome::applied && applied.error_code == 0,
		"item creation did not apply: outcome=" +
			std::to_string(static_cast<unsigned int>(applied.outcome)) +
			" error=" + std::to_string(applied.error_code));
	item_transfer_result result = result_of(applied);
	require(result.root_item_uid == 100 && result.item_count == 2 &&
			result.from_owner_revision == 1 && result.to_owner_revision == 1 &&
			result.max_item_revision == 1,
		"item creation returned incorrect revisions");

	uint64_t owner_revision = 0;
	std::vector<flatfile_item_ownership_record> items;
	require(flatfile_item_repository_load_owner(
			root.string(), { item_owner_type::player, 42, 0 }, &owner_revision, &items,
			&error) == flatfile_item_repository_result::ok &&
			owner_revision == 1 && items.size() == 2 && items[0].item_uid == 100 &&
			items[0].parent_item_uid == 0 && items[1].parent_item_uid == 100 &&
			items[1].item_revision == 1,
		"created ownership topology did not round trip: " + error);

	applied = flatfile_item_repository_apply(root.string(), create);
	require(applied.outcome == critical_apply_outcome::already_applied &&
			result_of(applied).to_owner_revision == 1,
		"replayed creation was not idempotent");
	applied = flatfile_item_repository_apply(root.string(), creation(1, 8));
	require(applied.outcome == critical_apply_outcome::terminal_failure &&
			applied.error_code == EEXIST,
		"operation ID reuse with different content was accepted");

	item_transfer_payload move = {};
	move.from_owner = { item_owner_type::player, 42, 0 };
	move.to_owner = { item_owner_type::player, 77, 0 };
	move.reason = item_transfer_reason::player_give;
	move.reason_id = 9;
	move.expected_from_revision = 1;
	move.expected_to_revision = 0;
	move.selected_item_uid = 100;
	move.target_root_item_uid = 100;
	move.item_count = 2;
	for (size_t index = 0; index < items.size(); ++index)
		move.items[index] = { items[index].item_uid,
				      items[index].root_item_uid,
				      items[index].parent_item_uid,
				      items[index].item_revision,
				      items[index].vnum,
				      items[index].state };
	const std::vector<player_item_snapshot> exact_items = movement_items();
	std::vector<uint8_t> exact_blob;
	require(player_item_snapshot_list_encode(exact_items, &exact_blob) ==
				player_snapshot_codec_result::ok &&
			exact_blob.size() <= move.item_blob.size(),
		"could not encode exact transfer snapshot");
	move.item_blob_size = static_cast<uint32_t>(exact_blob.size());
	std::copy(exact_blob.begin(), exact_blob.end(), move.item_blob.begin());
	critical_command transfer = {};
	require(item_transfer_command_build(&transfer, operation(2), move,
					    critical_source_site::command,
					    critical_deadline_class::interactive),
		"could not build transfer command");
	transfer.accepted_at_usec = 2;
	setenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_IMAGE", "1", 1);
	applied = flatfile_item_repository_apply(root.string(), transfer);
	unsetenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_IMAGE");
	require(applied.outcome == critical_apply_outcome::retryable_failure &&
			applied.error_code == EIO,
		"interrupted transfer did not leave recoverable atomic intent");
	applied = flatfile_item_repository_apply(root.string(), transfer);
	result = result_of(applied);
	require(applied.outcome == critical_apply_outcome::already_applied &&
			result.from_owner_revision == 2 && result.to_owner_revision == 1 &&
			result.max_item_revision == 2,
		"cross-owner transfer did not recover atomically");
	items.clear();
	require(flatfile_item_repository_load_owner(
			root.string(), { item_owner_type::player, 77, 0 }, &owner_revision, &items,
			&error) == flatfile_item_repository_result::ok &&
			owner_revision == 1 && items.size() == 2 && items[0].item_revision == 2,
		"destination owner did not receive the complete topology");
	player_snapshot stale_source = {};
	stale_source.pid = 42;
	stale_source.items = exact_items;
	player_snapshot missing_destination = {};
	missing_destination.pid = 77;
	std::vector<flatfile_item_ownership_record> source_items;
	uint64_t source_revision = 0;
	const auto source_loaded = flatfile_item_repository_load_owner(
		root.string(), { item_owner_type::player, 42, 0 }, &source_revision, &source_items,
		&error);
	require((source_loaded == flatfile_item_repository_result::ok ||
		 source_loaded == flatfile_item_repository_result::not_found) &&
			source_items.empty(),
		"source owner unexpectedly retained transferred items");
	{
		flatfile_authority_lock reconciliation_lock;
		require(reconciliation_lock.acquire(root.string(), &error),
			"could not lock transfer reconciliation: " + error);
		require(flatfile_shop_trade_materialization_reconcile(
				root.string(), reconciliation_lock, 42, source_items, &stale_source,
				&error) == flatfile_shop_trade_materialization_result::ok &&
				stale_source.items.empty(),
			"restart reconciliation retained the stale source transfer: " + error);
		require(flatfile_shop_trade_materialization_reconcile(
				root.string(), reconciliation_lock, 77, items, &missing_destination,
				&error) == flatfile_shop_trade_materialization_result::ok &&
				missing_destination.items.size() == 2 &&
				missing_destination.items[0].object_uid == 100 &&
				missing_destination.items[0].short_description ==
					"an exact given container" &&
				missing_destination.items[1].parent_index == 0,
			"restart reconciliation did not reconstruct the exact destination transfer: " +
				error);
	}
	const fs::path room_root = root / "room-transfer";
	fs::create_directories(room_root / "domains");
	fs::permissions(room_root, fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(room_root / "domains", fs::perms::owner_all, fs::perm_options::replace);
	const item_owner_identity room_player = { item_owner_type::player, 77, 0 };
	const item_owner_identity room_owner = { item_owner_type::room, 9001, 0 };
	const std::vector<flatfile_item_ownership_record> room_player_items = {
		{ 100, 100, 0, room_player, 1, 500, item_custody_state::active },
		{ 101, 100, 100, room_player, 1, 501, item_custody_state::active },
	};
	require(flatfile_item_repository_establish_owner(room_root.string(), room_player,
							 room_player_items, &error) ==
			flatfile_item_baseline_result::applied,
		"could not establish isolated room transfer authority: " + error);
	std::vector<player_item_snapshot> room_container = movement_items();
	room_container[0].equipment_slot = 0;
	room_container[0].weight = 9;
	room_container[1].equipment_slot = 0;
	room_container[1].weight = 4;
	std::vector<uint8_t> room_container_blob;
	require(player_item_snapshot_list_encode(room_container, &room_container_blob) ==
			player_snapshot_codec_result::ok,
		"could not encode room container transfer");
	item_transfer_payload room_drop = {};
	room_drop.from_owner = room_player;
	room_drop.to_owner = room_owner;
	room_drop.reason = item_transfer_reason::player_drop;
	room_drop.reason_id = 9001;
	room_drop.expected_from_revision = 1;
	room_drop.expected_to_revision = 0;
	room_drop.selected_item_uid = 100;
	room_drop.target_root_item_uid = 100;
	room_drop.item_count = 2;
	room_drop.items[0] = { 100, 100, 0, 1, 500, item_custody_state::active };
	room_drop.items[1] = { 101, 100, 100, 1, 501, item_custody_state::active };
	room_drop.item_blob_size = static_cast<uint32_t>(room_container_blob.size());
	std::copy(room_container_blob.begin(), room_container_blob.end(),
		  room_drop.item_blob.begin());
	critical_command room_drop_command = {};
	require(item_transfer_command_build(&room_drop_command, operation(5), room_drop,
					    critical_source_site::command,
					    critical_deadline_class::interactive),
		"could not build room drop transfer");
	room_drop_command.accepted_at_usec = 5;
	setenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_IMAGE", "1", 1);
	applied = flatfile_item_repository_apply(room_root.string(), room_drop_command);
	unsetenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_IMAGE");
	require(applied.outcome == critical_apply_outcome::retryable_failure &&
			applied.error_code == EIO,
		"interrupted room drop did not retain composite intent");
	applied = flatfile_item_repository_apply(room_root.string(), room_drop_command);
	require(applied.outcome == critical_apply_outcome::already_applied &&
			result_of(applied).from_owner_revision == 2 &&
			result_of(applied).to_owner_revision == 1,
		"room drop did not recover atomically");
	std::vector<flatfile_room_item_record> room_records;
	require(flatfile_world_item_list_rooms(room_root.string(), &room_records, &error) ==
				flatfile_world_item_result::ok &&
			room_records.size() == 1 && room_records[0].revision == 1 &&
			room_records[0].items.size() == 2 &&
			room_records[0].items[0].equipment_slot == -1 &&
			room_records[0].items[1].parent_index == 0,
		"room drop did not publish the exact room aggregate: " + error);
	uint64_t room_revision = 0;
	std::vector<flatfile_item_ownership_record> owned_room_items;
	require(flatfile_item_repository_load_owner(room_root.string(), room_owner, &room_revision,
						    &owned_room_items, &error) ==
				flatfile_item_repository_result::ok &&
			room_revision == 1 && owned_room_items.size() == 2,
		"room drop did not publish matching ownership custody");

	const item_owner_identity insert_player = { item_owner_type::player, 88, 0 };
	require(flatfile_item_repository_establish_owner(
			room_root.string(), insert_player,
			{ { 200, 200, 0, insert_player, 1, 600, item_custody_state::active } },
			&error) == flatfile_item_baseline_result::applied,
		"could not establish room-container insertion item: " + error);
	player_item_snapshot inserted_item = {};
	inserted_item.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	inserted_item.equipment_slot = 0;
	inserted_item.object_uid = 200;
	inserted_item.generated_key = 1200;
	inserted_item.vnum = 600;
	inserted_item.name = "inserted item";
	inserted_item.short_description = "an inserted item";
	inserted_item.weight = 3;
	std::vector<uint8_t> inserted_blob;
	require(player_item_snapshot_list_encode({ inserted_item }, &inserted_blob) ==
			player_snapshot_codec_result::ok,
		"could not encode room-container insertion item");
	item_transfer_payload room_put = {};
	room_put.from_owner = insert_player;
	room_put.to_owner = room_owner;
	room_put.reason = item_transfer_reason::player_put;
	room_put.reason_id = 100;
	room_put.expected_from_revision = 1;
	room_put.expected_to_revision = 1;
	room_put.selected_item_uid = 200;
	room_put.target_root_item_uid = 100;
	room_put.target_parent_item_uid = 100;
	room_put.expected_target_parent_revision = 2;
	room_put.item_count = 1;
	room_put.items[0] = { 200, 200, 0, 1, 600, item_custody_state::active };
	room_put.item_blob_size = static_cast<uint32_t>(inserted_blob.size());
	std::copy(inserted_blob.begin(), inserted_blob.end(), room_put.item_blob.begin());
	critical_command room_put_command = {};
	require(item_transfer_command_build(&room_put_command, operation(6), room_put,
					    critical_source_site::command,
					    critical_deadline_class::interactive),
		"could not build room-container put transfer");
	room_put_command.accepted_at_usec = 6;
	applied = flatfile_item_repository_apply(room_root.string(), room_put_command);
	require(applied.outcome == critical_apply_outcome::applied,
		"room-container put did not apply");
	room_records.clear();
	require(flatfile_world_item_list_rooms(room_root.string(), &room_records, &error) ==
				flatfile_world_item_result::ok &&
			room_records[0].revision == 2 && room_records[0].items.size() == 3 &&
			room_records[0].items[0].weight == 12 &&
			room_records[0].items[2].parent_index == 0,
		"room-container put did not persist topology and propagated weight");

	std::vector<flatfile_item_ownership_record> inserted_ownership;
	uint64_t insert_revision = 0;
	require(flatfile_item_repository_load_owner(room_root.string(), room_owner, &room_revision,
						    &owned_room_items, &error) ==
				flatfile_item_repository_result::ok &&
			flatfile_item_repository_load_owner(room_root.string(), insert_player,
							    &insert_revision, &inserted_ownership,
							    &error) ==
				flatfile_item_repository_result::ok &&
			room_revision == 2 && insert_revision == 2 && inserted_ownership.empty(),
		"room-container put did not publish expected owner revisions");
	item_transfer_payload room_get = room_put;
	room_get.from_owner = room_owner;
	room_get.to_owner = insert_player;
	room_get.reason = item_transfer_reason::player_get;
	room_get.reason_id = 200;
	room_get.expected_from_revision = 2;
	room_get.expected_to_revision = 2;
	room_get.target_root_item_uid = 200;
	room_get.target_parent_item_uid = 0;
	room_get.expected_target_parent_revision = 0;
	room_get.items[0] = { 200, 100, 100, 2, 600, item_custody_state::active };
	critical_command room_get_command = {};
	require(item_transfer_command_build(&room_get_command, operation(7), room_get,
					    critical_source_site::command,
					    critical_deadline_class::interactive),
		"could not build room-container get transfer");
	room_get_command.accepted_at_usec = 7;
	applied = flatfile_item_repository_apply(room_root.string(), room_get_command);
	require(applied.outcome == critical_apply_outcome::applied,
		"room-container get did not apply");
	room_records.clear();
	require(flatfile_world_item_list_rooms(room_root.string(), &room_records, &error) ==
				flatfile_world_item_result::ok &&
			room_records[0].revision == 3 && room_records[0].items.size() == 2 &&
			room_records[0].items[0].weight == 9,
		"room-container get did not remove only the subtree and restore ancestor weight");

	const fs::path batch_root = root / "batch-transfer";
	fs::create_directories(batch_root / "domains");
	fs::permissions(batch_root, fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(batch_root / "domains", fs::perms::owner_all, fs::perm_options::replace);
	const item_owner_identity batch_player = { item_owner_type::player, 99, 0 };
	const item_owner_identity batch_room = { item_owner_type::room, 9100, 0 };
	require(flatfile_item_repository_establish_owner(
			batch_root.string(), batch_player,
			{ { 410, 410, 0, batch_player, 1, 810, item_custody_state::active },
			  { 420, 420, 0, batch_player, 1, 820, item_custody_state::active } },
			&error) == flatfile_item_baseline_result::applied,
		"could not establish multi-root batch authority: " + error);
	player_item_snapshot first_batch_item = {};
	first_batch_item.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	first_batch_item.equipment_slot = 0;
	first_batch_item.object_uid = 410;
	first_batch_item.vnum = 810;
	first_batch_item.name = "first batch item";
	first_batch_item.short_description = "a first batch item";
	player_item_snapshot second_batch_item = first_batch_item;
	second_batch_item.object_uid = 420;
	second_batch_item.vnum = 820;
	second_batch_item.name = "second batch item";
	second_batch_item.short_description = "a second batch item";
	std::vector<uint8_t> batch_blob;
	require(player_item_snapshot_list_encode({ first_batch_item, second_batch_item },
						 &batch_blob) == player_snapshot_codec_result::ok,
		"could not encode multi-root transfer snapshot");
	item_transfer_payload batch_drop = {};
	batch_drop.from_owner = batch_player;
	batch_drop.to_owner = batch_room;
	batch_drop.reason = item_transfer_reason::player_drop;
	batch_drop.reason_id = 9100;
	batch_drop.expected_from_revision = 1;
	batch_drop.expected_to_revision = 0;
	batch_drop.multi_root = true;
	batch_drop.item_count = 2;
	batch_drop.items[0] = { 410, 410, 0, 1, 810, item_custody_state::active };
	batch_drop.items[1] = { 420, 420, 0, 1, 820, item_custody_state::active };
	batch_drop.item_blob_size = static_cast<uint32_t>(batch_blob.size());
	std::copy(batch_blob.begin(), batch_blob.end(), batch_drop.item_blob.begin());
	critical_command batch_drop_command = {};
	require(item_transfer_command_build(&batch_drop_command, operation(50), batch_drop,
					    critical_source_site::command,
					    critical_deadline_class::interactive),
		"could not build multi-root drop command");
	batch_drop_command.accepted_at_usec = 50;
	applied = flatfile_item_repository_apply(batch_root.string(), batch_drop_command);
	require(applied.outcome == critical_apply_outcome::applied &&
			result_of(applied).item_count == 2 &&
			result_of(applied).root_item_uid == 410,
		"multi-root drop did not apply as one command: outcome=" +
			std::to_string(static_cast<unsigned int>(applied.outcome)) +
			" error=" + std::to_string(applied.error_code));
	owned_room_items.clear();
	require(flatfile_item_repository_load_owner(batch_root.string(), batch_room, &room_revision,
						    &owned_room_items, &error) ==
				flatfile_item_repository_result::ok &&
			room_revision == 1 && owned_room_items.size() == 2 &&
			owned_room_items[0].root_item_uid == 410 &&
			owned_room_items[1].root_item_uid == 420 &&
			owned_room_items[0].parent_item_uid == 0 &&
			owned_room_items[1].parent_item_uid == 0,
		"multi-root drop did not preserve independent destination roots: " + error);
	player_item_snapshot storage_root = {};
	storage_root.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	storage_root.equipment_slot = 0;
	storage_root.object_uid = 300;
	storage_root.generated_key = 1300;
	storage_root.vnum = 700;
	storage_root.type = ITEM_STORAGE;
	storage_root.name = "saved storage";
	storage_root.short_description = "a saved storage container";
	storage_root.weight = 2;
	player_item_snapshot storage_child = storage_root;
	storage_child.parent_index = 0;
	storage_child.object_uid = 301;
	storage_child.generated_key = 1301;
	storage_child.vnum = 701;
	storage_child.type = ITEM_CONTAINER;
	storage_child.name = "stored container";
	storage_child.short_description = "a stored container";
	storage_child.weight = 1;
	std::vector<uint8_t> storage_blob;
	require(player_item_snapshot_list_encode({ storage_root, storage_child }, &storage_blob) ==
			player_snapshot_codec_result::ok,
		"could not encode saved storage subtree");
	item_transfer_payload room_create = {};
	room_create.from_owner = { item_owner_type::system, 0, 0 };
	room_create.to_owner = room_owner;
	room_create.reason = item_transfer_reason::creation;
	room_create.reason_id = 9001;
	room_create.expected_from_revision = 0;
	room_create.expected_to_revision = 3;
	room_create.selected_item_uid = 300;
	room_create.target_root_item_uid = 300;
	room_create.item_count = 2;
	room_create.items[0] = { 300, 300,
				 0,   ITEM_TRANSFER_ABSENT_REVISION,
				 700, item_custody_state::absent };
	room_create.items[1] = { 301, 300,
				 300, ITEM_TRANSFER_ABSENT_REVISION,
				 701, item_custody_state::absent };
	room_create.item_blob_size = static_cast<uint32_t>(storage_blob.size());
	std::copy(storage_blob.begin(), storage_blob.end(), room_create.item_blob.begin());
	critical_command room_create_command = {};
	require(item_transfer_command_build(&room_create_command, operation(8), room_create,
					    critical_source_site::operator_repair,
					    critical_deadline_class::interactive),
		"could not build saved storage establishment");
	room_create_command.accepted_at_usec = 8;
	applied = flatfile_item_repository_apply(room_root.string(), room_create_command);
	require(applied.outcome == critical_apply_outcome::applied &&
			result_of(applied).to_owner_revision == 4,
		"saved storage establishment did not apply");
	room_records.clear();
	require(flatfile_world_item_list_rooms(room_root.string(), &room_records, &error) ==
				flatfile_world_item_result::ok &&
			room_records[0].revision == 4 && room_records[0].items.size() == 4 &&
			room_records[0].items[2].object_uid == 300 &&
			room_records[0].items[2].type == ITEM_STORAGE &&
			room_records[0].items[2].equipment_slot == -1 &&
			room_records[0].items[3].object_uid == 301 &&
			room_records[0].items[3].parent_index == 2,
		"saved storage establishment did not append the exact detached subtree");

	item_transfer_payload room_destroy = room_create;
	room_destroy.from_owner = room_owner;
	room_destroy.to_owner = { item_owner_type::destruction, 0, 0 };
	room_destroy.reason = item_transfer_reason::destruction;
	room_destroy.reason_id = 300;
	room_destroy.expected_from_revision = 4;
	room_destroy.expected_to_revision = 0;
	room_destroy.items[0] = { 300, 300, 0, 1, 700, item_custody_state::active };
	room_destroy.items[1] = { 301, 300, 300, 1, 701, item_custody_state::active };
	critical_command room_destroy_command = {};
	require(item_transfer_command_build(&room_destroy_command, operation(9), room_destroy,
					    critical_source_site::operator_repair,
					    critical_deadline_class::interactive),
		"could not build saved storage destruction");
	room_destroy_command.accepted_at_usec = 9;
	applied = flatfile_item_repository_apply(room_root.string(), room_destroy_command);
	require(applied.outcome == critical_apply_outcome::applied &&
			result_of(applied).from_owner_revision == 5,
		"saved storage destruction did not apply");
	room_records.clear();
	require(flatfile_world_item_list_rooms(room_root.string(), &room_records, &error) ==
				flatfile_world_item_result::ok &&
			room_records[0].revision == 5 && room_records[0].items.size() == 2,
		"saved storage destruction did not remove the exact room subtree");

	player_item_snapshot detached_child = room_container[1];
	detached_child.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	std::vector<uint8_t> detached_child_blob;
	require(player_item_snapshot_list_encode({ detached_child }, &detached_child_blob) ==
			player_snapshot_codec_result::ok,
		"could not encode saved storage child removal");
	item_transfer_payload room_reparent = {};
	room_reparent.from_owner = room_owner;
	room_reparent.to_owner = room_owner;
	room_reparent.reason = item_transfer_reason::operator_repair;
	room_reparent.reason_id = 100;
	room_reparent.expected_from_revision = 5;
	room_reparent.expected_to_revision = 5;
	room_reparent.selected_item_uid = 101;
	room_reparent.target_root_item_uid = 101;
	room_reparent.item_count = 1;
	room_reparent.items[0] = { 101, 100, 100, 2, 501, item_custody_state::active };
	room_reparent.item_blob_size = static_cast<uint32_t>(detached_child_blob.size());
	std::copy(detached_child_blob.begin(), detached_child_blob.end(),
		  room_reparent.item_blob.begin());
	critical_command room_reparent_command = {};
	require(item_transfer_command_build(&room_reparent_command, operation(10), room_reparent,
					    critical_source_site::operator_repair,
					    critical_deadline_class::interactive),
		"could not build saved storage child removal");
	room_reparent_command.accepted_at_usec = 10;
	applied = flatfile_item_repository_apply(room_root.string(), room_reparent_command);
	require(applied.outcome == critical_apply_outcome::applied &&
			result_of(applied).from_owner_revision == 6 &&
			result_of(applied).to_owner_revision == 6,
		"saved storage child removal did not apply");
	room_records.clear();
	require(flatfile_world_item_list_rooms(room_root.string(), &room_records, &error) ==
				flatfile_world_item_result::ok &&
			room_records[0].revision == 6 && room_records[0].items.size() == 2 &&
			room_records[0].items[0].weight == 5 &&
			room_records[0].items[1].object_uid == 101 &&
			room_records[0].items[1].parent_index == PLAYER_SNAPSHOT_NO_PARENT,
		"saved storage child removal did not detach the root or repair ancestor weight");

	const item_owner_identity batch_put_player = { item_owner_type::player, 90, 0 };
	require(flatfile_item_repository_establish_owner(
			room_root.string(), batch_put_player,
			{ { 210, 210, 0, batch_put_player, 1, 610, item_custody_state::active },
			  { 220, 220, 0, batch_put_player, 1, 620, item_custody_state::active } },
			&error) == flatfile_item_baseline_result::applied,
		"could not establish room-container batch items: " + error);
	player_item_snapshot first_inserted = inserted_item;
	first_inserted.object_uid = 210;
	first_inserted.vnum = 610;
	first_inserted.weight = 2;
	player_item_snapshot second_inserted = inserted_item;
	second_inserted.object_uid = 220;
	second_inserted.vnum = 620;
	second_inserted.weight = 5;
	std::vector<uint8_t> batch_inserted_blob;
	require(player_item_snapshot_list_encode({ first_inserted, second_inserted },
						 &batch_inserted_blob) ==
			player_snapshot_codec_result::ok,
		"could not encode room-container batch items");
	item_transfer_payload room_batch_put = {};
	room_batch_put.from_owner = batch_put_player;
	room_batch_put.to_owner = room_owner;
	room_batch_put.reason = item_transfer_reason::player_put;
	room_batch_put.reason_id = 100;
	room_batch_put.expected_from_revision = 1;
	room_batch_put.expected_to_revision = 6;
	room_batch_put.target_root_item_uid = 100;
	room_batch_put.target_parent_item_uid = 100;
	room_batch_put.expected_target_parent_revision = 2;
	room_batch_put.multi_root = true;
	room_batch_put.item_count = 2;
	room_batch_put.items[0] = { 210, 210, 0, 1, 610, item_custody_state::active };
	room_batch_put.items[1] = { 220, 220, 0, 1, 620, item_custody_state::active };
	room_batch_put.item_blob_size = static_cast<uint32_t>(batch_inserted_blob.size());
	std::copy(batch_inserted_blob.begin(), batch_inserted_blob.end(),
		  room_batch_put.item_blob.begin());
	critical_command room_batch_put_command = {};
	require(item_transfer_command_build(&room_batch_put_command, operation(51), room_batch_put,
					    critical_source_site::command,
					    critical_deadline_class::interactive),
		"could not build room-container batch put");
	room_batch_put_command.accepted_at_usec = 51;
	applied = flatfile_item_repository_apply(room_root.string(), room_batch_put_command);
	require(applied.outcome == critical_apply_outcome::applied,
		"room-container batch put did not apply");
	room_records.clear();
	require(flatfile_world_item_list_rooms(room_root.string(), &room_records, &error) ==
				flatfile_world_item_result::ok &&
			room_records[0].revision == 7 && room_records[0].items.size() == 4 &&
			room_records[0].items[0].weight == 12 &&
			room_records[0].items[2].parent_index == 0 &&
			room_records[0].items[3].parent_index == 0,
		"room-container batch put did not attach every root or propagate total weight");
	items.clear();
	require(flatfile_item_repository_load_owner(
			root.string(), { item_owner_type::player, 77, 0 }, &owner_revision, &items,
			&error) == flatfile_item_repository_result::ok &&
			owner_revision == 1 && items.size() == 2,
		"isolated room transfer changed the primary transfer fixture");
	const item_owner_identity locker_owner = { item_owner_type::locker, 2, 11 };
	require(flatfile_locker_establish(root.string(), { transfer_locker() }, {}, &error) ==
			flatfile_locker_result::ok,
		"could not establish transfer locker: " + error);
	require(flatfile_item_repository_establish_owner(
			root.string(), locker_owner,
			{ { 900, 900, 0, locker_owner, 1, 1900, item_custody_state::active },
			  { 901, 900, 900, locker_owner, 1, 1901, item_custody_state::active } },
			&error) == flatfile_item_baseline_result::applied,
		"could not establish existing locker custody: " + error);
	item_transfer_payload deposit = move;
	deposit.from_owner = { item_owner_type::player, 77, 0 };
	deposit.to_owner = locker_owner;
	deposit.reason = item_transfer_reason::locker_deposit;
	deposit.reason_id = 11;
	deposit.expected_from_revision = 1;
	deposit.expected_to_revision = 1;
	for (size_t index = 0; index < items.size(); ++index)
		deposit.items[index] = { items[index].item_uid,
					 items[index].root_item_uid,
					 items[index].parent_item_uid,
					 items[index].item_revision,
					 items[index].vnum,
					 items[index].state };
	critical_command deposit_command = {};
	require(item_transfer_command_build(&deposit_command, operation(6), deposit,
					    critical_source_site::command,
					    critical_deadline_class::interactive),
		"could not build locker deposit");
	deposit_command.accepted_at_usec = 6;
	setenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_IMAGE", "1", 1);
	applied = flatfile_item_repository_apply(root.string(), deposit_command);
	unsetenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_IMAGE");
	require(applied.outcome == critical_apply_outcome::retryable_failure &&
			applied.error_code == EIO,
		"interrupted locker deposit did not retain composite intent: outcome=" +
			std::to_string(static_cast<unsigned int>(applied.outcome)) +
			" error=" + std::to_string(applied.error_code));
	applied = flatfile_item_repository_apply(root.string(), deposit_command);
	require(applied.outcome == critical_apply_outcome::already_applied,
		"locker deposit did not recover exactly");
	std::vector<flatfile_locker_record> lockers;
	std::vector<flatfile_locker_access_record> locker_access;
	require(flatfile_locker_list(root.string(), &lockers, &locker_access, &error) ==
				flatfile_locker_result::ok &&
			lockers.size() == 1 && lockers[0].revision == 2 &&
			lockers[0].chests[0].revision == 2 &&
			lockers[0].chests[0].items.size() == 4 &&
			lockers[0].chests[0].items[2].short_description ==
				"an exact given container" &&
			lockers[0].chests[0].items[3].parent_index == 2,
		"recovered deposit did not publish exact locker aggregate: " + error);
	items.clear();
	require(flatfile_item_repository_load_owner(root.string(), locker_owner, &owner_revision,
						    &items, &error) ==
				flatfile_item_repository_result::ok &&
			owner_revision == 2 && items.size() == 4 && items[0].item_revision == 3 &&
			items[2].item_uid == 900 && items[2].item_revision == 1,
		"recovered deposit did not publish locker custody");
	item_transfer_payload withdraw = deposit;
	withdraw.from_owner = locker_owner;
	withdraw.to_owner = { item_owner_type::player, 77, 0 };
	withdraw.reason = item_transfer_reason::locker_withdraw;
	withdraw.expected_from_revision = 2;
	withdraw.expected_to_revision = 2;
	for (size_t index = 0; index < withdraw.item_count; ++index)
		withdraw.items[index] = { items[index].item_uid,
					  items[index].root_item_uid,
					  items[index].parent_item_uid,
					  items[index].item_revision,
					  items[index].vnum,
					  items[index].state };
	critical_command withdraw_command = {};
	require(item_transfer_command_build(&withdraw_command, operation(7), withdraw,
					    critical_source_site::command,
					    critical_deadline_class::interactive),
		"could not build locker withdrawal");
	withdraw_command.accepted_at_usec = 7;
	applied = flatfile_item_repository_apply(root.string(), withdraw_command);
	require(applied.outcome == critical_apply_outcome::applied,
		"locker withdrawal did not apply");
	lockers.clear();
	require(flatfile_locker_list(root.string(), &lockers, &locker_access, &error) ==
				flatfile_locker_result::ok &&
			lockers[0].revision == 3 && lockers[0].chests[0].revision == 3 &&
			lockers[0].chests[0].items.size() == 2 &&
			lockers[0].chests[0].items[0].object_uid == 900 &&
			lockers[0].chests[0].items[1].parent_index == 0,
		"locker withdrawal did not remove only the exact aggregate subtree");
	items.clear();
	require(flatfile_item_repository_load_owner(
			root.string(), { item_owner_type::player, 77, 0 }, &owner_revision, &items,
			&error) == flatfile_item_repository_result::ok &&
			owner_revision == 3 && items.size() == 2 && items[0].item_revision == 4,
		"locker withdrawal did not restore player custody");
	const item_owner_identity corpse_owner = { item_owner_type::corpse,
						   item_corpse_owner_id(42, 20), 0 };
	const flatfile_artifact_record corpse_artifact = {
		1999, true, FLATFILE_ARTIFACT_ON_CORPSE, 42, 5000, 1, 1000, -1, 0, 1
	};
	require(flatfile_artifact_establish(root.string(), { corpse_artifact }, &error) ==
			flatfile_artifact_result::ok,
		"could not establish corpse artifact authority: " + error);
	const flatfile_corpse_record initial_corpse = transfer_corpse();
	flatfile_corpse_record created_corpse = initial_corpse;
	created_corpse.owner_pid = 77;
	created_corpse.owner_name = "newowner";
	created_corpse.save_id = 30;
	created_corpse.room_vnum = 600;
	created_corpse.short_description = "the newly created corpse";
	created_corpse.description = "The newly created corpse is lying here.";
	created_corpse.keywords = "corpse newowner _pcorpse_";
	created_corpse.values[3] = 77;
	created_corpse.values[5] = 2;
	created_corpse.values[6] = 30;
	created_corpse.revision = 1;
	created_corpse.items.clear();
	require(flatfile_world_item_establish(root.string(), { initial_corpse, created_corpse }, {},
					      &error) == flatfile_world_item_result::ok,
		"could not establish transfer corpses: " + error);
	require(flatfile_item_repository_establish_owner(
			root.string(), corpse_owner,
			{ { 910, 910, 0, corpse_owner, 1, 1910, item_custody_state::active },
			  { 911, 910, 910, corpse_owner, 1, 1911, item_custody_state::active },
			  { 920, 920, 0, corpse_owner, 1, 1920, item_custody_state::active },
			  { 921, 920, 920, corpse_owner, 1, 1921, item_custody_state::active },
			  { 930, 930, 0, corpse_owner, 1, 1999, item_custody_state::active } },
			&error) == flatfile_item_baseline_result::applied,
		"could not establish corpse custody: " + error);
	const std::vector<player_item_snapshot> all_corpse_items = corpse_items();
	std::vector<player_item_snapshot> loot_items = { all_corpse_items[0], all_corpse_items[1] };
	std::vector<uint8_t> loot_blob;
	require(player_item_snapshot_list_encode(loot_items, &loot_blob) ==
			player_snapshot_codec_result::ok,
		"could not encode exact corpse loot");
	item_transfer_payload loot = {};
	loot.from_owner = corpse_owner;
	loot.to_owner = { item_owner_type::player, 77, 0 };
	loot.reason = item_transfer_reason::corpse_loot;
	loot.reason_id = 910;
	loot.expected_from_revision = 1;
	loot.expected_to_revision = 3;
	loot.selected_item_uid = 910;
	loot.target_root_item_uid = 910;
	loot.item_count = 2;
	loot.items[0] = { 910, 910, 0, 1, 1910, item_custody_state::active };
	loot.items[1] = { 911, 910, 910, 1, 1911, item_custody_state::active };
	loot.item_blob_size = static_cast<uint32_t>(loot_blob.size());
	std::copy(loot_blob.begin(), loot_blob.end(), loot.item_blob.begin());
	loot.corpse = corpse_metadata(initial_corpse, 1, 80);
	critical_command loot_command = {};
	require(item_transfer_command_build(&loot_command, operation(8), loot,
					    critical_source_site::command,
					    critical_deadline_class::interactive),
		"could not build corpse loot command");
	loot_command.accepted_at_usec = 8;
	setenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_IMAGE", "1", 1);
	applied = flatfile_item_repository_apply(root.string(), loot_command);
	unsetenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_IMAGE");
	require(applied.outcome == critical_apply_outcome::retryable_failure &&
			applied.error_code == EIO,
		"interrupted corpse loot did not retain composite intent");
	applied = flatfile_item_repository_apply(root.string(), loot_command);
	require(applied.outcome == critical_apply_outcome::already_applied,
		"corpse loot did not recover exactly");
	require(result_of(applied).corpse_revision == 5,
		"corpse loot completion omitted the aggregate revision");
	std::vector<flatfile_corpse_record> corpses;
	std::vector<flatfile_saved_world_item_record> saved_items;
	require(flatfile_world_item_list(root.string(), &corpses, &saved_items, &error) ==
				flatfile_world_item_result::ok &&
			corpses.size() == 2 && corpses[0].revision == 5 &&
			corpses[0].items.size() == 3 && corpses[0].items[0].object_uid == 920 &&
			corpses[0].items[1].parent_index == 0 &&
			corpses[0].items[2].object_uid == 930,
		"recovered corpse loot did not remove only the exact subtree: " + error);
	items.clear();
	require(flatfile_item_repository_load_owner(root.string(), corpse_owner, &owner_revision,
						    &items, &error) ==
				flatfile_item_repository_result::ok &&
			owner_revision == 2 && items.size() == 3 && items[0].item_uid == 920 &&
			items[0].item_revision == 1,
		"recovered corpse loot did not preserve remaining corpse custody");
	items.clear();
	require(flatfile_item_repository_load_owner(
			root.string(), { item_owner_type::player, 77, 0 }, &owner_revision, &items,
			&error) == flatfile_item_repository_result::ok &&
			owner_revision == 4 && items.size() == 4 && items[2].item_uid == 910 &&
			items[2].item_revision == 2,
		"recovered corpse loot did not restore exact player custody");
	std::vector<player_item_snapshot> artifact_items = { all_corpse_items[4] };
	std::vector<uint8_t> artifact_blob;
	require(player_item_snapshot_list_encode(artifact_items, &artifact_blob) ==
			player_snapshot_codec_result::ok,
		"could not encode fenced corpse artifact");
	item_transfer_payload artifact_loot = {};
	artifact_loot.from_owner = corpse_owner;
	artifact_loot.to_owner = { item_owner_type::player, 77, 0 };
	artifact_loot.reason = item_transfer_reason::corpse_loot;
	artifact_loot.reason_id = 930;
	artifact_loot.expected_from_revision = 2;
	artifact_loot.expected_to_revision = 4;
	artifact_loot.selected_item_uid = 930;
	artifact_loot.target_root_item_uid = 930;
	artifact_loot.item_count = 1;
	artifact_loot.items[0] = { 930, 930, 0, 1, 1999, item_custody_state::active };
	artifact_loot.item_blob_size = static_cast<uint32_t>(artifact_blob.size());
	std::copy(artifact_blob.begin(), artifact_blob.end(), artifact_loot.item_blob.begin());
	artifact_loot.corpse = corpse_metadata(initial_corpse, 2, 70);
	critical_command historical_artifact_command = {};
	require(item_transfer_command_build(&historical_artifact_command, operation(10),
					    artifact_loot, critical_source_site::command,
					    critical_deadline_class::interactive),
		"could not build historical artifact loot command");
	std::vector<uint8_t> historical_payload(ITEM_TRANSFER_PAYLOAD_BYTES + sizeof(uint32_t) +
						artifact_blob.size());
	std::copy_n(historical_artifact_command.payload.begin(),
		    ITEM_TRANSFER_HEADER_BYTES +
			    artifact_loot.item_count * ITEM_TRANSFER_ENTRY_BYTES,
		    historical_payload.begin());
	for (unsigned int byte = 0; byte < sizeof(uint32_t); ++byte)
		historical_payload[ITEM_TRANSFER_PAYLOAD_BYTES + byte] =
			static_cast<uint8_t>(artifact_blob.size() >> (byte * 8));
	std::copy(artifact_blob.begin(), artifact_blob.end(),
		  historical_payload.begin() + ITEM_TRANSFER_PAYLOAD_BYTES + sizeof(uint32_t));
	historical_artifact_command.payload = std::move(historical_payload);
	historical_artifact_command.payload_version = ITEM_TRANSFER_EXACT_PAYLOAD_VERSION;
	historical_artifact_command.accepted_at_usec = 9000000000ULL;
	applied = flatfile_item_repository_apply(root.string(), historical_artifact_command);
	require(applied.outcome == critical_apply_outcome::terminal_failure &&
			applied.error_code == EOPNOTSUPP,
		"historical artifact-bearing corpse loot bypassed its missing-context fence");
	applied = flatfile_item_repository_apply(root.string(), historical_artifact_command);
	require(applied.outcome == critical_apply_outcome::terminal_failure &&
			applied.error_code == EOPNOTSUPP,
		"historical artifact-bearing corpse loot fence was not durably replayable");
	critical_command artifact_command = {};
	require(item_transfer_command_build(&artifact_command, operation(9), artifact_loot,
					    critical_source_site::command,
					    critical_deadline_class::interactive),
		"could not build contextual artifact loot command");
	artifact_command.accepted_at_usec = 10000000000ULL;
	setenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_IMAGE", "1", 1);
	applied = flatfile_item_repository_apply(root.string(), artifact_command);
	unsetenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_IMAGE");
	require(applied.outcome == critical_apply_outcome::retryable_failure &&
			applied.error_code == EIO,
		"interrupted contextual artifact loot did not retain composite intent");
	applied = flatfile_item_repository_apply(root.string(), artifact_command);
	require(applied.outcome == critical_apply_outcome::already_applied,
		"contextual artifact loot did not recover all authority images");
	require(result_of(applied).corpse_revision == 6,
		"artifact loot completion omitted the aggregate revision");
	items.clear();
	require(flatfile_item_repository_load_owner(root.string(), corpse_owner, &owner_revision,
						    &items, &error) ==
				flatfile_item_repository_result::ok &&
			owner_revision == 3 && items.size() == 2 && items[0].item_uid == 920,
		"contextual artifact loot did not publish corpse custody");
	corpses.clear();
	require(flatfile_world_item_list(root.string(), &corpses, &saved_items, &error) ==
				flatfile_world_item_result::ok &&
			corpses.size() == 2 && corpses[0].revision == 6 &&
			corpses[0].weight == 70 && corpses[0].items.size() == 2 &&
			corpses[0].items[1].parent_index == 0,
		"contextual artifact loot did not publish exact corpse metadata and topology");
	std::vector<flatfile_artifact_record> artifact_records;
	require(flatfile_artifact_list(root.string(), &artifact_records, &error) ==
				flatfile_artifact_result::ok &&
			artifact_records.size() == 1 && artifact_records[0].owned &&
			artifact_records[0].location_type == FLATFILE_ARTIFACT_ON_PLAYER &&
			artifact_records[0].location == 77 && artifact_records[0].timer == 442000 &&
			artifact_records[0].last_update == 10000 &&
			artifact_records[0].bind_owner_pid == -1 &&
			artifact_records[0].bind_timer == 10000 &&
			artifact_records[0].revision == 2,
		"contextual artifact loot did not publish cross-race artifact semantics");

	const item_owner_identity created_corpse_owner = { item_owner_type::corpse,
							   item_corpse_owner_id(77, 30), 0 };
	item_transfer_payload create_corpse = artifact_loot;
	create_corpse.from_owner = { item_owner_type::player, 77, 0 };
	create_corpse.to_owner = created_corpse_owner;
	create_corpse.reason = item_transfer_reason::corpse_create;
	create_corpse.reason_id = 30;
	create_corpse.expected_from_revision = 5;
	create_corpse.expected_to_revision = 0;
	create_corpse.items[0].expected_item_revision = 2;
	create_corpse.corpse = corpse_metadata(created_corpse, 2, 100);
	critical_command create_corpse_command = {};
	require(item_transfer_command_build(&create_corpse_command, operation(11), create_corpse,
					    critical_source_site::combat,
					    critical_deadline_class::interactive),
		"could not build first corpse creation transfer");
	create_corpse_command.accepted_at_usec = 11000000000ULL;
	setenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_IMAGE", "1", 1);
	applied = flatfile_item_repository_apply(root.string(), create_corpse_command);
	unsetenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_IMAGE");
	require(applied.outcome == critical_apply_outcome::retryable_failure &&
			applied.error_code == EIO,
		"interrupted corpse creation did not retain composite intent");
	applied = flatfile_item_repository_apply(root.string(), create_corpse_command);
	require(applied.outcome == critical_apply_outcome::already_applied,
		"first corpse creation transfer did not recover exactly");
	require(result_of(applied).corpse_revision == 2,
		"lifecycle-first corpse creation omitted the aggregate revision");
	corpses.clear();
	require(flatfile_world_item_list(root.string(), &corpses, &saved_items, &error) ==
				flatfile_world_item_result::ok &&
			corpses.size() == 2 && corpses[1].owner_pid == 77 &&
			corpses[1].owner_name == "newowner" && corpses[1].save_id == 30 &&
			corpses[1].revision == 2 && corpses[1].weight == 100 &&
			corpses[1].items.size() == 1 && corpses[1].items[0].object_uid == 930,
		"lifecycle-first corpse transfer did not establish exact metadata and contents");
	require(flatfile_artifact_list(root.string(), &artifact_records, &error) ==
				flatfile_artifact_result::ok &&
			artifact_records[0].location_type == FLATFILE_ARTIFACT_ON_CORPSE &&
			artifact_records[0].location == 77 && artifact_records[0].timer == 442000 &&
			artifact_records[0].last_update == 11000 &&
			artifact_records[0].bind_timer == 0 && artifact_records[0].revision == 3,
		"corpse creation did not atomically move artifact authority");

	item_transfer_payload append_corpse = loot;
	append_corpse.from_owner = { item_owner_type::player, 77, 0 };
	append_corpse.to_owner = created_corpse_owner;
	append_corpse.reason = item_transfer_reason::corpse_create;
	append_corpse.reason_id = 30;
	append_corpse.expected_from_revision = 6;
	append_corpse.expected_to_revision = 1;
	append_corpse.items[0].expected_item_revision = 2;
	append_corpse.items[1].expected_item_revision = 2;
	append_corpse.corpse = corpse_metadata(created_corpse, 2, 120);
	critical_command append_corpse_command = {};
	require(item_transfer_command_build(&append_corpse_command, operation(12), append_corpse,
					    critical_source_site::combat,
					    critical_deadline_class::interactive),
		"could not build subsequent corpse creation transfer");
	append_corpse_command.accepted_at_usec = 12000000000ULL;
	applied = flatfile_item_repository_apply(root.string(), append_corpse_command);
	require(applied.outcome == critical_apply_outcome::applied,
		"subsequent corpse creation transfer did not apply");
	require(result_of(applied).corpse_revision == 3,
		"subsequent corpse creation completion omitted the aggregate revision");
	corpses.clear();
	require(flatfile_world_item_list(root.string(), &corpses, &saved_items, &error) ==
				flatfile_world_item_result::ok &&
			corpses[1].revision == 3 && corpses[1].weight == 120 &&
			corpses[1].items.size() == 3 && corpses[1].items[0].object_uid == 930 &&
			corpses[1].items[1].object_uid == 910 &&
			corpses[1].items[2].parent_index == 1,
		"subsequent corpse creation transfer did not append exact nested topology");
	items.clear();
	require(flatfile_item_repository_load_owner(root.string(), created_corpse_owner,
						    &owner_revision, &items, &error) ==
				flatfile_item_repository_result::ok &&
			owner_revision == 2 && items.size() == 3 && items[0].item_uid == 910 &&
			items[0].item_revision == 3 && items[2].item_uid == 930,
		"subsequent corpse creation transfer did not publish complete custody");

	move.expected_from_revision = 1;
	move.expected_to_revision = 0;
	critical_command stale = {};
	require(item_transfer_command_build(&stale, operation(3), move,
					    critical_source_site::command,
					    critical_deadline_class::interactive),
		"could not build stale transfer command");
	stale.accepted_at_usec = 3;
	applied = flatfile_item_repository_apply(root.string(), stale);
	require(applied.outcome == critical_apply_outcome::terminal_failure &&
			applied.error_code == ESTALE,
		"stale owner revisions were accepted");
	applied = flatfile_item_repository_apply(root.string(), stale);
	require(applied.outcome == critical_apply_outcome::terminal_failure &&
			applied.error_code == ESTALE,
		"stale rejection was not durably replayable");

	const critical_command concurrent = single_creation(4, 200, 88, 1);
	for (int child = 0; child < 2; ++child)
	{
		const pid_t pid = fork();
		require(pid >= 0, "ownership writer fork failed");
		if (!pid)
		{
			const critical_apply_result child_result =
				flatfile_item_repository_apply(root.string(), concurrent);
			_exit(child_result.outcome == critical_apply_outcome::applied ||
					      child_result.outcome ==
						      critical_apply_outcome::already_applied ?
				      0 :
				      2);
		}
	}
	for (int child = 0; child < 2; ++child)
	{
		int status = 0;
		require(wait(&status) > 0 && WIFEXITED(status) && WEXITSTATUS(status) == 0,
			"concurrent idempotent ownership writer failed");
	}
	items.clear();
	require(flatfile_item_repository_load_owner(
			root.string(), { item_owner_type::player, 88, 0 }, &owner_revision, &items,
			&error) == flatfile_item_repository_result::ok &&
			owner_revision == 1 && items.size() == 1 && items[0].item_uid == 200,
		"concurrent replay duplicated or lost item creation");

	const fs::path shop_root = fs::path(argv[1]).string() + "-shop";
	fs::create_directories(shop_root / "domains");
	fs::permissions(shop_root, fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(shop_root / "domains", fs::perms::owner_all, fs::perm_options::replace);
	const item_owner_identity shop_player = { item_owner_type::player, 42, 0 };
	const item_owner_identity shop_owner = { item_owner_type::shopkeeper,
						 item_shopkeeper_owner_id(0), 0 };
	require(flatfile_item_repository_establish_owner(
			shop_root.string(), shop_player,
			{ { 100, 100, 0, shop_player, 1, 700, item_custody_state::active },
			  { 101, 101, 0, shop_player, 1, 701, item_custody_state::active } },
			&error) == flatfile_item_baseline_result::applied,
		"shop player custody baseline failed: " + error);
	require(flatfile_item_repository_establish_owner(
			shop_root.string(), shop_owner,
			{ { 200, 200, 0, shop_owner, 1, 800, item_custody_state::active },
			  { 201, 201, 0, shop_owner, 1, 801, item_custody_state::active } },
			&error) == flatfile_item_baseline_result::applied,
		"shop inventory custody baseline failed: " + error);
	flatfile_item_shop_trade_mutation shop_mutation;
	unsigned int shop_result_code = 0;
	auto produced = shop_trade(shop_trade_action::buy_produced, 300,
				   ITEM_TRANSFER_ABSENT_REVISION, 801, 201, 1);
	{
		auto stale_stock = produced;
		stale_stock.expected_stock_item_revision = 2;
		flatfile_authority_lock lock;
		require(lock.acquire(shop_root.string(), &error),
			"could not acquire stale stock lock");
		require(flatfile_item_repository_prepare_shop_trade(
				shop_root.string(), lock, stale_stock, &shop_mutation,
				&shop_result_code, &error) == flatfile_item_repository_result::ok &&
				shop_result_code == ESTALE &&
				shop_mutation.after_image.bytes.empty(),
			"stale produced-stock exemplar was accepted");
	}
	{
		flatfile_authority_lock lock;
		require(lock.acquire(shop_root.string(), &error),
			"could not acquire produced lock");
		require(flatfile_item_repository_prepare_shop_trade(
				shop_root.string(), lock, produced, &shop_mutation,
				&shop_result_code, &error) == flatfile_item_repository_result::ok &&
				shop_result_code == 0 && shop_mutation.player_owner_revision == 2 &&
				shop_mutation.counterparty_owner_revision == 1 &&
				shop_mutation.item_revisions[0] == 1,
			"produced custody mutation did not prepare: " + error);
		require(flatfile_authority_transaction_commit(
				shop_root.string(), lock, { shop_mutation.after_image }, &error) ==
				flatfile_authority_transaction_result::ok,
			"produced custody mutation did not commit: " + error);
	}
	auto purchase = shop_trade(shop_trade_action::buy_existing, 200, 1, 800, 200, 1);
	{
		flatfile_authority_lock lock;
		require(lock.acquire(shop_root.string(), &error),
			"could not acquire purchase lock");
		require(flatfile_item_repository_prepare_shop_trade(
				shop_root.string(), lock, purchase, &shop_mutation,
				&shop_result_code, &error) == flatfile_item_repository_result::ok &&
				shop_result_code == 0 && shop_mutation.player_owner_revision == 3 &&
				shop_mutation.counterparty_owner_revision == 2 &&
				shop_mutation.item_revisions[0] == 2,
			"purchase custody mutation did not prepare: " + error);
		require(flatfile_authority_transaction_commit(
				shop_root.string(), lock, { shop_mutation.after_image }, &error) ==
				flatfile_authority_transaction_result::ok,
			"purchase custody mutation did not commit: " + error);
	}
	auto sale = shop_trade(shop_trade_action::sell_store, 100, 1, 700);
	{
		flatfile_authority_lock lock;
		require(lock.acquire(shop_root.string(), &error), "could not acquire sale lock");
		require(flatfile_item_repository_prepare_shop_trade(
				shop_root.string(), lock, sale, &shop_mutation, &shop_result_code,
				&error) == flatfile_item_repository_result::ok &&
				shop_result_code == 0 && shop_mutation.player_owner_revision == 4 &&
				shop_mutation.counterparty_owner_revision == 3 &&
				shop_mutation.item_revisions[0] == 2,
			"sale custody mutation did not prepare: " + error);
		require(flatfile_authority_transaction_commit(
				shop_root.string(), lock, { shop_mutation.after_image }, &error) ==
				flatfile_authority_transaction_result::ok,
			"sale custody mutation did not commit: " + error);
	}
	auto destruction = shop_trade(shop_trade_action::sell_destroy, 101, 1, 701);
	{
		flatfile_authority_lock lock;
		require(lock.acquire(shop_root.string(), &error),
			"could not acquire destruction lock");
		require(flatfile_item_repository_prepare_shop_trade(
				shop_root.string(), lock, destruction, &shop_mutation,
				&shop_result_code, &error) == flatfile_item_repository_result::ok &&
				shop_result_code == 0 && shop_mutation.player_owner_revision == 5 &&
				shop_mutation.counterparty_owner_revision == 1 &&
				shop_mutation.item_revisions[0] == 2,
			"destruction custody mutation did not prepare: " + error);
		require(flatfile_authority_transaction_commit(
				shop_root.string(), lock, { shop_mutation.after_image }, &error) ==
				flatfile_authority_transaction_result::ok,
			"destruction custody mutation did not commit: " + error);
	}
	items.clear();
	require(flatfile_item_repository_load_owner(shop_root.string(), shop_player,
						    &owner_revision, &items, &error) ==
				flatfile_item_repository_result::ok &&
			owner_revision == 5 && items.size() == 2 && items[0].item_uid == 200 &&
			items[1].item_uid == 300,
		"shop trades did not publish exact player custody");
	items.clear();
	require(flatfile_item_repository_load_owner(shop_root.string(), shop_owner, &owner_revision,
						    &items, &error) ==
				flatfile_item_repository_result::ok &&
			owner_revision == 3 && items.size() == 2 && items[0].item_uid == 100 &&
			items[1].item_uid == 201,
		"shop trades did not preserve exact shop custody");

	const fs::path combined_root = fs::path(argv[1]).string() + "-combined";
	fs::create_directories(combined_root / "domains");
	fs::permissions(combined_root, fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(combined_root / "domains", fs::perms::owner_all, fs::perm_options::replace);
	const item_owner_identity combined_player = { item_owner_type::player, 42, 0 };
	const item_owner_identity locker_public = { item_owner_type::locker, 2, 11 };
	const item_owner_identity locker_private = { item_owner_type::locker, 2, 12 };
	require(flatfile_item_repository_establish_owner(
			combined_root.string(), combined_player,
			{ { 400, 400, 0, combined_player, 1, 800, item_custody_state::active } },
			&error) == flatfile_item_baseline_result::applied,
		"combined player owner baseline failed: " + error);
	require(flatfile_item_repository_establish_owner(
			combined_root.string(), locker_public,
			{ { 500, 500, 0, locker_public, 1, 900, item_custody_state::active },
			  { 501, 500, 500, locker_public, 1, 901, item_custody_state::active } },
			&error) == flatfile_item_baseline_result::applied,
		"combined public locker owner baseline failed: " + error);
	require(flatfile_item_repository_establish_owner(combined_root.string(), locker_private, {},
							 &error) ==
			flatfile_item_baseline_result::applied,
		"combined empty private locker owner baseline failed: " + error);
	std::vector<flatfile_locker_custody_owner> custody = {
		{ locker_public, { { 500, 900 }, { 501, 901 } } }, { locker_private, {} }
	};
	{
		flatfile_authority_lock lock;
		flatfile_authority_operation prepared;
		auto mismatch = custody;
		mismatch[0].items[1].vnum = 999;
		require(lock.acquire(combined_root.string(), &error) &&
				flatfile_item_repository_prepare_player_and_locker_remove(
					combined_root.string(), lock, 42, mismatch, &prepared,
					&error) == flatfile_item_repository_result::invalid,
			"locker payload/custody mismatch was accepted");
		require(flatfile_item_repository_prepare_player_and_locker_remove(
				combined_root.string(), lock, 42, custody, &prepared, &error) ==
					flatfile_item_repository_result::ok &&
				prepared.filename == "item_ownership",
			"combined player/locker removal was not prepared: " + error);
		require(flatfile_authority_transaction_commit_operations(
				combined_root.string(), lock, { prepared }, &error) ==
				flatfile_authority_transaction_result::ok,
			"combined player/locker removal did not commit: " + error);
	}
	for (const auto &owner : { combined_player, locker_public, locker_private })
		require(flatfile_item_repository_load_owner(combined_root.string(), owner,
							    &owner_revision, &items, &error) ==
				flatfile_item_repository_result::not_found,
			"combined removal left an item owner authoritative");

	const fs::path authority = domains / "item_ownership";
	{
		std::fstream file(authority, std::ios::in | std::ios::out | std::ios::binary);
		require(file.good(), "could not open ownership authority for corruption test");
		file.seekg(-1, std::ios::end);
		char value = 0;
		file.read(&value, 1);
		value ^= 0x5c;
		file.seekp(-1, std::ios::end);
		file.write(&value, 1);
	}
	items.clear();
	require(flatfile_item_repository_load_owner(
			root.string(), { item_owner_type::player, 77, 0 }, &owner_revision, &items,
			&error) == flatfile_item_repository_result::invalid,
		"corrupt ownership checksum was accepted");
	require(flatfile_item_repository_list_active_player_items(root.string(), &items, &error) ==
			flatfile_item_repository_result::invalid,
		"corrupt ownership checksum was exposed through player item enumeration");
	applied = flatfile_item_repository_apply(root.string(), transfer);
	require(applied.outcome == critical_apply_outcome::terminal_failure &&
			applied.error_code == EILSEQ,
		"corrupt ownership authority was overwritten");
	for (const fs::directory_entry &entry : fs::directory_iterator(domains))
		require(entry.path().filename().string().find(".tmp.") == std::string::npos,
			"temporary ownership file was left behind");

	std::cout << "flat-file item ownership repository passed\n";
	return 0;
}
