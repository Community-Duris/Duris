#include "flatfile_ship_repository.h"

#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <sys/stat.h>
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

static bool resolve_legacy_owner(const char *owner, uint32_t *pid, std::string *error)
{
	if (!owner || !pid)
		return false;
	std::string name = owner;
	for (char &character : name)
		character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
	if (name == "player")
	{
		*pid = 42;
		return true;
	}
	if (error)
		*error = "unknown fixture owner";
	return false;
}

static void write_legacy_ship(const fs::path &path, const std::string &owner)
{
	std::ofstream file(path);
	require(file.good(), "could not create legacy ship fixture");
	file << "version:3\n"
	     << "3\n"
	     << owner << "\n"
	     << "The Legacy Ship\n"
	     << "4\n"
	     << "500 600\n"
	     << "10 20\n11 21\n12 22\n13 23\n"
	     << "70\n"
	     << "5\n"
	     << "1001 1002 1003 0 0 0\n"
	     << "6 7 8 0 0 0 0 0 0 0\n";
	for (int index = 0; index < 16; ++index)
	{
		if (index == 0)
			file << "1 2\n0 -3\n44 45 46 47 48\n";
		else if (index == 1)
			file << "2 1\n4 0\n5 100 999 998 997\n";
		else
			file << "0 -1\n-1 0\n-1 -1 -1 -1 -1\n";
	}
	require(file.good(), "could not finish legacy ship fixture");
}

int main(int argc, char **argv)
{
	/* Security-sensitive fixture metadata must not depend on the invoking
	 * account's umask (developer accounts commonly use 0002). */
	umask(0077);
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

	auto created = ship(0, 88, "Newowner");
	created.revision = 0;
	require(flatfile_ship_upsert(root.string(), &created, &error) == flatfile_ship_result::ok &&
			created.ship_id == 3 && created.revision == 1,
		"new ship was not assigned a durable identity: " + error);
	created.money = 901;
	created.owner_pid = 89;
	created.owner_name = "Replacement";
	require(flatfile_ship_upsert(root.string(), &created, &error) == flatfile_ship_result::ok &&
			created.revision == 2,
		"ship update or re-owner failed: " + error);
	require(flatfile_ship_list(root.string(), &records, &error) == flatfile_ship_result::ok &&
			records.size() == 3 && records[2].ship_id == 3 &&
			records[2].owner_pid == 89 && records[2].owner_name == "replacement" &&
			records[2].money == 901,
		"updated ship aggregate did not round trip");
	auto collision = created;
	collision.owner_pid = other.owner_pid;
	collision.owner_name = other.owner_name;
	require(flatfile_ship_upsert(root.string(), &collision, &error) ==
			flatfile_ship_result::conflict,
		"ship re-owner collision was accepted");
	require(flatfile_ship_remove(root.string(), created.ship_id, "wrong", &error) ==
			flatfile_ship_result::conflict,
		"ship removal accepted the wrong owner");
	require(flatfile_ship_remove(root.string(), created.ship_id, "rEpLaCeMeNt", &error) ==
			flatfile_ship_result::ok,
		"ship removal failed: " + error);
	require(flatfile_ship_remove(root.string(), created.ship_id, "replacement", &error) ==
			flatfile_ship_result::unchanged,
		"ship removal retry was not idempotent");

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
	other.money++;
	require(flatfile_ship_upsert(root.string(), &other, &error) ==
			flatfile_ship_result::invalid,
		"corrupt ship authority was overwritten by update");
	require(flatfile_ship_remove(root.string(), other.ship_id, other.owner_name, &error) ==
			flatfile_ship_result::invalid,
		"corrupt ship authority was overwritten by removal");

	const fs::path legacy_root = fs::path(argv[1]) / "legacy-state";
	const fs::path legacy_directory = fs::path(argv[1]) / "Ships";
	prepare_root(legacy_root);
	fs::create_directories(legacy_directory);
	{
		std::ofstream index(legacy_directory / "ship_index");
		index << "Player~\n$~";
	}
	write_legacy_ship(legacy_directory / "Player", "pLaYeR");
	require(flatfile_ship_import_legacy(legacy_root.string(), legacy_directory.string(),
					    resolve_legacy_owner,
					    &error) == flatfile_ship_result::ok,
		"legacy ship import failed: " + error);
	require(flatfile_ship_list(legacy_root.string(), &records, &error) ==
				flatfile_ship_result::ok &&
			records.size() == 1 && records[0].ship_id == 1 &&
			records[0].owner_pid == 42 && records[0].owner_name == "player" &&
			records[0].ship_name == "The Legacy Ship" &&
			records[0].slots.size() == 16 && records[0].slots[0].timer == 0 &&
			records[0].slots[0].values[3] == -1 && records[0].slots[1].values[2] == -1,
		"legacy ship did not preserve and normalize the version-3 fields");
	require(flatfile_ship_import_legacy(legacy_root.string(), legacy_directory.string(),
					    resolve_legacy_owner,
					    &error) == flatfile_ship_result::already_exists,
		"legacy ship import retry was not idempotent");

	const fs::path broken_root = fs::path(argv[1]) / "broken-legacy-state";
	const fs::path broken_directory = fs::path(argv[1]) / "broken-Ships";
	prepare_root(broken_root);
	fs::create_directories(broken_directory);
	{
		std::ofstream index(broken_directory / "ship_index");
		index << "Player~\n$~";
		std::ofstream partial(broken_directory / "Player");
		partial << "version:3\n3\nPlayer\npartial\n";
	}
	require(flatfile_ship_import_legacy(broken_root.string(), broken_directory.string(),
					    resolve_legacy_owner,
					    &error) == flatfile_ship_result::invalid,
		"partial legacy ship was imported");
	require(flatfile_ship_list(broken_root.string(), &records, &error) ==
			flatfile_ship_result::not_found,
		"failed legacy import published partial authority");
	require(flatfile_ship_import_legacy(
			broken_root.string(), (fs::path(argv[1]) / "missing-Ships").string(),
			resolve_legacy_owner, &error) == flatfile_ship_result::not_found,
		"missing legacy ship tree was not distinguished from corruption");
	const fs::path symlink_root = fs::path(argv[1]) / "symlink-legacy-state";
	const fs::path symlink_directory = fs::path(argv[1]) / "symlink-Ships";
	prepare_root(symlink_root);
	fs::create_directories(symlink_directory);
	{
		std::ofstream index(symlink_directory / "ship_index");
		index << "Player~\n$~";
	}
	fs::create_symlink(legacy_directory / "Player", symlink_directory / "Player");
	require(flatfile_ship_import_legacy(symlink_root.string(), symlink_directory.string(),
					    resolve_legacy_owner,
					    &error) == flatfile_ship_result::invalid,
		"legacy ship symlink was followed");
	std::cout << "flat-file ship repository passed\n";
	return 0;
}
