#include "flatfile/flatfile_locker_repository.h"

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
	value.name = "locker item";
	value.short_description = "a locker item";
	value.dynamic_affects.push_back({ 3, 4, 5 });
	value.extra_descriptions.push_back({ "runes", "small runes", true, { 7, 8 } });
	return value;
}

static flatfile_locker_record player_locker()
{
	flatfile_locker_chest_record private_chest = {};
	private_chest.chest_id = 12;
	private_chest.chest_name = "Private";
	private_chest.password_hash = "$hash";
	private_chest.sort_config = "weapons armor";
	private_chest.revision = 4;
	private_chest.items = { item(102, PLAYER_SNAPSHOT_NO_PARENT, 302) };
	flatfile_locker_chest_record public_chest = {};
	public_chest.chest_id = 11;
	public_chest.chest_name = "Public";
	public_chest.is_public = true;
	public_chest.revision = 3;
	public_chest.items = { item(100, PLAYER_SNAPSHOT_NO_PARENT, 300), item(101, 0, 301) };
	flatfile_locker_record locker = {};
	locker.locker_id = 2;
	locker.locker_name = "Hero.Locker";
	locker.owner_pid = 42;
	locker.racewar = 1;
	locker.race = 2;
	locker.revision = 9;
	locker.chests = { private_chest, public_chest };
	return locker;
}

static flatfile_locker_record guild_locker()
{
	flatfile_locker_chest_record public_chest = {};
	public_chest.chest_id = 20;
	public_chest.chest_name = "public";
	public_chest.is_public = true;
	public_chest.revision = 1;
	flatfile_locker_record locker = {};
	locker.locker_id = 1;
	locker.locker_name = "guild.7.locker";
	locker.owner_assoc_id = 7;
	locker.revision = 2;
	locker.chests = { public_chest };
	return locker;
}

