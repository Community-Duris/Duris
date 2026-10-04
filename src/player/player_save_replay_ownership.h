#ifndef PLAYER_SAVE_REPLAY_OWNERSHIP_H
#define PLAYER_SAVE_REPLAY_OWNERSHIP_H

#include "player/player_save_execution_guard.h"

#include <algorithm>
#include <chrono>
#include <span>
#include <system_error>
#include <vector>

class player_save_restored_publication_owner;

// Private opt-in leaf. Exact snapshot/receipt/namespace evidence stays with its
// source owner. This supplies neither a complete mutation census nor critical ACK
// authority. Production must not enable it until every residence/transfer and
// mutation entry point participates. In particular, it does not revisit replay.
namespace player_save_execution_guard
{
enum class ownership_status : uint8_t
{
	allowed,
	busy,
	held,
	unavailable,
	invalid_epoch
};
namespace detail
{
inline bool valid_epoch_locked(uint64_t epoch) noexcept
{
	return epoch && epoch == ownership_epoch && !integrity_failed;
}
inline void erase_idle_locked(int pid) noexcept
{
	const auto found = owned_pids.find(pid);
	if (found != owned_pids.end() && !found->second.claims && !found->second.ticket &&
	    !found->second.scope_owner)
		owned_pids.erase(found);
}
inline bool generation_available_locked() noexcept
{
	return next_owner_generation != std::numeric_limits<uint64_t>::max() &&
	       ownership_change != std::numeric_limits<uint64_t>::max();
}
inline bool scope_ready_locked(const scope_authority &authority) noexcept
{
	const auto found = owned_pids.find(authority.pid);
	if (!valid_epoch_locked(authority.epoch) || found == owned_pids.end() ||
	    found->second.scope_owner || permits.count(authority.pid) || held_locked(authority.pid))
		return false;
	if (authority.replay)
		return found->second.reserved && found->second.ticket == authority.owner &&
		       !found->second.claims;
	const auto claim = resident_claims.find(authority.owner);
	return !found->second.reserved && claim != resident_claims.end() &&
	       claim->second == authority.pid;
}
inline void enter_scope_locked(scope_link &link) noexcept
{
	for (size_t i = 0; i < link.size; ++i)
	{
		const auto &authority = link.entries[i];
		auto &state = owned_pids.find(authority.pid)->second;
		state.scope_owner = authority.owner;
		state.replay_scope = authority.replay;
	}
	link.previous = current_scope;
	current_scope = &link;
	changed_locked();
}
inline void leave_scope_locked(scope_link &link) noexcept
{
	// Scopes cannot move, cross threads, or outlive their nested permit borrows.
	// An invalid lifetime poisons admission and retains unresolved metadata.
	if (current_scope != &link)
	{
		integrity_failed = true;
		current_scope = nullptr;
		changed_locked();
		return;
	}
	current_scope = link.previous;
	for (size_t i = 0; i < link.size; ++i)
	{
		const auto &authority = link.entries[i];
		const auto found = owned_pids.find(authority.pid);
		if (!valid_epoch_locked(authority.epoch) || found == owned_pids.end() ||
		    found->second.scope_owner != authority.owner || permits.count(authority.pid))
		{
			integrity_failed = true;
			continue;
		}
		found->second.scope_owner = 0;
		found->second.replay_scope = false;
		erase_idle_locked(authority.pid);
	}
	changed_locked();
}
}

// Serial lifecycle owner only. Ending hold registration does not end tracking.
// An unsuccessful close retains the same epoch for an explicit shutdown retry.
inline bool begin_ownership_epoch(uint64_t *out) noexcept
{
	if (!out)
		return false;
	std::lock_guard<std::mutex> lock(detail::mutex);
	if (detail::integrity_failed || detail::ownership_epoch || detail::active_permits ||
	    !detail::permits.empty() || !detail::owned_pids.empty() ||
	    !detail::resident_claims.empty() ||
	    detail::next_ownership_epoch == std::numeric_limits<uint64_t>::max() ||
	    !detail::generation_available_locked())
		return false;
	try
	{
		(void)detail::ownership_event();
	}
	catch (const std::system_error &)
	{
		return false;
	}
	detail::ownership_epoch = ++detail::next_ownership_epoch;
	*out = detail::ownership_epoch;
	detail::changed_locked();
	return true;
}
inline bool end_ownership_epoch(uint64_t epoch) noexcept
{
	std::lock_guard<std::mutex> lock(detail::mutex);
	if (!detail::valid_epoch_locked(epoch) || detail::active_permits ||
	    !detail::permits.empty() || !detail::owned_pids.empty() ||
	    !detail::resident_claims.empty())
		return false;
	for (const auto &hold : detail::holds)
		if (hold.pid)
			return false;
	detail::ownership_epoch = 0;
	detail::changed_locked();
	return true;
}

// Return the actual epoch even after integrity refusal; callers must not turn a
// poisoned enabled epoch into an inactive/unowned admission bypass.
inline uint64_t current_ownership_epoch() noexcept
{
	std::lock_guard<std::mutex> lock(detail::mutex);
	return detail::ownership_epoch;
}

class resident_claim
{
    public:
	resident_claim() noexcept = default;
	resident_claim(uint64_t epoch, int pid) noexcept
	{
		std::lock_guard<std::mutex> lock(detail::mutex);
		if (!detail::valid_epoch_locked(epoch) || pid <= 0)
			return;
		// A restored publication owns this PID until its original ACK. A new
		// body cannot drain while held and must not become a permanent resident.
		if (detail::held_locked(pid))
		{
			status_ = ownership_status::held;
			return;
		}
		const auto found = detail::owned_pids.find(pid);
		if (found != detail::owned_pids.end() && found->second.ticket)
		{
			status_ = ownership_status::busy;
			return;
		}
		status_ = ownership_status::unavailable;
		if (detail::resident_claims.size() >= detail::max_resident_claims ||
		    (found == detail::owned_pids.end() &&
		     detail::owned_pids.size() >= detail::max_permitted_pids) ||
		    !detail::generation_available_locked())
			return;
		try
		{
			auto state = detail::owned_pids.try_emplace(pid).first;
			const uint64_t generation = detail::next_owner_generation + 1;
			detail::resident_claims.emplace(generation, pid);
			++state->second.claims;
			detail::next_owner_generation = generation;
			epoch_ = epoch;
			pid_ = pid;
			generation_ = generation;
			status_ = ownership_status::allowed;
			detail::changed_locked();
		}
		catch (const std::bad_alloc &)
		{
			detail::erase_idle_locked(pid);
		}
	}
	resident_claim(const resident_claim &) = delete;
	resident_claim &operator=(const resident_claim &) = delete;
	resident_claim(resident_claim &&other) noexcept { take(other); }
	resident_claim &operator=(resident_claim &&other) noexcept
	{
		if (this != &other)
		{
			release();
			take(other);
		}
		return *this;
	}
	~resident_claim() noexcept { release(); }
	ownership_status result() const noexcept { return status_; }
	explicit operator bool() const noexcept { return generation_ != 0; }
	bool matches_current(int pid) const noexcept
	{
		std::lock_guard<std::mutex> lock(detail::mutex);
		const auto found = detail::resident_claims.find(generation_);
		return pid > 0 && pid == pid_ && detail::valid_epoch_locked(epoch_) &&
		       generation_ && found != detail::resident_claims.end() &&
		       found->second == pid;
	}
	// Only an exact retained immutable body may derive another delivery owner.
	// The source owner checks those bytes; this is not admission for a new body.
	// Existing pinned retries must drain even after a replay ticket becomes pending.
	static resident_claim derive_existing(const resident_claim &original) noexcept
	{
		resident_claim out;
		std::lock_guard<std::mutex> lock(detail::mutex);
		const auto claim = detail::resident_claims.find(original.generation_);
		const auto state = detail::owned_pids.find(original.pid_);
		if (!detail::valid_epoch_locked(original.epoch_) || !original.generation_ ||
		    claim == detail::resident_claims.end() || claim->second != original.pid_ ||
		    state == detail::owned_pids.end() || !state->second.claims ||
		    state->second.reserved)
			return out;
		out.status_ = ownership_status::unavailable;
		if (detail::resident_claims.size() >= detail::max_resident_claims ||
		    !detail::generation_available_locked())
			return out;
		try
		{
			out.generation_ = detail::next_owner_generation + 1;
			detail::resident_claims.emplace(out.generation_, original.pid_);
			detail::next_owner_generation = out.generation_;
			++state->second.claims;
			out.epoch_ = original.epoch_;
			out.pid_ = original.pid_;
			out.status_ = ownership_status::allowed;
			detail::changed_locked();
		}
		catch (const std::bad_alloc &)
		{
			out.generation_ = 0;
		}
		return out;
	}

