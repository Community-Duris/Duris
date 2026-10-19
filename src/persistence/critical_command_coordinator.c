/****************************************************************************
 *
 *  File: critical_command_coordinator.c                        Part of Duris
 *  Usage: coordinates critical commands: fences, queueing, completions
 *
 *  Copyright 1990, 1991 - see LICENSE for complete information
 *
 *  Additions since 2025 by the Duris maintainers and since 2026 by
 *    LuminariMUD maintainers are public domain (Unlicense, see LICENSE)
 *
 *  Created by: Duris, LuminariMUD, Zusuk                  Date: 2026-09-23
 *
 ****************************************************************************/

#include "persistence/critical_command_coordinator.h"
#include "player/player_save_worker.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <deque>
#include <memory>
#include <mutex>
#include <new>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

// Commands run on the one persistence writer, queued at submit, so each lands in
// capture order with the saves around it. The operation table holds a command's entity
// fences until the game thread publishes its completion when requested by the caller.
namespace
{
struct operation_state
{
	critical_command command;
	size_t retained_bytes;
	uint64_t queued_at_usec;
	// The writer retries a command itself, so every completion is the first attempt's.
	unsigned int attempt;
	uint64_t attachments;
	bool retain_until_publication = false;
	bool publication_pending = false;
	critical_completion completion = {};
};

struct completed_state
{
	critical_command command;
	size_t encoded_size;
	critical_completion completion;
};

std::mutex coordinator_mutex;
std::unordered_map<std::string, std::unique_ptr<operation_state>> operations;
critical_completion_delivery completion_delivery;
std::unordered_map<std::string, std::deque<std::string>> fences;
std::unordered_map<std::string, completed_state> completed_cache;
std::deque<std::string> completed_order;
size_t completed_cache_bytes = 0;
critical_apply_fn apply_callback = nullptr;
void *apply_context = nullptr;
critical_drain_observer_fn drain_observer = nullptr;
critical_coordinator_health health = {};
// Each writer job has its own owner, so none replaces another.
uint64_t writer_sequence = 0;
// A job queued before a shutdown still lands, but its completion is not delivered to a
// later coordinator.
uint64_t generation = 0;

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
	uint64_t oldest = 0;
	const uint64_t now = now_usec();
	for (const auto &[identity, state] : operations)
	{
		(void)identity;
		++health.inflight;
		if (state->publication_pending)
			++health.publication_pending;
		health.retained_bytes += state->retained_bytes;
		if (!oldest || state->queued_at_usec < oldest)
			oldest = state->queued_at_usec;
	}
	health.oldest_age_msec = oldest && now > oldest ? (now - oldest) / 1000 : 0;
	health.high_water_operations = std::max(health.high_water_operations, health.inflight);
	health.high_water_bytes = std::max(health.high_water_bytes, health.retained_bytes);
	health.completed_cache = completed_cache.size();
}

void remember_completed(const std::string &identity, const critical_command &command,
			const critical_completion &completion)
{
	std::vector<uint8_t> encoded;
	if (critical_command_encode(command, &encoded) != critical_command_codec_result::ok)
		return;
	const size_t retained_size = encoded.size();
	if (retained_size > CRITICAL_COORDINATOR_COMPLETED_CACHE_BYTES)
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
								   .encoded_size = retained_size,
								   .completion = completion });
		completed_order.push_back(identity);
		completed_cache_bytes += retained_size;
	}
	catch (const std::bad_alloc &)
	{
		completed_cache.erase(identity);
	}
}

struct writer_job
{
	critical_command command;
	critical_apply_fn apply;
	void *context;
	uint64_t queued_at_usec;
	uint64_t generation;
};

