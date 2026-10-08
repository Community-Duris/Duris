#include "sql/sql_economic_runtime.h"

#ifndef __NO_MYSQL__
#include "core/prototypes.h"
#include "economy/economic_gameplay_authority.h"
#include "persistence/economic_sql_accounting_lifecycle_transaction.h"
#include "persistence/economic_sql_lifecycle_guard.h"
#include "sql/sql_exclusion_guard.h"
#include "world/quest_mobile_published_world_owner.h"
#include "world/quest_mobile_native_birth.h"
#include "world/events.h"
#include "player/player_save_pipeline.h"
#include "player/player_sql_transaction_cleanup.h"
#include "persistence/critical_command_coordinator.h"
#include <utility>
#include <cstdio>
#include <memory>
#include <unistd.h>

// Use the same verified target, session contract, reconnect policy and
// close-on-exec setup as every other runtime SQL connection.
MYSQL *sql_open_configured_connection(unsigned long client_flags);

namespace
{
// Private diagnostic successor: numeric failure only, never a source of authority.
void report_boot_refusal(unsigned int stage, unsigned int code = 0) noexcept
{
	static bool reported = false;
	if (!reported)
	{
		reported = true;
		std::fprintf(stderr, "boot-recovery-refusal stage=%u code=%u\n", stage, code);
	}
}
struct runtime_owner
{
	std::unique_ptr<MYSQL, decltype(&mysql_close)> connection{ nullptr, mysql_close };
	std::unique_ptr<economic_sql_lifecycle_guard> authority;
	pid_t process = 0;

	bool release() noexcept
	{
		if (process && process != getpid())
		{
			// A forked child must never send protocol traffic or RELEASE_LOCK
			// through a parent's inherited MYSQL socket. Exec/exit closes the
			// child's descriptors without ending the parent's SQL session.
			(void)authority.release();
			(void)connection.release();
			process = 0;
			return true;
		}
		if (authority && !authority->release())
			return false;
		authority.reset();
		connection.reset();
		process = 0;
		return true;
	}

	~runtime_owner()
	{
		if (!release())
		{
			// Process exit is the terminal boundary for an unresolved original
			// session. Do not destroy its mutex owner or close a borrowed handle.
			(void)authority.release();
			(void)connection.release();
		}
	}
};

runtime_owner &owner()
{
	static runtime_owner value;
	return value;
}
} // namespace

