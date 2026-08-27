#include "critical_command_coordinator.h"

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <new>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

namespace
{
struct operation_state
{
	critical_command command;
	size_t retained_bytes;
	uint64_t queued_at_usec;
	unsigned int attempt;
	uint64_t attachments;
	bool inflight;
	bool completed;
	bool blocked;
};

struct completed_state
{
	critical_command command;
	size_t encoded_size;
};

std::mutex coordinator_mutex;
std::condition_variable work_available;
std::condition_variable result_available;
std::unordered_map<std::string, std::unique_ptr<operation_state>> operations;
std::deque<std::string> pending;
std::deque<critical_completion> raw_results;
std::unordered_map<std::string, std::string> active_keys;
std::unordered_map<std::string, std::deque<std::string>> fences;
std::unordered_map<std::string, completed_state> completed_cache;
std::deque<std::string> completed_order;
size_t completed_cache_bytes = 0;
std::vector<std::thread> workers;
critical_apply_fn apply_callback = nullptr;
void *apply_context = nullptr;
critical_coordinator_health health = {};
bool stop_requested = false;

uint64_t now_usec()
{
	return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(
					     std::chrono::steady_clock::now().time_since_epoch())
					     .count());
}

uint64_t wall_now_usec()
{
	return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(
					     std::chrono::system_clock::now().time_since_epoch())
					     .count());
}

std::string operation_key(const critical_operation_id &operation_id)
{
	return std::string(reinterpret_cast<const char *>(operation_id.bytes.data()),
			   operation_id.bytes.size());
}

std::string entity_key(const critical_entity_key &key)
{
	std::string encoded(9, '\0');
	encoded[0] = static_cast<char>(key.type);
	for (unsigned int index = 0; index < 8; ++index)
		encoded[index + 1] = static_cast<char>(key.id >> (index * 8));
	return encoded;
}

bool keys_available(const std::string &identity, const critical_command &command)
{
	for (const critical_entity_key &key : command.keys)
	{
		const std::string encoded = entity_key(key);
		auto fence = fences.find(encoded);
		if (active_keys.find(encoded) != active_keys.end() || fence == fences.end() ||
		    fence->second.empty() || fence->second.front() != identity)
			return false;
	}
	return true;
}

void acquire_keys(const std::string &identity, const critical_command &command)
{
	for (const critical_entity_key &key : command.keys)
		active_keys.emplace(entity_key(key), identity);
}

void release_keys(const std::string &identity, const critical_command &command)
{
	for (const critical_entity_key &key : command.keys)
	{
		auto found = active_keys.find(entity_key(key));
		if (found != active_keys.end() && found->second == identity)
			active_keys.erase(found);
	}
}

void add_fences(const std::string &identity, const critical_command &command)
{
	for (const critical_entity_key &key : command.keys)
		fences[entity_key(key)].push_back(identity);
	health.fenced_keys = fences.size();
}

void remove_fences(const std::string &identity, const critical_command &command)
{
	for (const critical_entity_key &key : command.keys)
	{
		auto found = fences.find(entity_key(key));
		if (found == fences.end())
			continue;
		auto &identities = found->second;
		identities.erase(std::remove(identities.begin(), identities.end(), identity),
				 identities.end());
		if (identities.empty())
			fences.erase(found);
	}
	health.fenced_keys = fences.size();
}

void update_depth()
{
	health.queued = 0;
	health.inflight = 0;
	health.blocked = 0;
	health.retained_bytes = 0;
	uint64_t oldest = 0;
	const uint64_t now = now_usec();
	for (const auto &[identity, state] : operations)
	{
		(void)identity;
		if (state->completed)
			continue;
		if (state->blocked)
			++health.blocked;
		else if (state->inflight)
			++health.inflight;
		else
			++health.queued;
		health.retained_bytes += state->retained_bytes;
		if (!oldest || state->queued_at_usec < oldest)
			oldest = state->queued_at_usec;
	}
	health.oldest_age_msec = oldest && now > oldest ? (now - oldest) / 1000 : 0;
	health.high_water_operations =
		std::max(health.high_water_operations, health.queued + health.inflight);
	health.high_water_bytes = std::max(health.high_water_bytes, health.retained_bytes);
	health.completed_cache = completed_cache.size();
}

