#include "net/network_wakeup.h"
#include "player/player_save_worker.h"
#include "player/player_save_execution_guard.h"
#include "sql/sql_thread_init.h"

#include "persistence/persistence_observability.h"
#include "player/player_revision_state.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cerrno>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <memory>
#include <limits>
#include <mutex>
#include <new>
#include <optional>
#include <thread>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#ifndef __NO_MYSQL__
#include <mysql/mysql.h>
#endif

namespace
{
struct queued_snapshot
{
	player_snapshot snapshot;
	// Revision bookkeeping may narrow after an older ACK. It must not mutate
	// the encoded identity of a receipt-bearing journal frame.
	player_component_mask_t claimed_components = 0;
	uint64_t queued_at_usec = 0;
	unsigned int retry_count = 0;
	uint64_t request_generation = 0;
	bool wake_pending = false;
	player_save_execution_guard::resident_claim residence;
	uint64_t ownership_epoch = 0;
	bool waiting_for_ownership = false;
};

static_assert(std::is_nothrow_move_assignable_v<player_snapshot>);
static_assert(std::is_nothrow_move_assignable_v<player_save_completion>);
static_assert(std::is_nothrow_move_constructible_v<player_save_completion>);
static_assert(std::is_nothrow_default_constructible_v<player_save_completion>);
static_assert(std::is_nothrow_destructible_v<player_save_completion>);
static_assert(std::is_nothrow_move_assignable_v<player_save_owned_completion>);
static_assert(std::is_nothrow_move_constructible_v<player_save_owned_completion>);
static_assert(std::is_nothrow_default_constructible_v<player_save_owned_completion>);
static_assert(std::is_nothrow_destructible_v<player_save_owned_completion>);
static_assert(PLAYER_SAVE_WORKER_MAX_RESULTS >= PLAYER_SAVE_WORKER_MAX_PIDS);

struct pid_slot
{
	std::unique_ptr<queued_snapshot> active;
	std::unique_ptr<queued_snapshot> pending;
	bool dispatched = false;
	bool deferred = false;
	bool deferred_original = false;
	// Claimed results cannot transfer their residence until the worker scope
	// and all nested permits have actually unwound after completion staging.
	bool completion_ready = true;
};

std::mutex worker_mutex;
std::condition_variable job_available;
std::condition_variable result_available;
std::unordered_map<int, pid_slot> slots;
std::deque<int> ready_pids;
// Worker delivery must not allocate after a real journal ACK. All entries stay
// within the existing result bound; their receipts remain on the active job
// until pulse validates the revision and transfers that ownership.
class completion_queue
{
    public:
	bool empty() const noexcept { return count_ == 0; }
	size_t size() const noexcept { return count_; }
	player_save_completion &front() noexcept
	{
		assert(count_ > 0);
		return entries_[head_];
	}
	void push_back(player_save_completion &&completion) noexcept
	{
		assert(count_ < entries_.size());
		entries_[(head_ + count_) % entries_.size()] = std::move(completion);
		++count_;
	}
	void pop_front() noexcept
	{
		assert(count_ > 0);
		entries_[head_] = {};
		head_ = (head_ + 1) % entries_.size();
		--count_;
	}
	void clear() noexcept
	{
		while (!empty())
			pop_front();
		head_ = 0;
	}

