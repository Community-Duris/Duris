#include "flatfile/flatfile_player_domain_repository.h"
#include "combat/combat_outcome_command.h"
#include "economy/currency_command.h"
#include "world/epic_command.h"

#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <openssl/sha.h>
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

static uint32_t read_u32(const std::vector<uint8_t> &bytes, size_t *offset)
{
	require(*offset + 4 <= bytes.size(), "legacy player-domain conversion overflowed");
	uint32_t value = 0;
	for (size_t byte = 0; byte < 4; ++byte)
		value |= static_cast<uint32_t>(bytes[(*offset)++]) << (byte * 8);
	return value;
}

static void write_u32(std::vector<uint8_t> *bytes, size_t offset, uint32_t value)
{
	for (size_t byte = 0; byte < 4; ++byte)
		(*bytes)[offset + byte] = static_cast<uint8_t>(value >> (byte * 8));
}

static void convert_player_domain_to_v2(const fs::path &path)
{
	std::ifstream input(path, std::ios::binary);
	std::vector<uint8_t> file((std::istreambuf_iterator<char>(input)),
				  std::istreambuf_iterator<char>());
	require(file.size() >= 56, "player domain was too short for legacy conversion");
	std::vector<uint8_t> payload(file.begin() + 56, file.end());
	size_t offset = 4;
	const uint32_t account_size = read_u32(payload, &offset);
	offset += account_size + 1 + 24 + 32 + 24;
	const uint32_t death_count = read_u32(payload, &offset);
	offset += static_cast<size_t>(death_count) * 8;
	const uint32_t zone_count = read_u32(payload, &offset);
	offset += static_cast<size_t>(zone_count) * 4;
	require(offset + 28 <= payload.size(), "player stat authority was not in v3 payload");
	payload.erase(payload.begin() + offset, payload.begin() + offset + 28);
	write_u32(&file, 8, 2);
	write_u32(&file, 12, payload.size());
	file.resize(56);
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	SHA256(payload.data(), payload.size(), digest.data());
	std::copy(digest.begin(), digest.end(), file.begin() + 24);
	file.insert(file.end(), payload.begin(), payload.end());
	std::ofstream output(path, std::ios::binary | std::ios::trunc);
	output.write(reinterpret_cast<const char *>(file.data()), file.size());
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
	record.domains.base_stat_revision = 1;
	record.domains.base_stats = { 50, 51, 52, 53, 54, 55, 56, 57, 58, 59 };
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

static critical_command currency(currency_vector wallet_delta, currency_vector bank_delta,
				 uint64_t wallet_revision, uint64_t bank_revision,
				 uint8_t operation)
{
	critical_operation_id operation_id = {};
	operation_id.bytes[0] = operation;
	currency_command_payload payload = {};
	payload.pid = 42;
	payload.racewar = 1;
	payload.reason = currency_reason_type::operator_adjustment;
	payload.reason_id = 88;
	strcpy(payload.account_name.data(), "account-one");
	payload.wallet_delta = wallet_delta;
	payload.bank_delta = bank_delta;
	critical_command command;
	require(currency_command_build(&command, operation_id, payload, wallet_revision,
				       bank_revision, critical_source_site::command,
				       critical_deadline_class::interactive),
		"could not build currency command");
	command.accepted_at_usec = 1;
	require(critical_command_normalize(&command), "could not normalize currency command");
	return command;
}

static critical_command combat(uint8_t operation, uint64_t killer_frag_revision = 0,
			       int64_t killer_frag_delta = 5)
{
	critical_operation_id operation_id = {};
	operation_id.bytes[0] = operation;
	combat_outcome_payload payload = {};
	payload.victim_pid = 43;
	payload.room_vnum = 100;
	strcpy(payload.room_name.data(), "Arena");
	payload.participant_count = 2;
	payload.participants[0].pid = 42;
	payload.participants[0].role = combat_participant_role::killer;
	payload.participants[0].level = 50;
	payload.participants[0].racewar = 1;
	payload.participants[0].frag_delta = killer_frag_delta;
	payload.participants[0].epic_delta = 2;
	payload.participants[0].wallet_delta_copper = 11;
	payload.participants[0].expected_frag_revision = killer_frag_revision;
	payload.participants[0].expected_epic_revision = 1;
	payload.participants[0].expected_wallet_revision = 2;
	payload.participants[0].expected_bank_revision = 3;
	strcpy(payload.participants[0].account_name.data(), "account-one");
	strcpy(payload.participants[0].description.data(), "killer");
	payload.participants[1].pid = 43;
	payload.participants[1].role = combat_participant_role::victim;
	payload.participants[1].level = 45;
	payload.participants[1].racewar = 1;
	payload.participants[1].frag_delta = -2;
	payload.participants[1].epic_delta = 3;
	payload.participants[1].wallet_delta_copper = 7;
	payload.participants[1].expected_frag_revision = 0;
	payload.participants[1].expected_epic_revision = 0;
	payload.participants[1].expected_wallet_revision = 0;
	payload.participants[1].expected_bank_revision = 3;
	strcpy(payload.participants[1].account_name.data(), "account-one");
	strcpy(payload.participants[1].description.data(), "victim");
	critical_command command;
	require(combat_outcome_command_build(&command, operation_id, payload),
		"could not build combat outcome command");
	command.accepted_at_usec = 1;
	require(critical_command_normalize(&command), "could not normalize combat command");
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
			loaded.domains.frags == 10 && loaded.domains.base_stat_revision == 1 &&
			loaded.domains.base_stats == source.domains.base_stats &&
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

	critical_command currency_move =
		currency({ { 10, 0, 0, 0 } }, { { -2, 0, 0, 0 } }, 0, 1, 10);
	applied = flatfile_player_domain_apply(root.string(), currency_move);
	currency_command_result currency_result = {};
	require(applied.outcome == critical_apply_outcome::applied &&
			currency_command_decode_result(applied.result_payload.data(),
						       applied.result_size, &currency_result) &&
			currency_result.wallet.amount[0] == 11 &&
			currency_result.bank.amount[0] == 3 &&
			currency_result.wallet_revision == 1 && currency_result.bank_revision == 2,
		"wallet/shared-bank transaction did not apply");
	require(flatfile_player_domain_load(root.string(), 42, "account-one", 1, &loaded, &error) ==
				flatfile_player_domain_result::ok &&
			loaded.domains.wallet[0] == 11 && loaded.domains.bank[0] == 3 &&
			loaded.domains.wallet_revision == 1 && loaded.domains.bank_revision == 2,
		"wallet/shared-bank result did not load");
	applied = flatfile_player_domain_apply(root.string(), currency_move);
	require(applied.outcome == critical_apply_outcome::already_applied &&
			currency_command_decode_result(applied.result_payload.data(),
						       applied.result_size, &currency_result) &&
			currency_result.wallet.amount[0] == 11 &&
			currency_result.bank.amount[0] == 3,
		"currency replay did not return its original result");
	require(flatfile_player_domain_apply(root.string(), currency({ { 1, 0, 0, 0 } },
								     { { 0, 0, 0, 0 } }, 1, 2, 10))
				.error_code == EEXIST,
		"conflicting currency operation ID was accepted");
	critical_command stale_currency =
		currency({ { 1, 0, 0, 0 } }, { { 0, 0, 0, 0 } }, 0, 2, 11);
	require(flatfile_player_domain_apply(root.string(), stale_currency).error_code == ESTALE &&
			flatfile_player_domain_apply(root.string(), stale_currency).error_code ==
				ESTALE,
		"stale currency decision was not durably replayed");
	critical_command insufficient_currency =
		currency({ { 0, 0, 0, 0 } }, { { -99, 0, 0, 0 } }, 1, 2, 12);
	require(flatfile_player_domain_apply(root.string(), insufficient_currency).error_code ==
			ENOSPC,
		"insufficient bank funds were accepted");
	critical_command interrupted_currency =
		currency({ { 1, 0, 0, 0 } }, { { 1, 0, 0, 0 } }, 1, 2, 13);
	setenv("DURIS_FLATFILE_TEST_LEGACY_TRANSACTION", "1", 1);
	setenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_BANK", "1", 1);
	applied = flatfile_player_domain_apply(root.string(), interrupted_currency);
	unsetenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_BANK");
	unsetenv("DURIS_FLATFILE_TEST_LEGACY_TRANSACTION");
	require(applied.outcome == critical_apply_outcome::retryable_failure &&
			fs::exists(domains / ".currency-transaction"),
		"currency interruption did not preserve its legacy durable intent");
	require(flatfile_player_domain_load(root.string(), 42, "account-one", 1, &loaded, &error) ==
				flatfile_player_domain_result::ok &&
			loaded.domains.wallet[0] == 12 && loaded.domains.bank[0] == 4 &&
			loaded.domains.wallet_revision == 2 && loaded.domains.bank_revision == 3 &&
			!fs::exists(domains / ".currency-transaction"),
		"domain load did not recover the legacy currency transaction");
	applied = flatfile_player_domain_apply(root.string(), interrupted_currency);
	require(applied.outcome == critical_apply_outcome::already_applied,
		"recovered currency transaction did not replay from its operation ledger");

	critical_command combat_outcome = combat(20);
	setenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_BANK", "1", 1);
	applied = flatfile_player_domain_apply(root.string(), combat_outcome);
	unsetenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_BANK");
	require(applied.outcome == critical_apply_outcome::retryable_failure &&
			fs::exists(domains / ".player-domain-transaction"),
		"combat interruption did not preserve its multi-record intent");
	require(flatfile_player_domain_load(root.string(), 43, "account-one", 1, &loaded, &error) ==
				flatfile_player_domain_result::ok &&
			loaded.domains.frags == 8 && loaded.domains.old_frags == 10 &&
			loaded.domains.frag_revision == 1 && loaded.domains.epics == 12 &&
			loaded.domains.epic_revision == 1 &&
			loaded.domains.wallet == std::array<uint64_t, 4>{ 8, 2, 3, 4 } &&
			loaded.domains.wallet_revision == 1 && loaded.domains.bank_revision == 5 &&
			!fs::exists(domains / ".player-domain-transaction"),
		"domain load did not recover every combat after-image");
	require(flatfile_player_domain_load(root.string(), 42, "account-one", 1, &loaded, &error) ==
				flatfile_player_domain_result::ok &&
			loaded.domains.frags == 15 && loaded.domains.old_frags == 10 &&
			loaded.domains.frag_revision == 1 && loaded.domains.epics == 16 &&
			loaded.domains.epic_revision == 2 &&
			loaded.domains.wallet == std::array<uint64_t, 4>{ 3, 4, 3, 4 } &&
			loaded.domains.wallet_revision == 3 && loaded.domains.bank_revision == 5,
		"recovered combat state was incomplete");
	applied = flatfile_player_domain_apply(root.string(), combat_outcome);
	combat_outcome_result combat_result = {};
	require(applied.outcome == critical_apply_outcome::already_applied &&
			combat_outcome_command_decode_result(applied.result_payload.data(),
							     applied.result_size, &combat_result) &&
			combat_result.participant_count == 2 &&
			combat_result.participants[0].frags == 15 &&
			combat_result.participants[1].frags == 8 && combat_result.event_id != 0,
		"recovered combat command did not replay its original result");
	require(flatfile_player_domain_apply(root.string(), combat(20, 1, 6)).error_code == EEXIST,
		"conflicting combat operation ID was accepted");
	critical_command stale_combat = combat(21);
	require(flatfile_player_domain_apply(root.string(), stale_combat).error_code == ESTALE &&
			flatfile_player_domain_apply(root.string(), stale_combat).error_code ==
				ESTALE,
		"stale combat decision was not durably replayed");

	convert_player_domain_to_v2(domains / "player-45.domain");
	require(flatfile_player_domain_load(root.string(), 45, "account-one", 1, &loaded, &error) ==
				flatfile_player_domain_result::ok &&
			loaded.domains.base_stat_revision == 0 &&
			loaded.domains.base_stats == std::array<int16_t, 10>{},
		"legacy v2 player domain did not remain readable and snapshot-owned");

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
