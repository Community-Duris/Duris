#ifndef ITEM_MOVEMENT_TRANSACTION_H
#define ITEM_MOVEMENT_TRANSACTION_H
#include "combat/chaos_pouch_types.h"
#include "item/craft_recipe_continuation.h"

#include "persistence/critical_command_coordinator.h"
#include "item/item_transfer_command.h"
#include "economy/economic_accounting_plan.h"
#include "core/structs.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

enum class item_creation_prepare_result
{
	more,
	ready,
	failed
};
// Each invocation creates at most one detached root. Captures must own their
// data and must not retain a character pointer across pulses.
using item_creation_prepare_fn = std::function<item_creation_prepare_result(P_char, P_obj *)>;
using item_creation_grant_completion_fn = void (*)(P_char actor, uint64_t item_uid, bool committed,
						   unsigned int error_code);
bool item_creation_grant_defer(P_char actor, item_creation_prepare_fn prepare,
			       economic_source_kind source = {}, uint64_t source_id = 0);
void item_creation_grant_prepare_pulse(void);

constexpr size_t ITEM_MOVEMENT_PENDING_MAX = 1024;
// Durable quest publication captures a bounded group-credit snapshot here.
constexpr size_t ITEM_MOVEMENT_CONTEXT_MAX_BYTES = 768;
// Creation batches are held in the same bounded admission queue as movement
// transactions, so admission must never promise more roots than submission can carry.
constexpr size_t ITEM_CREATION_GRANT_MAX_ROOTS =
	ITEM_MOVEMENT_PENDING_MAX < ITEM_TRANSFER_MAX_ITEMS ? ITEM_MOVEMENT_PENDING_MAX :
							      ITEM_TRANSFER_MAX_ITEMS;
static_assert(ITEM_CREATION_GRANT_MAX_ROOTS <= ITEM_TRANSFER_MAX_ITEMS);

using item_movement_completion_fn = void (*)(P_char actor, bool committed,
					     const item_transfer_result &result,
					     unsigned int error_code, const uint8_t *context,
					     size_t context_size);
// Opt-in callbacks are the publication boundary: returning false retains the
// movement entry and the coordinator's entity fences for a later attempt. The
// operation ID is borrowed for this call; copy it before retaining it.
using item_movement_publication_fn = bool (*)(const critical_operation_id &operation_id,
					      P_char actor, bool committed,
					      const item_transfer_result &result,
					      unsigned int error_code, const uint8_t *context,
					      size_t context_size);
constexpr unsigned int ITEM_MOVEMENT_PUBLICATION_MAX_ATTEMPTS = 8;

// A submission can be refused for reasons that are operationally very different: a
// transient conflict the player should simply retry, versus ledger state that disagrees
// with live topology and will never resolve on its own. Callers map this onto both the
// player-facing text and the structured diagnostic, so the two classes stay separable.
enum class item_movement_reject
{
	none,
	invalid_request,
	queue_saturated,
	pending_conflict,
	active_accounting_unsupported,
	missing_owner_identity,
	owner_mismatch,
	missing_owner_revision,
	topology_mismatch,
	snapshot_failure,
	allocation_failure,
	command_build_failure,
	coordinator_unavailable,
	coordinator_overloaded,
	coordinator_invalid,
	coordinator_identity_conflict,
	coordinator_journal_failure,
	coordinator_journal_uncertain,
	coordinator_rejected,
};

const char *item_movement_reject_name(item_movement_reject reason);
bool item_movement_reject_is_transient(item_movement_reject reason);

// SQL schema-2 ordinary single-root drop only. Preparation owns identities and
// the literal checkpoint, never a character/object pointer across pulses. The
// completion is notification-only, after physical publication, ACK and release.
bool item_movement_transaction_prepare_sql_drop(P_char actor, P_obj root,
						item_movement_completion_fn completion,
						const void *context, size_t context_size,
						item_movement_reject *reject);
bool item_movement_transaction_prepare_sql_lockpick_retirement(P_char, P_obj,
							       const item_transfer_continuation &,
							       item_movement_publication_fn,
							       item_movement_reject *);
