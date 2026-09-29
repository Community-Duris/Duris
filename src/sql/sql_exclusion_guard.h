#ifndef DURIS_SQL_EXCLUSION_GUARD_H
#define DURIS_SQL_EXCLUSION_GUARD_H

/*
 * Database-native exclusion for player-death restitution.
 *
 * The lock is session-owned by MySQL/MariaDB.  It therefore survives COMMIT,
 * but is released by the server when the owning connection disappears.  The
 * runtime keeps it on its main connection and checks IS_USED_LOCK() before
 * using that connection or accepting a newly opened runtime connection.
 */

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <atomic>

#ifdef __NO_MYSQL__
#include "no_mysql/mysql.h"
#else
#include <mysql.h>
#include "persistence/economic_sql_lifecycle_lock_names.h"
#endif

#define DURIS_SQL_EXCLUSION_LOCK_EXPRESSION "CONCAT('duris.player.death.restitution.',DATABASE())"

struct duris_sql_exclusion_guard_state
{
	MYSQL *connection;
	unsigned long connection_id;
	pid_t process_id;
	std::atomic<bool> lost;
	unsigned long economic_connection_id = 0;
};

enum class duris_sql_exclusion_guard_status
{
	allowed,
	owner_lost,
	inconclusive
};

// External inline linkage gives sql.c and sql_pool.c one shared process state.
// Owner identity is published before worker startup and cleared after draining;
// concurrent probes may independently latch a loss, so that flag is atomic.
inline duris_sql_exclusion_guard_state &duris_sql_exclusion_guard_state_ref()
{
	static duris_sql_exclusion_guard_state state = { NULL, 0, 0, false };
	return state;
}

#ifndef __NO_MYSQL__

/* Execute one fixed scalar query and copy its non-NULL result. */
static inline bool duris_sql_exclusion_guard_scalar(MYSQL *connection, const char *query,
						    char *value, size_t value_size)
{
	if (!connection || !query || !value || value_size == 0 ||
	    mysql_real_query(connection, query, strlen(query)))
		return false;

	MYSQL_RES *result = mysql_store_result(connection);
	if (!result)
		return false;
	const bool shape_ok = mysql_num_fields(result) == 1 && mysql_num_rows(result) == 1;
	MYSQL_ROW row = shape_ok ? mysql_fetch_row(result) : NULL;
	unsigned long *lengths = row ? mysql_fetch_lengths(result) : NULL;
	bool value_ok = row && row[0] && lengths && lengths[0] < value_size;
	if (value_ok)
	{
		memcpy(value, row[0], lengths[0]);
		value[lengths[0]] = '\0';
	}
	mysql_free_result(result);
	if (mysql_next_result(connection) != -1)
		value_ok = false;
	return value_ok;
}

static inline duris_sql_exclusion_guard_status duris_sql_exclusion_guard_validate(MYSQL *probe)
{
	duris_sql_exclusion_guard_state &state = duris_sql_exclusion_guard_state_ref();
	if (state.lost)
		return duris_sql_exclusion_guard_status::owner_lost;
	if (!probe || !state.connection || state.connection_id == 0)
		return duris_sql_exclusion_guard_status::inconclusive;

	/* A forked child must not touch the inherited MYSQL handle.  It may use a
	 * newly opened handle, which still proves that the parent-owned lock exists. */
	if (state.process_id != getpid() && probe == state.connection)
		return duris_sql_exclusion_guard_status::inconclusive;

	char query[512];
	const int written =
		state.economic_connection_id ?
			snprintf(query, sizeof(query),
				 "SELECT IF(IFNULL(IS_USED_LOCK(%s),0)=%lu AND "
				 "IFNULL(IS_USED_LOCK('%s'),0)=%lu,1,0)",
				 DURIS_SQL_EXCLUSION_LOCK_EXPRESSION, state.connection_id,
				 ECONOMIC_SQL_BOOT_MAINTENANCE_LOCK_NAME,
				 state.economic_connection_id) :
			snprintf(query, sizeof(query),
				 "SELECT IF(IFNULL(IS_USED_LOCK(%s),0)=%lu,1,0)",
				 DURIS_SQL_EXCLUSION_LOCK_EXPRESSION, state.connection_id);
	if (written < 0 || (size_t)written >= sizeof(query))
		return duris_sql_exclusion_guard_status::inconclusive;
	char value[4];
	if (!duris_sql_exclusion_guard_scalar(probe, query, value, sizeof(value)))
		return duris_sql_exclusion_guard_status::inconclusive;
	if (!strcmp(value, "0"))
	{
		state.lost = true;
		return duris_sql_exclusion_guard_status::owner_lost;
	}
	if (strcmp(value, "1"))
		return duris_sql_exclusion_guard_status::inconclusive;
	if (state.lost)
		return duris_sql_exclusion_guard_status::owner_lost;
	return duris_sql_exclusion_guard_status::allowed;
}

