#include "persistence/critical_command_coordinator.h"

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <cerrno>
#include <memory>
#include <mutex>
#include <new>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace
{
enum class critical_operation_phase : uint8_t
{
	awaiting_durability,
	queued,
	executing,
	uncertain_admission,
	blocked,
	admission_failed,
	publication_pending,
};

struct operation_state
{
	critical_command command;
	size_t retained_bytes;
	uint64_t queued_at_usec;
	unsigned int attempt;
	uint64_t attachments;
	critical_operation_phase phase;
	bool retain_until_publication;
	bool admission_failure_queued;
	critical_completion admission_failure_completion;
	critical_completion publication_completion;
};

struct completed_state
{
	critical_command command;
	critical_completion completion;
	size_t encoded_size;
};

struct replay_observer_context
{
	critical_replay_observer_fn observer;
	void *context;
};

std::mutex coordinator_mutex;
std::condition_variable work_available;
std::condition_variable result_available;
std::condition_variable admission_available;
std::unordered_map<std::string, std::unique_ptr<operation_state>> operations;
std::deque<std::string> pending;
std::deque<std::string> pending_admission;
critical_completion_delivery completion_delivery;
std::unordered_map<std::string, std::string> active_keys;
std::unordered_map<std::string, std::deque<std::string>> fences;
std::unordered_map<std::string, completed_state> completed_cache;
std::deque<std::string> completed_order;
size_t completed_cache_bytes = 0;
size_t pending_admission_bytes = 0;
size_t admission_inflight_bytes = 0;
std::vector<std::thread> workers;
std::thread admission_worker;
critical_apply_fn apply_callback = nullptr;
void *apply_context = nullptr;
critical_drain_observer_fn drain_observer = nullptr;
critical_coordinator_health health = {};
bool stop_requested = false;
bool recovery_requested = false;
uint64_t uncertain_recovery_not_before_usec = 0;
uint64_t uncertain_recovery_delay_usec = 1000000;

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

void defer_uncertain_recovery()
{
	recovery_requested = true;
	const uint64_t now = now_usec();
	uncertain_recovery_not_before_usec = now + uncertain_recovery_delay_usec;
	uncertain_recovery_delay_usec =
		std::min<uint64_t>(uncertain_recovery_delay_usec * 2, 30000000);
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

bool operation_is_queued(const operation_state &state)
{
	return state.phase == critical_operation_phase::queued;
}

bool operation_is_executing(const operation_state &state)
{
	return state.phase == critical_operation_phase::executing;
}

bool operation_is_publication_pending(const operation_state &state)
{
	return state.phase == critical_operation_phase::publication_pending;
}

bool operation_is_uncertain(const operation_state &state)
{
	return state.phase == critical_operation_phase::uncertain_admission;
}

bool operation_is_awaiting_durability(const operation_state &state)
{
	return state.phase == critical_operation_phase::awaiting_durability;
}

bool operation_is_blocked(const operation_state &state)
{
	return state.phase == critical_operation_phase::uncertain_admission ||
	       state.phase == critical_operation_phase::blocked ||
	       state.phase == critical_operation_phase::admission_failed;
}

bool operation_is_admission_failed(const operation_state &state)
{
	return state.phase == critical_operation_phase::admission_failed;
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
	health.publication_pending = 0;
	health.retained_bytes = 0;
	health.awaiting_durability = 0;
	uint64_t oldest = 0;
	const uint64_t now = now_usec();
	for (const auto &[identity, state] : operations)
	{
		(void)identity;
		if (operation_is_publication_pending(*state))
			++health.publication_pending;
		else if (operation_is_blocked(*state))
			++health.blocked;
		else if (operation_is_executing(*state))
			++health.inflight;
		else
			++health.queued;
		if (operation_is_awaiting_durability(*state))
			++health.awaiting_durability;
		health.retained_bytes += state->retained_bytes;
		if (!oldest || state->queued_at_usec < oldest)
			oldest = state->queued_at_usec;
	}
	health.oldest_age_msec = oldest && now > oldest ? (now - oldest) / 1000 : 0;
	health.high_water_operations = std::max(health.high_water_operations,
						health.queued + health.inflight + health.blocked +
							health.publication_pending);
	health.high_water_bytes = std::max(health.high_water_bytes, health.retained_bytes);
	health.completed_cache = completed_cache.size();
	health.admission_queue_bytes = pending_admission_bytes + admission_inflight_bytes;
	health.admission_worker_running = admission_worker.joinable() && !stop_requested;
	health.append_inflight = admission_inflight_bytes != 0;
}

void remember_completed(const std::string &identity, const critical_command &command,
			const critical_completion &completion)
{
	std::vector<uint8_t> encoded;
	if (critical_command_encode(command, &encoded) != critical_command_codec_result::ok)
		return;
	const size_t retained_size = encoded.size() + sizeof(critical_completion);
	if (encoded.size() > CRITICAL_COORDINATOR_COMPLETED_CACHE_BYTES ||
	    retained_size < encoded.size() ||
	    retained_size > CRITICAL_COORDINATOR_COMPLETED_CACHE_BYTES)
		return;
	while (!completed_order.empty() &&
	       (completed_cache.size() >= CRITICAL_COORDINATOR_COMPLETED_CACHE_MAX ||
		completed_cache_bytes > CRITICAL_COORDINATOR_COMPLETED_CACHE_BYTES - retained_size))
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
		completed_cache.emplace(identity, completed_state{ .command = command,
								   .completion = completion,
								   .encoded_size = retained_size });
		completed_order.push_back(identity);
		completed_cache_bytes += retained_size;
	}
	catch (const std::bad_alloc &)
	{
		completed_cache.erase(identity);
	}
}

bool completion_is_retryable(const critical_completion &completion)
{
	return completion.outcome == critical_apply_outcome::retryable_failure ||
	       completion.outcome == critical_apply_outcome::ambiguous_commit;
}

bool schedule_retry_locked(const std::string &identity, operation_state &state,
			   const critical_completion &completion)
{
	if (!completion_is_retryable(completion) ||
	    state.attempt > CRITICAL_COORDINATOR_MAX_RETRIES)
		return false;
	release_keys(identity, state.command);
	state.phase = critical_operation_phase::queued;
	if (completion.outcome == critical_apply_outcome::ambiguous_commit)
		++health.ambiguous;
	++state.attempt;
	state.queued_at_usec = now_usec();
	pending.push_back(identity);
	++health.retries;
	work_available.notify_all();
	return true;
}

void retain_exhausted_retry_locked(const std::string &identity, operation_state &state,
				   const critical_completion &completion)
{
	release_keys(identity, state.command);
	state.phase = critical_operation_phase::blocked;
	if (completion.outcome == critical_apply_outcome::ambiguous_commit)
		++health.ambiguous;
	++health.terminal_failures;
}

bool enqueue_replayed(critical_command command, void *context)
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
		state->phase = critical_operation_phase::queued;
		state->admission_failure_queued = false;
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

	const replay_observer_context *replay =
		static_cast<const replay_observer_context *>(context);
	if (replay && replay->observer)
	{
		bool observed = false;
		try
		{
			observed =
				replay->observer(operations.at(identity)->command, replay->context);
		}
		catch (...)
		{
			observed = false;
		}
		if (!observed)
		{
			auto inserted = operations.find(identity);
			if (inserted != operations.end())
			{
				remove_fences(identity, inserted->second->command);
				operations.erase(inserted);
			}
			pending.erase(std::remove(pending.begin(), pending.end(), identity),
				      pending.end());
			update_depth();
			return false;
		}
	}
	update_depth();
	return true;
}

