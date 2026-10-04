#ifndef PLAYER_SAVE_EXECUTION_GUARD_H
#define PLAYER_SAVE_EXECUTION_GUARD_H

#include "persistence/critical_command.h"

#include <array>
#include <condition_variable>
#include <cstdint>
#include <limits>
#include <map>
#include <mutex>
#include <new>
#include <utility>

// Private leaf owner for ordinary save execution/checkpoint exclusion only.
// It does not prove a clean census, authorize critical ACK, or supply a wake.
// Pipeline registration owns the exact command bytes and commits its matching
// generation while holding pipeline_mutex. No callbacks or other owner locks
// may be acquired while this leaf mutex is held.
namespace player_save_execution_guard
{
enum class admission : uint8_t
{
	allowed,
	held,
	unavailable
};
namespace detail
{
struct held_pid
{
	int pid = 0;
	critical_operation_id operation = {};
	uint64_t generation = 0;
};
inline std::mutex mutex;
inline std::map<int, uint64_t> permits;
inline std::array<held_pid, 256> holds = {};
inline uint64_t next_generation = 0;
inline uint64_t active_permits = 0;
inline bool registration_open = false;
inline bool integrity_failed = false;
// This is an owner-memory bound, not a journal reader/admission record limit.
// It exceeds the number of distinct PIDs in a 256 MiB journal whose frame
// header alone exceeds 64 bytes, plus the runtime worker PID capacity.
inline constexpr size_t max_permitted_pids = 4 * 1024 * 1024 + 256;

// Replay ownership metadata enabled only by selected active SQL runtime boot.
// Source owners retain the exact bodies; these identities only fence execution.
struct owned_pid
{
	uint64_t claims = 0;
	uint64_t ticket = 0;
	bool reserved = false;
	uint64_t scope_owner = 0;
	bool replay_scope = false;
};
struct scope_authority
{
	int pid = 0;
	uint64_t epoch = 0, owner = 0;
	bool replay = false;
};
struct scope_link
{
	const scope_authority *entries = nullptr;
	size_t size = 0;
	scope_link *previous = nullptr;
};
inline std::map<int, owned_pid> owned_pids;
inline std::map<uint64_t, int> resident_claims;
inline uint64_t ownership_epoch = 0, next_ownership_epoch = 0;
inline uint64_t next_owner_generation = 0, ownership_change = 0;
// Construct the event only during explicit ownership preparation. The unused
// foundation introduces no new condition-variable initialization on inactive boot.
inline std::condition_variable &ownership_event()
{
	static std::condition_variable event;
	return event;
}
inline thread_local scope_link *current_scope = nullptr;
inline constexpr size_t max_resident_claims = 4096;

inline void changed_locked() noexcept
{
	if (ownership_change == std::numeric_limits<uint64_t>::max())
		integrity_failed = true;
	else
		++ownership_change;
	ownership_event().notify_all();
}
inline bool held_locked(int pid) noexcept
{
	for (const auto &hold : holds)
		if (hold.pid == pid)
			return true;
	return false;
}
inline bool authorized_locked(int pid) noexcept
{
	if (!ownership_epoch)
		return true;
	const auto found = owned_pids.find(pid);
	if (found == owned_pids.end() || !found->second.scope_owner)
		return false;
	// The top explicit scope alone supplies borrowing authority. A callback
	// cannot reach other PIDs through an outer checkpoint batch scope.
	if (current_scope)
		for (size_t i = 0; i < current_scope->size; ++i)
		{
			const auto &entry = current_scope->entries[i];
			if (entry.pid == pid && entry.epoch == ownership_epoch &&
			    entry.owner == found->second.scope_owner &&
			    entry.replay == found->second.replay_scope)
				return true;
		}
	return false;
}
}

class permit
{
    public:
	explicit permit(int pid) noexcept
		: pid_(pid)
	{
		if (pid <= 0)
			return;
		std::lock_guard<std::mutex> lock(detail::mutex);
		if (detail::integrity_failed ||
		    detail::active_permits == std::numeric_limits<uint64_t>::max())
			return;
		// An enabled ownership epoch requires an exact explicit scope. Ambient
		// thread identity and counted permits cannot borrow another owner's PID.
		if (!detail::authorized_locked(pid))
			return;
		for (const auto &hold : detail::holds)
			if (hold.pid == pid)
			{
				result_ = admission::held;
				return;
			}
		// Outside the prepared registration phase, no new hold can be installed.
		// Avoid allocation on the unchanged inactive/ordinary path, but count every
		// token so the next registration phase cannot overtake existing execution.
		if (!detail::registration_open && !detail::ownership_epoch)
		{
			++detail::active_permits;
			result_ = admission::allowed;
			return;
		}
		auto found = detail::permits.find(pid);
		if (found != detail::permits.end())
		{
			if (found->second == std::numeric_limits<uint64_t>::max())
				return;
			++found->second;
			++detail::active_permits;
			counted_pid_ = true;
			result_ = admission::allowed;
			return;
		}
		if (detail::permits.size() >= detail::max_permitted_pids)
			return;
		try
		{
			detail::permits.emplace(pid, 1);
			++detail::active_permits;
			counted_pid_ = true;
			result_ = admission::allowed;
		}
		catch (const std::bad_alloc &)
		{
		}
	}
	permit(const permit &) = delete;
	permit &operator=(const permit &) = delete;
	permit(permit &&other) noexcept
		: pid_(other.pid_)
		, result_(other.result_)
		, counted_pid_(other.counted_pid_)
	{
		other.result_ = admission::unavailable;
	}
	permit &operator=(permit &&) = delete;
	~permit() noexcept
	{
		if (result_ != admission::allowed)
			return;
		std::lock_guard<std::mutex> lock(detail::mutex);
		if (!detail::active_permits)
		{
			detail::integrity_failed = true;
			if (detail::ownership_epoch)
				detail::changed_locked();
			return;
		}
		--detail::active_permits;
		if (!counted_pid_)
			return;
		auto found = detail::permits.find(pid_);
		if (found == detail::permits.end() || !found->second)
		{
			detail::integrity_failed = true;
			if (detail::ownership_epoch)
				detail::changed_locked();
			return;
		}
		if (!--found->second)
			detail::permits.erase(found);
		if (detail::ownership_epoch)
			detail::changed_locked();
	}
	admission result() const noexcept { return result_; }
	explicit operator bool() const noexcept { return result_ == admission::allowed; }

