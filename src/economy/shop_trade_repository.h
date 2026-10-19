#ifndef DURIS_SHOP_TRADE_REPOSITORY_H
#define DURIS_SHOP_TRADE_REPOSITORY_H
#include "economy/shop_trade_command.h"
#include <mysql/mysql.h>

// The critical repository owns the enclosing inbox transaction.
bool shop_trade_repository_execute(MYSQL *connection, const critical_command &command,
				   shop_trade_result *result);
#endif