bool collect_replayed_identity(critical_command command, void *context)
{
	if (!context)
		return false;
	try
	{
		static_cast<std::unordered_set<std::string> *>(context)->insert(
			operation_key(command.operation_id));
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	return true;
}

unsigned int journal_failure_error(critical_command_journal_result result)
{
	switch (result)
	{
	case critical_command_journal_result::quota_exceeded:
		return ENOSPC;
	case critical_command_journal_result::unsafe_permissions:
		return EACCES;
	case critical_command_journal_result::corrupt_data:
		return EBADMSG;
	case critical_command_journal_result::not_initialized:
		return EPIPE;
	case critical_command_journal_result::invalid:
		return EINVAL;
	case critical_command_journal_result::io_failure:
	case critical_command_journal_result::replay_blocked:
	case critical_command_journal_result::append_uncertain:
	case critical_command_journal_result::ok:
		return EIO;
	}
	return EIO;
}

void retain_admission_failure_locked(operation_state &state, unsigned int error_code)
{
	state.phase = critical_operation_phase::admission_failed;
	state.admission_failure_completion = { .operation_id = state.command.operation_id,
					       .outcome = critical_apply_outcome::terminal_failure,
					       .durable_revision = 0,
					       .error_code = error_code,
					       .attempt = state.attempt,
					       .queued_at_usec = state.queued_at_usec,
					       .started_at_usec = 0,
					       .completed_at_usec = now_usec(),
					       .result_size = 0,
					       .result_payload = {} };
	if (!state.admission_failure_queued)
	{
		state.admission_failure_queued = completion_delivery.try_enqueue(
			critical_completion_channel::admission_failure,
			state.admission_failure_completion);
		// The completion remains in the operation state when the delivery buffer
		// is full or allocation is temporarily unavailable. pulse() retries it
		// before considering the operation for retirement.
	}
	++health.admission_failures;
}

bool recovery_due_locked()
{
	return recovery_requested && (!uncertain_recovery_not_before_usec ||
				      now_usec() >= uncertain_recovery_not_before_usec);
}

bool recover_uncertain_on_worker()
{
	std::vector<std::pair<std::string, critical_command>> candidates;
	try
	{
		std::lock_guard<std::mutex> lock(coordinator_mutex);
		if (!health.initialized || stop_requested)
			return false;
		for (const auto &[identity, state] : operations)
			if (operation_is_uncertain(*state))
				candidates.emplace_back(identity, state->command);
	}
	catch (const std::bad_alloc &)
	{
		std::lock_guard<std::mutex> lock(coordinator_mutex);
		defer_uncertain_recovery();
		return false;
	}

	if (candidates.empty())
	{
		std::lock_guard<std::mutex> lock(coordinator_mutex);
		recovery_requested = false;
		uncertain_recovery_not_before_usec = 0;
		uncertain_recovery_delay_usec = 1000000;
		update_depth();
		return true;
	}

	std::unordered_set<std::string> journal_identities;
	if (critical_command_journal_replay(collect_replayed_identity, &journal_identities) !=
	    critical_command_journal_result::ok)
	{
		std::lock_guard<std::mutex> lock(coordinator_mutex);
		defer_uncertain_recovery();
		return false;
	}
	if (critical_command_journal_sync() != critical_command_journal_result::ok)
	{
		std::lock_guard<std::mutex> lock(coordinator_mutex);
		defer_uncertain_recovery();
		return false;
	}

	bool recovered = true;
	bool woke_worker = false;
	for (const auto &[identity, command] : candidates)
	{
		const bool journaled = journal_identities.find(identity) !=
				       journal_identities.end();
		const critical_command_journal_result result =
			journaled ? critical_command_journal_result::ok :
				    critical_command_journal_append(command);
		std::lock_guard<std::mutex> lock(coordinator_mutex);
		auto found = operations.find(identity);
		if (found == operations.end() || !operation_is_uncertain(*found->second))
			continue;
		if (result == critical_command_journal_result::ok)
		{
			try
			{
				if (std::find(pending.begin(), pending.end(), identity) ==
				    pending.end())
					pending.push_back(identity);
			}
			catch (const std::bad_alloc &)
			{
				recovered = false;
				continue;
			}
			found->second->phase = critical_operation_phase::queued;
			woke_worker = true;
		}
		else if (result == critical_command_journal_result::append_uncertain)
		{
			recovered = false;
		}
		else
		{
			retain_admission_failure_locked(*found->second,
							journal_failure_error(result));
			recovered = false;
		}
	}

	{
		std::lock_guard<std::mutex> lock(coordinator_mutex);
		bool uncertain = false;
		for (const auto &[identity, state] : operations)
		{
			(void)identity;
			uncertain = uncertain || operation_is_uncertain(*state);
		}
		if (!uncertain)
		{
			recovery_requested = false;
			uncertain_recovery_not_before_usec = 0;
			uncertain_recovery_delay_usec = 1000000;
		}
		else
			defer_uncertain_recovery();
		update_depth();
	}
	if (woke_worker)
		work_available.notify_all();
	return recovered;
}

void finish_admission(const std::string &identity, critical_command_journal_result result)
{
	bool wake_worker = false;
	{
		std::lock_guard<std::mutex> lock(coordinator_mutex);
		auto found = operations.find(identity);
		if (found == operations.end())
			return;
		operation_state &state = *found->second;
		admission_inflight_bytes = 0;
		if (result == critical_command_journal_result::ok)
		{
			try
			{
				pending.push_back(identity);
				state.phase = critical_operation_phase::queued;
				++health.durable_admissions;
				wake_worker = true;
			}
			catch (const std::bad_alloc &)
			{
				// The journal is durable. Keep the identity fenced and ask the
				// recovery worker to retry the in-memory ready handoff.
				state.phase = critical_operation_phase::uncertain_admission;
				++health.admission_uncertain;
				defer_uncertain_recovery();
			}
		}
		else if (result == critical_command_journal_result::append_uncertain)
		{
			state.phase = critical_operation_phase::uncertain_admission;
			++health.ambiguous;
			++health.admission_uncertain;
			defer_uncertain_recovery();
		}
		else
		{
			retain_admission_failure_locked(state, journal_failure_error(result));
			++health.terminal_failures;
		}
		update_depth();
	}
	if (wake_worker)
		work_available.notify_all();
}

void admission_worker_main()
{
	{
		std::lock_guard<std::mutex> lock(coordinator_mutex);
		health.admission_worker_running = true;
		update_depth();
	}
	for (;;)
	{
		std::string identity;
		critical_command command;
		bool recover = false;
		{
			std::unique_lock<std::mutex> lock(coordinator_mutex);
			for (;;)
			{
				if (stop_requested || !pending_admission.empty())
					break;
				if (recovery_due_locked())
				{
					recover = true;
					break;
				}
				if (!recovery_requested)
				{
					admission_available.wait(
						lock,
						[]
						{
							return stop_requested ||
							       !pending_admission.empty() ||
							       recovery_requested;
						});
					continue;
				}
				const uint64_t now = now_usec();
				const uint64_t wait_usec =
					uncertain_recovery_not_before_usec > now ?
						uncertain_recovery_not_before_usec - now :
						0;
				admission_available.wait_for(lock,
							     std::chrono::microseconds(wait_usec));
			}
			if (stop_requested && pending_admission.empty())
				break;
			if (!pending_admission.empty())
			{
				identity = pending_admission.front();
				pending_admission.pop_front();
				auto found = operations.find(identity);
				if (found == operations.end() ||
				    !operation_is_awaiting_durability(*found->second))
				{
					update_depth();
					continue;
				}
				pending_admission_bytes -= found->second->retained_bytes;
				admission_inflight_bytes = found->second->retained_bytes;
				try
				{
					command = found->second->command;
				}
				catch (const std::bad_alloc &)
				{
					admission_inflight_bytes = 0;
					retain_admission_failure_locked(*found->second, ENOMEM);
					++health.terminal_failures;
					update_depth();
					continue;
				}
				update_depth();
			}
			else if (!recover)
			{
				continue;
			}
		}
		if (recover)
		{
			(void)recover_uncertain_on_worker();
			continue;
		}
		finish_admission(identity, critical_command_journal_append(command));
	}
	{
		std::lock_guard<std::mutex> lock(coordinator_mutex);
		health.admission_worker_running = false;
		admission_inflight_bytes = 0;
		update_depth();
	}
}

void worker_main()
{
	for (;;)
	{
		std::string identity;
		critical_command command;
		unsigned int attempt = 0;
		uint64_t queued_at = 0;
		bool retain_publication = false;
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
						    operation_is_queued(*found->second) &&
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
				if (found != operations.end() &&
				    operation_is_queued(*found->second) &&
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
			state.phase = critical_operation_phase::executing;
			acquire_keys(identity, state.command);
			try
			{
				command = state.command;
			}
			catch (const std::bad_alloc &)
			{
				release_keys(identity, state.command);
				state.phase = critical_operation_phase::blocked;
				++health.terminal_failures;
				update_depth();
				continue;
			}
			attempt = state.attempt;
			queued_at = state.queued_at_usec;
			retain_publication = state.retain_until_publication;
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
		if (!retain_publication &&
		    (applied.outcome == critical_apply_outcome::applied ||
		     applied.outcome == critical_apply_outcome::already_applied ||
		     applied.outcome == critical_apply_outcome::terminal_failure))
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
						   .completed_at_usec = now_usec(),
						   .failure_stage = applied.failure_stage,
						   .result_size = applied.result_size,
						   .result_payload = applied.result_payload };
		std::unique_lock<std::mutex> lock(coordinator_mutex);
		result_available.wait(
			lock, [] { return stop_requested || completion_delivery.has_capacity(); });
		if (stop_requested)
			return;
		completion_delivery.enqueue(critical_completion_channel::execution, completion);
	}
}
} // namespace

