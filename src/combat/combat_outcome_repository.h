#ifndef COMBAT_OUTCOME_REPOSITORY_H
#define COMBAT_OUTCOME_REPOSITORY_H

#include "combat/combat_outcome_command.h"

#include <mysql/mysql.h>

bool combat_outcome_repository_execute(MYSQL *connection, const critical_command &command,
				       combat_outcome_result *result, unsigned int *result_code,
				       bool *mutation_applied);

#endif
