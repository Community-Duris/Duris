#include "flatfile_world_item_repository.h"
#include "player_snapshot_codec.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

static void require(bool condition, const std::string &message)
{
	if (!condition)
	{
		std::cerr << message << '\n';
		exit(1);
	}
}

static void prepare_root(const fs::path &root)
{
	fs::create_directories(root / "domains");
	fs::create_directories(root / "players");
	fs::create_directories(root / "identities/names");
	fs::permissions(root, fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(root / "domains", fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(root / "players", fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(root / "identities", fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(root / "identities/names", fs::perms::owner_all, fs::perm_options::replace);
}

static player_item_snapshot item(uint64_t uid, int32_t parent, int32_t vnum)
{
	player_item_snapshot value = {};
	value.parent_index = parent;
	value.equipment_slot = -1;
	value.object_uid = uid;
	value.generated_key = static_cast<int64_t>(uid + 1000);
	value.vnum = vnum;
	value.name = "world item";
	value.short_description = "a world item";
	value.dynamic_affects.push_back({ 3, 4, 5 });
	value.extra_descriptions.push_back({ "runes", "small runes", true, { 7, 8 } });
	return value;
}

static flatfile_corpse_record corpse(uint32_t pid, uint32_t save_id, uint64_t uid)
{
	flatfile_corpse_record value = {};
	value.owner_pid = pid;
	value.owner_name = pid == 42 ? "Hero" : "Other";
	value.save_id = save_id;
	value.room_vnum = 500;
	value.short_description = "the corpse of Hero";
	value.description = "The corpse of Hero is lying here.";
	value.keywords = "corpse hero _pcorpse_";
	value.weight = 90;
	value.values = { 1, 2, 3, 4, 5, 6, 0, 8 };
	value.revision = 4;
	value.items = { item(uid, PLAYER_SNAPSHOT_NO_PARENT, 300), item(uid + 1, 0, 301) };
	return value;
}

static flatfile_saved_world_item_record saved_item()
{
	flatfile_saved_world_item_record value = {};
	value.item_key = "item.statue.1";
	value.room_vnum = 700;
	value.revision = 3;
	value.items = { item(200, PLAYER_SNAPSHOT_NO_PARENT, 400), item(201, 0, 401) };
	return value;
}

int main(int argc, char **argv)
{
	require(argc == 2, "state root argument required");
	const fs::path root = fs::path(argv[1]) / "world";
	prepare_root(root);
	std::string error;
	std::vector<flatfile_corpse_record> corpses;
	std::vector<flatfile_saved_world_item_record> saved_items;
	require(flatfile_world_item_list(root.string(), &corpses, &saved_items, &error) ==
			flatfile_world_item_result::not_found,
		"missing world item authority did not fail closed");
	const auto first = corpse(42, 20, 100);
	const auto second = corpse(77, 10, 110);
	const auto saved = saved_item();
	require(flatfile_world_item_establish(root.string(), { second, first }, { saved },
					      &error) == flatfile_world_item_result::ok,
		"world item establishment failed: " + error);
	require(flatfile_world_item_establish(root.string(), { first, second }, { saved },
					      &error) == flatfile_world_item_result::already_exists,
		"canonical world item establishment retry was not idempotent");
	require(flatfile_world_item_list(root.string(), &corpses, &saved_items, &error) ==
				flatfile_world_item_result::ok &&
			corpses.size() == 2 && corpses[0].owner_pid == 42 &&
			corpses[0].owner_name == "hero" && corpses[0].values[7] == 8 &&
			corpses[0].items.size() == 2 && corpses[0].items[1].parent_index == 0 &&
			corpses[0].items[0].dynamic_affects[0].extra2 == 5 &&
			saved_items.size() == 1 && saved_items[0].item_key == "item.statue.1" &&
			saved_items[0].items[1].extra_descriptions[0].spell_ids[1] == 8,
		"world item catalog was not canonical or did not round trip nested state");
	auto conflicting = first;
	conflicting.weight++;
	require(flatfile_world_item_establish(root.string(), { conflicting, second }, { saved },
					      &error) == flatfile_world_item_result::invalid,
		"conflicting world item establishment was accepted");

	const fs::path transfer_root = fs::path(argv[1]) / "transfer";
	prepare_root(transfer_root);
	require(flatfile_world_item_establish(transfer_root.string(), {}, {}, &error) ==
			flatfile_world_item_result::ok,
		"empty world item authority establishment failed");
	item_transfer_payload transfer = {};
	transfer.from_owner = { item_owner_type::player, 9, 0 };
	transfer.to_owner = { item_owner_type::corpse, item_corpse_owner_id(9, 33), 0 };
	transfer.reason = item_transfer_reason::corpse_create;
	transfer.selected_item_uid = 300;
	transfer.target_root_item_uid = 300;
	transfer.item_count = 1;
	transfer.items[0] = { 300, 300, 0, 1, 500, item_custody_state::active };
	const std::vector<player_item_snapshot> transferred_items = { item(
		300, PLAYER_SNAPSHOT_NO_PARENT, 500) };
	std::vector<uint8_t> transfer_blob;
	require(player_item_snapshot_list_encode(transferred_items, &transfer_blob) ==
			player_snapshot_codec_result::ok,
		"could not encode corpse transfer item");
	transfer.item_blob_size = static_cast<uint32_t>(transfer_blob.size());
	std::copy(transfer_blob.begin(), transfer_blob.end(), transfer.item_blob.begin());
	transfer.corpse.present = true;
	transfer.corpse.room_vnum = 900;
	transfer.corpse.weight = 55;
	transfer.corpse.actor_racewar = 1;
	transfer.corpse.values[3] = 9;
	transfer.corpse.values[5] = 1;
	transfer.corpse.values[6] = 33;
	transfer.corpse.owner_name = "TransferOwner";
	transfer.corpse.short_description = "the transfer corpse";
	transfer.corpse.description = "The transfer corpse is lying here.";
	transfer.corpse.keywords = "corpse transferowner _pcorpse_";
	flatfile_corpse_transfer_mutation transfer_mutation;
	{
		flatfile_authority_lock lock;
		require(lock.acquire(transfer_root.string(), &error),
			"could not acquire corpse creation authority");
		require(flatfile_world_item_prepare_corpse_transfer(
				transfer_root.string(), lock, transfer, &transfer_mutation,
				&error) == flatfile_world_item_result::ok &&
				transfer_mutation.created &&
				transfer_mutation.expected_items.empty() &&
				transfer_mutation.corpse_revision == 1,
			"first corpse transfer did not prepare establishment");
		require(flatfile_authority_transaction_commit(
				transfer_root.string(), lock, { transfer_mutation.after_image },
				&error) == flatfile_authority_transaction_result::ok,
			"first corpse transfer did not commit: " + error);
	}
	corpses.clear();
	saved_items.clear();
	require(flatfile_world_item_list(transfer_root.string(), &corpses, &saved_items, &error) ==
				flatfile_world_item_result::ok &&
			corpses.size() == 1 && corpses[0].owner_name == "transferowner" &&
			corpses[0].room_vnum == 900 && corpses[0].weight == 55 &&
			corpses[0].items.size() == 1 && corpses[0].items[0].object_uid == 300,
		"first corpse transfer did not preserve metadata and item state");
	transfer.from_owner = transfer.to_owner;
	transfer.to_owner = { item_owner_type::player, 10, 0 };
	transfer.reason = item_transfer_reason::corpse_loot;
	transfer.corpse.weight = 40;
	transfer.corpse.actor_racewar = 2;
	{
		flatfile_authority_lock lock;
		require(lock.acquire(transfer_root.string(), &error),
			"could not acquire corpse loot authority");
		require(flatfile_world_item_prepare_corpse_transfer(
				transfer_root.string(), lock, transfer, &transfer_mutation,
				&error) == flatfile_world_item_result::ok &&
				!transfer_mutation.created &&
				transfer_mutation.expected_items.size() == 1 &&
				transfer_mutation.expected_items[0].item_uid == 300 &&
				transfer_mutation.corpse_revision == 2,
			"corpse loot did not prepare exact prestate evidence");
		require(flatfile_authority_transaction_commit(
				transfer_root.string(), lock, { transfer_mutation.after_image },
				&error) == flatfile_authority_transaction_result::ok,
			"corpse loot did not commit: " + error);
	}
	corpses.clear();
	require(flatfile_world_item_list(transfer_root.string(), &corpses, &saved_items, &error) ==
				flatfile_world_item_result::ok &&
			corpses.size() == 1 && corpses[0].revision == 2 &&
			corpses[0].weight == 40 && corpses[0].items.empty(),
		"corpse loot did not retain the empty metadata aggregate");

	const fs::path invalid_root = fs::path(argv[1]) / "invalid";
	prepare_root(invalid_root);
	auto duplicate_uid = saved;
	duplicate_uid.items[0].object_uid = 100;
	require(flatfile_world_item_establish(invalid_root.string(), { first }, { duplicate_uid },
					      &error) == flatfile_world_item_result::invalid,
		"duplicate UID across corpse and saved room custody was accepted");
	auto malformed = saved;
	malformed.items[1].parent_index = 1;
	require(flatfile_world_item_establish(invalid_root.string(), {}, { malformed }, &error) ==
			flatfile_world_item_result::invalid,
		"malformed saved item nesting was accepted");
	auto duplicate_corpse = first;
	duplicate_corpse.owner_name = "Impostor";
	require(flatfile_world_item_establish(invalid_root.string(), { first, duplicate_corpse },
					      {}, &error) == flatfile_world_item_result::invalid,
		"duplicate corpse owner/save identity was accepted");
	auto two_roots = saved;
	two_roots.items[1].parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	require(flatfile_world_item_establish(invalid_root.string(), {}, { two_roots }, &error) ==
			flatfile_world_item_result::invalid,
		"saved item key with multiple roots was accepted");

	flatfile_world_item_player_removal removal;
	{
		flatfile_authority_lock lock;
		require(lock.acquire(root.string(), &error),
			"could not acquire world item authority");
		require(flatfile_world_item_prepare_player_remove(root.string(), lock, 42, "wrong",
								  &removal, &error) ==
				flatfile_world_item_result::conflict,
			"corpse owner name mismatch did not conflict");
		require(flatfile_world_item_prepare_player_remove(root.string(), lock, 42, "Hero",
								  &removal, &error) ==
					flatfile_world_item_result::ok &&
				removal.operation.filename == "world_item_catalog" &&
				removal.custody.size() == 1 &&
				removal.custody[0].owner.type == item_owner_type::corpse &&
				removal.custody[0].owner.id == item_corpse_owner_id(42, 20) &&
				removal.custody[0].items.size() == 2 &&
				removal.custody[0].items[0].item_uid == 100 &&
				removal.custody[0].items[1].vnum == 301,
			"corpse removal did not prepare exact custody evidence");
	}
	require(flatfile_world_item_list(root.string(), &corpses, &saved_items, &error) ==
				flatfile_world_item_result::ok &&
			corpses.size() == 2,
		"prepared corpse removal published before commit");
	{
		flatfile_authority_lock lock;
		require(lock.acquire(root.string(), &error),
			"could not reacquire world item authority");
		require(flatfile_authority_transaction_commit_operations(
				root.string(), lock, { removal.operation }, &error) ==
				flatfile_authority_transaction_result::ok,
			"corpse removal transaction failed: " + error);
	}
	require(flatfile_world_item_list(root.string(), &corpses, &saved_items, &error) ==
				flatfile_world_item_result::ok &&
			corpses.size() == 1 && corpses[0].owner_pid == 77 &&
			saved_items.size() == 1 && saved_items[0].items[0].object_uid == 200,
		"corpse removal did not preserve unrelated corpse and saved room state");
	{
		flatfile_authority_lock lock;
		require(lock.acquire(root.string(), &error),
			"could not acquire world item authority for retry");
		require(flatfile_world_item_prepare_player_remove(root.string(), lock, 42, "hero",
								  &removal, &error) ==
				flatfile_world_item_result::unchanged,
			"corpse removal retry was not idempotent");
	}

	const fs::path catalog = root / "domains/world_item_catalog";
	{
		std::fstream file(catalog, std::ios::in | std::ios::out | std::ios::binary);
		require(file.good(), "could not open world item catalog for corruption");
		file.seekg(-1, std::ios::end);
		char byte = 0;
		file.read(&byte, 1);
		byte ^= 0x5a;
		file.seekp(-1, std::ios::end);
		file.write(&byte, 1);
	}
	require(flatfile_world_item_list(root.string(), &corpses, &saved_items, &error) ==
			flatfile_world_item_result::invalid,
		"corrupt world item authority was exposed");
	std::cout << "flat-file world item repository passed\n";
	return 0;
}
