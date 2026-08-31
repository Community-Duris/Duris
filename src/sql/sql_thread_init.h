#ifndef SQL_THREAD_INIT_H
#define SQL_THREAD_INIT_H

/*
 * Per-thread client-library setup for the persistence workers.
 *
 * MySQL 8's client refuses mysql_thread_init() until mysql_library_init() has
 * run; MariaDB Connector/C initialises itself on demand and never needs it.
 * Every worker here treats a failed mysql_thread_init() as "give up and exit",
 * so against a MySQL 8 client the workers returned before draining anything and
 * the queue simply stopped -- silently, because a worker that never started has
 * no connection to report the failure on.
 *
 * Bring the library up exactly once, whichever client is linked.  Callers keep
 * their own handling of a genuine per-thread failure.
 */

#ifndef __NO_MYSQL__

#include <mutex>

#include <mysql/mysql.h>

/*
 * mysql_library_init() is not itself thread-safe, which is why the once_flag
 * guards it rather than a plain "have we done this" bool.
 */
inline int sql_worker_thread_init()
{
	static std::once_flag library_once;
	static bool library_ready = false;

	std::call_once(library_once,
		       [] { library_ready = mysql_library_init(0, nullptr, nullptr) == 0; });
	if (!library_ready)
		return 1;
	return mysql_thread_init();
}

#else

inline int sql_worker_thread_init()
{
	return 0;
}

#endif

#endif