void remember_completed(const std::string &identity, const critical_command &command)
{
	std::vector<uint8_t> encoded;
	if (critical_command_encode(command, &encoded) != critical_command_codec_result::ok ||
	    encoded.size() > CRITICAL_COORDINATOR_COMPLETED_CACHE_BYTES)
		return;
	while (!completed_order.empty() &&
	       (completed_cache.size() >= CRITICAL_COORDINATOR_COMPLETED_CACHE_MAX ||
		completed_cache_bytes >
			CRITICAL_COORDINATOR_COMPLETED_CACHE_BYTES - encoded.size()))
	{
		auto found = completed_cache.find(completed_order.front());
		if (found != completed_cache.end())
		{
			completed_cache_bytes -= found->second.encoded_size;
			completed_cache.erase(found);
		}
		completed_order.pop_front();
	}
	try
	{
		completed_cache.emplace(identity,
					completed_state{ .command = command,
							 .encoded_size = encoded.size() });
		completed_order.push_back(identity);
		completed_cache_bytes += encoded.size();
	}
	catch (const std::bad_alloc &)
	{
		completed_cache.erase(identity);
	}
}

bool enqueue_replayed(critical_command command, void *)
{
	std::vector<uint8_t> encoded;
	if (!critical_command_valid(command) ||
	    critical_command_encode(command, &encoded) != critical_command_codec_result::ok)
		return false;
	const std::string identity = operation_key(command.operation_id);
	if (operations.find(identity) != operations.end() ||
	    operations.size() >= CRITICAL_COORDINATOR_MAX_OPERATIONS ||
	    encoded.size() > CRITICAL_COORDINATOR_MAX_BYTES - health.retained_bytes)
		return false;
	try
	{
		auto state = std::make_unique<operation_state>();
		state->command = std::move(command);
		state->retained_bytes = encoded.size();
		state->queued_at_usec = now_usec();
		state->attempt = 1;
		state->attachments = 0;
		state->inflight = false;
		state->completed = false;
		state->blocked = false;
		operations.emplace(identity, std::move(state));
		pending.push_back(identity);
		add_fences(identity, operations.at(identity)->command);
	}
	catch (const std::bad_alloc &)
	{
		auto inserted = operations.find(identity);
		if (inserted != operations.end())
		{
			remove_fences(identity, inserted->second->command);
			operations.erase(inserted);
		}
		pending.erase(std::remove(pending.begin(), pending.end(), identity), pending.end());
		return false;
	}
	update_depth();
	return true;
}

void worker_main()
{
	for (;;)
	{
		std::string identity;
		critical_command command;
		unsigned int attempt = 0;
		uint64_t queued_at = 0;
		{
			std::unique_lock<std::mutex> lock(coordinator_mutex);
			work_available.wait(
				lock,
				[]
				{
					if (stop_requested)
						return true;
					for (const std::string &candidate : pending)
					{
						auto found = operations.find(candidate);
						if (found != operations.end() &&
						    !found->second->completed &&
						    !found->second->inflight &&
						    keys_available(candidate,
								   found->second->command))
							return true;
					}
					return false;
				});
			if (stop_requested)
				return;
			auto ready = pending.end();
			for (auto iterator = pending.begin(); iterator != pending.end(); ++iterator)
			{
				auto found = operations.find(*iterator);
				if (found != operations.end() && !found->second->completed &&
				    !found->second->inflight &&
				    keys_available(*iterator, found->second->command))
				{
					ready = iterator;
					break;
				}
			}
			if (ready == pending.end())
				continue;
			identity = *ready;
			pending.erase(ready);
			operation_state &state = *operations.at(identity);
			state.inflight = true;
			acquire_keys(identity, state.command);
			try
			{
				command = state.command;
			}
			catch (const std::bad_alloc &)
			{
				release_keys(identity, state.command);
				state.inflight = false;
				state.blocked = true;
				++health.terminal_failures;
				update_depth();
				continue;
			}
			attempt = state.attempt;
			queued_at = state.queued_at_usec;
			update_depth();
		}
		const uint64_t started = now_usec();
		critical_apply_result applied = {};
		try
		{
			applied = apply_callback(command, apply_context);
		}
		catch (...)
		{
			applied = { critical_apply_outcome::retryable_failure, 0, 0 };
		}
		if (applied.outcome == critical_apply_outcome::applied ||
		    applied.outcome == critical_apply_outcome::already_applied ||
		    applied.outcome == critical_apply_outcome::terminal_failure)
		{
			if (critical_command_journal_checkpoint(command.operation_id) !=
			    critical_command_journal_result::ok)
				applied = { critical_apply_outcome::retryable_failure,
					    applied.durable_revision, applied.error_code };
		}
		critical_completion completion = { .operation_id = command.operation_id,
						   .outcome = applied.outcome,
						   .durable_revision = applied.durable_revision,
						   .error_code = applied.error_code,
						   .attempt = attempt,
						   .queued_at_usec = queued_at,
						   .started_at_usec = started,
						   .completed_at_usec = now_usec() };
		std::unique_lock<std::mutex> lock(coordinator_mutex);
		result_available.wait(lock,
				      [] {
					      return stop_requested ||
						     raw_results.size() <
							     CRITICAL_COORDINATOR_MAX_RESULTS;
				      });
		if (stop_requested)
			return;
		raw_results.push_back(completion);
	}
}
} // namespace

