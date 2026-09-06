#include "flatfile/flatfile_player_repository.h"
#include "flatfile/flatfile_identity_repository.h"
#include "flatfile/flatfile_item_repository.h"
#include "flatfile/flatfile_player_domain_repository.h"
#include "persistence/persistence_observability.h"
#include "economy/coin_transfer_command.h"
#include "player/player_snapshot_codec.h"
#include "core/defines.h"
#include "world/vnum.obj.h"
#include <algorithm>
#include <cstring>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <sys/wait.h>
#include <unistd.h>

namespace fs = std::filesystem;

bool player_load_request_valid(const player_load_request &request, uint64_t now)
{
	const bool pid_identity = request.pid > 0 && !request.account_name.empty() &&
				  request.account_name.size() <= PLAYER_LOAD_ACCOUNT_MAX;
	const bool name_identity = request.pid == 0 && !request.player_name.empty() &&
				   request.player_name.size() <= PLAYER_LOAD_NAME_MAX;
	return request.schema_version == PLAYER_LOAD_SCHEMA_VERSION && request.request_id > 0 &&
	       (pid_identity || name_identity) && request.deadline_usec > now &&
	       request.deadline_usec - now <= PLAYER_LOAD_TIMEOUT_USEC &&
	       (!request.include_pets || request.include_items);
}

static void require(bool condition, const std::string &message)
{
	if (!condition)
	{
		std::cerr << message << '\n';
		exit(1);
	}
}

static player_snapshot make_full(player_revision_t revision)
{
	player_snapshot snapshot = {};
	snapshot.schema_version = PLAYER_SNAPSHOT_SCHEMA_VERSION;
	snapshot.pid = 42;
	snapshot.revision = revision;
	snapshot.components = PLAYER_CHECKPOINT_COMPONENT_ALL;
	snapshot.save_intent = 4;
	snapshot.room_vnum = 1201;
	snapshot.encoded_size_bound = 8192;
	snapshot.status_integers.push_back({ player_status_field::level, 50, 0, false });
	snapshot.status_integers.push_back({ player_status_field::racewar, 0, 0, false });
	snapshot.status_integers.push_back({ player_status_field::copper, 11, 0, false });
	snapshot.status_integers.push_back({ player_status_field::silver, 12, 0, false });
	snapshot.status_integers.push_back({ player_status_field::gold, 13, 0, false });
	snapshot.status_integers.push_back({ player_status_field::platinum, 14, 0, false });
	snapshot.status_integers.push_back({ player_status_field::epics, 15, 0, false });
	snapshot.status_integers.push_back({ player_status_field::frags, 16, 0, false });
	snapshot.status_integers.push_back({ player_status_field::old_frags, 17, 0, false });
	for (int index = 0; index < 10; ++index)
		snapshot.status_integers.push_back(
			{ static_cast<player_status_field>(
				  static_cast<unsigned int>(player_status_field::base_strength) +
				  index),
			  50 + index, 0, false });
	snapshot.status_strings.push_back({ player_status_string_field::name, "Player" });
	snapshot.conditions = { 1, 2, 3, 4, 5 };
	snapshot.quest_values[3] = 77;
	snapshot.languages.push_back({ 1, 90, 0 });
	snapshot.introductions.push_back({ 2, 44, 12345 });
	snapshot.timers.push_back({ 3, 67890, 0 });
	snapshot.undead_slots.push_back({ 4, 2, 0 });
	snapshot.forged_items.push_back({ 5, 6001, 0 });
	snapshot.granted_commands.push_back(42);
	snapshot.skills.push_back({ 9, 80, 1 });
	player_affect_snapshot affect = {};
	affect.type = 11;
	affect.duration = 12;
	affect.bitvectors[2] = 99;
	affect.wear_off_character = "gone";
	snapshot.affects.push_back(affect);
	player_item_snapshot parent = {};
	parent.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	parent.object_uid = 100;
	parent.vnum = 500;
	parent.string_mask = 1;
	parent.name = "container";
	parent.values[0] = 8;
	parent.dynamic_affects.push_back({ 1, 2, 3 });
	player_item_extra_description_snapshot description = {};
	description.keyword = "SPELLBOOK";
	description.spellbook = true;
	description.spell_ids = { 7, 12 };
	parent.extra_descriptions.push_back(description);
	snapshot.items.push_back(parent);
	player_item_snapshot child = {};
	child.parent_index = 0;
	child.object_uid = 101;
	child.vnum = 501;
	snapshot.items.push_back(child);
	player_pet_snapshot pet = {};
	pet.mob_vnum = 700;
	pet.room_vnum = 1201;
	pet.items.push_back(child);
	pet.items[0].parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	pet.items[0].object_uid = 102;
	snapshot.pets.push_back(pet);
	snapshot.shapes.push_back({ 800, 2, 100, 200 });
	snapshot.trophies.push_back({ 12, 300 });
	snapshot.recipes_are_external = true;
	return snapshot;
}

