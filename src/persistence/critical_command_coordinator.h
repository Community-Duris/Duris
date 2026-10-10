#ifndef CRITICAL_COMMAND_COORDINATOR_H
#define CRITICAL_COMMAND_COORDINATOR_H

#include "persistence/critical_command_completion.h"
#include "persistence/critical_command_journal.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <memory>
#include <mutex>
#include <thread>

class economic_sql_lifecycle_guard;
class flatfile_accounting_native_mobile_birth_shared_shop_transaction;
class flatfile_accounting_native_mobile_birth_ordinary_transaction;
class flatfile_accounting_zone_reset_item_transaction;
class economic_sql_cutover_transaction_owner;
class player_save_restored_publication_owner;
// Authenticate the delivered pending receipt, complete original command and
// fence heads for the nonconstructible passive flat SHOP owner. No ACK, pin,
// cancellation, journal or native effect follows from this observation.
bool critical_command_coordinator_restored_shop_publication_current(
	const player_save_restored_publication_owner &) noexcept;
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
	friend class flatfile_initialized_world_owner;
	// Exact original journal/operation/fence cut while this boot thread owns
	// the real lifecycle reservation. A diagnostic health copy is insufficient.
	static bool boot_recovery_ready();
	friend class economic_sql_lifecycle_guard;
	friend class economic_sql_cutover_transaction_owner;
	// Consume the actual current-thread boot reservation without restoring
	// admission. Strict readiness and a fresh genuine lease are checked under
	// the same coordinator mutex; failure leaves the reservation untouched.
	static bool transfer_lifecycle_guard_to_cutover_lease(uint64_t *generation,
							      uint64_t *lease_id) noexcept;
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
	// Reverse only the genuine runtime-origin transfer. Keep admission closed
	// while restoring its original reservation and owner-recorded reopen policy.
	static bool return_runtime_cutover_to_lifecycle_guard(uint64_t generation,
							      uint64_t lease_id,
							      const void *connection,
							      unsigned long session);
	// Actual late initialized boot cut; quiesce remains restrictive across its lifetime.
	static bool acquire_initialized_lifecycle_guard();
	// Exact current-thread late boot reservation cleanup; false keeps ownership.
	static bool finish_initialized_lifecycle_reservation();
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

// Borrowed observations of the SAME original queued/executing native owner.
// Only its actual coordinator worker can mint this noncopyable lifetime handle.
// Observations alone grant no admission, SQL, publication or ACK authority.
class critical_shared_native_execution_owner final
{
	friend class critical_shared_native_execution_dispatch;
	friend class critical_shared_native_sql_execution_owner;
	friend class critical_shared_native_flat_execution_owner;
	critical_shared_native_execution_owner() = default;
	bool current() const noexcept;
	bool current_locked() const noexcept;
	// Only the genuine private flat worker owns this proposal. It contains no
	// held lock: each callback releases its acquired lock on that same thread.
	// Reported bytes come from the private checked participant capacity proof.
	flatfile_accounting_native_mobile_birth_shared_shop_transaction *
	flat_transaction() const noexcept;
	bool retain_flat_transaction(
		std::unique_ptr<flatfile_accounting_native_mobile_birth_shared_shop_transaction> &,
		size_t participant_bytes) const noexcept;
	bool release_unpublished_flat_transaction(
		flatfile_accounting_native_mobile_birth_shared_shop_transaction *) const noexcept;
	const void *operation_ = nullptr;
	const void *native_ = nullptr;
	const critical_command *command_ = nullptr;
	std::span<const uint8_t> attachment_{};
	uint64_t generation_ = 0, revision_ = 0;
	unsigned int attempt_ = 0;
	critical_native_recovery_phase phase_ = critical_native_recovery_phase::execution_pending;
	std::thread::id worker_{};

    public:
	critical_shared_native_execution_owner(const critical_shared_native_execution_owner &) =
		delete;
	critical_shared_native_execution_owner &
	operator=(const critical_shared_native_execution_owner &) = delete;
	critical_shared_native_execution_owner(critical_shared_native_execution_owner &&) = delete;
	critical_shared_native_execution_owner &
	operator=(critical_shared_native_execution_owner &&) = delete;
	const critical_command &command() const noexcept { return *command_; }
	std::span<const uint8_t> attachment() const noexcept { return attachment_; }
	uint64_t revision() const noexcept { return revision_; }
	critical_native_recovery_phase phase() const noexcept { return phase_; }
	unsigned int attempt() const noexcept { return attempt_; }
	uint64_t generation() const noexcept { return generation_; }
};
using critical_shared_native_apply_fn =
	critical_apply_result (*)(const critical_shared_native_execution_owner &, void *context);