// Boot-only: publish the dedicated lifecycle owner before starting workers.
// Probes use their own SQL session; no worker touches the control connection.
static inline bool duris_sql_exclusion_guard_bind_economic_runtime(MYSQL *control)
{
	auto &state = duris_sql_exclusion_guard_state_ref();
	if (!control || !state.connection || control == state.connection || state.lost ||
	    state.economic_connection_id || state.process_id != getpid())
		return false;
	const auto session = mysql_thread_id(control);
	if (!session)
		return false;
	state.economic_connection_id = session;
	const auto status = duris_sql_exclusion_guard_validate(control);
	if (status == duris_sql_exclusion_guard_status::inconclusive && !state.lost)
		state.economic_connection_id = 0;
	return status == duris_sql_exclusion_guard_status::allowed;
}

/* No guard is required before the runtime owner is initialized.  Once bound,
 * callers can distinguish an inconclusive probe from proven owner loss. */
static inline duris_sql_exclusion_guard_status duris_sql_exclusion_guard_check(MYSQL *probe)
{
	duris_sql_exclusion_guard_state &state = duris_sql_exclusion_guard_state_ref();
	if (state.lost)
		return duris_sql_exclusion_guard_status::owner_lost;
	if (!state.connection)
		return duris_sql_exclusion_guard_status::allowed;
	return duris_sql_exclusion_guard_validate(probe);
}

/* A connection may be used only while the runtime's original owner is still
 * visible to the SQL server.  This also gates factory/reconnect probes. */
static inline bool duris_sql_exclusion_guard_allows(MYSQL *probe)
{
	duris_sql_exclusion_guard_state &state = duris_sql_exclusion_guard_state_ref();
	if (!state.connection && !state.lost)
		return true;
	return duris_sql_exclusion_guard_check(probe) == duris_sql_exclusion_guard_status::allowed;
}

static inline bool duris_sql_exclusion_guard_acquire(MYSQL *connection)
{
	duris_sql_exclusion_guard_state &state = duris_sql_exclusion_guard_state_ref();
	if (!connection || state.connection || state.lost)
		return false;

	char value[8];
	char query[192];
	const int written = snprintf(query, sizeof(query), "SELECT GET_LOCK(%s,0)",
				     DURIS_SQL_EXCLUSION_LOCK_EXPRESSION);
	if (written < 0 || (size_t)written >= sizeof(query) ||
	    !duris_sql_exclusion_guard_scalar(connection, query, value, sizeof(value)) ||
	    strcmp(value, "1"))
		return false;

	const unsigned long connection_id = mysql_thread_id(connection);
	if (connection_id == 0)
	{
		(void)duris_sql_exclusion_guard_scalar(
			connection, "SELECT RELEASE_LOCK(" DURIS_SQL_EXCLUSION_LOCK_EXPRESSION ")",
			value, sizeof(value));
		return false;
	}
	state.connection = connection;
	state.connection_id = connection_id;
	state.process_id = getpid();
	state.economic_connection_id = 0;
	state.lost = false;
	const auto status = duris_sql_exclusion_guard_validate(connection);
	if (status == duris_sql_exclusion_guard_status::allowed)
		return true;

	(void)duris_sql_exclusion_guard_scalar(
		connection, "SELECT RELEASE_LOCK(" DURIS_SQL_EXCLUSION_LOCK_EXPRESSION ")", value,
		sizeof(value));
	state.connection = NULL;
	state.connection_id = 0;
	state.process_id = 0;
	if (status == duris_sql_exclusion_guard_status::owner_lost)
		state.lost = true;
	return false;
}

static inline void duris_sql_exclusion_guard_release()
{
	duris_sql_exclusion_guard_state &state = duris_sql_exclusion_guard_state_ref();
	if (state.connection && !state.lost && state.process_id == getpid())
	{
		char value[8];
		(void)duris_sql_exclusion_guard_scalar(
			state.connection,
			"SELECT RELEASE_LOCK(" DURIS_SQL_EXCLUSION_LOCK_EXPRESSION ")", value,
			sizeof(value));
	}
	state.connection = NULL;
	state.connection_id = 0;
	state.process_id = 0;
	state.economic_connection_id = 0;
	state.lost = true;
}

#else

static inline duris_sql_exclusion_guard_status duris_sql_exclusion_guard_validate(MYSQL *)
{
	return duris_sql_exclusion_guard_status::inconclusive;
}

static inline duris_sql_exclusion_guard_status duris_sql_exclusion_guard_check(MYSQL *)
{
	return duris_sql_exclusion_guard_status::inconclusive;
}

static inline bool duris_sql_exclusion_guard_allows(MYSQL *)
{
	return false;
}

static inline bool duris_sql_exclusion_guard_acquire(MYSQL *)
{
	return false;
}

static inline bool duris_sql_exclusion_guard_bind_economic_runtime(MYSQL *)
{
	return false;
}

static inline void duris_sql_exclusion_guard_release() {}

#endif /* __NO_MYSQL__ */

#endif /* DURIS_SQL_EXCLUSION_GUARD_H */
