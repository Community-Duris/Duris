#ifndef DURIS_ECONOMIC_SQL_AUCTION_LISTING_TRANSACTION_H
#define DURIS_ECONOMIC_SQL_AUCTION_LISTING_TRANSACTION_H

#include "economy/auction_repository.h"
#include "persistence/economic_accounting_repository.h"

struct economic_sql_auction_listing_context
{
	economic_sql_authority_snapshot authority;
	uint32_t bank_id = 0;
	unsigned long session_id = 0;
};

// The caller holds the inbox transaction. Lock the active epoch and existing
// wallet/bank lifetimes before reading the listing's native item authority.
unsigned int economic_sql_auction_listing_lock(MYSQL *connection, const critical_command &command,
					       economic_sql_auction_listing_context *context);

// Create the auction and escrow mapping, then record its fee and ordered item
// references in that same transaction. The caller commits receipt/outbox or
// rolls back the entire transaction on any nonzero return.
unsigned int
economic_sql_auction_listing_execute_and_record(MYSQL *connection, const critical_command &command,
						const economic_sql_auction_listing_context &context,
						auction_command_result *result,
						unsigned int *result_code, bool *mutation_applied);

#endif
