#include "flatfile/flatfile_player_domain_repository.h"
#include "player/player_save_journal.h"
#include "item/locker_receipt.h"
#include "persistence/critical_command_journal.h"
#include "flatfile/flatfile_account_repository.h"
#include "flatfile/flatfile_identity_repository.h"
#include "flatfile/flatfile_player_repository.h"
#include "persistence/persistence_mode.h"
#include "flatfile/flatfile_shopkeeper_repository.h"
#include "flatfile/flatfile_world_item_repository.h"
#include "flatfile/flatfile_locker_repository.h"
#include "flatfile/flatfile_auction_repository.h"
#include "flatfile/flatfile_artifact_repository.h"
#include "flatfile/flatfile_ship_repository.h"
#include "flatfile/flatfile_association_repository.h"
#include "flatfile/flatfile_nexus_repository.h"
#include "flatfile/flatfile_corpse_repository.h"
#include "flatfile/flatfile_shop_trade_repository.h"
#include "flatfile/flatfile_boon_repository.h"
#include "flatfile/flatfile_item_repository.h"
#include "kingdom/kingdom_restore.h"

// Native parsers may log diagnostics containing identities; this process reports
// only aggregate success or a fixed failure code.
void logit(const char *, const char *, ...) {}

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <set>
#include <stdexcept>
#include <sys/stat.h>
#include <unistd.h>

// Same admission contract as the asynchronous player loader. This standalone
// process links the native repositories without game sockets or SQL clients.
bool player_load_request_valid(const player_load_request &request, uint64_t now)
{
	return request.schema_version == PLAYER_LOAD_SCHEMA_VERSION && request.request_id > 0 &&
	       request.pid > 0 && !request.account_name.empty() &&
	       request.account_name.size() <= PLAYER_LOAD_ACCOUNT_MAX &&
	       request.deadline_usec > now &&
	       request.deadline_usec - now <= PLAYER_LOAD_TIMEOUT_USEC &&
	       (!request.include_pets || request.include_items);
}

static void require(bool valid)
{
	if (!valid)
		throw std::runtime_error("native_restore_qualification_failed");
}

template <typename Result> static bool readable(Result result)
{
	return result == Result::ok || result == Result::not_found;
}

static std::string account_name(const std::string &stem)
{
	require(!stem.empty() && stem.size() % 2 == 0 &&
		stem.size() <= PLAYER_LOAD_ACCOUNT_MAX * 2);
	std::string name;
	const std::string digits = "0123456789abcdef";
	for (size_t i = 0; i < stem.size(); i += 2)
	{
		const auto first = digits.find(stem[i]);
		const auto second = digits.find(stem[i + 1]);
		require(first != std::string::npos && second != std::string::npos);
		const char character = static_cast<char>(first * 16 + second);
		require(character != '\0');
		name += character;
	}
	return name;
}

// Locker identification stores durable payment receipts alongside the critical
// WAL. The service lock is empty runtime metadata; receipts must survive restore
// and pass the same bounded decoder used when a player claims their result.
static void qualify_locker_receipts(const std::filesystem::path &directory)
{
	struct stat status = {};
	require(lstat(directory.c_str(), &status) == 0 && S_ISDIR(status.st_mode) &&
		status.st_uid == geteuid() && !(status.st_mode & 0077));
	for (const auto &entry : std::filesystem::directory_iterator(directory))
	{
		require(lstat(entry.path().c_str(), &status) == 0 && S_ISREG(status.st_mode) &&
			status.st_uid == geteuid() && status.st_nlink == 1 &&
			!(status.st_mode & 0077));
		const auto name = entry.path().filename().string();
		if (name == ".service-lock")
		{
			require(status.st_size == 0);
			continue;
		}
		require(entry.path().extension() == ".receipt");
		const auto stem = entry.path().stem().string();
		require(!stem.empty() && stem.size() <= 10 && stem[0] != '0' &&
			stem.find_first_not_of("0123456789") == std::string::npos);
		const auto pid = std::stoull(stem);
		require(pid > 0 && pid <= INT32_MAX);
		locker_receipt receipt;
		require(locker_receipt_read(directory.string(), static_cast<uint32_t>(pid),
					    &receipt) == flatfile_read_result::ok);
	}
}