int main(int argc, char **argv)
{
	require(argc == 2, "state root argument required");
	const fs::path root = fs::path(argv[1]) / "catalog";
	prepare_root(root);
	std::string error;
	std::vector<flatfile_locker_record> lockers;
	std::vector<flatfile_locker_access_record> access;
	const auto missing = flatfile_locker_list(root.string(), &lockers, &access, &error);
	require(missing == flatfile_locker_result::not_found,
		"missing locker authority did not fail closed: result=" +
			std::to_string(static_cast<int>(missing)) + " error=" + error);
	const auto player = player_locker();
	const auto guild = guild_locker();
	const std::vector<flatfile_locker_access_record> source_access = {
		{ "Hero.Locker", "Guest", 6 }, { "guild.7.locker", "Hero", 2 }
	};
	require(flatfile_locker_establish(root.string(), { player, guild }, source_access,
					  &error) == flatfile_locker_result::ok,
		"locker establishment failed: " + error);
	require(flatfile_locker_establish(root.string(), { guild, player },
					  { source_access[1], source_access[0] },
					  &error) == flatfile_locker_result::already_exists,
		"canonical locker establishment retry was not idempotent");
	require(flatfile_locker_list(root.string(), &lockers, &access, &error) ==
				flatfile_locker_result::ok &&
			lockers.size() == 2 && lockers[0].locker_id == 1 &&
			lockers[1].locker_name == "hero.locker" && lockers[1].owner_pid == 42 &&
			lockers[1].chests.size() == 2 && lockers[1].chests[0].chest_id == 11 &&
			lockers[1].chests[0].items.size() == 2 &&
			lockers[1].chests[0].items[1].parent_index == 0 &&
			lockers[1].chests[0].items[0].dynamic_affects[0].extra2 == 5 &&
			lockers[1].chests[1].password_hash == "$hash" && access.size() == 2 &&
			access[0].owner_name == "guild.7.locker" &&
			access[1].visitor_name == "guest",
		"locker catalog was not canonical or did not round trip nested state");
	auto conflicting = player;
	conflicting.revision++;
	require(flatfile_locker_establish(root.string(), { guild, conflicting }, source_access,
					  &error) == flatfile_locker_result::invalid,
		"conflicting locker establishment was accepted");

	const fs::path invalid_root = fs::path(argv[1]) / "invalid";
	prepare_root(invalid_root);
	auto account = player;
	account.locker_name = "account.user.1.locker";
	require(flatfile_locker_establish(invalid_root.string(), { account }, {}, &error) ==
			flatfile_locker_result::invalid,
		"account locker was accepted by player locker authority");
	auto duplicate_uid = player;
	duplicate_uid.chests[1].items[0].object_uid = 102;
	require(flatfile_locker_establish(invalid_root.string(), { duplicate_uid }, {}, &error) ==
			flatfile_locker_result::invalid,
		"duplicate item UID was accepted across locker chests");
	auto no_public = player;
	for (auto &chest : no_public.chests)
		chest.is_public = false;
	require(flatfile_locker_establish(invalid_root.string(), { no_public }, {}, &error) ==
			flatfile_locker_result::invalid,
		"locker without one public chest was accepted");
	require(flatfile_locker_establish(invalid_root.string(), { player },
					  { { "missing.locker", "guest", 1 } },
					  &error) == flatfile_locker_result::invalid,
		"dangling locker access was accepted");
	auto duplicate_owner = guild;
	duplicate_owner.locker_id = 3;
	duplicate_owner.locker_name = "guild.7.second";
	duplicate_owner.chests[0].chest_id = 30;
	require(flatfile_locker_establish(invalid_root.string(), { guild, duplicate_owner }, {},
					  &error) == flatfile_locker_result::invalid,
		"duplicate association locker owner was accepted");

	flatfile_locker_player_removal removal;
	{
		flatfile_authority_lock lock;
		require(lock.acquire(root.string(), &error), "could not acquire locker authority");
		require(flatfile_locker_prepare_player_remove(root.string(), lock, 42, "Hero",
							      &removal, &error) ==
					flatfile_locker_result::ok &&
				removal.operation.filename == "locker_catalog" &&
				removal.custody.size() == 2 &&
				removal.custody[0].owner.type == item_owner_type::locker &&
				removal.custody[0].owner.id == 2 &&
				removal.custody[0].owner.context_id == 11 &&
				removal.custody[0].items.size() == 2 &&
				removal.custody[0].items[0].item_uid == 100 &&
				removal.custody[1].owner.context_id == 12 &&
				removal.custody[1].items[0].vnum == 302,
			"player locker removal did not prepare exact custody metadata");
	}
	require(flatfile_locker_list(root.string(), &lockers, &access, &error) ==
				flatfile_locker_result::ok &&
			lockers.size() == 2 && access.size() == 2,
		"prepared locker removal published before transaction commit");
	{
		flatfile_authority_lock lock;
		require(lock.acquire(root.string(), &error),
			"could not reacquire locker authority for commit");
		require(flatfile_authority_transaction_commit_operations(
				root.string(), lock, { removal.operation }, &error) ==
				flatfile_authority_transaction_result::ok,
			"locker removal transaction failed: " + error);
	}
	require(flatfile_locker_list(root.string(), &lockers, &access, &error) ==
				flatfile_locker_result::ok &&
			lockers.size() == 1 && lockers[0].owner_assoc_id == 7 && access.empty(),
		"locker removal did not remove owned locker and player access rows");
	{
		flatfile_authority_lock lock;
		require(lock.acquire(root.string(), &error),
			"could not acquire locker authority for retry");
		require(flatfile_locker_prepare_player_remove(root.string(), lock, 42, "Hero",
							      &removal, &error) ==
				flatfile_locker_result::unchanged,
			"player locker removal retry was not idempotent");
	}

	const fs::path catalog = root / "domains/locker_catalog";
	{
		std::fstream file(catalog, std::ios::in | std::ios::out | std::ios::binary);
		require(file.good(), "could not open locker catalog for corruption");
		file.seekg(-1, std::ios::end);
		char byte = 0;
		file.read(&byte, 1);
		byte ^= 0x5a;
		file.seekp(-1, std::ios::end);
		file.write(&byte, 1);
	}
	require(flatfile_locker_list(root.string(), &lockers, &access, &error) ==
			flatfile_locker_result::invalid,
		"corrupt locker authority was exposed");
	std::cout << "flat-file locker repository passed\n";
	return 0;
}
