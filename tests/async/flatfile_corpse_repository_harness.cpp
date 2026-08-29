#include "corpse_lifecycle_command.h"
#include "flatfile_corpse_repository.h"
#include "flatfile_world_item_repository.h"

#include <cerrno>
#include <cstdlib>
#include <filesystem>
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

static void prepare_root(const fs::path &root)
{
	fs::create_directories(root / "domains");
	fs::create_directories(root / "players");
	fs::create_directories(root / "identities/names");
	fs::permissions(root, fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(root / "domains", fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(root / "players", fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(root / "identities", fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(root / "identities/names", fs::perms::owner_all, fs::perm_options::replace);
}

static critical_operation_id operation(uint8_t seed)
{
	critical_operation_id value = {};
	value.bytes[0] = seed;
	return value;
}

static corpse_lifecycle_payload upsert(uint64_t revision)
{
	corpse_lifecycle_payload payload = {};
	payload.owner_pid = 42;
	payload.save_id = 20;
	payload.expected_corpse_revision = revision;
	payload.room_vnum = 500;
	payload.weight = 90;
	payload.values[3] = 42;
	payload.values[5] = 1;
	payload.values[6] = 20;
	payload.money = { 11, 22, 33, 44 };
	payload.owner_name = "Hero";
	payload.short_description = "the corpse of Hero";
	payload.description = "The corpse of Hero is lying here.";
	payload.keywords = "hero corpse _pcorpse_";
	return payload;
}

static critical_command command(uint8_t seed, const corpse_lifecycle_payload &payload)
{
	critical_command value = {};
	require(corpse_lifecycle_command_build(&value, operation(seed), payload,
					       critical_source_site::command,
					       critical_deadline_class::terminal),
		"could not build corpse lifecycle command");
	value.accepted_at_usec = static_cast<uint64_t>(seed) * 1000;
	return value;
}

static player_item_snapshot item()
{
	player_item_snapshot value = {};
	value.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	value.equipment_slot = -1;
	value.object_uid = 900;
	value.vnum = 1900;
	return value;
}

int main(int argc, char **argv)
{
	require(argc == 2, "state root argument required");
	const fs::path root = fs::path(argv[1]) / "lifecycle";
	prepare_root(root);
	std::string error;
	require(flatfile_world_item_establish(root.string(), {}, {}, &error) ==
			flatfile_world_item_result::ok,
		"could not establish empty world item authority: " + error);
	auto create_payload = upsert(0);
	auto create = command(1, create_payload);
	setenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_IMAGE", "1", 1);
	auto applied = flatfile_corpse_repository_apply(root.string(), create);
	unsetenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_IMAGE");
	require(applied.outcome == critical_apply_outcome::retryable_failure &&
			applied.error_code == EIO,
		"interrupted corpse creation did not retain recoverable intent");
	applied = flatfile_corpse_repository_apply(root.string(), create);
	require(applied.outcome == critical_apply_outcome::already_applied &&
			applied.result_size == CORPSE_LIFECYCLE_RESULT_BYTES,
		"corpse creation did not recover and replay exactly");
	corpse_lifecycle_result result = {};
	require(corpse_lifecycle_command_decode_result(applied.result_payload.data(),
						       applied.result_size, &result) &&
			result.corpse_revision == 1 && result.catalog_revision == 2,
		"corpse creation result did not expose durable revisions");
	std::vector<flatfile_corpse_record> corpses;
	std::vector<flatfile_saved_world_item_record> saved;
	require(flatfile_world_item_list(root.string(), &corpses, &saved, &error) ==
				flatfile_world_item_result::ok &&
			corpses.size() == 1 && corpses[0].owner_name == "hero" &&
			corpses[0].revision == 1 && corpses[0].money == create_payload.money &&
			corpses[0].items.empty(),
		"empty money-bearing corpse was not established exactly");
	auto altered_payload = create_payload;
	altered_payload.room_vnum = 501;
	auto altered = command(1, altered_payload);
	applied = flatfile_corpse_repository_apply(root.string(), altered);
	require(applied.outcome == critical_apply_outcome::terminal_failure &&
			applied.error_code == EEXIST,
		"operation ID reuse with a different corpse command was accepted");
	corpse_lifecycle_payload remove_payload = {};
	remove_payload.action = corpse_lifecycle_action::remove;
	remove_payload.owner_pid = 42;
	remove_payload.save_id = 20;
	remove_payload.expected_corpse_revision = 1;
	remove_payload.owner_name = "Hero";
	auto money_remove = command(6, remove_payload);
	applied = flatfile_corpse_repository_apply(root.string(), money_remove);
	require(applied.outcome == critical_apply_outcome::terminal_failure &&
			applied.error_code == ENOTEMPTY,
		"money-bearing corpse removal did not fail closed");
	auto update_payload = upsert(1);
	update_payload.room_vnum = 700;
	update_payload.weight = 75;
	update_payload.money = {};
	auto update = command(2, update_payload);
	applied = flatfile_corpse_repository_apply(root.string(), update);
	require(applied.outcome == critical_apply_outcome::applied,
		"corpse metadata/relocation update did not apply");
	require(flatfile_world_item_list(root.string(), &corpses, &saved, &error) ==
				flatfile_world_item_result::ok &&
			corpses[0].revision == 2 && corpses[0].room_vnum == 700 &&
			corpses[0].weight == 75 && corpses[0].money == update_payload.money,
		"corpse metadata/relocation update did not preserve the expected state");
	auto stale_payload = update_payload;
	stale_payload.expected_corpse_revision = 1;
	stale_payload.room_vnum = 701;
	auto stale = command(3, stale_payload);
	applied = flatfile_corpse_repository_apply(root.string(), stale);
	require(applied.outcome == critical_apply_outcome::terminal_failure &&
			applied.error_code == ESTALE && !applied.result_size,
		"stale corpse update did not fail deterministically");
	applied = flatfile_corpse_repository_apply(root.string(), stale);
	require(applied.outcome == critical_apply_outcome::terminal_failure &&
			applied.error_code == ESTALE,
		"stale corpse update was not durably replayable");
	remove_payload.expected_corpse_revision = 2;
	auto remove = command(4, remove_payload);
	applied = flatfile_corpse_repository_apply(root.string(), remove);
	require(applied.outcome == critical_apply_outcome::applied,
		"empty corpse removal did not apply");
	require(flatfile_world_item_list(root.string(), &corpses, &saved, &error) ==
				flatfile_world_item_result::ok &&
			corpses.empty(),
		"empty corpse removal did not publish");
	applied = flatfile_corpse_repository_apply(root.string(), remove);
	require(applied.outcome == critical_apply_outcome::already_applied,
		"empty corpse removal did not replay exactly");

	const fs::path occupied_root = fs::path(argv[1]) / "occupied";
	prepare_root(occupied_root);
	flatfile_corpse_record occupied = {};
	occupied.owner_pid = 42;
	occupied.owner_name = "hero";
	occupied.save_id = 20;
	occupied.room_vnum = 500;
	occupied.values[3] = 42;
	occupied.values[5] = 1;
	occupied.values[6] = 20;
	occupied.revision = 3;
	occupied.items = { item() };
	require(flatfile_world_item_establish(occupied_root.string(), { occupied }, {}, &error) ==
			flatfile_world_item_result::ok,
		"could not establish occupied corpse");
	remove_payload.expected_corpse_revision = 3;
	auto occupied_remove = command(5, remove_payload);
	applied = flatfile_corpse_repository_apply(occupied_root.string(), occupied_remove);
	require(applied.outcome == critical_apply_outcome::terminal_failure &&
			applied.error_code == ENOTEMPTY,
		"occupied corpse removal did not fail closed");
	applied = flatfile_corpse_repository_apply(occupied_root.string(), occupied_remove);
	require(applied.outcome == critical_apply_outcome::terminal_failure &&
			applied.error_code == ENOTEMPTY,
		"occupied corpse removal result was not durable");
	require(flatfile_world_item_list(occupied_root.string(), &corpses, &saved, &error) ==
				flatfile_world_item_result::ok &&
			corpses.size() == 1 && corpses[0].items.size() == 1,
		"occupied corpse removal changed authority");
	std::cout << "flat-file corpse lifecycle repository passed\n";
	return 0;
}