bool critical_command_coordinator_init(const char *journal_directory_path, critical_apply_fn apply,
				       void *context, unsigned int worker_count,
				       critical_replay_observer_fn replay_observer,
				       void *replay_context)
{
	if (!apply || !worker_count || worker_count > CRITICAL_COORDINATOR_DEFAULT_WORKERS * 4)
		return false;
	std::unique_lock<std::mutex> lock(coordinator_mutex);
	if (health.initialized || !critical_command_journal_init(journal_directory_path))
		return false;
	operations.clear();
	pending.clear();
	pending_admission.clear();
	completion_delivery.clear();
	active_keys.clear();
	fences.clear();
	completed_cache.clear();
	completed_order.clear();
	completed_cache_bytes = 0;
	pending_admission_bytes = 0;
	admission_inflight_bytes = 0;
	health = {};
	health.initialized = true;
	health.accepting = true;
	health.running = true;
	apply_callback = apply;
	apply_context = context;
	stop_requested = false;
	recovery_requested = false;
	uncertain_recovery_not_before_usec = 0;
	uncertain_recovery_delay_usec = 1000000;
	replay_observer_context replay = { replay_observer, replay_context };
	if (critical_command_journal_replay(enqueue_replayed,
					    replay_observer ? &replay : nullptr) !=
	    critical_command_journal_result::ok)
	{
		health = {};
		critical_command_journal_shutdown();
		return false;
	}
	try
	{
		admission_worker = std::thread(admission_worker_main);
		for (unsigned int index = 0; index < worker_count; ++index)
			workers.emplace_back(worker_main);
	}
	catch (const std::system_error &)
	{
		stop_requested = true;
		work_available.notify_all();
		admission_available.notify_all();
		lock.unlock();
		if (admission_worker.joinable())
			admission_worker.join();
		for (std::thread &worker : workers)
			if (worker.joinable())
				worker.join();
		lock.lock();
		workers.clear();
		admission_worker = {};
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
		admission_available.notify_all();
	}
	if (admission_worker.joinable())
		admission_worker.join();
	for (std::thread &worker : workers)
		if (worker.joinable())
			worker.join();
	std::lock_guard<std::mutex> lock(coordinator_mutex);
	workers.clear();
	operations.clear();
	pending.clear();
	pending_admission.clear();
	completion_delivery.clear();
	active_keys.clear();
	fences.clear();
	completed_cache.clear();
	completed_order.clear();
	completed_cache_bytes = 0;
	pending_admission_bytes = 0;
	admission_inflight_bytes = 0;
	health = {};
	apply_callback = nullptr;
	apply_context = nullptr;
	recovery_requested = false;
	uncertain_recovery_not_before_usec = 0;
	uncertain_recovery_delay_usec = 1000000;
	critical_command_journal_shutdown();
}

