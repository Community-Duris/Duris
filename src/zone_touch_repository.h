#ifndef ZONE_TOUCH_REPOSITORY_H
#define ZONE_TOUCH_REPOSITORY_H

#include "zone_touch_command.h"

#include <mysql/mysql.h>

bool zone_touch_repository_execute(MYSQL *connection, const critical_command &command,
				   zone_touch_result *result, unsigned int *result_code,
				   bool *mutation_applied);

#endif