/** Create a minimal status-only snapshot with the revision, level, and room under test. */
static player_snapshot make_status(player_revision_t revision, int level, int room)
{
	player_snapshot snapshot = {};
	snapshot.schema_version = PLAYER_SNAPSHOT_SCHEMA_VERSION;
	snapshot.pid = 42;
	snapshot.revision = revision;
	snapshot.components = PLAYER_COMPONENT_STATUS;
	snapshot.save_intent = 1;
	snapshot.room_vnum = room;
	snapshot.encoded_size_bound = 1024;
	snapshot.status_integers.push_back({ player_status_field::level, level, 0, false });
	snapshot.status_strings.push_back({ player_status_string_field::name, "Player" });
	snapshot.recipes_are_external = true;
	return snapshot;
}

// Read existing synthetic authority without recovery or mutation. Used by the
// full-world journey to compare item identities across real server restarts.
static void inspect_authority(const std::string &root, int32_t pid)
{
	std::string error;
	player_snapshot snapshot;
	require(flatfile_player_snapshot_load(root, pid, &snapshot, &error) ==
			flatfile_player_load_result::ok,
		"inspect player snapshot: " + error);
	flatfile_authority_lock lock;
	require(lock.acquire(root, &error), "inspect authority lock: " + error);
	std::cout << "{\"revision\":" << snapshot.revision << ",\"intent\":" << snapshot.save_intent
		  << ",\"room\":" << snapshot.room_vnum;
	for (const auto &field : snapshot.status_integers)
		if (field.field == player_status_field::wimpy)
			std::cout
				<< ",\"wimpy\":"
				<< (field.is_unsigned ? field.unsigned_value : field.signed_value);
	for (const auto &owner :
	     { item_owner_identity{ item_owner_type::player, static_cast<uint64_t>(pid), 0 },
	       item_owner_identity{ item_owner_type::room,
				    static_cast<uint64_t>(snapshot.room_vnum), 0 } })
	{
		uint64_t revision = 0;
		std::vector<flatfile_item_ownership_record> items;
		const auto result = flatfile_item_repository_load_owner_locked(
			root, lock, owner, &revision, &items, &error);
		require(result == flatfile_item_repository_result::ok ||
				result == flatfile_item_repository_result::not_found,
			"inspect item owner: " + error);
		std::cout << (owner.type == item_owner_type::player ? ",\"player_items\":[" :
								      ",\"room_items\":[");
		bool first = true;
		for (const auto &item : items)
		{
			if (!first)
				std::cout << ',';
			first = false;
			std::cout << "{\"uid\":" << item.item_uid << ",\"vnum\":" << item.vnum
				  << '}';
		}
		std::cout << ']';
	}
	std::cout << "}\n";
}

