#include "redis_command_observability.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>

namespace
{
struct atomic_scope_health
{
	std::atomic<uint64_t> calls = 0;
	std::atomic<uint64_t> successes = 0;
	std::atomic<uint64_t> failures = 0;
	std::atomic<uint64_t> unavailable = 0;
	std::atomic<uint64_t> timeouts = 0;
	std::atomic<uint64_t> transport_failures = 0;
	std::atomic<uint64_t> error_replies = 0;
	std::atomic<uint64_t> no_replies = 0;
	std::atomic<uint64_t> consecutive_failures = 0;
	std::atomic<uint64_t> total_latency_usec = 0;
	std::atomic<uint64_t> last_latency_usec = 0;
	std::atomic<uint64_t> max_latency_usec = 0;
	std::atomic<uint64_t> last_success_msec = 0;
};

std::array<atomic_scope_health, REDIS_SHARED_SCOPE_COUNT> scopes;
std::array<std::atomic<uint64_t>, REDIS_SHARED_COMMAND_KIND_COUNT> command_kind_calls;
std::atomic<uint64_t> connection_attempts = 0;
std::atomic<uint64_t> connection_failures = 0;
std::atomic<uint64_t> reconnect_attempts = 0;
std::atomic<uint64_t> reconnect_successes = 0;
std::atomic<uint64_t> reconnect_failures = 0;
std::atomic<uint64_t> recovery_transitions = 0;
std::atomic<bool> observability_enabled = false;
std::atomic<bool> connection_available = false;

uint64_t monotonic_msec()
{
	return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
					     std::chrono::steady_clock::now().time_since_epoch())
					     .count());
}

void update_max(std::atomic<uint64_t> *maximum, uint64_t value)
{
	uint64_t current = maximum->load(std::memory_order_relaxed);
	while (current < value &&
	       !maximum->compare_exchange_weak(current, value, std::memory_order_relaxed))
	{
	}
}
} // namespace

const char *redis_shared_command_scope_name(redis_shared_command_scope scope)
{
	switch (scope)
	{
	case REDIS_SHARED_SCOPE_WORLD:
		return "world";
	case REDIS_SHARED_SCOPE_FLOOR:
		return "floor";
	case REDIS_SHARED_SCOPE_CACHE:
		return "cache_boot";
	case REDIS_SHARED_SCOPE_MAINTENANCE:
		return "maintenance";
	case REDIS_SHARED_SCOPE_COUNT:
		break;
	}
	return "unknown";
}

void redis_shared_command_observability_reset(bool enabled)
{
	for (atomic_scope_health &scope : scopes)
	{
		scope.calls.store(0, std::memory_order_relaxed);
		scope.successes.store(0, std::memory_order_relaxed);
		scope.failures.store(0, std::memory_order_relaxed);
		scope.unavailable.store(0, std::memory_order_relaxed);
		scope.timeouts.store(0, std::memory_order_relaxed);
		scope.transport_failures.store(0, std::memory_order_relaxed);
		scope.error_replies.store(0, std::memory_order_relaxed);
		scope.no_replies.store(0, std::memory_order_relaxed);
		scope.consecutive_failures.store(0, std::memory_order_relaxed);
		scope.total_latency_usec.store(0, std::memory_order_relaxed);
		scope.last_latency_usec.store(0, std::memory_order_relaxed);
		scope.max_latency_usec.store(0, std::memory_order_relaxed);
		scope.last_success_msec.store(0, std::memory_order_relaxed);
	}
	for (std::atomic<uint64_t> &calls : command_kind_calls)
		calls.store(0, std::memory_order_relaxed);
	connection_attempts.store(0, std::memory_order_relaxed);
	connection_failures.store(0, std::memory_order_relaxed);
	reconnect_attempts.store(0, std::memory_order_relaxed);
	reconnect_successes.store(0, std::memory_order_relaxed);
	reconnect_failures.store(0, std::memory_order_relaxed);
	recovery_transitions.store(0, std::memory_order_relaxed);
	connection_available.store(false, std::memory_order_relaxed);
	observability_enabled.store(enabled, std::memory_order_relaxed);
}

