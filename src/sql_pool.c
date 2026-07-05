/*
 * sql_pool.c — MySQL connection pool implementation.
 *
 * Fixed-size pool of MYSQL* connections shared by the
 * 3 async persistence worker threads (item, scalar, large-payload).
 *
 * Each connection is created with CLIENT_MULTI_STATEMENTS and
 * utf8mb4 charset, matching the main DB connection.
 *
 * Thread safety: pool_mutex protects slot[] and pool_size;
 * pool_cond is signalled when a connection is released, waking
 * one blocked acquirer.
 */

#include "prototypes.h"
#include "structs.h"
#include "utils.h"
#include "sql.h"
#include "sql_pool.h"

#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#ifndef __NO_MYSQL__

#include <mysql.h>

/* ------------------------------------------------------------------ */
/*  Internal pool state                                                */
/* ------------------------------------------------------------------ */

typedef struct
{
	MYSQL *conn;
	int    in_use;     /* boolean: 1 = borrowed, 0 = free */
} sql_pool_slot_t;

static sql_pool_slot_t *pool       = NULL;
static int              pool_size  = 0;
static pthread_mutex_t  pool_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t   pool_cond  = PTHREAD_COND_INITIALIZER;

static MYSQL *sql_pool_create_connection(const char *site, int slot)
{
	MYSQL *conn = mysql_init(NULL);
	if (!conn)
	{
		logit(LOG_DEBUG, "%s: mysql_init failed for slot %d", site, slot);
		return NULL;
	}

	/* Match the main connection: 10-second read/write timeouts. */
	unsigned int timeout = 10;
	mysql_options(conn, MYSQL_OPT_READ_TIMEOUT, &timeout);
	mysql_options(conn, MYSQL_OPT_WRITE_TIMEOUT, &timeout);

	/* Connect with CLIENT_MULTI_STATEMENTS so multi-statement batches work
	 * through the pool too. Use the shared db-name resolver so every slot
	 * agrees with the main DB connection about which database to target. */
	if (!mysql_real_connect(conn,
	                        DB_HOST,
	                        DB_USER,
	                        DB_PASSWD,
	                        sql_persistence_db_name(),
	                        DB_PORT,
	                        NULL,             /* unix_socket */
	                        CLIENT_MULTI_STATEMENTS))
	{
		logit(LOG_DEBUG, "%s: mysql_real_connect failed for slot %d: %s",
		      site, slot, mysql_error(conn));
		mysql_close(conn);
		return NULL;
	}

	mysql_set_character_set(conn, "utf8mb4");
	return conn;
}

/* ------------------------------------------------------------------ */
/*  Lifecycle                                                          */
/* ------------------------------------------------------------------ */

int sql_pool_init(int size)
{
	if (pool)
	{
		logit(LOG_DEBUG, "sql_pool_init: pool already initialised");
		return -1;
	}

	if (size <= 0)
		size = SQL_POOL_DEFAULT_SIZE;
	if (size > SQL_POOL_MAX_SIZE)
		size = SQL_POOL_MAX_SIZE;

	pool = (sql_pool_slot_t *)calloc((size_t)size, sizeof(sql_pool_slot_t));
	if (!pool)
	{
		logit(LOG_DEBUG, "sql_pool_init: calloc(%d) failed", size);
		return -1;
	}

	pool_size = size;

	for (int i = 0; i < size; i++)
	{
		MYSQL *conn = sql_pool_create_connection("sql_pool_init", i);
		if (!conn)
		{
			/* Clean up slots already created. */
			for (int j = 0; j < i; j++)
			{
				if (pool[j].conn)
					mysql_close(pool[j].conn);
			}
			free(pool);
			pool      = NULL;
			pool_size = 0;
			return -1;
		}

		pool[i].conn    = conn;
		pool[i].in_use  = 0;
	}

	logit(LOG_STATUS, "SQL connection pool initialised with %d connections.", size);
	return 0;
}

void sql_pool_shutdown(void)
{
	pthread_mutex_lock(&pool_mutex);

	if (!pool)
	{
		pthread_mutex_unlock(&pool_mutex);
		return;
	}

	for (int i = 0; i < pool_size; i++)
	{
		if (pool[i].conn)
		{
			mysql_close(pool[i].conn);
			pool[i].conn = NULL;
		}
		pool[i].in_use = 0;
	}

	free(pool);
	pool      = NULL;
	pool_size = 0;

	/* Wake every thread blocked in sql_pool_acquire().  They will see
	 * pool == NULL and return gracefully. */
	pthread_cond_broadcast(&pool_cond);

	pthread_mutex_unlock(&pool_mutex);

	logit(LOG_STATUS, "SQL connection pool shut down.");
}

