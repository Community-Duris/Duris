#include "economy/collector_catalog_source.h"

#include <cerrno>
#include <utility>

#ifndef __NO_MYSQL__
#include "economy/collector_repository.h"
#include "sql/sql_pool.h"
#include "sql/sql_thread_init.h"

#include <mysql.h>
#endif

bool collector_catalog_source_load(collector_bootstrap_snapshot &snapshot, std::string &error)
{
#ifdef __NO_MYSQL__
	(void)snapshot;
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
	if (!collector_repository_read_bootstrap(connection, &snapshot))
	{
		const unsigned int database_error = mysql_errno(connection);
		const unsigned int failure = database_error ? database_error : errno;
		error = "collector catalog read failed (error " + std::to_string(failure) + ")";
		return false;
	}
	return true;
#endif
}

bool collector_listing_source_load(uint64_t listing, collector_listing_detail &detail, bool &found,
				   unsigned int &error_code, std::string &error)
{
	if (!listing)
	{
		error_code = EINVAL;
		error = "invalid collector listing";
		return false;
	}
#ifdef __NO_MYSQL__
	(void)detail;
	(void)found;
	error_code = ENOTSUP;
	error = "flat-file collector authority is not available";
	return false;
#else
	if (sql_worker_thread_init() != 0)
	{
		error_code = EIO;
		error = "collector listing worker initialization failed";
		return false;
	}
	struct thread_guard
	{
		~thread_guard() { mysql_thread_end(); }
	} thread;
	MYSQL *connection = sql_pool_acquire();
	if (!connection)
	{
		error_code = EAGAIN;
		error = "collector listing database connection unavailable";
		return false;
	}
	struct connection_guard
	{
		MYSQL *value;
		~connection_guard() { sql_pool_release(value); }
	} borrowed{ connection };
	collector_listing_detail candidate;
	bool candidate_found = false;
	if (!collector_repository_read_listing(connection, listing, &candidate, &candidate_found))
	{
		const unsigned int database_error = mysql_errno(connection);
		error_code = database_error ? database_error : static_cast<unsigned int>(errno);
		if (!error_code)
			error_code = EIO;
		error = "collector listing read failed (error " + std::to_string(error_code) + ")";
		return false;
	}
	if (candidate_found)
		detail = std::move(candidate);
	found = candidate_found;
	error_code = 0;
	error.clear();
	return true;
#endif
}