void redis_shared_command_observability_set_enabled(bool enabled)
{
	observability_enabled.store(enabled, std::memory_order_relaxed);
	if (!enabled)
		connection_available.store(false, std::memory_order_relaxed);
}

void redis_shared_command_observability_record(redis_shared_command_scope scope,
					       redis_shared_command_kind kind,
					       redis_shared_command_outcome outcome,
					       uint64_t duration_usec)
{
	if (scope < 0 || scope >= REDIS_SHARED_SCOPE_COUNT || kind < 0 ||
	    kind >= REDIS_SHARED_COMMAND_KIND_COUNT)
		return;
	atomic_scope_health &health = scopes[scope];
	const bool primary_connection = scope == REDIS_SHARED_SCOPE_WORLD ||
					scope == REDIS_SHARED_SCOPE_FLOOR;
	health.calls.fetch_add(1, std::memory_order_relaxed);
	command_kind_calls[kind].fetch_add(1, std::memory_order_relaxed);
	health.total_latency_usec.fetch_add(duration_usec, std::memory_order_relaxed);
	health.last_latency_usec.store(duration_usec, std::memory_order_relaxed);
	update_max(&health.max_latency_usec, duration_usec);
	if (outcome == REDIS_SHARED_OUTCOME_SUCCESS)
	{
		health.successes.fetch_add(1, std::memory_order_relaxed);
		health.consecutive_failures.store(0, std::memory_order_relaxed);
		health.last_success_msec.store(monotonic_msec(), std::memory_order_relaxed);
		if (primary_connection)
			connection_available.store(true, std::memory_order_relaxed);
		return;
	}
	health.failures.fetch_add(1, std::memory_order_relaxed);
	health.consecutive_failures.fetch_add(1, std::memory_order_relaxed);
	switch (outcome)
	{
	case REDIS_SHARED_OUTCOME_UNAVAILABLE:
		health.unavailable.fetch_add(1, std::memory_order_relaxed);
		if (primary_connection)
			connection_available.store(false, std::memory_order_relaxed);
		break;
	case REDIS_SHARED_OUTCOME_TIMEOUT:
		health.timeouts.fetch_add(1, std::memory_order_relaxed);
		if (primary_connection)
			connection_available.store(false, std::memory_order_relaxed);
		break;
	case REDIS_SHARED_OUTCOME_TRANSPORT:
		health.transport_failures.fetch_add(1, std::memory_order_relaxed);
		if (primary_connection)
			connection_available.store(false, std::memory_order_relaxed);
		break;
	case REDIS_SHARED_OUTCOME_ERROR_REPLY:
		health.error_replies.fetch_add(1, std::memory_order_relaxed);
		if (primary_connection)
			connection_available.store(true, std::memory_order_relaxed);
		break;
	case REDIS_SHARED_OUTCOME_NO_REPLY:
		health.no_replies.fetch_add(1, std::memory_order_relaxed);
		if (primary_connection)
			connection_available.store(false, std::memory_order_relaxed);
		break;
	case REDIS_SHARED_OUTCOME_SUCCESS:
		break;
	}
}

void redis_shared_connection_observability_record(bool reconnect, bool success)
{
	if (reconnect)
	{
		reconnect_attempts.fetch_add(1, std::memory_order_relaxed);
		if (success)
		{
			reconnect_successes.fetch_add(1, std::memory_order_relaxed);
			recovery_transitions.fetch_add(1, std::memory_order_relaxed);
		}
		else
			reconnect_failures.fetch_add(1, std::memory_order_relaxed);
	}
	else
	{
		connection_attempts.fetch_add(1, std::memory_order_relaxed);
		if (!success)
			connection_failures.fetch_add(1, std::memory_order_relaxed);
	}
	connection_available.store(success, std::memory_order_relaxed);
}

