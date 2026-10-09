#include "sql/sql_economic_runtime.h"
#include "world/economic_initialized_world_owner.h"
#include <cerrno>

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
#include "player/player_save_replay_ownership.h"
#include "player/player_sql_transaction_cleanup.h"
#include "persistence/critical_command_coordinator.h"
#include "persistence/critical_outbox.h"
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
	friend class economic_initialized_world_owner;
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
	unsigned int begin_initialized_cut() noexcept
	{
		auto &runtime = owner();
		if (!prepared_ || !promoted_ || !admitted_ || !selection_.finished() ||
		    !selection_.selected() || !nevent_is_game_thread() ||
		    runtime.process != getpid() || !runtime.connection || !runtime.authority ||
		    !runtime.authority->is_valid_authority() ||
		    mysql_thread_id(runtime.connection.get()) != selection_.session_ ||
		    !economic_gameplay_authority::active_regular_sql() || late_reserved_ ||
		    writer_acquired_)
			return EPERM;
		if (!critical_command_coordinator_owner::acquire_initialized_lifecycle_guard())
			return EBUSY;
		late_reserved_ = true;
		const auto outbox = critical_outbox_health_copy();
		// An uninitialized drain has no durable-outbox proof. These observations
		// can refuse but cannot replace the original SQL-backed drain below.
		if (!outbox.initialized || !outbox.running)
			return EBUSY;
		late_outbox_resume_ = outbox.accepting;
		critical_outbox_quiesce();
		late_outbox_quiesced_ = true;
		// Drain original accepted saves/journal work BEFORE capturing a world cut.
		// This existing owned drain pumps actual native completion callbacks and
		// authenticates worker/retained-frame/ownership-epoch quiescence.
		if (!player_save_pipeline_drain_owned(3000) ||
		    !critical_command_coordinator_owner::boot_recovery_ready())
			return EBUSY;
		// Prior-process durable outbox rows can coexist with an empty command
		// journal. Invoke its actual registered native publication observer and
		// original SQL reconciliation, then reprove saves after those callbacks.
		critical_outbox_quiesce();
		if (!critical_outbox_drain(3000) || !player_save_pipeline_drain_owned(3000))
			return EBUSY;
		critical_outbox_quiesce();
		if (!critical_command_coordinator_owner::boot_recovery_ready())
			return EBUSY;
		const auto acquired = economic_sql_runtime_world_writer_guard::acquire(
			runtime.connection.get(), *runtime.authority, &writer_);
		if (acquired)
			return acquired;
		writer_acquired_ = true;
		const auto epoch = player_save_execution_guard::current_ownership_epoch();
		if (!epoch || !player_save_execution_guard::ownership_epoch_quiescent(epoch) ||
		    !writer_.valid() || !critical_command_coordinator_owner::boot_recovery_ready())
			return EPERM;
		economic_initialized_world_owner::boot_capability actual(
			runtime.connection.get(), selection_.session_, selection_.authority_id_,
			epoch, getpid(), runtime.authority.get(), &writer_);
		return economic_initialized_world_owner::capture_sql_cut(actual);
	}
	bool initialized_cut_valid(
		const economic_initialized_world_owner::boot_capability &actual) noexcept
	{
		auto &runtime = owner();
		return late_reserved_ && late_outbox_quiesced_ &&
		       !critical_outbox_health_copy().accepting && writer_acquired_ && prepared_ &&
		       promoted_ && admitted_ && selection_.finished() && selection_.selected() &&
		       nevent_is_game_thread() && runtime.process == getpid() &&
		       actual.process_ == getpid() && runtime.connection && runtime.authority &&
		       actual.connection_ == runtime.connection.get() &&
		       actual.runtime_ == runtime.authority.get() && actual.writer_ == &writer_ &&
		       actual.session_ == selection_.session_ &&
		       actual.authority_id_ == selection_.authority_id_ &&
		       mysql_thread_id(runtime.connection.get()) == actual.session_ &&
		       actual.save_epoch_ &&
		       actual.save_epoch_ ==
			       player_save_execution_guard::current_ownership_epoch() &&
		       player_save_execution_guard::ownership_epoch_quiescent(actual.save_epoch_) &&
		       economic_gameplay_authority::active_regular_sql() && writer_.valid() &&
		       critical_command_coordinator_owner::boot_recovery_ready();
	}
	unsigned int prepare_initialized_cutover() noexcept
	{
		auto &runtime = owner();
		if (cutover_ != cutover_phase::runtime ||
		    late_cleanup_ != late_cleanup_phase::held || !late_reserved_ ||
		    !late_outbox_quiesced_ || critical_outbox_health_copy().accepting ||
		    !writer_acquired_ || !prepared_ || !promoted_ || !admitted_ ||
		    !selection_.finished() || !selection_.selected() || !nevent_is_game_thread() ||
		    runtime.process != getpid() || !runtime.connection || !runtime.authority ||
		    mysql_thread_id(runtime.connection.get()) != selection_.session_ ||
		    !economic_gameplay_authority::active_regular_sql() || !writer_.valid() ||
		    !critical_command_coordinator_owner::boot_recovery_ready())
			return EPERM;
		const auto epoch = player_save_execution_guard::current_ownership_epoch();
		if (!epoch || !player_save_execution_guard::ownership_epoch_quiescent(epoch))
			return EBUSY;
		try
		{
			if (!cutover_owner_)
				cutover_owner_ =
					std::make_unique<economic_sql_cutover_transaction_owner>();
			// This is the real current ownership incarnation authenticated against
			// the retained boot capability before AND after this preparation call.
			late_cutover_save_epoch_ = epoch;
			return 0;
		}
		catch (...)
		{
			return ENOMEM;
		}
	}
	bool cutover_source_held() const noexcept
	{
		return nevent_is_game_thread() && owner().process == getpid() &&
		       late_outbox_quiesced_ && !critical_outbox_health_copy().accepting &&
		       late_cutover_save_epoch_ &&
		       late_cutover_save_epoch_ ==
			       player_save_execution_guard::current_ownership_epoch() &&
		       player_save_execution_guard::ownership_epoch_quiescent(
			       late_cutover_save_epoch_);
	}
	unsigned int begin_initialized_cutover() noexcept
	{
		auto &runtime = owner();
		if (!cutover_source_held() || !runtime.connection || !runtime.authority ||
		    !cutover_owner_)
			return EPERM;
		if (cutover_ == cutover_phase::runtime)
		{
			// Allocation/save-epoch retention happened while the actual capability
			// was still valid. Never replace its incarnation after revocation.
			if (late_cleanup_ != late_cleanup_phase::held || !late_reserved_ ||
			    !writer_acquired_ || !writer_.valid() ||
			    !critical_command_coordinator_owner::boot_recovery_ready() ||
			    !runtime.authority->promote_runtime_to_maintenance(
				    writer_, &cutover_capability_))
				return EPERM;
			// These obligations moved into the real maintenance guard/lease, not
			// through release/reacquire. The world capability was revoked first.
			writer_acquired_ = false;
			late_reserved_ = false;
			cutover_ = cutover_phase::promoted_guard;
		}
		if (cutover_ != cutover_phase::promoted_guard)
			return EPERM;
		const bool begun = cutover_owner_->begin(*runtime.authority, cutover_capability_);
		// START failure can still have transferred every fence. Retain the real
		// session owner and recognize that ownership instead of using its bool.
		if (cutover_owner_->connection_ == runtime.connection.get())
			cutover_ = cutover_phase::transaction;
		return begun ? 0 : EIO;
	}
	bool abort_initialized_cutover() noexcept
	{
		auto &runtime = owner();
		if (!cutover_source_held() || !runtime.connection || !runtime.authority ||
		    !cutover_owner_ ||
		    mysql_thread_id(runtime.connection.get()) != selection_.session_)
			return false;
		if (cutover_ == cutover_phase::runtime)
			return end_initialized_cut(); // Promotion never transferred ownership.
		if (cutover_ == cutover_phase::promoted_guard)
		{
			// The same genuine lease must reach its transaction owner before the
			// original rollback can establish an abort. Never release its guard.
			(void)begin_initialized_cutover();
			if (cutover_ != cutover_phase::transaction)
				return false;
		}
		if (cutover_ == cutover_phase::transaction)
		{
			if (!cutover_owner_->rollback_and_retain_runtime() ||
			    economic_sql_accounting_lifecycle_transaction::
				    verify_aborted_runtime_projection(runtime.connection.get(),
								      *cutover_owner_,
								      selection_) ||
			    !cutover_source_held() ||
			    !cutover_owner_->finish_aborted_runtime(runtime.authority.get()))
				return false;
			late_reserved_ = true;
			cutover_ = cutover_phase::returned_guard;
		}
		if (cutover_ == cutover_phase::returned_guard)
		{
			// A previous successful read is not a retry permit. Authenticate the
			// selected epoch/baseline/full mappings freshly under the returned guard.
			if (!critical_command_coordinator_owner::boot_recovery_ready() ||
			    economic_sql_accounting_lifecycle_transaction::
				    verify_returned_runtime_projection(runtime.connection.get(),
								       *runtime.authority,
								       selection_) ||
			    !cutover_source_held() ||
			    !runtime.authority->restore_runtime_writer(&writer_))
				return false;
			writer_acquired_ = true;
			cutover_ = cutover_phase::runtime_cleanup;
		}
		if (cutover_ != cutover_phase::runtime_cleanup)
			return false;
		return end_initialized_cut();
	}
	bool initialized_cutover_valid(economic_sql_cutover_transaction_owner &actual) noexcept
	{
		auto &runtime = owner();
		return !cutover_verification_abort_required_ &&
		       cutover_ == cutover_phase::transaction && cutover_source_held() &&
		       cutover_owner_.get() == &actual && runtime.connection &&
		       actual.connection_ == runtime.connection.get() && actual.runtime_handoff_ &&
		       actual.maintenance_ && actual.session_ == selection_.session_ &&
		       actual.sql_authority_id_ == selection_.authority_id_ && prepared_ &&
		       promoted_ && admitted_ && selection_.finished() && selection_.selected() &&
		       economic_gameplay_authority::active_regular_sql() && actual.is_valid();
	}
	unsigned int verify_initialized_cutover_sources(
		const economic_sql_lifecycle_request &request,
		const economic_sql_activation_evidence &evidence,
		economic_sql_initialized_activation_verifier verify) noexcept
	{
		if (!cutover_owner_ || !initialized_cutover_valid(*cutover_owner_))
			return EPERM;
		const auto result = economic_sql_accounting_lifecycle_transaction::
			verify_initialized_runtime_sources(owner().connection.get(),
							   *cutover_owner_, selection_, request,
							   evidence, verify);
		// No retry may capture verifier effects after any failed cleanup/proof.
		// This flag gates verification only; the original retained abort remains.
		if (result)
			cutover_verification_abort_required_ = true;
		return result;
	}
	bool end_initialized_cut() noexcept
	{
		auto &runtime = owner();
		if (!nevent_is_game_thread() || runtime.process != getpid())
			return false;
		if (late_cleanup_ == late_cleanup_phase::complete)
			return true; // Completed ownership cleanup, never a fresh world permit.
		if (!runtime.connection || !runtime.authority ||
		    mysql_thread_id(runtime.connection.get()) != selection_.session_ ||
		    !runtime.authority->is_valid_authority())
			return false;
		if (late_cleanup_ <= late_cleanup_phase::outbox_resume &&
		    (!late_outbox_quiesced_ || critical_outbox_health_copy().accepting))
			return false;
		if (late_cleanup_ <= late_cleanup_phase::reservation_release &&
		    (!late_reserved_ || !critical_command_coordinator_owner::boot_recovery_ready()))
			return false;
		if (late_cleanup_ == late_cleanup_phase::held)
		{
			if (!writer_acquired_ || !writer_.valid())
				return false;
			late_cleanup_ = late_cleanup_phase::writer_release;
		}
		if (late_cleanup_ == late_cleanup_phase::writer_release)
		{
			// release() invalidates confirmed_ before fallible SQL. Retry its actual
			// retained obligation; do not require pre-cleanup publication validity.
			if (!writer_acquired_ || !writer_.release())
				return false;
			writer_acquired_ = false;
			late_cleanup_ = late_cleanup_phase::reservation_release;
		}
		if (late_cleanup_ == late_cleanup_phase::reservation_release)
		{
			if (writer_acquired_ || !critical_command_coordinator_owner::
							finish_initialized_lifecycle_reservation())
				return false;
			late_reserved_ = false;
			late_cleanup_ = late_cleanup_phase::outbox_resume;
		}
		if (late_cleanup_ == late_cleanup_phase::outbox_resume)
		{
			if (late_outbox_resume_)
				critical_outbox_resume();
			late_outbox_quiesced_ = false;
			late_outbox_resume_ = false;
			late_cleanup_ = late_cleanup_phase::saves_resume;
		}
		if (late_cleanup_ == late_cleanup_phase::saves_resume)
		{
			player_save_pipeline_resume();
			late_cleanup_ = late_cleanup_phase::complete;
		}
		return late_cleanup_ == late_cleanup_phase::complete;
	}
	bool shutdown() noexcept
	{
		if (owner().process && owner().process != getpid())
			return false; // No child protocol traffic through inherited owner.
		// A failed selected boot retains its genuine writer/local gate,
		// coordinator reservation and consumed world holders until the original
		// terminal process/world cleanup boundary. Do not release one lease
		// merely because the later reservation cleanup will refuse.
		if ((reserved_ && !admitted_) || late_reserved_ ||
		    cutover_ == cutover_phase::promoted_guard ||
		    cutover_ == cutover_phase::transaction ||
		    cutover_ == cutover_phase::returned_guard)
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
	enum class cutover_phase
	{
		runtime,
		promoted_guard,
		transaction,
		returned_guard,
		runtime_cleanup
	};
	cutover_phase cutover_ = cutover_phase::runtime;
	uint64_t late_cutover_save_epoch_ = 0;
	bool cutover_verification_abort_required_ = false;
	economic_sql_cutover_capability cutover_capability_;
	std::unique_ptr<economic_sql_cutover_transaction_owner> cutover_owner_;
	economic_sql_runtime_boot_selection selection_;
	economic_sql_runtime_world_writer_guard writer_;
	world_owner world_;
	std::vector<world_owner::held_native> held_;
	bool prepared_ = false, reserved_ = false, writer_acquired_ = false;
	bool late_reserved_ = false, late_outbox_quiesced_ = false, late_outbox_resume_ = false;
	enum class late_cleanup_phase
	{
		held,
		writer_release,
		reservation_release,
		outbox_resume,
		saves_resume,
		complete
	};
	late_cleanup_phase late_cleanup_ = late_cleanup_phase::held;
	bool world_prepared_ = false, promoted_ = false, admitted_ = false;
};

