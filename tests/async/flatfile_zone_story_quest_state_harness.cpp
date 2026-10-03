#include "flatfile/flatfile_zone_story_quest_state.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

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
	const std::string aliases = "ZSQF|1\nN|1|1|506c61796572|1\nN|7|1|4f6c64416c696173|1\n"
				    "N|1|2|5365636f6e64|1\n";
	require(flatfile_zone_story_quest_state_save(root.string().c_str(), 7, aliases, &error) ==
			flatfile_zone_story_quest_result::ok,
		"alias fixture did not save");
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
		require(raw.substr(56) == aliases, "alias preparation changed native authority");
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