    private:
	static_assert(PLAYER_SAVE_WORKER_MAX_RESULTS > 0);
	std::array<player_save_completion, PLAYER_SAVE_WORKER_MAX_RESULTS> entries_{};
	size_t head_ = 0;
	size_t count_ = 0;
};
completion_queue results;
std::unordered_set<int> ready_set;
std::vector<std::thread> workers;
player_save_apply_fn apply_callback = nullptr;
void *apply_context = nullptr;
player_save_journal_append_fn journal_append_callback = nullptr;
player_save_journal_ack_fn journal_ack_callback = nullptr;
player_save_journal_terminal_fn journal_terminal_callback = nullptr;
void *journal_context = nullptr;
player_save_worker_health health = {};
size_t retained_bytes = 0;
bool stop_requested = false;
// Process-local identities remain monotonic through shutdown/reset. They do not
// change snapshot bytes or grant repository execution authority.
uint64_t next_request_generation = 0;
uint64_t worker_lifecycle = 0;
bool wake_turn = false;

uint64_t now_usec()
{
	return persistence_observability_now_usec();
}

void saturating_increment(uint64_t &counter)
{
	persistence_counter_saturating_add(&counter, 1);
}

void update_max(uint64_t &target, uint64_t candidate)
{
	if (candidate > target)
		target = candidate;
}

bool valid_snapshot(const player_snapshot &snapshot)
{
	const uint32_t required = snapshot.death ? PLAYER_SNAPSHOT_DEATH_SCHEMA_VERSION :
						   PLAYER_SNAPSHOT_SCHEMA_VERSION;
	return (snapshot.schema_version == required ||
		(snapshot.death &&
		 player_snapshot_is_death_request_schema(snapshot.schema_version)) ||
		(!snapshot.death &&
		 (snapshot.schema_version == PLAYER_SNAPSHOT_CRAFT_RECEIPT_SCHEMA_VERSION ||
		  snapshot.schema_version == PLAYER_SNAPSHOT_QUEST_REWARD_SCHEMA_VERSION ||
		  snapshot.schema_version ==
			  PLAYER_SNAPSHOT_SPELL_EFFECT_RECEIPT_SCHEMA_VERSION))) &&
	       snapshot.pid > 0 && snapshot.revision && snapshot.components &&
	       !(snapshot.components & ~PLAYER_CHECKPOINT_COMPONENT_ALL) &&
	       snapshot.encoded_size_bound &&
	       snapshot.encoded_size_bound <= PLAYER_SNAPSHOT_MAX_BYTES;
}

bool requires_sealed_identity(const player_snapshot &snapshot)
{
	return snapshot.death || !snapshot.quest_xp_receipts.empty() ||
	       !snapshot.spell_effect_receipts.empty() || !snapshot.craft_receipts.empty();
}

void queue_ready_locked(int pid)
{
	if (ready_set.contains(pid))
		return;
	if (ready_set.insert(pid).second)
	{
		try
		{
			ready_pids.push_back(pid);
		}
		catch (...)
		{
			ready_set.erase(pid);
			throw;
		}
		job_available.notify_one();
	}
}

// Called only for a ready entry staged by the current admission. No worker can
// consume it while worker_mutex is held, and erasing integer entries allocates nothing.
void cancel_ready_locked(int pid)
{
	const auto entry = std::find(ready_pids.begin(), ready_pids.end(), pid);
	if (entry != ready_pids.end())
		ready_pids.erase(entry);
	ready_set.erase(pid);
}

void update_depth_health_locked()
{
	uint64_t queued = 0;
	uint64_t inflight = 0;
	uint64_t deferred = 0;
	for (const auto &[pid, slot] : slots)
	{
		(void)pid;
		if (slot.active)
		{
			if (slot.dispatched)
				++inflight;
			else
				++queued;
		}
		if (slot.pending)
			++queued;
		if (slot.deferred)
			++deferred;
	}
	health.queued_pids = queued;
	health.inflight_pids = inflight;
	health.deferred_pids = deferred;
	health.queued_bytes = retained_bytes;
	update_max(health.high_water_pids, queued + inflight);
	update_max(health.high_water_bytes, retained_bytes);
}

bool notified_deferred_locked(const pid_slot &slot) noexcept
{
	return slot.active && !slot.dispatched && slot.deferred && slot.active->wake_pending;
}

bool has_notified_deferred_locked() noexcept
{
	return std::any_of(slots.begin(), slots.end(), [](const auto &entry)
			   { return notified_deferred_locked(entry.second); });
}

bool notify_active_locked(int pid, const player_save_deferred_identity *identity) noexcept
{
	const auto found = slots.find(pid);
	if (!health.running || stop_requested || !apply_callback || found == slots.end() ||
	    !found->second.active)
		return false;
	auto &job = *found->second.active;
	if (identity && (identity->revision != job.snapshot.revision ||
			 identity->request_generation != job.request_generation ||
			 identity->worker_lifecycle != worker_lifecycle))
		return false;
	job.wake_pending = true;
	job_available.notify_one();
	return true;
}

void worker_main()
{
#ifndef __NO_MYSQL__
	if (sql_worker_thread_init() != 0)
		return;
#endif
	{
		std::lock_guard<std::mutex> lock(worker_mutex);
		++health.running_workers;
	}
	for (;;)
	{
		int pid = 0;
		const queued_snapshot *job = nullptr;
		bool owned_attempt = false;
		bool result_staged = false;
		uint64_t request_generation = 0;
		{
			std::unique_lock<std::mutex> lock(worker_mutex);
			job_available.wait(lock,
					   [] {
						   return stop_requested || !ready_pids.empty() ||
							  has_notified_deferred_locked();
					   });
			if (stop_requested && ready_pids.empty())
				break;
			auto found = slots.end();
			const bool select_wake = !stop_requested &&
						 has_notified_deferred_locked() &&
						 (ready_pids.empty() || wake_turn);
			if (!select_wake)
			{
				pid = ready_pids.front();
				ready_pids.pop_front();
				ready_set.erase(pid);
				found = slots.find(pid);
				wake_turn = true;
			}
			else
			{
				found = std::find_if(
					slots.begin(), slots.end(), [](const auto &entry)
					{ return notified_deferred_locked(entry.second); });
				if (found != slots.end())
				{
					pid = found->first;
					found->second.deferred = false;
					wake_turn = false;
				}
			}
			if (found == slots.end() || !found->second.active ||
			    found->second.dispatched || found->second.deferred)
				continue;
			found->second.dispatched = true;
			// This attempt checks the guard after consuming the prior notification.
			// A newer notification during that check remains sticky through park.
			found->second.active->wake_pending = false;
			job = found->second.active.get();
			owned_attempt = static_cast<bool>(job->residence);
			request_generation = job->request_generation;
			// Publish the observation window before checking the leaf scope so a
			// release notice arriving before park remains attached to this job.
			found->second.active->waiting_for_ownership = owned_attempt;
			found->second.completion_ready = !owned_attempt;
			update_depth_health_locked();
		}

		{
			std::optional<player_save_execution_guard::execution_scope>
				ownership_execution;
			auto ownership = player_save_execution_guard::ownership_status::allowed;
			if (owned_attempt)
			{
				ownership_execution.emplace(job->residence);
				ownership = ownership_execution->result();
				std::lock_guard<std::mutex> lock(worker_mutex);
				slots.at(pid).active->waiting_for_ownership =
					ownership ==
					player_save_execution_guard::ownership_status::busy;
			}
			persistence_trace_event trace;
			trace.stage = persistence_trace_stage::save_apply;
			trace.pid = job->snapshot.pid;
			trace.revision = job->snapshot.revision;
			trace.components = job->snapshot.components;
			trace.attempt = job->retry_count;
			persistence_trace_record(trace);
			const uint64_t started = now_usec();
			// Admission and execution share the leaf guard. Keep this owner until
			// journal ACK/terminal handling and completion staging have finished;
			// a new publication hold cannot overtake an admitted ordinary save.
			player_save_execution_guard::permit execution(pid);
			player_save_apply_result applied = {};
			if (ownership == player_save_execution_guard::ownership_status::busy ||
			    ownership == player_save_execution_guard::ownership_status::held ||
			    execution.result() == player_save_execution_guard::admission::held)
			{
				applied = { player_save_apply_outcome::deferred, 0, 0 };
			}
			else if (ownership !=
					 player_save_execution_guard::ownership_status::allowed ||
				 execution.result() ==
					 player_save_execution_guard::admission::unavailable)
			{
				// Allocation/capacity failure has no release-triggered wake. Use
				// the existing bounded failure retry rather than parking as held.
				applied = { player_save_apply_outcome::retryable_failure, 0,
					    ENOMEM };
			}
			else
			{
				try
				{
					applied = apply_callback(job->snapshot, apply_context);
				}
				catch (const std::bad_alloc &)
				{
					applied = { player_save_apply_outcome::retryable_failure, 0,
						    ENOMEM };
				}
				catch (...)
				{
					applied = { player_save_apply_outcome::terminal_failure, 0,
						    EFAULT };
				}
			}
			// Keep the journal record when a receipt-bearing save's claimed success
			// belongs to a different durable revision.
			if ((applied.outcome == player_save_apply_outcome::applied ||
			     applied.outcome == player_save_apply_outcome::already_applied) &&
			    !player_save_result_matches_exact_request(job->snapshot, applied))
			{
				applied.outcome = player_save_apply_outcome::terminal_failure;
				applied.error_code = ESTALE;
			}
			trace.stage = persistence_trace_stage::save_result;
			trace.outcome = static_cast<uint32_t>(applied.outcome);
			trace.error = applied.error_code;
			trace.diagnosis = static_cast<uint32_t>(applied.custody_diagnosis);
			trace.witness = applied.custody_witness;
			trace.durable_revision = applied.durable_revision;
			trace.incident =
				applied.outcome == player_save_apply_outcome::terminal_failure ||
				((applied.outcome == player_save_apply_outcome::retryable_failure ||
				  applied.outcome == player_save_apply_outcome::ambiguous_commit) &&
				 job->retry_count >= PLAYER_SAVE_WORKER_MAX_RETRIES);
			persistence_trace_record(trace);
			if (applied.outcome == player_save_apply_outcome::deferred)
			{
				std::lock_guard<std::mutex> lock(worker_mutex);
				auto &slot = slots.at(pid);
				slot.dispatched = false;
				slot.deferred = true;
				slot.deferred_original = true;
				update_depth_health_locked();
				continue;
			}
			if ((applied.outcome == player_save_apply_outcome::applied ||
			     applied.outcome == player_save_apply_outcome::already_applied ||
			     applied.outcome == player_save_apply_outcome::stale_revision) &&
			    applied.durable_revision >= job->snapshot.revision &&
			    player_save_result_matches_exact_request(job->snapshot, applied))
			{
				player_save_journal_ack_fn acknowledge = nullptr;
				void *ack_context = nullptr;
				{
					std::lock_guard<std::mutex> lock(worker_mutex);
					acknowledge = journal_ack_callback;
					ack_context = journal_context;
				}
				bool acked = true;
				if (acknowledge)
				{
					try
					{
						acked = acknowledge(job->snapshot,
								    applied.durable_revision,
								    ack_context);
					}
					catch (...)
					{
						acked = false;
					}
				}
				trace.stage = persistence_trace_stage::save_checkpoint;
				trace.outcome = acked ? 0 : 1;
				trace.incident = false;
				persistence_trace_record(trace);
				if (!acked)
				{
					std::lock_guard<std::mutex> lock(worker_mutex);
					saturating_increment(health.journal_ack_failures);
					// DB durability is real, but do not ACK the player or discard the
					// retained job while its journal checkpoint has failed.
					applied.outcome =
						player_save_apply_outcome::retryable_failure;
					applied.error_code = EIO;
				}
			}
			// Do not expose an unresolved terminal result (or drop an exhausted
			// retry) while login can still consume an older durable DB snapshot.
			if (applied.outcome == player_save_apply_outcome::terminal_failure ||
			    ((applied.outcome == player_save_apply_outcome::retryable_failure ||
			      applied.outcome == player_save_apply_outcome::ambiguous_commit) &&
			     job->retry_count >= PLAYER_SAVE_WORKER_MAX_RETRIES))
			{
				player_save_journal_terminal_fn terminal = nullptr;
				void *terminal_context = nullptr;
				{
					std::lock_guard<std::mutex> lock(worker_mutex);
					terminal = journal_terminal_callback;
					terminal_context = journal_context;
				}
				if (terminal)
					terminal(job->snapshot, terminal_context);
				trace.stage = persistence_trace_stage::save_fence;
				trace.outcome = static_cast<uint32_t>(applied.outcome);
				trace.error = applied.error_code;
				trace.incident = true;
				persistence_trace_record(trace);
			}
			const uint64_t completed = now_usec();
			player_save_completion completion = {
				.pid = job->snapshot.pid,
				.revision = job->snapshot.revision,
				.components = job->snapshot.components,
				.outcome = applied.outcome,
				.durable_revision = applied.durable_revision,
				.error_code = applied.error_code,
				.custody_diagnosis = applied.custody_diagnosis,
				.retry_count = job->retry_count,
				.queued_at_usec = job->queued_at_usec,
				.started_at_usec = started,
				.completed_at_usec = completed,
				.quest_xp_receipts = {},
				.spell_effect_receipts = {},
				.failed_spell_effect_receipts = {},
				.custody_witness = applied.custody_witness,
			};
			{
				std::unique_lock<std::mutex> lock(worker_mutex);
				result_available.wait(
					lock,
					[] {
						return stop_requested ||
						       results.size() <
							       PLAYER_SAVE_WORKER_MAX_RESULTS;
					});
				if (results.size() < PLAYER_SAVE_WORKER_MAX_RESULTS)
				{
					// A nondeferred result belongs to this completed attempt; do not
					// carry a release notification into a later retry or replacement.
					slots.at(pid).active->wake_pending = false;
					results.push_back(std::move(completion));
					result_staged = true;
					if (!owned_attempt)
						network_wakeup_notify();
				}
			}
		} // The nested permit and ownership scope unwind before claim delivery.
		if (owned_attempt && result_staged)
		{
			{
				std::lock_guard<std::mutex> lock(worker_mutex);
				const auto found = slots.find(pid);
				if (found == slots.end() || !found->second.active ||
				    found->second.active->request_generation != request_generation)
					continue;
				found->second.completion_ready = true;
			}
			network_wakeup_notify();
		}
	}
	std::lock_guard<std::mutex> lock(worker_mutex);
	--health.running_workers;
#ifndef __NO_MYSQL__
	mysql_thread_end();
#endif
}

bool promote_pending_locked(int pid, pid_slot &slot)
{
	if (!slot.pending)
		return true;
	player_revision_snapshot revision = {};
	if (!player_revision_snapshot_copy(pid, &revision) ||
	    revision.queued_revision != slot.pending->snapshot.revision ||
	    !revision.queued_components ||
	    (slot.pending->snapshot.components & revision.queued_components) !=
		    revision.queued_components)
		return false;
	// Keep the original typed body for repository application and journal ACK.
	// Ordinary snapshots retain their existing narrowed-mask behavior.
	if (!requires_sealed_identity(slot.pending->snapshot))
		slot.pending->snapshot.components = revision.queued_components;
	if (!player_revision_begin_inflight(pid, slot.pending->snapshot.revision,
					    revision.queued_components))
		return false;
	slot.pending->claimed_components = revision.queued_components;
	slot.active = std::move(slot.pending);
	slot.dispatched = false;
	slot.deferred_original = false;
	queue_ready_locked(pid);
	return true;
}

void remove_active_bytes_locked(pid_slot &slot)
{
	if (slot.active)
		retained_bytes -= slot.active->snapshot.encoded_size_bound;
}

void account_completion_locked(const player_save_completion &completion, uint64_t ack_at)
{
	update_max(health.max_capture_to_apply_usec,
		   completion.started_at_usec - completion.queued_at_usec);
	update_max(health.max_apply_usec,
		   completion.completed_at_usec - completion.started_at_usec);
	update_max(health.max_ack_latency_usec, ack_at - completion.completed_at_usec);
	if (completion.revision > completion.durable_revision)
		update_max(health.max_revision_gap,
			   completion.revision - completion.durable_revision);
}
} // namespace

