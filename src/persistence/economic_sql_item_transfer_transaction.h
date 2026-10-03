#ifndef DURIS_ECONOMIC_SQL_ITEM_TRANSFER_TRANSACTION_H
#define DURIS_ECONOMIC_SQL_ITEM_TRANSFER_TRANSACTION_H

#include "economy/item_transfer_accounting.h"
#include "item/item_transfer_repository.h"
#include "persistence/economic_accounting_repository.h"

#include <mysql/mysql.h>

#include <cstddef>
#include <cstdint>

struct economic_sql_item_transfer_context
{
	economic_sql_authority_snapshot authority;
	unsigned long session_id = 0;
};

// Lock and validate the active lineage epoch before any item rows are changed.
// The caller's transaction owns this shared lock through commit or rollback.
unsigned int economic_sql_item_transfer_lock(MYSQL *connection, const critical_command &command,
					     economic_sql_item_transfer_context *context);

// Record a typed item-transfer root and its exact ownership-event references
// inside the caller's existing SQL transaction. This function never commits.
unsigned int economic_sql_item_transfer_record(MYSQL *connection, const critical_command &command,
					       const item_transfer_custody_delta *custody_delta,
					       unsigned int result_code, bool mutation_applied,
					       const economic_sql_item_transfer_context &context);

// Verify the exact root, canonical plan, references, and retained result on an
// operation-ID replay. This does not consult the current epoch or item state.
unsigned int economic_sql_item_transfer_verify_retained(MYSQL *connection,
							const critical_command &command,
							unsigned int result_code,
							const uint8_t *result_payload,
							size_t result_size);

#endif
