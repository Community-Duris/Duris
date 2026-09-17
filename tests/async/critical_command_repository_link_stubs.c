#include "sql/sql_pool.h"

/* Standalone repository harnesses never exercise the pool entry point.  Keep
 * that optional production dependency explicit without dragging the full game
 * runtime and its configured-connection globals into isolated tests. */
MYSQL *sql_pool_acquire(void)
{
	return nullptr;
}

void sql_pool_release(MYSQL *) {}

MYSQL *sql_pool_replace_connection(MYSQL *)
{
	return nullptr;
}
