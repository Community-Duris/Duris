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
// ownership rows for the item tree, and owner revisions. The locks and this
// context expire at the caller's commit or rollback. Physical item rows and
// the gameplay shop configuration still need witnesses before activation.
unsigned int economic_sql_shop_trade_lock(MYSQL *connection, const critical_command &command,
					  economic_sql_shop_trade_context *context);

#endif
