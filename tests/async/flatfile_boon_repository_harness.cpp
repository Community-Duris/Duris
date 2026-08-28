#include "boon.h"
#include "flatfile_boon_repository.h"
#include "flatfile_player_domain_repository.h"

#include <cerrno>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <openssl/sha.h>
#include <string>
#include <vector>

namespace fs = std::filesystem;

static void require(bool condition, const std::string &message)
{
	if (!condition)
	{
		std::cerr << message << '\n';
		exit(1);
	}
}

static critical_operation_id operation(uint8_t value)
{
	critical_operation_id id = {};
	id.bytes.front() = 0xb6;
	id.bytes.back() = value;
	return id;
}

static critical_command command(const boon_reward_payload &payload, uint8_t operation_value)
{
	critical_command value = {};
	require(boon_reward_command_build(&value, operation(operation_value), payload),
		"could not build boon command");
	value.accepted_at_usec = operation_value;
	require(critical_command_normalize(&value), "could not normalize boon command");
	return value;
}

static critical_command shop_command(uint32_t pid, uint8_t stat_index, uint8_t operation_value)
{
	critical_command value = {};
	require(boon_shop_command_build(&value, operation(operation_value), { pid, stat_index }),
		"could not build boon shop command");
	value.accepted_at_usec = operation_value;
	require(critical_command_normalize(&value), "could not normalize boon shop command");
	return value;
}

static boon_shop_result decode_shop_result(const critical_apply_result &applied)
{
	boon_shop_result result = {};
	require(boon_shop_command_decode_result(applied.result_payload.data(), applied.result_size,
						&result),
		"could not decode boon shop result");
	return result;
}

static flatfile_player_domain_record player(uint32_t pid)
{
	flatfile_player_domain_record record;
	record.pid = pid;
	record.account_name = "account-one";
	record.racewar = 1;
	record.domains.base_stat_revision = 1;
	record.domains.base_stats = { 50, 51, 52, 53, 54, 55, 56, 57, 58, 59 };
	return record;
}

static boon_reward_result decode_applied_result(const critical_apply_result &applied)
{
	boon_reward_result result = {};
	require(boon_reward_command_decode_result(applied.result_payload.data(),
						  applied.result_size, &result),
		"could not decode boon result");
	return result;
}

static flatfile_boon_definition definition(uint32_t id, uint8_t type, uint8_t option,
					   double criteria, double criteria2, double bonus,
					   bool repeat = false)
{
	flatfile_boon_definition value;
	value.id = id;
	value.type = type;
	value.option = option;
	value.criteria = criteria;
	value.criteria2 = criteria2;
	value.bonus = bonus;
	value.active = true;
	value.repeat = repeat;
	value.author = "test";
	return value;
}

static void expect_pending_reward(const std::string &root, uint32_t pid,
				  const critical_operation_id &operation_id, double event_data,
				  std::string *error)
{
	flatfile_boon_pending_reward reward;
	require(flatfile_boon_find_pending_reward(root, pid, &reward, error) ==
				flatfile_boon_result::ok &&
			critical_operation_id_equal(reward.operation_id, operation_id) &&
			reward.event_data == event_data && reward.result.pid == pid,
		"committed boon result was not durably pending");
	require(flatfile_boon_acknowledge_reward(root, operation_id, error) ==
				flatfile_boon_result::ok &&
			flatfile_boon_acknowledge_reward(root, operation_id, error) ==
				flatfile_boon_result::ok &&
			flatfile_boon_find_pending_reward(root, pid, &reward, error) ==
				flatfile_boon_result::not_found,
		"boon reward acknowledgement was not durable and idempotent");
}

static uint32_t read_u32(const std::vector<uint8_t> &bytes, size_t *offset)
{
	require(*offset + 4 <= bytes.size(), "legacy boon conversion exceeded payload");
	uint32_t value = 0;
	for (size_t index = 0; index < 4; ++index)
		value |= static_cast<uint32_t>(bytes[(*offset)++]) << (index * 8);
	return value;
}

static void write_u32(std::vector<uint8_t> *bytes, size_t offset, uint32_t value)
{
	for (size_t index = 0; index < 4; ++index)
		(*bytes)[offset + index] = static_cast<uint8_t>(value >> (index * 8));
}

