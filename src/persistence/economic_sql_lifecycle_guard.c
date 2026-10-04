#include "persistence/economic_sql_lifecycle_guard.h"
#include <cerrno>
#include <charconv>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>
#include <memory>
#include <string>
#include <type_traits>

namespace
{
#ifndef __NO_MYSQL__
std::shared_mutex &currency_gate()
{
	static std::shared_mutex value;
	return value;
}
#endif
std::mutex &authority_mutex()
{
	static std::mutex value;
	return value;
}
bool &runtime_authority_active()
{
	static bool value = false;
	return value;
}
bool &maintenance_authority_active()
{
	static bool value = false;
	return value;
}
#ifndef __NO_MYSQL__
uint64_t &next_authority_id()
{
	static uint64_t value = 1;
	return value;
}
uint64_t allocate_authority_id()
{
	uint64_t &next = next_authority_id();
	if (!next)
		return 0;
	const uint64_t result = next;
	if (next == std::numeric_limits<uint64_t>::max())
		next = 0;
	else
		++next;
	return result;
}
#endif
void clear_local_authority(bool runtime, bool maintenance)
{
	std::lock_guard lock(authority_mutex());
	if (runtime)
		runtime_authority_active() = false;
	if (maintenance)
		maintenance_authority_active() = false;
}
#ifndef __NO_MYSQL__
constexpr const char *boot_lock = ECONOMIC_SQL_BOOT_MAINTENANCE_LOCK_NAME;
constexpr const char *writer_lock = "duris:economic_sql_currency_writers";
using result_ptr = std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)>;
unsigned int mysql_error_code(MYSQL *connection)
{
	const auto code = mysql_errno(connection);
	return code ? code : EIO;
}
// Idle-only precondition for acquiring lifecycle locks/capabilities. Do not
// use this for a retained transaction: SERVER_STATUS_IN_TRANS is expected there.
bool idle(MYSQL *connection)
{
	if (!connection || (connection->server_status & SERVER_STATUS_IN_TRANS) ||
	    !(connection->server_status & SERVER_STATUS_AUTOCOMMIT))
		return false;
	using flag = std::remove_pointer_t<decltype(MYSQL_BIND{}.is_null)>;
	flag reconnect = false;
	return !mysql_get_option(connection, MYSQL_OPT_RECONNECT, &reconnect) && !reconnect;
}
bool owns_named_lock(MYSQL *connection, const char *name, unsigned long session)
{
	char query[160];
	const int length = std::snprintf(query, sizeof(query), "SELECT IS_USED_LOCK('%s')", name);
	if (length < 0 || static_cast<size_t>(length) >= sizeof(query) ||
	    mysql_real_query(connection, query, static_cast<unsigned long>(length)))
		return false;
	result_ptr result(mysql_store_result(connection), mysql_free_result);
	if (!result || mysql_num_rows(result.get()) != 1 || mysql_num_fields(result.get()) != 1)
		return false;
	const auto row = mysql_fetch_row(result.get());
	if (!row || !row[0])
		return false;
	uint64_t lock_session = 0;
	const auto row_length = std::char_traits<char>::length(row[0]);
	const auto parsed = std::from_chars(row[0], row[0] + row_length, lock_session);
	return parsed.ec == std::errc{} && parsed.ptr == row[0] + row_length &&
	       lock_session == static_cast<uint64_t>(session);
}
unsigned int lock(MYSQL *connection, const char *name, unsigned int timeout, bool *acquired)
{
	char query[160];
	const int length =
		std::snprintf(query, sizeof(query), "SELECT GET_LOCK('%s',%u)", name, timeout);
	if (length < 0 || static_cast<size_t>(length) >= sizeof(query))
		return EOVERFLOW;
	if (mysql_real_query(connection, query, static_cast<unsigned long>(length)))
		return mysql_error_code(connection);
	result_ptr result(mysql_store_result(connection), mysql_free_result);
	if (!result || mysql_num_rows(result.get()) != 1 || mysql_num_fields(result.get()) != 1)
		return mysql_error_code(connection);
	const auto row = mysql_fetch_row(result.get());
	if (!row || !row[0])
		return EBUSY;
	*acquired = std::strcmp(row[0], "1") == 0;
	return *acquired ? 0 : EBUSY;
}
void unlock(MYSQL *connection, unsigned long session, const char *name)
{
	if (!connection || mysql_thread_id(connection) != session)
		return;
	char query[160];
	const int length = std::snprintf(query, sizeof(query), "SELECT RELEASE_LOCK('%s')", name);
	if (length < 0 || static_cast<size_t>(length) >= sizeof(query))
		return;
	(void)mysql_real_query(connection, query, static_cast<unsigned long>(length));
	MYSQL_RES *result = mysql_store_result(connection);
	if (result)
		mysql_free_result(result);
}
unsigned int staged_installation(MYSQL *connection, bool reject_active_epoch = false)
{
	const char *query =
		reject_active_epoch ?
			"SELECT EXISTS(SELECT 1 FROM economic_sql_lifecycle_installation WHERE phase IN (1,2)) "
			"OR EXISTS(SELECT 1 FROM economic_lineage_state WHERE active_epoch IS NOT NULL)" :
			"SELECT COUNT(*) FROM economic_sql_lifecycle_installation WHERE phase IN (1,2)";
	if (mysql_real_query(connection, query, std::char_traits<char>::length(query)))
		return mysql_error_code(connection);
	result_ptr result(mysql_store_result(connection), mysql_free_result);
	if (!result || mysql_num_rows(result.get()) != 1 || mysql_num_fields(result.get()) != 1)
		return mysql_error_code(connection);
	const auto row = mysql_fetch_row(result.get());
	if (!row || !row[0])
		return EIO;
	uint64_t count = 0;
	const auto length = std::char_traits<char>::length(row[0]);
	const auto parsed = std::from_chars(row[0], row[0] + length, count);
	if (parsed.ec != std::errc{} || parsed.ptr != row[0] + length)
		return EILSEQ;
	return count ? EPERM : 0;
}
unsigned int runtime_installation_state(MYSQL *connection)
{
	// A staged or paused lineage cannot resume legacy gameplay. An active
	// lineage is admitted only with the same durable installation and epoch;
	// the runtime owner reads back the baseline and builds its cache next.
	const char *query =
		"SELECT EXISTS(SELECT 1 FROM economic_sql_lifecycle_installation i "
		"LEFT JOIN economic_sql_global_activation a ON a.lineage=i.lineage "
		"LEFT JOIN economic_lineage_state l ON l.lineage=i.lineage "
		"WHERE i.phase<>2 OR a.state IS NULL OR a.state<>1 OR "
		"a.epoch<>i.epoch OR a.installation_operation_id<>i.operation_id OR "
		"a.baseline_operation_id<>i.baseline_operation_id OR "
		"l.active_epoch IS NULL OR l.active_epoch<>i.epoch) "
		"OR EXISTS(SELECT 1 FROM economic_lineage_state l WHERE l.active_epoch IS NOT NULL "
		"AND NOT EXISTS(SELECT 1 FROM economic_sql_global_activation a "
		"JOIN economic_sql_lifecycle_installation i ON i.operation_id=a.installation_operation_id "
		"WHERE a.lineage=l.lineage AND a.epoch=l.active_epoch AND a.state=1 "
		"AND i.lineage=l.lineage AND i.epoch=l.active_epoch)) "
		"OR EXISTS(SELECT 1 FROM economic_sql_global_activation a "
		"LEFT JOIN economic_sql_lifecycle_installation i "
		"ON i.operation_id=a.installation_operation_id "
		"LEFT JOIN economic_lineage_state l ON l.lineage=a.lineage "
		"WHERE a.state<>1 OR i.operation_id IS NULL OR i.phase<>2 OR "
		"i.lineage<>a.lineage OR i.epoch<>a.epoch OR "
		"i.baseline_operation_id<>a.baseline_operation_id OR "
		"l.active_epoch IS NULL OR l.active_epoch<>a.epoch)";
	if (mysql_real_query(connection, query, std::char_traits<char>::length(query)))
		return mysql_error_code(connection);
	result_ptr result(mysql_store_result(connection), mysql_free_result);
	if (!result || mysql_num_rows(result.get()) != 1 || mysql_num_fields(result.get()) != 1)
		return mysql_error_code(connection);
	const auto row = mysql_fetch_row(result.get());
	if (!row || !row[0])
		return EIO;
	return std::strcmp(row[0], "0") == 0 ? 0 : EPERM;
}
#endif
} // namespace