bool item_movement_transaction_restore_held_retirement_recovery(
	const critical_native_recovery_envelope &) noexcept;
void item_movement_transaction_drop_prepare_pulse(void);
// Lifecycle cancellation of unadmitted preparations only. Their ordinary save
// bodies remain owned by the pipeline/worker; admitted original holds are untouched.
void item_movement_transaction_cancel_drop_preparations(void);

struct item_movement_health
{
	uint64_t pending;
	uint64_t retained_offline;
	uint64_t submitted;
	uint64_t committed;
	uint64_t rejected;
	uint64_t submission_failures;
	uint64_t stale_publications;
	uint64_t publication_retrying;
	uint64_t publication_blocked;
	uint64_t publication_ack_pending;
	uint64_t publication_owner_waiting;
};

bool item_movement_transaction_submit(
	P_char actor, P_obj root, P_obj target_container, const item_owner_identity &from_owner,
	const item_owner_identity &to_owner, item_transfer_reason reason, int64_t reason_id,
	item_movement_completion_fn completion, const void *context, size_t context_size,
	P_obj corpse_context = NULL, item_movement_reject *reject = NULL,
	item_movement_publication_fn publication = nullptr,
	economic_source_kind lifecycle_source = {}, uint64_t logical_source_id = 0,
	const item_transfer_continuation &continuation = {});
// A corpse_create batch validates and publishes all captured live roots before
// invoking completion. Its callback persists/finalizes the corpse, not the moves.
// Stale topology retains the movement and busy fence without calling completion.
bool item_movement_transaction_submit_batch(
	P_char actor, P_obj const *roots, size_t root_count, P_obj target_container,
	const item_owner_identity &from_owner, const item_owner_identity &to_owner,
	item_transfer_reason reason, int64_t reason_id, item_movement_completion_fn completion,
	const void *context, size_t context_size, P_obj corpse_context = NULL,
	item_movement_reject *reject = NULL, item_movement_publication_fn publication = nullptr,
	economic_source_kind lifecycle_source = {}, uint64_t logical_source_id = 0);
bool item_movement_transaction_submit_batch(
	P_char actor, P_obj const *roots, size_t root_count, P_obj target_container,
	const item_owner_identity &from_owner, const item_owner_identity &to_owner,
	item_transfer_reason reason, int64_t reason_id, item_movement_completion_fn completion,
	const void *context, size_t context_size, P_obj corpse_context,
	item_movement_reject *reject, item_movement_publication_fn publication,
	economic_source_kind lifecycle_source, uint64_t logical_source_id,
	const item_transfer_continuation &continuation);
// Atomically retire captured input trees and publish one or more detached output
// trees through the existing critical-command coordinator.
bool item_movement_transaction_submit_craft(
	P_char actor, P_obj const *inputs, size_t input_count, P_obj const *outputs,
	size_t output_count, int64_t recipe_id, item_movement_completion_fn completion,
	const void *context, size_t context_size, item_movement_reject *reject = NULL,
	P_obj retained_pouch = nullptr, const chaos_material_pouch_usage *pouch_usage = nullptr,
	size_t pouch_usage_count = 0,
	chaos_pouch_usage_mode pouch_mode = chaos_pouch_usage_mode::generated,
	const craft_recipe_continuation *recipe = nullptr);
bool item_creation_grant_submit_to_player(P_char actor, P_obj object, P_char recipient,
					  P_obj target_container = NULL,
					  economic_source_kind source = {}, uint64_t source_id = 0);
/* As above, but invoke `completion` only after the ownership authority has
 * published the detached object to the recipient (or has terminally rejected
 * the grant). The callback context is copied into the bounded transaction
 * state and must not contain live pointers. */
bool item_creation_grant_submit_to_player_with_completion(
	P_char actor, P_obj object, P_char recipient, item_movement_completion_fn completion,
	const void *context, size_t context_size, P_obj target_container = NULL,
	economic_source_kind source = {}, uint64_t source_id = 0);
