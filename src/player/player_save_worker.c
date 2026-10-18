/****************************************************************************
 *
 *  File: player_save_worker.c                                  Part of Duris
 *  Usage: the player save writer thread and its queue
 *
 *  Copyright 1990, 1991 - see LICENSE for complete information
 *
 *  Additions since 2025 by the Duris maintainers and since 2026 by
 *    LuminariMUD maintainers are public domain (Unlicense, see LICENSE)
 *
 *  Created by: Duris, LuminariMUD, Zusuk                  Date: 2026-09-23
 *
 ****************************************************************************/

#include "player/player_save_worker.h"
#include "sql/sql_thread_init.h"

#include "persistence/persistence_observability.h"

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <new>
#include <thread>
#include <utility>

#ifndef __NO_MYSQL__
#include <mysql/mysql.h>
#endif

namespace
{
struct queued_job
{
	persistence_job_kind kind = persistence_job_kind::player;
	uint64_t owner = 0;
	player_snapshot snapshot;
	persistence_job_write_fn write;
	size_t bytes = 0;
	uint64_t queued_at_usec = 0;
};

using job_list = std::list<queued_job>;

std::mutex worker_mutex;
std::condition_variable job_available;
std::condition_variable writer_idle;
std::condition_variable retry_wakeup;
// Capture order. Each owner has at most one queued job here, indexed below.
job_list queue;
std::map<persistence_job_owner, job_list::iterator> queued_by_owner;
// The job being written, owned by the writer thread while set.
std::unique_ptr<queued_job> inflight;
std::deque<player_save_completion> results;
std::thread writer;
player_save_apply_fn apply_callback = nullptr;
void *apply_context = nullptr;
player_save_worker_health health = {};
size_t queued_bytes = 0;
bool stop_requested = false;
// False while a writer thread runs, including one shutdown left behind.
bool writer_exited = true;

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

bool succeeded(player_save_apply_outcome outcome)
{
	return outcome == player_save_apply_outcome::applied ||
	       outcome == player_save_apply_outcome::already_applied ||
	       outcome == player_save_apply_outcome::stale_revision;
}

bool connection_lost(player_save_apply_outcome outcome)
{
	return outcome == player_save_apply_outcome::retryable_failure ||
	       outcome == player_save_apply_outcome::ambiguous_commit;
}

void update_depth_health_locked()
{
	health.queued_jobs = queue.size();
	health.inflight_jobs = inflight ? 1 : 0;
	health.completion_jobs = results.size();
	health.queued_bytes = queued_bytes;
	health.inflight_bytes = inflight ? inflight->bytes : 0;
	update_max(health.high_water_jobs,
		   health.queued_jobs + health.inflight_jobs + health.completion_jobs);
	update_max(health.high_water_bytes, queued_bytes + health.inflight_bytes);
}

player_save_apply_result write_job(const queued_job &job)
{
	try
	{
		if (job.kind == persistence_job_kind::player)
			return apply_callback(job.snapshot, apply_context);
		return job.write();
	}
	catch (const std::bad_alloc &)
	{
		return { player_save_apply_outcome::terminal_failure, 0, ENOMEM };
	}
	catch (...)
	{
		return { player_save_apply_outcome::terminal_failure, 0, EFAULT };
	}
}

void writer_main()
{
#ifndef __NO_MYSQL__
	const bool mysql_ready = sql_worker_thread_init() == 0;
#else
	const bool mysql_ready = true;
#endif
	for (;;)
	{
		{
			std::unique_lock<std::mutex> lock(worker_mutex);
			job_available.wait(lock, [] { return stop_requested || !queue.empty(); });
			if (stop_requested)
				break;
			// An owner can have an older save queued ahead of its newest one; the
			// index names the newest.
			const auto owned =
				queued_by_owner.find({ queue.front().kind, queue.front().owner });
			if (owned != queued_by_owner.end() && owned->second == queue.begin())
				queued_by_owner.erase(owned);
			inflight = std::make_unique<queued_job>(std::move(queue.front()));
			queue.pop_front();
			queued_bytes -= inflight->bytes;
			update_depth_health_locked();
		}

		const uint64_t started = now_usec();
		unsigned int retries = 0;
		uint64_t delay_msec = PLAYER_SAVE_WORKER_RETRY_INITIAL_MSEC;
		player_save_apply_result applied = {};
		for (;;)
		{
			applied = mysql_ready ?
					  write_job(*inflight) :
					  player_save_apply_result{
						  player_save_apply_outcome::retryable_failure, 0,
						  ENOTCONN
					  };
			if (!connection_lost(applied.outcome))
				break;
			// Order is the whole point of one writer: keep this job at the head
			// and try again rather than letting a later save overtake it.
			std::unique_lock<std::mutex> lock(worker_mutex);
			saturating_increment(health.connection_retries);
			++retries;
			if (retry_wakeup.wait_for(lock, std::chrono::milliseconds(delay_msec),
						  [] { return stop_requested; }))
				break;
			delay_msec = std::min(delay_msec * 2, PLAYER_SAVE_WORKER_RETRY_MAX_MSEC);
		}
		const uint64_t completed = now_usec();

		std::lock_guard<std::mutex> lock(worker_mutex);
		const queued_job &job = *inflight;
		player_save_completion completion = {
			.kind = job.kind,
			.owner = job.owner,
			.pid = job.kind == persistence_job_kind::player ? job.snapshot.pid : 0,
			.revision = job.kind == persistence_job_kind::player ?
					    job.snapshot.revision :
					    0,
			.components = job.kind == persistence_job_kind::player ?
					      job.snapshot.components :
					      0,
			.outcome = applied.outcome,
			.durable_revision = applied.durable_revision ?
						    applied.durable_revision :
						    (job.kind == persistence_job_kind::player ?
							     job.snapshot.revision :
							     0),
			.error_code = applied.error_code,
			.retry_count = retries,
			.queued_at_usec = job.queued_at_usec,
			.started_at_usec = started,
			.completed_at_usec = completed,
			.failed_field = applied.failed_field,
			.rolled_back = applied.rolled_back,
		};
		update_max(health.max_capture_to_apply_usec, started - job.queued_at_usec);
		update_max(health.max_apply_usec, completed - started);
		if (succeeded(applied.outcome))
			saturating_increment(health.applied);
		else if (!connection_lost(applied.outcome))
			saturating_increment(health.failures);
		// An abandoned retry at shutdown stays pending: the caller names it.
		if (!stop_requested || !connection_lost(applied.outcome))
		{
			try
			{
				results.push_back(completion);
			}
			catch (const std::bad_alloc &)
			{
			}
			inflight.reset();
		}
		update_depth_health_locked();
		writer_idle.notify_all();
		if (stop_requested)
			break;
	}
#ifndef __NO_MYSQL__
	if (mysql_ready)
		mysql_thread_end();
#endif
	std::lock_guard<std::mutex> lock(worker_mutex);
	writer_exited = true;
	writer_idle.notify_all();
}

player_save_submit_result enqueue(queued_job job)
{
	std::lock_guard<std::mutex> lock(worker_mutex);
	if (!health.running || stop_requested)
	{
		errno = ESHUTDOWN;
		return player_save_submit_result::unavailable;
	}
	const persistence_job_owner key = { job.kind, job.owner };
	const auto found = queued_by_owner.find(key);
	const bool replace_tail = found != queued_by_owner.end() &&
				  found->second == std::prev(queue.end());
	const size_t retained_bytes = queued_bytes + (inflight ? inflight->bytes : 0) -
				      (replace_tail ? found->second->bytes : 0);
	const size_t pending_jobs = queue.size() + (inflight ? 1 : 0) + results.size();
	if (job.bytes > PLAYER_SAVE_WORKER_MAX_PENDING_BYTES ||
	    retained_bytes > PLAYER_SAVE_WORKER_MAX_PENDING_BYTES - job.bytes ||
	    (!replace_tail && pending_jobs >= PLAYER_SAVE_WORKER_MAX_PENDING_JOBS))
	{
		errno = ENOSPC;
		return player_save_submit_result::unavailable;
	}
	bool replaced = false;
	try
	{
		// Replace only the tail, without erasing its old capture before a possible
		// allocation failure. A new owner index and list entry are admitted together.
		job.queued_at_usec = now_usec();
		const size_t bytes = job.bytes;
		if (replace_tail)
		{
			queued_bytes -= found->second->bytes;
			*found->second = std::move(job);
			replaced = true;
		}
		else
		{
			queue.push_back(std::move(job));
			try
			{
				queued_by_owner.insert_or_assign(key, std::prev(queue.end()));
			}
			catch (...)
			{
				queue.pop_back();
				throw;
			}
		}
		queued_bytes += bytes;
	}
	catch (const std::bad_alloc &)
	{
		errno = ENOMEM;
		return player_save_submit_result::unavailable;
	}
	saturating_increment(replaced ? health.replaced : health.submitted);
	update_depth_health_locked();
	job_available.notify_one();
	return replaced ? player_save_submit_result::replaced : player_save_submit_result::accepted;
}

bool idle_locked()
{
	return queue.empty() && !inflight;
}
} // namespace