    private:
	friend class execution_scope;
	uint64_t epoch_ = 0, generation_ = 0;
	int pid_ = 0;
	ownership_status status_ = ownership_status::invalid_epoch;
	void take(resident_claim &other) noexcept
	{
		epoch_ = std::exchange(other.epoch_, 0);
		generation_ = std::exchange(other.generation_, 0);
		pid_ = std::exchange(other.pid_, 0);
		status_ = std::exchange(other.status_, ownership_status::invalid_epoch);
	}
	void release() noexcept
	{
		if (!generation_)
			return;
		std::lock_guard<std::mutex> lock(detail::mutex);
		const auto claim = detail::resident_claims.find(generation_);
		const auto state = detail::owned_pids.find(pid_);
		if (!detail::valid_epoch_locked(epoch_) || claim == detail::resident_claims.end() ||
		    claim->second != pid_ || state == detail::owned_pids.end() ||
		    !state->second.claims || state->second.scope_owner == generation_)
			detail::integrity_failed = true;
		else
		{
			detail::resident_claims.erase(claim);
			--state->second.claims;
			detail::erase_idle_locked(pid_);
		}
		generation_ = epoch_ = 0;
		pid_ = 0;
		detail::changed_locked();
	}
};

// Publication reserves exclusion only. It deliberately supplies no authority
// accepted by execution_scope or permit, and never temporarily removes a hold.
class held_publication_reservation
{
    public:
	held_publication_reservation(uint64_t epoch, int pid,
				     const critical_operation_id &operation,
				     uint64_t hold_generation) noexcept
	{
		std::lock_guard<std::mutex> lock(detail::mutex);
		if (!detail::valid_epoch_locked(epoch) || pid <= 0 || !hold_generation)
			return;
		bool exact_hold = false;
		for (const auto &hold : detail::holds)
			exact_hold = exact_hold ||
				     (hold.pid == pid && hold.operation.bytes == operation.bytes &&
				      hold.generation == hold_generation);
		if (!exact_hold)
			return;
		const auto found = detail::owned_pids.find(pid);
		status_ = ownership_status::busy;
		if ((found != detail::owned_pids.end() &&
		     (found->second.claims || found->second.ticket || found->second.reserved ||
		      found->second.scope_owner)) ||
		    detail::permits.count(pid))
			return;
		status_ = ownership_status::unavailable;
		if ((found == detail::owned_pids.end() &&
		     detail::owned_pids.size() >= detail::max_permitted_pids) ||
		    !detail::generation_available_locked())
			return;
		try
		{
			auto state = detail::owned_pids.try_emplace(pid).first;
			generation_ = ++detail::next_owner_generation;
			state->second.ticket = generation_;
			state->second.reserved = true;
			epoch_ = epoch;
			pid_ = pid;
			operation_ = operation;
			hold_generation_ = hold_generation;
			status_ = ownership_status::allowed;
			detail::changed_locked();
		}
		catch (const std::bad_alloc &)
		{
		}
	}
	held_publication_reservation(const held_publication_reservation &) = delete;
	held_publication_reservation &operator=(const held_publication_reservation &) = delete;
	~held_publication_reservation() noexcept
	{
		if (!generation_)
			return;
		std::lock_guard<std::mutex> lock(detail::mutex);
		if (!matches_locked())
			detail::integrity_failed = true;
		else
		{
			auto &state = detail::owned_pids.find(pid_)->second;
			state.ticket = 0;
			state.reserved = false;
			detail::erase_idle_locked(pid_);
		}
		detail::changed_locked();
	}
	ownership_status result() const noexcept { return status_; }
	bool valid() const noexcept
	{
		std::lock_guard<std::mutex> lock(detail::mutex);
		return !detail::integrity_failed && matches_locked();
	}
	bool matches_pid(int pid) const noexcept { return pid > 0 && pid == pid_ && valid(); }

