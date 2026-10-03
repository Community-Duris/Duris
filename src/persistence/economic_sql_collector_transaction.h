#ifndef DURIS_ECONOMIC_SQL_COLLECTOR_TRANSACTION_H
#define DURIS_ECONOMIC_SQL_COLLECTOR_TRANSACTION_H

#include "economy/collector_repository.h"
#include "persistence/economic_accounting_repository.h"

struct economic_sql_collector_context
{
	economic_sql_authority_snapshot authority;
	economic_account_key wallet_account = {};
	economic_account_key bank_account = {};
	uint32_t bank_id = 0;
	unsigned long session_id = 0;
};

// The caller holds an active inbox transaction. This takes the active epoch
// and (for purchase) both lifetime mapping locks before the native writer.
unsigned int economic_sql_collector_lock(MYSQL *connection, const critical_command &command,
					 economic_sql_collector_context *context);

// Runs the singleton collector mutation and records its EAP1 root, money rows,
// and exact item reference in the same transaction. Never commits, writes the
// receipt, or publishes the outbox; the caller must do those and roll back on
// any nonzero return. Business rejections return zero here with result_code.
unsigned int
economic_sql_collector_execute_and_record(MYSQL *connection, const critical_command &command,
					  const economic_sql_collector_context &context,
					  collector_command_result *result,
					  unsigned int *result_code, bool *mutation_applied);

// Check a committed root against its frozen intent, canonical plan and exact
// accounting rows. The caller checks the inbox receipt and outbox separately.
unsigned int economic_sql_collector_verify_retained(MYSQL *connection,
						    const critical_command &command,
						    unsigned int result_code,
						    const uint8_t *result_payload,
						    size_t result_size);

#endif
