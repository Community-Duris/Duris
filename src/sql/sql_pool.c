/*
 * sql_pool.c -- MySQL connection pool implementation.
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

#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utils.h"
#include "sql/sql.h"
#include "sql/sql_exclusion_guard.h"
#include "sql/sql_pool.h"

#include <pthread.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifndef __NO_MYSQL__

#include <mysql.h>

/* ------------------------------------------------------------------ */
/*  Internal pool state                                                */
/* ------------------------------------------------------------------ */

typedef struct
{
	MYSQL *conn;
	int in_use; /* boolean: 1 = borrowed, 0 = free */
	int discard_on_release;
} sql_pool_slot_t;

static sql_pool_slot_t *pool = NULL;
static int pool_size = 0;
static pthread_mutex_t pool_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t pool_cond = PTHREAD_COND_INITIALIZER;
static int pool_closing = 0;

static MYSQL *sql_pool_create_connection(const char *site, int slot)
{
	MYSQL *conn = sql_open_configured_connection(CLIENT_MULTI_STATEMENTS);
	if (!conn)
	{
		logit(LOG_DEBUG, "%s: validated connection failed for slot %d", site, slot);
		return NULL;
	}
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
	pool_closing = 0;

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
			pool = NULL;
			pool_size = 0;
			return -1;
		}

		pool[i].conn = conn;
		pool[i].in_use = 0;
	}

	logit(LOG_STATUS, "SQL connection pool initialised with %d connections.", size);
	return 0;
}