    private:
	friend class ::player_save_restored_publication_owner;
	// The pipeline consumes this only after its guarded coordinator ACK. Clear
	// exclusion and the exact hold together; another PID's integrity refusal
	// cannot invalidate an already durable ACK's otherwise exact local cleanup.
	bool consume_acknowledged_hold() noexcept
	{
		std::lock_guard<std::mutex> lock(detail::mutex);
		if (!matches_locked())
			return false;
		for (auto &hold : detail::holds)
			if (hold.pid == pid_ && hold.generation == hold_generation_ &&
			    hold.operation.bytes == operation_.bytes)
			{
				hold = {};
				auto &state = detail::owned_pids.find(pid_)->second;
				state.ticket = 0;
				state.reserved = false;
				detail::erase_idle_locked(pid_);
				generation_ = 0;
				detail::changed_locked();
				return true;
			}
		return false;
	}

	uint64_t epoch_ = 0, generation_ = 0, hold_generation_ = 0;
	int pid_ = 0;
	critical_operation_id operation_ = {};
	ownership_status status_ = ownership_status::invalid_epoch;
	bool matches_locked() const noexcept
	{
		const auto found = detail::owned_pids.find(pid_);
		if (!epoch_ || epoch_ != detail::ownership_epoch || !generation_ ||
		    found == detail::owned_pids.end() || found->second.ticket != generation_ ||
		    !found->second.reserved || found->second.claims || found->second.scope_owner ||
		    detail::permits.count(pid_))
			return false;
		for (const auto &hold : detail::holds)
			if (hold.pid == pid_ && hold.generation == hold_generation_ &&
			    hold.operation.bytes == operation_.bytes)
				return true;
		return false;
	}
};

class replay_reservation;
class replay_ticket
{
    public:
	replay_ticket(uint64_t epoch, int pid) noexcept
	{
		std::lock_guard<std::mutex> lock(detail::mutex);
		if (!detail::valid_epoch_locked(epoch) || pid <= 0)
			return;
		const auto found = detail::owned_pids.find(pid);
		if (found != detail::owned_pids.end() && found->second.ticket)
		{
			status_ = ownership_status::busy;
			return;
		}
		status_ = ownership_status::unavailable;
		if ((found == detail::owned_pids.end() &&
		     detail::owned_pids.size() >= detail::max_permitted_pids) ||
		    !detail::generation_available_locked())
			return;
		try
		{
			auto state = detail::owned_pids.try_emplace(pid).first;
			generation_ = ++detail::next_owner_generation;
			state->second.ticket = generation_;
			epoch_ = epoch;
			pid_ = pid;
			status_ = ownership_status::allowed;
			detail::changed_locked();
		}
		catch (const std::bad_alloc &)
		{
		}
	}
	replay_ticket(const replay_ticket &) = delete;
	replay_ticket &operator=(const replay_ticket &) = delete;
	replay_ticket(replay_ticket &&other) noexcept
		: epoch_(std::exchange(other.epoch_, 0))
		, generation_(std::exchange(other.generation_, 0))
		, pid_(std::exchange(other.pid_, 0))
		, status_(std::exchange(other.status_, ownership_status::invalid_epoch))
	{
	}
	replay_ticket &operator=(replay_ticket &&) = delete;
	~replay_ticket() noexcept
	{
		if (!generation_)
			return;
		std::lock_guard<std::mutex> lock(detail::mutex);
		const auto found = detail::owned_pids.find(pid_);
		if (!detail::valid_epoch_locked(epoch_) || found == detail::owned_pids.end() ||
		    found->second.ticket != generation_ || found->second.reserved)
			detail::integrity_failed = true;
		else
		{
			found->second.ticket = 0;
			detail::erase_idle_locked(pid_);
		}
		detail::changed_locked();
	}
	ownership_status result() const noexcept { return status_; }

