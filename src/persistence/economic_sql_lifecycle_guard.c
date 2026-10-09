#include "persistence/economic_sql_lifecycle_guard.h"
#include "sql/sql_pool.h"
#include "sql/sql_exclusion_guard.h"
#include "persistence/critical_command_coordinator.h"
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
bool query_lock_owner(MYSQL *connection, unsigned long session, const char *name,
		      unsigned long *owner, bool *has_owner)
{
	if (!connection || !session || mysql_thread_id(connection) != session)
		return false;
	char query[160];
	const int length = std::snprintf(query, sizeof(query), "SELECT IS_USED_LOCK('%s')", name);
	if (length < 0 || static_cast<size_t>(length) >= sizeof(query) ||
	    mysql_real_query(connection, query, static_cast<unsigned long>(length)))
		return false;
	result_ptr result(mysql_store_result(connection), mysql_free_result);
	if (!result || mysql_num_rows(result.get()) != 1 || mysql_num_fields(result.get()) != 1)
		return false;
	const auto row = mysql_fetch_row(result.get());
	if (!row || mysql_thread_id(connection) != session)
		return false;
	if (!row[0])
	{
		*owner = 0;
		*has_owner = false;
		return true;
	}
	uint64_t lock_session = 0;
	const auto row_length = std::char_traits<char>::length(row[0]);
	const auto parsed = std::from_chars(row[0], row[0] + row_length, lock_session);
	if (parsed.ec != std::errc{} || parsed.ptr != row[0] + row_length || !lock_session ||
	    lock_session > std::numeric_limits<unsigned long>::max())
		return false;
	*owner = static_cast<unsigned long>(lock_session);
	*has_owner = true;
	return true;
}
bool owns_named_lock(MYSQL *connection, const char *name, unsigned long session)
{
	unsigned long owner = 0;
	bool has_owner = false;
	return query_lock_owner(connection, session, name, &owner, &has_owner) && has_owner &&
	       owner == session;
}
unsigned int lock(MYSQL *connection, unsigned long session, const char *name, unsigned int timeout,
		  bool *obligation)
{
	unsigned long owner = 0;
	bool has_owner = false;
	if (!idle(connection) || !query_lock_owner(connection, session, name, &owner, &has_owner))
		return EIO;
	// Do not borrow or recursively increment another owner's same-session lease.
	if (has_owner && owner == session)
		return EPERM;
	char query[160];
	const int length =
		std::snprintf(query, sizeof(query), "SELECT GET_LOCK('%s',%u)", name, timeout);
	if (length < 0 || static_cast<size_t>(length) >= sizeof(query))
		return EOVERFLOW;
	// Bind the cleanup obligation before issuing SQL: a lost reply is ambiguous.
	*obligation = true;
	if (mysql_real_query(connection, query, static_cast<unsigned long>(length)))
		return mysql_error_code(connection);
	result_ptr result(mysql_store_result(connection), mysql_free_result);
	if (!result || mysql_num_rows(result.get()) != 1 || mysql_num_fields(result.get()) != 1)
		return EIO;
	const auto row = mysql_fetch_row(result.get());
	if (!row || !row[0] || mysql_thread_id(connection) != session)
		return EIO;
	if (std::strcmp(row[0], "0") == 0)
		return EBUSY;
	return std::strcmp(row[0], "1") == 0 && owns_named_lock(connection, name, session) ? 0 :
											     EIO;
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

bool economic_sql_lifecycle_guard::release_named_lock(MYSQL *connection, unsigned long session,
						      const char *name, bool *attempted) noexcept
{
#ifdef __NO_MYSQL__
	(void)connection;
	(void)session;
	(void)name;
	(void)attempted;
	return false;
#else
	try
	{
		if (!attempted || !idle(connection) || mysql_thread_id(connection) != session)
			return false;
		unsigned long owner = 0;
		bool has_owner = false;
		if (!query_lock_owner(connection, session, name, &owner, &has_owner))
			return false;
		if (!has_owner || owner != session)
			return true;
		// Issue at most one release per checked cleanup call. After a successful
		// release reply, readback alone may finish cleanup; never drain a recursive
		// same-session lease by sending another successful RELEASE.
		if (*attempted)
			return false;
		char query[160];
		const int length =
			std::snprintf(query, sizeof(query), "SELECT RELEASE_LOCK('%s')", name);
		if (length < 0 || static_cast<size_t>(length) >= sizeof(query))
			return false;
		*attempted = true;
		const int rc =
			mysql_real_query(connection, query, static_cast<unsigned long>(length));
		if (!rc)
		{
			result_ptr result(mysql_store_result(connection), mysql_free_result);
			if (!result || mysql_num_rows(result.get()) != 1 ||
			    mysql_num_fields(result.get()) != 1 || !mysql_fetch_row(result.get()))
				return false;
		}
		// The actual original-session readback, not the RPC reply, proves cleanup.
		if (!idle(connection) ||
		    !query_lock_owner(connection, session, name, &owner, &has_owner))
			return false;
		if (!has_owner || owner != session)
			return true;
		// A failed RPC with fresh proof that the original fence remains held may
		// be retried by that owner on a later call. The same exclusively borrowed
		// session must not acquire another named-lock count outside this guard.
		// A successful RPC that still leaves ownership is never repeated.
		if (rc)
			*attempted = false;
		return false;
	}
	catch (...)
	{
		return false;
	}
#endif
}

bool economic_sql_lifecycle_guard::release() noexcept
{
	if (runtime_handoff_)
		return false; // Only the actual boot return owner may consume this exclusion.
	if (owner_thread_ != std::thread::id{} && owner_thread_ != std::this_thread::get_id())
		return false;
	acquisition_confirmed_ = false;
	try
	{
#ifndef __NO_MYSQL__
		if ((writer_lock_ || runtime_lock_ || coordinator_release_) &&
		    (!idle(connection_) || mysql_thread_id(connection_) != session_))
			return false;
		if (writer_lock_)
		{
			if (!release_named_lock(connection_, session_, writer_lock,
						&writer_release_attempted_))
				return false;
			writer_lock_ = false;
		}
		if (runtime_lock_)
		{
			if (!release_named_lock(connection_, session_, boot_lock,
						&runtime_release_attempted_))
				return false;
			runtime_lock_ = false;
		}
#endif
		if (local_exclusive_.owns_lock())
			local_exclusive_.unlock();
		// Retain local authority flags until the exact coordinator lease is released.
		if (coordinator_release_ &&
		    !coordinator_release_(coordinator_generation_, coordinator_lease_id_))
			return false;
		if (local_runtime_ || local_maintenance_)
			clear_local_authority(local_runtime_, local_maintenance_);
		coordinator_release_ = nullptr;
		coordinator_generation_ = 0;
		coordinator_lease_id_ = 0;
		authority_id_ = 0;
		session_ = 0;
		owner_thread_ = {};
		connection_ = nullptr;
		maintenance_ = local_runtime_ = local_maintenance_ = false;
		runtime_release_attempted_ = writer_release_attempted_ = false;
		return true;
	}
	catch (...)
	{
		return false;
	}
}

economic_sql_lifecycle_guard::~economic_sql_lifecycle_guard()
{
	if (!release() && local_exclusive_.owns_lock())
		(void)local_exclusive_.release();
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
	try
	{
		if (!output || output->connection_ || !idle(connection) ||
		    !mysql_thread_id(connection))
			return EINVAL;
		{
			std::lock_guard lock(authority_mutex());
			if (runtime_authority_active() || maintenance_authority_active())
				return EBUSY;
			output->authority_id_ = allocate_authority_id();
			if (!output->authority_id_)
				return EOVERFLOW;
			runtime_authority_active() = true;
		}
		output->local_runtime_ = true;
		output->owner_thread_ = std::this_thread::get_id();
		output->connection_ = connection;
		output->session_ = mysql_thread_id(connection);
		auto status =
			lock(connection, output->session_, boot_lock, 0, &output->runtime_lock_);
		if (!status)
			status = runtime_installation_state(connection);
		if (!status &&
		    (!idle(connection) || mysql_thread_id(connection) != output->session_))
			status = ENOTCONN;
		if (status)
		{
			(void)output->release();
			return status;
		}
		output->acquisition_confirmed_ = true;
		return 0;
	}
	catch (...)
	{
		if (output)
			(void)output->release();
		return EIO;
	}
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
	try
	{
		if (!output || output->connection_ || !idle(connection) ||
		    !mysql_thread_id(connection))
			return EINVAL;
		std::unique_lock<std::shared_mutex> local(currency_gate(), std::try_to_lock);
		if (!local.owns_lock())
			return EBUSY;
		{
			std::lock_guard lock(authority_mutex());
			if (runtime_authority_active() || maintenance_authority_active())
				return EBUSY;
			output->authority_id_ = allocate_authority_id();
			if (!output->authority_id_)
				return EOVERFLOW;
			maintenance_authority_active() = true;
		}
		output->local_maintenance_ = true;
		output->maintenance_ = true;
		output->local_exclusive_ = std::move(local);
		output->owner_thread_ = std::this_thread::get_id();
		output->connection_ = connection;
		output->session_ = mysql_thread_id(connection);
		auto status =
			lock(connection, output->session_, boot_lock, 0, &output->runtime_lock_);
		if (!status)
			status = lock(connection, output->session_, writer_lock, 10,
				      &output->writer_lock_);
		if (!status &&
		    (!idle(connection) || mysql_thread_id(connection) != output->session_))
			status = ENOTCONN;
		if (status)
		{
			(void)output->release();
			return status;
		}
		output->acquisition_confirmed_ = true;
		return 0;
	}
	catch (...)
	{
		if (output)
			(void)output->release();
		return EIO;
	}
#endif
}

bool economic_sql_lifecycle_guard::is_maintenance_authority() const noexcept
{
	return owner_thread_ == std::this_thread::get_id() && acquisition_confirmed_ &&
	       connection_ && runtime_lock_ && writer_lock_ && maintenance_ &&
	       local_exclusive_.owns_lock() && mysql_thread_id(connection_) == session_;
}

bool economic_sql_lifecycle_guard::is_valid_authority() const noexcept
{
#ifdef __NO_MYSQL__
	return false;
#else
	try
	{
		if (owner_thread_ != std::this_thread::get_id() || !acquisition_confirmed_ ||
		    !connection_ || !authority_id_ || !session_ || !runtime_lock_)
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

bool economic_sql_currency_writer_guard::release() noexcept
{
	if (owner_thread_ != std::thread::id{} && owner_thread_ != std::this_thread::get_id())
		return false;
	acquisition_confirmed_ = false;
	try
	{
#ifndef __NO_MYSQL__
		if (writer_lock_ &&
		    !economic_sql_lifecycle_guard::release_named_lock(
			    connection_, session_, writer_lock, &release_attempted_))
			return false;
#endif
		writer_lock_ = false;
		if (local_shared_.owns_lock())
			local_shared_.unlock();
		connection_ = nullptr;
		session_ = 0;
		owner_thread_ = {};
		release_attempted_ = false;
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool economic_sql_currency_writer_guard::retire_pooled_session() noexcept
{
#ifdef __NO_MYSQL__
	return false;
#else
	if (!connection_ || owner_thread_ != std::this_thread::get_id())
		return false;
	try
	{
		if (!sql_pool_retire_owned_connection(connection_))
			return false;
		// The native pool has closed the exact owned handle. No further MYSQL
		// traffic, including a destructor retry, may touch that consumed pointer.
		connection_ = nullptr;
		session_ = 0;
		writer_lock_ = false;
		acquisition_confirmed_ = false;
		release_attempted_ = false;
		if (local_shared_.owns_lock())
			local_shared_.unlock();
		owner_thread_ = {};
		return true;
	}
	catch (...)
	{
		return false;
	}
#endif
}

economic_sql_currency_writer_guard::~economic_sql_currency_writer_guard()
{
	// A borrowed handle cannot be closed here. Unconfirmed destruction deliberately
	// retains process exclusion; connection owners must use explicit cleanup.
	if (!release())
	{
		// Unresolved borrowed/direct-DB cleanup has no safe replacement owner.
		// Latch runtime SQL admission closed; never close that borrowed handle.
		duris_sql_exclusion_guard_state_ref().lost = true;
		if (local_shared_.owns_lock())
			(void)local_shared_.release();
	}
}

bool economic_sql_currency_writer_guard::is_valid_for(MYSQL *connection) const noexcept
{
#ifdef __NO_MYSQL__
	(void)connection;
	return false;
#else
	try
	{
		if (owner_thread_ != std::this_thread::get_id() || !acquisition_confirmed_ ||
		    !connection || connection != connection_ || !session_ || !writer_lock_ ||
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
	try
	{
		if (!output || output->connection_ || !idle(connection) ||
		    !mysql_thread_id(connection))
			return EINVAL;
		{
			std::lock_guard lock(authority_mutex());
			if (maintenance_authority_active())
				return EPERM;
		}
		output->local_shared_ = std::shared_lock<std::shared_mutex>(currency_gate());
		output->owner_thread_ = std::this_thread::get_id();
		output->connection_ = connection;
		output->session_ = mysql_thread_id(connection);
		auto status =
			lock(connection, output->session_, writer_lock, 10, &output->writer_lock_);
		if (!status)
			status = staged_installation(connection, true);
		if (!status &&
		    (!idle(connection) || mysql_thread_id(connection) != output->session_))
			status = ENOTCONN;
		if (status)
		{
			(void)output->release();
			return status;
		}
		output->acquisition_confirmed_ = true;
		return 0;
	}
	catch (...)
	{
		if (output)
			(void)output->release();
		return EIO;
	}
#endif
}

bool economic_sql_lifecycle_guard::promote_runtime_to_maintenance(
	economic_sql_runtime_world_writer_guard &writer,
	economic_sql_cutover_capability *output) noexcept
{
#ifdef __NO_MYSQL__
	(void)writer;
	(void)output;
	return false;
#else
	try
	{
		if (!output || output->sql_authority_id_ || output->sql_session_ ||
		    output->coordinator_generation_ || output->coordinator_lease_id_ ||
		    maintenance_ || !local_runtime_ || local_maintenance_ || writer_lock_ ||
		    local_exclusive_.mutex() || runtime_release_attempted_ ||
		    writer_release_attempted_ || coordinator_release_ || coordinator_generation_ ||
		    coordinator_lease_id_ || !authority_id_ || writer.runtime_ != this ||
		    writer.connection_ != connection_ || writer.session_ != session_ ||
		    writer.thread_ != owner_thread_ || writer.release_attempted_ || !writer.valid())
			return false;
		std::lock_guard lock(authority_mutex());
		if (!runtime_authority_active() || maintenance_authority_active())
			return false;
		uint64_t generation = 0, lease = 0;
		if (!critical_command_coordinator_owner::transfer_lifecycle_guard_to_cutover_lease(
			    &generation, &lease))
			return false;
		// From this point all ownership moves are nonthrowing. Bind the real
		// lease immediately and move its existing SQL/local exclusion intact.
		static_assert(
			std::is_nothrow_move_assignable_v<std::unique_lock<std::shared_mutex>>);
		coordinator_release_ = critical_command_coordinator_owner::release_cutover_lease;
		coordinator_generation_ = generation;
		coordinator_lease_id_ = lease;
		local_exclusive_ = std::move(writer.local_);
		writer_lock_ = writer.lock_;
		runtime_handoff_ = true;
		maintenance_ = true;
		local_runtime_ = false;
		local_maintenance_ = true;
		maintenance_authority_active() = true;
		runtime_authority_active() = false;
		writer.connection_ = nullptr;
		writer.runtime_ = nullptr;
		writer.session_ = 0;
		writer.thread_ = {};
		writer.lock_ = false;
		writer.confirmed_ = false;
		writer.release_attempted_ = false;
		output->sql_authority_id_ = authority_id_;
		output->sql_session_ = session_;
		output->coordinator_generation_ = generation;
		output->coordinator_lease_id_ = lease;
		return true;
	}
	catch (...)
	{
		return false;
	}
#endif
}

bool economic_sql_lifecycle_guard::restore_runtime_writer(
	economic_sql_runtime_world_writer_guard *writer) noexcept
{
#ifdef __NO_MYSQL__
	(void)writer;
	return false;
#else
	try
	{
		if (!writer || writer->connection_ || writer->runtime_ || writer->session_ ||
		    writer->thread_ != std::thread::id{} || writer->lock_ || writer->confirmed_ ||
		    writer->release_attempted_ || writer->local_.mutex() || !runtime_handoff_ ||
		    !maintenance_ || local_runtime_ || !local_maintenance_ || !writer_lock_ ||
		    !local_exclusive_.owns_lock() || runtime_release_attempted_ ||
		    writer_release_attempted_ || coordinator_release_ || coordinator_generation_ ||
		    coordinator_lease_id_ || !is_valid_authority() ||
		    !critical_command_coordinator_lifecycle_guard_held_by_current_thread() ||
		    !critical_command_coordinator_owner::boot_recovery_ready())
			return false;
		std::lock_guard lock(authority_mutex());
		if (runtime_authority_active() || !maintenance_authority_active() ||
		    !critical_command_coordinator_lifecycle_guard_held_by_current_thread() ||
		    !critical_command_coordinator_owner::boot_recovery_ready())
			return false;
		// All ownership moves below are nonthrowing. The original SQL locks remain
		// on this same session; the currency gate remains locked through the move.
		static_assert(
			std::is_nothrow_move_assignable_v<std::unique_lock<std::shared_mutex>>);
		writer->connection_ = connection_;
		writer->runtime_ = this;
		writer->session_ = session_;
		writer->thread_ = owner_thread_;
		writer->local_ = std::move(local_exclusive_);
		writer->lock_ = writer_lock_;
		writer->confirmed_ = true;
		writer->release_attempted_ = false;
		writer_lock_ = false;
		maintenance_ = false;
		local_maintenance_ = false;
		local_runtime_ = true;
		runtime_handoff_ = false;
		maintenance_authority_active() = false;
		runtime_authority_active() = true;
		return true;
	}
	catch (...)
	{
		return false;
	}
#endif
}

unsigned int economic_sql_runtime_world_writer_guard::acquire(
	MYSQL *connection, const economic_sql_lifecycle_guard &runtime,
	economic_sql_runtime_world_writer_guard *output) noexcept
{
#ifdef __NO_MYSQL__
	(void)connection;
	(void)runtime;
	(void)output;
	return ENOTSUP;
#else
	try
	{
		if (!output || output->connection_ || runtime.connection_ != connection ||
		    runtime.maintenance_ || !runtime.is_valid_authority() ||
		    !critical_command_coordinator_lifecycle_guard_held_by_current_thread())
			return EPERM;
		std::unique_lock<std::shared_mutex> local(currency_gate(), std::try_to_lock);
		if (!local.owns_lock())
			return EBUSY;
		output->connection_ = connection;
		output->runtime_ = &runtime;
		output->session_ = mysql_thread_id(connection);
		output->thread_ = std::this_thread::get_id();
		output->local_ = std::move(local);
		const auto status =
			lock(connection, output->session_, writer_lock, 0, &output->lock_);
		if (status || !idle(connection) ||
		    mysql_thread_id(connection) != output->session_ ||
		    !owns_named_lock(connection, writer_lock, output->session_))
			return status ? status : EIO;
		output->confirmed_ = true;
		return 0;
	}
	catch (...)
	{
		return EIO;
	}
#endif
}

bool economic_sql_runtime_world_writer_guard::valid() const noexcept
{
#ifdef __NO_MYSQL__
	return false;
#else
	try
	{
		return confirmed_ && connection_ && runtime_ && session_ && lock_ &&
		       thread_ == std::this_thread::get_id() && local_.owns_lock() &&
		       mysql_thread_id(connection_) == session_ && idle(connection_) &&
		       runtime_->is_valid_authority() &&
		       critical_command_coordinator_lifecycle_guard_held_by_current_thread() &&
		       owns_named_lock(connection_, writer_lock, session_);
	}
	catch (...)
	{
		return false;
	}
#endif
}

bool economic_sql_runtime_world_writer_guard::release() noexcept
{
	if (thread_ != std::thread::id{} && thread_ != std::this_thread::get_id())
		return false;
	confirmed_ = false;
	try
	{
#ifndef __NO_MYSQL__
		if (lock_ && !economic_sql_lifecycle_guard::release_named_lock(
				     connection_, session_, writer_lock, &release_attempted_))
			return false;
#endif
		lock_ = false;
		if (local_.owns_lock())
			local_.unlock();
		connection_ = nullptr;
		runtime_ = nullptr;
		session_ = 0;
		thread_ = {};
		release_attempted_ = false;
		return true;
	}
	catch (...)
	{
		return false;
	}
}

economic_sql_runtime_world_writer_guard::~economic_sql_runtime_world_writer_guard() noexcept
{
	if (!release())
	{
		duris_sql_exclusion_guard_state_ref().lost = true;
		if (local_.owns_lock())
			(void)local_.release();
	}
}