void sql_pool_shutdown(void)
{
	pthread_mutex_lock(&pool_mutex);

	if (!pool)
	{
		pool_closing = 0;
		pthread_mutex_unlock(&pool_mutex);
		return;
	}

	/* Prevent new borrowers and wake any threads waiting in acquire(). */
	pool_closing = 1;
	pthread_cond_broadcast(&pool_cond);

	/* Borrowers own the MYSQL handle outside pool_mutex.  Do not close
	 * anything until every borrower has returned its handle. */
	while (1)
	{
		int borrowed = 0;
		for (int i = 0; i < pool_size; i++)
			borrowed += pool[i].in_use;
		if (borrowed == 0)
			break;
		pthread_cond_wait(&pool_cond, &pool_mutex);
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
	pool = NULL;
	pool_size = 0;
	pool_closing = 0;

	/* Wake every thread blocked in sql_pool_acquire().  They will see
	 * pool == NULL and return gracefully. */
	pthread_cond_broadcast(&pool_cond);

	pthread_mutex_unlock(&pool_mutex);

	logit(LOG_STATUS, "SQL connection pool shut down.");
}

/* ------------------------------------------------------------------ */
/*  Acquire / Release                                                  */
/* ------------------------------------------------------------------ */

MYSQL *sql_pool_acquire_with_status(int *pool_was_active)
{
	MYSQL *conn = NULL;
	struct timespec deadline;
	int wait_result;

	if (pool_was_active)
		*pool_was_active = 0;

	pthread_mutex_lock(&pool_mutex);

	if (!pool || pool_closing)
	{
		pthread_mutex_unlock(&pool_mutex);
		return NULL;
	}
	if (pool_was_active)
		*pool_was_active = 1;

	clock_gettime(CLOCK_REALTIME, &deadline);
	deadline.tv_sec += SQL_POOL_ACQUIRE_TIMEOUT_MS / 1000;
	deadline.tv_nsec += (long)(SQL_POOL_ACQUIRE_TIMEOUT_MS % 1000) * 1000000L;
	if (deadline.tv_nsec >= 1000000000L)
	{
		deadline.tv_sec++;
		deadline.tv_nsec -= 1000000000L;
	}

	while (1)
	{
		int retry_scan = 0;
		/* Linear scan for a free slot -- pool is small (4-16), so O(n)
		 * is fine. */
		for (int i = 0; i < pool_size; i++)
		{
			if (!pool[i].in_use && pool[i].conn)
			{
				pool[i].in_use = 1;
				conn = pool[i].conn;
				pthread_mutex_unlock(&pool_mutex);
				const auto guard_status = duris_sql_exclusion_guard_check(conn);
				if (guard_status == duris_sql_exclusion_guard_status::allowed)
					return conn;
				if (guard_status == duris_sql_exclusion_guard_status::owner_lost)
				{
					sql_pool_release(conn);
					return NULL;
				}
				/* An inconclusive probe says nothing about the original lock owner.
				 * Retire this worker and rescan so another healthy slot can serve the
				 * request or this slot can be refilled with a fresh configured handle. */
				sql_pool_discard_connection(conn);
				sql_pool_release(conn);
				retry_scan = 1;
				break;
			}
		}
		if (retry_scan)
			continue;

		/* A discarded handle must never be reused, but permanently losing its
		 * slot would eventually strand every persistence worker. Prefer the
		 * healthy slots above; otherwise reserve one empty slot while opening
		 * a validated replacement outside the mutex. Shutdown counts this
		 * reservation as a borrower and cannot free the slot underneath us. */
		for (int i = 0; i < pool_size; i++)
		{
			if (!pool[i].in_use && !pool[i].conn)
			{
				pool[i].in_use = 1;
				pthread_mutex_unlock(&pool_mutex);
				try
				{
					conn = sql_pool_create_connection("sql_pool_acquire", i);
				}
				catch (...)
				{
					conn = NULL;
				}
				pthread_mutex_lock(&pool_mutex);
				if (pool_closing || !conn)
				{
					if (conn)
						mysql_close(conn);
					pool[i].in_use = 0;
					pthread_cond_broadcast(&pool_cond);
					pthread_mutex_unlock(&pool_mutex);
					return NULL;
				}
				pool[i].conn = conn;
				pthread_mutex_unlock(&pool_mutex);
				const auto guard_status = duris_sql_exclusion_guard_check(conn);
				if (guard_status == duris_sql_exclusion_guard_status::allowed)
					return conn;
				if (guard_status == duris_sql_exclusion_guard_status::inconclusive)
				{
					/* The configured factory already validated this handle. If this
					 * independent lease check is inconclusive, leave the slot empty for
					 * a later acquisition rather than returning an unproven session. */
					sql_pool_discard_connection(conn);
				}
				sql_pool_release(conn);
				return NULL;
			}
		}

		/* All busy -- wait only until the fixed acquisition deadline. */
		wait_result = pthread_cond_timedwait(&pool_cond, &pool_mutex, &deadline);
		if (wait_result == ETIMEDOUT)
		{
			int borrowed = 0;
			int total = pool_size;
			for (int i = 0; i < pool_size; i++)
				borrowed += pool[i].in_use;
			pthread_mutex_unlock(&pool_mutex);
			logit(LOG_STATUS,
			      "SQL pool acquisition timed out after %d ms (%d/%d connections borrowed).",
			      SQL_POOL_ACQUIRE_TIMEOUT_MS, borrowed, total);
			return NULL;
		}
		if (wait_result != 0)
		{
			pthread_mutex_unlock(&pool_mutex);
			logit(LOG_STATUS, "SQL pool acquisition wait failed error_code=%d",
			      wait_result);
			return NULL;
		}

		if (!pool || pool_closing)
		{
			pthread_mutex_unlock(&pool_mutex);
			return NULL;
		}
	}
}

MYSQL *sql_pool_acquire(void)
{
	return sql_pool_acquire_with_status(NULL);
}

void sql_pool_discard_connection(MYSQL *conn)
{
	if (!conn)
		return;

	pthread_mutex_lock(&pool_mutex);
	if (pool)
		for (int i = 0; i < pool_size; i++)
			if (pool[i].conn == conn && pool[i].in_use)
			{
				pool[i].discard_on_release = 1;
				break;
			}
	pthread_mutex_unlock(&pool_mutex);
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
			if (pool[i].in_use && pool[i].discard_on_release)
			{
				mysql_close(pool[i].conn);
				pool[i].conn = NULL;
				pool[i].discard_on_release = 0;
			}
			pool[i].in_use = 0;
			if (pool_closing)
				pthread_cond_broadcast(&pool_cond);
			else
				pthread_cond_signal(&pool_cond);
			break;
		}
	}

	pthread_mutex_unlock(&pool_mutex);
}

