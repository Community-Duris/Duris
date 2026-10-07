#ifndef CRITICAL_COMMAND_COORDINATOR_H
#define CRITICAL_COMMAND_COORDINATOR_H

#include "persistence/critical_command_completion.h"
#include "persistence/critical_command_journal.h"

#include <cstddef>
#include <cstdint>
#include <span>

class economic_sql_lifecycle_guard;
class economic_sql_cutover_transaction_owner;
class player_save_restored_publication_owner;
// Only an exact private pipeline owner can consume a canonical pre-admission
// collector refusal. This neither checkpoints a journal nor fabricates ACK.
bool critical_command_coordinator_cancel_collector_publication(
	player_save_restored_publication_owner &);
// Exact private SHOP refusal: authenticate and pin the retained original
// before cleanup; release the hold only after successful same-generation proof.
bool critical_command_coordinator_cancel_shop_publication(
	player_save_restored_publication_owner &owner,
	bool (*native_cleanup)(const critical_command &, const critical_completion &,
			       void *) noexcept,
	void *context);

// Only the private auction save owner can consume an original never-admitted
// refusal after exact retained native-body and successful absence/BEFORE proof.
bool critical_command_coordinator_cancel_auction_publication(
	player_save_restored_publication_owner &,
	bool (*)(const critical_command &, const critical_completion &, void *) noexcept, void *);

// Private coordinator-side lease operations used only by the SQL lifecycle
// owner. They expose no readiness boolean or lease identity to public callers.
class critical_command_coordinator_owner final
{
	friend class sql_economic_runtime_boot_owner;
	// Exact original journal/operation/fence cut while this boot thread owns
	// the real lifecycle reservation. A diagnostic health copy is insufficient.
	static bool boot_recovery_ready();
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
	uint64_t native_continuation_pending;
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

// Passive original native registration under coordinator_mutex; no reentry,
// submission, SQL or native effects. The context is the existing replay_context.
using critical_native_recovery_observer_fn = bool (*)(const critical_native_recovery_envelope &,
						      void *context);
// Pure structural check of the retained original publication body and exact
// retained receipt. No reentry/effects; this is never a caller ACK permit.
using critical_native_recovery_publication_validator_fn =
	bool (*)(const critical_native_recovery_envelope &, const critical_completion &) noexcept;

// Pure original quest parent/child body correlation. Registered once with the
// real domain codec; this carries no current SQL/world or reward ACK authority.
using critical_native_recovery_pair_validator_fn = bool (*)(
	const critical_native_recovery_envelope &, const critical_native_recovery_envelope &,
	const critical_native_recovery_envelope *) noexcept;

// Pure birth-domain body checks. These authenticate bounded original progress,
// never current SQL/world state or a caller's physical-publication assertion.
// All five callbacks are required for birth-v2/v3 envelopes; quest authority is separate.
struct critical_native_birth_recovery_validators
{
	bool (*valid)(const critical_native_recovery_envelope &) noexcept = nullptr;
	bool (*initial)(const critical_native_recovery_envelope &) noexcept = nullptr;
	bool (*successor)(const critical_native_recovery_envelope &,
			  const critical_native_recovery_envelope &) noexcept = nullptr;
	bool (*publication)(const critical_native_recovery_envelope &,
			    const critical_completion &) noexcept = nullptr;
	bool (*terminal)(const critical_native_recovery_envelope &) noexcept = nullptr;
};

// Pure auction NAR checks. All callbacks must be registered by the genuine
// domain codec before typed admission/replay. They grant no SQL/world authority.
struct critical_native_auction_recovery_validators
{
	bool (*valid)(const critical_native_recovery_envelope &) noexcept = nullptr;
	bool (*initial)(const critical_native_recovery_envelope &) noexcept = nullptr;
	bool (*successor)(const critical_native_recovery_envelope &,
			  const critical_native_recovery_envelope &) noexcept = nullptr;
	bool (*publication)(const critical_native_recovery_envelope &,
			    const critical_completion &) noexcept = nullptr;
	bool (*terminal)(const critical_native_recovery_envelope &) noexcept = nullptr;
};

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
	critical_extension_validator_fn extension_validator = nullptr,
	critical_native_recovery_observer_fn native_replay_observer = nullptr,
	critical_native_recovery_publication_validator_fn native_publication_validator = nullptr,
	critical_native_birth_recovery_validators birth_validators = {},
	critical_native_recovery_pair_validator_fn quest_pair_validator = nullptr,
	critical_native_auction_recovery_validators auction_validators = {});