    private:
	friend class replay_reservation;
	uint64_t epoch_ = 0, generation_ = 0;
	int pid_ = 0;
	ownership_status status_ = ownership_status::invalid_epoch;
};

class replay_reservation
{
    public:
	// On busy/held/refusal the original ticket remains pending and retryable.
	explicit replay_reservation(replay_ticket &ticket) noexcept
	{
		std::lock_guard<std::mutex> lock(detail::mutex);
		const auto found = detail::owned_pids.find(ticket.pid_);
		if (!detail::valid_epoch_locked(ticket.epoch_) || !ticket.generation_ ||
		    found == detail::owned_pids.end() ||
		    found->second.ticket != ticket.generation_ || found->second.reserved)
			return;
		if (detail::held_locked(ticket.pid_))
		{
			status_ = ownership_status::held;
			return;
		}
		if (found->second.claims || found->second.scope_owner ||
		    detail::permits.count(ticket.pid_))
		{
			status_ = ownership_status::busy;
			return;
		}
		if (!detail::generation_available_locked())
		{
			status_ = ownership_status::unavailable;
			return;
		}
		found->second.reserved = true;
		epoch_ = std::exchange(ticket.epoch_, 0);
		generation_ = std::exchange(ticket.generation_, 0);
		pid_ = std::exchange(ticket.pid_, 0);
		ticket.status_ = ownership_status::invalid_epoch;
		status_ = ownership_status::allowed;
		detail::changed_locked();
	}
	replay_reservation(const replay_reservation &) = delete;
	replay_reservation &operator=(const replay_reservation &) = delete;
	replay_reservation(replay_reservation &&other) noexcept
		: epoch_(std::exchange(other.epoch_, 0))
		, generation_(std::exchange(other.generation_, 0))
		, pid_(std::exchange(other.pid_, 0))
		, status_(std::exchange(other.status_, ownership_status::invalid_epoch))
	{
	}
	replay_reservation &operator=(replay_reservation &&) = delete;
	~replay_reservation() noexcept
	{
		if (!generation_)
			return;
		std::lock_guard<std::mutex> lock(detail::mutex);
		const auto found = detail::owned_pids.find(pid_);
		if (!detail::valid_epoch_locked(epoch_) || found == detail::owned_pids.end() ||
		    found->second.ticket != generation_ || !found->second.reserved ||
		    found->second.scope_owner || detail::permits.count(pid_))
			detail::integrity_failed = true;
		else
		{
			found->second.ticket = 0;
			found->second.reserved = false;
			detail::erase_idle_locked(pid_);
		}
		detail::changed_locked();
	}
	ownership_status result() const noexcept { return status_; }

