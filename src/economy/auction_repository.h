#ifndef AUCTION_REPOSITORY_H
#define AUCTION_REPOSITORY_H

#include "economy/auction_command.h"

#include <mysql/mysql.h>

bool auction_repository_execute(MYSQL *connection, const critical_command &command,
				auction_command_result *result, unsigned int *result_code,
				bool *mutation_applied);

#endif
