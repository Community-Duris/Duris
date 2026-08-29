#include "corpse_lifecycle_command.h"
#include "flatfile_artifact_repository.h"
#include "flatfile_corpse_repository.h"
#include "flatfile_item_repository.h"
#include "flatfile_player_domain_repository.h"
#include "flatfile_shop_trade_materialization.h"
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

static player_item_snapshot release_item(uint64_t uid, int32_t parent, int32_t vnum,
					 bool artifact = false)
{
	player_item_snapshot value = {};
	value.parent_index = parent;
	value.equipment_slot = -1;
	value.object_uid = uid;
	value.vnum = vnum;
	value.name = "released item";
	value.extra_flags = artifact ? 1U << 28 : 0;
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

	const fs::path release_root = fs::path(argv[1]) / "release";
	prepare_root(release_root);
	flatfile_corpse_record released_corpse = {};
	released_corpse.owner_pid = 42;
	released_corpse.owner_name = "hero";
	released_corpse.save_id = 20;
	released_corpse.room_vnum = 500;
	released_corpse.values[3] = 42;
	released_corpse.values[5] = 1;
	released_corpse.values[6] = 20;
	released_corpse.money = { 10, 20, 30, 40 };
	released_corpse.revision = 3;
	released_corpse.items = {
		release_item(900, PLAYER_SNAPSHOT_NO_PARENT, 1900),
		release_item(901, 0, 1901, true),
	};
	require(flatfile_world_item_establish(release_root.string(), { released_corpse }, {},
					      &error) == flatfile_world_item_result::ok,
		"could not establish releasable corpse: " + error);
	const item_owner_identity corpse_owner = { item_owner_type::corpse,
						   item_corpse_owner_id(42, 20), 0 };
	require(flatfile_item_repository_establish_owner(
			release_root.string(), corpse_owner,
			{ { 900, 900, 0, corpse_owner, 1, 1900, item_custody_state::active },
			  { 901, 900, 900, corpse_owner, 1, 1901, item_custody_state::active } },
			&error) == flatfile_item_baseline_result::applied,
		"could not establish releasable corpse custody: " + error);
	const flatfile_artifact_record corpse_artifact = {
		1901, true, FLATFILE_ARTIFACT_ON_CORPSE, 42, 5000, 1, 1000, -1, 0, 1
	};
	require(flatfile_artifact_establish(release_root.string(), { corpse_artifact }, &error) ==
			flatfile_artifact_result::ok,
		"could not establish releasable corpse artifact: " + error);
	corpse_lifecycle_payload release_payload = {};
	release_payload.action = corpse_lifecycle_action::release;
	release_payload.owner_pid = 42;
	release_payload.save_id = 20;
	release_payload.expected_corpse_revision = 3;
	release_payload.expected_room_revision = 0;
	release_payload.room_vnum = 500;
	release_payload.owner_name = "Hero";
	auto release_command = command(7, release_payload);
	release_command.accepted_at_usec = 12000000;
	setenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_IMAGE", "2", 1);
	applied = flatfile_corpse_repository_apply(release_root.string(), release_command);
	unsetenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_IMAGE");
	require(applied.outcome == critical_apply_outcome::retryable_failure &&
			applied.error_code == EIO,
		"interrupted corpse release did not retain recoverable composite intent");
	applied = flatfile_corpse_repository_apply(release_root.string(), release_command);
	require(applied.outcome == critical_apply_outcome::already_applied &&
			applied.result_size == CORPSE_LIFECYCLE_RESULT_BYTES,
		"corpse release did not recover and replay exactly");
	result = {};
	require(corpse_lifecycle_command_decode_result(applied.result_payload.data(),
						       applied.result_size, &result) &&
			result.action == corpse_lifecycle_action::release &&
			result.corpse_revision == 0 && result.catalog_revision == 2 &&
			result.corpse_owner_revision == 2 && result.room_owner_revision == 1 &&
			result.max_item_revision == 2 && result.item_count == 2,
		"corpse release result did not expose all durable revisions");
	corpses.clear();
	saved.clear();
	require(flatfile_world_item_list(release_root.string(), &corpses, &saved, &error) ==
				flatfile_world_item_result::ok &&
			corpses.empty(),
		"recovered corpse release retained the corpse aggregate");
	std::vector<flatfile_room_item_record> rooms;
	require(flatfile_world_item_list_rooms(release_root.string(), &rooms, &error) ==
				flatfile_world_item_result::ok &&
			rooms.size() == 1 && rooms[0].room_vnum == 500 && rooms[0].revision == 1 &&
			rooms[0].money == released_corpse.money && rooms[0].items.size() == 2 &&
			rooms[0].items[0].object_uid == 900 && rooms[0].items[1].parent_index == 0,
		"recovered corpse release did not publish the exact room aggregate");
	uint64_t room_revision = 0;
	std::vector<flatfile_item_ownership_record> room_items;
	require(flatfile_item_repository_load_owner(
			release_root.string(), { item_owner_type::room, 500, 0 }, &room_revision,
			&room_items, &error) == flatfile_item_repository_result::ok &&
			room_revision == 1 && room_items.size() == 2 &&
			room_items[0].owner.type == item_owner_type::room &&
			room_items[0].item_revision == 2 && room_items[1].parent_item_uid == 900,
		"recovered corpse release did not publish room item custody");
	flatfile_artifact_record grounded_artifact;
	require(flatfile_artifact_get(release_root.string(), 1901, &grounded_artifact, &error) ==
				flatfile_artifact_result::ok &&
			grounded_artifact.owned &&
			grounded_artifact.location_type == FLATFILE_ARTIFACT_ON_GROUND &&
			grounded_artifact.location == 500 && grounded_artifact.last_update == 12 &&
			grounded_artifact.revision == 2,
		"recovered corpse release did not ground the artifact atomically");
	auto altered_release_payload = release_payload;
	altered_release_payload.room_vnum = 501;
	applied = flatfile_corpse_repository_apply(release_root.string(),
						   command(7, altered_release_payload));
	require(applied.outcome == critical_apply_outcome::terminal_failure &&
			applied.error_code == EEXIST,
		"corpse release operation ID reuse with different content was accepted");
	applied = flatfile_corpse_repository_apply(release_root.string(),
						   command(8, release_payload));
	require(applied.outcome == critical_apply_outcome::terminal_failure &&
			applied.error_code == ENOENT,
		"second corpse release did not fail durably after aggregate removal");
	auto money_corpse = upsert(0);
	money_corpse.owner_pid = 77;
	money_corpse.save_id = 21;
	money_corpse.values[3] = 77;
	money_corpse.values[6] = 21;
	money_corpse.owner_name = "Other";
	money_corpse.money = { 1, 2, 3, 4 };
	applied = flatfile_corpse_repository_apply(release_root.string(), command(9, money_corpse));
	require(applied.outcome == critical_apply_outcome::applied,
		"money-only corpse did not establish beside the room aggregate");
	corpse_lifecycle_payload money_release = {};
	money_release.action = corpse_lifecycle_action::release;
	money_release.owner_pid = 77;
	money_release.save_id = 21;
	money_release.expected_corpse_revision = 1;
	money_release.expected_room_revision = 1;
	money_release.room_vnum = 500;
	money_release.owner_name = "Other";
	applied =
		flatfile_corpse_repository_apply(release_root.string(), command(10, money_release));
	require(applied.outcome == critical_apply_outcome::applied,
		"money-only corpse did not release into the existing room aggregate");
	result = {};
	require(corpse_lifecycle_command_decode_result(applied.result_payload.data(),
						       applied.result_size, &result) &&
			!result.item_count && !result.max_item_revision &&
			result.corpse_owner_revision == 1 && result.room_owner_revision == 2,
		"money-only corpse release returned incorrect owner revisions");
	rooms.clear();
	require(flatfile_world_item_list_rooms(release_root.string(), &rooms, &error) ==
				flatfile_world_item_result::ok &&
			rooms.size() == 1 && rooms[0].revision == 2 &&
			rooms[0].money == std::array<int32_t, 4>{ 11, 22, 33, 44 } &&
			rooms[0].items.size() == 2,
		"money-only corpse release did not accumulate the existing room aggregate");

	const fs::path destruction_root = fs::path(argv[1]) / "destruction";
	prepare_root(destruction_root);
	require(flatfile_world_item_establish(destruction_root.string(), { released_corpse }, {},
					      &error) == flatfile_world_item_result::ok,
		"could not establish destructible corpse: " + error);
	require(flatfile_item_repository_establish_owner(
			destruction_root.string(), corpse_owner,
			{ { 900, 900, 0, corpse_owner, 1, 1900, item_custody_state::active },
			  { 901, 900, 900, corpse_owner, 1, 1901, item_custody_state::active } },
			&error) == flatfile_item_baseline_result::applied,
		"could not establish destructible corpse custody: " + error);
	flatfile_artifact_record destructible_artifact = corpse_artifact;
	destructible_artifact.bind_owner_pid = 42;
	destructible_artifact.bind_timer = 6000;
	require(flatfile_artifact_establish(destruction_root.string(), { destructible_artifact },
					    &error) == flatfile_artifact_result::ok,
		"could not establish destructible corpse artifact: " + error);
	corpse_lifecycle_payload destruction_payload = release_payload;
	destruction_payload.action = corpse_lifecycle_action::destroy;
	auto destruction_command = command(11, destruction_payload);
	destruction_command.accepted_at_usec = 13000000;
	setenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_IMAGE", "2", 1);
	applied = flatfile_corpse_repository_apply(destruction_root.string(), destruction_command);
	unsetenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_IMAGE");
	require(applied.outcome == critical_apply_outcome::retryable_failure &&
			applied.error_code == EIO,
		"interrupted corpse destruction did not retain recoverable composite intent");
	applied = flatfile_corpse_repository_apply(destruction_root.string(), destruction_command);
	require(applied.outcome == critical_apply_outcome::already_applied,
		"corpse destruction did not recover and replay exactly");
	result = {};
	require(corpse_lifecycle_command_decode_result(applied.result_payload.data(),
						       applied.result_size, &result) &&
			result.action == corpse_lifecycle_action::destroy &&
			result.corpse_revision == 0 && result.catalog_revision == 2 &&
			result.corpse_owner_revision == 2 && result.room_owner_revision == 1 &&
			result.max_item_revision == 2 && result.item_count == 2,
		"corpse destruction result did not expose destruction revisions");
	corpses.clear();
	saved.clear();
	require(flatfile_world_item_list(destruction_root.string(), &corpses, &saved, &error) ==
				flatfile_world_item_result::ok &&
			corpses.empty(),
		"recovered corpse destruction retained the corpse aggregate");
	rooms.clear();
	require(flatfile_world_item_list_rooms(destruction_root.string(), &rooms, &error) ==
				flatfile_world_item_result::ok &&
			rooms.empty(),
		"corpse destruction incorrectly published contents or money to a room");
	uint64_t destruction_revision = 0;
	std::vector<flatfile_item_ownership_record> destroyed_items;
	require(flatfile_item_repository_load_owner(
			destruction_root.string(), { item_owner_type::destruction, 0, 0 },
			&destruction_revision, &destroyed_items,
			&error) == flatfile_item_repository_result::ok &&
			destruction_revision == 1 && destroyed_items.empty(),
		"recovered corpse destruction retained active destination item custody");
	flatfile_artifact_record destroyed_artifact;
	require(flatfile_artifact_get(destruction_root.string(), 1901, &destroyed_artifact,
				      &error) == flatfile_artifact_result::ok &&
			!destroyed_artifact.owned &&
			destroyed_artifact.location_type == FLATFILE_ARTIFACT_NOT_IN_GAME &&
			destroyed_artifact.location == -1 && destroyed_artifact.last_update == 13 &&
			destroyed_artifact.bind_owner_pid == -1 &&
			destroyed_artifact.bind_timer == 0 && destroyed_artifact.revision == 2,
		"recovered corpse destruction did not clear artifact custody and binding");

	const fs::path resurrection_root = fs::path(argv[1]) / "resurrection";
	prepare_root(resurrection_root);
	flatfile_corpse_record resurrection_corpse = released_corpse;
	resurrection_corpse.money = { 40, 30, 20, 10 };
	require(flatfile_world_item_establish(resurrection_root.string(), { resurrection_corpse },
					      {}, &error) == flatfile_world_item_result::ok,
		"could not establish resurrectable corpse: " + error);
	require(flatfile_item_repository_establish_owner(
			resurrection_root.string(), corpse_owner,
			{ { 900, 900, 0, corpse_owner, 1, 1900, item_custody_state::active },
			  { 901, 900, 900, corpse_owner, 1, 1901, item_custody_state::active } },
			&error) == flatfile_item_baseline_result::applied,
		"could not establish resurrectable corpse custody: " + error);
	const item_owner_identity resurrected_player_owner = { item_owner_type::player, 70, 0 };
	require(flatfile_item_repository_establish_owner(
			resurrection_root.string(), resurrected_player_owner,
			{ { 800, 800, 0, resurrected_player_owner, 1, 1800,
			    item_custody_state::active } },
			&error) == flatfile_item_baseline_result::applied,
		"could not establish resurrection target custody: " + error);
	flatfile_player_domain_record resurrection_player = {};
	resurrection_player.pid = 70;
	resurrection_player.account_name = "resurrection-account";
	resurrection_player.racewar = 1;
	resurrection_player.domains.wallet = { 1, 2, 3, 4 };
	require(flatfile_player_domain_establish(resurrection_root.string(), resurrection_player,
						 &error) == flatfile_player_domain_result::ok,
		"could not establish resurrection target wallet: " + error);
	flatfile_artifact_record resurrection_artifact = corpse_artifact;
	resurrection_artifact.bind_owner_pid = 42;
	resurrection_artifact.bind_timer = 6000;
	require(flatfile_artifact_establish(resurrection_root.string(), { resurrection_artifact },
					    &error) == flatfile_artifact_result::ok,
		"could not establish resurrectable corpse artifact: " + error);
	corpse_lifecycle_payload resurrection_payload = {};
	resurrection_payload.action = corpse_lifecycle_action::resurrect;
	resurrection_payload.owner_pid = 42;
	resurrection_payload.save_id = 20;
	resurrection_payload.expected_corpse_revision = 3;
	resurrection_payload.expected_room_revision = 0;
	resurrection_payload.destination_player_pid = 70;
	resurrection_payload.old_room_vnum = 600;
	resurrection_payload.expected_player_revision = 1;
	resurrection_payload.expected_wallet_revision = 0;
	resurrection_payload.room_vnum = 500;
	resurrection_payload.money = { 1, 2, 3, 4 };
	resurrection_payload.owner_name = "Hero";
	auto resurrection_command = command(12, resurrection_payload);
	resurrection_command.accepted_at_usec = 14000000;
	setenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_IMAGE", "3", 1);
	applied =
		flatfile_corpse_repository_apply(resurrection_root.string(), resurrection_command);
	unsetenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_IMAGE");
	require(applied.outcome == critical_apply_outcome::retryable_failure &&
			applied.error_code == EIO,
		"interrupted corpse resurrection did not retain recoverable composite intent");
	applied =
		flatfile_corpse_repository_apply(resurrection_root.string(), resurrection_command);
	require(applied.outcome == critical_apply_outcome::already_applied,
		"corpse resurrection did not recover and replay exactly");
	result = {};
	require(corpse_lifecycle_command_decode_result(applied.result_payload.data(),
						       applied.result_size, &result) &&
			result.action == corpse_lifecycle_action::resurrect &&
			result.catalog_revision == 2 && result.corpse_owner_revision == 2 &&
			result.room_owner_revision == 1 && result.player_owner_revision == 2 &&
			result.wallet_revision == 1 && result.max_item_revision == 2 &&
			result.item_count == 2 && result.wallet == resurrection_corpse.money,
		"corpse resurrection result did not expose all committed revisions");
	corpses.clear();
	saved.clear();
	require(flatfile_world_item_list(resurrection_root.string(), &corpses, &saved, &error) ==
				flatfile_world_item_result::ok &&
			corpses.empty(),
		"recovered corpse resurrection retained the corpse aggregate");
	rooms.clear();
	require(flatfile_world_item_list_rooms(resurrection_root.string(), &rooms, &error) ==
				flatfile_world_item_result::ok &&
			rooms.size() == 1 && rooms[0].room_vnum == 600 && rooms[0].revision == 1 &&
			rooms[0].money == resurrection_payload.money && rooms[0].items.empty(),
		"recovered corpse resurrection did not deposit the target's old wallet");
	uint64_t resurrected_player_revision = 0;
	std::vector<flatfile_item_ownership_record> resurrected_player_items;
	require(flatfile_item_repository_load_owner(
			resurrection_root.string(), resurrected_player_owner,
			&resurrected_player_revision, &resurrected_player_items,
			&error) == flatfile_item_repository_result::ok &&
			resurrected_player_revision == 2 && resurrected_player_items.size() == 3 &&
			resurrected_player_items[1].item_uid == 900 &&
			resurrected_player_items[1].item_revision == 2 &&
			resurrected_player_items[2].parent_item_uid == 900,
		"recovered corpse resurrection did not transfer target item custody");
	flatfile_player_domain_record loaded_resurrection_player;
	require(flatfile_player_domain_load(resurrection_root.string(), 70, "resurrection-account",
					    1, &loaded_resurrection_player,
					    &error) == flatfile_player_domain_result::ok &&
			loaded_resurrection_player.domains.wallet_revision == 1 &&
			loaded_resurrection_player.domains.wallet ==
				std::array<uint64_t, 4>{ 40, 30, 20, 10 },
		"recovered corpse resurrection did not exchange the target wallet");
	flatfile_artifact_record resurrected_artifact;
	require(flatfile_artifact_get(resurrection_root.string(), 1901, &resurrected_artifact,
				      &error) == flatfile_artifact_result::ok &&
			resurrected_artifact.owned &&
			resurrected_artifact.location_type == FLATFILE_ARTIFACT_ON_PLAYER &&
			resurrected_artifact.location == 70 &&
			resurrected_artifact.bind_owner_pid == 42 &&
			resurrected_artifact.bind_timer == 6000 &&
			resurrected_artifact.last_update == 14 &&
			resurrected_artifact.revision == 2,
		"recovered corpse resurrection did not move the artifact without changing its soul binding");
	player_snapshot stale_resurrection_snapshot = {};
	stale_resurrection_snapshot.pid = 70;
	{
		flatfile_authority_lock reconciliation_lock;
		require(reconciliation_lock.acquire(resurrection_root.string(), &error),
			"could not lock resurrection materialization: " + error);
		require(flatfile_shop_trade_materialization_reconcile(
				resurrection_root.string(), reconciliation_lock, 70,
				resurrected_player_items, &stale_resurrection_snapshot,
				&error) == flatfile_shop_trade_materialization_result::ok &&
				stale_resurrection_snapshot.items.size() == 2 &&
				stale_resurrection_snapshot.items[0].object_uid == 900 &&
				stale_resurrection_snapshot.items[1].parent_index == 0,
			"restart reconciliation did not materialize resurrected corpse items: " +
				error);
	}
	std::cout << "flat-file corpse lifecycle repository passed\n";
	return 0;
}
