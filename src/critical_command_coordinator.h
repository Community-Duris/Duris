#ifndef CRITICAL_COMMAND_COORDINATOR_H
#define CRITICAL_COMMAND_COORDINATOR_H

#include "critical_command.h"
#include "critical_command_journal.h"

#include <cstddef>
#include <cstdint>

constexpr size_t CRITICAL_COORDINATOR_MAX_OPERATIONS = 1024;
constexpr size_t CRITICAL_COORDINATOR_MAX_BYTES = 64 * 1024 * 1024;
constexpr size_t CRITICAL_COORDINATOR_MAX_RESULTS = 2048;
constexpr size_t CRITICAL_COORDINATOR_COMPLETED_CACHE_MAX = 256;
constexpr size_t CRITICAL_COORDINATOR_COMPLETED_CACHE_BYTES = 8 * 1024 * 1024;
constexpr unsigned int CRITICAL_COORDINATOR_MAX_RETRIES = 8;
constexpr unsigned int CRITICAL_COORDINATOR_DEFAULT_WORKERS = 2;

enum class critical_apply_outcome : uint8_t
{
	applied,
	already_applied,
	retryable_failure,
	ambiguous_commit,
	terminal_failure,
};

struct critical_apply_result
{
	critical_apply_outcome outcome;
	uint64_t durable_revision;
	unsigned int error_code;
};

struct critical_completion
{
	critical_operation_id operation_id;
	critical_apply_outcome outcome;
	uint64_t durable_revision;
	unsigned int error_code;
	unsigned int attempt;
	uint64_t queued_at_usec;
	uint64_t started_at_usec;
	uint64_t completed_at_usec;
};

enum class critical_submit_result : uint8_t
{
	accepted,
	attached,
	invalid,
	identity_conflict,
	overloaded,
	journal_failure,
	unavailable,
};

struct critical_coordinator_health
{
	uint64_t queued;
	uint64_t inflight;
	uint64_t blocked;
	uint64_t retained_bytes;
	uint64_t completed_cache;
	uint64_t fenced_keys;
	uint64_t high_water_operations;
	uint64_t high_water_bytes;
	uint64_t oldest_age_msec;
	uint64_t accepted;
	uint64_t attached;
	uint64_t completed;
	uint64_t retries;
	uint64_t ambiguous;
	uint64_t terminal_failures;
	uint64_t stale_completions;
	uint64_t overloads;
	bool initialized;
	bool accepting;
	bool running;
};

using critical_apply_fn = critical_apply_result (*)(const critical_command &command, void *context);

bool critical_command_coordinator_init(const char *journal_directory, critical_apply_fn apply,
				       void *context,
				       unsigned int workers = CRITICAL_COORDINATOR_DEFAULT_WORKERS);
void critical_command_coordinator_shutdown(void);
critical_submit_result critical_command_coordinator_submit(critical_command command);
size_t critical_command_coordinator_pulse(critical_completion *completions, size_t capacity);
bool critical_command_coordinator_is_fenced(const critical_entity_key &key,
					    critical_operation_id *operation_id);
void critical_command_coordinator_quiesce(void);
void critical_command_coordinator_resume(void);
bool critical_command_coordinator_drain(uint64_t timeout_msec);
critical_coordinator_health critical_command_coordinator_health_copy(void);
bool critical_command_coordinator_inject_completion_for_tests(const critical_completion &completion);
void critical_command_coordinator_reset_for_tests(void);

#endif