// Init uses the production codecs and may quarantine a corrupt player frame.
// This runs only on copied candidate journals; any quarantine is a hard failure.
static void qualify_journals(const std::filesystem::path &candidate, bool drained)
{
	require(candidate.is_absolute() &&
		std::filesystem::is_regular_file(candidate / "ISOLATED_RESTORE"));
	for (const char *name : { "players", "critical" })
	{
		const auto directory = candidate / "journals" / name;
		std::filesystem::create_directories(directory);
		const std::string filename = std::string(name) == "players" ?
						     "player-save.journal" :
						     "critical-command.journal";
		for (const auto &entry : std::filesystem::directory_iterator(directory))
		{
			if (std::string(name) == "critical" &&
			    entry.path().filename() == "locker-identification")
			{
				qualify_locker_receipts(entry.path());
				continue;
			}
			require(entry.is_regular_file() && !entry.is_symlink());
			require(entry.path().filename() == filename ||
				(entry.path().filename() == "player-save.journal.quarantine" &&
				 entry.file_size() == 0));
		}
	}
	require(player_save_journal_init((candidate / "journals/players").c_str()));
	const auto player = player_save_journal_health_copy();
	player_save_journal_shutdown();
	require(player.initialized && player.corrupt_records == 0 &&
		player.unsupported_records == 0 && player.quarantined_bytes == 0 &&
		(!drained || player.records == 0));
	require(critical_command_journal_init((candidate / "journals/critical").c_str()));
	const auto critical = critical_command_journal_health_copy();
	critical_command_journal_shutdown();
	require(critical.initialized && critical.corrupt_records == 0 &&
		critical.last_result == critical_command_journal_result::ok &&
		(!drained || critical.records == 0));
	std::cout << "{\"player_records\":" << player.records
		  << ",\"critical_records\":" << critical.records << "}\n";
}

