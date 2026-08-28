#include "flatfile_boon_repository.h"
#include "flatfile_character_delete.h"
#include "flatfile_artifact_repository.h"
#include "flatfile_association_repository.h"
#include "flatfile_frag_leaderboard_repository.h"
#include "flatfile_identity_repository.h"
#include "flatfile_item_repository.h"
#include "flatfile_locker_repository.h"
#include "flatfile_offline_message_repository.h"
#include "flatfile_player_domain_repository.h"
#include "flatfile_player_repository.h"
#include "flatfile_recipe_repository.h"
#include "flatfile_ship_repository.h"
#include "flatfile_spellbook_repository.h"
#include "flatfile_world_item_repository.h"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

namespace fs = std::filesystem;

bool player_load_request_valid(const player_load_request &request, uint64_t now)
{
	return request.schema_version == PLAYER_LOAD_SCHEMA_VERSION && request.request_id &&
	       request.pid > 0 && !request.account_name.empty() && request.deadline_usec > now;
}

static void require(bool condition, const std::string &message)
{
	if (!condition)
	{
		std::cerr << message << '\n';
		exit(1);
	}
}

static player_snapshot make_snapshot(int32_t pid)
{
	player_snapshot snapshot = {};
	snapshot.schema_version = PLAYER_SNAPSHOT_SCHEMA_VERSION;
	snapshot.pid = pid;
	snapshot.revision = 1;
	snapshot.components = PLAYER_CHECKPOINT_COMPONENT_ALL;
	snapshot.encoded_size_bound = 4096;
	snapshot.status_strings.push_back({ player_status_string_field::name, "Player" });
	snapshot.status_integers.push_back({ player_status_field::racewar, 0, 0, false });
	snapshot.status_integers.push_back({ player_status_field::copper, 1, 0, true });
	snapshot.status_integers.push_back({ player_status_field::silver, 2, 0, true });
	snapshot.status_integers.push_back({ player_status_field::gold, 3, 0, true });
	snapshot.status_integers.push_back({ player_status_field::platinum, 4, 0, true });
	snapshot.status_integers.push_back({ player_status_field::epics, 5, 0, false });
	snapshot.status_integers.push_back({ player_status_field::frags, 6, 0, false });
	snapshot.status_integers.push_back({ player_status_field::old_frags, 7, 0, false });
	for (int index = 0; index < 10; ++index)
		snapshot.status_integers.push_back(
			{ static_cast<player_status_field>(
				  static_cast<unsigned int>(player_status_field::base_strength) +
				  index),
			  40 + index, 0, false });
	snapshot.recipes_are_external = true;
	return snapshot;
}

