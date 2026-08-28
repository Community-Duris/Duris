#include "flatfile_artifact_repository.h"

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
	const fs::path root = fs::path(argv[1]) / "release";
	prepare_root(root);
	std::string error;
	std::vector<flatfile_artifact_record> records;
	require(flatfile_artifact_list(root.string(), &records, &error) ==
			flatfile_artifact_result::not_found,
		"missing artifact authority did not fail closed");
	const flatfile_artifact_record held = {
		100, true, FLATFILE_ARTIFACT_ON_PLAYER, 42, 9000, 1, 1000, 42, 8000, 5
	};
	const flatfile_artifact_record bound = {
		200, false, FLATFILE_ARTIFACT_ON_GROUND, 1201, 0, 2, 1001, 42, 7000, 7
	};
	const flatfile_artifact_record untouched = {
		300, true, FLATFILE_ARTIFACT_ON_NPC, 81, 6000, 3, 1002, -1, 0, 2
	};
	require(flatfile_artifact_establish(root.string(), { untouched, held, bound }, &error) ==
			flatfile_artifact_result::ok,
		"artifact establishment failed: " + error);
	require(flatfile_artifact_establish(root.string(), { bound, untouched, held }, &error) ==
			flatfile_artifact_result::already_exists,
		"exact artifact establishment retry was not idempotent");
	auto conflicting = held;
	conflicting.timer++;
	require(flatfile_artifact_establish(root.string(), { conflicting }, &error) ==
			flatfile_artifact_result::invalid,
		"conflicting artifact establishment was accepted");
	require(flatfile_artifact_list(root.string(), &records, &error) ==
				flatfile_artifact_result::ok &&
			records == std::vector<flatfile_artifact_record>{ held, bound, untouched },
		"artifact catalog was not canonical or did not round trip");
	{
		flatfile_authority_lock lock;
		require(lock.acquire(root.string(), &error),
			"could not acquire artifact authority");
		flatfile_authority_operation operation;
		require(flatfile_artifact_prepare_player_release(root.string(), lock, 99,
								 &operation, &error) ==
				flatfile_artifact_result::unchanged,
			"unreferenced player did not produce an unchanged artifact result");
		require(flatfile_artifact_prepare_player_release(root.string(), lock, 42,
								 &operation, &error) ==
					flatfile_artifact_result::ok &&
				operation.store == flatfile_authority_store::domains &&
				operation.kind == flatfile_authority_operation_kind::write &&
				operation.filename == "artifact_catalog",
			"player artifact release did not prepare a catalog image");
		require(flatfile_authority_transaction_commit_operations(root.string(), lock,
									 { operation }, &error) ==
				flatfile_authority_transaction_result::ok,
			"artifact release transaction failed: " + error);
	}
	require(flatfile_artifact_list(root.string(), &records, &error) ==
				flatfile_artifact_result::ok &&
			records.size() == 3 && !records[0].owned &&
			records[0].location_type == FLATFILE_ARTIFACT_NOT_IN_GAME &&
			records[0].location == 0 && records[0].timer == 0 &&
			records[0].bind_owner_pid == -1 && records[0].bind_timer == 0 &&
			records[0].revision == 6 &&
			records[1].location_type == FLATFILE_ARTIFACT_ON_GROUND &&
			records[1].location == 1201 && records[1].bind_owner_pid == -1 &&
			records[1].bind_timer == 0 && records[1].revision == 8 &&
			records[2] == untouched,
		"artifact release did not preserve and rewrite the expected fields");
	{
		flatfile_authority_lock lock;
		require(lock.acquire(root.string(), &error),
			"could not reacquire artifact authority");
		flatfile_authority_operation operation;
		require(flatfile_artifact_prepare_player_release(root.string(), lock, 42,
								 &operation, &error) ==
				flatfile_artifact_result::unchanged,
			"artifact release retry was not idempotent");
	}

	const fs::path corpse_root = fs::path(argv[1]) / "corpse";
	prepare_root(corpse_root);
	const flatfile_artifact_record corpse = {
		400, true, FLATFILE_ARTIFACT_ON_CORPSE, 42, 5000, 1, 1003, 42, 4000, 1
	};
	require(flatfile_artifact_establish(corpse_root.string(), { corpse }, &error) ==
			flatfile_artifact_result::ok,
		"corpse artifact establishment failed");
	{
		flatfile_authority_lock lock;
		require(lock.acquire(corpse_root.string(), &error),
			"could not acquire corpse artifact authority");
		flatfile_authority_operation operation;
		require(flatfile_artifact_prepare_player_release(corpse_root.string(), lock, 42,
								 &operation, &error) ==
				flatfile_artifact_result::conflict,
			"corpse-held artifact did not fence player deletion");
	}
	require(flatfile_artifact_list(corpse_root.string(), &records, &error) ==
				flatfile_artifact_result::ok &&
			records == std::vector<flatfile_artifact_record>{ corpse },
		"corpse conflict changed artifact authority");

	const fs::path catalog = root / "domains/artifact_catalog";
	{
		std::fstream file(catalog, std::ios::in | std::ios::out | std::ios::binary);
		require(file.good(), "could not open artifact catalog for corruption");
		file.seekg(-1, std::ios::end);
		char byte = 0;
		file.read(&byte, 1);
		byte ^= 0x5a;
		file.seekp(-1, std::ios::end);
		file.write(&byte, 1);
	}
	require(flatfile_artifact_list(root.string(), &records, &error) ==
			flatfile_artifact_result::invalid,
		"corrupt artifact authority was exposed");
	{
		flatfile_authority_lock lock;
		require(lock.acquire(root.string(), &error),
			"could not acquire corrupt artifact authority");
		flatfile_authority_operation operation;
		require(flatfile_artifact_prepare_player_release(root.string(), lock, 42,
								 &operation, &error) ==
				flatfile_artifact_result::invalid,
			"corrupt artifact authority was accepted for deletion");
	}
	std::cout << "flat-file artifact repository passed\n";
	return 0;
}