bool player_save_worker_init(player_save_apply_fn apply, void *context, unsigned int worker_threads)
{
	if (!apply || !worker_threads || worker_threads > PLAYER_SAVE_WORKER_DEFAULT_THREADS * 4)
		return false;
	{
		std::lock_guard<std::mutex> lock(worker_mutex);
		if (health.running || !workers.empty())
			return false;
		if (worker_lifecycle == std::numeric_limits<uint64_t>::max())
			return false;
		++worker_lifecycle;
		for (auto &[pid, slot] : slots)
		{
			(void)pid;
			if (slot.active)
				slot.active->wake_pending = false;
		}
		apply_callback = apply;
		apply_context = context;
		stop_requested = false;
		health.running = true;
		health.stop_pending = false;
		health.worker_threads = worker_threads;
	}
	try
	{
		for (unsigned int index = 0; index < worker_threads; ++index)
			workers.emplace_back(worker_main);
	}
	catch (const std::system_error &)
	{
		{
			std::lock_guard<std::mutex> lock(worker_mutex);
			stop_requested = true;
			health.running = false;
			health.stop_pending = true;
			job_available.notify_all();
		}
		for (std::thread &worker : workers)
			if (worker.joinable())
				worker.join();
		workers.clear();
		return false;
	}
	return true;
}