const char *persistence_job_kind_name(persistence_job_kind kind)
{
	switch (kind)
	{
	case persistence_job_kind::player:
		return "player";
	case persistence_job_kind::corpse:
		return "corpse";
	case persistence_job_kind::locker:
		return "locker";
	case persistence_job_kind::saved_item:
		return "saved_item";
	case persistence_job_kind::log:
		return "log";
	case persistence_job_kind::critical:
		return "critical";
	case persistence_job_kind::bank:
		return "bank";
	case persistence_job_kind::shopkeeper:
		return "shopkeeper";
	case persistence_job_kind::sql:
		return "sql";
	case persistence_job_kind::artifact:
		return "artifact";
	case persistence_job_kind::economy:
		return "economy";
	}
	return "unknown";
}

bool player_save_worker_init(player_save_apply_fn apply, void *context)
{
	if (!apply)
		return false;
	{
		std::lock_guard<std::mutex> lock(worker_mutex);
		if (health.running || writer.joinable() || !writer_exited)
			return false;
		apply_callback = apply;
		apply_context = context;
		stop_requested = false;
		health.running = true;
		health.stop_pending = false;
		writer_exited = false;
	}
	try
	{
		writer = std::thread(writer_main);
	}
	catch (const std::system_error &)
	{
		std::lock_guard<std::mutex> lock(worker_mutex);
		writer_exited = true;
		health.running = false;
		apply_callback = nullptr;
		apply_context = nullptr;
		return false;
	}
	return true;
}

