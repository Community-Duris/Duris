#include "economy/collector_transaction.h"

#include "economy/collector_collection_preparation.h"
#include "economy/collector_accounting.h"
#include "player/player_save_pipeline.h"
#include "persistence/persistence_mode.h"
#include "economy/collector_catalog_cache.h"
#include "economy/collector_runtime.h"
#include "economy/currency_transaction.h"
#include "economy/economic_gameplay_authority.h"
#include "item/item_ownership_runtime.h"
#include "core/prototypes.h"
#include "core/utils.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <climits>
#include <chrono>
#include <mutex>
#include <memory>
#include <map>
#include <new>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

extern P_obj object_list;

namespace
{
struct pending_collector
{
	uint32_t actor_pid = 0;
	std::unique_ptr<collector_command_payload> payload;
	collector_completion_fn completion = nullptr;
	bool completion_ready = false;
	critical_completion completed = {};
};

struct completed_player_recovery
{
	uint32_t actor_pid = 0;
	std::unique_ptr<collector_command_payload> payload;
	collector_command_result result = {};
	collector_completion_fn completion = nullptr;
};

std::unordered_map<std::string, pending_collector> pending;

struct pending_purchase
{
	uint32_t actor_pid = 0;
	std::unique_ptr<critical_command> command;
	std::unique_ptr<collector_command_payload> payload;
	collector::record original_listing = {};
	collector_purchase_effect_fn effect = nullptr;
	collector_completion_fn notify = nullptr;
	critical_completion sealed = {};
	collector_purchase_publication_state effects;
	bool sealed_ready = false;
	bool restore_registration_pending = false;
	bool blocked = false;
	bool batch_conflict = false;
	bool publishing = false;
	bool acknowledged = false;
	bool notification_started = false;
};
// Fixed operation keys and stable nodes survive callback reentry/admission.
std::map<std::array<uint8_t, CRITICAL_COMMAND_ID_BYTES>, pending_purchase> purchases;
bool purchase_pump_running = false;

bool committed_outcome(critical_apply_outcome outcome)
{
	return outcome == critical_apply_outcome::applied ||
	       outcome == critical_apply_outcome::already_applied;
}

bool same_purchase_receipt(const critical_completion &left, const critical_completion &right)
{
	return left.operation_id.bytes == right.operation_id.bytes &&
	       left.disposition == right.disposition &&
	       ((committed_outcome(left.outcome) && committed_outcome(right.outcome)) ||
		left.outcome == right.outcome) &&
	       left.durable_revision == right.durable_revision &&
	       left.error_code == right.error_code && left.failure_stage == right.failure_stage &&
	       left.result_size == right.result_size && left.result_payload == right.result_payload;
}

bool purchase_receipt_valid(const pending_purchase &entry, const critical_completion &completion,
			    collector_command_result *result)
{
	if (!entry.command || !entry.payload || !result ||
	    completion.operation_id.bytes != entry.command->operation_id.bytes ||
	    !critical_completion_disposition_valid(completion) ||
	    completion.failure_stage != critical_failure_stage::none)
		return false;
	if (completion.disposition == critical_completion_disposition::never_admitted)
	{
		*result = {};
		// Existing canonical proven admission refusal, not native rejection.
		// Primary releases the exact original hold without publication/ACK.
		return true;
	}
	if (completion.disposition != critical_completion_disposition::execution ||
	    completion.result_size != COLLECTOR_COMMAND_RESULT_BYTES ||
	    (!committed_outcome(completion.outcome) &&
	     completion.outcome != critical_apply_outcome::terminal_failure) ||
	    (committed_outcome(completion.outcome) ? completion.error_code != 0 :
						     completion.error_code == 0))
		return false;
	std::array<uint8_t, COLLECTOR_COMMAND_RESULT_BYTES> encoded{};
	if (!collector_command_decode_result(completion.result_payload.data(),
					     completion.result_size, result) ||
	    !collector_command_encode_result(*result, &encoded) ||
	    !std::equal(encoded.begin(), encoded.end(), completion.result_payload.begin()) ||
	    std::any_of(completion.result_payload.begin() + encoded.size(),
			completion.result_payload.end(), [](uint8_t byte) { return byte != 0; }))
		return false;
	if (!committed_outcome(completion.outcome))
		// Primary still proves original rejection/no effects in the native owner.
		return !result->record_present;
	const auto &payload = *entry.payload;
	const auto next = [](uint64_t before, uint64_t after)
	{ return before != UINT64_MAX && after == before + 1; };
	collector::record expected = entry.original_listing;
	std::array<uint8_t, collector::encoded_record_bytes> expected_bytes{}, result_bytes{};
	if (collector::purchase(&expected, payload.expected_listing_revision, payload.actor_pid,
				entry.original_listing.price_value, payload.capacity_admitted,
				payload.observed_at) != collector::outcome::applied ||
	    collector::record_encode(expected, &expected_bytes) != collector::codec_result::ok ||
	    collector::record_encode(result->entry, &result_bytes) != collector::codec_result::ok ||
	    expected_bytes != result_bytes || !result->record_present ||
	    result->action != collector_action::purchase || !result->catalog_revision ||
	    !next(payload.items[0].expected_item_revision, result->entry.item_revision) ||
	    !next(payload.expected_from_owner_revision, result->from_owner_revision) ||
	    !next(payload.expected_to_owner_revision, result->to_owner_revision) ||
	    !next(payload.expected_wallet_revision, result->wallet_revision) ||
	    !next(payload.expected_bank_revision, result->bank_revision))
		return false;
	// Wallet denomination/value and unchanged bank vector are authoritative
	// immutable receipt data, verified against native before/after by primary.
	for (const auto &vector : { result->wallet, result->bank })
		for (int64_t amount : vector.amount)
			if (amount < 0 || amount > INT_MAX)
				return false;
	// Exact backend durable binding belongs to primary's retained verifier;
	// flat readback can include a later catalog revision, unlike SQL's receipt.
	return completion.durable_revision >=
	       std::max({ result->catalog_revision, result->entry.item_revision,
			  result->from_owner_revision, result->to_owner_revision,
			  result->wallet_revision, result->bank_revision });
}

bool publish_purchase(decltype(purchases)::iterator found)
{
	auto &entry = found->second;
	if (!entry.sealed_ready || entry.restore_registration_pending || entry.blocked ||
	    entry.publishing || entry.notification_started)
		return false;
	collector_command_result result{};
	if (!purchase_receipt_valid(entry, entry.sealed, &result))
		return false;
	entry.publishing = true;
	try
	{
		if (!entry.acknowledged)
		{
			P_char actor = find_player_by_pid(entry.actor_pid);
			const uint64_t runtime_id = actor ? actor->runtime_id : 0;
			const auto original = entry.sealed;
			// Revalidates native authority and physical projection even after a
			// prior successful effect/failed ACK. Never refund or replace the UID.
			const bool acknowledged = collector_purchase_publication_attempt(
				*entry.command, original, runtime_id, entry.effects, entry.effect);
			// A callback may have delivered a contradictory completion. Keep the
			// first seal and original owner even if the callback returned true.
			if (entry.blocked || !same_purchase_receipt(original, entry.sealed))
			{
				entry.publishing = false;
				return false;
			}
			if (!acknowledged)
			{
				entry.publishing = false;
				return false;
			}
			entry.acknowledged = true;
		}
		// Keep busy ownership during notification reentry. A throwing notifier
		// cannot be repeated: it may already have performed its visible effect.
		entry.notification_started = true;
		if (entry.notify)
			entry.notify(find_player_by_pid(entry.actor_pid),
				     committed_outcome(entry.sealed.outcome), result,
				     entry.sealed.error_code, *entry.payload);
		purchases.erase(found);
		return true;
	}
	catch (...)
	{
		entry.publishing = false;
		return false;
	}
}

void pump_purchases(uint32_t pid = 0)
{
	if (purchase_pump_running)
		return;
	purchase_pump_running = true;
	// Reentrant completion ingestion may change receipt state, but cannot
	// recurse or extend this dispatch by continuously admitting new nodes.
	std::array<std::array<uint8_t, CRITICAL_COMMAND_ID_BYTES>, COLLECTOR_PENDING_MAX> keys{};
	size_t count = 0;
	for (const auto &[key, entry] : purchases)
	{
		if (count == keys.size())
			break;
		if (!pid || entry.actor_pid == pid)
			keys[count++] = key;
	}
	for (size_t index = 0; index < count; ++index)
	{
		auto found = purchases.find(keys[index]);
		if (found != purchases.end())
			publish_purchase(found);
	}
	purchase_pump_running = false;
}
// A committed purchase must not remain in `pending`: that map is the live
// admission fence for listings and players.  Keep a bounded, non-blocking
// recovery handoff for a buyer that disappeared before the game-thread
// callback.  The durable purchase row remains the source of truth if this
// process restarts before the player returns.
std::unordered_map<std::string, completed_player_recovery> player_recoveries;
constexpr size_t COLLECTOR_PLAYER_RECOVERY_MAX = 128;

enum class outbox_publication_state : uint8_t
{
	queued,
	publishing,
	published,
};

struct pending_outbox_publication
{
	critical_operation_id operation_id = {};
	collector_command_result result = {};
	outbox_publication_state state = outbox_publication_state::queued;
};

struct outbox_publication_work
{
	uint64_t outbox_id = 0;
	critical_operation_id operation_id = {};
	collector_command_result result = {};
};

std::mutex outbox_mutex;
std::unordered_map<uint64_t, pending_outbox_publication> outbox_publications;
constexpr size_t COLLECTOR_OUTBOX_PENDING_MAX = 1024;

std::string operation_key(const critical_operation_id &operation_id)
{
	return std::string(reinterpret_cast<const char *>(operation_id.bytes.data()),
			   operation_id.bytes.size());
}

bool player_recovery_item_loaded(const completed_player_recovery &recovery, P_char character)
{
	if (!character || !recovery.payload || !recovery.result.entry.uid)
		return false;
	for (P_obj object = object_list; object; object = object->next)
	{
		if (object->obj_uid != recovery.result.entry.uid)
			continue;
		P_obj outer = object;
		size_t depth = 0;
		while (outer && OBJ_INSIDE(outer) && outer->loc.inside && depth++ < 4096)
			outer = outer->loc.inside;
		if (outer && depth < 4096 &&
		    (OBJ_CARRIED_BY(outer, character) || OBJ_WORN_BY(outer, character)))
			return true;
	}
	return false;
}

bool player_pending(uint32_t pid)
{
	return std::any_of(purchases.begin(), purchases.end(),
			   [pid](const auto &entry) { return entry.second.actor_pid == pid; }) ||
	       std::any_of(pending.begin(), pending.end(),
			   [pid](const auto &entry) { return entry.second.actor_pid == pid; });
}

bool listing_pending(uint64_t listing)
{
	return listing && (std::any_of(purchases.begin(), purchases.end(),
				       [listing](const auto &entry) {
					       return entry.second.payload &&
						      entry.second.payload->listing == listing;
				       }) ||
			   std::any_of(pending.begin(), pending.end(),
				       [listing](const auto &entry) {
					       return entry.second.payload &&
						      entry.second.payload->listing == listing;
				       }));
}

bool publishes_authority(const collector_command_payload &payload)
{
	return payload.action == collector_action::collect ||
	       payload.action == collector_action::purchase ||
	       payload.action == collector_action::expire ||
	       (payload.action == collector_action::cancel && payload.item_count);
}

bool publish(std::unordered_map<std::string, pending_collector>::iterator found, P_char character)
{
	pending_collector &entry = found->second;
	if (!entry.payload)
	{
		pending.erase(found);
		return false;
	}
	const collector_command_payload &submitted_payload = *entry.payload;
	collector_command_result result = {};
	const bool decoded = collector_command_decode_result(entry.completed.result_payload.data(),
							     entry.completed.result_size, &result);
	const bool durable_commit = entry.completed.outcome == critical_apply_outcome::applied ||
				    entry.completed.outcome ==
					    critical_apply_outcome::already_applied;
	const bool committed = decoded && durable_commit;
	bool published = decoded;
	unsigned int publication_error = entry.completed.error_code;
	if (!decoded)
		publication_error = entry.completed.error_code ? entry.completed.error_code :
								 EBADMSG;
	if (published && committed &&
	    (!result.record_present || result.action != submitted_payload.action))
	{
		published = false;
		publication_error = EBADMSG;
	}
	P_obj collected_item = nullptr;
	if (published && committed && submitted_payload.action == collector_action::collect &&
	    !collector_collection_live_matches(submitted_payload, &collected_item))
	{
		published = false;
		publication_error = ESTALE;
	}
	if (published && committed && publishes_authority(submitted_payload) &&
	    !item_ownership_runtime_apply_collector(submitted_payload, result))
	{
		published = false;
		publication_error = ESTALE;
	}
	if (published && committed && submitted_payload.action == collector_action::collect &&
	    !collector_collection_detach_live(collected_item))
	{
		published = false;
		publication_error = ESTALE;
	}
	if (published && committed && submitted_payload.action == collector_action::purchase &&
	    character &&
	    !currency_transaction_publish_balances(
		    character, submitted_payload.account_name.data(), submitted_payload.racewar,
		    result.wallet, result.bank, result.wallet_revision, result.bank_revision))
	{
		published = false;
		publication_error = ESTALE;
	}
	// The repository has already durably applied the wallet and item rows.  A
	// purchase may therefore be published without a live character: custody and
	// catalog state must stop fencing the listing, while the player's in-memory
	// wallet and notification are naturally refreshed on the next login.  The
	// completion callback intentionally accepts nullptr for this recovery path.
	if (published && committed && !collector_runtime_publish(result))
	{
		published = false;
		publication_error = ESTALE;
	}

	const auto completion = entry.completion;
	std::unique_ptr<collector_command_payload> payload = std::move(entry.payload);
	bool completion_deferred = false;
	if (committed && decoded && submitted_payload.action == collector_action::purchase &&
	    !character && completion && player_recoveries.size() < COLLECTOR_PLAYER_RECOVERY_MAX)
	{
		// Retain the payload until the buyer is ready.  This closes the reconnect
		// gap where a live body is reused without a fresh player-item load.
		try
		{
			auto recovery_payload =
				std::make_unique<collector_command_payload>(*payload);
			const std::string key = operation_key(entry.completed.operation_id);
			const auto inserted = player_recoveries.emplace(
				key, completed_player_recovery{ submitted_payload.actor_pid,
								std::move(recovery_payload), result,
								completion });
			completion_deferred = inserted.second;
		}
		catch (const std::bad_alloc &)
		{
			// The callback below still reports the durable commit.  A subsequent
			// normal login can recover the item from the authoritative row.
		}
	}
	pending.erase(found);
	if (completion && !completion_deferred)
		completion(character, durable_commit, decoded ? result : collector_command_result{},
			   publication_error, *payload);
	return committed && published;
}

bool submit(P_char character, const critical_operation_id &operation_id,
	    const collector_command_payload &payload, collector_completion_fn completion,
	    critical_source_site source, critical_deadline_class deadline)
{
	if (pending.size() + purchases.size() >= COLLECTOR_PENDING_MAX ||
	    listing_pending(payload.listing) ||
	    purchases.find(operation_id.bytes) != purchases.end() ||
	    critical_operation_id_is_zero(operation_id) ||
	    (payload.actor_pid && (!character || IS_NPC(character) || GET_PID(character) <= 0 ||
				   static_cast<uint32_t>(GET_PID(character)) != payload.actor_pid ||
				   player_pending(payload.actor_pid))))
		return false;
	critical_command command = {};
	if (!collector_command_build(&command, operation_id, payload, source, deadline))
		return false;
	std::string key;
	try
	{
		key = operation_key(operation_id);
		auto payload_copy = std::make_unique<collector_command_payload>(payload);
		const auto inserted =
			pending.emplace(key, pending_collector{ payload.actor_pid,
								std::move(payload_copy),
								completion,
								false,
								{} });
		if (!inserted.second)
			return false;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	const critical_submit_result submitted =
		critical_command_coordinator_submit(std::move(command));
	if (!critical_submit_result_keeps_operation(submitted))
	{
		pending.erase(key);
		return false;
	}
	return true;
}
}

economic_accounting_error
collector_purchase_prepare_accounted(critical_command *command,
				     const collector::record &original_listing)
{
	return economic_gameplay_authority::prepare_collector_purchase(command, original_listing);
}

bool collector_purchase_cold_restore_owner::restore(const critical_command &original,
						    collector_purchase_effect_fn effect,
						    collector_completion_fn notify) noexcept
{
	try
	{
		if (!nevent_is_game_thread() || !effect || !notify ||
#ifndef __NO_MYSQL__
		    persistence_mode_get() == PERSISTENCE_MODE_FLATFILE_PRIMARY ||
#else
		    persistence_mode_get() != PERSISTENCE_MODE_FLATFILE_PRIMARY ||
#endif
		    !original.publication_required ||
		    original.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION ||
		    !critical_command_envelope_valid(original))
			return false;
		economic_frozen_intent intent;
		collector_command_payload payload{};
		collector::record listing{};
		economic_account_key wallet{}, bank{};
		// The admitted intent supplies the original listing, not today's catalog
		// and not a result-only outbox event. No native completion exists at boot.
		if (collector_purchase_accounting_decode(original, &intent, &payload, &listing,
							 &wallet,
							 &bank) != economic_accounting_error::ok ||
		    !payload.actor_pid || payload.actor_pid > INT_MAX ||
		    payload.action != collector_action::purchase || payload.item_count != 1 ||
		    !payload.selected_item_uid)
			return false;
		auto found = purchases.find(original.operation_id.bytes);
		if (found != purchases.end())
		{
			auto &entry = found->second;
			// Even exact duplicates preserve the first seal, conflict flags and
			// native handler stages; no completion or effect is replaced here.
			if (!entry.command || !critical_command_equal(*entry.command, original) ||
			    entry.effect != effect || entry.notify != notify ||
			    !player_save_pipeline_restore_sql_collector_purchase_obligation(
				    original))
				return false;
			entry.restore_registration_pending = false;
			return true;
		}
		if (pending.size() + purchases.size() >= COLLECTOR_PENDING_MAX ||
		    player_pending(payload.actor_pid) || listing_pending(payload.listing) ||
		    pending.find(operation_key(original.operation_id)) != pending.end())
			return false;
		pending_purchase entry;
		entry.actor_pid = payload.actor_pid;
		entry.command = std::make_unique<critical_command>(original);
		entry.payload = std::make_unique<collector_command_payload>(payload);
		entry.original_listing = listing;
		entry.effect = effect;
		entry.notify = notify;
		entry.restore_registration_pending = true;
		// All domain allocation precedes shared hold registration. Retain this
		// original on registration doubt, but do not let it publish until the
		// same original has successfully registered its exact prepared hold.
		const auto inserted =
			purchases.emplace(original.operation_id.bytes, std::move(entry));
		if (!inserted.second ||
		    !player_save_pipeline_restore_sql_collector_purchase_obligation(original))
			return false;
		inserted.first->second.restore_registration_pending = false;
		// Genuine full native completions arrive through handle_completions.
		// Startup registration performs no materialization, projection or ACK.
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool collector_transaction_submit_purchase_identified(P_char character,
						      const critical_operation_id &operation_id,
						      const collector_command_payload &payload,
						      const collector::record &original_listing,
						      collector_purchase_effect_fn effect,
						      collector_completion_fn notify)
{
	if (!character || IS_NPC(character) || GET_PID(character) <= 0 || !effect ||
	    payload.action != collector_action::purchase || payload.item_count != 1 ||
	    static_cast<uint32_t>(GET_PID(character)) != payload.actor_pid ||
	    critical_operation_id_is_zero(operation_id) ||
	    pending.size() + purchases.size() >= COLLECTOR_PENDING_MAX ||
	    player_pending(payload.actor_pid) || listing_pending(payload.listing) ||
	    original_listing.listing != payload.listing ||
	    original_listing.uid != payload.selected_item_uid ||
	    original_listing.revision != payload.expected_listing_revision ||
	    original_listing.item_revision != payload.items[0].expected_item_revision)
		return false;
	critical_command submission{};
	try
	{
		pending_purchase entry;
		entry.actor_pid = payload.actor_pid;
		entry.command = std::make_unique<critical_command>();
		entry.payload = std::make_unique<collector_command_payload>(payload);
		entry.original_listing = original_listing;
		entry.effect = effect;
		entry.notify = notify;
		if (!collector_command_build(entry.command.get(), operation_id, payload,
					     critical_source_site::command,
					     critical_deadline_class::interactive) ||
		    collector_purchase_prepare_accounted(entry.command.get(), original_listing) !=
			    economic_accounting_error::ok ||
		    entry.command->schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION)
			return false;
		entry.command->publication_required = true;
		// Freeze the same real acceptance time in the domain owner and the
		// coordinator. Its timestamp is assigned only after intent preparation.
		entry.command->accepted_at_usec =
			std::chrono::duration_cast<std::chrono::microseconds>(
				std::chrono::system_clock::now().time_since_epoch())
				.count();
		if (!critical_command_envelope_valid(*entry.command) ||
		    pending.find(operation_key(operation_id)) != pending.end())
			return false;
		// Finish the allocating immutable command copy before admission/hold.
		// A copy refusal therefore cannot become a spurious uncertain owner.
		submission = *entry.command;
		if (!purchases.emplace(operation_id.bytes, std::move(entry)).second)
			return false;
	}
	catch (...)
	{
		return false;
	}
	try
	{
		const auto submitted =
			collector_purchase_submit_for_publication(std::move(submission));
		if (!critical_submit_result_keeps_operation(submitted))
		{
			purchases.erase(operation_id.bytes);
			return false;
		}
	}
	catch (...)
	{
		// Submission may have installed the original hold/journal obligation.
		// Preserve its exact operation; never queue another purchase on doubt.
	}
	return true;
}

bool collector_transaction_submit(P_char character, const collector_command_payload &payload,
				  collector_completion_fn completion,
				  critical_deadline_class deadline)
{
	critical_operation_id operation_id = {};
	return critical_operation_id_generate(&operation_id) &&
	       collector_transaction_submit_identified(character, operation_id, payload, completion,
						       deadline);
}

bool collector_transaction_submit_identified(P_char character,
					     const critical_operation_id &operation_id,
					     const collector_command_payload &payload,
					     collector_completion_fn completion,
					     critical_deadline_class deadline)
{
	return payload.actor_pid && submit(character, operation_id, payload, completion,
					   critical_source_site::command, deadline);
}

bool collector_transaction_submit_background(const collector_command_payload &payload,
					     collector_completion_fn completion)
{
	critical_operation_id operation_id = {};
	return critical_operation_id_generate(&operation_id) &&
	       collector_transaction_submit_background_identified(operation_id, payload,
								  completion);
}

bool collector_transaction_submit_background_identified(const critical_operation_id &operation_id,
							const collector_command_payload &payload,
							collector_completion_fn completion)
{
	return !payload.actor_pid &&
	       submit(nullptr, operation_id, payload, completion, critical_source_site::zone_event,
		      critical_deadline_class::background);
}

void collector_transaction_handle_completions(const critical_completion *completions, size_t count)
{
	if (count && !completions)
		return;
	for (auto &[key, entry] : purchases)
	{
		(void)key;
		entry.batch_conflict = false;
	}
	// Seal the whole delivered batch before any physical effect or ACK. A
	// contradiction followed by a canonical duplicate in this batch stays held.
	for (size_t index = 0; index < count; ++index)
	{
		auto found = purchases.find(completions[index].operation_id.bytes);
		if (found == purchases.end())
			continue;
		auto &entry = found->second;
		collector_command_result result{};
		if (!purchase_receipt_valid(entry, completions[index], &result) ||
		    (entry.sealed_ready &&
		     !same_purchase_receipt(entry.sealed, completions[index])))
		{
			entry.blocked = entry.batch_conflict = entry.effects.receipt_conflict =
				true;
			continue;
		}
		if (!entry.sealed_ready)
		{
			entry.sealed = completions[index];
			entry.sealed_ready = true;
		}
		if (!entry.publishing && !entry.batch_conflict)
			entry.blocked = entry.effects.receipt_conflict = false;
	}
	pump_purchases();
	for (size_t index = 0; index < count; ++index)
	{
		if (purchases.find(completions[index].operation_id.bytes) != purchases.end())
			continue;
		std::string key;
		try
		{
			key = operation_key(completions[index].operation_id);
		}
		catch (const std::bad_alloc &)
		{
			continue;
		}
		auto found = pending.find(key);
		if (found == pending.end())
			continue;
		found->second.completed = completions[index];
		found->second.completion_ready = true;
		P_char character = found->second.actor_pid ?
					   find_player_by_pid(found->second.actor_pid) :
					   nullptr;
		// Durable completion may race with a disconnect.  Publish the durable
		// custody/catalog transition immediately even when the character is no
		// longer live; the in-memory wallet and player-facing callback are guarded
		// by `character` inside publish() and will be recovered by the next login.
		publish(found, character);
	}
}

void collector_transaction_player_ready(P_char character)
{
	if (!character || IS_NPC(character) || GET_PID(character) <= 0)
		return;
	pump_purchases(static_cast<uint32_t>(GET_PID(character)));
	for (auto found = pending.begin(); found != pending.end();)
	{
		auto current = found++;
		if (current->second.actor_pid == static_cast<uint32_t>(GET_PID(character)) &&
		    current->second.completion_ready)
			publish(current, character);
	}
	for (auto found = player_recoveries.begin(); found != player_recoveries.end();)
	{
		auto current = found++;
		if (current->second.actor_pid != static_cast<uint32_t>(GET_PID(character)))
			continue;
		// enter_game hydrates player_items before this hook.  If the committed
		// UID is already attached to this player's live graph, the durable row is
		// the cold-login recovery; replaying the callback would attempt to
		// materialize a second copy.
		if (player_recovery_item_loaded(current->second, character))
		{
			player_recoveries.erase(current);
			continue;
		}
		completed_player_recovery recovery = std::move(current->second);
		player_recoveries.erase(current);
		if (recovery.completion && recovery.payload)
			recovery.completion(character, true, recovery.result, 0, *recovery.payload);
	}
}

bool collector_transaction_player_busy(P_char character)
{
	return character && !IS_NPC(character) && GET_PID(character) > 0 &&
	       player_pending(static_cast<uint32_t>(GET_PID(character)));
}

bool collector_transaction_listing_busy(uint64_t listing)
{
	return listing_pending(listing);
}

bool collector_transaction_item_busy(uint64_t item_uid)
{
	if (!item_uid)
		return false;
	if (std::any_of(purchases.begin(), purchases.end(),
			[item_uid](const auto &entry) {
				return entry.second.payload &&
				       entry.second.payload->selected_item_uid == item_uid;
			}))
		return true;
	return std::any_of(pending.begin(), pending.end(),
			   [item_uid](const auto &entry)
			   {
				   if (!entry.second.payload ||
				       entry.second.payload->action != collector_action::collect)
					   return false;
				   const collector_command_payload &payload = *entry.second.payload;
				   return std::any_of(payload.items.begin(),
						      payload.items.begin() + payload.item_count,
						      [item_uid](const item_transfer_entry &item)
						      { return item.item_uid == item_uid; });
			   });
}

critical_outbox_delivery_result
collector_transaction_outbox_delivery(const critical_outbox_record &record, void *context)
{
	if (record.destination != COLLECTOR_OUTBOX_DESTINATION)
		return critical_outbox_test_destination(record, context);
	collector_command_result result = {};
	if (record.event_type != COLLECTOR_OUTBOX_EVENT_MUTATED ||
	    critical_operation_id_is_zero(record.operation_id) ||
	    (record.payload_version != COLLECTOR_COMMAND_RESULT_VERSION &&
	     record.payload_version != COLLECTOR_COMMAND_PREVIOUS_RESULT_VERSION) ||
	    !collector_command_decode_result(record.payload.data(), record.payload.size(),
					     &result) ||
	    !result.record_present)
		return critical_outbox_delivery_result::terminal_failure;
	std::lock_guard<std::mutex> lock(outbox_mutex);
	auto found = outbox_publications.find(record.outbox_id);
	if (found != outbox_publications.end())
	{
		if (found->second.state == outbox_publication_state::published)
		{
			outbox_publications.erase(found);
			return critical_outbox_delivery_result::delivered;
		}
		return critical_outbox_delivery_result::retryable_failure;
	}
	try
	{
		if (!record.outbox_id || outbox_publications.size() >= COLLECTOR_OUTBOX_PENDING_MAX)
			return critical_outbox_delivery_result::retryable_failure;
		outbox_publications.emplace(
			record.outbox_id,
			pending_outbox_publication{ record.operation_id, result,
						    outbox_publication_state::queued });
	}
	catch (const std::bad_alloc &)
	{
		return critical_outbox_delivery_result::retryable_failure;
	}
	return critical_outbox_delivery_result::retryable_failure;
}

void collector_transaction_publish_outbox(void)
{
	pump_purchases();
	std::vector<outbox_publication_work> work;
	{
		std::lock_guard<std::mutex> lock(outbox_mutex);
		try
		{
			work.reserve(std::min<size_t>(64, outbox_publications.size()));
			for (auto &[outbox_id, publication] : outbox_publications)
			{
				if (work.size() >= 64)
					break;
				if (publication.state == outbox_publication_state::queued)
				{
					publication.state = outbox_publication_state::publishing;
					work.push_back({ outbox_id, publication.operation_id,
							 publication.result });
				}
			}
		}
		catch (const std::bad_alloc &)
		{
			for (auto &[outbox_id, publication] : outbox_publications)
			{
				(void)outbox_id;
				if (publication.state == outbox_publication_state::publishing)
					publication.state = outbox_publication_state::queued;
			}
			return;
		}
	}
	bool published_any = false;
	for (const outbox_publication_work &entry : work)
	{
		if (purchases.find(entry.operation_id.bytes) != purchases.end())
		{
			// Result-only outbox events cannot replace a sealed typed completion
			// or supply its durable revision/native proof. Retry existing owner.
			std::lock_guard<std::mutex> lock(outbox_mutex);
			auto found = outbox_publications.find(entry.outbox_id);
			if (found != outbox_publications.end())
				found->second.state = outbox_publication_state::queued;
			continue;
		}
		bool published = false;
		std::string key;
		try
		{
			key = operation_key(entry.operation_id);
		}
		catch (const std::bad_alloc &)
		{
			key.clear();
		}
		auto pending_found = key.empty() ? pending.end() : pending.find(key);
		if (pending_found != pending.end())
		{
			std::array<uint8_t, COLLECTOR_COMMAND_RESULT_BYTES> encoded = {};
			if (collector_command_encode_result(entry.result, &encoded))
			{
				pending_found->second.completed = {};
				pending_found->second.completed.operation_id = entry.operation_id;
				pending_found->second.completed.outcome =
					critical_apply_outcome::already_applied;
				pending_found->second.completed.result_size = encoded.size();
				std::copy(encoded.begin(), encoded.end(),
					  pending_found->second.completed.result_payload.begin());
				pending_found->second.completion_ready = true;
				P_char character =
					pending_found->second.actor_pid ?
						find_player_by_pid(
							pending_found->second.actor_pid) :
						nullptr;
				// The durable purchase result is sufficient to converge custody and
				// catalog state after a disconnect.  `publish()` skips live-wallet
				// synchronization when character is null and the next login reloads
				// that already-committed balance/item state.
				published = publish(pending_found, character);
			}
		}
		else
		{
			published =
				collector_publish_committed_event(entry.result, entry.outbox_id);
			if (published)
				// A restarted process has no retained item payload.  The durable
				// catalog and held-item projection are the recovery source of truth;
				// force the cache to reconcile them before exposing the next command.
				collector_catalog_cache_invalidate();
		}
		std::lock_guard<std::mutex> lock(outbox_mutex);
		auto found = outbox_publications.find(entry.outbox_id);
		if (found != outbox_publications.end())
			found->second.state = published ? outbox_publication_state::published :
							  outbox_publication_state::queued;
		published_any = published_any || published;
	}
	if (published_any)
		critical_outbox_resume();
}

void collector_transaction_reset_for_tests(void)
{
	purchases.clear();
	pending.clear();
	player_recoveries.clear();
	std::lock_guard<std::mutex> lock(outbox_mutex);
	outbox_publications.clear();
}

#include <limits>
#include <type_traits>
#include <compare>

namespace
{
bool collector_replay_add(size_t &bytes, size_t extra) noexcept
{
	if (extra > SIZE_MAX - bytes)
		return false;
	bytes += extra;
	return true;
}
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
template <class Key, class Value>
bool collector_table_heap(const std::unordered_map<Key, Value> &table, size_t &bytes) noexcept
{
	using map_type = std::unordered_map<Key, Value>;
	using node_type =
		std::__detail::_Hash_node<typename map_type::value_type,
					  std::__cache_default<Key, std::hash<Key>>::value>;
	const size_t buckets = table.bucket_count();
	// GCC13 _M_allocate_buckets(1) returns the genuine inline single bucket.
	if (!buckets ||
	    (buckets > 1 &&
	     (buckets > SIZE_MAX / sizeof(std::__detail::_Hash_node_base *) ||
	      !collector_replay_add(bytes, buckets * sizeof(std::__detail::_Hash_node_base *)))))
		return false;
	return table.size() <= SIZE_MAX / sizeof(node_type) &&
	       collector_replay_add(bytes, table.size() * sizeof(node_type));
}
#endif
struct collector_replay_budget
{
	bool (*reserve)(size_t, void *) noexcept;
	void *context;
	size_t outer, frames;
	const economic_frozen_intent *intent = nullptr;
	const pending_purchase *entry = nullptr;
	const std::string *key = nullptr;
	const std::vector<uint8_t> *left = nullptr, *right = nullptr;
	bool current(size_t &bytes, size_t extra = 0) const noexcept
	{
		size_t heap = 0;
		if (!collector_transaction_replay_current_storage_bytes(&bytes) ||
		    !collector_replay_add(bytes, outer) || !collector_replay_add(bytes, frames) ||
		    !collector_replay_add(bytes, sizeof(*this)) ||
		    !collector_replay_add(bytes,
					  collector_transaction_replay_observer_frame_bytes()) ||
		    !collector_replay_add(bytes, extra) ||
		    (intent && !collector_replay_add(bytes, intent->admission.facts.capacity())) ||
		    (key && key->capacity() > 15 &&
		     (key->capacity() == SIZE_MAX ||
		      !collector_replay_add(bytes, key->capacity() + 1))) ||
		    (left && !collector_replay_add(bytes, left->capacity())) ||
		    (right && !collector_replay_add(bytes, right->capacity())))
			return false;
		if (entry && entry->command &&
		    (!collector_replay_add(bytes, sizeof(critical_command)) ||
		     !critical_command_current_heap_bytes(*entry->command, &heap) ||
		     !collector_replay_add(bytes, heap)))
			return false;
		return !entry || !entry->payload ||
		       collector_replay_add(bytes, sizeof(collector_command_payload));
	}
	bool peak(size_t extra = 0) const noexcept
	{
		size_t bytes = 0;
		return current(bytes, extra) && reserve && reserve(bytes, context);
	}
	static bool child(size_t request, void *opaque) noexcept
	{
		return static_cast<collector_replay_budget *>(opaque)->peak(request);
	}
	bool equal(const critical_command &a, const critical_command &b) noexcept
	{
		// Preserve original canonical wire comparison, not a projected field subset.
		const size_t own = 2 * sizeof(std::vector<uint8_t>) + 8 * sizeof(void *) +
				   4 * sizeof(size_t) + 3 * sizeof(bool) +
				   critical_command_valid_frame_bytes() +
				   critical_command_copy_frame_bytes();
		if (!peak(own))
			return false;
		std::vector<uint8_t> a_bytes, b_bytes;
		left = &a_bytes;
		right = &b_bytes;
		bool matched = false;
		do
		{
			if (critical_command_encode_bounded(
				    a, &a_bytes, collector_replay_budget::child, this, own) !=
			    critical_command_codec_result::ok)
				break;
			if (critical_command_encode_bounded(
				    b, &b_bytes, collector_replay_budget::child, this, own) !=
			    critical_command_codec_result::ok)
				break;
			matched = a_bytes == b_bytes;
		} while (false);
		left = nullptr;
		right = nullptr;
		return matched;
	}
};
}

size_t collector_transaction_replay_observer_frame_bytes() noexcept
{
	// Actual lock/scoped iterator/node/string/owned-command observation closures.
	return sizeof(std::lock_guard<std::mutex>) + 24 * sizeof(void *) + 17 * sizeof(size_t) +
	       7 * sizeof(bool) + critical_command_copy_frame_bytes();
}
bool collector_transaction_replay_current_storage_bytes(size_t *output) noexcept
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
	if (!output || !nevent_is_game_thread())
		return false;
	try
	{
		size_t bytes = sizeof(pending) + sizeof(purchases) + sizeof(purchase_pump_running) +
			       sizeof(player_recoveries) + sizeof(outbox_mutex) +
			       sizeof(outbox_publications);
		if (!collector_table_heap(pending, bytes) ||
		    !collector_table_heap(player_recoveries, bytes))
			return false;
		using purchase_node = std::_Rb_tree_node<typename decltype(purchases)::value_type>;
		if (purchases.size() > SIZE_MAX / sizeof(purchase_node) ||
		    !collector_replay_add(bytes, purchases.size() * sizeof(purchase_node)))
			return false;
		for (const auto &value : pending)
		{
			if (value.first.capacity() > 15 &&
			    (value.first.capacity() == SIZE_MAX ||
			     !collector_replay_add(bytes, value.first.capacity() + 1)))
				return false;
			if (value.second.payload &&
			    !collector_replay_add(bytes, sizeof(collector_command_payload)))
				return false;
		}
		for (const auto &value : player_recoveries)
		{
			if (value.first.capacity() > 15 &&
			    (value.first.capacity() == SIZE_MAX ||
			     !collector_replay_add(bytes, value.first.capacity() + 1)))
				return false;
			if (value.second.payload &&
			    !collector_replay_add(bytes, sizeof(collector_command_payload)))
				return false;
		}
		for (const auto &value : purchases)
		{
			size_t heap = 0;
			if (value.second.command &&
			    (!collector_replay_add(bytes, sizeof(critical_command)) ||
			     !critical_command_current_heap_bytes(*value.second.command, &heap) ||
			     !collector_replay_add(bytes, heap)))
				return false;
			if (value.second.payload &&
			    !collector_replay_add(bytes, sizeof(collector_command_payload)))
				return false;
		}
		{
			// This is the existing leaf mutex, independent of coordinator/pipeline.
			std::lock_guard<std::mutex> lock(outbox_mutex);
			if (!collector_table_heap(outbox_publications, bytes))
				return false;
		}
		*output = bytes;
		return true;
	}
	catch (...)
	{
		return false;
	}
#else
	(void)output;
	return false;
#endif
}

namespace
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
size_t collector_hinted_emplace_source_frames() noexcept
{
	using tree_type = decltype(purchases);
	using key_type = typename tree_type::key_type;
	using iterator = typename tree_type::iterator;
	using const_iterator = typename tree_type::const_iterator;
	using result = std::pair<iterator, bool>;
	using position = std::pair<std::_Rb_tree_node_base *, std::_Rb_tree_node_base *>;
	using references = std::pair<const key_type &, pending_purchase &>;
	static_assert(std::is_same_v<decltype(std::declval<const key_type &>() <=>
					      std::declval<const key_type &>()),
				     std::strong_ordering>);
	// stl_map.h:590-605: this/__args, real pair<_Args&...> temporary,
	// hidden structured-binding reference plus __a/__v, __k, __i, result.
	constexpr size_t map_emplace = 3 * sizeof(void *) + sizeof(references) +
				       4 * sizeof(void *) + sizeof(iterator) + sizeof(result);
	// stl_pair.h reference-pair lvalue ctor(this,x,y); two get(rvalue pair)
	// -> __move_get -> forward chains each own argument/result references.
	// Returned pair<iterator,bool> ctor(this,x,y), its two forwards and the
	// original true argument are separate actual selected source scopes.
	constexpr size_t argument_pair = 3 * sizeof(void *) + 12 * sizeof(void *) +
					 3 * sizeof(void *) + 4 * sizeof(void *) + sizeof(bool);
	// map::lower_bound -> tree::lower_bound -> _M_lower_bound(x,y,k),
	// including actual return iterator and its node-pointer constructor.
	constexpr size_t lower_bound = 2 * sizeof(void *) + sizeof(iterator) + 2 * sizeof(void *) +
				       sizeof(iterator) + 4 * sizeof(void *) + sizeof(iterator) +
				       2 * sizeof(void *);
	// map::emplace_hint -> _M_emplace_hint_unique: actual const hint,
	// this/two forwarded references, return iterator, _Auto_node's tree& and
	// node pointer, and __res (actual two-node-pointer position pair).
	constexpr size_t emplace_hint = 3 * sizeof(void *) + sizeof(const_iterator) +
					sizeof(iterator) + 3 * sizeof(void *) +
					sizeof(const_iterator) + sizeof(iterator) +
					2 * sizeof(void *) + sizeof(position);
	// _M_get_insert_hint_unique_pos: this/position/key, __pos, __before or
	// __after and returned position. Original fallback _M_get_insert_unique_pos
	// additionally owns x/y/comp/j/result; both searches are iterative.
	constexpr size_t hint_position = 2 * sizeof(void *) + sizeof(const_iterator) +
					 3 * sizeof(iterator) + sizeof(position) +
					 4 * sizeof(void *) + sizeof(bool) + sizeof(iterator) +
					 sizeof(position);
	// _Auto_node ctor/destructor/_M_key/_M_insert: this/tree/forward refs,
	// __p value, __it and returned iterator. No fabricated node owner exists.
	constexpr size_t auto_node_calls = 4 * sizeof(void *) + sizeof(void *) +
					   2 * sizeof(void *) + sizeof(void *) + sizeof(position) +
					   2 * sizeof(iterator);
	// _M_create_node(this,args,tmp,result), _M_get_node(this,result),
	// _M_construct_node(this,node,args), node allocator access, value/storage
	// pointer access, placement new and real traits::construct/construct_at.
	constexpr size_t node_construction =
		5 * sizeof(void *) + 2 * sizeof(void *) + 4 * sizeof(void *) + 2 * sizeof(void *) +
		4 * sizeof(void *) + 2 * sizeof(void *) + sizeof(size_t) + 4 * sizeof(void *) +
		4 * sizeof(void *) + 4 * sizeof(void *);
	// Pair<const key,pending_purchase> ctor and forwards, generated key/body
	// moves and their unique_ptr/tuple reference carriers; full existing
	// command ownership/cleanup profile remains additional below.
	constexpr size_t value_construction = 3 * sizeof(void *) + 4 * sizeof(void *) +
					      2 * sizeof(void *) + 2 * sizeof(void *) +
					      4 * sizeof(void *) + 4 * sizeof(void *);
	// alloc_traits/allocator/new_allocator/operator new arguments/results,
	// actual new_allocator max_size; one genuine fresh node allocation.
	constexpr size_t allocator_calls =
		8 * sizeof(void *) + 4 * sizeof(size_t) + 2 * sizeof(void *) + 2 * sizeof(size_t);
	// _M_insert_node(this,x,p,z,insert_left,result) and original exported
	// rebalance bool/z/p/header arguments. Its emitted implementation remains
	// part of the existing independent native/library qualification gate.
	constexpr size_t insert_node = 4 * sizeof(void *) + sizeof(bool) + sizeof(iterator) +
				       3 * sizeof(void *) + sizeof(bool);
	// Selected tree accessors: key/value/aligned storage/Select1st, left/right,
	// begin/end/leftmost/rightmost, const_cast, iterator ctor/deref/equality,
	// increment/decrement and their original exported node arguments/results.
	constexpr size_t accessors =
		2 * sizeof(void *) + 2 * sizeof(void *) + 2 * sizeof(void *) + 2 * sizeof(void *) +
		3 * sizeof(void *) + 2 * sizeof(void *) + 2 * sizeof(void *) + 2 * sizeof(void *) +
		2 * sizeof(void *) + 2 * sizeof(void *) + 2 * sizeof(void *) + 2 * sizeof(void *) +
		2 * sizeof(void *) + 2 * sizeof(void *) + 2 * sizeof(void *) + 2 * sizeof(void *) +
		sizeof(bool) + 2 * sizeof(void *) + 2 * sizeof(void *) + 2 * sizeof(void *) +
		2 * sizeof(void *) +
		// map/tree key_comp this/reference and their actual stateless return.
		2 * (sizeof(void *) + sizeof(typename tree_type::key_compare));
	// stl_function.h less<key>: this/x/y/result. C++20 array<uchar,32>
	// compares by its real memcmp fast path and returns strong_ordering;
	// include array arguments/n/result, memcmp inputs/int result, actual
	// strong_ordering construction and operator<(strong_ordering,__unspec).
	// __unspec's consteval constructor has no runtime call frame.
	constexpr size_t key_comparison =
		3 * sizeof(void *) + sizeof(bool) + 2 * sizeof(void *) + sizeof(size_t) +
		sizeof(std::strong_ordering) + 2 * sizeof(void *) + sizeof(size_t) + sizeof(int) +
		sizeof(void *) + sizeof(std::__cmp_cat::_Ord) + sizeof(std::strong_ordering) +
		sizeof(std::strong_ordering) + sizeof(std::__cmp_cat::__unspec) + sizeof(bool);
	// Node cleanup _M_destroy_node/_M_drop_node/_M_put_node, allocator destroy,
	// destroy_at, actual pair/unique_ptr cleanup, traits/allocator/new_allocator
	// deallocate and sized delete. The actual owned command's four vectors use
	// the already authenticated original complete command cleanup profile.
	constexpr size_t cleanup = 3 * (2 * sizeof(void *)) + 3 * sizeof(void *) +
				   2 * sizeof(void *) + 2 * sizeof(void *) + 2 * sizeof(void *) +
				   4 * (2 * sizeof(void *) + sizeof(size_t)) + sizeof(void *) +
				   sizeof(size_t);
	// This existing command profile is a genuine runtime function. Keep all
	// pure fixed carrier subtotals constexpr, but obtain its total at runtime.
	return map_emplace + argument_pair + lower_bound + emplace_hint + hint_position +
	       auto_node_calls + node_construction + value_construction + allocator_calls +
	       insert_node + accessors + key_comparison + cleanup +
	       critical_command_copy_frame_bytes();
}
#endif
bool register_hold(const critical_command &original,
		   player_save_coin_replay_budget_scope_owner &scope,
		   collector_replay_budget &budget) noexcept
{
	size_t full = 0, owned = 0;
	if (!budget.current(full) ||
	    !player_save_sql_collector_replay_owner::current_storage_bytes(scope, &owned) ||
	    owned > budget.outer || owned > full)
		return false;
	return player_save_sql_collector_replay_owner::restore(original, scope, full - owned);
}
}

bool collector_purchase_cold_restore_owner::restore_bounded(
	const critical_command &original, collector_purchase_effect_fn effect,
	collector_completion_fn notify, player_save_coin_replay_budget_scope_owner &scope,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer_live) noexcept
{
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI || defined(_GLIBCXX_DEBUG)
	(void)original;
	(void)effect;
	(void)notify;
	(void)scope;
	(void)reserve;
	(void)context;
	(void)outer_live;
	return false;
#else
	using tree_type = decltype(purchases);
	using tree_node = std::_Rb_tree_node<typename tree_type::value_type>;
	const size_t frames =
		sizeof(economic_frozen_intent) + sizeof(collector_command_payload) +
		sizeof(collector::record) + 2 * sizeof(economic_account_key) +
		sizeof(pending_purchase) + sizeof(std::string) + sizeof(tree_type::iterator) +
		sizeof(std::pair<tree_type::iterator, bool>) + 14 * sizeof(void *) +
		11 * sizeof(size_t) + 8 * sizeof(bool) + critical_command_valid_frame_bytes() +
		critical_command_copy_frame_bytes();
	collector_replay_budget budget{ reserve, context, outer_live, frames };
	if (!budget.peak())
		return false;
	try
	{
		if (!nevent_is_game_thread() || !effect || !notify ||
#ifndef __NO_MYSQL__
		    persistence_mode_get() == PERSISTENCE_MODE_FLATFILE_PRIMARY ||
#else
		    persistence_mode_get() != PERSISTENCE_MODE_FLATFILE_PRIMARY ||
#endif
		    !original.publication_required ||
		    original.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION ||
		    !critical_command_envelope_valid(original))
			return false;
		economic_frozen_intent intent;
		budget.intent = &intent;
		collector_command_payload payload{};
		collector::record listing{};
		economic_account_key wallet{}, bank{};
		// The admitted intent supplies the original listing, not today's catalog
		// and not a result-only outbox event. No native completion exists at boot.
		if (collector_purchase_accounting_decode_bounded(
			    original, &intent, &payload, &listing, &wallet, &bank,
			    collector_replay_budget::child, &budget,
			    0) != economic_accounting_error::ok ||
		    !payload.actor_pid || payload.actor_pid > INT_MAX ||
		    payload.action != collector_action::purchase || payload.item_count != 1 ||
		    !payload.selected_item_uid)
			return false;
		auto found = purchases.find(original.operation_id.bytes);
		if (found != purchases.end())
		{
			auto &entry = found->second;
			// Even exact duplicates preserve the first seal, conflict flags and
			// native handler stages; no completion or effect is replaced here.
			if (!entry.command || !budget.equal(*entry.command, original) ||
			    entry.effect != effect || entry.notify != notify ||
			    !register_hold(original, scope, budget))
				return false;
			entry.restore_registration_pending = false;
			return true;
		}
		const size_t key_frames =
			sizeof(std::string) + original.operation_id.bytes.size() + 1 +
			// Original string constructor/_M_construct/_M_create/allocator/destruction.
			17 * sizeof(void *) + 10 * sizeof(size_t) + 3 * sizeof(bool) +
			critical_command_copy_frame_bytes();
		if (!budget.peak(key_frames))
			return false;
		std::string key = operation_key(original.operation_id);
		budget.key = &key;
		if (pending.size() + purchases.size() >= COLLECTOR_PENDING_MAX ||
		    player_pending(payload.actor_pid) || listing_pending(payload.listing) ||
		    pending.find(key) != pending.end())
			return false;
		pending_purchase entry;
		budget.entry = &entry;
		entry.actor_pid = payload.actor_pid;
		size_t copy_request = 0;
		if (!critical_command_fresh_copy_request_bytes(original, &copy_request) ||
		    !collector_replay_add(copy_request, sizeof(critical_command)) ||
		    !budget.peak(copy_request))
			return false;
		entry.command = std::make_unique<critical_command>(original);
		if (!budget.peak(sizeof(collector_command_payload) + 4 * sizeof(void *) +
				 sizeof(size_t)))
			return false;
		entry.payload = std::make_unique<collector_command_payload>(payload);
		entry.original_listing = listing;
		entry.effect = effect;
		entry.notify = notify;
		entry.restore_registration_pending = true;
		// All domain allocation precedes shared hold registration. Retain this
		// original on registration doubt, but do not let it publish until the
		// same original has successfully registered its exact prepared hold.
		// Exact fresh tree node; old tree and private unique owners stay live.
		// GCC13 selects the usable-key lower_bound/emplace_hint branch for this
		// actual const array key reference and pending_purchase rvalue argument.
		const size_t insertion_frames =
			sizeof(tree_node) + collector_hinted_emplace_source_frames();
		if (!budget.peak(insertion_frames))
			return false;
		const auto inserted =
			purchases.emplace(original.operation_id.bytes, std::move(entry));
		budget.entry = nullptr;
		if (!inserted.second || !register_hold(original, scope, budget))
			return false;
		inserted.first->second.restore_registration_pending = false;
		// Genuine full native completions arrive through handle_completions.
		// Startup registration performs no materialization, projection or ACK.
		return true;
	}
	catch (...)
	{
		return false;
	}
#endif
}
