#include "flatfile_player_domain_repository.h"
#include "epic_command.h"

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

static critical_command epic(int64_t delta, uint64_t expected_revision, uint8_t operation,
			     uint16_t flags = 0)
{
	critical_operation_id operation_id = {};
	operation_id.bytes[0] = operation;
	epic_command_payload payload = { 42, delta, epic_reason_type::quest_award, flags, 77 };
	critical_command command;
	require(epic_command_build(&command, operation_id, payload, expected_revision,
				   critical_source_site::command,
				   critical_deadline_class::interactive),
		"could not build epic command");
	command.accepted_at_usec = 1;
	require(critical_command_normalize(&command), "could not normalize epic command");
	return command;
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

	critical_command epic_gain = epic(5, 0, 1);
	critical_apply_result applied = flatfile_player_domain_apply(root.string(), epic_gain);
	epic_command_result epic_result = {};
	require(applied.outcome == critical_apply_outcome::applied && applied.error_code == 0 &&
			epic_command_decode_result(applied.result_payload.data(),
						   applied.result_size, &epic_result) &&
			epic_result.balance == 14 && epic_result.revision == 1,
		"epic gain did not apply");
	require(flatfile_player_domain_load(root.string(), 42, "account-one", 1, &loaded, &error) ==
				flatfile_player_domain_result::ok &&
			loaded.domains.epics == 14 && loaded.domains.epic_revision == 1,
		"epic authority did not load after mutation");
	applied = flatfile_player_domain_apply(root.string(), epic_gain);
	require(applied.outcome == critical_apply_outcome::already_applied &&
			epic_command_decode_result(applied.result_payload.data(),
						   applied.result_size, &epic_result) &&
			epic_result.balance == 14 && epic_result.revision == 1,
		"epic command replay did not return its original result");
	require(flatfile_player_domain_apply(root.string(), epic(6, 1, 1)).error_code == EEXIST,
		"conflicting epic operation ID was accepted");
	critical_command stale = epic(2, 0, 2);
	applied = flatfile_player_domain_apply(root.string(), stale);
	require(applied.outcome == critical_apply_outcome::terminal_failure &&
			applied.error_code == ESTALE,
		"stale epic revision was accepted");
	require(flatfile_player_domain_apply(root.string(), stale).error_code == ESTALE,
		"stale epic result was not replayed");
	applied = flatfile_player_domain_apply(root.string(),
					       epic(-99, 1, 3, EPIC_COMMAND_REQUIRE_FUNDS));
	require(applied.outcome == critical_apply_outcome::terminal_failure &&
			applied.error_code == ENOSPC,
		"insufficient epic funds were accepted");

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