bool critical_command_coordinator_init(const char *journal_directory_path, critical_apply_fn apply,
				       void *context, unsigned int worker_count)
{
	if (!apply || !worker_count || worker_count > CRITICAL_COORDINATOR_DEFAULT_WORKERS * 4)
		return false;
	std::unique_lock<std::mutex> lock(coordinator_mutex);
	if (health.initialized || !critical_command_journal_init(journal_directory_path))
		return false;
	operations.clear();
	pending.clear();
	raw_results.clear();
	active_keys.clear();
	fences.clear();
	completed_cache.clear();
	completed_order.clear();
	completed_cache_bytes = 0;
	health = {};
	health.initialized = true;
	health.accepting = true;
	health.running = true;
	apply_callback = apply;
	apply_context = context;
	stop_requested = false;
	if (critical_command_journal_replay(enqueue_replayed, nullptr) !=
	    critical_command_journal_result::ok)
	{
		health = {};
		critical_command_journal_shutdown();
		return false;
	}
	try
	{
		for (unsigned int index = 0; index < worker_count; ++index)
			workers.emplace_back(worker_main);
	}
	catch (const std::system_error &)
	{
		stop_requested = true;
		work_available.notify_all();
		lock.unlock();
		for (std::thread &worker : workers)
			if (worker.joinable())
				worker.join();
		lock.lock();
		workers.clear();
		health = {};
		critical_command_journal_shutdown();
		return false;
	}
	work_available.notify_all();
	return true;
}

void critical_command_coordinator_shutdown(void)
{
	{
		std::lock_guard<std::mutex> lock(coordinator_mutex);
		stop_requested = true;
		health.accepting = false;
		work_available.notify_all();
		result_available.notify_all();
	}
	for (std::thread &worker : workers)
		if (worker.joinable())
			worker.join();
	std::lock_guard<std::mutex> lock(coordinator_mutex);
	workers.clear();
	operations.clear();
	pending.clear();
	raw_results.clear();
	active_keys.clear();
	fences.clear();
	completed_cache.clear();
	completed_order.clear();
	completed_cache_bytes = 0;
	health = {};
	apply_callback = nullptr;
	apply_context = nullptr;
	critical_command_journal_shutdown();
}

critical_submit_result critical_command_coordinator_submit(critical_command command)
{
	const bool supplied_acceptance_time = command.accepted_at_usec != 0;
	if (!supplied_acceptance_time)
		command.accepted_at_usec = wall_now_usec();
	if (!critical_command_normalize(&command))
		return critical_submit_result::invalid;
	std::lock_guard<std::mutex> lock(coordinator_mutex);
	if (!health.initialized || !health.accepting || stop_requested)
		return critical_submit_result::unavailable;
	const std::string identity = operation_key(command.operation_id);
	auto completed = completed_cache.find(identity);
	if (completed != completed_cache.end())
	{
		if (!supplied_acceptance_time)
			command.accepted_at_usec = completed->second.command.accepted_at_usec;
		if (!critical_command_equal(completed->second.command, command))
			return critical_submit_result::identity_conflict;
		++health.attached;
		return critical_submit_result::attached;
	}
	auto found = operations.find(identity);
	if (found != operations.end())
	{
		if (!supplied_acceptance_time)
			command.accepted_at_usec = found->second->command.accepted_at_usec;
		if (!critical_command_equal(found->second->command, command))
			return critical_submit_result::identity_conflict;
		++found->second->attachments;
		++health.attached;
		return critical_submit_result::attached;
	}
	std::vector<uint8_t> encoded;
	if (critical_command_encode(command, &encoded) != critical_command_codec_result::ok)
		return critical_submit_result::invalid;
	if (operations.size() >= CRITICAL_COORDINATOR_MAX_OPERATIONS ||
	    encoded.size() > CRITICAL_COORDINATOR_MAX_BYTES - health.retained_bytes)
	{
		++health.overloads;
		return critical_submit_result::overloaded;
	}
	try
	{
		auto state = std::make_unique<operation_state>();
		state->command = command;
		state->retained_bytes = encoded.size();
		state->queued_at_usec = now_usec();
		state->attempt = 1;
		state->attachments = 0;
		state->inflight = false;
		state->completed = false;
		state->blocked = false;
		operations.emplace(identity, std::move(state));
		pending.push_back(identity);
		add_fences(identity, operations.at(identity)->command);
	}
	catch (const std::bad_alloc &)
	{
		auto inserted = operations.find(identity);
		if (inserted != operations.end())
		{
			remove_fences(identity, inserted->second->command);
			operations.erase(inserted);
		}
		pending.erase(std::remove(pending.begin(), pending.end(), identity), pending.end());
		++health.overloads;
		return critical_submit_result::overloaded;
	}
	if (critical_command_journal_append(command) != critical_command_journal_result::ok)
	{
		remove_fences(identity, operations.at(identity)->command);
		operations.erase(identity);
		pending.erase(std::remove(pending.begin(), pending.end(), identity), pending.end());
		update_depth();
		return critical_submit_result::journal_failure;
	}
	++health.accepted;
	update_depth();
	work_available.notify_all();
	return critical_submit_result::accepted;
}

