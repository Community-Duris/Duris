#ifndef DURIS_CORPSE_LIFECYCLE_REPOSITORY_H
#define DURIS_CORPSE_LIFECYCLE_REPOSITORY_H

#include "economy/collector_command.h"
#include "persistence/corpse_lifecycle_command.h"

#include <mysql/mysql.h>
#include <vector>

// Metadata is projected in the enclosing operation transaction, without replacing contents.
bool corpse_metadata_repository_execute(MYSQL *connection, const corpse_lifecycle_payload &payload,
					corpse_lifecycle_result *result, unsigned int *result_code);

// Executes one already-journaled disposal inside the caller's active critical
// transaction. The repository never starts or commits independently.
bool corpse_lifecycle_repository_execute(MYSQL *connection, const critical_command &command,
					 corpse_lifecycle_result *result, unsigned int *result_code,
					 bool *mutation_applied, uint64_t *collector_revision,
					 std::vector<collector_command_result> *collector_events);

#endif