// Runs on the persistence writer thread.
player_save_apply_result execute(const writer_job &job)
{
	const uint64_t started = now_usec();
	critical_apply_result applied = {};
	try
	{
		applied = job.apply(job.command, job.context);
	}
	catch (...)
	{
		applied = { critical_apply_outcome::retryable_failure, 0, 0 };
	}
	// A lost connection or a lock wait goes back to the writer, which tries this
	// command again before anything queued after it.
	if (applied.outcome == critical_apply_outcome::retryable_failure ||
	    applied.outcome == critical_apply_outcome::ambiguous_commit)
	{
		std::lock_guard<std::mutex> lock(coordinator_mutex);
		if (health.initialized && job.generation == generation)
		{
			++health.retries;
			if (applied.outcome == critical_apply_outcome::ambiguous_commit)
				++health.ambiguous;
		}
		return { player_save_apply_outcome::retryable_failure, 0, applied.error_code };
	}
	critical_completion completion = { .operation_id = job.command.operation_id,
					   .type = job.command.type,
					   .outcome = applied.outcome,
					   .durable_revision = applied.durable_revision,
					   .error_code = applied.error_code,
					   .attempt = 1,
					   .queued_at_usec = job.queued_at_usec,
					   .started_at_usec = started,
					   .completed_at_usec = now_usec(),
					   .result_size = applied.result_size,
					   .result_payload = applied.result_payload };
	if (applied.outcome == critical_apply_outcome::terminal_failure)
	{
		// Carry owned diagnostic values to the game thread before the command is erased.
		// A bounded record names any omitted keys explicitly; the repository's exact
		// failed comparison is kept first so large commands cannot displace it.
		static const char *source_names[] = { "unknown",	"command", "combat",
						      "zone_event",	"login",   "recovery",
						      "operator_repair" };
		const auto source = static_cast<size_t>(job.command.source_site);
		snprintf(completion.failure_detail.data(), completion.failure_detail.size(),
			 "%s source=%s key_count=%zu keys=", applied.failure_detail.data(),
			 source < std::size(source_names) ? source_names[source] : "unknown",
			 job.command.keys.size());
		static const char *key_names[] = { "unknown",	 "player",    "account",  "item",
						   "guild",	 "locker",    "corpse",	  "auction",
						   "room",	 "system",    "artifact", "zone",
						   "shopkeeper", "collector", "pet" };
		for (size_t index = 0; index < job.command.keys.size(); ++index)
		{
			const size_t used = strlen(completion.failure_detail.data());
			const size_t remaining = completion.failure_detail.size() - used;
			if (remaining < 96)
			{
				snprintf(completion.failure_detail.data() + used, remaining,
					 " omitted_keys=%zu", job.command.keys.size() - index);
				break;
			}
			const auto &key = job.command.keys[index];
			const auto type = static_cast<size_t>(key.type);
			snprintf(completion.failure_detail.data() + used, remaining, "%s%s:%llu",
				 index ? "," : "",
				 type < std::size(key_names) ? key_names[type] : "unknown",
				 static_cast<unsigned long long>(key.id));
		}
	}
	std::lock_guard<std::mutex> lock(coordinator_mutex);
	// There is room for one completion per operation, so none is dropped here.
	if (health.initialized && job.generation == generation)
		(void)completion_delivery.try_enqueue(critical_completion_channel::execution,
						      completion);
	return { player_save_apply_outcome::applied, 0, 0 };
}

// Reserve the operation and its fences. The caller queues it on the writer.
critical_submit_result reserve_locked(const std::string &identity, const critical_command &command,
				      size_t encoded_size, writer_job *job)
{
	try
	{
		auto state = std::make_unique<operation_state>();
		state->command = command;
		state->retained_bytes = encoded_size;
		state->queued_at_usec = now_usec();
		state->attempt = 1;
		state->attachments = 0;
		*job = { command, apply_callback, apply_context, state->queued_at_usec,
			 generation };
		operations.emplace(identity, std::move(state));
		add_fences(identity, command);
	}
	catch (const std::bad_alloc &)
	{
		auto inserted = operations.find(identity);
		if (inserted != operations.end())
		{
			remove_fences(identity, inserted->second->command);
			operations.erase(inserted);
		}
		++health.overloads;
		errno = ENOMEM;
		return critical_submit_result::overloaded;
	}
	update_depth();
	return critical_submit_result::accepted;
}

bool queue_on_writer(const std::string &identity, writer_job job)
{
	const size_t bytes = job.command.payload.size() + sizeof(job);
	uint64_t owner = 0;
	{
		std::lock_guard<std::mutex> lock(coordinator_mutex);
		owner = ++writer_sequence;
	}
	player_save_submit_result submitted = player_save_submit_result::unavailable;
	try
	{
		submitted = persistence_writer_submit(persistence_job_kind::critical, owner, bytes,
						      [job = std::move(job)]()
						      { return execute(job); });
	}
	catch (const std::bad_alloc &)
	{
		// Constructing the owned callback can fail after reservation, before the
		// writer sees it. Release the operation below just as for refused admission.
		errno = ENOMEM;
	}
	const unsigned int error = errno;
	std::lock_guard<std::mutex> lock(coordinator_mutex);
	if (submitted == player_save_submit_result::accepted)
	{
		++health.accepted;
		return true;
	}
	if (error == ENOMEM)
		++health.overloads;
	auto found = operations.find(identity);
	if (found != operations.end())
	{
		remove_fences(identity, found->second->command);
		operations.erase(found);
	}
	update_depth();
	return false;
}

} // namespace

bool critical_command_coordinator_init(critical_apply_fn apply, void *context)
{
	if (!apply)
		return false;
	std::lock_guard<std::mutex> lock(coordinator_mutex);
	if (health.initialized)
		return false;
	operations.clear();
	completion_delivery.clear();
	fences.clear();
	completed_cache.clear();
	completed_order.clear();
	completed_cache_bytes = 0;
	health = {};
	apply_callback = apply;
	apply_context = context;
	++generation;
	health.initialized = true;
	health.accepting = true;
	health.running = true;
	return true;
}