critical_submit_result critical_command_coordinator_submit_internal(critical_command command,
								    bool retain_until_publication)
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
		if (retain_until_publication && !found->second->retain_until_publication)
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
	bool admission_queued = false;
	try
	{
		auto state = std::make_unique<operation_state>();
		state->command = command;
		state->retained_bytes = encoded.size();
		state->queued_at_usec = now_usec();
		state->attempt = 1;
		state->attachments = 0;
		state->phase = critical_operation_phase::awaiting_durability;
		state->retain_until_publication = retain_until_publication;
		state->admission_failure_queued = false;
		operations.emplace(identity, std::move(state));
		pending_admission.push_back(identity);
		pending_admission_bytes += encoded.size();
		admission_queued = true;
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
		pending_admission.erase(std::remove(pending_admission.begin(),
						    pending_admission.end(), identity),
					pending_admission.end());
		if (admission_queued)
			pending_admission_bytes -= encoded.size();
		++health.overloads;
		return critical_submit_result::overloaded;
	}
	++health.accepted;
	update_depth();
	// The operation and its per-key fence are now retained, but the journal
	// worker must acknowledge fsync before it is moved to the execution queue.
	admission_available.notify_one();
	return critical_submit_result::awaiting_durability;
}

critical_submit_result critical_command_coordinator_submit(critical_command command)
{
	return critical_command_coordinator_submit_internal(std::move(command), false);
}

