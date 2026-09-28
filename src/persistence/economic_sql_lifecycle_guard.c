#include "persistence/economic_sql_lifecycle_guard.h"
#include <cerrno>
#include <charconv>
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
bool idle(MYSQL *connection)
{
	if (!connection || (connection->server_status & SERVER_STATUS_IN_TRANS) ||
	    !(connection->server_status & SERVER_STATUS_AUTOCOMMIT))
		return false;
	using flag = std::remove_pointer_t<decltype(MYSQL_BIND{}.is_null)>;
	flag reconnect = false;
	return !mysql_get_option(connection, MYSQL_OPT_RECONNECT, &reconnect) && !reconnect;
}
unsigned int lock(MYSQL *connection, const char *name, unsigned int timeout, bool *acquired)
{
	const std::string query =
		"SELECT GET_LOCK('" + std::string(name) + "'," + std::to_string(timeout) + ")";
	if (mysql_real_query(connection, query.data(), query.size()))
		return mysql_error_code(connection);
	result_ptr result(mysql_store_result(connection), mysql_free_result);
	if (!result || mysql_num_rows(result.get()) != 1 || mysql_num_fields(result.get()) != 1)
		return mysql_error_code(connection);
	const auto row = mysql_fetch_row(result.get());
	if (!row || !row[0])
		return EBUSY;
	*acquired = std::string(row[0]) == "1";
	return *acquired ? 0 : EBUSY;
}
void unlock(MYSQL *connection, unsigned long session, const char *name)
{
	if (!connection || mysql_thread_id(connection) != session)
		return;
	const std::string query = "SELECT RELEASE_LOCK('" + std::string(name) + "')";
	(void)mysql_real_query(connection, query.data(), query.size());
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
#endif
} // namespace

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
	{
		std::lock_guard lock(authority_mutex());
		if (runtime_authority_active() || maintenance_authority_active())
			return EBUSY;
		runtime_authority_active() = true;
	}
	bool acquired = false;
	const auto status = lock(connection, boot_lock, 0, &acquired);
	if (status)
	{
		clear_local_authority(true, false);
		return status;
	}
	const auto staged = staged_installation(connection, true);
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
	{
		std::lock_guard lock(authority_mutex());
		if (runtime_authority_active() || maintenance_authority_active())
			return EBUSY;
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
	output->local_exclusive_ = std::move(local);
	return 0;
#endif
}

bool economic_sql_lifecycle_guard::is_maintenance_authority() const noexcept
{
	return connection_ && runtime_lock_ && writer_lock_ && maintenance_ &&
	       local_exclusive_.owns_lock() && mysql_thread_id(connection_) == session_;
}

economic_sql_currency_writer_guard::~economic_sql_currency_writer_guard()
{
#ifndef __NO_MYSQL__
	if (writer_lock_)
		unlock(connection_, session_, writer_lock);
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
