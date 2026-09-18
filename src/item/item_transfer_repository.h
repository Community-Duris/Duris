#ifndef ITEM_TRANSFER_REPOSITORY_H
#define ITEM_TRANSFER_REPOSITORY_H

#include "item/item_transfer_command.h"

#include <mysql/mysql.h>

bool item_transfer_repository_execute(MYSQL *connection, const critical_command &command,
				      item_transfer_result *result, unsigned int *result_code,
				      bool *mutation_applied);
// Compound commands may use one inbox operation for consecutive transfer
// segments. The offset keeps their item-ledger event indexes disjoint.
bool item_transfer_repository_execute_at_offset(MYSQL *connection, const critical_command &command,
						uint16_t event_index_base,
						item_transfer_result *result,
						unsigned int *result_code, bool *mutation_applied);
// Called only inside the enclosing coin transaction; never commits independently.
bool item_transfer_repository_execute_coin(MYSQL *connection, const critical_command &command,
					   const std::array<int32_t, 4> &before,
					   item_transfer_result *result, unsigned int *result_code,
					   bool *mutation_applied);
// Transaction-scoped owner primitives used by compound authority commands.
// Callers must acquire every participating owner in canonical identity order.
bool item_transfer_repository_ensure_owner(MYSQL *connection, const item_owner_identity &owner);
bool item_transfer_repository_lock_owner(MYSQL *connection, const item_owner_identity &owner,
					 uint64_t *revision);
bool item_transfer_repository_advance_owner(MYSQL *connection, const item_owner_identity &owner,
					    uint64_t prior_revision);
bool item_transfer_repository_destroy_owners(MYSQL *connection, const item_owner_identity *owners,
					     size_t owner_count);

#endif