// Separate original owner capabilities: continuation owners cannot submit or
// cross physical ACK. Opaque context carries no source/SQL/publication authority.
// Only the original birth owner can cross this physical publication boundary.
// It must first prove its original SQL receipt, current native/custody image and
// consumed world publication. These methods themselves grant none of that proof.
// Birth uses its original command and typed progress carrier, not a player save token.
class critical_native_mobile_birth_publication_owner final
{
	friend class quest_mobile_native_birth_owner;
	// Typed v2/v3 admission and exact same-phase progress CAS. Raw v1 remains readable.
	static critical_submit_result submit(critical_native_recovery_envelope);
	static bool copy_context(const critical_command &,
				 critical_native_recovery_envelope *) noexcept;
	static bool checkpoint_context(const critical_native_recovery_envelope &,
				       const critical_native_recovery_envelope &) noexcept;
	static bool observe_generation(const critical_native_recovery_envelope &,
				       uint64_t *) noexcept;
	static bool cancel_refusal(const critical_native_recovery_envelope &,
				   const critical_completion &, uint64_t,
				   bool (*)(const critical_command &, const critical_completion &,
					    void *) noexcept,
				   void *) noexcept;
	// Physical ACK retains the exact body in continuation phase. Terminal retirement
	// is separate, and never consumes a player-save hold or uses command-only cache.
	static bool acknowledge(const critical_native_recovery_envelope &,
				const critical_completion &, uint64_t) noexcept;
	static bool retire(const critical_native_recovery_envelope &) noexcept;
	// Constructor births retain advancement fences across phase2. This pins
	// the actual terminal envelope/generation over the original native owner's
	// durable transfer, then retires the journal and releases fences on success.
	static bool retire(const critical_native_recovery_envelope &, uint64_t,
			   bool (*)(const critical_native_recovery_envelope &, void *) noexcept,
			   void *) noexcept;
	static bool observe_generation(const critical_command &, uint64_t *) noexcept;
	// Only a delivered, exact retained no-admission receipt permits cleanup.
	// Pins the original operation over cleanup and removes it only on success.
	// No journal checkpoint, completed cache entry or execution ACK is created.
	static bool cancel_refusal(const critical_command &, const critical_completion &,
				   uint64_t original_coordinator_generation,
				   bool (*native_cleanup)(const critical_command &,
							  const critical_completion &,
							  void *) noexcept,
				   void *context) noexcept;
	static bool acknowledge(const critical_command &, const critical_completion &,
				uint64_t original_coordinator_generation) noexcept;
};

// These are private capabilities of the actual auction save/native owners.
// Publication retains the physical hold; notice continuation cannot cross ACK.
class critical_native_auction_submission_owner final
{
	friend class player_save_auction_checkpoint_owner;
	static critical_submit_result submit(critical_native_recovery_envelope);
};
class critical_native_auction_publication_owner final
{
	friend class player_save_auction_publication_owner;
	friend class critical_native_auction_background_publication_owner;
	static bool copy_context(const critical_command &,
				 critical_native_recovery_envelope *) noexcept;
	static bool checkpoint_context(const critical_native_recovery_envelope &,
				       const critical_native_recovery_envelope &) noexcept;
};
// Native actor-zero finalize/removal has no player save slot or synthetic PID.
// Only its actual domain owner can consume original SQL/world proof through ACK.
class critical_native_auction_background_publication_owner final
{
	friend class auction_native_publication_owner;
	static critical_submit_result submit(critical_native_recovery_envelope);
	static bool copy_context(const critical_command &,
				 critical_native_recovery_envelope *) noexcept;
	static bool checkpoint_context(const critical_native_recovery_envelope &,
				       const critical_native_recovery_envelope &) noexcept;
	static bool observe_generation(const critical_native_recovery_envelope &,
				       uint64_t *) noexcept;
	static bool cancel_refusal(const critical_native_recovery_envelope &,
				   const critical_completion &, uint64_t,
				   bool (*)(const critical_command &, const critical_completion &,
					    void *) noexcept,
				   void *) noexcept;
	static bool acknowledge(const critical_native_recovery_envelope &,
				const critical_completion &, uint64_t) noexcept;
};