// Distinct original ordinary NMB4 worker lifetime. This pin authenticates
// coordinator storage only; actual producer source/live-world protection
// must remain a separate genuine owner cut before atomic entry.
class critical_ordinary_native_execution_owner final
{
	friend class critical_shared_native_execution_dispatch;
	friend class critical_ordinary_native_flat_execution_owner;
	friend class quest_mobile_native_birth_ordinary_execution_lease;
	friend class quest_mobile_native_birth_owner;
	critical_ordinary_native_execution_owner() = default;
	bool current() const noexcept;
	bool current_locked() const noexcept;
	bool borrowed_current() const noexcept;
	bool borrowed_current_locked() const noexcept;
	// Only the genuine private flat worker owns this proposal. It contains no
	// held lock: each callback releases its acquired lock on that same thread.
	// Reported bytes come from the private checked participant capacity proof.
	flatfile_accounting_native_mobile_birth_ordinary_transaction *
	flat_transaction() const noexcept;
	bool retain_flat_transaction(
		std::unique_ptr<flatfile_accounting_native_mobile_birth_ordinary_transaction> &,
		size_t participant_bytes) const noexcept;
	bool release_unpublished_flat_transaction(
		flatfile_accounting_native_mobile_birth_ordinary_transaction *) const noexcept;
	const void *operation_ = nullptr;
	const void *native_ = nullptr;
	const critical_command *command_ = nullptr;
	std::span<const uint8_t> attachment_{};
	uint64_t generation_ = 0, revision_ = 0;
	unsigned int attempt_ = 0;
	critical_native_recovery_phase phase_ = critical_native_recovery_phase::execution_pending;
	std::thread::id worker_{};

    public:
	critical_ordinary_native_execution_owner(const critical_ordinary_native_execution_owner &) =
		delete;
	critical_ordinary_native_execution_owner &
	operator=(const critical_ordinary_native_execution_owner &) = delete;
	critical_ordinary_native_execution_owner(critical_ordinary_native_execution_owner &&) =
		delete;
	critical_ordinary_native_execution_owner &
	operator=(critical_ordinary_native_execution_owner &&) = delete;
	const critical_command &command() const noexcept { return *command_; }
	std::span<const uint8_t> attachment() const noexcept { return attachment_; }
	uint64_t revision() const noexcept { return revision_; }
	critical_native_recovery_phase phase() const noexcept { return phase_; }
	unsigned int attempt() const noexcept { return attempt_; }
	uint64_t generation() const noexcept { return generation_; }
};
using critical_ordinary_native_apply_fn =
	critical_apply_result (*)(const critical_ordinary_native_execution_owner &, void *context);
// Distinct original ROOM execution pin, never a shared SHOP/source capability.
// Actual root callback validates full original INITIAL carrier/season OUTSIDE
// coordinator mutex before any storage effect. No public mint/copy/permission.
class critical_zone_reset_item_execution_owner final
{
	friend class critical_shared_native_execution_dispatch;
	friend class critical_zone_reset_item_flat_execution_owner;
	critical_zone_reset_item_execution_owner() = default;
	bool current() const noexcept;
	bool current_locked() const noexcept;
	// Only the genuine private flat worker owns this proposal. It contains no
	// held lock: each callback releases its acquired lock on that same thread.
	// Reported bytes come from the private checked participant capacity proof.
	flatfile_accounting_zone_reset_item_transaction *flat_transaction() const noexcept;
	bool
	retain_flat_transaction(std::unique_ptr<flatfile_accounting_zone_reset_item_transaction> &,
				size_t participant_bytes) const noexcept;
	bool release_unpublished_flat_transaction(
		flatfile_accounting_zone_reset_item_transaction *) const noexcept;
	const void *operation_ = nullptr;
	const void *native_ = nullptr;
	const critical_command *command_ = nullptr;
	std::span<const uint8_t> attachment_{};
	uint64_t generation_ = 0, revision_ = 0;
	unsigned int attempt_ = 0;
	critical_native_recovery_phase phase_ = critical_native_recovery_phase::execution_pending;
	std::thread::id worker_{};

