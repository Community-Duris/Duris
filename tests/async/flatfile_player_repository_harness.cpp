#include "flatfile_player_repository.h"
#include "flatfile_identity_repository.h"
#include "flatfile_item_repository.h"
#include "persistence_observability.h"

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

int main(int argc, char **argv)
{
	require(argc == 2, "state root argument required");
	const fs::path root = argv[1];
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
			load_result.outcome == player_load_outcome::component_failure &&
			load_result.error_code == ENOTSUP &&
			std::string(load_result.failed_component) == "external_domains",
		"verified snapshot did not fail closed at missing external domains");
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
			load_result.error_code == ENOTSUP,
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
