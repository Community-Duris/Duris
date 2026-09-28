#include "persistence/economic_sql_shop_trade_transaction.h"

#include <cassert>
#include <cerrno>

int main()
{
	critical_command command = {};
	economic_sql_shop_trade_context context;
	context.keeper_id = 37;
	assert(economic_sql_shop_trade_lock(nullptr, command, &context) == ENOTSUP);
	assert(context.keeper_id == 37);
}
