#include "player/player_save_worker.h"
#include "sql_thread_init.h"

#include "persistence_observability.h"
#include "player/player_revision_state.h"

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <new>
#include <thread>
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
	uint64_t queued_at_usec = 0;
	unsigned int retry_count = 0;
};

struct pid_slot
{
	std::unique_ptr<queued_snapshot> active;
	std::unique_ptr<queued_snapshot> pending;
	bool dispatched = false;
};

std::mutex worker_mutex;
std::condition_variable job_available;
std::condition_variable result_available;
std::unordered_map<int, pid_slot> slots;
std::deque<int> ready_pids;
std::deque<player_save_completion> results;
std::unordered_set<int> ready_set;
std::vector<std::thread> workers;
player_save_apply_fn apply_callback = nullptr;
void *apply_context = nullptr;
player_save_journal_append_fn journal_append_callback = nullptr;
player_save_journal_ack_fn journal_ack_callback = nullptr;
void *journal_context = nullptr;
player_save_worker_health health = {};
size_t retained_bytes = 0;
bool stop_requested = false;

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
	return snapshot.schema_version == PLAYER_SNAPSHOT_SCHEMA_VERSION && snapshot.pid > 0 &&
	       snapshot.revision && snapshot.components &&
	       !(snapshot.components & ~PLAYER_CHECKPOINT_COMPONENT_ALL) &&
	       snapshot.encoded_size_bound &&
	       snapshot.encoded_size_bound <= PLAYER_SNAPSHOT_MAX_BYTES;
}

void queue_ready_locked(int pid)
{
	if (ready_set.insert(pid).second)
	{
		ready_pids.push_back(pid);
		job_available.notify_one();
	}
}

void update_depth_health_locked()
{
	uint64_t queued = 0;
	uint64_t inflight = 0;
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
	}
	health.queued_pids = queued;
	health.inflight_pids = inflight;
	health.queued_bytes = retained_bytes;
	update_max(health.high_water_pids, queued + inflight);
	update_max(health.high_water_bytes, retained_bytes);
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
		{
			std::unique_lock<std::mutex> lock(worker_mutex);
			job_available.wait(lock,
					   [] { return stop_requested || !ready_pids.empty(); });
			if (stop_requested && ready_pids.empty())
				break;
			pid = ready_pids.front();
			ready_pids.pop_front();
			ready_set.erase(pid);
			auto found = slots.find(pid);
			if (found == slots.end() || !found->second.active ||
			    found->second.dispatched)
				continue;
			found->second.dispatched = true;
			job = found->second.active.get();
			update_depth_health_locked();
		}

		const uint64_t started = now_usec();
		player_save_apply_result applied = {};
		try
		{
			applied = apply_callback(job->snapshot, apply_context);
		}
		catch (const std::bad_alloc &)
		{
			applied = { player_save_apply_outcome::retryable_failure, 0, ENOMEM };
		}
		catch (...)
		{
			applied = { player_save_apply_outcome::terminal_failure, 0, EFAULT };
		}
		if ((applied.outcome == player_save_apply_outcome::applied ||
		     applied.outcome == player_save_apply_outcome::already_applied ||
		     applied.outcome == player_save_apply_outcome::stale_revision) &&
		    applied.durable_revision >= job->snapshot.revision)
		{
			player_save_journal_ack_fn acknowledge = nullptr;
			void *ack_context = nullptr;
			{
				std::lock_guard<std::mutex> lock(worker_mutex);
				acknowledge = journal_ack_callback;
				ack_context = journal_context;
			}
			if (acknowledge)
				acknowledge(job->snapshot.pid, applied.durable_revision,
					    ack_context);
		}
		const uint64_t completed = now_usec();
		player_save_completion completion = {
			.pid = job->snapshot.pid,
			.revision = job->snapshot.revision,
			.components = job->snapshot.components,
			.outcome = applied.outcome,
			.durable_revision = applied.durable_revision,
			.error_code = applied.error_code,
			.retry_count = job->retry_count,
			.queued_at_usec = job->queued_at_usec,
			.started_at_usec = started,
			.completed_at_usec = completed,
		};
		{
			std::unique_lock<std::mutex> lock(worker_mutex);
			result_available.wait(lock,
					      [] {
						      return stop_requested ||
							     results.size() <
								     PLAYER_SAVE_WORKER_MAX_RESULTS;
					      });
			if (results.size() < PLAYER_SAVE_WORKER_MAX_RESULTS)
				results.push_back(completion);
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
	/* An older exact ACK may remove a redundantly captured bit from the newer queued
	 * identity. The value rows stay sealed; the authoritative mask narrows safely. */
	slot.pending->snapshot.components = revision.queued_components;
	if (!player_revision_begin_inflight(pid, slot.pending->snapshot.revision,
					    slot.pending->snapshot.components))
		return false;
	slot.active = std::move(slot.pending);
	slot.dispatched = false;
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
					  player_save_journal_ack_fn acknowledge, void *context)
{
	if (append && !acknowledge)
		return false;
	std::lock_guard<std::mutex> lock(worker_mutex);
	if (!slots.empty())
		return false;
	journal_append_callback = append;
	journal_ack_callback = acknowledge;
	journal_context = context;
	return true;
}

