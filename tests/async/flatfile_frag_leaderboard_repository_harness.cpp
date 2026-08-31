#include "flatfile/flatfile_frag_leaderboard_repository.h"

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

int main(int argc, char **argv)
{
	require(argc == 2, "state root argument required");
	const fs::path root = argv[1];
	const fs::path domains = root / "domains";
	fs::create_directories(domains);
	fs::create_directories(root / "players");
	fs::create_directories(root / "identities/names");
	fs::permissions(root, fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(domains, fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(root / "players", fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(root / "identities", fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(root / "identities/names", fs::perms::owner_all, fs::perm_options::replace);
	std::string error;
	std::vector<flatfile_frag_leaderboard_record> records;
	require(flatfile_frag_leaderboard_list(root.string(), &records, &error) ==
			flatfile_frag_leaderboard_result::not_found,
		"missing leaderboard authority did not fail closed");
	const flatfile_frag_leaderboard_record first = { 42, "Account-A", "Alpha",   1234,
							 1,  "Human",	  "Warrior", 50,
							 0,  1000,	  3 };
	const flatfile_frag_leaderboard_record second = { 7, "Account-B", "Beta",   -200,
							  2, "Drow",	  "Cleric", 40,
							  0, 1001,	  2 };
	const fs::path bootstrap = root / "bootstrap";
	fs::create_directories(bootstrap / "domains");
	fs::permissions(bootstrap, fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(bootstrap / "domains", fs::perms::owner_all, fs::perm_options::replace);
	require(flatfile_frag_leaderboard_upsert(bootstrap.string(), first, &error) ==
			flatfile_frag_leaderboard_result::ok,
		"first leaderboard upsert did not establish a missing catalog: " + error);
	require(flatfile_frag_leaderboard_list(bootstrap.string(), &records, &error) ==
				flatfile_frag_leaderboard_result::ok &&
			records == std::vector<flatfile_frag_leaderboard_record>{ first },
		"first leaderboard upsert did not round trip");
	require(flatfile_frag_leaderboard_establish(root.string(), { first, second }, &error) ==
			flatfile_frag_leaderboard_result::ok,
		"leaderboard establishment failed: " + error);
	require(flatfile_frag_leaderboard_establish(root.string(), { second, first }, &error) ==
			flatfile_frag_leaderboard_result::already_exists,
		"exact leaderboard establishment retry was not idempotent");
	auto conflict = first;
	conflict.total_frags++;
	require(flatfile_frag_leaderboard_establish(root.string(), { conflict }, &error) ==
			flatfile_frag_leaderboard_result::invalid,
		"conflicting leaderboard establishment was accepted");
	require(flatfile_frag_leaderboard_list(root.string(), &records, &error) ==
				flatfile_frag_leaderboard_result::ok &&
			records == std::vector<flatfile_frag_leaderboard_record>{ second, first },
		"leaderboard was not canonical or did not round trip");
	auto updated = first;
	updated.total_frags = 1300;
	updated.level = 51;
	updated.last_updated = 1100;
	updated.revision = 1;
	require(flatfile_frag_leaderboard_upsert(root.string(), updated, &error) ==
			flatfile_frag_leaderboard_result::ok,
		"leaderboard update failed: " + error);
	flatfile_frag_leaderboard_record inserted = { 99,     "Account-C", "Gamma", 500,  1, "Elf",
						      "Mage", 35,	   0,	    1200, 1 };
	require(flatfile_frag_leaderboard_upsert(root.string(), inserted, &error) ==
			flatfile_frag_leaderboard_result::ok,
		"leaderboard insertion failed: " + error);
	require(flatfile_frag_leaderboard_list(root.string(), &records, &error) ==
				flatfile_frag_leaderboard_result::ok &&
			records.size() == 3 && records[1].pid == 42 &&
			records[1].total_frags == 1300 && records[1].level == 51 &&
			records[1].revision == 4 && records[2] == inserted,
		"leaderboard upsert did not preserve PID order or revision semantics");
	{
		flatfile_authority_lock lock;
		require(lock.acquire(root.string(), &error),
			"could not acquire leaderboard authority lock");
		flatfile_authority_operation operation;
		require(flatfile_frag_leaderboard_prepare_tombstone(root.string(), lock, 500, 2000,
								    &operation, &error) ==
				flatfile_frag_leaderboard_result::unchanged,
			"absent PID was not an idempotent leaderboard tombstone");
		require(flatfile_frag_leaderboard_prepare_tombstone(root.string(), lock, 42, 2000,
								    &operation, &error) ==
					flatfile_frag_leaderboard_result::ok &&
				operation.store == flatfile_authority_store::domains &&
				operation.kind == flatfile_authority_operation_kind::write &&
				operation.filename == "frag_leaderboard",
			"leaderboard tombstone did not prepare the expected image");
		require(flatfile_authority_transaction_commit_operations(root.string(), lock,
									 { operation }, &error) ==
				flatfile_authority_transaction_result::ok,
			"leaderboard tombstone transaction failed: " + error);
	}
	require(flatfile_frag_leaderboard_list(root.string(), &records, &error) ==
				flatfile_frag_leaderboard_result::ok &&
			records[1].pid == 42 && records[1].deleted_at == 2000 &&
			records[1].last_updated == 2000 && records[1].revision == 5,
		"leaderboard tombstone did not publish expected fields");
	{
		flatfile_authority_lock lock;
		require(lock.acquire(root.string(), &error),
			"could not reacquire leaderboard authority lock");
		flatfile_authority_operation operation;
		require(flatfile_frag_leaderboard_prepare_tombstone(root.string(), lock, 42, 3000,
								    &operation, &error) ==
				flatfile_frag_leaderboard_result::unchanged,
			"leaderboard tombstone retry changed deletion time or revision");
	}
	const fs::path catalog = domains / "frag_leaderboard";
	{
		std::fstream file(catalog, std::ios::in | std::ios::out | std::ios::binary);
		require(file.good(), "could not open leaderboard for corruption");
		file.seekg(-1, std::ios::end);
		char byte = 0;
		file.read(&byte, 1);
		byte ^= 0x33;
		file.seekp(-1, std::ios::end);
		file.write(&byte, 1);
	}
	require(flatfile_frag_leaderboard_list(root.string(), &records, &error) ==
				flatfile_frag_leaderboard_result::invalid &&
			flatfile_frag_leaderboard_upsert(root.string(), inserted, &error) ==
				flatfile_frag_leaderboard_result::invalid,
		"corrupt leaderboard was exposed or overwritten");
	std::cout << "flat-file frag leaderboard repository passed\n";
	return 0;
}