static void coin_player_matrix(const fs::path &path)
{
	const std::string root = path.string();
	for (const auto &directory : { path, path / "players", path / "domains",
				       path / "identities", path / "identities/names" })
	{
		fs::create_directories(directory);
		fs::permissions(directory, fs::perms::owner_all, fs::perm_options::replace);
	}
	std::string error;
	int32_t pid = 0;
	for (int32_t i = 1; i <= 42; ++i)
		require(flatfile_identity_allocate_pid(root, &pid, &error) ==
				flatfile_identity_result::ok,
			"coin player identity allocation");
	require(flatfile_identity_claim(root, 42, "Player", "Account-One", &error) ==
			flatfile_identity_result::ok,
		"coin player identity claim");
	auto snapshot = make_full(1);
	snapshot.items[1].vnum = VOBJ_COINS;
	snapshot.items[1].type = ITEM_MONEY;
	snapshot.items[1].values[0] = 50;
	snapshot.items[1].name = "legacy coins";
	snapshot.items[1].string_mask = 1;
	require(flatfile_player_snapshot_apply(root, snapshot, &error).outcome ==
			player_save_apply_outcome::applied,
		"coin player snapshot baseline: " + error);
	const item_owner_identity owner = { item_owner_type::player, 42, 0 };
	for (int32_t before : { 50, 20 })
	{
		const int32_t after = before == 50 ? 20 : 0;
		flatfile_player_domain_record domain;
		require(flatfile_player_domain_load(root, 42, "Account-One", 0, &domain, &error) ==
				flatfile_player_domain_result::ok,
			"coin player domain load");
		uint64_t owner_revision = 0, destroyed_revision = 0;
		std::vector<flatfile_item_ownership_record> owned, destroyed;
		require(flatfile_item_repository_load_owner(root, owner, &owner_revision, &owned,
							    &error) ==
				flatfile_item_repository_result::ok,
			"coin player custody load");
		flatfile_item_repository_load_owner(root, { item_owner_type::destruction, 0, 0 },
						    &destroyed_revision, &destroyed, &error);
		const auto found = std::find_if(owned.begin(), owned.end(), [](const auto &item)
						{ return item.item_uid == 101; });
		require(found != owned.end(), "coin player pile custody missing");
		coin_transfer_payload payload;
		payload.source.before[0] = before;
		payload.source.after[0] = after;
		item_transfer_payload pile = {};
		pile.from_owner = owner;
		pile.to_owner = after ? owner :
					item_owner_identity{ item_owner_type::destruction, 0, 0 };
		pile.expected_from_revision = owner_revision;
		pile.expected_to_revision = after ? owner_revision : destroyed_revision;
		pile.reason = after ? item_transfer_reason::player_put :
				      item_transfer_reason::destruction;
		pile.selected_item_uid = 101;
		pile.target_root_item_uid = after ? 100 : 101;
		pile.target_parent_item_uid = after ? 100 : 0;
		pile.expected_target_parent_revision = after ? 1 : 0;
		pile.item_count = 1;
		pile.items[0] = { 101,	      100,
				  100,	      found->item_revision,
				  VOBJ_COINS, item_custody_state::active };
		auto item = snapshot.items[1];
		item.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
		item.equipment_slot = -1;
		item.values[0] = after ? after : before;
		std::vector<uint8_t> blob;
		require(player_item_snapshot_list_encode({ item }, &blob) ==
				player_snapshot_codec_result::ok,
			"coin player encode");
		pile.item_blob_size = blob.size();
		std::copy(blob.begin(), blob.end(), pile.item_blob.begin());
		critical_operation_id id;
		require(critical_operation_id_generate(&id), "coin player operation id");
		require(item_transfer_command_build(&payload.source.change, id, pile,
						    critical_source_site::command,
						    critical_deadline_class::interactive),
			"coin player pile command");
		currency_command_payload currency = {};
		currency.pid = 42;
		currency.reason = currency_reason_type::coin_transfer;
		strcpy(currency.account_name.data(), "Account-One");
		currency.wallet_delta.amount[0] = before - after;
		for (size_t denomination = 0; denomination < 4; ++denomination)
			payload.destination.before[denomination] =
				payload.destination.after[denomination] =
					domain.domains.wallet[denomination];
		payload.destination.after[0] += before - after;
		require(currency_command_build(&payload.destination.change, id, currency,
					       domain.domains.wallet_revision,
					       domain.domains.bank_revision,
					       critical_source_site::command,
					       critical_deadline_class::interactive),
			"coin player wallet command");
		critical_command command;
		require(coin_transfer_command_build(&command, id, payload,
						    critical_source_site::command,
						    critical_deadline_class::interactive),
			"coin player command");
		command.accepted_at_usec = 1;
		auto apply = [&]()
		{
			return flatfile_critical_command_repository_apply_selected(
				command, const_cast<char *>(root.c_str()));
		};
		const auto applied = apply();
		require(applied.outcome == critical_apply_outcome::applied &&
				apply().outcome == critical_apply_outcome::already_applied,
			"coin player pickup failed: " + std::to_string(applied.error_code));
		// Saving an old projection cannot undo either the durable remainder or retirement.
		++snapshot.revision;
		require(flatfile_player_snapshot_apply(root, snapshot, &error).outcome ==
				player_save_apply_outcome::applied,
			"coin stale snapshot write");
		player_load_request request = {};
		request.request_id = 1;
		request.pid = 42;
		request.account_name = "Account-One";
		request.deadline_usec =
			persistence_observability_now_usec() + PLAYER_LOAD_TIMEOUT_USEC;
		const auto loaded = flatfile_player_load_repository_execute(root, request);
		require(loaded.outcome == player_load_outcome::applied && !loaded.stale_item_rows &&
				loaded.domains.wallet[0] == static_cast<uint64_t>(61 - after) &&
				loaded.snapshot.items.size() == (after ? 2u : 1u) &&
				loaded.snapshot.pets[0].items.size() == 1,
			"coin player re-entry failed or restored old money: " +
				std::to_string(loaded.error_code));
		if (after)
			require(loaded.snapshot.items[1].object_uid == 101 &&
					loaded.snapshot.items[1].values[0] == after &&
					loaded.snapshot.items[1].parent_index == 0,
				"coin player re-entry lost authoritative remainder/topology");
	}
	std::cout << "flatfile legacy player coin pickup, replay and re-entry passed\n";
}