size_t critical_command_coordinator_pulse(critical_completion *completions, size_t capacity)
{
	if (capacity && !completions)
		return 0;
	std::lock_guard<std::mutex> lock(coordinator_mutex);
	size_t published = 0;
	while (!raw_results.empty())
	{
		const critical_completion completion = raw_results.front();
		const std::string identity = operation_key(completion.operation_id);
		auto found = operations.find(identity);
		if (found == operations.end() || found->second->completed ||
		    !found->second->inflight || found->second->attempt != completion.attempt)
		{
			raw_results.pop_front();
			result_available.notify_one();
			++health.stale_completions;
			continue;
		}
		const bool retryable =
			completion.outcome == critical_apply_outcome::retryable_failure ||
			completion.outcome == critical_apply_outcome::ambiguous_commit;
		if (!retryable && published >= capacity)
			break;
		raw_results.pop_front();
		result_available.notify_one();
		operation_state &state = *found->second;
		release_keys(identity, state.command);
		state.inflight = false;
		if (retryable)
		{
			if (completion.outcome == critical_apply_outcome::ambiguous_commit)
				++health.ambiguous;
			if (state.attempt <= CRITICAL_COORDINATOR_MAX_RETRIES)
			{
				++state.attempt;
				state.queued_at_usec = now_usec();
				pending.push_back(identity);
				++health.retries;
				work_available.notify_all();
				continue;
			}
			state.blocked = true;
			++health.terminal_failures;
			if (published < capacity)
				completions[published++] = completion;
			continue;
		}
		state.completed = true;
		remove_fences(identity, state.command);
		remember_completed(identity, state.command);
		if (completion.outcome == critical_apply_outcome::terminal_failure)
			++health.terminal_failures;
		++health.completed;
		if (published < capacity)
			completions[published++] = completion;
		operations.erase(found);
	}
	update_depth();
	work_available.notify_all();
	return published;
}

bool critical_command_coordinator_is_fenced(const critical_entity_key &key,
					    critical_operation_id *operation_id)
{
	std::lock_guard<std::mutex> lock(coordinator_mutex);
	auto found = fences.find(entity_key(key));
	if (found == fences.end() || found->second.empty())
		return false;
	if (operation_id)
	{
		auto operation = operations.find(found->second.front());
		if (operation == operations.end())
			return false;
		*operation_id = operation->second->command.operation_id;
	}
	return true;
}

void critical_command_coordinator_quiesce(void)
{
	std::lock_guard<std::mutex> lock(coordinator_mutex);
	health.accepting = false;
}

void critical_command_coordinator_resume(void)
{
	std::lock_guard<std::mutex> lock(coordinator_mutex);
	if (health.initialized && !stop_requested)
		health.accepting = true;
}

bool critical_command_coordinator_drain(uint64_t timeout_msec)
{
	const auto deadline =
		std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_msec);
	critical_completion completions[64] = {};
	for (;;)
	{
		critical_command_coordinator_pulse(completions, 64);
		const critical_coordinator_health snapshot =
			critical_command_coordinator_health_copy();
		if (!snapshot.queued && !snapshot.inflight && !snapshot.blocked)
			return true;
		if (std::chrono::steady_clock::now() >= deadline)
			return false;
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}

critical_coordinator_health critical_command_coordinator_health_copy(void)
{
	std::lock_guard<std::mutex> lock(coordinator_mutex);
	update_depth();
	return health;
}

bool critical_command_coordinator_inject_completion_for_tests(const critical_completion &completion)
{
	std::lock_guard<std::mutex> lock(coordinator_mutex);
	if (raw_results.size() >= CRITICAL_COORDINATOR_MAX_RESULTS)
		return false;
	raw_results.push_back(completion);
	return true;
}

void critical_command_coordinator_reset_for_tests(void)
{
	critical_command_coordinator_shutdown();
	critical_command_journal_reset_for_tests();
}
