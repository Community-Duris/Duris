#include "redis/redis_presence_worker.h"
#include "redis/redis_connection.h"

#include <hiredis/hiredis.h>

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <deque>
#include <memory>
#include <mutex>
#include <new>
#include <random>
#include <string>
#include <sys/time.h>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

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
	std::shared_ptr<const std::string> payload;
};

enum class execution_result : uint8_t
{
	failure,
	success,
	fenced,
};

constexpr const char *PRESENCE_SCRIPT =
	"if redis.call('EXISTS',KEYS[3]..ARGV[1])==1 then return 1 end "
	"if ARGV[2]=='clear' then redis.call('SET',KEYS[1],ARGV[6],'EX',ARGV[7]) "
	"redis.call('DEL',KEYS[5]) "
	"else local current=redis.call('GET',KEYS[1]) "
	"if current and current~=ARGV[6] then return 2 end "
	"redis.call('SET',KEYS[1],ARGV[6],'EX',ARGV[7]) "
	"local session=KEYS[2]..ARGV[6]..':'..ARGV[3] "
	"if ARGV[2]=='online' then redis.call('SET',session,ARGV[4],'EX',ARGV[7]) "
	"else redis.call('DEL',session) end end "
	"redis.call('SET',KEYS[3]..ARGV[1],'1','EX',3600) "
	"if ARGV[5]~='' then redis.call('PUBLISH',KEYS[4],ARGV[5]) end "
	"return 1";
constexpr const char *PRESENCE_HEARTBEAT_SCRIPT =
	"if redis.call('GET',KEYS[1])~=ARGV[1] then return -1 end "
	"redis.call('EXPIRE',KEYS[1],ARGV[2]) "
	"local count=0 for i=3,#ARGV,2 do "
	"redis.call('SET',KEYS[2]..ARGV[1]..':'..ARGV[i],ARGV[i+1],'EX',ARGV[2]) "
	"count=count+1 end return count";

std::mutex worker_mutex;
std::condition_variable work_available;
std::condition_variable worker_drained;
std::deque<presence_job> pending_jobs;
// Desired live sessions, updated at submission so a rejected offline job still stops renewal.
std::unordered_map<int, std::shared_ptr<const std::string>> active_sessions;
std::thread worker_thread;
redis_presence_worker_health health = {};
const redis_connection_settings *configured_connection = nullptr;
std::string configured_current_key;
std::string configured_session_prefix;
std::string configured_retry_prefix;
std::string configured_event_channel;
std::string configured_legacy_online_key;
unsigned int configured_session_ttl_seconds = 0;
unsigned int configured_heartbeat_interval_msec = 0;
uint64_t instance_id = 0;
uint64_t next_sequence = 1;
bool accepting = false;
bool stop_requested = false;
bool generation_claimed = false;

uint64_t operation_elapsed(uint64_t started_usec)
{
	const uint64_t finished_usec = redis_observability_now_usec();
	return finished_usec >= started_usec ? finished_usec - started_usec : 0;
}

bool valid_surface(const char *value)
{
	return value && *value && strnlen(value, 161) <= 160;
}