class sql_economic_runtime_boot_owner final
{
	friend bool sql_economic_runtime_start_recovery() noexcept;
	friend sql_economic_boot_progress sql_economic_runtime_recover_boot_step(
		void (*)(const critical_completion *, size_t)) noexcept;
	friend bool sql_economic_runtime_finish_boot_admission() noexcept;
	friend void sql_economic_runtime_shutdown() noexcept;
	using world_owner = quest_mobile_published_world_owner;
	sql_economic_runtime_boot_owner() noexcept = default;
	// Process-lifetime retention is intentional. Failed selected boot must not
	// destroy consumed/uncertain world holders or release unresolved exclusion.
	static sql_economic_runtime_boot_owner &instance()
	{
		static auto *value = new sql_economic_runtime_boot_owner;
		return *value;
	}
	bool start() noexcept
	{
		try
		{
			auto &runtime = owner();
			if (prepared_ || runtime.connection || runtime.authority)
				return false;
			runtime.connection.reset(sql_open_configured_connection(0));
			if (!runtime.connection)
				return false;
			runtime.process = getpid();
			runtime.authority = std::make_unique<economic_sql_lifecycle_guard>();
			if (economic_sql_lifecycle_guard::acquire_runtime(
				    runtime.connection.get(), runtime.authority.get()) ||
			    !duris_sql_exclusion_guard_bind_economic_runtime(
				    runtime.connection.get()))
				return false;
			if (economic_sql_accounting_lifecycle_transaction::prepare_runtime_boot(
				    runtime.connection.get(), *runtime.authority, &selection_) ||
			    selection_.selected() !=
				    economic_gameplay_authority::active_sql_recovery())
				return false;
			prepared_ = true;
			return true;
		}
		catch (...)
		{
			return false;
		}
	}
	bool full_cut(std::vector<world_owner::locked_native> *output) noexcept
	{
		auto &runtime = owner();
		MYSQL *connection = runtime.connection.get();
		if (!output || !writer_.valid() || !runtime.authority ||
		    !critical_command_coordinator_owner::boot_recovery_ready() ||
		    !economic_gameplay_authority::active_sql_recovery())
		{
			report_boot_refusal(101);
			return false;
		}
		player_sql_cleanup cleanup;
		player_sql_transaction_cleanup transaction(connection, cleanup);
		std::vector<world_owner::locked_native> complete;
		transaction.starting();
		bool read = false;
		unsigned int read_error = 0;
		unsigned int read_stage = 102;
		try
		{
			if (!mysql_real_query(
				    connection, "SET TRANSACTION ISOLATION LEVEL REPEATABLE READ",
				    sizeof("SET TRANSACTION ISOLATION LEVEL REPEATABLE READ") -
					    1) &&
			    (read_stage = 103,
			     !mysql_real_query(
				     connection, "START TRANSACTION WITH CONSISTENT SNAPSHOT",
				     sizeof("START TRANSACTION WITH CONSISTENT SNAPSHOT") - 1)))
			{
				read_stage = 104;
				read_error = world_owner::read_locked(
					connection, selection_.lineage(), {}, &complete);
				read = !read_error;
			}
			else
				read_error = mysql_errno(connection);
		}
		catch (...)
		{
			read = false;
			read_stage = 105;
		}
		transaction.finish();
		if (!read || !transaction.same_session() || !cleanup.rollback_confirmed ||
		    cleanup.disposition != player_sql_cleanup_disposition::idle_verified ||
		    cleanup.cleanup_error || !writer_.valid() ||
		    !critical_command_coordinator_owner::boot_recovery_ready())
		{
			report_boot_refusal(read ? 106 : read_stage, read_error);
			return false;
		}
		*output = std::move(complete);
		return true;
	}
	sql_economic_boot_progress step(void (*original_completions)(const critical_completion *,
								     size_t)) noexcept
	{
		using progress = sql_economic_boot_progress;
		try
		{
			auto &runtime = owner();
			if (!prepared_ || admitted_ || !original_completions ||
			    !nevent_is_game_thread() || runtime.process != getpid() ||
			    !runtime.connection || !runtime.authority ||
			    !runtime.authority->is_valid_authority())
			{
				report_boot_refusal(201);
				return progress::refused;
			}
			if (promoted_)
				return progress::ready;
			if (!selection_.selected())
			{
				bool active = false;
				if (economic_sql_accounting_lifecycle_transaction::
					    finish_runtime_boot(runtime.connection.get(),
								*runtime.authority, selection_,
								&active) ||
				    active || economic_gameplay_authority::active())
				{
					report_boot_refusal(202);
					return progress::refused;
				}
				promoted_ = true;
				return progress::ready;
			}
			if (!economic_gameplay_authority::active_sql_recovery())
			{
				report_boot_refusal(203);
				return progress::refused;
			}
			if (!reserved_)
			{
				if (!critical_command_coordinator_try_acquire_lifecycle_guard())
				{
					report_boot_refusal(204);
					return progress::refused;
				}
				reserved_ = true;
			}
			if (!critical_command_coordinator_lifecycle_guard_held_by_current_thread())
			{
				report_boot_refusal(205);
				return progress::refused;
			}
			critical_completion completions[64]{};
			const size_t count = critical_command_coordinator_pulse(completions, 64);
			original_completions(completions, count);
			if (!quest_mobile_native_birth_recovery_pulse() ||
			    !critical_command_coordinator_owner::boot_recovery_ready())
				return progress::pending;
			// No accepted worker remains. Acquire the ORIGINAL writer lock/local
			// gate now, never before journal execution or across pending workers.
			if (!writer_acquired_)
			{
				if (economic_sql_runtime_world_writer_guard::acquire(
					    runtime.connection.get(), *runtime.authority, &writer_))
				{
					report_boot_refusal(206);
					return progress::refused;
				}
				writer_acquired_ = true;
			}
			std::vector<world_owner::locked_native> complete, unheld;
			if (!full_cut(&complete))
			{
				report_boot_refusal(207);
				return progress::refused;
			}
			world_owner::idle_token token(runtime.connection.get(),
						      mysql_thread_id(runtime.connection.get()),
						      selection_.lineage());
			if (!world_prepared_)
			{
				if (!world_owner::select_held(complete, &held_))
				{
					report_boot_refusal(208);
					return progress::refused;
				}
				if (!world_owner::split_held(complete, held_, &unheld))
				{
					report_boot_refusal(209);
					return progress::refused;
				}
				if (!world_.prepare(std::move(unheld), token))
				{
					report_boot_refusal(210);
					return progress::refused;
				}
				world_prepared_ = true;
				return progress::pending;
			}
			if (!world_owner::split_held(complete, held_, &unheld))
			{
				report_boot_refusal(211);
				return progress::refused;
			}
			const auto result = world_.advance(unheld, token);
			if (result == world_owner::progress::refused)
			{
				report_boot_refusal(212);
				return progress::refused;
			}
			if (result != world_owner::progress::complete)
				return progress::pending;
			// Reauthenticate the FULL union, including actual journal-restored
			// bodies. Local world completion alone never grants boot readiness.
			if (!full_cut(&complete) ||
			    !world_owner::split_held(complete, held_, &unheld) ||
			    !world_.verify(unheld, token) ||
			    !quest_mobile_native_birth_recovery_pulse() ||
			    !critical_command_coordinator_owner::boot_recovery_ready())
			{
				report_boot_refusal(213);
				return progress::refused;
			}
			bool active = false;
			if (economic_sql_accounting_lifecycle_transaction::finish_runtime_boot(
				    runtime.connection.get(), *runtime.authority, selection_,
				    &active) ||
			    !active || !economic_gameplay_authority::active_regular_sql() ||
			    !writer_.valid() || !writer_.release())
			{
				report_boot_refusal(214);
				return progress::refused;
			}
			writer_acquired_ = false;
			promoted_ = true;
			// Admission remains reserved until actual ordinary save startup.
			return progress::ready;
		}
		catch (...)
		{
			{
				report_boot_refusal(215);
				return progress::refused;
			}
		}
	}
	bool admit() noexcept
	{
		if (!prepared_ || !promoted_ || admitted_ || !selection_.finished() ||
		    !nevent_is_game_thread() || owner().process != getpid())
			return false;
		if (selection_.selected())
		{
			const auto saves = player_save_pipeline_health_copy();
			if (!reserved_ || !saves.initialized || !saves.dispatcher_running ||
			    !saves.accepting ||
			    !economic_gameplay_authority::active_regular_sql() ||
			    !critical_command_coordinator_owner::boot_recovery_ready())
				return false;
			critical_command_coordinator_release_lifecycle_guard();
			if (critical_command_coordinator_lifecycle_guard_held_by_current_thread())
				return false;
			reserved_ = false;
		}
		admitted_ = true;
		return true;
	}
	bool shutdown() noexcept
	{
		if (owner().process && owner().process != getpid())
			return false; // No child protocol traffic through inherited owner.
		// A failed selected boot retains its genuine writer/local gate,
		// coordinator reservation and consumed world holders until the original
		// terminal process/world cleanup boundary. Do not release one lease
		// merely because the later reservation cleanup will refuse.
		if (reserved_ && !admitted_)
			return false;
		if (!writer_.release())
			return false;
		if (reserved_)
		{
			critical_command_coordinator_release_lifecycle_guard();
			reserved_ = false;
		}
		return true;
	}
	economic_sql_runtime_boot_selection selection_;
	economic_sql_runtime_world_writer_guard writer_;
	world_owner world_;
	std::vector<world_owner::held_native> held_;
	bool prepared_ = false, reserved_ = false, writer_acquired_ = false;
	bool world_prepared_ = false, promoted_ = false, admitted_ = false;
};

