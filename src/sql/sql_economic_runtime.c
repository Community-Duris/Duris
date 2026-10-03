#include "sql/sql_economic_runtime.h"

#ifndef __NO_MYSQL__
#include "economy/economic_gameplay_authority.h"
#include "persistence/economic_sql_accounting_lifecycle_transaction.h"
#include "persistence/economic_sql_lifecycle_guard.h"
#include "sql/sql_exclusion_guard.h"
#include <memory>
#include <unistd.h>

// Use the same verified target, session contract, reconnect policy and
// close-on-exec setup as every other runtime SQL connection.
MYSQL *sql_open_configured_connection(unsigned long client_flags);

namespace
{
struct runtime_owner
{
	std::unique_ptr<MYSQL, decltype(&mysql_close)> connection{ nullptr, mysql_close };
	std::unique_ptr<economic_sql_lifecycle_guard> authority;
	pid_t process = 0;

	void release() noexcept
	{
		if (process && process != getpid())
		{
			// A forked child must never send protocol traffic or RELEASE_LOCK
			// through a parent's inherited MYSQL socket. Exec/exit closes the
			// child's descriptors without ending the parent's SQL session.
			(void)authority.release();
			(void)connection.release();
			process = 0;
			return;
		}
		authority.reset();
		connection.reset();
		process = 0;
	}

	~runtime_owner() { release(); }
};

runtime_owner &owner()
{
	static runtime_owner value;
	return value;
}
} // namespace

bool sql_economic_runtime_start() noexcept
{
	try
	{
		auto &runtime = owner();
		if (runtime.connection || runtime.authority)
			return false;
		std::unique_ptr<MYSQL, decltype(&mysql_close)> connection(
			sql_open_configured_connection(0), mysql_close);
		if (!connection)
			return false;
		auto authority = std::make_unique<economic_sql_lifecycle_guard>();
		if (economic_sql_lifecycle_guard::acquire_runtime(connection.get(),
								  authority.get()))
			return false;
		if (!duris_sql_exclusion_guard_bind_economic_runtime(connection.get()))
			return false;
		bool active = false;
		if (economic_sql_accounting_lifecycle_transaction::recover_runtime(
			    connection.get(), *authority, &active) ||
		    active != economic_gameplay_authority::active())
		{
			economic_gameplay_authority::clear_sql_runtime();
			return false;
		}
		runtime.connection = std::move(connection);
		runtime.authority = std::move(authority);
		runtime.process = getpid();
		return true;
	}
	catch (...)
	{
		economic_gameplay_authority::clear_sql_runtime();
		return false;
	}
}

void sql_economic_runtime_shutdown() noexcept
{
	economic_gameplay_authority::clear_sql_runtime();
	owner().release();
}
#else
bool sql_economic_runtime_start() noexcept
{
	return false;
}

void sql_economic_runtime_shutdown() noexcept {}
#endif
