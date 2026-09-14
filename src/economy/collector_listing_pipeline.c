#include "economy/collector_listing_pipeline.h"

#include "economy/collector_catalog_source.h"

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <new>
#include <string>
#include <thread>
#include <unordered_set>
#include <utility>

namespace
{
std::mutex pipeline_mutex;
std::condition_variable work_available;
std::deque<collector_listing_request> jobs;
std::deque<collector_listing_result> completions;
std::unordered_set<uint64_t> active_ids;
std::unordered_set<uint64_t> cancelled_ids;
std::thread worker;
collector_listing_execute_fn execute_callback = nullptr;
void *execute_context = nullptr;
collector_listing_pipeline_health health = {};
uint64_t inflight_id = 0;
bool stop_requested = false;
std::atomic<uint64_t> next_request_id = 1;

bool permanent_error(unsigned int error_code)
{
	return error_code == EINVAL || error_code == EBADMSG || error_code == E2BIG ||
	       error_code == ERANGE;
}

collector_listing_result execute_selected(const collector_listing_request &request, void *)
{
	collector_listing_result result;
	result.request_id = request.request_id;
	result.listing = request.listing;
	result.consumer = request.consumer;
	bool found = false;
	std::string error;
	if (collector_listing_source_load(request.listing, result.detail, found, result.error_code,
					  error))
		result.outcome = found ? collector_listing_outcome::found :
					 collector_listing_outcome::not_found;
	else
		result.outcome = permanent_error(result.error_code) ?
					 collector_listing_outcome::invalid_data :
					 collector_listing_outcome::retryable_failure;
	return result;
}

bool valid_found_detail(const collector_listing_request &request,
			const collector_listing_result &result)
{
	if (result.detail.entry.listing != request.listing ||
	    !collector::valid_record(result.detail.entry) ||
	    result.detail.item_blob.size() > ITEM_TRANSFER_ITEM_BLOB_MAX_BYTES)
		return false;
	const bool held = result.detail.entry.status == collector::state::collected ||
			  result.detail.entry.status == collector::state::available;
	return !held || !result.detail.item_blob.empty();
}

bool known_outcome(collector_listing_outcome outcome)
{
	switch (outcome)
	{
	case collector_listing_outcome::found:
	case collector_listing_outcome::not_found:
	case collector_listing_outcome::retryable_failure:
	case collector_listing_outcome::invalid_data:
	case collector_listing_outcome::cancelled:
		return true;
	}
	return false;
}

bool known_consumer(collector_listing_consumer consumer)
{
	switch (consumer)
	{
	case collector_listing_consumer::player:
	case collector_listing_consumer::maintenance:
		return true;
	}
	return false;
}

void refresh_health_locked()
{
	health.queued = jobs.size();
	health.inflight = inflight_id ? 1 : 0;
	health.completions = completions.size();
	health.high_water = std::max(health.high_water, health.queued + health.inflight);
}

void record_result_locked(const collector_listing_result &result)
{
	switch (result.outcome)
	{
	case collector_listing_outcome::found:
		++health.found;
		break;
	case collector_listing_outcome::not_found:
		++health.not_found;
		break;
	case collector_listing_outcome::retryable_failure:
		++health.retryable_failures;
		break;
	case collector_listing_outcome::invalid_data:
		++health.invalid_data;
		break;
	case collector_listing_outcome::cancelled:
		break;
	}
}

void worker_main()
{
	for (;;)
	{
		collector_listing_request request;
		{
			std::unique_lock<std::mutex> lock(pipeline_mutex);
			work_available.wait(lock, [] { return stop_requested || !jobs.empty(); });
			if (stop_requested && jobs.empty())
				break;
			request = jobs.front();
			jobs.pop_front();
			inflight_id = request.request_id;
			refresh_health_locked();
		}
		collector_listing_result result;
		result.request_id = request.request_id;
		result.listing = request.listing;
		result.consumer = request.consumer;
		{
			std::lock_guard<std::mutex> lock(pipeline_mutex);
			if (cancelled_ids.count(request.request_id))
				result.outcome = collector_listing_outcome::cancelled;
		}
		if (result.outcome != collector_listing_outcome::cancelled)
			try
			{
				result = execute_callback(request, execute_context);
			}
			catch (const std::bad_alloc &)
			{
				result.outcome = collector_listing_outcome::retryable_failure;
				result.error_code = ENOMEM;
			}
			catch (...)
			{
				result.outcome = collector_listing_outcome::invalid_data;
				result.error_code = EFAULT;
			}
		const bool identity_mismatch = result.request_id != request.request_id ||
					       result.listing != request.listing ||
					       result.consumer != request.consumer;
		result.request_id = request.request_id;
		result.listing = request.listing;
		result.consumer = request.consumer;
		if (identity_mismatch || !known_consumer(request.consumer) ||
		    !known_outcome(result.outcome) ||
		    (result.outcome == collector_listing_outcome::found &&
		     (result.error_code || !valid_found_detail(request, result))))
		{
			result.outcome = collector_listing_outcome::invalid_data;
			result.error_code = EBADMSG;
			result.detail = {};
		}
		else if (result.outcome != collector_listing_outcome::found)
		{
			result.detail = {};
			if (result.outcome == collector_listing_outcome::retryable_failure &&
			    !result.error_code)
				result.error_code = EIO;
			else if (result.outcome == collector_listing_outcome::invalid_data &&
				 !result.error_code)
				result.error_code = EBADMSG;
		}
		{
			std::lock_guard<std::mutex> lock(pipeline_mutex);
			if (cancelled_ids.erase(request.request_id))
			{
				result.outcome = collector_listing_outcome::cancelled;
				result.detail = {};
			}
			inflight_id = 0;
			record_result_locked(result);
			completions.push_back(std::move(result));
			refresh_health_locked();
		}
	}
}
} // namespace