/** Inspect synthetic authority on request, otherwise exercise player repository durability and recovery. */
int main(int argc, char **argv)
{
	if (argc == 4 && std::string(argv[2]) == "inspect")
	{
		inspect_authority(argv[1], std::stoi(argv[3]));
		return 0;
	}
	require(argc == 2, "state root argument required");
	const fs::path root = argv[1];
	coin_player_matrix(root / "coin-player");
	const fs::path players = root / "players";
	const fs::path identities = root / "identities/names";
	const fs::path domains = root / "domains";
	fs::create_directories(players);
	fs::create_directories(identities);
	fs::create_directories(domains);
	fs::permissions(root, fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(players, fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(root / "identities", fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(identities, fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(domains, fs::perms::owner_all, fs::perm_options::replace);

	std::string error;
	int32_t allocated_pid = 0;
	for (int32_t expected_pid = 1; expected_pid <= 42; ++expected_pid)
		require(flatfile_identity_allocate_pid(root.string(), &allocated_pid, &error) ==
					flatfile_identity_result::ok &&
				allocated_pid == expected_pid,
			"could not allocate player PID: " + error);
	require(flatfile_identity_claim(root.string(), 42, "Player", "Account-One", &error) ==
			flatfile_identity_result::ok,
		"could not claim player identity: " + error);
	player_save_apply_result applied =
		flatfile_player_snapshot_apply(root.string(), make_status(1, 10, 100), &error);
	require(applied.outcome == player_save_apply_outcome::terminal_failure &&
			applied.error_code == ENOENT,
		"partial snapshot created a missing player baseline");
	player_snapshot full = make_full(1);
	applied = flatfile_player_snapshot_apply(root.string(), full, &error);
	require(applied.outcome == player_save_apply_outcome::applied &&
			applied.durable_revision == 1,
		"full baseline apply failed: " + error);
	player_snapshot loaded;
	require(flatfile_player_snapshot_load(root.string(), 42, &loaded, &error) ==
				flatfile_player_load_result::ok &&
			loaded.components == PLAYER_CHECKPOINT_COMPONENT_ALL &&
			loaded.revision == 1 && loaded.status_integers[0].signed_value == 50 &&
			loaded.languages[0].value == 90 && loaded.items.size() == 2 &&
			loaded.items[1].parent_index == 0 &&
			loaded.items[0].extra_descriptions[0].spell_ids[1] == 12 &&
			loaded.pets[0].items[0].vnum == 501 && loaded.shapes[0].mob_vnum == 800 &&
			loaded.trophies[0].experience == 300,
		"full player snapshot did not round trip: " + error);
	player_load_request load_request = {};
	load_request.request_id = 1;
	load_request.pid = 42;
	load_request.account_name = "account-one";
	load_request.deadline_usec =
		persistence_observability_now_usec() + PLAYER_LOAD_TIMEOUT_USEC;
	player_load_result load_result =
		flatfile_player_load_repository_execute(root.string(), load_request);
	require(load_result.pid == 42 && load_result.snapshot.revision == 1 &&
			load_result.item_owner_revision == 1 &&
			load_result.item_identities.size() == 2 &&
			load_result.item_identities[1].parent_item_uid == 100 &&
			load_result.pet_identities.size() == 1 &&
			load_result.pet_identities[0].item_identities.size() == 1 &&
			load_result.domains.wallet == std::array<uint64_t, 4>{ 11, 12, 13, 14 } &&
			load_result.domains.bank_revision == 1 && load_result.domains.epics == 15 &&
			load_result.domains.frags == 16 && load_result.domains.old_frags == 17 &&
			load_result.domains.base_stat_revision == 1 &&
			load_result.domains.base_stats[0] == 50 &&
			load_result.domains.base_stats[9] == 59 &&
			load_result.read_components == PLAYER_LOAD_SESSION04_READS &&
			load_result.outcome == player_load_outcome::applied &&
			load_result.error_code == 0 && !load_result.failed_component,
		"verified snapshot/domain load was not reported as materializable");
	load_request.request_id = 2;
	load_request.account_name = "wrong-account";
	load_result = flatfile_player_load_repository_execute(root.string(), load_request);
	require(load_result.outcome == player_load_outcome::component_failure &&
			load_result.error_code == EACCES &&
			std::string(load_result.failed_component) == "identity",
		"account/PID mismatch was accepted");
	load_request = {};
	load_request.request_id = 3;
	load_request.player_name = "pLaYeR";
	load_request.deadline_usec =
		persistence_observability_now_usec() + PLAYER_LOAD_TIMEOUT_USEC;
	load_result = flatfile_player_load_repository_execute(root.string(), load_request);
	require(load_result.pid == 42 && load_result.account_name == "Account-One" &&
			load_result.outcome == player_load_outcome::applied,
		"canonical name lookup did not resolve the snapshot identity");
	load_request.deadline_usec = persistence_observability_now_usec();
	load_result = flatfile_player_load_repository_execute(root.string(), load_request);
	require(load_result.outcome == player_load_outcome::timed_out &&
			load_result.error_code == ETIMEDOUT,
		"expired flat-file load request was accepted");
	applied = flatfile_player_snapshot_apply(root.string(), full, &error);
	require(applied.outcome == player_save_apply_outcome::already_applied &&
			applied.durable_revision == 1,
		"duplicate revision was not idempotent");

	applied = flatfile_player_snapshot_apply(root.string(), make_status(2, 51, 1202), &error);
	require(applied.outcome == player_save_apply_outcome::applied &&
			applied.durable_revision == 2,
		"partial status merge failed: " + error);
	require(flatfile_player_snapshot_load(root.string(), 42, &loaded, &error) ==
				flatfile_player_load_result::ok &&
			loaded.revision == 2 && loaded.room_vnum == 1202 &&
			loaded.status_integers[0].signed_value == 51 && loaded.items.size() == 2 &&
			loaded.languages[0].value == 90,
		"partial status merge discarded an untouched component");
	applied = flatfile_player_snapshot_apply(root.string(), full, &error);
	require(applied.outcome == player_save_apply_outcome::stale_revision &&
			applied.durable_revision == 2,
		"stale player revision was accepted");

	player_snapshot torn_items = {};
	torn_items.schema_version = PLAYER_SNAPSHOT_SCHEMA_VERSION;
	torn_items.pid = 42;
	torn_items.revision = 3;
	torn_items.components = PLAYER_COMPONENT_INVENTORY;
	torn_items.encoded_size_bound = 256;
	applied = flatfile_player_snapshot_apply(root.string(), torn_items, &error);
	require(applied.outcome == player_save_apply_outcome::terminal_failure &&
			applied.error_code == EINVAL,
		"one-sided item component replacement was accepted");

	for (player_revision_t revision : { 4U, 5U })
	{
		const pid_t child_process = fork();
		require(child_process >= 0, "player writer fork failed");
		if (!child_process)
		{
			std::string child_error;
			const player_save_apply_result child_result =
				flatfile_player_snapshot_apply(root.string(),
							       make_status(revision, 50 + revision,
									   1200 + revision),
							       &child_error);
			_exit(child_result.outcome == player_save_apply_outcome::applied ||
					      child_result.outcome ==
						      player_save_apply_outcome::stale_revision ?
				      0 :
				      2);
		}
	}
	for (int child = 0; child < 2; ++child)
	{
		int status = 0;
		require(wait(&status) > 0 && WIFEXITED(status) && WEXITSTATUS(status) == 0,
			"concurrent player writer failed");
	}
	require(flatfile_player_snapshot_load(root.string(), 42, &loaded, &error) ==
				flatfile_player_load_result::ok &&
			loaded.revision == 5 && loaded.status_integers[0].signed_value == 55,
		"concurrent player writers lost the highest revision");

	// The ownership catalog is written once, at baseline, so a later save can leave the
	// two files disagreeing. None of these disagreements may lock the character out.
	auto reload = [&](const player_snapshot &snapshot, uint64_t request_id)
	{
		require(flatfile_player_snapshot_apply(root.string(), snapshot, &error).outcome ==
				player_save_apply_outcome::applied,
			"item-consistency fixture save failed: " + error);
		player_load_request items_request = {};
		items_request.request_id = request_id;
		items_request.pid = 42;
		items_request.account_name = "account-one";
		items_request.deadline_usec =
			persistence_observability_now_usec() + PLAYER_LOAD_TIMEOUT_USEC;
		return flatfile_player_load_repository_execute(root.string(), items_request);
	};
	player_item_snapshot orphan = {};
	orphan.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	orphan.object_uid = 103;
	orphan.vnum = 503;

	// A stale projection that still shows an authoritative child at top level heals
	// back into its container instead of refusing the character.
	player_snapshot stale_parent = make_full(6);
	stale_parent.items[1].parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	player_load_result recovered = reload(stale_parent, 9);
	require(recovered.outcome == player_load_outcome::applied &&
			recovered.repaired_item_rows == 1 &&
			recovered.snapshot.items[1].parent_index == 0 &&
			recovered.item_identities[1].serialized_parent_id ==
				recovered.item_identities[0].database_id,
		"stale flat-file item placement refused the load");

	player_snapshot extra_payload = make_full(7);
	extra_payload.items.push_back(orphan);
	recovered = reload(extra_payload, 10);
	require(recovered.outcome == player_load_outcome::applied &&
			recovered.snapshot.items.size() == 2 && recovered.stale_item_rows == 1 &&
			recovered.missing_payload_rows == 0 &&
			recovered.authoritative_item_count == 3,
		"a payload item missing from the ownership catalog refused the load");

	player_snapshot dropped_payload = make_full(8);
	dropped_payload.items.pop_back();
	recovered = reload(dropped_payload, 11);
	require(recovered.outcome == player_load_outcome::applied &&
			recovered.snapshot.items.size() == 1 &&
			recovered.missing_payload_rows == 1 && recovered.stale_item_rows == 0 &&
			recovered.authoritative_item_count == 2,
		"an ownership record without its payload item refused the load");

	// The orphan is the container this time: its contents load at the top level rather
	// than disappearing with it.
	player_snapshot orphan_container = make_full(9);
	orphan_container.items.insert(orphan_container.items.begin(), orphan);
	orphan_container.items[1].parent_index = 0;
	orphan_container.items[2].parent_index = 1;
	recovered = reload(orphan_container, 12);
	require(recovered.outcome == player_load_outcome::applied &&
			recovered.snapshot.items.size() == 2 && recovered.stale_item_rows == 1 &&
			recovered.promoted_item_rows == 1 && recovered.missing_payload_rows == 0 &&
			recovered.snapshot.items[0].parent_index == PLAYER_SNAPSHOT_NO_PARENT &&
			recovered.snapshot.items[1].parent_index == 0 &&
			recovered.item_identities[0].root_item_uid == 100 &&
			!recovered.item_identities[0].parent_item_uid &&
			recovered.item_identities[1].root_item_uid == 100,
		"contents of an orphaned container did not survive the load");
	require(flatfile_player_snapshot_apply(root.string(), make_full(10), &error).outcome ==
			player_save_apply_outcome::applied,
		"could not restore the consistent item fixture: " + error);
	{
		flatfile_player_snapshot_lock snapshot_lock;
		flatfile_authority_lock authority_lock;
		flatfile_authority_operation snapshot_remove, domain_remove;
		require(snapshot_lock.acquire(root.string(), 42, &error) &&
				authority_lock.acquire(root.string(), &error),
			"could not acquire player deletion preparation locks: " + error);
		require(flatfile_player_snapshot_prepare_remove(
				root.string(), snapshot_lock, authority_lock, 42, &snapshot_remove,
				&error) == flatfile_player_load_result::ok &&
				snapshot_remove.store == flatfile_authority_store::players &&
				snapshot_remove.kind == flatfile_authority_operation_kind::remove &&
				snapshot_remove.filename == "42.snapshot",
			"player snapshot removal was not prepared: " + error);
		require(flatfile_player_domain_prepare_remove(root.string(), authority_lock, 42,
							      &domain_remove, &error) ==
					flatfile_player_domain_result::ok &&
				domain_remove.store == flatfile_authority_store::domains &&
				domain_remove.kind == flatfile_authority_operation_kind::remove &&
				domain_remove.filename == "player-42.domain",
			"player domain removal was not prepared: " + error);
	}

	const fs::path snapshot_path = players / "42.snapshot";
	{
		std::fstream file(snapshot_path, std::ios::in | std::ios::out | std::ios::binary);
		require(file.good(), "could not open player snapshot for corruption test");
		file.seekg(-1, std::ios::end);
		char value = 0;
		file.read(&value, 1);
		value ^= 0x5a;
		file.seekp(-1, std::ios::end);
		file.write(&value, 1);
	}
	require(flatfile_player_snapshot_load(root.string(), 42, &loaded, &error) ==
			flatfile_player_load_result::invalid,
		"corrupt player checksum was accepted");
	for (const fs::directory_entry &entry : fs::directory_iterator(players))
		require(entry.path().filename().string().find(".tmp.") == std::string::npos,
			"temporary player file was left behind");

	std::cout << "flat-file player repository passed\n";
	return 0;
}
