#include "flatfile_ship_repository.h"

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

static flatfile_ship_record ship(uint32_t id, uint32_t pid, const std::string &owner)
{
	flatfile_ship_record value = {};
	value.ship_id = id;
	value.owner_pid = pid;
	value.owner_name = owner;
	value.ship_name = "The Test Ship";
	value.ship_class = 3;
	value.frags = 4;
	value.anchor_room = 500;
	value.time_played = 600;
	value.mainsail = 70;
	value.race = 2;
	value.money = 800;
	value.flags = 9;
	value.armor = { 10, 11, 12, 13 };
	value.internal = { 20, 21, 22, 23 };
	value.crew = { 5, 1001, 1002, 1003, 6, 7, 8 };
	value.slots = { { 2, 30, 31, 32, 33, { 34, 35, 36, 37, 38 } },
			{ 0, 40, 41, 42, 43, { 44, 45, 46, 47, 48 } } };
	value.revision = 4;
	return value;
}

int main(int argc, char **argv)
{
	require(argc == 2, "state root argument required");
	const fs::path root = fs::path(argv[1]) / "ship";
	prepare_root(root);
	std::string error;
	std::vector<flatfile_ship_record> records;
	require(flatfile_ship_list(root.string(), &records, &error) ==
			flatfile_ship_result::not_found,
		"missing ship authority did not fail closed");
	const auto player = ship(2, 42, "Player");
	auto other = ship(1, 77, "Other");
	other.ship_name = "Other Ship";
	require(flatfile_ship_establish(root.string(), { player, other }, &error) ==
			flatfile_ship_result::ok,
		"ship establishment failed: " + error);
	require(flatfile_ship_establish(root.string(), { other, player }, &error) ==
			flatfile_ship_result::already_exists,
		"canonical ship establishment retry was not idempotent");
	require(flatfile_ship_list(root.string(), &records, &error) == flatfile_ship_result::ok &&
			records.size() == 2 && records[0].ship_id == 1 &&
			records[1].owner_name == "player" && records[1].armor[3] == 13 &&
			records[1].crew.repair_skill_milli == 1003 &&
			records[1].slots.size() == 2 && records[1].slots[0].slot_index == 0 &&
			records[1].slots[0].values[4] == 48 && records[1].slots[1].slot_type == 30,
		"ship catalog was not canonical or did not round trip cargo slots");
	auto conflicting = player;
	conflicting.money++;
	require(flatfile_ship_establish(root.string(), { other, conflicting }, &error) ==
			flatfile_ship_result::invalid,
		"conflicting ship establishment was accepted");

	const fs::path invalid_root = fs::path(argv[1]) / "invalid";
	prepare_root(invalid_root);
	auto duplicate = other;
	duplicate.ship_id = 3;
	duplicate.owner_pid = player.owner_pid;
	duplicate.owner_name = "third";
	require(flatfile_ship_establish(invalid_root.string(), { player, duplicate }, &error) ==
			flatfile_ship_result::invalid,
		"duplicate ship owner PID was accepted");
	auto duplicate_slot = player;
	duplicate_slot.slots[1].slot_index = duplicate_slot.slots[0].slot_index;
	require(flatfile_ship_establish(invalid_root.string(), { duplicate_slot }, &error) ==
			flatfile_ship_result::invalid,
		"duplicate ship slot was accepted");

	flatfile_authority_operation operation;
	{
		flatfile_authority_lock lock;
		require(lock.acquire(root.string(), &error), "could not acquire ship authority");
		require(flatfile_ship_prepare_player_remove(root.string(), lock, 42, "pLaYeR",
							    &operation,
							    &error) == flatfile_ship_result::ok &&
				operation.filename == "ship_catalog",
			"ship removal was not prepared: " + error);
	}
	require(flatfile_ship_list(root.string(), &records, &error) == flatfile_ship_result::ok &&
			records.size() == 2,
		"prepared ship removal published before commit");
	{
		flatfile_authority_lock lock;
		require(lock.acquire(root.string(), &error), "could not reacquire ship authority");
		require(flatfile_authority_transaction_commit_operations(root.string(), lock,
									 { operation }, &error) ==
				flatfile_authority_transaction_result::ok,
			"ship removal transaction failed: " + error);
	}
	require(flatfile_ship_list(root.string(), &records, &error) == flatfile_ship_result::ok &&
			records.size() == 1 && records[0].owner_pid == 77,
		"ship removal did not preserve unrelated ship");
	{
		flatfile_authority_lock lock;
		require(lock.acquire(root.string(), &error),
			"could not acquire ship authority for retry");
		require(flatfile_ship_prepare_player_remove(root.string(), lock, 42, "player",
							    &operation, &error) ==
				flatfile_ship_result::unchanged,
			"ship removal retry was not idempotent");
	}

	const fs::path catalog = root / "domains/ship_catalog";
	{
		std::fstream file(catalog, std::ios::in | std::ios::out | std::ios::binary);
		require(file.good(), "could not open ship catalog for corruption");
		file.seekg(-1, std::ios::end);
		char byte = 0;
		file.read(&byte, 1);
		byte ^= 0x5a;
		file.seekp(-1, std::ios::end);
		file.write(&byte, 1);
	}
	require(flatfile_ship_list(root.string(), &records, &error) ==
			flatfile_ship_result::invalid,
		"corrupt ship authority was exposed");
	std::cout << "flat-file ship repository passed\n";
	return 0;
}
