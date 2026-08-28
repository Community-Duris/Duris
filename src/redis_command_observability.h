#ifndef REDIS_COMMAND_OBSERVABILITY_H
#define REDIS_COMMAND_OBSERVABILITY_H

#include <stdint.h>

enum redis_shared_command_scope
{
	REDIS_SHARED_SCOPE_WORLD = 0,
	REDIS_SHARED_SCOPE_FLOOR,
	REDIS_SHARED_SCOPE_CACHE,
	REDIS_SHARED_SCOPE_MAINTENANCE,
	REDIS_SHARED_SCOPE_COUNT
};

enum redis_shared_command_kind
{
	REDIS_SHARED_COMMAND_READ = 0,
	REDIS_SHARED_COMMAND_WRITE,
	REDIS_SHARED_COMMAND_SCAN,
	REDIS_SHARED_COMMAND_SCRIPT,
	REDIS_SHARED_COMMAND_KIND_COUNT
};

enum redis_shared_command_outcome
{
	REDIS_SHARED_OUTCOME_SUCCESS = 0,
	REDIS_SHARED_OUTCOME_UNAVAILABLE,
	REDIS_SHARED_OUTCOME_TIMEOUT,
	REDIS_SHARED_OUTCOME_TRANSPORT,
	REDIS_SHARED_OUTCOME_ERROR_REPLY,
	REDIS_SHARED_OUTCOME_NO_REPLY
};

struct redis_shared_scope_health
{
	uint64_t calls;
	uint64_t successes;
	uint64_t failures;
	uint64_t unavailable;
	uint64_t timeouts;
	uint64_t transport_failures;
	uint64_t error_replies;
	uint64_t no_replies;
	uint64_t consecutive_failures;
	uint64_t total_latency_usec;
	uint64_t last_latency_usec;
	uint64_t max_latency_usec;
	uint64_t last_success_age_msec;
	bool last_success_available;
};

struct redis_shared_command_health
{
	redis_shared_scope_health scopes[REDIS_SHARED_SCOPE_COUNT];
	uint64_t command_kind_calls[REDIS_SHARED_COMMAND_KIND_COUNT];
	uint64_t connection_attempts;
	uint64_t connection_failures;
	uint64_t reconnect_attempts;
	uint64_t reconnect_successes;
	uint64_t reconnect_failures;
	uint64_t recovery_transitions;
	bool enabled;
	bool connection_available;
};

const char *redis_shared_command_scope_name(redis_shared_command_scope scope);
void redis_shared_command_observability_reset(bool enabled);
void redis_shared_command_observability_set_enabled(bool enabled);
void redis_shared_command_observability_record(redis_shared_command_scope scope,
					       redis_shared_command_kind kind,
					       redis_shared_command_outcome outcome,
					       uint64_t duration_usec);
void redis_shared_connection_observability_record(bool reconnect, bool success);
redis_shared_command_health redis_shared_command_health_copy(void);

#endif