    public:
	critical_zone_reset_item_execution_owner(const critical_zone_reset_item_execution_owner &) =
		delete;
	critical_zone_reset_item_execution_owner &
	operator=(const critical_zone_reset_item_execution_owner &) = delete;
	critical_zone_reset_item_execution_owner(critical_zone_reset_item_execution_owner &&) =
		delete;
	critical_zone_reset_item_execution_owner &
	operator=(critical_zone_reset_item_execution_owner &&) = delete;
	const critical_command &command() const noexcept { return *command_; }
	std::span<const uint8_t> attachment() const noexcept { return attachment_; }
	uint64_t revision() const noexcept { return revision_; }
	critical_native_recovery_phase phase() const noexcept { return phase_; }
	unsigned int attempt() const noexcept { return attempt_; }
	uint64_t generation() const noexcept { return generation_; }
};
using critical_zone_reset_item_apply_fn =
	critical_apply_result (*)(const critical_zone_reset_item_execution_owner &, void *context);
using critical_drain_observer_fn = void (*)(const critical_completion *completions, size_t count);
using critical_replay_observer_fn = bool (*)(const critical_command &command, void *context);
// Optional genuine passive legacy replay companion. Original observer remains
// mandatory when installed; absence of a paired implementation refuses the new
// capability. Registration/caller selection is separate and unchanged here.
// The actual held coordinator lender owns CURRENT C; outer carries all authentic
// journal/caller/input storage and reserve must not reenter coordinator/journal.
using critical_replay_observer_bounded_fn = bool (*)(const critical_command &, void *replay_context,
						     bool (*)(size_t, void *) noexcept,
						     void *budget_context,
						     size_t outer_live) noexcept;

// Passive original native registration under coordinator_mutex; no reentry,
// submission, SQL or native effects. The context is the existing replay_context.
using critical_native_recovery_observer_fn = bool (*)(const critical_native_recovery_envelope &,
						      void *context);
// Optional complete ROOM-only passive restoration companion. It receives the
// original replay context separately from the actual budget context and full
// live prefix. Caller owns all real journal/envelope/coordinator/ROOT lifetimes;
// reserve must not acquire coordinator/journal locks. Other native families are
// outside this companion's scope. Registration does not select bounded replay.
using critical_native_recovery_observer_bounded_fn = bool (*)(
	const critical_native_recovery_envelope &, void *replay_context,
	bool (*)(size_t, void *) noexcept, void *budget_context, size_t outer_live) noexcept;

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
	// Optional full original owning initial companions, exact family dispatch.
	// Existing five-field aggregate initialization/readiness remains unchanged.
	// Missing companions refuse ONLY the new bounded submission provider.
	// Callbacks must prospectively own full codec/caller storage and use the
	// given actual held-coordinator reserve relay; they grant no source permit.
	bool (*initial_bounded)(const critical_native_recovery_envelope &,
				bool (*)(size_t, void *) noexcept, void *,
				size_t) noexcept = nullptr;
	bool (*cash_role_initial_bounded)(const critical_native_recovery_envelope &,
					  bool (*)(size_t, void *) noexcept, void *,
					  size_t) noexcept = nullptr;
	bool (*shared_shop_valid_bounded)(const critical_native_recovery_envelope &,
					  bool (*)(size_t, void *) noexcept, void *,
					  size_t) noexcept = nullptr;
	bool (*shared_shop_initial_bounded)(const critical_native_recovery_envelope &,
					    bool (*)(size_t, void *) noexcept, void *,
					    size_t) noexcept = nullptr;
};