    private:
	int pid_ = 0;
	admission result_ = admission::unavailable;
	bool counted_pid_ = false;
};

// Only the serial pipeline lifecycle owner may open/close registration.
// Opening refuses any native execution admitted before preparation.
inline bool begin_registration() noexcept
{
	std::lock_guard<std::mutex> lock(detail::mutex);
	if (detail::integrity_failed || detail::registration_open || detail::active_permits ||
	    detail::ownership_epoch)
		return false;
	detail::registration_open = true;
	return true;
}
inline void end_registration() noexcept
{
	std::lock_guard<std::mutex> lock(detail::mutex);
	detail::registration_open = false;
}

// Only the pipeline owner may call these hold mutations. Exact encoded command
// equality is checked in its authoritative slot before idempotent registration.
inline bool install_hold(int pid, const critical_operation_id &operation,
			 uint64_t *generation) noexcept
{
	bool nonzero = false;
	for (const auto byte : operation.bytes)
		nonzero = nonzero || byte != 0;
	if (pid <= 0 || !generation || !nonzero)
		return false;
	std::lock_guard<std::mutex> lock(detail::mutex);
	if (detail::integrity_failed || detail::permits.count(pid))
		return false;
	const auto owner = detail::owned_pids.find(pid);
	if (owner != detail::owned_pids.end() &&
	    (owner->second.reserved || owner->second.scope_owner))
		return false;
	detail::held_pid *available = nullptr;
	for (auto &hold : detail::holds)
	{
		if (hold.pid == pid || (hold.pid && hold.operation.bytes == operation.bytes))
		{
			if (hold.pid != pid || hold.operation.bytes != operation.bytes)
				return false;
			*generation = hold.generation;
			return true;
		}
		if (!hold.pid && !available)
			available = &hold;
	}
	if (!detail::registration_open || !available ||
	    detail::next_generation == std::numeric_limits<uint64_t>::max())
		return false;
	*available = { pid, operation, ++detail::next_generation };
	*generation = available->generation;
	return true;
}

inline void poison_integrity() noexcept
{
	std::lock_guard<std::mutex> lock(detail::mutex);
	detail::integrity_failed = true;
	if (detail::ownership_epoch)
		detail::changed_locked();
}

inline bool publication_operation_held(const critical_operation_id &operation) noexcept
{
	std::lock_guard<std::mutex> lock(detail::mutex);
	for (const auto &hold : detail::holds)
		if (hold.pid && hold.operation.bytes == operation.bytes)
			return true;
	return false;
}

// Caller clears its exact slot first under pipeline_mutex, then releases the
// matching generation here. A violated internal identity poisons admission.
inline bool release_hold(int pid, const critical_operation_id &operation,
			 uint64_t generation) noexcept
{
	std::lock_guard<std::mutex> lock(detail::mutex);
	for (auto &hold : detail::holds)
		if (hold.pid == pid && hold.operation.bytes == operation.bytes &&
		    hold.generation == generation && generation && !detail::permits.count(pid))
		{
			hold = {};
			if (detail::ownership_epoch)
				detail::changed_locked();
			return true;
		}
	detail::integrity_failed = true;
	if (detail::ownership_epoch)
		detail::changed_locked();
	return false;
}

// Shutdown may discard resident holds only after all execution owners drain.
// Counters/generations are never reset while a token could still refer to them.
inline bool discard_quiesced_holds() noexcept
{
	std::lock_guard<std::mutex> lock(detail::mutex);
	if (detail::active_permits || !detail::permits.empty() || detail::integrity_failed ||
	    detail::ownership_epoch)
		return false;
	detail::holds.fill({});
	detail::registration_open = false;
	return true;
}
}
#endif
