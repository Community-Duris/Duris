#include "persistence/economic_sql_lifecycle_guard.h"
#include "persistence/critical_command_coordinator.h"

#include <charconv>
#include <cstdio>
#include <limits>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>

namespace
{
#ifndef __NO_MYSQL__
using result_ptr = std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)>;

bool reconnect_disabled(MYSQL *connection)
{
	if (!connection)
		return false;
	using flag = std::remove_pointer_t<decltype(MYSQL_BIND{}.is_null)>;
	flag reconnect = true;
	return !mysql_get_option(connection, MYSQL_OPT_RECONNECT, &reconnect) && !reconnect;
}

bool query_lock_owner(MYSQL *connection, const char *name, unsigned long *owner, bool *has_owner)
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
	if (!row)
		return false;
	if (!row[0])
	{
		*owner = 0;
		*has_owner = false;
		return true;
	}
	uint64_t parsed_owner = 0;
	const auto row_length = std::char_traits<char>::length(row[0]);
	const auto parsed = std::from_chars(row[0], row[0] + row_length, parsed_owner);
	if (parsed.ec != std::errc{} || parsed.ptr != row[0] + row_length || !parsed_owner ||
	    parsed_owner > std::numeric_limits<unsigned long>::max())
		return false;
	*owner = static_cast<unsigned long>(parsed_owner);
	*has_owner = true;
	return true;
}

bool owns_exact_lock(MYSQL *connection, const char *name, unsigned long session)
{
	unsigned long owner = 0;
	bool has_owner = false;
	return mysql_thread_id(connection) == session &&
	       query_lock_owner(connection, name, &owner, &has_owner) && has_owner &&
	       owner == session;
}

bool exact_live_transaction(MYSQL *connection, unsigned long session)
{
	return connection && session && mysql_thread_id(connection) == session &&
	       reconnect_disabled(connection) && !mysql_ping(connection) &&
	       mysql_thread_id(connection) == session && reconnect_disabled(connection) &&
	       (connection->server_status & SERVER_STATUS_IN_TRANS);
}
#endif
} // namespace

bool economic_sql_lifecycle_guard::acquire_cutover_capability(
	uint64_t drain_timeout_msec, economic_sql_cutover_capability *output) noexcept
{
#ifdef __NO_MYSQL__
	(void)drain_timeout_msec;
	(void)output;
	return false;
#else
	if (!output || output->sql_authority_id_ || output->sql_session_ ||
	    output->coordinator_generation_ || output->coordinator_lease_id_ || !authority_id_ ||
	    coordinator_lease_id_ || !is_valid_authority())
		return false;

	uint64_t generation = 0;
	uint64_t lease_id = 0;
	try
	{
		if (!critical_command_coordinator_owner::acquire_cutover_lease(
			    drain_timeout_msec, &generation, &lease_id))
			return false;
		// Bind immediately. Failed SQL validation retains the exact coordinator
		// lease until checked guard cleanup, rather than discarding its identity.
		coordinator_release_ = critical_command_coordinator_owner::release_cutover_lease;
		coordinator_generation_ = generation;
		coordinator_lease_id_ = lease_id;
		if (!is_valid_authority())
		{
			acquisition_confirmed_ = false;
			return false;
		}
	}
	catch (...)
	{
		if (generation && lease_id)
		{
			coordinator_release_ =
				critical_command_coordinator_owner::release_cutover_lease;
			coordinator_generation_ = generation;
			coordinator_lease_id_ = lease_id;
			acquisition_confirmed_ = false;
		}
		return false;
	}

	output->sql_authority_id_ = authority_id_;
	output->sql_session_ = session_;
	output->coordinator_generation_ = generation;
	output->coordinator_lease_id_ = lease_id;
	return true;
#endif
}

bool economic_sql_lifecycle_guard::is_valid_cutover_capability(
	const economic_sql_cutover_capability &capability) const noexcept
{
#ifdef __NO_MYSQL__
	(void)capability;
	return false;
#else
	if (!capability.sql_authority_id_ || !capability.sql_session_ ||
	    !capability.coordinator_generation_ || !capability.coordinator_lease_id_ ||
	    capability.sql_authority_id_ != authority_id_ || capability.sql_session_ != session_ ||
	    capability.coordinator_generation_ != coordinator_generation_ ||
	    capability.coordinator_lease_id_ != coordinator_lease_id_ || !is_valid_authority())
		return false;
	try
	{
		return critical_command_coordinator_owner::validate_cutover_lease(
			capability.coordinator_generation_, capability.coordinator_lease_id_);
	}
	catch (...)
	{
		return false;
	}
#endif
}

