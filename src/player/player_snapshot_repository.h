#ifndef PLAYER_SNAPSHOT_REPOSITORY_H
#define PLAYER_SNAPSHOT_REPOSITORY_H

#include "player/player_save_worker.h"
#include <mysql/mysql.h>

#include <string>
#include <cstdint>

// Caller owns the transaction. Shared by checkpoint and legacy save adapters.
bool player_snapshot_repository_write_pets(MYSQL *connection, const player_snapshot &snapshot);

player_save_apply_result player_snapshot_repository_apply(MYSQL *connection,
							  const player_snapshot &snapshot);
player_save_apply_result player_snapshot_repository_apply_from_pool(const player_snapshot &snapshot,
								    void *context);

// One log_entries row (sql_log()), with the time it was logged. Strings are written as
// given; the caller keeps them within their columns.
struct log_entry_snapshot
{
	int64_t logged_at = 0;
	std::string kind;
	std::string ip_address;
	int32_t pid = 0;
	std::string player_name;
	int32_t zone_number = 0;
	int32_t room_vnum = 0;
	std::string message;
};

player_save_apply_result log_entry_repository_apply(MYSQL *connection,
						    const log_entry_snapshot &entry);
player_save_apply_result log_entry_repository_apply_from_pool(const log_entry_snapshot &entry);

#endif