// Reports only the final outcome: committed means the durable grant was also
// published into the requested live inventory/container. The callback runs
// after the grant queue releases this request, so it may submit a successor.
bool item_creation_grant_submit_to_player_with_completion(
	P_char actor, P_obj object, P_char recipient, P_obj target_container,
	item_creation_grant_completion_fn completion, economic_source_kind source = {},
	uint64_t source_id = 0);
bool item_creation_grant_submit_to_player_before_entry_with_completion(
	P_char actor, P_obj object, P_char recipient, item_creation_grant_completion_fn completion,
	economic_source_kind source, uint64_t source_id);
bool item_creation_grant_submit_to_player_before_entry(P_char actor, P_obj object, P_char recipient,
						       economic_source_kind source = {},
						       uint64_t source_id = 0);
// Admit all detached roots before starting any ownership operation. A refused
// batch leaves every object with the caller; an accepted batch owns every root.
bool item_creation_grant_submit_batch_to_player_before_entry(P_char actor, P_obj const *objects,
							     size_t count, P_char recipient,
							     economic_source_kind source = {},
							     uint64_t source_id = 0);
bool item_creation_grant_submit_to_room(P_char actor, P_obj object, int room,
					economic_source_kind source,
					item_creation_grant_completion_fn completion = nullptr,
					uint64_t source_id = 0);
bool item_creation_grant_mark_blocking(P_char actor);
bool item_creation_grant_blocks_commands(P_char actor);
// Only admitted grants block snapshot capture. Queued grants waiting for an
// older save must allow that save to finish; unrelated publication owners may
// themselves require a receipt-bearing save.
bool item_creation_grant_player_publication_pending(P_char player);
// Orderly maintenance must not quiesce between the roots of an accepted kit.
bool item_creation_grant_batches_pending(void);
// A disconnected pre-entry character cannot finish unsubmitted kit roots.
void item_creation_grant_cancel_batch_before_entry(P_char actor);
void item_movement_transaction_handle_completions(const critical_completion *completions,
						  size_t count);
// Restore the in-memory publication owner for replayed item commands whose
// durable continuation is sufficient to finish recovery without the original
// process-local callback.
bool item_movement_transaction_restore_replayed_command(const critical_command &command);
// Copy outstanding spell publications for a player's load request. The loader
// only needs receipts for commands whose publication is still fenced.
bool item_movement_transaction_pending_craft_progression(
	uint32_t actor_pid, std::vector<critical_operation_id> *operations);
bool item_movement_transaction_pending_spell_effects(
	uint32_t actor_pid, std::vector<critical_operation_id> *operations);
bool item_movement_transaction_restore_replayed_publication(
	const critical_command &command, item_movement_publication_fn publication,
	const void *context, size_t context_size);
void item_movement_transaction_player_ready(P_char actor);
bool item_movement_transaction_player_busy(P_char actor);
// A creation commit can advance custody before its item is published live.
// Ordinary snapshots must wait until every inbound creation has published.
bool item_movement_transaction_player_creation_busy(P_char actor);
item_movement_health item_movement_transaction_health_copy(void);
void item_movement_transaction_reset_for_tests(void);

struct critical_native_recovery_envelope;
// Startup observer only: retain the original domain continuation without
// executing, submitting, acknowledging, or entering the coordinator.
bool item_movement_transaction_restore_native_recovery(
	const critical_native_recovery_envelope &) noexcept;

class quest_native_consumption_capture;

enum class item_native_quest_preparation_state : uint8_t
{
	pending,
	ready,
	refused,
	not_matched
};
struct item_native_quest_preparation_token
{
    private:
	critical_operation_id operation_{};
	uint64_t generation_ = 0;
	friend class item_native_quest_preparation_owner;
	friend class item_native_quest_gameplay_publication_owner;
	friend class item_native_quest_publication_owner;
};
class quest_native_gameplay_owner;
enum class item_native_quest_gameplay_result : uint8_t
{
	pending,
	acceptance_applied,
	prefix_applied,
	completion_applied,
	rejected,
	unavailable
};
// Original quest driver can retain and consume only its exact generation's
// completed guard publication. These facts never grant ACK or native authority.
// The original birth owner shares the existing preparation cap without replacing
// quest-driver bytes or acquiring quest publication/ACK capabilities.
class item_native_quest_birth_budget_owner final
{
    private:
	friend class quest_mobile_native_birth_owner;
	static bool retained_budget(size_t actual_birth_bytes) noexcept;
};