void player_save_worker_shutdown(void (*interrupt)(void))
{
	bool writing = false;
	{
		std::lock_guard<std::mutex> lock(worker_mutex);
		stop_requested = true;
		health.stop_pending = true;
		writing = inflight != nullptr;
		job_available.notify_all();
		retry_wakeup.notify_all();
	}
	// A job still being written is cut short: its database call returns as a lost
	// connection, and the stopping writer leaves it pending. Opening a new connection
	// cannot be cut short, so a writer still in one after the grace is left behind.
	if (writing && interrupt)
	{
		interrupt();
		std::unique_lock<std::mutex> lock(worker_mutex);
		if (!writer_idle.wait_for(
			    lock, std::chrono::milliseconds(PLAYER_SAVE_WORKER_STOP_GRACE_MSEC),
			    [] { return writer_exited; }))
		{
			lock.unlock();
			writer.detach();
		}
	}
	if (writer.joinable())
		writer.join();
	std::lock_guard<std::mutex> lock(worker_mutex);
	health.running = false;
	health.stop_pending = false;
	apply_callback = nullptr;
	apply_context = nullptr;
	writer_idle.notify_all();
}

player_save_submit_result player_save_worker_submit(player_snapshot snapshot)
{
	if (!valid_snapshot(snapshot))
		return player_save_submit_result::invalid;
	queued_job job;
	job.kind = persistence_job_kind::player;
	job.owner = static_cast<uint64_t>(snapshot.pid);
	job.bytes = snapshot.encoded_size_bound;
	job.snapshot = std::move(snapshot);
	return enqueue(std::move(job));
}

