#ifndef DURIS_ECONOMIC_SQL_SHOP_TRADE_TRANSACTION_H
#define DURIS_ECONOMIC_SQL_SHOP_TRADE_TRANSACTION_H

#include "economy/shop_trade_accounting.h"
#include "persistence/economic_accounting_repository.h"

struct economic_sql_shop_trade_context
{
	economic_sql_authority_snapshot authority;
	shop_trade_accounting_authority before;
	uint32_t bank_id = 0;
	uint32_t keeper_id = 0;
	unsigned long session_id = 0;
};

// Standalone inactive shop capability. The caller owns an open inbox
// transaction. Lock all three money lifetimes, native cash and wallet rows,
// ownership and physical inventory rows for the item tree, and owner revisions.
// The locks and this context expire at the caller's commit or rollback. The
// gameplay shop configuration still needs a durable witness before activation.
unsigned int economic_sql_shop_trade_lock(MYSQL *connection, const critical_command &command,
					  economic_sql_shop_trade_context *context);

// Apply one admitted trade and record its canonical accounting root inside the
// caller's open inbox transaction. The caller writes the result receipt and
// outbox before commit; this function never commits.
unsigned int
economic_sql_shop_trade_execute_and_record(MYSQL *connection, const critical_command &command,
					   const economic_sql_shop_trade_context &context,
					   shop_trade_result *result, unsigned int *result_code,
					   bool *mutation_applied);

// Verify the retained canonical root, postings, item references, and source
// claim on operation-ID replay. The caller verifies the inbox receipt/outbox.
unsigned int economic_sql_shop_trade_verify_retained(MYSQL *connection,
						     const critical_command &command,
						     unsigned int result_code,
						     const uint8_t *result_payload,
						     size_t result_size);

#endif