void player_save_worker_shutdown(void)
{
	{
		std::lock_guard<std::mutex> lock(worker_mutex);
		stop_requested = true;
		health.stop_pending = true;
		for (auto &[pid, slot] : slots)
		{
			(void)pid;
			if (slot.active)
				slot.active->wake_pending = false;
		}
		job_available.notify_all();
		result_available.notify_all();
	}
	for (std::thread &worker : workers)
		if (worker.joinable())
			worker.join();
	std::lock_guard<std::mutex> lock(worker_mutex);
	workers.clear();
	health.running = false;
	health.stop_pending = false;
	health.worker_threads = 0;
	apply_callback = nullptr;
	apply_context = nullptr;
}

bool player_save_worker_set_journal_hooks(player_save_journal_append_fn append,
					  player_save_journal_ack_fn acknowledge, void *context,
					  player_save_journal_terminal_fn terminal)
{
	if (append && !acknowledge)
		return false;
	std::lock_guard<std::mutex> lock(worker_mutex);
	if (!slots.empty())
		return false;
	journal_append_callback = append;
	journal_ack_callback = acknowledge;
	journal_terminal_callback = terminal;
	journal_context = context;
	return true;
}

player_save_submit_result
player_save_worker_submit_owned_retained(player_snapshot *snapshot_pointer,
					 player_save_execution_guard::resident_claim &residence)
{
	if (!snapshot_pointer)
		return player_save_submit_result::invalid;
	player_snapshot &snapshot = *snapshot_pointer;
	if (!valid_snapshot(snapshot))
		return player_save_submit_result::invalid;
	const uint64_t epoch = player_save_execution_guard::current_ownership_epoch();
	if ((epoch && !residence.matches_current(snapshot.pid)) || (!epoch && residence))
		return player_save_submit_result::worker_unavailable;
	player_save_journal_append_fn append = nullptr;
	void *append_context = nullptr;
	{
		std::lock_guard<std::mutex> lock(worker_mutex);
		append = journal_append_callback;
		append_context = journal_context;
	}
	bool durably_journaled = false;
	{
		std::optional<player_save_execution_guard::execution_scope> append_owner;
		if (residence)
		{
			append_owner.emplace(residence);
			const auto admitted = append_owner->result();
			if (admitted == player_save_execution_guard::ownership_status::busy ||
			    admitted == player_save_execution_guard::ownership_status::held)
				return player_save_submit_result::replay_busy;
			if (admitted != player_save_execution_guard::ownership_status::allowed)
				return player_save_submit_result::worker_unavailable;
		}
		durably_journaled = append && append(snapshot, append_context);
	}
	if (append && !durably_journaled)
		return player_save_submit_result::journal_failure;
	std::lock_guard<std::mutex> lock(worker_mutex);
	if (!health.running || stop_requested || !apply_callback)
		return durably_journaled ? player_save_submit_result::durably_spilled :
					   player_save_submit_result::worker_unavailable;
	if (next_request_generation == std::numeric_limits<uint64_t>::max())
		return durably_journaled ? player_save_submit_result::durably_spilled :
					   player_save_submit_result::capacity_exceeded;

	auto found = slots.find(snapshot.pid);
	if (found == slots.end())
	{
		if (slots.size() >= PLAYER_SAVE_WORKER_MAX_PIDS ||
		    snapshot.encoded_size_bound > PLAYER_SAVE_WORKER_MAX_BYTES - retained_bytes)
			return durably_journaled ? player_save_submit_result::durably_spilled :
						   player_save_submit_result::capacity_exceeded;
		player_revision_snapshot revision_state = {};
		if (!player_revision_snapshot_copy(snapshot.pid, &revision_state) ||
		    revision_state.queued_revision != snapshot.revision ||
		    !revision_state.queued_components ||
		    (snapshot.components & revision_state.queued_components) !=
			    revision_state.queued_components)
			return player_save_submit_result::revision_state_mismatch;
		const int pid = snapshot.pid;
		bool slot_staged = false;
		try
		{
			// Allocate every owner before consuming the caller's retained capture
			// or claiming its revision. Even ready-container growth can fail.
			auto job = std::make_unique<queued_snapshot>();
			auto inserted = slots.try_emplace(pid);
			if (!inserted.second)
				return player_save_submit_result::revision_state_mismatch;
			slot_staged = true;
			queue_ready_locked(pid);
			if (!player_revision_begin_inflight(pid, snapshot.revision,
							    revision_state.queued_components))
			{
				cancel_ready_locked(pid);
				slots.erase(inserted.first);
				return player_save_submit_result::revision_state_mismatch;
			}
			if (!requires_sealed_identity(snapshot))
				snapshot.components = revision_state.queued_components;
			job->claimed_components = revision_state.queued_components;
			job->request_generation = ++next_request_generation;
			job->snapshot = std::move(snapshot);
			job->residence = std::move(residence);
			job->ownership_epoch = epoch;
			job->queued_at_usec = now_usec();
			retained_bytes += job->snapshot.encoded_size_bound;
			inserted.first->second.active = std::move(job);
		}
		catch (const std::bad_alloc &)
		{
			if (slot_staged)
			{
				cancel_ready_locked(pid);
				slots.erase(pid);
			}
			return player_save_submit_result::capacity_exceeded;
		}
		saturating_increment(health.submitted);
		update_depth_health_locked();
		return player_save_submit_result::accepted;
	}

	pid_slot &slot = found->second;
	const queued_snapshot *newest = slot.pending ? slot.pending.get() : slot.active.get();
	if (!newest || snapshot.revision <= newest->snapshot.revision)
		return player_save_submit_result::stale;
	const auto required_components = slot.pending ? newest->snapshot.components :
							newest->claimed_components;
	if ((snapshot.components & required_components) != required_components)
		return player_save_submit_result::revision_state_mismatch;

	const bool replace_undispatched = !slot.dispatched && !slot.deferred_original &&
					  !slot.pending;
	const size_t replaced_bytes = slot.pending ? slot.pending->snapshot.encoded_size_bound :
				      replace_undispatched ?
						     slot.active->snapshot.encoded_size_bound :
						     0;
	if (snapshot.encoded_size_bound >
	    PLAYER_SAVE_WORKER_MAX_BYTES - (retained_bytes - replaced_bytes))
		return player_save_submit_result::capacity_exceeded;
	bool ready_staged = false;
	try
	{
		// Construct the empty owner first; a moved aggregate argument would
		// consume the input before make_unique attempts its allocation.
		auto pending = std::make_unique<queued_snapshot>();
		if (replace_undispatched)
		{
			player_revision_snapshot revision = {};
			if (!player_revision_snapshot_copy(snapshot.pid, &revision) ||
			    revision.current_revision != snapshot.revision ||
			    revision.queued_revision != snapshot.revision ||
			    !revision.queued_components ||
			    (snapshot.components & revision.queued_components) !=
				    revision.queued_components ||
			    revision.unacknowledged_components != revision.queued_components)
				return player_save_submit_result::revision_state_mismatch;
			ready_staged = !ready_set.contains(snapshot.pid);
			queue_ready_locked(snapshot.pid);
			if (!player_revision_fail_inflight(slot.active->snapshot.pid,
							   slot.active->snapshot.revision,
							   slot.active->claimed_components) ||
			    !player_revision_begin_inflight(snapshot.pid, snapshot.revision,
							    revision.queued_components))
			{
				if (ready_staged)
					cancel_ready_locked(snapshot.pid);
				return player_save_submit_result::revision_state_mismatch;
			}
			pending->claimed_components = revision.queued_components;
		}
		pending->request_generation = ++next_request_generation;
		pending->snapshot = std::move(snapshot);
		pending->residence = std::move(residence);
		pending->ownership_epoch = epoch;
		pending->queued_at_usec = now_usec();
		retained_bytes =
			retained_bytes - replaced_bytes + pending->snapshot.encoded_size_bound;
		if (replace_undispatched)
			slot.active = std::move(pending);
		else
			slot.pending = std::move(pending);
	}
	catch (const std::bad_alloc &)
	{
		if (ready_staged)
			cancel_ready_locked(snapshot.pid);
		return player_save_submit_result::capacity_exceeded;
	}
	saturating_increment(health.coalesced);
	update_depth_health_locked();
	return player_save_submit_result::coalesced;
}

