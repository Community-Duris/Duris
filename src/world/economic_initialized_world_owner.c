#include "world/economic_initialized_world_owner.h"
#include "core/prototypes.h"
#include "economy/economic_gameplay_authority.h"

#include <cerrno>
#include <memory>
#include <thread>
#include <utility>
#include <unistd.h>

struct economic_initialized_world_owner::implementation
{
	enum class phase
	{
		empty,
		historical,
		qualified,
		cutover_owned,
		cleanup_pending,
		invalidated
	};
	economic_initialized_world_snapshot snapshot;
	std::unique_ptr<boot_capability> capability;
	std::thread::id thread;
	int64_t process = 0;
	phase state = phase::empty;
	bool borrowed = false;
};
economic_initialized_world_owner::implementation &economic_initialized_world_owner::state()
{
	// Process-lifetime owner; failed selected boot retains original exclusions.
	static auto *value = new implementation;
	return *value;
}
bool economic_initialized_world_owner::complete_boot() noexcept
{
	try
	{
		auto &s = state();
		if (s.state != implementation::phase::empty || !nevent_is_game_thread())
			return false;
		if (economic_gameplay_authority::active_regular_sql())
			return !sql_begin_cut();
		// No qualified capability follows from an inactive or flatfile census.
		// Complete capture remains useful private evidence; a refusal changes no
		// inactive admission, production data, world objects or network behavior.
		economic_initialized_world_snapshot observed;
		if (!economic_initialized_world_snapshot_capture({}, &observed))
			s.snapshot = std::move(observed);
		s.process = getpid();
		s.thread = std::this_thread::get_id();
		s.state = implementation::phase::historical;
		return true;
	}
	catch (...)
	{
		return nevent_is_game_thread() &&
		       !economic_gameplay_authority::active_regular_sql();
	}
}
unsigned int
economic_initialized_world_owner::capture_sql_cut(const boot_capability &actual) noexcept
{
	try
	{
		auto &s = state();
		if (s.state != implementation::phase::empty || !nevent_is_game_thread() ||
		    !sql_cut_valid(actual))
			return EPERM;
		// Preallocate the exact original owner token before retaining source values.
		auto retained = std::unique_ptr<boot_capability>(new boot_capability(
			actual.connection_, actual.session_, actual.authority_id_,
			actual.save_epoch_, actual.process_, actual.runtime_, actual.writer_));
		economic_initialized_world_snapshot observed;
		const auto error = economic_initialized_world_snapshot_capture({}, &observed);
		if (error)
			return error;
		if (!sql_cut_valid(actual))
			return EPERM;
		s.snapshot = std::move(observed);
		s.capability = std::move(retained);
		s.process = getpid();
		s.thread = std::this_thread::get_id();
		s.state = implementation::phase::qualified;
		return 0;
	}
	catch (...)
	{
		return ENOMEM;
	}
}
bool economic_initialized_world_owner::with_boot_cut(consumer inspect, void *context) noexcept
{
	try
	{
		auto &s = state();
		if (!inspect || s.borrowed || s.state != implementation::phase::qualified ||
		    s.process != getpid() || s.thread != std::this_thread::get_id() ||
		    !nevent_is_game_thread() || !s.capability || !sql_cut_valid(*s.capability))
			return false;
		s.borrowed = true;
		const bool accepted = inspect(s.snapshot, *s.capability, context);
		s.borrowed = false;
		return accepted && s.state == implementation::phase::qualified && s.capability &&
		       sql_cut_valid(*s.capability);
	}
	catch (...)
	{
		return false;
	}
}
unsigned int economic_initialized_world_owner::begin_cutover() noexcept
{
	try
	{
		auto &s = state();
		if (s.borrowed || s.state != implementation::phase::qualified ||
		    s.process != getpid() || s.thread != std::this_thread::get_id() ||
		    !nevent_is_game_thread() || !s.capability || !sql_cut_valid(*s.capability))
			return EPERM;
		// Attach the actual transaction slot to its process-lifetime owner before
		// any resource transfer, and before destroying the one borrowed capability.
		const auto prepared = sql_prepare_cutover();
		if (prepared || !sql_cut_valid(*s.capability))
			return prepared ? prepared : EPERM;
		s.capability.reset();
		s.state = implementation::phase::cutover_owned;
		return sql_begin_cutover();
	}
	catch (...)
	{
		return EIO;
	}
}
bool economic_initialized_world_owner::abort_cutover() noexcept
{
	try
	{
		auto &s = state();
		if (s.borrowed || s.state != implementation::phase::cutover_owned || s.capability ||
		    s.process != getpid() || s.thread != std::this_thread::get_id() ||
		    !nevent_is_game_thread())
			return false;
		// The SQL owner repeats fresh projection authentication on every return
		// retry, retaining its original locks/session through any refusal.
		if (!sql_abort_cutover())
			return false;
		s.state = implementation::phase::invalidated;
		return true;
	}
	catch (...)
	{
		return false;
	}
}
bool economic_initialized_world_owner::with_cutover_cut(
	economic_sql_cutover_transaction_owner &actual, cutover_consumer inspect,
	void *context) noexcept
{
	try
	{
		auto &s = state();
		if (!inspect || s.borrowed || s.state != implementation::phase::cutover_owned ||
		    s.capability || s.process != getpid() ||
		    s.thread != std::this_thread::get_id() || !nevent_is_game_thread() ||
		    !sql_cutover_valid(actual))
			return false;
		struct borrow_scope
		{
			bool &borrowed;
			~borrow_scope() noexcept { borrowed = false; }
		} borrowing{ s.borrowed };
		s.borrowed = true;
		const bool accepted = inspect(s.snapshot, context);
		s.borrowed = false;
		return accepted && s.state == implementation::phase::cutover_owned &&
		       !s.capability && sql_cutover_valid(actual);
	}
	catch (...)
	{
		return false;
	}
}
unsigned int economic_initialized_world_owner::verify_cutover_sources(
	const economic_sql_lifecycle_request &request,
	const economic_sql_activation_evidence &evidence,
	economic_sql_initialized_activation_verifier verify) noexcept
{
	try
	{
		auto &s = state();
		if (s.borrowed || s.state != implementation::phase::cutover_owned || s.capability ||
		    s.process != getpid() || s.thread != std::this_thread::get_id() ||
		    !nevent_is_game_thread())
			return EPERM;
		return sql_verify_cutover_sources(request, evidence, verify);
	}
	catch (...)
	{
		return EIO;
	}
}
bool economic_initialized_world_owner::before_world_callbacks() noexcept
{
	try
	{
		auto &s = state();
		// An inactive best-effort capture may fail before owner allocation/retention.
		// That absence confers no authority and must not reject ordinary startup.
		if (s.state == implementation::phase::empty && nevent_is_game_thread() &&
		    !economic_gameplay_authority::active_regular_sql())
			return true;
		if (s.state == implementation::phase::cutover_owned)
			return abort_cutover();
		if ((s.state != implementation::phase::historical &&
		     s.state != implementation::phase::qualified &&
		     s.state != implementation::phase::cleanup_pending &&
		     s.state != implementation::phase::invalidated) ||
		    s.borrowed || s.process != getpid() || s.thread != std::this_thread::get_id() ||
		    !nevent_is_game_thread())
			return false;
		if (s.state == implementation::phase::invalidated)
			return true; // Idempotent completed cleanup, with no capability reissue.
		const bool selected = s.state == implementation::phase::qualified ||
				      s.state == implementation::phase::cleanup_pending;
		// Revoke exactly once before any lease release/callback. Failed cleanup
		// retains its real obligation but can never reissue the borrowed token.
		s.state = selected ? implementation::phase::cleanup_pending :
				     implementation::phase::invalidated;
		s.capability.reset();
		if (selected && !sql_end_cut())
			return false;
		s.state = implementation::phase::invalidated;
		return true;
	}
	catch (...)
	{
		// No SQL cut can precede failure to create the process-lifetime owner.
		return nevent_is_game_thread() &&
		       !economic_gameplay_authority::active_regular_sql();
	}
}