// Separate type22 checks; missing callbacks refuse admission and replay.
struct critical_zone_reset_recovery_validators
{
	bool (*valid)(const critical_native_recovery_envelope &) noexcept = nullptr;
	bool (*initial)(const critical_native_recovery_envelope &) noexcept = nullptr;
	bool (*successor)(const critical_native_recovery_envelope &,
			  const critical_native_recovery_envelope &) noexcept = nullptr;
	bool (*publication)(const critical_native_recovery_envelope &,
			    const critical_completion &) noexcept = nullptr;
	bool (*terminal)(const critical_native_recovery_envelope &) noexcept = nullptr;
	// Complete pure owning companions for prospectively admitted retirement.
	// Missing companions affect only the new bounded capability.
	bool (*valid_bounded)(const critical_native_recovery_envelope &,
			      bool (*)(size_t, void *) noexcept, void *, size_t) noexcept = nullptr;
	bool (*terminal_bounded)(const critical_native_recovery_envelope &,
				 bool (*)(size_t, void *) noexcept, void *,
				 size_t) noexcept = nullptr;
	bool (*successor_bounded)(const critical_native_recovery_envelope &,
				  const critical_native_recovery_envelope &,
				  bool (*)(size_t, void *) noexcept, void *,
				  size_t) noexcept = nullptr;
	bool (*publication_bounded)(const critical_native_recovery_envelope &,
				    const critical_completion &, bool (*)(size_t, void *) noexcept,
				    void *, size_t) noexcept = nullptr;
	// Complete original initial revision/phase/no-receipt/no-progress proof.
	// Optional companion only; ordinary initial callback/readiness is unchanged.
	bool (*initial_bounded)(const critical_native_recovery_envelope &,
				bool (*)(size_t, void *) noexcept, void *,
				size_t) noexcept = nullptr;
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
// Optional ROOM-only prospective companion, paired with the selected original
// callback. Full absolute prefix is forwarded; reserve may run under the
// coordinator mutex and must not acquire coordinator/journal locks. Other
// command families remain outside this bounded companion's scope.
using critical_extension_validator_bounded_fn = bool (*)(const critical_command &,
							 bool (*)(size_t, void *) noexcept, void *,
							 size_t) noexcept;

bool critical_command_coordinator_init(
	const char *journal_directory, critical_apply_fn apply, void *context,
	unsigned int workers = CRITICAL_COORDINATOR_DEFAULT_WORKERS,
	critical_replay_observer_fn replay_observer = nullptr, void *replay_context = nullptr,
	critical_extension_validator_fn extension_validator = nullptr,
	critical_native_recovery_observer_fn native_replay_observer = nullptr,
	critical_native_recovery_publication_validator_fn native_publication_validator = nullptr,
	critical_native_birth_recovery_validators birth_validators = {},
	critical_native_recovery_pair_validator_fn quest_pair_validator = nullptr,
	critical_native_auction_recovery_validators auction_validators = {},
	critical_zone_reset_recovery_validators reset_validators = {},
	critical_shared_native_apply_fn shared_native_apply = nullptr,
	critical_zone_reset_item_apply_fn zone_reset_apply = nullptr,
	critical_extension_validator_bounded_fn extension_validator_bounded = nullptr,
	critical_native_recovery_observer_bounded_fn native_replay_observer_bounded = nullptr,
	critical_ordinary_native_apply_fn ordinary_native_apply = nullptr);
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
	// Complete original typed-birth submit from a const input reference.
	// Caller owns authentic input/current registry/journal/old output and
	// all first-observer profiles in outer, excluding ALL retained coordinator.
	// Every callback runs under the original actual coordinator unique_lock,
	// with fresh full CURRENT C added exactly once through its genuine lender.
	// Reserve must not reacquire coordinator/journal. Registered ROOT requires
	// its exact callback+active guard; closed birth charge is not that permit.
	// Full nullable family companions are required only for this new provider;
	// original SQL/inactive/ordinary unsupported-flat predicates stay intact.
	// Output is only a complete genuine same-lock snapshot after successful
	// census, never a lease, logical zero, admission or activation authority.
	// Actual selection and the ROOT/birth shared-scope join remain separate.
	static critical_submit_result submit_bounded(const critical_native_recovery_envelope &,
						     bool (*)(size_t, void *) noexcept, void *,
						     size_t outer,
						     size_t *current_coordinator_bytes) noexcept;
	static bool copy_context(const critical_command &,
				 critical_native_recovery_envelope *) noexcept;
	// Exact retained shared carrier plus generation from one mutex observation.
	// Synchronous reservation precedes all key/codec/copy allocations and runs
	// outside the coordinator mutex. Caller includes retained input/prior output.
	static bool copy_context_bounded(const critical_native_recovery_envelope &,
					 critical_native_recovery_envelope *, uint64_t *,
					 bool (*)(size_t, void *) noexcept, void *,
					 size_t) noexcept;