bool collector_listing_pipeline_init(collector_listing_execute_fn execute, void *context)
{
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	if (health.running || worker.joinable())
		return false;
	execute_callback = execute ? execute : execute_selected;
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
		execute_context = nullptr;
		return false;
	}
	return true;
}

uint64_t collector_listing_pipeline_next_request_id(void)
{
	uint64_t request_id = next_request_id.fetch_add(1, std::memory_order_relaxed);
	if (!request_id)
		request_id = next_request_id.fetch_add(1, std::memory_order_relaxed);
	return request_id;
}

collector_listing_submit_outcome
collector_listing_pipeline_submit(const collector_listing_request &request)
{
	if (!request.request_id || !request.listing || !known_consumer(request.consumer))
		return collector_listing_submit_outcome::invalid;
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	if (!health.running || stop_requested || !execute_callback)
		return collector_listing_submit_outcome::unavailable;
	if (active_ids.count(request.request_id))
		return collector_listing_submit_outcome::duplicate;
	if (active_ids.size() >= COLLECTOR_LISTING_MAX_PENDING ||
	    completions.size() >= COLLECTOR_LISTING_MAX_COMPLETIONS)
		return collector_listing_submit_outcome::capacity_exceeded;
	try
	{
		active_ids.insert(request.request_id);
		jobs.push_back(request);
	}
	catch (const std::bad_alloc &)
	{
		active_ids.erase(request.request_id);
		return collector_listing_submit_outcome::capacity_exceeded;
	}
	++health.submitted;
	refresh_health_locked();
	work_available.notify_one();
	return collector_listing_submit_outcome::accepted;
}

bool collector_listing_pipeline_cancel(uint64_t request_id)
{
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	if (!request_id || !active_ids.count(request_id))
		return false;
	if (cancelled_ids.insert(request_id).second)
	{
		++health.cancelled;
		for (collector_listing_result &result : completions)
			if (result.request_id == request_id)
			{
				result.outcome = collector_listing_outcome::cancelled;
				result.detail = {};
				break;
			}
	}
	return true;
}

size_t collector_listing_pipeline_pulse(collector_listing_result *results, size_t capacity)
{
	if (!results || !capacity)
		return 0;
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	size_t count = 0;
	while (count < capacity && !completions.empty())
	{
		results[count] = std::move(completions.front());
		active_ids.erase(results[count].request_id);
		cancelled_ids.erase(results[count].request_id);
		completions.pop_front();
		++health.delivered;
		++count;
	}
	refresh_health_locked();
	return count;
}

size_t collector_listing_pipeline_pulse_for(collector_listing_consumer consumer,
					    collector_listing_result *results, size_t capacity)
{
	if (!known_consumer(consumer) || !results || !capacity)
		return 0;
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	size_t count = 0;
	for (auto current = completions.begin(); current != completions.end() && count < capacity;)
	{
		if (current->consumer != consumer)
		{
			++current;
			continue;
		}
		results[count] = std::move(*current);
		active_ids.erase(results[count].request_id);
		cancelled_ids.erase(results[count].request_id);
		current = completions.erase(current);
		++health.delivered;
		++count;
	}
	refresh_health_locked();
	return count;
}

void collector_listing_pipeline_shutdown(void)
{
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		stop_requested = true;
		health.stop_pending = true;
		for (const auto &request : jobs)
			cancelled_ids.insert(request.request_id);
		if (inflight_id)
			cancelled_ids.insert(inflight_id);
		work_available.notify_all();
	}
	if (worker.joinable())
		worker.join();
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	jobs.clear();
	completions.clear();
	active_ids.clear();
	cancelled_ids.clear();
	inflight_id = 0;
	stop_requested = false;
	health.running = false;
	health.stop_pending = false;
	execute_callback = nullptr;
	execute_context = nullptr;
	refresh_health_locked();
}

collector_listing_pipeline_health collector_listing_pipeline_health_copy(void)
{
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	refresh_health_locked();
	return health;
}

void collector_listing_pipeline_reset_for_tests(void)
{
	collector_listing_pipeline_shutdown();
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	health = {};
	next_request_id.store(1, std::memory_order_relaxed);
}
