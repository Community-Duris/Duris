#ifndef COMBAT_OUTCOME_REPOSITORY_H
#define COMBAT_OUTCOME_REPOSITORY_H

#include "combat_outcome_command.h"

struct st_mysql;
typedef struct st_mysql MYSQL;

bool combat_outcome_repository_execute(MYSQL *connection, const critical_command &command,
				       combat_outcome_result *result, unsigned int *result_code,
				       bool *mutation_applied);

#endif
