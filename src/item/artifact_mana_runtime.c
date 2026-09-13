#include "item/artifact_mana_runtime.h"

#include <array>
#include <condition_variable>
#include <map>
#include <mutex>
#include <thread>

namespace
{
constexpr size_t queue_capacity = 4096;
constexpr size_t cache_capacity = 65536;
constexpr uint64_t maximum_unacknowledged_ms = 2000;

struct job
{
	uint64_t uid = 0;
	uint64_t expected = 0;
	bool write = false;
	artifact_mana_record record;
};

struct completion
{
	job request;
	artifact_mana_read read = artifact_mana_read::error;
	bool written = false;
};

struct cached_pool
{
	artifact_mana_record record;
	artifact_mana_record writing;
	uint64_t durable_version = 0;
	uint64_t dirty_since = 0;
	uint64_t retry_at = 0;
	uint64_t last_token = 0;
	bool loaded = false;
	bool busy = false;
};

class persistent_backend final : public artifact_mana_backend
{
	const bool sql;
	const std::string root;

    public:
	persistent_backend(bool mysql, const std::string &directory)
		: sql(mysql)
		, root(directory)
	{
	}
	artifact_mana_read read(uint64_t uid, artifact_mana_record &record) override
	{
		return artifact_mana_store_read(sql, root, uid, record);
	}
	bool write(uint64_t expected, const artifact_mana_record &record) override
	{
		return artifact_mana_store_write(sql, root, expected, record);
	}
};
} // namespace

struct artifact_mana_runtime::implementation
{
	std::unique_ptr<artifact_mana_backend> backend;
	std::map<uint64_t, cached_pool> pools;
	std::array<job, queue_capacity> jobs = {};
	std::array<completion, queue_capacity> completions = {};
	mutable std::mutex mutex;
	std::condition_variable wake;
	std::thread worker;
	size_t jobs_head = 0, jobs_count = 0;
	size_t completions_head = 0, completions_count = 0;
	size_t outstanding = 0;
	uint64_t failures = 0;
	bool stopping = false;

	explicit implementation(std::unique_ptr<artifact_mana_backend> value)
		: backend(std::move(value))
	{
	}

	void run()
	{
		for (;;)
		{
			completion result;
			{
				std::unique_lock lock(mutex);
				wake.wait(lock, [&] { return stopping || jobs_count; });
				if (stopping)
					return;
				result.request = jobs[jobs_head];
				jobs_head = (jobs_head + 1) % queue_capacity;
				--jobs_count;
			}
			try
			{
				if (result.request.write)
					result.written = backend->write(result.request.expected,
									result.request.record);
				else
					result.read = backend->read(result.request.uid,
								    result.request.record);
			}
			catch (...)
			{
				// Keep the request unacknowledged. No optimistic reset on I/O failure.
			}
			{
				std::lock_guard lock(mutex);
				completions[(completions_head + completions_count) % queue_capacity] =
					result;
				++completions_count;
			}
		}
	}

	bool submit(const job &request)
	{
		std::lock_guard lock(mutex);
		if (stopping || !backend || outstanding == queue_capacity)
			return false;
		if (!worker.joinable())
		{
			try
			{
				worker = std::thread([this] { run(); });
			}
			catch (...)
			{
				return false;
			}
		}
		jobs[(jobs_head + jobs_count) % queue_capacity] = request;
		++jobs_count;
		++outstanding;
		wake.notify_one();
		return true;
	}

	void schedule(uint64_t uid, cached_pool &pool, uint64_t mono)
	{
		if (pool.busy || mono < pool.retry_at)
			return;
		if (!pool.loaded)
			pool.busy = submit({ uid, 0, false, {} });
		else if (pool.record.version != pool.durable_version)
		{
			if (!pool.writing.version)
				pool.writing = pool.record;
			pool.busy = submit({ uid, pool.durable_version, true, pool.writing });
		}
	}

	cached_pool *get(uint64_t uid, const artifact_mana_profile &profile, uint64_t wall,
			 uint64_t mono)
	{
		if (!uid || !mono || !artifact_mana_valid(profile) || stopping)
			return nullptr;
		auto found = pools.find(uid);
		if (found == pools.end())
		{
			if (pools.size() == cache_capacity)
				return nullptr;
			found = pools.emplace(uid, cached_pool{}).first;
		}
		auto &pool = found->second;
		if (pool.loaded && !pool.record.version)
		{
			pool.record = artifact_mana_empty(uid, profile, wall);
			pool.dirty_since = mono;
		}
		schedule(uid, pool, mono);
		// Empty enrollment must be durable before this instance can spend.
		return pool.loaded && pool.durable_version ? &pool : nullptr;
	}
};

