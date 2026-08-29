#include "flatfile_artifact_repository.h"
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
	const fs::path root = state_root;
	fs::create_directories(root / "domains");
	fs::create_directories(root / "players");
	fs::create_directories(root / "identities/names");
	fs::permissions(root, fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(root / "domains", fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(root / "players", fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(root / "identities", fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(root / "identities/names", fs::perms::owner_all, fs::perm_options::replace);

	int owner_pid = 99;
	int timer = 99;
	require(!sql_get_bind_data(600, &owner_pid, &timer) && owner_pid == 0 && timer == 0,
		"missing flat artifact authority did not fail closed");
	std::string error;
	const flatfile_artifact_record artifact = {
		600, true, FLATFILE_ARTIFACT_ON_PLAYER, 42, 5000, 1, 1000, 42, 4000, 1
	};
	require(flatfile_artifact_establish(state_root, { artifact }, &error) ==
			flatfile_artifact_result::ok,
		"could not establish runtime artifact authority: " + error);
	require(sql_get_bind_data(600, &owner_pid, &timer) && owner_pid == 42 && timer == 4000,
		"SQL-compatible bind lookup did not read flat authority");
	owner_pid = 77;
	timer = 4100;
	sql_update_bind_data(600, &owner_pid, &timer);
	require(sql_get_bind_data(600, &owner_pid, &timer) && owner_pid == 77 && timer == 4100,
		"SQL-compatible bind update did not persist flat authority");
	require(!sql_get_bind_data(601, &owner_pid, &timer) && owner_pid == 0 && timer == 0,
		"SQL-compatible bind lookup synthesized a missing artifact");

	const fs::path authority = root / "domains/artifact_catalog";
	std::fstream corrupt(authority, std::ios::in | std::ios::out | std::ios::binary);
	require(corrupt.good(), "could not open artifact authority for corruption test");
	corrupt.seekg(-1, std::ios::end);
	char byte = 0;
	corrupt.read(&byte, 1);
	byte ^= 0x5a;
	corrupt.seekp(-1, std::ios::end);
	corrupt.write(&byte, 1);
	corrupt.close();
	owner_pid = 88;
	timer = 88;
	require(!sql_get_bind_data(600, &owner_pid, &timer) && owner_pid == 0 && timer == 0,
		"SQL-compatible bind lookup exposed corrupt authority");
	owner_pid = 12;
	timer = 4200;
	sql_update_bind_data(600, &owner_pid, &timer);
	int32_t stored_owner_pid = 0;
	int64_t stored_timer = 0;
	require(flatfile_artifact_bind_get(state_root, 600, &stored_owner_pid, &stored_timer,
					   &error) == flatfile_artifact_result::invalid,
		"SQL-compatible bind update overwrote corrupt authority");

	std::cout << "flat-file artifact bind runtime passed\n";
	return 0;
}
