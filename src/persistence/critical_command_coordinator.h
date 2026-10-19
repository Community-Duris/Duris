/****************************************************************************
 *
 *  File: critical_command_coordinator.h                        Part of Duris
 *  Usage: critical command coordinator interface and health
 *
 *  Copyright 1990, 1991 - see LICENSE for complete information
 *
 *  Additions since 2025 by the Duris maintainers and since 2026 by
 *    LuminariMUD maintainers are public domain (Unlicense, see LICENSE)
 *
 *  Created by: Duris, LuminariMUD, Zusuk                  Date: 2026-09-23
 *
 ****************************************************************************/

#ifndef CRITICAL_COMMAND_COORDINATOR_H
#define CRITICAL_COMMAND_COORDINATOR_H

#include "persistence/critical_command_completion.h"
#include "persistence/critical_command_journal.h"

#include <cstddef>
#include <cstdint>

constexpr size_t CRITICAL_COORDINATOR_MAX_OPERATIONS = 1024;
constexpr size_t CRITICAL_COORDINATOR_MAX_BYTES = 64 * 1024 * 1024;
constexpr size_t CRITICAL_COORDINATOR_COMPLETED_CACHE_MAX = 256;
constexpr size_t CRITICAL_COORDINATOR_COMPLETED_CACHE_BYTES = 8 * 1024 * 1024;
constexpr unsigned int CRITICAL_COORDINATOR_MAX_RETRIES = 8;
constexpr unsigned int CRITICAL_COORDINATOR_DEFAULT_WORKERS = 2;

enum class critical_submit_result : uint8_t
{
	// Queued on the one persistence writer; durable once it has landed.
	accepted,
	awaiting_durability,
	attached,
	invalid,
	identity_conflict,
	overloaded,
	journal_failure,
	journal_uncertain,
	unavailable,
};

inline bool critical_submit_result_keeps_operation(critical_submit_result result)
{
	return result == critical_submit_result::accepted ||
	       result == critical_submit_result::awaiting_durability ||
	       result == critical_submit_result::attached ||
	       result == critical_submit_result::journal_uncertain;
}

enum class critical_command_durability : uint8_t
{
	unknown,
	awaiting_durability,
	durable,
	uncertain,
	failed,
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
	uint64_t awaiting_durability = 0;
	uint64_t admission_queue_bytes = 0;
	bool admission_worker_running = false;
	bool append_inflight = false;
	uint64_t durable_admissions = 0;
	uint64_t admission_failures = 0;
	uint64_t admission_uncertain = 0;
	uint64_t publication_pending = 0;
	bool initialized;
	bool accepting;
	bool running;
};

using critical_apply_fn = critical_apply_result (*)(const critical_command &command, void *context);
using critical_drain_observer_fn = void (*)(const critical_completion *completions, size_t count);

// Commands run on the one persistence writer (player_save_worker.h), which must be
// running.
bool critical_command_coordinator_init(critical_apply_fn apply, void *context);
void critical_command_coordinator_shutdown(void);
critical_submit_result critical_command_coordinator_submit(critical_command command);
size_t critical_command_coordinator_pulse(critical_completion *completions, size_t capacity);
bool critical_command_coordinator_is_fenced(const critical_entity_key &key,
					    critical_operation_id *operation_id);
void critical_command_coordinator_quiesce(void);
void critical_command_coordinator_resume(void);
bool critical_command_coordinator_drain(uint64_t timeout_msec);
void critical_command_coordinator_set_drain_observer(critical_drain_observer_fn observer);
critical_coordinator_health critical_command_coordinator_health_copy(void);
bool critical_command_coordinator_inject_completion_for_tests(const critical_completion &completion);
void critical_command_coordinator_reset_for_tests(void);

using critical_replay_observer_fn = bool (*)(const critical_command &command, void *context);

inline bool
critical_command_coordinator_init(const char * /*journal_directory*/, critical_apply_fn apply,
				  void *context,
				  unsigned int /*workers*/ = CRITICAL_COORDINATOR_DEFAULT_WORKERS,
				  critical_replay_observer_fn /*replay_observer*/ = nullptr,
				  void * /*replay_context*/ = nullptr)
{
	return critical_command_coordinator_init(apply, context);
}

// These callers retain their entity fences through deferred or failed live publication.
critical_submit_result
critical_command_coordinator_submit_for_publication(critical_command command);
bool critical_command_coordinator_acknowledge_publication(const critical_operation_id &operation_id);

inline critical_command_durability
critical_command_coordinator_durability(const critical_operation_id &)
{
	return critical_command_durability::durable;
}

inline bool critical_command_coordinator_recover_uncertain(void)
{
	return true;
}

bool critical_command_coordinator_get_completed(const critical_operation_id &operation_id,
						critical_completion *completion);

struct critical_recovery_case
{
	critical_operation_id operation_id = {};
	char correlation[33] = {};
	const char *owner = "critical_command";
	const char *state = "unresolved";
	unsigned int attempts = 0;
	unsigned int error_code = 0;
	uint64_t elapsed_msec = 0;
};

inline size_t critical_command_coordinator_recovery_copy(critical_recovery_case *, size_t,
							 size_t *total, size_t = 0)
{
	if (total)
		*total = 0;
	return 0;
}

#endif