player_save_submit_result player_save_worker_submit_retained(player_snapshot *snapshot)
{
	if (!snapshot)
		return player_save_submit_result::invalid;
	return player_save_worker_submit(std::move(*snapshot));
}

player_save_submit_result persistence_writer_submit(persistence_job_kind kind, uint64_t owner,
						    size_t bytes, persistence_job_write_fn write)
{
	if (kind == persistence_job_kind::player || !owner || !write)
		return player_save_submit_result::invalid;
	// Value-initialized: the job carries no snapshot, whose bool would otherwise be
	// indeterminate when the job is moved.
	queued_job job = {};
	job.kind = kind;
	job.owner = owner;
	job.bytes = bytes;
	job.write = std::move(write);
	return enqueue(std::move(job));
}

size_t player_save_worker_pulse(player_save_completion *completions_out, size_t capacity)
{
	if (capacity && !completions_out)
		return 0;
	std::lock_guard<std::mutex> lock(worker_mutex);
	size_t consumed = 0;
	while (consumed < capacity && !results.empty())
	{
		completions_out[consumed++] = results.front();
		results.pop_front();
	}
	update_depth_health_locked();
	return consumed;
}

bool player_save_worker_pid_pending(int pid)
{
	if (pid <= 0)
		return false;
	std::lock_guard<std::mutex> lock(worker_mutex);
	if (inflight && inflight->kind == persistence_job_kind::player &&
	    inflight->snapshot.pid == pid)
		return true;
	if (queued_by_owner.count({ persistence_job_kind::player, static_cast<uint64_t>(pid) }))
		return true;
	for (const auto &completion : results)
		if (completion.kind == persistence_job_kind::player && completion.pid == pid)
			return true;
	return false;
}

bool persistence_writer_pending(persistence_job_kind kind, uint64_t owner)
{
	std::lock_guard<std::mutex> lock(worker_mutex);
	if (inflight && inflight->kind == kind && inflight->owner == owner)
		return true;
	return queued_by_owner.count({ kind, owner }) != 0;
}

bool persistence_writer_wait_idle(uint64_t timeout_msec)
{
	std::unique_lock<std::mutex> lock(worker_mutex);
	return writer_idle.wait_for(lock, std::chrono::milliseconds(timeout_msec),
				    [] { return idle_locked() || !health.running; }) &&
	       idle_locked();
}

std::vector<persistence_job_owner> persistence_writer_pending_owners(void)
{
	std::vector<persistence_job_owner> owners;
	std::lock_guard<std::mutex> lock(worker_mutex);
	try
	{
		if (inflight)
			owners.emplace_back(inflight->kind, inflight->owner);
		for (const queued_job &job : queue)
			owners.emplace_back(job.kind, job.owner);
	}
	catch (const std::bad_alloc &)
	{
	}
	return owners;
}

player_save_worker_health player_save_worker_health_copy(void)
{
	std::lock_guard<std::mutex> lock(worker_mutex);
	player_save_worker_health snapshot = health;
	const uint64_t current = now_usec();
	uint64_t oldest = 0;
	if (inflight && current >= inflight->queued_at_usec)
		oldest = (current - inflight->queued_at_usec) / 1000;
	if (!queue.empty() && current >= queue.front().queued_at_usec)
		update_max(oldest, (current - queue.front().queued_at_usec) / 1000);
	snapshot.oldest_age_msec = oldest;
	snapshot.age_limit_exceeded = oldest > PLAYER_SAVE_WORKER_MAX_AGE_MSEC;
	return snapshot;
}

void player_save_worker_reset_for_tests(void)
{
	player_save_worker_shutdown();
	std::unique_lock<std::mutex> lock(worker_mutex);
	writer_idle.wait(lock, [] { return writer_exited; });
	queue.clear();
	queued_by_owner.clear();
	inflight.reset();
	results.clear();
	queued_bytes = 0;
	stop_requested = false;
	health = {};
}
