#include "economy/collector_catalog_source.h"

#ifndef __NO_MYSQL__
#include "economy/collector_repository.h"
#include "sql/sql_pool.h"
#include "sql/sql_thread_init.h"

#include <cerrno>
#include <mysql.h>
#endif

bool collector_catalog_source_load(collector::catalog &catalog, std::string &error)
{
#ifdef __NO_MYSQL__
	(void)catalog;
	error = "flat-file collector authority is not available";
	return false;
#else
	if (sql_worker_thread_init() != 0)
	{
		error = "collector catalog worker initialization failed";
		return false;
	}
	struct thread_guard
	{
		~thread_guard() { mysql_thread_end(); }
	} thread;
	MYSQL *connection = sql_pool_acquire();
	if (!connection)
	{
		error = "collector catalog database connection unavailable";
		return false;
	}
	struct connection_guard
	{
		MYSQL *value;
		~connection_guard() { sql_pool_release(value); }
	} borrowed{ connection };
	if (!collector_repository_read_catalog(connection, &catalog))
	{
		const unsigned int database_error = mysql_errno(connection);
		const unsigned int failure = database_error ? database_error : errno;
		error = "collector catalog read failed (error " + std::to_string(failure) + ")";
		return false;
	}
	return true;
#endif
}
