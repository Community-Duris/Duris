#include "boon.h"
#include "flatfile_boon_repository.h"

#include <cerrno>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
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
	boon_reward_payload conflicting = mob;
	conflicting.victim_vnum = 901;
	require(flatfile_boon_repository_apply(root.string(), command(conflicting, 1)).error_code ==
			EEXIST,
		"conflicting boon operation ID was accepted");
	flatfile_boon_player_projection shop;
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
	boon_reward_payload zone = mob;
	zone.option = BOPT_ZONE;
	zone.data = 77;
	zone.victim_flags = 0;
	applied = flatfile_boon_repository_apply(root.string(), command(zone, 4));
	require(applied.outcome == critical_apply_outcome::applied &&
			decode_applied_result(applied).entry_count == 1 &&
			(decode_applied_result(applied).entries[0].flags & BOON_RESULT_COMPLETED),
		"non-progress zone boon did not complete immediately");
	boon_reward_payload frag = mob;
	frag.option = BOPT_FRAG;
	frag.data = 12;
	frag.victim_flags = 0;
	applied = flatfile_boon_repository_apply(root.string(), command(frag, 5));
	require(applied.outcome == critical_apply_outcome::applied &&
			decode_applied_result(applied).entry_count == 1 &&
			decode_applied_result(applied).entries[0].boon_id == 4,
		"frag-threshold boon eligibility did not match SQL semantics");
	boon_reward_payload excluded = mob;
	excluded.victim_flags = 0;
	applied = flatfile_boon_repository_apply(root.string(), command(excluded, 6));
	require(applied.outcome == critical_apply_outcome::applied &&
			decode_applied_result(applied).entry_count == 0,
		"pet/conjured mob exclusion was not preserved");
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
