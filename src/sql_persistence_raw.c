/*
 * sql_persistence_raw.c — raw SQL execution on the persistence DB connection.
 * Separated from sql.c to keep that file manageable in size.
 * Called by the large-payload event queue worker thread.
 *
 * NOTE: Only compiles when __NO_MYSQL__ is NOT defined. The __NO_MYSQL__ stub
 * for sql_persistence_execute_raw() lives in sql.c alongside the other stubs.
 */

#include "prototypes.h"
#include "structs.h"
#include "utils.h"
#include "sql.h"
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#ifndef __NO_MYSQL__

#include <mysql.h>

extern MYSQL *DB;

#include "sql_pool.h"

/* Connection state is owned by sql.c; we just use it here.
 * persistenceDB is the legacy singleton fallback.
 * persistence_sql_mutex is kept for backward compat but is no longer
 * needed for connection serialisation (the pool handles that). */
extern MYSQL *persistenceDB;
extern pthread_mutex_t persistence_sql_mutex;

bool sql_persistence_execute_raw(const char *sql)
{
	MYSQL *db;
	int ret;
	bool need_repair = false;

	if (!sql || !*sql)
		return FALSE;

	db = sql_persistence_connection();
	if (!db)
	{
		logit(LOG_DEBUG, "Persistence MySQL: sql_persistence_execute_raw() failed - no connection");
		return FALSE;
	}

	/* The pool provides one connection per worker, so the raw execution
	 * path can run concurrently when the pool is active.  Keep the legacy
	 * singleton serialized if we ever fall back to it before pool init. */
	if (db == persistenceDB)
		pthread_mutex_lock(&persistence_sql_mutex);

	sql_clear_results_on(db);
	ret = mysql_real_query(db, sql, strlen(sql));
	if (!ret)
	{
		/* Drain every result set produced by CLIENT_MULTI_STATEMENTS so the
		 * pooled connection is clean for the next caller.  Ignore the returned
		 * result set when the statement has no rows; still advance through any
		 * remaining results until mysql_more_results() is false. */
		do
		{
			MYSQL_RES *res = mysql_store_result(db);
			if (res)
				mysql_free_result(res);
		} while (mysql_more_results(db) && mysql_next_result(db) == 0);

		if (mysql_more_results(db))
		{
			logit(LOG_DEBUG,
			      "Persistence MySQL error in sql_persistence_execute_raw(): %s",
			      mysql_error(db));
			logit(LOG_DEBUG,
			      "Persistence MySQL failed query (first 200 chars): %.200s",
			      sql);
			ret = 1;
			need_repair = (db != persistenceDB);
		}
	}
	else
	{
		logit(LOG_DEBUG, "Persistence MySQL error in sql_persistence_execute_raw(): %s", mysql_error(db));
		logit(LOG_DEBUG, "Persistence MySQL failed query (first 200 chars): %.200s", sql);
		need_repair = (db != persistenceDB);
	}

	if (need_repair)
	{
		MYSQL *replacement = sql_pool_replace_connection(db);
		if (replacement)
			db = replacement;
		else
			logit(LOG_DEBUG, "Persistence MySQL: failed to repair pooled connection after query error");
	}

	/* Release the connection back to the pool so other
	 * worker threads can use it. */
	sql_persistence_release_connection(db);

	if (db == persistenceDB)
		pthread_mutex_unlock(&persistence_sql_mutex);

	return ret == 0;
}
#endif /* __NO_MYSQL__ */
