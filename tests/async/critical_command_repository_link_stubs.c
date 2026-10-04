#include "sql/sql_pool.h"

/* Standalone repository harnesses never exercise the pool entry point.  Keep
 * that optional production dependency explicit without dragging the full game
 * runtime and its configured-connection globals into isolated tests. */
MYSQL *sql_pool_acquire(void)
{
	return nullptr;
}

void sql_pool_release(MYSQL *) {}

bool sql_pool_retire_owned_connection(MYSQL *)
{
	// No pool lease exists in this fixture; never consume a borrowed/direct handle.
	return false;
}

MYSQL *sql_pool_replace_connection(MYSQL *)
{
	return nullptr;
}
