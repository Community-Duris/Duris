#ifndef AUCTION_REPOSITORY_H
#define AUCTION_REPOSITORY_H

#include "auction_command.h"

struct st_mysql;
typedef struct st_mysql MYSQL;

bool auction_repository_execute(MYSQL *connection, const critical_command &command,
				auction_command_result *result, unsigned int *result_code,
				bool *mutation_applied);

#endif
