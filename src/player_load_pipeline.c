#include "player_load_pipeline.h"
#include "sql_thread_init.h"

#include "flatfile_player_repository.h"
#include "persistence_observability.h"
#include "sql_pool.h"

#ifndef __NO_MYSQL__
#include <mysql/mysql.h>
#endif

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <ctime>
#include <deque>
#include <mutex>
#include <new>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace
{
struct queued_load
{
	player_load_request request;
	uint64_t queued_at_usec = 0;
};

std::mutex pipeline_mutex;
std::condition_variable work_available;
std::condition_variable completion_available;
std::deque<queued_load> jobs;
std::deque<player_load_result> completions;
std::unordered_set<uint64_t> active_ids;
std::unordered_set<uint64_t> cancelled_ids;
std::unordered_map<uint64_t, uint64_t> submitted_at;
std::thread worker;
player_load_execute_fn execute_callback = nullptr;
void *execute_context = nullptr;
player_load_pipeline_health health = {};
uint64_t inflight_id = 0;
bool stop_requested = false;
std::atomic<uint64_t> next_request_id = 1;

uint64_t now_usec()
{
	return persistence_observability_now_usec();
}

struct pool_connection_guard
{
	MYSQL *connection;

	~pool_connection_guard()
	{
		if (connection)
			sql_pool_release(connection);
	}
};

#ifndef __NO_MYSQL__
player_load_result execute_repository(const player_load_request &request, void *)
{
	player_load_result result = {};
	result.request_id = request.request_id;
	result.pid = request.pid;
	MYSQL *connection = sql_pool_acquire();
	if (!connection)
	{
		result.outcome = player_load_outcome::retryable_failure;
		return result;
	}
	pool_connection_guard guard = { connection };
	try
	{
		result = player_load_repository_execute(connection, request);
	}
	catch (...)
	{
		mysql_rollback(connection);
		throw;
	}
	return result;
}
#endif

player_load_execute_fn selected_execute_callback()
{
#ifdef __NO_MYSQL__
	return flatfile_player_load_repository_execute_selected;
#else
	return execute_repository;
#endif
}

void refresh_health_locked()
{
	health.queued = jobs.size();
	health.inflight = inflight_id ? 1 : 0;
	health.completions = completions.size();
	health.high_water = std::max(health.high_water, health.queued + health.inflight);
	if (!jobs.empty())
		health.oldest_age_msec = (now_usec() - jobs.front().queued_at_usec) / 1000;
	else
		health.oldest_age_msec = 0;
}

void record_result_locked(const player_load_result &result)
{
	switch (result.outcome)
	{
	case player_load_outcome::applied:
		++health.applied;
		break;
	case player_load_outcome::retryable_failure:
		++health.retryable_failures;
		break;
	case player_load_outcome::component_failure:
	case player_load_outcome::not_found:
		++health.component_failures;
		break;
	case player_load_outcome::limit_exceeded:
		++health.limit_exceeded;
		break;
	case player_load_outcome::timed_out:
		++health.timed_out;
		break;
	case player_load_outcome::cancelled:
		break;
	case player_load_outcome::stale:
		++health.stale;
		break;
	}
	if (result.outcome != player_load_outcome::cancelled)
	{
		health.last_transaction_usec = result.metrics.transaction_usec;
		health.last_snapshot_bytes = result.metrics.byte_count;
		const time_t wall_now = time(nullptr);
		health.last_snapshot_age_sec = result.saved_at > 0 && result.saved_at <= wall_now ?
						       wall_now - result.saved_at :
						       0;
		health.last_query_count = result.metrics.query_count;
		health.last_row_count = result.metrics.row_count;
	}
}

void record_delivery_locked(uint64_t request_id)
{
	auto found = submitted_at.find(request_id);
	if (found == submitted_at.end())
		return;
	health.last_completion_latency_usec = now_usec() - found->second;
	health.max_completion_latency_usec =
		std::max(health.max_completion_latency_usec, health.last_completion_latency_usec);
	submitted_at.erase(found);
}

void worker_main()
{
#ifndef __NO_MYSQL__
	if (sql_worker_thread_init() != 0)
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		health.running = false;
		stop_requested = true;
		completion_available.notify_all();
		return;
	}
#endif
	for (;;)
	{
		queued_load job = {};
		{
			std::unique_lock<std::mutex> lock(pipeline_mutex);
			work_available.wait(lock, [] { return stop_requested || !jobs.empty(); });
			if (stop_requested && jobs.empty())
				break;
			job = std::move(jobs.front());
			jobs.pop_front();
			inflight_id = job.request.request_id;
			refresh_health_locked();
		}
		player_load_result result = {};
		result.request_id = job.request.request_id;
		result.pid = job.request.pid;
		try
		{
			std::lock_guard<std::mutex> lock(pipeline_mutex);
			if (cancelled_ids.count(job.request.request_id))
				result.outcome = player_load_outcome::cancelled;
		}
		catch (...)
		{
			result.outcome = player_load_outcome::retryable_failure;
		}
		if (result.outcome != player_load_outcome::cancelled)
			try
			{
				result = execute_callback(job.request, execute_context);
			}
			catch (const std::bad_alloc &)
			{
				result.outcome = player_load_outcome::retryable_failure;
				result.error_code = ENOMEM;
			}
			catch (...)
			{
				result.outcome = player_load_outcome::component_failure;
				result.error_code = EFAULT;
			}
		const bool identity_mismatch =
			result.outcome == player_load_outcome::applied &&
			(result.request_id != job.request.request_id ||
			 (job.request.pid > 0 && result.pid != job.request.pid) || result.pid <= 0);
		result.request_id = job.request.request_id;
		if (identity_mismatch)
		{
			result.outcome = player_load_outcome::stale;
		}
		{
			std::lock_guard<std::mutex> lock(pipeline_mutex);
			if (cancelled_ids.erase(job.request.request_id))
				result.outcome = player_load_outcome::cancelled;
			inflight_id = 0;
			record_result_locked(result);
			completions.push_back(std::move(result));
			refresh_health_locked();
			completion_available.notify_all();
		}
	}
#ifndef __NO_MYSQL__
	mysql_thread_end();
#endif
}
} // namespace