bool economic_sql_cutover_capability::is_valid_for(
	const economic_sql_lifecycle_guard &guard) const noexcept
{
	return guard.is_valid_cutover_capability(*this);
}

economic_sql_cutover_transaction_owner::~economic_sql_cutover_transaction_owner()
{
	if (active_ && !terminal_ && local_exclusive_.owns_lock())
	{
		// Intentionally leave the local writer gate locked when outcome or
		// terminal cleanup is unresolved; destruction is not an abort protocol.
		(void)local_exclusive_.release();
	}
}

bool economic_sql_cutover_transaction_owner::begin(
	economic_sql_lifecycle_guard &guard,
	const economic_sql_cutover_capability &capability) noexcept
{
#ifdef __NO_MYSQL__
	(void)guard;
	(void)capability;
	return false;
#else
	if (active_ || terminal_ || connection_ || !guard.connection_ || !guard.session_ ||
	    !guard.coordinator_release_ || !capability.is_valid_for(guard) ||
	    capability.sql_authority_id_ != guard.authority_id_ ||
	    capability.sql_session_ != guard.session_ ||
	    capability.coordinator_generation_ != guard.coordinator_generation_ ||
	    capability.coordinator_lease_id_ != guard.coordinator_lease_id_ ||
	    mysql_thread_id(guard.connection_) != guard.session_ ||
	    !reconnect_disabled(guard.connection_))
		return false;

	MYSQL *const exact_connection = guard.connection_;
	const unsigned long exact_session = guard.session_;
	const uint64_t generation = guard.coordinator_generation_;
	const uint64_t lease_id = guard.coordinator_lease_id_;
	try
	{
		if (!critical_command_coordinator_owner::begin_cutover_transaction(
			    generation, lease_id, exact_connection, exact_session))
			return false;
	}
	catch (...)
	{
		return false;
	}

	owner_thread_ = guard.owner_thread_;
	connection_ = exact_connection;
	session_ = exact_session;
	sql_authority_id_ = guard.authority_id_;
	coordinator_generation_ = generation;
	coordinator_lease_id_ = lease_id;
	runtime_lock_ = guard.runtime_lock_;
	writer_lock_ = guard.writer_lock_;
	maintenance_ = guard.maintenance_;
	local_runtime_ = guard.local_runtime_;
	local_maintenance_ = guard.local_maintenance_;
	local_exclusive_ = std::move(guard.local_exclusive_);
	active_ = true;
	started_ = false;
	outcome_uncertain_ = true;
	publication_pending_ = false;
	terminal_outcome_ = economic_sql_cutover_terminal_outcome::unresolved;
	sql_resources_released_ = false;

	guard.coordinator_release_ = nullptr;
	guard.coordinator_generation_ = 0;
	guard.coordinator_lease_id_ = 0;
	guard.connection_ = nullptr;
	guard.session_ = 0;
	guard.owner_thread_ = {};
	guard.acquisition_confirmed_ = false;
	guard.runtime_release_attempted_ = false;
	guard.writer_release_attempted_ = false;
	guard.runtime_lock_ = false;
	guard.writer_lock_ = false;
	guard.maintenance_ = false;
	guard.local_runtime_ = false;
	guard.local_maintenance_ = false;
	guard.authority_id_ = 0;

	try
	{
		static constexpr char isolation[] =
			"SET TRANSACTION ISOLATION LEVEL REPEATABLE READ";
		static constexpr char start[] = "START TRANSACTION WITH CONSISTENT SNAPSHOT";
		if (mysql_real_query(connection_, isolation, sizeof(isolation) - 1) ||
		    mysql_real_query(connection_, start, sizeof(start) - 1) ||
		    mysql_thread_id(connection_) != session_ ||
		    !(connection_->server_status & SERVER_STATUS_IN_TRANS) ||
		    !reconnect_disabled(connection_))
		{
			critical_command_coordinator_owner::set_cutover_outcome_uncertain(
				generation, lease_id, connection_, session_, true);
			return false;
		}
		started_ = true;
		outcome_uncertain_ = false;
		critical_command_coordinator_owner::set_cutover_outcome_uncertain(
			generation, lease_id, connection_, session_, false);
		return true;
	}
	catch (...)
	{
		outcome_uncertain_ = true;
		try
		{
			critical_command_coordinator_owner::set_cutover_outcome_uncertain(
				generation, lease_id, connection_, session_, true);
		}
		catch (...)
		{
		}
		return false;
	}
#endif
}

