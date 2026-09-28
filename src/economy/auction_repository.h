#ifndef AUCTION_REPOSITORY_H
#define AUCTION_REPOSITORY_H

#include "economy/auction_command.h"

#include <mysql/mysql.h>

bool auction_repository_execute(MYSQL *connection, const critical_command &command,
				auction_command_result *result, unsigned int *result_code,
				bool *mutation_applied);

// Only schema-2 auction components may call this entry point, inside their
// already-locked inbox transaction. The caller must record the matching EAP1
// root before writing the receipt and committing. Other actions refuse.
bool auction_repository_execute_accounted(MYSQL *connection, const critical_command &command,
					  auction_command_result *result, unsigned int *result_code,
					  bool *mutation_applied);

#endif