critical_submit_result critical_command_coordinator_submit_for_publication(critical_command command)
{
	return critical_command_coordinator_submit_internal(std::move(command), true);
}

critical_command_durability
critical_command_coordinator_durability(const critical_operation_id &operation_id)
{
	if (critical_operation_id_is_zero(operation_id))
		return critical_command_durability::unknown;
	std::lock_guard<std::mutex> lock(coordinator_mutex);
	const std::string identity = operation_key(operation_id);
	if (completed_cache.find(identity) != completed_cache.end())
		return critical_command_durability::durable;
	auto found = operations.find(identity);
	if (found == operations.end())
		return critical_command_durability::unknown;
	if (operation_is_admission_failed(*found->second))
		return critical_command_durability::failed;
	if (operation_is_uncertain(*found->second))
		return critical_command_durability::uncertain;
	if (operation_is_awaiting_durability(*found->second))
		return critical_command_durability::awaiting_durability;
	return critical_command_durability::durable;
}

bool critical_command_coordinator_recover_uncertain(void)
{
	std::lock_guard<std::mutex> lock(coordinator_mutex);
	if (!health.initialized || stop_requested)
		return false;
	bool uncertain = false;
	for (const auto &[identity, state] : operations)
	{
		(void)identity;
		uncertain = uncertain || operation_is_uncertain(*state);
	}
	if (!uncertain)
	{
		recovery_requested = false;
		uncertain_recovery_not_before_usec = 0;
		uncertain_recovery_delay_usec = 1000000;
		return true;
	}
	recovery_requested = true;
	uncertain_recovery_not_before_usec = 0;
	admission_available.notify_one();
	return true;
}