static void establish(const fs::path &root, bool establish_boons)
{
	fs::create_directories(root / "players");
	fs::create_directories(root / "identities/names");
	fs::create_directories(root / "domains");
	fs::permissions(root, fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(root / "players", fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(root / "identities", fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(root / "identities/names", fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(root / "domains", fs::perms::owner_all, fs::perm_options::replace);
	std::string error;
	int32_t pid = 0;
	require(flatfile_identity_allocate_pid(root.string(), &pid, &error) ==
				flatfile_identity_result::ok &&
			pid == 1,
		"identity allocation failed: " + error);
	require(flatfile_identity_claim(root.string(), pid, "Player", "Account", &error) ==
			flatfile_identity_result::ok,
		"identity claim failed: " + error);
	const auto applied =
		flatfile_player_snapshot_apply(root.string(), make_snapshot(pid), &error);
	require(applied.outcome == player_save_apply_outcome::applied,
		"player baseline failed: " + error);
	require(flatfile_recipe_establish(root.string(), { { 1, { 1001, 1002 } } }, &error) ==
			flatfile_recipe_result::ok,
		"recipe baseline failed: " + error);
	require(flatfile_spellbook_establish(root.string(), { { 1, { 2001 } } }, &error) ==
			flatfile_spellbook_result::ok,
		"spellbook baseline failed: " + error);
	require(flatfile_artifact_establish(
			root.string(),
			{ { 3001, true, FLATFILE_ARTIFACT_ON_PLAYER, 1, 9999, 1, 100, 1, 8888, 1 },
			  { 3002, true, FLATFILE_ARTIFACT_ON_CORPSE, 1, 7777, 1, 101, 1, 6666, 1 } },
			&error) == flatfile_artifact_result::ok,
		"artifact baseline failed: " + error);
	require(flatfile_frag_leaderboard_establish(
			root.string(),
			{ { 1, "Account", "Player", 1234, 1, "Human", "Warrior", 50, 0, 100, 1 } },
			&error) == flatfile_frag_leaderboard_result::ok,
		"frag leaderboard baseline failed: " + error);
	player_item_snapshot locker_item = {};
	locker_item.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	locker_item.equipment_slot = -1;
	locker_item.object_uid = 900;
	locker_item.vnum = 3900;
	locker_item.name = "stored item";
	flatfile_locker_chest_record player_chest = {};
	player_chest.chest_id = 11;
	player_chest.chest_name = "public";
	player_chest.is_public = true;
	player_chest.revision = 1;
	player_chest.items = { locker_item };
	flatfile_locker_record player_locker = {};
	player_locker.locker_id = 10;
	player_locker.locker_name = "player.locker";
	player_locker.owner_pid = 1;
	player_locker.revision = 1;
	player_locker.chests = { player_chest };
	flatfile_locker_chest_record guild_chest = {};
	guild_chest.chest_id = 21;
	guild_chest.chest_name = "public";
	guild_chest.is_public = true;
	guild_chest.revision = 1;
	flatfile_locker_record guild_locker = {};
	guild_locker.locker_id = 20;
	guild_locker.locker_name = "guild.7.locker";
	guild_locker.owner_assoc_id = 7;
	guild_locker.revision = 1;
	guild_locker.chests = { guild_chest };
	require(flatfile_locker_establish(root.string(), { guild_locker, player_locker },
					  { { "player.locker", "guest", 1 },
					    { "guild.7.locker", "player", 1 } },
					  &error) == flatfile_locker_result::ok,
		"locker baseline failed: " + error);
	flatfile_association_record association = {};
	association.association_id = 7;
	association.name = "Test Guild";
	association.frags = 10;
	association.top_frags = 6;
	association.top_fragger = "Player";
	association.revision = 1;
	association.members = { { 1, "Player", 1, 0, 2, 6, 1 }, { 2, "Other", 1, 0, 0, 4, 1 } };
	require(flatfile_association_establish(root.string(), { association }, &error) ==
			flatfile_association_result::ok,
		"association baseline failed: " + error);
	flatfile_ship_record ship = {};
	ship.ship_id = 30;
	ship.owner_pid = 1;
	ship.owner_name = "Player";
	ship.ship_name = "Player Ship";
	ship.revision = 1;
	ship.slots = { { 0, 4, 5, 6, 7, { 8, 9, 10, 11, 12 } } };
	require(flatfile_ship_establish(root.string(), { ship }, &error) ==
			flatfile_ship_result::ok,
		"ship baseline failed: " + error);
	player_item_snapshot corpse_item = locker_item;
	corpse_item.object_uid = 901;
	corpse_item.vnum = 3002;
	corpse_item.name = "corpse artifact";
	flatfile_corpse_record corpse = {};
	corpse.owner_pid = 1;
	corpse.owner_name = "Player";
	corpse.save_id = 40;
	corpse.room_vnum = 500;
	corpse.short_description = "the corpse of Player";
	corpse.revision = 1;
	corpse.items = { corpse_item };
	flatfile_saved_world_item_record saved = {};
	saved.item_key = "item.statue.1";
	saved.room_vnum = 700;
	saved.revision = 1;
	player_item_snapshot saved_item = locker_item;
	saved_item.object_uid = 902;
	saved_item.vnum = 3902;
	saved.items = { saved_item };
	require(flatfile_world_item_establish(root.string(), { corpse }, { saved }, &error) ==
			flatfile_world_item_result::ok,
		"world item baseline failed: " + error);
	const item_owner_identity locker_owner = { item_owner_type::locker, 10, 11 };
	require(flatfile_item_repository_establish_owner(
			root.string(), locker_owner,
			{ { 900, 900, 0, locker_owner, 1, 3900, item_custody_state::active } },
			&error) == flatfile_item_baseline_result::applied,
		"locker custody baseline failed: " + error);
	const item_owner_identity corpse_owner = { item_owner_type::corpse,
						   item_corpse_owner_id(1, 40), 0 };
	require(flatfile_item_repository_establish_owner(
			root.string(), corpse_owner,
			{ { 901, 901, 0, corpse_owner, 1, 3002, item_custody_state::active } },
			&error) == flatfile_item_baseline_result::applied,
		"corpse custody baseline failed: " + error);
	if (establish_boons)
		require(flatfile_boon_establish(root.string(), {}, &error) ==
				flatfile_boon_result::ok,
			"boon baseline failed: " + error);
	flatfile_offline_message_id message_id = {};
	message_id[0] = 1;
	require(flatfile_offline_message_enqueue(root.string(), 1, message_id, "pending", &error) ==
			flatfile_offline_message_result::ok,
		"offline message baseline failed: " + error);
}

int main(int argc, char **argv)
{
	require(argc == 2, "state root argument required");
	const fs::path root = fs::path(argv[1]) / "recover";
	establish(root, true);
	std::string error;
	setenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_IMAGE", "1", 1);
	require(flatfile_character_delete(root.string(), 1, "pLaYeR", &error) ==
			flatfile_character_delete_result::io_error,
		"fault injection did not interrupt character deletion");
	unsetenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_IMAGE");
	require(fs::exists(root / "domains/.critical-authority-transaction"),
		"interrupted deletion did not leave a recovery journal");
	error.clear();
	require(flatfile_character_delete(root.string(), 1, "Player", &error) ==
			flatfile_character_delete_result::already_deleted,
		"deletion recovery was not idempotent: " + error);
	require(!fs::exists(root / "domains/.critical-authority-transaction"),
		"recovered deletion left its journal behind");
	flatfile_identity_record identity;
	require(flatfile_identity_lookup_pid(root.string(), 1, &identity, &error) ==
				flatfile_identity_result::ok &&
			!identity.active && identity.blocked,
		"recovered deletion did not tombstone identity");
	player_snapshot snapshot;
	require(flatfile_player_snapshot_load(root.string(), 1, &snapshot, &error) ==
			flatfile_player_load_result::not_found,
		"recovered deletion retained player snapshot");
	flatfile_player_domain_record domain;
	require(flatfile_player_domain_load(root.string(), 1, "Account", 0, &domain, &error) ==
			flatfile_player_domain_result::not_found,
		"recovered deletion retained player domain");
	std::vector<int32_t> values;
	require(flatfile_recipe_list(root.string(), 1, &values, &error) ==
				flatfile_recipe_result::ok &&
			values.empty(),
		"recovered deletion retained recipes");
	require(flatfile_spellbook_list(root.string(), 1, &values, &error) ==
				flatfile_spellbook_result::ok &&
			values.empty(),
		"recovered deletion retained spellbook entries");
	uint64_t owner_revision = 0;
	std::vector<flatfile_item_ownership_record> items;
	require(flatfile_item_repository_load_owner(
			root.string(), { item_owner_type::player, 1, 0 }, &owner_revision, &items,
			&error) == flatfile_item_repository_result::not_found,
		"recovered deletion retained the player item owner");
	require(flatfile_item_repository_load_owner(
			root.string(), { item_owner_type::locker, 10, 11 }, &owner_revision, &items,
			&error) == flatfile_item_repository_result::not_found,
		"recovered deletion retained locker item custody");
	const item_owner_identity corpse_owner = { item_owner_type::corpse,
						   item_corpse_owner_id(1, 40), 0 };
	require(flatfile_item_repository_load_owner(root.string(), corpse_owner, &owner_revision,
						    &items, &error) ==
			flatfile_item_repository_result::not_found,
		"recovered deletion retained corpse item custody");
	std::vector<flatfile_locker_record> lockers;
	std::vector<flatfile_locker_access_record> locker_access;
	require(flatfile_locker_list(root.string(), &lockers, &locker_access, &error) ==
				flatfile_locker_result::ok &&
			lockers.size() == 1 && lockers[0].owner_assoc_id == 7 &&
			locker_access.empty(),
		"recovered deletion retained player locker or access state");
	std::vector<flatfile_association_record> associations;
	require(flatfile_association_list(root.string(), &associations, &error) ==
				flatfile_association_result::ok &&
			associations.size() == 1 && associations[0].members.size() == 1 &&
			associations[0].members[0].pid == 2 && associations[0].frags == 4 &&
			associations[0].top_frags == 0 && associations[0].top_fragger.empty(),
		"recovered deletion retained association membership or frag contribution");
	std::vector<flatfile_ship_record> ships;
	require(flatfile_ship_list(root.string(), &ships, &error) == flatfile_ship_result::ok &&
			ships.empty(),
		"recovered deletion retained player ship or cargo slots");
	std::vector<flatfile_corpse_record> corpses;
	std::vector<flatfile_saved_world_item_record> saved_items;
	require(flatfile_world_item_list(root.string(), &corpses, &saved_items, &error) ==
				flatfile_world_item_result::ok &&
			corpses.empty() && saved_items.size() == 1 &&
			saved_items[0].items[0].object_uid == 902,
		"recovered deletion retained corpse or removed saved room state");
	std::vector<flatfile_artifact_record> artifacts;
	require(flatfile_artifact_list(root.string(), &artifacts, &error) ==
				flatfile_artifact_result::ok &&
			artifacts.size() == 2 && !artifacts[0].owned &&
			artifacts[0].location_type == FLATFILE_ARTIFACT_NOT_IN_GAME &&
			artifacts[0].location == 0 && artifacts[0].timer == 0 &&
			artifacts[0].bind_owner_pid == -1 && artifacts[0].bind_timer == 0 &&
			artifacts[0].revision == 2 && !artifacts[1].owned &&
			artifacts[1].location_type == FLATFILE_ARTIFACT_NOT_IN_GAME &&
			artifacts[1].location == 0 && artifacts[1].timer == 0 &&
			artifacts[1].bind_owner_pid == -1 && artifacts[1].bind_timer == 0 &&
			artifacts[1].revision == 2,
		"recovered deletion did not release player and corpse artifacts");
	std::vector<flatfile_frag_leaderboard_record> leaderboard;
	require(flatfile_frag_leaderboard_list(root.string(), &leaderboard, &error) ==
				flatfile_frag_leaderboard_result::ok &&
			leaderboard.size() == 1 && leaderboard[0].pid == 1 &&
			leaderboard[0].deleted_at > 0 &&
			leaderboard[0].last_updated == leaderboard[0].deleted_at &&
			leaderboard[0].revision == 2,
		"recovered deletion did not tombstone frag leaderboard state");
	std::vector<flatfile_offline_message_record> messages;
	require(flatfile_offline_message_list(root.string(), 1, &messages, &error) ==
				flatfile_offline_message_result::ok &&
			messages.empty(),
		"recovered deletion retained offline messages");

	const fs::path missing = fs::path(argv[1]) / "missing";
	establish(missing, false);
	error.clear();
	require(flatfile_character_delete(missing.string(), 1, "Player", &error) ==
			flatfile_character_delete_result::not_found,
		"missing boon authority did not fail closed");
	require(flatfile_identity_lookup_pid(missing.string(), 1, &identity, &error) ==
				flatfile_identity_result::ok &&
			identity.active,
		"failed deletion changed identity authority");
	require(flatfile_player_snapshot_load(missing.string(), 1, &snapshot, &error) ==
			flatfile_player_load_result::ok,
		"failed deletion changed player snapshot authority");
	require(!fs::exists(missing / "domains/.critical-authority-transaction"),
		"failed deletion published a transaction journal");

	const fs::path direct = fs::path(argv[1]) / "direct";
	establish(direct, true);
	error.clear();
	require(flatfile_character_delete(direct.string(), 1, "Player", &error) ==
			flatfile_character_delete_result::ok,
		"direct deletion failed: " + error);
	require(flatfile_character_delete(direct.string(), 1, "Player", &error) ==
			flatfile_character_delete_result::already_deleted,
		"direct deletion retry was not idempotent: " + error);

	std::cout << "flat-file character deletion passed\n";
	return 0;
}
