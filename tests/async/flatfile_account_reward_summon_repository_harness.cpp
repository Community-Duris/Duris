#include "flatfile/flatfile_account_reward_summon_repository.h"

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

int main(int argc, char **argv)
{
	require(argc == 2, "state root argument required");
	const fs::path root = fs::path(argv[1]) / "summons";
	prepare_root(root);
	std::string error;
	std::vector<flatfile_account_reward_summon_record> records;
	require(flatfile_account_reward_summon_list(root.string(), &records, &error) ==
			flatfile_account_reward_summon_result::not_found,
		"missing summon authority did not fail closed");
	const flatfile_account_reward_summon_record first = { 20, 42, 1000, false, 3 };
	const flatfile_account_reward_summon_record second = { 10, 77, 2000, true, 4 };
	const flatfile_account_reward_summon_record third = { 10, 42, 3000, true, 5 };
	require(flatfile_account_reward_summon_establish(root.string(), { first, second, third },
							 &error) ==
			flatfile_account_reward_summon_result::ok,
		"summon establishment failed: " + error);
	require(flatfile_account_reward_summon_establish(root.string(), { third, first, second },
							 &error) ==
			flatfile_account_reward_summon_result::already_exists,
		"canonical summon establishment retry was not idempotent");
	require(flatfile_account_reward_summon_list(root.string(), &records, &error) ==
				flatfile_account_reward_summon_result::ok &&
			records.size() == 3 && records[0].grant_id == 10 && records[0].pid == 42 &&
			records[0].last_summoned_at == 3000 && records[0].recovery_ready &&
			records[2].grant_id == 20 && records[2].revision == 3,
		"summon catalog was not canonical or did not round trip");
	auto conflicting = first;
	conflicting.recovery_ready = true;
	require(flatfile_account_reward_summon_establish(root.string(), { conflicting }, &error) ==
			flatfile_account_reward_summon_result::invalid,
		"conflicting summon establishment was accepted");
	const fs::path invalid_root = fs::path(argv[1]) / "invalid";
	prepare_root(invalid_root);
	require(flatfile_account_reward_summon_establish(invalid_root.string(), { first, first },
							 &error) ==
			flatfile_account_reward_summon_result::invalid,
		"duplicate grant/PID summon reference was accepted");
	auto invalid = first;
	invalid.last_summoned_at = -1;
	require(flatfile_account_reward_summon_establish(invalid_root.string(), { invalid },
							 &error) ==
			flatfile_account_reward_summon_result::invalid,
		"negative summon timestamp was accepted");

	flatfile_authority_operation operation;
	{
		flatfile_authority_lock lock;
		require(lock.acquire(root.string(), &error), "could not acquire summon authority");
		require(flatfile_account_reward_summon_prepare_player_remove(
				root.string(), lock, 42, &operation, &error) ==
					flatfile_account_reward_summon_result::ok &&
				operation.filename == "account_reward_summon_catalog",
			"summon reference removal was not prepared");
	}
	require(flatfile_account_reward_summon_list(root.string(), &records, &error) ==
				flatfile_account_reward_summon_result::ok &&
			records.size() == 3,
		"prepared summon removal published before commit");
	{
		flatfile_authority_lock lock;
		require(lock.acquire(root.string(), &error),
			"could not reacquire summon authority");
		require(flatfile_authority_transaction_commit_operations(root.string(), lock,
									 { operation }, &error) ==
				flatfile_authority_transaction_result::ok,
			"summon removal transaction failed: " + error);
	}
	require(flatfile_account_reward_summon_list(root.string(), &records, &error) ==
				flatfile_account_reward_summon_result::ok &&
			records.size() == 1 && records[0].pid == 77 && records[0].grant_id == 10,
		"summon removal did not preserve another character's grant reference");
	{
		flatfile_authority_lock lock;
		require(lock.acquire(root.string(), &error),
			"could not acquire summon authority for retry");
		require(flatfile_account_reward_summon_prepare_player_remove(
				root.string(), lock, 42, &operation, &error) ==
				flatfile_account_reward_summon_result::unchanged,
			"summon removal retry was not idempotent");
	}

	const fs::path catalog = root / "domains/account_reward_summon_catalog";
	{
		std::fstream file(catalog, std::ios::in | std::ios::out | std::ios::binary);
		require(file.good(), "could not open summon catalog for corruption");
		file.seekg(-1, std::ios::end);
		char byte = 0;
		file.read(&byte, 1);
		byte ^= 0x5a;
		file.seekp(-1, std::ios::end);
		file.write(&byte, 1);
	}
	require(flatfile_account_reward_summon_list(root.string(), &records, &error) ==
			flatfile_account_reward_summon_result::invalid,
		"corrupt summon authority was exposed");
	std::cout << "flat-file account reward summon repository passed\n";
	return 0;
}