bool recovery_due()
{
	std::lock_guard<std::mutex> lock(coordinator_mutex);
	if (!health.initialized || stop_requested)
		return false;
	bool uncertain = false;
	for (const auto &[identity, state] : operations)
	{
		(void)identity;
		uncertain = uncertain || operation_is_uncertain(*state);
	}
	if (!uncertain || !recovery_due_locked())
		return false;
	recovery_requested = true;
	return true;
}

bool critical_command_coordinator_get_completed(const critical_operation_id &operation_id,
						critical_completion *completion)
{
	if (!completion || critical_operation_id_is_zero(operation_id))
		return false;
	std::lock_guard<std::mutex> lock(coordinator_mutex);
	const std::string identity = operation_key(operation_id);
	auto operation = operations.find(identity);
	if (operation != operations.end() && operation_is_publication_pending(*operation->second))
	{
		*completion = operation->second->publication_completion;
		return true;
	}
	const auto found = completed_cache.find(identity);
	if (found == completed_cache.end())
		return false;
	*completion = found->second.completion;
	return true;
}

bool critical_command_coordinator_acknowledge_publication(const critical_operation_id &operation_id)
{
	if (critical_operation_id_is_zero(operation_id))
		return false;
	std::lock_guard<std::mutex> lock(coordinator_mutex);
	const std::string identity = operation_key(operation_id);
	auto found = operations.find(identity);
	if (found == operations.end() || !operation_is_publication_pending(*found->second))
		return false;
	if (critical_command_journal_checkpoint(operation_id) !=
	    critical_command_journal_result::ok)
		return false;
	operation_state &state = *found->second;
	remove_fences(identity, state.command);
	remember_completed(identity, state.command, state.publication_completion);
	operations.erase(found);
	++health.completed;
	update_depth();
	work_available.notify_all();
	return true;
}

void queue_unqueued_admission_failures_locked()
{
	for (const auto &[identity, state] : operations)
	{
		(void)identity;
		if (!operation_is_admission_failed(*state) || state->admission_failure_queued)
			continue;
		if (!completion_delivery.try_enqueue(critical_completion_channel::admission_failure,
						     state->admission_failure_completion))
			return;
		state->admission_failure_queued = true;
	}
}

