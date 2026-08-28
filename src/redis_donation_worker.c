#include "redis_donation_worker.h"
#include "redis_connection.h"
#include "redis_key_registry.h"

#include <hiredis/hiredis.h>

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <deque>
#include <mutex>
#include <new>
#include <sys/poll.h> // local src/poll.h shadows <poll.h> via -I.
#include <string>
#include <sys/time.h>
#include <thread>

namespace
{
std::mutex worker_mutex;
std::condition_variable worker_wakeup;
std::deque<donation_event> pending_events;
std::deque<std::string> seen_event_ids;
std::thread worker_thread;
redis_donation_worker_health health = {};
std::string configured_secret;
const redis_connection_settings *configured_connection = nullptr;
bool stop_requested = false;

redisContext *connect_bounded()
{
	redisContext *context = redis_connection_open(configured_connection);
	if (context && context->err)
	{
		redisFree(context);
		return nullptr;
	}
	return context;
}

bool subscribe(redisContext *context)
{
	redisReply *reply =
		(redisReply *)redisCommand(context, "SUBSCRIBE %s", REDIS_DONATION_CHANNEL);
	const bool subscribed = reply && reply->type == REDIS_REPLY_ARRAY;
	if (reply)
		freeReplyObject(reply);
	return subscribed;
}

bool wait_for_retry(unsigned int seconds)
{
	std::unique_lock<std::mutex> lock(worker_mutex);
	return !worker_wakeup.wait_for(lock, std::chrono::seconds(seconds),
				       [] { return stop_requested; });
}

void drop_connection(redisContext **context)
{
	if (*context)
		redisFree(*context);
	*context = nullptr;
	std::lock_guard<std::mutex> lock(worker_mutex);
	health.connected = false;
	++health.connection_failures;
}

void accept_payload(const char *payload, size_t length)
{
	donation_event event = {};
	if (!donation_event_decode(payload, length, configured_secret.c_str(), time(nullptr),
				   &event))
	{
		std::lock_guard<std::mutex> lock(worker_mutex);
		++health.rejected;
		return;
	}

	try
	{
		std::lock_guard<std::mutex> lock(worker_mutex);
		const std::string event_id(event.event_id);
		if (std::find(seen_event_ids.begin(), seen_event_ids.end(), event_id) !=
		    seen_event_ids.end())
		{
			++health.replayed;
			return;
		}
		if (seen_event_ids.size() >= REDIS_DONATION_REPLAY_CAPACITY)
			seen_event_ids.pop_front();
		seen_event_ids.push_back(event_id);
		++health.validated;
		if (pending_events.size() >= REDIS_DONATION_QUEUE_CAPACITY)
		{
			++health.dropped;
			return;
		}
		pending_events.push_back(event);
		health.queued = pending_events.size();
		health.high_water = std::max(health.high_water, pending_events.size());
	}
	catch (const std::bad_alloc &)
	{
		std::lock_guard<std::mutex> lock(worker_mutex);
		++health.dropped;
	}
}

void handle_reply(redisReply *reply)
{
	if (!reply || reply->type != REDIS_REPLY_ARRAY || reply->elements != 3 ||
	    !reply->element[0] || !reply->element[2] ||
	    reply->element[0]->type != REDIS_REPLY_STRING || !reply->element[0]->str ||
	    strcmp(reply->element[0]->str, "message") != 0 ||
	    reply->element[2]->type != REDIS_REPLY_STRING || !reply->element[2]->str)
		return;
	{
		std::lock_guard<std::mutex> lock(worker_mutex);
		++health.received;
	}
	accept_payload(reply->element[2]->str, reply->element[2]->len);
}

size_t drain_reader(redisContext *context, bool *failed)
{
	*failed = false;
	size_t handled = 0;
	for (; handled < REDIS_DONATION_WORK_BATCH; ++handled)
	{
		redisReply *reply = nullptr;
		if (redisGetReplyFromReader(context, (void **)&reply) != REDIS_OK)
		{
			if (reply)
				freeReplyObject(reply);
			*failed = true;
			break;
		}
		if (!reply)
			break;
		handle_reply(reply);
		freeReplyObject(reply);
	}
	return handled;
}

void worker_main()
{
	redisContext *context = nullptr;
	unsigned int reconnect_delay_seconds = 1;
	bool connected_once = false;
	for (;;)
	{
		{
			std::lock_guard<std::mutex> lock(worker_mutex);
			if (stop_requested)
				break;
		}
		if (!context)
		{
			context = connect_bounded();
			if (!context || !subscribe(context))
			{
				drop_connection(&context);
				if (!wait_for_retry(reconnect_delay_seconds))
					break;
				reconnect_delay_seconds =
					std::min(reconnect_delay_seconds * 2,
						 REDIS_DONATION_MAX_RECONNECT_DELAY_SECONDS);
				continue;
			}
			{
				std::lock_guard<std::mutex> lock(worker_mutex);
				health.connected = true;
				if (connected_once)
					++health.reconnects;
			}
			connected_once = true;
			reconnect_delay_seconds = 1;
		}

		bool reader_failed = false;
		if (drain_reader(context, &reader_failed) == REDIS_DONATION_WORK_BATCH)
			continue;
		if (reader_failed)
		{
			drop_connection(&context);
			continue;
		}

		struct pollfd descriptor = { context->fd, POLLIN, 0 };
		const int ready = poll(&descriptor, 1, 100);
		if (ready < 0)
		{
			if (errno == EINTR)
				continue;
			drop_connection(&context);
			continue;
		}
		if (!ready)
			continue;
		if (descriptor.revents & (POLLERR | POLLHUP | POLLNVAL) ||
		    !(descriptor.revents & POLLIN) || redisBufferRead(context) != REDIS_OK)
		{
			drop_connection(&context);
			continue;
		}
	}
	if (context)
		redisFree(context);
	std::lock_guard<std::mutex> lock(worker_mutex);
	health.connected = false;
}
} // namespace

