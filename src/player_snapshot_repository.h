#ifndef PLAYER_SNAPSHOT_REPOSITORY_H
#define PLAYER_SNAPSHOT_REPOSITORY_H

#include "player_save_worker.h"
#include <mysql/mysql.h>

player_save_apply_result player_snapshot_repository_apply(MYSQL *connection,
							  const player_snapshot &snapshot);
player_save_apply_result player_snapshot_repository_apply_from_pool(const player_snapshot &snapshot,
								    void *context);

#endif