redisContext *connect_bounded()
{
	return redis_connection_open(configured_connection);
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

execution_result execute_job(redisContext *context, const presence_job &job)
{
	char operation_id[64];
	const int operation_id_length = snprintf(operation_id, sizeof operation_id, "%llx:%llu",
						 (unsigned long long)instance_id,
						 (unsigned long long)job.sequence);
	if (operation_id_length <= 0 || (size_t)operation_id_length >= sizeof operation_id)
		return execution_result::failure;

	char pid[32] = {};
	if (job.pid > 0)
	{
		const int pid_length = snprintf(pid, sizeof pid, "%d", job.pid);
		if (pid_length <= 0 || (size_t)pid_length >= sizeof pid)
			return execution_result::failure;
	}
	char event[128] = {};
	if (job.publish_event)
	{
		const int event_length = snprintf(
			event, sizeof event, "{\"event\":\"%s\",\"pid\":%d}",
			job.operation == presence_operation::online ? "login" : "logout", job.pid);
		if (event_length <= 0 || (size_t)event_length >= sizeof event)
			return execution_result::failure;
	}
	char instance[32] = {};
	const int instance_length =
		snprintf(instance, sizeof instance, "%llx", (unsigned long long)instance_id);
	char ttl[16] = {};
	const int ttl_length = snprintf(ttl, sizeof ttl, "%u", configured_session_ttl_seconds);
	if (instance_length <= 0 || (size_t)instance_length >= sizeof instance || ttl_length <= 0 ||
	    (size_t)ttl_length >= sizeof ttl)
		return execution_result::failure;

	redisReply *reply =
		command(context,
			"EVAL %b 5 %s %s %s %s %s "
			"%b %s %b %b %b %b %b",
			PRESENCE_SCRIPT, strlen(PRESENCE_SCRIPT), configured_current_key.c_str(),
			configured_session_prefix.c_str(), configured_retry_prefix.c_str(),
			configured_event_channel.c_str(), configured_legacy_online_key.c_str(),
			operation_id, (size_t)operation_id_length, operation_name(job.operation),
			pid, strlen(pid), job.payload ? job.payload->data() : "",
			job.payload ? job.payload->size() : 0, event, strlen(event), instance,
			(size_t)instance_length, ttl, (size_t)ttl_length);
	const execution_result result = !reply || reply->type != REDIS_REPLY_INTEGER ?
						execution_result::failure :
					reply->integer == 1 ? execution_result::success :
					reply->integer == 2 ? execution_result::fenced :
							      execution_result::failure;
	if (reply)
		freeReplyObject(reply);
	return result;
}

execution_result refresh_active_sessions(redisContext *context)
{
	std::vector<std::pair<int, std::shared_ptr<const std::string>>> sessions;
	try
	{
		std::lock_guard<std::mutex> lock(worker_mutex);
		sessions.reserve(active_sessions.size());
		for (const auto &session : active_sessions)
			sessions.push_back(session);
	}
	catch (const std::bad_alloc &)
	{
		return execution_result::failure;
	}
	if (sessions.empty())
		return execution_result::success;
	char instance[32] = {};
	const int instance_length =
		snprintf(instance, sizeof instance, "%llx", (unsigned long long)instance_id);
	char ttl[16] = {};
	const int ttl_length = snprintf(ttl, sizeof ttl, "%u", configured_session_ttl_seconds);
	if (instance_length <= 0 || (size_t)instance_length >= sizeof instance || ttl_length <= 0 ||
	    (size_t)ttl_length >= sizeof ttl)
		return execution_result::failure;
	auto cursor = sessions.cbegin();
	while (cursor != sessions.cend())
	{
		std::vector<std::string> pids;
		std::vector<const char *> arguments;
		std::vector<size_t> lengths;
		try
		{
			pids.reserve(REDIS_PRESENCE_HEARTBEAT_BATCH);
			arguments.reserve(7 + REDIS_PRESENCE_HEARTBEAT_BATCH * 2);
			lengths.reserve(7 + REDIS_PRESENCE_HEARTBEAT_BATCH * 2);
			auto add_argument = [&arguments, &lengths](const char *value, size_t length)
			{
				arguments.push_back(value);
				lengths.push_back(length);
			};
			add_argument("EVAL", 4);
			add_argument(PRESENCE_HEARTBEAT_SCRIPT, strlen(PRESENCE_HEARTBEAT_SCRIPT));
			add_argument("2", 1);
			add_argument(configured_current_key.data(), configured_current_key.size());
			add_argument(configured_session_prefix.data(),
				     configured_session_prefix.size());
			add_argument(instance, static_cast<size_t>(instance_length));
			add_argument(ttl, static_cast<size_t>(ttl_length));
			for (size_t count = 0;
			     cursor != sessions.cend() && count < REDIS_PRESENCE_HEARTBEAT_BATCH;
			     ++cursor, ++count)
			{
				pids.push_back(std::to_string(cursor->first));
				add_argument(pids.back().data(), pids.back().size());
				add_argument(cursor->second->data(), cursor->second->size());
			}
		}
		catch (const std::bad_alloc &)
		{
			return execution_result::failure;
		}
		const uint64_t operation_started = redis_observability_now_usec();
		redisReply *reply =
			(redisReply *)redisCommandArgv(context, static_cast<int>(arguments.size()),
						       arguments.data(), lengths.data());
		const bool command_succeeded = reply && reply->type == REDIS_REPLY_INTEGER;
		{
			std::lock_guard<std::mutex> lock(worker_mutex);
			redis_worker_operation_record(&health.operations,
						      redis_command_outcome(context,
									    command_succeeded),
						      operation_elapsed(operation_started));
		}
		if (!command_succeeded)
		{
			if (reply)
				freeReplyObject(reply);
			return execution_result::failure;
		}
		if (reply->integer < 0)
		{
			freeReplyObject(reply);
			return execution_result::fenced;
		}
		const uint64_t refreshed = static_cast<uint64_t>(reply->integer);
		freeReplyObject(reply);
		std::lock_guard<std::mutex> lock(worker_mutex);
		health.lease_refreshes += refreshed;
	}
	return execution_result::success;
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
	auto next_heartbeat = std::chrono::steady_clock::now() +
			      std::chrono::milliseconds(configured_heartbeat_interval_msec);
	for (;;)
	{
		presence_job job;
		bool heartbeat = false;
		bool heartbeat_has_sessions = false;
		{
			std::unique_lock<std::mutex> lock(worker_mutex);
			if (pending_jobs.empty() &&
			    std::chrono::steady_clock::now() < next_heartbeat)
				work_available.wait_until(
					lock, next_heartbeat,
					[] { return stop_requested || !pending_jobs.empty(); });
			if (stop_requested)
				break;
			heartbeat = std::chrono::steady_clock::now() >= next_heartbeat;
			heartbeat_has_sessions = heartbeat && generation_claimed &&
						 !active_sessions.empty();
			if (!heartbeat)
			{
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
			}
			health.busy = true;
		}
		if (heartbeat && !heartbeat_has_sessions)
		{
			next_heartbeat =
				std::chrono::steady_clock::now() +
				std::chrono::milliseconds(configured_heartbeat_interval_msec);
			std::lock_guard<std::mutex> lock(worker_mutex);
			health.busy = false;
			continue;
		}

		if (!context || context->err)
		{
			if (context)
				redisFree(context);
			context = connect_bounded();
			const bool connected = context && !context->err;
			{
				std::lock_guard<std::mutex> lock(worker_mutex);
				health.connected = connected;
				if (health.connected)
					++health.reconnects;
				else
					++health.connection_failures;
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

		if (heartbeat)
		{
			const execution_result result = refresh_active_sessions(context);
			if (result == execution_result::success ||
			    result == execution_result::fenced)
			{
				next_heartbeat = std::chrono::steady_clock::now() +
						 std::chrono::milliseconds(
							 configured_heartbeat_interval_msec);
				std::lock_guard<std::mutex> lock(worker_mutex);
				if (result == execution_result::fenced)
				{
					active_sessions.clear();
					generation_claimed = false;
				}
				health.active_sessions = active_sessions.size();
				health.busy = false;
				continue;
			}
			{
				std::lock_guard<std::mutex> lock(worker_mutex);
				++health.lease_failures;
				++health.command_failures;
				health.connected = false;
				health.busy = false;
			}
			// A failed due heartbeat must not remain perpetually overdue and starve
			// queued state changes while Redis rejects the lease command.
			next_heartbeat =
				std::chrono::steady_clock::now() +
				std::chrono::milliseconds(configured_heartbeat_interval_msec);
			redisFree(context);
			context = nullptr;
			if (!wait_for_retry(reconnect_delay_msec))
				break;
			reconnect_delay_msec = std::min(reconnect_delay_msec * 2, 60000U);
			continue;
		}

		const uint64_t operation_started = redis_observability_now_usec();
		const execution_result result = execute_job(context, job);
		const uint64_t operation_duration = operation_elapsed(operation_started);
		const bool succeeded = result == execution_result::success ||
				       result == execution_result::fenced;
		const redis_shared_command_outcome outcome =
			redis_command_outcome(context, succeeded);
		if (result == execution_result::success || result == execution_result::fenced)
		{
			std::lock_guard<std::mutex> lock(worker_mutex);
			redis_worker_operation_record(&health.operations, outcome,
						      operation_duration);
			if (result == execution_result::fenced)
			{
				active_sessions.clear();
				generation_claimed = false;
			}
			else
				generation_claimed = true;
			pending_jobs.pop_front();
			++health.completed;
			health.queued = pending_jobs.size();
			health.active_sessions = active_sessions.size();
			health.busy = false;
			if (pending_jobs.empty())
				worker_drained.notify_all();
			continue;
		}

		{
			std::lock_guard<std::mutex> lock(worker_mutex);
			redis_worker_operation_record(&health.operations, outcome,
						      operation_duration);
			++health.command_failures;
			health.connected = false;
			++pending_jobs.front().attempts;
			if (pending_jobs.front().attempts >= REDIS_PRESENCE_MAX_COMMAND_ATTEMPTS)
			{
				if (pending_jobs.front().operation == presence_operation::online)
				{
					const auto active =
						active_sessions.find(pending_jobs.front().pid);
					if (active != active_sessions.end() &&
					    active->second == pending_jobs.front().payload)
						active_sessions.erase(active);
				}
				pending_jobs.pop_front();
				++health.dropped;
				health.queued = pending_jobs.size();
				health.active_sessions = active_sessions.size();
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
	if (operation == presence_operation::offline)
		active_sessions.erase(pid);
	else if (operation == presence_operation::clear)
		active_sessions.clear();
	health.active_sessions = active_sessions.size();
	if (pending_jobs.size() >= REDIS_PRESENCE_QUEUE_CAPACITY)
	{
		++health.dropped;
		return false;
	}
	bool queued = false;
	try
	{
		presence_job job;
		job.operation = operation;
		job.pid = pid;
		job.publish_event = publish_event;
		job.sequence = next_sequence++;
		if (payload)
			job.payload = std::make_shared<const std::string>(payload, payload_size);
		pending_jobs.push_back(std::move(job));
		queued = true;
		if (operation == presence_operation::online)
		{
			if (active_sessions.size() >= REDIS_PRESENCE_QUEUE_CAPACITY &&
			    active_sessions.find(pid) == active_sessions.end())
				throw std::bad_alloc();
			active_sessions.insert_or_assign(pid, pending_jobs.back().payload);
		}
	}
	catch (const std::bad_alloc &)
	{
		if (queued)
			pending_jobs.pop_back();
		++health.dropped;
		return false;
	}
	++health.submitted;
	health.queued = pending_jobs.size();
	health.active_sessions = active_sessions.size();
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
	active_sessions.clear();
	health.queued = 0;
	health.active_sessions = 0;
	health.initialized = false;
	health.connected = false;
	health.busy = false;
	generation_claimed = false;
}
} // namespace

bool redis_presence_worker_init(const struct redis_presence_worker_config *config)
{
	if (!config || !config->connection || config->session_ttl_seconds < 2 ||
	    !config->heartbeat_interval_msec ||
	    config->heartbeat_interval_msec >= config->session_ttl_seconds * 1000ULL ||
	    !valid_surface(config->current_key) || !valid_surface(config->session_prefix) ||
	    !valid_surface(config->retry_prefix) || !valid_surface(config->event_channel) ||
	    !valid_surface(config->legacy_online_key))
		return false;
	std::lock_guard<std::mutex> lock(worker_mutex);
	if (health.initialized)
		return true;
	try
	{
		configured_connection = config->connection;
		configured_current_key = config->current_key;
		configured_session_prefix = config->session_prefix;
		configured_retry_prefix = config->retry_prefix;
		configured_event_channel = config->event_channel;
		configured_legacy_online_key = config->legacy_online_key;
		configured_session_ttl_seconds = config->session_ttl_seconds;
		configured_heartbeat_interval_msec = config->heartbeat_interval_msec;
		std::random_device random;
		instance_id = (static_cast<uint64_t>(random()) << 32) | random();
		if (!instance_id)
			instance_id = 1;
		next_sequence = 1;
		pending_jobs.clear();
		active_sessions.clear();
		health = {};
		health.initialized = true;
		accepting = true;
		stop_requested = false;
		generation_claimed = false;
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
	redis_presence_worker_health snapshot = health;
	redis_worker_operation_prepare_snapshot(&snapshot.operations);
	return snapshot;
}

void redis_presence_worker_reset_for_tests(void)
{
	redis_presence_worker_cancel();
	std::lock_guard<std::mutex> lock(worker_mutex);
	health = {};
	configured_connection = nullptr;
	configured_current_key.clear();
	configured_session_prefix.clear();
	configured_retry_prefix.clear();
	configured_event_channel.clear();
	configured_legacy_online_key.clear();
	configured_session_ttl_seconds = 0;
	configured_heartbeat_interval_msec = 0;
	instance_id = 0;
	next_sequence = 1;
	accepting = false;
	stop_requested = false;
	generation_claimed = false;
}
