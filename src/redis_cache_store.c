#include "redis_cache_store.h"

#include <hiredis/hiredis.h>

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdarg>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <new>
#include <string>
#include <sys/time.h>
#include <thread>

namespace
{
enum class cache_operation : uint8_t
{
	set,
	remove,
};

struct cache_job
{
	cache_operation operation = cache_operation::remove;
	std::string key;
	std::shared_ptr<const std::string> value;
	int ttl_seconds = 0;
	unsigned int attempts = 0;
};

struct local_cache_entry
{
	std::shared_ptr<const std::string> value;
	std::chrono::steady_clock::time_point expires = {};
};

std::mutex store_mutex;
std::condition_variable work_available;
std::condition_variable store_drained;
std::deque<std::shared_ptr<cache_job>> pending_jobs;
std::map<std::string, local_cache_entry> local_cache;
std::thread worker_thread;
redis_cache_store_health health = {};
std::string configured_host;
int configured_port = 0;
int configured_connect_timeout_msec = 0;
int configured_command_timeout_msec = 0;
size_t pending_bytes = 0;
bool accepting = false;
bool stop_requested = false;

size_t job_bytes(const std::shared_ptr<cache_job> &job)
{
	return job && job->value ? job->value->size() : 0;
}

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

redisReply *command(redisContext *context, const char *format, ...)
{
	if (!context || context->err || !format)
		return nullptr;
	va_list arguments;
	va_start(arguments, format);
	redisReply *reply = (redisReply *)redisvCommand(context, format, arguments);
	va_end(arguments);
	if (!reply || reply->type == REDIS_REPLY_ERROR)
	{
		if (reply)
			freeReplyObject(reply);
		return nullptr;
	}
	return reply;
}

bool execute_job(redisContext *context, const std::shared_ptr<cache_job> &job)
{
	if (!job)
		return false;
	redisReply *reply = nullptr;
	if (job->operation == cache_operation::remove)
		reply = command(context, "DEL %b", job->key.data(), job->key.size());
	else if (job->value && job->ttl_seconds > 0)
		reply = command(context, "SETEX %b %d %b", job->key.data(), job->key.size(),
				job->ttl_seconds, job->value->data(), job->value->size());
	else if (job->value)
		reply = command(context, "SET %b %b", job->key.data(), job->key.size(),
				job->value->data(), job->value->size());
	const bool succeeded = reply && ((job->operation == cache_operation::remove &&
					  reply->type == REDIS_REPLY_INTEGER) ||
					 (job->operation == cache_operation::set &&
					  reply->type == REDIS_REPLY_STATUS));
	if (reply)
		freeReplyObject(reply);
	return succeeded;
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
	pending_bytes -= job_bytes(pending_jobs.front());
	pending_jobs.pop_front();
	if (dropped)
		++health.dropped;
	health.queued = pending_jobs.size();
	health.queued_bytes = pending_bytes;
}

void worker_main()
{
	redisContext *context = nullptr;
	unsigned int reconnect_delay_msec = 100;
	for (;;)
	{
		std::shared_ptr<cache_job> job;
		{
			std::unique_lock<std::mutex> lock(store_mutex);
			work_available.wait(lock,
					    [] { return stop_requested || !pending_jobs.empty(); });
			if (stop_requested)
				break;
			job = pending_jobs.front();
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

		if (execute_job(context, job))
		{
			std::lock_guard<std::mutex> lock(store_mutex);
			remove_front_locked(false);
			++health.completed;
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
			if (pending_jobs.front()->attempts >= REDIS_CACHE_MAX_COMMAND_ATTEMPTS)
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

bool enqueue_locked(const std::shared_ptr<cache_job> &job)
{
	const size_t bytes = job_bytes(job);
	const size_t first_replaceable = health.busy ? 1 : 0;
	for (size_t index = pending_jobs.size(); index > first_replaceable; --index)
	{
		const size_t candidate = index - 1;
		if (pending_jobs[candidate]->key != job->key)
			continue;
		const size_t replaced_bytes = job_bytes(pending_jobs[candidate]);
		if (pending_bytes - replaced_bytes > REDIS_CACHE_QUEUE_MAX_BYTES - bytes)
		{
			++health.dropped;
			return false;
		}
		pending_bytes = pending_bytes - replaced_bytes + bytes;
		pending_jobs[candidate] = job;
		++health.submitted;
		++health.coalesced;
		health.queued_bytes = pending_bytes;
		health.high_water_bytes = std::max(health.high_water_bytes, pending_bytes);
		return true;
	}
	if (pending_jobs.size() >= REDIS_CACHE_QUEUE_CAPACITY ||
	    pending_bytes > REDIS_CACHE_QUEUE_MAX_BYTES - bytes)
	{
		++health.dropped;
		return false;
	}
	pending_jobs.push_back(job);
	pending_bytes += bytes;
	++health.submitted;
	health.queued = pending_jobs.size();
	health.queued_bytes = pending_bytes;
	health.high_water = std::max(health.high_water, pending_jobs.size());
	health.high_water_bytes = std::max(health.high_water_bytes, pending_bytes);
	work_available.notify_one();
	return true;
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
		health.dropped += pending_jobs.size();
	pending_jobs.clear();
	local_cache.clear();
	pending_bytes = 0;
	health.queued = 0;
	health.queued_bytes = 0;
	health.local_entries = 0;
	health.initialized = false;
	health.connected = false;
	health.busy = false;
}
} // namespace

bool redis_cache_store_init(const struct redis_cache_store_config *config)
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
		local_cache.clear();
		health = {};
		health.initialized = true;
		accepting = true;
		stop_requested = false;
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

bool redis_cache_store_set(const char *key, const char *value, int ttl_seconds)
{
	if (!key || !value || ttl_seconds < 0)
		return false;
	const size_t key_size = strnlen(key, REDIS_CACHE_MAX_KEY_BYTES + 1);
	const size_t value_size = strnlen(value, REDIS_CACHE_MAX_VALUE_BYTES + 1);
	if (!key_size || key_size > REDIS_CACHE_MAX_KEY_BYTES ||
	    value_size > REDIS_CACHE_MAX_VALUE_BYTES)
		return false;
	try
	{
		auto owned_value = std::make_shared<const std::string>(value, value_size);
		auto job = std::make_shared<cache_job>();
		job->operation = cache_operation::set;
		job->key.assign(key, key_size);
		job->value = owned_value;
		job->ttl_seconds = ttl_seconds;

		std::lock_guard<std::mutex> lock(store_mutex);
		if (!health.initialized || !accepting)
			return false;
		auto found = local_cache.find(job->key);
		if (found == local_cache.end() && local_cache.size() >= REDIS_CACHE_LOCAL_CAPACITY)
		{
			++health.dropped;
			return false;
		}
		local_cache_entry entry;
		entry.value = owned_value;
		if (ttl_seconds > 0)
			entry.expires = std::chrono::steady_clock::now() +
					std::chrono::seconds(ttl_seconds);
		local_cache[job->key] = std::move(entry);
		health.local_entries = local_cache.size();
		return enqueue_locked(job);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

bool redis_cache_store_seed(const char *key, const char *value, int ttl_seconds)
{
	if (!key || !value || ttl_seconds <= 0)
		return false;
	const size_t key_size = strnlen(key, REDIS_CACHE_MAX_KEY_BYTES + 1);
	const size_t value_size = strnlen(value, REDIS_CACHE_MAX_VALUE_BYTES + 1);
	if (!key_size || key_size > REDIS_CACHE_MAX_KEY_BYTES ||
	    value_size > REDIS_CACHE_MAX_VALUE_BYTES)
		return false;
	try
	{
		auto owned_value = std::make_shared<const std::string>(value, value_size);
		std::string owned_key(key, key_size);
		std::lock_guard<std::mutex> lock(store_mutex);
		if (!health.initialized || !accepting)
			return false;
		auto found = local_cache.find(owned_key);
		if (found == local_cache.end() && local_cache.size() >= REDIS_CACHE_LOCAL_CAPACITY)
			return false;
		local_cache_entry entry;
		entry.value = std::move(owned_value);
		entry.expires =
			std::chrono::steady_clock::now() + std::chrono::seconds(ttl_seconds);
		local_cache[std::move(owned_key)] = std::move(entry);
		health.local_entries = local_cache.size();
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

char *redis_cache_store_get(const char *key)
{
	if (!key)
		return nullptr;
	const size_t key_size = strnlen(key, REDIS_CACHE_MAX_KEY_BYTES + 1);
	if (!key_size || key_size > REDIS_CACHE_MAX_KEY_BYTES)
		return nullptr;
	std::string owned_key;
	try
	{
		owned_key.assign(key, key_size);
	}
	catch (const std::bad_alloc &)
	{
		return nullptr;
	}
	std::shared_ptr<const std::string> value;
	{
		std::lock_guard<std::mutex> lock(store_mutex);
		if (!health.initialized)
			return nullptr;
		auto found = local_cache.find(owned_key);
		if (found == local_cache.end())
			return nullptr;
		if (found->second.expires != std::chrono::steady_clock::time_point{} &&
		    std::chrono::steady_clock::now() >= found->second.expires)
		{
			local_cache.erase(found);
			health.local_entries = local_cache.size();
			return nullptr;
		}
		value = found->second.value;
	}
	if (!value)
		return nullptr;
	char *result = (char *)malloc(value->size() + 1);
	if (!result)
		return nullptr;
	memcpy(result, value->data(), value->size());
	result[value->size()] = '\0';
	return result;
}

bool redis_cache_store_delete(const char *key)
{
	if (!key)
		return false;
	const size_t key_size = strnlen(key, REDIS_CACHE_MAX_KEY_BYTES + 1);
	if (!key_size || key_size > REDIS_CACHE_MAX_KEY_BYTES)
		return false;
	try
	{
		auto job = std::make_shared<cache_job>();
		job->operation = cache_operation::remove;
		job->key.assign(key, key_size);
		std::lock_guard<std::mutex> lock(store_mutex);
		if (!health.initialized || !accepting)
			return false;
		local_cache.erase(job->key);
		health.local_entries = local_cache.size();
		return enqueue_locked(job);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

bool redis_cache_store_drain(uint64_t timeout_msec)
{
	std::unique_lock<std::mutex> lock(store_mutex);
	if (!health.initialized)
		return true;
	return store_drained.wait_for(lock, std::chrono::milliseconds(timeout_msec),
				      [] { return pending_jobs.empty() && !health.busy; });
}

bool redis_cache_store_shutdown(uint64_t timeout_msec)
{
	{
		std::lock_guard<std::mutex> lock(store_mutex);
		if (!health.initialized)
			return true;
		accepting = false;
	}
	const bool drained = redis_cache_store_drain(timeout_msec);
	stop_worker(!drained);
	return drained;
}

void redis_cache_store_cancel(void)
{
	{
		std::lock_guard<std::mutex> lock(store_mutex);
		if (!health.initialized)
			return;
	}
	stop_worker(true);
}

struct redis_cache_store_health redis_cache_store_health_copy(void)
{
	std::lock_guard<std::mutex> lock(store_mutex);
	return health;
}

void redis_cache_store_reset_for_tests(void)
{
	redis_cache_store_cancel();
	std::lock_guard<std::mutex> lock(store_mutex);
	pending_jobs.clear();
	local_cache.clear();
	health = {};
	configured_host.clear();
	configured_port = 0;
	configured_connect_timeout_msec = 0;
	configured_command_timeout_msec = 0;
	pending_bytes = 0;
	accepting = false;
	stop_requested = false;
}
