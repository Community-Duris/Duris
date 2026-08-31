#include "flatfile/flatfile_world_quest_history.h"
#include "sql.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace fs = std::filesystem;

namespace
{
std::string state_root;

void require(bool condition, const std::string &message)
{
	if (!condition)
	{
		std::cerr << message << '\n';
		exit(1);
	}
}
} // namespace

const char *persistence_mode_flatfile_root()
{
	return state_root.c_str();
}

float get_property(const char *, double)
{
	return 2.0F;
}

void debug(const char *, ...) {}
void logit(const char *, const char *, ...) {}
void persistence_alert(int, const char *, const char *, const char *, const char *, const char *,
		       const char *, ...)
{
}

[[noreturn]] int panic_corruption_int(const char *, const char *, ...)
{
	abort();
}

int main(int argc, char **argv)
{
	require(argc == 2, "state root argument required");
	state_root = argv[1];
	const fs::path players = fs::path(state_root) / "players";
	fs::create_directories(players);
	fs::permissions(state_root, fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(players, fs::perms::owner_all, fs::perm_options::replace);

	constexpr int64_t day = 100 * 60 * 60 * 24;
	std::string error;
	int count = -1;
	require(flatfile_world_quest_count_day(state_root.c_str(), 7, 30, day + 10, &count,
					       &error) == flatfile_world_quest_result::ok &&
			count == 0,
		"missing world-quest history was not an empty history");
	require(flatfile_world_quest_record(state_root.c_str(), 7, 1001, 30, day + 20, &error) ==
			flatfile_world_quest_result::ok,
		"deterministic world-quest completion failed");
	bool completed = false;
	require(flatfile_world_quest_completed(state_root.c_str(), 7, 1001, &completed, &error) ==
				flatfile_world_quest_result::ok &&
			completed,
		"completed target was not retained");
	require(flatfile_world_quest_count_day(state_root.c_str(), 7, 30, day + 30, &count,
					       &error) == flatfile_world_quest_result::ok &&
			count == 1,
		"same-level daily completion was not counted");
	require(flatfile_world_quest_count_day(state_root.c_str(), 7, 31, day + 30, &count,
					       &error) == flatfile_world_quest_result::ok &&
			count == 0,
		"sub-level-50 daily count included another level");
	require(flatfile_world_quest_count_day(state_root.c_str(), 7, 50, day + 30, &count,
					       &error) == flatfile_world_quest_result::ok &&
			count == 1,
		"level-50 daily count did not include all completion levels");
	require(flatfile_world_quest_record(state_root.c_str(), 7, 1002, 30, day - 60 * 60 * 24,
					    &error) == flatfile_world_quest_result::ok &&
			flatfile_world_quest_count_day(state_root.c_str(), 7, 30, day + 30, &count,
						       &error) == flatfile_world_quest_result::ok &&
			count == 1,
		"prior-day completion affected the current daily allowance");

	pc_only_data pc = {};
	pc.pid = 42;
	char_data player = {};
	player.only.pc = &pc;
	player.player.name = const_cast<char *>("Tester");
	player.player.level = 30;
	require(sql_world_quest_can_do_another(&player) == 2,
		"missing SQL-compatible history did not return the configured allowance");
	pc.quest_mob_vnum = 2001;
	sql_world_quest_finished(&player, nullptr);
	require(sql_world_quest_done_already(&player, 2001) == 1 &&
			sql_world_quest_can_do_another(&player) == 1,
		"first SQL-compatible completion was not retained or counted");
	pc.quest_mob_vnum = 2002;
	sql_world_quest_finished(&player, nullptr);
	require(sql_world_quest_can_do_another(&player) == 0,
		"daily world-quest allowance was not exhausted");
	player.player.level = 31;
	require(sql_world_quest_can_do_another(&player) == 2,
		"sub-level-50 allowance did not follow current-level database semantics");
	player.player.level = 50;
	require(sql_world_quest_can_do_another(&player) == 0,
		"level-50 allowance did not count all today's completions");

	const fs::path authority = players / "42.world-quests";
	require(fs::is_regular_file(authority), "world-quest history file was not created");
	require((fs::status(authority).permissions() & fs::perms::group_all) == fs::perms::none &&
			(fs::status(authority).permissions() & fs::perms::others_all) ==
				fs::perms::none,
		"world-quest history permissions were not private");
	std::fstream corrupt(authority, std::ios::in | std::ios::out | std::ios::binary);
	require(corrupt.good(), "could not open world-quest history for corruption test");
	corrupt.seekg(-1, std::ios::end);
	char byte = 0;
	corrupt.read(&byte, 1);
	byte ^= 0x5a;
	corrupt.seekp(-1, std::ios::end);
	corrupt.write(&byte, 1);
	corrupt.close();
	require(sql_world_quest_done_already(&player, 2001) < 0 &&
			sql_world_quest_can_do_another(&player) < 0,
		"corrupt world-quest history did not fail closed");
	require(flatfile_world_quest_record(state_root.c_str(), 42, 2003, 50, day + 40, &error) ==
			flatfile_world_quest_result::corrupt,
		"completion overwrote corrupt world-quest history");

	std::cout << "flat-file world-quest history regression passed\n";
	return 0;
}
