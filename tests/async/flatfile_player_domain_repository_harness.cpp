#include "flatfile_player_domain_repository.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace fs = std::filesystem;

static void require(bool condition, const std::string &message)
{
	if (!condition)
	{
		std::cerr << message << '\n';
		exit(1);
	}
}

static flatfile_player_domain_record baseline(int32_t pid)
{
	flatfile_player_domain_record record;
	record.pid = pid;
	record.account_name = "Account-One";
	record.racewar = 1;
	record.domains.wallet = { 1, 2, 3, 4 };
	record.domains.bank = { 5, 6, 7, 8 };
	record.domains.epics = 9;
	record.domains.frags = 10;
	record.domains.old_frags = 11;
	record.recent_pvp_deaths = { 3000, 2000, 1000 };
	record.completed_epic_zones = { 7, 12, 99 };
	return record;
}

int main(int argc, char **argv)
{
	require(argc == 2, "state root argument required");
	const fs::path root = argv[1];
	const fs::path domains = root / "domains";
	fs::create_directories(domains);
	fs::permissions(root, fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(domains, fs::perms::owner_all, fs::perm_options::replace);

	std::string error;
	flatfile_player_domain_record source = baseline(42);
	require(flatfile_player_domain_establish(root.string(), source, &error) ==
			flatfile_player_domain_result::ok,
		"domain baseline failed: " + error);
	flatfile_player_domain_record loaded;
	require(flatfile_player_domain_load(root.string(), 42, "account-one", 1, &loaded, &error) ==
				flatfile_player_domain_result::ok &&
			loaded.account_name == "account-one" &&
			loaded.domains.wallet == source.domains.wallet &&
			loaded.domains.bank == source.domains.bank &&
			loaded.domains.bank_revision == 1 && loaded.domains.epics == 9 &&
			loaded.domains.frags == 10 &&
			loaded.recent_pvp_deaths == source.recent_pvp_deaths &&
			loaded.completed_epic_zones == source.completed_epic_zones,
		"domain baseline did not round trip: " + error);
	require(flatfile_player_domain_establish(root.string(), source, &error) ==
			flatfile_player_domain_result::ok,
		"exact domain baseline retry was not idempotent");
	flatfile_player_domain_record bank_retry_conflict = source;
	bank_retry_conflict.domains.bank[0] = 99;
	require(flatfile_player_domain_establish(root.string(), bank_retry_conflict, &error) ==
			flatfile_player_domain_result::conflict,
		"conflicting bank on player baseline retry was accepted");
	source.domains.epics = 10;
	require(flatfile_player_domain_establish(root.string(), source, &error) ==
			flatfile_player_domain_result::conflict,
		"conflicting player domain baseline was accepted");

	flatfile_player_domain_record sibling = baseline(43);
	require(flatfile_player_domain_establish(root.string(), sibling, &error) ==
			flatfile_player_domain_result::ok,
		"second character could not share the account bank");
	flatfile_player_domain_record creation = baseline(45);
	creation.domains.bank = {};
	require(flatfile_player_domain_establish_initial_player(root.string(), creation, &error) ==
			flatfile_player_domain_result::ok,
		"new character could not reuse an existing authoritative account bank");
	require(flatfile_player_domain_load(root.string(), 45, "account-one", 1, &loaded, &error) ==
				flatfile_player_domain_result::ok &&
			loaded.domains.bank == sibling.domains.bank,
		"new character did not load the existing authoritative account bank");
	flatfile_player_domain_record conflict = baseline(44);
	conflict.domains.bank[0] = 99;
	require(flatfile_player_domain_establish(root.string(), conflict, &error) ==
			flatfile_player_domain_result::conflict,
		"conflicting shared bank baseline was accepted");
	require(flatfile_player_domain_load(root.string(), 42, "wrong-account", 1, &loaded,
					    &error) == flatfile_player_domain_result::conflict,
		"account mismatch was accepted during domain load");

	const fs::path player = domains / "player-42.domain";
	{
		std::fstream file(player, std::ios::in | std::ios::out | std::ios::binary);
		require(file.good(), "could not open player domain for corruption test");
		file.seekg(-1, std::ios::end);
		char value = 0;
		file.read(&value, 1);
		value ^= 0x44;
		file.seekp(-1, std::ios::end);
		file.write(&value, 1);
	}
	require(flatfile_player_domain_load(root.string(), 42, "account-one", 1, &loaded, &error) ==
			flatfile_player_domain_result::invalid,
		"corrupt player-domain checksum was accepted");
	require(flatfile_player_domain_establish(root.string(), baseline(42), &error) ==
			flatfile_player_domain_result::invalid,
		"corrupt player domain was overwritten");
	for (const fs::directory_entry &entry : fs::directory_iterator(domains))
		require(entry.path().filename().string().find(".tmp.") == std::string::npos,
			"temporary domain file was left behind");

	std::cout << "flat-file player domain repository passed\n";
	return 0;
}
