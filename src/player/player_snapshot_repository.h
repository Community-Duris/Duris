#ifndef PLAYER_SNAPSHOT_REPOSITORY_H
#define PLAYER_SNAPSHOT_REPOSITORY_H

#include "player/player_save_worker.h"
#include <mysql/mysql.h>

// Caller owns the transaction. Shared by checkpoint and legacy save adapters.
bool player_snapshot_repository_write_pets(MYSQL *connection, const player_snapshot &snapshot);

enum class player_death_terminal_write_outcome : uint8_t
{
	written,
	already_written,
	failed,
};

struct player_death_terminal_write_result
{
	player_death_terminal_write_outcome outcome;
	unsigned int error_code;
};

// Internal transaction participant, NOT a durability acknowledgement. Requires
// the exact retained evidence row and its source revision in the caller's live
// transaction. Never commits, projects items/pets, changes custody, or converts
// money. The conflict owner must commit before acknowledging terminal release.
player_death_terminal_write_result
player_snapshot_repository_write_retained_death(MYSQL *connection, const player_snapshot &request,
						const player_snapshot &retained,
						player_revision_t source_revision);

player_save_apply_result player_snapshot_repository_apply(MYSQL *connection,
							  const player_snapshot &snapshot);
struct player_save_recovery_record;
player_save_apply_result
player_snapshot_repository_recovery_apply(MYSQL *, const player_save_recovery_record &);
player_save_apply_result player_snapshot_repository_apply_from_pool(const player_snapshot &snapshot,
								    void *context);

#endif