// Private pure byte-accounting handoff for the one actual bounded flat ROOT
// guard. Inside scope ROOT scratch counts complete globals once; outside scope
// aggregate observes CURRENT persistent bytes. Observer has static lifetime,
// performs no allocations and takes no coordinator/journal locks. Actual ROOT
// clears scalar scratch before end, then charge(0), without callbacks between.
// No source/publication/activation/native retry capability follows.
class item_native_quest_global_budget_scope_owner final
{
	friend class zone_reset_item_owner;
	friend class zone_reset_room_publication_owner;
	friend class quest_mobile_native_birth_owner;
	// True only when the actual registered observer owns whole object/affect
	// and mobile pools, cached mobile strings and published NPC-only storage.
	// Detached NPC-only storage remains in its actual birth owner until list
	// consumption. The selecting ROOT joins all paired private consumers.
	static bool begin(const void *actual_guard, bool (*current_storage)(size_t *) noexcept,
			  bool includes_literal_pool = false) noexcept;
	static bool end(const void *actual_guard) noexcept;
	static bool literal_pool_owned() noexcept;
};

// Private current-byte ownership split. Only the real ROOT can register the
// outside observer; only the coordinator's mutex-owning lender can borrow it.
// No source, execution, publication, retry, activation or ACK authority follows.
class item_native_quest_coordinator_budget_scope_owner final
{
	friend class zone_reset_item_owner;
	friend class critical_room_shared_budget_lender;
	static bool register_observer(const void *actual_guard,
				      bool (*current_storage)(size_t *) noexcept,
				      bool (*actual_reserve)(size_t, void *) noexcept) noexcept;
	static bool registered() noexcept;
	static bool begin_borrow(const void *actual_lender,
				 bool (*actual_reserve)(size_t, void *) noexcept,
				 void *actual_guard, size_t current) noexcept;
	// Refresh only this SAME active loan from its genuine mutex-owned scan.
	// The original begin/end identity and nonnested lifetime remain unchanged.
	static bool refresh_borrow(const void *actual_lender,
				   bool (*actual_reserve)(size_t, void *) noexcept,
				   void *actual_guard, size_t freshly_observed_current) noexcept;
	static bool end_borrow(const void *actual_lender) noexcept;
	// Pure authentication of the real active loan for the same guard/callback.
	// Grants no new loan, observer access, byte baseline or reserve capability.
	static bool borrowed_for(const void *actual_guard,
				 bool (*actual_reserve)(size_t, void *) noexcept) noexcept;
	// Authenticate the same active global guard after the last loan has ended.
	static bool unborrowed_for(const void *actual_guard,
				   bool (*actual_reserve)(size_t, void *) noexcept) noexcept;
	static bool exclusive_prefix(const void *actual_guard,
				     bool (*actual_reserve)(size_t, void *) noexcept, size_t full,
				     size_t *exclusive) noexcept;
	static bool reset_before_replay() noexcept;
};

