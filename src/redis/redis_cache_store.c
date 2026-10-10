#include "player/player_retained_deque.h"
#include <cerrno>
#include "redis/redis_cache_store.h"
#include "redis/redis_connection.h"

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
// A genuine inherited deque exposes its own installed protected base state;
// no reinterpret-cast, mirrored capacity ledger or private-member override.
class artifact_birth_cache_deque : public player_retained_deque<std::shared_ptr<cache_job>>
{
    public:
	bool push_back_request(size_t *output) const noexcept
	{
		if (!output)
			return false;
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI == 1 && !defined(_GLIBCXX_DEBUG)
		using element = std::shared_ptr<cache_job>;
		using gnu_base = std::_Deque_base<element, std::allocator<element>>;
		const auto &impl = this->gnu_base::_M_impl;
		if (!impl._M_map || !impl._M_finish._M_node || !impl._M_start._M_node)
			return false;
		if (impl._M_finish._M_cur != impl._M_finish._M_last - 1)
		{
			*output = 0;
			return true;
		}
		const size_t block = std::__deque_buf_size(sizeof(element)) * sizeof(element);
		const size_t map_tail =
			impl._M_map_size - size_t(impl._M_finish._M_node - impl._M_map);
		size_t request = block;
		if (2 > map_tail)
		{
			const size_t old_nodes =
				size_t(impl._M_finish._M_node - impl._M_start._M_node) + 1;
			const size_t new_nodes = old_nodes + 1;
			if (impl._M_map_size <= 2 * new_nodes)
			{
				if (impl._M_map_size > (SIZE_MAX - 2) / 2)
					return false;
				const size_t new_map = impl._M_map_size +
						       std::max(impl._M_map_size, size_t(1)) + 2;
				if (new_map > (SIZE_MAX - request) / sizeof(element *))
					return false;
				request += new_map * sizeof(element *);
			}
		}
		*output = request;
		return true;
#else
		return false;
#endif
	}
};

artifact_birth_cache_deque pending_jobs;
std::map<std::string, local_cache_entry> local_cache;
std::thread worker_thread;
redis_cache_store_health health = {};
const redis_connection_settings *configured_connection = nullptr;
size_t pending_bytes = 0;
bool accepting = false;
bool stop_requested = false;
std::shared_ptr<cache_job> artifact_cache_worker_job;
thread_local const std::unique_lock<std::mutex> *artifact_cache_borrowed_lock = nullptr;
struct artifact_cache_worker_job_scope
{
	~artifact_cache_worker_job_scope()
	{
		std::lock_guard<std::mutex> lock(store_mutex);
		artifact_cache_worker_job.reset();
	}
};

uint64_t operation_elapsed(uint64_t started_usec)
{
	const uint64_t finished_usec = redis_observability_now_usec();
	return finished_usec >= started_usec ? finished_usec - started_usec : 0;
}

size_t job_bytes(const std::shared_ptr<cache_job> &job)
{
	return job && job->value ? job->value->size() : 0;
}

bool cache_entry_expired(const local_cache_entry &entry, std::chrono::steady_clock::time_point now)
{
	return entry.expires != std::chrono::steady_clock::time_point{} && now >= entry.expires;
}