artifact_mana_runtime::artifact_mana_runtime(std::unique_ptr<artifact_mana_backend> backend)
	: impl(std::make_unique<implementation>(std::move(backend)))
{
}

artifact_mana_runtime::~artifact_mana_runtime()
{
	stop();
}

bool artifact_mana_runtime::inspect(uint64_t uid, const artifact_mana_profile &profile,
				    uint64_t wall, uint64_t mono, artifact_mana_record &record)
{
	auto *pool = impl->get(uid, profile, wall, mono);
	if (!pool)
		return false;
	record = pool->record;
	return artifact_mana_project(record, profile, wall);
}

bool artifact_mana_runtime::debit(uint64_t uid, const artifact_mana_profile &profile, uint64_t wall,
				  uint64_t mono, uint64_t cost, bool passive, uint64_t token)
{
	auto *pool = impl->get(uid, profile, wall, mono);
	if (!pool || !token || token <= pool->last_token ||
	    (pool->dirty_since &&
	     (mono < pool->dirty_since || mono - pool->dirty_since >= maximum_unacknowledged_ms)))
		return false;
	auto next = pool->record;
	if (!artifact_mana_spend(next, profile, wall, cost, passive))
		return false;
	// One in-flight write per UID. Later spends are coalesced behind that write;
	// its exact version is acknowledged before a subsequent CAS is submitted.
	if (!pool->busy)
	{
		const auto snapshot = pool->writing.version ? pool->writing : next;
		if (!impl->submit({ uid, pool->durable_version, true, snapshot }))
			return false;
		pool->writing = snapshot;
	}
	pool->busy = true;
	pool->record = next;
	pool->last_token = token;
	if (!pool->dirty_since)
		pool->dirty_since = mono;
	return true;
}

void artifact_mana_runtime::pulse(uint64_t mono)
{
	for (size_t count = 0; count < queue_capacity; ++count)
	{
		completion result;
		{
			std::lock_guard lock(impl->mutex);
			if (!impl->completions_count)
				break;
			result = impl->completions[impl->completions_head];
			impl->completions_head = (impl->completions_head + 1) % queue_capacity;
			--impl->completions_count;
			--impl->outstanding;
		}
		auto &pool = impl->pools.at(result.request.uid);
		pool.busy = false;
		if (result.request.write && result.written)
		{
			pool.durable_version = result.request.record.version;
			pool.writing = {};
			if (pool.durable_version == pool.record.version)
				pool.dirty_since = 0;
		}
		else if (!result.request.write && result.read != artifact_mana_read::error)
		{
			pool.loaded = true;
			pool.record = result.read == artifact_mana_read::found ?
					      result.request.record :
					      artifact_mana_record{};
			pool.durable_version = pool.record.version;
		}
		else
		{
			++impl->failures;
			pool.retry_at = mono + 1000;
		}
	}
	for (auto &[uid, pool] : impl->pools)
		impl->schedule(uid, pool, mono);
}

artifact_mana_health artifact_mana_runtime::health() const
{
	artifact_mana_health result;
	result.cached = impl->pools.size();
	for (const auto &[uid, pool] : impl->pools)
	{
		(void)uid;
		if (pool.record.version != pool.durable_version)
			++result.dirty;
	}
	std::lock_guard lock(impl->mutex);
	result.outstanding = impl->outstanding;
	result.write_failures = impl->failures;
	return result;
}

bool artifact_mana_runtime::spending_ready(uint64_t uid, uint64_t mono) const
{
	const auto found = impl->pools.find(uid);
	if (impl->stopping || !mono || found == impl->pools.end() || !found->second.durable_version)
		return false;
	const auto since = found->second.dirty_since;
	return !since || (mono >= since && mono - since < maximum_unacknowledged_ms);
}

void artifact_mana_runtime::stop()
{
	{
		std::lock_guard lock(impl->mutex);
		impl->stopping = true;
		impl->wake.notify_one();
	}
	if (impl->worker.joinable())
		impl->worker.join();
}

std::unique_ptr<artifact_mana_backend> artifact_mana_persistent_backend(bool sql,
									const std::string &root)
{
	return std::make_unique<persistent_backend>(sql, root);
}
