#include "redis_floor_store.h"

#include <hiredis/hiredis.h>

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <memory>
#include <mutex>
#include <new>
#include <string>
#include <sys/time.h>
#include <thread>
#include <vector>

namespace
{
struct owned_mutation
{
	uint64_t uid = 0;
	std::string value;
	bool remove = false;
};

enum class job_type : uint8_t
{
	batch,
	barrier,
};

struct floor_job
{
	job_type type = job_type::batch;
	std::string key;
	std::vector<owned_mutation> mutations;
	size_t bytes = 0;
	unsigned int attempts = 0;
};

std::mutex store_mutex;
std::condition_variable work_available;
std::condition_variable store_drained;
std::deque<std::shared_ptr<floor_job>> pending_jobs;
std::thread worker_thread;
redis_floor_store_health health = {};
std::string configured_host;
int configured_port = 0;
int configured_connect_timeout_msec = 0;
int configured_command_timeout_msec = 0;
size_t pending_bytes = 0;
bool accepting = false;
bool stop_requested = false;
bool barrier_ready = false;
bool barrier_succeeded = false;
bool failure_before_barrier = false;

redisContext *connect_bounded()
{
	struct timeval connect_timeout = { configured_connect_timeout_msec / 1000,
					   (configured_connect_timeout_msec % 1000) * 1000 };
	struct timeval command_timeout = { configured_command_timeout_msec / 1000,
					   (configured_command_timeout_msec % 1000) * 1000 };
	redisContext *context =
		redisConnectWithTimeout(configured_host.c_str(), configured_port, connect_timeout);
	if (!context || context->err)
		return context;
	if (redisSetTimeout(context, command_timeout) != REDIS_OK)
	{
		redisFree(context);
		return nullptr;
	}
	return context;
}

bool execute_batch(redisContext *context, const std::shared_ptr<floor_job> &job)
{
	if (!context || context->err || !job || job->type != job_type::batch)
		return false;
	size_t appended = 0;
	for (const owned_mutation &mutation : job->mutations)
	{
		const int result = mutation.remove ?
					   redisAppendCommand(context, "HDEL %b %llu",
							      job->key.data(), job->key.size(),
							      (unsigned long long)mutation.uid) :
					   redisAppendCommand(context, "HSET %b %llu %b",
							      job->key.data(), job->key.size(),
							      (unsigned long long)mutation.uid,
							      mutation.value.data(),
							      mutation.value.size());
		if (result != REDIS_OK)
			break;
		++appended;
	}
	bool valid = appended == job->mutations.size();
	for (size_t index = 0; index < appended; ++index)
	{
		void *raw_reply = nullptr;
		if (redisGetReply(context, &raw_reply) != REDIS_OK || !raw_reply)
			return false;
		redisReply *reply = static_cast<redisReply *>(raw_reply);
		valid = valid && reply->type == REDIS_REPLY_INTEGER;
		freeReplyObject(reply);
	}
	return valid;
}

bool wait_for_retry(unsigned int delay_msec)
{
	std::unique_lock<std::mutex> lock(store_mutex);
	return !work_available.wait_for(lock, std::chrono::milliseconds(delay_msec),
					[] { return stop_requested; });
}

void remove_front_locked(bool dropped)
{
	if (pending_jobs.empty())
		return;
	pending_bytes -= pending_jobs.front()->bytes;
	pending_jobs.pop_front();
	if (dropped)
	{
		++health.dropped_batches;
		failure_before_barrier = true;
	}
	health.queued_batches = pending_jobs.size();
	health.queued_bytes = pending_bytes;
}

void worker_main()
{
	redisContext *context = nullptr;
	unsigned int reconnect_delay_msec = 100;
	for (;;)
	{
		std::shared_ptr<floor_job> job;
		{
			std::unique_lock<std::mutex> lock(store_mutex);
			work_available.wait(lock,
					    [] {
						    return stop_requested ||
							   (!health.paused &&
							    !pending_jobs.empty());
					    });
			if (stop_requested)
				break;
			job = pending_jobs.front();
			if (job->type == job_type::barrier)
			{
				remove_front_locked(false);
				if (!accepting)
				{
					health.barrier_requested = false;
					if (pending_jobs.empty())
						store_drained.notify_all();
					continue;
				}
				health.paused = true;
				barrier_succeeded = !failure_before_barrier;
				failure_before_barrier = false;
				barrier_ready = true;
				store_drained.notify_all();
				continue;
			}
			health.busy = true;
		}

		if (!context || context->err)
		{
			if (context)
				redisFree(context);
			context = connect_bounded();
			{
				std::lock_guard<std::mutex> lock(store_mutex);
				health.connected = context && !context->err;
				if (health.connected)
					++health.reconnects;
			}
			if (!context || context->err)
			{
				if (context)
				{
					redisFree(context);
					context = nullptr;
				}
				if (!wait_for_retry(reconnect_delay_msec))
					break;
				reconnect_delay_msec = std::min(reconnect_delay_msec * 2, 60000U);
				continue;
			}
			reconnect_delay_msec = 100;
		}

		if (execute_batch(context, job))
		{
			std::lock_guard<std::mutex> lock(store_mutex);
			remove_front_locked(false);
			++health.completed_batches;
			health.completed_mutations += job->mutations.size();
			health.busy = false;
			if (pending_jobs.empty())
				store_drained.notify_all();
			continue;
		}

		{
			std::lock_guard<std::mutex> lock(store_mutex);
			++health.command_failures;
			health.connected = false;
			++pending_jobs.front()->attempts;
			if (pending_jobs.front()->attempts >= REDIS_FLOOR_MAX_COMMAND_ATTEMPTS)
			{
				remove_front_locked(true);
				health.busy = false;
				if (pending_jobs.empty())
					store_drained.notify_all();
			}
		}
		redisFree(context);
		context = nullptr;
		if (!wait_for_retry(reconnect_delay_msec))
			break;
		reconnect_delay_msec = std::min(reconnect_delay_msec * 2, 60000U);
	}
	if (context)
		redisFree(context);
	std::lock_guard<std::mutex> lock(store_mutex);
	health.connected = false;
	health.busy = false;
	store_drained.notify_all();
}

void stop_worker(bool discard_pending)
{
	{
		std::lock_guard<std::mutex> lock(store_mutex);
		accepting = false;
		stop_requested = true;
	}
	work_available.notify_all();
	if (worker_thread.joinable())
		worker_thread.join();
	std::lock_guard<std::mutex> lock(store_mutex);
	if (discard_pending)
		health.dropped_batches += pending_jobs.size();
	pending_jobs.clear();
	pending_bytes = 0;
	health.queued_batches = 0;
	health.queued_bytes = 0;
	health.initialized = false;
	health.connected = false;
	health.busy = false;
	health.barrier_requested = false;
	health.paused = false;
	barrier_ready = false;
}
} // namespace

