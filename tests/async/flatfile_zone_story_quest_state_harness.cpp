#include "flatfile/flatfile_zone_story_quest_state.h"

#include <cstdlib>
#include <vector>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <cstring>
#include <openssl/sha.h>

namespace
{
void require(bool condition, const char *message)
{
	if (!condition)
	{
		std::cerr << message << '\n';
		std::exit(1);
	}
}
} // namespace

int main()
{
	const std::filesystem::path root =
		std::filesystem::temp_directory_path() / "duris-zone-story-flatfile-state-test";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root / "domains");
	std::filesystem::permissions(root, std::filesystem::perms::owner_all,
				     std::filesystem::perm_options::replace);
	std::filesystem::permissions(root / "domains", std::filesystem::perms::owner_all,
				     std::filesystem::perm_options::replace);
	std::string error;
	const std::string state = "ZSQF|1\nT|74657374|76616c7565\n";
	require(flatfile_zone_story_quest_state_save(root.string().c_str(), 7, state, &error) ==
			flatfile_zone_story_quest_result::ok,
		"flat-file state did not save");
	std::string recovered;
	require(flatfile_zone_story_quest_state_load(root.string().c_str(), 7, &recovered,
						     &error) ==
				flatfile_zone_story_quest_result::ok &&
			recovered == state,
		"flat-file state did not round-trip");
	require(flatfile_zone_story_quest_state_load(root.string().c_str(), 8, &recovered,
						     &error) ==
			flatfile_zone_story_quest_result::corrupt,
		"flat-file state accepted a stale catalog revision");

	const auto state_path = root / "domains" / "zone-story-quests.state";
	std::fstream file(state_path, std::ios::in | std::ios::out | std::ios::binary);
	file.seekp(-1, std::ios::end);
	char value = 0;
	file.read(&value, 1);
	file.seekp(-1, std::ios::end);
	value ^= 1;
	file.write(&value, 1);
	file.close();
	require(flatfile_zone_story_quest_state_load(root.string().c_str(), 7, &recovered,
						     &error) ==
			flatfile_zone_story_quest_result::corrupt,
		"flat-file checksum corruption was not detected");

	using namespace zone_story_quest_state;
	changes snapshot;
	snapshot.replace = true;
	snapshot.values = { { "meta", "ZSQF|2\nK|0\n" },
			    { "character:7:42", "N|7|42|416c696365|1\n" } };
	require(flatfile_zone_story_quest_records_save(root.string().c_str(), 2, snapshot,
						       &error) ==
			flatfile_zone_story_quest_result::ok,
		"journal snapshot did not save");
	changes delta{ { { "character:7:42",
			   "N|7|42|416c696365|1\nV|7|42|831|83450|864001|arrival\n" } },
		       false };
	require(flatfile_zone_story_quest_records_save(root.string().c_str(), 2, delta, &error) ==
			flatfile_zone_story_quest_result::ok,
		"journal delta did not save");
	bool legacy = true;
	require(flatfile_zone_story_quest_state_load(root.string().c_str(), 2, &recovered, &error,
						     &legacy) ==
				flatfile_zone_story_quest_result::ok &&
			!legacy && recovered.find("V|7|42|831") != std::string::npos,
		"journal delta did not recover");
	const auto committed_size = std::filesystem::file_size(state_path);
	const std::string committed_state = recovered;
	changes remove_metadata{ { { "meta", "" } }, false };
	require(flatfile_zone_story_quest_records_save(root.string().c_str(), 2, remove_metadata,
						       &error) ==
				flatfile_zone_story_quest_result::corrupt &&
			std::filesystem::file_size(state_path) == committed_size &&
			flatfile_zone_story_quest_state_load(root.string().c_str(), 2, &recovered,
							     &error) ==
				flatfile_zone_story_quest_result::ok &&
			recovered == committed_state,
		"metadata deletion damaged the committed journal");
	std::ifstream input(state_path, std::ios::binary);
	std::vector<char> initial(70);
	input.read(initial.data(), initial.size());
	input.close();
	std::ofstream append(state_path, std::ios::binary | std::ios::app);
	append.write(initial.data(), initial.size());
	append.close();
	require(flatfile_zone_story_quest_state_load(root.string().c_str(), 2, &recovered,
						     &error) ==
				flatfile_zone_story_quest_result::ok &&
			recovered.find("V|7|42|831") != std::string::npos,
		"interrupted final append contributed facts");
	changes next{ { { "character:7:43", "N|7|43|426f62|1\n" } }, false };
	require(flatfile_zone_story_quest_records_save(root.string().c_str(), 2, next, &error) ==
				flatfile_zone_story_quest_result::ok &&
			std::filesystem::file_size(state_path) > committed_size,
		"journal did not repair its torn tail");
	require(flatfile_zone_story_quest_state_load(root.string().c_str(), 2, &recovered,
						     &error) ==
				flatfile_zone_story_quest_result::ok &&
			recovered.find("N|7|43") != std::string::npos,
		"journal append after torn tail was lost");
	snapshot.values = { { "meta", "ZSQF|2\nK|0\n" },
			    { "character:7:42", "X|7|42\n" },
			    { "character:7:43", "N|7|43|426f62|1\n" } };
	require(flatfile_zone_story_quest_records_save(root.string().c_str(), 2, snapshot,
						       &error) ==
			flatfile_zone_story_quest_result::ok,
		"erasure snapshot failed");
	require(flatfile_zone_story_quest_state_load(root.string().c_str(), 2, &recovered,
						     &error) ==
				flatfile_zone_story_quest_result::ok &&
			recovered.find("V|") == std::string::npos &&
			recovered.find("N|7|43") != std::string::npos,
		"erasure lost another PID or retained erased facts");
	std::ifstream erased(state_path, std::ios::binary);
	const std::string erased_bytes((std::istreambuf_iterator<char>(erased)), {});
	require(erased_bytes.find(hex("V|7|42|831")) == std::string::npos,
		"erased discovery remains in old journal frames");
	const std::string aliases = "ZSQF|1\nN|1|1|506c61796572|1\nN|7|1|4f6c64416c696173|1\n"
				    "N|1|2|5365636f6e64|1\n";
	require(flatfile_zone_story_quest_state_save(root.string().c_str(), 7, aliases, &error) ==
			flatfile_zone_story_quest_result::ok,
		"alias fixture did not save");
	std::ifstream aliases_file(state_path, std::ios::binary);
	const std::string aliases_bytes((std::istreambuf_iterator<char>(aliases_file)), {});
	aliases_file.close();
	{
		flatfile_authority_lock lock;
		require(lock.acquire(root.string(), &error), "alias transaction lock failed");
		flatfile_authority_operation operation;
		require(flatfile_zone_story_quest_state_prepare_player_remove(
				root.string(), lock, 8, 1, &operation, &error) ==
					flatfile_zone_story_quest_result::corrupt &&
				operation.bytes.empty(),
			"stale alias catalog revision did not refuse preparation");
		require(flatfile_zone_story_quest_state_prepare_player_remove(
				root.string(), lock, 7, 1, &operation, &error) ==
				flatfile_zone_story_quest_result::ok,
			"alias erasure did not prepare");
		// Preparation must leave the authority untouched until its journal commits.
		std::ifstream original(state_path, std::ios::binary);
		std::string raw((std::istreambuf_iterator<char>(original)), {});
		require(raw == aliases_bytes, "alias preparation changed native authority");
		require(flatfile_authority_transaction_commit_operations(root.string(), lock,
									 { operation }, &error) ==
				flatfile_authority_transaction_result::ok,
			"alias transaction did not commit");
		require(flatfile_zone_story_quest_state_prepare_player_remove(
				root.string(), lock, 7, 1, &operation, &error) ==
					flatfile_zone_story_quest_result::unchanged &&
				operation.bytes.empty(),
			"alias erasure retry was not idempotent");
	}
	require(flatfile_zone_story_quest_state_load(root.string().c_str(), 7, &recovered,
						     &error) ==
				flatfile_zone_story_quest_result::ok &&
			recovered.find("N|1|1|") == std::string::npos &&
			recovered.find("N|7|1|") == std::string::npos &&
			recovered.find("N|1|2|") != std::string::npos &&
			recovered.find("X|1|1") != std::string::npos &&
			recovered.find("X|7|1") != std::string::npos,
		"alias commit retained personal state or erased another PID");
	// Seed the aggregate format used before record journals. Catalog revision
	// one remains readable during the supported revision-two upgrade.
	std::vector<unsigned char> old_snapshot(56 + aliases.size());
	std::memcpy(old_snapshot.data(), "DURZQST1", 8);
	old_snapshot[8] = 1;
	old_snapshot[12] = 1;
	for (size_t offset = 0; offset < 8; ++offset)
		old_snapshot[16 + offset] =
			(static_cast<uint64_t>(aliases.size()) >> (8 * offset)) & 255;
	SHA256(reinterpret_cast<const unsigned char *>(aliases.data()), aliases.size(),
	       old_snapshot.data() + 24);
	std::memcpy(old_snapshot.data() + 56, aliases.data(), aliases.size());
	{
		std::ofstream old_file(state_path, std::ios::binary | std::ios::trunc);
		old_file.write(reinterpret_cast<const char *>(old_snapshot.data()),
			       old_snapshot.size());
	}
	legacy = false;
	require(flatfile_zone_story_quest_state_load(root.string().c_str(), 2, &recovered, &error,
						     &legacy) ==
				flatfile_zone_story_quest_result::ok &&
			legacy && recovered == aliases,
		"legacy aggregate revision upgrade did not load");
	{
		flatfile_authority_lock lock;
		require(lock.acquire(root.string(), &error), "legacy aggregate lock failed");
		flatfile_authority_operation operation;
		require(flatfile_zone_story_quest_state_prepare_player_remove(
				root.string(), lock, 2, 1, &operation, &error) ==
					flatfile_zone_story_quest_result::ok &&
				flatfile_authority_transaction_commit_operations(
					root.string(), lock, { operation }, &error) ==
					flatfile_authority_transaction_result::ok,
			"legacy aggregate erasure did not commit");
	}
	legacy = true;
	require(flatfile_zone_story_quest_state_load(root.string().c_str(), 2, &recovered, &error,
						     &legacy) ==
				flatfile_zone_story_quest_result::ok &&
			!legacy && recovered.find("N|1|1|") == std::string::npos &&
			recovered.find("N|7|1|") == std::string::npos &&
			recovered.find("N|1|2|") != std::string::npos,
		"legacy aggregate erasure lost another player or retained erased aliases");
	const std::string encounters = "ZSQF|3\nK|0\nN|1|1|506c61796572|1\n"
				       "V|1|1|5000|544842|100|arrival\nM|1|1|500023|544842|100\n"
				       "N|1|2|5365636f6e64|1\n"
				       "V|1|2|5000|544842|100|arrival\nM|1|2|500023|544842|100\n";
	require(flatfile_zone_story_quest_state_save(root.string().c_str(), 7, encounters,
						     &error) ==
			flatfile_zone_story_quest_result::ok,
		"encounter fixture did not save");
	{
		flatfile_authority_lock lock;
		require(lock.acquire(root.string(), &error), "encounter transaction lock failed");
		flatfile_authority_operation operation;
		require(flatfile_zone_story_quest_state_prepare_player_remove(
				root.string(), lock, 7, 1, &operation, &error) ==
					flatfile_zone_story_quest_result::ok &&
				flatfile_authority_transaction_commit_operations(
					root.string(), lock, { operation }, &error) ==
					flatfile_authority_transaction_result::ok,
			"encounter erasure did not commit");
	}
	// The cache still describes the old inode. An incremental writer must reload
	// the authority before it appends, without requiring a separate public read.
	changes third{ { { "character:1:3", "N|1|3|5468697264|1\n" } }, false };
	require(flatfile_zone_story_quest_records_save(root.string().c_str(), 7, third, &error) ==
				flatfile_zone_story_quest_result::ok &&
			flatfile_zone_story_quest_state_load(root.string().c_str(), 7, &recovered,
							     &error) ==
				flatfile_zone_story_quest_result::ok,
		"incremental write after borrowed erasure did not reload authority");
	require(recovered.find("N|1|1|") == std::string::npos &&
			recovered.find("V|1|1|") == std::string::npos &&
			recovered.find("M|1|1|") == std::string::npos &&
			recovered.find("N|1|2|") != std::string::npos &&
			recovered.find("V|1|2|") != std::string::npos &&
			recovered.find("M|1|2|") != std::string::npos &&
			recovered.find("N|1|3|") != std::string::npos,
		"borrowed erasure resurrected personal facts or lost unrelated encounters");
	require(flatfile_zone_story_quest_state_save(root.string().c_str(), 7, "ZSQF|1\nN|broken\n",
						     &error) ==
			flatfile_zone_story_quest_result::ok,
		"checksummed corrupt payload fixture did not save");
	{
		flatfile_authority_lock lock;
		require(lock.acquire(root.string(), &error),
			"corrupt payload transaction lock failed");
		flatfile_authority_operation operation;
		require(flatfile_zone_story_quest_state_prepare_player_remove(
				root.string(), lock, 7, 1, &operation, &error) ==
					flatfile_zone_story_quest_result::corrupt &&
				operation.bytes.empty(),
			"checksummed malformed payload admitted erasure");
	}
	std::filesystem::remove_all(root);
	std::cout << "zone-story flat-file state regression passed\n";
	return 0;
}