bool redis_donation_worker_init(const struct redis_donation_worker_config *config)
{
	if (!config || !config->connection || !config->secret || strlen(config->secret) < 32)
		return false;
	std::lock_guard<std::mutex> lock(worker_mutex);
	if (health.initialized)
		return false;
	try
	{
		configured_connection = config->connection;
		configured_secret = config->secret;
		pending_events.clear();
		seen_event_ids.clear();
		health = {};
		health.initialized = true;
		stop_requested = false;
		worker_thread = std::thread(worker_main);
	}
	catch (...)
	{
		health = {};
		configured_secret.clear();
		return false;
	}
	return true;
}

bool redis_donation_worker_take(struct donation_event *event)
{
	if (!event)
		return false;
	std::lock_guard<std::mutex> lock(worker_mutex);
	if (pending_events.empty())
		return false;
	*event = pending_events.front();
	pending_events.pop_front();
	health.queued = pending_events.size();
	return true;
}

void redis_donation_worker_shutdown(void)
{
	{
		std::lock_guard<std::mutex> lock(worker_mutex);
		if (!health.initialized)
			return;
		stop_requested = true;
	}
	worker_wakeup.notify_all();
	if (worker_thread.joinable())
		worker_thread.join();
	std::lock_guard<std::mutex> lock(worker_mutex);
	pending_events.clear();
	seen_event_ids.clear();
	health.queued = 0;
	health.initialized = false;
	health.connected = false;
	configured_secret.clear();
}

struct redis_donation_worker_health redis_donation_worker_health_copy(void)
{
	std::lock_guard<std::mutex> lock(worker_mutex);
	return health;
}

void redis_donation_worker_reset_for_tests(void)
{
	redis_donation_worker_shutdown();
	std::lock_guard<std::mutex> lock(worker_mutex);
	health = {};
	configured_connection = nullptr;
	configured_secret.clear();
	stop_requested = false;
}
