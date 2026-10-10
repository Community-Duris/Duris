#include "flatfile/flatfile_initialized_world_owner.h"
#include "core/prototypes.h"
#include "economy/economic_gameplay_authority.h"
#include "persistence/critical_command_coordinator.h"
#include "persistence/persistence_mode.h"
#include "player/player_save_pipeline.h"
#include "player/player_save_replay_ownership.h"

#include <cerrno>
#include <cstring>
#include <memory>
#include <new>
#include <thread>
#include <utility>
#include <unistd.h>
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
#include <bits/cxxabi_tweaks.h>
#endif

struct flatfile_initialized_world_owner::implementation
{
	enum class phase
	{
		empty,
		preparing,
		qualified,
		cleanup_pending,
		invalidated
	};
	phase status = phase::empty;
	economic_initialized_world_snapshot snapshot;
	std::string root;
	std::unique_ptr<flatfile_identity_lock> identity;
	std::unique_ptr<flatfile_authority_lock> authority;
	std::thread::id thread;
	int64_t process = 0;
	uint64_t save_epoch = 0;
	size_t world_heap = 0;
	bool reserved = false, borrowed = false, save_drain_started = false;
};

flatfile_initialized_world_owner::implementation &flatfile_initialized_world_owner::state() noexcept
{
	// Empty strings/vectors and null unique_ptrs make this owner allocation-free.
	// Failed cleanup is retained for process lifetime; no destructor releases a
	// genuine acquired lock before the explicit terminal callback boundary.
	static implementation value;
	return value;
}

bool flatfile_initialized_world_owner::retained_bytes(size_t *output) noexcept
{
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI || defined(_GLIBCXX_DEBUG)
	return false;
#else
	if (!output)
		return false;
	auto &s = state();
	size_t bytes = sizeof(implementation) + sizeof(__cxxabiv1::__guard);
	const auto add = [&bytes](size_t amount) noexcept
	{
		if (amount > SIZE_MAX - bytes)
			return false;
		bytes += amount;
		return true;
	};
	if (s.root.capacity() > 15 &&
	    (s.root.capacity() == SIZE_MAX || !add(s.root.capacity() + 1)))
		return false;
	size_t lock_bytes = 0;
	if ((s.identity && (!s.identity->retained_bytes(&lock_bytes) || !add(lock_bytes))) ||
	    (s.authority && (!s.authority->retained_bytes(&lock_bytes) || !add(lock_bytes))) ||
	    !add(s.world_heap))
		return false;
	*output = bytes;
	return true;
#endif
}

bool flatfile_initialized_world_owner::held(const implementation &s) noexcept
{
	try
	{
		const char *configured = persistence_mode_flatfile_root();
		return s.reserved && s.process == getpid() &&
		       s.thread == std::this_thread::get_id() && nevent_is_game_thread() &&
		       !persistence_mode_requires_mysql() &&
		       economic_gameplay_authority::active_regular_flat() && configured &&
		       s.root == configured && s.save_epoch &&
		       s.save_epoch == player_save_execution_guard::current_ownership_epoch() &&
		       player_save_execution_guard::ownership_epoch_quiescent(s.save_epoch) &&
		       critical_command_coordinator_owner::boot_recovery_ready() && s.identity &&
		       s.identity->matches(s.root) && s.authority && s.authority->matches(s.root);
	}
	catch (...)
	{
		return false;
	}
}