bool redis_floor_store_init(const struct redis_floor_store_config *config)
{
	if (!config || !config->host || !*config->host || config->port <= 0 ||
	    config->port > 65535 || config->connect_timeout_msec <= 0 ||
	    config->command_timeout_msec <= 0)
		return false;
	std::lock_guard<std::mutex> lock(store_mutex);
	if (health.initialized)
		return true;
	try
	{
		configured_host = config->host;
		configured_port = config->port;
		configured_connect_timeout_msec = config->connect_timeout_msec;
		configured_command_timeout_msec = config->command_timeout_msec;
		pending_jobs.clear();
		pending_bytes = 0;
		health = {};
		health.initialized = true;
		accepting = true;
		stop_requested = false;
		barrier_ready = false;
		barrier_succeeded = false;
		failure_before_barrier = false;
		worker_thread = std::thread(worker_main);
	}
	catch (...)
	{
		health = {};
		accepting = false;
		stop_requested = true;
		return false;
	}
	return true;
}

bool redis_floor_store_submit(const char *key, const struct redis_floor_mutation *mutations,
			      size_t count)
{
	if (!key || !*key || !mutations || !count || count > REDIS_FLOOR_BATCH_CAPACITY)
		return false;
	const size_t key_size = strnlen(key, REDIS_FLOOR_KEY_MAX_BYTES + 1);
	if (!key_size || key_size > REDIS_FLOOR_KEY_MAX_BYTES)
		return false;
	try
	{
		auto job = std::make_shared<floor_job>();
		job->key.assign(key, key_size);
		job->mutations.reserve(count);
		for (size_t index = 0; index < count; ++index)
		{
			const redis_floor_mutation &source = mutations[index];
			if (!source.uid ||
			    (!source.remove && (!source.value || !source.value_size ||
						source.value_size > REDIS_FLOOR_VALUE_MAX_BYTES)) ||
			    (source.remove && (source.value || source.value_size)))
				return false;
			owned_mutation mutation;
			mutation.uid = source.uid;
			mutation.remove = source.remove;
			if (!source.remove)
			{
				mutation.value.assign(reinterpret_cast<const char *>(source.value),
						      source.value_size);
				job->bytes += source.value_size;
			}
			job->mutations.push_back(std::move(mutation));
		}
		std::lock_guard<std::mutex> lock(store_mutex);
		if (!health.initialized || !accepting ||
		    pending_jobs.size() >= REDIS_FLOOR_QUEUE_CAPACITY ||
		    job->bytes > REDIS_FLOOR_QUEUE_MAX_BYTES - pending_bytes)
		{
			++health.dropped_batches;
			return false;
		}
		pending_jobs.push_back(std::move(job));
		pending_bytes += pending_jobs.back()->bytes;
		++health.submitted_batches;
		health.queued_batches = pending_jobs.size();
		health.queued_bytes = pending_bytes;
		health.high_water_batches =
			std::max(health.high_water_batches, pending_jobs.size());
		health.high_water_bytes = std::max(health.high_water_bytes, pending_bytes);
		work_available.notify_one();
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

bool redis_floor_store_request_barrier(void)
{
	std::lock_guard<std::mutex> lock(store_mutex);
	if (!health.initialized || !accepting)
		return false;
	if (health.barrier_requested)
		return true;
	if (pending_jobs.size() >= REDIS_FLOOR_QUEUE_CAPACITY)
		return false;
	try
	{
		auto job = std::make_shared<floor_job>();
		job->type = job_type::barrier;
		pending_jobs.push_back(std::move(job));
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	health.barrier_requested = true;
	health.queued_batches = pending_jobs.size();
	health.high_water_batches = std::max(health.high_water_batches, pending_jobs.size());
	work_available.notify_one();
	return true;
}

bool redis_floor_store_take_barrier(bool *succeeded)
{
	if (!succeeded)
		return false;
	std::lock_guard<std::mutex> lock(store_mutex);
	if (!barrier_ready)
		return false;
	*succeeded = barrier_succeeded;
	barrier_ready = false;
	return true;
}

void redis_floor_store_resume(void)
{
	{
		std::lock_guard<std::mutex> lock(store_mutex);
		if (!health.initialized || !health.barrier_requested)
			return;
		health.barrier_requested = false;
		health.paused = false;
		barrier_ready = false;
	}
	work_available.notify_all();
}

bool redis_floor_store_drain(uint64_t timeout_msec)
{
	std::unique_lock<std::mutex> lock(store_mutex);
	if (!health.initialized)
		return true;
	return store_drained.wait_for(lock, std::chrono::milliseconds(timeout_msec),
				      [] { return pending_jobs.empty() && !health.busy; });
}

bool redis_floor_store_shutdown(uint64_t timeout_msec)
{
	{
		std::lock_guard<std::mutex> lock(store_mutex);
		if (!health.initialized)
			return true;
		accepting = false;
		if (health.paused)
		{
			health.paused = false;
			health.barrier_requested = false;
		}
	}
	work_available.notify_all();
	const bool drained = redis_floor_store_drain(timeout_msec);
	stop_worker(!drained);
	return drained;
}

void redis_floor_store_cancel(void)
{
	{
		std::lock_guard<std::mutex> lock(store_mutex);
		if (!health.initialized)
			return;
	}
	stop_worker(true);
}

struct redis_floor_store_health redis_floor_store_health_copy(void)
{
	std::lock_guard<std::mutex> lock(store_mutex);
	return health;
}

void redis_floor_store_reset_for_tests(void)
{
	redis_floor_store_cancel();
	std::lock_guard<std::mutex> lock(store_mutex);
	pending_jobs.clear();
	health = {};
	configured_host.clear();
	configured_port = 0;
	configured_connect_timeout_msec = 0;
	configured_command_timeout_msec = 0;
	pending_bytes = 0;
	accepting = false;
	stop_requested = false;
	barrier_ready = false;
	barrier_succeeded = false;
	failure_before_barrier = false;
}
