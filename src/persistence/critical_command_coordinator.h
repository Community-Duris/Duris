#ifndef CRITICAL_COMMAND_COORDINATOR_H
#define CRITICAL_COMMAND_COORDINATOR_H

#include "persistence/critical_command_completion.h"
#include "persistence/critical_command_journal.h"

#include <cstddef>
#include <cstdint>

class economic_sql_lifecycle_guard;
class economic_sql_cutover_transaction_owner;

// Private coordinator-side lease operations used only by the SQL lifecycle
// owner. They expose no readiness boolean or lease identity to public callers.
class critical_command_coordinator_owner final
{
	friend class economic_sql_lifecycle_guard;
	friend class economic_sql_cutover_transaction_owner;
	static bool acquire_cutover_lease(uint64_t timeout_msec, uint64_t *generation,
					  uint64_t *lease_id);
	static bool validate_cutover_lease(uint64_t generation, uint64_t lease_id);
	static bool release_cutover_lease(uint64_t generation, uint64_t lease_id);
	static bool begin_cutover_transaction(uint64_t generation, uint64_t lease_id,
					      const void *connection, unsigned long session);
	static bool validate_cutover_transaction(uint64_t generation, uint64_t lease_id,
						 const void *connection, unsigned long session);
	static void set_cutover_outcome_uncertain(uint64_t generation, uint64_t lease_id,
						  const void *connection, unsigned long session,
						  bool uncertain);
	static bool finish_cutover_transaction(uint64_t generation, uint64_t lease_id,
					       const void *connection, unsigned long session);
};

constexpr size_t CRITICAL_COORDINATOR_MAX_OPERATIONS = 1024;
constexpr size_t CRITICAL_COORDINATOR_MAX_BYTES = 64 * 1024 * 1024;
constexpr size_t CRITICAL_COORDINATOR_COMPLETED_CACHE_MAX = 256;
constexpr size_t CRITICAL_COORDINATOR_COMPLETED_CACHE_BYTES = 8 * 1024 * 1024;
constexpr unsigned int CRITICAL_COORDINATOR_MAX_RETRIES = 8;
constexpr unsigned int CRITICAL_COORDINATOR_DEFAULT_WORKERS = 2;

enum class critical_submit_result : uint8_t
{
	accepted,
	// The operation is reserved in memory and queued for the journal worker.
	// This result is not evidence that the command is durable or executable.
	awaiting_durability,
	attached,
	invalid,
	identity_conflict,
	overloaded,
	journal_failure,
	journal_uncertain,
	unavailable,
};

// journal_uncertain retains the original coordinator operation and the caller's pending
// state; it is not a durability success, but callers must not erase or retry it.
inline bool critical_submit_result_keeps_operation(critical_submit_result result)
{
	return result == critical_submit_result::accepted ||
	       result == critical_submit_result::awaiting_durability ||
	       result == critical_submit_result::attached ||
	       result == critical_submit_result::journal_uncertain;
}

// This reports journal admission only.  `durable` does not imply that execution
// or live publication has completed.
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
	uint64_t publication_pending;
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
	uint64_t awaiting_durability;
	uint64_t admission_queue_bytes;
	uint64_t durable_admissions;
	uint64_t admission_failures;
	uint64_t admission_uncertain;
	bool initialized;
	bool accepting;
	bool running;
	bool admission_worker_running;
	bool append_inflight;
	bool cutover_issuing;
	bool cutover_transaction_active;
	bool cutover_outcome_uncertain;
	bool shutdown_refused;
};

using critical_apply_fn = critical_apply_result (*)(const critical_command &command, void *context);
using critical_drain_observer_fn = void (*)(const critical_completion *completions, size_t count);
using critical_replay_observer_fn = bool (*)(const critical_command &command, void *context);

// Optional support for canonical schema-2 commands. The validator must be pure,
// bounded and noexcept; it verifies typed immutable evidence, never current
// authority or activation state (retained receipts must remain replayable).
// It runs under the coordinator mutex and must not call coordinator APIs.
// The caller must pair it with an apply function supporting the same routes.
using critical_extension_validator_fn = bool (*)(const critical_command &) noexcept;

bool critical_command_coordinator_init(
	const char *journal_directory, critical_apply_fn apply, void *context,
	unsigned int workers = CRITICAL_COORDINATOR_DEFAULT_WORKERS,
	critical_replay_observer_fn replay_observer = nullptr, void *replay_context = nullptr,
	critical_extension_validator_fn extension_validator = nullptr);
// Atomically reserves an owner-free application lifecycle boundary without
// stopping workers or discarding accepted operations. False leaves the active
// owner untouched. Keep the guard through shutdown/copyover dependencies, then
// release it if the lifecycle operation is cancelled or after SQL teardown.
bool critical_command_coordinator_try_acquire_lifecycle_guard(void);
void critical_command_coordinator_release_lifecycle_guard(void);
// Returns false without changing coordinator lifetime when an owner is issuing
// or holds a cutover. Retry only after that owner reaches a terminal boundary.
bool critical_command_coordinator_shutdown(void);
critical_submit_result critical_command_coordinator_submit(critical_command command);
// Opt-in path for commands whose durable result is not complete until the game
// thread has safely published its live projection. The operation and all of its
// entity fences remain held until critical_command_coordinator_acknowledge_publication().
critical_submit_result
critical_command_coordinator_submit_for_publication(critical_command command);
// `awaiting_durability` is the only positive submit result before the admission
// worker has acknowledged the journal append and fsync.
critical_command_durability
critical_command_coordinator_durability(const critical_operation_id &operation_id);
bool critical_command_coordinator_recover_uncertain(void);
bool critical_command_coordinator_get_completed(const critical_operation_id &operation_id,
						critical_completion *completion);
// Release a publication-held operation only after the live callback succeeded and
// the journal checkpoint was durable. A false result leaves the operation fenced.
bool critical_command_coordinator_acknowledge_publication(const critical_operation_id &operation_id);
size_t critical_command_coordinator_pulse(critical_completion *completions, size_t capacity);
bool critical_command_coordinator_is_fenced(const critical_entity_key &key,
					    critical_operation_id *operation_id);
void critical_command_coordinator_quiesce(void);
void critical_command_coordinator_resume(void);
bool critical_command_coordinator_drain(uint64_t timeout_msec);
// Point-in-time queue/journal preflight for diagnostics and callers that do not
// own a cutover. It is never authority to begin SQL work. The private owner API
// reserves admission before drain and retains it through exact-session terminal
// SQL completion; ordinary resume cannot override that retained owner.
bool critical_command_coordinator_cutover_ready(void);
void critical_command_coordinator_set_drain_observer(critical_drain_observer_fn observer);
critical_coordinator_health critical_command_coordinator_health_copy(void);
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
// Diagnostic copy of retained corpse commands, including exhausted retries.
// Capacity bounds output; total retains the count when display is truncated.
size_t critical_command_coordinator_recovery_copy(critical_recovery_case *cases, size_t capacity,
						  size_t *total, size_t offset = 0);
bool critical_command_coordinator_inject_completion_for_tests(const critical_completion &completion);
void critical_command_coordinator_reset_for_tests(void);

#endif
