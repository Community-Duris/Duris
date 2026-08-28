#include "redis_presence_worker.h"

#include <hiredis/hiredis.h>

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <deque>
#include <mutex>
#include <new>
#include <random>
#include <string>
#include <sys/time.h>
#include <thread>

namespace
{
enum class presence_operation : uint8_t
{
	online,
	offline,
	clear,
};

struct presence_job
{
	presence_operation operation = presence_operation::clear;
	int pid = 0;
	bool publish_event = false;
	unsigned int attempts = 0;
	uint64_t sequence = 0;
	std::string payload;
};

constexpr const char *PRESENCE_SCRIPT =
	"if redis.call('EXISTS',KEYS[3]..ARGV[1])==1 then return 1 end "
	"if ARGV[2]=='clear' then redis.call('DEL',KEYS[1]) "
	"elseif ARGV[2]=='online' then redis.call('HSET',KEYS[1],ARGV[3],ARGV[4]) "
	"else redis.call('HDEL',KEYS[1],ARGV[3]) end "
	"redis.call('SET',KEYS[3]..ARGV[1],'1','EX',3600) "
	"if ARGV[5]~='' then redis.call('PUBLISH',KEYS[2],ARGV[5]) end "
	"return 1";

std::mutex worker_mutex;
std::condition_variable work_available;
std::condition_variable worker_drained;
std::deque<presence_job> pending_jobs;
std::thread worker_thread;
redis_presence_worker_health health = {};
std::string configured_host;
int configured_port = 0;
int configured_connect_timeout_msec = 0;
int configured_command_timeout_msec = 0;
uint64_t instance_id = 0;
uint64_t next_sequence = 1;
bool accepting = false;
bool stop_requested = false;

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

const char *operation_name(presence_operation operation)
{
	switch (operation)
	{
	case presence_operation::online:
		return "online";
	case presence_operation::offline:
		return "offline";
	case presence_operation::clear:
		return "clear";
	}
	return "";
}

bool execute_job(redisContext *context, const presence_job &job)
{
	char operation_id[64];
	const int operation_id_length = snprintf(operation_id, sizeof operation_id, "%llx:%llu",
						 (unsigned long long)instance_id,
						 (unsigned long long)job.sequence);
	if (operation_id_length <= 0 || (size_t)operation_id_length >= sizeof operation_id)
		return false;

	char pid[32] = {};
	if (job.pid > 0)
	{
		const int pid_length = snprintf(pid, sizeof pid, "%d", job.pid);
		if (pid_length <= 0 || (size_t)pid_length >= sizeof pid)
			return false;
	}
	char event[128] = {};
	if (job.publish_event)
	{
		const int event_length = snprintf(
			event, sizeof event, "{\"event\":\"%s\",\"pid\":%d}",
			job.operation == presence_operation::online ? "login" : "logout", job.pid);
		if (event_length <= 0 || (size_t)event_length >= sizeof event)
			return false;
	}

	redisReply *reply =
		command(context, "EVAL %b 3 mud:online mud:player mud:presence_op: %b %s %b %b %b",
			PRESENCE_SCRIPT, strlen(PRESENCE_SCRIPT), operation_id,
			(size_t)operation_id_length, operation_name(job.operation), pid,
			strlen(pid), job.payload.data(), job.payload.size(), event, strlen(event));
	const bool succeeded = reply && reply->type == REDIS_REPLY_INTEGER && reply->integer == 1;
	if (reply)
		freeReplyObject(reply);
	return succeeded;
}

bool wait_for_retry(unsigned int delay_msec)
{
	std::unique_lock<std::mutex> lock(worker_mutex);
	return !work_available.wait_for(lock, std::chrono::milliseconds(delay_msec),
					[] { return stop_requested; });
}

void worker_main()
{
	redisContext *context = nullptr;
	unsigned int reconnect_delay_msec = 100;
	for (;;)
	{
		presence_job job;
		{
			std::unique_lock<std::mutex> lock(worker_mutex);
			work_available.wait(lock,
					    [] { return stop_requested || !pending_jobs.empty(); });
			if (stop_requested)
				break;
			try
			{
				job = pending_jobs.front();
			}
			catch (const std::bad_alloc &)
			{
				pending_jobs.pop_front();
				++health.dropped;
				health.queued = pending_jobs.size();
				if (pending_jobs.empty())
					worker_drained.notify_all();
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
				std::lock_guard<std::mutex> lock(worker_mutex);
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
			std::lock_guard<std::mutex> lock(worker_mutex);
			pending_jobs.pop_front();
			++health.completed;
			health.queued = pending_jobs.size();
			health.busy = false;
			if (pending_jobs.empty())
				worker_drained.notify_all();
			continue;
		}

		{
			std::lock_guard<std::mutex> lock(worker_mutex);
			++health.command_failures;
			health.connected = false;
			++pending_jobs.front().attempts;
			if (pending_jobs.front().attempts >= REDIS_PRESENCE_MAX_COMMAND_ATTEMPTS)
			{
				pending_jobs.pop_front();
				++health.dropped;
				health.queued = pending_jobs.size();
				health.busy = false;
				if (pending_jobs.empty())
					worker_drained.notify_all();
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
	std::lock_guard<std::mutex> lock(worker_mutex);
	health.connected = false;
	health.busy = false;
	worker_drained.notify_all();
}

bool submit(presence_operation operation, int pid, const char *payload, bool publish_event)
{
	const size_t payload_size =
		payload ? strnlen(payload, REDIS_PRESENCE_MAX_PAYLOAD_BYTES + 1) : 0;
	std::lock_guard<std::mutex> lock(worker_mutex);
	if (!health.initialized || !accepting ||
	    (operation != presence_operation::clear && pid <= 0))
		return false;
	if (payload_size > REDIS_PRESENCE_MAX_PAYLOAD_BYTES)
	{
		++health.dropped;
		return false;
	}
	if (pending_jobs.size() >= REDIS_PRESENCE_QUEUE_CAPACITY)
	{
		++health.dropped;
		return false;
	}
	try
	{
		presence_job job;
		job.operation = operation;
		job.pid = pid;
		job.publish_event = publish_event;
		job.sequence = next_sequence++;
		if (payload)
			job.payload.assign(payload, payload_size);
		pending_jobs.push_back(std::move(job));
	}
	catch (const std::bad_alloc &)
	{
		++health.dropped;
		return false;
	}
	++health.submitted;
	health.queued = pending_jobs.size();
	health.high_water = std::max(health.high_water, pending_jobs.size());
	work_available.notify_one();
	return true;
}

void stop_worker(bool discard_pending)
{
	{
		std::lock_guard<std::mutex> lock(worker_mutex);
		accepting = false;
		stop_requested = true;
	}
	work_available.notify_all();
	if (worker_thread.joinable())
		worker_thread.join();
	std::lock_guard<std::mutex> lock(worker_mutex);
	if (discard_pending)
		health.dropped += pending_jobs.size();
	pending_jobs.clear();
	health.queued = 0;
	health.initialized = false;
	health.connected = false;
	health.busy = false;
}
} // namespace

bool redis_presence_worker_init(const struct redis_presence_worker_config *config)
{
	if (!config || !config->host || !*config->host || config->port <= 0 ||
	    config->port > 65535 || config->connect_timeout_msec <= 0 ||
	    config->command_timeout_msec <= 0)
		return false;
	std::lock_guard<std::mutex> lock(worker_mutex);
	if (health.initialized)
		return true;
	try
	{
		configured_host = config->host;
		configured_port = config->port;
		configured_connect_timeout_msec = config->connect_timeout_msec;
		configured_command_timeout_msec = config->command_timeout_msec;
		std::random_device random;
		instance_id = (static_cast<uint64_t>(random()) << 32) | random();
		if (!instance_id)
			instance_id = 1;
		next_sequence = 1;
		pending_jobs.clear();
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

bool redis_presence_worker_submit_online(int pid, const char *json, bool publish_event)
{
	if (!json)
		return false;
	return submit(presence_operation::online, pid, json, publish_event);
}

bool redis_presence_worker_submit_offline(int pid, bool publish_event)
{
	return submit(presence_operation::offline, pid, nullptr, publish_event);
}

bool redis_presence_worker_submit_clear(void)
{
	return submit(presence_operation::clear, 0, nullptr, false);
}

bool redis_presence_worker_drain(uint64_t timeout_msec)
{
	std::unique_lock<std::mutex> lock(worker_mutex);
	if (!health.initialized)
		return true;
	return worker_drained.wait_for(lock, std::chrono::milliseconds(timeout_msec),
				       [] { return pending_jobs.empty() && !health.busy; });
}

bool redis_presence_worker_shutdown(uint64_t timeout_msec)
{
	{
		std::lock_guard<std::mutex> lock(worker_mutex);
		if (!health.initialized)
			return true;
		accepting = false;
	}
	const bool drained = redis_presence_worker_drain(timeout_msec);
	stop_worker(!drained);
	return drained;
}

void redis_presence_worker_cancel(void)
{
	{
		std::lock_guard<std::mutex> lock(worker_mutex);
		if (!health.initialized)
			return;
	}
	stop_worker(true);
}

struct redis_presence_worker_health redis_presence_worker_health_copy(void)
{
	std::lock_guard<std::mutex> lock(worker_mutex);
	return health;
}

void redis_presence_worker_reset_for_tests(void)
{
	redis_presence_worker_cancel();
	std::lock_guard<std::mutex> lock(worker_mutex);
	health = {};
	configured_host.clear();
	configured_port = 0;
	configured_connect_timeout_msec = 0;
	configured_command_timeout_msec = 0;
	instance_id = 0;
	next_sequence = 1;
	accepting = false;
	stop_requested = false;
}
