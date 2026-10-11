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
	// Complete historical/ordinary validity counterparts for replay; appended
	// after old fields to preserve all original aggregate initialization.
	bool (*valid_bounded)(const critical_native_recovery_envelope &,
			      bool (*)(size_t, void *) noexcept, void *, size_t) noexcept = nullptr;
	bool (*cash_role_valid_bounded)(const critical_native_recovery_envelope &,
					bool (*)(size_t, void *) noexcept, void *,
					size_t) noexcept = nullptr;
	// Additional complete initial companions. Original first nine fields,
	// appended two valid fields, and historical readiness/defaults stay intact.
	// The new full provider proves exact target identity before fixed profiles.
	bool (*historical_initial_fixed_bounded)(const critical_native_recovery_envelope &,
						 bool (*)(size_t, void *) noexcept, void *,
						 size_t) noexcept = nullptr;
	bool (*cash_role_initial_fixed_bounded)(const critical_native_recovery_envelope &,
						bool (*)(size_t, void *) noexcept, void *,
						size_t) noexcept = nullptr;
	bool (*shared_shop_initial_fixed_bounded)(const critical_native_recovery_envelope &,
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
	// Full original NAR validity; optional companion, no readiness promotion.
	bool (*valid_bounded)(const critical_native_recovery_envelope &,
			      bool (*)(size_t, void *) noexcept, void *, size_t) noexcept = nullptr;
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

	// Exact room_submit_workspace construction and empty unlocked early cleanup.
	// Caller must transiently preadmit this SOURCE + actual INITIAL before entry;
	// submit self-retains SOURCE before its first callback. No later populated
	// cleanup, held-lock body, validator or whole-submit profile is granted.
	// Both pure outputs are strong on null/unsupported refusal.
	static bool submit_workspace_entry_source_frames(size_t *) noexcept;
	static bool submit_workspace_initial_inline_bytes(size_t *) noexcept;
	static constexpr size_t submit_workspace_entry_source_query_frames() noexcept
	{
		// Direct pure output parameter and declared bool return; all predicates
		// and assignments use actual fixed constants, with no child getter call.
		// Returned accessor size_t remains a distinct caller carrier.
		return sizeof(void *) + sizeof(bool);
	}

	// Exact private room_submit_local_prefix Source, excluding physical heaps,
	// persistent C and lender/callback descendants. Caller preadmits the pure
	// query before accessing it; submit retains the named prefix Source in outer
	// before the first callback and every later recensus. Output is strong.
	static bool submit_private_current_source_frames(size_t *) noexcept;
	static constexpr size_t submit_private_current_source_query_frames() noexcept
	{
		// Direct output parameter and bool result; no dynamic child query/errno.
		return sizeof(void *) + sizeof(bool);
	}

	// Complete OWNED native submit retained Source, actual workspace INITIAL,
	// and genuinely zero caller-retained OWNED supplement. Strong pure outputs.
	// Caller preadmits this query, then full owned Source+INITIAL BEFORE entry;
	// real outer excludes that prospective entry because submit retains its
	// checked Source/components and actual private workspace internally once.
	// This does not price external extension/ROOT callbacks or runtime bodies:
	// their matched complete contracts remain selecting caller dependencies.
	static bool submit_owned_source_frames(size_t *) noexcept;
	static bool submit_owned_initial_inline_bytes(size_t *) noexcept;
	static bool submit_owned_source_supplement_frames(size_t *) noexcept;
	static constexpr size_t submit_owned_source_query_frames() noexcept
	{
		// Source getter: outputP/totalN/partN/resultB plus max(actual child
		// query17, checked add17) =42. Supplement owns outputP/fullN/resultB
		// while that getter runs: 17+42=59. INITIAL delegation18 is smaller.
		// This accessor's returned N is separately selecting-caller-owned.
		return (sizeof(void *) + sizeof(size_t) + sizeof(bool)) +
		       (sizeof(void *) + 2 * sizeof(size_t) + sizeof(bool)) +
		       (sizeof(void *) + sizeof(size_t) + sizeof(bool));
	}

	// Exact submit lock/unlock gthread headers and pthread declarations only.
	// Unique/mutex receiver/error/controllers already remain owned separately.
	// External pthread/system-error implementations are not supplied here.
	static bool submit_lock_gthread_header_source_frames(size_t *) noexcept;
	static constexpr size_t submit_lock_gthread_header_source_query_frames() noexcept
	{
		return sizeof(void *) + sizeof(bool);
	}

	// Exact lender lock/scalar-loan/thread header Source only. Registered ROOT
	// reserve callback and external pthread implementation remain caller-owned.
	// Strong supported output; callback carrier/current heap already owned by
	// the genuine existing lender and caller. Pure fixed query P+B, no errno.
	static bool submit_lender_header_source_frames(size_t *) noexcept;
	static constexpr size_t submit_lender_header_source_query_frames() noexcept
	{
		return sizeof(void *) + sizeof(bool);
	}

	// Exact published critical valid/envelope/legacy allocation-free Source,
	// including the original runtime getter return N once. Strong supported
	// output; input/current/callback/heap ownership remains the actual caller's.
	// This getter query owns output P/result B plus runtime child return N;
	// query preentry is transient, separate from the returned original allowance.
	static bool submit_command_validation_source_frames(size_t *) noexcept;
	static constexpr size_t submit_command_validation_source_query_frames() noexcept
	{
		return sizeof(void *) + sizeof(bool) + sizeof(size_t);
	}

	// Exact admitted-command diagnostic header/atomic/gthread Source only.
	// Original two event objects/lock and retained depth chrono headers own their
	// physical/source terms once. This is not an all-diagnostics/runtime profile.
	// Caller preadmits query and Source; submit retains Source before callbacks.
	static bool submit_admitted_trace_source_frames(size_t *) noexcept;
	static constexpr size_t submit_admitted_trace_source_query_frames() noexcept
	{
		return sizeof(void *) + sizeof(bool);
	}

	// Exact SAME-TU whole pure CURRENT-C census Source/profile, with identical
	// supported-mode/width/iterator guard and strong output to the mixed owner.
	// This does not query retained C or establish a loan. Caller preentry query
	// is transient; submit retains Source before its first same-lock lender scan.
	// Lender/ROOT/G/J callback Source and native/runtime qualification stay separate.
	static bool submit_coordinator_current_source_frames(size_t *) noexcept;
	static constexpr size_t submit_coordinator_current_source_query_frames() noexcept
	{
		return sizeof(void *) + sizeof(bool);
	}

	// Exact enclosing admit/relay/return/rollback and completed-cache wrapper
	// source, plus existing classifier/update-depth and direct lock headers.
	// This is a retained supplement, not complete lender/validator/diagnostic/
	// runtime Source. Caller must preadmit before workspace construction; the
	// submit retains it before the first budget call. Strong pure query9.
	static bool submit_header_controller_source_frames(size_t *) noexcept;
	static constexpr size_t submit_header_controller_source_query_frames() noexcept
	{
		return sizeof(void *) + sizeof(bool);
	}

	// Actual same-overload table/key/queue/rollback descendant Source. The
	// clone profile's named shared string/state value lifetime stays retained
	// once. Caller preentry is transient; submit owns this supplement before
	// its first callback. Enclosing submit/lock/return/trace/notify, completed
	// cache wrappers, selected callbacks and external runtime remain separate.
	static bool submit_mutation_child_source_frames(size_t *) noexcept;
	static constexpr size_t submit_mutation_child_source_query_frames() noexcept
	{
		return sizeof(void *) + sizeof(bool);
	}

	// Exact direct envelope-copy/state/context construction and true moves,
	// plus the same actual populated cleanup descendants. Caller transiently
	// preadmits this Source before entry; submit retains its phase maximum in
	// outer. Existing real private CURRENT/heap requests are not duplicated.
	// Allocation-runtime/table/queue/trace/callback Source remains separate.
	static bool submit_clone_state_source_frames(size_t *) noexcept;
	static constexpr size_t submit_clone_state_source_query_frames() noexcept
	{
		return sizeof(void *) + sizeof(bool);
	}

	// Exact full duplicate native-state + two canonical command encodes.
	// Strong pure query outputs; caller input/C/current and old output stay
	// external. Full Source+actual two-vector INITIAL preentry is transient;
	// the genuine helper retains own Source/capacities and each codec's
	// missing supplement internally. Does not qualify whole submit/host.
	static bool submit_matches_source_frames(size_t *) noexcept;
	static bool submit_matches_initial_inline_bytes(size_t *) noexcept;
	static constexpr size_t submit_matches_source_query_frames() noexcept
	{
		// Public wrapper P+B; private output/codec/total/bool P+2N+B;
		// actual complete codec getter P+B; checked add P+N+B.
		// Accessor size_t result is a separate caller local/carrier.
		return 4 * sizeof(void *) + 3 * sizeof(size_t) + 4 * sizeof(bool);
	}

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

	// Exact matching NONborrowed original observer local/header Source only:
	// reviewed private CURRENT census plus original lock_guard/mutex/gthread
	// and declared pthread calls. Caller retains full returned local/header
	// Source; observer admits none. INITIAL is actual local lock_guard only.
	// Do not call the locking observer under the coordinator loan; borrowed
	// path retains its parent's same-lock scan Source instead, exactly once.
	// External pthread/system-error bodies remain caller/Root qualification.
    public:
	// Passive size queries only; no observation, admission or execution authority.
	static bool current_storage_header_source_frames(size_t *) noexcept;
	static bool current_storage_initial_inline_bytes(size_t *) noexcept;
	static constexpr size_t current_storage_source_query_frames() noexcept
	{
		return sizeof(void *) + sizeof(bool);
	}

    private:
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

class player_save_coin_replay_budget_scope_owner;
class player_save_prepared_startup_lifecycle_owner;
// Minted only by the real coordinator initializer while its actual unique_lock
// and earlier-acquired lifecycle owner remain live. Prospective helpers below
// are unselected: ROOT still owns initial frame/registry/journal admission and
// all complete bounded family joins. No public claimed-baseline DTO exists.
class critical_mixed_startup_replay_owner final
{
    private:
	friend class zone_reset_item_owner;
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
	// Concrete lexical host: this actual guard surrounds the entire startup
	// callback, including every complete C-loan invocation, and dies afterward.
	static size_t physical_startup_frame_bytes() noexcept;
	bool run_physical_startup(bool (*)(size_t, void *) noexcept, void *, size_t,
				  bool (*)(bool (*)(size_t, void *) noexcept, void *, size_t,
					   void *) noexcept,
				  void *) const noexcept;
	// Selecting ROOT preadmits this additional typed closure and the authentic
	// CURRENT-C profile before the first scan, and retains its actual G guard.
	static size_t coordinator_operation_frame_bytes() noexcept;
	bool invoke_coordinator_operation(bool (*)(size_t, void *) noexcept, void *, size_t,
					  bool (*)(bool (*)(size_t, void *) noexcept, void *,
						   size_t, void *) noexcept,
					  void *) const noexcept;
	// Genuine additional match/forwarding source only; not full enqueue/init
	// SOURCE. ROOT transiently admits this query, then retains the returned
	// supplement once through real replay work and observers. Null/unsupported
	// output is strong; no observation, registration or authority follows.
	// Full original native-envelope retained-size helper's disjoint source
	// and actual local encoded-vector INLINE. These pure strong queries do
	// not scan C/J/G or grant startup/transport/execution authority.
	// Complete pure same-lock CURRENT-C census SOURCE. ROOT must preadmit
	// this before every genuine locked observation; actual persistent C bytes,
	// the lock, ROOT/J and callback/loan source remain separately owned.
	// No local owning workspace exists: all census scalars/iterators are SOURCE.
	// Null/unsupported output is strong, no errno, observation or authority.
	// Complete actual private prefix and fresh native-copy request scans.
	// Existing workspace/old input/actual heap remain separately owned. These
	// pure fixed queries grant no admission/lock/observer authority. ROOT
	// retains the selected source before each real scan; no workspace-size
	// shortcut supplies constructor/mutation/rollback source.
	// Actual fresh default state/context make_unique + null-destination transfer.
	// Object/returned unique INLINE and requested heaps remain original owners;
	// callee admits these Source windows before the real constructors.
	// Actual fresh const-copy member assignments and legacy true-allocator move.
	// Sources are admitted at the original prospective/move cuts internally;
	// old inputs, requested/capacity heaps and state INLINE remain separate.
	static bool replay_command_copy_source_frames(size_t *) noexcept;
	static bool replay_command_move_source_frames(size_t *) noexcept;
	static bool replay_attachment_copy_source_frames(size_t *) noexcept;
	static constexpr size_t replay_command_assignment_source_query_frames() noexcept
	{
		return sizeof(void *) + sizeof(bool);
	}
	// Actual default one-node deque construction and its allocation-failure cleanup.
	// Source augments the original table admission; initial heap request stays separate.
	static bool replay_fence_queue_construction_source_frames(size_t *) noexcept;
	static constexpr size_t replay_fence_queue_construction_source_query_frames() noexcept
	{
		return sizeof(void *) + sizeof(bool);
	}
	// Actual untouched native table forecast, conditionally paired with authentic
	// GNU13.3 release Prime-policy Source; runtime-library binding remains required.
	// Retained once in private replay outer before the first existing callback.
	static bool replay_table_forecast_source_frames(size_t *) noexcept;
	static constexpr size_t replay_table_forecast_source_query_frames() noexcept
	{
		return sizeof(void *) + sizeof(bool);
	}
	// Exact char-allocator value copy/failure and completed destruction window.
	// Value objects and current/fresh char heaps remain existing container owners.
	// Populated state/private reset and operation-pair cleanup, with all three
	// actual transaction implementations. Cleanup members are sequential.
	static bool replay_operation_state_cleanup_source_frames(size_t *) noexcept;
	// Internally retained maximum of completed value-copy and rollback cleanup;
	// do not sum with the overlapping string-value/cleanup family getters.
	static bool replay_value_lifetime_source_frames(size_t *) noexcept;
	static constexpr size_t replay_value_lifetime_source_query_frames() noexcept
	{
		return sizeof(void *) + sizeof(bool);
	}
	// Complete original queue peak forecast and const-value push controller.
	// Excludes the string ctor already in retained value-lifetime window.
	// Actual key pointer/count construction, generated array return and true
	// allocator string move; already dominated by retained value-lifetime.
	static bool replay_key_value_source_frames(size_t *) noexcept;
	// Populated deque destruction CONTROLLERS, excluding already-retained
	// string element cleanup Source. Push/controller max is internally held.
	static bool replay_queue_cleanup_source_frames(size_t *) noexcept;
	static constexpr size_t replay_key_queue_source_query_frames() noexcept
	{
		return sizeof(void *) + sizeof(bool);
	}
	// Standard string-key find/at and actual two selected hash/comparator lanes.
	// GCC13.3 external hash source is conditional on ROOT runtime binding.
	static bool replay_table_key_lookup_source_frames(size_t *) noexcept;
	static constexpr size_t replay_table_key_lookup_source_query_frames() noexcept
	{
		return sizeof(void *) + sizeof(bool);
	}
	// Actual node/bucket storage primitives, value construction/destruction excluded.
	// Companion retaining pair/tuple/unique/deque child profiles is still required.
	static bool replay_table_node_storage_source_frames(size_t *) noexcept;
	static constexpr size_t replay_table_node_storage_source_query_frames() noexcept
	{
		return sizeof(void *) + sizeof(bool);
	}
	// Actual operation pair / fence piecewise tuple value controllers.
	// Named member constructors reuse the already-retained value lifetime bound.
	static bool replay_table_value_construction_source_frames(size_t *) noexcept;
	static constexpr size_t replay_table_value_construction_source_query_frames() noexcept
	{
		return sizeof(void *) + sizeof(bool);
	}
	// Actual table emplacement/subscript/insertion/rehash own controller source.
	// Retains the separately named node/value/lookup/Prime children once.
	static bool replay_table_insert_source_frames(size_t *) noexcept;
	static constexpr size_t replay_table_insert_source_query_frames() noexcept
	{
		return sizeof(void *) + sizeof(bool);
	}
	// Direct inherited Hashtable iterator-erase overloads and actual unlink controllers.
	// Node/member teardown remains in separately retained genuine child profiles.
	static bool replay_table_iterator_erase_source_frames(size_t *) noexcept;
	static constexpr size_t replay_table_iterator_erase_source_query_frames() noexcept
	{
		return sizeof(void *) + sizeof(bool);
	}
	// Transparent rollback view-key lookup uncovered supplement only.
	// Original lookup source must remain owned once alongside this supplement.
	static bool replay_transparent_lookup_source_supplement_frames(size_t *) noexcept;
	// Full graph query is transient; never retain it on top of original lookup.
	static bool replay_transparent_lookup_full_source_frames(size_t *) noexcept;
	static constexpr size_t replay_transparent_lookup_source_query_frames() noexcept
	{
		return sizeof(void *) + sizeof(bool);
	}
	// Original erase(remove(begin,end,id),end) tail-only controllers.
	// Named queue destruction children remain separately retained once.
	static bool replay_deque_tail_erase_source_supplement_frames(size_t *) noexcept;
	// Full tail graph query is transient; retain only its disjoint supplement.
	static bool replay_deque_tail_erase_full_source_frames(size_t *) noexcept;
	static constexpr size_t replay_deque_tail_erase_source_query_frames() noexcept
	{
		return sizeof(void *) + sizeof(bool);
	}
	// Original std::remove iterator/predicate/controller supplement only.
	// Named actual table equality and string move children remain owned once.
	// Actual remove_fences and legacy/native rollback caller controllers only.
	// Existing named lookup/value/queue/remove/tail children remain owned once;
	// update_depth clock/thread Source is a distinct pending child.
	static bool replay_rollback_controller_source_supplement_frames(size_t *) noexcept;
	static constexpr size_t replay_rollback_controller_source_query_frames() noexcept
	{
		return sizeof(void *) + sizeof(bool);
	}
	// Actual update_depth table/unique/phase/chrono-header/max/thread source only.
	// Out-of-line steady_clock::now remains an explicit separate external leaf;
	// this own-header supplement does not claim a complete clock/host profile.
	static bool replay_update_depth_header_source_supplement_frames(size_t *) noexcept;
	static constexpr size_t replay_update_depth_header_source_query_frames() noexcept
	{
		return sizeof(void *) + sizeof(bool);
	}
	// Actual legacy/native workspace default-member construction Source.
	// Caller preadmits Source+actual initial INLINE before entry; both
	// entries retain Source once. Published/default behavior is unchanged.
	static bool replay_workspace_entry_source_frames(size_t *) noexcept;
	static bool replay_workspace_initial_inline_bytes(size_t *) noexcept;
	static constexpr size_t replay_workspace_entry_source_query_frames() noexcept
	{
		return sizeof(void *) + sizeof(bool);
	}
	// Complete OWNED legacy/native enqueue graph: enclosing/current/relay,
	// state/default/copy/move/table/key/queue/populated rollback and header clock.
	// External callback providers, CURRENT C and steady_clock implementation
	// are separate actual selecting-owner dependencies, never included by guess.
	// Caller transiently preadmits owned Source+actual initial INLINE before
	// entry, then excludes them from outer: each new entry self-retains its
	// persistent scopes and precise original transient construction requests.
	// Supplement is the disjoint advance over immutable89a, also internally
	// retained. It must not be added again by the selecting caller.
	static bool replay_enqueue_owned_source_frames(size_t *) noexcept;
	static bool replay_enqueue_owned_source_supplement_frames(size_t *) noexcept;
	static bool replay_enqueue_initial_inline_bytes(size_t *) noexcept;
	static constexpr size_t replay_enqueue_owned_source_query_frames() noexcept
	{
		// Public output/bool; total local; genuine lower size-return;
		// checked_add(ref,amount,bool). Initial/retained fixed queries are smaller.
		// Accessor's own size_t result remains a separate caller carrier.
		return 2 * sizeof(void *) + 3 * sizeof(size_t) + 2 * sizeof(bool);
	}
	static bool replay_std_remove_source_supplement_frames(size_t *) noexcept;
	// Full graph query is transient; only disjoint own supplement stays retained.
	static bool replay_std_remove_full_source_frames(size_t *) noexcept;
	static constexpr size_t replay_std_remove_source_query_frames() noexcept
	{
		return sizeof(void *) + sizeof(bool);
	}
	static bool replay_queue_push_source_frames(size_t *) noexcept;
	static constexpr size_t replay_queue_push_source_query_frames() noexcept
	{
		return sizeof(void *) + sizeof(bool);
	}
	static bool replay_string_value_source_frames(size_t *) noexcept;
	static constexpr size_t replay_string_value_source_query_frames() noexcept
	{
		return sizeof(void *) + sizeof(bool);
	}
	static bool replay_state_construction_source_frames(size_t *) noexcept;
	static bool replay_native_context_construction_source_frames(size_t *) noexcept;
	static constexpr size_t replay_state_construction_source_query_frames() noexcept
	{
		return sizeof(void *) + sizeof(bool);
	}
	static bool replay_private_current_source_frames(size_t *) noexcept;
	static bool replay_fresh_copy_request_source_frames(size_t *) noexcept;
	static constexpr size_t replay_private_current_source_query_frames() noexcept
	{
		return sizeof(void *) + sizeof(bool);
	}
	static bool coordinator_current_source_frames(size_t *) noexcept;
	static constexpr size_t coordinator_current_source_query_frames() noexcept
	{
		// Direct constant getter output pointer/bool. Accessor return N is
		// separately owned by the selecting caller. No descendant queries.
		return sizeof(void *) + sizeof(bool);
	}
	static bool native_checkpoint_size_source_frames(size_t *) noexcept;
	static bool native_checkpoint_size_initial_inline_bytes(size_t *) noexcept;
	static constexpr size_t native_checkpoint_size_source_query_frames() noexcept
	{
		// Larger genuine initial query: export and internal output/bool,
		// two named size_t locals, lower query, checked-add. Accessor result
		// size_t remains a separate selecting caller carrier. No errno write.
		return 2 * (sizeof(void *) + sizeof(bool)) + 2 * sizeof(size_t) +
		       critical_command_startup_codec_complete_source_query_frame_bytes() +
		       sizeof(void *) + sizeof(size_t) + sizeof(bool);
	}
	static bool replay_forwarding_source_supplement(size_t *) noexcept;
	static constexpr size_t replay_forwarding_source_query_frames() noexcept
	{
		// Query output pointer and bool result, plus the actual returned int *
		// from __errno_location() on null/unsupported refusal. The constexpr
		// accessor's size_t result remains separately caller-owned.
		return 2 * sizeof(void *) + sizeof(bool);
	}
	bool coordinator_operation_callback_matches(bool (*)(size_t, void *) noexcept,
						    void *) const noexcept;
	// Additional receiver closure; selecting ROOT separately owns the complete
	// authentic held/thread/pipeline/currency profiles before constructing scope.
	static size_t borrowed_currency_receiver_frame_bytes() noexcept;
	bool restore_currency_borrowed(const critical_command &, bool (*)(size_t, void *) noexcept,
				       void *, size_t) const noexcept;
	// Actual private SAME-scope receivers for the original mixed dispatcher.
	// Only named original providers are admitted, under the genuine whole-C
	// operation callback. Root owns complete preentry/receiver SOURCE before
	// invoking either; these declarations do not supply a first-call budget.
	using legacy_pipeline_restore_fn = bool (*)(const critical_command &,
						    player_save_coin_replay_budget_scope_owner &,
						    bool (*)(size_t, void *) noexcept, void *,
						    size_t) noexcept;
	using native_pipeline_restore_fn = bool (*)(const critical_native_recovery_envelope &,
						    player_save_coin_replay_budget_scope_owner &,
						    bool (*)(size_t, void *) noexcept, void *,
						    size_t) noexcept;
	bool restore_legacy_pipeline_borrowed(const critical_command &, legacy_pipeline_restore_fn,
					      bool (*)(size_t, void *) noexcept, void *,
					      size_t) const noexcept;
	bool restore_native_pipeline_borrowed(const critical_native_recovery_envelope &,
					      native_pipeline_restore_fn,
					      bool (*)(size_t, void *) noexcept, void *,
					      size_t) const noexcept;
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