size_t critical_command_coordinator_pulse(critical_completion *completions, size_t capacity)
{
	if (capacity && !completions)
		return 0;
	if (recovery_due())
		admission_available.notify_one();
	std::lock_guard<std::mutex> lock(coordinator_mutex);
	queue_unqueued_admission_failures_locked();
	size_t published = 0;
	while (completion_delivery.size(critical_completion_channel::execution))
	{
		const critical_completion *front =
			completion_delivery.front(critical_completion_channel::execution);
		if (!front)
			break;
		const critical_completion completion = *front;
		const std::string identity = operation_key(completion.operation_id);
		auto found = operations.find(identity);
		if (found == operations.end() || !operation_is_executing(*found->second) ||
		    found->second->attempt != completion.attempt)
		{
			completion_delivery.pop_front(critical_completion_channel::execution);
			result_available.notify_one();
			++health.stale_completions;
			continue;
		}
		const bool retryable = completion_is_retryable(completion);
		// Exhausted retries need a final notification just like other outcomes.
		// Retain the result until it can be published, before changing its state.
		const bool will_retry = retryable &&
					found->second->attempt <= CRITICAL_COORDINATOR_MAX_RETRIES;
		if (!will_retry && published >= capacity)
			break;
		completion_delivery.pop_front(critical_completion_channel::execution);
		result_available.notify_one();
		operation_state &state = *found->second;
		if (state.retain_until_publication && !retryable)
		{
			release_keys(identity, state.command);
			state.phase = critical_operation_phase::publication_pending;
			state.publication_completion = completion;
			if (completion.outcome == critical_apply_outcome::terminal_failure)
				++health.terminal_failures;
			if (published < capacity)
				completions[published++] = completion;
			continue;
		}
		if (retryable)
		{
			if (will_retry && schedule_retry_locked(identity, state, completion))
				continue;
			retain_exhausted_retry_locked(identity, state, completion);
			if (published < capacity)
				completions[published++] = completion;
			continue;
		}
		release_keys(identity, state.command);
		remove_fences(identity, state.command);
		remember_completed(identity, state.command, completion);
		if (completion.outcome == critical_apply_outcome::terminal_failure)
			++health.terminal_failures;
		++health.completed;
		if (published < capacity)
			completions[published++] = completion;
		operations.erase(found);
	}
	while (completion_delivery.size(critical_completion_channel::admission_failure))
	{
		const critical_completion *front =
			completion_delivery.front(critical_completion_channel::admission_failure);
		if (!front)
			break;
		const critical_completion completion = *front;
		const std::string identity = operation_key(completion.operation_id);
		auto found = operations.find(identity);
		if (found == operations.end() || !operation_is_admission_failed(*found->second) ||
		    found->second->attempt != completion.attempt)
		{
			completion_delivery.pop_front(
				critical_completion_channel::admission_failure);
			++health.stale_completions;
			continue;
		}
		if (published >= capacity)
			break;
		completion_delivery.pop_front(critical_completion_channel::admission_failure);
		operation_state &state = *found->second;
		state.admission_failure_queued = false;
		release_keys(identity, state.command);
		remove_fences(identity, state.command);
		completions[published++] = completion;
		operations.erase(found);
	}
	if (published < capacity)
	{
		for (auto found = operations.begin(); found != operations.end(); ++found)
		{
			if (!operation_is_admission_failed(*found->second) ||
			    found->second->admission_failure_queued)
				continue;
			const std::string identity = found->first;
			const critical_completion completion =
				found->second->admission_failure_completion;
			release_keys(identity, found->second->command);
			remove_fences(identity, found->second->command);
			completions[published++] = completion;
			operations.erase(found);
			break;
		}
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
		const size_t completed = critical_command_coordinator_pulse(completions, 64);
		critical_drain_observer_fn observer = nullptr;
		{
			std::lock_guard<std::mutex> lock(coordinator_mutex);
			observer = drain_observer;
		}
		if (completed && observer)
			observer(completions, completed);
		const critical_coordinator_health snapshot =
			critical_command_coordinator_health_copy();
		if (!snapshot.queued && !snapshot.inflight && !snapshot.blocked &&
		    !snapshot.publication_pending && !snapshot.awaiting_durability &&
		    !snapshot.admission_queue_bytes && !snapshot.append_inflight)
			return true;
		if (std::chrono::steady_clock::now() >= deadline)
			return false;
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}

void critical_command_coordinator_set_drain_observer(critical_drain_observer_fn observer)
{
	std::lock_guard<std::mutex> lock(coordinator_mutex);
	drain_observer = observer;
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
	return completion_delivery.try_enqueue(critical_completion_channel::execution, completion);
}

void critical_command_coordinator_reset_for_tests(void)
{
	critical_command_coordinator_shutdown();
	critical_command_coordinator_set_drain_observer(nullptr);
	critical_command_journal_reset_for_tests();
}