unsigned int economic_initialized_world_owner::sql_begin_cut() noexcept
{
	try
	{
		return sql_economic_runtime_boot_owner::instance().begin_initialized_cut();
	}
	catch (...)
	{
		return EIO;
	}
}
bool economic_initialized_world_owner::sql_cut_valid(const boot_capability &actual) noexcept
{
	try
	{
		return sql_economic_runtime_boot_owner::instance().initialized_cut_valid(actual);
	}
	catch (...)
	{
		return false;
	}
}
unsigned int economic_initialized_world_owner::sql_prepare_cutover() noexcept
{
	try
	{
		return sql_economic_runtime_boot_owner::instance().prepare_initialized_cutover();
	}
	catch (...)
	{
		return EIO;
	}
}
unsigned int economic_initialized_world_owner::sql_begin_cutover() noexcept
{
	try
	{
		return sql_economic_runtime_boot_owner::instance().begin_initialized_cutover();
	}
	catch (...)
	{
		return EIO;
	}
}
bool economic_initialized_world_owner::sql_abort_cutover() noexcept
{
	try
	{
		return sql_economic_runtime_boot_owner::instance().abort_initialized_cutover();
	}
	catch (...)
	{
		return false;
	}
}
bool economic_initialized_world_owner::sql_cutover_valid(
	economic_sql_cutover_transaction_owner &actual) noexcept
{
	try
	{
		return sql_economic_runtime_boot_owner::instance().initialized_cutover_valid(
			actual);
	}
	catch (...)
	{
		return false;
	}
}
unsigned int economic_initialized_world_owner::sql_verify_cutover_sources(
	const economic_sql_lifecycle_request &request,
	const economic_sql_activation_evidence &evidence,
	economic_sql_initialized_activation_verifier verify) noexcept
{
	try
	{
		return sql_economic_runtime_boot_owner::instance()
			.verify_initialized_cutover_sources(request, evidence, verify);
	}
	catch (...)
	{
		return EIO;
	}
}
bool economic_initialized_world_owner::sql_end_cut() noexcept
{
	try
	{
		return sql_economic_runtime_boot_owner::instance().end_initialized_cut();
	}
	catch (...)
	{
		return false;
	}
}
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
unsigned int economic_initialized_world_owner::sql_begin_cut() noexcept
{
	return ENOTSUP;
}
bool economic_initialized_world_owner::sql_cut_valid(const boot_capability &) noexcept
{
	return false;
}
unsigned int economic_initialized_world_owner::sql_prepare_cutover() noexcept
{
	return ENOTSUP;
}
unsigned int economic_initialized_world_owner::sql_begin_cutover() noexcept
{
	return ENOTSUP;
}
bool economic_initialized_world_owner::sql_abort_cutover() noexcept
{
	return false;
}
bool economic_initialized_world_owner::sql_cutover_valid(
	economic_sql_cutover_transaction_owner &) noexcept
{
	return false;
}
unsigned int economic_initialized_world_owner::sql_verify_cutover_sources(
	const economic_sql_lifecycle_request &, const economic_sql_activation_evidence &,
	economic_sql_initialized_activation_verifier) noexcept
{
	return ENOTSUP;
}
bool economic_initialized_world_owner::sql_end_cut() noexcept
{
	return false;
}
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
