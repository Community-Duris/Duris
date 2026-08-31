/*
 * sql_persistence_raw.c -- raw SQL execution on the persistence DB connection.
 * Separated from sql.c to keep that file manageable in size.
 * Called by the large-payload event queue worker thread.
 *
 * NOTE: Only compiles when __NO_MYSQL__ is NOT defined. The __NO_MYSQL__ stub
 * for sql_persistence_execute_raw() lives in sql.c alongside the other stubs.
 */

#include "prototypes.h"
#include "structs.h"
#include "utils.h"
#include "sql/sql.h"
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#ifndef __NO_MYSQL__

#include <mysql.h>

extern MYSQL *DB;

#include "sql/sql_pool.h"

/* Connection state is owned by sql.c; we just use it here.
 * persistenceDB is the legacy singleton fallback.
 * persistence_sql_mutex is kept for backward compat but is no longer
 * needed for connection serialisation (the pool handles that). */
extern MYSQL *persistenceDB;
extern pthread_mutex_t persistence_sql_mutex;

bool sql_persistence_execute_raw(const char *sql)
{
	(void)sql;
	return false;
}
#endif /* __NO_MYSQL__ */
