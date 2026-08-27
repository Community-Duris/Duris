#ifndef BOON_REWARD_REPOSITORY_H
#define BOON_REWARD_REPOSITORY_H

#include "boon_reward_command.h"

struct st_mysql;
typedef struct st_mysql MYSQL;

bool boon_reward_repository_execute(MYSQL *connection, const critical_command &command,
				    boon_reward_result *result, unsigned int *result_code,
				    bool *mutation_applied);

#endif