player_save_submit_result player_save_worker_submit_retained(player_snapshot *snapshot)
{
	if (!snapshot || !valid_snapshot(*snapshot))
		return player_save_submit_result::invalid;
	player_save_execution_guard::resident_claim residence;
	const auto epoch = player_save_execution_guard::current_ownership_epoch();
	if (epoch)
	{
		residence = player_save_execution_guard::resident_claim(epoch, snapshot->pid);
		if (residence.result() == player_save_execution_guard::ownership_status::busy)
			return player_save_submit_result::replay_busy;
		if (!residence)
			return player_save_submit_result::worker_unavailable;
	}
	return player_save_worker_submit_owned_retained(snapshot, residence);
}

player_save_submit_result player_save_worker_submit(player_snapshot snapshot)
{
	return player_save_worker_submit_retained(&snapshot);
}

namespace
{
size_t pulse_impl(player_save_completion *completions_out, player_save_owned_completion *owned_out,
		  size_t capacity)
{
	if (capacity && !completions_out && !owned_out)
		return 0;
	std::unique_lock<std::mutex> lock(worker_mutex);
	size_t consumed = 0;
	while (consumed < capacity && !results.empty())
	{
		// Retain the deliverable result until every allocation needed for its
		// retry or pending promotion has succeeded. A real journal ACK may
		// already have happened; consuming this result early loses its receipts.
		const auto &front = results.front();
		auto staged_slot = slots.find(front.pid);
		bool ready_staged = false;
		if (staged_slot != slots.end() && staged_slot->second.active &&
		    staged_slot->second.active->snapshot.revision == front.revision &&
		    staged_slot->second.active->snapshot.components == front.components)
		{
			const auto &slot = staged_slot->second;
			// Keep the original residence in the worker until its scope has
			// unwound and a move-only recipient can carry it through callbacks.
			if (slot.active->residence && (!owned_out || !slot.completion_ready))
				break;
			const bool retry =
				front.outcome == player_save_apply_outcome::retryable_failure ||
				front.outcome == player_save_apply_outcome::ambiguous_commit;
			const bool retry_ready = retry && slot.active->retry_count <
								  PLAYER_SAVE_WORKER_MAX_RETRIES;
			const bool finish_ready =
				slot.pending &&
				(front.outcome == player_save_apply_outcome::applied ||
				 front.outcome == player_save_apply_outcome::already_applied ||
				 front.outcome == player_save_apply_outcome::stale_revision ||
				 front.outcome == player_save_apply_outcome::terminal_failure ||
				 (retry && !retry_ready));
			if (retry_ready || finish_ready)
			{
				try
				{
					ready_staged = !ready_set.contains(front.pid);
					queue_ready_locked(front.pid);
				}
				catch (const std::bad_alloc &)
				{
					// queue_ready_locked removes any partial ready entry. Leave
					// all result/job/revision/receipt ownership for a later pulse.
					break;
				}
			}
		}
		player_save_completion completion = std::move(results.front());
		results.pop_front();
		result_available.notify_one();
		auto found = slots.find(completion.pid);
		if (found == slots.end() || !found->second.active ||
		    found->second.active->snapshot.revision != completion.revision ||
		    found->second.active->snapshot.components != completion.components)
			continue;

		pid_slot &slot = found->second;
		// Public completion identifies the persisted capture, including the full
		// death/literal mask consumed by the pipeline. Revision bookkeeping uses
		// only the obligation this job actually claimed.
		const auto claimed_components = slot.active->claimed_components;
		const uint64_t ack_at = now_usec();
		account_completion_locked(completion, ack_at);
		bool finished = false;
		bool exact_acknowledged = false;
		player_save_execution_guard::resident_claim delivered_residence;
		switch (completion.outcome)
		{
		case player_save_apply_outcome::applied:
		case player_save_apply_outcome::already_applied:
			if (player_revision_acknowledge(completion.pid, completion.revision,
							claimed_components))
			{
				saturating_increment(health.applied);
				exact_acknowledged = true;
				persistence_trace_event trace;
				trace.stage = persistence_trace_stage::save_ack;
				trace.pid = completion.pid;
				trace.revision = completion.revision;
				trace.durable_revision = completion.durable_revision;
				trace.components = claimed_components;
				persistence_trace_record(trace);
				finished = true;
			}
			else
			{
				saturating_increment(health.terminal_failures);
				player_revision_fail_inflight(completion.pid, completion.revision,
							      claimed_components);
				finished = true;
			}
			break;
		case player_save_apply_outcome::retryable_failure:
		case player_save_apply_outcome::ambiguous_commit:
			saturating_increment(health.retryable_failures);
			if (slot.active->retry_count < PLAYER_SAVE_WORKER_MAX_RETRIES)
			{
				++slot.active->retry_count;
				slot.active->queued_at_usec = ack_at;
				slot.dispatched = false;
				queue_ready_locked(completion.pid);
			}
			else
			{
				saturating_increment(health.retries_exhausted);
				player_revision_fail_inflight(completion.pid, completion.revision,
							      claimed_components);
				finished = true;
			}
			break;
		case player_save_apply_outcome::stale_revision:
			saturating_increment(health.stale);
			player_revision_fail_inflight(completion.pid, completion.revision,
						      claimed_components);
			finished = true;
			break;
		case player_save_apply_outcome::terminal_failure:
			saturating_increment(health.terminal_failures);
			if (completion.error_code == PLAYER_SAVE_ERROR_CUSTODY_PAYLOAD_MISMATCH)
				saturating_increment(health.custody_payload_mismatches);
			player_revision_fail_inflight(completion.pid, completion.revision,
						      claimed_components);
			finished = true;
			break;
		case player_save_apply_outcome::deferred:
			// worker_main parks this outcome before creating a completion.
			break;
		}

		if (finished)
		{
			if (exact_acknowledged)
			{
				completion.quest_xp_receipts =
					std::move(slot.active->snapshot.quest_xp_receipts);
				completion.spell_effect_receipts =
					std::move(slot.active->snapshot.spell_effect_receipts);
				completion.craft_receipts =
					std::move(slot.active->snapshot.craft_receipts);
			}
			else
			{
				completion.failed_spell_effect_receipts =
					std::move(slot.active->snapshot.spell_effect_receipts);
				completion.failed_craft_receipts =
					std::move(slot.active->snapshot.craft_receipts);
			}
			remove_active_bytes_locked(slot);
			delivered_residence = std::move(slot.active->residence);
			slot.active.reset();
			slot.dispatched = false;
			slot.deferred_original = false;
			if (!promote_pending_locked(completion.pid, slot))
			{
				if (ready_staged)
					cancel_ready_locked(completion.pid);
				saturating_increment(health.terminal_failures);
				retained_bytes -= slot.pending->snapshot.encoded_size_bound;
				slot.pending.reset();
			}
			if (!slot.active && !slot.pending)
				slots.erase(found);
		}
		if (completions_out)
			completions_out[consumed] = std::move(completion);
		else
		{
			owned_out[consumed].completion = std::move(completion);
			owned_out[consumed].residence = std::move(delivered_residence);
		}
		++consumed;
	}
	update_depth_health_locked();
	return consumed;
}
} // namespace

