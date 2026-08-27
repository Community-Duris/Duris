#include "maintenance_scheduler.h"

#include "persistence_observability.h"

#include <mysql/mysql.h>

#include <algorithm>
#include <array>
#include <cerrno>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <deque>
#include <fcntl.h>
#include <mutex>
#include <new>
#include <string>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>

namespace
{
constexpr std::array<maintenance_job_definition, MAINTENANCE_JOB_COUNT> registry = { {
	{ maintenance_job_id::auction_due_scan, 240, 1, 64, 25000, true },
	{ maintenance_job_id::poll_expiration, 1200, 3, 64, 25000, true },
	{ maintenance_job_id::epic_task_catalog, 14400, 5, 256, 50000, true },
	{ maintenance_job_id::epic_zone_balance, 480, 2, 64, 25000, true },
	{ maintenance_job_id::level_cap, 240, 2, 32, 25000, true },
	{ maintenance_job_id::zone_trophy, 240, 4, 256, 50000, true },
	{ maintenance_job_id::epic_zone_modifiers, 240, 4, 256, 50000, true },
	{ maintenance_job_id::boon_scan, 240, 2, 64, 25000, true },
	{ maintenance_job_id::web_status, 300, 6, 1, 25000, true },
	{ maintenance_job_id::cargo_market, 240, 4, 100, 50000, true },
	{ maintenance_job_id::operational_statistics, 300, 6, 1, 50000, true },
	{ maintenance_job_id::lifecycle_archive, 2400, 7, 64, 25000, false },
} };

struct queued_job
{
	maintenance_request request;
	uint64_t queued_tick;
};

std::mutex scheduler_mutex;
std::condition_variable work_available;
std::deque<queued_job> queue;
std::deque<maintenance_result> completions;
std::thread worker;
maintenance_execute_fn execute_callback = nullptr;
void *execute_context = nullptr;
maintenance_prepare_fn prepare_callback = nullptr;
void *prepare_callback_context = nullptr;
maintenance_scheduler_health health = {};
uint64_t instance = 0;
uint64_t current_tick = 0;
bool stop_requested = false;
bool quiesced = false;
uint64_t executing_work_id = 0;
uint64_t executing_started_tick = 0;
std::string state_path;

struct durable_job_state
{
	uint64_t work_id;
	uint64_t cursor;
	bool request_pending;
	maintenance_request request;
	bool completion_pending;
	maintenance_result completion;
};

struct durable_scheduler_state
{
	char magic[8];
	uint32_t version;
	uint32_t job_count;
	std::array<durable_job_state, MAINTENANCE_JOB_COUNT> jobs;
	uint64_t checksum;
};

struct durable_scheduler_state_v2
{
	char magic[8];
	uint32_t version;
	uint32_t job_count;
	std::array<durable_job_state, 11> jobs;
	uint64_t checksum;
};

durable_scheduler_state durable_state = {};
bool durable_state_dirty = false;

uint64_t state_checksum(const durable_scheduler_state &state)
{
	const auto *bytes = reinterpret_cast<const uint8_t *>(&state);
	const size_t length = offsetof(durable_scheduler_state, checksum);
	uint64_t hash = 1469598103934665603ULL;
	for (size_t index = 0; index < length; ++index)
	{
		hash ^= bytes[index];
		hash *= 1099511628211ULL;
	}
	return hash;
}

uint64_t state_checksum(const durable_scheduler_state_v2 &state)
{
	const auto *bytes = reinterpret_cast<const uint8_t *>(&state);
	const size_t length = offsetof(durable_scheduler_state_v2, checksum);
	uint64_t hash = 1469598103934665603ULL;
	for (size_t index = 0; index < length; ++index)
	{
		hash ^= bytes[index];
		hash *= 1099511628211ULL;
	}
	return hash;
}

bool write_all(int descriptor, const void *raw, size_t size)
{
	const auto *bytes = static_cast<const uint8_t *>(raw);
	while (size)
	{
		const ssize_t written = write(descriptor, bytes, size);
		if (written < 0 && errno == EINTR)
			continue;
		if (written <= 0)
			return false;
		bytes += written;
		size -= static_cast<size_t>(written);
	}
	return true;
}

bool persist_state(const durable_scheduler_state &state)
{
	if (state_path.empty())
		return true;
	durable_scheduler_state stored = state;
	stored.checksum = state_checksum(stored);
	const std::string temporary = state_path + ".tmp";
	const int descriptor = open(temporary.c_str(),
				    O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC | O_NOFOLLOW, 0600);
	if (descriptor < 0)
		return false;
	const bool written = write_all(descriptor, &stored, sizeof(stored));
	const bool synced = written && fsync(descriptor) == 0;
	const bool closed = close(descriptor) == 0;
	if (!synced || !closed || rename(temporary.c_str(), state_path.c_str()) != 0)
	{
		unlink(temporary.c_str());
		return false;
	}
	return true;
}

bool load_state()
{
	durable_state = {};
	memcpy(durable_state.magic, "DMSMNT3", 7);
	durable_state.version = 3;
	durable_state.job_count = MAINTENANCE_JOB_COUNT;
	if (state_path.empty())
		return true;
	const int descriptor = open(state_path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
	if (descriptor < 0)
		return errno == ENOENT;
	struct stat metadata = {};
	if (fstat(descriptor, &metadata) != 0 || !S_ISREG(metadata.st_mode))
	{
		close(descriptor);
		return false;
	}
	if (metadata.st_size == static_cast<off_t>(sizeof(durable_scheduler_state_v2)))
	{
		durable_scheduler_state_v2 loaded_v2 = {};
		auto *old_bytes = reinterpret_cast<uint8_t *>(&loaded_v2);
		size_t old_remaining = sizeof(loaded_v2);
		while (old_remaining)
		{
			const ssize_t count = read(descriptor, old_bytes, old_remaining);
			if (count < 0 && errno == EINTR)
				continue;
			if (count <= 0)
				break;
			old_bytes += count;
			old_remaining -= static_cast<size_t>(count);
		}
		const bool closed = close(descriptor) == 0;
		if (old_remaining || !closed || memcmp(loaded_v2.magic, "DMSMNT2", 7) != 0 ||
		    loaded_v2.version != 2 || loaded_v2.job_count != 11 ||
		    loaded_v2.checksum != state_checksum(loaded_v2))
			return false;
		for (size_t index = 0; index < loaded_v2.jobs.size(); ++index)
			durable_state.jobs[index] = loaded_v2.jobs[index];
	}
	else if (metadata.st_size == static_cast<off_t>(sizeof(durable_scheduler_state)))
	{
		durable_scheduler_state loaded = {};
		auto *bytes = reinterpret_cast<uint8_t *>(&loaded);
		size_t remaining = sizeof(loaded);
		while (remaining)
		{
			const ssize_t count = read(descriptor, bytes, remaining);
			if (count < 0 && errno == EINTR)
				continue;
			if (count <= 0)
				break;
			bytes += count;
			remaining -= static_cast<size_t>(count);
		}
		const bool closed = close(descriptor) == 0;
		if (remaining || !closed || memcmp(loaded.magic, "DMSMNT3", 7) != 0 ||
		    loaded.version != 3 || loaded.job_count != MAINTENANCE_JOB_COUNT ||
		    loaded.checksum != state_checksum(loaded))
			return false;
		durable_state = loaded;
	}
	else
	{
		close(descriptor);
		return false;
	}
	for (const auto &job : durable_state.jobs)
		if ((!job.work_id &&
		     (job.cursor || job.request_pending || job.completion_pending)) ||
		    (job.request_pending &&
		     (job.request.work_id != job.work_id || job.request.cursor != job.cursor ||
		      static_cast<size_t>(job.request.job_id) >= MAINTENANCE_JOB_COUNT)) ||
		    (job.completion_pending &&
		     (job.completion.work_id != job.work_id ||
		      static_cast<size_t>(job.completion.job_id) >= MAINTENANCE_JOB_COUNT)))
			return false;
	return true;
}

size_t index_of(maintenance_job_id id)
{
	return static_cast<size_t>(id);
}

uint64_t next_work_id(maintenance_job_id id, uint64_t scheduled_tick)
{
	return (scheduled_tick << 8) | (static_cast<uint64_t>(id) + 1);
}

void refresh_health_locked()
{
	health.queued = queue.size();
	health.inflight = executing_work_id ? 1 : 0;
	health.completions = completions.size();
	health.high_water = std::max(health.high_water, health.queued + health.inflight);
	health.oldest_queue_age_ticks = queue.empty() || current_tick < queue.front().queued_tick ?
						0 :
						current_tick - queue.front().queued_tick;
	health.inflight_age_ticks = executing_work_id && current_tick >= executing_started_tick ?
					    current_tick - executing_started_tick :
					    0;
}

void worker_main()
{
	if (mysql_thread_init() != 0)
	{
		std::lock_guard<std::mutex> lock(scheduler_mutex);
		health.running = false;
		stop_requested = true;
		return;
	}
	for (;;)
	{
		queued_job queued = {};
		{
			std::unique_lock<std::mutex> lock(scheduler_mutex);
			work_available.wait(lock,
					    [] {
						    return stop_requested || durable_state_dirty ||
							   !queue.empty();
					    });
			if (durable_state_dirty && queue.empty())
			{
				const durable_scheduler_state state = durable_state;
				durable_state_dirty = false;
				lock.unlock();
				if (!persist_state(state))
				{
					lock.lock();
					if (!stop_requested)
						durable_state_dirty = true;
				}
				continue;
			}
			if (stop_requested && queue.empty())
				break;
			queued = queue.front();
			queue.pop_front();
			executing_work_id = queued.request.work_id;
			executing_started_tick = current_tick;
			refresh_health_locked();
		}
		maintenance_result result = { queued.request.work_id,
					      queued.request.job_id,
					      maintenance_outcome::permanent_failure,
					      queued.request.cursor,
					      0,
					      0,
					      0,
					      0,
					      {} };
		const uint64_t started = persistence_observability_now_usec();
		durable_scheduler_state active_state = {};
		{
			std::lock_guard<std::mutex> lock(scheduler_mutex);
			auto &durable = durable_state.jobs[index_of(queued.request.job_id)];
			durable.work_id = queued.request.work_id;
			durable.cursor = queued.request.cursor;
			durable.request_pending = true;
			durable.request = queued.request;
			durable.completion_pending = false;
			durable.completion = {};
			active_state = durable_state;
		}
		if (!persist_state(active_state))
		{
			result.outcome = maintenance_outcome::retryable_failure;
			result.error_code = EIO;
		}
		try
		{
			if (!result.error_code)
				result = execute_callback(queued.request, execute_context);
		}
		catch (const std::bad_alloc &)
		{
			result.outcome = maintenance_outcome::retryable_failure;
			result.error_code = ENOMEM;
		}
		catch (...)
		{
			result.outcome = maintenance_outcome::permanent_failure;
			result.error_code = EFAULT;
		}
		if (result.work_id != queued.request.work_id ||
		    result.job_id != queued.request.job_id)
		{
			result = { queued.request.work_id,
				   queued.request.job_id,
				   maintenance_outcome::permanent_failure,
				   queued.request.cursor,
				   0,
				   persistence_observability_now_usec() - started,
				   EPROTO,
				   0,
				   {} };
		}
		else
			result.run_usec = persistence_observability_now_usec() - started;
		durable_scheduler_state completed_state = {};
		{
			std::lock_guard<std::mutex> lock(scheduler_mutex);
			auto &durable = durable_state.jobs[index_of(queued.request.job_id)];
			durable.completion_pending = true;
			durable.completion = result;
			completed_state = durable_state;
		}
		if (!persist_state(completed_state))
		{
			result.outcome = maintenance_outcome::retryable_failure;
			result.error_code = EIO;
			std::lock_guard<std::mutex> lock(scheduler_mutex);
			auto &durable = durable_state.jobs[index_of(queued.request.job_id)];
			durable.completion_pending = false;
			durable.completion = {};
		}
		{
			std::lock_guard<std::mutex> lock(scheduler_mutex);
			executing_work_id = 0;
			executing_started_tick = 0;
			if (stop_requested)
				result.outcome = maintenance_outcome::cancelled;
			completions.push_back(result);
			refresh_health_locked();
			work_available.notify_all();
		}
	}
	mysql_thread_end();
}

void submit_due_locked(uint64_t tick)
{
	const maintenance_job_definition *selected = nullptr;
	for (const auto &definition : registry)
	{
		if (!definition.enabled)
			continue;
		auto &job = health.jobs[index_of(definition.id)];
		if (tick < job.next_due_tick)
			continue;
		if (job.active)
		{
			++job.overlap_suppressed;
			job.next_due_tick += definition.cadence_ticks;
			continue;
		}
		if (!selected || definition.priority < selected->priority ||
		    (definition.priority == selected->priority &&
		     job.next_due_tick < health.jobs[index_of(selected->id)].next_due_tick))
			selected = &definition;
	}
	if (!selected || queue.size() >= MAINTENANCE_QUEUE_MAX ||
	    completions.size() >= MAINTENANCE_COMPLETION_MAX)
		return;
	auto &job = health.jobs[index_of(selected->id)];
	const uint64_t work_id = job.work_id ? job.work_id :
					       next_work_id(selected->id, job.next_due_tick);
	maintenance_request request = { work_id,
					selected->id,
					job.cursor,
					job.next_due_tick,
					persistence_observability_now_usec() +
						selected->time_budget_usec,
					selected->row_budget,
					selected->time_budget_usec };
	if (prepare_callback && !prepare_callback(request, prepare_callback_context))
	{
		job.next_due_tick += selected->cadence_ticks;
		return;
	}
	queue.push_back({ request, tick });
	job.work_id = work_id;
	job.active = true;
	++job.submitted;
	job.next_due_tick += selected->cadence_ticks;
	refresh_health_locked();
	work_available.notify_one();
}

void apply_completion_locked(const maintenance_result &result, uint64_t tick)
{
	auto &job = health.jobs[index_of(result.job_id)];
	if (!job.active || job.work_id != result.work_id)
		return;
	job.last_run_usec = result.run_usec;
	job.rows += result.rows;
	job.active = false;
	switch (result.outcome)
	{
	case maintenance_outcome::complete:
		job.cursor = 0;
		job.work_id = 0;
		job.retries = 0;
		++job.completed;
		break;
	case maintenance_outcome::more:
		job.cursor = result.next_cursor;
		job.next_due_tick = tick + 1;
		++job.completed;
		break;
	case maintenance_outcome::retryable_failure:
		++job.retries;
		job.next_due_tick =
			tick + std::min<uint64_t>(1ULL << std::min<uint64_t>(job.retries, 7),
						  MAINTENANCE_RETRY_MAX_TICKS);
		break;
	case maintenance_outcome::permanent_failure:
		++job.failures;
		job.cursor = 0;
		job.work_id = 0;
		job.retries = 0;
		break;
	case maintenance_outcome::cancelled:
		break;
	}
	auto &durable = durable_state.jobs[index_of(result.job_id)];
	durable.work_id = job.work_id;
	durable.cursor = job.cursor;
	durable.request_pending = false;
	durable.request = {};
	durable.completion_pending = false;
	durable.completion = {};
	durable_state_dirty = true;
	work_available.notify_one();
}
} // namespace

const maintenance_job_definition *maintenance_registry(size_t *count)
{
	if (count)
		*count = registry.size();
	return registry.data();
}

const char *maintenance_job_name(maintenance_job_id id)
{
	constexpr const char *names[] = { "auction_due_scan",
					  "poll_expiration",
					  "epic_task_catalog",
					  "epic_zone_balance",
					  "level_cap",
					  "zone_trophy",
					  "epic_zone_modifiers",
					  "boon_scan",
					  "web_status",
					  "cargo_market",
					  "operational_statistics",
					  "lifecycle_archive" };
	const size_t index = index_of(id);
	return index < MAINTENANCE_JOB_COUNT ? names[index] : "invalid";
}

uint64_t maintenance_job_offset(maintenance_job_id id, uint64_t cadence_ticks,
				uint64_t instance_seed)
{
	if (!cadence_ticks)
		return 0;
	uint64_t hash = 1469598103934665603ULL;
	for (unsigned shift = 0; shift < 64; shift += 8)
	{
		hash ^= (instance_seed >> shift) & 0xff;
		hash *= 1099511628211ULL;
	}
	hash ^= static_cast<uint8_t>(id) + 1;
	hash *= 1099511628211ULL;
	return hash % cadence_ticks;
}

bool maintenance_activity_due(uint64_t tick, uint64_t cadence_ticks, uint64_t activity_id)
{
	if (!cadence_ticks)
		return false;
	uint64_t seed = 0;
	{
		std::lock_guard<std::mutex> lock(scheduler_mutex);
		seed = instance;
	}
	uint64_t hash = 1469598103934665603ULL;
	for (unsigned shift = 0; shift < 64; shift += 8)
	{
		hash ^= ((seed ^ activity_id) >> shift) & 0xff;
		hash *= 1099511628211ULL;
	}
	const uint64_t offset = hash % cadence_ticks;
	return tick >= offset && (tick - offset) % cadence_ticks == 0;
}

bool maintenance_scheduler_init(uint64_t instance_seed, maintenance_execute_fn execute,
				void *context, maintenance_prepare_fn prepare,
				void *prepare_context)
{
	if (!execute)
		return false;
	std::lock_guard<std::mutex> lock(scheduler_mutex);
	if (health.running || worker.joinable())
		return false;
	if (!load_state())
		return false;
	health = {};
	instance = instance_seed;
	execute_callback = execute;
	execute_context = context;
	prepare_callback = prepare;
	prepare_callback_context = prepare_context;
	stop_requested = false;
	quiesced = false;
	durable_state_dirty = false;
	executing_work_id = 0;
	executing_started_tick = 0;
	for (const auto &definition : registry)
	{
		auto &job = health.jobs[index_of(definition.id)];
		job.enabled = definition.enabled;
		job.offset_ticks =
			maintenance_job_offset(definition.id, definition.cadence_ticks, instance);
		job.next_due_tick = job.offset_ticks;
		job.work_id = durable_state.jobs[index_of(definition.id)].work_id;
		job.cursor = durable_state.jobs[index_of(definition.id)].cursor;
		if (job.work_id)
		{
			job.next_due_tick = 0;
			if (durable_state.jobs[index_of(definition.id)].completion_pending)
			{
				job.active = true;
				completions.push_back(
					durable_state.jobs[index_of(definition.id)].completion);
			}
			else if (durable_state.jobs[index_of(definition.id)].request_pending)
			{
				job.active = true;
				auto request = durable_state.jobs[index_of(definition.id)].request;
				request.deadline_usec = persistence_observability_now_usec() +
							request.time_budget_usec;
				queue.push_back({ request, 0 });
			}
		}
	}
	health.running = true;
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

bool maintenance_scheduler_set_state_path(const char *path)
{
	std::lock_guard<std::mutex> lock(scheduler_mutex);
	if (health.running || worker.joinable() || !path || !*path || strlen(path) >= 4096)
		return false;
	state_path = path;
	return true;
}

size_t maintenance_scheduler_pulse(uint64_t tick, maintenance_result *results, size_t capacity)
{
	std::lock_guard<std::mutex> lock(scheduler_mutex);
	current_tick = tick;
	size_t count = 0;
	while (results && count < capacity && !completions.empty())
	{
		results[count] = completions.front();
		completions.pop_front();
		apply_completion_locked(results[count], tick);
		++count;
	}
	if (health.running && !stop_requested && !quiesced)
		submit_due_locked(tick);
	refresh_health_locked();
	return count;
}

void maintenance_scheduler_quiesce(void)
{
	std::lock_guard<std::mutex> lock(scheduler_mutex);
	quiesced = true;
}

bool maintenance_scheduler_drain(uint64_t timeout_msec)
{
	std::unique_lock<std::mutex> lock(scheduler_mutex);
	if (!quiesced)
		return false;
	const auto deadline =
		std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_msec);
	return work_available.wait_until(lock, deadline,
					 [] { return queue.empty() && !executing_work_id; });
}

void maintenance_scheduler_resume(void)
{
	std::lock_guard<std::mutex> lock(scheduler_mutex);
	if (!stop_requested && health.running)
		quiesced = false;
}

void maintenance_scheduler_shutdown(void)
{
	{
		std::lock_guard<std::mutex> lock(scheduler_mutex);
		stop_requested = true;
		health.stop_pending = true;
		queue.clear();
		work_available.notify_all();
	}
	if (worker.joinable())
		worker.join();
	std::lock_guard<std::mutex> lock(scheduler_mutex);
	queue.clear();
	completions.clear();
	for (auto &job : health.jobs)
		job.active = false;
	health.running = false;
	health.stop_pending = false;
	quiesced = false;
	executing_work_id = 0;
	executing_started_tick = 0;
	execute_callback = nullptr;
	execute_context = nullptr;
	prepare_callback = nullptr;
	prepare_callback_context = nullptr;
	refresh_health_locked();
}

maintenance_scheduler_health maintenance_scheduler_health_copy(uint64_t tick)
{
	std::lock_guard<std::mutex> lock(scheduler_mutex);
	current_tick = tick;
	refresh_health_locked();
	return health;
}

void maintenance_scheduler_reset_for_tests(void)
{
	maintenance_scheduler_shutdown();
	std::lock_guard<std::mutex> lock(scheduler_mutex);
	health = {};
	durable_state = {};
	durable_state_dirty = false;
	instance = 0;
	current_tick = 0;
	stop_requested = false;
	quiesced = false;
	executing_work_id = 0;
	executing_started_tick = 0;
	state_path.clear();
}