redis_shared_command_health redis_shared_command_health_copy(void)
{
	redis_shared_command_health snapshot = {};
	const uint64_t now_msec = monotonic_msec();
	for (size_t index = 0; index < scopes.size(); ++index)
	{
		const atomic_scope_health &source = scopes[index];
		redis_shared_scope_health &target = snapshot.scopes[index];
		target.calls = source.calls.load(std::memory_order_relaxed);
		target.successes = source.successes.load(std::memory_order_relaxed);
		target.failures = source.failures.load(std::memory_order_relaxed);
		target.unavailable = source.unavailable.load(std::memory_order_relaxed);
		target.timeouts = source.timeouts.load(std::memory_order_relaxed);
		target.transport_failures =
			source.transport_failures.load(std::memory_order_relaxed);
		target.error_replies = source.error_replies.load(std::memory_order_relaxed);
		target.no_replies = source.no_replies.load(std::memory_order_relaxed);
		target.consecutive_failures =
			source.consecutive_failures.load(std::memory_order_relaxed);
		target.total_latency_usec =
			source.total_latency_usec.load(std::memory_order_relaxed);
		target.last_latency_usec = source.last_latency_usec.load(std::memory_order_relaxed);
		target.max_latency_usec = source.max_latency_usec.load(std::memory_order_relaxed);
		const uint64_t last_success =
			source.last_success_msec.load(std::memory_order_relaxed);
		target.last_success_available = last_success != 0;
		if (last_success)
			target.last_success_age_msec =
				now_msec >= last_success ? now_msec - last_success : 0;
	}
	for (size_t index = 0; index < command_kind_calls.size(); ++index)
		snapshot.command_kind_calls[index] =
			command_kind_calls[index].load(std::memory_order_relaxed);
	snapshot.connection_attempts = connection_attempts.load(std::memory_order_relaxed);
	snapshot.connection_failures = connection_failures.load(std::memory_order_relaxed);
	snapshot.reconnect_attempts = reconnect_attempts.load(std::memory_order_relaxed);
	snapshot.reconnect_successes = reconnect_successes.load(std::memory_order_relaxed);
	snapshot.reconnect_failures = reconnect_failures.load(std::memory_order_relaxed);
	snapshot.recovery_transitions = recovery_transitions.load(std::memory_order_relaxed);
	snapshot.enabled = observability_enabled.load(std::memory_order_relaxed);
	snapshot.connection_available = connection_available.load(std::memory_order_relaxed);
	return snapshot;
}

uint64_t redis_observability_now_usec(void)
{
	return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(
					     std::chrono::steady_clock::now().time_since_epoch())
					     .count());
}

void redis_worker_operation_record(redis_worker_operation_health *health,
				   redis_shared_command_outcome outcome, uint64_t duration_usec)
{
	if (!health)
		return;
	++health->calls;
	health->total_latency_usec += duration_usec;
	health->last_latency_usec = duration_usec;
	health->max_latency_usec = std::max(health->max_latency_usec, duration_usec);
	if (outcome == REDIS_SHARED_OUTCOME_SUCCESS)
	{
		++health->successes;
		health->consecutive_failures = 0;
		health->last_success_monotonic_msec = monotonic_msec();
		health->last_success_available = true;
		return;
	}
	++health->failures;
	++health->consecutive_failures;
	switch (outcome)
	{
	case REDIS_SHARED_OUTCOME_UNAVAILABLE:
		++health->unavailable;
		break;
	case REDIS_SHARED_OUTCOME_TIMEOUT:
		++health->timeouts;
		break;
	case REDIS_SHARED_OUTCOME_TRANSPORT:
		++health->transport_failures;
		break;
	case REDIS_SHARED_OUTCOME_ERROR_REPLY:
	case REDIS_SHARED_OUTCOME_NO_REPLY:
		++health->response_failures;
		break;
	case REDIS_SHARED_OUTCOME_SUCCESS:
		break;
	}
}

void redis_worker_operation_prepare_snapshot(redis_worker_operation_health *health)
{
	if (!health)
		return;
	health->last_success_age_msec = 0;
	if (!health->last_success_available)
		return;
	const uint64_t now_msec = monotonic_msec();
	health->last_success_age_msec = now_msec >= health->last_success_monotonic_msec ?
						now_msec - health->last_success_monotonic_msec :
						0;
}