void prune_expired_locked()
{
	const auto now = std::chrono::steady_clock::now();
	for (auto iterator = local_cache.begin(); iterator != local_cache.end();)
	{
		if (cache_entry_expired(iterator->second, now))
			iterator = local_cache.erase(iterator);
		else
			++iterator;
	}
	health.local_entries = local_cache.size();
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
		artifact_cache_worker_job_scope lifetime;
		std::shared_ptr<cache_job> &job = artifact_cache_worker_job;
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
			const bool connected = context && !context->err;
			{
				std::lock_guard<std::mutex> lock(store_mutex);
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

		const uint64_t operation_started = redis_observability_now_usec();
		const bool succeeded = execute_job(context, job);
		const uint64_t operation_duration = operation_elapsed(operation_started);
		const redis_shared_command_outcome outcome =
			redis_command_outcome(context, succeeded);
		if (succeeded)
		{
			std::lock_guard<std::mutex> lock(store_mutex);
			redis_worker_operation_record(&health.operations, outcome,
						      operation_duration);
			remove_front_locked(false);
			++health.completed;
			health.busy = false;
			if (pending_jobs.empty())
				store_drained.notify_all();
			continue;
		}

		{
			std::lock_guard<std::mutex> lock(store_mutex);
			redis_worker_operation_record(&health.operations, outcome,
						      operation_duration);
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

std::shared_ptr<const std::string> local_value(const char *key)
{
	if (!key)
		return {};
	const size_t key_size = strnlen(key, REDIS_CACHE_MAX_KEY_BYTES + 1);
	if (!key_size || key_size > REDIS_CACHE_MAX_KEY_BYTES)
		return {};
	std::string owned_key;
	try
	{
		owned_key.assign(key, key_size);
	}
	catch (const std::bad_alloc &)
	{
		return {};
	}
	std::lock_guard<std::mutex> lock(store_mutex);
	if (!health.initialized)
		return {};
	auto found = local_cache.find(owned_key);
	if (found == local_cache.end())
		return {};
	if (cache_entry_expired(found->second, std::chrono::steady_clock::now()))
	{
		local_cache.erase(found);
		health.local_entries = local_cache.size();
		return {};
	}
	return found->second.value;
}
} // namespace

bool redis_cache_store_init(const struct redis_cache_store_config *config)
{
	if (!config || !config->connection)
		return false;
	std::lock_guard<std::mutex> lock(store_mutex);
	if (health.initialized)
		return true;
	try
	{
		configured_connection = config->connection;
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
		prune_expired_locked();
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
		prune_expired_locked();
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
	const std::shared_ptr<const std::string> value = local_value(key);
	if (!value)
		return nullptr;
	char *result = (char *)malloc(value->size() + 1);
	if (!result)
		return nullptr;
	memcpy(result, value->data(), value->size());
	result[value->size()] = '\0';
	return result;
}

char *redis_cache_store_transform(const char *key, redis_cache_store_transform_fn transform,
				  void *context)
{
	if (!transform)
		return nullptr;
	const std::shared_ptr<const std::string> value = local_value(key);
	return value ? transform(value->c_str(), context) : nullptr;
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
	redis_cache_store_health snapshot = health;
	redis_worker_operation_prepare_snapshot(&snapshot.operations);
	return snapshot;
}

void redis_cache_store_reset_for_tests(void)
{
	redis_cache_store_cancel();
	std::lock_guard<std::mutex> lock(store_mutex);
	pending_jobs.clear();
	local_cache.clear();
	health = {};
	configured_connection = nullptr;
	pending_bytes = 0;
	accepting = false;
	stop_requested = false;
}

namespace
{
bool artifact_cache_add(size_t &total, size_t extra) noexcept
{
	if (extra > SIZE_MAX - total)
		return false;
	total += extra;
	return true;
}
bool artifact_cache_string(const std::string &text, size_t &total) noexcept
{
	return text.capacity() <= 15 || artifact_cache_add(total, text.capacity() + 1);
}
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI == 1 && !defined(_GLIBCXX_DEBUG)
using artifact_cache_job_control =
	std::_Sp_counted_ptr_inplace<cache_job, std::allocator<void>, __gnu_cxx::_S_atomic>;
using artifact_cache_value_control =
	std::_Sp_counted_ptr_inplace<const std::string, std::allocator<void>, __gnu_cxx::_S_atomic>;
bool artifact_cache_same_value_before(const std::string *value, size_t pending_limit,
				      const std::string *local_limit, bool include_worker) noexcept
{
	if (!value)
		return true;
	if (include_worker && artifact_cache_worker_job &&
	    artifact_cache_worker_job->value.get() == value)
		return true;
	for (size_t index = 0; index < pending_limit; ++index)
		if (pending_jobs[index] && pending_jobs[index]->value.get() == value)
			return true;
	if (!local_limit)
		return false;
	for (const auto &entry : local_cache)
	{
		if (&entry.first == local_limit)
			break;
		if (entry.second.value.get() == value)
			return true;
	}
	return false;
}
bool artifact_cache_value(const std::shared_ptr<const std::string> &value, size_t &total) noexcept
{
	return !value || (artifact_cache_add(total, sizeof(artifact_cache_value_control)) &&
			  artifact_cache_string(*value, total));
}
bool artifact_cache_job(const std::shared_ptr<cache_job> &job, size_t &total) noexcept
{
	return !job || (artifact_cache_add(total, sizeof(artifact_cache_job_control)) &&
			artifact_cache_string(job->key, total));
}
#endif
bool artifact_cache_current_locked(size_t *output) noexcept
{
	if (!output)
		return false;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	_GLIBCXX_USE_CXX11_ABI != 1 || defined(_GLIBCXX_DEBUG)
	return false;
#else
	size_t total = sizeof(store_mutex) + sizeof(work_available) + sizeof(store_drained) +
		       sizeof(pending_jobs) + sizeof(local_cache) + sizeof(worker_thread) +
		       sizeof(health) + sizeof(configured_connection) + sizeof(pending_bytes) +
		       sizeof(accepting) + sizeof(stop_requested) +
		       sizeof(artifact_cache_worker_job) + sizeof(artifact_cache_borrowed_lock);
	size_t deque_heap = 0;
	if (!pending_jobs.current_heap_bytes(&deque_heap) || !artifact_cache_add(total, deque_heap))
		return false;
	// Observe the actual removed-but-still-live worker slot first. Shared control
	// allocations are counted once even when the same job/value is in the queue.
	if (!artifact_cache_job(artifact_cache_worker_job, total))
		return false;
	if (artifact_cache_worker_job &&
	    !artifact_cache_value(artifact_cache_worker_job->value, total))
		return false;
	for (size_t index = 0; index < pending_jobs.size(); ++index)
	{
		const auto &job = pending_jobs[index];
		bool seen = job.get() == artifact_cache_worker_job.get();
		for (size_t prior = 0; !seen && prior < index; ++prior)
			seen = pending_jobs[prior].get() == job.get();
		if (seen)
			continue;
		if (!artifact_cache_job(job, total))
			return false;
		if (job &&
		    !artifact_cache_same_value_before(job->value.get(), index, nullptr, true) &&
		    !artifact_cache_value(job->value, total))
			return false;
	}
	using map_value = std::pair<const std::string, local_cache_entry>;
	for (const auto &entry : local_cache)
	{
		if (!artifact_cache_add(total, sizeof(std::_Rb_tree_node<map_value>)) ||
		    !artifact_cache_string(entry.first, total))
			return false;
		if (!artifact_cache_same_value_before(entry.second.value.get(), pending_jobs.size(),
						      &entry.first, true) &&
		    !artifact_cache_value(entry.second.value, total))
			return false;
	}
	*output = total;
	return true;
#endif
}
}

bool redis_cache_store_retained_bytes(size_t *output) noexcept
{
	if (artifact_cache_borrowed_lock)
	{
		if (artifact_cache_borrowed_lock->mutex() != &store_mutex ||
		    !artifact_cache_borrowed_lock->owns_lock())
			return false;
		return artifact_cache_current_locked(output);
	}
	std::lock_guard<std::mutex> lock(store_mutex);
	return artifact_cache_current_locked(output);
}

int redis_cache_store_delete_bounded(const char *key, bool *submitted,
				     bool (*reserve)(size_t, void *) noexcept, void *context,
				     size_t outer) noexcept
{
	if (!key || !submitted || !reserve)
		return EINVAL;
	if (artifact_cache_borrowed_lock)
		return EDEADLK;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	_GLIBCXX_USE_CXX11_ABI != 1 || defined(_GLIBCXX_DEBUG)
	return ENOTSUP;
#else
	struct work
	{
		std::shared_ptr<cache_job> job;
		size_t key_size = 0, request = 0, deque_request = 0;
		bool append = false;
	};
	const size_t key_size = strnlen(key, REDIS_CACHE_MAX_KEY_BYTES + 1);
	if (!key_size || key_size > REDIS_CACHE_MAX_KEY_BYTES)
	{
		*submitted = false;
		return 0;
	}
	// Lock precedes the ROOT callback. The scoped same-thread borrow lets only its
	// current observer inspect these same stable owners without recursive locking.
	// No coordinator lock is held when this leaf is invoked by the native owner.
	std::unique_lock<std::mutex> lock(store_mutex);
	struct borrow
	{
		explicit borrow(const std::unique_lock<std::mutex> &owner)
		{
			artifact_cache_borrowed_lock = &owner;
		}
		~borrow() { artifact_cache_borrowed_lock = nullptr; }
	} borrowed(lock);
	work w;
	w.key_size = key_size;
	// Actual own key/submitted/reserve/context and outer arguments, distinct
	// key_size local, callback size/context/result and int return; the declared
	// first_replaceable/index/coalesces scope is conservatively held alongside
	// the original enqueue bytes/first_replaceable/index/candidate/replaced_bytes
	// and its job reference/boolean result. No captured peak supplies CURRENT.
	constexpr size_t delete_carriers = 4 * sizeof(void *) + 2 * sizeof(size_t) +
					   sizeof(size_t) + sizeof(void *) + sizeof(bool) +
					   sizeof(int) + 2 * sizeof(size_t) + sizeof(bool) +
					   5 * sizeof(size_t) + sizeof(void *) + sizeof(bool);
	w.request = sizeof(work) + sizeof(lock) + sizeof(borrowed) +
		    sizeof(artifact_cache_job_control) + delete_carriers;
	if (key_size > 15 && !artifact_cache_add(w.request, std::max(key_size, size_t(30)) + 1))
		return ENOBUFS;
	// Original job/key creation precedes the initialized/accepting check. Full
	// job construction still occurs in that branch, preserving false acceptance.
	if (health.initialized && accepting)
	{
		const size_t first_replaceable = health.busy ? 1 : 0;
		bool coalesces = false;
		for (size_t index = pending_jobs.size(); index > first_replaceable; --index)
			if (pending_jobs[index - 1]->key == key)
			{
				coalesces = true;
				break;
			}
		w.append = !coalesces && pending_jobs.size() < REDIS_CACHE_QUEUE_CAPACITY &&
			   pending_bytes <= REDIS_CACHE_QUEUE_MAX_BYTES;
		if (w.append && !pending_jobs.push_back_request(&w.deque_request))
			return ENOTSUP;
		if (!artifact_cache_add(w.request, w.deque_request))
			return ENOBUFS;
	}
	if (w.request > SIZE_MAX - outer || !reserve(outer + w.request, context))
		return ENOBUFS;
	try
	{
		w.job = std::make_shared<cache_job>();
		w.job->operation = cache_operation::remove;
		w.job->key.assign(key, key_size);
		if (!health.initialized || !accepting)
		{
			*submitted = false;
			return 0;
		}
		local_cache.erase(w.job->key);
		health.local_entries = local_cache.size();
		*submitted = enqueue_locked(w.job);
		return 0;
	}
	// The original cache deletion catches allocator failure as ordinary false;
	// artifact invalidation ignores that bool. Preserve it after real admission.
	catch (const std::bad_alloc &)
	{
		*submitted = false;
		return 0;
	}
	catch (...)
	{
		*submitted = false;
		return EOVERFLOW;
	}
#endif
}