bool economic_sql_cutover_transaction_owner::is_valid() noexcept
{
#ifdef __NO_MYSQL__
	return false;
#else
	if (!active_ || !started_ || outcome_uncertain_ ||
	    terminal_outcome_ != economic_sql_cutover_terminal_outcome::unresolved ||
	    !connection_ || !runtime_lock_)
		return false;
	try
	{
		const bool valid =
			critical_command_coordinator_owner::validate_cutover_transaction(
				coordinator_generation_, coordinator_lease_id_, connection_,
				session_) &&
			exact_live_transaction(connection_, session_) &&
			owns_exact_lock(connection_, ECONOMIC_SQL_BOOT_MAINTENANCE_LOCK_NAME,
					session_) &&
			(!writer_lock_ ||
			 owns_exact_lock(connection_, "duris:economic_sql_currency_writers",
					 session_)) &&
			mysql_thread_id(connection_) == session_ &&
			(connection_->server_status & SERVER_STATUS_IN_TRANS);
		if (valid)
			return true;
	}
	catch (...)
	{
	}
	outcome_uncertain_ = true;
	try
	{
		critical_command_coordinator_owner::set_cutover_outcome_uncertain(
			coordinator_generation_, coordinator_lease_id_, connection_, session_,
			true);
	}
	catch (...)
	{
	}
	return false;
#endif
}

bool economic_sql_cutover_transaction_owner::commit_and_retain_publication() noexcept
{
#ifdef __NO_MYSQL__
	return false;
#else
	if (!active_ || !started_ || outcome_uncertain_ || publication_pending_ ||
	    terminal_outcome_ != economic_sql_cutover_terminal_outcome::unresolved ||
	    !maintenance_ || !runtime_lock_ || !writer_lock_ || local_runtime_ ||
	    !local_maintenance_ || !local_exclusive_.owns_lock() || !is_valid())
		return false;
	if (mysql_commit(connection_) || mysql_thread_id(connection_) != session_ ||
	    (connection_->server_status & SERVER_STATUS_IN_TRANS))
	{
		outcome_uncertain_ = true;
		try
		{
			critical_command_coordinator_owner::set_cutover_outcome_uncertain(
				coordinator_generation_, coordinator_lease_id_, connection_,
				session_, true);
		}
		catch (...)
		{
		}
		return false;
	}
	started_ = false;
	outcome_uncertain_ = false;
	terminal_outcome_ = economic_sql_cutover_terminal_outcome::committed;
	publication_pending_ = true;
	return is_valid_for_publication();
#endif
}

bool economic_sql_cutover_transaction_owner::is_valid_for_publication() noexcept
{
#ifdef __NO_MYSQL__
	return false;
#else
	if (!active_ || !publication_pending_ || started_ || outcome_uncertain_ ||
	    terminal_outcome_ != economic_sql_cutover_terminal_outcome::committed || !connection_ ||
	    !session_ || !sql_authority_id_ || !maintenance_ || !runtime_lock_ || !writer_lock_ ||
	    local_runtime_ || !local_maintenance_ || sql_resources_released_ ||
	    !local_exclusive_.owns_lock())
		return false;
	try
	{
		const auto coordinator_valid = [&]
		{
			return critical_command_coordinator_owner::validate_cutover_transaction(
				coordinator_generation_, coordinator_lease_id_, connection_,
				session_);
		};
		if (!coordinator_valid() || mysql_thread_id(connection_) != session_ ||
		    !reconnect_disabled(connection_) || mysql_ping(connection_) ||
		    mysql_thread_id(connection_) != session_ || !reconnect_disabled(connection_) ||
		    (connection_->server_status & SERVER_STATUS_IN_TRANS) ||
		    !(connection_->server_status & SERVER_STATUS_AUTOCOMMIT) ||
		    !owns_exact_lock(connection_, ECONOMIC_SQL_BOOT_MAINTENANCE_LOCK_NAME,
				     session_) ||
		    !owns_exact_lock(connection_, "duris:economic_sql_currency_writers",
				     session_) ||
		    mysql_thread_id(connection_) != session_ || !reconnect_disabled(connection_) ||
		    (connection_->server_status & SERVER_STATUS_IN_TRANS) ||
		    !(connection_->server_status & SERVER_STATUS_AUTOCOMMIT) ||
		    !coordinator_valid())
			return false;
		return true;
	}
	catch (...)
	{
		return false;
	}
#endif
}