	// Ordinary birth-only complete post-submit observations. Caller outer owns
	// genuine retained input, old output, registry/journal and all noncoordinator
	// state. Reserve uses the exact registered ROOT callback/common guard and
	// never reacquires coordinator/journal locks. Fresh CURRENT C is lent under
	// the actual held mutex once per request; public outputs stay unchanged on
	// false. Original SQL/shared observations and ROOM authority are unchanged.
	// These snapshots grant no delivery, execution, publication, ACK or activation
	// authority. Complete first-admission/native frame qualification is separate.
	static bool observe_generation_bounded(const critical_native_recovery_envelope &,
					       uint64_t *, bool (*)(size_t, void *) noexcept,
					       void *, size_t outer,
					       size_t *current_coordinator_bytes) noexcept;
	static bool completion_bounded(const critical_operation_id &, critical_completion *,
				       bool (*)(size_t, void *) noexcept, void *, size_t outer,
				       size_t *current_coordinator_bytes) noexcept;
	static bool copy_context_ordinary_bounded(const critical_command &,
						  critical_native_recovery_envelope *,
						  bool (*)(size_t, void *) noexcept, void *,
						  size_t outer,
						  size_t *current_coordinator_bytes) noexcept;
	// Complete ordinary NMB4-only context CAS, physical ACK, delivered refusal
	// cleanup and original terminal transfer/retirement. Caller outer includes
	// all genuine inputs, prior outputs, registry/journal and external ROOT
	// storage; it excludes all retained coordinator storage. Every nested
	// reserve borrows fresh CURRENT C once under its actual mutex and the exact
	// registered ROOT callback/common guard. Original SQL/shared/ROOM unchanged.
	// Outside callbacks receive the full scratch prefix and a genuine relay
	// acquiring coordinator only for each budget request; they must use it for
	// all nested allocations and must not hold coordinator/journal themselves.
	// Cleanup markers/context survive owner destruction; false after a called
	// successful cleanup cannot authorize repeating that effect. Terminal
	// transfer is mandatory before durable retirement and fence release.
	// No callback/encode/allocation follows confirmed durable retirement.
	// Selection, native frame qualification and activation remain separate.
	// Fresh budget bridge for genuine outside publication/capture requests.
	// outer excludes all coordinator storage; actual mutex and original lender
	// add CURRENT C exactly once and authenticate the registered ROOT guard.
	// Callback must not acquire coordinator/journal; caller holds neither.
	// Admission provides no operation, delivery, publication or ACK authority.
	static bool reserve_ordinary_bounded(bool (*)(size_t, void *) noexcept, void *,
					     size_t exclusive_live) noexcept;
	static bool checkpoint_context_ordinary_bounded(const critical_native_recovery_envelope &,
							const critical_native_recovery_envelope &,
							bool (*)(size_t, void *) noexcept, void *,
							size_t outer,
							uint64_t expected_generation = 0) noexcept;
	static bool acknowledge_ordinary_bounded(const critical_native_recovery_envelope &,
						 const critical_completion &, uint64_t,
						 bool (*)(size_t, void *) noexcept, void *,
						 size_t outer) noexcept;
	static bool retire_ordinary_bounded(const critical_native_recovery_envelope &, uint64_t,
					    bool (*)(const critical_native_recovery_envelope &,
						     void *, bool (*)(size_t, void *) noexcept,
						     void *, size_t) noexcept,
					    void *transfer_context,
					    bool (*)(size_t, void *) noexcept, void *budget_context,
					    size_t outer) noexcept;
	static bool cancel_refusal_ordinary_bounded(
		const critical_native_recovery_envelope &, const critical_completion &, uint64_t,
		bool (*)(const critical_command &, const critical_completion &, void *,
			 bool (*)(size_t, void *) noexcept, void *, size_t) noexcept,
		void *cleanup_context, bool (*)(size_t, void *) noexcept, void *budget_context,
		size_t outer, bool *cleanup_called, bool *cleanup_succeeded) noexcept;
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

// Only the actual reset world owner can cross type22 native publication.
// Full current SQL/world proof is independently required; these methods only
// authenticate original coordinator delivery, exact context CAS and lifetime.
class critical_zone_reset_item_publication_owner final
{
	friend class zone_reset_room_publication_owner;
	static critical_submit_result submit(critical_native_recovery_envelope);
	// Complete original ROOM admission with a genuinely owned admitted clone.
	// outer includes authentic caller input/context/output once, excludes ALL
	// coordinator storage. Every reserve runs under the original coordinator
	// mutex with its complete fresh current census added exactly once; it must
	// not acquire coordinator/journal locks. Output is a genuine current snapshot,
	// never a lease, and is unchanged until an actual census succeeds. The full
	// ROOT/journal aggregate handoff and qualified selector remain separate.
	static critical_submit_result submit_bounded(const critical_native_recovery_envelope &,
						     bool (*)(size_t, void *) noexcept, void *,
						     size_t outer_live,
						     size_t *current_coordinator_bytes) noexcept;
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
	// Keep actual room/item advancement fences until original terminal service
	// evidence transfers durably and the exact continuation journal retires.
	static bool retire(const critical_native_recovery_envelope &, uint64_t,
			   bool (*)(const critical_native_recovery_envelope &, void *) noexcept,
			   void *) noexcept;
	// Same actual original generation/operation/fence/uncertainty ownership.
	// Callback receives coordinator's whole simultaneous scratch prefix while
	// the genuine operation is pinned. Both callbacks must avoid acquiring
	// coordinator/journal mutexes; they may run under either original lock.
	// Persistent journal storage is separately counted once by the aggregate.
	// No allocation/encode/budget callback follows successful journal retirement.
	static bool retire_bounded(const critical_native_recovery_envelope &, uint64_t,
				   bool (*)(const critical_native_recovery_envelope &, void *,
					    size_t) noexcept,
				   void *transfer_context, bool (*)(size_t, void *) noexcept,
				   void *budget_context, size_t outer_live) noexcept;
	// Complete original same-phase context CAS. Full prepared clone and pure
	// callbacks precede the pinned journal rewrite; no allocating work follows
	// durable success. Callback/outer/persistent-storage contract matches retire.
	static bool checkpoint_context_bounded(const critical_native_recovery_envelope &,
					       const critical_native_recovery_envelope &,
					       bool (*)(size_t, void *) noexcept, void *,
					       size_t outer_live,
					       uint64_t expected_generation = 0) noexcept;
	// Original receipt/delivery-authenticated execution->continuation ACK. Both
	// full envelope clones and domain proofs precede the pinned durable CAS;
	// original ROOM fences survive physical ACK until terminal retirement.
	static bool acknowledge_bounded(const critical_native_recovery_envelope &,
					const critical_completion &, uint64_t,
					bool (*)(size_t, void *) noexcept, void *,
					size_t outer_live) noexcept;
	// Full current native carrier copy and original generation observation.
	// Strong outputs, complete original command/BODY proof and actual fresh
	// clone/key/codec storage; no new delivery/source/ACK authority.
	static bool copy_context_bounded(const critical_command &,
					 critical_native_recovery_envelope *,
					 bool (*)(size_t, void *) noexcept, void *,
					 size_t outer_live) noexcept;
	static bool observe_generation_bounded(const critical_native_recovery_envelope &,
					       uint64_t *, bool (*)(size_t, void *) noexcept,
					       void *, size_t outer_live) noexcept;
	// Strong passive snapshot of actual retained coordinator objects/capacities
	// under its original mutex; includes safe immutable flat-owner charges.
	// Snapshot is not a lease and does not cover external worker-local transient
	// workspaces. Never acquire this mutex from a held coordinator/journal callback.
	static bool current_storage_bytes(size_t *) noexcept;
	// ROOM-only complete support proof through the bounded callback paired with
	// the actually selected original admission callback. Pure same-lock proof;
	// no admission, source, delivery, execution or activation authority.
	static bool admission_supported_bounded(const critical_command &,
						bool (*)(size_t, void *) noexcept, void *,
						size_t outer_live) noexcept;
	// Original same-lock pending-publication/completed-cache receipt lookup.
	// Actual binary-key string/lock storage is admitted before construction;
	// fixed caller-owned output belongs to outer and remains unchanged on false.
	// No extra health/generation/type/receipt gate or delivery/ACK authority.
	static bool completion_bounded(const critical_operation_id &, critical_completion *,
				       bool (*)(size_t, void *) noexcept, void *,
				       size_t outer_live) noexcept;
	// Complete original delivered never-admitted cleanup and fence removal.
	// Cleanup runs outside the coordinator mutex with its operation pinned and
	// the complete live prefix; reserve may run under the original mutex and
	// must not acquire coordinator/journal locks. Context, markers and caller
	// prefix must survive the genuine cleanup's destruction of its native owner.
	// Markers precede later lock/rechecks; false after successful cleanup must
	// never authorize repeating that native effect. No journal rewrite or ACK.
	static bool cancel_refusal_bounded(const critical_native_recovery_envelope &,
					   const critical_completion &, uint64_t,
					   bool (*)(const critical_command &,
						    const critical_completion &, void *,
						    size_t) noexcept,
					   void *cleanup_context, bool (*)(size_t, void *) noexcept,
					   void *budget_context, size_t outer_live,
					   bool *cleanup_called, bool *cleanup_succeeded) noexcept;
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

class player_save_prepared_startup_lifecycle_owner;
// Minted only by the real coordinator initializer while its actual unique_lock
// and earlier-acquired lifecycle owner remain live. Prospective helpers below
// are unselected: ROOT still owns initial frame/registry/journal admission and
// all complete bounded family joins. No public claimed-baseline DTO exists.
class critical_mixed_startup_replay_owner final
{
    private:
	friend class critical_gameplay_startup_owner;
	friend bool critical_command_coordinator_init(
		const char *journal_directory_path, critical_apply_fn apply, void *context,
		unsigned int worker_count, critical_replay_observer_fn replay_observer,
		void *replay_context, critical_extension_validator_fn extension_validator,
		critical_native_recovery_observer_fn native_replay_observer,
		critical_native_recovery_publication_validator_fn native_publication_validator,
		critical_native_birth_recovery_validators birth_validators,
		critical_native_recovery_pair_validator_fn quest_pair_validator,
		critical_native_auction_recovery_validators auction_validators,
		critical_zone_reset_recovery_validators reset_validators,
		critical_shared_native_apply_fn shared_native_apply,
		critical_zone_reset_item_apply_fn zone_reset_apply,
		critical_extension_validator_bounded_fn extension_validator_bounded,
		critical_native_recovery_observer_bounded_fn native_replay_observer_bounded,
		critical_ordinary_native_apply_fn ordinary_native_apply);

