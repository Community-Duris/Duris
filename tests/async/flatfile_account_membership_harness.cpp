#include "flatfile_account_adapter.h"
#include "flatfile_account_repository.h"
#include "flatfile_identity_repository.h"
#include "persistence_mode.h"

#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
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

int main(int argc, char **argv)
{
	require(argc == 2, "state root argument required");
	const fs::path root = argv[1];
	fs::create_directories(root);
	fs::permissions(root, fs::perms::owner_all, fs::perm_options::replace);
	setenv("PERSISTENCE_MODE", "flatfile-primary", 1);
	setenv("FLATFILE_STATE_DIR", root.c_str(), 1);
	char configure_error[2048] = {};
	require(persistence_mode_configure(configure_error, sizeof(configure_error)),
		"flat backend failed boot preflight: " + std::string(configure_error));
	require(persistence_mode_flatfile_root() != nullptr,
		"flat state root was not retained after preflight");

	std::string error;
	int32_t pid = 0;
	require(flatfile_identity_allocate_pid(root.string(), &pid, &error) ==
				flatfile_identity_result::ok &&
			pid == 1,
		"membership PID allocation failed: " + error);
	struct acct_entry account = {};
	account.acct_name = strdup("Account-One");
	account.acct_email = strdup("one@example.test");
	account.acct_password = strdup("hash");
	account.acct_confirmation = strdup("confirmed");
	struct acct_chars character = {};
	character.pid = pid;
	character.charname = strdup("Hero");
	character.count = 7;
	character.last = 100;
	character.blocked = 0;
	character.racewar = 2;
	character.level = 50;
	character.race = 4;
	character.m_class = 8;
	character.secondary_class = 9;
	character.last_room = 600;
	character.last_save = 700;
	account.acct_character_list = &character;
	account.num_chars = 1;
	require(flatfile_account_state_save(&account, &error),
		"account membership save failed: " + error);
	require(account.persistence_revision == 1, "account scalar revision did not advance");

	flatfile_account_record scalar_record;
	require(flatfile_account_load(root.string(), "account-one", &scalar_record, &error) ==
				flatfile_account_result::ok &&
			scalar_record.characters.empty(),
		"account record retained a second membership authority");
	P_acct loaded = flatfile_account_state_load("ACCOUNT-ONE", &error);
	require(loaded && loaded->num_chars == 1 && loaded->acct_character_list &&
			loaded->acct_character_list->pid == pid &&
			!strcmp(loaded->acct_character_list->charname, "Hero") &&
			loaded->acct_character_list->count == 7 &&
			loaded->acct_character_list->last == 100 &&
			loaded->acct_character_list->racewar == 2 &&
			loaded->acct_character_list->level == 50 &&
			loaded->acct_character_list->race == 4 &&
			loaded->acct_character_list->m_class == 8 &&
			loaded->acct_character_list->secondary_class == 9 &&
			loaded->acct_character_list->last_room == 600 &&
			loaded->acct_character_list->last_save == 700,
		"catalog membership did not materialize into the account DTO: " + error);

	free(loaded->acct_character_list->charname);
	loaded->acct_character_list->charname = strdup("HeroRenamed");
	loaded->acct_character_list->blocked = 1;
	require(flatfile_account_state_save(loaded, &error),
		"renamed membership save failed: " + error);
	flatfile_identity_record identity;
	require(flatfile_identity_lookup_name(root.string(), "Hero", &identity, &error) ==
				flatfile_identity_result::not_found &&
			flatfile_identity_lookup_name(root.string(), "herorenamed", &identity,
						      &error) == flatfile_identity_result::ok &&
			identity.pid == pid && identity.blocked,
		"membership rename/block did not publish atomically");

	free(loaded->acct_character_list->charname);
	free(loaded->acct_character_list);
	loaded->acct_character_list = nullptr;
	loaded->num_chars = 0;
	require(flatfile_account_state_save(loaded, &error),
		"membership removal save failed: " + error);
	require(flatfile_identity_lookup_name(root.string(), "HeroRenamed", &identity, &error) ==
				flatfile_identity_result::not_found &&
			flatfile_identity_lookup_pid(root.string(), pid, &identity, &error) ==
				flatfile_identity_result::ok &&
			!identity.active,
		"membership removal did not retain the PID tombstone");

	std::cout << "flat-file account membership authority passed\n";
	return 0;
}
