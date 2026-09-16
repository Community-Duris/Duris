#ifndef DURIS_CURRENCY_REPOSITORY_H
#define DURIS_CURRENCY_REPOSITORY_H

#include "economy/currency_command.h"

#include <mysql/mysql.h>

// Executes one currency command inside the caller's already-open critical
// transaction. It never starts or commits a transaction independently.
bool currency_repository_execute(MYSQL *connection, const critical_command &command,
				 currency_command_result *result, unsigned int *result_code,
				 bool *mutation_applied);

#endif