class item_native_quest_gameplay_publication_owner final
{
    private:
	friend class quest_native_gameplay_owner;
	static bool retain(const item_native_quest_preparation_token &) noexcept;
	static bool retained_budget(size_t actual_driver_bytes) noexcept;
	static bool restore_budget(size_t actual_driver_bytes) noexcept;
	static bool restore_budget_bounded(size_t actual_driver_bytes,
					   bool (*)(size_t, void *) noexcept, void *,
					   size_t) noexcept;
	// Current repository receipt plus the complete original world cut; the
	// returned runtime IDs are observed live bodies, never persisted IDs.
	static bool restored_readback(const critical_native_recovery_envelope &, P_char *,
				      P_char *) noexcept;
	static bool restored_participant(const critical_command &,
					 item_native_quest_preparation_token *, P_char *,
					 P_char *) noexcept;
	static bool restored_token(const critical_command &,
				   item_native_quest_preparation_token *) noexcept;
	// Exact completed original command, borrowed by shared ownership before take.
	// Does not consume the retained publication or grant native/ACK authority.
	// The quest driver must reserve its original aggregate budget and persist
	// its continuation handoff before consuming this publication generation.
	static item_native_quest_gameplay_result
	observe_completed(const item_native_quest_preparation_token &,
			  std::shared_ptr<const critical_command> *) noexcept;
	static item_native_quest_gameplay_result
	take(const item_native_quest_preparation_token &) noexcept;
	// Fresh complete world/UID census for one original post-GIVE callback.
	// No historical/custody/SQL proof, ACK or source authority is returned.
	static bool observe_give(uint64_t player_runtime, uint32_t pid, uint64_t native_runtime,
				 uint64_t original_root_uid, P_char *player, P_char *mobile,
				 P_obj *root) noexcept;
};

// Original already-LIVE native acceptance only. No birth/adoption, cash,
// source issuance, generic admission or bool-based publication/ACK.
class item_native_quest_preparation_owner final
{
    public:
	static item_native_quest_preparation_state
	begin_acceptance(P_char actor, P_char native_mobile, P_obj carried_root,
			 item_native_quest_preparation_token *) noexcept;
	static item_native_quest_preparation_state
	poll_acceptance(const item_native_quest_preparation_token &, P_char actor,
			P_char native_mobile) noexcept;
	// Genuine zero-item ordinary coin GIVE. No original quester trigger, source
	// issuance or synthetic carried root; complete SQL/publication owner follows.
	static item_native_quest_preparation_state
	begin_money_acceptance(P_char actor, P_char native_mobile, uint8_t denomination,
			       int32_t quantity, item_native_quest_preparation_token *) noexcept;
	static item_native_quest_preparation_state
	poll_money_acceptance(const item_native_quest_preparation_token &, P_char actor,
			      P_char native_mobile) noexcept;
	static critical_submit_result
	submit_money_acceptance(const item_native_quest_preparation_token &, P_char actor,
				P_char native_mobile) noexcept;
	static item_native_quest_preparation_state
	begin_consumption(P_char actor, P_char native_mobile,
			  std::shared_ptr<const quest_native_consumption_capture>,
			  item_native_quest_preparation_token *) noexcept;
	static item_native_quest_preparation_state
	poll_consumption(const item_native_quest_preparation_token &, P_char actor,
			 P_char native_mobile) noexcept;
	static critical_submit_result
	submit_consumption(const item_native_quest_preparation_token &, P_char actor,
			   P_char native_mobile) noexcept;
	static bool cancel(const item_native_quest_preparation_token &) noexcept;
	static critical_submit_result submit_acceptance(const item_native_quest_preparation_token &,
							P_char actor,
							P_char native_mobile) noexcept;

    private:
	friend class quest_native_gameplay_owner;
	// Freeze the exact acknowledged consumption command BEFORE its journal
	// admission, so the original parent can retain the whole child handoff.
	// True means preparation only, never durable execution or publication.
	// Output remains unchanged on refusal; failure preserves submit diagnostics.
	static bool prepare_consumption_command(const item_native_quest_preparation_token &,
						P_char actor, P_char native_mobile,
						std::shared_ptr<const critical_command> *,
						critical_submit_result *failure) noexcept;
};

// Original coin feedback only, after exact original cash publication. This
// private owner never debits/credits, executes quester, or grants receipt/ACK.
class item_native_quest_publication_owner;
class quest_native_coin_give_notice_owner final
{
    private:
	friend class item_native_quest_publication_owner;
	static bool publish(P_char actor, P_char native_mobile, uint8_t denomination,
			    int32_t quantity, int32_t original_room_vnum) noexcept;
};

