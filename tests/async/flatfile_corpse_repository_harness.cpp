#include "core/defines.h"
#include "persistence/corpse_lifecycle_command.h"
#include "flatfile/flatfile_artifact_repository.h"
#include "flatfile/flatfile_authority_transaction.h"
#include "flatfile/flatfile_collector_repository.h"
#include "flatfile/flatfile_corpse_repository.h"
#include "flatfile/flatfile_item_repository.h"
#include "flatfile/flatfile_player_domain_repository.h"
#include "flatfile/flatfile_shop_trade_materialization.h"
#include "flatfile/flatfile_world_item_repository.h"
#include "player/player_snapshot_codec.h"

#include <algorithm>
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

static void seed_collector_candidate(const std::string &root, const flatfile_corpse_record &corpse,
				     uint8_t operation_seed, std::string *error)
{
	require(corpse.items.size() == 2 && corpse.items[0].object_uid == 900 &&
			corpse.items[1].object_uid == 901,
		"collector corpse fixture topology changed unexpectedly");
	std::vector<player_item_snapshot> snapshots = corpse.items;
	for (player_item_snapshot &snapshot : snapshots)
	{
		snapshot.type = ITEM_WEAPON;
		snapshot.name = "eligible antique";
		snapshot.short_description = "an eligible antique";
		snapshot.description = "An eligible antique is here.";
		snapshot.wear_flags = ITEM_TAKE;
		snapshot.cost = 500;
	}

	item_transfer_payload death = {};
	death.to_owner = { item_owner_type::corpse,
			   item_corpse_owner_id(corpse.owner_pid, corpse.save_id), 0 };
	death.reason = item_transfer_reason::corpse_create;
	death.reason_id = corpse.save_id;
	death.selected_item_uid = snapshots[0].object_uid;
	death.target_root_item_uid = snapshots[0].object_uid;
	death.item_count = static_cast<uint16_t>(snapshots.size());
	for (size_t index = 0; index < snapshots.size(); ++index)
	{
		const uint64_t parent =
			snapshots[index].parent_index == PLAYER_SNAPSHOT_NO_PARENT ?
				UINT64_C(0) :
				snapshots[static_cast<size_t>(snapshots[index].parent_index)]
					.object_uid;
		death.items[index] = {
			snapshots[index].object_uid, snapshots[0].object_uid,	parent, 0,
			snapshots[index].vnum,	     item_custody_state::active
		};
	}
	std::vector<uint8_t> blob;
	require(player_item_snapshot_list_encode(snapshots, &blob) ==
				player_snapshot_codec_result::ok &&
			!blob.empty() && blob.size() <= death.item_blob.size(),
		"could not encode collector corpse fixture");
	death.item_blob_size = static_cast<uint32_t>(blob.size());
	std::copy(blob.begin(), blob.end(), death.item_blob.begin());
	death.collector.present = true;
	death.collector.death_operation = operation(operation_seed);
	death.collector.beneficiary_pid = corpse.owner_pid;
	death.collector.death_time = 1000 + operation_seed;
	death.collector.policy = { 10, 20, 30, 200, 100 };
	death.collector.eligible_item_uids = { 900 };

	item_transfer_result transfer = {};
	transfer.root_item_uid = 900;
	transfer.item_count = 2;
	transfer.from_owner_revision = 1;
	transfer.to_owner_revision = 1;
	transfer.max_item_revision = 1;
	transfer.corpse_revision = corpse.revision;
	flatfile_authority_lock lock;
	require(lock.acquire(root, error), "could not lock collector corpse fixture: " + *error);
	flatfile_collector_enrollment_mutation mutation;
	unsigned int result_code = 0;
	const auto prepared = flatfile_collector_prepare_death_enrollment(
		root, lock, death, transfer, &mutation, &result_code, error);
	require(prepared == flatfile_collector_repository_result::ok && !result_code &&
			mutation.enrolled == 1 && !mutation.after_image.bytes.empty(),
		"could not seed collector corpse candidate: " + *error + " (result " +
			std::to_string(static_cast<unsigned int>(prepared)) + ", code " +
			std::to_string(result_code) + ")");
	require(flatfile_authority_transaction_commit(root, lock, { mutation.after_image },
						      error) ==
			flatfile_authority_transaction_result::ok,
		"could not commit collector corpse candidate: " + *error);
}

static void require_collector_listing(const std::string &root, collector::state status,
				      collector::reason reason, uint64_t item_revision,
				      uint64_t catalog_revision, std::string *error)
{
	collector_bootstrap_snapshot bootstrap;
	require(flatfile_collector_repository_read_bootstrap(root, &bootstrap, error) ==
				flatfile_collector_repository_result::ok &&
			bootstrap.catalog.revision == catalog_revision &&
			bootstrap.catalog.records.size() == 1,
		"collector corpse catalog was not readable exactly: " + *error);
	const collector::record &listing = bootstrap.catalog.records[0];
	require(listing.listing == 1 && listing.uid == 900 && listing.status == status &&
			listing.closed_reason == reason && listing.item_revision == item_revision &&
			listing.revision ==
				(status == collector::state::candidate ? UINT64_C(1) : UINT64_C(2)),
		"collector corpse listing did not reach the expected durable state");
}

