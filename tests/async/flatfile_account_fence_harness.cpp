#include "flatfile/flatfile_account_adapter.h"
#include "flatfile/flatfile_accounting_authority.h"
#include "flatfile/flatfile_accounting_store.h"
#include "flatfile/flatfile_identity_repository.h"
#include "persistence/persistence_mode.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>

namespace fs = std::filesystem;
using images = std::map<std::string, std::string>;

class flatfile_accounting_test_access
{
    public:
	static constexpr auto bootstrap = &flatfile_accounting_authority_storage::bootstrap;
	static constexpr auto append_epoch = &flatfile_accounting_authority_storage::append_epoch;
	static constexpr auto select_epoch = &flatfile_accounting_authority_storage::select_epoch;
	static auto commit(const std::string &root, const flatfile_authority_lock &lock,
			   const std::vector<flatfile_authority_operation> &operations,
			   std::string *error)
	{
		return flatfile_accounting_storage::commit(root, lock, operations, error);
	}
};

static void require(bool value, const std::string &message)
{
	if (!value)
	{
		std::cerr << message << '\n';
		std::exit(1);
	}
}

static std::string read(const fs::path &path)
{
	std::ifstream input(path, std::ios::binary);
	require(input.good(), "cannot read owned fixture file");
	return { std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>() };
}

static images snapshot(const fs::path &root)
{
	images result;
	for (const auto &entry : fs::recursive_directory_iterator(root))
		if (entry.is_regular_file() && !entry.path().filename().string().ends_with(".lock"))
			result.emplace(entry.path().lexically_relative(root).string(),
				       read(entry.path()));
	return result;
}

static critical_operation_id id(uint8_t value)
{
	critical_operation_id result = {};
	result.bytes[0] = value;
	return result;
}

// Test-only native metadata codecs; this is not a valid activation baseline.
static void select_epoch(const fs::path &root, bool initialize, bool active)
{
	for (const auto &directory : { root / "economic-evidence", root / "accounting" })
	{
		fs::create_directories(directory);
		fs::permissions(directory, fs::perms::owner_all, fs::perm_options::replace);
	}
	flatfile_authority_lock lock;
	std::string error;
	require(lock.acquire(root.string(), &error), "control lock: " + error);
	std::vector<flatfile_authority_operation> changes;
	const auto commit = [&]
	{
		require(flatfile_accounting_test_access::commit(root.string(), lock, changes,
								&error) ==
				flatfile_authority_transaction_result::ok,
			"control commit: " + error);
		changes.clear();
	};
	const auto revision = [&]
	{
		flatfile_economic_control control;
		require(flatfile_economic_control_read(root.string(), lock, &control, &error) == 0,
			"control read: " + error);
		return control.revision;
	};
	if (initialize)
	{
		require(flatfile_accounting_test_access::bootstrap(root.string(), lock, id(1),
								   id(2), &changes, &error) == 0,
			"control bootstrap: " + error);
		commit();
		flatfile_economic_epoch epoch;
		epoch.epoch = id(3);
		epoch.ordinal = 1;
		epoch.creating_operation = id(4);
		epoch.transition_kind = 1;
		epoch.transition_digest[0] = 42;
		require(flatfile_accounting_test_access::append_epoch(
				root.string(), lock, revision(), epoch, &changes, &error) == 0,
			"control epoch: " + error);
		commit();
	}
	require(flatfile_accounting_test_access::select_epoch(root.string(), lock, revision(),
							      active, id(active ? 5 : 6), &changes,
							      &error) == 0,
		"control selection: " + error);
	commit();
}

int main(int argc, char **argv)
{
	require(argc == 2, "owned state root required");
	const fs::path root = argv[1];
	fs::create_directories(root);
	fs::permissions(root, fs::perms::owner_all, fs::perm_options::replace);
	setenv("PERSISTENCE_MODE", "flatfile-primary", 1);
	setenv("FLATFILE_STATE_DIR", root.c_str(), 1);
	char configure_error[2048] = {};
	require(persistence_mode_configure(configure_error, sizeof(configure_error)),
		"native fixture preflight");
	std::string error;
	int32_t pid = 0;
	require(flatfile_identity_allocate_pid(root.string(), &pid, &error) ==
			flatfile_identity_result::ok,
		"fixture PID: " + error);
	char name[] = "FenceFixture", email[] = "fixture@example.test", password[] = "fixture-hash";
	char confirmation[] = "fixture", character_name[] = "FenceCharacter";
	acct_chars character = {};
	character.pid = pid;
	character.racewar = 2;
	character.level = 50;
	character.race = 4;
	character.m_class = 8;
	character.charname = character_name;
	acct_entry account = {};
	account.acct_name = name;
	account.acct_email = email;
	account.acct_password = password;
	account.acct_confirmation = confirmation;
	account.acct_character_list = &character;
	account.num_chars = 1;
	require(flatfile_account_state_save(&account, &error), "inactive account save: " + error);
	using owner = std::unique_ptr<acct_entry, decltype(&flatfile_account_state_release)>;
	owner loaded(flatfile_account_state_load(name, &error), flatfile_account_state_release);
	require(loaded && loaded->acct_blocked == 0 && loaded->num_chars == 1,
		"native account baseline");
	select_epoch(root, true, true);
	const auto metadata_revision = loaded->persistence_revision;
	require(flatfile_account_state_save(loaded.get(), &error) &&
			loaded->persistence_revision == metadata_revision + 1,
		"active accounting blocked ordinary account metadata publication");
	loaded->acct_blocked = ACCOUNT_BLOCK_DELETION;
	const auto revision_before = loaded->persistence_revision;
	const auto before = snapshot(root);
	require(!flatfile_account_state_save(loaded.get(), &error),
		"RED_FLAT_NATIVE_FENCE: active accounting admitted a permanent account fence");
	require(loaded->persistence_revision == revision_before && snapshot(root) == before,
		"active refusal changed native authority or acknowledged account revision");
	// Reopen the native scope and refuse again without clearing any authority.
	require(!flatfile_account_state_save(loaded.get(), &error) && snapshot(root) == before,
		"reopened admission lost its active refusal");
	const auto control = root / "economic-evidence/authority.eal";
	const auto original_control = read(control);
	{
		std::ofstream output(control, std::ios::binary | std::ios::trunc);
		output << "synthetic corrupt control";
	}
	const auto corrupt = snapshot(root);
	require(!flatfile_account_state_save(loaded.get(), &error) && snapshot(root) == corrupt &&
			loaded->persistence_revision == revision_before,
		"corrupt accounting admitted or changed a permanent account fence");
	{
		std::ofstream output(control, std::ios::binary | std::ios::trunc);
		output << original_control;
	}
	select_epoch(root, false, false);
	require(flatfile_account_state_save(loaded.get(), &error), "paused fence retry: " + error);
	require(loaded->persistence_revision == revision_before + 1,
		"paused retry did not commit exactly one account revision");
	owner restarted(flatfile_account_state_load(name, &error), flatfile_account_state_release);
	require(restarted && restarted->acct_blocked == ACCOUNT_BLOCK_DELETION &&
			restarted->persistence_revision == loaded->persistence_revision &&
			restarted->num_chars == 1 && restarted->acct_character_list->pid == pid,
		"native fence/membership did not survive fresh load");
	std::cout
		<< "PASS: native flatfile active/corrupt fence refusal, unchanged authority, paused retry and fresh load\n";
}