// Caller holds pool_mutex and owns this slot's lease.
static void sql_pool_retire_borrowed_slot(int slot)
{
	mysql_close(pool[slot].conn);
	pool[slot].conn = NULL;
	pool[slot].in_use = 0;
	pool[slot].discard_on_release = 0;
	if (pool_closing)
		pthread_cond_broadcast(&pool_cond);
	else
		pthread_cond_signal(&pool_cond);
}

MYSQL *sql_pool_replace_connection(MYSQL *conn)
{
	MYSQL *replacement = NULL;
	int slot = -1;

	if (!conn)
		return NULL;

	pthread_mutex_lock(&pool_mutex);
	if (pool)
		for (int i = 0; i < pool_size; i++)
			if (pool[i].conn == conn && pool[i].in_use)
			{
				slot = i;
				break;
			}
	if (slot < 0)
	{
		pthread_mutex_unlock(&pool_mutex);
		return NULL;
	}
	if (pool_closing || pool[slot].discard_on_release)
	{
		sql_pool_retire_borrowed_slot(slot);
		pthread_mutex_unlock(&pool_mutex);
		return NULL;
	}
	// Keep the original lease reserved while the factory is outside the mutex.
	// Shutdown waits for it; no other borrower can acquire/reuse its address.
	pthread_mutex_unlock(&pool_mutex);
	try
	{
		replacement = sql_pool_create_connection("sql_pool_replace_connection", slot);
	}
	catch (...)
	{
		logit(LOG_DEBUG, "sql_pool_replace_connection: connection factory failed");
	}

	pthread_mutex_lock(&pool_mutex);
	if (!pool || slot >= pool_size || pool[slot].conn != conn || !pool[slot].in_use)
	{
		// Invalid concurrent use of a lease must not retire somebody else's slot.
		pthread_mutex_unlock(&pool_mutex);
		if (replacement)
			mysql_close(replacement);
		return NULL;
	}
	if (!replacement || pool_closing || pool[slot].discard_on_release)
	{
		// Close the unpublished fresh session before releasing the reservation,
		// so shutdown cannot report quiescence while that session remains open.
		if (replacement)
			mysql_close(replacement);
		sql_pool_retire_borrowed_slot(slot);
		pthread_mutex_unlock(&pool_mutex);
		return NULL;
	}

	mysql_close(pool[slot].conn);
	pool[slot].conn = replacement;
	pthread_mutex_unlock(&pool_mutex);
	return replacement;
}

/* ---- Stats (debug / monitoring) ---- */

int sql_pool_is_active(void)
{
	int active;
	pthread_mutex_lock(&pool_mutex);
	active = pool != NULL;
	pthread_mutex_unlock(&pool_mutex);
	return active;
}

int sql_pool_available(void)
{
	int avail = 0;
	pthread_mutex_lock(&pool_mutex);
	if (pool)
	{
		for (int i = 0; i < pool_size; i++)
			if (!pool[i].in_use && pool[i].conn)
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

#else /* __NO_MYSQL__ */

/* Stubs -- no MySQL available.  The pool is a no-op. */

int sql_pool_init(int size)
{
	(void)size;
	return -1;
}

void sql_pool_shutdown(void) {}

MYSQL *sql_pool_acquire_with_status(int *pool_was_active)
{
	if (pool_was_active)
		*pool_was_active = 0;
	return NULL;
}

MYSQL *sql_pool_acquire(void)
{
	return sql_pool_acquire_with_status(NULL);
}

void sql_pool_release(MYSQL *conn)
{
	(void)conn;
}

void sql_pool_discard_connection(MYSQL *conn)
{
	(void)conn;
}

MYSQL *sql_pool_replace_connection(MYSQL *conn)
{
	(void)conn;
	return NULL;
}

int sql_pool_is_active(void)
{
	return 0;
}
int sql_pool_available(void)
{
	return 0;
}
int sql_pool_in_use(void)
{
	return 0;
}
int sql_pool_total(void)
{
	return 0;
}

#endif /* __NO_MYSQL__ */