    private:
	friend class execution_scope;
	friend class replay_checkpoint_scope;
	uint64_t epoch_ = 0, generation_ = 0;
	int pid_ = 0;
	ownership_status status_ = ownership_status::invalid_epoch;
};

// A stable, thread-local borrowing scope. It is deliberately nonmovable. The
// owning claim/reservation outlives both the scope and all nested permit borrows.
class execution_scope
{
    public:
	explicit execution_scope(const resident_claim &claim) noexcept
		: authority_{ claim.pid_, claim.epoch_, claim.generation_, false }
	{
		enter();
	}
	explicit execution_scope(const replay_reservation &reservation) noexcept
		: authority_{ reservation.pid_, reservation.epoch_, reservation.generation_, true }
	{
		enter();
	}
	execution_scope(const execution_scope &) = delete;
	execution_scope &operator=(const execution_scope &) = delete;
	~execution_scope() noexcept
	{
		if (status_ == ownership_status::allowed)
		{
			std::lock_guard<std::mutex> lock(detail::mutex);
			detail::leave_scope_locked(link_);
		}
	}
	ownership_status result() const noexcept { return status_; }

    private:
	detail::scope_authority authority_;
	detail::scope_link link_ = {};
	ownership_status status_ = ownership_status::invalid_epoch;
	void enter() noexcept
	{
		std::lock_guard<std::mutex> lock(detail::mutex);
		if (!detail::valid_epoch_locked(authority_.epoch))
			return;
		status_ = detail::held_locked(authority_.pid) ? ownership_status::held :
								ownership_status::busy;
		if (!detail::scope_ready_locked(authority_))
			return;
		if (!detail::generation_available_locked())
		{
			status_ = ownership_status::unavailable;
			return;
		}
		link_.entries = &authority_;
		link_.size = 1;
		detail::enter_scope_locked(link_);
		status_ = ownership_status::allowed;
	}
};

// Prepare every batch authority before invoking any native replay callback.
// enter() itself allocates nothing. Expose the batch only around checkpoint;
// each apply callback instead gets its single-PID execution_scope.
class replay_checkpoint_scope
{
    public:
	explicit replay_checkpoint_scope(
		std::span<const replay_reservation *const> reservations) noexcept
	{
		try
		{
			if (reservations.empty() ||
			    reservations.size() > detail::max_permitted_pids)
				return;
			authorities_.reserve(reservations.size());
			for (const auto *reservation : reservations)
			{
				if (!reservation || !reservation->generation_)
					return;
				authorities_.push_back({ reservation->pid_, reservation->epoch_,
							 reservation->generation_, true });
			}
			std::sort(authorities_.begin(), authorities_.end(),
				  [](const auto &a, const auto &b) { return a.pid < b.pid; });
			for (size_t i = 1; i < authorities_.size(); ++i)
				if (authorities_[i - 1].pid == authorities_[i].pid)
					return;
			prepared_ = true;
		}
		catch (const std::bad_alloc &)
		{
		}
	}
	replay_checkpoint_scope(const replay_checkpoint_scope &) = delete;
	replay_checkpoint_scope &operator=(const replay_checkpoint_scope &) = delete;
	~replay_checkpoint_scope() noexcept
	{
		if (entered_)
		{
			std::lock_guard<std::mutex> lock(detail::mutex);
			detail::leave_scope_locked(link_);
		}
	}
	bool prepared() const noexcept { return prepared_; }
	ownership_status enter() noexcept
	{
		std::lock_guard<std::mutex> lock(detail::mutex);
		if (!prepared_ || entered_ || detail::integrity_failed)
			return ownership_status::unavailable;
		for (const auto &authority : authorities_)
		{
			if (!detail::valid_epoch_locked(authority.epoch))
				return ownership_status::invalid_epoch;
			if (detail::held_locked(authority.pid))
				return ownership_status::held;
			if (!detail::scope_ready_locked(authority))
				return ownership_status::busy;
		}
		if (!detail::generation_available_locked())
			return ownership_status::unavailable;
		link_.entries = authorities_.data();
		link_.size = authorities_.size();
		detail::enter_scope_locked(link_);
		entered_ = true;
		return ownership_status::allowed;
	}

