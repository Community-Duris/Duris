#ifndef DURIS_COLLECTOR_REPOSITORY_H
#define DURIS_COLLECTOR_REPOSITORY_H

#include "economy/collector_command.h"

#include <mysql/mysql.h>

// Read-only restart bootstrap. The implementation uses one streaming statement
// so the catalog cursor and every record come from the same committed snapshot.
// On failure, catalog is unchanged and errno carries an errno/MySQL-style code.
bool collector_repository_read_catalog(MYSQL *connection, collector::catalog *catalog);

// Executes one already-journaled collector command inside the caller's active
// transaction. Terminal policy/custody failures are returned through
// result_code without mutating domain state; database failures return false.
bool collector_repository_execute(MYSQL *connection, const critical_command &command,
				  collector_command_result *result, unsigned int *result_code,
				  bool *mutation_applied);

#endif