bool player_load_pipeline_init(player_load_execute_fn execute, void *context)
{
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	if (health.running || worker.joinable())
		return false;
	execute_callback = execute ? execute : selected_execute_callback();
	execute_context = context;
	stop_requested = false;
	health.running = true;
	health.stop_pending = false;
	try
	{
		worker = std::thread(worker_main);
	}
	catch (...)
	{
		health.running = false;
		execute_callback = nullptr;
		return false;
	}
	return true;
}

uint64_t player_load_pipeline_next_request_id(void)
{
	uint64_t request_id = next_request_id.fetch_add(1, std::memory_order_relaxed);
	if (!request_id)
		request_id = next_request_id.fetch_add(1, std::memory_order_relaxed);
	return request_id;
}

void player_load_pipeline_shutdown(void)
{
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		stop_requested = true;
		health.stop_pending = true;
		for (const queued_load &job : jobs)
			cancelled_ids.insert(job.request.request_id);
		if (inflight_id)
			cancelled_ids.insert(inflight_id);
		work_available.notify_all();
		completion_available.notify_all();
	}
	if (worker.joinable())
		worker.join();
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	jobs.clear();
	completions.clear();
	active_ids.clear();
	cancelled_ids.clear();
	submitted_at.clear();
	inflight_id = 0;
	health.running = false;
	health.stop_pending = false;
	execute_callback = nullptr;
	execute_context = nullptr;
	refresh_health_locked();
}

player_load_submit_outcome player_load_pipeline_submit(player_load_request request)
{
	const uint64_t now = now_usec();
	const uint64_t request_id = request.request_id;
	if (!player_load_request_valid(request, now))
		return player_load_submit_outcome::invalid;
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	if (!health.running || stop_requested || !execute_callback)
		return player_load_submit_outcome::unavailable;
	if (active_ids.count(request.request_id))
		return player_load_submit_outcome::duplicate;
	if (active_ids.size() >= PLAYER_LOAD_MAX_PENDING ||
	    completions.size() >= PLAYER_LOAD_MAX_COMPLETIONS)
		return player_load_submit_outcome::capacity_exceeded;
	try
	{
		active_ids.insert(request_id);
		submitted_at.emplace(request_id, now);
		jobs.push_back({ std::move(request), now });
	}
	catch (const std::bad_alloc &)
	{
		active_ids.erase(request_id);
		submitted_at.erase(request_id);
		return player_load_submit_outcome::capacity_exceeded;
	}
	++health.submitted;
	refresh_health_locked();
	work_available.notify_one();
	return player_load_submit_outcome::accepted;
}

bool player_load_pipeline_cancel(uint64_t request_id)
{
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	if (!request_id || !active_ids.count(request_id))
		return false;
	if (cancelled_ids.insert(request_id).second)
		++health.cancelled;
	return true;
}

size_t player_load_pipeline_pulse(player_load_result *results_out, size_t capacity)
{
	if (!results_out || !capacity)
		return 0;
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	size_t count = 0;
	while (count < capacity && !completions.empty())
	{
		results_out[count] = std::move(completions.front());
		active_ids.erase(results_out[count].request_id);
		record_delivery_locked(results_out[count].request_id);
		completions.pop_front();
		++count;
	}
	refresh_health_locked();
	completion_available.notify_all();
	return count;
}

bool player_load_pipeline_wait(player_load_request request, player_load_result *result_out,
			       uint64_t timeout_msec)
{
	if (!result_out || !timeout_msec ||
	    player_load_pipeline_submit(request) != player_load_submit_outcome::accepted)
		return false;
	const auto deadline =
		std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_msec);
	std::unique_lock<std::mutex> lock(pipeline_mutex);
	for (;;)
	{
		for (auto found = completions.begin(); found != completions.end(); ++found)
			if (found->request_id == request.request_id)
			{
				*result_out = std::move(*found);
				completions.erase(found);
				active_ids.erase(request.request_id);
				record_delivery_locked(request.request_id);
				refresh_health_locked();
				return true;
			}
		if (stop_requested)
			return false;
		if (completion_available.wait_until(lock, deadline) == std::cv_status::timeout)
		{
			if (cancelled_ids.insert(request.request_id).second)
				++health.cancelled;
			return false;
		}
	}
}

player_load_pipeline_health player_load_pipeline_health_copy(void)
{
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	refresh_health_locked();
	return health;
}

void player_load_pipeline_note_stale(void)
{
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	++health.stale;
}

void player_load_pipeline_reset_for_tests(void)
{
	player_load_pipeline_shutdown();
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	health = {};
}