bool sql_economic_runtime_start_recovery() noexcept
{
	try
	{
		return sql_economic_runtime_boot_owner::instance().start();
	}
	catch (...)
	{
		return false;
	}
}

sql_economic_boot_progress sql_economic_runtime_recover_boot_step(
	void (*original_completions)(const critical_completion *, size_t)) noexcept
{
	try
	{
		return sql_economic_runtime_boot_owner::instance().step(original_completions);
	}
	catch (...)
	{
		return sql_economic_boot_progress::refused;
	}
}

bool sql_economic_runtime_finish_boot_admission() noexcept
{
	try
	{
		return sql_economic_runtime_boot_owner::instance().admit();
	}
	catch (...)
	{
		return false;
	}
}

bool sql_economic_runtime_start() noexcept
{
	try
	{
		auto &runtime = owner();
		if (runtime.connection || runtime.authority)
			return false;
		runtime.connection.reset(sql_open_configured_connection(0));
		if (!runtime.connection)
			return false;
		runtime.process = getpid();
		runtime.authority = std::make_unique<economic_sql_lifecycle_guard>();
		if (economic_sql_lifecycle_guard::acquire_runtime(runtime.connection.get(),
								  runtime.authority.get()) ||
		    !duris_sql_exclusion_guard_bind_economic_runtime(runtime.connection.get()))
		{
			(void)runtime.release();
			return false;
		}
		bool active = false;
		if (economic_sql_accounting_lifecycle_transaction::recover_runtime(
			    runtime.connection.get(), *runtime.authority, &active) ||
		    active != economic_gameplay_authority::active())
		{
			economic_gameplay_authority::clear_sql_runtime();
			(void)runtime.release();
			return false;
		}
		return true;
	}
	catch (...)
	{
		economic_gameplay_authority::clear_sql_runtime();
		(void)owner().release();
		return false;
	}
}

void sql_economic_runtime_shutdown() noexcept
{
	try
	{
		if (!sql_economic_runtime_boot_owner::instance().shutdown())
			return;
	}
	catch (...)
	{
		return;
	}
	economic_gameplay_authority::clear_sql_runtime();
	(void)owner().release();
}
#else
bool sql_economic_runtime_start() noexcept
{
	return false;
}

void sql_economic_runtime_shutdown() noexcept {}
bool sql_economic_runtime_start_recovery() noexcept
{
	return false;
}
sql_economic_boot_progress
sql_economic_runtime_recover_boot_step(void (*)(const critical_completion *, size_t)) noexcept
{
	return sql_economic_boot_progress::refused;
}
bool sql_economic_runtime_finish_boot_admission() noexcept
{
	return false;
}
#endif
