#include "economy/shop_trade_publication.h"
#include "flatfile/flatfile_accounting_shop_transaction.h"
#include "world/db.h"
#include "economy/shop.h"
#include "economy/account_bank_balances.h"
#include "player/inert_item_stage.h"
#ifndef __NO_MYSQL__
#include "player/player_load_items.h"
#endif
#include "world/object_template.h"

#include "economy/currency_transaction.h"
#include "item/item_ownership_runtime.h"
#include "core/prototypes.h"
#include "economy/shop_trade_runtime.h"
#include "economy/shop_trade_world_witness.h"
#include "core/utils.h"
#include "world/handler.h"

#include <algorithm>
#include <cerrno>
#include <new>
#include <map>
#include <climits>
#include <chrono>
#include "economy/shop_trade_accounting.h"
#include "economy/economic_command_admission.h"
#include <cstring>
#include <memory>
#include <limits>
#include <type_traits>
#include "player/player_snapshot_capture.h"
#include "player/player_snapshot_codec.h"
#include "persistence/shop_item_runtime_payload.h"
#include "economy/shop_trade_item_payload.h"
#include "economy/shop_trade_recovery_image.h"
#include "persistence/economic_sql_shop_trade_transaction.h"
#include "persistence/critical_command_repository.h"
#include "persistence/persistence_mode.h"
#include <set>
#ifndef __NO_MYSQL__
#include "player/player_sql_transaction_cleanup.h"
#include "sql/sql_pool.h"
#endif

extern struct shop_data *shop_index;
extern int number_of_shops;
extern P_index obj_index;
extern int top_of_objt;
extern P_obj object_list;

// The implementation owns pipeline friendship. Neither an operation ID nor a
// caller-supplied physical success callback can construct its guarded proof.
class shop_trade_native_publication_owner final
{
	friend class shop_trade_preparation_owner;
	friend bool
	shop_trade_transaction_restore_replayed_command(const critical_command &) noexcept;
	static bool expected_player_forest(const shop_trade_payload &,
					   const std::vector<player_item_snapshot> &, bool rejected,
					   std::vector<player_item_snapshot> *) noexcept;
	// Pure ordering conversion after the owner has censused these transient
	// bodies/objects. Values and pointers alone never grant native/ACK authority.
	static bool expected_player_order(const shop_trade_payload &, P_char actor, P_obj selected,
					  P_obj destination,
					  const std::vector<player_item_snapshot> &,
					  std::vector<player_item_snapshot> *) noexcept;
	static bool expected_keeper_forest(const shop_trade_payload &, P_char keeper,
					   P_obj selected,
					   const std::vector<player_item_snapshot> &, bool rejected,
					   std::vector<player_item_snapshot> *) noexcept;
	static bool expected_destination_forest(const shop_trade_payload &, P_char actor,
						P_obj selected, P_obj destination,
						std::span<const uint8_t> original,
						std::vector<player_item_snapshot> *) noexcept;
	static bool cold_publish(const critical_command &, const critical_completion &,
				 void *) noexcept;
	// Pure literal reconstruction from the genuine borrowed CURRENT flat cut.
	// Full original v8 bindings authenticate every reconstructed BEFORE byte;
	// this method supplies no storage, enrollment, publication or ACK proof.
	static bool cold_flat_original_forests(const critical_command &,
					       const flatfile_accounting_shop_projection &,
					       std::vector<player_item_snapshot> *,
					       std::vector<player_item_snapshot> *) noexcept;
	// SQL-free wrapper over the original bounded physical cold observer.
	// Strict requests prove actual presence/absence and complete locations;
	// relaxed observations expose the original pre-effect/transitional state.
	static bool cold_flat_world(void *, std::span<const player_item_snapshot>,
				    std::span<const player_item_snapshot>, bool strict,
				    bool actor_absent, bool keeper_absent,
				    shop_trade_world_cold_observation *,
				    std::vector<shop_trade_world_uid_expectation> *) noexcept;
	// Complete unpublished flat literal allocation only. Its real native caller
	// still owns original root/receipt/world proof and the full immutable charge.
	static bool cold_prepare_selected_flat(void *) noexcept;
	// Immutable reachable values only; root owns every native effect/phase return.
	static bool cold_prepare_working_flat(void *,
					      const critical_command *retained = nullptr) noexcept;
	static bool cold_flat_working_state(void *, size_t completed_legs,
					    std::span<const player_item_snapshot> *,
					    std::span<const player_item_snapshot> *,
					    std::span<const player_item_snapshot> *,
					    shop_trade_cold_native_step *, bool *terminal,
					    bool *for_nesting) noexcept;
	// Exact CURRENT retained-payload census before first native consumption.
	// Includes all immutable working variants before the single native charge.
	static bool
	cold_flat_retained_payload_bytes(void *, size_t *,
					 const critical_command *retained = nullptr) noexcept;
	static bool cold_enroll_selected_flat(player_save_restored_publication_owner &,
					      const std::string &, const flatfile_authority_lock &,
					      const flatfile_accounting_shop_projection &,
					      size_t complete_retained_stage_bytes,
					      void *) noexcept;
	static bool cold_prepare_selected(void *) noexcept;
	static bool cold_enroll_selected(void *) noexcept;
	static bool live_flat_entry(void *) noexcept;
	static bool cold_flat_entry(void *) noexcept;
	static bool restore_flat(const critical_command &) noexcept;
	static bool native_publish_flat_cold(player_save_restored_publication_owner &,
					     void *) noexcept;
	static bool native_publish_flat(player_save_restored_publication_owner &, void *) noexcept;
	static bool native_publish_flat_refusal(player_save_restored_publication_owner &,
						void *) noexcept;
	static bool native_publish(const critical_command &, const critical_completion &,
				   void *) noexcept;
	static bool no_native_effect(const critical_command &, const critical_completion &,
				     void *) noexcept
	{
		return false;
	}

	// The original live preparation keeps its separate produced/notification
	// continuation after cancellation. This private callback grants no native
	// execution/publication proof; the coordinator authenticates the exact
	// never-admitted command and refusal before invoking it.
	static bool original_refusal_no_native_effect(const critical_command &command,
						      const critical_completion &completion,
						      void *context) noexcept
	{
		return !context && critical_completion_disposition_valid(completion) &&
		       completion.disposition == critical_completion_disposition::never_admitted &&
		       command.operation_id.bytes == completion.operation_id.bytes;
	}

    public:
	static bool publish_retained(const critical_command &command,
				     const critical_completion &sealed, void *entry) noexcept
	{
		if (live_flat_entry(entry))
			return player_save_restored_publication_owner::publish_shop_flat(
				command, sealed, native_publish_flat, entry);
		if (cold_flat_entry(entry))
			return player_save_restored_publication_owner::publish_shop_flat_restored(
				command, sealed, native_publish_flat_cold, entry);
		return player_save_restored_publication_owner::publish_shop(command, sealed,
									    native_publish, entry);
	}
#ifndef __NO_MYSQL__
	// Borrowed original-transaction values only. This does not place an object,
	// publish a balance, clean up a lease or grant guarded ACK authority.
	static bool current_native_image(MYSQL *, const critical_command &,
					 const critical_completion &,
					 const player_shop_checkpoint_token &,
					 economic_sql_shop_trade_publication *) noexcept;
	// Cold v8 value counterpart, never a substitute token or native/ACK owner.
	// The exact historical receipt is verified under the same original SQL cut.
	static bool current_native_image(MYSQL *, const critical_command &,
					 const critical_completion &,
					 economic_sql_shop_trade_publication *) noexcept;

    private:
	static bool verified_current_receipt(MYSQL *, const critical_command &,
					     const critical_completion &,
					     const economic_sql_shop_trade_publication &) noexcept;

    public:
#endif
	static bool retire_never_admitted(const critical_command &command,
					  const critical_completion &completion) noexcept
	{
		return completion.disposition == critical_completion_disposition::never_admitted &&
		       player_save_restored_publication_owner::publish_shop(
			       command, completion, original_refusal_no_native_effect, nullptr);
	}

    public:
	static bool passive_current_storage_bytes(size_t *) noexcept;
	static size_t passive_storage_observer_frame_bytes() noexcept;
};

namespace
{
struct flat_trade_preparation
{
	player_flat_shop_checkpoint_token player_token{};
	player_shop_checkpoint_stage player_stage{};
	std::string selected_root;
	bool player_held = false;
	shop_trade_flat_native_checkpoint_phase phase =
		shop_trade_flat_native_checkpoint_phase::preparing;
	std::unique_ptr<shop_trade_flat_native_checkpoint_stage> native;
	bool outcome_recorded = false;
	flatfile_authority_transaction_result first_result =
		flatfile_authority_transaction_result::invalid;
	flatfile_authority_commit_outcome first_outcome =
		flatfile_authority_commit_outcome::publication_uncertain;
	size_t reserved_bytes = 0;
};
struct trade_preparation
{
	uint64_t generation = 0, actor_runtime_id = 0, keeper_runtime_id = 0;
	std::unique_ptr<flat_trade_preparation> flat;
	economic_shop_checkpoint_projection mapping{};
	player_shop_checkpoint_token player_token{};
	player_shop_checkpoint_stage player_stage{};
	bool player_held = false;
	bool submission_started = false;
	bool destination_shell_started = false, destination_shell_returned = false;
	shop_trade_destination_weight destination_weight{};
	bool destination_weight_ready = false;
	std::unique_ptr<critical_command> command;
#ifndef __NO_MYSQL__
	bool native_attempted = false, native_sealed = false, native_ready = false;
	shop_trade_native_checkpoint_stage native_stage;
#endif
	std::vector<uint8_t> selected, stock, destination, keeper_blob;
	size_t selected_capture_bytes = 0, destination_capture_bytes = 0;
	std::vector<player_item_snapshot> keeper_items;
	std::vector<uint64_t> fenced_uids;
};
uint64_t preparation_generation = 0;
// Distinct flat holder: the existing SQL stage and capability remain separate.
// All allocations and private child links precede enrollment/native callbacks.
struct cold_flat_literal_stage
{
	struct reload_item
	{
		const object_template *prototype = nullptr;
		std::array<shop_trade_original_reload_effect, 4> effects{};
		std::vector<shop_trade_original_reload_effect> proclib_probes;
	};
	std::vector<shop_trade_original_item_stage> staged;
	std::vector<reload_item> reload;
	std::vector<std::pair<int, size_t>> enrollment_counts;
	shop_trade_original_procedure_binding_stage bindings;
	bool binding_started = false, binding_returned = false;
};
// Only forests reachable from one genuine pre-effect observation are retained.
// Fixed scalar selectors refer to immutable allocations, never native pointers.
struct cold_flat_working_variants
{
	struct state
	{
		uint8_t player = 0, keeper = 0, target = 0;
		bool for_nesting = false;
	};
	const critical_command *command = nullptr;
	std::array<std::vector<player_item_snapshot>, 4> player;
	std::array<std::vector<player_item_snapshot>, 3> keeper;
	std::array<std::vector<player_item_snapshot>, 2> target;
	uint8_t player_count = 0, keeper_count = 0, target_count = 0;
	std::array<state, 7> states{};
	std::array<shop_trade_cold_native_step, 6> steps{};
	uint8_t leg_count = 0;
};
struct cold_trade_restore
{
	std::unique_ptr<critical_command> command;
	bool registration_pending = true, initialized = false;
	// Set only by the genuine passive flat registration, never a live token.
	bool flat_restored = false, flat_rejected_cleanup_returned = false;
	size_t flat_registration_bytes = 0, flat_publication_bytes = 0;
	size_t flat_completed_legs = 0, flat_effect_leg = 0;
	// Original cancellation proof is distinct from an execution receipt.
	bool refusal_verified = false;
	bool enrolled = false, enrollment_started = false, for_nesting = false;
	bool balances_started = false, balances_returned = false;
	// These are fresh cold-observation lifetimes, never fabricated preboot IDs.
	uint64_t actor_runtime_id = 0, keeper_runtime_id = 0;
	bool actor_present = false, keeper_present = false;
	std::vector<uint64_t> fenced_uids;
	std::vector<player_item_snapshot> original_player, original_keeper;
	std::vector<player_item_snapshot> working_player, working_keeper, source, selected_after;
	std::vector<player_item_snapshot> working_target;
	std::unique_ptr<cold_flat_literal_stage> flat_literals;
	std::unique_ptr<const cold_flat_working_variants> flat_working;
#ifndef __NO_MYSQL__
	std::vector<shop_trade_original_item_stage> staged;
	shop_trade_original_procedure_binding_stage bindings;
	std::map<int, size_t> enrollment_counts;
	bool binding_started = false, binding_returned = false;
	struct reload_item
	{
		const object_template *prototype = nullptr;
		std::array<shop_trade_original_reload_effect, 4> effects{};
		std::vector<shop_trade_original_reload_effect> proclib_probes;
	};
	std::vector<reload_item> reload;
#endif
	shop_trade_cold_native_effect effect;
};
struct pending_trade
{
	uint32_t player_pid = 0;
	std::unique_ptr<trade_preparation> preparation;
	std::unique_ptr<cold_trade_restore> cold;
	shop_trade_payload payload = {};

	shop_trade_completion_fn completion = nullptr;
	shop_trade_preparation_refusal_fn refusal = nullptr;
	bool production_owned = false, driving = false;
	bool local_refused = false, refusal_attempted = false;
	unsigned int refusal_error = ECANCELED;
	bool completion_ready = false;
	// Exact never-journaled hold cancellation is independent of a live body.
	// Keep this original domain/produced continuation until notification returns.
	bool never_admitted_retired = false;
	// These belong to this original live flat cancellation, not a receipt or
	// notification. A started but unreturned extraction never becomes absent proof.
	bool flat_refusal_verified = false;
	bool flat_refusal_cleanup_started = false, flat_refusal_cleanup_returned = false;
	bool native_acknowledged = false;
	bool native_receipt_verified = false;
	// These are original ordered values; never SQL UID-map iteration order.
	bool publication_forests_ready = false;
	std::vector<player_item_snapshot> before_player, after_player, after_keeper;
	std::vector<uint8_t> after_destination;

	critical_completion completed = {};
	shop_trade_physical_publication_fn publication = nullptr;
	shop_trade_accounted_publication_fn accounted_publication = nullptr;
	uint32_t physical_stages = 0;
	bool balances_published = false;
	uint64_t balance_runtime_id = 0;
	bool custody_published = false;
	bool revision_published = false;
	bool physical_published = false;
	bool publishing = false;
	bool blocked = false;
};

// Fixed keys and node insertion keep lookup and exception restoration allocation
// free. Native callbacks may reenter admission; map iterators remain valid.
std::map<std::array<uint8_t, CRITICAL_COMMAND_ID_BYTES>, pending_trade> pending;
size_t notifying = 0;

bool player_pending(uint32_t pid)
{
	return std::any_of(pending.begin(), pending.end(),
			   [pid](const auto &entry) { return entry.second.player_pid == pid; });
}

bool publish_ownership(const shop_trade_payload &payload, const shop_trade_result &result)
{
	const item_owner_identity player = { item_owner_type::player, payload.player_pid, 0 };
	const item_owner_identity shop = { item_owner_type::shopkeeper,
					   item_shopkeeper_owner_id(payload.shop_id), 0 };
	const item_owner_identity system = { item_owner_type::system, 0, 0 };
	const item_owner_identity destruction = { item_owner_type::destruction, 0, 0 };
	item_transfer_payload transfer = {};
	if (payload.action == shop_trade_action::buy_existing)
	{
		transfer.from_owner = shop;
		transfer.to_owner = player;
		transfer.reason = item_transfer_reason::shop_buy;
	}
	else if (payload.action == shop_trade_action::buy_produced)
	{
		transfer.from_owner = system;
		transfer.to_owner = player;
		transfer.reason = item_transfer_reason::creation;
	}
	else if (payload.action == shop_trade_action::sell_store)
	{
		transfer.from_owner = player;
		transfer.to_owner = shop;
		transfer.reason = item_transfer_reason::shop_sell;
	}
	else if (payload.action == shop_trade_action::sell_destroy)
	{
		transfer.from_owner = player;
		transfer.to_owner = destruction;
		transfer.reason = item_transfer_reason::destruction;
	}
	else if (payload.action == shop_trade_action::discard_invalid)
	{
		transfer.from_owner = shop;
		transfer.to_owner = destruction;
		transfer.reason = item_transfer_reason::destruction;
	}
	else
		return false;
	transfer.reason_id = payload.shop_id;
	transfer.selected_item_uid = payload.selected_item_uid;
	transfer.target_root_item_uid = payload.target_root_item_uid ?
						payload.target_root_item_uid :
						payload.selected_item_uid;
	transfer.target_parent_item_uid = payload.target_parent_item_uid;
	transfer.expected_target_parent_revision = payload.expected_target_parent_revision;
	transfer.item_count = payload.item_count;
	for (size_t index = 0; index < payload.item_count; ++index)
		transfer.items[index] = { payload.items[index].item_uid,
					  payload.items[index].root_item_uid,
					  payload.items[index].parent_item_uid,
					  payload.items[index].expected_item_revision,
					  payload.items[index].vnum,
					  payload.items[index].expected_state };
	const bool buying = payload.action == shop_trade_action::buy_existing ||
			    payload.action == shop_trade_action::buy_produced;
	const bool cleanup = payload.action == shop_trade_action::discard_invalid;
	item_transfer_result transfer_result = {
		.root_item_uid = payload.selected_item_uid,
		.item_count = result.item_count,
		.from_owner_revision = cleanup ? result.player_owner_revision :
				       buying  ? result.counterparty_owner_revision :
						 result.player_owner_revision,
		.to_owner_revision = cleanup ? result.counterparty_owner_revision :
				     buying  ? result.player_owner_revision :
					       result.counterparty_owner_revision,
		.max_item_revision = 0,
		.corpse_revision = 0,
	};
	for (size_t index = 0; index < result.item_count; ++index)
		transfer_result.max_item_revision =
			std::max(transfer_result.max_item_revision, result.item_revisions[index]);
	return item_ownership_runtime_apply(transfer, transfer_result);
}

bool same_receipt(const critical_completion &left, const critical_completion &right)
{
	const auto committed = [](critical_apply_outcome outcome)
	{
		return outcome == critical_apply_outcome::applied ||
		       outcome == critical_apply_outcome::already_applied;
	};
	const bool same_committed_result = committed(left.outcome) && committed(right.outcome);
	return left.disposition == right.disposition &&
	       (same_committed_result ||
		(left.outcome == right.outcome && left.attempt == right.attempt)) &&
	       left.durable_revision == right.durable_revision &&
	       left.error_code == right.error_code && left.failure_stage == right.failure_stage &&
	       left.result_size == right.result_size && left.result_payload == right.result_payload;
}

// Cancellation retains the exact authenticated original refusal, including
// timing/attempt metadata. Execution replay equivalence must not weaken it.
bool same_original_refusal(const critical_completion &left, const critical_completion &right)
{
	return critical_completion_disposition_valid(left) &&
	       critical_completion_disposition_valid(right) &&
	       left.disposition == critical_completion_disposition::never_admitted &&
	       right.disposition == left.disposition &&
	       left.operation_id.bytes == right.operation_id.bytes &&
	       left.outcome == right.outcome && left.attempt == right.attempt &&
	       left.queued_at_usec == right.queued_at_usec &&
	       left.started_at_usec == right.started_at_usec &&
	       left.completed_at_usec == right.completed_at_usec &&
	       left.durable_revision == right.durable_revision &&
	       left.error_code == right.error_code && left.failure_stage == right.failure_stage &&
	       left.result_size == right.result_size && left.result_payload == right.result_payload;
}

// Schema2 native delivery metadata can differ; the first verified original
// canonical result remains sealed, including its full fixed payload tail.
bool same_native_receipt(const critical_completion &left, const critical_completion &right)
{
	auto committed = [](critical_apply_outcome value)
	{
		return value == critical_apply_outcome::applied ||
		       value == critical_apply_outcome::already_applied;
	};
	return left.operation_id.bytes == right.operation_id.bytes &&
	       left.disposition == right.disposition &&
	       ((committed(left.outcome) && committed(right.outcome)) ||
		left.outcome == right.outcome) &&
	       left.durable_revision == right.durable_revision &&
	       left.error_code == right.error_code && left.failure_stage == right.failure_stage &&
	       left.result_size == right.result_size && left.result_payload == right.result_payload;
}

bool committed_result_matches(const pending_trade &entry, const shop_trade_result &result)
{
	const auto &payload = entry.payload;
	if (result.action != payload.action || result.item_count != payload.item_count ||
	    entry.completed.error_code || payload.expected_shop_revision == UINT64_MAX ||
	    result.shop_revision != payload.expected_shop_revision + 1)
		return false;
	if (result.keeper_cash_recorded)
	{
		int64_t expected_cash = payload.expected_keeper_cash;
		if (payload.action == shop_trade_action::buy_existing ||
		    payload.action == shop_trade_action::buy_produced)
			expected_cash += payload.price;
		else if (payload.action != shop_trade_action::discard_invalid &&
			 expected_cash >= payload.price)
			expected_cash -= payload.price;
		if (result.keeper_cash != expected_cash)
			return false;
	}
	for (size_t index = 0; index < payload.item_count; ++index)
	{
		const auto &item = payload.items[index];
		const uint64_t expected = item.expected_item_revision ==
							  ITEM_TRANSFER_ABSENT_REVISION ?
						  1 :
						  item.expected_item_revision + 1;
		if (result.item_uids[index] != item.item_uid || !expected ||
		    result.item_revisions[index] != expected)
			return false;
	}
	return true;
}

bool publish(decltype(pending)::iterator found, P_char character)
{
	pending_trade &entry = found->second;
	if (entry.blocked || entry.publishing)
		return false;
	if (entry.cold)
	{
		if (entry.cold->registration_pending || !entry.cold->command ||
		    !entry.completion_ready ||
		    !critical_completion_disposition_valid(entry.completed))
			return false;
		// The shared owner authenticates the original refusal before calling the
		// cold cleanup callback, then consumes its guarded cancellation proof.
		const critical_completion original = entry.completed;
		entry.publishing = true;
		const bool acknowledged = shop_trade_native_publication_owner::publish_retained(
			*entry.cold->command, original, &entry);
		entry.publishing = false;
		if (!acknowledged || entry.blocked ||
		    (original.disposition == critical_completion_disposition::never_admitted ?
			     !same_original_refusal(original, entry.completed) :
			     !same_native_receipt(original, entry.completed)))
			return false;
		// The private restored-slot owner consumed guarded ACK or original refusal
		// cancellation after native proof. No live callback or bulk continuation.
		pending.erase(found);
		return true;
	}
	if (entry.preparation)
	{
		if (!entry.preparation->submission_started || !entry.preparation->command)
			return false;
		if (entry.completed.disposition == critical_completion_disposition::never_admitted)
		{
			// An attempted flat source checkpoint needs genuine terminal cleanup.
			// Receipt absence and coordinator refusal alone cannot retire its hold.
			if (entry.preparation->flat)
			{
				if (!entry.never_admitted_retired)
				{
					const critical_completion original = entry.completed;
					entry.publishing = true;
					const bool retired =
						shop_trade_native_publication_owner::publish_retained(
							*entry.preparation->command, original,
							&entry);
					entry.publishing = false;
					if (!retired || entry.blocked ||
					    !same_original_refusal(original, entry.completed))
						return false;
					entry.never_admitted_retired = true;
				}
				// Notification remains a distinct live continuation. In particular,
				// authentic body absence can finish native cancellation, but cannot
				// discharge the original produced-purchase notification/sequence.
				character = find_character_by_runtime_id(
					entry.preparation->actor_runtime_id);
				if (!character || !IS_PC(character) || !character->only.pc ||
				    static_cast<uint32_t>(GET_PID(character)) != entry.player_pid)
					return false;
			}
			else
			{
				if (!entry.never_admitted_retired &&
				    !shop_trade_native_publication_owner::retire_never_admitted(
					    *entry.preparation->command, entry.completed))
					return false;
				entry.never_admitted_retired = true;
				entry.preparation.reset();
			}
		}
		else if (!entry.native_acknowledged)
		{
			const critical_completion original = entry.completed;
			entry.publishing = true;
			const bool acknowledged =
				shop_trade_native_publication_owner::publish_retained(
					*entry.preparation->command, original, &entry);
			entry.publishing = false;
			if (!acknowledged || entry.blocked ||
			    !same_native_receipt(original, entry.completed))
				return false;
			entry.native_acknowledged = true;
		}
	}
	if (!critical_completion_disposition_valid(entry.completed))
	{
		entry.blocked = true;
		return false;
	}
	const bool committed = entry.completed.outcome == critical_apply_outcome::applied ||
			       entry.completed.outcome == critical_apply_outcome::already_applied;
	if (!committed && entry.completed.outcome != critical_apply_outcome::terminal_failure)
		return false;
	shop_trade_result result = {};
	const bool decoded = shop_trade_command_decode_result(entry.completed.result_payload.data(),
							      entry.completed.result_size, &result);
	if (committed && (!decoded || !committed_result_matches(entry, result)))
	{
		entry.blocked = true;
		return false;
	}
	const bool legacy_committed = committed && !entry.preparation;
	entry.publishing = true;
	try
	{
		if (legacy_committed && !entry.revision_published &&
		    !shop_trade_runtime_can_advance(entry.payload.shop_id,
						    entry.payload.expected_shop_revision,
						    result.shop_revision))
		{
			entry.publishing = false;
			return false;
		}
		if (decoded && !entry.preparation &&
		    (!entry.balances_published ||
		     entry.balance_runtime_id != character->runtime_id))
		{
			const uint64_t runtime_id = character->runtime_id;
			if (!currency_transaction_publish_balances(
				    character, entry.payload.account_name.data(),
				    entry.payload.racewar, result.wallet, result.bank,
				    result.wallet_revision, result.bank_revision))
			{
				entry.publishing = false;
				return false;
			}
			entry.balances_published = true;
			entry.balance_runtime_id = runtime_id;
		}
		if (entry.blocked)
		{
			entry.publishing = false;
			return false;
		}
		if (legacy_committed && !entry.custody_published)
		{
			if (!publish_ownership(entry.payload, result))
			{
				entry.publishing = false;
				return false;
			}
			entry.custody_published = true;
		}
		if (entry.blocked)
		{
			entry.publishing = false;
			return false;
		}
		if (legacy_committed && !entry.revision_published)
		{
			if (!shop_trade_runtime_advance(entry.payload.shop_id,
							entry.payload.expected_shop_revision,
							result.shop_revision))
			{
				entry.publishing = false;
				return false;
			}
			entry.revision_published = true;
		}
		if (entry.blocked)
		{
			entry.publishing = false;
			return false;
		}
		if (legacy_committed && !entry.physical_published)
		{
			P_char current = find_player_by_pid(entry.player_pid);
			if (!current || current != character ||
			    current->runtime_id != entry.balance_runtime_id)
			{
				entry.publishing = false;
				return false;
			}
			if (entry.publication &&
			    !entry.publication(character, result, entry.payload,
					       entry.physical_stages))
			{
				entry.publishing = false;
				return false;
			}
			entry.physical_published = true;
		}
		if (entry.blocked)
		{
			entry.publishing = false;
			return false;
		}
	}
	catch (...)
	{
		entry.publishing = false;
		return false;
	}
	entry.publishing = false;
	if (entry.preparation)
	{
		character = find_character_by_runtime_id(entry.preparation->actor_runtime_id);
		if (!character || !IS_PC(character) || !character->only.pc ||
		    static_cast<uint32_t>(GET_PID(character)) != entry.player_pid)
			return false;
	}
	// Own the original context independently while a successful notification
	// starts another purchase. A thrown notification is retained but never
	// automatically repeated: it may already have advanced that continuation.
	auto node = pending.extract(found);
	++notifying;
	try
	{
		const auto &finished = node.mapped();
		if (finished.completion)
			finished.completion(character, committed, result,
					    finished.completed.error_code, finished.payload);
	}
	catch (...)
	{
		node.mapped().blocked = true;
		pending.insert(std::move(node));
		--notifying;
		return false;
	}
	--notifying;
	return true;
}

#ifndef __NO_MYSQL__
#endif

bool shop_remove_selected(const std::vector<player_item_snapshot> &input,
			  const shop_trade_payload &payload,
			  std::vector<player_item_snapshot> *output)
{
	std::set<uint64_t> selected;
	for (size_t index = 0; index < payload.item_count; ++index)
		selected.insert(payload.items[index].item_uid);
	std::vector<player_item_snapshot> result;
	std::vector<int32_t> positions(input.size(), PLAYER_SNAPSHOT_NO_PARENT);
	for (size_t index = 0; index < input.size(); ++index)
	{
		const auto &item = input[index];
		if (selected.count(item.object_uid))
			continue;
		if (item.parent_index < PLAYER_SNAPSHOT_NO_PARENT ||
		    item.parent_index >= static_cast<int32_t>(index) ||
		    (item.parent_index >= 0 && positions[item.parent_index] < 0))
			return false;
		positions[index] = static_cast<int32_t>(result.size());
		result.push_back(item);
		if (item.parent_index >= 0)
			result.back().parent_index = positions[item.parent_index];
	}
	*output = std::move(result);
	return true;
}

#ifndef __NO_MYSQL__
bool shop_image_matches(const std::vector<player_item_snapshot> &forest,
			const shop_item_runtime_image &image)
{
	if (forest.size() != image.size())
		return false;
	std::vector<uint64_t> roots(forest.size());
	for (size_t index = 0; index < forest.size(); ++index)
	{
		const auto &item = forest[index];
		if (item.parent_index < PLAYER_SNAPSHOT_NO_PARENT ||
		    item.parent_index >= static_cast<int32_t>(index))
			return false;
		roots[index] = item.parent_index < 0 ? item.object_uid : roots[item.parent_index];
		const auto found = image.find(item.object_uid);
		if (found == image.end() || !found->second.payload_present ||
		    found->second.root_uid != roots[index] ||
		    found->second.slot != item.equipment_slot)
			return false;
		const uint64_t parent_id =
			item.parent_index < 0 ? 0 :
						image.at(forest[item.parent_index].object_uid).id;
		if (found->second.parent_id != parent_id)
			return false;
		auto expected = item;
		expected.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
		std::vector<uint8_t> a, b;
		if (player_item_snapshot_list_encode({ expected }, &a) !=
			    player_snapshot_codec_result::ok ||
		    player_item_snapshot_list_encode({ found->second.item }, &b) !=
			    player_snapshot_codec_result::ok ||
		    a != b)
			return false;
	}
	return true;
}

bool shop_actor_matches(P_char actor, P_char keeper, const pending_trade &entry)
{
	if (!entry.preparation || !actor || !IS_PC(actor) || !actor->only.pc || !keeper ||
	    !IS_NPC(keeper) || static_cast<uint32_t>(GET_PID(actor)) != entry.player_pid ||
	    actor->runtime_id != entry.preparation->actor_runtime_id ||
	    keeper->runtime_id != entry.preparation->keeper_runtime_id ||
	    GET_VNUM(keeper) != entry.payload.keeper_vnum ||
	    static_cast<uint32_t>(GET_LEVEL(actor)) != entry.payload.expected_player_level ||
	    GET_RACEWAR(actor) != entry.payload.racewar || !shop_index ||
	    entry.payload.shop_id >= static_cast<uint32_t>(number_of_shops) ||
	    GET_RNUM(keeper) != shop_index[entry.payload.shop_id].keeper ||
	    (shop_index[entry.payload.shop_id].shop_is_roaming != 0) !=
		    (entry.payload.keeper_roaming != 0))
		return false;
	const char *account = get_account_name_safe(actor);
	return account && !strcmp(account, entry.payload.account_name.data());
}

bool preparation_destination_tree(P_obj, std::vector<uint8_t> *, std::vector<uint64_t> * = nullptr,
				  size_t *estimated_bytes = nullptr);

bool shop_world(pending_trade &entry, const std::vector<player_item_snapshot> &player,
		const std::vector<player_item_snapshot> &keeper,
		const std::vector<player_item_snapshot> &detached,
		const economic_sql_shop_trade_publication *native, bool final,
		shop_trade_world_witness *output)
{
	std::vector<shop_trade_world_uid_expectation> locations;
	std::set<uint64_t> included;
	const auto append = [&](const auto &forest, bool is_keeper, bool is_detached,
				const shop_item_runtime_image *image) -> bool
	{
		for (size_t index = 0; index < forest.size(); ++index)
		{
			const auto &item = forest[index];
			if (!included.insert(item.object_uid).second || item.parent_index < -1 ||
			    item.parent_index >= static_cast<int32_t>(index))
				return false;
			const uint64_t parent =
				item.parent_index < 0 ? 0 : forest[item.parent_index].object_uid;
			int32_t row_id = -1;
			if (image)
			{
				const auto found = image->find(item.object_uid);
				if (found == image->end() || !found->second.id ||
				    found->second.id > INT_MAX)
					return false;
				row_id = static_cast<int32_t>(found->second.id);
			}
			const auto location =
				parent	    ? shop_trade_world_location::inside :
				is_detached ? shop_trade_world_location::detached :
				is_keeper   ? (item.equipment_slot ?
						       shop_trade_world_location::keeper_equipment :
						       shop_trade_world_location::keeper_inventory) :
					      (item.equipment_slot ?
						       shop_trade_world_location::actor_equipment :
						       shop_trade_world_location::actor_inventory);
			locations.push_back(
				{ item.object_uid, location, parent, item.equipment_slot, row_id });
		}
		return true;
	};
	if (!append(player, false, false, native ? &native->whole_player_items : nullptr) ||
	    !append(keeper, true, false, native ? &native->keeper_items : nullptr) ||
	    !append(detached, false, true, nullptr))
		return false;
	if (final)
		for (size_t index = 0; index < entry.payload.item_count; ++index)
			if (!included.count(entry.payload.items[index].item_uid))
				locations.push_back({ entry.payload.items[index].item_uid,
						      shop_trade_world_location::absent, 0, 0,
						      -1 });
	std::vector<uint64_t> literal;
	if (std::any_of(player.begin(), player.end(), [&](const auto &item)
			{ return item.object_uid == entry.payload.selected_item_uid; }))
		literal.push_back(entry.payload.selected_item_uid);
	shop_trade_world_expectation expected{};
	expected.actor_pid = entry.player_pid;
	expected.actor_runtime_id = entry.preparation->actor_runtime_id;
	expected.keeper_runtime_id = entry.preparation->keeper_runtime_id;
	expected.shop_id = entry.payload.shop_id;
	expected.keeper_vnum = entry.payload.keeper_vnum;
	expected.player_items = player;
	expected.keeper_items = keeper;
	expected.detached_items = detached;
	expected.literal_player_root_uids = literal;
	expected.uid_locations = locations;
	return shop_trade_world_witness_observe(expected, output) &&
	       shop_actor_matches(output->actor, output->keeper, entry);
}

P_obj shop_witness_object(const shop_trade_world_witness &witness, uint64_t uid)
{
	if (!uid)
		return nullptr;
	const auto found = std::find_if(witness.objects.begin(), witness.objects.end(),
					[uid](P_obj object)
					{ return object && object->obj_uid == uid; });
	return found == witness.objects.end() ? nullptr : *found;
}
#endif

} // namespace

bool shop_trade_native_publication_owner::expected_player_forest(
	const shop_trade_payload &payload, const std::vector<player_item_snapshot> &original,
	bool rejected, std::vector<player_item_snapshot> *output) noexcept
{
	if (!output || original.size() > PLAYER_SNAPSHOT_MAX_OBJECTS)
		return false;
	try
	{
		std::vector<player_item_snapshot> selected;
		if (!shop_trade_accounted_after_items(payload, &selected))
			return false;
		std::map<uint64_t, size_t> original_indices;
		std::vector<uint64_t> original_roots(original.size());
		for (size_t index = 0; index < original.size(); ++index)
		{
			const auto &item = original[index];
			if (!item.object_uid || item.object_uid == UINT64_MAX ||
			    !original_indices.emplace(item.object_uid, index).second ||
			    item.parent_index < PLAYER_SNAPSHOT_NO_PARENT ||
			    item.parent_index >= static_cast<int32_t>(index))
				return false;
			original_roots[index] = item.parent_index < 0 ?
							item.object_uid :
							original_roots[item.parent_index];
		}
		std::set<uint64_t> selected_uids;
		for (const auto &item : selected)
			if (!selected_uids.insert(item.object_uid).second)
				return false;
		const bool selling = payload.action == shop_trade_action::sell_store ||
				     payload.action == shop_trade_action::sell_destroy;
		const bool buying = payload.action == shop_trade_action::buy_existing ||
				    payload.action == shop_trade_action::buy_produced;
		if (!selling && !buying && payload.action != shop_trade_action::discard_invalid)
			return false;
		for (const auto &item : selected)
		{
			const auto found = original_indices.find(item.object_uid);
			if (!selling)
			{
				if (found != original_indices.end())
					return false;
				continue;
			}
			if (found == original_indices.end())
				return false;
			const size_t index = found->second;
			const auto &before = original[index];
			const uint64_t parent_uid =
				before.parent_index < 0 ? 0 :
							  original[before.parent_index].object_uid;
			const auto expected = std::find_if(
				payload.items.begin(), payload.items.begin() + payload.item_count,
				[&](const auto &entry)
				{ return entry.item_uid == item.object_uid; });
			if (expected == payload.items.begin() + payload.item_count ||
			    original_roots[index] != expected->root_item_uid ||
			    parent_uid != expected->parent_item_uid || before.equipment_slot)
				return false;
			auto before_literal = before;
			auto selected_literal = item;
			before_literal.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
			selected_literal.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
			std::vector<uint8_t> before_bytes, selected_bytes;
			if (player_item_snapshot_list_encode({ before_literal }, &before_bytes) !=
				    player_snapshot_codec_result::ok ||
			    player_item_snapshot_list_encode({ selected_literal },
							     &selected_bytes) !=
				    player_snapshot_codec_result::ok ||
			    before_bytes != selected_bytes)
				return false;
		}
		std::vector<player_item_snapshot> candidate;
		std::vector<int32_t> retained_indices(original.size(), PLAYER_SNAPSHOT_NO_PARENT);
		candidate.reserve(original.size());
		for (size_t index = 0; index < original.size(); ++index)
		{
			const auto &item = original[index];
			if (!rejected && selling && selected_uids.count(item.object_uid))
				continue;
			if (item.parent_index >= 0 && retained_indices[item.parent_index] < 0)
				return false; // Never leave an unselected child of a removed source.
			retained_indices[index] = static_cast<int32_t>(candidate.size());
			candidate.push_back(item);
			if (item.parent_index >= 0)
				candidate.back().parent_index = retained_indices[item.parent_index];
		}
		if (!rejected && buying)
		{
			if (selected.size() > PLAYER_SNAPSHOT_MAX_OBJECTS - candidate.size())
				return false;
			int32_t target_parent = PLAYER_SNAPSHOT_NO_PARENT;
			if (payload.target_parent_item_uid)
			{
				const auto found =
					original_indices.find(payload.target_parent_item_uid);
				if (found == original_indices.end() ||
				    original_roots[found->second] != payload.target_root_item_uid)
					return false;
				target_parent = retained_indices[found->second];
				if (target_parent < 0)
					return false;
				if (payload.native_destination_weight_recorded)
				{
					if (!shop_trade_destination_weight_verify(
						    candidate[target_parent],
						    selected.front().weight,
						    payload.destination_weight))
						return false;
					candidate[target_parent].weight =
						payload.destination_weight.after;
				}
			}
			const size_t base = candidate.size();
			for (const auto &item : selected)
			{
				candidate.push_back(item);
				candidate.back().parent_index =
					item.parent_index < 0 ?
						target_parent :
						static_cast<int32_t>(base + item.parent_index);
			}
		}
		std::vector<uint8_t> canonical;
		if (player_item_snapshot_list_encode(candidate, &canonical) !=
			    player_snapshot_codec_result::ok ||
		    canonical.size() > PLAYER_SNAPSHOT_MAX_BYTES - sizeof(uint32_t))
			return false;
		*output = std::move(candidate);
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool shop_trade_native_publication_owner::expected_player_order(
	const shop_trade_payload &payload, P_char actor, P_obj selected, P_obj destination,
	const std::vector<player_item_snapshot> &values,
	std::vector<player_item_snapshot> *output) noexcept
{
	return shop_trade_world_expected_player_order(payload, actor, selected, destination, values,
						      output);
}

#ifndef __NO_MYSQL__
#endif

bool shop_trade_native_publication_owner::expected_keeper_forest(
	const shop_trade_payload &payload, P_char keeper, P_obj selected,
	const std::vector<player_item_snapshot> &original, bool rejected,
	std::vector<player_item_snapshot> *output) noexcept
{
	if (!output || original.size() > PLAYER_SNAPSHOT_MAX_OBJECTS)
		return false;
	try
	{
		auto candidate = original;
		if (!rejected && (payload.action == shop_trade_action::buy_existing ||
				  payload.action == shop_trade_action::discard_invalid))
		{
			if (!shop_remove_selected(original, payload, &candidate))
				return false;
		}
		if (!rejected && payload.action == shop_trade_action::sell_store)
		{
			std::vector<player_item_snapshot> selected_after;
			if (!shop_trade_accounted_after_items(payload, &selected_after) ||
			    selected_after.size() > PLAYER_SNAPSHOT_MAX_OBJECTS - candidate.size())
				return false;
			const auto base = static_cast<int32_t>(candidate.size());
			for (auto item : selected_after)
			{
				if (item.parent_index >= 0)
					item.parent_index += base;
				candidate.push_back(std::move(item));
			}
			std::vector<player_item_snapshot> ordered;
			if (!shop_trade_world_expected_keeper_order(payload, keeper, selected,
								    candidate, &ordered))
				return false;
			candidate = std::move(ordered);
		}
		std::vector<uint8_t> canonical;
		if (player_item_snapshot_list_encode(candidate, &canonical) !=
			    player_snapshot_codec_result::ok ||
		    canonical.size() > PLAYER_SNAPSHOT_MAX_BYTES - sizeof(uint32_t) ||
		    !shop_trade_world_player_values_supported(candidate))
			return false;
		*output = std::move(candidate);
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool shop_trade_native_publication_owner::expected_destination_forest(
	const shop_trade_payload &payload, P_char actor, P_obj selected, P_obj destination,
	std::span<const uint8_t> original, std::vector<player_item_snapshot> *output) noexcept
{
	if (!output || !destination || !payload.target_parent_item_uid ||
	    !payload.native_destination_weight_recorded)
		return false;
	try
	{
		std::vector<player_item_snapshot> literal, selected_after, ordered;
		if (player_item_snapshot_list_decode(original.data(), original.size(), &literal) !=
			    player_snapshot_codec_result::ok ||
		    literal.empty() ||
		    literal.front().object_uid != payload.target_parent_item_uid ||
		    !shop_trade_accounted_after_items(payload, &selected_after) ||
		    selected_after.empty() ||
		    !shop_trade_destination_weight_verify(literal.front(),
							  selected_after.front().weight,
							  payload.destination_weight) ||
		    selected_after.size() > PLAYER_SNAPSHOT_MAX_OBJECTS - literal.size())
			return false;
		literal.front().weight = payload.destination_weight.after;
		const auto base = static_cast<int32_t>(literal.size());
		for (auto item : selected_after)
		{
			item.parent_index = item.parent_index < 0 ? 0 : base + item.parent_index;
			literal.push_back(std::move(item));
		}
		if (!expected_player_order(payload, actor, selected, destination, literal,
					   &ordered))
			return false;
		*output = std::move(ordered);
		return true;
	}
	catch (...)
	{
		return false;
	}
}

#ifndef __NO_MYSQL__
bool shop_trade_native_publication_owner::verified_current_receipt(
	MYSQL *connection, const critical_command &command, const critical_completion &sealed,
	const economic_sql_shop_trade_publication &current) noexcept
{
	try
	{
		const bool success = sealed.outcome == critical_apply_outcome::applied ||
				     sealed.outcome == critical_apply_outcome::already_applied;
		// SQL identities are uint64 values, while live obj_data::db_item_id
		// is a signed int. Reject an unrepresentable complete native cut before
		// any future physical row-ID assignment, callback or ACK proof.
		const auto native_rows_fit = [](const shop_item_runtime_image &image)
		{
			return std::all_of(image.begin(), image.end(),
					   [](const auto &entry)
					   {
						   const auto &row = entry.second;
						   return row.id && row.id <= INT_MAX &&
							  row.parent_id <= INT_MAX;
					   });
		};
		if (!native_rows_fit(current.whole_player_items) ||
		    !native_rows_fit(current.keeper_items) ||
		    !native_rows_fit(current.player_items))
			return false;
		const auto authentic = critical_command_repository_verify_shop_trade_in_transaction(
			connection, command);
		const bool authentic_success =
			authentic.outcome == critical_apply_outcome::applied ||
			authentic.outcome == critical_apply_outcome::already_applied;
		if ((success ? !authentic_success : authentic.outcome != sealed.outcome) ||
		    authentic.error_code != sealed.error_code ||
		    authentic.failure_stage != sealed.failure_stage ||
		    authentic.durable_revision != sealed.durable_revision ||
		    authentic.result_size != sealed.result_size ||
		    authentic.result_payload != sealed.result_payload || !connection ||
		    mysql_thread_id(connection) != current.session_id ||
		    !(connection->server_status & SERVER_STATUS_IN_TRANS))
			return false;
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool shop_trade_native_publication_owner::current_native_image(
	MYSQL *connection, const critical_command &command, const critical_completion &sealed,
	economic_sql_shop_trade_publication *output) noexcept
{
	if (!output || !nevent_is_game_thread() ||
	    sealed.disposition != critical_completion_disposition::execution ||
	    sealed.operation_id.bytes != command.operation_id.bytes ||
	    command.payload_version != SHOP_TRADE_RECOVERY_MANIFEST_VERSION)
		return false;
	try
	{
		shop_trade_payload payload{};
		if (!shop_trade_command_decode_payload(command, &payload) ||
		    !payload.recovery_manifest_recorded)
			return false;
		economic_sql_shop_trade_publication current;
		if (economic_sql_shop_trade_lock_publication(connection, command, sealed,
							     &current) ||
		    !verified_current_receipt(connection, command, sealed, current))
			return false;
		*output = std::move(current);
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool shop_trade_native_publication_owner::current_native_image(
	MYSQL *connection, const critical_command &command, const critical_completion &sealed,
	const player_shop_checkpoint_token &token,
	economic_sql_shop_trade_publication *output) noexcept
{
	if (!output || !nevent_is_game_thread() ||
	    sealed.disposition != critical_completion_disposition::execution ||
	    sealed.operation_id.bytes != command.operation_id.bytes)
		return false;
	try
	{
		shop_trade_payload payload{};
		std::vector<player_item_snapshot> original, expected;
		player_shop_checkpoint_stage stage{};
		const bool success = sealed.outcome == critical_apply_outcome::applied ||
				     sealed.outcome == critical_apply_outcome::already_applied;
		if ((!success && sealed.outcome != critical_apply_outcome::terminal_failure) ||
		    !shop_trade_command_decode_payload(command, &payload) ||
		    !player_save_shop_checkpoint_owner::original_held_body(token, command,
									   &original, &stage) ||
		    stage.save_revision != payload.expected_player_save_revision ||
		    stage.level != payload.expected_player_level ||
		    !expected_player_forest(payload, original, !success, &expected))
			return false;
		economic_sql_shop_trade_publication current;
		if (economic_sql_shop_trade_lock_publication(connection, command, sealed, expected,
							     &current))
			return false;
		if (payload.recovery_manifest_recorded)
		{
			std::vector<uint8_t> original_bytes;
			std::vector<player_item_snapshot> player_cut, keeper_cut;
			const auto &manifest = payload.recovery_manifest;
			if (player_item_snapshot_list_encode(original, &original_bytes) !=
				    player_snapshot_codec_result::ok ||
			    !shop_trade_recovery_forest_verify(
				    original_bytes, shop_trade_recovery_forest_role::player_before,
				    manifest.player_before) ||
			    !shop_trade_recovery_image_reconstruct(
				    success ? manifest.player_after : manifest.player_before,
				    success ? shop_trade_recovery_forest_role::player_after :
					      shop_trade_recovery_forest_role::player_before,
				    current.whole_player_items, &player_cut) ||
			    !shop_trade_recovery_image_reconstruct(
				    success ? manifest.keeper_after : manifest.keeper_before,
				    success ? shop_trade_recovery_forest_role::keeper_after :
					      shop_trade_recovery_forest_role::keeper_before,
				    current.keeper_items, &keeper_cut))
				return false;
		}
		if (!verified_current_receipt(connection, command, sealed, current))
			return false;
		*output = std::move(current);
		return true;
	}
	catch (...)
	{
		return false;
	}
}
#endif

#ifndef __NO_MYSQL__
namespace
{
bool cold_forest_equal(std::span<const player_item_snapshot> a,
		       std::span<const player_item_snapshot> b)
{
	std::vector<uint8_t> left, right;
	return player_item_snapshot_list_encode(
		       std::vector<player_item_snapshot>(a.begin(), a.end()), &left) ==
		       player_snapshot_codec_result::ok &&
	       player_item_snapshot_list_encode(
		       std::vector<player_item_snapshot>(b.begin(), b.end()), &right) ==
		       player_snapshot_codec_result::ok &&
	       left == right;
}

// Values come from the authenticated complete current SQL cut and the original
// selected literal. Only recorded selected placement effects and frozen target
// weight are reversed. Every resulting byte must authenticate its original
// ordered manifest binding; UIDs/order alone never supply a fabricated BEFORE.
bool cold_original_forests(const shop_trade_payload &payload,
			   const economic_sql_shop_trade_publication &current,
			   std::span<const player_item_snapshot> source,
			   std::vector<player_item_snapshot> &player,
			   std::vector<player_item_snapshot> &keeper)
{
	struct value
	{
		player_item_snapshot item;
		uint64_t parent = 0;
	};
	std::map<uint64_t, value> values;
	for (const auto *image : { &current.whole_player_items, &current.keeper_items })
	{
		std::map<uint64_t, uint64_t> row_ids;
		for (const auto &[uid, row] : *image)
			if (!row.id || !row_ids.emplace(row.id, uid).second)
				return false;
		for (const auto &[uid, row] : *image)
		{
			uint64_t parent = 0;
			if (row.parent_id)
			{
				const auto found = row_ids.find(row.parent_id);
				if (found == row_ids.end())
					return false;
				parent = found->second;
			}
			if (!values.emplace(uid, value{ row.item, parent }).second)
				return false;
		}
	}
	for (size_t index = 0; index < source.size(); ++index)
	{
		const auto &item = source[index];
		if (item.parent_index < -1 || item.parent_index >= static_cast<int32_t>(index))
			return false;
		values[item.object_uid] = { item, item.parent_index < 0 ?
							  0 :
							  source[item.parent_index].object_uid };
	}
	const bool buying = payload.action == shop_trade_action::buy_existing ||
			    payload.action == shop_trade_action::buy_produced;
	if (buying && payload.target_parent_item_uid && payload.native_destination_weight_recorded)
	{
		const auto target = values.find(payload.target_parent_item_uid);
		if (target == values.end())
			return false;
		target->second.item.weight = payload.destination_weight.before;
	}
	const auto reconstruct = [&](const shop_trade_recovery_forest_binding &binding,
				     shop_trade_recovery_forest_role role,
				     std::vector<player_item_snapshot> &output)
	{
		std::vector<player_item_snapshot> candidate;
		std::map<uint64_t, int32_t> positions;
		for (uint64_t uid : binding.ordered_item_uids)
		{
			const auto found = values.find(uid);
			if (found == values.end())
				return false;
			auto item = found->second.item;
			item.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
			if (found->second.parent)
			{
				const auto parent = positions.find(found->second.parent);
				if (parent == positions.end())
					return false;
				item.parent_index = parent->second;
			}
			if (!positions.emplace(uid, static_cast<int32_t>(candidate.size())).second)
				return false;
			candidate.push_back(std::move(item));
		}
		std::vector<uint8_t> bytes;
		if (player_item_snapshot_list_encode(candidate, &bytes) !=
			    player_snapshot_codec_result::ok ||
		    !shop_trade_recovery_forest_verify(bytes, role, binding))
			return false;
		output = std::move(candidate);
		return true;
	};
	return reconstruct(payload.recovery_manifest.player_before,
			   shop_trade_recovery_forest_role::player_before, player) &&
	       reconstruct(payload.recovery_manifest.keeper_before,
			   shop_trade_recovery_forest_role::keeper_before, keeper);
}

bool cold_world(pending_trade &entry, const std::vector<player_item_snapshot> &player,
		const std::vector<player_item_snapshot> &keeper, bool strict, bool actor_absent,
		bool keeper_absent, const economic_sql_shop_trade_publication *row_ids,
		shop_trade_world_cold_observation &observed,
		std::vector<shop_trade_world_uid_expectation> &locations)
{
	std::map<uint64_t, shop_trade_world_uid_expectation> ordered;
	const auto append = [&](const auto &forest, bool npc)
	{
		for (size_t index = 0; index < forest.size(); ++index)
		{
			const auto &item = forest[index];
			if (item.parent_index < -1 ||
			    item.parent_index >= static_cast<int32_t>(index))
				return false;
			const uint64_t parent =
				item.parent_index < 0 ? 0 : forest[item.parent_index].object_uid;
			const bool absent = strict && (npc ? keeper_absent : actor_absent);
			const auto place =
				absent ? shop_trade_world_location::absent :
				parent ? shop_trade_world_location::inside :
				npc    ? (item.equipment_slot ?
						  shop_trade_world_location::keeper_equipment :
						  shop_trade_world_location::keeper_inventory) :
					 (item.equipment_slot ?
						  shop_trade_world_location::actor_equipment :
						  shop_trade_world_location::actor_inventory);
			int32_t id = -1;
			if (row_ids && !absent)
			{
				const auto &image = npc ? row_ids->keeper_items :
							  row_ids->whole_player_items;
				const auto found = image.find(item.object_uid);
				if (found == image.end() || !found->second.id ||
				    found->second.id > INT_MAX)
					return false;
				id = static_cast<int32_t>(found->second.id);
			}
			if (!ordered.emplace(item.object_uid,
					     shop_trade_world_uid_expectation{
						     item.object_uid, place, absent ? 0 : parent,
						     static_cast<int16_t>(
							     absent ? 0 : item.equipment_slot),
						     id })
				     .second)
				return false;
		}
		return true;
	};
	if (!append(player, false) || !append(keeper, true))
		return false;
	for (uint64_t uid : entry.cold->fenced_uids)
		if (!ordered.count(uid))
			ordered.emplace(uid, shop_trade_world_uid_expectation{
						     uid,
						     strict ? shop_trade_world_location::absent :
							      shop_trade_world_location::detached,
						     0, 0, -1 });
	locations.clear();
	for (const auto &[uid, location] : ordered)
		locations.push_back(location);
	// Explicit absence uses an empty REQUEST span only for that absent body:
	// the authenticated full SQL forest remains retained above, and the second
	// complete census must still prove body absence plus every original UID.
	// This never equates an absent body with a present canonical empty body.
	shop_trade_world_cold_request request{
		&entry.payload,
		strict && actor_absent ? std::span<const player_item_snapshot>{} :
					 std::span<const player_item_snapshot>(player),
		strict && keeper_absent ? std::span<const player_item_snapshot>{} :
					  std::span<const player_item_snapshot>(keeper),
		locations
	};
	return shop_trade_world_cold_observe(request, &observed);
}
P_obj cold_object(const shop_trade_world_cold_observation &observed,
		  const std::vector<shop_trade_world_uid_expectation> &locations, uint64_t uid)
{
	const auto found = std::lower_bound(locations.begin(), locations.end(), uid,
					    [](const auto &item, uint64_t value)
					    { return item.uid < value; });
	return found == locations.end() || found->uid != uid ?
		       nullptr :
		       observed.objects[static_cast<size_t>(found - locations.begin())];
}
bool cold_missing_owner(const shop_trade_payload &payload,
			const shop_trade_world_cold_observation &observed,
			const std::vector<shop_trade_world_uid_expectation> &locations,
			const shop_trade_recovery_forest_binding &a,
			const shop_trade_recovery_forest_binding &b)
{
	for (const auto *binding : { &a, &b })
		for (uint64_t uid : binding->ordered_item_uids)
		{
			const bool selected = std::any_of(
				payload.items.begin(), payload.items.begin() + payload.item_count,
				[uid](const auto &item) { return item.item_uid == uid; });
			if (!selected && cold_object(observed, locations, uid))
				return false;
		}
	return true;
}
}
#endif

bool shop_trade_native_publication_owner::cold_prepare_selected(void *opaque) noexcept
{
#ifdef __NO_MYSQL__
	(void)opaque;
	return false;
#else
	if (!opaque || !nevent_is_game_thread())
		return false;
	auto &entry = *static_cast<pending_trade *>(opaque);
	if (!entry.cold || entry.cold->enrollment_started || entry.cold->enrolled)
		return false;
	auto &cold = *entry.cold;
	try
	{
		std::vector<shop_trade_original_item_stage> prepared(cold.source.size());
		std::vector<cold_trade_restore::reload_item> reload(cold.source.size());
		std::map<int, size_t> counts;
		for (size_t index = 0; index < cold.source.size(); ++index)
		{
			const auto &item = cold.source[index];
			const auto *prototype = find_recovery_object_template(item.vnum);
			if (!prototype ||
			    !shop_trade_original_item_stage::prepare(*prototype, item,
								     prepared[index]) ||
			    !prepared[index].object_ || item.parent_index < -1 ||
			    item.parent_index >= static_cast<int32_t>(index) ||
			    (index ? item.parent_index < 0 : item.parent_index != -1) ||
			    item.equipment_slot)
				return false;
			reload[index].prototype = prototype;
			reload[index].proclib_probes.resize(item.extra_descriptions.size());
			++counts[prototype->R_num];
		}
		for (const auto &[number, count] : counts)
			if (!obj_index || number < 0 || number > top_of_objt ||
			    obj_index[number].number < 0 ||
			    count > static_cast<size_t>(INT_MAX - obj_index[number].number))
				return false;
		std::vector<P_obj> objects;
		objects.reserve(prepared.size());
		for (const auto &stage : prepared)
			objects.push_back(stage.object_);
		shop_trade_original_procedure_binding_stage bindings;
		if (!shop_trade_original_procedure_binding_stage::prepare(objects, cold.source,
									  bindings))
			return false;
		// One complete strong allocation precedes any world enrollment. The
		// independent stages own their nodes until all bounds/prototypes pass.
		cold.staged = std::move(prepared);
		cold.reload = std::move(reload);
		cold.bindings = std::move(bindings);
		cold.enrollment_counts = std::move(counts);
		for (size_t index = cold.staged.size(); index-- > 1;)
		{
			P_obj child = cold.staged[index].object_;
			P_obj parent =
				cold.staged[static_cast<size_t>(cold.source[index].parent_index)]
					.object_;
			child->loc_p = LOC_INSIDE;
			child->loc.inside = parent;
			child->next_content = parent->contains;
			parent->contains = child;
		}
		return true;
	}
	catch (...)
	{
		return false;
	}
#endif
}

bool shop_trade_native_publication_owner::cold_enroll_selected(void *opaque) noexcept
{
#ifdef __NO_MYSQL__
	(void)opaque;
	return false;
#else
	if (!opaque || !nevent_is_game_thread())
		return false;
	auto &entry = *static_cast<pending_trade *>(opaque);
	if (!entry.cold)
		return false;
	auto &cold = *entry.cold;
	if (cold.enrollment_started || cold.enrolled || cold.binding_started ||
	    cold.staged.size() != cold.source.size() || cold.reload.size() != cold.source.size() ||
	    !cold.bindings.valid())
		return false;
	for (const auto &stage : cold.staged)
		if (!stage.object_ ||
		    !quest_mobile_native_item_cold_prepend_body_ready(stage.object_))
			return false;
	for (const auto &[number, count] : cold.enrollment_counts)
		if (!obj_index || number < 0 || number > top_of_objt ||
		    obj_index[number].number < 0 ||
		    count > static_cast<size_t>(INT_MAX - obj_index[number].number))
			return false;
	if (!quest_mobile_native_item_cold_prepend_cut_ready(cold.staged.size()))
		return false;
	// The caller re-proved the original held SQL/world cut after ALL allocation.
	// No failure/callback/allocation follows this retained native-effect start.
	cold.enrollment_started = cold.binding_started = true;
	cold.bindings.commit_unchecked();
	cold.binding_returned = true;
	// Allocation-free pointer/count enrollment, no normal constructor, UID
	// issuer, prototype defaults, native callback or source-event adoption.
	for (auto &stage : cold.staged)
	{
		P_obj object = stage.object_;
		quest_mobile_native_item_observe_native_prepend(object);
		object->next = object_list;
		if (object_list)
			object_list->prev = object;
		object_list = object;
		++obj_index[object->R_num].number;
		stage.object_ = nullptr;
		stage.pool_ = nullptr;
		stage.affect_pool_ = nullptr;
	}
	cold.enrolled = true;
	return true;
#endif
}

bool shop_trade_native_publication_owner::cold_publish(const critical_command &command,
						       const critical_completion &sealed,
						       void *opaque) noexcept
{
#ifdef __NO_MYSQL__
	(void)command;
	(void)sealed;
	(void)opaque;
	return false;
#else
	if (!opaque || !nevent_is_game_thread())
		return false;
	auto &entry = *static_cast<pending_trade *>(opaque);
	if (!entry.cold || entry.cold->registration_pending || !entry.cold->command ||
	    entry.blocked || !critical_command_equal(*entry.cold->command, command) ||
	    !critical_completion_disposition_valid(sealed) ||
	    (sealed.disposition != critical_completion_disposition::execution &&
	     sealed.disposition != critical_completion_disposition::never_admitted) ||
	    command.operation_id.bytes != sealed.operation_id.bytes)
		return false;
	const bool never_admitted = sealed.disposition ==
				    critical_completion_disposition::never_admitted;
	const auto retained_matches = [&]()
	{
		return never_admitted ? same_original_refusal(entry.completed, sealed) :
					same_native_receipt(entry.completed, sealed);
	};
	if (!retained_matches())
		return false;
	const auto refusal_produced_cache_absent = [&]()
	{
		if (!never_admitted || entry.payload.action != shop_trade_action::buy_produced)
			return true;
		for (size_t index = 0; index < entry.payload.item_count; ++index)
		{
			item_ownership_runtime_entry cached{};
			if (item_ownership_runtime_lookup(entry.payload.items[index].item_uid,
							  &cached))
				return false;
		}
		return true;
	};
	auto &cold = *entry.cold;
	if ((cold.effect.started && (!cold.effect.returned || !cold.effect.succeeded)) ||
	    (cold.enrollment_started && !cold.enrolled) ||
	    (cold.binding_started && !cold.binding_returned) ||
	    (cold.balances_started && !cold.balances_returned))
		return false;
	for (const auto &reload : cold.reload)
	{
		for (const auto &effect : reload.effects)
			if (effect.started && (!effect.returned || !effect.succeeded))
				return false;
		for (const auto &probe : reload.proclib_probes)
			if (probe.started && (!probe.returned || !probe.succeeded))
				return false;
	}
	try
	{
		MYSQL *connection = sql_pool_acquire();
		player_sql_pool_lease lease(connection);
		if (!connection || player_sql_idle_error(connection))
			return false;
		player_sql_cleanup cleanup;
		player_sql_transaction_cleanup transaction(connection, cleanup);
		transaction.starting();
		bool proven = false;
		try
		{
			if (mysql_real_query(connection, "START TRANSACTION", 17))
				throw EIO;
			economic_sql_shop_trade_publication current;
			if (never_admitted)
			{
				// No receipt/result is accepted or synthesized for this BEFORE cut.
				if (economic_sql_shop_trade_lock_never_admitted_before(
					    connection, command, &current) ||
				    !current.never_admitted || current.rejected)
					throw EAGAIN;
				for (const auto *image :
				     { &current.whole_player_items, &current.keeper_items,
				       &current.player_items })
					for (const auto &[uid, row] : *image)
						if (!row.id || row.id > INT_MAX ||
						    row.parent_id > INT_MAX)
							throw EAGAIN;
				if (!refusal_produced_cache_absent())
					throw EAGAIN;
				cold.refusal_verified = true;
			}
			else
			{
				if (!current_native_image(connection, command, sealed, &current))
					throw EAGAIN;
				entry.native_receipt_verified = true;
			}
			const bool success =
				!never_admitted &&
				(sealed.outcome == critical_apply_outcome::applied ||
				 sealed.outcome == critical_apply_outcome::already_applied);
			shop_trade_result receipt{};
			if (success &&
			    (!shop_trade_command_decode_result(sealed.result_payload.data(),
							       sealed.result_size, &receipt) ||
			     !committed_result_matches(entry, receipt)))
				throw EAGAIN;
			const auto &manifest = entry.payload.recovery_manifest;
			std::vector<player_item_snapshot> player, keeper;
			if (!shop_trade_recovery_image_reconstruct(
				    success ? manifest.player_after : manifest.player_before,
				    success ? shop_trade_recovery_forest_role::player_after :
					      shop_trade_recovery_forest_role::player_before,
				    current.whole_player_items, &player) ||
			    !shop_trade_recovery_image_reconstruct(
				    success ? manifest.keeper_after : manifest.keeper_before,
				    success ? shop_trade_recovery_forest_role::keeper_after :
					      shop_trade_recovery_forest_role::keeper_before,
				    current.keeper_items, &keeper))
				throw EAGAIN;
			if (entry.publication_forests_ready &&
			    (!cold_forest_equal(player, entry.after_player) ||
			     !cold_forest_equal(keeper, entry.after_keeper)))
				throw EAGAIN;
			entry.after_player = std::move(player);
			entry.after_keeper = std::move(keeper);
			entry.publication_forests_ready = true;
			shop_trade_world_cold_observation observed;
			std::vector<shop_trade_world_uid_expectation> locations;
			const auto session = [&]()
			{
				return transaction.same_session() &&
				       mysql_thread_id(connection) == current.session_id &&
				       (connection->server_status & SERVER_STATUS_IN_TRANS) &&
				       !entry.blocked && retained_matches();
			};
			const auto general_observe = [&]()
			{
				return cold_world(entry, entry.after_player, entry.after_keeper,
						  false, false, false, nullptr, observed,
						  locations) &&
				       session();
			};
			const auto refresh_refusal_before = [&]()
			{
				if (!never_admitted)
					return true;
				economic_sql_shop_trade_publication fresh;
				if (!session() ||
				    economic_sql_shop_trade_lock_never_admitted_before(
					    connection, command, &fresh) ||
				    !fresh.never_admitted || fresh.rejected ||
				    fresh.session_id != current.session_id ||
				    fresh.bank_id != current.bank_id ||
				    fresh.keeper_id != current.keeper_id ||
				    fresh.player_save_revision != current.player_save_revision ||
				    fresh.player_level != current.player_level ||
				    fresh.wallet.amount != current.wallet.amount ||
				    fresh.bank.amount != current.bank.amount ||
				    fresh.wallet_revision != current.wallet_revision ||
				    fresh.bank_revision != current.bank_revision ||
				    fresh.keeper_cash != current.keeper_cash ||
				    fresh.shop_revision != current.shop_revision ||
				    fresh.keeper_roaming != current.keeper_roaming ||
				    fresh.payload_checkpoint_recorded !=
					    current.payload_checkpoint_recorded ||
				    fresh.payload_checkpoint_revision !=
					    current.payload_checkpoint_revision ||
				    fresh.player_owner_revision != current.player_owner_revision ||
				    fresh.counterparty_owner_revision !=
					    current.counterparty_owner_revision ||
				    fresh.wallet_owner_revision != current.wallet_owner_revision ||
				    fresh.keeper_owner_revision != current.keeper_owner_revision ||
				    fresh.custody_vnums != current.custody_vnums ||
				    fresh.custody.size() != current.custody.size() ||
				    fresh.authority.lineage.bytes !=
					    current.authority.lineage.bytes ||
				    fresh.authority.epoch.bytes != current.authority.epoch.bytes ||
				    fresh.authority.lineage_revision !=
					    current.authority.lineage_revision ||
				    fresh.authority.mappings.size() !=
					    current.authority.mappings.size())
					return false;
				for (size_t index = 0; index < fresh.authority.mappings.size();
				     ++index)
				{
					const auto &a = fresh.authority.mappings[index];
					const auto &b = current.authority.mappings[index];
					if (!economic_account_key_equal(a.request.account,
									b.request.account) ||
					    a.request.locator_kind != b.request.locator_kind ||
					    a.request.native_id != b.request.native_id ||
					    a.revision != b.revision)
						return false;
				}
				for (size_t index = 0; index < fresh.custody.size(); ++index)
					if (fresh.custody[index].uid !=
						    current.custody[index].uid ||
					    !economic_item_position_equal(
						    fresh.custody[index].position,
						    current.custody[index].position))
						return false;
				const auto same_rows = [](const auto &a, const auto &b)
				{
					if (a.size() != b.size())
						return false;
					for (const auto &[uid, row] : a)
					{
						const auto found = b.find(uid);
						if (found == b.end())
							return false;
						const auto &other = found->second;
						auto literal = row.item, other_literal = other.item;
						if (literal.parent_index !=
						    other_literal.parent_index)
							return false;
						literal.parent_index = other_literal.parent_index =
							PLAYER_SNAPSHOT_NO_PARENT;
						if (row.id != other.id ||
						    row.parent_id != other.parent_id ||
						    row.root_uid != other.root_uid ||
						    row.revision != other.revision ||
						    row.slot != other.slot ||
						    row.payload_present != other.payload_present ||
						    !cold_forest_equal(
							    std::span<const player_item_snapshot>(
								    &literal, 1),
							    std::span<const player_item_snapshot>(
								    &other_literal, 1)))
							return false;
					}
					return true;
				};
				return same_rows(fresh.whole_player_items,
						 current.whole_player_items) &&
				       same_rows(fresh.keeper_items, current.keeper_items) &&
				       same_rows(fresh.player_items, current.player_items) &&
				       session();
			};
			const auto final_observe = [&](bool ids)
			{
				if (!general_observe())
					return false;
				const bool actor_absent = !observed.actor_present,
					   keeper_absent = !observed.keeper_present;
				if (!cold_world(entry, entry.after_player, entry.after_keeper, true,
						actor_absent, keeper_absent,
						ids ? &current : nullptr, observed, locations) ||
				    observed.actor_present == actor_absent ||
				    observed.keeper_present == keeper_absent ||
				    !observed.uid_locations_current_match ||
				    (observed.actor_present &&
				     (!observed.player_current_match ||
				      static_cast<uint32_t>(GET_LEVEL(observed.actor)) !=
					      current.player_level)) ||
				    (observed.keeper_present && !observed.keeper_current_match))
					return false;
				if (entry.payload.target_parent_item_uid && observed.actor_present)
				{
					if (!observed.target_present ||
					    (success ?
						     observed.observed_bindings.live_target_after !=
							     manifest.live_target_after :
						     observed.observed_bindings.live_target_before !=
							     manifest.live_target_before))
						return false;
				}
				return session();
			};
			const auto keeper_cash_matches = [&]()
			{
				if (!observed.keeper)
					return true;
				if (GET_COPPER(observed.keeper) < 0 ||
				    GET_SILVER(observed.keeper) < 0 ||
				    GET_GOLD(observed.keeper) < 0 ||
				    GET_PLATINUM(observed.keeper) < 0)
					return false;
				const int64_t cash =
					static_cast<int64_t>(GET_COPPER(observed.keeper)) +
					10LL * GET_SILVER(observed.keeper) +
					100LL * GET_GOLD(observed.keeper) +
					1000LL * GET_PLATINUM(observed.keeper);
				return cash == current.keeper_cash ||
				       cash == entry.payload.expected_keeper_cash;
			};
			const bool buying = entry.payload.action ==
						    shop_trade_action::buy_existing ||
					    entry.payload.action == shop_trade_action::buy_produced;
			const bool destroying =
				entry.payload.action == shop_trade_action::sell_destroy ||
				entry.payload.action == shop_trade_action::discard_invalid;
			if (!general_observe())
				throw EAGAIN;
			// Original PID/account/race and bound keeper identity are independently
			// observed. Unavailable account data is neither mismatch nor absence.
			if (observed.actor_present &&
			    static_cast<uint32_t>(GET_LEVEL(observed.actor)) !=
				    current.player_level)
				throw EAGAIN;
			if (!observed.actor_present &&
			    !cold_missing_owner(entry.payload, observed, locations,
						manifest.player_before, manifest.player_after))
				throw EAGAIN;
			if (!observed.keeper_present &&
			    !cold_missing_owner(entry.payload, observed, locations,
						manifest.keeper_before, manifest.keeper_after))
				throw EAGAIN;
			if (!keeper_cash_matches())
				throw EAGAIN;
			if (!success && !final_observe(false))
			{
				if (entry.payload.action != shop_trade_action::buy_produced ||
				    !general_observe() || !keeper_cash_matches() ||
				    (observed.actor_present && !observed.player_current_match) ||
				    (observed.keeper_present && !observed.keeper_current_match) ||
				    (!observed.actor_present &&
				     !cold_missing_owner(entry.payload, observed, locations,
							 manifest.player_before,
							 manifest.player_after)) ||
				    (!observed.keeper_present &&
				     !cold_missing_owner(entry.payload, observed, locations,
							 manifest.keeper_before,
							 manifest.keeper_after)))
					throw EAGAIN;
				std::vector<player_item_snapshot> source;
				if (player_item_snapshot_list_decode(entry.payload.item_blob.data(),
								     entry.payload.item_blob_size,
								     &source) !=
				    player_snapshot_codec_result::ok)
					throw EAGAIN;
				P_obj selected = cold_object(observed, locations,
							     entry.payload.selected_item_uid);
				if (selected)
				{
					if (!OBJ_NOWHERE(selected) || selected->next_content ||
					    !cold_forest_equal(observed.selected_items, source) ||
					    !session() ||
					    (cold.effect.started &&
					     (!cold.effect.returned || !cold.effect.succeeded)))
						throw EAGAIN;
					cold.effect = {};
					cold.effect.step =
						shop_trade_cold_native_step::reject_produced;
					if (!shop_trade_cold_native_step_execute(
						    observed.actor, observed.keeper, selected,
						    nullptr, entry.payload, cold.effect))
						throw EAGAIN;
				}
				// Already-absent originals take the same complete global proof. Never
				// manufacture a completion, inventory, trade callback or SQL holding.
				if (!final_observe(false) || !keeper_cash_matches())
					throw EAGAIN;
			}
			if (!final_observe(false))
			{
				if (!success || !general_observe())
					throw EAGAIN;
				if (!cold.initialized)
				{
					if (player_item_snapshot_list_decode(
						    entry.payload.item_blob.data(),
						    entry.payload.item_blob_size, &cold.source) !=
						    player_snapshot_codec_result::ok ||
					    !shop_trade_accounted_after_items(
						    entry.payload, &cold.selected_after) ||
					    !cold_original_forests(
						    entry.payload, current, cold.source,
						    cold.original_player, cold.original_keeper))
						throw EAGAIN;
					if ((observed.actor_present &&
					     !cold_forest_equal(observed.player_items,
								cold.original_player) &&
					     !cold_forest_equal(observed.player_items,
								entry.after_player)) ||
					    (observed.keeper_present &&
					     !cold_forest_equal(observed.keeper_items,
								cold.original_keeper) &&
					     !cold_forest_equal(observed.keeper_items,
								entry.after_keeper)))
						throw EAGAIN;
					if (!observed.selected_items.empty() &&
					    !cold_forest_equal(observed.selected_items,
							       cold.source) &&
					    !cold_forest_equal(observed.selected_items,
							       cold.selected_after))
						throw EAGAIN;
					P_obj selected =
						cold_object(observed, locations,
							    entry.payload.selected_item_uid);
					// A foreign startup detached stage cannot invent normally returned
					// transfer tails. Original produced BEFORE is actually detached.
					if (selected && OBJ_NOWHERE(selected) &&
					    entry.payload.action != shop_trade_action::buy_produced)
						throw EAGAIN;
					cold.actor_present = observed.actor_present;
					cold.keeper_present = observed.keeper_present;
					cold.actor_runtime_id = observed.actor_runtime_id;
					cold.keeper_runtime_id = observed.keeper_runtime_id;
					cold.working_player = observed.actor_present ?
								      observed.player_items :
								      entry.after_player;
					cold.working_keeper = observed.keeper_present ?
								      observed.keeper_items :
								      entry.after_keeper;
					cold.working_target = observed.target_items;
					if (entry.payload.target_parent_item_uid &&
					    observed.actor_present &&
					    (!observed.target_present ||
					     observed.observed_bindings.live_target_before !=
						     manifest.live_target_before))
						throw EAGAIN;
					cold.initialized = true;
				}
				const auto working_observe = [&]()
				{
					if (!cold_world(entry, cold.working_player,
							cold.working_keeper, false, false, false,
							nullptr, observed, locations) ||
					    !session() || !keeper_cash_matches() ||
					    observed.actor_present != cold.actor_present ||
					    observed.keeper_present != cold.keeper_present ||
					    observed.actor_runtime_id != cold.actor_runtime_id ||
					    observed.keeper_runtime_id != cold.keeper_runtime_id ||
					    (observed.actor_present &&
					     !cold_forest_equal(observed.player_items,
								cold.working_player)) ||
					    (observed.keeper_present &&
					     !cold_forest_equal(observed.keeper_items,
								cold.working_keeper)) ||
					    (entry.payload.target_parent_item_uid &&
					     observed.actor_present &&
					     (!observed.target_present ||
					      !cold_forest_equal(observed.target_items,
								 cold.working_target))))
						return false;
					return (observed.actor_present ||
						cold_missing_owner(entry.payload, observed,
								   locations,
								   manifest.player_before,
								   manifest.player_after)) &&
					       (observed.keeper_present ||
						cold_missing_owner(entry.payload, observed,
								   locations,
								   manifest.keeper_before,
								   manifest.keeper_after));
				};
				// Every handler is surrounded by a complete new census. No pointer
				// survives a callback or pulse; only exact expected values persist.
				for (unsigned int leg = 0; leg < 6 && !final_observe(false); ++leg)
				{
					if (!working_observe() ||
					    !shop_trade_current_runtime_owner::publish(
						    connection, entry.payload, current) ||
					    !working_observe())
						throw EAGAIN;
					if (cold.effect.started &&
					    (!cold.effect.returned || !cold.effect.succeeded))
						throw EAGAIN;
					cold.effect = {};
					P_obj selected =
						cold_object(observed, locations,
							    entry.payload.selected_item_uid);
					P_obj target =
						cold_object(observed, locations,
							    entry.payload.target_parent_item_uid);
					const bool destination_absent =
						buying ? !observed.actor_present :
							 !observed.keeper_present;
					if (!selected)
					{
						if (destroying || destination_absent ||
						    cold.enrolled)
							throw EAGAIN;
						for (const auto &item : cold.source)
							if (cold_object(observed, locations,
									item.object_uid))
								throw EAGAIN;
						if (!session() || !cold_prepare_selected(&entry) ||
						    !working_observe() || !session() ||
						    !cold_enroll_selected(&entry) ||
						    !working_observe())
							throw EAGAIN;
						selected = cold_object(
							observed, locations,
							entry.payload.selected_item_uid);
						if (!selected ||
						    !cold_forest_equal(observed.selected_items,
								       cold.source))
							throw EAGAIN;
					}
					if (cold.enrolled)
					{
						if (cold.reload.size() != cold.source.size())
							throw EAGAIN;
						for (size_t index = 0; index < cold.reload.size();
						     ++index)
							for (unsigned int step = 0; step < 4;
							     ++step)
							{
								auto &reload = cold.reload[index];
								auto &effect = reload.effects[step];
								if (effect.started)
								{
									if (!effect.returned ||
									    !effect.succeeded)
										throw EAGAIN;
									continue;
								}
								if (!working_observe() ||
								    !session() ||
								    !reload.prototype ||
								    !cold_forest_equal(
									    observed.selected_items,
									    cold.source))
									throw EAGAIN;
								P_obj object = cold_object(
									observed, locations,
									cold.source[index]
										.object_uid);
								if (step == 1)
									effect.periodic =
										reload.effects[0]
											.periodic;
								if (step == 2)
								{
									if (reload.proclib_probes
										    .size() !=
									    cold.source[index]
										    .extra_descriptions
										    .size())
										throw EAGAIN;
									bool periodic = false;
									for (size_t description = 0;
									     description <
									     reload.proclib_probes
										     .size();
									     ++description)
									{
										auto &probe =
											reload.proclib_probes
												[description];
										if (probe.started)
										{
											if (!probe.returned ||
											    !probe.succeeded)
												throw EAGAIN;
										}
										else
										{
											if (!working_observe() ||
											    !session() ||
											    !cold_forest_equal(
												    observed.selected_items,
												    cold.source))
												throw EAGAIN;
											P_obj fresh = cold_object(
												observed,
												locations,
												cold.source[index]
													.object_uid);
											if (!fresh ||
											    !shop_trade_original_item_stage::proclib_probe(
												    fresh,
												    *reload.prototype,
												    description,
												    probe) ||
											    !working_observe() ||
											    !cold_forest_equal(
												    observed.selected_items,
												    cold.source))
												throw EAGAIN;
										}
										periodic =
											periodic ||
											probe.periodic;
									}
									effect.periodic = periodic;
									// Reacquire after every probe; no object pointer crosses its callback.
									if (!working_observe() ||
									    !session())
										throw EAGAIN;
									object = cold_object(
										observed, locations,
										cold.source[index]
											.object_uid);
								}
								if (!object ||
								    !shop_trade_original_item_stage::
									    reload_step(
										    object,
										    *reload.prototype,
										    step, effect) ||
								    !working_observe() ||
								    !cold_forest_equal(
									    observed.selected_items,
									    cold.source))
									throw EAGAIN;
							}
						// No selected/target pointer survives any reload callback.
						if (!working_observe())
							throw EAGAIN;
						selected = cold_object(
							observed, locations,
							entry.payload.selected_item_uid);
						target = cold_object(
							observed, locations,
							entry.payload.target_parent_item_uid);
						if (!selected)
							throw EAGAIN;
					}
					if (!cold_forest_equal(observed.selected_items,
							       cold.source) &&
					    !cold_forest_equal(observed.selected_items,
							       cold.selected_after))
						throw EAGAIN;
					auto next_player = cold.working_player;
					auto next_keeper = cold.working_keeper;
					auto next_target = cold.working_target;
					bool next_nesting = cold.for_nesting;
					const bool on_actor =
						observed.actor &&
						OBJ_CARRIED_BY(selected, observed.actor);
					const bool on_keeper =
						observed.keeper &&
						OBJ_CARRIED_BY(selected, observed.keeper);
					if (destroying || destination_absent)
					{
						if ((destroying &&
						     !(entry.payload.action ==
								       shop_trade_action::
									       discard_invalid ?
							       on_keeper :
							       on_actor)) ||
						    (!destroying && !(on_actor || on_keeper ||
								      OBJ_NOWHERE(selected))))
							throw EAGAIN;
						if (on_actor && !shop_remove_selected(next_player,
										      entry.payload,
										      &next_player))
							throw EAGAIN;
						if (on_keeper && !shop_remove_selected(
									 next_keeper, entry.payload,
									 &next_keeper))
							throw EAGAIN;
						cold.effect.step =
							destroying ?
								shop_trade_cold_native_step::destroy :
								shop_trade_cold_native_step::
									retire_copy;
					}
					else if (on_actor || on_keeper)
					{
						if (buying && on_actor &&
						    !entry.payload.target_parent_item_uid)
							throw EAGAIN;
						if (!buying && on_keeper)
							throw EAGAIN;
						if (on_actor && !shop_remove_selected(next_player,
										      entry.payload,
										      &next_player))
							throw EAGAIN;
						if (on_keeper && !shop_remove_selected(
									 next_keeper, entry.payload,
									 &next_keeper))
							throw EAGAIN;
						next_nesting = buying && on_actor &&
							       entry.payload.target_parent_item_uid;
						cold.effect.step =
							shop_trade_cold_native_step::detach;
					}
					else if (OBJ_NOWHERE(selected))
					{
						if (buying && cold.for_nesting)
						{
							std::vector<player_item_snapshot> values;
							std::vector<uint8_t> original_target;
							if (!target ||
							    !expected_player_forest(
								    entry.payload,
								    cold.working_player, false,
								    &values) ||
							    !expected_player_order(
								    entry.payload, observed.actor,
								    selected, target, values,
								    &next_player) ||
							    player_item_snapshot_list_encode(
								    cold.working_target,
								    &original_target) !=
								    player_snapshot_codec_result::ok ||
							    !expected_destination_forest(
								    entry.payload, observed.actor,
								    selected, target,
								    original_target,
								    &next_target) ||
							    !cold_forest_equal(next_player,
									       entry.after_player))
								throw EAGAIN;
							cold.effect.step =
								shop_trade_cold_native_step::nest;
						}
						else if (buying)
						{
							auto root_destination = entry.payload;
							root_destination.target_parent_item_uid = 0;
							root_destination.target_root_item_uid =
								root_destination.selected_item_uid;
							root_destination
								.expected_target_parent_revision =
								0;
							root_destination
								.native_destination_weight_recorded =
								false;
							root_destination.destination_weight = {};
							root_destination.recovery_manifest_recorded =
								false;
							root_destination.recovery_manifest = {};
							std::vector<player_item_snapshot> values;
							if (!expected_player_forest(
								    root_destination,
								    cold.working_player, false,
								    &values) ||
							    !expected_player_order(root_destination,
										   observed.actor,
										   selected,
										   nullptr, values,
										   &next_player) ||
							    (!entry.payload.target_parent_item_uid &&
							     !cold_forest_equal(next_player,
										entry.after_player)))
								throw EAGAIN;
							cold.effect.step =
								shop_trade_cold_native_step::
									place_player;
						}
						else
						{
							if (!expected_keeper_forest(
								    entry.payload, observed.keeper,
								    selected, cold.working_keeper,
								    false, &next_keeper) ||
							    !cold_forest_equal(next_keeper,
									       entry.after_keeper))
								throw EAGAIN;
							cold.effect.step =
								shop_trade_cold_native_step::
									place_keeper;
						}
					}
					else
						throw EAGAIN;
					if (!session() ||
					    !shop_trade_cold_native_step_execute(
						    observed.actor, observed.keeper, selected,
						    target, entry.payload, cold.effect))
						throw EAGAIN;
					// All candidate copies allocated before the first effect; moves
					// retain the exact next graph even if its fresh census refuses.
					cold.working_player = std::move(next_player);
					cold.working_keeper = std::move(next_keeper);
					cold.working_target = std::move(next_target);
					cold.for_nesting = next_nesting;
					if (!working_observe())
						throw EAGAIN;
				}
				if (!final_observe(false))
					throw EAGAIN;
			}
			if (!refresh_refusal_before() || !final_observe(false))
				throw EAGAIN;
			if (!refusal_produced_cache_absent())
				throw EAGAIN;
			if (!shop_trade_current_runtime_owner::publish(connection, entry.payload,
								       current) ||
			    !final_observe(false))
				throw EAGAIN;
			for (int64_t amount : current.wallet.amount)
				if (amount < 0 || amount > INT_MAX)
					throw EAGAIN;
			for (int64_t amount : current.bank.amount)
				if (amount < 0 || amount > INT_MAX)
					throw EAGAIN;
			if (observed.actor &&
			    (observed.actor->only.pc->wallet_revision > current.wallet_revision ||
			     observed.actor->only.pc->bank_revision > current.bank_revision))
				throw EAGAIN;
			{
				// Normally returned current projections may repeat with fresh
				// bodies/current revisions. An unreturned previous call never does.
				cold.balances_started = true;
				cold.balances_returned = false;
				if (observed.actor)
				{
					if (!currency_transaction_publish_balances(
						    observed.actor,
						    entry.payload.account_name.data(),
						    entry.payload.racewar, current.wallet,
						    current.bank, current.wallet_revision,
						    current.bank_revision))
						throw EAGAIN;
				}
				else
				{
					const AccountBankBalances bank_values{
						static_cast<int>(current.bank.amount[0]),
						static_cast<int>(current.bank.amount[1]),
						static_cast<int>(current.bank.amount[2]),
						static_cast<int>(current.bank.amount[3])
					};
					publish_account_bank_balances_revision(
						entry.payload.account_name.data(),
						entry.payload.racewar, &bank_values,
						current.bank_revision);
				}
				cold.balances_returned = true;
			}
			if (!final_observe(false))
				throw EAGAIN;
			if (observed.keeper)
			{
				const int64_t cash =
					static_cast<int64_t>(GET_COPPER(observed.keeper)) +
					10LL * GET_SILVER(observed.keeper) +
					100LL * GET_GOLD(observed.keeper) +
					1000LL * GET_PLATINUM(observed.keeper);
				if (cash != current.keeper_cash &&
				    cash != entry.payload.expected_keeper_cash)
					throw EAGAIN;
				int64_t remainder = current.keeper_cash;
				GET_PLATINUM(observed.keeper) = static_cast<int>(remainder / 1000);
				remainder %= 1000;
				GET_GOLD(observed.keeper) = static_cast<int>(remainder / 100);
				remainder %= 100;
				GET_SILVER(observed.keeper) = static_cast<int>(remainder / 10);
				GET_COPPER(observed.keeper) = static_cast<int>(remainder % 10);
			}
			// Only fresh final locations receive signed native row IDs. Missing
			// bodies have explicit full global absence requirements, never an
			// empty invented inventory or a created NPC/player body.
			for (size_t index = 0; index < locations.size(); ++index)
			{
				P_obj object = observed.objects[index];
				if (!object)
					continue;
				const auto player_row =
					current.whole_player_items.find(locations[index].uid);
				const auto keeper_row =
					current.keeper_items.find(locations[index].uid);
				const uint64_t id = player_row != current.whole_player_items.end() ?
							    player_row->second.id :
						    keeper_row != current.keeper_items.end() ?
							    keeper_row->second.id :
							    0;
				if (!id || id > INT_MAX)
					throw EAGAIN;
				object->db_item_id = static_cast<int>(id);
			}
			if (!refresh_refusal_before() || !refusal_produced_cache_absent() ||
			    !final_observe(true) || !session())
				throw EAGAIN;
			proven = !observed.actor ||
				 (observed.actor->only.pc->wallet_revision ==
					  current.wallet_revision &&
				  observed.actor->only.pc->bank_revision == current.bank_revision &&
				  current.wallet.amount ==
					  std::array<int64_t, 4>{ GET_COPPER(observed.actor),
								  GET_SILVER(observed.actor),
								  GET_GOLD(observed.actor),
								  GET_PLATINUM(observed.actor) } &&
				  current.bank.amount ==
					  std::array<int64_t, 4>{
						  GET_BALANCE_COPPER(observed.actor),
						  GET_BALANCE_SILVER(observed.actor),
						  GET_BALANCE_GOLD(observed.actor),
						  GET_BALANCE_PLATINUM(observed.actor) });
			if (observed.keeper)
				proven = proven &&
					 current.keeper_cash ==
						 static_cast<int64_t>(GET_COPPER(observed.keeper)) +
							 10LL * GET_SILVER(observed.keeper) +
							 100LL * GET_GOLD(observed.keeper) +
							 1000LL * GET_PLATINUM(observed.keeper);
		}
		catch (...)
		{
			proven = false;
		}
		transaction.finish();
		if (!cleanup.rollback_confirmed || cleanup.cleanup_error ||
		    cleanup.disposition != player_sql_cleanup_disposition::idle_verified)
			return false;
		lease.reuse(cleanup);
		return proven && !entry.blocked && retained_matches();
	}
	catch (...)
	{
		return false;
	}
#endif
}

bool shop_trade_native_publication_owner::native_publish(const critical_command &command,
							 const critical_completion &sealed,
							 void *opaque) noexcept
{
#ifdef __NO_MYSQL__
	(void)command;
	(void)sealed;
	(void)opaque;
	return false;
#else
	if (!opaque || !nevent_is_game_thread())
		return false;
	auto &entry = *static_cast<pending_trade *>(opaque);
	if (entry.cold)
		return cold_publish(command, sealed, opaque);
	if (!entry.preparation || !entry.preparation->command || entry.blocked ||
	    !same_native_receipt(entry.completed, sealed) ||
	    command.operation_id.bytes != sealed.operation_id.bytes || !entry.accounted_publication)
		return false;
	try
	{
		auto &prepared = *entry.preparation;
		MYSQL *connection = sql_pool_acquire();
		player_sql_pool_lease lease(connection);
		if (!connection || player_sql_idle_error(connection))
			return false;
		player_sql_cleanup cleanup;
		player_sql_transaction_cleanup transaction(connection, cleanup);
		transaction.starting();
		bool proven = false;
		try
		{
			if (mysql_real_query(connection, "START TRANSACTION", 17))
				throw EIO;
			economic_sql_shop_trade_publication current;
			if (!current_native_image(connection, command, sealed,
						  prepared.player_token, &current))
				throw EAGAIN;
			entry.native_receipt_verified = true;
			const bool success = sealed.outcome == critical_apply_outcome::applied ||
					     sealed.outcome ==
						     critical_apply_outcome::already_applied;
			std::vector<player_item_snapshot> original, source;
			player_shop_checkpoint_stage held{};
			if (!player_save_shop_checkpoint_owner::original_held_body(
				    prepared.player_token, command, &original, &held) ||
			    held.save_revision != entry.payload.expected_player_save_revision ||
			    held.level != entry.payload.expected_player_level ||
			    player_item_snapshot_list_decode(
				    entry.payload.item_blob.data(), entry.payload.item_blob_size,
				    &source) != player_snapshot_codec_result::ok)
				throw EAGAIN;
			const bool produced = entry.payload.action ==
					      shop_trade_action::buy_produced;
			const bool buying = produced ||
					    entry.payload.action == shop_trade_action::buy_existing;
			const bool selling =
				entry.payload.action == shop_trade_action::sell_store ||
				entry.payload.action == shop_trade_action::sell_destroy;
			const bool cleanup_action = entry.payload.action ==
						    shop_trade_action::discard_invalid;
			std::vector<player_item_snapshot> staged =
				produced ? source : std::vector<player_item_snapshot>{};
			shop_trade_world_witness observed;
			if (!entry.publication_forests_ready)
			{
				// This original full BEFORE census is indispensable. A matching
				// target graph alone cannot invent completed native handler tails.
				if (!shop_world(entry, original, prepared.keeper_items, staged,
						nullptr, false, &observed))
					throw EAGAIN;
				P_obj selected = shop_witness_object(
					observed, entry.payload.selected_item_uid);
				P_obj destination = shop_witness_object(
					observed, entry.payload.target_parent_item_uid);
				if (entry.payload.native_destination_weight_recorded && destination)
				{
					std::vector<uint8_t> literal;
					if (!preparation_destination_tree(destination, &literal) ||
					    literal != prepared.destination)
						throw EAGAIN;
				}
				std::vector<player_item_snapshot> player, keeper, selected_after;
				if (!selected ||
				    !expected_player_forest(entry.payload, original, !success,
							    &player) ||
				    !shop_trade_accounted_after_items(entry.payload,
								      &selected_after))
					throw EAGAIN;
				if (!expected_keeper_forest(entry.payload, observed.keeper,
							    selected, prepared.keeper_items,
							    !success, &keeper))
					throw EAGAIN;
				if (success && buying)
				{
					std::vector<player_item_snapshot> ordered;
					if (!expected_player_order(entry.payload, observed.actor,
								   selected, destination, player,
								   &ordered))
						throw EAGAIN;
					player = std::move(ordered);
				}
				if (!shop_image_matches(player, current.whole_player_items) ||
				    !shop_image_matches(keeper, current.keeper_items))
					throw EAGAIN;
				if (success && entry.payload.native_destination_weight_recorded &&
				    destination)
				{
					std::vector<player_item_snapshot> literal;
					if (!expected_destination_forest(
						    entry.payload, observed.actor, selected,
						    destination, prepared.destination, &literal) ||
					    player_item_snapshot_list_encode(
						    literal, &entry.after_destination) !=
						    player_snapshot_codec_result::ok)
						throw EAGAIN;
				}
				// All allocation-backed retained copies precede callbacks.
				entry.before_player = std::move(original);
				entry.after_player = std::move(player);
				entry.after_keeper = std::move(keeper);
				entry.publication_forests_ready = true;
			}
			else if (!shop_image_matches(entry.after_player,
						     current.whole_player_items) ||
				 !shop_image_matches(entry.after_keeper, current.keeper_items))
				throw EAGAIN;
			const auto destination_matches = [&](bool after)
			{
				if (!entry.payload.native_destination_weight_recorded ||
				    !entry.payload.target_parent_item_uid)
					return true;
				P_obj destination = shop_witness_object(
					observed, entry.payload.target_parent_item_uid);
				std::vector<uint8_t> literal;
				const auto &expected = after ? entry.after_destination :
							       prepared.destination;
				return destination && !expected.empty() &&
				       preparation_destination_tree(destination, &literal) &&
				       literal == expected;
			};
			const auto observe_final = [&](bool row_ids)
			{
				return shop_world(entry, entry.after_player, entry.after_keeper,
						  (!success && produced) ?
							  staged :
							  std::vector<player_item_snapshot>{},
						  row_ids ? &current : nullptr, true, &observed) &&
				       destination_matches(success);
			};
			if (entry.physical_published || (entry.physical_stages & 32U) ||
			    (success && (entry.physical_stages & 4096U) &&
			     (entry.physical_stages & 131072U)))
			{
				if (!observe_final(false))
					throw EAGAIN;
			}
			else if (!shop_world(entry, entry.before_player, prepared.keeper_items,
					     staged, nullptr, false, &observed))
			{
				// A normally returned departure can resume placement. Incomplete
				// started handler tails remain held by the physical callback's
				// explicit stage checks; topology cannot discharge them.
				std::vector<player_item_snapshot> player = entry.before_player;
				std::vector<player_item_snapshot> keeper = prepared.keeper_items;
				if (selling &&
				    !shop_remove_selected(player, entry.payload, &player))
					throw EAGAIN;
				if ((buying && !produced) || cleanup_action)
					if (!shop_remove_selected(keeper, entry.payload, &keeper))
						throw EAGAIN;
				auto detached = source;
				if (buying && (entry.physical_stages & 65536U))
					detached.front().extra2_flags |= ITEM2_STOREITEM;
				if (buying && (entry.physical_stages & 1024U) &&
				    !shop_trade_accounted_after_items(entry.payload, &detached))
					throw EAGAIN;
				bool resumed = (entry.physical_stages & 65536U) &&
					       shop_world(entry, player, keeper, detached, nullptr,
							  false, &observed);
				if (!resumed && buying && entry.payload.target_parent_item_uid &&
				    (entry.physical_stages & 1024U))
				{
					// obj_to_char returned before the existing nesting leg. Its
					// temporary carried-root placement has the same native grouping
					// policy; never pretend it is the final container placement.
					auto temporary = entry.payload;
					temporary.target_parent_item_uid = 0;
					temporary.target_root_item_uid =
						temporary.selected_item_uid;
					temporary.expected_target_parent_revision = 0;
					temporary.destination_weight = {};
					std::vector<player_item_snapshot> values, ordered;
					P_char actor = find_character_by_runtime_id(
						prepared.actor_runtime_id);
					P_obj selected = nullptr;
					// Bounded temporary carrying observation resolves the pointer;
					// the subsequent full world census must still prove uniqueness.
					if (expected_player_forest(temporary, entry.before_player,
								   false, &values))
					{
						// The ordering helper checks the complete target chain;
						// the following full census must prove every alias surface.
						std::set<P_obj> seen;
						for (P_obj object = actor ? actor->carrying :
									    nullptr;
						     object; object = object->next_content)
						{
							if (seen.size() >=
								    PLAYER_SNAPSHOT_MAX_OBJECTS ||
							    !seen.insert(object).second ||
							    !OBJ_CARRIED_BY(object, actor))
								throw EAGAIN;
							if (object->obj_uid ==
							    temporary.selected_item_uid)
							{
								selected = object;
								break;
							}
						}
						if (selected && expected_player_order(
									temporary, actor, selected,
									nullptr, values, &ordered))
							resumed = shop_world(entry, ordered, keeper,
									     {}, nullptr, false,
									     &observed);
					}
				}
				if (!resumed)
					throw EAGAIN;
			}
			// Before nesting, bind every original literal child. Normally
			// returned nesting instead requires the retained complete afterimage.
			if (!destination_matches(success && (entry.physical_stages & 4096U) &&
						 (entry.physical_stages & 131072U)))
				throw EAGAIN;
			if (!shop_actor_matches(observed.actor, observed.keeper, entry) ||
			    entry.blocked || !same_native_receipt(entry.completed, sealed) ||
			    !transaction.same_session())
				throw EAGAIN;
			shop_trade_result projected{};
			if (!shop_trade_command_decode_result(sealed.result_payload.data(),
							      sealed.result_size, &projected))
				throw EAGAIN;
			projected.wallet = current.wallet;
			projected.bank = current.bank;
			projected.wallet_revision = current.wallet_revision;
			projected.bank_revision = current.bank_revision;
			projected.keeper_cash = current.keeper_cash;
			projected.keeper_cash_recorded = true;
			projected.shop_revision = current.shop_revision;
			projected.player_owner_revision = current.player_owner_revision;
			projected.counterparty_owner_revision = current.counterparty_owner_revision;
			// Checked native placement consumes CURRENT destination custody. Publish
			// this original-session complete cut after retaining the original world
			// and ordered AFTER values, before any placement/extraction callback.
			// Keep the final full runtime/world verification below before guarded ACK.
			if (!shop_trade_current_runtime_owner::publish(connection, entry.payload,
								       current))
				throw EAGAIN;
			if (success)
			{
				// Retained old callback owns audit/notice/native handler stages.
				// It receives only current value projections, never a replacement
				// receipt; SQL authority and the original reservation remain held.
				const bool returned = entry.accounted_publication(
					observed.actor, observed.keeper, projected, entry.payload,
					entry.physical_stages);
				if (returned)
					entry.physical_published = true;
				if (!returned || entry.blocked ||
				    !same_native_receipt(entry.completed, sealed) ||
				    !observe_final(false))
					throw EAGAIN;
			}
			else if (!observe_final(false))
				throw EAGAIN;
			// Reobserve both original bodies after every callback. No raw body or
			// object pointer survives a pulse, and no historical cash is restored.
			if (!shop_actor_matches(observed.actor, observed.keeper, entry) ||
			    observed.actor->only.pc->wallet_revision > current.wallet_revision ||
			    observed.actor->only.pc->bank_revision > current.bank_revision ||
			    !currency_transaction_publish_balances(
				    observed.actor, entry.payload.account_name.data(),
				    entry.payload.racewar, current.wallet, current.bank,
				    current.wallet_revision, current.bank_revision) ||
			    entry.blocked || !same_native_receipt(entry.completed, sealed) ||
			    !observe_final(false))
				throw EAGAIN;
			int64_t cash = current.keeper_cash;
			GET_PLATINUM(observed.keeper) = static_cast<int>(cash / 1000);
			cash %= 1000;
			GET_GOLD(observed.keeper) = static_cast<int>(cash / 100);
			cash %= 100;
			GET_SILVER(observed.keeper) = static_cast<int>(cash / 10);
			GET_COPPER(observed.keeper) = static_cast<int>(cash % 10);
			// Checked signed native row IDs are assigned only to the freshly
			// censused final physical graph; no old row ID becomes custody proof.
			for (P_obj object : observed.objects)
				if (object)
				{
					const auto player_row =
						current.whole_player_items.find(object->obj_uid);
					const auto keeper_row =
						current.keeper_items.find(object->obj_uid);
					const uint64_t row_id =
						player_row != current.whole_player_items.end() ?
							player_row->second.id :
						keeper_row != current.keeper_items.end() ?
							keeper_row->second.id :
							0;
					if (!row_id && !success && produced &&
					    std::any_of(source.begin(), source.end(),
							[&](const auto &item) {
								return item.object_uid ==
								       object->obj_uid;
							}))
						continue; // Original rejected staged output is not a native holding.
					if (!row_id || row_id > INT_MAX)
						throw EAGAIN;
					object->db_item_id = static_cast<int>(row_id);
				}
			if (!shop_trade_current_runtime_owner::publish(connection, entry.payload,
								       current) ||
			    !observe_final(true) || entry.blocked ||
			    !same_native_receipt(entry.completed, sealed) ||
			    !transaction.same_session() ||
			    mysql_thread_id(connection) != current.session_id ||
			    !(connection->server_status & SERVER_STATUS_IN_TRANS))
				throw EAGAIN;
			const auto actor = observed.actor;
			proven = actor->only.pc->wallet_revision == current.wallet_revision &&
				 actor->only.pc->bank_revision == current.bank_revision &&
				 GET_COPPER(actor) == current.wallet.amount[0] &&
				 GET_SILVER(actor) == current.wallet.amount[1] &&
				 GET_GOLD(actor) == current.wallet.amount[2] &&
				 GET_PLATINUM(actor) == current.wallet.amount[3] &&
				 current.bank.amount ==
					 std::array<int64_t, 4>{ GET_BALANCE_COPPER(actor),
								 GET_BALANCE_SILVER(actor),
								 GET_BALANCE_GOLD(actor),
								 GET_BALANCE_PLATINUM(actor) } &&
				 current.keeper_cash ==
					 static_cast<int64_t>(GET_COPPER(observed.keeper)) +
						 10LL * GET_SILVER(observed.keeper) +
						 100LL * GET_GOLD(observed.keeper) +
						 1000LL * GET_PLATINUM(observed.keeper);
		}
		catch (...)
		{
			proven = false;
		}
		transaction.finish();
		if (!cleanup.rollback_confirmed || cleanup.cleanup_error ||
		    cleanup.disposition != player_sql_cleanup_disposition::idle_verified)
			return false;
		lease.reuse(cleanup);
		return proven && !entry.blocked && same_native_receipt(entry.completed, sealed);
	}
	catch (...)
	{
		return false;
	}
#endif
}

bool shop_trade_transaction_restore_replayed_command(const critical_command &original) noexcept
{
	if (persistence_mode_get() == PERSISTENCE_MODE_FLATFILE_PRIMARY)
		return shop_trade_native_publication_owner::restore_flat(original);
#ifdef __NO_MYSQL__
	(void)original;
	return false;
#else
	try
	{
		if (!nevent_is_game_thread() ||
		    persistence_mode_get() == PERSISTENCE_MODE_FLATFILE_PRIMARY ||
		    !original.publication_required ||
		    original.type != critical_command_type::shop_trade ||
		    original.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION ||
		    original.payload_version != SHOP_TRADE_RECOVERY_MANIFEST_VERSION ||
		    !critical_command_envelope_valid(original))
			return false;
		economic_frozen_intent intent;
		shop_trade_payload payload{};
		economic_account_key wallet{}, bank{}, keeper{};
		if (shop_trade_accounting_decode(original, &intent, &payload, &wallet, &bank,
						 &keeper) != economic_accounting_error::ok ||
		    !payload.recovery_manifest_recorded || !payload.player_pid ||
		    payload.player_pid > INT_MAX)
			return false;
		auto found = pending.find(original.operation_id.bytes);
		if (found != pending.end())
		{
			// Exact registration retries never replace sealed completion, native
			// effect state, conflict flags or the first immutable command.
			if (!found->second.cold || !found->second.cold->command ||
			    !critical_command_equal(*found->second.cold->command, original) ||
			    !player_save_pipeline_restore_sql_shop_obligation(original))
				return false;
			found->second.cold->registration_pending = false;
			return true;
		}
		if (pending.size() + notifying >= SHOP_TRADE_PENDING_MAX ||
		    player_pending(payload.player_pid) ||
		    shop_trade_transaction_keeper_busy(payload.shop_id))
			return false;
		pending_trade entry;
		entry.player_pid = payload.player_pid;
		entry.payload = payload;
		entry.cold = std::make_unique<cold_trade_restore>();
		entry.cold->command = std::make_unique<critical_command>(original);
		const auto &manifest = payload.recovery_manifest;
		for (const auto *binding :
		     { &manifest.player_before, &manifest.player_after, &manifest.keeper_before,
		       &manifest.keeper_after, &manifest.live_target_before,
		       &manifest.live_target_after })
			entry.cold->fenced_uids.insert(entry.cold->fenced_uids.end(),
						       binding->ordered_item_uids.begin(),
						       binding->ordered_item_uids.end());
		for (size_t index = 0; index < payload.item_count; ++index)
			entry.cold->fenced_uids.push_back(payload.items[index].item_uid);
		auto &uids = entry.cold->fenced_uids;
		std::sort(uids.begin(), uids.end());
		uids.erase(std::unique(uids.begin(), uids.end()), uids.end());
		// Every allocation and domain insertion precedes the typed shared hold.
		// Hold registration doubt keeps the original map node nonpublishable.
		const auto inserted =
			pending.emplace(original.operation_id.bytes, std::move(entry));
		if (!inserted.second || !player_save_pipeline_restore_sql_shop_obligation(original))
			return false;
		inserted.first->second.cold->registration_pending = false;
		// Coordinator replay invokes this observer under its mutex: no submit,
		// replay, completion lookup, world effect, cancellation or ACK here.
		return true;
	}
	catch (...)
	{
		return false;
	}
#endif
}

void shop_trade_transaction_restore_pulse() noexcept
{
	if (!nevent_is_game_thread())
		return;
	std::array<std::array<uint8_t, CRITICAL_COMMAND_ID_BYTES>, SHOP_TRADE_PENDING_MAX> ids{};
	size_t count = 0;
	for (const auto &[key, entry] : pending)
		if (entry.cold && entry.completion_ready && count < ids.size())
			ids[count++] = key;
	for (size_t index = 0; index < count; ++index)
	{
		auto found = pending.find(ids[index]);
		if (found != pending.end() && found->second.cold && found->second.completion_ready)
			publish(found, nullptr);
	}
}

bool shop_trade_transaction_submit_with_publication(P_char character,
						    const shop_trade_payload &payload,
						    shop_trade_physical_publication_fn publication,
						    shop_trade_completion_fn completion)
{
	if (!character || IS_NPC(character) || GET_PID(character) <= 0 ||
	    static_cast<uint32_t>(GET_PID(character)) != payload.player_pid ||
	    pending.size() + notifying >= SHOP_TRADE_PENDING_MAX ||
	    player_pending(payload.player_pid))
		return false;
	critical_operation_id operation_id = {};
	critical_command command = {};
	try
	{
		if (!critical_operation_id_generate(&operation_id) ||
		    !shop_trade_command_build(&command, operation_id, payload,
					      critical_source_site::command,
					      critical_deadline_class::interactive))
			return false;
		pending_trade entry = {};
		entry.player_pid = payload.player_pid;
		entry.payload = payload;
		entry.completion = completion;
		entry.publication = publication;
		if (!pending.emplace(operation_id.bytes, std::move(entry)).second)
			return false;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	try
	{
		const auto submitted = critical_command_coordinator_submit(std::move(command));
		if (!critical_submit_result_keeps_operation(submitted))
		{
			pending.erase(operation_id.bytes);
			return false;
		}
	}
	catch (...)
	{
		// Admission may already own the operation. Do not fabricate rejection.
		return true;
	}
	return true;
}

bool shop_trade_transaction_submit(P_char character, const shop_trade_payload &payload,
				   shop_trade_completion_fn completion)
{
	return shop_trade_transaction_submit_with_publication(character, payload, nullptr,
							      completion);
}

void shop_trade_transaction_handle_completions(const critical_completion *completions, size_t count)
{
	if (count && !completions)
		return;
	std::array<std::array<uint8_t, CRITICAL_COMMAND_ID_BYTES>, SHOP_TRADE_PENDING_MAX>
		attempted = {};
	size_t attempt_count = 0;
	for (size_t index = 0; index < count; ++index)
	{
		auto found = pending.find(completions[index].operation_id.bytes);
		if (found == pending.end())
			continue;
		auto &entry = found->second;
		if (entry.local_refused ||
		    (entry.preparation && !entry.preparation->submission_started))
			continue;
		const bool changed_retired_timing =
			entry.never_admitted_retired &&
			(entry.completed.queued_at_usec != completions[index].queued_at_usec ||
			 entry.completed.started_at_usec != completions[index].started_at_usec ||
			 entry.completed.completed_at_usec != completions[index].completed_at_usec);
		const bool cold_refusal_verified = entry.cold && entry.cold->refusal_verified;
		const bool original_refusal_verified = cold_refusal_verified ||
						       entry.flat_refusal_verified;
		if (entry.completion_ready &&
		    (!(original_refusal_verified ?
			       same_original_refusal(entry.completed, completions[index]) :
		       entry.native_receipt_verified ?
			       same_native_receipt(entry.completed, completions[index]) :
			       same_receipt(entry.completed, completions[index])) ||
		     changed_retired_timing))
		{
			const bool original_committed =
				entry.completed.outcome == critical_apply_outcome::applied ||
				entry.completed.outcome == critical_apply_outcome::already_applied;
			if (original_committed || entry.native_receipt_verified ||
			    entry.never_admitted_retired || original_refusal_verified ||
			    entry.publishing || entry.blocked)
			{
				entry.blocked = true;
				continue;
			}
		}
		// An exact readback may say already applied; keep the first committed
		// receipt and projection stages while accepting that equivalent proof.
		if (!entry.completion_ready ||
		    (!entry.never_admitted_retired && !entry.native_receipt_verified &&
		     !original_refusal_verified &&
		     entry.completed.outcome != critical_apply_outcome::applied &&
		     entry.completed.outcome != critical_apply_outcome::already_applied))
			entry.completed = completions[index];
		entry.completion_ready = true;
		if (std::find(attempted.begin(), attempted.begin() + attempt_count,
			      completions[index].operation_id.bytes) !=
		    attempted.begin() + attempt_count)
			continue;
		if (attempt_count == attempted.size())
			continue;
		attempted[attempt_count++] = completions[index].operation_id.bytes;
		if (entry.cold || (entry.preparation && entry.preparation->flat &&
				   entry.completed.disposition ==
					   critical_completion_disposition::never_admitted))
			publish(found, nullptr);
		else if (P_char character = find_player_by_pid(entry.player_pid))
			publish(found, character);
		else if (!entry.blocked && !entry.publishing && entry.preparation &&
			 !entry.preparation->flat && !entry.never_admitted_retired &&
			 entry.preparation->submission_started && entry.preparation->command &&
			 shop_trade_native_publication_owner::retire_never_admitted(
				 *entry.preparation->command, entry.completed))
		{
			// The private owner has cancelled the exact original hold. No live
			// callback is safe here: shop_trade_completion(NULL) would skip its
			// produced-item/sequence cleanup. Retain the immutable command,
			// original payload and callback for player_ready, with no new queue.
			entry.never_admitted_retired = true;
		}
	}
}

void shop_trade_transaction_player_ready(P_char character)
{
	if (!character || IS_NPC(character) || GET_PID(character) <= 0)
		return;
	// One owner per PID and one bounded attempt per invocation. Notification can
	// create a new owner; do not chase it or spin on a retained failed stage.
	auto found =
		std::find_if(pending.begin(), pending.end(),
			     [&](const auto &entry)
			     {
				     return entry.second.player_pid ==
						    static_cast<uint32_t>(GET_PID(character)) &&
					    entry.second.completion_ready;
			     });
	if (found != pending.end())
		publish(found, character);
}

bool shop_trade_transaction_player_busy(P_char character)
{
	return character && !IS_NPC(character) && GET_PID(character) > 0 &&
	       player_pending(static_cast<uint32_t>(GET_PID(character)));
}

namespace
{
#ifndef __NO_MYSQL__
bool preparation_tree_bounded(P_obj root, std::vector<uint8_t> *bytes, std::vector<uint64_t> *uids,
			      size_t max_items, size_t max_bytes, size_t *estimated_bytes)
{
	if (!root)
	{
		bytes->clear();
		return true;
	}
	std::vector<player_item_snapshot> items;
	std::vector<uint8_t> encoded;
	size_t estimated = 0;
	if (player_item_snapshot_tree_capture_literal(root, &items, &estimated) !=
		    player_snapshot_capture_result::ok ||
	    items.empty() || items.size() > max_items ||
	    player_item_snapshot_list_encode(items, &encoded) != player_snapshot_codec_result::ok ||
	    encoded.size() > max_bytes)
		return false;
	if (uids)
		for (const auto &item : items)
			uids->push_back(item.object_uid);
	*bytes = std::move(encoded);
	if (estimated_bytes)
		*estimated_bytes = estimated;
	return true;
}

bool preparation_tree(P_obj root, std::vector<uint8_t> *bytes,
		      std::vector<uint64_t> *uids = nullptr, size_t *estimated_bytes = nullptr)
{
	return preparation_tree_bounded(root, bytes, uids, SHOP_TRADE_MAX_ITEMS,
					SHOP_TRADE_ITEM_BLOB_MAX_BYTES, estimated_bytes);
}

bool preparation_destination_tree(P_obj root, std::vector<uint8_t> *bytes,
				  std::vector<uint64_t> *uids, size_t *estimated_bytes)
{
	return preparation_tree_bounded(root, bytes, uids, PLAYER_SNAPSHOT_MAX_OBJECTS,
					PLAYER_SNAPSHOT_MAX_BYTES - sizeof(uint32_t),
					estimated_bytes);
}

bool same_mapping(const economic_shop_checkpoint_projection &a,
		  const economic_shop_checkpoint_projection &b)
{
	return a.lineage.bytes == b.lineage.bytes && a.epoch.bytes == b.epoch.bytes &&
	       economic_account_key_equal(a.wallet, b.wallet) &&
	       economic_account_key_equal(a.bank, b.bank);
}

bool preparation_body(P_char actor, P_char keeper, uint32_t shop)
{
	return actor && IS_PC(actor) && actor->only.pc && GET_PID(actor) > 0 && actor->runtime_id &&
	       find_character_by_runtime_id(actor->runtime_id) == actor && keeper &&
	       IS_NPC(keeper) && keeper->runtime_id &&
	       find_character_by_runtime_id(keeper->runtime_id) == keeper && shop_index &&
	       number_of_shops > 0 && shop < static_cast<uint32_t>(number_of_shops) &&
	       GET_RNUM(keeper) == shop_index[shop].keeper && GET_VNUM(keeper) > 0;
}
#endif
}

shop_trade_preparation_state
shop_trade_preparation_owner::begin(P_char actor, P_char keeper, P_obj selected, P_obj stock,
				    P_obj destination, uint32_t shop, shop_trade_action action,
				    int64_t price, shop_trade_preparation_token *output) noexcept
{
#ifdef __NO_MYSQL__
	(void)actor;
	(void)keeper;
	(void)selected;
	(void)stock;
	(void)destination;
	(void)shop;
	(void)action;
	(void)price;
	(void)output;
	return shop_trade_preparation_state::refused;
#else
	if (!output || !preparation_body(actor, keeper, shop) || !selected || !selected->obj_uid ||
	    price < 0 || price > INT_MAX || action <= shop_trade_action::unknown ||
	    action > shop_trade_action::sell_destroy ||
	    ((action == shop_trade_action::buy_produced) != (stock != nullptr)) ||
	    (destination && action != shop_trade_action::buy_produced) ||
	    ((action == shop_trade_action::sell_store ||
	      action == shop_trade_action::sell_destroy) &&
	     (!price || !OBJ_CARRIED_BY(selected, actor))) ||
	    (action == shop_trade_action::buy_existing && !OBJ_CARRIED_BY(selected, keeper)) ||
	    (stock && !OBJ_CARRIED_BY(stock, keeper)) ||
	    (destination && !OBJ_CARRIED_BY(destination, actor)) ||
	    pending.size() + notifying >= SHOP_TRADE_PENDING_MAX ||
	    player_pending(static_cast<uint32_t>(GET_PID(actor))) ||
	    shop_trade_transaction_keeper_busy(shop) ||
	    preparation_generation == std::numeric_limits<uint64_t>::max())
		return shop_trade_preparation_state::refused;
	critical_operation_id operation{};
	bool owns_entry = false;
	try
	{
		const char *account = get_account_name_safe(actor);
		if (!account || !strcmp(account, "Unknown") ||
		    strlen(account) > CURRENCY_ACCOUNT_NAME_MAX_BYTES ||
		    GET_RACEWAR(actor) > INT8_MAX)
			return shop_trade_preparation_state::refused;
		const int64_t keeper_cash = static_cast<int64_t>(GET_COPPER(keeper)) +
					    10LL * GET_SILVER(keeper) + 100LL * GET_GOLD(keeper) +
					    1000LL * GET_PLATINUM(keeper);
		if (GET_COPPER(keeper) < 0 || GET_SILVER(keeper) < 0 || GET_GOLD(keeper) < 0 ||
		    GET_PLATINUM(keeper) < 0 || keeper_cash < 0 || keeper_cash > INT_MAX)
			return shop_trade_preparation_state::refused;
		auto prepared = std::make_unique<trade_preparation>();
		if (!economic_gameplay_authority::observe_shop_checkpoint(
			    GET_PID(actor), account, static_cast<uint8_t>(GET_RACEWAR(actor)),
			    &prepared->mapping) ||
		    !preparation_tree(selected, &prepared->selected, &prepared->fenced_uids,
				      &prepared->selected_capture_bytes) ||
		    !preparation_tree(stock, &prepared->stock, &prepared->fenced_uids) ||
		    !preparation_destination_tree(destination, &prepared->destination,
						  &prepared->fenced_uids,
						  &prepared->destination_capture_bytes) ||
		    !shop_item_runtime_capture_keeper_literal(keeper, &prepared->keeper_items) ||
		    player_item_snapshot_list_encode(prepared->keeper_items,
						     &prepared->keeper_blob) !=
			    player_snapshot_codec_result::ok)
			return shop_trade_preparation_state::refused;
		for (const auto &item : prepared->keeper_items)
			prepared->fenced_uids.push_back(item.object_uid);
		std::sort(prepared->fenced_uids.begin(), prepared->fenced_uids.end());
		prepared->fenced_uids.erase(std::unique(prepared->fenced_uids.begin(),
							prepared->fenced_uids.end()),
					    prepared->fenced_uids.end());
		for (const auto uid : prepared->fenced_uids)
			if (shop_trade_transaction_item_busy(uid))
				return shop_trade_preparation_state::refused;
		if (!critical_operation_id_generate(&operation))
			return shop_trade_preparation_state::refused;
		prepared->generation = ++preparation_generation;
		prepared->actor_runtime_id = actor->runtime_id;
		prepared->keeper_runtime_id = keeper->runtime_id;
		const uint64_t generation = prepared->generation;
		pending_trade entry{};
		entry.player_pid = static_cast<uint32_t>(GET_PID(actor));
		entry.payload.player_pid = entry.player_pid;
		entry.payload.shop_id = shop;
		entry.payload.action = action;
		entry.payload.price = price;
		entry.payload.keeper_vnum = GET_VNUM(keeper);
		entry.payload.expected_keeper_cash = keeper_cash;
		entry.payload.keeper_roaming = shop_index[shop].shop_is_roaming != 0;
		entry.payload.selected_item_uid = selected->obj_uid;
		entry.payload.stock_item_uid = stock ? stock->obj_uid : 0;
		entry.payload.target_parent_item_uid = destination ? destination->obj_uid : 0;
		entry.payload.racewar = static_cast<uint8_t>(GET_RACEWAR(actor));
		strcpy(entry.payload.account_name.data(), account);
		entry.preparation = std::move(prepared);
		auto [found, inserted] = pending.emplace(operation.bytes, std::move(entry));
		if (!inserted)
			return shop_trade_preparation_state::refused;
		owns_entry = true;
		shop_trade_preparation_token token;
		token.operation_id_ = operation;
		token.generation_ = generation;
		*output = token;
		// Own every original body/UID before the one native shell probe. An
		// uncertain construction/extraction tail retains this same preparation.
		if (destination)
		{
			auto &owned = *found->second.preparation;
			std::vector<player_item_snapshot> target, source;
			if (player_item_snapshot_list_decode(owned.destination.data(),
							     owned.destination.size(), &target) !=
				    player_snapshot_codec_result::ok ||
			    player_item_snapshot_list_decode(owned.selected.data(),
							     owned.selected.size(), &source) !=
				    player_snapshot_codec_result::ok ||
			    target.empty() || source.empty() ||
			    target.front().type != ITEM_CONTAINER ||
			    source.size() > PLAYER_SNAPSHOT_MAX_OBJECTS - target.size())
			{
				pending.erase(found);
				return shop_trade_preparation_state::refused;
			}
			// Native capture charges structures/dynamic rows in addition to
			// wire bytes. Two separate captures count the envelope twice.
			const size_t envelope = sizeof(player_snapshot);
			if (owned.selected_capture_bytes < envelope ||
			    owned.destination_capture_bytes < envelope ||
			    owned.destination_capture_bytes > PLAYER_SNAPSHOT_MAX_BYTES ||
			    owned.selected_capture_bytes - envelope >
				    PLAYER_SNAPSHOT_MAX_BYTES - owned.destination_capture_bytes)
			{
				pending.erase(found);
				return shop_trade_preparation_state::refused;
			}
			int32_t shell = 0, direct = 0;
			if (target.front().values[4] > 0)
			{
				int64_t sum = 0;
				for (size_t index = 1; index < target.size(); ++index)
					if (target[index].parent_index == 0)
					{
						sum += target[index].weight;
						if (sum < INT32_MIN || sum > INT32_MAX)
						{
							pending.erase(found);
							return shop_trade_preparation_state::refused;
						}
					}
				direct = static_cast<int32_t>(sum);
				owned.destination_shell_started = true;
				const bool captured =
					obj_capture_container_shell_weight(destination, &shell);
				owned.destination_shell_returned = true;
				if (!captured)
				{
					pending.erase(found);
					return shop_trade_preparation_state::refused;
				}
				// Hooks may touch the world. Resolve the original bodies again
				// and compare all literal source values before any checkpoint.
				actor = find_character_by_runtime_id(owned.actor_runtime_id);
				keeper = find_character_by_runtime_id(owned.keeper_runtime_id);
				if (!preparation_body(actor, keeper, shop))
					return shop_trade_preparation_state::pending;
				std::vector<uint8_t> current_target, current_selected;
				// The original UID census is established again by poll before
				// SQL; pointers returned across a probe are never retained.
				P_obj original_destination = nullptr, original_selected = nullptr;
				size_t count = 0;
				for (P_obj object = actor->carrying; object;
				     object = object->next_content)
				{
					if (++count > PLAYER_SNAPSHOT_MAX_OBJECTS ||
					    !OBJ_CARRIED_BY(object, actor))
						return shop_trade_preparation_state::pending;
					if (object->obj_uid ==
					    found->second.payload.target_parent_item_uid)
					{
						if (original_destination)
							return shop_trade_preparation_state::pending;
						original_destination = object;
					}
				}
				// Produced selection is detached, so resolve it from the global
				// object list through the same original UID; never read a stale pointer.
				extern P_obj object_list;
				count = 0;
				for (P_obj object = object_list; object; object = object->next)
				{
					if (++count > 262144)
						return shop_trade_preparation_state::pending;
					if (object->obj_uid ==
					    found->second.payload.selected_item_uid)
					{
						if (original_selected)
							return shop_trade_preparation_state::pending;
						original_selected = object;
					}
				}
				if (!original_destination || !original_selected ||
				    !preparation_destination_tree(original_destination,
								  &current_target) ||
				    !preparation_tree(original_selected, &current_selected) ||
				    current_target != owned.destination ||
				    current_selected != owned.selected)
					return shop_trade_preparation_state::pending;
				destination = original_destination;
				selected = original_selected;
			}
			if (!shop_trade_destination_weight_compute(
				    target.front(), source.front().weight, direct, shell,
				    &owned.destination_weight))
			{
				pending.erase(found);
				return shop_trade_preparation_state::refused;
			}
			// The complete future literal destination must fit the same capture
			// envelope now, before checkpoint/SQL/admission. Selected STORE/key
			// transforms are fixed-width and cannot enlarge this encoding.
			auto combined = target;
			combined.front().weight = owned.destination_weight.after;
			const auto base = static_cast<int32_t>(combined.size());
			for (auto item : source)
			{
				item.parent_index =
					item.parent_index < 0 ? 0 : base + item.parent_index;
				combined.push_back(std::move(item));
			}
			std::vector<uint8_t> encoded;
			if (player_item_snapshot_list_encode(combined, &encoded) !=
				    player_snapshot_codec_result::ok ||
			    encoded.size() > PLAYER_SNAPSHOT_MAX_BYTES - sizeof(uint32_t))
			{
				pending.erase(found);
				return shop_trade_preparation_state::refused;
			}
		}
		found->second.preparation->destination_weight_ready = true;

		// The domain entry already owns the keeper and every frozen UID while
		// the ordinary save request captures/queues the player's checkpoint.
		const bool selling = action == shop_trade_action::sell_store ||
				     action == shop_trade_action::sell_destroy;
		const auto state = player_save_pipeline_shop_checkpoint_begin(
			actor, selling ? selected : nullptr, NOWHERE,
			&found->second.preparation->player_token);
		if (state == player_literal_inventory_state::refused)
		{
			pending.erase(found);
			return shop_trade_preparation_state::refused;
		}
		return shop_trade_preparation_state::pending;
	}
	catch (...)
	{
		// A matching generated ID is not ownership if insertion failed.
		if (!owns_entry)
			return shop_trade_preparation_state::refused;
		auto found = pending.find(operation.bytes);
		if (found == pending.end())
			return shop_trade_preparation_state::refused;
		if (found->second.preparation->destination_shell_started &&
		    !found->second.preparation->destination_shell_returned)
			return shop_trade_preparation_state::pending;
		const auto &player = found->second.preparation->player_token;
		// The pipeline exposes the installed slot before enqueue. A zero
		// token proves this preparation never acquired a player checkpoint;
		// no keeper write or critical admission has been attempted yet.
		if (!player.pid && !player.actor_runtime_id && !player.generation)
		{
			pending.erase(found);
			return shop_trade_preparation_state::refused;
		}
		return shop_trade_preparation_state::pending;
	}
#endif
}

shop_trade_preparation_state
shop_trade_preparation_owner::poll(const shop_trade_preparation_token &token, P_char actor,
				   P_char keeper, P_obj selected, P_obj stock,
				   P_obj destination) noexcept
{
#ifdef __NO_MYSQL__
	(void)token;
	(void)actor;
	(void)keeper;
	(void)selected;
	(void)stock;
	(void)destination;
	return shop_trade_preparation_state::refused;
#else
	auto found = pending.find(token.operation_id_.bytes);
	if (found == pending.end() || !found->second.preparation ||
	    found->second.preparation->generation != token.generation_)
		return shop_trade_preparation_state::refused;
	auto &entry = found->second;
	auto &prepared = *entry.preparation;
	if (prepared.submission_started ||
	    (prepared.destination_shell_started && !prepared.destination_shell_returned) ||
	    !prepared.destination_weight_ready)
		return shop_trade_preparation_state::refused;
	try
	{
		if (!preparation_body(actor, keeper, entry.payload.shop_id) ||
		    actor->runtime_id != prepared.actor_runtime_id ||
		    keeper->runtime_id != prepared.keeper_runtime_id ||
		    static_cast<uint32_t>(GET_PID(actor)) != entry.player_pid || !selected ||
		    selected->obj_uid != entry.payload.selected_item_uid ||
		    (stock ? stock->obj_uid : 0) != entry.payload.stock_item_uid ||
		    (destination ? destination->obj_uid : 0) !=
			    entry.payload.target_parent_item_uid ||
		    GET_VNUM(keeper) != entry.payload.keeper_vnum ||
		    (shop_index[entry.payload.shop_id].shop_is_roaming != 0) !=
			    (entry.payload.keeper_roaming != 0) ||
		    GET_RACEWAR(actor) != entry.payload.racewar ||
		    ((entry.payload.action == shop_trade_action::sell_store ||
		      entry.payload.action == shop_trade_action::sell_destroy) &&
		     !OBJ_CARRIED_BY(selected, actor)) ||
		    (entry.payload.action == shop_trade_action::buy_existing &&
		     !OBJ_CARRIED_BY(selected, keeper)) ||
		    (stock && !OBJ_CARRIED_BY(stock, keeper)) ||
		    (destination && !OBJ_CARRIED_BY(destination, actor)))
			return shop_trade_preparation_state::refused;
		const int64_t keeper_cash = static_cast<int64_t>(GET_COPPER(keeper)) +
					    10LL * GET_SILVER(keeper) + 100LL * GET_GOLD(keeper) +
					    1000LL * GET_PLATINUM(keeper);
		if (GET_COPPER(keeper) < 0 || GET_SILVER(keeper) < 0 || GET_GOLD(keeper) < 0 ||
		    GET_PLATINUM(keeper) < 0 || keeper_cash != entry.payload.expected_keeper_cash)
			return shop_trade_preparation_state::refused;
		const char *account = get_account_name_safe(actor);
		economic_shop_checkpoint_projection mapping{};
		std::vector<uint8_t> a, b, c, d;
		std::vector<player_item_snapshot> keeper_items;
		if (!account || strcmp(account, entry.payload.account_name.data()) ||
		    !economic_gameplay_authority::observe_shop_checkpoint(
			    entry.player_pid, account, entry.payload.racewar, &mapping) ||
		    !same_mapping(mapping, prepared.mapping) || !preparation_tree(selected, &a) ||
		    a != prepared.selected || !preparation_tree(stock, &b) || b != prepared.stock ||
		    !preparation_destination_tree(destination, &c) || c != prepared.destination ||
		    !shop_item_runtime_capture_keeper_literal(keeper, &keeper_items) ||
		    player_item_snapshot_list_encode(keeper_items, &d) !=
			    player_snapshot_codec_result::ok ||
		    d != prepared.keeper_blob)
			return shop_trade_preparation_state::refused;
		if (prepared.player_held)
		{
			player_shop_checkpoint_stage held{};
			if (!player_save_shop_checkpoint_owner::observe_held(
				    prepared.player_token, actor, token.operation_id_, &held) ||
			    held.save_revision != prepared.player_stage.save_revision ||
			    held.level != prepared.player_stage.level)
				return shop_trade_preparation_state::refused;
			return shop_trade_preparation_state::ready;
		}
		player_shop_checkpoint_stage stage{};
		const auto state = player_save_pipeline_shop_checkpoint_poll(prepared.player_token,
									     actor, &stage);
		if (state == player_literal_inventory_state::pending)
			return shop_trade_preparation_state::pending;
		if (state != player_literal_inventory_state::database_acknowledged ||
		    !player_save_pipeline_shop_checkpoint_hold(prepared.player_token,
							       token.operation_id_))
			return shop_trade_preparation_state::refused;
		prepared.player_stage = stage;
		prepared.player_held = true;
		return shop_trade_preparation_state::ready;
	}
	catch (...)
	{
		return shop_trade_preparation_state::refused;
	}
#endif
}

bool shop_trade_preparation_owner::build_accounted_command(
	const shop_trade_preparation_token &token, P_char actor, P_char keeper, P_obj selected,
	P_obj stock, P_obj destination, critical_command *output) noexcept
{
#ifdef __NO_MYSQL__
	(void)token;
	(void)actor;
	(void)keeper;
	(void)selected;
	(void)stock;
	(void)destination;
	(void)output;
	return false;
#else
	if (!output || !nevent_is_game_thread() ||
	    poll(token, actor, keeper, selected, stock, destination) !=
		    shop_trade_preparation_state::ready)
		return false;
	auto found = pending.find(token.operation_id_.bytes);
	if (found == pending.end() || !found->second.preparation ||
	    found->second.preparation->generation != token.generation_)
		return false;
	auto &entry = found->second;
	auto &prepared = *entry.preparation;
	if (!prepared.native_ready || !prepared.native_sealed || !prepared.player_held)
		return false;
	try
	{
		const auto &native = prepared.native_stage;
		const std::array<int64_t, 4> wallet{ GET_COPPER(actor), GET_SILVER(actor),
						     GET_GOLD(actor), GET_PLATINUM(actor) };
		const std::array<int64_t, 4> bank{ GET_BALANCE_COPPER(actor),
						   GET_BALANCE_SILVER(actor),
						   GET_BALANCE_GOLD(actor),
						   GET_BALANCE_PLATINUM(actor) };
		if (wallet != native.wallet.amount || bank != native.bank.amount ||
		    actor->only.pc->wallet_revision != native.wallet_revision ||
		    actor->only.pc->bank_revision != native.bank_revision)
			return false;
		if (prepared.command)
		{
			critical_command copied = *prepared.command;
			*output = std::move(copied);
			return true;
		}
		shop_trade_payload payload{};
		if (shop_trade_runtime_build_accounted_payload(
			    actor, keeper, selected, stock, destination, entry.payload.shop_id,
			    entry.payload.action, entry.payload.price, prepared.player_stage,
			    native.shop_revision_after,
			    &payload) != shop_trade_payload_build_result::ok)
			return false;
		payload.native_destination_weight_recorded = true;
		payload.destination_weight = prepared.destination_weight;
		// Refuse an unpublishable complete player AFTER before journal admission
		// or SQL. Reuse the exact held original body and the final observer rules.
		player_shop_checkpoint_stage held{};
		std::vector<player_item_snapshot> original, prospective;
		if (!player_save_shop_checkpoint_owner::observe_held(
			    prepared.player_token, actor, token.operation_id_, &held, &original) ||
		    held.save_revision != payload.expected_player_save_revision ||
		    held.level != payload.expected_player_level ||
		    !shop_trade_native_publication_owner::expected_player_forest(
			    payload, original, false, &prospective))
			return false;
		if (payload.action == shop_trade_action::buy_existing ||
		    payload.action == shop_trade_action::buy_produced)
		{
			std::vector<player_item_snapshot> ordered;
			if (!shop_trade_native_publication_owner::expected_player_order(
				    payload, actor, selected, destination, prospective, &ordered))
				return false;
			prospective = std::move(ordered);
		}
		if (!shop_trade_world_player_values_supported(prospective))
			return false;

		// Saved-policy inventory omits some physical NORENT bodies; its row
		// count alone cannot prove that the final bounded world census fits.
		std::vector<uint64_t> literal_roots;
		if (std::any_of(original.begin(), original.end(), [&](const auto &item)
				{ return item.object_uid == payload.selected_item_uid; }))
			literal_roots.push_back(payload.selected_item_uid);
		std::vector<player_item_snapshot> detached;
		if (payload.action == shop_trade_action::buy_produced &&
		    player_item_snapshot_list_decode(payload.item_blob.data(),
						     payload.item_blob_size,
						     &detached) != player_snapshot_codec_result::ok)
			return false;
		shop_trade_world_expectation before_world{};
		before_world.actor_pid = payload.player_pid;
		before_world.actor_runtime_id = prepared.actor_runtime_id;
		before_world.keeper_runtime_id = prepared.keeper_runtime_id;
		before_world.shop_id = payload.shop_id;
		before_world.keeper_vnum = payload.keeper_vnum;
		before_world.player_items = original;
		before_world.keeper_items = prepared.keeper_items;
		before_world.detached_items = detached;
		before_world.literal_player_root_uids = literal_roots;
		shop_trade_world_witness before_observed;
		if (!shop_trade_world_witness_observe(before_world, &before_observed) ||
		    ((payload.action == shop_trade_action::buy_existing ||
		      payload.action == shop_trade_action::buy_produced) &&
		     payload.item_count > PLAYER_SNAPSHOT_MAX_OBJECTS -
						  before_observed.player_physical_item_count))
			return false;

		// Journal the original ordered forest bindings before admission. These
		// values are recovery comparison inputs; SQL must authenticate their
		// complete original images under the existing authority/custody locks.
		std::vector<player_item_snapshot> keeper_after;
		if (!shop_trade_native_publication_owner::expected_keeper_forest(
			    payload, keeper, selected, prepared.keeper_items, false, &keeper_after))
			return false;
		const auto freeze_forest = [](const std::vector<player_item_snapshot> &forest,
					      shop_trade_recovery_forest_role role,
					      shop_trade_recovery_forest_binding *binding)
		{
			std::vector<uint8_t> canonical;
			return player_item_snapshot_list_encode(forest, &canonical) ==
				       player_snapshot_codec_result::ok &&
			       shop_trade_recovery_forest_freeze(canonical, role, binding);
		};
		shop_trade_recovery_manifest manifest;
		if (!freeze_forest(original, shop_trade_recovery_forest_role::player_before,
				   &manifest.player_before) ||
		    !freeze_forest(prospective, shop_trade_recovery_forest_role::player_after,
				   &manifest.player_after) ||
		    !freeze_forest(prepared.keeper_items,
				   shop_trade_recovery_forest_role::keeper_before,
				   &manifest.keeper_before) ||
		    !freeze_forest(keeper_after, shop_trade_recovery_forest_role::keeper_after,
				   &manifest.keeper_after))
			return false;
		if (payload.target_parent_item_uid)
		{
			// Full live literal includes omitted non-durable NORENT children.
			// Cold recovery uses the separate saved-policy player binding and
			// must neither demand nor recreate those omitted children.
			std::vector<player_item_snapshot> target_after;
			if (!shop_trade_recovery_forest_freeze(
				    prepared.destination,
				    shop_trade_recovery_forest_role::live_target_before,
				    &manifest.live_target_before) ||
			    !shop_trade_native_publication_owner::expected_destination_forest(
				    payload, actor, selected, destination, prepared.destination,
				    &target_after) ||
			    !freeze_forest(target_after,
					   shop_trade_recovery_forest_role::live_target_after,
					   &manifest.live_target_after))
				return false;
		}
		payload.recovery_manifest = std::move(manifest);
		payload.recovery_manifest_recorded = true;

		auto command = std::make_unique<critical_command>();
		if (!shop_trade_command_build_recovery(command.get(), token.operation_id_, payload,
						       critical_source_site::command,
						       critical_deadline_class::interactive) ||
		    economic_gameplay_authority::prepare_shop_trade(command.get()) !=
			    economic_accounting_error::ok ||
		    command->schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION)
			return false;
		economic_frozen_intent intent;
		shop_trade_payload verified{};
		economic_account_key verified_wallet, verified_bank, counterparty;
		auto projection = *command;
		projection.accepted_at_usec = 1;
		if (shop_trade_accounting_decode(projection, &intent, &verified, &verified_wallet,
						 &verified_bank,
						 &counterparty) != economic_accounting_error::ok ||
		    intent.admission.metadata.epoch.bytes != prepared.mapping.epoch.bytes ||
		    !economic_account_key_equal(verified_wallet, prepared.mapping.wallet) ||
		    !economic_account_key_equal(verified_bank, prepared.mapping.bank))
			return false;
		const auto now = std::chrono::duration_cast<std::chrono::microseconds>(
					 std::chrono::system_clock::now().time_since_epoch())
					 .count();
		if (now <= 0)
			return false;
		command->accepted_at_usec = static_cast<uint64_t>(now);
		command->publication_required = true;
		if (!critical_command_envelope_valid(*command))
			return false;
		// All allocating copies finish before retaining the immutable decision.
		// Subsequent exact-ID attempts preserve its original real acceptance time.
		critical_command copied = *command;
		prepared.command = std::move(command);
		*output = std::move(copied);
		return true;
	}
	catch (...)
	{
		return false;
	}
#endif
}

critical_submit_result shop_trade_preparation_owner::submit_accounted(
	const shop_trade_preparation_token &token, P_char actor, P_char keeper, P_obj selected,
	P_obj stock, P_obj destination, shop_trade_accounted_publication_fn publication,
	shop_trade_completion_fn completion) noexcept
{
#ifdef __NO_MYSQL__
	(void)token;
	(void)actor;
	(void)keeper;
	(void)selected;
	(void)stock;
	(void)destination;
	(void)publication;
	(void)completion;
	return critical_submit_result::unavailable;
#else
	if (!nevent_is_game_thread() || !publication)
		return critical_submit_result::invalid;
	auto found = pending.find(token.operation_id_.bytes);
	if (found == pending.end() || !found->second.preparation ||
	    found->second.preparation->generation != token.generation_)
		return critical_submit_result::identity_conflict;
	auto &entry = found->second;
	auto &prepared = *entry.preparation;
	try
	{
		critical_command command;
		shop_trade_payload payload{};
		if (prepared.submission_started)
		{
			if (!prepared.command || entry.accounted_publication != publication ||
			    entry.completion != completion)
				return critical_submit_result::identity_conflict;
			command = *prepared.command;
		}
		else if (!build_accounted_command(token, actor, keeper, selected, stock,
						  destination, &command))
			return critical_submit_result::unavailable;
		if (!shop_trade_command_decode_payload(command, &payload) ||
		    command.operation_id.bytes != token.operation_id_.bytes ||
		    (command.payload_version != SHOP_TRADE_NATIVE_PAYLOAD_VERSION &&
		     command.payload_version != SHOP_TRADE_RECOVERY_PAYLOAD_VERSION) ||
		    payload.player_pid != entry.player_pid || !prepared.native_ready ||
		    !prepared.native_sealed || !prepared.player_held)
			return critical_submit_result::invalid;
		// Before admission the selection placeholder stays intact for full poll
		// recapture. Freeze the complete v6 payload only after that poll succeeds.
		const auto previous_payload = entry.payload;
		const auto previous_publication = entry.accounted_publication;
		const auto previous_completion = entry.completion;
		const bool was_started = prepared.submission_started;
		entry.payload = payload;
		entry.accounted_publication = publication;
		entry.completion = completion;
		prepared.submission_started = true;
		bool checkpoint_released = false;
		const auto submitted = player_save_shop_checkpoint_owner::submit_owned(
			prepared.player_token, std::move(command), &checkpoint_released);
		if (critical_submit_result_keeps_operation(submitted))
			return submitted;
		if (checkpoint_released)
		{
			// The pipeline proved synchronous refusal before journal admission and
			// consumed its exact original execution generation. The keeper's durable
			// checkpoint remains: retiring preparation is no economic rollback.

			if (entry.production_owned)
			{
				entry.local_refused = true;
				entry.refusal_error = EAGAIN;
				prepared.player_held = false;
			}
			else
				pending.erase(found);
			return submitted;
		}
		if (was_started)
			return critical_submit_result::journal_uncertain;
		// No execution hold was installed. Preserve the original held checkpoint
		// and selection so a later original-token attempt can revalidate it.
		entry.payload = previous_payload;
		entry.accounted_publication = previous_publication;
		entry.completion = previous_completion;
		prepared.submission_started = false;
		return submitted;
	}
	catch (...)
	{
		return prepared.submission_started ? critical_submit_result::journal_uncertain :
						     critical_submit_result::invalid;
	}
#endif
}

bool shop_trade_preparation_owner::cancel(const shop_trade_preparation_token &token) noexcept
{
	auto found = pending.find(token.operation_id_.bytes);
	if (found == pending.end() || !found->second.preparation ||
	    found->second.preparation->generation != token.generation_)
		return false;
	auto &prepared = *found->second.preparation;

	if (found->second.local_refused)
		return true;
	if (prepared.destination_shell_started && !prepared.destination_shell_returned)
		return false;
#ifndef __NO_MYSQL__
	// An unresolved native attempt still owns its original rows/revisions/body.
	// Committed preparation also remains owned until guarded admission/publication.
	if (prepared.native_attempted || prepared.native_ready)
		return false;
#endif
	if (prepared.submission_started)
		return false;
	if (!prepared.player_token.pid && !prepared.player_token.actor_runtime_id &&
	    !prepared.player_token.generation && !prepared.player_held)
	{
		// No checkpoint, SQL preparation or command admission exists. A normally
		// returned probe can be abandoned; an uncertain native tail cannot.

		if (found->second.production_owned)
			found->second.local_refused = true;
		else
			pending.erase(found);
		return true;
	}
	const bool released =
		prepared.player_held ?
			player_save_pipeline_shop_checkpoint_release(prepared.player_token,
								     token.operation_id_) :
			player_save_pipeline_shop_checkpoint_cancel(prepared.player_token);

	if (!released)
		return false;
	if (found->second.production_owned)
	{
		found->second.local_refused = true;
		prepared.player_held = false;
	}
	else
		pending.erase(found);
	return true;
}

bool shop_trade_preparation_owner::production_available() noexcept
{
#ifdef __NO_MYSQL__
	return false;
#else
	return nevent_is_game_thread() && economic_gameplay_authority::active_regular_sql() &&
	       economic_shop_trade_admission_available();
#endif
}

bool shop_trade_preparation_owner::start(P_char actor, P_char keeper, P_obj selected, P_obj stock,
					 P_obj destination, uint32_t shop, shop_trade_action action,
					 int64_t price,
					 shop_trade_accounted_publication_fn publication,
					 shop_trade_completion_fn completion,
					 shop_trade_preparation_refusal_fn refusal) noexcept
{
	if (!publication || !completion || !refusal || !production_available())
		return false;
	shop_trade_preparation_token token;
	begin(actor, keeper, selected, stock, destination, shop, action, price, &token);
	auto found = pending.find(token.operation_id_.bytes);
	if (found == pending.end() || !found->second.preparation ||
	    found->second.preparation->generation != token.generation_ ||
	    found->second.production_owned)
		return false;
	// begin may retain an uncertain native probe tail. Ownership is the retained
	// original entry, not its status enum; never let the caller free that stage.
	auto &entry = found->second;
	entry.accounted_publication = publication;
	entry.completion = completion;
	entry.refusal = refusal;
	entry.production_owned = true;
	return true;
}

void shop_trade_preparation_owner::notify_refusal(const shop_trade_preparation_token &token) noexcept
{
	auto found = pending.find(token.operation_id_.bytes);
	if (found == pending.end() || !found->second.preparation ||
	    found->second.preparation->generation != token.generation_)
		return;
	auto &entry = found->second;
	if (!entry.production_owned || !entry.local_refused || entry.blocked || entry.publishing ||
	    entry.refusal_attempted || !entry.refusal)
		return;
	// This is notification after proven cancellation, not rebinding a native
	// checkpoint. A reconnect can receive its original PID/account cancellation.
	P_char actor = find_player_by_pid(entry.player_pid);
	if (!actor || !IS_PC(actor) || !actor->only.pc || GET_PID(actor) <= 0 ||
	    static_cast<uint32_t>(GET_PID(actor)) != entry.player_pid ||
	    GET_RACEWAR(actor) != entry.payload.racewar)
		return;
	const char *account = get_account_name_safe(actor);
	if (!account || strcmp(account, entry.payload.account_name.data()))
		return;
	entry.refusal_attempted = entry.publishing = true;
	++notifying;
	bool finished = false;
	try
	{
		// Keep the entry visible to PID/keeper/UID exclusions during callbacks.
		// A partial/throwing cleanup cannot admit a second overlapping owner.
		finished = entry.refusal(actor, entry.payload, entry.preparation->selected,
					 entry.refusal_error);
	}
	catch (...)
	{
		finished = false;
	}
	--notifying;
	found = pending.find(token.operation_id_.bytes);
	if (found == pending.end() || !found->second.preparation ||
	    found->second.preparation->generation != token.generation_)
		return;
	found->second.publishing = false;
	if (finished)
		pending.erase(found);
	else
		found->second.blocked = true;
}

void shop_trade_preparation_owner::drive(const shop_trade_preparation_token &token) noexcept
{
#ifndef __NO_MYSQL__
	auto found = pending.find(token.operation_id_.bytes);
	if (found == pending.end() || !found->second.preparation ||
	    found->second.preparation->generation != token.generation_ ||
	    !found->second.production_owned || found->second.driving || found->second.blocked ||
	    found->second.publishing || found->second.completion_ready)
		return;
	if (found->second.local_refused)
	{
		notify_refusal(token);
		return;
	}
	found->second.driving = true;
	struct driving_scope
	{
		std::array<uint8_t, CRITICAL_COMMAND_ID_BYTES> key;
		uint64_t generation;
		~driving_scope()
		{
			auto current = pending.find(key);
			if (current != pending.end() && current->second.preparation &&
			    current->second.preparation->generation == generation)
				current->second.driving = false;
		}
	} scope{ token.operation_id_.bytes, token.generation_ };
	// Submission retries use only the previously frozen command and callbacks.
	// They never reselect a keeper/item, recreate a stage or reroll a copy.
	if (found->second.preparation->submission_started)
	{
		submit_accounted(token, nullptr, nullptr, nullptr, nullptr, nullptr,
				 found->second.accounted_publication, found->second.completion);
		return;
	}
	// Resolve an existing uncertain checkpoint against its retained SQL stage
	// before requiring any original live body. This retry API cannot issue a
	// fresh write; a settled commit still needs full live proof below.
	if (found->second.preparation->native_attempted && !found->second.preparation->native_ready)
	{
		const auto reconciled = shop_trade_native_checkpoint_owner::attempt(
			token, nullptr, nullptr, nullptr, nullptr, nullptr);
		found = pending.find(token.operation_id_.bytes);
		if (found == pending.end() || !found->second.preparation ||
		    found->second.preparation->generation != token.generation_ ||
		    found->second.blocked || found->second.local_refused ||
		    found->second.completion_ready)
			return;
		if (reconciled != shop_trade_preparation_state::ready)
			return;
	}
	if (!production_available())
	{
		cancel(token);
		return;
	}
	auto unique_object = [](uint64_t uid) -> P_obj
	{
		if (!uid)
			return nullptr;
		extern P_obj object_list;
		P_obj result = nullptr;
		size_t count = 0;
		for (P_obj object = object_list; object; object = object->next)
		{
			if (++count > 262144)
				return nullptr;
			if (object->obj_uid != uid)
				continue;
			if (result)
				return nullptr;
			result = object;
		}
		return result;
	};
	// Reobserve all original identities after each native attempt. The helper
	// merely produces transient pointers; poll owns full literal equality proof.
	auto observe = [&](P_char &actor, P_char &keeper, P_obj &selected, P_obj &stock,
			   P_obj &destination)
	{
		auto current = pending.find(token.operation_id_.bytes);
		if (current == pending.end() || !current->second.preparation ||
		    current->second.preparation->generation != token.generation_ ||
		    current->second.blocked || current->second.local_refused ||
		    current->second.completion_ready)
			return false;
		const auto &entry = current->second;
		actor = find_character_by_runtime_id(entry.preparation->actor_runtime_id);
		keeper = find_character_by_runtime_id(entry.preparation->keeper_runtime_id);
		selected = unique_object(entry.payload.selected_item_uid);
		stock = unique_object(entry.payload.stock_item_uid);
		destination = unique_object(entry.payload.target_parent_item_uid);
		return actor && keeper && selected && (!entry.payload.stock_item_uid || stock) &&
		       (!entry.payload.target_parent_item_uid || destination);
	};
	P_char actor = nullptr, keeper = nullptr;
	P_obj selected = nullptr, stock = nullptr, destination = nullptr;
	if (!observe(actor, keeper, selected, stock, destination))
	{
		cancel(token);
		return;
	}
	const auto player = poll(token, actor, keeper, selected, stock, destination);
	if (player == shop_trade_preparation_state::pending)
		return;
	if (player != shop_trade_preparation_state::ready)
	{
		cancel(token);
		return;
	}
	const auto native = shop_trade_native_checkpoint_owner::attempt(
		token, actor, keeper, selected, stock, destination);
	if (native == shop_trade_preparation_state::pending)
		return;
	if (native != shop_trade_preparation_state::ready)
	{
		// cancel refuses any uncertain native tail or committed checkpoint.
		cancel(token);
		return;
	}
	if (!observe(actor, keeper, selected, stock, destination))
		return;
	found = pending.find(token.operation_id_.bytes);
	submit_accounted(token, actor, keeper, selected, stock, destination,
			 found->second.accounted_publication, found->second.completion);
#else
	(void)token;
#endif
}

void shop_trade_preparation_owner::pulse() noexcept
{
	if (!nevent_is_game_thread())
		return;
	std::array<shop_trade_preparation_token, SHOP_TRADE_PENDING_MAX> original{};
	size_t count = 0;
	for (const auto &[key, entry] : pending)
	{
		if (count == original.size())
			break;
		if (!entry.production_owned || !entry.preparation)
			continue;
		original[count].operation_id_.bytes = key;
		original[count].generation_ = entry.preparation->generation;
		++count;
	}
	for (size_t index = 0; index < count; ++index)
		drive(original[index]);
}

bool shop_trade_preparation_owner::checkpoint_context(
	const shop_trade_preparation_token &token, shop_trade_checkpoint_context *output) noexcept
{
	auto found = pending.find(token.operation_id_.bytes);
	if (!output || found == pending.end() || !found->second.preparation ||
	    found->second.preparation->generation != token.generation_ ||
	    !found->second.preparation->player_held)
		return false;
	try
	{
		const auto &entry = found->second;
		const auto &prepared = *entry.preparation;
		shop_trade_checkpoint_context stage;
		stage.operation_id = token.operation_id_;
		stage.generation = token.generation_;
		stage.actor_runtime_id = prepared.actor_runtime_id;
		stage.keeper_runtime_id = prepared.keeper_runtime_id;
		stage.player_pid = entry.player_pid;
		stage.shop_id = entry.payload.shop_id;
		stage.account_name = entry.payload.account_name;
		stage.racewar = entry.payload.racewar;
		stage.keeper_roaming = entry.payload.keeper_roaming;
		stage.keeper_cash = entry.payload.expected_keeper_cash;
		stage.keeper_vnum = entry.payload.keeper_vnum;
		stage.mapping = prepared.mapping;
		stage.player = prepared.player_stage;
		stage.keeper_items = prepared.keeper_items;
		*output = std::move(stage);
		return true;
	}
	catch (...)
	{
		return false;
	}
}

#ifndef __NO_MYSQL__
bool shop_trade_preparation_owner::begin_native_checkpoint(
	const shop_trade_preparation_token &token, P_char actor, P_char keeper, P_obj selected,
	P_obj stock, P_obj destination, shop_trade_checkpoint_context *context,
	shop_trade_native_checkpoint_stage *retained, bool *retry) noexcept
{
	if (!nevent_is_game_thread() || !context || !retained || !retry)
		return false;
	auto found = pending.find(token.operation_id_.bytes);
	if (found == pending.end() || !found->second.preparation ||
	    found->second.preparation->generation != token.generation_)
		return false;
	auto &prepared = *found->second.preparation;
	if (!prepared.player_held || prepared.native_ready)
		return false;
	// Existing uncertain writes resolve against original SQL facts even if the
	// actor has disconnected or gameplay recapture changed. Retry grants no write.
	if (!prepared.native_attempted && poll(token, actor, keeper, selected, stock,
					       destination) != shop_trade_preparation_state::ready)
		return false;
	try
	{
		shop_trade_checkpoint_context copied_context;
		if (!checkpoint_context(token, &copied_context))
			return false;
		// Copying for a retry may allocate. Failure cannot discard the original
		// sealed stage or reopen cancellation after an uncertain native mutation.
		shop_trade_native_checkpoint_stage copied_stage = prepared.native_stage;
		const bool existing_attempt = prepared.native_attempted;
		*context = std::move(copied_context);
		*retained = std::move(copied_stage);
		*retry = existing_attempt;
		prepared.native_attempted = true;
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool shop_trade_preparation_owner::completed_native_checkpoint(
	const shop_trade_preparation_token &token,
	shop_trade_native_checkpoint_stage *output) noexcept
{
	if (!nevent_is_game_thread() || !output)
		return false;
	auto found = pending.find(token.operation_id_.bytes);
	if (found == pending.end() || !found->second.preparation ||
	    found->second.preparation->generation != token.generation_)
		return false;
	const auto &prepared = *found->second.preparation;
	if (!prepared.player_held || !prepared.native_attempted || !prepared.native_sealed ||
	    !prepared.native_ready)
		return false;
	try
	{
		shop_trade_native_checkpoint_stage copied = prepared.native_stage;
		*output = std::move(copied);
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool shop_trade_preparation_owner::seal_native_checkpoint(
	const shop_trade_preparation_token &token,
	shop_trade_native_checkpoint_stage &&stage) noexcept
{
	if (!nevent_is_game_thread())
		return false;
	auto found = pending.find(token.operation_id_.bytes);
	if (found == pending.end() || !found->second.preparation ||
	    found->second.preparation->generation != token.generation_)
		return false;
	auto &prepared = *found->second.preparation;
	if (!prepared.player_held || !prepared.native_attempted || prepared.native_sealed ||
	    prepared.native_ready || !stage.keeper_id || !stage.bank_id ||
	    stage.shop_revision_before == UINT64_MAX ||
	    stage.shop_revision_after != stage.shop_revision_before + 1 ||
	    (stage.payload_checkpoint_recorded_before ?
		     (!stage.payload_checkpoint_revision_before ||
		      stage.payload_checkpoint_revision_before > stage.shop_revision_before) :
		     stage.payload_checkpoint_revision_before != 0) ||
	    stage.keeper_image.size() != prepared.keeper_items.size())
		return false;
	prepared.native_stage = std::move(stage);
	prepared.native_sealed = true;
	return true;
}

bool shop_trade_preparation_owner::finish_native_checkpoint(
	const shop_trade_preparation_token &token,
	shop_trade_native_checkpoint_disposition disposition) noexcept
{
	if (!nevent_is_game_thread())
		return false;
	auto found = pending.find(token.operation_id_.bytes);
	if (found == pending.end() || !found->second.preparation ||
	    found->second.preparation->generation != token.generation_)
		return false;
	auto &prepared = *found->second.preparation;
	if (!prepared.player_held || !prepared.native_attempted || prepared.native_ready)
		return false;
	switch (disposition)
	{
	case shop_trade_native_checkpoint_disposition::committed:
		if (!prepared.native_sealed)
			return false;
		try
		{
			const auto &native = prepared.native_stage;
			const auto &forest = prepared.keeper_items;
			if (forest.size() > PLAYER_SNAPSHOT_MAX_OBJECTS ||
			    native.keeper_image.size() != forest.size())
				return false;
			const item_owner_identity owner{
				item_owner_type::shopkeeper,
				item_shopkeeper_owner_id(found->second.payload.shop_id), 0
			};
			if (!item_owner_identity_valid(owner))
				return false;
			std::vector<uint64_t> roots(forest.size());
			std::set<uint64_t> physical_ids;
			std::vector<item_ownership_runtime_entry> custody;
			custody.reserve(forest.size());
			for (size_t index = 0; index < forest.size(); ++index)
			{
				const auto &item = forest[index];
				if (!item.object_uid || item.object_uid == UINT64_MAX ||
				    item.vnum <= 0 ||
				    item.parent_index < PLAYER_SNAPSHOT_NO_PARENT ||
				    item.parent_index >= static_cast<int32_t>(index))
					return false;
				const auto parent = item.parent_index;
				roots[index] = parent < 0 ? item.object_uid : roots[parent];
				const auto row = native.keeper_image.find(item.object_uid);
				if (row == native.keeper_image.end() || !row->second.id ||
				    !row->second.revision || row->second.root_uid != roots[index] ||
				    row->second.slot != item.equipment_slot ||
				    !physical_ids.insert(row->second.id).second)
					return false;
				const uint64_t parent_uid = parent < 0 ? 0 :
									 forest[parent].object_uid;
				const uint64_t parent_id =
					parent < 0 ? 0 : native.keeper_image.at(parent_uid).id;
				if (row->second.parent_id != parent_id)
					return false;
				auto expected = item;
				expected.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
				std::vector<uint8_t> expected_bytes, native_bytes;
				if (player_item_snapshot_list_encode({ expected },
								     &expected_bytes) !=
					    player_snapshot_codec_result::ok ||
				    player_item_snapshot_list_encode({ row->second.item },
								     &native_bytes) !=
					    player_snapshot_codec_result::ok ||
				    expected_bytes != native_bytes)
					return false;
				// payload_present records the original BEFORE. Only this private
				// committed disposition follows the native owner's exact AFTER
				// sidecar verification, same-cut readback and clean session finish.
				// Retain every BEFORE field; project its unchanged current custody.
				custody.push_back({ item.object_uid, roots[index], parent_uid,
						    owner, row->second.revision,
						    native.keeper_owner_revision, item.vnum,
						    item_custody_state::active });
			}
			// Allocation/conflict failure leaves sealed/attempted/held ownership
			// intact. The original native retry resolves the same SQL cut again;
			// rolled-back or uncertain dispositions never hydrate this registry.
			if (!item_ownership_runtime_hydrate_many_atomic(custody.data(),
									custody.size()))
				return false;
		}
		catch (...)
		{
			return false;
		}
		prepared.native_ready = true;
		return true;
	case shop_trade_native_checkpoint_disposition::never_mutated_retired:
		// Exact original session closure plus absence of a sealed stage proves
		// no native mutation could have been issued. It proves no COMMIT outcome.
		if (prepared.native_sealed)
			return false;
		[[fallthrough]];
	case shop_trade_native_checkpoint_disposition::rolled_back:
		prepared.native_stage = {};
		prepared.native_sealed = prepared.native_attempted = false;
		return true;
	case shop_trade_native_checkpoint_disposition::uncertain:
		return false;
	}
	return false;
}
#endif

namespace
{
bool flat_preparation_tree_bounded(P_obj root, std::vector<uint8_t> *bytes,
				   std::vector<uint64_t> *uids, size_t max_items, size_t max_bytes,
				   size_t *estimated_bytes)
{
	if (!root)
	{
		bytes->clear();
		return true;
	}
	std::vector<player_item_snapshot> items;
	std::vector<uint8_t> encoded;
	size_t estimated = 0;
	if (player_item_snapshot_tree_capture_literal(root, &items, &estimated) !=
		    player_snapshot_capture_result::ok ||
	    items.empty() || items.size() > max_items ||
	    player_item_snapshot_list_encode(items, &encoded) != player_snapshot_codec_result::ok ||
	    encoded.size() > max_bytes)
		return false;
	if (uids)
		for (const auto &item : items)
			uids->push_back(item.object_uid);
	*bytes = std::move(encoded);
	if (estimated_bytes)
		*estimated_bytes = estimated;
	return true;
}

bool flat_preparation_tree(P_obj root, std::vector<uint8_t> *bytes,
			   std::vector<uint64_t> *uids = nullptr, size_t *estimated_bytes = nullptr)
{
	return flat_preparation_tree_bounded(root, bytes, uids, SHOP_TRADE_MAX_ITEMS,
					     SHOP_TRADE_ITEM_BLOB_MAX_BYTES, estimated_bytes);
}

bool flat_preparation_destination_tree(P_obj root, std::vector<uint8_t> *bytes,
				       std::vector<uint64_t> *uids = nullptr,
				       size_t *estimated_bytes = nullptr)
{
	return flat_preparation_tree_bounded(root, bytes, uids, PLAYER_SNAPSHOT_MAX_OBJECTS,
					     PLAYER_SNAPSHOT_MAX_BYTES - sizeof(uint32_t),
					     estimated_bytes);
}

bool flat_same_mapping(const economic_shop_checkpoint_projection &a,
		       const economic_shop_checkpoint_projection &b)
{
	return a.lineage.bytes == b.lineage.bytes && a.epoch.bytes == b.epoch.bytes &&
	       economic_account_key_equal(a.wallet, b.wallet) &&
	       economic_account_key_equal(a.bank, b.bank);
}

bool flat_preparation_body(P_char actor, P_char keeper, uint32_t shop)
{
	return nevent_is_game_thread() && economic_gameplay_authority::active_regular_flat() &&
	       persistence_mode_get() == PERSISTENCE_MODE_FLATFILE_PRIMARY &&
	       persistence_mode_flatfile_root() && *persistence_mode_flatfile_root() && actor &&
	       IS_PC(actor) && actor->only.pc && GET_PID(actor) > 0 && actor->runtime_id &&
	       find_character_by_runtime_id(actor->runtime_id) == actor && keeper &&
	       IS_NPC(keeper) && keeper->runtime_id &&
	       find_character_by_runtime_id(keeper->runtime_id) == keeper && shop_index &&
	       number_of_shops > 0 && shop < static_cast<uint32_t>(number_of_shops) &&
	       GET_RNUM(keeper) == shop_index[shop].keeper && GET_VNUM(keeper) > 0;
}
// Real retained object structures and actual allocation capacities, checked
// against the original literal aggregate. No wire-size multiplier or new cap.
struct flat_preparation_bytes
{
	size_t value = 0;
	bool add(size_t amount) noexcept
	{
		if (amount > PLAYER_SAVE_PIPELINE_MAX_BYTES - value)
			return false;
		value += amount;
		return true;
	}
	template <class T> bool vector(const std::vector<T> &v) noexcept
	{
		return v.capacity() <= PLAYER_SAVE_PIPELINE_MAX_BYTES / sizeof(T) &&
		       add(v.capacity() * sizeof(T));
	}
	bool string(const std::string &s) noexcept
	{
		// Inline string bytes already belong to their containing sizeof charge.
		const uintptr_t data = reinterpret_cast<uintptr_t>(s.data());
		const uintptr_t object = reinterpret_cast<uintptr_t>(&s);
		if (data >= object && data - object < sizeof(s))
			return true;
		return add(s.capacity()) && add(1);
	}
	bool items(const std::vector<player_item_snapshot> &v) noexcept
	{
		if (!vector(v))
			return false;
		for (const auto &item : v)
		{
			if (!string(item.name) || !string(item.short_description) ||
			    !string(item.description) || !string(item.action_description) ||
			    !vector(item.dynamic_affects) || !vector(item.extra_descriptions))
				return false;
			for (const auto &description : item.extra_descriptions)
				if (!string(description.keyword) ||
				    !string(description.description) ||
				    !vector(description.spell_ids))
					return false;
		}
		return true;
	}
	bool evidence(const player_death_evidence_table &table) noexcept
	{
		if (!vector(table.columns) || !vector(table.rows))
			return false;
		for (const auto &column : table.columns)
			if (!string(column))
				return false;
		for (const auto &row : table.rows)
		{
			if (!vector(row))
				return false;
			for (const auto &field : row)
				if (field && !string(*field))
					return false;
		}
		return true;
	}
	bool snapshot(const player_snapshot &s) noexcept
	{
		if (!vector(s.status_integers) || !vector(s.status_strings) ||
		    !vector(s.languages) || !vector(s.introductions) || !vector(s.timers) ||
		    !vector(s.undead_slots) || !vector(s.forged_items) ||
		    !vector(s.granted_commands) || !vector(s.skills) || !vector(s.affects) ||
		    !items(s.items) || !vector(s.pets) || !vector(s.shapes) ||
		    !vector(s.trophies) || !vector(s.quest_xp_receipts) ||
		    !vector(s.spell_effect_receipts) || !vector(s.craft_receipts) ||
		    !string(s.output_preferences))
			return false;
		for (const auto &field : s.status_strings)
			if (!string(field.value))
				return false;
		for (const auto &affect : s.affects)
			if (!string(affect.wear_off_character) || !string(affect.wear_off_room))
				return false;
		for (const auto &pet : s.pets)
			if (!items(pet.items) || !string(pet.restore_state))
				return false;
		if (s.death)
		{
			if (!items(s.death->corpse) || !vector(s.death->custody) ||
			    !vector(s.death->unresolved_operations))
				return false;
			if (s.death->conflict_evidence)
			{
				const auto &e = *s.death->conflict_evidence;
				if (!evidence(e.player_items) || !evidence(e.player_item_affects) ||
				    !evidence(e.player_item_extra_descr) ||
				    !evidence(e.item_current_owner) ||
				    !evidence(e.item_owner_revision))
					return false;
			}
		}
		return true;
	}
	bool custody(const std::vector<flatfile_item_ownership_record> &rows) noexcept
	{
		if (!vector(rows))
			return false;
		for (const auto &row : rows)
			if (!vector(row.coin_payload))
				return false;
		return true;
	}
	bool keeper(const flatfile_shopkeeper_record &record) noexcept
	{
		return vector(record.affects) && items(record.items);
	}
	bool manifest(const shop_trade_recovery_manifest &m) noexcept
	{
		return vector(m.player_before.ordered_item_uids) &&
		       vector(m.player_after.ordered_item_uids) &&
		       vector(m.keeper_before.ordered_item_uids) &&
		       vector(m.keeper_after.ordered_item_uids) &&
		       vector(m.live_target_before.ordered_item_uids) &&
		       vector(m.live_target_after.ordered_item_uids);
	}
};

// Values derive only from the genuine private retained stage and original
// captured selected tree. This does not hydrate or treat runtime cache as storage.
bool flat_accounted_payload(const pending_trade &entry,
			    const shop_trade_flat_native_checkpoint_stage &native,
			    shop_trade_payload *output)
{
	if (!output || !entry.preparation || !entry.preparation->flat)
		return false;
	const auto &prepared = *entry.preparation;
	const auto &source = entry.payload;
	const bool creates = source.action == shop_trade_action::buy_produced;
	const bool shop_owned = creates || source.action == shop_trade_action::buy_existing ||
				source.action == shop_trade_action::discard_invalid;
	const item_owner_identity player_owner{ item_owner_type::player, entry.player_pid, 0 };
	const item_owner_identity keeper_owner{ item_owner_type::shopkeeper,
						item_shopkeeper_owner_id(source.shop_id), 0 };
	const auto expected_owner = shop_owned ? keeper_owner : player_owner;
	std::map<uint64_t, const flatfile_item_ownership_record *> rows;
	const auto index = [&](const auto &custody)
	{
		for (const auto &row : custody)
			if (!rows.emplace(row.item_uid, &row).second)
				return false;
		return true;
	};
	if (!index(native.player.player_custody) || !index(native.keeper_custody))
		return false;
	for (const auto &pet : native.player.pet_custody)
		if (!index(pet.custody))
			return false;
	std::vector<player_item_snapshot> selected;
	if (player_item_snapshot_list_decode(prepared.selected.data(), prepared.selected.size(),
					     &selected) != player_snapshot_codec_result::ok ||
	    selected.empty() || selected.size() > SHOP_TRADE_MAX_ITEMS ||
	    selected.front().object_uid != source.selected_item_uid ||
	    prepared.selected.size() > SHOP_TRADE_ITEM_BLOB_MAX_BYTES)
		return false;
	shop_trade_payload payload{};
	payload.action = source.action;
	payload.player_pid = entry.player_pid;
	payload.shop_id = source.shop_id;
	payload.racewar = source.racewar;
	payload.account_name = source.account_name;
	payload.price = source.price;
	payload.keeper_vnum = source.keeper_vnum;
	payload.expected_keeper_cash = source.expected_keeper_cash;
	payload.keeper_roaming = source.keeper_roaming;
	payload.expected_wallet_revision = native.player.native_money.domains.wallet_revision;
	payload.expected_bank_revision = native.player.native_money.domains.bank_revision;
	payload.expected_shop_revision = native.keeper_after.revision;
	payload.expected_player_save_revision = native.acknowledged_status.save_revision;
	payload.expected_player_level = native.acknowledged_status.level;
	payload.selected_item_uid = source.selected_item_uid;
	payload.target_root_item_uid = source.selected_item_uid;
	payload.item_count = static_cast<uint16_t>(selected.size());
	payload.item_blob_size = static_cast<uint32_t>(prepared.selected.size());
	std::copy(prepared.selected.begin(), prepared.selected.end(), payload.item_blob.begin());
	for (size_t i = 0; i < selected.size(); ++i)
	{
		const auto &item = selected[i];
		if (item.parent_index != PLAYER_SNAPSHOT_NO_PARENT &&
		    (item.parent_index < 0 || static_cast<size_t>(item.parent_index) >= i))
			return false;
		const uint64_t parent =
			item.parent_index == PLAYER_SNAPSHOT_NO_PARENT ?
				0 :
				selected[static_cast<size_t>(item.parent_index)].object_uid;
		const auto found = rows.find(item.object_uid);
		item_ownership_runtime_entry runtime{};
		if (creates)
		{
			if (found != rows.end() ||
			    item_ownership_runtime_lookup(item.object_uid, &runtime))
				return false;
		}
		else if (found == rows.end() ||
			 !item_owner_identity_equal(found->second->owner, expected_owner) ||
			 found->second->state != item_custody_state::active ||
			 !found->second->item_revision ||
			 found->second->root_item_uid != source.selected_item_uid ||
			 found->second->parent_item_uid != parent ||
			 found->second->vnum != item.vnum)
			return false;
		payload.items[i] = {
			item.object_uid,
			source.selected_item_uid,
			parent,
			creates ? ITEM_TRANSFER_ABSENT_REVISION : found->second->item_revision,
			item.vnum,
			creates ? item_custody_state::absent : item_custody_state::active
		};
	}
	std::sort(payload.items.begin(), payload.items.begin() + payload.item_count,
		  [](const auto &a, const auto &b) { return a.item_uid < b.item_uid; });
	const auto current_root =
		[&](uint64_t uid,
		    const item_owner_identity &owner) -> const flatfile_item_ownership_record *
	{
		const auto found = rows.find(uid);
		if (found == rows.end() ||
		    !item_owner_identity_equal(found->second->owner, owner) ||
		    found->second->state != item_custody_state::active ||
		    !found->second->item_revision || found->second->root_item_uid != uid ||
		    found->second->parent_item_uid)
			return nullptr;
		return found->second;
	};
	if (creates)
	{
		const auto *stock = current_root(source.stock_item_uid, keeper_owner);
		std::vector<player_item_snapshot> original_stock;
		if (!stock || stock->item_uid == source.selected_item_uid ||
		    stock->vnum != selected.front().vnum ||
		    player_item_snapshot_list_decode(prepared.stock.data(), prepared.stock.size(),
						     &original_stock) !=
			    player_snapshot_codec_result::ok ||
		    original_stock.empty() ||
		    original_stock.front().object_uid != stock->item_uid ||
		    original_stock.front().vnum != stock->vnum)
			return false;
		payload.stock_item_uid = stock->item_uid;
		payload.expected_stock_item_revision = stock->item_revision;
		payload.stock_vnum = stock->vnum;
		if (source.target_parent_item_uid)
		{
			const auto *target =
				current_root(source.target_parent_item_uid, player_owner);
			std::vector<player_item_snapshot> original_target;
			if (!target || target->item_uid == source.selected_item_uid ||
			    player_item_snapshot_list_decode(
				    prepared.destination.data(), prepared.destination.size(),
				    &original_target) != player_snapshot_codec_result::ok ||
			    original_target.empty() ||
			    original_target.front().object_uid != target->item_uid ||
			    original_target.front().vnum != target->vnum)
				return false;
			payload.target_root_item_uid = target->root_item_uid;
			payload.target_parent_item_uid = target->item_uid;
			payload.expected_target_parent_revision = target->item_revision;
		}
	}
	else if (shop_owned)
	{
		const auto *stock = current_root(source.selected_item_uid, keeper_owner);
		if (!stock)
			return false;
		payload.stock_item_uid = stock->item_uid;
		payload.expected_stock_item_revision = stock->item_revision;
		payload.stock_vnum = stock->vnum;
	}
	std::vector<uint8_t> encoded;
	if (!shop_trade_command_encode_accounted_payload(payload, &encoded))
		return false;
	*output = std::move(payload);
	return true;
}

bool flat_command_retained_bytes(const critical_command &command, size_t *output) noexcept
{
	flat_preparation_bytes bytes;
	if (!output || !bytes.add(sizeof(command)) || !bytes.vector(command.keys) ||
	    !bytes.vector(command.expected_revisions) || !bytes.vector(command.payload) ||
	    !bytes.vector(command.accounting_intent))
		return false;
	*output = bytes.value;
	return true;
}

bool flat_native_retained_bytes(const pending_trade &entry,
				const shop_trade_flat_native_checkpoint_stage &stage,
				size_t *output) noexcept
{
	if (!output || !entry.preparation || !entry.preparation->flat || entry.cold ||
	    entry.preparation->flat->native || entry.preparation->submission_started ||
	    entry.preparation->command || entry.production_owned)
		return false;
	const auto &prepared = *entry.preparation;
#ifndef __NO_MYSQL__
	if (prepared.native_attempted || prepared.native_sealed || prepared.native_ready ||
	    !prepared.native_stage.keeper_image.empty())
		return false;
#endif
	flat_preparation_bytes bytes;
	if (!bytes.add(sizeof(decltype(pending)::value_type)) ||
	    !bytes.add(sizeof(trade_preparation)) || !bytes.add(sizeof(flat_trade_preparation)) ||
	    !bytes.add(sizeof(stage)) || !bytes.string(prepared.flat->selected_root) ||
	    !bytes.vector(prepared.selected) || !bytes.vector(prepared.stock) ||
	    !bytes.vector(prepared.destination) || !bytes.vector(prepared.keeper_blob) ||
	    !bytes.items(prepared.keeper_items) || !bytes.vector(prepared.fenced_uids) ||
	    !bytes.manifest(entry.payload.recovery_manifest) || !bytes.items(entry.before_player) ||
	    !bytes.items(entry.after_player) || !bytes.items(entry.after_keeper) ||
	    !bytes.vector(entry.after_destination) ||
	    !bytes.string(stage.player_hold_cut.selected_root) ||
	    !bytes.snapshot(stage.original_queued_ack) ||
	    !bytes.vector(stage.player.authority.mappings))
		return false;
	for (const auto &mapping : stage.player.authority.mappings)
		if (!bytes.string(mapping.locator.name))
			return false;
	if (!bytes.string(stage.player.native_money.account_name) ||
	    !bytes.vector(stage.player.native_money.recent_pvp_deaths) ||
	    !bytes.vector(stage.player.native_money.completed_epic_zones) ||
	    !bytes.snapshot(stage.player.player) || !bytes.custody(stage.player.player_custody) ||
	    !bytes.vector(stage.player.pet_custody) || !bytes.custody(stage.keeper_custody) ||
	    !bytes.keeper(stage.keeper_before) || !bytes.keeper(stage.keeper_after) ||
	    !bytes.string(stage.catalog_before.filename) ||
	    !bytes.vector(stage.catalog_before.bytes) ||
	    !bytes.string(stage.catalog_after.filename) || !bytes.vector(stage.catalog_after.bytes))
		return false;
	for (const auto &pet : stage.player.pet_custody)
		if (!bytes.custody(pet.custody))
			return false;
	*output = bytes.value;
	return bytes.value != 0;
}

}

shop_trade_preparation_state shop_trade_preparation_owner::begin_flat(
	P_char actor, P_char keeper, P_obj selected, P_obj stock, P_obj destination, uint32_t shop,
	shop_trade_action action, int64_t price, shop_trade_flat_preparation_token *output) noexcept
{
	if (!nevent_is_game_thread() || !output || !flat_preparation_body(actor, keeper, shop) ||
	    !selected || !selected->obj_uid || price < 0 || price > INT_MAX ||
	    action <= shop_trade_action::unknown || action > shop_trade_action::sell_destroy ||
	    ((action == shop_trade_action::buy_produced) != (stock != nullptr)) ||
	    (destination && action != shop_trade_action::buy_produced) ||
	    ((action == shop_trade_action::sell_store ||
	      action == shop_trade_action::sell_destroy) &&
	     (!price || !OBJ_CARRIED_BY(selected, actor))) ||
	    (action == shop_trade_action::buy_existing && !OBJ_CARRIED_BY(selected, keeper)) ||
	    (stock && !OBJ_CARRIED_BY(stock, keeper)) ||
	    (destination && !OBJ_CARRIED_BY(destination, actor)) ||
	    pending.size() + notifying >= SHOP_TRADE_PENDING_MAX ||
	    player_pending(static_cast<uint32_t>(GET_PID(actor))) ||
	    shop_trade_transaction_keeper_busy(shop) ||
	    preparation_generation == std::numeric_limits<uint64_t>::max())
		return shop_trade_preparation_state::refused;
	critical_operation_id operation{};
	bool owns_entry = false;
	try
	{
		const char *account = get_account_name_safe(actor);
		if (!account || !strcmp(account, "Unknown") ||
		    strlen(account) > CURRENCY_ACCOUNT_NAME_MAX_BYTES ||
		    GET_RACEWAR(actor) > INT8_MAX)
			return shop_trade_preparation_state::refused;
		const int64_t keeper_cash = static_cast<int64_t>(GET_COPPER(keeper)) +
					    10LL * GET_SILVER(keeper) + 100LL * GET_GOLD(keeper) +
					    1000LL * GET_PLATINUM(keeper);
		if (GET_COPPER(keeper) < 0 || GET_SILVER(keeper) < 0 || GET_GOLD(keeper) < 0 ||
		    GET_PLATINUM(keeper) < 0 || keeper_cash < 0 || keeper_cash > INT_MAX)
			return shop_trade_preparation_state::refused;
		auto prepared = std::make_unique<trade_preparation>();
		prepared->flat = std::make_unique<flat_trade_preparation>();
		prepared->flat->selected_root = persistence_mode_flatfile_root();
		if (!economic_gameplay_authority::observe_flat_shop_checkpoint(
			    GET_PID(actor), account, static_cast<uint8_t>(GET_RACEWAR(actor)),
			    &prepared->mapping) ||
		    !flat_preparation_tree(selected, &prepared->selected, &prepared->fenced_uids,
					   &prepared->selected_capture_bytes) ||
		    !flat_preparation_tree(stock, &prepared->stock, &prepared->fenced_uids) ||
		    !flat_preparation_destination_tree(destination, &prepared->destination,
						       &prepared->fenced_uids,
						       &prepared->destination_capture_bytes) ||
		    !shop_item_runtime_capture_keeper_literal(keeper, &prepared->keeper_items) ||
		    player_item_snapshot_list_encode(prepared->keeper_items,
						     &prepared->keeper_blob) !=
			    player_snapshot_codec_result::ok)
			return shop_trade_preparation_state::refused;
		for (const auto &item : prepared->keeper_items)
			prepared->fenced_uids.push_back(item.object_uid);
		std::sort(prepared->fenced_uids.begin(), prepared->fenced_uids.end());
		prepared->fenced_uids.erase(std::unique(prepared->fenced_uids.begin(),
							prepared->fenced_uids.end()),
					    prepared->fenced_uids.end());
		for (const auto uid : prepared->fenced_uids)
			if (shop_trade_transaction_item_busy(uid))
				return shop_trade_preparation_state::refused;
		if (!critical_operation_id_generate(&operation))
			return shop_trade_preparation_state::refused;
		prepared->generation = ++preparation_generation;
		prepared->actor_runtime_id = actor->runtime_id;
		prepared->keeper_runtime_id = keeper->runtime_id;
		const uint64_t generation = prepared->generation;
		pending_trade entry{};
		entry.player_pid = static_cast<uint32_t>(GET_PID(actor));
		entry.payload.player_pid = entry.player_pid;
		entry.payload.shop_id = shop;
		entry.payload.action = action;
		entry.payload.price = price;
		entry.payload.keeper_vnum = GET_VNUM(keeper);
		entry.payload.expected_keeper_cash = keeper_cash;
		entry.payload.keeper_roaming = shop_index[shop].shop_is_roaming != 0;
		entry.payload.selected_item_uid = selected->obj_uid;
		entry.payload.stock_item_uid = stock ? stock->obj_uid : 0;
		entry.payload.target_parent_item_uid = destination ? destination->obj_uid : 0;
		entry.payload.racewar = static_cast<uint8_t>(GET_RACEWAR(actor));
		strcpy(entry.payload.account_name.data(), account);
		entry.preparation = std::move(prepared);
		auto [found, inserted] = pending.emplace(operation.bytes, std::move(entry));
		if (!inserted)
			return shop_trade_preparation_state::refused;
		owns_entry = true;
		shop_trade_flat_preparation_token token;
		token.operation_id_ = operation;
		token.generation_ = generation;
		*output = token;
		// Own every original body/UID before the one native shell probe. An
		// uncertain construction/extraction tail retains this same preparation.
		if (destination)
		{
			auto &owned = *found->second.preparation;
			std::vector<player_item_snapshot> target, source;
			if (player_item_snapshot_list_decode(owned.destination.data(),
							     owned.destination.size(), &target) !=
				    player_snapshot_codec_result::ok ||
			    player_item_snapshot_list_decode(owned.selected.data(),
							     owned.selected.size(), &source) !=
				    player_snapshot_codec_result::ok ||
			    target.empty() || source.empty() ||
			    target.front().type != ITEM_CONTAINER ||
			    source.size() > PLAYER_SNAPSHOT_MAX_OBJECTS - target.size())
			{
				pending.erase(found);
				return shop_trade_preparation_state::refused;
			}
			// Native capture charges structures/dynamic rows in addition to
			// wire bytes. Two separate captures count the envelope twice.
			const size_t envelope = sizeof(player_snapshot);
			if (owned.selected_capture_bytes < envelope ||
			    owned.destination_capture_bytes < envelope ||
			    owned.destination_capture_bytes > PLAYER_SNAPSHOT_MAX_BYTES ||
			    owned.selected_capture_bytes - envelope >
				    PLAYER_SNAPSHOT_MAX_BYTES - owned.destination_capture_bytes)
			{
				pending.erase(found);
				return shop_trade_preparation_state::refused;
			}
			int32_t shell = 0, direct = 0;
			if (target.front().values[4] > 0)
			{
				int64_t sum = 0;
				for (size_t index = 1; index < target.size(); ++index)
					if (target[index].parent_index == 0)
					{
						sum += target[index].weight;
						if (sum < INT32_MIN || sum > INT32_MAX)
						{
							pending.erase(found);
							return shop_trade_preparation_state::refused;
						}
					}
				direct = static_cast<int32_t>(sum);
				owned.destination_shell_started = true;
				const bool captured =
					obj_capture_container_shell_weight(destination, &shell);
				owned.destination_shell_returned = true;
				if (!captured)
				{
					pending.erase(found);
					return shop_trade_preparation_state::refused;
				}
				// Hooks may touch the world. Resolve the original bodies again
				// and compare all literal source values before any checkpoint.
				actor = find_character_by_runtime_id(owned.actor_runtime_id);
				keeper = find_character_by_runtime_id(owned.keeper_runtime_id);
				if (!flat_preparation_body(actor, keeper, shop))
					return shop_trade_preparation_state::pending;
				std::vector<uint8_t> current_target, current_selected;
				// The original UID census is established again by poll before
				// SQL; pointers returned across a probe are never retained.
				P_obj original_destination = nullptr, original_selected = nullptr;
				size_t count = 0;
				for (P_obj object = actor->carrying; object;
				     object = object->next_content)
				{
					if (++count > PLAYER_SNAPSHOT_MAX_OBJECTS ||
					    !OBJ_CARRIED_BY(object, actor))
						return shop_trade_preparation_state::pending;
					if (object->obj_uid ==
					    found->second.payload.target_parent_item_uid)
					{
						if (original_destination)
							return shop_trade_preparation_state::pending;
						original_destination = object;
					}
				}
				// Produced selection is detached, so resolve it from the global
				// object list through the same original UID; never read a stale pointer.
				extern P_obj object_list;
				count = 0;
				for (P_obj object = object_list; object; object = object->next)
				{
					if (++count > 262144)
						return shop_trade_preparation_state::pending;
					if (object->obj_uid ==
					    found->second.payload.selected_item_uid)
					{
						if (original_selected)
							return shop_trade_preparation_state::pending;
						original_selected = object;
					}
				}
				if (!original_destination || !original_selected ||
				    !flat_preparation_destination_tree(original_destination,
								       &current_target) ||
				    !flat_preparation_tree(original_selected, &current_selected) ||
				    current_target != owned.destination ||
				    current_selected != owned.selected)
					return shop_trade_preparation_state::pending;
				destination = original_destination;
				selected = original_selected;
			}
			if (!shop_trade_destination_weight_compute(
				    target.front(), source.front().weight, direct, shell,
				    &owned.destination_weight))
			{
				pending.erase(found);
				return shop_trade_preparation_state::refused;
			}
			// The complete future literal destination must fit the same capture
			// envelope now, before checkpoint/SQL/admission. Selected STORE/key
			// transforms are fixed-width and cannot enlarge this encoding.
			auto combined = target;
			combined.front().weight = owned.destination_weight.after;
			const auto base = static_cast<int32_t>(combined.size());
			for (auto item : source)
			{
				item.parent_index =
					item.parent_index < 0 ? 0 : base + item.parent_index;
				combined.push_back(std::move(item));
			}
			std::vector<uint8_t> encoded;
			if (player_item_snapshot_list_encode(combined, &encoded) !=
				    player_snapshot_codec_result::ok ||
			    encoded.size() > PLAYER_SNAPSHOT_MAX_BYTES - sizeof(uint32_t))
			{
				pending.erase(found);
				return shop_trade_preparation_state::refused;
			}
		}
		found->second.preparation->destination_weight_ready = true;

		// The domain entry already owns the keeper and every frozen UID while
		// the ordinary save request captures/queues the player's checkpoint.
		const bool selling = action == shop_trade_action::sell_store ||
				     action == shop_trade_action::sell_destroy;
		const auto state = player_save_pipeline_flat_shop_checkpoint_begin(
			actor, selling ? selected : nullptr, NOWHERE,
			&found->second.preparation->flat->player_token);
		if (state == player_literal_inventory_state::refused)
		{
			pending.erase(found);
			return shop_trade_preparation_state::refused;
		}
		return shop_trade_preparation_state::pending;
	}
	catch (...)
	{
		// A matching generated ID is not ownership if insertion failed.
		if (!owns_entry)
			return shop_trade_preparation_state::refused;
		auto found = pending.find(operation.bytes);
		if (found == pending.end())
			return shop_trade_preparation_state::refused;
		if (found->second.preparation->destination_shell_started &&
		    !found->second.preparation->destination_shell_returned)
			return shop_trade_preparation_state::pending;
		const auto &player = found->second.preparation->flat->player_token;
		// The pipeline exposes the installed slot before enqueue. A zero
		// token proves this preparation never acquired a player checkpoint;
		// no keeper write or critical admission has been attempted yet.
		if (!player.pid && !player.actor_runtime_id && !player.generation)
		{
			pending.erase(found);
			return shop_trade_preparation_state::refused;
		}
		return shop_trade_preparation_state::pending;
	}
}

shop_trade_preparation_state
shop_trade_preparation_owner::poll_flat(const shop_trade_flat_preparation_token &token,
					P_char actor, P_char keeper, P_obj selected, P_obj stock,
					P_obj destination) noexcept
{
	if (!nevent_is_game_thread())
		return shop_trade_preparation_state::refused;
	auto found = pending.find(token.operation_id_.bytes);
	if (!nevent_is_game_thread() || found == pending.end() || !found->second.preparation ||
	    !found->second.preparation->flat ||
	    found->second.preparation->generation != token.generation_)
		return shop_trade_preparation_state::refused;
	auto &entry = found->second;
	auto &prepared = *entry.preparation;
	if (prepared.flat->phase != shop_trade_flat_native_checkpoint_phase::preparing ||
	    !persistence_mode_flatfile_root() ||
	    prepared.flat->selected_root != persistence_mode_flatfile_root() ||
	    prepared.submission_started ||
	    (prepared.destination_shell_started && !prepared.destination_shell_returned) ||
	    !prepared.destination_weight_ready)
		return shop_trade_preparation_state::refused;
	try
	{
		if (!flat_preparation_body(actor, keeper, entry.payload.shop_id) ||
		    actor->runtime_id != prepared.actor_runtime_id ||
		    keeper->runtime_id != prepared.keeper_runtime_id ||
		    static_cast<uint32_t>(GET_PID(actor)) != entry.player_pid || !selected ||
		    selected->obj_uid != entry.payload.selected_item_uid ||
		    (stock ? stock->obj_uid : 0) != entry.payload.stock_item_uid ||
		    (destination ? destination->obj_uid : 0) !=
			    entry.payload.target_parent_item_uid ||
		    GET_VNUM(keeper) != entry.payload.keeper_vnum ||
		    (shop_index[entry.payload.shop_id].shop_is_roaming != 0) !=
			    (entry.payload.keeper_roaming != 0) ||
		    GET_RACEWAR(actor) != entry.payload.racewar ||
		    ((entry.payload.action == shop_trade_action::sell_store ||
		      entry.payload.action == shop_trade_action::sell_destroy) &&
		     !OBJ_CARRIED_BY(selected, actor)) ||
		    (entry.payload.action == shop_trade_action::buy_existing &&
		     !OBJ_CARRIED_BY(selected, keeper)) ||
		    (stock && !OBJ_CARRIED_BY(stock, keeper)) ||
		    (destination && !OBJ_CARRIED_BY(destination, actor)))
			return shop_trade_preparation_state::refused;
		const int64_t keeper_cash = static_cast<int64_t>(GET_COPPER(keeper)) +
					    10LL * GET_SILVER(keeper) + 100LL * GET_GOLD(keeper) +
					    1000LL * GET_PLATINUM(keeper);
		if (GET_COPPER(keeper) < 0 || GET_SILVER(keeper) < 0 || GET_GOLD(keeper) < 0 ||
		    GET_PLATINUM(keeper) < 0 || keeper_cash != entry.payload.expected_keeper_cash)
			return shop_trade_preparation_state::refused;
		const char *account = get_account_name_safe(actor);
		economic_shop_checkpoint_projection mapping{};
		std::vector<uint8_t> a, b, c, d;
		std::vector<player_item_snapshot> keeper_items;
		if (!account || strcmp(account, entry.payload.account_name.data()) ||
		    !economic_gameplay_authority::observe_flat_shop_checkpoint(
			    entry.player_pid, account, entry.payload.racewar, &mapping) ||
		    !flat_same_mapping(mapping, prepared.mapping) ||
		    !flat_preparation_tree(selected, &a) || a != prepared.selected ||
		    !flat_preparation_tree(stock, &b) || b != prepared.stock ||
		    !flat_preparation_destination_tree(destination, &c) ||
		    c != prepared.destination ||
		    !shop_item_runtime_capture_keeper_literal(keeper, &keeper_items) ||
		    player_item_snapshot_list_encode(keeper_items, &d) !=
			    player_snapshot_codec_result::ok ||
		    d != prepared.keeper_blob)
			return shop_trade_preparation_state::refused;
		if (prepared.flat->player_held)
		{
			player_shop_checkpoint_stage held{};
			player_snapshot original_ack;
			economic_shop_checkpoint_projection held_mapping;
			player_flat_shop_checkpoint_cut cut;
			if (!player_save_shop_checkpoint_owner::observe_held_flat(
				    prepared.flat->player_token, actor, token.operation_id_,
				    &original_ack, &held, &held_mapping, &cut) ||
			    !flat_same_mapping(held_mapping, prepared.mapping) ||
			    cut.selected_root != prepared.flat->selected_root ||
			    held.save_revision != prepared.flat->player_stage.save_revision ||
			    held.level != prepared.flat->player_stage.level)
				return shop_trade_preparation_state::refused;
			return shop_trade_preparation_state::ready;
		}
		player_shop_checkpoint_stage stage{};
		const auto state = player_save_pipeline_flat_shop_checkpoint_poll(
			prepared.flat->player_token, actor, &stage);
		if (state == player_literal_inventory_state::pending)
			return shop_trade_preparation_state::pending;
		if (state != player_literal_inventory_state::database_acknowledged ||
		    !player_save_shop_checkpoint_owner::hold_flat(prepared.flat->player_token,
								  token.operation_id_))
			return shop_trade_preparation_state::refused;
		prepared.flat->player_stage = stage;
		prepared.flat->player_held = true;
		return shop_trade_preparation_state::ready;
	}
	catch (...)
	{
		return shop_trade_preparation_state::refused;
	}
}

bool shop_trade_preparation_owner::cancel_flat(
	const shop_trade_flat_preparation_token &token) noexcept
{
	if (!nevent_is_game_thread())
		return false;
	auto found = pending.find(token.operation_id_.bytes);
	if (found == pending.end() || !found->second.preparation ||
	    !found->second.preparation->flat ||
	    found->second.preparation->generation != token.generation_)
		return false;
	auto &prepared = *found->second.preparation;
	auto &flat = *prepared.flat;
	if (found->second.production_owned || prepared.submission_started ||
	    (prepared.destination_shell_started && !prepared.destination_shell_returned) ||
	    (flat.phase != shop_trade_flat_native_checkpoint_phase::preparing &&
	     flat.phase != shop_trade_flat_native_checkpoint_phase::sealed))
		return false;
	if (flat.player_token.pid || flat.player_token.actor_runtime_id ||
	    flat.player_token.generation)
	{
		const bool released =
			flat.player_held ?
				player_save_shop_checkpoint_owner::release_unattempted_flat(
					flat.player_token, token.operation_id_) :
				player_save_pipeline_flat_shop_checkpoint_cancel(flat.player_token);
		if (!released)
			return false;
	}
	else if (flat.player_held || flat.native)
		return false;
	pending.erase(found);
	return true;
}

bool shop_trade_preparation_owner::begin_native_checkpoint_flat(
	const shop_trade_flat_preparation_token &token, P_char actor, P_char keeper, P_obj selected,
	P_obj stock, P_obj destination, shop_trade_flat_checkpoint_context *context,
	const shop_trade_flat_native_checkpoint_stage **retained,
	shop_trade_flat_native_checkpoint_phase *phase) noexcept
{
	if (!nevent_is_game_thread() || !context || !retained || !phase)
		return false;
	auto found = pending.find(token.operation_id_.bytes);
	if (found == pending.end() || !found->second.preparation ||
	    !found->second.preparation->flat ||
	    found->second.preparation->generation != token.generation_)
		return false;
	auto &entry = found->second;
	auto &prepared = *entry.preparation;
	auto &flat = *prepared.flat;
	if (entry.production_owned || prepared.submission_started)
		return false;
	// An existing stage is an actor-independent original lifetime. This path
	// performs no gameplay recapture, factory, catalog staging or new attempt.
	if (flat.native)
	{
		if (!flat.player_held ||
		    flat.native->operation_id.bytes != token.operation_id_.bytes ||
		    flat.phase == shop_trade_flat_native_checkpoint_phase::preparing ||
		    !flat.reserved_bytes)
			return false;
		*retained = flat.native.get();
		*phase = flat.phase;
		return true; // context stays unchanged; the retained stage owns the full cut.
	}
	if (flat.phase != shop_trade_flat_native_checkpoint_phase::preparing ||
	    poll_flat(token, actor, keeper, selected, stock, destination) !=
		    shop_trade_preparation_state::ready)
		return false;
	try
	{
		shop_trade_flat_checkpoint_context copied;
		economic_shop_checkpoint_projection mapping;
		player_shop_checkpoint_stage status;
		if (!player_save_shop_checkpoint_owner::observe_held_flat(
			    flat.player_token, actor, token.operation_id_,
			    &copied.original_queued_ack, &status, &mapping,
			    &copied.player_hold_cut) ||
		    !flat_same_mapping(mapping, prepared.mapping) ||
		    copied.player_hold_cut.selected_root != flat.selected_root ||
		    status.save_revision != flat.player_stage.save_revision ||
		    status.level != flat.player_stage.level)
			return false;
		copied.original.operation_id = token.operation_id_;
		copied.original.generation = token.generation_;
		copied.original.actor_runtime_id = prepared.actor_runtime_id;
		copied.original.keeper_runtime_id = prepared.keeper_runtime_id;
		copied.original.player_pid = entry.player_pid;
		copied.original.shop_id = entry.payload.shop_id;
		copied.original.account_name = entry.payload.account_name;
		copied.original.racewar = entry.payload.racewar;
		copied.original.keeper_roaming = entry.payload.keeper_roaming;
		copied.original.keeper_cash = entry.payload.expected_keeper_cash;
		copied.original.keeper_vnum = entry.payload.keeper_vnum;
		copied.original.mapping = prepared.mapping;
		copied.original.player = status;
		copied.original.keeper_items = prepared.keeper_items;
		copied.player_token = flat.player_token;
		static_assert(
			std::is_nothrow_move_assignable_v<shop_trade_flat_checkpoint_context>);
		*context = std::move(copied);
		*retained = nullptr;
		*phase = shop_trade_flat_native_checkpoint_phase::preparing;
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool shop_trade_preparation_owner::seal_native_checkpoint_flat(
	const shop_trade_flat_preparation_token &token,
	std::unique_ptr<shop_trade_flat_native_checkpoint_stage> &&stage) noexcept
{
	if (!nevent_is_game_thread() || !stage)
		return false;
	auto found = pending.find(token.operation_id_.bytes);
	if (found == pending.end() || !found->second.preparation ||
	    !found->second.preparation->flat ||
	    found->second.preparation->generation != token.generation_)
		return false;
	auto &entry = found->second;
	auto &prepared = *entry.preparation;
	auto &flat = *prepared.flat;
	if (!flat.player_held || flat.native || flat.reserved_bytes ||
	    flat.phase != shop_trade_flat_native_checkpoint_phase::preparing ||
	    stage->operation_id.bytes != token.operation_id_.bytes ||
	    stage->player_token != flat.player_token ||
	    !flat_same_mapping(stage->mapping, prepared.mapping) ||
	    stage->player_hold_cut.selected_root != flat.selected_root ||
	    !stage->player_hold_cut.ownership_epoch ||
	    !stage->player_hold_cut.execution_hold_generation ||
	    stage->acknowledged_status.save_revision != flat.player_stage.save_revision ||
	    stage->acknowledged_status.level != flat.player_stage.level ||
	    stage->original_queued_ack.pid != static_cast<int32_t>(entry.player_pid) ||
	    stage->original_queued_ack.revision != flat.player_stage.save_revision ||
	    stage->player.player.pid != static_cast<int32_t>(entry.player_pid) ||
	    stage->player.player.revision != flat.player_stage.save_revision ||
	    stage->player.authority.lineage.bytes != prepared.mapping.lineage.bytes ||
	    stage->player.authority.epoch.bytes != prepared.mapping.epoch.bytes ||
	    !stage->keeper_owner_revision ||
	    stage->keeper_custody.size() != prepared.keeper_items.size() ||
	    stage->keeper_before.shop_id != entry.payload.shop_id ||
	    stage->keeper_after.shop_id != entry.payload.shop_id ||
	    !stage->keeper_before.revision || stage->keeper_before.revision == UINT64_MAX ||
	    stage->keeper_after.revision != stage->keeper_before.revision + 1 ||
	    stage->keeper_before.mob_vnum != entry.payload.keeper_vnum ||
	    stage->keeper_after.mob_vnum != entry.payload.keeper_vnum ||
	    stage->keeper_before.cash != entry.payload.expected_keeper_cash ||
	    stage->keeper_after.cash != entry.payload.expected_keeper_cash ||
	    stage->keeper_before.roaming != (entry.payload.keeper_roaming != 0) ||
	    stage->keeper_after.roaming != (entry.payload.keeper_roaming != 0) ||
	    stage->catalog_before.filename.empty() ||
	    stage->catalog_before.filename != stage->catalog_after.filename ||
	    stage->catalog_before.bytes.empty() || stage->catalog_after.bytes.empty())
		return false;
	try
	{
		player_snapshot ack;
		player_shop_checkpoint_stage status;
		economic_shop_checkpoint_projection mapping;
		player_flat_shop_checkpoint_cut cut;
		std::vector<uint8_t> current_ack, staged_ack, keeper;
		if (!player_save_shop_checkpoint_owner::observe_held_flat(
			    flat.player_token,
			    find_character_by_runtime_id(prepared.actor_runtime_id),
			    token.operation_id_, &ack, &status, &mapping, &cut) ||
		    !flat_same_mapping(mapping, stage->mapping) ||
		    cut.selected_root != stage->player_hold_cut.selected_root ||
		    cut.ownership_epoch != stage->player_hold_cut.ownership_epoch ||
		    cut.execution_hold_generation !=
			    stage->player_hold_cut.execution_hold_generation ||
		    player_snapshot_encode(ack, &current_ack) != player_snapshot_codec_result::ok ||
		    player_snapshot_encode(stage->original_queued_ack, &staged_ack) !=
			    player_snapshot_codec_result::ok ||
		    current_ack != staged_ack ||
		    player_item_snapshot_list_encode(stage->keeper_after.items, &keeper) !=
			    player_snapshot_codec_result::ok ||
		    keeper != prepared.keeper_blob)
			return false;
		size_t bytes = 0;
		if (!flat_native_retained_bytes(entry, *stage, &bytes) ||
		    !player_save_shop_checkpoint_owner::reserve_flat_native_checkpoint(
			    flat.player_token, token.operation_id_, bytes))
			return false;
		// Original aggregate reservation precedes a nonthrowing ownership transfer.
		flat.native = std::move(stage);
		flat.reserved_bytes = bytes;
		flat.phase = shop_trade_flat_native_checkpoint_phase::sealed;
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool shop_trade_preparation_owner::start_native_checkpoint_flat(
	const shop_trade_flat_preparation_token &token) noexcept
{
	if (!nevent_is_game_thread())
		return false;
	auto found = pending.find(token.operation_id_.bytes);
	if (found == pending.end() || !found->second.preparation ||
	    !found->second.preparation->flat ||
	    found->second.preparation->generation != token.generation_)
		return false;
	auto &flat = *found->second.preparation->flat;
	if (!flat.player_held || !flat.native || !flat.reserved_bytes ||
	    flat.phase != shop_trade_flat_native_checkpoint_phase::sealed ||
	    flat.native->operation_id.bytes != token.operation_id_.bytes ||
	    !player_save_shop_checkpoint_owner::begin_native_attempt_flat(flat.player_token,
									  token.operation_id_))
		return false;
	// The genuine exact leaf marker precedes first journal work; no fallible tail.
	flat.phase = shop_trade_flat_native_checkpoint_phase::attempt_started;
	return true;
}

bool shop_trade_preparation_owner::retain_native_outcome_flat(
	const shop_trade_flat_preparation_token &token,
	flatfile_authority_transaction_result result,
	flatfile_authority_commit_outcome outcome) noexcept
{
	if (!nevent_is_game_thread())
		return false;
	auto found = pending.find(token.operation_id_.bytes);
	if (found == pending.end() || !found->second.preparation ||
	    !found->second.preparation->flat ||
	    found->second.preparation->generation != token.generation_)
		return false;
	auto &flat = *found->second.preparation->flat;
	if (!flat.native || !flat.player_held ||
	    (flat.phase != shop_trade_flat_native_checkpoint_phase::attempt_started &&
	     flat.phase != shop_trade_flat_native_checkpoint_phase::uncertain))
		return false;
	switch (result)
	{
	case flatfile_authority_transaction_result::ok:
	case flatfile_authority_transaction_result::not_found:
	case flatfile_authority_transaction_result::invalid:
	case flatfile_authority_transaction_result::io_error:
		break;
	default:
		return false;
	}
	switch (outcome)
	{
	case flatfile_authority_commit_outcome::not_published:
	case flatfile_authority_commit_outcome::publication_uncertain:
	case flatfile_authority_commit_outcome::committed:
		break;
	default:
		return false;
	}
	if (flat.outcome_recorded)
		return flat.first_result == result && flat.first_outcome == outcome;
	flat.first_result = result;
	flat.first_outcome = outcome;
	flat.outcome_recorded = true;
	flat.phase = shop_trade_flat_native_checkpoint_phase::uncertain;
	return true;
}

bool shop_trade_preparation_owner::resolve_native_checkpoint_flat(
	const shop_trade_flat_preparation_token &token,
	const shop_trade_flat_native_checkpoint_resolution &proof) noexcept
{
	if (!nevent_is_game_thread())
		return false;
	auto found = pending.find(token.operation_id_.bytes);
	if (found == pending.end() || !found->second.preparation ||
	    !found->second.preparation->flat ||
	    found->second.preparation->generation != token.generation_)
		return false;
	auto &flat = *found->second.preparation->flat;
	if (!flat.native || !flat.player_held || !flat.reserved_bytes ||
	    proof.original_ != flat.native.get() ||
	    flat.native->operation_id.bytes != token.operation_id_.bytes ||
	    (flat.phase != shop_trade_flat_native_checkpoint_phase::attempt_started &&
	     flat.phase != shop_trade_flat_native_checkpoint_phase::uncertain))
		return false;
	switch (proof.proved_)
	{
	case shop_trade_flat_native_checkpoint_resolution::cut::after:
		if (flat.outcome_recorded &&
		    flat.first_outcome == flatfile_authority_commit_outcome::not_published)
			return false;
		flat.phase = shop_trade_flat_native_checkpoint_phase::ready;
		return true;
	case shop_trade_flat_native_checkpoint_resolution::cut::before:
		if (flat.outcome_recorded &&
		    flat.first_outcome == flatfile_authority_commit_outcome::committed)
			return false;
		flat.phase = shop_trade_flat_native_checkpoint_phase::unpublished_before;
		return true;
	}
	return false;
}

bool shop_trade_preparation_owner::completed_native_checkpoint_flat(
	const shop_trade_flat_preparation_token &token,
	const shop_trade_flat_native_checkpoint_stage **output) noexcept
{
	if (!nevent_is_game_thread() || !output)
		return false;
	auto found = pending.find(token.operation_id_.bytes);
	if (found == pending.end() || !found->second.preparation ||
	    !found->second.preparation->flat ||
	    found->second.preparation->generation != token.generation_)
		return false;
	const auto &flat = *found->second.preparation->flat;
	if (!flat.player_held || !flat.native || !flat.reserved_bytes ||
	    flat.phase != shop_trade_flat_native_checkpoint_phase::ready ||
	    flat.native->operation_id.bytes != token.operation_id_.bytes)
		return false;
	*output = flat.native.get();
	return true;
}

bool shop_trade_preparation_owner::build_accounted_command_flat(
	const shop_trade_flat_preparation_token &token, P_char actor, P_char keeper, P_obj selected,
	P_obj stock, P_obj destination, critical_command *output) noexcept
{
	const shop_trade_flat_native_checkpoint_stage *retained = nullptr;
	if (!output || !completed_native_checkpoint_flat(token, &retained))
		return false;
	auto found = pending.find(token.operation_id_.bytes);
	if (found == pending.end() || !found->second.preparation ||
	    found->second.preparation->generation != token.generation_)
		return false;
	auto &entry = found->second;
	auto &prepared = *entry.preparation;
	auto &flat = *prepared.flat;
	const auto &native = *retained;
	if (entry.production_owned || prepared.submission_started ||
	    !persistence_mode_flatfile_root() ||
	    flat.selected_root != persistence_mode_flatfile_root())
		return false;
	try
	{
		if (prepared.command)
		{
			size_t bytes = 0;
			critical_command copied = *prepared.command;
			if (!flat_command_retained_bytes(*prepared.command, &bytes) ||
			    !player_save_shop_checkpoint_owner::reserve_flat_command_checkpoint(
				    flat.player_token, token.operation_id_, native.player_hold_cut,
				    bytes))
				return false;
			*output = std::move(copied);
			return true;
		}
		if ((prepared.destination_shell_started && !prepared.destination_shell_returned) ||
		    !prepared.destination_weight_ready)
			return false;
		if (!flat_preparation_body(actor, keeper, entry.payload.shop_id) ||
		    actor->runtime_id != prepared.actor_runtime_id ||
		    keeper->runtime_id != prepared.keeper_runtime_id ||
		    static_cast<uint32_t>(GET_PID(actor)) != entry.player_pid || !selected ||
		    selected->obj_uid != entry.payload.selected_item_uid ||
		    (stock ? stock->obj_uid : 0) != entry.payload.stock_item_uid ||
		    (destination ? destination->obj_uid : 0) !=
			    entry.payload.target_parent_item_uid ||
		    GET_VNUM(keeper) != entry.payload.keeper_vnum ||
		    (shop_index[entry.payload.shop_id].shop_is_roaming != 0) !=
			    (entry.payload.keeper_roaming != 0) ||
		    GET_RACEWAR(actor) != entry.payload.racewar ||
		    ((entry.payload.action == shop_trade_action::sell_store ||
		      entry.payload.action == shop_trade_action::sell_destroy) &&
		     !OBJ_CARRIED_BY(selected, actor)) ||
		    (entry.payload.action == shop_trade_action::buy_existing &&
		     !OBJ_CARRIED_BY(selected, keeper)) ||
		    (stock && !OBJ_CARRIED_BY(stock, keeper)) ||
		    (destination && !OBJ_CARRIED_BY(destination, actor)))
			return false;
		const int64_t keeper_cash = static_cast<int64_t>(GET_COPPER(keeper)) +
					    10LL * GET_SILVER(keeper) + 100LL * GET_GOLD(keeper) +
					    1000LL * GET_PLATINUM(keeper);
		if (GET_COPPER(keeper) < 0 || GET_SILVER(keeper) < 0 || GET_GOLD(keeper) < 0 ||
		    GET_PLATINUM(keeper) < 0 || keeper_cash != entry.payload.expected_keeper_cash)
			return false;
		const char *account = get_account_name_safe(actor);
		economic_shop_checkpoint_projection mapping{};
		std::vector<uint8_t> a, b, c, d;
		std::vector<player_item_snapshot> keeper_items;
		if (!account || strcmp(account, entry.payload.account_name.data()) ||
		    !economic_gameplay_authority::observe_flat_shop_checkpoint(
			    entry.player_pid, account, entry.payload.racewar, &mapping) ||
		    !flat_same_mapping(mapping, prepared.mapping) ||
		    !flat_preparation_tree(selected, &a) || a != prepared.selected ||
		    !flat_preparation_tree(stock, &b) || b != prepared.stock ||
		    !flat_preparation_destination_tree(destination, &c) ||
		    c != prepared.destination ||
		    !shop_item_runtime_capture_keeper_literal(keeper, &keeper_items) ||
		    player_item_snapshot_list_encode(keeper_items, &d) !=
			    player_snapshot_codec_result::ok ||
		    d != prepared.keeper_blob)
			return false;

		player_snapshot ack;
		player_shop_checkpoint_stage held;
		economic_shop_checkpoint_projection held_mapping;
		player_flat_shop_checkpoint_cut cut;
		std::vector<uint8_t> ack_bytes, original_ack_bytes;
		if (!player_save_shop_checkpoint_owner::observe_held_flat(
			    flat.player_token, actor, token.operation_id_, &ack, &held,
			    &held_mapping, &cut) ||
		    !flat_same_mapping(held_mapping, native.mapping) ||
		    cut.selected_root != native.player_hold_cut.selected_root ||
		    cut.ownership_epoch != native.player_hold_cut.ownership_epoch ||
		    cut.execution_hold_generation !=
			    native.player_hold_cut.execution_hold_generation ||
		    held.save_revision != native.acknowledged_status.save_revision ||
		    held.level != native.acknowledged_status.level ||
		    static_cast<uint32_t>(GET_LEVEL(actor)) != held.level ||
		    player_snapshot_encode(ack, &ack_bytes) != player_snapshot_codec_result::ok ||
		    player_snapshot_encode(native.original_queued_ack, &original_ack_bytes) !=
			    player_snapshot_codec_result::ok ||
		    ack_bytes != original_ack_bytes)
			return false;
		const std::array<int64_t, 4> wallet{ GET_COPPER(actor), GET_SILVER(actor),
						     GET_GOLD(actor), GET_PLATINUM(actor) };
		const std::array<int64_t, 4> bank{ GET_BALANCE_COPPER(actor),
						   GET_BALANCE_SILVER(actor),
						   GET_BALANCE_GOLD(actor),
						   GET_BALANCE_PLATINUM(actor) };
		for (size_t i = 0; i < wallet.size(); ++i)
			if (wallet[i] < 0 || bank[i] < 0 ||
			    static_cast<uint64_t>(wallet[i]) !=
				    native.player.native_money.domains.wallet[i] ||
			    static_cast<uint64_t>(bank[i]) !=
				    native.player.native_money.domains.bank[i])
				return false;
		if (actor->only.pc->wallet_revision !=
			    native.player.native_money.domains.wallet_revision ||
		    actor->only.pc->bank_revision !=
			    native.player.native_money.domains.bank_revision)
			return false;
		shop_trade_payload payload{};
		if (!flat_accounted_payload(entry, native, &payload))
			return false;
		payload.native_destination_weight_recorded = true;
		payload.destination_weight = prepared.destination_weight;
		// Refuse an unpublishable complete player AFTER before journal admission
		// or SQL. Reuse the exact held original body and the final observer rules.
		const auto &original = native.player.player.items;
		std::vector<player_item_snapshot> prospective;
		if (!shop_trade_native_publication_owner::expected_player_forest(
			    payload, original, false, &prospective))
			return false;
		if (payload.action == shop_trade_action::buy_existing ||
		    payload.action == shop_trade_action::buy_produced)
		{
			std::vector<player_item_snapshot> ordered;
			if (!shop_trade_native_publication_owner::expected_player_order(
				    payload, actor, selected, destination, prospective, &ordered))
				return false;
			prospective = std::move(ordered);
		}
		if (!shop_trade_world_player_values_supported(prospective))
			return false;

		// Saved-policy inventory omits some physical NORENT bodies; its row
		// count alone cannot prove that the final bounded world census fits.
		std::vector<uint64_t> literal_roots;
		if (std::any_of(original.begin(), original.end(), [&](const auto &item)
				{ return item.object_uid == payload.selected_item_uid; }))
			literal_roots.push_back(payload.selected_item_uid);
		std::vector<player_item_snapshot> detached;
		if (payload.action == shop_trade_action::buy_produced &&
		    player_item_snapshot_list_decode(payload.item_blob.data(),
						     payload.item_blob_size,
						     &detached) != player_snapshot_codec_result::ok)
			return false;
		shop_trade_world_expectation before_world{};
		before_world.actor_pid = payload.player_pid;
		before_world.actor_runtime_id = prepared.actor_runtime_id;
		before_world.keeper_runtime_id = prepared.keeper_runtime_id;
		before_world.shop_id = payload.shop_id;
		before_world.keeper_vnum = payload.keeper_vnum;
		before_world.player_items = original;
		before_world.keeper_items = native.keeper_after.items;
		before_world.detached_items = detached;
		before_world.literal_player_root_uids = literal_roots;
		shop_trade_world_witness before_observed;
		if (!shop_trade_world_witness_observe(before_world, &before_observed) ||
		    ((payload.action == shop_trade_action::buy_existing ||
		      payload.action == shop_trade_action::buy_produced) &&
		     payload.item_count > PLAYER_SNAPSHOT_MAX_OBJECTS -
						  before_observed.player_physical_item_count))
			return false;

		// Journal the original ordered forest bindings before admission. These
		// values are recovery comparison inputs; SQL must authenticate their
		// complete original images under the existing authority/custody locks.
		std::vector<player_item_snapshot> keeper_after;
		if (!shop_trade_native_publication_owner::expected_keeper_forest(
			    payload, keeper, selected, native.keeper_after.items, false,
			    &keeper_after))
			return false;
		const auto freeze_forest = [](const std::vector<player_item_snapshot> &forest,
					      shop_trade_recovery_forest_role role,
					      shop_trade_recovery_forest_binding *binding)
		{
			std::vector<uint8_t> canonical;
			return player_item_snapshot_list_encode(forest, &canonical) ==
				       player_snapshot_codec_result::ok &&
			       shop_trade_recovery_forest_freeze(canonical, role, binding);
		};
		shop_trade_recovery_manifest manifest;
		if (!freeze_forest(original, shop_trade_recovery_forest_role::player_before,
				   &manifest.player_before) ||
		    !freeze_forest(prospective, shop_trade_recovery_forest_role::player_after,
				   &manifest.player_after) ||
		    !freeze_forest(native.keeper_after.items,
				   shop_trade_recovery_forest_role::keeper_before,
				   &manifest.keeper_before) ||
		    !freeze_forest(keeper_after, shop_trade_recovery_forest_role::keeper_after,
				   &manifest.keeper_after))
			return false;
		if (payload.target_parent_item_uid)
		{
			// Full live literal includes omitted non-durable NORENT children.
			// Cold recovery uses the separate saved-policy player binding and
			// must neither demand nor recreate those omitted children.
			std::vector<player_item_snapshot> target_after;
			if (!shop_trade_recovery_forest_freeze(
				    prepared.destination,
				    shop_trade_recovery_forest_role::live_target_before,
				    &manifest.live_target_before) ||
			    !shop_trade_native_publication_owner::expected_destination_forest(
				    payload, actor, selected, destination, prepared.destination,
				    &target_after) ||
			    !freeze_forest(target_after,
					   shop_trade_recovery_forest_role::live_target_after,
					   &manifest.live_target_after))
				return false;
		}
		payload.recovery_manifest = std::move(manifest);
		payload.recovery_manifest_recorded = true;

		auto command = std::make_unique<critical_command>();
		if (!shop_trade_command_build_recovery(command.get(), token.operation_id_, payload,
						       critical_source_site::command,
						       critical_deadline_class::interactive) ||
		    economic_gameplay_authority::prepare_shop_trade(command.get()) !=
			    economic_accounting_error::ok ||
		    command->schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION)
			return false;
		economic_frozen_intent intent;
		shop_trade_payload verified{};
		economic_account_key verified_wallet, verified_bank, counterparty;
		auto projection = *command;
		projection.accepted_at_usec = 1;
		if (shop_trade_accounting_decode(projection, &intent, &verified, &verified_wallet,
						 &verified_bank,
						 &counterparty) != economic_accounting_error::ok ||
		    intent.admission.metadata.epoch.bytes != prepared.mapping.epoch.bytes ||
		    !economic_account_key_equal(verified_wallet, prepared.mapping.wallet) ||
		    !economic_account_key_equal(verified_bank, prepared.mapping.bank))
			return false;
		const auto now = std::chrono::duration_cast<std::chrono::microseconds>(
					 std::chrono::system_clock::now().time_since_epoch())
					 .count();
		if (now <= 0)
			return false;
		command->accepted_at_usec = static_cast<uint64_t>(now);
		command->publication_required = true;
		if (!critical_command_envelope_valid(*command))
			return false;

		// Copy and account every retained command allocation before installation.
		critical_command copied = *command;
		size_t bytes = 0;
		if (!flat_command_retained_bytes(*command, &bytes) ||
		    !player_save_shop_checkpoint_owner::reserve_flat_command_checkpoint(
			    flat.player_token, token.operation_id_, native.player_hold_cut, bytes))
			return false;
		static_assert(std::is_nothrow_move_assignable_v<critical_command>);
		prepared.command = std::move(command);
		*output = std::move(copied);
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool shop_trade_transaction_keeper_busy(uint32_t shop)
{
	return std::any_of(pending.begin(), pending.end(), [shop](const auto &entry)
			   { return entry.second.payload.shop_id == shop; });
}

bool shop_trade_transaction_item_busy(uint64_t uid)
{
	if (!uid)
		return false;
	return std::any_of(
		pending.begin(), pending.end(),
		[uid](const auto &value)
		{
			const auto &entry = value.second;
			if (entry.cold)
				return std::binary_search(entry.cold->fenced_uids.begin(),
							  entry.cold->fenced_uids.end(), uid);
			if (entry.preparation)
				return std::binary_search(entry.preparation->fenced_uids.begin(),
							  entry.preparation->fenced_uids.end(),
							  uid);
			const auto &p = entry.payload;
			if (p.selected_item_uid == uid || p.stock_item_uid == uid ||
			    p.target_parent_item_uid == uid)
				return true;
			for (size_t i = 0; i < p.item_count; ++i)
				if (p.items[i].item_uid == uid)
					return true;
			return false;
		});
}

void shop_trade_transaction_reset_for_tests(void)
{
	pending.clear();
	notifying = 0;
	preparation_generation = 0;
}

critical_submit_result shop_trade_preparation_owner::submit_accounted_flat(
	const shop_trade_flat_preparation_token &token, P_char actor, P_char keeper, P_obj selected,
	P_obj stock, P_obj destination, shop_trade_accounted_publication_fn publication,
	shop_trade_completion_fn completion) noexcept
{
#ifndef __NO_MYSQL__
	(void)token;
	(void)actor;
	(void)keeper;
	(void)selected;
	(void)stock;
	(void)destination;
	(void)publication;
	(void)completion;
	return critical_submit_result::unavailable;
#else
	const shop_trade_flat_native_checkpoint_stage *native = nullptr;
	if (!nevent_is_game_thread() || !publication || !completion ||
	    !completed_native_checkpoint_flat(token, &native))
		return critical_submit_result::invalid;
	auto found = pending.find(token.operation_id_.bytes);
	if (found == pending.end() || !found->second.preparation ||
	    found->second.preparation->generation != token.generation_ || found->second.blocked ||
	    found->second.cold || found->second.local_refused || found->second.completion_ready ||
	    found->second.publishing)
		return critical_submit_result::identity_conflict;
	auto &entry = found->second;
	auto &prepared = *entry.preparation;
	try
	{
		critical_command command;
		if (prepared.submission_started)
		{
			if (!prepared.command || entry.accounted_publication != publication ||
			    entry.completion != completion)
				return critical_submit_result::identity_conflict;
			command = *prepared.command;
		}
		else if (!build_accounted_command_flat(token, actor, keeper, selected, stock,
						       destination, &command))
			return critical_submit_result::unavailable;
		economic_frozen_intent intent;
		shop_trade_payload payload{};
		economic_account_key wallet, bank, counterparty;
		if (shop_trade_accounting_decode(command, &intent, &payload, &wallet, &bank,
						 &counterparty) != economic_accounting_error::ok ||
		    command.operation_id.bytes != token.operation_id_.bytes ||
		    command.payload_version != SHOP_TRADE_RECOVERY_PAYLOAD_VERSION ||
		    payload.player_pid != entry.player_pid || !prepared.flat->player_held ||
		    native->player_token != prepared.flat->player_token)
			return critical_submit_result::invalid;
		// All fallible copies complete before the original submission marker.
		// Admission refusal cannot undo the already attempted native checkpoint.
		if (!prepared.submission_started)
		{
			flat_preparation_bytes bytes;
			if (!bytes.manifest(payload.recovery_manifest) ||
			    !player_save_shop_checkpoint_owner::reserve_flat_payload_checkpoint(
				    prepared.flat->player_token, token.operation_id_,
				    native->player_hold_cut, bytes.value))
				return critical_submit_result::unavailable;
			static_assert(std::is_nothrow_move_assignable_v<shop_trade_payload>);
			entry.payload = std::move(payload);
			entry.accounted_publication = publication;
			entry.completion = completion;
			prepared.submission_started = true;
		}
		// Exact retries preserve the first retained decoded payload and capacity;
		// they never install another manifest allocation under the old charge.
		return player_save_shop_checkpoint_owner::submit_owned_flat(
			prepared.flat->player_token, native->player_hold_cut, std::move(command));
	}
	catch (...)
	{
		return prepared.submission_started ? critical_submit_result::journal_uncertain :
						     critical_submit_result::invalid;
	}
#endif
}

namespace
{
bool flat_publication_actor(P_char actor, P_char keeper, const pending_trade &entry)
{
	if (!entry.preparation || !entry.preparation->flat ||
	    !flat_preparation_body(actor, keeper, entry.payload.shop_id) ||
	    static_cast<uint32_t>(GET_PID(actor)) != entry.player_pid ||
	    actor->runtime_id != entry.preparation->actor_runtime_id ||
	    keeper->runtime_id != entry.preparation->keeper_runtime_id ||
	    GET_VNUM(keeper) != entry.payload.keeper_vnum ||
	    static_cast<uint32_t>(GET_LEVEL(actor)) != entry.payload.expected_player_level ||
	    GET_RACEWAR(actor) != entry.payload.racewar ||
	    (shop_index[entry.payload.shop_id].shop_is_roaming != 0) !=
		    (entry.payload.keeper_roaming != 0))
		return false;
	const char *account = get_account_name_safe(actor);
	return account && !strcmp(account, entry.payload.account_name.data());
}

// Original world observer and UID location convention, without SQL row IDs.
bool flat_publication_world(pending_trade &entry, const std::vector<player_item_snapshot> &player,
			    const std::vector<player_item_snapshot> &keeper,
			    const std::vector<player_item_snapshot> &detached, bool final,
			    shop_trade_world_witness *output)
{
	std::vector<shop_trade_world_uid_expectation> locations;
	std::set<uint64_t> included;
	const auto append = [&](const auto &forest, bool is_keeper, bool is_detached)
	{
		for (size_t index = 0; index < forest.size(); ++index)
		{
			const auto &item = forest[index];
			if (!included.insert(item.object_uid).second || item.parent_index < -1 ||
			    item.parent_index >= static_cast<int32_t>(index))
				return false;
			const uint64_t parent =
				item.parent_index < 0 ? 0 : forest[item.parent_index].object_uid;
			const auto location =
				parent	    ? shop_trade_world_location::inside :
				is_detached ? shop_trade_world_location::detached :
				is_keeper   ? (item.equipment_slot ?
						       shop_trade_world_location::keeper_equipment :
						       shop_trade_world_location::keeper_inventory) :
					      (item.equipment_slot ?
						       shop_trade_world_location::actor_equipment :
						       shop_trade_world_location::actor_inventory);
			locations.push_back(
				{ item.object_uid, location, parent, item.equipment_slot, -1 });
		}
		return true;
	};
	if (!append(player, false, false) || !append(keeper, true, false) ||
	    !append(detached, false, true))
		return false;
	if (final)
		for (size_t index = 0; index < entry.payload.item_count; ++index)
			if (!included.count(entry.payload.items[index].item_uid))
				locations.push_back({ entry.payload.items[index].item_uid,
						      shop_trade_world_location::absent, 0, 0,
						      -1 });
	std::vector<uint64_t> literal;
	if (std::any_of(player.begin(), player.end(), [&](const auto &item)
			{ return item.object_uid == entry.payload.selected_item_uid; }))
		literal.push_back(entry.payload.selected_item_uid);
	shop_trade_world_expectation expected{};
	expected.actor_pid = entry.player_pid;
	expected.actor_runtime_id = entry.preparation->actor_runtime_id;
	expected.keeper_runtime_id = entry.preparation->keeper_runtime_id;
	expected.shop_id = entry.payload.shop_id;
	expected.keeper_vnum = entry.payload.keeper_vnum;
	expected.player_items = player;
	expected.keeper_items = keeper;
	expected.detached_items = detached;
	expected.literal_player_root_uids = literal;
	expected.uid_locations = locations;
	return shop_trade_world_witness_observe(expected, output) &&
	       flat_publication_actor(output->actor, output->keeper, entry);
}
P_obj flat_publication_object(const shop_trade_world_witness &witness, uint64_t uid)
{
	if (!uid)
		return nullptr;
	const auto found = std::find_if(witness.objects.begin(), witness.objects.end(),
					[uid](P_obj object)
					{ return object && object->obj_uid == uid; });
	return found == witness.objects.end() ? nullptr : *found;
}
bool flat_publication_items(const std::vector<player_item_snapshot> &a,
			    const std::vector<player_item_snapshot> &b)
{
	std::vector<uint8_t> x, y;
	return player_item_snapshot_list_encode(a, &x) == player_snapshot_codec_result::ok &&
	       player_item_snapshot_list_encode(b, &y) == player_snapshot_codec_result::ok &&
	       x == y;
}
// Pure intermediate placement values, not an alternate command or manifest.
// Decode/transform ONLY the immutable original command's selected AFTER tree.
// Its carried-root form leaves the original container's BEFORE weight unchanged.
bool flat_publication_carried_values(const shop_trade_payload &original,
				     const std::vector<player_item_snapshot> &before,
				     std::vector<player_item_snapshot> *output)
{
	if (!output || !original.target_parent_item_uid ||
	    (original.action != shop_trade_action::buy_existing &&
	     original.action != shop_trade_action::buy_produced) ||
	    before.size() > PLAYER_SNAPSHOT_MAX_OBJECTS)
		return false;
	std::vector<player_item_snapshot> selected;
	if (!shop_trade_accounted_after_items(original, &selected) || selected.empty() ||
	    selected.size() > PLAYER_SNAPSHOT_MAX_OBJECTS - before.size())
		return false;
	std::set<uint64_t> seen;
	for (size_t index = 0; index < before.size(); ++index)
		if (!seen.insert(before[index].object_uid).second ||
		    before[index].parent_index < PLAYER_SNAPSHOT_NO_PARENT ||
		    before[index].parent_index >= static_cast<int32_t>(index))
			return false;
	auto candidate = before;
	candidate.reserve(before.size() + selected.size());
	for (size_t index = 0; index < selected.size(); ++index)
	{
		const auto &item = selected[index];
		if (!seen.insert(item.object_uid).second || item.equipment_slot ||
		    (index ? (item.parent_index < 0 ||
			      static_cast<size_t>(item.parent_index) >= index) :
			     item.parent_index != PLAYER_SNAPSHOT_NO_PARENT))
			return false;
		candidate.push_back(item);
		if (item.parent_index >= 0)
			candidate.back().parent_index += static_cast<int32_t>(before.size());
	}
	if (!shop_trade_world_player_values_supported(candidate))
		return false;
	*output = std::move(candidate);
	return true;
}

bool flat_publication_rows(const std::vector<flatfile_item_ownership_record> &a,
			   const std::vector<flatfile_item_ownership_record> &b)
{
	if (a.size() != b.size())
		return false;
	for (size_t index = 0; index < a.size(); ++index)
	{
		const auto &x = a[index], &y = b[index];
		if (x.item_uid != y.item_uid || x.root_item_uid != y.root_item_uid ||
		    x.parent_item_uid != y.parent_item_uid ||
		    !item_owner_identity_equal(x.owner, y.owner) ||
		    x.item_revision != y.item_revision || x.vnum != y.vnum || x.state != y.state ||
		    x.coin_payload != y.coin_payload || x.equipment_slot != y.equipment_slot)
			return false;
	}
	return true;
}
bool flat_publication_same_current(const flatfile_accounting_shop_projection &a,
				   const flatfile_accounting_shop_projection &b)
{
	const auto &x = a.player_domains, &y = b.player_domains;
	if (x.pid != y.pid || x.account_name != y.account_name || x.racewar != y.racewar ||
	    x.domains.wallet != y.domains.wallet || x.domains.bank != y.domains.bank ||
	    x.domains.wallet_revision != y.domains.wallet_revision ||
	    x.domains.bank_revision != y.domains.bank_revision ||
	    a.player_owner_revision != b.player_owner_revision ||
	    a.keeper_owner_revision != b.keeper_owner_revision ||
	    a.primary_owner_revision != b.primary_owner_revision ||
	    a.counterparty_owner_revision != b.counterparty_owner_revision ||
	    a.pet_owner_revisions.size() != b.pet_owner_revisions.size() ||
	    !flat_publication_rows(a.player_custody, b.player_custody) ||
	    !flat_publication_rows(a.keeper_custody, b.keeper_custody) ||
	    !flat_publication_rows(a.pet_custody, b.pet_custody) ||
	    !flat_publication_rows(a.selected_custody, b.selected_custody))
		return false;
	for (size_t index = 0; index < a.pet_owner_revisions.size(); ++index)
		if (!item_owner_identity_equal(a.pet_owner_revisions[index].owner,
					       b.pet_owner_revisions[index].owner) ||
		    a.pet_owner_revisions[index].owner_revision !=
			    b.pet_owner_revisions[index].owner_revision)
			return false;
	std::vector<uint8_t> first, second;
	if (player_snapshot_encode(a.player, &first) != player_snapshot_codec_result::ok ||
	    player_snapshot_encode(b.player, &second) != player_snapshot_codec_result::ok ||
	    first != second)
		return false;
	const auto &k = a.keeper, &l = b.keeper;
	if (k.shop_id != l.shop_id || k.mob_vnum != l.mob_vnum || k.room_vnum != l.room_vnum ||
	    k.saved_at != l.saved_at || k.revision != l.revision || k.cash != l.cash ||
	    k.roaming != l.roaming || k.affects.size() != l.affects.size() ||
	    !flat_publication_items(k.items, l.items))
		return false;
	for (size_t index = 0; index < k.affects.size(); ++index)
	{
		const auto &f = k.affects[index], &g = l.affects[index];
		if (f.type != g.type || f.duration != g.duration || f.modifier != g.modifier ||
		    f.location != g.location || f.bitvectors != g.bitvectors)
			return false;
	}
	return true;
}
bool flat_publication_money(P_char actor, const flatfile_player_domain_record &source)
{
	if (!actor || !IS_PC(actor) || !actor->only.pc ||
	    actor->only.pc->wallet_revision != source.domains.wallet_revision ||
	    actor->only.pc->bank_revision != source.domains.bank_revision)
		return false;
	const std::array<int64_t, 4> wallet{ GET_COPPER(actor), GET_SILVER(actor), GET_GOLD(actor),
					     GET_PLATINUM(actor) };
	const std::array<int64_t, 4> bank{ GET_BALANCE_COPPER(actor), GET_BALANCE_SILVER(actor),
					   GET_BALANCE_GOLD(actor), GET_BALANCE_PLATINUM(actor) };
	for (size_t index = 0; index < wallet.size(); ++index)
		if (wallet[index] < 0 || bank[index] < 0 ||
		    static_cast<uint64_t>(wallet[index]) != source.domains.wallet[index] ||
		    static_cast<uint64_t>(bank[index]) != source.domains.bank[index])
			return false;
	return true;
}
} // namespace

bool shop_trade_native_publication_owner::cold_flat_entry(void *opaque) noexcept
{
	return opaque && static_cast<pending_trade *>(opaque)->cold &&
	       static_cast<pending_trade *>(opaque)->cold->flat_restored;
}

bool shop_trade_native_publication_owner::restore_flat(const critical_command &original) noexcept
{
#if !defined(__NO_MYSQL__) || !defined(__GLIBCXX__)
	(void)original;
	return false;
#else
	try
	{
		if (!nevent_is_game_thread() ||
		    persistence_mode_get() != PERSISTENCE_MODE_FLATFILE_PRIMARY ||
		    !original.publication_required ||
		    original.type != critical_command_type::shop_trade ||
		    original.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION ||
		    original.payload_version != SHOP_TRADE_RECOVERY_MANIFEST_VERSION ||
		    !critical_command_envelope_valid(original))
			return false;
		economic_frozen_intent intent;
		shop_trade_payload payload{};
		economic_account_key wallet{}, bank{}, counterparty{};
		if (shop_trade_accounting_decode(original, &intent, &payload, &wallet, &bank,
						 &counterparty) != economic_accounting_error::ok ||
		    !payload.recovery_manifest_recorded || !payload.player_pid ||
		    payload.player_pid > INT_MAX)
			return false;
		auto found = pending.find(original.operation_id.bytes);
		if (found != pending.end())
		{
			// Observer runs inside coordinator_mutex. This exact passive retry
			// never calls coordinator APIs or replaces any original/native state.
			if (!found->second.cold || !found->second.cold->flat_restored ||
			    !found->second.cold->command ||
			    !found->second.cold->flat_registration_bytes ||
			    !critical_command_equal(*found->second.cold->command, original) ||
			    !player_save_restored_publication_owner::restore_shop_flat_obligation(
				    original, found->second.cold->flat_registration_bytes))
				return false;
			found->second.cold->registration_pending = false;
			return true;
		}
		if (pending.size() + notifying >= SHOP_TRADE_PENDING_MAX ||
		    player_pending(payload.player_pid) ||
		    shop_trade_transaction_keeper_busy(payload.shop_id))
			return false;
		pending_trade entry;
		entry.player_pid = payload.player_pid;
		entry.payload = payload;
		entry.cold = std::make_unique<cold_trade_restore>();
		entry.cold->command = std::make_unique<critical_command>(original);
		entry.cold->flat_restored = true;
		const auto &manifest = entry.payload.recovery_manifest;
		for (const auto *binding :
		     { &manifest.player_before, &manifest.player_after, &manifest.keeper_before,
		       &manifest.keeper_after, &manifest.live_target_before,
		       &manifest.live_target_after })
			entry.cold->fenced_uids.insert(entry.cold->fenced_uids.end(),
						       binding->ordered_item_uids.begin(),
						       binding->ordered_item_uids.end());
		for (size_t index = 0; index < payload.item_count; ++index)
			entry.cold->fenced_uids.push_back(payload.items[index].item_uid);
		auto &uids = entry.cold->fenced_uids;
		std::sort(uids.begin(), uids.end());
		uids.erase(std::unique(uids.begin(), uids.end()), uids.end());
		if (uids.size() > 3 * PLAYER_SNAPSHOT_MAX_OBJECTS)
			return false;
		flat_preparation_bytes bytes;
		// The selected server ABI uses the default libstdc++ map allocator:
		// sizeof its actual node includes links/alignment/value exactly, not a
		// guessed overhead or diagnostic allocator header. Other ABIs refuse above.
		using map_type = decltype(pending);
		static_assert(std::is_same_v<map_type::allocator_type,
					     std::allocator<map_type::value_type>>);
		if (!bytes.add(sizeof(std::_Rb_tree_node<map_type::value_type>)) ||
		    !bytes.add(sizeof(cold_trade_restore)) ||
		    !bytes.add(sizeof(critical_command)) ||
		    !bytes.vector(entry.cold->command->keys) ||
		    !bytes.vector(entry.cold->command->expected_revisions) ||
		    !bytes.vector(entry.cold->command->payload) ||
		    !bytes.vector(entry.cold->command->accounting_intent) || !bytes.vector(uids))
			return false;
		for (const auto *binding :
		     { &manifest.player_before, &manifest.player_after, &manifest.keeper_before,
		       &manifest.keeper_after, &manifest.live_target_before,
		       &manifest.live_target_after })
			if (!bytes.vector(binding->ordered_item_uids))
				return false;
		entry.cold->flat_registration_bytes = bytes.value;
		// Allocate the actual same-allocator node off-map. A failed hold/budget
		// leaves no uncharged passive entry; successful admission is followed only
		// by allocation-free node transfer on this original game-thread observer.
		map_type staged;
		staged.emplace(original.operation_id.bytes, std::move(entry));
		auto node = staged.extract(staged.begin());
		if (!player_save_restored_publication_owner::restore_shop_flat_obligation(
			    original, node.mapped().cold->flat_registration_bytes))
			return false;
		node.mapped().cold->registration_pending = false;
		const auto inserted = pending.insert(std::move(node));
		if (!inserted.inserted)
			return false;
		return true;
	}
	catch (...)
	{
		return false;
	}
#endif
}

bool shop_trade_native_publication_owner::cold_flat_original_forests(
	const critical_command &command, const flatfile_accounting_shop_projection &current,
	std::vector<player_item_snapshot> *player,
	std::vector<player_item_snapshot> *keeper) noexcept
{
	if (!player || !keeper || player == keeper ||
	    command.payload_version != SHOP_TRADE_RECOVERY_MANIFEST_VERSION)
		return false;
	try
	{
		shop_trade_payload payload{};
		std::vector<player_item_snapshot> source;
		if (!shop_trade_command_decode_payload(command, &payload) ||
		    !payload.recovery_manifest_recorded ||
		    current.player.pid != static_cast<int64_t>(payload.player_pid) ||
		    current.player_domains.pid != static_cast<int64_t>(payload.player_pid) ||
		    current.player_domains.account_name != payload.account_name.data() ||
		    current.player_domains.racewar != payload.racewar ||
		    current.keeper.shop_id != payload.shop_id ||
		    current.keeper.mob_vnum != payload.keeper_vnum ||
		    current.keeper.roaming != payload.keeper_roaming ||
		    player_item_snapshot_list_decode(payload.item_blob.data(),
						     payload.item_blob_size,
						     &source) != player_snapshot_codec_result::ok ||
		    source.empty() || source.size() != payload.item_count ||
		    source.front().object_uid != payload.selected_item_uid)
			return false;
		struct value
		{
			player_item_snapshot item;
			uint64_t parent = 0;
		};
		std::map<uint64_t, value> values;
		for (const auto *forest : { &current.player.items, &current.keeper.items })
			for (size_t index = 0; index < forest->size(); ++index)
			{
				const auto &item = (*forest)[index];
				if (!item.object_uid ||
				    item.parent_index < PLAYER_SNAPSHOT_NO_PARENT ||
				    item.parent_index >= static_cast<int64_t>(index))
					return false;
				const uint64_t parent_uid =
					item.parent_index < 0 ?
						0 :
						(*forest)[static_cast<size_t>(item.parent_index)]
							.object_uid;
				if (!values.emplace(item.object_uid, value{ item, parent_uid })
					     .second)
					return false;
			}
		// The original selected literal restores only that frozen tree. No UID,
		// parent, prototype default or CURRENT value is invented for a missing
		// non-selected item. Every resulting complete forest is verified below.
		for (size_t index = 0; index < source.size(); ++index)
		{
			const auto &item = source[index];
			if (!item.object_uid || item.parent_index < PLAYER_SNAPSHOT_NO_PARENT ||
			    item.parent_index >= static_cast<int64_t>(index) ||
			    (index ? item.parent_index < 0 :
				     item.parent_index != PLAYER_SNAPSHOT_NO_PARENT))
				return false;
			const uint64_t parent_uid =
				item.parent_index < 0 ?
					0 :
					source[static_cast<size_t>(item.parent_index)].object_uid;
			values[item.object_uid] = { item, parent_uid };
		}
		const bool buying = payload.action == shop_trade_action::buy_existing ||
				    payload.action == shop_trade_action::buy_produced;
		if (buying && payload.target_parent_item_uid &&
		    payload.native_destination_weight_recorded)
		{
			const auto target = values.find(payload.target_parent_item_uid);
			if (target == values.end())
				return false;
			target->second.item.weight = payload.destination_weight.before;
		}
		const auto reconstruct = [&](const shop_trade_recovery_forest_binding &binding,
					     shop_trade_recovery_forest_role role,
					     std::vector<player_item_snapshot> &result)
		{
			if (binding.ordered_item_uids.size() > static_cast<size_t>(INT32_MAX))
				return false;
			std::vector<player_item_snapshot> candidate;
			candidate.reserve(binding.ordered_item_uids.size());
			std::map<uint64_t, int32_t> positions;
			for (uint64_t uid : binding.ordered_item_uids)
			{
				const auto found = values.find(uid);
				if (found == values.end())
					return false;
				auto item = found->second.item;
				item.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
				if (found->second.parent)
				{
					const auto parent_position =
						positions.find(found->second.parent);
					if (parent_position == positions.end())
						return false;
					item.parent_index = parent_position->second;
				}
				if (!positions.emplace(uid, static_cast<int32_t>(candidate.size()))
					     .second)
					return false;
				candidate.push_back(std::move(item));
			}
			std::vector<uint8_t> encoded;
			if (player_item_snapshot_list_encode(candidate, &encoded) !=
				    player_snapshot_codec_result::ok ||
			    !shop_trade_recovery_forest_verify(encoded, role, binding))
				return false;
			result = std::move(candidate);
			return true;
		};
		std::vector<player_item_snapshot> original_player, original_keeper;
		if (!reconstruct(payload.recovery_manifest.player_before,
				 shop_trade_recovery_forest_role::player_before, original_player) ||
		    !reconstruct(payload.recovery_manifest.keeper_before,
				 shop_trade_recovery_forest_role::keeper_before, original_keeper))
			return false;
		// Both moves are nonthrowing; a refusal above preserves both outputs.
		*player = std::move(original_player);
		*keeper = std::move(original_keeper);
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool shop_trade_native_publication_owner::cold_flat_world(
	void *opaque, std::span<const player_item_snapshot> player,
	std::span<const player_item_snapshot> keeper, bool strict, bool actor_absent,
	bool keeper_absent, shop_trade_world_cold_observation *output,
	std::vector<shop_trade_world_uid_expectation> *output_locations) noexcept
{
	if (!opaque || !output || !output_locations || !nevent_is_game_thread() ||
	    player.size() > PLAYER_SNAPSHOT_MAX_OBJECTS ||
	    keeper.size() > PLAYER_SNAPSHOT_MAX_OBJECTS)
		return false;
	auto &entry = *static_cast<pending_trade *>(opaque);
	if (!entry.cold || !entry.cold->command || entry.cold->registration_pending ||
	    entry.cold->command->payload_version != SHOP_TRADE_RECOVERY_MANIFEST_VERSION ||
	    entry.cold->fenced_uids.size() > 3 * PLAYER_SNAPSHOT_MAX_OBJECTS)
		return false;
	try
	{
		shop_trade_payload original{};
		std::vector<uint8_t> original_bytes, retained_bytes;
		if (!shop_trade_command_decode_payload(*entry.cold->command, &original) ||
		    original.player_pid != entry.player_pid ||
		    !shop_trade_command_encode_recovery_payload(original, &original_bytes) ||
		    !shop_trade_command_encode_recovery_payload(entry.payload, &retained_bytes) ||
		    original_bytes != retained_bytes)
			return false;
		std::set<uint64_t> fenced;
		const auto &manifest = original.recovery_manifest;
		for (const auto *binding :
		     { &manifest.player_before, &manifest.player_after, &manifest.keeper_before,
		       &manifest.keeper_after, &manifest.live_target_before,
		       &manifest.live_target_after })
			for (uint64_t uid : binding->ordered_item_uids)
			{
				if (!uid || uid == UINT64_MAX)
					return false;
				fenced.insert(uid);
				if (fenced.size() > 3 * PLAYER_SNAPSHOT_MAX_OBJECTS)
					return false;
			}
		for (size_t index = 0; index < original.item_count; ++index)
		{
			fenced.insert(original.items[index].item_uid);
			if (fenced.size() > 3 * PLAYER_SNAPSHOT_MAX_OBJECTS)
				return false;
		}
		// Authenticate the complete original registered union; a retained subset
		// cannot suppress a required selected/target/global absence observation.
		if (fenced.size() != entry.cold->fenced_uids.size() ||
		    !std::equal(fenced.begin(), fenced.end(), entry.cold->fenced_uids.begin()))
			return false;
		std::map<uint64_t, shop_trade_world_uid_expectation> ordered;
		const auto append = [&](std::span<const player_item_snapshot> forest, bool npc)
		{
			for (size_t index = 0; index < forest.size(); ++index)
			{
				const auto &item = forest[index];
				if (!item.object_uid || item.object_uid == UINT64_MAX ||
				    item.parent_index < PLAYER_SNAPSHOT_NO_PARENT ||
				    item.parent_index >= static_cast<int64_t>(index))
					return false;
				const uint64_t parent =
					item.parent_index < 0 ?
						0 :
						forest[static_cast<size_t>(item.parent_index)]
							.object_uid;
				const bool absent = strict && (npc ? keeper_absent : actor_absent);
				const auto place =
					absent ? shop_trade_world_location::absent :
					parent ? shop_trade_world_location::inside :
					npc    ? (item.equipment_slot ?
							  shop_trade_world_location::keeper_equipment :
							  shop_trade_world_location::keeper_inventory) :
						 (item.equipment_slot ?
							  shop_trade_world_location::actor_equipment :
							  shop_trade_world_location::actor_inventory);
				if (!ordered.emplace(item.object_uid,
						     shop_trade_world_uid_expectation{
							     item.object_uid, place,
							     absent ? 0 : parent,
							     static_cast<int16_t>(
								     absent ? 0 :
									      item.equipment_slot),
							     -1 })
					     .second ||
				    ordered.size() > 3 * PLAYER_SNAPSHOT_MAX_OBJECTS)
					return false;
			}
			return true;
		};
		if (!append(player, false) || !append(keeper, true))
			return false;
		for (uint64_t uid : fenced)
		{
			if (!ordered.count(uid))
				ordered.emplace(
					uid, shop_trade_world_uid_expectation{
						     uid,
						     strict ? shop_trade_world_location::absent :
							      shop_trade_world_location::detached,
						     0, 0, -1 });
			if (ordered.size() > 3 * PLAYER_SNAPSHOT_MAX_OBJECTS)
				return false;
		}
		std::vector<shop_trade_world_uid_expectation> locations;
		locations.reserve(ordered.size());
		for (const auto &[uid, location] : ordered)
			locations.push_back(location);
		// An absent body's complete authenticated forest remains retained in
		// the caller. Its REQUEST span alone is empty: the original observer
		// rejects absent requirements appearing as expected present objects.
		const shop_trade_world_cold_request request{
			&original,
			strict && actor_absent ? std::span<const player_item_snapshot>{} : player,
			strict && keeper_absent ? std::span<const player_item_snapshot>{} : keeper,
			locations
		};
		shop_trade_world_cold_observation observed;
		if (!shop_trade_world_cold_observe(request, &observed))
			return false;
		if (strict && (observed.actor_present == actor_absent ||
			       observed.keeper_present == keeper_absent ||
			       !observed.uid_locations_current_match ||
			       (observed.actor_present && !observed.player_current_match) ||
			       (observed.keeper_present && !observed.keeper_current_match)))
			return false;
		// Relaxed observation deliberately preserves false match flags. The
		// original detached/absent selected tree is not a returned-handler or
		// native publication proof. All pointers are fresh and transient.
		*output = std::move(observed);
		*output_locations = std::move(locations);
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool shop_trade_native_publication_owner::live_flat_entry(void *opaque) noexcept
{
	return opaque && static_cast<pending_trade *>(opaque)->preparation &&
	       static_cast<pending_trade *>(opaque)->preparation->flat;
}

bool shop_trade_native_publication_owner::cold_prepare_selected_flat(void *opaque) noexcept
{
	if (!opaque || !nevent_is_game_thread() ||
	    persistence_mode_get() != PERSISTENCE_MODE_FLATFILE_PRIMARY ||
	    !flatfile_coin_boot_templates::ready())
		return false;
	auto &entry = *static_cast<pending_trade *>(opaque);
	if (!entry.cold || !entry.cold->command || entry.preparation || entry.blocked ||
	    entry.cold->registration_pending || !entry.cold->initialized ||
	    entry.cold->enrollment_started || entry.cold->enrolled || entry.cold->flat_literals ||
	    entry.cold->source.empty() || entry.cold->source.size() != entry.payload.item_count ||
	    entry.cold->source.size() > PLAYER_SNAPSHOT_MAX_OBJECTS)
		return false;
	try
	{
		const auto &source = entry.cold->source;
		auto prepared = std::make_unique<cold_flat_literal_stage>();
		prepared->staged.resize(source.size());
		prepared->reload.resize(source.size());
		prepared->enrollment_counts.reserve(source.size());
		std::vector<P_obj> objects;
		objects.reserve(source.size());
		std::set<uint64_t> uids;
		for (size_t index = 0; index < source.size(); ++index)
		{
			const auto &item = source[index];
			// One genuine complete selected subtree, in original parent-first order.
			// No default literal, new UID, native row identity or VNUM-only matching.
			// Command item entries are UID-sorted; persisted literals retain their
			// original DFS order. Bind by genuine UID, never by matching indices.
			const auto reference = std::lower_bound(
				entry.payload.items.begin(),
				entry.payload.items.begin() + entry.payload.item_count,
				item.object_uid, [](const auto &entry, uint64_t uid)
				{ return entry.item_uid < uid; });
			if (!item.object_uid || !uids.insert(item.object_uid).second ||
			    (index == 0 && item.object_uid != entry.payload.selected_item_uid) ||
			    reference == entry.payload.items.begin() + entry.payload.item_count ||
			    reference->item_uid != item.object_uid ||
			    reference->vnum != item.vnum ||
			    item.parent_index < PLAYER_SNAPSHOT_NO_PARENT ||
			    item.parent_index >= static_cast<int32_t>(index) ||
			    (index ? item.parent_index < 0 : item.parent_index != -1) ||
			    item.equipment_slot)
				return false;
			if (index &&
			    reference->parent_item_uid !=
				    source[static_cast<size_t>(item.parent_index)].object_uid)
				return false;
			const auto *prototype = flatfile_coin_boot_templates::find(item.vnum);
			if (!prototype ||
			    !shop_trade_original_item_stage::prepare(*prototype, item,
								     prepared->staged[index]) ||
			    !prepared->staged[index].object_)
				return false;
			prepared->reload[index].prototype = prototype;
			prepared->reload[index].proclib_probes.resize(
				item.extra_descriptions.size());
			objects.push_back(prepared->staged[index].object_);
			prepared->enrollment_counts.emplace_back(prototype->R_num, 1);
		}
		// A vector supplies an exact-capacity census without hidden tree-node sizes.
		auto &counts = prepared->enrollment_counts;
		std::sort(counts.begin(), counts.end(),
			  [](const auto &a, const auto &b) { return a.first < b.first; });
		size_t compacted = 0;
		for (const auto &count : counts)
		{
			if (compacted && counts[compacted - 1].first == count.first)
				++counts[compacted - 1].second;
			else
				counts[compacted++] = count;
		}
		counts.resize(compacted);
		for (const auto &[number, count] : counts)
			if (!obj_index || number < 0 || number > top_of_objt ||
			    obj_index[number].number < 0 ||
			    count > static_cast<size_t>(INT_MAX - obj_index[number].number))
				return false;
		if (!shop_trade_original_procedure_binding_stage::prepare_flat(
			    objects, source, prepared->bindings) ||
		    !prepared->bindings.valid_flat())
			return false;
		// Reverse parent-first order reproduces each original sibling order.
		// Stages remain private and independently own nodes until consumption.
		for (size_t index = source.size(); index-- > 1;)
		{
			P_obj child = prepared->staged[index].object_;
			P_obj parent =
				prepared->staged[static_cast<size_t>(source[index].parent_index)]
					.object_;
			child->loc_p = LOC_INSIDE;
			child->loc.inside = parent;
			child->next_content = parent->contains;
			parent->contains = child;
		}
		// No world/count/procedure commit, handler, reload or ACK occurs here.
		// The complete original held-cut charge must precede later enrollment.
		entry.cold->flat_literals = std::move(prepared);
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool shop_trade_native_publication_owner::cold_prepare_working_flat(
	void *opaque, const critical_command *retained) noexcept
{
	if (!opaque || !nevent_is_game_thread() ||
	    persistence_mode_get() != PERSISTENCE_MODE_FLATFILE_PRIMARY)
		return false;
	auto &entry = *static_cast<pending_trade *>(opaque);
	if (!entry.cold || !entry.cold->command || entry.preparation || entry.blocked ||
	    entry.cold->registration_pending || !entry.cold->initialized ||
	    !entry.native_receipt_verified || !entry.publication_forests_ready ||
	    (entry.completed.outcome != critical_apply_outcome::applied &&
	     entry.completed.outcome != critical_apply_outcome::already_applied))
		return false;
	auto &cold = *entry.cold;
	try
	{
		shop_trade_payload payload{};
		std::vector<uint8_t> original_bytes, retained_bytes;
		if (!shop_trade_command_decode_payload(*cold.command, &payload) ||
		    cold.command->payload_version != SHOP_TRADE_RECOVERY_MANIFEST_VERSION ||
		    !shop_trade_command_encode_recovery_payload(payload, &original_bytes) ||
		    !shop_trade_command_encode_recovery_payload(entry.payload, &retained_bytes) ||
		    original_bytes != retained_bytes || payload.player_pid != entry.player_pid)
			return false;
		// Retry never grows/replaces the charged holder or recaptures its path.
		if (retained && !critical_command_equal(*retained, *cold.command))
			return false;
		const auto *bound_command = retained ? retained : cold.command.get();
		if (cold.flat_working)
			return cold.flat_working->command == bound_command;
		if (cold.enrollment_started || cold.enrolled || cold.effect.started ||
		    cold.balances_started ||
		    (cold.flat_literals &&
		     (cold.flat_literals->binding_started || cold.flat_literals->binding_returned)))
			return false;
		const auto verifies = [](const std::vector<player_item_snapshot> &forest,
					 shop_trade_recovery_forest_role role,
					 const shop_trade_recovery_forest_binding &binding)
		{
			std::vector<uint8_t> bytes;
			return forest.size() <= PLAYER_SNAPSHOT_MAX_OBJECTS &&
			       player_item_snapshot_list_encode(forest, &bytes) ==
				       player_snapshot_codec_result::ok &&
			       shop_trade_recovery_forest_verify(bytes, role, binding);
		};
		const auto &manifest = payload.recovery_manifest;
		if (!verifies(cold.original_player, shop_trade_recovery_forest_role::player_before,
			      manifest.player_before) ||
		    !verifies(cold.original_keeper, shop_trade_recovery_forest_role::keeper_before,
			      manifest.keeper_before) ||
		    !verifies(entry.after_player, shop_trade_recovery_forest_role::player_after,
			      manifest.player_after) ||
		    !verifies(entry.after_keeper, shop_trade_recovery_forest_role::keeper_after,
			      manifest.keeper_after))
			return false;
		std::vector<player_item_snapshot> source, selected_after;
		if (player_item_snapshot_list_decode(payload.item_blob.data(),
						     payload.item_blob_size,
						     &source) != player_snapshot_codec_result::ok ||
		    !shop_trade_accounted_after_items(payload, &selected_after) || source.empty() ||
		    !flat_publication_items(source, cold.source) ||
		    !flat_publication_items(selected_after, cold.selected_after))
			return false;
		auto candidate = std::make_unique<cold_flat_working_variants>();
		candidate->command = bound_command;
		// Dedupe only complete canonical equality; no UID/VNUM approximation.
		const auto retain = [](auto &forests, uint8_t &count,
				       std::vector<player_item_snapshot> values, uint8_t *index)
		{
			for (uint8_t i = 0; i < count; ++i)
				if (flat_publication_items(forests[i], values))
				{
					*index = i;
					return true;
				}
			if (count == forests.size() || values.size() > PLAYER_SNAPSHOT_MAX_OBJECTS)
				return false;
			std::vector<uint8_t> bytes;
			if (player_item_snapshot_list_encode(values, &bytes) !=
				    player_snapshot_codec_result::ok ||
			    bytes.size() > PLAYER_SNAPSHOT_MAX_BYTES - sizeof(uint32_t))
				return false;
			*index = count;
			forests[count++] = std::move(values);
			return true;
		};
		shop_trade_world_cold_observation observed;
		std::vector<shop_trade_world_uid_expectation> locations;
		const bool terminal = cold_flat_world(opaque, entry.after_player,
						      entry.after_keeper, true, !cold.actor_present,
						      !cold.keeper_present, &observed,
						      &locations) &&
				      (!payload.target_parent_item_uid || !observed.actor_present ||
				       (observed.target_present &&
					verifies(observed.target_items,
						 shop_trade_recovery_forest_role::live_target_after,
						 manifest.live_target_after)));
		if (!terminal &&
		    (!cold_flat_world(opaque, cold.working_player, cold.working_keeper, false,
				      false, false, &observed, &locations) ||
		     observed.actor_present != cold.actor_present ||
		     observed.keeper_present != cold.keeper_present ||
		     observed.actor_runtime_id != cold.actor_runtime_id ||
		     observed.keeper_runtime_id != cold.keeper_runtime_id ||
		     (observed.actor_present &&
		      !flat_publication_items(observed.player_items, cold.working_player)) ||
		     (observed.keeper_present &&
		      !flat_publication_items(observed.keeper_items, cold.working_keeper))))
			return false;
		if (observed.actor_present != cold.actor_present ||
		    observed.keeper_present != cold.keeper_present ||
		    observed.actor_runtime_id != cold.actor_runtime_id ||
		    observed.keeper_runtime_id != cold.keeper_runtime_id)
			return false;
		// Actual absent-body request spans remain empty; the retained full owner
		// AFTER forest remains authoritative. No target literal is invented there.
		if ((!observed.actor_present &&
		     !flat_publication_items(cold.working_player, entry.after_player)) ||
		    (!observed.keeper_present &&
		     !flat_publication_items(cold.working_keeper, entry.after_keeper)))
			return false;
		auto &initial = candidate->states[0];
		if (!retain(candidate->player, candidate->player_count,
			    terminal ? entry.after_player : cold.working_player, &initial.player) ||
		    !retain(candidate->keeper, candidate->keeper_count,
			    terminal ? entry.after_keeper : cold.working_keeper, &initial.keeper) ||
		    !retain(candidate->target, candidate->target_count, observed.target_items,
			    &initial.target))
			return false;
		initial.for_nesting = cold.for_nesting;
		if (!terminal)
		{
			const auto object = [&](uint64_t uid) -> P_obj
			{
				const auto found =
					std::lower_bound(locations.begin(), locations.end(), uid,
							 [](const auto &location, uint64_t value)
							 { return location.uid < value; });
				if (locations.size() != observed.objects.size() ||
				    found == locations.end() || found->uid != uid)
					return nullptr;
				return observed
					.objects[static_cast<size_t>(found - locations.begin())];
			};
			P_obj selected = object(payload.selected_item_uid);
			if (selected && !flat_publication_items(observed.selected_items, source) &&
			    !flat_publication_items(observed.selected_items, selected_after))
				return false;
			// A missing selected body has only this genuine, still unpublished
			// private literal root as an ordering witness. Enrollment remains root-owned.
			if (!selected && cold.flat_literals && !cold.flat_literals->staged.empty())
				selected = cold.flat_literals->staged.front().object_;
			if (!selected || selected->obj_uid != payload.selected_item_uid)
				return false;
			const bool buying = payload.action == shop_trade_action::buy_existing ||
					    payload.action == shop_trade_action::buy_produced;
			const bool destroying = payload.action == shop_trade_action::sell_destroy ||
						payload.action ==
							shop_trade_action::discard_invalid;
			const bool destination_absent = buying ? !observed.actor_present :
								 !observed.keeper_present;
			enum class place
			{
				actor,
				keeper,
				detached,
				nested,
				absent
			};
			place selected_place;
			if (observed.actor && OBJ_CARRIED_BY(selected, observed.actor))
				selected_place = place::actor;
			else if (observed.keeper && OBJ_CARRIED_BY(selected, observed.keeper))
				selected_place = place::keeper;
			else if (OBJ_NOWHERE(selected))
				selected_place = place::detached;
			else
				return false;
			P_obj target = payload.target_parent_item_uid ?
					       object(payload.target_parent_item_uid) :
					       nullptr;
			if (payload.target_parent_item_uid && observed.actor_present &&
			    (!target || !observed.target_present ||
			     !flat_publication_items(observed.target_items, cold.working_target) ||
			     !verifies(observed.target_items,
				       shop_trade_recovery_forest_role::live_target_before,
				       manifest.live_target_before)))
				return false;
			const auto final_place = destroying || destination_absent ? place::absent :
						 buying ? (payload.target_parent_item_uid ?
								   place::nested :
								   place::actor) :
							  place::keeper;
			bool finished = false;
			while (candidate->leg_count < candidate->steps.size())
			{
				const auto current = candidate->states[candidate->leg_count];
				auto next = current;
				auto player = candidate->player[current.player];
				auto keeper = candidate->keeper[current.keeper];
				auto destination = candidate->target[current.target];
				shop_trade_cold_native_step step;
				const bool on_actor = selected_place == place::actor;
				const bool on_keeper = selected_place == place::keeper;
				if (destroying || destination_absent)
				{
					if (destroying &&
					    !(payload.action == shop_trade_action::discard_invalid ?
						      on_keeper :
						      on_actor))
						return false;
					if ((on_actor &&
					     !shop_remove_selected(player, payload, &player)) ||
					    (on_keeper &&
					     !shop_remove_selected(keeper, payload, &keeper)))
						return false;
					step = destroying ?
						       shop_trade_cold_native_step::destroy :
						       shop_trade_cold_native_step::retire_copy;
					selected_place = place::absent;
				}
				else if (on_actor || on_keeper)
				{
					if ((buying && on_actor &&
					     !payload.target_parent_item_uid) ||
					    (!buying && on_keeper) ||
					    (on_actor &&
					     !shop_remove_selected(player, payload, &player)) ||
					    (on_keeper &&
					     !shop_remove_selected(keeper, payload, &keeper)))
						return false;
					next.for_nesting = buying && on_actor &&
							   payload.target_parent_item_uid;
					step = shop_trade_cold_native_step::detach;
					selected_place = place::detached;
				}
				else if (selected_place == place::detached && buying)
				{
					std::vector<player_item_snapshot> values, ordered;
					if (current.for_nesting)
					{
						std::vector<uint8_t> original_target;
						if (!target ||
						    !expected_player_forest(payload, player, false,
									    &values) ||
						    !expected_player_order(payload, observed.actor,
									   selected, target, values,
									   &ordered) ||
						    player_item_snapshot_list_encode(
							    destination, &original_target) !=
							    player_snapshot_codec_result::ok ||
						    !expected_destination_forest(
							    payload, observed.actor, selected,
							    target, original_target,
							    &destination) ||
						    !verifies(destination,
							      shop_trade_recovery_forest_role::
								      live_target_after,
							      manifest.live_target_after))
							return false;
						step = shop_trade_cold_native_step::nest;
						selected_place = place::nested;
					}
					else
					{
						// Original immutable command supplies AFTER values. Only the
						// ordering selector describes temporary carrying; never encode it.
						shop_trade_payload carrying{};
						carrying.player_pid = payload.player_pid;
						carrying.action = payload.action;
						carrying.selected_item_uid =
							payload.selected_item_uid;
						if (!(payload.target_parent_item_uid ?
							      flat_publication_carried_values(
								      payload, player, &values) :
							      expected_player_forest(payload,
										     player, false,
										     &values)) ||
						    !expected_player_order(carrying, observed.actor,
									   selected, nullptr,
									   values, &ordered))
							return false;
						step = shop_trade_cold_native_step::place_player;
						selected_place = place::actor;
					}
					player = std::move(ordered);
				}
				else if (selected_place == place::detached && !buying &&
					 payload.action == shop_trade_action::sell_store)
				{
					if (!expected_keeper_forest(payload, observed.keeper,
								    selected, keeper, false,
								    &keeper))
						return false;
					step = shop_trade_cold_native_step::place_keeper;
					selected_place = place::keeper;
				}
				else
					return false;
				if (!retain(candidate->player, candidate->player_count,
					    std::move(player), &next.player) ||
				    !retain(candidate->keeper, candidate->keeper_count,
					    std::move(keeper), &next.keeper) ||
				    !retain(candidate->target, candidate->target_count,
					    std::move(destination), &next.target))
					return false;
				candidate->steps[candidate->leg_count++] = step;
				candidate->states[candidate->leg_count] = next;
				if (selected_place == final_place)
				{
					finished = flat_publication_items(
							   candidate->player[next.player],
							   entry.after_player) &&
						   flat_publication_items(
							   candidate->keeper[next.keeper],
							   entry.after_keeper);
					break;
				}
			}
			if (!finished)
				return false;
		}
		// Only this final nonthrowing move changes the retained stage. Root must
		// census/charge all its capacities before any enrollment/native effect.
		cold.flat_working = std::move(candidate);
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool shop_trade_native_publication_owner::cold_flat_working_state(
	void *opaque, size_t completed_legs, std::span<const player_item_snapshot> *player,
	std::span<const player_item_snapshot> *keeper,
	std::span<const player_item_snapshot> *target, shop_trade_cold_native_step *step,
	bool *terminal, bool *for_nesting) noexcept
{
	if (!opaque || !player || !keeper || !target || !step || !terminal || !for_nesting ||
	    player == keeper || player == target || keeper == target || terminal == for_nesting ||
	    !nevent_is_game_thread() || persistence_mode_get() != PERSISTENCE_MODE_FLATFILE_PRIMARY)
		return false;
	const auto &entry = *static_cast<pending_trade *>(opaque);
	if (!entry.cold || !entry.cold->command || !entry.cold->flat_working || entry.blocked)
		return false;
	const auto &plan = *entry.cold->flat_working;
	if (plan.command != entry.cold->command.get() || completed_legs > plan.leg_count)
		return false;
	const auto state = plan.states[completed_legs];
	if (state.player >= plan.player_count || state.keeper >= plan.keeper_count ||
	    state.target >= plan.target_count)
		return false;
	*player = plan.player[state.player];
	*keeper = plan.keeper[state.keeper];
	*target = plan.target[state.target];
	*terminal = completed_legs == plan.leg_count;
	// The terminal state has no callback. Its step output is not a permission.
	*step = *terminal ? shop_trade_cold_native_step::detach : plan.steps[completed_legs];
	*for_nesting = state.for_nesting;
	return true;
}

bool shop_trade_native_publication_owner::cold_flat_retained_payload_bytes(
	void *opaque, size_t *output, const critical_command *retained) noexcept
{
	if (!opaque || !output || !nevent_is_game_thread() ||
	    persistence_mode_get() != PERSISTENCE_MODE_FLATFILE_PRIMARY)
		return false;
	const auto &entry = *static_cast<pending_trade *>(opaque);
	if (!entry.cold || !entry.cold->command || entry.preparation || entry.blocked ||
	    entry.cold->registration_pending || !entry.cold->initialized ||
	    entry.cold->enrollment_started || entry.cold->enrolled || entry.cold->source.empty() ||
	    entry.cold->source.size() != entry.payload.item_count)
		return false;
	try
	{
		const auto &cold = *entry.cold;
		flat_preparation_bytes bytes;
		// The earlier passive registration already charges the map/cold structs,
		// original command, manifest and UID union. These are additional real
		// allocations, never a second charge for their embedded vector objects.
		for (const auto *forest :
		     { &entry.before_player, &entry.after_player, &entry.after_keeper,
		       &cold.original_player, &cold.original_keeper, &cold.working_player,
		       &cold.working_keeper, &cold.source, &cold.selected_after,
		       &cold.working_target })
			if (forest->size() > PLAYER_SNAPSHOT_MAX_OBJECTS || !bytes.items(*forest))
				return false;
		if ((retained && !critical_command_equal(*retained, *cold.command)) ||
		    !cold.flat_working ||
		    cold.flat_working->command != (retained ? retained : cold.command.get()) ||
		    !bytes.add(sizeof(cold_flat_working_variants)))
			return false;
		for (const auto &forest : cold.flat_working->player)
			if (!bytes.items(forest))
				return false;
		for (const auto &forest : cold.flat_working->keeper)
			if (!bytes.items(forest))
				return false;
		for (const auto &forest : cold.flat_working->target)
			if (!bytes.items(forest))
				return false;
		if (!bytes.vector(entry.after_destination))
			return false;
		if (cold.flat_literals)
		{
			const auto &stage = *cold.flat_literals;
			if (stage.binding_started || stage.binding_returned ||
			    stage.staged.size() != cold.source.size() ||
			    stage.reload.size() != cold.source.size() ||
			    stage.enrollment_counts.size() > cold.source.size() ||
			    !stage.bindings.valid_flat() || !bytes.add(sizeof(stage)) ||
			    !bytes.vector(stage.staged) || !bytes.vector(stage.reload) ||
			    !bytes.vector(stage.enrollment_counts))
				return false;
			// Binding's own visitor includes sizeof(bindings), already embedded
			// in sizeof(stage). Add only its independently retained capacities.
			const size_t binding_bytes = stage.bindings.retained_bytes();
			if (binding_bytes < sizeof(stage.bindings) ||
			    !bytes.add(binding_bytes - sizeof(stage.bindings)))
				return false;
			for (size_t index = 0; index < stage.staged.size(); ++index)
			{
				size_t native_bytes = 0;
				if (!stage.staged[index].object_ ||
				    !stage.staged[index].retained_bytes(cold.source[index],
									&native_bytes) ||
				    !native_bytes || !bytes.add(native_bytes) ||
				    !stage.reload[index].prototype ||
				    stage.reload[index].proclib_probes.size() !=
					    cold.source[index].extra_descriptions.size() ||
				    !bytes.vector(stage.reload[index].proclib_probes))
					return false;
			}
		}
		// A consumed graph cannot shrink an existing reservation: this visitor
		// is closed once enrollment starts. Caller records the one complete count
		// only after all future retained working-state storage is allocated too.
		if (!bytes.value)
			return false;
		*output = bytes.value;
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool shop_trade_native_publication_owner::cold_enroll_selected_flat(
	player_save_restored_publication_owner &publication, const std::string &root,
	const flatfile_authority_lock &authority,
	const flatfile_accounting_shop_projection &current, size_t complete_retained_stage_bytes,
	void *opaque) noexcept
{
#ifndef __NO_MYSQL__
	(void)publication;
	(void)root;
	(void)authority;
	(void)current;
	(void)complete_retained_stage_bytes;
	(void)opaque;
	return false;
#else
	if (!opaque || !nevent_is_game_thread() ||
	    persistence_mode_get() != PERSISTENCE_MODE_FLATFILE_PRIMARY || root.empty() ||
	    !persistence_mode_flatfile_root() || root != persistence_mode_flatfile_root() ||
	    !authority.matches(root) || !complete_retained_stage_bytes ||
	    complete_retained_stage_bytes > PLAYER_SAVE_PIPELINE_MAX_BYTES ||
	    !publication.flat_shop_restored_ || publication.flat_shop_ ||
	    publication.acknowledged_ || publication.publication_proven_ ||
	    !publication.reservation_.valid() ||
	    publication.completion_.disposition != critical_completion_disposition::execution ||
	    (publication.completion_.outcome != critical_apply_outcome::applied &&
	     publication.completion_.outcome != critical_apply_outcome::already_applied))
		return false;
	auto &entry = *static_cast<pending_trade *>(opaque);
	if (!entry.cold || !entry.cold->command || !entry.cold->flat_literals ||
	    entry.preparation || entry.blocked || entry.cold->registration_pending ||
	    !entry.cold->initialized || entry.cold->enrollment_started || entry.cold->enrolled ||
	    publication.pid_ != static_cast<int>(entry.player_pid) ||
	    publication.flat_shop_restored_root_uid_ != entry.payload.selected_item_uid ||
	    !critical_command_equal(*entry.cold->command, publication.command_) ||
	    !same_native_receipt(entry.completed, publication.completion_))
		return false;
	auto &cold = *entry.cold;
	auto &stage = *cold.flat_literals;
	if (stage.binding_started || stage.binding_returned ||
	    stage.staged.size() != cold.source.size() ||
	    stage.reload.size() != cold.source.size() || !stage.bindings.valid_flat())
		return false;
	try
	{
		const auto original_current = [&]() noexcept
		{
			return !entry.blocked && publication.reservation_.valid() &&
			       critical_command_equal(*cold.command, publication.command_) &&
			       same_native_receipt(entry.completed, publication.completion_) &&
			       critical_command_coordinator_restored_shop_publication_current(
				       publication);
		};
		std::string error;
		flatfile_accounting_shop_projection rechecked;
		shop_trade_world_cold_observation observed;
		std::vector<shop_trade_world_uid_expectation> locations;
		std::vector<uint8_t> retained_payload, original_payload;
		shop_trade_payload original{};
		if (!original_current() ||
		    !shop_trade_command_decode_payload(publication.command_, &original) ||
		    !shop_trade_command_encode_recovery_payload(original, &original_payload) ||
		    !shop_trade_command_encode_recovery_payload(entry.payload, &retained_payload) ||
		    original_payload != retained_payload ||
		    flatfile_accounting_shop_transaction::read_current_locked(
			    root, authority, publication.command_, publication.completion_,
			    &rechecked, &error) ||
		    !flat_publication_same_current(current, rechecked) ||
		    !cold_flat_world(&entry, cold.working_player, cold.working_keeper, false, false,
				     false, &observed, &locations) ||
		    observed.actor_present != cold.actor_present ||
		    observed.keeper_present != cold.keeper_present ||
		    observed.actor_runtime_id != cold.actor_runtime_id ||
		    observed.keeper_runtime_id != cold.keeper_runtime_id ||
		    (observed.actor_present &&
		     !flat_publication_items(observed.player_items, cold.working_player)) ||
		    (observed.keeper_present &&
		     !flat_publication_items(observed.keeper_items, cold.working_keeper)) ||
		    (entry.payload.target_parent_item_uid && observed.actor_present &&
		     (!observed.target_present ||
		      !flat_publication_items(observed.target_items, cold.working_target))))
			return false;
		for (const auto &item : cold.source)
		{
			const auto found = std::lower_bound(locations.begin(), locations.end(),
							    item.object_uid,
							    [](const auto &location, uint64_t uid)
							    { return location.uid < uid; });
			if (found == locations.end() || found->uid != item.object_uid ||
			    observed.objects[static_cast<size_t>(found - locations.begin())])
				return false;
		}
		// This supplemental source proof does not replace the full original world
		// census. Cash follows the real retained BEFORE/CURRENT publication phase.
		int64_t cash = (entry.physical_stages & 128U) ? current.keeper.cash :
								entry.payload.expected_keeper_cash;
		if (observed.keeper)
		{
			if (GET_COPPER(observed.keeper) < 0 || GET_SILVER(observed.keeper) < 0 ||
			    GET_GOLD(observed.keeper) < 0 || GET_PLATINUM(observed.keeper) < 0)
				return false;
			const int64_t actual = static_cast<int64_t>(GET_COPPER(observed.keeper)) +
					       10LL * GET_SILVER(observed.keeper) +
					       100LL * GET_GOLD(observed.keeper) +
					       1000LL * GET_PLATINUM(observed.keeper);
			if (actual != cash &&
			    ((entry.physical_stages & 128U) || actual != current.keeper.cash))
				return false;
			cash = actual; // Genuine original BEFORE or already CURRENT cash only.
		}
		if (!shop_trade_native_checkpoint_owner::observe_flat_cold_publication_sources_locked(
			    root, authority, observed.actor, observed.keeper, entry.payload.shop_id,
			    current, cold.working_player, cold.working_keeper, cash) ||
		    !publication.reserve_shop_flat_restored_publication_checkpoint(
			    complete_retained_stage_bytes) ||
		    !original_current())
			return false;
		for (const auto &item : stage.staged)
			if (!item.object_ ||
			    !quest_mobile_native_item_cold_prepend_body_ready(item.object_))
				return false;
		for (const auto &[number, count] : stage.enrollment_counts)
			if (!obj_index || number < 0 || number > top_of_objt ||
			    obj_index[number].number < 0 ||
			    count > static_cast<size_t>(INT_MAX - obj_index[number].number))
				return false;
		if (!stage.bindings.valid_flat() ||
		    !quest_mobile_native_item_cold_prepend_cut_ready(stage.staged.size()) ||
		    !original_current())
			return false;
		// All fallible work/allocation completes before these retained once marks.
		// Original binding commit and body/count enrollment are allocation-free.
		cold.enrollment_started = stage.binding_started = true;
		stage.bindings.commit_flat_unchecked();
		stage.binding_returned = true;
		for (auto &item : stage.staged)
		{
			P_obj object = item.object_;
			quest_mobile_native_item_observe_native_prepend(object);
			object->next = object_list;
			if (object_list)
				object_list->prev = object;
			object_list = object;
			++obj_index[object->R_num].number;
			item.object_ = nullptr;
			item.pool_ = nullptr;
			item.affect_pool_ = nullptr;
		}
		cold.enrolled = true;
		return true;
	}
	catch (...)
	{
		return false;
	}
#endif
}

bool shop_trade_native_publication_owner::native_publish_flat_cold(
	player_save_restored_publication_owner &publication, void *opaque) noexcept
{
#ifndef __NO_MYSQL__
	(void)publication;
	(void)opaque;
	return false;
#else
	if (!opaque || !nevent_is_game_thread() ||
	    persistence_mode_get() != PERSISTENCE_MODE_FLATFILE_PRIMARY ||
	    !publication.flat_shop_restored_ || publication.flat_shop_ ||
	    publication.acknowledged_ || publication.publication_proven_ ||
	    !publication.reservation_.matches_pid(publication.pid_) ||
	    publication.completion_.disposition != critical_completion_disposition::execution)
		return false;
	auto &entry = *static_cast<pending_trade *>(opaque);
	if (!entry.cold || !entry.cold->flat_restored || !entry.cold->command ||
	    entry.cold->registration_pending || entry.preparation || entry.blocked ||
	    !entry.completion_ready || publication.pid_ != static_cast<int>(entry.player_pid) ||
	    publication.flat_shop_restored_root_uid_ != entry.payload.selected_item_uid ||
	    !same_native_receipt(entry.completed, publication.completion_))
		return false;
	auto &cold = *entry.cold;
	if ((cold.effect.started && (!cold.effect.returned || !cold.effect.succeeded)) ||
	    (cold.enrollment_started && !cold.enrolled) ||
	    (cold.balances_started && !cold.balances_returned))
		return false;
	try
	{
		const auto &command = publication.command_;
		const auto &sealed = publication.completion_;
		std::vector<uint8_t> frozen, original_payload, retained_payload;
		shop_trade_payload payload{};
		if (!critical_command_equal(command, *cold.command) ||
		    critical_command_encode(command, &frozen) !=
			    critical_command_codec_result::ok ||
		    frozen != publication.frozen_ ||
		    !shop_trade_command_decode_payload(command, &payload) ||
		    !shop_trade_command_encode_recovery_payload(payload, &original_payload) ||
		    !shop_trade_command_encode_recovery_payload(entry.payload, &retained_payload) ||
		    original_payload != retained_payload || !persistence_mode_flatfile_root())
			return false;
		const std::string root(persistence_mode_flatfile_root());
		const auto session = [&]() noexcept
		{
			return !entry.blocked && !root.empty() &&
			       persistence_mode_get() == PERSISTENCE_MODE_FLATFILE_PRIMARY &&
			       persistence_mode_flatfile_root() &&
			       root == persistence_mode_flatfile_root() &&
			       publication.reservation_.matches_pid(publication.pid_) &&
			       same_native_receipt(entry.completed, sealed) &&
			       critical_command_equal(*cold.command, command) &&
			       critical_command_coordinator_restored_shop_publication_current(
				       publication);
		};
		if (!session())
			return false;
		flatfile_authority_lock authority;
		std::string error;
		if (!authority.acquire(root, &error) || !session())
			return false;
		flatfile_accounting_shop_projection current;
		if (flatfile_accounting_shop_transaction::read_current_locked(
			    root, authority, command, sealed, &current, &error))
			return false;
		const bool success = sealed.outcome == critical_apply_outcome::applied ||
				     sealed.outcome == critical_apply_outcome::already_applied;
		if (!success && sealed.outcome != critical_apply_outcome::terminal_failure)
			return false;
		const auto &manifest = payload.recovery_manifest;
		const bool produced = payload.action == shop_trade_action::buy_produced;
		const bool buying = produced || payload.action == shop_trade_action::buy_existing;
		const bool destroying = payload.action == shop_trade_action::sell_destroy ||
					payload.action == shop_trade_action::discard_invalid;
		uint64_t current_level = 0;
		bool level_seen = false;
		for (const auto &row : current.player.status_integers)
			if (row.field == player_status_field::level)
			{
				if (level_seen ||
				    (row.is_unsigned ?
					     !row.unsigned_value || row.unsigned_value > UINT8_MAX :
					     row.signed_value <= 0 || row.signed_value > UINT8_MAX))
					return false;
				current_level = row.is_unsigned ?
							row.unsigned_value :
							static_cast<uint64_t>(row.signed_value);
				level_seen = true;
			}
		if (!level_seen || current.player.revision < payload.expected_player_save_revision)
			return false;
		currency_vector wallet{}, bank{};
		for (size_t index = 0; index < wallet.amount.size(); ++index)
		{
			if (current.player_domains.domains.wallet[index] > INT_MAX ||
			    current.player_domains.domains.bank[index] > INT_MAX)
				return false;
			wallet.amount[index] =
				static_cast<int64_t>(current.player_domains.domains.wallet[index]);
			bank.amount[index] =
				static_cast<int64_t>(current.player_domains.domains.bank[index]);
		}
		const auto verify_forest = [](const std::vector<player_item_snapshot> &forest,
					      shop_trade_recovery_forest_role role,
					      const shop_trade_recovery_forest_binding &binding)
		{
			std::vector<uint8_t> bytes;
			return player_item_snapshot_list_encode(forest, &bytes) ==
				       player_snapshot_codec_result::ok &&
			       shop_trade_recovery_forest_verify(bytes, role, binding);
		};
		if (!verify_forest(current.player.items,
				   success ? shop_trade_recovery_forest_role::player_after :
					     shop_trade_recovery_forest_role::player_before,
				   success ? manifest.player_after : manifest.player_before) ||
		    !verify_forest(current.keeper.items,
				   success ? shop_trade_recovery_forest_role::keeper_after :
					     shop_trade_recovery_forest_role::keeper_before,
				   success ? manifest.keeper_after : manifest.keeper_before))
			return false;
		shop_trade_world_cold_observation observed;
		std::vector<shop_trade_world_uid_expectation> locations;
		const auto object = [&](uint64_t uid) -> P_obj
		{
			const auto found = std::lower_bound(locations.begin(), locations.end(), uid,
							    [](const auto &item, uint64_t value)
							    { return item.uid < value; });
			return locations.size() != observed.objects.size() ||
					       found == locations.end() || found->uid != uid ?
				       nullptr :
				       observed.objects[static_cast<size_t>(found -
									    locations.begin())];
		};
		const auto missing_owner = [&](const shop_trade_recovery_forest_binding &before,
					       const shop_trade_recovery_forest_binding &after)
		{
			for (const auto *binding : { &before, &after })
				for (uint64_t uid : binding->ordered_item_uids)
				{
					const bool selected = std::any_of(
						payload.items.begin(),
						payload.items.begin() + payload.item_count,
						[uid](const auto &item)
						{ return item.item_uid == uid; });
					if (!selected && object(uid))
						return false;
				}
			return true;
		};
		const auto sources = [&](const std::vector<player_item_snapshot> &player,
					 const std::vector<player_item_snapshot> &keeper)
		{
			if ((observed.actor &&
			     (static_cast<uint64_t>(GET_LEVEL(observed.actor)) != current_level ||
			      observed.actor->only.pc->wallet_revision >
				      current.player_domains.domains.wallet_revision ||
			      observed.actor->only.pc->bank_revision >
				      current.player_domains.domains.bank_revision)) ||
			    (!observed.actor_present &&
			     !missing_owner(manifest.player_before, manifest.player_after)) ||
			    (!observed.keeper_present &&
			     !missing_owner(manifest.keeper_before, manifest.keeper_after)))
				return false;
			int64_t cash = (entry.physical_stages & 128U) ?
					       current.keeper.cash :
					       payload.expected_keeper_cash;
			if (observed.keeper)
			{
				if (GET_COPPER(observed.keeper) < 0 ||
				    GET_SILVER(observed.keeper) < 0 ||
				    GET_GOLD(observed.keeper) < 0 ||
				    GET_PLATINUM(observed.keeper) < 0)
					return false;
				const int64_t actual =
					static_cast<int64_t>(GET_COPPER(observed.keeper)) +
					10LL * GET_SILVER(observed.keeper) +
					100LL * GET_GOLD(observed.keeper) +
					1000LL * GET_PLATINUM(observed.keeper);
				// Original cold restore accepts genuine BEFORE or already CURRENT
				// cash. A returned cash stage requires CURRENT; no other value is adopted.
				if (actual != cash && ((entry.physical_stages & 128U) ||
						       actual != current.keeper.cash))
					return false;
				cash = actual;
			}
			return session() &&
			       shop_trade_native_checkpoint_owner::
				       observe_flat_cold_publication_sources_locked(
					       root, authority, observed.actor, observed.keeper,
					       payload.shop_id, current, player, keeper, cash);
		};
		const auto original_current = [&]()
		{
			flatfile_accounting_shop_projection fresh;
			return session() && authority.matches(root) &&
			       !flatfile_accounting_shop_transaction::read_current_locked(
				       root, authority, command, sealed, &fresh, &error) &&
			       flat_publication_same_current(current, fresh) && session();
		};
		// All retained cold allocations are built off-map, charged in full, then
		// installed by nonthrowing moves. Failed reservation cannot accumulate an
		// uncharged stage in other passive slots. Original command/UID union stay put.
		if (!cold.initialized)
		{
			pending_trade candidate;
			candidate.player_pid = entry.player_pid;
			candidate.payload = entry.payload;
			candidate.completed = sealed;
			candidate.completion_ready = candidate.native_receipt_verified = true;
			candidate.cold = std::make_unique<cold_trade_restore>();
			auto &prepared = *candidate.cold;
			prepared.command = std::make_unique<critical_command>(*cold.command);
			prepared.fenced_uids = cold.fenced_uids;
			prepared.registration_pending = false;
			if (!cold_flat_original_forests(command, current, &prepared.original_player,
							&prepared.original_keeper) ||
			    player_item_snapshot_list_decode(
				    payload.item_blob.data(), payload.item_blob_size,
				    &prepared.source) != player_snapshot_codec_result::ok ||
			    !shop_trade_accounted_after_items(payload, &prepared.selected_after) ||
			    !cold_flat_world(&candidate, prepared.original_player,
					     prepared.original_keeper, false, false, false,
					     &observed, &locations))
				return false;
			if ((observed.actor_present &&
			     !flat_publication_items(observed.player_items,
						     prepared.original_player) &&
			     !flat_publication_items(observed.player_items,
						     current.player.items)) ||
			    (observed.keeper_present &&
			     !flat_publication_items(observed.keeper_items,
						     prepared.original_keeper) &&
			     !flat_publication_items(observed.keeper_items,
						     current.keeper.items)) ||
			    (!observed.selected_items.empty() &&
			     !flat_publication_items(observed.selected_items, prepared.source) &&
			     !flat_publication_items(observed.selected_items,
						     prepared.selected_after)) ||
			    !sources(observed.actor_present ? observed.player_items :
							      prepared.original_player,
				     observed.keeper_present ? observed.keeper_items :
							       prepared.original_keeper))
				return false;
			prepared.actor_present = observed.actor_present;
			prepared.keeper_present = observed.keeper_present;
			prepared.actor_runtime_id = observed.actor_runtime_id;
			prepared.keeper_runtime_id = observed.keeper_runtime_id;
			prepared.working_player = observed.actor_present ? observed.player_items :
									   current.player.items;
			prepared.working_keeper = observed.keeper_present ? observed.keeper_items :
									    current.keeper.items;
			prepared.working_target = observed.target_items;
			candidate.before_player = prepared.original_player;
			candidate.after_player = current.player.items;
			candidate.after_keeper = current.keeper.items;
			candidate.publication_forests_ready = true;
			prepared.initialized = true;
			P_obj selected = object(payload.selected_item_uid);
			shop_trade_world_cold_observation final;
			std::vector<shop_trade_world_uid_expectation> final_locations;
			const bool final_state =
				cold_flat_world(&candidate, candidate.after_player,
						candidate.after_keeper, true,
						!prepared.actor_present, !prepared.keeper_present,
						&final, &final_locations) &&
				(!payload.target_parent_item_uid || !final.actor_present ||
				 (final.target_present &&
				  verify_forest(
					  final.target_items,
					  success ?
						  shop_trade_recovery_forest_role::live_target_after :
						  shop_trade_recovery_forest_role::live_target_before,
					  success ? manifest.live_target_after :
						    manifest.live_target_before)));
			if (success)
			{
				if (!final_state)
				{
					if (selected && OBJ_NOWHERE(selected) && !produced)
						return false; // No normally returned preboot departure tail is invented.
					if (!selected)
					{
						if (destroying ||
						    (buying ? !prepared.actor_present :
							      !prepared.keeper_present))
							return false;
						for (const auto &item : prepared.source)
							if (object(item.object_uid))
								return false;
						if (!cold_prepare_selected_flat(&candidate))
							return false;
					}
				}
				if (!cold_prepare_working_flat(&candidate, cold.command.get()))
					return false;
			}
			else
			{
				// Genuine execution rejection authenticates unchanged complete BEFORE.
				// Only an original produced detached copy may require terminal cleanup.
				if (!flat_publication_items(prepared.working_player,
							    candidate.after_player) ||
				    !flat_publication_items(prepared.working_keeper,
							    candidate.after_keeper) ||
				    (!final_state &&
				     (!produced || !selected || !OBJ_NOWHERE(selected) ||
				      selected->next_content ||
				      !flat_publication_items(observed.selected_items,
							      prepared.source))) ||
				    (payload.target_parent_item_uid && prepared.actor_present &&
				     (!observed.target_present ||
				      !verify_forest(
					      observed.target_items,
					      shop_trade_recovery_forest_role::live_target_before,
					      manifest.live_target_before))))
					return false;
				auto working = std::make_unique<cold_flat_working_variants>();
				working->command = cold.command.get();
				working->player[0] = prepared.working_player;
				working->keeper[0] = prepared.working_keeper;
				working->target[0] = prepared.working_target;
				working->player_count = working->keeper_count =
					working->target_count = 1;
				prepared.flat_working = std::move(working);
			}
			size_t complete_bytes = 0;
			if (!cold_flat_retained_payload_bytes(&candidate, &complete_bytes,
							      cold.command.get()) ||
			    !original_current() ||
			    !sources(prepared.working_player, prepared.working_keeper) ||
			    !publication.reserve_shop_flat_restored_publication_checkpoint(
				    complete_bytes))
				return false;
			// Reserve succeeded; every following retained move is nonthrowing.
			entry.before_player = std::move(candidate.before_player);
			entry.after_player = std::move(candidate.after_player);
			entry.after_keeper = std::move(candidate.after_keeper);
			entry.after_destination = std::move(candidate.after_destination);
			cold.original_player = std::move(prepared.original_player);
			cold.original_keeper = std::move(prepared.original_keeper);
			cold.working_player = std::move(prepared.working_player);
			cold.working_keeper = std::move(prepared.working_keeper);
			cold.source = std::move(prepared.source);
			cold.selected_after = std::move(prepared.selected_after);
			cold.working_target = std::move(prepared.working_target);
			cold.flat_literals = std::move(prepared.flat_literals);
			cold.flat_working = std::move(prepared.flat_working);
			cold.actor_present = prepared.actor_present;
			cold.keeper_present = prepared.keeper_present;
			cold.actor_runtime_id = prepared.actor_runtime_id;
			cold.keeper_runtime_id = prepared.keeper_runtime_id;
			cold.flat_publication_bytes = complete_bytes;
			cold.initialized = entry.publication_forests_ready =
				entry.native_receipt_verified = true;
		}
		if (!cold.flat_working || !cold.flat_publication_bytes ||
		    !entry.publication_forests_ready ||
		    !flat_publication_items(entry.after_player, current.player.items) ||
		    !flat_publication_items(entry.after_keeper, current.keeper.items) ||
		    !publication.reserve_shop_flat_restored_publication_checkpoint(
			    cold.flat_publication_bytes))
			return false;
		std::span<const player_item_snapshot> player, keeper, target_values;
		shop_trade_cold_native_step step{};
		bool terminal = false, for_nesting = false;
		const auto working_observe = [&]()
		{
			if (!cold_flat_working_state(&entry, cold.flat_completed_legs, &player,
						     &keeper, &target_values, &step, &terminal,
						     &for_nesting) ||
			    !cold_flat_world(&entry, player, keeper, false, false, false, &observed,
					     &locations) ||
			    observed.actor_present != cold.actor_present ||
			    observed.keeper_present != cold.keeper_present ||
			    observed.actor_runtime_id != cold.actor_runtime_id ||
			    observed.keeper_runtime_id != cold.keeper_runtime_id ||
			    (observed.actor_present &&
			     !flat_publication_items(
				     observed.player_items,
				     cold.flat_working
					     ->player[cold.flat_working
							      ->states[cold.flat_completed_legs]
							      .player])) ||
			    (observed.keeper_present &&
			     !flat_publication_items(
				     observed.keeper_items,
				     cold.flat_working
					     ->keeper[cold.flat_working
							      ->states[cold.flat_completed_legs]
							      .keeper])) ||
			    (payload.target_parent_item_uid && observed.actor_present &&
			     (!observed.target_present ||
			      !flat_publication_items(
				      observed.target_items,
				      cold.flat_working
					      ->target[cold.flat_working
							       ->states[cold.flat_completed_legs]
							       .target]))))
				return false;
			const auto state = cold.flat_working->states[cold.flat_completed_legs];
			return sources(cold.flat_working->player[state.player],
				       cold.flat_working->keeper[state.keeper]) &&
			       original_current();
		};
		const auto final_observe = [&]()
		{
			return cold_flat_world(&entry, entry.after_player, entry.after_keeper, true,
					       !cold.actor_present, !cold.keeper_present, &observed,
					       &locations) &&
			       observed.actor_runtime_id == cold.actor_runtime_id &&
			       observed.keeper_runtime_id == cold.keeper_runtime_id &&
			       (!payload.target_parent_item_uid || !observed.actor_present ||
				(observed.target_present &&
				 verify_forest(
					 observed.target_items,
					 success ?
						 shop_trade_recovery_forest_role::live_target_after :
						 shop_trade_recovery_forest_role::live_target_before,
					 success ? manifest.live_target_after :
						   manifest.live_target_before))) &&
			       sources(entry.after_player, entry.after_keeper) &&
			       original_current();
		};
		if (!working_observe())
			return false;
		// The CURRENT backend/runtime owner independently proves all owner-wide
		// custody and foreign active UID/root/parent aliases before native placement.
		if (!shop_trade_current_runtime_owner::publish_flat(root, authority, command,
								    sealed) ||
		    !working_observe())
			return false;
		if (success && cold.flat_literals && !cold.enrolled)
		{
			if (!cold_enroll_selected_flat(publication, root, authority, current,
						       cold.flat_publication_bytes, &entry) ||
			    !working_observe())
				return false;
		}
		if (cold.enrolled)
		{
			if (!cold.flat_literals ||
			    cold.flat_literals->reload.size() != cold.source.size())
				return false;
			for (size_t index = 0; index < cold.flat_literals->reload.size(); ++index)
				for (unsigned int phase = 0; phase < 4; ++phase)
				{
					auto &reload = cold.flat_literals->reload[index];
					auto &effect = reload.effects[phase];
					if (effect.started)
					{
						if (!effect.returned || !effect.succeeded)
							return false;
						continue;
					}
					if (!reload.prototype || !working_observe() ||
					    !flat_publication_items(observed.selected_items,
								    cold.source))
						return false;
					if (phase == 1)
						effect.periodic = reload.effects[0].periodic;
					if (phase == 2)
					{
						if (reload.proclib_probes.size() !=
						    cold.source[index].extra_descriptions.size())
							return false;
						bool periodic = false;
						for (size_t description = 0;
						     description < reload.proclib_probes.size();
						     ++description)
						{
							auto &probe =
								reload.proclib_probes[description];
							if (probe.started)
							{
								if (!probe.returned ||
								    !probe.succeeded)
									return false;
							}
							else
							{
								if (!working_observe() ||
								    !flat_publication_items(
									    observed.selected_items,
									    cold.source))
									return false;
								P_obj fresh =
									object(cold.source[index]
										       .object_uid);
								if (!fresh ||
								    !shop_trade_original_item_stage::
									    proclib_probe_flat(
										    fresh,
										    *reload.prototype,
										    description,
										    probe) ||
								    !working_observe() ||
								    !flat_publication_items(
									    observed.selected_items,
									    cold.source))
									return false;
							}
							periodic = periodic || probe.periodic;
						}
						effect.periodic = periodic;
						if (!working_observe())
							return false;
					}
					P_obj fresh = object(cold.source[index].object_uid);
					if (!fresh ||
					    !shop_trade_original_item_stage::reload_step_flat(
						    fresh, *reload.prototype, phase, effect) ||
					    !working_observe() ||
					    !flat_publication_items(observed.selected_items,
								    cold.source))
						return false;
				}
		}
		if (!success && produced && !cold.flat_rejected_cleanup_returned)
		{
			if (!working_observe())
				return false;
			P_obj selected = object(payload.selected_item_uid);
			if (selected)
			{
				if (cold.effect.started || !OBJ_NOWHERE(selected) ||
				    selected->next_content ||
				    !flat_publication_items(observed.selected_items, cold.source) ||
				    !original_current())
					return false;
				cold.effect.step = shop_trade_cold_native_step::reject_produced;
				if (!shop_trade_cold_native_step_execute(
					    observed.actor, observed.keeper, selected, nullptr,
					    payload, cold.effect) ||
				    !cold.effect.returned || !cold.effect.succeeded)
					return false;
			}
			cold.flat_rejected_cleanup_returned = true;
			if (!final_observe())
				return false;
		}
		while (success)
		{
			if (!working_observe())
				return false;
			if (terminal)
				break;
			if (cold.effect.started)
			{
				if (!cold.effect.returned || !cold.effect.succeeded ||
				    cold.flat_effect_leg + 1 != cold.flat_completed_legs)
					return false;
				cold.effect = {};
			}
			P_obj selected = object(payload.selected_item_uid);
			P_obj target = payload.target_parent_item_uid ?
					       object(payload.target_parent_item_uid) :
					       nullptr;
			if (!selected ||
			    (!flat_publication_items(observed.selected_items, cold.source) &&
			     !flat_publication_items(observed.selected_items, cold.selected_after)))
				return false;
			// Recheck actual handler grouping with scratch only; never grow/replace
			// the charged immutable forests after a callback changes the native graph.
			if (step == shop_trade_cold_native_step::place_player ||
			    step == shop_trade_cold_native_step::place_keeper ||
			    step == shop_trade_cold_native_step::nest)
			{
				std::vector<player_item_snapshot> values, ordered;
				const auto next =
					cold.flat_working->states[cold.flat_completed_legs + 1];
				if (step == shop_trade_cold_native_step::place_keeper)
				{
					const auto state =
						cold.flat_working->states[cold.flat_completed_legs];
					if (!expected_keeper_forest(
						    payload, observed.keeper, selected,
						    cold.flat_working->keeper[state.keeper], false,
						    &ordered) ||
					    !flat_publication_items(
						    ordered,
						    cold.flat_working->keeper[next.keeper]))
						return false;
				}
				else
				{
					shop_trade_payload carrying{};
					carrying.player_pid = payload.player_pid;
					carrying.action = payload.action;
					carrying.selected_item_uid = payload.selected_item_uid;
					if (!expected_player_order(
						    step == shop_trade_cold_native_step::nest ?
							    payload :
							    carrying,
						    observed.actor, selected,
						    step == shop_trade_cold_native_step::nest ?
							    target :
							    nullptr,
						    cold.flat_working->player[next.player],
						    &ordered) ||
					    !flat_publication_items(
						    ordered,
						    cold.flat_working->player[next.player]))
						return false;
					if (step == shop_trade_cold_native_step::nest &&
					    (!expected_player_order(
						     payload, observed.actor, selected, target,
						     cold.flat_working->target[next.target],
						     &values) ||
					     !flat_publication_items(
						     values,
						     cold.flat_working->target[next.target])))
						return false;
				}
			}
			if (!original_current())
				return false;
			cold.flat_effect_leg = cold.flat_completed_legs;
			cold.effect.step = step;
			if (!shop_trade_cold_native_step_execute(observed.actor, observed.keeper,
								 selected, target, payload,
								 cold.effect) ||
			    !cold.effect.returned || !cold.effect.succeeded)
				return false;
			// Commit advanced expectation immediately after the returned handler,
			// BEFORE any fresh census may refuse. Never replay that returned leg.
			++cold.flat_completed_legs;
			if (!working_observe())
				return false;
		}
		if (!final_observe() ||
		    !shop_trade_current_runtime_owner::publish_flat(root, authority, command,
								    sealed) ||
		    !final_observe())
			return false;
		// Original CURRENT money publication is a retained normally returned
		// notification stage. A failed post-census never reissues its callbacks.
		if (!cold.balances_returned)
		{
			cold.balances_started = true;
			cold.balances_returned = false;
			if (observed.actor)
			{
				if (!currency_transaction_publish_balances(
					    observed.actor, payload.account_name.data(),
					    payload.racewar, wallet, bank,
					    current.player_domains.domains.wallet_revision,
					    current.player_domains.domains.bank_revision))
					return false;
			}
			else
			{
				const AccountBankBalances amounts{ static_cast<int>(bank.amount[0]),
								   static_cast<int>(bank.amount[1]),
								   static_cast<int>(bank.amount[2]),
								   static_cast<int>(
									   bank.amount[3]) };
				publish_account_bank_balances_revision(
					payload.account_name.data(), payload.racewar, &amounts,
					current.player_domains.domains.bank_revision);
			}
			cold.balances_returned = true;
		}
		if (!final_observe())
			return false;
		if (observed.keeper)
		{
			int64_t remainder = current.keeper.cash;
			GET_PLATINUM(observed.keeper) = static_cast<int>(remainder / 1000);
			remainder %= 1000;
			GET_GOLD(observed.keeper) = static_cast<int>(remainder / 100);
			remainder %= 100;
			GET_SILVER(observed.keeper) = static_cast<int>(remainder / 10);
			GET_COPPER(observed.keeper) = static_cast<int>(remainder % 10);
		}
		entry.physical_stages |= 128U;
		if (!final_observe() ||
		    !shop_trade_current_runtime_owner::publish_flat(root, authority, command,
								    sealed) ||
		    !final_observe() || !session())
			return false;
		// No SQL row IDs or live notices are synthesized. Only the original outer
		// owner may turn this full native/source/custody proof into guarded ACK.
		return !observed.actor ||
		       flat_publication_money(observed.actor, current.player_domains);
	}
	catch (...)
	{
		return false;
	}
#endif
}

// Invoked only inside the original coordinator's authenticated cancellation
// callback, after its exact refusal and the pipeline's real flat slot are proved.
// Absence of receipts is supplementary source evidence, never that authority.
bool shop_trade_native_publication_owner::native_publish_flat_refusal(
	player_save_restored_publication_owner &publication, void *opaque) noexcept
{
	if (!opaque || !nevent_is_game_thread() || !publication.flat_shop_ ||
	    publication.flat_shop_restored_ || publication.acknowledged_ ||
	    publication.publication_proven_ || !publication.reservation_.valid() ||
	    publication.completion_.disposition != critical_completion_disposition::never_admitted)
		return false;
	auto &entry = *static_cast<pending_trade *>(opaque);
	if (entry.cold || !entry.preparation || !entry.preparation->flat ||
	    !entry.preparation->command || !entry.preparation->submission_started ||
	    !entry.completion_ready || entry.blocked || entry.native_receipt_verified ||
	    entry.native_acknowledged || entry.physical_published || entry.physical_stages ||
	    !same_original_refusal(entry.completed, publication.completion_) ||
	    (entry.flat_refusal_cleanup_started && !entry.flat_refusal_cleanup_returned))
		return false;
	try
	{
		auto &prepared = *entry.preparation;
		auto &flat = *prepared.flat;
		const auto &command = publication.command_;
		const auto &sealed = publication.completion_;
		if (!flat.player_held || !flat.native || !flat.reserved_bytes ||
		    flat.phase != shop_trade_flat_native_checkpoint_phase::ready ||
		    flat.native->operation_id.bytes != command.operation_id.bytes ||
		    !critical_command_equal(*prepared.command, command) ||
		    !publication.reservation_.matches_pid(static_cast<int>(entry.player_pid)) ||
		    flat.player_token.actor_runtime_id != prepared.actor_runtime_id ||
		    flat.selected_root != flat.native->player_hold_cut.selected_root)
			return false;
		std::vector<uint8_t> frozen, bound_bytes, retained_bytes;
		shop_trade_payload bound{};
		if (critical_command_encode(command, &frozen) !=
			    critical_command_codec_result::ok ||
		    frozen != publication.frozen_ ||
		    !shop_trade_command_decode_payload(command, &bound) ||
		    !bound.recovery_manifest_recorded ||
		    !shop_trade_command_encode_recovery_payload(bound, &bound_bytes) ||
		    !shop_trade_command_encode_recovery_payload(entry.payload, &retained_bytes) ||
		    bound_bytes != retained_bytes)
			return false;
		// This seals the exact refusal before any fallible observation or handler.
		// Later completion delivery cannot replace its timing, attempt or payload.
		entry.flat_refusal_verified = true;
		const auto session = [&]() noexcept
		{
			return !entry.blocked && !entry.never_admitted_retired &&
			       same_original_refusal(entry.completed, sealed) &&
			       critical_command_equal(*prepared.command, command) &&
			       publication.reservation_.matches_pid(
				       static_cast<int>(entry.player_pid)) &&
			       persistence_mode_get() == PERSISTENCE_MODE_FLATFILE_PRIMARY &&
			       persistence_mode_flatfile_root() &&
			       flat.selected_root == persistence_mode_flatfile_root();
		};
		if (!session())
			return false;
		flatfile_authority_lock authority;
		std::string error;
		if (!authority.acquire(flat.selected_root, &error) || !session())
			return false;
		flatfile_accounting_shop_projection current;
		if (flatfile_accounting_shop_transaction::read_never_admitted_before_locked(
			    flat.selected_root, authority, command, &current, &error) ||
		    !flat_publication_items(current.player.items,
					    flat.native->player.player.items) ||
		    !flat_publication_items(current.keeper.items, prepared.keeper_items) ||
		    !flat_publication_items(current.keeper.items,
					    flat.native->keeper_after.items) ||
		    current.keeper.cash != entry.payload.expected_keeper_cash)
			return false;
		// The completed source checkpoint AFTER is the refused trade's BEFORE.
		// Retain its literal forests; never reset/reissue that original source write.
		if (!entry.publication_forests_ready)
		{
			auto before = current.player.items, player = current.player.items;
			auto keeper = current.keeper.items;
			auto destination = prepared.destination;
			flat_preparation_bytes bytes;
			if (!bytes.items(before) || !bytes.items(player) || !bytes.items(keeper) ||
			    !bytes.vector(destination) || !bytes.value ||
			    !player_save_shop_checkpoint_owner::reserve_flat_publication_checkpoint(
				    flat.player_token, command.operation_id,
				    flat.native->player_hold_cut, publication, bytes.value))
				return false;
			entry.before_player = std::move(before);
			entry.after_player = std::move(player);
			entry.after_keeper = std::move(keeper);
			entry.after_destination = std::move(destination);
			entry.publication_forests_ready = true;
		}
		const auto retained_slot = [&]()
		{
			flat_preparation_bytes bytes;
			return session() &&
			       flat_publication_items(entry.before_player, current.player.items) &&
			       flat_publication_items(entry.after_player, current.player.items) &&
			       flat_publication_items(entry.after_keeper, current.keeper.items) &&
			       entry.after_destination == prepared.destination &&
			       bytes.items(entry.before_player) &&
			       bytes.items(entry.after_player) && bytes.items(entry.after_keeper) &&
			       bytes.vector(entry.after_destination) &&
			       player_save_shop_checkpoint_owner::reserve_flat_publication_checkpoint(
				       flat.player_token, command.operation_id,
				       flat.native->player_hold_cut, publication, bytes.value);
		};
		const auto same_cut = [&]()
		{
			flatfile_accounting_shop_projection fresh;
			return retained_slot() && authority.matches(flat.selected_root) &&
			       !flatfile_accounting_shop_transaction::
				       read_never_admitted_before_locked(flat.selected_root,
									 authority, command, &fresh,
									 &error) &&
			       flat_publication_same_current(current, fresh) && retained_slot();
		};
		std::vector<player_item_snapshot> produced_source;
		const bool produced = entry.payload.action == shop_trade_action::buy_produced;
		if (produced &&
		    (player_item_snapshot_list_decode(
			     entry.payload.item_blob.data(), entry.payload.item_blob_size,
			     &produced_source) != player_snapshot_codec_result::ok ||
		     produced_source.size() != entry.payload.item_count || produced_source.empty()))
			return false;
		shop_trade_world_cold_observation observed;
		// Every observation rebuilds the complete locations from authenticated
		// literal values, including full absence when an original body disappeared.
		// No stale pointer, PID lookup or VNUM/order substitution proves absence.
		const auto observe = [&]()
		{
			shop_trade_world_cold_observation initial;
			if (!shop_trade_world_cold_observe(
				    { &bound, entry.before_player, entry.after_keeper, {} },
				    &initial) ||
			    (initial.actor_present &&
			     initial.actor_runtime_id != prepared.actor_runtime_id) ||
			    (initial.keeper_present &&
			     initial.keeper_runtime_id != prepared.keeper_runtime_id))
				return false;
			std::map<uint64_t, shop_trade_world_uid_expectation> required;
			const auto append =
				[&](const auto &forest, bool npc, bool detached, bool absent)
			{
				for (size_t n = 0; n < forest.size(); ++n)
				{
					const auto &item = forest[n];
					if (!item.object_uid || item.parent_index < -1 ||
					    item.parent_index >= static_cast<int64_t>(n))
						return false;
					const uint64_t parent =
						item.parent_index < 0 ?
							0 :
							forest[item.parent_index].object_uid;
					const auto place =
						absent	 ? shop_trade_world_location::absent :
						parent	 ? shop_trade_world_location::inside :
						detached ? shop_trade_world_location::detached :
						npc	 ? (item.equipment_slot ?
								    shop_trade_world_location::
									    keeper_equipment :
								    shop_trade_world_location::
									    keeper_inventory) :
							   (item.equipment_slot ?
								    shop_trade_world_location::
									    actor_equipment :
								    shop_trade_world_location::
									    actor_inventory);
					if (!required.emplace(
							     item.object_uid,
							     shop_trade_world_uid_expectation{
								     item.object_uid, place,
								     absent ? 0 : parent,
								     static_cast<int16_t>(
									     absent ?
										     0 :
										     item.equipment_slot),
								     -1 })
						     .second ||
					    required.size() > 3 * PLAYER_SNAPSHOT_MAX_OBJECTS)
						return false;
				}
				return true;
			};
			if (!append(entry.before_player, false, false, !initial.actor_present) ||
			    !append(entry.after_keeper, true, false, !initial.keeper_present) ||
			    (produced && !append(produced_source, false, true,
						 entry.flat_refusal_cleanup_returned)))
				return false;
			std::vector<shop_trade_world_uid_expectation> locations;
			locations.reserve(required.size());
			for (const auto &[uid, location] : required)
				locations.push_back(location);
			if (!shop_trade_world_cold_observe(
				    { &bound,
				      initial.actor_present ?
					      std::span<const player_item_snapshot>(
						      entry.before_player) :
					      std::span<const player_item_snapshot>{},
				      initial.keeper_present ?
					      std::span<const player_item_snapshot>(
						      entry.after_keeper) :
					      std::span<const player_item_snapshot>{},
				      locations },
				    &observed) ||
			    observed.actor_runtime_id != initial.actor_runtime_id ||
			    observed.keeper_runtime_id != initial.keeper_runtime_id ||
			    !observed.uid_locations_current_match ||
			    (observed.actor_present &&
			     (!observed.player_current_match ||
			      static_cast<uint32_t>(GET_LEVEL(observed.actor)) !=
				      entry.payload.expected_player_level ||
			      !flat_publication_money(observed.actor, current.player_domains))) ||
			    (observed.keeper_present && !observed.keeper_current_match) ||
			    (produced && !entry.flat_refusal_cleanup_returned &&
			     !flat_publication_items(observed.selected_items, produced_source)) ||
			    (entry.payload.target_parent_item_uid && observed.actor_present &&
			     !observed.target_present))
				return false;
			if (entry.payload.target_parent_item_uid && observed.actor_present)
			{
				std::vector<uint8_t> target;
				if (player_item_snapshot_list_encode(observed.target_items,
								     &target) !=
					    player_snapshot_codec_result::ok ||
				    target != prepared.destination)
					return false;
			}
			return shop_trade_native_checkpoint_owner::
				       observe_flat_cold_publication_sources_locked(
					       flat.selected_root, authority, observed.actor,
					       observed.keeper, entry.payload.shop_id, current,
					       entry.before_player, entry.after_keeper,
					       entry.payload.expected_keeper_cash) &&
			       same_cut();
		};
		if (!observe() ||
		    !shop_trade_current_runtime_owner::publish_flat(flat.selected_root, authority,
								    command, sealed) ||
		    !observe())
			return false;
		if (produced && !entry.flat_refusal_cleanup_returned)
		{
			const auto selected = std::find_if(
				observed.objects.begin(), observed.objects.end(),
				[&](P_obj object) {
					return object &&
					       object->obj_uid == entry.payload.selected_item_uid;
				});
			if (entry.flat_refusal_cleanup_started ||
			    selected == observed.objects.end() || !OBJ_NOWHERE(*selected) ||
			    (*selected)->next_content || !retained_slot())
				return false;
			entry.flat_refusal_cleanup_started = true;
			extract_obj(*selected, FALSE);
			// Record genuine normal return immediately, before post-handler census.
			// Exceptions or unresolved tails keep the original owner started forever.
			entry.flat_refusal_cleanup_returned = true;
		}
		return observe() &&
		       shop_trade_current_runtime_owner::publish_flat(flat.selected_root, authority,
								      command, sealed) &&
		       observe() && retained_slot();
	}
	catch (...)
	{
		return false;
	}
}

bool shop_trade_native_publication_owner::native_publish_flat(
	player_save_restored_publication_owner &publication, void *opaque) noexcept
{
	if (!nevent_is_game_thread() || !publication.flat_shop_ || !opaque ||
	    !publication.reservation_.valid() || publication.acknowledged_)
		return false;
	if (publication.completion_.disposition == critical_completion_disposition::never_admitted)
		return native_publish_flat_refusal(publication, opaque);
	auto &entry = *static_cast<pending_trade *>(opaque);
	if (entry.cold || !entry.preparation || !entry.preparation->flat ||
	    !entry.preparation->command || !entry.preparation->submission_started ||
	    entry.blocked || !entry.accounted_publication ||
	    !same_native_receipt(entry.completed, publication.completion_))
		return false;
	try
	{
		auto &prepared = *entry.preparation;
		auto &flat = *prepared.flat;
		const auto &command = publication.command_;
		const auto &sealed = publication.completion_;
		if (!flat.player_held || !flat.native || !flat.reserved_bytes ||
		    flat.phase != shop_trade_flat_native_checkpoint_phase::ready ||
		    flat.native->operation_id.bytes != command.operation_id.bytes ||
		    command.operation_id.bytes != sealed.operation_id.bytes ||
		    !persistence_mode_flatfile_root() ||
		    flat.selected_root != persistence_mode_flatfile_root() ||
		    flat.selected_root != flat.native->player_hold_cut.selected_root)
			return false;
		const auto &native = *flat.native;
		std::vector<uint8_t> frozen, retained_command, payload_bytes, retained_payload;
		shop_trade_payload bound{};
		if (critical_command_encode(command, &frozen) !=
			    critical_command_codec_result::ok ||
		    frozen != publication.frozen_ ||
		    critical_command_encode(*prepared.command, &retained_command) !=
			    critical_command_codec_result::ok ||
		    retained_command != frozen ||
		    !shop_trade_command_decode_payload(command, &bound) ||
		    !bound.recovery_manifest_recorded ||
		    !shop_trade_command_encode_recovery_payload(bound, &payload_bytes) ||
		    !shop_trade_command_encode_recovery_payload(entry.payload, &retained_payload) ||
		    payload_bytes != retained_payload)
			return false;
		player_snapshot held_body;
		player_shop_checkpoint_stage held{};
		// The outer authentic reservation is borrowed, before the root lock.
		if (!player_save_shop_checkpoint_owner::original_held_body_flat(
			    flat.player_token, native.player_hold_cut, publication, &held_body,
			    &held) ||
		    held.save_revision != entry.payload.expected_player_save_revision ||
		    held.level != entry.payload.expected_player_level ||
		    !flat_publication_items(held_body.items, native.player.player.items) ||
		    !flat_publication_items(prepared.keeper_items, native.keeper_after.items))
			return false;
		flatfile_authority_lock authority;
		std::string error;
		if (!authority.acquire(flat.selected_root, &error))
			return false;
		flatfile_accounting_shop_projection current;
		if (flatfile_accounting_shop_transaction::read_current_locked(
			    flat.selected_root, authority, command, sealed, &current, &error))
			return false;
		entry.native_receipt_verified = true;
		const bool success = sealed.outcome == critical_apply_outcome::applied ||
				     sealed.outcome == critical_apply_outcome::already_applied;
		std::vector<player_item_snapshot> source;
		if (player_item_snapshot_list_decode(entry.payload.item_blob.data(),
						     entry.payload.item_blob_size,
						     &source) != player_snapshot_codec_result::ok ||
		    source.empty())
			return false;
		const bool produced = entry.payload.action == shop_trade_action::buy_produced;
		const bool buying = produced ||
				    entry.payload.action == shop_trade_action::buy_existing;
		const bool selling = entry.payload.action == shop_trade_action::sell_store ||
				     entry.payload.action == shop_trade_action::sell_destroy;
		const bool cleanup_action = entry.payload.action ==
					    shop_trade_action::discard_invalid;
		std::vector<player_item_snapshot> staged =
			produced ? source : std::vector<player_item_snapshot>{};
		shop_trade_world_witness observed;
		const auto source_supported = [&](const auto &player, const auto &keeper)
		{
			const int64_t cash = (entry.physical_stages & 128U) ?
						     current.keeper.cash :
						     entry.payload.expected_keeper_cash;
			return flat_publication_actor(observed.actor, observed.keeper, entry) &&
			       shop_trade_native_checkpoint_owner::
				       observe_flat_publication_sources_locked(
					       flat.selected_root, authority, observed.actor,
					       observed.keeper, entry.payload.shop_id, current,
					       player, keeper, cash) &&
			       (flat_publication_money(observed.actor,
						       native.player.native_money) ||
				flat_publication_money(observed.actor, current.player_domains));
		};
		if (!entry.publication_forests_ready)
		{
			if (!flat_publication_world(entry, held_body.items, prepared.keeper_items,
						    staged, false, &observed) ||
			    !source_supported(held_body.items, prepared.keeper_items))
				return false;
			P_obj selected =
				flat_publication_object(observed, entry.payload.selected_item_uid);
			P_obj destination = flat_publication_object(
				observed, entry.payload.target_parent_item_uid);
			std::vector<uint8_t> literal, after_destination;
			if (destination &&
			    (!flat_preparation_destination_tree(destination, &literal) ||
			     literal != prepared.destination))
				return false;
			std::vector<player_item_snapshot> player, keeper;
			if (!selected ||
			    !expected_player_forest(entry.payload, held_body.items, !success,
						    &player) ||
			    !expected_keeper_forest(entry.payload, observed.keeper, selected,
						    prepared.keeper_items, !success, &keeper))
				return false;
			if (success && buying)
			{
				std::vector<player_item_snapshot> ordered;
				if (!expected_player_order(entry.payload, observed.actor, selected,
							   destination, player, &ordered))
					return false;
				player = std::move(ordered);
			}
			if (!flat_publication_items(player, current.player.items) ||
			    !flat_publication_items(keeper, current.keeper.items))
				return false;
			if (success && destination)
			{
				std::vector<player_item_snapshot> after;
				if (!expected_destination_forest(entry.payload, observed.actor,
								 selected, destination,
								 prepared.destination, &after) ||
				    player_item_snapshot_list_encode(after, &after_destination) !=
					    player_snapshot_codec_result::ok)
					return false;
			}
			flat_preparation_bytes bytes;
			if (!bytes.items(held_body.items) || !bytes.items(player) ||
			    !bytes.items(keeper) || !bytes.vector(after_destination) ||
			    !bytes.value ||
			    !player_save_shop_checkpoint_owner::reserve_flat_publication_checkpoint(
				    flat.player_token, command.operation_id, native.player_hold_cut,
				    publication, bytes.value))
				return false;
			// All allocations and the exact separate charge precede these nonthrowing moves.
			static_assert(
				std::is_nothrow_move_assignable_v<std::vector<player_item_snapshot>>);
			static_assert(std::is_nothrow_move_assignable_v<std::vector<uint8_t>>);
			entry.before_player = std::move(held_body.items);
			entry.after_player = std::move(player);
			entry.after_keeper = std::move(keeper);
			entry.after_destination = std::move(after_destination);
			entry.publication_forests_ready = true;
		}
		else
		{
			flat_preparation_bytes bytes;
			if (!flat_publication_items(entry.after_player, current.player.items) ||
			    !flat_publication_items(entry.after_keeper, current.keeper.items) ||
			    !bytes.items(entry.before_player) || !bytes.items(entry.after_player) ||
			    !bytes.items(entry.after_keeper) ||
			    !bytes.vector(entry.after_destination) ||
			    !player_save_shop_checkpoint_owner::reserve_flat_publication_checkpoint(
				    flat.player_token, command.operation_id, native.player_hold_cut,
				    publication, bytes.value))
				return false;
		}
		const auto destination_matches = [&](bool after)
		{
			if (!entry.payload.target_parent_item_uid)
				return true;
			P_obj destination = flat_publication_object(
				observed, entry.payload.target_parent_item_uid);
			std::vector<uint8_t> literal;
			const auto &expected = after ? entry.after_destination :
						       prepared.destination;
			return destination && !expected.empty() &&
			       flat_preparation_destination_tree(destination, &literal) &&
			       literal == expected;
		};
		const auto observe_final = [&]()
		{
			return flat_publication_world(entry, entry.after_player, entry.after_keeper,
						      (!success && produced) ?
							      staged :
							      std::vector<player_item_snapshot>{},
						      true, &observed) &&
			       destination_matches(success) &&
			       source_supported(entry.after_player, entry.after_keeper);
		};
		if (entry.physical_published || (entry.physical_stages & 32U) ||
		    (success && (entry.physical_stages & 4096U) &&
		     (entry.physical_stages & 131072U)))
		{
			if (!observe_final())
				return false;
		}
		else if (!flat_publication_world(entry, entry.before_player, prepared.keeper_items,
						 staged, false, &observed))
		{
			// A normally returned departure can resume placement. Incomplete
			// started handler tails remain held by the physical callback's
			// explicit stage checks; topology cannot discharge them.
			std::vector<player_item_snapshot> player = entry.before_player;
			std::vector<player_item_snapshot> keeper = prepared.keeper_items;
			if (selling && !shop_remove_selected(player, entry.payload, &player))
				return false;
			if ((buying && !produced) || cleanup_action)
				if (!shop_remove_selected(keeper, entry.payload, &keeper))
					return false;
			auto detached = source;
			if (buying && (entry.physical_stages & 65536U))
				detached.front().extra2_flags |= ITEM2_STOREITEM;
			if (buying && (entry.physical_stages & 1024U) &&
			    !shop_trade_accounted_after_items(entry.payload, &detached))
				return false;
			bool resumed = (entry.physical_stages & 65536U) &&
				       flat_publication_world(entry, player, keeper, detached,
							      false, &observed);
			if (!resumed && buying && entry.payload.target_parent_item_uid &&
			    (entry.physical_stages & 1024U))
			{
				// obj_to_char returned before the existing nesting leg. Its
				// temporary carried-root placement has the same native grouping
				// policy; never pretend it is the final container placement.
				// The insertion observer consumes only these placement selectors.
				// This value is never encoded, frozen, compiled or submitted.
				shop_trade_payload carried_selection{};
				carried_selection.player_pid = entry.payload.player_pid;
				carried_selection.action = entry.payload.action;
				carried_selection.selected_item_uid =
					entry.payload.selected_item_uid;
				std::vector<player_item_snapshot> values, ordered;
				P_char actor =
					find_character_by_runtime_id(prepared.actor_runtime_id);
				P_obj selected = nullptr;
				// Bounded temporary carrying observation resolves the pointer;
				// the subsequent full world census must still prove uniqueness.
				if (flat_publication_carried_values(entry.payload,
								    entry.before_player, &values))
				{
					// The ordering helper checks the complete target chain;
					// the following full census must prove every alias surface.
					std::set<P_obj> seen;
					for (P_obj object = actor ? actor->carrying : nullptr;
					     object; object = object->next_content)
					{
						if (seen.size() >= PLAYER_SNAPSHOT_MAX_OBJECTS ||
						    !seen.insert(object).second ||
						    !OBJ_CARRIED_BY(object, actor))
							return false;
						if (object->obj_uid ==
						    entry.payload.selected_item_uid)
						{
							selected = object;
							break;
						}
					}
					if (selected && expected_player_order(
								carried_selection, actor, selected,
								nullptr, values, &ordered))
						resumed = flat_publication_world(entry, ordered,
										 keeper, {}, false,
										 &observed);
				}
			}
			if (!resumed)
				return false;
		}
		// Before nesting, bind every original literal child. Normally
		// returned nesting instead requires the retained complete afterimage.
		if (!destination_matches(success && (entry.physical_stages & 4096U) &&
					 (entry.physical_stages & 131072U)))
			return false;

		if (!source_supported(observed.player_items, observed.keeper_items) ||
		    entry.blocked || !same_native_receipt(entry.completed, sealed) ||
		    !publication.reservation_.valid())
			return false;
		shop_trade_result projected{};
		if (!shop_trade_command_decode_result(sealed.result_payload.data(),
						      sealed.result_size, &projected))
			return false;
		const auto &money = current.player_domains.domains;
		for (size_t index = 0; index < projected.wallet.amount.size(); ++index)
		{
			if (money.wallet[index] > INT_MAX || money.bank[index] > INT_MAX)
				return false;
			projected.wallet.amount[index] = static_cast<int64_t>(money.wallet[index]);
			projected.bank.amount[index] = static_cast<int64_t>(money.bank[index]);
		}
		projected.wallet_revision = money.wallet_revision;
		projected.bank_revision = money.bank_revision;
		projected.keeper_cash = current.keeper.cash;
		projected.keeper_cash_recorded = true;
		projected.shop_revision = current.keeper.revision;
		projected.player_owner_revision = current.primary_owner_revision;
		projected.counterparty_owner_revision = current.counterparty_owner_revision;
		// Actual current AFTER custody and the complete foreign union precede placement.
		if (!shop_trade_current_runtime_owner::publish_flat(flat.selected_root, authority,
								    command, sealed))
			return false;
		if (success && !entry.physical_published)
		{
			const bool returned = entry.accounted_publication(observed.actor,
									  observed.keeper,
									  projected, entry.payload,
									  entry.physical_stages);
			if (returned)
				entry.physical_published = true;
			if (!returned)
				return false;
		}
		// Refusal never invokes the physical callback. Success retries preserve its
		// returned once stages; unresolved started tails remain held by that callback.
		if (entry.blocked || !same_native_receipt(entry.completed, sealed) ||
		    !observe_final())
			return false;
		flatfile_accounting_shop_projection rechecked;
		if (flatfile_accounting_shop_transaction::read_current_locked(
			    flat.selected_root, authority, command, sealed, &rechecked, &error) ||
		    !flat_publication_same_current(current, rechecked) ||
		    !currency_transaction_publish_balances(
			    observed.actor, entry.payload.account_name.data(),
			    entry.payload.racewar, projected.wallet, projected.bank,
			    projected.wallet_revision, projected.bank_revision) ||
		    entry.blocked || !same_native_receipt(entry.completed, sealed) ||
		    !observe_final() ||
		    !flat_publication_money(observed.actor, current.player_domains))
			return false;
		// The original callback owns keeper cash; never restore it from a historical
		// receipt or fabricate SQL native row IDs in the flat physical graph.
		if (!shop_trade_current_runtime_owner::publish_flat(flat.selected_root, authority,
								    command, sealed) ||
		    !observe_final() || !publication.reservation_.valid() ||
		    !same_native_receipt(entry.completed, sealed))
			return false;
		return flat_publication_money(observed.actor, current.player_domains);
	}
	catch (...)
	{
		return false;
	}
}

namespace
{
struct shop_replay_bytes
{
	size_t value = 0;
	bool add(size_t amount) noexcept
	{
		if (amount > SIZE_MAX - value)
			return false;
		value += amount;
		return true;
	}
	template <class T> bool vector(const std::vector<T> &v) noexcept
	{
		return v.capacity() <= SIZE_MAX / sizeof(T) && add(v.capacity() * sizeof(T));
	}
	template <class K, class V> bool map_nodes(const std::map<K, V> &v) noexcept
	{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
		using node = std::_Rb_tree_node<typename std::map<K, V>::value_type>;
		return v.size() <= SIZE_MAX / sizeof(node) && add(v.size() * sizeof(node));
#else
		(void)v;
		return false;
#endif
	}
	bool string(const std::string &s) noexcept
	{
		const auto address = reinterpret_cast<uintptr_t>(s.data());
		const auto object = reinterpret_cast<uintptr_t>(&s);
		return (address >= object && address - object < sizeof(s)) ||
		       (s.capacity() != SIZE_MAX && add(s.capacity() + 1));
	}
	bool item(const player_item_snapshot &row) noexcept
	{
		size_t heap = 0;
		return player_item_snapshot_current_heap_bytes(row, &heap) && add(heap);
	}
	bool items(const std::vector<player_item_snapshot> &rows) noexcept
	{
		if (!vector(rows))
			return false;
		for (const auto &row : rows)
			if (!item(row))
				return false;
		return true;
	}
	bool snapshot(const player_snapshot &row) noexcept
	{
		size_t heap = 0;
		return player_snapshot_current_heap_bytes(row, &heap) && add(heap);
	}
	bool command(const critical_command &row) noexcept
	{
		size_t heap = 0;
		return critical_command_current_heap_bytes(row, &heap) && add(heap);
	}
	bool payload(const shop_trade_payload &row) noexcept
	{
		size_t heap = 0;
		return shop_trade_accounting_payload_current_heap_bytes(row, &heap) && add(heap);
	}
	bool custody(const std::vector<flatfile_item_ownership_record> &rows) noexcept
	{
		if (!vector(rows))
			return false;
		for (const auto &row : rows)
			if (!vector(row.coin_payload))
				return false;
		return true;
	}
	bool keeper(const flatfile_shopkeeper_record &row) noexcept
	{
		return vector(row.affects) && items(row.items);
	}
	bool flat_native(const shop_trade_flat_native_checkpoint_stage &stage) noexcept
	{
		if (!string(stage.player_hold_cut.selected_root) ||
		    !snapshot(stage.original_queued_ack) ||
		    !vector(stage.player.authority.mappings) ||
		    !string(stage.player.native_money.account_name) ||
		    !vector(stage.player.native_money.recent_pvp_deaths) ||
		    !vector(stage.player.native_money.completed_epic_zones) ||
		    !snapshot(stage.player.player) || !custody(stage.player.player_custody) ||
		    !vector(stage.player.pet_custody) || !custody(stage.keeper_custody) ||
		    !keeper(stage.keeper_before) || !keeper(stage.keeper_after) ||
		    !string(stage.catalog_before.filename) || !vector(stage.catalog_before.bytes) ||
		    !string(stage.catalog_after.filename) || !vector(stage.catalog_after.bytes))
			return false;
		for (const auto &mapping : stage.player.authority.mappings)
			if (!string(mapping.locator.name))
				return false;
		for (const auto &pet : stage.player.pet_custody)
			if (!custody(pet.custody))
				return false;
		return true;
	}
};
} // namespace

bool shop_trade_native_publication_owner::passive_current_storage_bytes(size_t *output) noexcept
{
	if (!output || !nevent_is_game_thread())
		return false;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI || defined(_GLIBCXX_DEBUG)
	return false;
#else
	shop_replay_bytes bytes;
	if (!bytes.add(sizeof(pending)) || !bytes.add(sizeof(notifying)) ||
	    !bytes.add(sizeof(preparation_generation)) || !bytes.map_nodes(pending))
		return false;
	for (const auto &[id, entry] : pending)
	{
		if (!bytes.payload(entry.payload) || !bytes.items(entry.before_player) ||
		    !bytes.items(entry.after_player) || !bytes.items(entry.after_keeper) ||
		    !bytes.vector(entry.after_destination))
			return false;
		if (entry.preparation)
		{
			const auto &p = *entry.preparation;
			if (!bytes.add(sizeof(p)) || !bytes.vector(p.selected) ||
			    !bytes.vector(p.stock) || !bytes.vector(p.destination) ||
			    !bytes.vector(p.keeper_blob) || !bytes.items(p.keeper_items) ||
			    !bytes.vector(p.fenced_uids))
				return false;
			if (p.command &&
			    (!bytes.add(sizeof(*p.command)) || !bytes.command(*p.command)))
				return false;
			if (p.flat)
			{
				if (!bytes.add(sizeof(*p.flat)) ||
				    !bytes.string(p.flat->selected_root))
					return false;
				if (p.flat->native && (!bytes.add(sizeof(*p.flat->native)) ||
						       !bytes.flat_native(*p.flat->native)))
					return false;
			}
#ifndef __NO_MYSQL__
			if (!bytes.map_nodes(p.native_stage.keeper_image))
				return false;
			for (const auto &[key, row] : p.native_stage.keeper_image)
				if (!bytes.item(row.item))
					return false;
#endif
		}
		if (!entry.cold)
			continue;
		const auto &c = *entry.cold;
		if (!bytes.add(sizeof(c)) || !bytes.vector(c.fenced_uids))
			return false;
		if (c.command && (!bytes.add(sizeof(*c.command)) || !bytes.command(*c.command)))
			return false;
		for (const auto *forest :
		     { &c.original_player, &c.original_keeper, &c.working_player, &c.working_keeper,
		       &c.source, &c.selected_after, &c.working_target })
			if (!bytes.items(*forest))
				return false;
		if (c.flat_working)
		{
			if (!bytes.add(sizeof(*c.flat_working)))
				return false;
			for (const auto &forest : c.flat_working->player)
				if (!bytes.items(forest))
					return false;
			for (const auto &forest : c.flat_working->keeper)
				if (!bytes.items(forest))
					return false;
			for (const auto &forest : c.flat_working->target)
				if (!bytes.items(forest))
					return false;
		}
		if (c.flat_literals)
		{
			const auto &s = *c.flat_literals;
			const size_t binding = s.bindings.retained_bytes();
			if (binding < sizeof(s.bindings) || !bytes.add(sizeof(s)) ||
			    !bytes.add(binding - sizeof(s.bindings)) || !bytes.vector(s.staged) ||
			    !bytes.vector(s.reload) || !bytes.vector(s.enrollment_counts))
				return false;
			// Unconsumed private stages retain the source row in exact index
			// order. A transferred object has a null stage pointer; ROOT G owns
			// its strings/descriptors thereafter. No saved-byte cache is used.
			for (size_t i = 0; i < s.staged.size(); ++i)
			{
				size_t heap = 0;
				if (s.staged[i].object_ &&
				    (i >= c.source.size() ||
				     !s.staged[i].current_private_heap_bytes(c.source[i], &heap)))
					return false;
				if (!bytes.add(heap))
					return false;
			}
			for (const auto &reload : s.reload)
				if (!bytes.vector(reload.proclib_probes))
					return false;
		}
#ifndef __NO_MYSQL__
		const size_t binding = c.bindings.retained_bytes();
		if (binding < sizeof(c.bindings) || !bytes.add(binding - sizeof(c.bindings)) ||
		    !bytes.vector(c.staged) || !bytes.vector(c.reload) ||
		    !bytes.map_nodes(c.enrollment_counts))
			return false;
		for (size_t i = 0; i < c.staged.size(); ++i)
		{
			size_t heap = 0;
			if (c.staged[i].object_ &&
			    (i >= c.source.size() ||
			     !c.staged[i].current_private_heap_bytes(c.source[i], &heap)))
				return false;
			if (!bytes.add(heap))
				return false;
		}
		for (const auto &reload : c.reload)
			if (!bytes.vector(reload.proclib_probes))
				return false;
#endif
	}
	*output = bytes.value;
	return true;
#endif
}
bool shop_trade_transaction_replay_current_storage_bytes(size_t *output) noexcept
{
	return shop_trade_native_publication_owner::passive_current_storage_bytes(output);
}

size_t shop_trade_native_publication_owner::passive_storage_observer_frame_bytes() noexcept
{
	// Source-declared live visitor references, typed container range iterators,
	// nested collector/source-row scan/checked-add and borrowed leaf observers.
	// Literal/forest inline objects stay in the retained nodes and are not
	// multiplied into the observation stack. Emitted/libc qualification is open.
	return sizeof(shop_replay_bytes) + 24 * sizeof(void *) + 12 * sizeof(size_t) +
	       8 * sizeof(bool) + 2 * sizeof(decltype(pending)::const_iterator) +
	       7 * sizeof(const std::vector<player_item_snapshot> *) +
	       sizeof(std::initializer_list<const std::vector<player_item_snapshot> *>) +
	       6 * sizeof(std::vector<player_item_snapshot>::const_iterator) +
	       shop_trade_accounting_decoded_heap_observer_frame_bytes() +
	       shop_trade_original_item_stage::current_private_heap_observer_frame_bytes() +
	       player_item_snapshot_copy_frame_bytes();
}
size_t shop_trade_transaction_replay_storage_observer_frame_bytes() noexcept
{
	return shop_trade_native_publication_owner::passive_storage_observer_frame_bytes();
}

namespace
{
size_t shop_replay_fixed_source_frames() noexcept
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
	using iterator = decltype(pending)::iterator;
	using const_iterator = decltype(pending)::const_iterator;
	using node = std::_Rb_tree_node<decltype(pending)::value_type>;
	using tree_pointer = std::_Rb_tree_node_base *;
	// Actual public restore arguments/flat/result, all retained function
	// locals and range-array temporaries. Work/entry/command inline objects
	// themselves are charged through sizeof(shop_replay_work), not this list.
	constexpr size_t caller =
		5 * sizeof(void *) + 2 * sizeof(size_t) + 2 * sizeof(bool) + 2 * sizeof(size_t) +
		sizeof(iterator) + 2 * sizeof(void *) + sizeof(size_t) +
		6 * sizeof(const shop_trade_recovery_forest_binding *) +
		sizeof(std::initializer_list<const shop_trade_recovery_forest_binding *>) +
		2 * sizeof(const shop_trade_recovery_forest_binding **) +
		sizeof(std::pair<iterator, bool>) + sizeof(decltype(pending)::insert_return_type) +
		sizeof(shop_replay_bytes) + sizeof(size_t);
	// prefix/peak/forward/append/add/vector and their current/getter calls.
	constexpr size_t budget = 19 * sizeof(void *) + 15 * sizeof(size_t) + 7 * sizeof(bool) +
				  sizeof(shop_replay_bytes) + 2 * sizeof(iterator) +
				  sizeof(const pending_trade *);
	// Original player_pending/keeper_busy: any_of -> none_of -> find_if ->
	// __find_if(input-iterator). Source loops are iterative; their PID/shop
	// captures, iterators/parameters/returned iterator/predicate adapter and
	// original lambda row references coexist once, independent of map size.
	constexpr size_t predicates = 4 * sizeof(uint32_t) + 11 * sizeof(iterator) +
				      8 * sizeof(void *) + 7 * sizeof(bool);
	// map::find -> tree::find -> _M_lower_bound and real _S_key/_S_left/
	// _S_right/iterator dereference/end paths. The genuine array<uint8_t,16>
	// std::less expression uses C++20 array::operator<=> runtime memcmp and
	// strong_ordering < 0, not a synthesized lexicographical comparator.
	constexpr size_t lookup = 12 * sizeof(void *) + 3 * sizeof(iterator) +
				  2 * sizeof(const_iterator) + 2 * sizeof(tree_pointer) +
				  4 * sizeof(bool);
	constexpr size_t array_key_comparison =
		3 * sizeof(void *) + sizeof(bool) + 2 * sizeof(void *) +
		sizeof(std::strong_ordering) + sizeof(size_t) + 4 * (2 * sizeof(void *)) +
		3 * sizeof(void *) + sizeof(size_t) + sizeof(int) + sizeof(std::strong_ordering) +
		sizeof(std::__cmp_cat::__unspec) + 3 * sizeof(bool);
	// GCC13 map::emplace has an exact two-argument/default-allocator usable-key
	// fast path (stl_map.h588): argument-reference pair, key, lower_bound and
	// hint, then _M_emplace_hint_unique's real _Auto_node (tree ref/node ptr),
	// hint/key/result position pair. _M_create_node/_M_construct_node and
	// allocator_traits/new_allocator/construct_at/pair forward that same pair.
	constexpr size_t node_create =
		sizeof(std::pair<const std::array<uint8_t, CRITICAL_COMMAND_ID_BYTES> &,
				 pending_trade &>) +
		17 * sizeof(void *) + 3 * sizeof(iterator) + 2 * sizeof(const_iterator) +
		2 * sizeof(tree_pointer) + 3 * sizeof(std::pair<tree_pointer, tree_pointer>) +
		2 * sizeof(std::allocator<node>) + 3 * sizeof(size_t) + 5 * sizeof(bool);
	// Real map node extract/reinsert: original node_handle's pointer/optional
	// allocator belongs to work.node, but transient result/allocator/reference
	// move carriers, _M_get_insert_unique_pos and _M_insert_node remain live.
	// Single-node staged map is the only private tree ever destroyed here;
	// its _M_erase call has depth one. Persistent pending is never cleared.
	constexpr size_t node_transfer =
		sizeof(decltype(pending)::insert_return_type) + 13 * sizeof(void *) +
		4 * sizeof(iterator) + 2 * sizeof(const_iterator) +
		2 * sizeof(std::pair<tree_pointer, tree_pointer>) + 3 * sizeof(bool) +
		sizeof(std::allocator<node>) + 2 * sizeof(size_t);
	// Fresh uint64 vector copy/range-insert/push_back: actual old/new/begin/end,
	// position, n, elements_after, old_finish, mid and _M_check_len max/len.
	// Allocator_traits/new_allocator/_Construct and uninitialized copy/relocate
	// are real type-specific, iterative source scopes; old heap remains in
	// CURRENT and new heap is a prospective request, outside these carriers.
	constexpr size_t vector_copy = 17 * sizeof(void *) + 12 * sizeof(size_t) +
				       4 * sizeof(std::vector<uint64_t>::iterator) +
				       sizeof(uint64_t) + 5 * sizeof(bool) +
				       2 * sizeof(std::allocator<uint64_t>);
	// Default workspace/cold constructors and cleanup execute the actual
	// default vector -> _Vector_base -> _Vector_impl -> _Vector_impl_data
	// /allocator chain, map -> _Rb_tree/_Rb_tree_impl/_Rb_tree_header and node
	// handle construction. These alternatives are admitted together as a
	// finite inventory, without claiming compiler-emitted stack measurement.
	constexpr size_t construction = 16 * sizeof(void *) + 2 * sizeof(std::_Rb_tree_header) +
					3 * sizeof(std::allocator<uint64_t>) +
					sizeof(std::allocator<node>);
	// unique_ptr construction/reset/destroy + command/payload vector move
	// assignment/destruction: real _M_move_assign temporary, allocator and
	// _Vector_impl_data swap holders. Only one nested vector chain is active;
	// six manifest members are moved/destroyed sequentially, not six heaps.
	constexpr size_t cleanup = 18 * sizeof(void *) + 2 * sizeof(std::vector<uint64_t>) +
				   sizeof(std::vector<uint8_t>) +
				   4 * sizeof(std::allocator<uint64_t>) + 4 * sizeof(size_t) +
				   3 * sizeof(bool);
	// Duplicate identity uses the original TWO encodes and byte-vector ==.
	// Genuine encoder/profile/envelope arguments/results, wire_bytes/status,
	// key/revision range iterators and pad loops, append_le<uint64_t>'s value
	// and byte loop, range-insert/push-back/allocator and typed copy/move paths.
	// The encoder's private result vector and actual fresh heap request are
	// admitted by critical_command_encode_bounded, not duplicated here.
	constexpr size_t duplicate_encode =
		// equal_commands this/left/right/candidate ref/nested/equal and return.
		4 * sizeof(void *) + sizeof(size_t) + 2 * sizeof(bool) + 14 * sizeof(void *) +
		9 * sizeof(size_t) + 2 * sizeof(critical_command_codec_result) + sizeof(uint64_t) +
		2 * sizeof(unsigned int) + 4 * sizeof(bool) +
		2 * sizeof(std::vector<critical_entity_key>::const_iterator) +
		2 * sizeof(std::vector<critical_expected_revision>::const_iterator) +
		6 * sizeof(std::vector<uint8_t>::iterator) +
		8 * sizeof(std::vector<uint8_t>::const_iterator) +
		// GCC13 vector equality -> equal -> __equal_aux/aux1 -> raw-byte
		// __equal<true>/__memcmp: actual refs/iterators/n/result; no recursion.
		20 * sizeof(void *) + 4 * sizeof(std::ptrdiff_t) + 6 * sizeof(bool) + sizeof(int) +
		// uint8 vector range insert/push_back and relocation/copy/deallocation.
		19 * sizeof(void *) + 11 * sizeof(size_t) + 2 * sizeof(std::allocator<uint8_t>);
	return caller + budget + predicates + lookup + array_key_comparison + node_create +
	       node_transfer + vector_copy + construction + cleanup + duplicate_encode +
	       critical_command_valid_frame_bytes() + critical_command_copy_frame_bytes() +
	       // ROOT owns the complete SAME-scope observer closure once. This
	       // only charges the actual borrowed SHOP wrapper's source carriers.
	       player_save_shop_replay_owner::current_observer_frame_bytes() +
	       // Pure sort-profile observer params/logarithm/n/depth/leaf and the
	       // actual three-element max initializer-list/returned reference.
	       2 * sizeof(void *) + 8 * sizeof(size_t) + sizeof(std::initializer_list<size_t>) +
	       sizeof(bool);
#else
	return 0;
#endif
}

bool shop_replay_uid_sort_source_frames(size_t count, size_t *output) noexcept
{
	if (!output)
		return false;
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
	using iterator = std::vector<uint64_t>::iterator;
	using compare = __gnu_cxx::__ops::_Iter_less_iter;
	using value_compare = __gnu_cxx::__ops::_Val_less_iter;
	using iter_value_compare = __gnu_cxx::__ops::_Iter_less_val;
	using difference = std::vector<uint64_t>::difference_type;
	// Authenticated GCC13 complete sort inventory, with these actual default
	// uint64 iterator/value adapters (not scheduler function-pointer wrappers).
	constexpr size_t setup = 4 * sizeof(iterator) + 2 * sizeof(compare) + 2 * sizeof(void *) +
				 3 * sizeof(difference) + sizeof(int) +
				 8 * (sizeof(void *) + sizeof(iterator)) + 4 * sizeof(bool);
	constexpr size_t recursive = 3 * sizeof(iterator) + sizeof(difference) + sizeof(compare);
	constexpr size_t partition = 12 * sizeof(iterator) + 3 * sizeof(compare) +
				     2 * sizeof(bool) + 7 * sizeof(void *) + sizeof(uint64_t);
	constexpr size_t insertion = 10 * sizeof(iterator) + 2 * sizeof(compare) +
				     2 * sizeof(value_compare) + sizeof(iter_value_compare) +
				     3 * sizeof(uint64_t) + 2 * sizeof(difference) +
				     12 * sizeof(void *) + 3 * sizeof(bool);
	constexpr size_t heap = 12 * sizeof(iterator) + 14 * sizeof(difference) +
				5 * sizeof(compare) + 2 * sizeof(value_compare) +
				2 * sizeof(iter_value_compare) + 4 * sizeof(uint64_t) +
				18 * sizeof(void *) + 3 * sizeof(bool);
	constexpr size_t comparator = 2 * sizeof(iterator) + 2 * sizeof(void *) + 3 * sizeof(bool);
	// Full default unique -> adjacent_find -> __adjacent_find and __unique,
	// Iter_equal_to_iter plus result/next and original vector erase-at-end /
	// typed destroy/move iterator closures. These loops never recurse.
	constexpr size_t unique_erase = 15 * sizeof(iterator) +
					4 * sizeof(__gnu_cxx::__ops::_Iter_equal_to_iter) +
					12 * sizeof(void *) + 3 * sizeof(difference) +
					4 * sizeof(size_t) + 5 * sizeof(bool);
	size_t logarithm = 0;
	for (size_t n = count; n > 1; n >>= 1)
		++logarithm;
	// Actual __sort initializes depth to 2*__lg(n); each recursive right
	// partition strictly shrinks and the left partition reuses the frame.
	const size_t depth = count <= 16 ? 1 : std::min(logarithm * 2 + 1, count - 16 + 1);
	const size_t leaf = std::max({ partition, insertion, heap });
	if (depth > (SIZE_MAX - setup - leaf - comparator) / recursive)
		return false;
	*output = std::max(setup + depth * recursive + leaf + comparator, unique_erase);
	return true;
#else
	(void)count;
	return false;
#endif
}

struct shop_replay_work
{
	economic_frozen_intent intent;
	shop_trade_payload payload{};
	economic_account_key wallet{}, bank{}, keeper{};
	pending_trade entry;
	decltype(pending) staged;
	decltype(pending)::node_type node;
};
struct shop_replay_duplicate_work
{
	std::vector<uint8_t> left, right;
};
struct shop_replay_budget
{
	player_save_coin_replay_budget_scope_owner &pipeline;
	bool (*reserve)(size_t, void *) noexcept;
	void *context;
	size_t outer;
	shop_replay_work *work = nullptr;
	const shop_replay_duplicate_work *duplicate = nullptr;
	bool denied = false;
	static bool forward(size_t amount, void *opaque) noexcept
	{
		auto &b = *static_cast<shop_replay_budget *>(opaque);
		if (b.denied || !b.reserve || !b.reserve(amount, b.context))
		{
			b.denied = true;
			return false;
		}
		return true;
	}
	bool prefix(size_t &out, bool include_pipeline = true, size_t extra = 0) noexcept
	{
		shop_replay_bytes bytes;
		size_t current = 0;
		if (!bytes.add(outer) || !bytes.add(sizeof(*this)) ||
		    !bytes.add(sizeof(shop_replay_work)) ||
		    !bytes.add(shop_replay_fixed_source_frames()) ||
		    !bytes.add(shop_trade_transaction_replay_storage_observer_frame_bytes()) ||
		    !shop_trade_transaction_replay_current_storage_bytes(&current) ||
		    !bytes.add(current))
			return false;
		if (include_pipeline &&
		    (!player_save_shop_replay_owner::current_storage_bytes(pipeline, &current) ||
		     !bytes.add(current)))
			return false;
		if (work)
		{
			if (!bytes.add(work->intent.admission.facts.capacity()) ||
			    !bytes.payload(work->payload) || !bytes.payload(work->entry.payload))
				return false;
			if (work->entry.cold && (!bytes.add(sizeof(*work->entry.cold)) ||
						 !bytes.vector(work->entry.cold->fenced_uids) ||
						 (work->entry.cold->command &&
						  (!bytes.add(sizeof(*work->entry.cold->command)) ||
						   !bytes.command(*work->entry.cold->command)))))
				return false;
			const pending_trade *private_entry = nullptr;
			if (!work->staged.empty())
			{
				if (!bytes.map_nodes(work->staged))
					return false;
				private_entry = &work->staged.begin()->second;
			}
			if (!work->node.empty())
			{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13
				if (!bytes.add(sizeof(
					    std::_Rb_tree_node<decltype(pending)::value_type>)))
					return false;
				private_entry = &work->node.mapped();
#else
				return false;
#endif
			}
			if (private_entry &&
			    (!bytes.payload(private_entry->payload) || !private_entry->cold ||
			     !bytes.add(sizeof(*private_entry->cold)) ||
			     !bytes.vector(private_entry->cold->fenced_uids) ||
			     !private_entry->cold->command ||
			     !bytes.add(sizeof(*private_entry->cold->command)) ||
			     !bytes.command(*private_entry->cold->command)))
				return false;
		}
		if (duplicate &&
		    (!bytes.add(sizeof(*duplicate)) || !bytes.vector(duplicate->left) ||
		     !bytes.vector(duplicate->right)))
			return false;
		if (!bytes.add(extra))
			return false;
		out = bytes.value;
		return true;
	}
	bool peak(size_t extra = 0) noexcept
	{
		size_t current = 0;
		if (!prefix(current, true, extra))
		{
			denied = true;
			return false;
		}
		return forward(current, this);
	}
	bool append(std::vector<uint64_t> &v, const uint64_t *first, size_t count)
	{
		if (!count)
			return peak();
		if (count > v.max_size() - v.size())
			return false;
		const size_t desired = v.size() + count;
		size_t request = 0;
		if (desired > v.capacity())
		{
			const size_t growth = std::max(v.size(), count);
			if (growth > v.max_size() - v.size())
				return false;
			const size_t capacity = v.size() + growth;
			if (capacity > SIZE_MAX / sizeof(uint64_t))
				return false;
			request = capacity * sizeof(uint64_t);
		}
		// Original range insert retains old capacity while the actual GCC13
		// _M_check_len(size+max(size,n)) new allocation is live.
		if (!peak(request))
			return false;
		v.insert(v.end(), first, first + count);
		return true;
	}
	bool append_one(std::vector<uint64_t> &v, uint64_t uid)
	{
		size_t request = 0;
		if (v.size() == v.capacity())
		{
			const size_t grow = std::max(v.size(), size_t{ 1 });
			if (grow > v.max_size() - v.size() ||
			    v.size() + grow > SIZE_MAX / sizeof(uint64_t))
				return false;
			request = (v.size() + grow) * sizeof(uint64_t);
		}
		if (!peak(request))
			return false;
		v.push_back(uid);
		return true;
	}
	bool equal_commands(const critical_command &left, const critical_command &right) noexcept
	{
		if (!work || !peak(sizeof(shop_replay_duplicate_work)))
			return false;
		// Keep the complete original critical_command_equal algorithm and
		// failure law: left encode, right encode, then exact wire equality.
		// Both outputs' actual capacities coexist through the right encoder,
		// then original local cleanup finishes before the authentic hold.
		size_t nested = 0;
		shop_replay_duplicate_work candidate;
		duplicate = &candidate;
		const bool equal = prefix(nested) &&
				   critical_command_encode_bounded(left, &candidate.left, forward,
								   this, nested) ==
					   critical_command_codec_result::ok &&
				   prefix(nested) &&
				   critical_command_encode_bounded(right, &candidate.right, forward,
								   this, nested) ==
					   critical_command_codec_result::ok &&
				   candidate.left == candidate.right;
		duplicate = nullptr;
		return equal;
	}
};
} // namespace

bool shop_trade_transaction_restore_replayed_command_bounded(
	const critical_command &original, player_save_coin_replay_budget_scope_owner &pipeline,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer) noexcept
{
	if (!reserve || !nevent_is_game_thread())
		return false;
	const bool flat = persistence_mode_get() == PERSISTENCE_MODE_FLATFILE_PRIMARY;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI || defined(_GLIBCXX_DEBUG)
	return false;
#else
#ifdef __NO_MYSQL__
	if (!flat)
		return false;
#else
	if (flat)
		return false;
#endif
	shop_replay_budget budget{ pipeline, reserve, context, outer };
	if (sizeof(void *) != 8 || sizeof(unsigned long) != 8)
		return false;
	if (!budget.peak())
		return false;
	if (!original.publication_required || original.type != critical_command_type::shop_trade ||
	    original.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION ||
	    original.payload_version != SHOP_TRADE_RECOVERY_MANIFEST_VERSION ||
	    !critical_command_envelope_valid(original))
		return false;
	shop_replay_work work;
	budget.work = &work;
	size_t nested = 0, request = 0;
	try
	{
		if (!budget.prefix(nested) ||
		    shop_trade_accounting_decode_bounded(original, &work.intent, &work.payload,
							 &work.wallet, &work.bank, &work.keeper,
							 shop_replay_budget::forward, &budget,
							 nested) != economic_accounting_error::ok ||
		    !work.payload.recovery_manifest_recorded || !work.payload.player_pid ||
		    work.payload.player_pid > INT_MAX)
			return false;
		auto found = pending.find(original.operation_id.bytes);
		if (found != pending.end())
		{
			if (!found->second.cold || !found->second.cold->command ||
			    (flat && (!found->second.cold->flat_restored ||
				      !found->second.cold->flat_registration_bytes)) ||
			    !budget.equal_commands(*found->second.cold->command, original) ||
			    !budget.prefix(nested, false))
				return false;
			const bool held =
				flat ? player_save_shop_replay_owner::restore_flat(
					       original,
					       found->second.cold->flat_registration_bytes,
					       pipeline, nested) :
				       player_save_shop_replay_owner::restore_sql(original,
										  pipeline, nested);
			if (!held)
				return false;
			found->second.cold->registration_pending = false;
			return true;
		}
		if (pending.size() + notifying >= SHOP_TRADE_PENDING_MAX ||
		    player_pending(work.payload.player_pid) ||
		    shop_trade_transaction_keeper_busy(work.payload.shop_id))
			return false;
		work.entry.player_pid = work.payload.player_pid;
		// Fresh six-vector copy, exactly the original immutable payload. Old
		// parsed capacities remain retained alongside all actual new requests.
		shop_replay_bytes copy;
		for (const auto *binding : { &work.payload.recovery_manifest.player_before,
					     &work.payload.recovery_manifest.player_after,
					     &work.payload.recovery_manifest.keeper_before,
					     &work.payload.recovery_manifest.keeper_after,
					     &work.payload.recovery_manifest.live_target_before,
					     &work.payload.recovery_manifest.live_target_after })
			if (binding->ordered_item_uids.size() > SIZE_MAX / sizeof(uint64_t) ||
			    !copy.add(binding->ordered_item_uids.size() * sizeof(uint64_t)))
				return false;
		if (!budget.peak(copy.value))
			return false;
		work.entry.payload = work.payload;
		if (!budget.peak(sizeof(cold_trade_restore)))
			return false;
		work.entry.cold = std::make_unique<cold_trade_restore>();
		if (!critical_command_fresh_copy_request_bytes(original, &request) ||
		    request > SIZE_MAX - sizeof(critical_command) ||
		    !budget.peak(sizeof(critical_command) + request +
				 critical_command_copy_frame_bytes()))
			return false;
		work.entry.cold->command = std::make_unique<critical_command>(original);
		work.entry.cold->flat_restored = flat;
		const auto &manifest = work.entry.payload.recovery_manifest;
		for (const auto *binding :
		     { &manifest.player_before, &manifest.player_after, &manifest.keeper_before,
		       &manifest.keeper_after, &manifest.live_target_before,
		       &manifest.live_target_after })
			if (!budget.append(work.entry.cold->fenced_uids,
					   binding->ordered_item_uids.data(),
					   binding->ordered_item_uids.size()))
				return false;
		for (size_t i = 0; i < work.payload.item_count; ++i)
			if (!budget.append_one(work.entry.cold->fenced_uids,
					       work.payload.items[i].item_uid))
				return false;
		auto &uids = work.entry.cold->fenced_uids;
		// Preserve the full original sort/unique/erase; authenticate the real
		// input-driven introsort recursion before entering any algorithm.
		size_t sort_frames = 0;
		if (!shop_replay_uid_sort_source_frames(uids.size(), &sort_frames) ||
		    !budget.peak(sort_frames))
			return false;
		std::sort(uids.begin(), uids.end());
		uids.erase(std::unique(uids.begin(), uids.end()), uids.end());
		if (flat && uids.size() > 3 * PLAYER_SNAPSHOT_MAX_OBJECTS)
			return false;
		using map_type = decltype(pending);
		using node_type = std::_Rb_tree_node<map_type::value_type>;
		if (flat)
		{
			// Exact original registration owner's byte law, distinct from full
			// ROOT CURRENT: node + cold + command + all original capacities.
			shop_replay_bytes bytes;
			if (!bytes.add(sizeof(node_type)) ||
			    !bytes.add(sizeof(cold_trade_restore)) ||
			    !bytes.add(sizeof(critical_command)) ||
			    !bytes.command(*work.entry.cold->command) || !bytes.vector(uids) ||
			    !bytes.payload(work.entry.payload) ||
			    bytes.value > PLAYER_SAVE_PIPELINE_MAX_BYTES)
				return false;
			work.entry.cold->flat_registration_bytes = bytes.value;
			if (!budget.peak(sizeof(node_type)))
				return false;
			work.staged.emplace(original.operation_id.bytes, std::move(work.entry));
			work.node = work.staged.extract(work.staged.begin());
			if (!budget.prefix(nested, false))
				return false;
			if (!player_save_shop_replay_owner::restore_flat(
				    original, work.node.mapped().cold->flat_registration_bytes,
				    pipeline, nested))
				return false;
			// No reservation/allocation follows the irreversible shared hold.
			work.node.mapped().cold->registration_pending = false;
			const auto inserted = pending.insert(std::move(work.node));
			return inserted.inserted;
		}
		if (!budget.peak(sizeof(node_type)))
			return false;
		const auto inserted =
			pending.emplace(original.operation_id.bytes, std::move(work.entry));
		if (!inserted.second || !budget.prefix(nested, false) ||
		    !player_save_shop_replay_owner::restore_sql(original, pipeline, nested))
			return false;
		// Uncertain registration retains the original inserted pending node.
		// An exact retry repeats the same typed hold without replacing state.
		inserted.first->second.cold->registration_pending = false;
		return true;
	}
	catch (...)
	{
		return false;
	}
#endif
}