int main(int argc, char **argv)
{
	require(argc == 2, "state root argument required");
	const fs::path root = fs::path(argv[1]) / "lifecycle";
	prepare_root(root);
	std::string error;
	require(!fs::exists(root / "domains/world_item_catalog"),
		"first corpse fixture unexpectedly has a world catalog");
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
	seed_collector_candidate(release_root.string(), released_corpse, 70, &error);
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
			result.max_item_revision == 2 && result.item_count == 2 &&
			!result.collector_catalog_changed,
		"corpse release result did not expose all durable revisions");
	require_collector_listing(release_root.string(), collector::state::candidate,
				  collector::reason::none, 1, 1, &error);
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
			rooms[0].money == std::array<int32_t, 4>{} && rooms[0].items.size() == 2 &&
			rooms[0].items[0].object_uid == 900 && rooms[0].items[1].parent_index == 0,
		"recovered corpse release persisted live floor money in the room aggregate");
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
			rooms[0].money == std::array<int32_t, 4>{} && rooms[0].items.size() == 2,
		"money-only corpse release persisted a rebootable room-money copy");

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
	seed_collector_candidate(destruction_root.string(), released_corpse, 71, &error);
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
			result.max_item_revision == 2 && result.item_count == 2 &&
			result.collector_catalog_changed,
		"corpse destruction result did not expose destruction revisions");
	require_collector_listing(destruction_root.string(), collector::state::cancelled,
				  collector::reason::destroyed, 2, 2, &error);
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
	seed_collector_candidate(resurrection_root.string(), resurrection_corpse, 72, &error);
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
			result.item_count == 2 && result.wallet == resurrection_corpse.money &&
			result.collector_catalog_changed,
		"corpse resurrection result did not expose all committed revisions");
	require_collector_listing(resurrection_root.string(), collector::state::cancelled,
				  collector::reason::claimed, 2, 2, &error);
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
			rooms[0].money == std::array<int32_t, 4>{} && rooms[0].items.empty(),
		"recovered corpse resurrection persisted the target's live floor wallet");
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

	const fs::path raise_root = fs::path(argv[1]) / "raise";
	prepare_root(raise_root);
	flatfile_corpse_record raised_corpse = released_corpse;
	raised_corpse.money = { 40, 30, 20, 10 };
	require(flatfile_world_item_establish(raise_root.string(), { raised_corpse }, {}, &error) ==
			flatfile_world_item_result::ok,
		"could not establish raiseable corpse: " + error);
	require(flatfile_item_repository_establish_owner(
			raise_root.string(), corpse_owner,
			{ { 900, 900, 0, corpse_owner, 1, 1900, item_custody_state::active },
			  { 901, 900, 900, corpse_owner, 1, 1901, item_custody_state::active } },
			&error) == flatfile_item_baseline_result::applied,
		"could not establish raiseable corpse custody: " + error);
	const item_owner_identity raising_player_owner = { item_owner_type::player, 71, 0 };
	require(flatfile_item_repository_establish_owner(raise_root.string(), raising_player_owner,
							 { { 810, 810, 0, raising_player_owner, 1,
							     1810, item_custody_state::active } },
							 &error) ==
			flatfile_item_baseline_result::applied,
		"could not establish raising player custody: " + error);
	flatfile_player_domain_record raising_player = {};
	raising_player.pid = 71;
	raising_player.account_name = "raising-account";
	raising_player.racewar = 1;
	raising_player.domains.wallet = { 5, 6, 7, 8 };
	require(flatfile_player_domain_establish(raise_root.string(), raising_player, &error) ==
			flatfile_player_domain_result::ok,
		"could not establish raising player wallet: " + error);
	flatfile_artifact_record raised_artifact = corpse_artifact;
	raised_artifact.bind_owner_pid = 42;
	raised_artifact.bind_timer = 6000;
	require(flatfile_artifact_establish(raise_root.string(), { raised_artifact }, &error) ==
			flatfile_artifact_result::ok,
		"could not establish raised corpse artifact: " + error);
	seed_collector_candidate(raise_root.string(), raised_corpse, 73, &error);
	corpse_lifecycle_payload raise_payload = {};
	raise_payload.action = corpse_lifecycle_action::raise_follower;
	raise_payload.owner_pid = 42;
	raise_payload.save_id = 20;
	raise_payload.expected_corpse_revision = 3;
	raise_payload.destination_player_pid = 71;
	raise_payload.expected_player_revision = 1;
	raise_payload.expected_wallet_revision = 0;
	raise_payload.room_vnum = 500;
	raise_payload.money = { 5, 6, 7, 8 };
	raise_payload.owner_name = "Hero";
	raise_payload.pet_uid = item_corpse_owner_id(42, 20);
	raise_payload.pet_mob_vnum = 701;
	raise_payload.pet_hit = 20;
	raise_payload.pet_max_hit = 20;
	raise_payload.pet_mana = 10;
	raise_payload.pet_max_mana = 10;
	raise_payload.pet_vitality = 5;
	raise_payload.pet_max_vitality = 5;
	raise_payload.pet_charm_duration = -1;
	auto raise_command = command(15, raise_payload);
	raise_command.accepted_at_usec = 15000000;
	setenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_IMAGE", "3", 1);
	applied = flatfile_corpse_repository_apply(raise_root.string(), raise_command);
	unsetenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_IMAGE");
	require(applied.outcome == critical_apply_outcome::retryable_failure &&
			applied.error_code == EIO,
		"interrupted corpse raise did not retain recoverable composite intent");
	applied = flatfile_corpse_repository_apply(raise_root.string(), raise_command);
	require(applied.outcome == critical_apply_outcome::already_applied,
		"corpse raise did not recover and replay exactly");
	result = {};
	require(corpse_lifecycle_command_decode_result(applied.result_payload.data(),
						       applied.result_size, &result) &&
			result.action == corpse_lifecycle_action::raise_follower &&
			result.catalog_revision == 2 && result.corpse_owner_revision == 2 &&
			result.room_owner_revision == 0 && result.player_owner_revision == 0 &&
			result.pet_owner_revision == 1 && result.wallet_revision == 1 &&
			result.max_item_revision == 2 && result.item_count == 2 &&
			result.wallet == std::array<int32_t, 4>{ 45, 36, 27, 18 } &&
			result.collector_catalog_changed,
		"corpse raise result did not expose all committed revisions");
	require_collector_listing(raise_root.string(), collector::state::cancelled,
				  collector::reason::claimed, 2, 2, &error);
	corpses.clear();
	saved.clear();
	require(flatfile_world_item_list(raise_root.string(), &corpses, &saved, &error) ==
				flatfile_world_item_result::ok &&
			corpses.empty(),
		"recovered corpse raise retained the corpse aggregate");
	rooms.clear();
	require(flatfile_world_item_list_rooms(raise_root.string(), &rooms, &error) ==
				flatfile_world_item_result::ok &&
			rooms.empty(),
		"recovered corpse raise unexpectedly changed room authority");
	uint64_t raising_player_revision = 0;
	std::vector<flatfile_item_ownership_record> raising_player_items;
	require(flatfile_item_repository_load_owner(
			raise_root.string(), raising_player_owner, &raising_player_revision,
			&raising_player_items, &error) == flatfile_item_repository_result::ok &&
			raising_player_revision == 1 && raising_player_items.size() == 1 &&
			raising_player_items[0].item_uid == 810,
		"recovered corpse raise changed player item custody");
	const item_owner_identity raised_pet_owner = { item_owner_type::pet, raise_payload.pet_uid,
						       71 };
	uint64_t raised_pet_revision = 0;
	std::vector<flatfile_item_ownership_record> raised_pet_items;
	require(flatfile_item_repository_load_owner(
			raise_root.string(), raised_pet_owner, &raised_pet_revision,
			&raised_pet_items, &error) == flatfile_item_repository_result::ok &&
			raised_pet_revision == 1 && raised_pet_items.size() == 2 &&
			raised_pet_items[0].item_uid == 900 &&
			raised_pet_items[0].item_revision == 2 &&
			raised_pet_items[1].parent_item_uid == 900,
		"recovered corpse raise did not transfer nested pet item custody");
	flatfile_player_domain_record loaded_raising_player;
	require(flatfile_player_domain_load(raise_root.string(), 71, "raising-account", 1,
					    &loaded_raising_player,
					    &error) == flatfile_player_domain_result::ok &&
			loaded_raising_player.domains.wallet_revision == 1 &&
			loaded_raising_player.domains.wallet ==
				std::array<uint64_t, 4>{ 45, 36, 27, 18 },
		"recovered corpse raise did not credit corpse money");
	flatfile_artifact_record raised_player_artifact;
	require(flatfile_artifact_get(raise_root.string(), 1901, &raised_player_artifact, &error) ==
				flatfile_artifact_result::ok &&
			raised_player_artifact.owned &&
			raised_player_artifact.location_type == FLATFILE_ARTIFACT_ON_PLAYER &&
			raised_player_artifact.location == 71 &&
			raised_player_artifact.bind_owner_pid == 42 &&
			raised_player_artifact.bind_timer == 6000 &&
			raised_player_artifact.last_update == 15 &&
			raised_player_artifact.revision == 2,
		"recovered corpse raise did not move the artifact without changing its binding");
	player_snapshot stale_raise_snapshot = {};
	stale_raise_snapshot.pid = 71;
	{
		flatfile_authority_lock reconciliation_lock;
		require(reconciliation_lock.acquire(raise_root.string(), &error),
			"could not lock raise materialization: " + error);
		require(flatfile_shop_trade_materialization_reconcile(
				raise_root.string(), reconciliation_lock, 71, raising_player_items,
				&stale_raise_snapshot,
				&error) == flatfile_shop_trade_materialization_result::ok &&
				stale_raise_snapshot.items.empty() &&
				stale_raise_snapshot.pets.size() == 1 &&
				stale_raise_snapshot.pets[0].pet_uid == raise_payload.pet_uid &&
				stale_raise_snapshot.pets[0].items.size() == 2 &&
				stale_raise_snapshot.pets[0].items[0].object_uid == 900 &&
				stale_raise_snapshot.pets[0].items[1].parent_index == 0,
			"restart reconciliation did not materialize raised corpse items: " + error);
		require(flatfile_shop_trade_materialization_reconcile(
				raise_root.string(), reconciliation_lock, 71, raising_player_items,
				&stale_raise_snapshot,
				&error) == flatfile_shop_trade_materialization_result::ok &&
				stale_raise_snapshot.pets.size() == 1 &&
				stale_raise_snapshot.pets[0].items.size() == 2,
			"repeat pet materialization duplicated the follower or its equipment");
	}
	const auto pet_transfer = [&](uint8_t seed, bool returning, uint64_t from_revision,
				      uint64_t to_revision, uint64_t item_revision,
				      const std::vector<player_item_snapshot> &items)
	{
		item_transfer_payload payload = {};
		payload.from_owner = returning ? raised_pet_owner : raising_player_owner;
		payload.to_owner = returning ? raising_player_owner : raised_pet_owner;
		payload.reason = returning ? item_transfer_reason::pet_return :
					     item_transfer_reason::pet_give;
		payload.reason_id = static_cast<int64_t>(raise_payload.pet_uid);
		payload.expected_from_revision = from_revision;
		payload.expected_to_revision = to_revision;
		payload.selected_item_uid = 900;
		payload.target_root_item_uid = 900;
		payload.item_count = 2;
		payload.items[0] = { 900, 900, 0, item_revision, 1900, item_custody_state::active };
		payload.items[1] = {
			901, 900, 900, item_revision, 1901, item_custody_state::active
		};
		std::vector<uint8_t> bytes;
		require(player_item_snapshot_list_encode(items, &bytes) ==
					player_snapshot_codec_result::ok &&
				!bytes.empty() && bytes.size() <= payload.item_blob.size(),
			"could not encode nested pet transfer");
		payload.item_blob_size = static_cast<uint32_t>(bytes.size());
		std::copy(bytes.begin(), bytes.end(), payload.item_blob.begin());
		critical_command transfer = {};
		require(item_transfer_command_build(&transfer, operation(seed), payload,
						    critical_source_site::command,
						    critical_deadline_class::interactive),
			"could not build nested pet transfer");
		transfer.accepted_at_usec = static_cast<uint64_t>(seed) * 1000;
		return transfer;
	};
	const auto return_command =
		pet_transfer(90, true, 1, 1, 2, stale_raise_snapshot.pets[0].items);
	auto pet_move = flatfile_item_repository_apply(raise_root.string(), return_command);
	require(pet_move.outcome == critical_apply_outcome::applied && !pet_move.error_code,
		"nested pet return did not commit: " + std::to_string(pet_move.error_code));
	require(flatfile_item_repository_apply(raise_root.string(), return_command).outcome ==
			critical_apply_outcome::already_applied,
		"nested pet return did not replay exactly once");
	require(flatfile_item_repository_load_owner(
			raise_root.string(), raising_player_owner, &raising_player_revision,
			&raising_player_items, &error) == flatfile_item_repository_result::ok &&
			raising_player_revision == 2 && raising_player_items.size() == 3,
		"nested pet return did not assign both UIDs to player custody");
	{
		flatfile_authority_lock lock;
		require(lock.acquire(raise_root.string(), &error),
			"could not lock pet return reconciliation: " + error);
		require(flatfile_shop_trade_materialization_reconcile(
				raise_root.string(), lock, 71, raising_player_items,
				&stale_raise_snapshot,
				&error) == flatfile_shop_trade_materialization_result::ok &&
				stale_raise_snapshot.items.size() == 2 &&
				stale_raise_snapshot.items[0].object_uid == 900 &&
				stale_raise_snapshot.items[1].parent_index == 0 &&
				stale_raise_snapshot.pets[0].items.empty(),
			"pet return did not reconcile nested graph to player exactly once: " +
				error);
	}
	const auto give_command = pet_transfer(91, false, 2, 2, 3, stale_raise_snapshot.items);
	pet_move = flatfile_item_repository_apply(raise_root.string(), give_command);
	require(pet_move.outcome == critical_apply_outcome::applied && !pet_move.error_code,
		"nested pet give did not commit: " + std::to_string(pet_move.error_code));
	require(flatfile_item_repository_apply(raise_root.string(), give_command).outcome ==
			critical_apply_outcome::already_applied,
		"nested pet give did not replay exactly once");
	require(flatfile_item_repository_load_owner(
			raise_root.string(), raising_player_owner, &raising_player_revision,
			&raising_player_items, &error) == flatfile_item_repository_result::ok &&
			raising_player_revision == 3 && raising_player_items.size() == 1,
		"nested pet give did not retire player custody");
	{
		flatfile_authority_lock lock;
		require(lock.acquire(raise_root.string(), &error),
			"could not lock pet give reconciliation: " + error);
		require(flatfile_shop_trade_materialization_reconcile(
				raise_root.string(), lock, 71, raising_player_items,
				&stale_raise_snapshot,
				&error) == flatfile_shop_trade_materialization_result::ok &&
				stale_raise_snapshot.items.empty() &&
				stale_raise_snapshot.pets[0].items.size() == 2 &&
				stale_raise_snapshot.pets[0].items[0].object_uid == 900 &&
				stale_raise_snapshot.pets[0].items[1].parent_index == 0,
			"pet give did not reconcile nested graph to follower exactly once: " +
				error);
	}

	const fs::path nested_room_root = fs::path(argv[1]) / "nested-room";
	prepare_root(nested_room_root);
	flatfile_corpse_record container_corpse = {};
	container_corpse.owner_pid = 88;
	container_corpse.owner_name = "carrier";
	container_corpse.save_id = 30;
	container_corpse.room_vnum = 500;
	container_corpse.values[3] = 88;
	container_corpse.values[5] = 1;
	container_corpse.values[6] = 30;
	container_corpse.revision = 1;
	container_corpse.items = {
		release_item(800, PLAYER_SNAPSHOT_NO_PARENT, 1800),
	};
	flatfile_corpse_record nested_room_corpse = released_corpse;
	nested_room_corpse.money = { 1, 2, 3, 4 };
	require(flatfile_world_item_establish(nested_room_root.string(),
					      { container_corpse, nested_room_corpse }, {},
					      &error) == flatfile_world_item_result::ok,
		"could not establish nested room fixtures: " + error);
	const item_owner_identity container_corpse_owner = { item_owner_type::corpse,
							     item_corpse_owner_id(88, 30), 0 };
	require(flatfile_item_repository_establish_owner(
			nested_room_root.string(), container_corpse_owner,
			{ { 800, 800, 0, container_corpse_owner, 1, 1800,
			    item_custody_state::active } },
			&error) == flatfile_item_baseline_result::applied,
		"could not establish nested room container custody: " + error);
	require(flatfile_item_repository_establish_owner(
			nested_room_root.string(), corpse_owner,
			{ { 900, 900, 0, corpse_owner, 1, 1900, item_custody_state::active },
			  { 901, 900, 900, corpse_owner, 1, 1901, item_custody_state::active } },
			&error) == flatfile_item_baseline_result::applied,
		"could not establish nested room corpse custody: " + error);
	require(flatfile_artifact_establish(nested_room_root.string(), { corpse_artifact },
					    &error) == flatfile_artifact_result::ok,
		"could not establish nested room artifact: " + error);
	seed_collector_candidate(nested_room_root.string(), nested_room_corpse, 74, &error);
	corpse_lifecycle_payload container_release = {};
	container_release.action = corpse_lifecycle_action::release;
	container_release.owner_pid = 88;
	container_release.save_id = 30;
	container_release.expected_corpse_revision = 1;
	container_release.expected_room_revision = 0;
	container_release.room_vnum = 500;
	container_release.owner_name = "Carrier";
	applied = flatfile_corpse_repository_apply(nested_room_root.string(),
						   command(20, container_release));
	require(applied.outcome == critical_apply_outcome::applied,
		"could not seed the nested room container");
	corpse_lifecycle_payload nested_room_payload = {};
	nested_room_payload.action = corpse_lifecycle_action::release_nested;
	nested_room_payload.owner_pid = 42;
	nested_room_payload.save_id = 20;
	nested_room_payload.expected_corpse_revision = 3;
	nested_room_payload.expected_room_revision = 1;
	nested_room_payload.room_vnum = 500;
	nested_room_payload.target_root_item_uid = 800;
	nested_room_payload.target_parent_item_uid = 800;
	nested_room_payload.expected_target_parent_revision = 2;
	nested_room_payload.owner_name = "Hero";
	auto nested_room_command = command(21, nested_room_payload);
	nested_room_command.accepted_at_usec = 21000000;
	setenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_IMAGE", "2", 1);
	applied = flatfile_corpse_repository_apply(nested_room_root.string(), nested_room_command);
	unsetenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_IMAGE");
	require(applied.outcome == critical_apply_outcome::retryable_failure &&
			applied.error_code == EIO,
		"interrupted nested room release did not retain recoverable intent");
	applied = flatfile_corpse_repository_apply(nested_room_root.string(), nested_room_command);
	require(applied.outcome == critical_apply_outcome::already_applied &&
			corpse_lifecycle_command_decode_result(applied.result_payload.data(),
							       applied.result_size, &result) &&
			result.action == corpse_lifecycle_action::release_nested &&
			result.room_owner_revision == 2 && result.item_count == 2 &&
			!result.collector_catalog_changed,
		"nested room release did not recover exactly");
	require_collector_listing(nested_room_root.string(), collector::state::candidate,
				  collector::reason::none, 1, 1, &error);
	require(flatfile_world_item_list(nested_room_root.string(), &corpses, &saved, &error) ==
				flatfile_world_item_result::ok &&
			corpses.empty(),
		"nested room release retained a corpse aggregate");
	require(flatfile_world_item_list_rooms(nested_room_root.string(), &rooms, &error) ==
				flatfile_world_item_result::ok &&
			rooms.size() == 1 && rooms[0].revision == 2 && rooms[0].items.size() == 3 &&
			rooms[0].items[0].object_uid == 800 &&
			rooms[0].items[1].parent_index == 0 &&
			rooms[0].items[2].parent_index == 1 &&
			rooms[0].money == std::array<int32_t, 4>{},
		"nested room release did not preserve the containing item topology");
	uint64_t nested_room_revision = 0;
	std::vector<flatfile_item_ownership_record> nested_room_items;
	require(flatfile_item_repository_load_owner(
			nested_room_root.string(), { item_owner_type::room, 500, 0 },
			&nested_room_revision, &nested_room_items,
			&error) == flatfile_item_repository_result::ok &&
			nested_room_revision == 2 && nested_room_items.size() == 3 &&
			nested_room_items[1].root_item_uid == 800 &&
			nested_room_items[1].parent_item_uid == 800 &&
			nested_room_items[2].root_item_uid == 800 &&
			nested_room_items[2].parent_item_uid == 900,
		"nested room release did not reparent durable custody");

	const fs::path nested_player_root = fs::path(argv[1]) / "nested-player";
	prepare_root(nested_player_root);
	flatfile_corpse_record nested_player_corpse = released_corpse;
	nested_player_corpse.money = { 4, 3, 2, 1 };
	require(flatfile_world_item_establish(nested_player_root.string(), { nested_player_corpse },
					      {}, &error) == flatfile_world_item_result::ok,
		"could not establish nested player corpse: " + error);
	require(flatfile_item_repository_establish_owner(
			nested_player_root.string(), corpse_owner,
			{ { 900, 900, 0, corpse_owner, 1, 1900, item_custody_state::active },
			  { 901, 900, 900, corpse_owner, 1, 1901, item_custody_state::active } },
			&error) == flatfile_item_baseline_result::applied,
		"could not establish nested player corpse custody: " + error);
	const item_owner_identity nested_player_owner = { item_owner_type::player, 72, 0 };
	require(flatfile_item_repository_establish_owner(
			nested_player_root.string(), nested_player_owner,
			{ { 800, 800, 0, nested_player_owner, 1, 1800,
			    item_custody_state::active } },
			&error) == flatfile_item_baseline_result::applied,
		"could not establish nested player container custody: " + error);
	flatfile_player_domain_record nested_player = {};
	nested_player.pid = 72;
	nested_player.account_name = "nested-account";
	nested_player.racewar = 1;
	nested_player.domains.wallet = { 5, 6, 7, 8 };
	require(flatfile_player_domain_establish(nested_player_root.string(), nested_player,
						 &error) == flatfile_player_domain_result::ok,
		"could not establish nested player wallet: " + error);
	require(flatfile_artifact_establish(nested_player_root.string(), { corpse_artifact },
					    &error) == flatfile_artifact_result::ok,
		"could not establish nested player artifact: " + error);
	seed_collector_candidate(nested_player_root.string(), nested_player_corpse, 75, &error);
	corpse_lifecycle_payload nested_player_payload = nested_room_payload;
	nested_player_payload.expected_room_revision = 0;
	nested_player_payload.destination_player_pid = 72;
	nested_player_payload.expected_player_revision = 1;
	nested_player_payload.expected_wallet_revision = 0;
	nested_player_payload.expected_target_parent_revision = 1;
	nested_player_payload.money = { 5, 6, 7, 8 };
	auto nested_player_command = command(22, nested_player_payload);
	nested_player_command.accepted_at_usec = 22000000;
	setenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_IMAGE", "3", 1);
	applied = flatfile_corpse_repository_apply(nested_player_root.string(),
						   nested_player_command);
	unsetenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_IMAGE");
	require(applied.outcome == critical_apply_outcome::retryable_failure &&
			applied.error_code == EIO,
		"interrupted nested player release did not retain recoverable intent: outcome=" +
			std::to_string(static_cast<unsigned int>(applied.outcome)) +
			" error=" + std::to_string(applied.error_code));
	applied = flatfile_corpse_repository_apply(nested_player_root.string(),
						   nested_player_command);
	require(applied.outcome == critical_apply_outcome::already_applied &&
			corpse_lifecycle_command_decode_result(applied.result_payload.data(),
							       applied.result_size, &result) &&
			result.action == corpse_lifecycle_action::release_nested &&
			result.player_owner_revision == 2 && result.wallet_revision == 1 &&
			result.wallet == std::array<int32_t, 4>{ 9, 9, 9, 9 } &&
			result.collector_catalog_changed,
		"nested player release did not recover exactly");
	require_collector_listing(nested_player_root.string(), collector::state::cancelled,
				  collector::reason::claimed, 2, 2, &error);
	uint64_t nested_player_revision = 0;
	std::vector<flatfile_item_ownership_record> nested_player_items;
	require(flatfile_item_repository_load_owner(
			nested_player_root.string(), nested_player_owner, &nested_player_revision,
			&nested_player_items, &error) == flatfile_item_repository_result::ok &&
			nested_player_revision == 2 && nested_player_items.size() == 3 &&
			nested_player_items[1].root_item_uid == 800 &&
			nested_player_items[1].parent_item_uid == 800 &&
			nested_player_items[2].root_item_uid == 800,
		"nested player release did not reparent durable custody");
	flatfile_player_domain_record loaded_nested_player;
	require(flatfile_player_domain_load(nested_player_root.string(), 72, "nested-account", 1,
					    &loaded_nested_player,
					    &error) == flatfile_player_domain_result::ok &&
			loaded_nested_player.domains.wallet ==
				std::array<uint64_t, 4>{ 9, 9, 9, 9 },
		"nested player release did not credit corpse money");
	player_snapshot stale_nested_player = {};
	stale_nested_player.pid = 72;
	stale_nested_player.items = {
		release_item(800, PLAYER_SNAPSHOT_NO_PARENT, 1800),
	};
	{
		flatfile_authority_lock reconciliation_lock;
		require(reconciliation_lock.acquire(nested_player_root.string(), &error),
			"could not lock nested player materialization: " + error);
		require(flatfile_shop_trade_materialization_reconcile(
				nested_player_root.string(), reconciliation_lock, 72,
				nested_player_items, &stale_nested_player,
				&error) == flatfile_shop_trade_materialization_result::ok &&
				stale_nested_player.items.size() == 3 &&
				stale_nested_player.items[1].parent_index == 0 &&
				stale_nested_player.items[2].parent_index == 1,
			"restart reconciliation did not preserve nested player topology: " + error);
	}

	const fs::path world_raise_root = fs::path(argv[1]) / "world-raise";
	prepare_root(world_raise_root);
	constexpr uint64_t world_corpse_uid = 9200;
	auto world_item = [](uint64_t uid, int32_t parent, int32_t vnum, int8_t type,
			     uint32_t wear_flags, uint32_t extra_flags, int32_t weight,
			     const char *name)
	{
		player_item_snapshot value = {};
		value.parent_index = parent;
		value.equipment_slot = -1;
		value.object_uid = uid;
		value.vnum = vnum;
		value.type = type;
		value.wear_flags = wear_flags;
		value.extra_flags = extra_flags;
		value.weight = weight;
		value.name = name;
		value.short_description = name;
		return value;
	};
	flatfile_saved_world_item_record world_corpse = {};
	world_corpse.item_key = "item.uid.9200";
	world_corpse.room_vnum = 501;
	world_corpse.revision = 1;
	world_corpse.items = {
		world_item(world_corpse_uid, PLAYER_SNAPSHOT_NO_PARENT, 2, ITEM_CORPSE, 0, 0, 12,
			   "an ordinary corpse"),
		world_item(9201, 0, 48, ITEM_CONTAINER, ITEM_TAKE, 0, 5, "an ordinary backpack"),
		world_item(9202, 1, 5, ITEM_ARMOR, 0, 0, 2, "a no-take token"),
		world_item(9203, 2, 5, ITEM_ARMOR, 0, 2050, 1, "a no-show token"),
		world_item(9204, 1, 5, ITEM_ARMOR, ITEM_TAKE, ITEM_TRANSIENT, 1, "a fading token"),
		world_item(9205, 0, 3, ITEM_MONEY, ITEM_TAKE, 0, 2, "some coins"),
	};
	require(flatfile_world_item_establish(world_raise_root.string(), {}, { world_corpse },
					      &error) == flatfile_world_item_result::ok,
		"could not establish equipped NPC corpse world graph: " + error);
	const item_owner_identity world_room_owner = { item_owner_type::room, 501, 0 };
	require(flatfile_item_repository_establish_owner(
			world_raise_root.string(), world_room_owner,
			{ { world_corpse_uid, world_corpse_uid, 0, world_room_owner, 1, 2,
			    item_custody_state::active },
			  { 9201, world_corpse_uid, world_corpse_uid, world_room_owner, 1, 48,
			    item_custody_state::active },
			  { 9202, world_corpse_uid, 9201, world_room_owner, 1, 5,
			    item_custody_state::active },
			  { 9203, world_corpse_uid, 9202, world_room_owner, 1, 5,
			    item_custody_state::active },
			  { 9204, world_corpse_uid, 9201, world_room_owner, 1, 5,
			    item_custody_state::active },
			  { 9205, world_corpse_uid, world_corpse_uid, world_room_owner, 1, 3,
			    item_custody_state::active } },
			&error) == flatfile_item_baseline_result::applied,
		"could not establish equipped NPC corpse custody: " + error);
	const item_owner_identity world_player_owner = { item_owner_type::player, 73, 0 };
	require(flatfile_item_repository_establish_owner(
			world_raise_root.string(), world_player_owner,
			{ { 9300, 9300, 0, world_player_owner, 1, 5, item_custody_state::active } },
			&error) == flatfile_item_baseline_result::applied,
		"could not establish NPC-corpse caster custody: " + error);
	corpse_lifecycle_payload world_raise_payload = {};
	world_raise_payload.action = corpse_lifecycle_action::raise_world_follower;
	world_raise_payload.owner_pid = 0;
	world_raise_payload.save_id = static_cast<uint32_t>(world_corpse_uid);
	world_raise_payload.expected_corpse_revision = 1;
	world_raise_payload.expected_room_revision = 1;
	world_raise_payload.destination_player_pid = 73;
	world_raise_payload.expected_player_revision = 1;
	world_raise_payload.room_vnum = 501;
	world_raise_payload.owner_name = "ordinary npc corpse";
	world_raise_payload.pet_uid = world_corpse_uid;
	world_raise_payload.pet_mob_vnum = 701;
	world_raise_payload.pet_hit = world_raise_payload.pet_max_hit = 20;
	world_raise_payload.pet_mana = world_raise_payload.pet_max_mana = 10;
	world_raise_payload.pet_vitality = world_raise_payload.pet_max_vitality = 5;
	auto world_raise_command = command(30, world_raise_payload);
	world_raise_command.accepted_at_usec = 30000000;
	setenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_IMAGE", "2", 1);
	applied = flatfile_corpse_repository_apply(world_raise_root.string(), world_raise_command);
	unsetenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_IMAGE");
	require(applied.outcome == critical_apply_outcome::retryable_failure &&
			applied.error_code == EIO,
		"interrupted equipped NPC-corpse raise did not retain recoverable intent");
	applied = flatfile_corpse_repository_apply(world_raise_root.string(), world_raise_command);
	result = {};
	require(applied.outcome == critical_apply_outcome::already_applied &&
			corpse_lifecycle_command_decode_result(applied.result_payload.data(),
							       applied.result_size, &result) &&
			result.action == corpse_lifecycle_action::raise_world_follower &&
			result.catalog_revision == 2 && result.corpse_owner_revision == 5 &&
			result.room_owner_revision == 0 && result.player_owner_revision == 0 &&
			result.pet_owner_revision == 1 && result.wallet_revision == 0 &&
			result.max_item_revision == 3 && result.item_count == 3 &&
			result.destruction_owner_revision == 1 &&
			result.max_discarded_item_revision == 3 && result.discarded_item_count == 3,
		"equipped NPC-corpse raise did not expose the split custody revisions");
	corpses.clear();
	saved.clear();
	require(flatfile_world_item_list(world_raise_root.string(), &corpses, &saved, &error) ==
				flatfile_world_item_result::ok &&
			saved.empty(),
		"equipped NPC-corpse raise retained the saved world graph");
	const item_owner_identity world_pet_owner = { item_owner_type::pet, world_corpse_uid, 73 };
	uint64_t world_pet_revision = 0;
	std::vector<flatfile_item_ownership_record> world_pet_items;
	require(flatfile_item_repository_load_owner(
			world_raise_root.string(), world_pet_owner, &world_pet_revision,
			&world_pet_items, &error) == flatfile_item_repository_result::ok &&
			world_pet_revision == 1 && world_pet_items.size() == 3 &&
			world_pet_items[0].item_uid == 9201 &&
			world_pet_items[0].root_item_uid == 9201 &&
			world_pet_items[0].parent_item_uid == 0 &&
			world_pet_items[0].item_revision == 3 &&
			world_pet_items[1].root_item_uid == 9201 &&
			world_pet_items[1].parent_item_uid == 9201 &&
			world_pet_items[2].parent_item_uid == 9202,
		"ordinary, no-take, and no-show gear did not enter nested pet custody");
	const item_owner_identity destruction_owner = { item_owner_type::destruction, 0, 0 };
	uint64_t world_destruction_revision = 0;
	std::vector<flatfile_item_ownership_record> destroyed_world_items;
	require(flatfile_item_repository_load_owner(
			world_raise_root.string(), destruction_owner, &world_destruction_revision,
			&destroyed_world_items, &error) == flatfile_item_repository_result::ok &&
			world_destruction_revision == 1 && destroyed_world_items.empty(),
		"NPC corpse root, money, and transient item were not retired together");
	{
		flatfile_authority_lock destruction_lock;
		require(destruction_lock.acquire(world_raise_root.string(), &error),
			"could not lock NPC-corpse tombstones: " + error);
		require(flatfile_item_repository_load_coins_locked(
				world_raise_root.string(), destruction_lock,
				{ world_corpse_uid, 9204, 9205 }, &destroyed_world_items,
				&error) == flatfile_item_repository_result::ok &&
				destroyed_world_items.size() == 3 &&
				std::all_of(destroyed_world_items.begin(),
					    destroyed_world_items.end(), [](const auto &item)
					    { return item.state == item_custody_state::destroyed; }),
			"NPC-corpse tombstones did not retain destroyed custody");
	}
	uint64_t world_player_revision = 0;
	std::vector<flatfile_item_ownership_record> world_player_items;
	require(flatfile_item_repository_load_owner(
			world_raise_root.string(), world_player_owner, &world_player_revision,
			&world_player_items, &error) == flatfile_item_repository_result::ok &&
			world_player_revision == 1 && world_player_items.size() == 1,
		"equipped NPC-corpse raise leaked equipment to the caster");
	player_snapshot stale_world_raise = {};
	stale_world_raise.pid = 73;
	{
		flatfile_authority_lock reconciliation_lock;
		require(reconciliation_lock.acquire(world_raise_root.string(), &error),
			"could not lock equipped NPC-corpse materialization: " + error);
		require(flatfile_shop_trade_materialization_reconcile(
				world_raise_root.string(), reconciliation_lock, 73,
				world_player_items, &stale_world_raise,
				&error) == flatfile_shop_trade_materialization_result::ok &&
				stale_world_raise.items.empty() &&
				stale_world_raise.pets.size() == 1 &&
				stale_world_raise.pets[0].pet_uid == world_corpse_uid &&
				stale_world_raise.pets[0].items.size() == 3 &&
				stale_world_raise.pets[0].items[0].parent_index ==
					PLAYER_SNAPSHOT_NO_PARENT &&
				stale_world_raise.pets[0].items[1].parent_index == 0 &&
				stale_world_raise.pets[0].items[2].parent_index == 1 &&
				stale_world_raise.pets[0].items[1].wear_flags == 0 &&
				stale_world_raise.pets[0].items[2].extra_flags == 2050,
			"restart reconciliation did not preserve restricted nested NPC gear: " +
				error);
	}

	const fs::path hostile_world_raise_root = fs::path(argv[1]) / "hostile-world-raise";
	prepare_root(hostile_world_raise_root);
	constexpr uint64_t hostile_corpse_uid = 9400;
	flatfile_saved_world_item_record hostile_world_corpse = {};
	hostile_world_corpse.item_key = "item.uid.9400";
	hostile_world_corpse.room_vnum = 502;
	hostile_world_corpse.revision = 1;
	hostile_world_corpse.items = {
		world_item(hostile_corpse_uid, PLAYER_SNAPSHOT_NO_PARENT, 2, ITEM_CORPSE, 0, 0, 10,
			   "a hostile corpse"),
		world_item(9401, 0, 5, ITEM_ARMOR, 0, 0, 2, "some no-take armor"),
	};
	require(flatfile_world_item_establish(hostile_world_raise_root.string(), {},
					      { hostile_world_corpse },
					      &error) == flatfile_world_item_result::ok,
		"could not establish hostile equipped NPC corpse: " + error);
	const item_owner_identity hostile_room_owner = { item_owner_type::room, 502, 0 };
	require(flatfile_item_repository_establish_owner(
			hostile_world_raise_root.string(), hostile_room_owner,
			{ { hostile_corpse_uid, hostile_corpse_uid, 0, hostile_room_owner, 1, 2,
			    item_custody_state::active },
			  { 9401, hostile_corpse_uid, hostile_corpse_uid, hostile_room_owner, 1, 5,
			    item_custody_state::active } },
			&error) == flatfile_item_baseline_result::applied,
		"could not establish hostile NPC-corpse custody: " + error);
	const item_owner_identity hostile_player_owner = { item_owner_type::player, 74, 0 };
	require(flatfile_item_repository_establish_owner(
			hostile_world_raise_root.string(), hostile_player_owner,
			{ { 9500, 9500, 0, hostile_player_owner, 1, 5,
			    item_custody_state::active } },
			&error) == flatfile_item_baseline_result::applied,
		"could not establish hostile NPC-corpse caster custody: " + error);
	corpse_lifecycle_payload hostile_world_raise = {};
	hostile_world_raise.action = corpse_lifecycle_action::raise_world_follower;
	hostile_world_raise.save_id = static_cast<uint32_t>(hostile_corpse_uid);
	hostile_world_raise.expected_corpse_revision = 1;
	hostile_world_raise.expected_room_revision = 1;
	hostile_world_raise.destination_player_pid = 74;
	hostile_world_raise.expected_player_revision = 1;
	hostile_world_raise.room_vnum = 502;
	hostile_world_raise.owner_name = "hostile npc corpse";
	auto hostile_world_command = command(31, hostile_world_raise);
	hostile_world_command.accepted_at_usec = 31000000;
	applied = flatfile_corpse_repository_apply(hostile_world_raise_root.string(),
						   hostile_world_command);
	result = {};
	require(applied.outcome == critical_apply_outcome::applied &&
			corpse_lifecycle_command_decode_result(applied.result_payload.data(),
							       applied.result_size, &result) &&
			result.action == corpse_lifecycle_action::raise_world_follower &&
			result.catalog_revision == 2 && result.corpse_owner_revision == 2 &&
			result.pet_owner_revision == 0 && result.item_count == 0 &&
			result.max_item_revision == 0 && result.destruction_owner_revision == 1 &&
			result.max_discarded_item_revision == 2 && result.discarded_item_count == 2,
		"hostile NPC-corpse raise did not retire its complete graph");
	corpses.clear();
	saved.clear();
	require(flatfile_world_item_list(hostile_world_raise_root.string(), &corpses, &saved,
					 &error) == flatfile_world_item_result::ok &&
			saved.empty(),
		"hostile NPC-corpse raise retained the saved world graph");
	world_destruction_revision = 0;
	destroyed_world_items.clear();
	require(flatfile_item_repository_load_owner(hostile_world_raise_root.string(),
						    destruction_owner, &world_destruction_revision,
						    &destroyed_world_items, &error) ==
				flatfile_item_repository_result::ok &&
			world_destruction_revision == 1 && destroyed_world_items.empty(),
		"hostile NPC-corpse raise did not advance destruction custody");
	{
		flatfile_authority_lock destruction_lock;
		require(destruction_lock.acquire(hostile_world_raise_root.string(), &error),
			"could not lock hostile NPC-corpse tombstones: " + error);
		require(flatfile_item_repository_load_coins_locked(
				hostile_world_raise_root.string(), destruction_lock,
				{ hostile_corpse_uid, 9401 }, &destroyed_world_items,
				&error) == flatfile_item_repository_result::ok &&
				destroyed_world_items.size() == 2 &&
				std::all_of(destroyed_world_items.begin(),
					    destroyed_world_items.end(), [](const auto &item)
					    { return item.state == item_custody_state::destroyed; }),
			"hostile NPC-corpse graph did not retain destroyed tombstones");
	}
	applied = flatfile_corpse_repository_apply(hostile_world_raise_root.string(),
						   hostile_world_command);
	require(applied.outcome == critical_apply_outcome::already_applied,
		"hostile NPC-corpse replay was not idempotent");
	std::cout << "flat-file corpse lifecycle repository passed\n";
	return 0;
}