unsigned int flatfile_initialized_world_owner::begin_initialized_cut(
	const std::string &root, flatfile_scratch_reserve_fn reserve, void *context,
	size_t outer_live_scratch) noexcept
{
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI || defined(_GLIBCXX_DEBUG)
	return ENOTSUP;
#else
	// An existing owner retains its admitted root, world and exclusion locks.
	// Reject before ANY fresh callback can replace that absolute reservation.
	auto &s = state();
	if (s.status != implementation::phase::empty)
		return EPERM;
	// Charge the actual inline workspace BEFORE construction and retain it
	// across nested providers. Their explicit storage is additional to outer.
	struct workspace
	{
		const char *configured = nullptr;
		uint64_t epoch = 0;
		size_t live = 0, retained = 0, world_heap = 0;
		unsigned int error = 0;
		economic_sql_source_limits limits{};
	};
	constexpr size_t initial =
		sizeof(implementation) + sizeof(__cxxabiv1::__guard) + sizeof(workspace);
	if (!reserve || outer_live_scratch > SIZE_MAX - initial ||
	    !reserve(outer_live_scratch + initial, context))
		return ENOBUFS;
	workspace work;
	work.configured = persistence_mode_flatfile_root();
	work.epoch = player_save_execution_guard::current_ownership_epoch();
	if (!nevent_is_game_thread() || persistence_mode_requires_mysql() ||
	    !economic_gameplay_authority::active_regular_flat() || !work.configured ||
	    root.empty() || root != work.configured || !work.epoch)
		return EPERM;
	try
	{
		s.process = getpid();
		s.thread = std::this_thread::get_id();
		s.save_epoch = work.epoch;
		if (!critical_command_coordinator_owner::acquire_initialized_lifecycle_guard())
			return EBUSY;
		s.reserved = true;
		s.status = implementation::phase::preparing;
		// Fresh length construction has the installed GCC13 exact-length request.
		// Reservation ownership is already retained if this admission fails.
		work.live = outer_live_scratch + initial;
		if (sizeof(std::string) > SIZE_MAX - work.live)
			return ENOBUFS;
		work.live += sizeof(std::string);
		if (root.size() > 15 &&
		    (root.size() == SIZE_MAX || root.size() + 1 > SIZE_MAX - work.live))
			return ENOBUFS;
		if (!reserve(work.live + (root.size() > 15 ? root.size() + 1 : 0), context))
			return ENOBUFS;
		{
			std::string owned(root.data(), root.size());
			s.root = std::move(owned);
		}
		// Original drain pumps real accepted completion/replay before exclusion
		// locks, never while holding identity or authority over hydration.
		s.save_drain_started = true;
		if (!player_save_pipeline_drain_owned(3000) ||
		    s.save_epoch != player_save_execution_guard::current_ownership_epoch() ||
		    !player_save_execution_guard::ownership_epoch_quiescent(s.save_epoch) ||
		    !critical_command_coordinator_owner::boot_recovery_ready())
			return EBUSY;
		if (!retained_bytes(&work.retained) ||
		    work.retained > SIZE_MAX - outer_live_scratch - sizeof(workspace))
			return ENOBUFS;
		work.live = outer_live_scratch + sizeof(workspace) + work.retained;
		if (sizeof(flatfile_identity_lock) > SIZE_MAX - work.live ||
		    !reserve(work.live + sizeof(flatfile_identity_lock), context))
			return ENOBUFS;
		s.identity.reset(new flatfile_identity_lock(reserve, context, work.live));
		if (!retained_bytes(&work.retained))
			return errno ? errno : ENOMEM;
		if (work.retained > SIZE_MAX - outer_live_scratch - sizeof(workspace))
			return ENOBUFS;
		work.live = outer_live_scratch + sizeof(workspace) + work.retained;
		if (!s.identity->acquire_bounded(s.root, reserve, context, work.live))
			return errno ? errno : EIO;
		if (!retained_bytes(&work.retained) ||
		    work.retained > SIZE_MAX - outer_live_scratch - sizeof(workspace))
			return ENOBUFS;
		work.live = outer_live_scratch + sizeof(workspace) + work.retained;
		if (sizeof(flatfile_authority_lock) > SIZE_MAX - work.live ||
		    !reserve(work.live + sizeof(flatfile_authority_lock), context))
			return ENOBUFS;
		s.authority.reset(new flatfile_authority_lock(reserve, context, work.live));
		if (!retained_bytes(&work.retained))
			return errno ? errno : ENOMEM;
		if (work.retained > SIZE_MAX - outer_live_scratch - sizeof(workspace))
			return ENOBUFS;
		work.live = outer_live_scratch + sizeof(workspace) + work.retained;
		if (!s.authority->acquire_bounded(s.root, reserve, context, work.live))
			return errno ? errno : EIO;
		if (!held(s) || !retained_bytes(&work.retained))
			return EPERM;
		if (work.retained > SIZE_MAX - outer_live_scratch - sizeof(workspace))
			return ENOBUFS;
		work.live = outer_live_scratch + sizeof(workspace) + work.retained;
		work.error = economic_initialized_world_snapshot_capture_bounded(
			work.limits, &s.snapshot, reserve, context, work.live, &work.world_heap);
		if (work.error)
			return work.error;
		s.world_heap = work.world_heap;
		if (!held(s))
			return EPERM;
		s.status = implementation::phase::qualified;
		return 0;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
	catch (...)
	{
		return EIO;
	}
#endif
}

bool flatfile_initialized_world_owner::with_boot_cut(consumer inspect, void *context) noexcept
{
	try
	{
		auto &s = state();
		if (!inspect || s.borrowed || s.status != implementation::phase::qualified ||
		    !held(s))
			return false;
		struct borrow_scope
		{
			bool &borrowed;
			~borrow_scope() noexcept { borrowed = false; }
		};
		borrow_scope borrowing{ s.borrowed };
		s.borrowed = true;
		const bool accepted =
			inspect(s.snapshot, s.root, *s.identity, *s.authority, context);
		s.borrowed = false;
		return accepted && s.status == implementation::phase::qualified && held(s);
	}
	catch (...)
	{
		return false;
	}
}

bool flatfile_initialized_world_owner::before_world_callbacks() noexcept
{
	try
	{
		auto &s = state();
		if (s.status == implementation::phase::empty && !s.reserved)
			return nevent_is_game_thread();
		if (s.borrowed || s.process != getpid() || s.thread != std::this_thread::get_id() ||
		    !nevent_is_game_thread())
			return false;
		if (s.status == implementation::phase::invalidated)
			return true;
		// Preparing failures have the same real cleanup obligation. A partial
		// capture or invalid epoch can never produce a fresh qualified capability.
		s.status = implementation::phase::cleanup_pending;
		if (!s.reserved || !s.save_epoch ||
		    s.save_epoch != player_save_execution_guard::current_ownership_epoch() ||
		    !player_save_execution_guard::ownership_epoch_quiescent(s.save_epoch) ||
		    !critical_command_coordinator_owner::boot_recovery_ready() ||
		    !critical_command_coordinator_owner::finish_initialized_lifecycle_reservation())
			return false;
		s.reserved = false;
		// The only fallible owner release has passed. Original lock destructors
		// release in reverse order; never unlock/reacquire a failed cleanup owner.
		s.authority.reset();
		s.identity.reset();
		if (s.save_drain_started)
			player_save_pipeline_resume();
		s.status = implementation::phase::invalidated;
		return true;
	}
	catch (...)
	{
		return false;
	}
}
