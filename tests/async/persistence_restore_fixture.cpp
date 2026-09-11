// Synthetic account/player/domain fixture and deliberately interrupted durable
// transaction. Never use this executable with an existing authority root.
#define main player_repository_regression_main
#include "flatfile_player_repository_harness.cpp"
#undef main
#include "flatfile/flatfile_account_repository.h"
#include "flatfile/flatfile_authority_transaction.h"
#include "flatfile/flatfile_store.h"
#include "player/player_save_journal.h"
#include "persistence/critical_command_journal.h"

// Native fixture output must not expose repository log details.
void logit(const char *, const char *, ...) {}

static std::vector<uint8_t> fixture_bytes(const char *text)
{
	return { text, text + std::strlen(text) };
}

static critical_command fixture_currency_command()
{
	currency_command_payload payload = {};
	payload.pid = 42;
	payload.racewar = 0;
	payload.reason = currency_reason_type::wallet_reward;
	std::strcpy(payload.account_name.data(), "Account-One");
	payload.wallet_delta.amount[0] = 5;
	critical_operation_id operation = {};
	operation.bytes[0] = 0xa3;
	critical_command command;
	// The baseline seeded below has wallet revision 0 and bank revision 1.
	// Do not load it here: that would replay the pending authority transaction.
	require(currency_command_build(&command, operation, payload, 0, 1,
				       critical_source_site::command,
				       critical_deadline_class::interactive),
		"synthetic WAL currency encoding failed");
	command.accepted_at_usec = 1;
	require(critical_command_normalize(&command), "synthetic WAL normalization failed");
	return command;
}
static void seed_fixture_journals(const fs::path &root, const player_snapshot &pending,
				  bool critical)
{
	const auto journals = root.parent_path() / "journals";
	require(!fs::exists(journals), "WAL seed requires absent synthetic journals");
	fs::create_directories(journals / "players");
	fs::create_directories(journals / "critical");
	for (const auto &directory : { journals, journals / "players", journals / "critical" })
		fs::permissions(directory, fs::perms::owner_all, fs::perm_options::replace);
	require(player_save_journal_init((journals / "players").c_str()), "player WAL init failed");
	require(player_save_journal_append(pending) == player_save_journal_result::ok,
		"native player WAL append failed");
	require(player_save_journal_health_copy().records == 1, "player WAL is empty");
	player_save_journal_shutdown();
	require(critical_command_journal_init((journals / "critical").c_str()),
		"critical WAL init failed");
	if (critical)
		require(critical_command_journal_append(fixture_currency_command()) ==
				critical_command_journal_result::ok,
			"native critical WAL append failed");
	require(critical_command_journal_health_copy().records == (critical ? 1u : 0u),
		"critical WAL record count mismatch");
	critical_command_journal_shutdown();
}
int main(int argc, char **argv)
{
	require(argc == 3, "fixture mode and root required");
	const std::string mode = argv[1];
	const fs::path root = argv[2];
	std::string error;
	if (mode == "seed-wal" || mode == "seed-wal-blocked")
	{
		require(fs::is_directory(root), "WAL seed requires synthetic authority");
		player_snapshot pending = make_status(2, 51, 1202);
		// STATUS is a complete component replacement, not a sparse field patch.
		// Match the native capturer, including racewar required by the loader.
		const player_snapshot complete = make_full(2);
		pending.status_integers = complete.status_integers;
		pending.status_strings = complete.status_strings;
		pending.conditions = complete.conditions;
		pending.quest_values = complete.quest_values;
		pending.room_vnum = 1202;
		for (auto &row : pending.status_integers)
			if (row.field == player_status_field::level)
				row.signed_value = 51;
		if (mode == "seed-wal-blocked")
			pending.pid = 999;
		seed_fixture_journals(root, pending, true);
		return 0;
	}
	if (mode == "verify" || mode == "verify-wal")
	{
		flatfile_account_record account;
		require(flatfile_account_load(root.string(), "Account-One", &account, &error) ==
					flatfile_account_result::ok &&
				account.characters.size() == 1 && account.characters[0].pid == 42,
			"restored synthetic account mismatch");
		player_load_request request = {};
		request.request_id = 1;
		request.pid = 42;
		request.account_name = "Account-One";
		request.deadline_usec =
			persistence_observability_now_usec() + PLAYER_LOAD_TIMEOUT_USEC;
		const auto loaded = flatfile_player_load_repository_execute(root.string(), request);
		require(loaded.outcome == player_load_outcome::applied &&
				loaded.domains.wallet ==
					std::array<uint64_t, 4>{ mode == "verify-wal" ? 16u : 11u,
								 12, 13, 14 } &&
				loaded.domains.epics == 15 &&
				loaded.snapshot.revision == (mode == "verify-wal" ? 2u : 1u) &&
				loaded.item_identities.size() == 2,
			"restored synthetic player/domain mismatch");
		if (mode == "verify-wal")
		{
			require(loaded.snapshot.room_vnum == 1202 &&
					loaded.domains.wallet_revision == 1,
				"player or critical WAL revision was not applied");
			require(std::any_of(loaded.snapshot.status_integers.begin(),
					    loaded.snapshot.status_integers.end(),
					    [](const auto &row) {
						    return row.field ==
								   player_status_field::level &&
							   row.signed_value == 51;
					    }),
				"pending player status was not replayed");
			require(flatfile_player_domain_apply(root.string(),
							     fixture_currency_command())
						.outcome == critical_apply_outcome::already_applied,
				"critical WAL operation was not durably deduplicated");
		}
		for (const char *name : { "restore-probe-one", "restore-probe-two" })
		{
			std::ifstream stream(root / "domains" / name);
			std::string value;
			stream >> value;
			require(value == "after",
				"pending transaction after-image was not replayed");
		}
		require(!fs::exists(root / "domains/.critical-authority-transaction"),
			"pending transaction was not retired");
		return 0;
	}
	require((mode == "seed" || mode == "seed-first-wal") && !fs::exists(root),
		"seed requires an absent disposable root");
	for (const char *directory :
	     { "identities/names", "identities/accounts", "players", "domains" })
		fs::create_directories(root / directory);
	for (const auto &entry : fs::recursive_directory_iterator(root))
		fs::permissions(entry.path(), fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(root, fs::perms::owner_all, fs::perm_options::replace);
	flatfile_account_record account;
	account.name = "Account-One";
	account.email = "fixture@example.test";
	account.confirmed = 1;
	flatfile_account_character character;
	character.pid = 42;
	character.name = "Player";
	character.level = 50;
	account.characters.push_back(character);
	uint64_t revision = 0;
	require(flatfile_account_save(root.string(), account, 0, &revision, &error) ==
			flatfile_account_result::ok,
		"synthetic account seed failed");
	int32_t pid = 0;
	for (int expected = 1; expected <= 42; ++expected)
		require(flatfile_identity_allocate_pid(root.string(), &pid, &error) ==
					flatfile_identity_result::ok &&
				pid == expected,
			"synthetic identity allocation failed");
	require(flatfile_identity_claim(root.string(), 42, "Player", "Account-One", &error) ==
			flatfile_identity_result::ok,
		"synthetic identity claim failed");
	if (mode != "seed-first-wal")
		require(flatfile_player_snapshot_apply(root.string(), make_full(1), &error)
					.outcome == player_save_apply_outcome::applied,
			"synthetic snapshot seed failed");
	require(flatfile_boon_establish(root.string(), {}, &error) == flatfile_boon_result::ok,
		"synthetic boon seed failed");
	for (const char *name : { "restore-probe-one", "restore-probe-two" })
		require(flatfile_atomic_write((root / "domains").string(), name,
					      fixture_bytes("before"), &error),
			"synthetic transaction seed failed");
	{
		flatfile_authority_lock lock;
		require(lock.acquire(root.string(), &error), "synthetic authority lock failed");
		setenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_IMAGE", "1", 1);
		const auto result = flatfile_authority_transaction_commit(
			root.string(), lock,
			{ { "restore-probe-one", fixture_bytes("after") },
			  { "restore-probe-two", fixture_bytes("after") } },
			&error);
		unsetenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_IMAGE");
		require(result == flatfile_authority_transaction_result::io_error,
			"synthetic transaction was not interrupted");
	}
	require(fs::exists(root / "domains/.critical-authority-transaction"),
		"synthetic pending transaction missing");
	for (const char *name : { "restore-probe-one", "restore-probe-two" })
	{
		std::ifstream stream(root / "domains" / name);
		std::string value;
		stream >> value;
		require(value == (std::string(name) == "restore-probe-one" ? "after" : "before"),
			"synthetic interruption did not leave a split durable transaction");
	}
	if (mode == "seed-first-wal")
		seed_fixture_journals(root, make_full(1), false);
	return 0;
}