size_t player_save_worker_pulse(player_save_completion *completions_out, size_t capacity)
{
	return pulse_impl(completions_out, nullptr, capacity);
}

size_t player_save_worker_pulse_owned(player_save_owned_completion *completions_out,
				      size_t capacity)
{
	return pulse_impl(nullptr, completions_out, capacity);
}

bool player_save_worker_pid_pending(int pid)
{
	if (pid <= 0)
		return true;
	std::lock_guard<std::mutex> lock(worker_mutex);
	const auto found = slots.find(pid);
	return found != slots.end() &&
	       (found->second.active != nullptr || found->second.pending != nullptr);
}

bool player_save_worker_resume_deferred(int pid) noexcept
{
	if (pid <= 0)
		return false;
	std::lock_guard<std::mutex> lock(worker_mutex);
	return notify_active_locked(pid, nullptr);
}

bool player_save_worker_deferred_identity(int pid, player_save_deferred_identity *out) noexcept
{
	if (pid <= 0 || !out)
		return false;
	std::lock_guard<std::mutex> lock(worker_mutex);
	const auto found = slots.find(pid);
	if (!health.running || stop_requested || !apply_callback || found == slots.end() ||
	    !found->second.active)
		return false;
	const auto &job = *found->second.active;
	*out = { pid, job.snapshot.revision, job.request_generation, worker_lifecycle };
	return true;
}

