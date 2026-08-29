#include "flatfile_association_repository.h"

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

static flatfile_association_record guild()
{
	flatfile_association_record value = {};
	value.association_id = 7;
	value.name = "The Test Guild";
	value.racewar = 1;
	value.bits = 2;
	value.prestige = 100;
	value.construction = 200;
	value.platinum = 3;
	value.gold = 4;
	value.silver = 5;
	value.copper = 6;
	value.frags = 1500;
	value.top_frags = 1200;
	value.top_fragger = "Player";
	value.ranks[0] = "Member";
	value.ranks[7] = "Leader";
	value.revision = 9;
	value.members = { { 2, "Other", 3, 40, 0, 300, 2 }, { 1, "Player", 7, 50, 2, 1200, 4 } };
	return value;
}

int main(int argc, char **argv)
{
	require(argc == 2, "state root argument required");
	const fs::path root = fs::path(argv[1]) / "association";
	prepare_root(root);
	std::string error;
	std::vector<flatfile_association_record> records;
	require(flatfile_association_list(root.string(), &records, &error) ==
			flatfile_association_result::not_found,
		"missing association authority did not fail closed");
	const auto source = guild();
	require(flatfile_association_establish(root.string(), { source }, &error) ==
			flatfile_association_result::ok,
		"association establishment failed: " + error);
	require(flatfile_association_establish(root.string(), { source }, &error) ==
			flatfile_association_result::already_exists,
		"association establishment retry was not idempotent");
	require(flatfile_association_save(root.string(), source, &error) ==
			flatfile_association_result::unchanged,
		"unchanged association save advanced authority");
	require(flatfile_association_list(root.string(), &records, &error) ==
				flatfile_association_result::ok &&
			records.size() == 1 && records[0].members.size() == 2 &&
			records[0].members[0].pid == 1 && records[0].members[0].name == "player" &&
			records[0].members[0].contributed_frags == 1200 &&
			records[0].members[1].pid == 2 && records[0].top_fragger == "player" &&
			records[0].ranks[7] == "Leader" && records[0].platinum == 3,
		"association catalog was not canonical or did not round trip");
	auto conflicting = source;
	conflicting.frags++;
	require(flatfile_association_establish(root.string(), { conflicting }, &error) ==
			flatfile_association_result::invalid,
		"conflicting association establishment was accepted");

	const fs::path invalid_root = fs::path(argv[1]) / "invalid";
	prepare_root(invalid_root);
	auto duplicate = source;
	duplicate.association_id = 8;
	duplicate.members = { source.members[0] };
	require(flatfile_association_establish(invalid_root.string(), { source, duplicate },
					       &error) == flatfile_association_result::invalid,
		"duplicate cross-association member PID was accepted");
	auto bad_top = source;
	bad_top.top_fragger.clear();
	require(flatfile_association_establish(invalid_root.string(), { bad_top }, &error) ==
			flatfile_association_result::invalid,
		"inconsistent top-fragger aggregate was accepted");

	auto updated = source;
	updated.prestige = 101;
	updated.members[1].debt = 51;
	require(flatfile_association_save(root.string(), updated, &error) ==
			flatfile_association_result::ok,
		"association update failed: " + error);
	require(flatfile_association_list(root.string(), &records, &error) ==
				flatfile_association_result::ok &&
			records.size() == 1 && records[0].prestige == 101 &&
			records[0].revision == 10 && records[0].members[0].debt == 51 &&
			records[0].members[0].revision == 5 && records[0].members[1].revision == 2,
		"association update did not preserve and advance revisions exactly");
	auto duplicate_save = source;
	duplicate_save.association_id = 8;
	duplicate_save.name = "Duplicate Member Guild";
	require(flatfile_association_save(root.string(), duplicate_save, &error) ==
			flatfile_association_result::invalid,
		"association save accepted a member PID owned by another guild");
	auto second = source;
	second.association_id = 8;
	second.name = "Second Guild";
	second.frags = 0;
	second.top_frags = 0;
	second.top_fragger.clear();
	second.members.clear();
	require(flatfile_association_save(root.string(), second, &error) ==
			flatfile_association_result::ok,
		"new association save failed: " + error);
	require(flatfile_association_erase(root.string(), 8, &error) ==
			flatfile_association_result::ok,
		"association erase failed: " + error);
	require(flatfile_association_erase(root.string(), 8, &error) ==
			flatfile_association_result::unchanged,
		"association erase retry was not idempotent");

	std::vector<std::string> messages;
	require(flatfile_association_ledger_list(root.string(), 7, false, &messages, &error) ==
			flatfile_association_result::not_found,
		"missing association ledger did not report not found");
	require(flatfile_association_ledger_append(root.string(), 7, false, "Player first",
						   &error) == flatfile_association_result::ok &&
			flatfile_association_ledger_append(root.string(), 7, true,
							   "System withdrew coins", &error) ==
				flatfile_association_result::ok &&
			flatfile_association_ledger_append(root.string(), 7, false, "Player second",
							   &error) ==
				flatfile_association_result::ok,
		"association ledger append failed: " + error);
	require(flatfile_association_ledger_list(root.string(), 7, false, &messages, &error) ==
				flatfile_association_result::ok &&
			messages == std::vector<std::string>{ "Player second", "Player first" },
		"player association ledger did not list newest first");
	require(flatfile_association_ledger_list(root.string(), 7, true, &messages, &error) ==
				flatfile_association_result::ok &&
			messages == std::vector<std::string>{ "System withdrew coins" },
		"system association ledger was not filtered independently");
	for (int index = 0; index <= 100; ++index)
		require(flatfile_association_ledger_append(
				root.string(), 7, false, "Player " + std::to_string(index),
				&error) == flatfile_association_result::ok,
			"association ledger retention append failed: " + error);
	require(flatfile_association_ledger_list(root.string(), 7, false, &messages, &error) ==
				flatfile_association_result::ok &&
			messages.size() == 100 && messages.front() == "Player 100" &&
			messages.back() == "Player 1",
		"association ledger did not retain the newest 100 player entries");
	require(flatfile_association_ledger_list(root.string(), 7, true, &messages, &error) ==
				flatfile_association_result::ok &&
			messages == std::vector<std::string>{ "System withdrew coins" },
		"player ledger retention removed system history");
	const fs::path ledger = root / "domains/association_ledger_7";
	const auto ledger_permissions = fs::status(ledger).permissions();
	require((ledger_permissions & (fs::perms::group_all | fs::perms::others_all)) ==
			fs::perms::none,
		"association ledger permissions are not private");
	require(flatfile_association_ledger_append(root.string(), 7, false, "bad\nentry", &error) ==
			flatfile_association_result::invalid,
		"association ledger accepted a control character");

	std::vector<flatfile_alliance_record> alliance_records;
	require(flatfile_alliance_list(root.string(), &alliance_records, &error) ==
			flatfile_association_result::not_found,
		"missing alliance authority did not report not found");
	const std::vector<flatfile_alliance_record> alliance_source = { { 9, 10, 20 },
									{ 7, 8, -5 } };
	require(flatfile_alliance_replace(root.string(), alliance_source, &error) ==
			flatfile_association_result::ok,
		"alliance authority creation failed: " + error);
	require(flatfile_alliance_list(root.string(), &alliance_records, &error) ==
				flatfile_association_result::ok &&
			alliance_records.size() == 2 &&
			alliance_records[0].forging_association_id == 7 &&
			alliance_records[0].joining_association_id == 8 &&
			alliance_records[0].tribute_owed == -5 &&
			alliance_records[1].forging_association_id == 9,
		"alliance authority was not canonical or did not round trip");
	require(flatfile_alliance_replace(root.string(), alliance_source, &error) ==
			flatfile_association_result::unchanged,
		"unchanged alliance rewrite advanced authority");
	require(flatfile_alliance_replace(root.string(), { { 7, 8, 0 }, { 7, 9, 0 } }, &error) ==
			flatfile_association_result::invalid,
		"alliance authority accepted a guild in two alliances");
	require(flatfile_alliance_replace(root.string(), { { 7, 7, 0 } }, &error) ==
			flatfile_association_result::invalid,
		"alliance authority accepted a self-alliance");
	require(flatfile_alliance_replace(root.string(), {}, &error) ==
				flatfile_association_result::ok &&
			flatfile_alliance_list(root.string(), &alliance_records, &error) ==
				flatfile_association_result::ok &&
			alliance_records.empty(),
		"alliance authority did not persist an empty replacement");
	require(flatfile_alliance_replace(root.string(), alliance_source, &error) ==
			flatfile_association_result::ok,
		"alliance authority could not be restored for corruption test");

	flatfile_authority_operation operation;
	{
		flatfile_authority_lock lock;
		require(lock.acquire(root.string(), &error),
			"could not acquire association authority");
		require(flatfile_association_prepare_player_remove(root.string(), lock, 1, "pLaYeR",
								   &operation, &error) ==
					flatfile_association_result::ok &&
				operation.filename == "association_catalog",
			"association member removal was not prepared: " + error);
	}
	require(flatfile_association_list(root.string(), &records, &error) ==
				flatfile_association_result::ok &&
			records[0].members.size() == 2,
		"prepared association removal published before commit");
	{
		flatfile_authority_lock lock;
		require(lock.acquire(root.string(), &error),
			"could not reacquire association authority");
		require(flatfile_authority_transaction_commit_operations(root.string(), lock,
									 { operation }, &error) ==
				flatfile_authority_transaction_result::ok,
			"association removal transaction failed: " + error);
	}
	require(flatfile_association_list(root.string(), &records, &error) ==
				flatfile_association_result::ok &&
			records[0].members.size() == 1 && records[0].members[0].pid == 2 &&
			records[0].frags == 300 && records[0].top_frags == 0 &&
			records[0].top_fragger.empty() && records[0].revision == 11,
		"association removal did not update membership and frag aggregates");
	{
		flatfile_authority_lock lock;
		require(lock.acquire(root.string(), &error),
			"could not acquire association authority for retry");
		require(flatfile_association_prepare_player_remove(root.string(), lock, 1, "player",
								   &operation, &error) ==
				flatfile_association_result::unchanged,
			"association removal retry was not idempotent");
	}

	{
		std::fstream file(ledger, std::ios::in | std::ios::out | std::ios::binary);
		require(file.good(), "could not open association ledger for corruption");
		file.seekg(-1, std::ios::end);
		char byte = 0;
		file.read(&byte, 1);
		byte ^= 0x33;
		file.seekp(-1, std::ios::end);
		file.write(&byte, 1);
	}
	require(flatfile_association_ledger_list(root.string(), 7, false, &messages, &error) ==
			flatfile_association_result::invalid,
		"corrupt association ledger was exposed");
	require(flatfile_association_ledger_append(root.string(), 7, false, "overwrite", &error) ==
			flatfile_association_result::invalid,
		"association ledger append overwrote corrupt history");
	const fs::path alliance_file = root / "domains/association_alliances";
	{
		std::fstream file(alliance_file, std::ios::in | std::ios::out | std::ios::binary);
		require(file.good(), "could not open alliance authority for corruption");
		file.seekg(-1, std::ios::end);
		char byte = 0;
		file.read(&byte, 1);
		byte ^= 0x71;
		file.seekp(-1, std::ios::end);
		file.write(&byte, 1);
	}
	require(flatfile_alliance_list(root.string(), &alliance_records, &error) ==
			flatfile_association_result::invalid,
		"corrupt alliance authority was exposed");
	require(flatfile_alliance_replace(root.string(), alliance_source, &error) ==
			flatfile_association_result::invalid,
		"alliance replacement overwrote corrupt authority");

	const fs::path catalog = root / "domains/association_catalog";
	{
		std::fstream file(catalog, std::ios::in | std::ios::out | std::ios::binary);
		require(file.good(), "could not open association catalog for corruption");
		file.seekg(-1, std::ios::end);
		char byte = 0;
		file.read(&byte, 1);
		byte ^= 0x5a;
		file.seekp(-1, std::ios::end);
		file.write(&byte, 1);
	}
	require(flatfile_association_list(root.string(), &records, &error) ==
			flatfile_association_result::invalid,
		"corrupt association authority was exposed");
	require(flatfile_association_save(root.string(), source, &error) ==
			flatfile_association_result::invalid,
		"association save overwrote corrupt authority");
	require(flatfile_association_erase(root.string(), 7, &error) ==
			flatfile_association_result::invalid,
		"association erase overwrote corrupt authority");
	std::cout << "flat-file association repository passed\n";
	return 0;
}