static void convert_catalog_to_legacy_v1(const fs::path &path)
{
	std::ifstream input(path, std::ios::binary);
	std::vector<uint8_t> file((std::istreambuf_iterator<char>(input)),
				  std::istreambuf_iterator<char>());
	require(file.size() >= 56, "boon catalog too short for legacy conversion");
	std::vector<uint8_t> payload(file.begin() + 56, file.end());
	size_t offset = 0;
	const uint32_t definition_count = read_u32(payload, &offset);
	for (uint32_t definition = 0; definition < definition_count; ++definition)
	{
		offset += 62;
		require(offset + 2 <= payload.size(), "legacy boon conversion missed author");
		const uint16_t author_size = static_cast<uint16_t>(payload[offset]) |
					     static_cast<uint16_t>(payload[offset + 1]) << 8;
		offset += 2 + author_size;
	}
	offset += static_cast<size_t>(read_u32(payload, &offset)) * 16;
	offset += static_cast<size_t>(read_u32(payload, &offset)) * 20;
	const size_t operations_header = offset;
	const uint32_t operation_count = read_u32(payload, &offset);
	constexpr size_t version_two_operation_size = 16 + 32 + 4 + BOON_REWARD_RESULT_BYTES + 9;
	constexpr size_t version_one_operation_size = version_two_operation_size - 9;
	require(offset + static_cast<size_t>(operation_count) * version_two_operation_size + 4 ==
			payload.size(),
		"legacy boon conversion did not locate operations");
	require(payload[payload.size() - 4] == 0 && payload[payload.size() - 3] == 0 &&
			payload[payload.size() - 2] == 0 && payload[payload.size() - 1] == 0,
		"legacy boon conversion found shop operations");
	std::vector<uint8_t> legacy(payload.begin(), payload.begin() + operations_header + 4);
	for (uint32_t operation = 0; operation < operation_count; ++operation)
	{
		legacy.insert(legacy.end(), payload.begin() + offset,
			      payload.begin() + offset + version_one_operation_size);
		offset += version_two_operation_size;
	}
	write_u32(&file, 8, 1);
	write_u32(&file, 12, legacy.size());
	file.resize(56);
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	SHA256(legacy.data(), legacy.size(), digest.data());
	std::copy(digest.begin(), digest.end(), file.begin() + 24);
	file.insert(file.end(), legacy.begin(), legacy.end());
	std::ofstream output(path, std::ios::binary | std::ios::trunc);
	output.write(reinterpret_cast<const char *>(file.data()), file.size());
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
	std::vector<flatfile_boon_definition> definitions = {
		definition(1, BTYPE_POINT, BOPT_MOB, 2, -1, 5),
		definition(2, BTYPE_STATS, BOPT_MOB, 1, -1, 2, true),
		definition(3, BTYPE_CASH, BOPT_ZONE, 77, 0, 100),
		definition(4, BTYPE_EXP, BOPT_FRAG, 10, 0, 1000),
	};
	require(flatfile_boon_establish(root.string(), definitions, &error) ==
				flatfile_boon_result::ok &&
			flatfile_boon_establish(root.string(), definitions, &error) ==
				flatfile_boon_result::already_exists,
		"could not establish boon catalog: " + error);
	require(flatfile_player_domain_establish(root.string(), player(42), &error) ==
				flatfile_player_domain_result::ok &&
			flatfile_player_domain_establish(root.string(), player(43), &error) ==
				flatfile_player_domain_result::ok,
		"could not establish player stat authority: " + error);
	std::vector<flatfile_boon_definition> loaded_definitions;
	require(flatfile_boon_load_definitions(root.string(), &loaded_definitions, &error) ==
				flatfile_boon_result::ok &&
			loaded_definitions.size() == definitions.size() &&
			loaded_definitions[0].id == definitions[0].id &&
			loaded_definitions[0].author == definitions[0].author &&
			loaded_definitions[3].bonus == definitions[3].bonus,
		"boon definition projection did not preserve the catalog");
	flatfile_boon_definition created = definition(0, BTYPE_EXP, BOPT_LEVEL, 60, 0, 500);
	created.duration = -1;
	created.target_pid = 42;
	require(flatfile_boon_create(root.string(), &created, &error) == flatfile_boon_result::ok &&
			created.id == 5 &&
			flatfile_boon_create(root.string(), &created, &error) ==
				flatfile_boon_result::invalid,
		"boon definition creation was not bounded or monotonic");
	require(flatfile_boon_extend(root.string(), created.id, 10, 100, "ops", nullptr, &error) ==
			flatfile_boon_result::invalid,
		"boon extension accepted a missing active-state output");
	require(flatfile_boon_deactivate(root.string(), 3, &error) == flatfile_boon_result::ok &&
			flatfile_boon_deactivate(root.string(), 3, &error) ==
				flatfile_boon_result::ok,
		"boon deactivation was not durable and idempotent");
	bool was_active = true;
	require(flatfile_boon_extend(root.string(), created.id, 10, 100, "ops", &was_active,
				     &error) == flatfile_boon_result::invalid,
		"boon extension changed a forever definition");
	require(flatfile_boon_extend(root.string(), 3, 10, 100, "ops", &was_active, &error) ==
				flatfile_boon_result::ok &&
			!was_active &&
			flatfile_boon_load_definitions(root.string(), &loaded_definitions,
						       &error) == flatfile_boon_result::ok &&
			loaded_definitions[2].active && loaded_definitions[2].start_time == 100 &&
			loaded_definitions[2].duration == 10 &&
			loaded_definitions[2].author == "*ops",
		"boon extension did not reactivate from the current expiry boundary");
	boon_reward_payload mob = { .pid = 42,
				    .racewar = 1,
				    .level = 50,
				    .zone_number = 77,
				    .option = BOPT_MOB,
				    .data = 0,
				    .victim_vnum = 900,
				    .victim_race = 4,
				    .victim_flags = 1 };
	const critical_command first_command = command(mob, 1);
	critical_apply_result applied =
		flatfile_boon_repository_apply(root.string(), first_command);
	boon_reward_result first = decode_applied_result(applied);
	require(applied.outcome == critical_apply_outcome::applied && first.entry_count == 2 &&
			first.entries[0].boon_id == 1 && first.entries[0].counter == 1 &&
			!(first.entries[0].flags & BOON_RESULT_COMPLETED) &&
			first.entries[1].boon_id == 2 && first.entries[1].counter == 0 &&
			(first.entries[1].flags & BOON_RESULT_COMPLETED),
		"first mob event did not preserve SQL boon progress semantics");
	require(flatfile_boon_repository_apply(root.string(), first_command).outcome ==
			critical_apply_outcome::already_applied,
		"boon command did not replay exactly");
	double progress = 0;
	require(flatfile_boon_load_progress(root.string(), 1, 42, &progress, &error) ==
				flatfile_boon_result::ok &&
			progress == 1 &&
			flatfile_boon_load_progress(root.string(), 1, 43, &progress, &error) ==
				flatfile_boon_result::not_found,
		"boon progress projection did not return the exact player row");
	boon_reward_payload conflicting = mob;
	conflicting.victim_vnum = 901;
	require(flatfile_boon_repository_apply(root.string(), command(conflicting, 1)).error_code ==
			EEXIST,
		"conflicting boon operation ID was accepted");
	expect_pending_reward(root.string(), 42, first_command.operation_id, mob.data, &error);
	flatfile_boon_player_projection shop;
	flatfile_boon_pending_reward pending;
	require(flatfile_boon_load_player(root.string(), 42, &shop, &error) ==
				flatfile_boon_result::ok &&
			shop.points == 0 && shop.stats == 2,
		"repeatable stat boon was not reflected in the flat shop");
	applied = flatfile_boon_repository_apply(root.string(), command(mob, 2));
	boon_reward_result second = decode_applied_result(applied);
	require(applied.outcome == critical_apply_outcome::applied && second.entry_count == 2 &&
			(second.entries[0].flags & BOON_RESULT_COMPLETED) &&
			second.entries[0].counter == -1 && second.entries[1].counter == 0 &&
			flatfile_boon_load_player(root.string(), 42, &shop, &error) ==
				flatfile_boon_result::ok &&
			shop.points == 5 && shop.stats == 4,
		"second mob event did not complete one-shot and repeatable rewards");
	expect_pending_reward(root.string(), 42, operation(2), mob.data, &error);
	applied = flatfile_boon_repository_apply(root.string(), command(mob, 3));
	boon_reward_result third = decode_applied_result(applied);
	require(applied.outcome == critical_apply_outcome::applied && third.entry_count == 2 &&
			!(third.entries[0].flags & BOON_RESULT_COMPLETED) &&
			third.entries[0].counter == -1 &&
			(third.entries[1].flags & BOON_RESULT_COMPLETED) &&
			flatfile_boon_load_player(root.string(), 42, &shop, &error) ==
				flatfile_boon_result::ok &&
			shop.points == 5 && shop.stats == 6,
		"completed one-shot boon repeated or repeatable boon stopped");
	expect_pending_reward(root.string(), 42, operation(3), mob.data, &error);
	boon_reward_payload zone = mob;
	zone.option = BOPT_ZONE;
	zone.data = 77;
	zone.victim_flags = 0;
	applied = flatfile_boon_repository_apply(root.string(), command(zone, 4));
	require(applied.outcome == critical_apply_outcome::applied &&
			decode_applied_result(applied).entry_count == 1 &&
			(decode_applied_result(applied).entries[0].flags & BOON_RESULT_COMPLETED),
		"non-progress zone boon did not complete immediately");
	expect_pending_reward(root.string(), 42, operation(4), zone.data, &error);
	boon_reward_payload frag = mob;
	frag.option = BOPT_FRAG;
	frag.data = 12;
	frag.victim_flags = 0;
	applied = flatfile_boon_repository_apply(root.string(), command(frag, 5));
	require(applied.outcome == critical_apply_outcome::applied &&
			decode_applied_result(applied).entry_count == 1 &&
			decode_applied_result(applied).entries[0].boon_id == 4,
		"frag-threshold boon eligibility did not match SQL semantics");
	expect_pending_reward(root.string(), 42, operation(5), frag.data, &error);
	boon_reward_payload excluded = mob;
	excluded.victim_flags = 0;
	applied = flatfile_boon_repository_apply(root.string(), command(excluded, 6));
	require(applied.outcome == critical_apply_outcome::applied &&
			decode_applied_result(applied).entry_count == 0,
		"pet/conjured mob exclusion was not preserved");
	const fs::path legacy_root = root.string() + "-legacy";
	fs::copy(root, legacy_root, fs::copy_options::recursive);
	const fs::path legacy_catalog = legacy_root / "domains" / "boon_catalog";
	convert_catalog_to_legacy_v1(legacy_catalog);
	require(flatfile_boon_load_player(legacy_root.string(), 42, &shop, &error) ==
				flatfile_boon_result::ok &&
			shop.points == 5 && shop.stats == 6,
		"legacy v1 boon catalog was not readable after pending-reward upgrade");

	const critical_command purchase = shop_command(42, 0, 20);
	applied = flatfile_boon_shop_repository_apply(root.string(), purchase);
	boon_shop_result purchased = decode_shop_result(applied);
	require(applied.outcome == critical_apply_outcome::applied && purchased.stat_value == 51 &&
			purchased.stat_revision == 2 && purchased.remaining_stat_points == 5 &&
			flatfile_boon_shop_repository_apply(root.string(), purchase).outcome ==
				critical_apply_outcome::already_applied,
		"boon shop purchase did not commit and replay exactly");
	require(flatfile_boon_shop_repository_apply(root.string(), shop_command(42, 1, 20))
				.error_code == EEXIST,
		"conflicting boon shop operation ID was accepted");
	flatfile_player_domain_record loaded_player;
	require(flatfile_player_domain_load(root.string(), 42, "account-one", 1, &loaded_player,
					    &error) == flatfile_player_domain_result::ok &&
			loaded_player.domains.base_stats[0] == 51 &&
			loaded_player.domains.base_stat_revision == 2 &&
			flatfile_boon_load_player(root.string(), 42, &shop, &error) ==
				flatfile_boon_result::ok &&
			shop.stats == 5,
		"boon shop did not atomically publish catalog and player stat authority");

	const critical_command interrupted = shop_command(42, 1, 21);
	setenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_IMAGE", "1", 1);
	require(flatfile_boon_shop_repository_apply(root.string(), interrupted).outcome ==
			critical_apply_outcome::retryable_failure,
		"boon shop fault injection did not interrupt after the catalog image");
	unsetenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_IMAGE");
	applied = flatfile_boon_shop_repository_apply(root.string(), interrupted);
	purchased = decode_shop_result(applied);
	require(applied.outcome == critical_apply_outcome::already_applied &&
			purchased.stat_value == 52 && purchased.stat_revision == 3 &&
			purchased.remaining_stat_points == 4 &&
			flatfile_player_domain_load(root.string(), 42, "account-one", 1,
						    &loaded_player,
						    &error) == flatfile_player_domain_result::ok &&
			loaded_player.domains.base_stats[1] == 52 &&
			loaded_player.domains.base_stat_revision == 3,
		"interrupted boon shop commit did not recover both after-images exactly once");
	const critical_command no_points = shop_command(43, 0, 30);
	applied = flatfile_boon_shop_repository_apply(root.string(), no_points);
	require(applied.outcome == critical_apply_outcome::terminal_failure &&
			applied.error_code == ENOSPC &&
			decode_shop_result(applied).stat_value == 50 &&
			flatfile_boon_shop_repository_apply(root.string(), no_points).error_code ==
				ENOSPC,
		"boon shop insufficient-points rejection was not durable and replayable");
	{
		flatfile_authority_lock lock;
		flatfile_authority_operation operation;
		require(lock.acquire(root.string(), &error) &&
				flatfile_boon_prepare_player_remove(root.string(), lock, 42,
								    &operation, &error) ==
					flatfile_boon_result::ok &&
				operation.filename == "boon_catalog" && !operation.bytes.empty(),
			"boon player state removal was not prepared: " + error);
		require(flatfile_authority_transaction_commit_operations(root.string(), lock,
									 { operation }, &error) ==
				flatfile_authority_transaction_result::ok,
			"prepared boon player state removal did not commit: " + error);
	}
	require(flatfile_boon_load_player(root.string(), 42, &shop, &error) ==
				flatfile_boon_result::ok &&
			shop.points == 0 && shop.stats == 0 &&
			flatfile_boon_load_progress(root.string(), 1, 42, &progress, &error) ==
				flatfile_boon_result::not_found &&
			flatfile_boon_find_pending_reward(root.string(), 42, &pending, &error) ==
				flatfile_boon_result::not_found &&
			flatfile_boon_load_definitions(root.string(), &loaded_definitions,
						       &error) == flatfile_boon_result::ok &&
			loaded_definitions[4].target_pid == 0 && !loaded_definitions[4].active,
		"prepared boon removal left PID-keyed player state");
	const fs::path overflow_root = root / "overflow";
	const fs::path overflow_domains = overflow_root / "domains";
	fs::create_directories(overflow_domains);
	fs::permissions(overflow_root, fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(overflow_domains, fs::perms::owner_all, fs::perm_options::replace);
	std::vector<flatfile_boon_definition> overflow_definitions;
	for (uint32_t id = 100; id < 100 + BOON_REWARD_MAX_RESULTS + 1; ++id)
		overflow_definitions.push_back(definition(id, BTYPE_EXP, BOPT_NONE, 77, 0, 1));
	require(flatfile_boon_establish(overflow_root.string(), overflow_definitions, &error) ==
			flatfile_boon_result::ok,
		"could not establish overflow boon catalog");
	boon_reward_payload overflow = mob;
	overflow.option = BOPT_NONE;
	overflow.victim_flags = 0;
	const critical_command overflow_command = command(overflow, 7);
	require(flatfile_boon_repository_apply(overflow_root.string(), overflow_command)
					.error_code == E2BIG &&
			flatfile_boon_repository_apply(overflow_root.string(), overflow_command)
					.error_code == E2BIG,
		"oversized boon match set was not rolled back and durably rejected");
	const fs::path catalog = domains / "boon_catalog";
	{
		std::fstream file(catalog, std::ios::in | std::ios::out | std::ios::binary);
		require(file.good(), "could not open boon catalog for corruption");
		file.seekg(-1, std::ios::end);
		char value = 0;
		file.read(&value, 1);
		value ^= 0x51;
		file.seekp(-1, std::ios::end);
		file.write(&value, 1);
	}
	require(flatfile_boon_repository_apply(root.string(), first_command).error_code == EILSEQ &&
			flatfile_boon_load_player(root.string(), 42, &shop, &error) ==
				flatfile_boon_result::invalid,
		"corrupt boon catalog was accepted or exposed");
	for (const auto &entry : fs::directory_iterator(domains))
		require(entry.path().filename().string().find(".tmp.") == std::string::npos,
			"temporary boon catalog file was left behind");
	std::cout << "flat-file boon repository passed\n";
	return 0;
}