bool economic_sql_cutover_transaction_owner::finish_publication(
	economic_sql_lifecycle_guard *lifetime_guard) noexcept
{
#ifdef __NO_MYSQL__
	(void)lifetime_guard;
	return false;
#else
	if (!lifetime_guard || lifetime_guard->connection_ || lifetime_guard->session_ ||
	    lifetime_guard->owner_thread_ != std::thread::id{} ||
	    lifetime_guard->acquisition_confirmed_ || lifetime_guard->runtime_release_attempted_ ||
	    lifetime_guard->writer_release_attempted_ || lifetime_guard->runtime_lock_ ||
	    lifetime_guard->writer_lock_ || lifetime_guard->maintenance_ ||
	    lifetime_guard->local_runtime_ || lifetime_guard->local_maintenance_ ||
	    lifetime_guard->coordinator_release_ || lifetime_guard->authority_id_ ||
	    lifetime_guard->coordinator_generation_ || lifetime_guard->coordinator_lease_id_ ||
	    lifetime_guard->local_exclusive_.owns_lock() ||
	    lifetime_guard->local_exclusive_.mutex() || !publication_pending_ ||
	    !is_valid_for_publication())
		return false;

	// Finish admission only after publication, while this owner still holds every
	// SQL/local fence. A refusal leaves both the output and retained owner intact.
	try
	{
		if (!critical_command_coordinator_owner::finish_cutover_transaction(
			    coordinator_generation_, coordinator_lease_id_, connection_, session_))
			return false;
	}
	catch (...)
	{
		return false;
	}

	// All remaining writes are non-throwing. The SQL locks stay owned by this
	// exact session and the local mutex stays locked as unique_lock ownership moves.
	static_assert(std::is_nothrow_move_assignable_v<std::unique_lock<std::shared_mutex>>);
	lifetime_guard->acquisition_confirmed_ = true;
	lifetime_guard->owner_thread_ = owner_thread_;
	lifetime_guard->connection_ = connection_;
	lifetime_guard->session_ = session_;
	lifetime_guard->runtime_lock_ = runtime_lock_;
	lifetime_guard->writer_lock_ = writer_lock_;
	lifetime_guard->maintenance_ = maintenance_;
	lifetime_guard->local_runtime_ = local_runtime_;
	lifetime_guard->local_maintenance_ = local_maintenance_;
	lifetime_guard->authority_id_ = sql_authority_id_;
	lifetime_guard->local_exclusive_ = std::move(local_exclusive_);

	active_ = false;
	terminal_ = true;
	publication_pending_ = false;
	connection_ = nullptr;
	session_ = 0;
	owner_thread_ = {};
	sql_authority_id_ = 0;
	coordinator_generation_ = 0;
	coordinator_lease_id_ = 0;
	runtime_lock_ = false;
	writer_lock_ = false;
	maintenance_ = false;
	local_runtime_ = false;
	local_maintenance_ = false;
	return true;
#endif
}

bool economic_sql_cutover_transaction_owner::release_after_terminal() noexcept
{
#ifdef __NO_MYSQL__
	return false;
#else
	if (owner_thread_ != std::this_thread::get_id() || !active_ || publication_pending_ ||
	    terminal_outcome_ == economic_sql_cutover_terminal_outcome::unresolved || !connection_)
		return false;
	if (!sql_resources_released_)
	{
		try
		{
			if (writer_lock_)
			{
				if (!economic_sql_lifecycle_guard::release_named_lock(
					    connection_, session_,
					    "duris:economic_sql_currency_writers",
					    &writer_release_attempted_))
					return false;
				writer_lock_ = false;
			}
			if (runtime_lock_)
			{
				if (!economic_sql_lifecycle_guard::release_named_lock(
					    connection_, session_,
					    ECONOMIC_SQL_BOOT_MAINTENANCE_LOCK_NAME,
					    &runtime_release_attempted_))
					return false;
				runtime_lock_ = false;
			}
			if (local_exclusive_.owns_lock())
				local_exclusive_.unlock();
			if (local_runtime_ || local_maintenance_)
				economic_sql_lifecycle_guard::clear_transferred_local_authority(
					local_runtime_, local_maintenance_);
			sql_resources_released_ = true;
		}
		catch (...)
		{
			return false;
		}
	}
	try
	{
		if (!critical_command_coordinator_owner::finish_cutover_transaction(
			    coordinator_generation_, coordinator_lease_id_, connection_, session_))
			return false;
	}
	catch (...)
	{
		return false;
	}
	active_ = false;
	terminal_ = true;
	connection_ = nullptr;
	session_ = 0;
	owner_thread_ = {};
	sql_authority_id_ = 0;
	coordinator_generation_ = 0;
	coordinator_lease_id_ = 0;
	runtime_lock_ = false;
	writer_lock_ = false;
	maintenance_ = false;
	local_runtime_ = false;
	local_maintenance_ = false;
	return true;
#endif
}