void economic_sql_lifecycle_guard::clear_transferred_local_authority(bool runtime,
								     bool maintenance) noexcept
{
	clear_local_authority(runtime, maintenance);
}

economic_sql_lifecycle_guard::~economic_sql_lifecycle_guard()
{
#ifndef __NO_MYSQL__
	if (writer_lock_)
		unlock(connection_, session_, writer_lock);
	if (runtime_lock_)
		unlock(connection_, session_, boot_lock);
#endif
	if (local_exclusive_.owns_lock())
		local_exclusive_.unlock();
	if (local_runtime_ || local_maintenance_)
		clear_local_authority(local_runtime_, local_maintenance_);
#ifndef __NO_MYSQL__
	// Releasing the coordinator lease is last: a no-transaction guard may
	// reopen admission only after its SQL and in-process fences are gone.
	if (coordinator_release_)
	{
		try
		{
			coordinator_release_(coordinator_generation_, coordinator_lease_id_);
		}
		catch (...)
		{
			// A failed release is fail-closed; never substitute another lease.
		}
	}
#endif
	coordinator_release_ = nullptr;
	coordinator_generation_ = 0;
	coordinator_lease_id_ = 0;
	authority_id_ = 0;
	session_ = 0;
	connection_ = nullptr;
}

unsigned int
economic_sql_lifecycle_guard::acquire_runtime(MYSQL *connection,
					      economic_sql_lifecycle_guard *output) noexcept
{
#ifdef __NO_MYSQL__
	(void)connection;
	(void)output;
	return ENOTSUP;
#else
	if (!output || output->connection_ || !idle(connection))
		return EINVAL;
	uint64_t authority_id = 0;
	{
		std::lock_guard lock(authority_mutex());
		if (runtime_authority_active() || maintenance_authority_active())
			return EBUSY;
		authority_id = allocate_authority_id();
		if (!authority_id)
			return EOVERFLOW;
		runtime_authority_active() = true;
	}
	bool acquired = false;
	const auto status = lock(connection, boot_lock, 0, &acquired);
	if (status)
	{
		clear_local_authority(true, false);
		return status;
	}
	const auto staged = runtime_installation_state(connection);
	if (staged)
	{
		unlock(connection, mysql_thread_id(connection), boot_lock);
		clear_local_authority(true, false);
		return staged;
	}
	output->connection_ = connection;
	output->session_ = mysql_thread_id(connection);
	output->runtime_lock_ = acquired;
	output->local_runtime_ = true;
	output->maintenance_ = false;
	output->authority_id_ = authority_id;
	return 0;
#endif
}

