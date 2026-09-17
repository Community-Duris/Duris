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
	require(flatfile_zone_story_quest_state_load(root.string().c_str(), 7, &recovered, &error) ==
			flatfile_zone_story_quest_result::ok && recovered == state,
		"flat-file state did not round-trip");
	require(flatfile_zone_story_quest_state_load(root.string().c_str(), 8, &recovered, &error) ==
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
	require(flatfile_zone_story_quest_state_load(root.string().c_str(), 7, &recovered, &error) ==
			flatfile_zone_story_quest_result::corrupt,
		"flat-file checksum corruption was not detected");
	std::filesystem::remove_all(root);
	std::cout << "zone-story flat-file state regression passed\n";
	return 0;
}