bool economic_sql_cutover_transaction_owner::commit() noexcept
{
#ifdef __NO_MYSQL__
	return false;
#else
	if (publication_pending_)
		return false;
	if (terminal_outcome_ != economic_sql_cutover_terminal_outcome::unresolved)
		return terminal_outcome_ == economic_sql_cutover_terminal_outcome::committed &&
		       release_after_terminal();
	if (!active_ || !started_ || outcome_uncertain_ || !is_valid())
		return false;
	if (mysql_commit(connection_) || mysql_thread_id(connection_) != session_ ||
	    (connection_->server_status & SERVER_STATUS_IN_TRANS))
	{
		outcome_uncertain_ = true;
		try
		{
			critical_command_coordinator_owner::set_cutover_outcome_uncertain(
				coordinator_generation_, coordinator_lease_id_, connection_,
				session_, true);
		}
		catch (...)
		{
		}
		return false;
	}
	started_ = false;
	outcome_uncertain_ = false;
	terminal_outcome_ = economic_sql_cutover_terminal_outcome::committed;
	return release_after_terminal();
#endif
}

bool economic_sql_cutover_transaction_owner::rollback() noexcept
{
#ifdef __NO_MYSQL__
	return false;
#else
	if (terminal_outcome_ != economic_sql_cutover_terminal_outcome::unresolved)
		return terminal_outcome_ == economic_sql_cutover_terminal_outcome::rolled_back &&
		       release_after_terminal();
	if (!active_ || !connection_)
		return false;
	try
	{
		const bool exact_recovery_session =
			critical_command_coordinator_owner::validate_cutover_transaction(
				coordinator_generation_, coordinator_lease_id_, connection_,
				session_) &&
			mysql_thread_id(connection_) == session_ &&
			reconnect_disabled(connection_) &&
			(!started_ || (connection_->server_status & SERVER_STATUS_IN_TRANS));
		if (!exact_recovery_session)
		{
			outcome_uncertain_ = true;
			critical_command_coordinator_owner::set_cutover_outcome_uncertain(
				coordinator_generation_, coordinator_lease_id_, connection_,
				session_, true);
			return false;
		}
		if (mysql_rollback(connection_) || mysql_thread_id(connection_) != session_ ||
		    (connection_->server_status & SERVER_STATUS_IN_TRANS))
		{
			outcome_uncertain_ = true;
			critical_command_coordinator_owner::set_cutover_outcome_uncertain(
				coordinator_generation_, coordinator_lease_id_, connection_,
				session_, true);
			return false;
		}
		started_ = false;
		outcome_uncertain_ = false;
		terminal_outcome_ = economic_sql_cutover_terminal_outcome::rolled_back;
		critical_command_coordinator_owner::set_cutover_outcome_uncertain(
			coordinator_generation_, coordinator_lease_id_, connection_, session_,
			false);
		return release_after_terminal();
	}
	catch (...)
	{
		outcome_uncertain_ = true;
		try
		{
			critical_command_coordinator_owner::set_cutover_outcome_uncertain(
				coordinator_generation_, coordinator_lease_id_, connection_,
				session_, true);
		}
		catch (...)
		{
		}
		return false;
	}
#endif
}

bool economic_sql_cutover_transaction_owner::retry_cleanup() noexcept
{
#ifdef __NO_MYSQL__
	return false;
#else
	if (publication_pending_ ||
	    terminal_outcome_ == economic_sql_cutover_terminal_outcome::unresolved)
		return false;
	return release_after_terminal();
#endif
}