unsigned int
economic_sql_lifecycle_guard::acquire_maintenance(MYSQL *connection,
						  economic_sql_lifecycle_guard *output) noexcept
{
#ifdef __NO_MYSQL__
	(void)connection;
	(void)output;
	return ENOTSUP;
#else
	if (!output || output->connection_ || !idle(connection))
		return EINVAL;
	std::unique_lock<std::shared_mutex> local(currency_gate(), std::try_to_lock);
	if (!local.owns_lock())
		return EBUSY;
	uint64_t authority_id = 0;
	{
		std::lock_guard lock(authority_mutex());
		if (runtime_authority_active() || maintenance_authority_active())
			return EBUSY;
		authority_id = allocate_authority_id();
		if (!authority_id)
			return EOVERFLOW;
		maintenance_authority_active() = true;
	}
	bool boot = false, writers = false;
	unsigned int status = lock(connection, boot_lock, 0, &boot);
	if (status)
	{
		clear_local_authority(false, true);
		return status;
	}
	status = lock(connection, writer_lock, 10, &writers);
	if (status)
	{
		unlock(connection, mysql_thread_id(connection), boot_lock);
		clear_local_authority(false, true);
		return status;
	}
	output->connection_ = connection;
	output->session_ = mysql_thread_id(connection);
	output->runtime_lock_ = boot;
	output->writer_lock_ = writers;
	output->maintenance_ = true;
	output->local_maintenance_ = true;
	output->authority_id_ = authority_id;
	output->local_exclusive_ = std::move(local);
	return 0;
#endif
}

