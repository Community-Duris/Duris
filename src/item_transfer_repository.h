#ifndef ITEM_TRANSFER_REPOSITORY_H
#define ITEM_TRANSFER_REPOSITORY_H

#include "item_transfer_command.h"

struct st_mysql;
typedef struct st_mysql MYSQL;

bool item_transfer_repository_execute(MYSQL *connection, const critical_command &command,
				      item_transfer_result *result, unsigned int *result_code,
				      bool *mutation_applied);

#endif