void critical_command_coordinator_shutdown(void)
{
	std::lock_guard<std::mutex> lock(coordinator_mutex);
	operations.clear();
	completion_delivery.clear();
	fences.clear();
	completed_cache.clear();
	completed_order.clear();
	completed_cache_bytes = 0;
	health = {};
	apply_callback = nullptr;
	apply_context = nullptr;
	++generation;
}

static critical_submit_result submit_command(critical_command command,
					     bool retain_until_publication)
{
	const bool supplied_acceptance_time = command.accepted_at_usec != 0;
	if (!supplied_acceptance_time)
		command.accepted_at_usec = wall_now_usec();
	if (!critical_command_normalize(&command))
		return critical_submit_result::invalid;
	const std::string identity = operation_key(command.operation_id);
	writer_job job = {};
	{
		std::lock_guard<std::mutex> lock(coordinator_mutex);
		if (!health.initialized || !health.accepting)
		{
			errno = ESHUTDOWN;
			return critical_submit_result::unavailable;
		}
		auto completed = completed_cache.find(identity);
		if (completed != completed_cache.end())
		{
			if (!supplied_acceptance_time)
				command.accepted_at_usec =
					completed->second.command.accepted_at_usec;
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
			errno = ENOSPC;
			return critical_submit_result::overloaded;
		}
		const critical_submit_result reserved =
			reserve_locked(identity, command, encoded.size(), &job);
		if (reserved != critical_submit_result::accepted)
			return reserved;
		operations.at(identity)->retain_until_publication = retain_until_publication;
	}
	return queue_on_writer(identity, std::move(job)) ? critical_submit_result::accepted :
							   critical_submit_result::unavailable;
}

critical_submit_result critical_command_coordinator_submit(critical_command command)
{
	return submit_command(std::move(command), false);
}

critical_submit_result critical_command_coordinator_submit_for_publication(critical_command command)
{
	return submit_command(std::move(command), true);
}

size_t critical_command_coordinator_pulse(critical_completion *completions, size_t capacity)
{
	if (capacity && !completions)
		return 0;
	std::lock_guard<std::mutex> lock(coordinator_mutex);
	size_t published = 0;
	while (published < capacity &&
	       completion_delivery.size(critical_completion_channel::execution))
	{
		const critical_completion *front =
			completion_delivery.front(critical_completion_channel::execution);
		if (!front)
			break;
		const critical_completion completion = *front;
		std::string identity;
		try
		{
			identity = operation_key(completion.operation_id);
		}
		catch (const std::bad_alloc &)
		{
			// The queue still owns this receipt; a later pulse can deliver it.
			break;
		}
		completion_delivery.pop_front(critical_completion_channel::execution);
		auto found = operations.find(identity);
		if (found == operations.end() || found->second->attempt != completion.attempt ||
		    found->second->publication_pending)
		{
			++health.stale_completions;
			continue;
		}
		operation_state &state = *found->second;
		if (completion.outcome == critical_apply_outcome::terminal_failure)
			++health.terminal_failures;
		completions[published++] = completion;
		if (state.retain_until_publication)
		{
			state.completion = completion;
			state.publication_pending = true;
			continue;
		}
		remove_fences(identity, state.command);
		remember_completed(identity, state.command, completion);
		++health.completed;
		operations.erase(found);
	}
	update_depth();
	return published;
}

bool critical_command_coordinator_acknowledge_publication(const critical_operation_id &operation_id)
{
	std::lock_guard<std::mutex> lock(coordinator_mutex);
	std::string identity;
	try
	{
		identity = operation_key(operation_id);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	auto found = operations.find(identity);
	if (found == operations.end())
		return completed_cache.count(identity) != 0;
	const operation_state &state = *found->second;
	if (!state.publication_pending)
		return false;
	remember_completed(identity, state.command, state.completion);
	remove_fences(identity, state.command);
	operations.erase(found);
	++health.completed;
	update_depth();
	return true;
}

bool critical_command_coordinator_get_completed(const critical_operation_id &operation_id,
						critical_completion *completion)
{
	if (!completion)
		return false;
	std::lock_guard<std::mutex> lock(coordinator_mutex);
	const auto identity = operation_key(operation_id);
	const auto pending = operations.find(identity);
	if (pending != operations.end() && pending->second->publication_pending)
	{
		*completion = pending->second->completion;
		return true;
	}
	const auto completed = completed_cache.find(identity);
	if (completed == completed_cache.end())
		return false;
	*completion = completed->second.completion;
	return true;
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
	if (health.initialized)
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
		if (!snapshot.inflight)
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
}
