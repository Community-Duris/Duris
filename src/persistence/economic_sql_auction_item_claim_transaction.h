#ifndef DURIS_ECONOMIC_SQL_AUCTION_ITEM_CLAIM_TRANSACTION_H
#define DURIS_ECONOMIC_SQL_AUCTION_ITEM_CLAIM_TRANSACTION_H

#include "economy/auction_repository.h"
#include "persistence/economic_accounting_repository.h"

struct economic_sql_auction_item_claim_context
{
	economic_sql_authority_snapshot authority;
	uint32_t bank_id = 0;
	unsigned long session_id = 0;
};

// The caller holds an active inbox transaction. Lock the epoch and player
// money lifetimes before reading the staged claim and native custody authority.
unsigned int economic_sql_auction_item_claim_lock(MYSQL *connection,
						  const critical_command &command,
						  economic_sql_auction_item_claim_context *context);

// Renew locks, run the native item handoff, and record one EAP1 item event and
// exact ownership-ledger reference per UID. The caller commits receipt/outbox
// with this transaction or rolls the whole transaction back on any error.
unsigned int economic_sql_auction_item_claim_execute_and_record(
	MYSQL *connection, const critical_command &command,
	const economic_sql_auction_item_claim_context &context, auction_command_result *result,
	unsigned int *result_code, bool *mutation_applied);

#endif
