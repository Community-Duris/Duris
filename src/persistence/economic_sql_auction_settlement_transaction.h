#ifndef DURIS_ECONOMIC_SQL_AUCTION_SETTLEMENT_TRANSACTION_H
#define DURIS_ECONOMIC_SQL_AUCTION_SETTLEMENT_TRANSACTION_H

#include "economy/auction_repository.h"
#include "persistence/economic_accounting_repository.h"

struct economic_sql_auction_settlement_context
{
	economic_sql_authority_snapshot authority;
	uint32_t actor_bank_id = 0;
	unsigned long session_id = 0;
};

// Caller holds a pending inbox transaction. Lock the active lineage and all
// ordinary lifetimes used by a sale or removal before reading native authority.
unsigned int economic_sql_auction_settlement_lock(MYSQL *connection,
						  const critical_command &command,
						  economic_sql_auction_settlement_context *context);

// Renew the locks, close the native auction, and record the EAP1 money plan and
// unchanged item witnesses before the caller writes receipt/outbox and commits.
// Any error requires rollback of the caller-owned transaction.
unsigned int economic_sql_auction_settlement_execute_and_record(
	MYSQL *connection, const critical_command &command,
	const economic_sql_auction_settlement_context &context, auction_command_result *result,
	unsigned int *result_code, bool *mutation_applied);

#endif