/* ------------------------------------------------------------------ */
/*  Acquire / Release                                                  */
/* ------------------------------------------------------------------ */

MYSQL *sql_pool_acquire(void)
{
	MYSQL *conn = NULL;

	pthread_mutex_lock(&pool_mutex);

	if (!pool)
	{
		pthread_mutex_unlock(&pool_mutex);
		return NULL;
	}

	while (1)
	{
		/* Linear scan for a free slot — pool is small (4–16), so O(n)
		 * is fine. */
		for (int i = 0; i < pool_size; i++)
		{
			if (!pool[i].in_use && pool[i].conn)
			{
				pool[i].in_use = 1;
				conn            = pool[i].conn;
				pthread_mutex_unlock(&pool_mutex);
				return conn;
			}
		}

		/* All busy — block until someone releases. */
		pthread_cond_wait(&pool_cond, &pool_mutex);

		if (!pool)
		{
			pthread_mutex_unlock(&pool_mutex);
			return NULL;
		}
	}
}

void sql_pool_release(MYSQL *conn)
{
	if (!conn)
		return;

	pthread_mutex_lock(&pool_mutex);

	if (!pool)
	{
		pthread_mutex_unlock(&pool_mutex);
		return;
	}

	for (int i = 0; i < pool_size; i++)
	{
		if (pool[i].conn == conn)
		{
			pool[i].in_use = 0;
			pthread_cond_signal(&pool_cond);
			break;
		}
	}

	pthread_mutex_unlock(&pool_mutex);
}

MYSQL *sql_pool_replace_connection(MYSQL *conn)
{
	MYSQL *replacement = NULL;
	MYSQL *old_conn    = NULL;
	int    slot        = -1;

	if (!conn)
		return NULL;

	pthread_mutex_lock(&pool_mutex);
	if (!pool)
	{
		pthread_mutex_unlock(&pool_mutex);
		return NULL;
	}

	for (int i = 0; i < pool_size; i++)
	{
		if (pool[i].conn == conn)
		{
			slot     = i;
			old_conn = pool[i].conn;
			break;
		}
	}
	pthread_mutex_unlock(&pool_mutex);

	if (slot < 0)
		return NULL;

	replacement = sql_pool_create_connection("sql_pool_replace_connection", slot);
	if (!replacement)
	{
		pthread_mutex_lock(&pool_mutex);
		if (pool && slot < pool_size && pool[slot].conn == old_conn)
		{
			mysql_close(pool[slot].conn);
			pool[slot].conn    = NULL;
			pool[slot].in_use  = 0;
			pthread_cond_signal(&pool_cond);
		}
		pthread_mutex_unlock(&pool_mutex);
		return NULL;
	}

	pthread_mutex_lock(&pool_mutex);
	if (!pool || slot >= pool_size || pool[slot].conn != old_conn)
	{
		pthread_mutex_unlock(&pool_mutex);
		mysql_close(replacement);
		return NULL;
	}

	mysql_close(pool[slot].conn);
	pool[slot].conn = replacement;
	pthread_mutex_unlock(&pool_mutex);
	return replacement;
}

/* ------------------------------------------------------------------ */
/*  Stats                                                              */
/* ------------------------------------------------------------------ */

int sql_pool_available(void)
{
	int avail = 0;
	pthread_mutex_lock(&pool_mutex);
	if (pool)
	{
		for (int i = 0; i < pool_size; i++)
			if (!pool[i].in_use)
				avail++;
	}
	pthread_mutex_unlock(&pool_mutex);
	return avail;
}

int sql_pool_in_use(void)
{
	int used = 0;
	pthread_mutex_lock(&pool_mutex);
	if (pool)
	{
		for (int i = 0; i < pool_size; i++)
			if (pool[i].in_use)
				used++;
	}
	pthread_mutex_unlock(&pool_mutex);
	return used;
}

int sql_pool_total(void)
{
	int total;
	pthread_mutex_lock(&pool_mutex);
	total = pool_size;
	pthread_mutex_unlock(&pool_mutex);
	return total;
}

#else  /* __NO_MYSQL__ */

/* Stubs — no MySQL available.  The pool is a no-op. */

int sql_pool_init(int size)
{
	(void)size;
	return -1;
}

void sql_pool_shutdown(void) {}

MYSQL *sql_pool_acquire(void)
{
	return NULL;
}

void sql_pool_release(MYSQL *conn)
{
	(void)conn;
}

MYSQL *sql_pool_replace_connection(MYSQL *conn)
{
	(void)conn;
	return NULL;
}

int sql_pool_available(void) { return 0; }
int sql_pool_in_use(void)    { return 0; }
int sql_pool_total(void)     { return 0; }

#endif /* __NO_MYSQL__ */
