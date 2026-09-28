#ifndef DURIS_ECONOMIC_SQL_AUCTION_BID_TRANSACTION_H
#define DURIS_ECONOMIC_SQL_AUCTION_BID_TRANSACTION_H

#include "economy/auction_repository.h"
#include "persistence/economic_accounting_repository.h"

struct economic_sql_auction_bid_context
{
	economic_sql_authority_snapshot authority;
	uint32_t bank_id = 0;
	unsigned long session_id = 0;
};

// The caller owns an active inbox transaction. This locks the active epoch and
// all money lifetimes that a bid can mutate before the native repository runs.
unsigned int economic_sql_auction_bid_lock(MYSQL *connection, const critical_command &command,
					   economic_sql_auction_bid_context *context);

// Rechecks the locks, runs one bid, and stores its root, effects, and postings
// in that transaction. The caller writes receipt/outbox and commits once; any
// nonzero return requires rollback.
unsigned int
economic_sql_auction_bid_execute_and_record(MYSQL *connection, const critical_command &command,
					    const economic_sql_auction_bid_context &context,
					    auction_command_result *result,
					    unsigned int *result_code, bool *mutation_applied);

#endif
