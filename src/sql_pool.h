/*
 * sql_pool.h — MySQL connection pool for async persistence workers.
 *
 * Replaces the single persistenceDB connection shared by
 * 3 persistence worker threads with a fixed-size pool (default 4).
 * Each worker acquires a connection, executes its query, and releases
 * it back — eliminating the persistence_sql_mutex bottleneck and
 * allowing concurrent MySQL queries across worker threads.
 *
 * Thread safety: all public functions are internally synchronized.
 */

#ifndef __SQL_POOL_H__
#define __SQL_POOL_H__

#include <mysql.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Pool sizing */
#define SQL_POOL_DEFAULT_SIZE 4
#define SQL_POOL_MAX_SIZE     16
#define SQL_POOL_ACQUIRE_TIMEOUT_MS 2000

/* ---- Lifecycle ---- */

/* Initialise the pool with `size` connections to the database
 * identified by the DB_HOST / DB_USER / DB_PASSWD / DB_NAME / DB_PORT
 * macros.  Call once after initialize_mysql() succeeds.
 * Returns 0 on success, -1 on failure. */
int sql_pool_init(int size);

/* Close every connection in the pool and free all memory.
 * Safe to call more than once. */
void sql_pool_shutdown(void);

/* ---- Connection borrowing ---- */

/* Acquire a connection from the pool. Waits up to
 * SQL_POOL_ACQUIRE_TIMEOUT_MS when all connections are in use.
 * Returns NULL if the pool has not been initialised, is closing, or
 * remains exhausted at the deadline. */
MYSQL *sql_pool_acquire(void);

/* As above, while reporting whether an active pool existed when the
 * acquisition began. This lets bootstrap callers distinguish "no pool"
 * (legacy fallback is allowed) from "active pool exhausted" (fail closed). */
MYSQL *sql_pool_acquire_with_status(int *pool_was_active);

/* Return a connection to the pool so another thread can use it.
 * Signals one waiting acquirer.  No-op when conn is NULL. */
void sql_pool_release(MYSQL *conn);

/* Replace a pooled connection with a fresh handle after it has become
 * unusable. Returns the replacement handle on success, or NULL if the
 * pool is unavailable or reconnect fails. */
MYSQL *sql_pool_replace_connection(MYSQL *conn);

/* ---- Stats (debug / monitoring) ---- */

int sql_pool_is_active(void);    /* whether a usable pool was initialised */
int sql_pool_available(void);
int sql_pool_in_use(void);      /* how many are currently borrowed */
int sql_pool_total(void);       /* total pool size (0 before init) */

#ifdef __cplusplus
}
#endif

#endif /* __SQL_POOL_H__ */
