#ifndef PLAYER_SNAPSHOT_REPOSITORY_H
#define PLAYER_SNAPSHOT_REPOSITORY_H

#include "player/player_save_worker.h"
#include <mysql/mysql.h>

// Caller owns the transaction. Shared by checkpoint and legacy save adapters.
bool player_snapshot_repository_write_pets(MYSQL *connection, const player_snapshot &snapshot);

player_save_apply_result player_snapshot_repository_apply(MYSQL *connection,
							  const player_snapshot &snapshot);
player_save_apply_result player_snapshot_repository_apply_from_pool(const player_snapshot &snapshot,
								    void *context);

#endif