	static player_save_prepared_startup_lifecycle_owner acquire_lifecycle();
	critical_mixed_startup_replay_owner(
		const std::unique_lock<std::mutex> &,
		const player_save_prepared_startup_lifecycle_owner &) noexcept;
	~critical_mixed_startup_replay_owner() noexcept;
	critical_mixed_startup_replay_owner(const critical_mixed_startup_replay_owner &) = delete;
	critical_mixed_startup_replay_owner &
	operator=(const critical_mixed_startup_replay_owner &) = delete;
	static const critical_mixed_startup_replay_owner *current() noexcept;
	bool held() const noexcept;
	// Additional real caller/observer profiles. ROOT already owns the original
	// init lock/locals, full CURRENT-C observation/lender profile, journal and
	// every foreign registry/caller profile BEFORE the first retained query.
	static size_t additional_frame_bytes() noexcept;
	// Genuine coordinator-only bridge; full pipeline/currency receivers live
	// in comm.c so minimal original coordinator links gain no new providers.
	bool reserve_coordinator_cut(bool (*)(size_t, void *) noexcept, void *,
				     size_t) const noexcept;
	bool reserve_current_cut(bool (*)(size_t, void *) noexcept, void *, size_t) const noexcept;
	bool restore_currency(const critical_command &, bool (*)(size_t, void *) noexcept, void *,
			      size_t) const noexcept;
	struct budget_bridge
	{
		const critical_mixed_startup_replay_owner &owner;
		bool (*reserve)(size_t, void *) noexcept;
		void *context;
		static bool relay(size_t, void *) noexcept;
	};
	const std::unique_lock<std::mutex> &init_lock_;
	const player_save_prepared_startup_lifecycle_owner &lifecycle_;
};

#endif
