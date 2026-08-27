#ifndef ARTIFACT_GUILD_REPOSITORY_H
#define ARTIFACT_GUILD_REPOSITORY_H

#include "artifact_guild_command.h"

#include <mysql/mysql.h>

bool artifact_guild_repository_execute(MYSQL *connection, const critical_command &command,
				       artifact_guild_result *result, unsigned int *result_code,
				       bool *mutation_applied);

#endif
