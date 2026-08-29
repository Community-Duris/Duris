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