// The addressed original quest continuation invokes this only after native
// acceptance publication. Preparation owns the real ordered decision and its
// acknowledged final-giver fence; it supplies no publication or ACK authority.
item_native_quest_preparation_state
quest_native_completion_prepare(P_char native_mobile, P_char final_giver, int quester_id,
				int completion_index,
				item_native_quest_preparation_token *) noexcept;

// Pure original native-quest preparation/publication CURRENT. Actual shared
// command ownership priority is ordinary pending, then native preparation,
// then quest gameplay. No source/admission/execution/ACK authority follows.
// Observe all providers under the same genuine game-thread exclusion; consume
// both item CURRENT getters and quest_native_retained_storage_bytes once.
// The predicate returns false for unavailable observation, never false absence;
// all output values remain unchanged on failure.
bool item_native_quest_retained_storage_bytes(size_t *) noexcept;
// Pure source closure queries; caller admits results before CURRENT.
bool item_native_quest_current_source_frames(size_t *) noexcept;
constexpr size_t item_native_quest_current_source_profile_query_frames() noexcept
{
	// Output formal; eleven real scalar locals, dependent CURRENT profile query,
	// constexpr thread/range return carriers and success/failure booleans.
	return 2 * sizeof(void *) + 20 * sizeof(size_t) + 4 * sizeof(bool);
}
constexpr size_t
item_native_quest_retained_command_allocation_owned_source_profile_query_frames() noexcept
{
	return sizeof(void *) + 4 * sizeof(size_t) + 2 * sizeof(bool);
}
bool item_native_quest_retained_command_allocation_owned_source_frames(size_t *) noexcept;
bool item_native_quest_retained_command_allocation_owned(const critical_command *, bool *) noexcept;

// Pure CURRENT replay registration storage. Selecting caller owns other item
// drivers and foreign domains; this observer owns the actual pending table and
// health, full payload heaps and distinct original shared command allocations.
bool item_movement_transaction_replay_current_storage_bytes(size_t *) noexcept;
size_t item_movement_transaction_replay_observer_frame_bytes() noexcept;
// Complete passive original restore companions. Authentic caller outer excludes
// the above owner once and includes all other live owners and command/native
// inputs. No gameplay, source, execution or publication authority follows.
bool item_movement_transaction_restore_replayed_publication_bounded(
	const critical_command &, item_movement_publication_fn, const void *, size_t,
	bool (*)(size_t, void *) noexcept, void *, size_t) noexcept;
class player_save_coin_replay_budget_scope_owner;
bool item_movement_transaction_restore_replayed_command_bounded(
	const critical_command &, player_save_coin_replay_budget_scope_owner &,
	bool (*)(size_t, void *) noexcept, void *, size_t) noexcept;

bool item_native_quest_restore_budget_source_frames(size_t *) noexcept;
constexpr size_t item_native_quest_restore_budget_source_profile_query_frames() noexcept
{
	return sizeof(void *) + 5 * sizeof(size_t) + 2 * sizeof(bool);
}

class item_native_recovery_replay_owner final
{
    public:
	static bool restore_held_source_frames(size_t *) noexcept;
	static bool restore_execution_source_frames(size_t *) noexcept;
	static bool restore_continuation_source_frames(size_t *) noexcept;
	static constexpr size_t source_profile_query_frames() noexcept
	{
		// Actual output/current/frame locals plus registry eight-subtotal, control
		// named-control subtotals, bucket/relay/retention/capacity and existing CURRENT query scopes.
		return 4 * sizeof(void *) + 92 * sizeof(size_t) + 8 * sizeof(bool);
	}

	static bool restore_held(const critical_native_recovery_envelope &,
				 player_save_coin_replay_budget_scope_owner &,
				 bool (*)(size_t, void *) noexcept, void *, size_t) noexcept;
	static bool restore_execution(const critical_native_recovery_envelope &,
				      player_save_coin_replay_budget_scope_owner &,
				      bool (*)(size_t, void *) noexcept, void *, size_t) noexcept;
	static bool restore_continuation(const critical_native_recovery_envelope &,
					 player_save_coin_replay_budget_scope_owner &,
					 bool (*)(size_t, void *) noexcept, void *,
					 size_t) noexcept;
};

#endif
