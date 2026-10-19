#include "persistence/ram_runtime.h"
#include "persistence/ram_command.h"
#include "persistence/operation_log.h"
#include "persistence/persistence_mode.h"
#include "persistence/critical_command_repository.h"
#include "player/player_checkpoint_command.h"
#include "player/player_save_journal.h"
#include "player/player_snapshot_repository.h"
#include "flatfile/flatfile_item_repository.h"
#include "flatfile/flatfile_player_repository.h"
#include "economy/economic_state.h"
#include "economy/economic_state_repository.h"
#include "economy/auction_ram.h"
#include "guild/artifact_guild_state.h"

#include <cerrno>
#include <cstring>
#include <set>
#include <thread>

namespace
{
MYSQL *startup_connection = nullptr;
std::thread::id world_thread;
std::set<std::array<uint8_t, CRITICAL_COMMAND_ID_BYTES>> historical_ids;

player_save_apply_result snapshot_apply(const player_snapshot &snapshot, void *)
{
	if (!persistence_mode_requires_mysql())
		return flatfile_player_snapshot_apply_selected(snapshot, nullptr);
#ifdef __NO_MYSQL__
	return { player_save_apply_outcome::retryable_failure, 0, ENOTSUP };
#else
	return std::this_thread::get_id() == world_thread ?
		       player_snapshot_repository_apply(startup_connection, snapshot) :
		       player_snapshot_repository_apply_from_pool(snapshot, nullptr);
#endif
}

critical_apply_result project(const critical_command &command, void *)
{
	if (command.type == critical_command_type::player_checkpoint)
		return player_checkpoint_command_project(command, snapshot_apply, nullptr);
	if (!persistence_mode_requires_mysql())
		return flatfile_critical_command_repository_apply_selected(command, nullptr);
#ifdef __NO_MYSQL__
	return { critical_apply_outcome::retryable_failure, 0, ENOTSUP };
#else
	return std::this_thread::get_id() == world_thread ?
		       critical_command_repository_apply(startup_connection, command) :
		       critical_command_repository_apply_from_pool(command, nullptr);
#endif
}

critical_receipt_status lookup(const critical_command &request, critical_apply_result *receipt)
{
	const auto found = operation_log_lookup(request, receipt);
	if (found != critical_receipt_status::missing)
		return found;
	// Old SQL receipts retain a hash that includes the old arrival timestamp,
	// not the original request. Do not guess a new identity or execute them twice.
	return historical_ids.contains(request.operation_id.bytes) ?
		       critical_receipt_status::identity_conflict :
		       critical_receipt_status::missing;
}
} // namespace

bool ram_runtime_recover(MYSQL *connection, const char *operation_directory,
			 const char *legacy_player_directory, std::string *error)
try
{
	startup_connection = connection;
	world_thread = std::this_thread::get_id();
	historical_ids.clear();
	if (!player_save_journal_init(legacy_player_directory))
	{
		if (error)
			*error = "Cannot open the legacy player journal for transition";
		return false;
	}
	const auto replayed = player_save_journal_replay(snapshot_apply, nullptr);
	player_save_journal_shutdown();
	if (replayed != player_save_journal_result::ok)
	{
		if (error)
			*error = "Legacy player journal replay failed; records retained";
		return false;
	}
	if (!operation_log_init(operation_directory, project, nullptr))
	{
		if (error)
			*error = "Atomic operation-log recovery failed; records retained";
		return false;
	}
#ifndef __NO_MYSQL__
	if (!persistence_mode_requires_mysql())
		return true;
	const char *sql = "SELECT operation_id FROM critical_operation_inbox";
	if (!connection || mysql_real_query(connection, sql, strlen(sql)))
		return false;
	MYSQL_RES *rows = mysql_store_result(connection);
	if (!rows)
		return false;
	bool valid = true;
	while (MYSQL_ROW row = mysql_fetch_row(rows))
	{
		const auto *lengths = mysql_fetch_lengths(rows);
		if (!row[0] || !lengths || lengths[0] != CRITICAL_COMMAND_ID_BYTES)
		{
			valid = false;
			break;
		}
		std::array<uint8_t, CRITICAL_COMMAND_ID_BYTES> id{};
		memcpy(id.data(), row[0], id.size());
		historical_ids.insert(id);
	}
	mysql_free_result(rows);
	if (!valid)
		return false;
#endif
	return true;
}
catch (...)
{
	if (error)
		*error = "Economic recovery could not retain complete startup state";
	return false;
}

bool ram_runtime_hydrate(MYSQL *connection, const char *flatfile_root, std::string *error)
{
	startup_connection = connection;
	return operation_log_ready() &&
	       economic_state_repository_load(
		       connection, persistence_mode_requires_mysql() ? nullptr : flatfile_root,
		       error);
}

bool ram_runtime_start()
{
#ifndef __NO_MYSQL__
	// World boot can run legacy artifact registration after the recovered base
	// was loaded. Refresh that boundary once before opening economic admission.
	if (persistence_mode_requires_mysql() &&
	    (!economic_state_repository_refresh_artifacts(startup_connection) ||
	     !artifact_guild_state_hydrate()))
		return false;
#endif
	if (!operation_log_ready() ||
	    !critical_command_coordinator_init_ram(ram_command_prepare, nullptr, ram_command_submit,
						   lookup, operation_log_remember_rejection))
		return false;
	economic_state_activate();
	return true;
}

void ram_runtime_shutdown()
{
	// The caller has stopped admission and the ordered writer before releasing
	// the journal lease or any state used by its callbacks.
	operation_log_shutdown();
	economic_state_reset();
	auction_ram_reset();
	historical_ids.clear();
	startup_connection = nullptr;
}
