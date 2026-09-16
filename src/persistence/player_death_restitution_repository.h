#ifndef PLAYER_DEATH_RESTITUTION_REPOSITORY_H
#define PLAYER_DEATH_RESTITUTION_REPOSITORY_H

#include "persistence/critical_command_coordinator.h"
#include "persistence/player_death_restitution_command.h"

#include <mysql/mysql.h>

constexpr uint16_t PLAYER_DEATH_RESTITUTION_OUTBOX_DESTINATION = 11;
constexpr uint16_t PLAYER_DEATH_RESTITUTION_OUTBOX_EVENT = 1;
constexpr uint16_t PLAYER_DEATH_RESTITUTION_OUTBOX_PAYLOAD_VERSION = 1;

// Execute only the restitution mutation.  The caller owns the surrounding
// InnoDB transaction; no inbox row, commit, or rollback is performed here.
bool player_death_restitution_repository_execute(MYSQL *connection, const critical_command &command,
						 player_death_restitution_result *result,
						 unsigned int *result_code, bool *mutation_applied);

// Called by critical_command_repository_apply after START TRANSACTION and the
// parent critical inbox insert.  All receipt, item, delivery, runtime,
// projection, ownership, outbox, and inbox updates share one COMMIT.
critical_apply_result
player_death_restitution_repository_apply_in_transaction(MYSQL *connection,
							 const critical_command &command);

// Pure preflight used by tests and the runtime boundary.  Database evidence and
// ownership/revision fences are checked by execute().
bool player_death_restitution_repository_validate_plan(const player_death_restitution_plan &plan,
						       unsigned int *error_code);

#endif
