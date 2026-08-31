#include "redis/redis_ship_legacy.h"

#include "redis/redis_command_observability.h"
#include "redis/redis_connection.h"
#include "redis/redis_key_registry.h"

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <deque>
#include <mutex>
#include <new>
#include <string>
#include <thread>
#include <utility>

#ifndef __NO_REDIS__
#include <hiredis/hiredis.h>
#endif

namespace
{
#ifndef __NO_REDIS__
struct ship_delete_job
{
	std::string key;
	unsigned int attempts = 0;
};

std::mutex worker_mutex;
std::condition_variable work_available;
std::condition_variable worker_drained;
std::deque<ship_delete_job> pending_jobs;
std::thread worker_thread;
const redis_connection_settings *configured_connection = nullptr;
bool worker_initialized = false;
bool worker_busy = false;
bool accepting = false;
bool stop_requested = false;

redisReply *ship_command(redis_shared_command_kind kind, redisContext *context, const char *format,
			 ...)
{
	const uint64_t started_usec = redis_observability_now_usec();
	redisReply *reply = NULL;
	if (context && !context->err)
	{
		va_list arguments;
		va_start(arguments, format);
		reply = static_cast<redisReply *>(redisvCommand(context, format, arguments));
		va_end(arguments);
	}
	const uint64_t finished_usec = redis_observability_now_usec();
	const uint64_t duration_usec =
		finished_usec >= started_usec ? finished_usec - started_usec : 0;
	redis_shared_command_outcome outcome = REDIS_SHARED_OUTCOME_SUCCESS;
	if (!context || context->err)
		outcome = redis_command_outcome(context, false);
	else if (!reply)
		outcome = REDIS_SHARED_OUTCOME_NO_REPLY;
	else if (reply->type == REDIS_REPLY_ERROR)
		outcome = REDIS_SHARED_OUTCOME_ERROR_REPLY;
	redis_shared_command_observability_record(REDIS_SHARED_SCOPE_MAINTENANCE, kind, outcome,
						  duration_usec);
	if (outcome == REDIS_SHARED_OUTCOME_SUCCESS)
		return reply;
	if (reply)
		freeReplyObject(reply);
	return NULL;
}

bool ship_snapshot_scan_empty(redisContext *context)
{
	char cursor[64] = "0";
	do
	{
		redisReply *scan = ship_command(REDIS_SHARED_COMMAND_SCAN, context,
						"SCAN %s MATCH %s COUNT 256", cursor,
						REDIS_SHIP_SNAPSHOT_PATTERN);
		if (!scan || scan->type != REDIS_REPLY_ARRAY || scan->elements != 2 ||
		    !scan->element[0] || !scan->element[1] || !scan->element[0]->str ||
		    scan->element[0]->type != REDIS_REPLY_STRING ||
		    scan->element[1]->type != REDIS_REPLY_ARRAY)
		{
			if (scan)
				freeReplyObject(scan);
			return false;
		}
		snprintf(cursor, sizeof cursor, "%s", scan->element[0]->str);
		const bool empty = scan->element[1]->elements == 0;
		freeReplyObject(scan);
		if (!empty)
			return false;
	} while (strcmp(cursor, "0"));
	return true;
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
		ship_delete_job job;
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
				if (pending_jobs.empty())
					worker_drained.notify_all();
				continue;
			}
			worker_busy = true;
		}

		if (!context || context->err)
		{
			if (context)
				redisFree(context);
			context = redis_connection_open(configured_connection);
			if (!context || context->err)
			{
				redis_shared_command_observability_record(
					REDIS_SHARED_SCOPE_MAINTENANCE, REDIS_SHARED_COMMAND_WRITE,
					redis_command_outcome(context, false), 0);
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

		redisReply *reply = ship_command(REDIS_SHARED_COMMAND_WRITE, context, "DEL %b",
						 job.key.data(), job.key.size());
		const bool deleted = reply && reply->type == REDIS_REPLY_INTEGER;
		if (reply)
			freeReplyObject(reply);
		if (deleted)
		{
			std::lock_guard<std::mutex> lock(worker_mutex);
			pending_jobs.pop_front();
			worker_busy = false;
			if (pending_jobs.empty())
				worker_drained.notify_all();
			continue;
		}

		{
			std::lock_guard<std::mutex> lock(worker_mutex);
			++pending_jobs.front().attempts;
			if (pending_jobs.front().attempts >= REDIS_SHIP_LEGACY_MAX_COMMAND_ATTEMPTS)
			{
				pending_jobs.pop_front();
				worker_busy = false;
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
	worker_busy = false;
	worker_drained.notify_all();
}

bool enqueue_delete(const char *key, size_t key_size)
{
	try
	{
		std::lock_guard<std::mutex> lock(worker_mutex);
		if (!worker_initialized || !accepting)
			return false;
		const auto duplicate =
			std::find_if(pending_jobs.cbegin(), pending_jobs.cend(),
				     [key, key_size](const ship_delete_job &job) {
					     return job.key.size() == key_size &&
						    !memcmp(job.key.data(), key, key_size);
				     });
		if (duplicate != pending_jobs.cend())
			return true;
		if (pending_jobs.size() >= REDIS_SHIP_LEGACY_QUEUE_CAPACITY)
			return false;
		ship_delete_job job;
		job.key.assign(key, key_size);
		pending_jobs.push_back(std::move(job));
		work_available.notify_one();
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
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
		pending_jobs.clear();
	worker_initialized = false;
	worker_busy = false;
}
#endif
} // namespace

bool redis_ship_legacy_worker_init(const redis_connection_settings *connection)
{
#ifdef __NO_REDIS__
	(void)connection;
	return true;
#else
	if (!connection)
		return false;
	std::lock_guard<std::mutex> lock(worker_mutex);
	if (worker_initialized)
		return true;
	try
	{
		configured_connection = connection;
		pending_jobs.clear();
		worker_initialized = true;
		worker_busy = false;
		accepting = true;
		stop_requested = false;
		worker_thread = std::thread(worker_main);
	}
	catch (...)
	{
		configured_connection = nullptr;
		worker_initialized = false;
		accepting = false;
		stop_requested = true;
		return false;
	}
	return true;
#endif
}

bool redis_ship_legacy_worker_drain(uint64_t timeout_msec)
{
#ifdef __NO_REDIS__
	(void)timeout_msec;
	return true;
#else
	std::unique_lock<std::mutex> lock(worker_mutex);
	if (!worker_initialized)
		return true;
	return worker_drained.wait_for(lock, std::chrono::milliseconds(timeout_msec),
				       [] { return pending_jobs.empty() && !worker_busy; });
#endif
}

bool redis_ship_legacy_worker_shutdown(uint64_t timeout_msec)
{
#ifdef __NO_REDIS__
	(void)timeout_msec;
	return true;
#else
	{
		std::lock_guard<std::mutex> lock(worker_mutex);
		if (!worker_initialized)
			return true;
		accepting = false;
	}
	const bool drained = redis_ship_legacy_worker_drain(timeout_msec);
	stop_worker(!drained);
	return drained;
#endif
}

void redis_ship_legacy_worker_cancel(void)
{
#ifndef __NO_REDIS__
	{
		std::lock_guard<std::mutex> lock(worker_mutex);
		if (!worker_initialized)
			return;
	}
	stop_worker(true);
#endif
}

void redis_invalidate_ship_snapshot(const char *owner_name)
{
#ifndef __NO_REDIS__
	if (!owner_name)
		return;
	char key[256];
	const int written = snprintf(key, sizeof key, REDIS_SHIP_SNAPSHOT_FORMAT, owner_name);
	if (written <= 0 || static_cast<size_t>(written) >= sizeof key)
		return;
	enqueue_delete(key, static_cast<size_t>(written));
#else
	(void)owner_name;
#endif
}

bool redis_clear_ship_snapshots(struct redisContext *context)
{
#ifdef __NO_REDIS__
	(void)context;
	return true;
#else
	if (!context || context->err)
		return false;
	char cursor[64] = "0";
	do
	{
		redisReply *scan = ship_command(REDIS_SHARED_COMMAND_SCAN, context,
						"SCAN %s MATCH %s COUNT 256", cursor,
						REDIS_SHIP_SNAPSHOT_PATTERN);
		if (!scan || scan->type != REDIS_REPLY_ARRAY || scan->elements != 2 ||
		    !scan->element[0] || !scan->element[1] || !scan->element[0]->str ||
		    scan->element[0]->type != REDIS_REPLY_STRING ||
		    scan->element[1]->type != REDIS_REPLY_ARRAY)
		{
			if (scan)
				freeReplyObject(scan);
			return false;
		}

		snprintf(cursor, sizeof cursor, "%s", scan->element[0]->str);
		redisReply *keys = scan->element[1];
		for (size_t index = 0; index < keys->elements; ++index)
		{
			redisReply *key = keys->element[index];
			if (!key || key->type != REDIS_REPLY_STRING || !key->str)
			{
				freeReplyObject(scan);
				return false;
			}
			redisReply *deleted = ship_command(REDIS_SHARED_COMMAND_WRITE, context,
							   "DEL %b", key->str, key->len);
			if (!deleted || (deleted->type != REDIS_REPLY_INTEGER &&
					 deleted->type != REDIS_REPLY_NIL))
			{
				if (deleted)
					freeReplyObject(deleted);
				freeReplyObject(scan);
				return false;
			}
			freeReplyObject(deleted);
		}
		freeReplyObject(scan);
	} while (strcmp(cursor, "0"));
	return ship_snapshot_scan_empty(context);
#endif
}
