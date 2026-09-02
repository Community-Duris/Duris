#ifndef ITEM_TRANSFER_REPOSITORY_H
#define ITEM_TRANSFER_REPOSITORY_H

#include "item/item_transfer_command.h"

#include <mysql/mysql.h>

bool item_transfer_repository_execute(MYSQL *connection, const critical_command &command,
				      item_transfer_result *result, unsigned int *result_code,
				      bool *mutation_applied);
bool item_transfer_repository_destroy_owners(MYSQL *connection, const item_owner_identity *owners,
					     size_t owner_count);

#endif
