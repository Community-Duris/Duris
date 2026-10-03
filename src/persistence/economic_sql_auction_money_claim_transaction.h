#ifndef DURIS_ECONOMIC_SQL_AUCTION_MONEY_CLAIM_TRANSACTION_H
#define DURIS_ECONOMIC_SQL_AUCTION_MONEY_CLAIM_TRANSACTION_H

#include "economy/auction_repository.h"
#include "persistence/economic_accounting_repository.h"

struct economic_sql_auction_money_claim_context
{
	economic_sql_authority_snapshot authority;
	uint32_t bank_id = 0;
	unsigned long session_id = 0;
};

// Caller owns a pending inbox transaction. Lock the active epoch and the
// wallet, bank, and pending claim lifetimes before reading native authority.
unsigned int
economic_sql_auction_money_claim_lock(MYSQL *connection, const critical_command &command,
				      economic_sql_auction_money_claim_context *context);

// Refuse any unattributed/mismatched aggregate before mutation. Consume every
// locked source in the same transaction as the native pickup, EAP1 evidence,
// receipt, and outbox. Caller commits or rolls back the entire transaction.
unsigned int economic_sql_auction_money_claim_execute_and_record(
	MYSQL *connection, const critical_command &command,
	const economic_sql_auction_money_claim_context &context, auction_command_result *result,
	unsigned int *result_code, bool *mutation_applied);

#endif