int main(int argc, char **argv)
{
	try
	{
		if (argc == 3 && (std::string(argv[1]) == "--journals-preflight" ||
				  std::string(argv[1]) == "--journals-drained"))
		{
			qualify_journals(argv[2], std::string(argv[1]) == "--journals-drained");
			return 0;
		}
		const bool preflight = argc == 3 && std::string(argv[1]) == "--state-preflight";
		require(argc == 2 || preflight);
		const std::string root = argv[argc - 1];
		require(std::filesystem::path(root).is_absolute());
		// The manager creates this marker only in its fresh candidate. Direct use
		// against a configured live root must not initiate recovery.
		require(std::filesystem::is_regular_file(std::filesystem::path(root).parent_path() /
							 "ISOLATED_RESTORE"));
		setenv("PERSISTENCE_MODE", "flatfile-primary", 1);
		setenv("FLATFILE_STATE_DIR", root.c_str(), 1);
		char mode_error[256] = {};
		require(persistence_mode_configure(mode_error, sizeof(mode_error)));
		std::string error;
		require(flatfile_player_domain_restore_recover(root, &error) ==
			flatfile_player_domain_result::ok);
		// Mini-world boot does not materialize every persistent world domain.
		// Exercise their native decoders before any qualification receipt.
		require(flatfile_corpse_repository_validate(root, &error));
		require(flatfile_shop_trade_repository_validate(root, &error));
		require(kingdom_flatfile_restore_validate(root, &error));
		std::vector<flatfile_boon_definition> boons;
		std::vector<flatfile_item_ownership_record> ownership;
		require(readable(flatfile_boon_load_definitions(root, &boons, &error)));
		require(readable(flatfile_item_repository_list_active_player_items(root, &ownership,
										   &error)));
		std::vector<flatfile_shopkeeper_record> shops;
		std::vector<flatfile_corpse_record> corpses;
		std::vector<flatfile_saved_world_item_record> saved;
		std::vector<flatfile_room_item_record> rooms;
		std::vector<flatfile_locker_record> lockers;
		std::vector<flatfile_locker_access_record> access;
		std::vector<flatfile_auction_listing_projection> auctions;
		std::vector<flatfile_artifact_record> artifacts;
		std::vector<flatfile_ship_record> ships;
		std::vector<flatfile_association_record> guilds;
		std::vector<flatfile_alliance_record> alliances;
		std::vector<flatfile_guildhall_record> halls;
		std::vector<flatfile_outpost_record> outposts;
		std::vector<flatfile_nexus_record> nexus;
		require(readable(flatfile_shopkeeper_list(root, &shops, &error)));
		require(readable(flatfile_world_item_list(root, &corpses, &saved, &error)));
		require(readable(flatfile_world_item_list_rooms(root, &rooms, &error)));
		require(readable(flatfile_locker_list(root, &lockers, &access, &error)));
		require(readable(flatfile_auction_list_open(root, &auctions, &error)));
		require(readable(flatfile_artifact_list(root, &artifacts, &error)));
		require(readable(flatfile_ship_list(root, &ships, &error)));
		require(readable(flatfile_association_list(root, &guilds, &error)));
		require(readable(flatfile_alliance_list(root, &alliances, &error)));
		require(readable(flatfile_guildhall_list(root, &halls, &error)));
		require(readable(flatfile_outpost_list(root, &outposts, &error)));
		require(readable(flatfile_nexus_list(root, &nexus, &error)));
		size_t accounts = 0, identities = 0, loaded = 0, snapshots = 0;
		std::set<int32_t> known;
		for (const auto &entry :
		     std::filesystem::directory_iterator(root + "/identities/accounts"))
		{
			if (entry.path().extension() != ".acct")
				continue;
			const auto name = account_name(entry.path().stem().string());
			flatfile_account_record account;
			require(flatfile_account_load(root, name, &account, &error) ==
				flatfile_account_result::ok);
			++accounts;
			std::vector<flatfile_identity_record> records;
			require(flatfile_identity_list_account(root, name, &records, &error) ==
				flatfile_identity_result::ok);
			for (const auto &record : records)
			{
				++identities;
				require(known.insert(record.pid).second);
				if (!record.active || record.blocked || account.blocked)
					continue;
				// A complete first snapshot may exist only in the WAL. Validate
				// loadability after replay while checking existing raw files below.
				if (preflight)
					continue;
				player_load_request request;
				request.request_id = identities;
				request.pid = record.pid;
				request.account_name = name;
				request.deadline_usec =
					std::chrono::duration_cast<std::chrono::microseconds>(
						std::chrono::steady_clock::now().time_since_epoch())
						.count() +
					PLAYER_LOAD_TIMEOUT_USEC;
				const auto result =
					flatfile_player_load_repository_execute(root, request);
				require(result.outcome == player_load_outcome::applied &&
					result.stale_item_rows == 0 &&
					result.missing_payload_rows == 0 &&
					result.promoted_item_rows == 0 &&
					result.repaired_item_rows == 0);
				++loaded;
			}
		}
		for (const auto &entry : std::filesystem::directory_iterator(root + "/players"))
		{
			if (entry.path().extension() != ".snapshot")
				continue;
			const auto stem = entry.path().stem().string();
			size_t end = 0;
			const auto pid = std::stol(stem, &end);
			require(end == stem.size() && pid > 0 && pid <= INT32_MAX &&
				known.count(pid) == 1);
			player_snapshot snapshot;
			require(flatfile_player_snapshot_read(root, static_cast<int32_t>(pid),
							      &snapshot, &error) ==
				flatfile_player_load_result::ok);
			++snapshots;
		}
		for (const auto *pending :
		     { ".critical-authority-transaction", ".currency-transaction",
		       ".player-domain-transaction" })
			require(!std::filesystem::exists(root + "/domains/" + pending));
		std::cout << "{\"accounts\":" << accounts << ",\"identities\":" << identities
			  << ",\"players_loaded\":" << loaded << ",\"snapshots\":" << snapshots
			  << "}\n";
		return 0;
	}
	catch (...)
	{
		std::cerr << "native_restore_qualification_failed\n";
		return 1;
	}
}
