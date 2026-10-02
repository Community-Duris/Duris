#include "flatfile/flatfile_zone_story_quest_state.h"

#include <cstdlib>
#include <vector>
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
	std::filesystem::remove_all(root);
	std::cout << "zone-story flat-file state regression passed\n";
	return 0;
}
