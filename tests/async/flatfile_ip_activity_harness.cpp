#include "flatfile/flatfile_ip_activity_repository.h"
#include "sql/sql.h"

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

void debug(const char *, ...) {}
void logit(const char *, const char *, ...) {}

[[noreturn]] int panic_corruption_int(const char *, const char *, ...)
{
	abort();
}

int main(int argc, char **argv)
{
	require(argc == 2, "state root argument required");
	state_root = argv[1];
	const fs::path metadata = fs::path(state_root) / "metadata";
	fs::create_directories(metadata);
	fs::permissions(state_root, fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(metadata, fs::perms::owner_all, fs::perm_options::replace);

	std::string error;
	flatfile_ip_activity_record record;
	require(flatfile_ip_activity_get(state_root.c_str(), 1, &record, &error) ==
			flatfile_ip_activity_result::not_found,
		"missing IP activity was not reported as missing");
	require(flatfile_ip_activity_connect(state_root.c_str(), 1, "198.51.100.8", 1, 1000,
					     &error) == flatfile_ip_activity_result::ok,
		"first deterministic connection failed");
	require(flatfile_ip_activity_disconnect(state_root.c_str(), 1, 1, 1100, &error) ==
			flatfile_ip_activity_result::ok,
		"deterministic disconnect failed");
	require(flatfile_ip_activity_connect(state_root.c_str(), 2, "198.51.100.8", 2, 1200,
					     &error) == flatfile_ip_activity_result::ok,
		"second deterministic connection failed");
	require(flatfile_ip_activity_find_latest(state_root.c_str(), "198.51.100.8", &record,
						 &error) == flatfile_ip_activity_result::ok &&
			record.pid == 2 && record.racewar_side == 2 &&
			record.last_connect == 1200 && record.last_disconnect == 0,
		"latest connection for a shared IP was not selected");
	require(flatfile_ip_activity_reset_active(state_root.c_str(), 1300, &error) ==
				flatfile_ip_activity_result::ok &&
			flatfile_ip_activity_get(state_root.c_str(), 2, &record, &error) ==
				flatfile_ip_activity_result::ok &&
			record.last_disconnect == 1300,
		"boot reset did not close an interrupted active session");
	require(flatfile_ip_activity_get(state_root.c_str(), 1, &record, &error) ==
				flatfile_ip_activity_result::ok &&
			record.last_disconnect == 1100,
		"boot reset modified an already closed session");

	pc_only_data pc = {};
	pc.pid = 42;
	descriptor_data descriptor = {};
	snprintf(descriptor.host, sizeof(descriptor.host), "203.0.113.9");
	char_data player = {};
	player.only.pc = &pc;
	player.desc = &descriptor;
	player.player.name = const_cast<char *>("Tester");
	player.player.level = 50;
	player.player.racewar = 3;

	sql_connectIP(&player);
	char ip[64];
	time_t last_connect = 0, last_disconnect = 0;
	require(std::string(sql_select_IP_info(&player, ip, sizeof(ip), &last_connect,
					       &last_disconnect)) == "203.0.113.9" &&
			last_connect <= 2 && last_disconnect == 0,
		"SQL compatibility reader did not expose flat connection data");
	int racewar_side = RACEWAR_NONE;
	require(sql_find_racewar_for_ip(descriptor.host, &racewar_side) == 3600 &&
			racewar_side == 3,
		"active connection did not enforce the one-hour racewar rule");
	sql_disconnectIP(&player);
	const int remaining = sql_find_racewar_for_ip(descriptor.host, &racewar_side);
	require(remaining >= 3598 && remaining <= 3600 && racewar_side == 3,
		"recent disconnect did not enforce the remaining one-hour delay");
	require(std::string(sql_select_IP_info(&player, ip, sizeof(ip), &last_connect,
					       &last_disconnect)) == "203.0.113.9" &&
			last_disconnect <= 2,
		"SQL compatibility reader did not expose flat disconnect data");

	const fs::path authority = metadata / "ip_activity";
	require(fs::is_regular_file(authority), "IP activity authority was not created");
	require((fs::status(authority).permissions() & fs::perms::group_all) == fs::perms::none &&
			(fs::status(authority).permissions() & fs::perms::others_all) ==
				fs::perms::none,
		"IP activity authority permissions were not private");
	std::fstream corrupt(authority, std::ios::in | std::ios::out | std::ios::binary);
	require(corrupt.good(), "could not open IP authority for corruption test");
	corrupt.seekg(-1, std::ios::end);
	char byte = 0;
	corrupt.read(&byte, 1);
	byte ^= 0x5a;
	corrupt.seekp(-1, std::ios::end);
	corrupt.write(&byte, 1);
	corrupt.close();
	require(sql_find_racewar_for_ip(descriptor.host, &racewar_side) < 0 &&
			racewar_side == RACEWAR_NONE,
		"corrupt IP activity did not fail closed");
	require(flatfile_ip_activity_connect(state_root.c_str(), 43, "203.0.113.10", 4, 1400,
					     &error) == flatfile_ip_activity_result::corrupt,
		"mutation overwrote a corrupt IP authority");

	std::cout << "flat-file IP activity regression passed\n";
	return 0;
}