player_save_submit_result player_save_worker_submit_retained(player_snapshot *snapshot_pointer)
{
	if (!snapshot_pointer)
		return player_save_submit_result::invalid;
	player_snapshot &snapshot = *snapshot_pointer;
	if (!valid_snapshot(snapshot))
		return player_save_submit_result::invalid;
	player_save_journal_append_fn append = nullptr;
	void *append_context = nullptr;
	{
		std::lock_guard<std::mutex> lock(worker_mutex);
		append = journal_append_callback;
		append_context = journal_context;
	}
	const bool durably_journaled = append && append(snapshot, append_context);
	if (append && !durably_journaled)
		return player_save_submit_result::journal_failure;
	std::lock_guard<std::mutex> lock(worker_mutex);
	if (!health.running || stop_requested || !apply_callback)
		return durably_journaled ? player_save_submit_result::durably_spilled :
					   player_save_submit_result::worker_unavailable;

	auto found = slots.find(snapshot.pid);
	if (found == slots.end())
	{
		if (slots.size() >= PLAYER_SAVE_WORKER_MAX_PIDS ||
		    snapshot.encoded_size_bound > PLAYER_SAVE_WORKER_MAX_BYTES - retained_bytes)
			return durably_journaled ? player_save_submit_result::durably_spilled :
						   player_save_submit_result::capacity_exceeded;
		if (!player_revision_begin_inflight(snapshot.pid, snapshot.revision,
						    snapshot.components))
			return player_save_submit_result::revision_state_mismatch;
		const int pid = snapshot.pid;
		const player_revision_t revision = snapshot.revision;
		const player_component_mask_t components = snapshot.components;
		const size_t snapshot_bytes = snapshot.encoded_size_bound;
		bool bytes_accounted = false;
		try
		{
			pid_slot slot;
			slot.active = std::make_unique<queued_snapshot>(
				queued_snapshot{ std::move(snapshot), now_usec(), 0 });
			retained_bytes += snapshot_bytes;
			bytes_accounted = true;
			slots.emplace(pid, std::move(slot));
			queue_ready_locked(pid);
		}
		catch (const std::bad_alloc &)
		{
			if (bytes_accounted)
				retained_bytes -= snapshot_bytes;
			player_revision_fail_inflight(pid, revision, components);
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
	if ((snapshot.components & newest->snapshot.components) != newest->snapshot.components)
		return player_save_submit_result::revision_state_mismatch;

	const bool replace_undispatched = !slot.dispatched && !slot.pending;
	const size_t replaced_bytes = slot.pending ? slot.pending->snapshot.encoded_size_bound :
				      replace_undispatched ?
						     slot.active->snapshot.encoded_size_bound :
						     0;
	if (snapshot.encoded_size_bound >
	    PLAYER_SAVE_WORKER_MAX_BYTES - (retained_bytes - replaced_bytes))
		return player_save_submit_result::capacity_exceeded;
	try
	{
		auto pending = std::make_unique<queued_snapshot>(
			queued_snapshot{ std::move(snapshot), now_usec(), 0 });
		if (replace_undispatched)
		{
			player_revision_snapshot revision = {};
			if (!player_revision_snapshot_copy(pending->snapshot.pid, &revision) ||
			    revision.queued_revision != pending->snapshot.revision ||
			    revision.queued_components != pending->snapshot.components ||
			    !player_revision_fail_inflight(slot.active->snapshot.pid,
							   slot.active->snapshot.revision,
							   slot.active->snapshot.components) ||
			    !player_revision_begin_inflight(pending->snapshot.pid,
							    pending->snapshot.revision,
							    pending->snapshot.components))
				return player_save_submit_result::revision_state_mismatch;
			retained_bytes = retained_bytes - replaced_bytes +
					 pending->snapshot.encoded_size_bound;
			slot.active = std::move(pending);
			queue_ready_locked(slot.active->snapshot.pid);
		}
		else
		{
			retained_bytes = retained_bytes - replaced_bytes +
					 pending->snapshot.encoded_size_bound;
			slot.pending = std::move(pending);
		}
	}
	catch (const std::bad_alloc &)
	{
		return player_save_submit_result::capacity_exceeded;
	}
	saturating_increment(health.coalesced);
	update_depth_health_locked();
	return player_save_submit_result::coalesced;
}

player_save_submit_result player_save_worker_submit(player_snapshot snapshot)
{
	return player_save_worker_submit_retained(&snapshot);
}

size_t player_save_worker_pulse(player_save_completion *completions_out, size_t capacity)
{
	if (capacity && !completions_out)
		return 0;
	std::unique_lock<std::mutex> lock(worker_mutex);
	size_t consumed = 0;
	while (consumed < capacity && !results.empty())
	{
		const player_save_completion completion = results.front();
		results.pop_front();
		result_available.notify_one();
		auto found = slots.find(completion.pid);
		if (found == slots.end() || !found->second.active ||
		    found->second.active->snapshot.revision != completion.revision ||
		    found->second.active->snapshot.components != completion.components)
			continue;

		pid_slot &slot = found->second;
		const uint64_t ack_at = now_usec();
		account_completion_locked(completion, ack_at);
		bool finished = false;
		switch (completion.outcome)
		{
		case player_save_apply_outcome::applied:
		case player_save_apply_outcome::already_applied:
			if (player_revision_acknowledge(completion.pid, completion.revision,
							completion.components))
			{
				saturating_increment(health.applied);
				finished = true;
			}
			else
			{
				saturating_increment(health.terminal_failures);
				player_revision_fail_inflight(completion.pid, completion.revision,
							      completion.components);
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
							      completion.components);
				finished = true;
			}
			break;
		case player_save_apply_outcome::stale_revision:
			saturating_increment(health.stale);
			player_revision_fail_inflight(completion.pid, completion.revision,
						      completion.components);
			finished = true;
			break;
		case player_save_apply_outcome::terminal_failure:
			saturating_increment(health.terminal_failures);
			player_revision_fail_inflight(completion.pid, completion.revision,
						      completion.components);
			finished = true;
			break;
		}

		if (finished)
		{
			remove_active_bytes_locked(slot);
			slot.active.reset();
			slot.dispatched = false;
			if (!promote_pending_locked(completion.pid, slot))
			{
				saturating_increment(health.terminal_failures);
				retained_bytes -= slot.pending->snapshot.encoded_size_bound;
				slot.pending.reset();
			}
			if (!slot.active && !slot.pending)
				slots.erase(found);
		}
		if (completions_out)
			completions_out[consumed] = completion;
		++consumed;
	}
	update_depth_health_locked();
	return consumed;
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
	journal_context = nullptr;
}
