#include "flatfile_boon_repository.h"
#include "flatfile_character_delete.h"
#include "flatfile_identity_repository.h"
#include "flatfile_item_repository.h"
#include "flatfile_player_domain_repository.h"
#include "flatfile_player_repository.h"
#include "flatfile_recipe_repository.h"
#include "flatfile_spellbook_repository.h"

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
	if (establish_boons)
		require(flatfile_boon_establish(root.string(), {}, &error) ==
				flatfile_boon_result::ok,
			"boon baseline failed: " + error);
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
