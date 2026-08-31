#include "core/structs.h"
#include "account/multiplay_whitelist.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <fnmatch.h>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

bool add_to_whitelist(P_char ch, const char *player, const char *pattern, const char *description);
bool remove_from_whitelist(P_char ch, const char *pattern);

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

P_desc descriptor_list = nullptr;

const char *persistence_mode_flatfile_root()
{
	return state_root.c_str();
}

bool match_pattern(const char *pattern, const char *value)
{
	return pattern && value && fnmatch(pattern, value, 0) == 0;
}

void logit(const char *, const char *, ...) {}
void sql_log(P_char, const char *, const char *, ...) {}

int main(int argc, char **argv)
{
	require(argc == 2, "state root argument required");
	state_root = argv[1];
	const fs::path metadata = fs::path(state_root) / "metadata";
	fs::create_directories(metadata);
	fs::permissions(state_root, fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(metadata, fs::perms::owner_all, fs::perm_options::replace);

	char_data admin = {};
	admin.player.name = const_cast<char *>("Admin");
	require(get_whitelist().empty(), "missing whitelist was not an empty authority");
	require(!whitelisted_host("127.0.0.1"), "missing whitelist allowed a host");
	require(add_to_whitelist(&admin, "Player", "127.0.*.*", "  shared household  "),
		"first whitelist add failed");
	require(add_to_whitelist(&admin, "Other", "10.*.*.*", "staff approval"),
		"second whitelist add failed");
	require(add_to_whitelist(&admin, "Sibling", "127.0.*.*", "second approval"),
		"duplicate-pattern whitelist add failed");

	std::vector<whitelist_data> entries = get_whitelist();
	require(entries.size() == 3 && entries[0].id == 1 && entries[1].id == 2 &&
			entries[2].id == 3 && entries[0].created_on.size() == 10 &&
			entries[0].admin == "Admin" && entries[0].player == "Player" &&
			entries[0].description == "shared household",
		"flat whitelist did not preserve database-visible fields");
	require(whitelisted_host("127.0.8.9") && whitelisted_host("10.2.3.4") &&
			!whitelisted_host("192.168.1.1"),
		"flat whitelist host matching failed");
	require(remove_from_whitelist(&admin, " 127.0.*.* "), "whitelist pattern removal failed");
	entries = get_whitelist();
	require(entries.size() == 1 && entries[0].pattern == "10.*.*.*" &&
			!whitelisted_host("127.0.8.9") && whitelisted_host("10.2.3.4"),
		"whitelist removal did not delete every exact pattern row");
	require(remove_from_whitelist(&admin, "172.16.*.*"),
		"missing pattern did not preserve database DELETE semantics");

	const fs::path record = metadata / "multiplay_whitelist";
	require(fs::is_regular_file(record), "whitelist record was not created");
	require((fs::status(record).permissions() & fs::perms::group_all) == fs::perms::none &&
			(fs::status(record).permissions() & fs::perms::others_all) ==
				fs::perms::none,
		"whitelist record permissions were not private");
	std::vector<char> original(fs::file_size(record));
	{
		std::ifstream input(record, std::ios::binary);
		input.read(original.data(), static_cast<std::streamsize>(original.size()));
		require(input.good(), "whitelist record could not be captured");
	}
	{
		std::fstream file(record, std::ios::in | std::ios::out | std::ios::binary);
		require(file.good(), "whitelist record could not be opened for corruption");
		file.seekg(-1, std::ios::end);
		char byte = 0;
		file.read(&byte, 1);
		byte ^= 0x5a;
		file.seekp(-1, std::ios::end);
		file.write(&byte, 1);
	}
	require(get_whitelist().empty() && !whitelisted_host("10.2.3.4"),
		"corrupt whitelist granted an exception");
	require(!add_to_whitelist(&admin, "Third", "192.168.*.*", "must not overwrite"),
		"corrupt whitelist was overwritten by an add");
	original.back() ^= 0x5a;
	std::vector<char> after_failed_add(original.size());
	{
		std::ifstream input(record, std::ios::binary);
		input.read(after_failed_add.data(),
			   static_cast<std::streamsize>(after_failed_add.size()));
		require(input.good(), "whitelist record could not be checked after failed add");
	}
	require(after_failed_add == original, "failed add changed the corrupt authority record");

	std::cout << "flat-file multiplay whitelist passed\n";
	return 0;
}
