#ifndef BOON_REWARD_REPOSITORY_H
#define BOON_REWARD_REPOSITORY_H

#include "boon_reward_command.h"

#include <mysql/mysql.h>

bool boon_reward_repository_execute(MYSQL *connection, const critical_command &command,
				    boon_reward_result *result, unsigned int *result_code,
				    bool *mutation_applied);

#endif