    private:
	std::vector<detail::scope_authority> authorities_;
	detail::scope_link link_ = {};
	bool prepared_ = false, entered_ = false;
};

struct ownership_observation
{
	uint64_t epoch = 0, change_sequence = 0, resident_count = 0;
	bool replay_pending = false, replay_reserved = false, executing = false;
	bool publication_held = false;
	bool available = false;
};
inline ownership_observation observe_ownership(uint64_t epoch, int pid) noexcept
{
	std::lock_guard<std::mutex> lock(detail::mutex);
	ownership_observation out;
	if (!detail::valid_epoch_locked(epoch) || pid <= 0)
		return out;
	out.epoch = epoch;
	out.change_sequence = detail::ownership_change;
	out.available = true;
	out.publication_held = detail::held_locked(pid);
	const auto found = detail::owned_pids.find(pid);
	if (found != detail::owned_pids.end())
	{
		out.resident_count = found->second.claims;
		out.replay_pending = found->second.ticket != 0;
		out.replay_reserved = found->second.reserved;
		out.executing = found->second.scope_owner != 0;
	}
	return out;
}
inline void signal_ownership_change(uint64_t epoch) noexcept
{
	std::lock_guard<std::mutex> lock(detail::mutex);
	if (detail::valid_epoch_locked(epoch))
		detail::changed_locked();
}
// The existing dispatcher uses this event for new work, stop and ownership
// release. Queue owners signal after publication; no periodic ownership retry.
inline void wait_ownership_change(uint64_t epoch, uint64_t observed) noexcept
{
	if (detail::current_scope)
		return;
	std::unique_lock<std::mutex> lock(detail::mutex);
	if (!detail::valid_epoch_locked(epoch))
		return;
	detail::ownership_event().wait(lock,
				       [&] {
					       return !detail::valid_epoch_locked(epoch) ||
						      detail::ownership_change != observed;
				       });
}
// External dispatcher observes sequence BEFORE inspecting its queues. No caller
// waits while holding pipeline/worker/journal/SQL locks or an execution scope.
inline bool wait_ownership_change(uint64_t epoch, uint64_t observed,
				  std::chrono::milliseconds timeout) noexcept
{
	if (detail::current_scope || timeout.count() < 0)
		return false;
	std::unique_lock<std::mutex> lock(detail::mutex);
	if (!detail::valid_epoch_locked(epoch))
		return true;
	return detail::ownership_event().wait_for(
		lock, timeout,
		[&] {
			return !detail::valid_epoch_locked(epoch) ||
			       detail::ownership_change != observed;
		});
}
}
#endif