bool economic_sql_lifecycle_guard::is_maintenance_authority() const noexcept
{
	return connection_ && runtime_lock_ && writer_lock_ && maintenance_ &&
	       local_exclusive_.owns_lock() && mysql_thread_id(connection_) == session_;
}

bool economic_sql_lifecycle_guard::is_valid_authority() const noexcept
{
#ifdef __NO_MYSQL__
	return false;
#else
	try
	{
		if (!connection_ || !authority_id_ || !session_ || !runtime_lock_)
			return false;
		{
			std::lock_guard lock(authority_mutex());
			if ((maintenance_ && !maintenance_authority_active()) ||
			    (!maintenance_ && !runtime_authority_active()))
				return false;
		}
		if (maintenance_)
		{
			if (!writer_lock_ || !local_maintenance_ || !local_exclusive_.owns_lock())
				return false;
		}
		else if (writer_lock_ || local_maintenance_ || !local_runtime_ ||
			 local_exclusive_.owns_lock())
			return false;

		if (mysql_thread_id(connection_) != session_ || !idle(connection_) ||
		    mysql_ping(connection_) || mysql_thread_id(connection_) != session_ ||
		    !owns_named_lock(connection_, boot_lock, session_))
			return false;
		return !maintenance_ || owns_named_lock(connection_, writer_lock, session_);
	}
	catch (...)
	{
		return false;
	}
#endif
}

economic_sql_currency_writer_guard::~economic_sql_currency_writer_guard()
{
#ifndef __NO_MYSQL__
	if (writer_lock_)
		unlock(connection_, session_, writer_lock);
#endif
}

bool economic_sql_currency_writer_guard::is_valid_for(MYSQL *connection) const noexcept
{
#ifdef __NO_MYSQL__
	(void)connection;
	return false;
#else
	try
	{
		if (!connection || connection != connection_ || !session_ || !writer_lock_ ||
		    !local_shared_.owns_lock() || mysql_thread_id(connection) != session_)
			return false;
		using flag = std::remove_pointer_t<decltype(MYSQL_BIND{}.is_null)>;
		flag reconnect = false;
		if (mysql_get_option(connection, MYSQL_OPT_RECONNECT, &reconnect) || reconnect ||
		    mysql_ping(connection) || mysql_thread_id(connection) != session_)
			return false;
		return owns_named_lock(connection, writer_lock, session_);
	}
	catch (...)
	{
		return false;
	}
#endif
}

unsigned int
economic_sql_currency_writer_guard::acquire(MYSQL *connection,
					    economic_sql_currency_writer_guard *output) noexcept
{
#ifdef __NO_MYSQL__
	(void)connection;
	(void)output;
	return ENOTSUP;
#else
	if (!output || output->connection_ || !idle(connection))
		return EINVAL;
	{
		std::lock_guard lock(authority_mutex());
		if (maintenance_authority_active())
			return EPERM;
	}
	std::shared_lock<std::shared_mutex> local(currency_gate());
	bool acquired = false;
	const auto status = lock(connection, writer_lock, 10, &acquired);
	if (status)
		return status;
	const auto staged = staged_installation(connection, true);
	if (staged)
	{
		unlock(connection, mysql_thread_id(connection), writer_lock);
		return staged;
	}
	output->connection_ = connection;
	output->session_ = mysql_thread_id(connection);
	output->writer_lock_ = acquired;
	output->local_shared_ = std::move(local);
	return 0;
#endif
}