bool player_save_worker_resume_deferred_exact(const player_save_deferred_identity &identity) noexcept
{
	if (identity.pid <= 0 || !identity.revision || !identity.request_generation ||
	    !identity.worker_lifecycle)
		return false;
	std::lock_guard<std::mutex> lock(worker_mutex);
	return notify_active_locked(identity.pid, &identity);
}

size_t player_save_worker_ownership_waiters(player_save_ownership_waiter *out,
					    size_t capacity) noexcept
{
	if (capacity && !out)
		return 0;
	std::lock_guard<std::mutex> lock(worker_mutex);
	if (!health.running || stop_requested || !apply_callback)
		return 0;
	size_t count = 0;
	for (const auto &[pid, slot] : slots)
	{
		if (count == capacity)
			break;
		if (!slot.active || !slot.active->residence || !slot.active->waiting_for_ownership)
			continue;
		const auto &job = *slot.active;
		out[count++] = { { pid, job.snapshot.revision, job.request_generation,
				   worker_lifecycle },
				 job.ownership_epoch };
	}
	return count;
}

player_save_worker_health player_save_worker_health_copy(void)
{
	std::lock_guard<std::mutex> lock(worker_mutex);
	player_save_worker_health snapshot = health;
	const uint64_t current = now_usec();
	uint64_t oldest = 0;
	for (const auto &[pid, slot] : slots)
	{
		(void)pid;
		if (slot.active && current >= slot.active->queued_at_usec)
			update_max(oldest, (current - slot.active->queued_at_usec) / 1000);
		if (slot.pending && current >= slot.pending->queued_at_usec)
			update_max(oldest, (current - slot.pending->queued_at_usec) / 1000);
	}
	snapshot.oldest_age_msec = oldest;
	snapshot.age_limit_exceeded = oldest > PLAYER_SAVE_WORKER_MAX_AGE_MSEC;
	return snapshot;
}

void player_save_worker_reset_for_tests(void)
{
	player_save_worker_shutdown();
	std::lock_guard<std::mutex> lock(worker_mutex);
	slots.clear();
	ready_pids.clear();
	results.clear();
	ready_set.clear();
	retained_bytes = 0;
	stop_requested = false;
	health = {};
	journal_append_callback = nullptr;
	journal_ack_callback = nullptr;
	journal_terminal_callback = nullptr;
	journal_context = nullptr;
}