class critical_native_auction_continuation_owner final
{
	friend class auction_native_publication_owner;
	static bool copy_context(const critical_command &,
				 critical_native_recovery_envelope *) noexcept;
	static bool checkpoint_context(const critical_native_recovery_envelope &,
				       const critical_native_recovery_envelope &) noexcept;
	static bool retire_continuation(const critical_native_recovery_envelope &) noexcept;
};

class critical_native_quest_submission_owner final
{
	friend class player_save_native_quest_checkpoint_owner;
	static critical_submit_result submit(critical_native_recovery_envelope envelope);
};

class critical_native_quest_publication_owner final
{
	friend class player_save_native_quest_publication_owner;
	friend class player_save_native_quest_checkpoint_owner;
	// Borrow one authentic retained fee parent without changing either owner.
	static bool
	copy_fee_acceptance_context(const critical_command &actual_fee,
				    critical_native_recovery_envelope *original_parent) noexcept;
	static bool copy_fee_ack_context(const critical_command &,
					 critical_native_recovery_envelope *) noexcept;
	static bool copy_context(const critical_command &,
				 critical_native_recovery_envelope *) noexcept;
	static bool checkpoint_context(const critical_native_recovery_envelope &expected,
				       const critical_native_recovery_envelope &successor) noexcept;
};

class critical_native_quest_continuation_owner final
{
	friend class quest_native_gameplay_owner;
	friend class quest_native_frozen_continuation_owner;
	friend class quest_reward_obligation_native_fee_owner;
	static bool copy_context(const critical_command &,
				 critical_native_recovery_envelope *) noexcept;
	// Retained-state correlation only: select one authentic phase2 parent of
	// this exact physically released child through the registered domain check.
	// No SQL/world/journal effects or completion authority. Ambiguity/refusal
	// preserves output; context uncertainty remains eligible for paired retries.
	static bool copy_parent_context(const critical_native_recovery_envelope &child,
					critical_native_recovery_envelope *output) noexcept;
	// Read-only borrowing of one authentic released fee child and its retained
	// acceptance parent; ambiguity or uncertain checkpoints refuse unchanged.
	static bool
	copy_fee_obligation_context(const critical_operation_id &actual_action,
				    std::span<const uint8_t> literal_continuation,
				    critical_native_recovery_envelope *actual_child,
				    critical_native_recovery_envelope *original_parent) noexcept;
	static bool checkpoint_context(const critical_native_recovery_envelope &expected,
				       const critical_native_recovery_envelope &successor) noexcept;
	static bool retire_continuation(const critical_native_recovery_envelope &expected) noexcept;
	// Pins both exact phase2 operations and their retained byte budget over one
	// original journal rewrite. Prefix successor retains the authentic latest
	// child; terminal null requires prior genuine domain/reward-completion proof.
	// Success must be latched by the owner; absence never confirms a repeat.
	static bool
	transition_pair(const critical_native_recovery_envelope &parent,
			const critical_native_recovery_envelope &child,
			const critical_native_recovery_envelope *parent_successor) noexcept;
};

// Atomically reserves an owner-free application lifecycle boundary without
// stopping workers or discarding accepted operations. False leaves the active
// owner untouched. Keep the guard through shutdown/copyover dependencies, then
// release it if the lifecycle operation is cancelled or after SQL teardown.
bool critical_command_coordinator_try_acquire_lifecycle_guard(void);
bool critical_command_coordinator_lifecycle_guard_held_by_current_thread(void);
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
// Only the original restored save owner can supply this nonconstructible proof.
bool critical_command_coordinator_acknowledge_publication(
	player_save_restored_publication_owner &owner);
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
