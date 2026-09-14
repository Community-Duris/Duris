#ifndef DURIS_COLLECTOR_REPOSITORY_H
#define DURIS_COLLECTOR_REPOSITORY_H

#include "economy/collector_command.h"

#include <mysql/mysql.h>

// Executes one already-journaled collector command inside the caller's active
// transaction. Terminal policy/custody failures are returned through
// result_code without mutating domain state; database failures return false.
bool collector_repository_execute(MYSQL *connection, const critical_command &command,
				  collector_command_result *result, unsigned int *result_code,
				  bool *mutation_applied);

#endif
