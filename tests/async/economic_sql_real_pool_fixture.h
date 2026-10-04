#ifndef DURIS_TEST_ECONOMIC_SQL_REAL_POOL_FIXTURE_H
#define DURIS_TEST_ECONOMIC_SQL_REAL_POOL_FIXTURE_H

#include "economic_sql_commit_reply_loss_fixture.h"
#include "sql/sql_pool.h"
#include <cassert>
#include <cstdio>

// The production pool owns every lease. Only its disposable connection factory
// is supplied by the enclosing native harness; production boot is separate.
MYSQL *sql_open_configured_connection(unsigned long flags)
{
	return open_pool_test_connection(flags);
}
void logit(const char *, const char *, ...) {}

extern "C" MYSQL *__real_sql_pool_acquire();
extern "C" void __real_sql_pool_release(MYSQL *);
extern "C" MYSQL *__real_sql_pool_replace_connection(MYSQL *);
extern "C" void __real_sql_pool_discard_connection(MYSQL *);
extern "C" MYSQL *__wrap_sql_pool_acquire()
{
	auto *connection = __real_sql_pool_acquire();
	economic_sql_commit_reply_loss_fixture::acquired(connection);
	return connection;
}
extern "C" void __wrap_sql_pool_release(MYSQL *connection)
{
	economic_sql_commit_reply_loss_fixture::closing(connection, false);
	__real_sql_pool_release(connection);
}
extern "C" MYSQL *__wrap_sql_pool_replace_connection(MYSQL *connection)
{
	const bool lost = economic_sql_commit_reply_loss_fixture::lost_reply(connection);
	const unsigned long old_session = connection ? mysql_thread_id(connection) : 0;
	economic_sql_commit_reply_loss_fixture::closing(connection, true);
	auto *replacement = __real_sql_pool_replace_connection(connection);
	if (lost)
		assert(replacement && mysql_thread_id(replacement) != old_session);
	return replacement;
}
extern "C" void __wrap_sql_pool_discard_connection(MYSQL *connection)
{
	economic_sql_commit_reply_loss_fixture::closing(connection, false);
	__real_sql_pool_discard_connection(connection);
}

struct economic_sql_real_pool_lifecycle
{
	economic_sql_real_pool_lifecycle() { assert(sql_pool_init(1) == 0); }
	~economic_sql_real_pool_lifecycle()
	{
		assert(sql_pool_in_use() == 0 && sql_pool_total() == 1);
		auto *clean = sql_pool_acquire();
		if (!clean || (clean->server_status & SERVER_STATUS_IN_TRANS) ||
		    !(clean->server_status & SERVER_STATUS_AUTOCOMMIT))
			std::fprintf(
				stderr,
				"pool cleanup assertion: handle=%u status=%u session=%lu error=%u active=%d available=%d in_use=%d total=%d\n",
				clean ? 1U : 0U, clean ? clean->server_status : 0U,
				clean ? mysql_thread_id(clean) : 0UL,
				clean ? mysql_errno(clean) : 0U, sql_pool_is_active(),
				sql_pool_available(), sql_pool_in_use(), sql_pool_total());
		assert(clean && !(clean->server_status & SERVER_STATUS_IN_TRANS) &&
		       (clean->server_status & SERVER_STATUS_AUTOCOMMIT));
		sql_pool_release(clean);
		assert(sql_pool_available() == 1);
		sql_pool_shutdown();
		assert(sql_pool_total() == 0);
	}
};
#endif
