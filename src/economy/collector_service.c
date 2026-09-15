#include "economy/collector_service.h"

#include "cmd/interp.h"
#include "core/prototypes.h"
#include "core/utility.h"
#include "core/utils.h"
#include "economy/auction_room_registry.h"
#include "economy/collector_catalog_cache.h"
#include "economy/collector_listing_pipeline.h"
#include "economy/collector_presence.h"
#include "economy/collector_purchase_preparation.h"
#include "economy/collector_runtime.h"
#include "economy/collector_transaction.h"
#include "economy/currency_transaction.h"
#include "item/item_movement_transaction.h"
#include "item/item_ownership_runtime.h"
#include "persistence/persistence_mode.h"
#include "player/player_load_items.h"
#include "player/player_snapshot_codec.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cinttypes>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <limits>
#include <memory>
#include <new>
#include <string>
#include <unordered_map>
#include <vector>

extern P_obj object_list;
extern P_room world;
extern const int top_of_world;
extern struct str_app_type str_app[];

namespace
{
enum class detail_intent : uint8_t
{
	inspect,
	purchase,
};

struct pending_detail
{
	uint32_t actor_pid = 0;
	uint64_t listing = 0;
	detail_intent intent = detail_intent::inspect;
	critical_operation_id operation_id = {};
};

std::unordered_map<uint64_t, pending_detail> pending_details;

struct pending_purchase_recovery
{
	uint32_t actor_pid = 0;
	std::unique_ptr<collector_command_payload> payload;
	collector_command_result result = {};
};

std::unordered_map<uint64_t, pending_purchase_recovery> purchase_recoveries;
constexpr size_t COLLECTOR_PURCHASE_RECOVERY_MAX = 128;
// Keep a second bounded retry path for the rare case where the keyed map is
// full.  The per-character runtime fence below still protects the player if
// even this payload-retaining array is exhausted.
constexpr size_t COLLECTOR_PURCHASE_FALLBACK_MAX = 256;
std::array<pending_purchase_recovery, COLLECTOR_PURCHASE_FALLBACK_MAX>
	purchase_fallback_recoveries = {};
collector_service_health health = {};

void clear_purchase_recovery_slot(pending_purchase_recovery &recovery)
{
	recovery.payload.reset();
	recovery.actor_pid = 0;
	recovery.result = {};
}

bool player_has_pending_detail(uint32_t pid, bool purchases_only)
{
	return std::any_of(pending_details.begin(), pending_details.end(),
			   [&](const auto &entry)
			   {
				   return entry.second.actor_pid == pid &&
					  (!purchases_only ||
					   entry.second.intent == detail_intent::purchase);
			   });
}

bool player_has_purchase_recovery(uint32_t pid)
{
	return pid &&
	       std::any_of(purchase_recoveries.begin(), purchase_recoveries.end(),
			   [pid](const auto &entry) { return entry.second.actor_pid == pid; });
}

bool player_has_fallback_purchase_recovery(uint32_t pid)
{
	return pid &&
	       std::any_of(purchase_fallback_recoveries.begin(), purchase_fallback_recoveries.end(),
			   [pid](const pending_purchase_recovery &recovery)
			   { return recovery.actor_pid == pid; });
}

bool player_has_purchase_save_fence(P_char character)
{
	return character && IS_PC(character) && GET_PID(character) > 0 &&
	       IS_SET(character->runtime_flags, CHAR_RFLAG_COLLECTOR_SAVE_FENCE);
}

void clear_purchase_save_fence_if_recovered(P_char character)
{
	if (!character || IS_NPC(character) || GET_PID(character) <= 0 ||
	    IS_SET(character->runtime_flags, CHAR_RFLAG_COLLECTOR_SAVE_RELOAD))
		return;
	const uint32_t pid = static_cast<uint32_t>(GET_PID(character));
	if (!player_has_purchase_recovery(pid) && !player_has_fallback_purchase_recovery(pid))
		REMOVE_BIT(character->runtime_flags, CHAR_RFLAG_COLLECTOR_SAVE_FENCE);
}

bool purchase_transaction_busy(P_char character)
{
	return currency_transaction_player_busy(character) ||
	       collector_transaction_player_busy(character) ||
	       item_movement_transaction_player_busy(character) ||
	       bulk_get_player_busy(character) ||
	       (character && GET_PID(character) > 0 &&
		(player_has_purchase_recovery(static_cast<uint32_t>(GET_PID(character))) ||
		 player_has_purchase_save_fence(character))) ||
	       !currency_transaction_can_submit_nonrebasable(character);
}

bool parse_listing(const char *text, uint64_t *listing)
{
	if (!text || !listing)
		return false;
	while (*text == ' ')
		++text;
	if (!*text || *text == '-')
		return false;
	char *end = nullptr;
	errno = 0;
	const unsigned long long value = strtoull(text, &end, 10);
	while (end && *end == ' ')
		++end;
	if (errno || !end || end == text || *end || !value)
		return false;
	*listing = static_cast<uint64_t>(value);
	return true;
}

bool collector_accessible(P_char character, bool report)
{
	if (!character || !IS_PC(character) || !IS_ALIVE(character))
		return false;
	if (IS_FIGHTING(character))
	{
		if (report)
			send_to_char("You are too busy fighting to consult the collector.\r\n",
				     character);
		return false;
	}
	if (character->in_room < 0 || !collector_presence_room_active(character->in_room))
	{
		if (report)
			send_to_char("The Collector of Antiquities is not available here right "
				     "now.\r\n",
				     character);
		return false;
	}
	if (!collector_catalog_cache_ready())
	{
		if (report)
			send_to_char("The collector's records are unavailable right now.\r\n",
				     character);
		return false;
	}
	return true;
}

std::string currency_text(uint64_t value)
{
	const uint64_t platinum = value / 1000;
	value %= 1000;
	const uint64_t gold = value / 100;
	value %= 100;
	const uint64_t silver = value / 10;
	const uint64_t copper = value % 10;
	std::string text;
	try
	{
		if (platinum)
			text += std::to_string(platinum) + " platinum";
		if (gold)
			text += (text.empty() ? "" : ", ") + std::to_string(gold) + " gold";
		if (silver)
			text += (text.empty() ? "" : ", ") + std::to_string(silver) + " silver";
		if (copper || text.empty())
			text += (text.empty() ? "" : ", ") + std::to_string(copper) + " copper";
	}
	catch (const std::bad_alloc &)
	{
		return "an unavailable amount";
	}
	return text;
}

uint64_t seconds_remaining(uint64_t deadline, uint64_t now)
{
	return deadline > now ? deadline - now : 0;
}

void show_help(P_char character)
{
	send_to_char("&+WCollector commands:&n\r\n"
		     "  collector list             list your available antiquities\r\n"
		     "  collector inspect <number> inspect one exact item\r\n"
		     "  collector buy <number>     buy back one item with carried coins\r\n",
		     character);
}

void show_list(P_char character)
{
	std::vector<collector::record> entries;
	if (!collector_runtime_available_for(static_cast<uint32_t>(GET_PID(character)), 100,
					     &entries))
	{
		send_to_char("The collector cannot consult the catalog right now.\r\n", character);
		return;
	}
	if (entries.empty())
	{
		send_to_char("The collector is holding no antiquities for you.\r\n", character);
		return;
	}
	const uint64_t now = static_cast<uint64_t>(time(nullptr));
	send_to_char("&+WYour antiquities:&n\r\n", character);
	for (const collector::record &entry : entries)
	{
		const std::string price = currency_text(entry.price_value);
		const uint64_t remaining = seconds_remaining(entry.expires_at, now);
		send_to_char_f(character,
			       "  &+C%" PRIu64 "&n  item UID %" PRIu64 "  %s  (%" PRIu64
			       "h %" PRIu64 "m remaining)\r\n",
			       entry.listing, entry.uid, price.c_str(), remaining / 3600,
			       (remaining / 60) % 60);
	}
	if (entries.size() == 100)
		send_to_char("Only the first 100 entries are shown.\r\n", character);
}

bool submit_detail(P_char character, uint64_t listing, detail_intent intent)
{
	const uint32_t pid = static_cast<uint32_t>(GET_PID(character));
	collector::record runtime_entry;
	if (!collector_runtime_find(listing, &runtime_entry) ||
	    runtime_entry.status != collector::state::available || runtime_entry.holding_paused ||
	    runtime_entry.beneficiary != pid)
	{
		++health.rejected_details;
		send_to_char("That antiquity is not available to you.\r\n", character);
		return false;
	}
	if (player_has_pending_detail(pid, false))
	{
		send_to_char("The collector is already checking a record for you.\r\n", character);
		return false;
	}
	if (intent == detail_intent::purchase && purchase_transaction_busy(character))
	{
		send_to_char("Another transaction is still being settled for you.\r\n", character);
		return false;
	}
	critical_operation_id operation_id = {};
	if (intent == detail_intent::purchase && !critical_operation_id_generate(&operation_id))
	{
		send_to_char("The collector cannot begin a purchase right now.\r\n", character);
		return false;
	}
	const uint64_t request_id = collector_listing_pipeline_next_request_id();
	if (!request_id)
	{
		send_to_char("The collector's records are busy right now.\r\n", character);
		return false;
	}
	try
	{
		if (!pending_details
			     .emplace(request_id,
				      pending_detail{ pid, listing, intent, operation_id })
			     .second)
		{
			send_to_char("The collector's records are busy right now.\r\n", character);
			return false;
		}
	}
	catch (const std::bad_alloc &)
	{
		send_to_char("The collector's records are busy right now.\r\n", character);
		return false;
	}
	const collector_listing_submit_outcome submitted =
		collector_listing_pipeline_submit({ request_id, listing });
	if (submitted != collector_listing_submit_outcome::accepted)
	{
		pending_details.erase(request_id);
		++health.rejected_details;
		send_to_char("The collector's records are busy right now.\r\n", character);
		return false;
	}
	++health.submitted_details;
	send_to_char(intent == detail_intent::purchase ?
			     "The collector begins verifying that antiquity.\r\n" :
			     "The collector consults the item's exact record.\r\n",
		     character);
	return true;
}

bool wallet_value(P_char character, uint64_t *value)
{
	if (!character || !value)
		return false;
	const std::array<int64_t, CURRENCY_DENOMINATION_COUNT> amounts = {
		GET_COPPER(character), GET_SILVER(character), GET_GOLD(character),
		GET_PLATINUM(character)
	};
	constexpr std::array<uint64_t, CURRENCY_DENOMINATION_COUNT> values = { 1, 10, 100, 1000 };
	uint64_t total = 0;
	for (size_t index = 0; index < amounts.size(); ++index)
	{
		if (amounts[index] < 0 ||
		    static_cast<uint64_t>(amounts[index]) >
			    (std::numeric_limits<uint64_t>::max() - total) / values[index])
			return false;
		total += static_cast<uint64_t>(amounts[index]) * values[index];
	}
	*value = total;
	return true;
}

bool fill_actor_snapshot(P_char character, uint64_t now, collector_purchase_actor_snapshot *actor)
{
	if (!character || !actor || !character->only.pc || GET_PID(character) <= 0 || !now)
		return false;
	const char *account_name = get_account_name_safe(character);
	if (!account_name)
		return false;
	const size_t account_length = strlen(account_name);
	if (!account_length || account_length >= actor->account_name.size())
		return false;
	collector_purchase_actor_snapshot candidate;
	candidate.pid = static_cast<uint32_t>(GET_PID(character));
	candidate.racewar = static_cast<uint8_t>(GET_RACEWAR(character));
	memcpy(candidate.account_name.data(), account_name, account_length + 1);
	candidate.wallet_revision = character->only.pc->wallet_revision;
	candidate.bank_revision = character->only.pc->bank_revision;
	const item_owner_identity owner = { item_owner_type::player, candidate.pid, 0 };
	if (!item_ownership_runtime_owner_revision(owner, &candidate.item_owner_revision) ||
	    !wallet_value(character, &candidate.carried_currency_value))
		return false;
	candidate.carried_weight = total_carried_weight(character);
	candidate.carry_weight_limit = CAN_CARRY_W(character);
	candidate.carried_items = static_cast<size_t>(IS_CARRYING_N(character));
	candidate.carry_item_limit = static_cast<size_t>(std::max(0, CAN_CARRY_N(character)));
	candidate.observed_at = now;
	*actor = candidate;
	return true;
}

void show_inspection(P_char character, const collector::record &entry,
		     const player_item_snapshot &item)
{
	const std::string price = currency_text(entry.price_value);
	const uint64_t now = static_cast<uint64_t>(time(nullptr));
	const uint64_t remaining = seconds_remaining(entry.expires_at, now);
	send_to_char_f(character,
		       "&+WCollector listing %" PRIu64 "&n\r\n"
		       "  %s\r\n"
		       "  UID: %" PRIu64 "   prototype: %d\r\n"
		       "  condition: %d   weight: %d   stored base value: %d copper\r\n"
		       "  buyback price: %s\r\n"
		       "  holding time remaining: %" PRIu64 "h %" PRIu64 "m\r\n",
		       entry.listing,
		       item.short_description.empty() ? "an unnamed antiquity" :
							item.short_description.c_str(),
		       item.object_uid, item.vnum, static_cast<int>(item.condition), item.weight,
		       item.cost, price.c_str(), remaining / 3600, (remaining / 60) % 60);
}

P_obj find_live_item(uint64_t uid)
{
	for (P_obj object = object_list; object; object = object->next)
		if (object->obj_uid == uid)
			return object;
	return nullptr;
}

bool materialize_purchase(P_char character, const collector_command_result &result,
			  const collector_command_payload &payload)
{
	if (!character || result.action != collector_action::purchase || !result.record_present ||
	    result.entry.status != collector::state::purchased ||
	    result.entry.uid != payload.selected_item_uid || payload.item_count != 1 ||
	    result.entry.item_revision != payload.items[0].expected_item_revision + 1 ||
	    !result.to_owner_revision)
		return false;
	if (P_obj existing = find_live_item(result.entry.uid))
		return OBJ_CARRIED_BY(existing, character);
	std::vector<player_item_snapshot> items;
	if (player_item_snapshot_list_decode(payload.item_blob.data(), payload.item_blob_size,
					     &items) != player_snapshot_codec_result::ok ||
	    items.size() != 1 || items[0].parent_index != PLAYER_SNAPSHOT_NO_PARENT ||
	    items[0].equipment_slot != 0 || items[0].object_uid != result.entry.uid ||
	    items[0].vnum != payload.items[0].vnum)
		return false;
	const item_owner_identity owner = { item_owner_type::player, payload.actor_pid, 0 };
	const bool flatfile_primary = persistence_mode_get() == PERSISTENCE_MODE_FLATFILE_PRIMARY;
	// Flat-file materialization allocates its own snapshot identity.  The
	// temporary graph identity is deliberately non-SQL, while a SQL result must
	// carry the actual auto-increment id returned by the repository.  Select by
	// runtime persistence mode so a client-capable binary cannot accidentally
	// reject a valid flat-file-primary purchase.
	const uint32_t database_id = flatfile_primary ? 1 : result.materialized_item_id;
	if (!flatfile_primary && !database_id)
		return false;
	std::vector<player_load_item_identity> identities = {
		{ database_id, 0, 1, PLAYER_LOAD_ITEM_OVERRIDE_ALL, result.entry.uid,
		  result.entry.uid, 0, owner, result.entry.item_revision, result.to_owner_revision,
		  item_custody_state::active }
	};
	player_load_item_materialize_metrics metrics = {};
	if (!player_load_item_graph_materialize_for_owner(character, items, identities, owner,
							  result.to_owner_revision, false, true,
							  &metrics))
		return false;
	if (flatfile_primary)
	{
		P_obj materialized = find_live_item(result.entry.uid);
		if (!materialized || !OBJ_CARRIED_BY(materialized, character))
			return false;
		// Flat-file player snapshots allocate their own per-snapshot row
		// identities. Never let this temporary graph identity masquerade as a
		// SQL row ID.
		materialized->db_item_id = 0;
	}
	return true;
}

bool queue_purchase_fallback(const collector_command_result &result,
			     const collector_command_payload &payload)
{
	if (!payload.actor_pid)
		return false;
	auto available = std::find_if(purchase_fallback_recoveries.begin(),
				      purchase_fallback_recoveries.end(),
				      [](const pending_purchase_recovery &recovery)
				      { return !recovery.actor_pid; });
	if (available == purchase_fallback_recoveries.end())
		return false;
	try
	{
		auto retained_payload = std::make_unique<collector_command_payload>(payload);
		available->actor_pid = payload.actor_pid;
		available->payload = std::move(retained_payload);
		available->result = result;
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	return false;
}

bool queue_purchase_recovery(const collector_command_result &result,
			     const collector_command_payload &payload)
{
	const uint64_t recovery_key = payload.selected_item_uid ? payload.selected_item_uid :
								  payload.listing;
	if (!payload.actor_pid)
		return false;
	if (recovery_key && (purchase_recoveries.size() < COLLECTOR_PURCHASE_RECOVERY_MAX ||
			     purchase_recoveries.find(recovery_key) != purchase_recoveries.end()))
	{
		try
		{
			auto retained_payload =
				std::make_unique<collector_command_payload>(payload);
			purchase_recoveries.insert_or_assign(
				recovery_key,
				pending_purchase_recovery{ payload.actor_pid,
							   std::move(retained_payload), result });
			return true;
		}
		catch (const std::bad_alloc &)
		{
			// A second bounded payload path below still gives the online buyer a
			// retryable recovery instead of relying only on a save fence.
		}
	}
	return queue_purchase_fallback(result, payload);
}

bool recover_purchase_for_player(P_char character)
{
	if (!character || IS_NPC(character) || GET_PID(character) <= 0)
		return true;
	const uint32_t pid = static_cast<uint32_t>(GET_PID(character));
	bool complete = true;
	for (auto found = purchase_recoveries.begin(); found != purchase_recoveries.end();)
	{
		auto current = found++;
		if (current->second.actor_pid != pid)
			continue;
		if (!current->second.payload ||
		    !materialize_purchase(character, current->second.result,
					  *current->second.payload))
		{
			complete = false;
			continue;
		}
		send_to_char(
			"Your committed collector purchase is now available in your inventory.\r\n",
			character);
		purchase_recoveries.erase(current);
		clear_purchase_save_fence_if_recovered(character);
	}
	for (pending_purchase_recovery &recovery : purchase_fallback_recoveries)
	{
		if (recovery.actor_pid != pid)
			continue;
		if (!recovery.payload ||
		    !materialize_purchase(character, recovery.result, *recovery.payload))
		{
			complete = false;
			continue;
		}
		send_to_char(
			"Your committed collector purchase is now available in your inventory.\r\n",
			character);
		clear_purchase_recovery_slot(recovery);
	}
	const bool fallback_recovery_pending = player_has_fallback_purchase_recovery(pid);
	if (fallback_recovery_pending ||
	    IS_SET(character->runtime_flags, CHAR_RFLAG_COLLECTOR_SAVE_RELOAD))
		complete = false;
	if (!fallback_recovery_pending)
		clear_purchase_save_fence_if_recovered(character);
	return complete;
}

void purchase_completed(P_char character, bool committed, const collector_command_result &result,
			unsigned int error_code, const collector_command_payload &payload)
{
	if (!character)
		return;
	if (!committed)
	{
		++health.rejected_purchases;
		if (error_code == ENOSPC)
			send_to_char(
				"You no longer have enough carried currency for that item.\r\n",
				character);
		else if (error_code == ENOBUFS)
			send_to_char("You no longer have capacity to carry that item.\r\n",
				     character);
		else if (error_code == EACCES)
			send_to_char("That antiquity is not yours to buy.\r\n", character);
		else if (error_code == ESTALE || error_code == EAGAIN || error_code == ENOENT)
			send_to_char("That listing changed before the purchase could commit.\r\n",
				     character);
		else
			send_to_char("The purchase did not commit; your currency is unchanged.\r\n",
				     character);
		return;
	}
	++health.committed_purchases;
	if (error_code || !materialize_purchase(character, result, payload))
	{
		++health.materialization_failures;
		persistence_alert(AVATAR, "collector", "player", "redacted", "purchase_publish",
				  "live_materialization_failed", "listing=%" PRIu64 " uid=%" PRIu64,
				  payload.listing, payload.selected_item_uid);
		const bool retry_queued = queue_purchase_recovery(result, payload);
		if (!retry_queued)
		{
			SET_BIT(character->runtime_flags, CHAR_RFLAG_COLLECTOR_SAVE_FENCE);
			SET_BIT(character->runtime_flags, CHAR_RFLAG_COLLECTOR_SAVE_RELOAD);
		}
		else
		{
			SET_BIT(character->runtime_flags, CHAR_RFLAG_COLLECTOR_SAVE_FENCE);
			REMOVE_BIT(character->runtime_flags, CHAR_RFLAG_COLLECTOR_SAVE_RELOAD);
		}
		send_to_char(
			retry_queued ?
				"Your purchase committed, but live restoration is still pending. The item remains safely recorded; this session will not be saved until restoration succeeds.\r\n" :
				"Your purchase committed, but its live restoration needs operator recovery. The item remains safely recorded; do not assume it is present in this session.\r\n",
			character);
		return;
	}
	const std::string price = currency_text(result.entry.price_value);
	send_to_char_f(character, "You buy back %s for %s.\r\n",
		       payload.item_count ? "your antiquity" : "the antiquity", price.c_str());
}

void report_prepare_failure(P_char character, collector_purchase_prepare_outcome outcome)
{
	switch (outcome)
	{
	case collector_purchase_prepare_outcome::prepared:
		return;
	case collector_purchase_prepare_outcome::forbidden:
		send_to_char("That antiquity is not yours to buy.\r\n", character);
		return;
	case collector_purchase_prepare_outcome::insufficient_funds:
		send_to_char("You do not have enough carried currency for that antiquity.\r\n",
			     character);
		return;
	case collector_purchase_prepare_outcome::capacity:
		send_to_char("You cannot carry that antiquity.\r\n", character);
		return;
	case collector_purchase_prepare_outcome::unavailable:
	case collector_purchase_prepare_outcome::stale_listing:
	case collector_purchase_prepare_outcome::stale_custody:
		send_to_char("That listing changed while the collector was checking it.\r\n",
			     character);
		return;
	case collector_purchase_prepare_outcome::invalid_actor:
	case collector_purchase_prepare_outcome::invalid_item:
	case collector_purchase_prepare_outcome::allocation_failure:
		send_to_char("The collector cannot verify that antiquity right now.\r\n",
			     character);
		return;
	}
}

void handle_found(P_char character, const pending_detail &request,
		  const collector_listing_detail &detail)
{
	collector::record runtime_entry;
	if (!collector_runtime_find(request.listing, &runtime_entry) ||
	    runtime_entry.status != collector::state::available || runtime_entry.holding_paused ||
	    runtime_entry.beneficiary != static_cast<uint32_t>(GET_PID(character)))
	{
		send_to_char("That antiquity is no longer available to you.\r\n", character);
		return;
	}
	if (request.intent == detail_intent::inspect)
	{
		player_item_snapshot item;
		const collector_listing_decode_outcome decoded =
			collector_listing_decode_item(runtime_entry, detail, &item);
		if (decoded != collector_listing_decode_outcome::decoded)
		{
			++health.rejected_details;
			send_to_char("The collector cannot verify that antiquity right now.\r\n",
				     character);
			return;
		}
		show_inspection(character, runtime_entry, item);
		return;
	}
	if (purchase_transaction_busy(character))
	{
		send_to_char("Another transaction is still being settled for you.\r\n", character);
		return;
	}
	item_ownership_runtime_entry held_item = {};
	collector_purchase_actor_snapshot actor;
	const uint64_t now = static_cast<uint64_t>(time(nullptr));
	if (!item_ownership_runtime_lookup(runtime_entry.uid, &held_item) ||
	    !fill_actor_snapshot(character, now, &actor))
	{
		send_to_char("The collector cannot verify custody for that antiquity.\r\n",
			     character);
		return;
	}
	std::unique_ptr<collector_command_payload> payload;
	player_item_snapshot item;
	const collector_purchase_prepare_outcome prepared = collector_purchase_prepare(
		runtime_entry, detail, held_item, actor, &payload, &item);
	if (prepared != collector_purchase_prepare_outcome::prepared)
	{
		report_prepare_failure(character, prepared);
		return;
	}
	if (!payload || !collector_transaction_submit_identified(character, request.operation_id,
								 *payload, purchase_completed))
	{
		++health.rejected_purchases;
		send_to_char("The collector could not queue that purchase; nothing changed.\r\n",
			     character);
		return;
	}
	++health.submitted_purchases;
	send_to_char("The collector is committing your purchase.\r\n", character);
}

void handle_detail_result(collector_listing_result result)
{
	auto found = pending_details.find(result.request_id);
	if (found == pending_details.end())
	{
		++health.stale_results;
		return;
	}
	const pending_detail request = found->second;
	pending_details.erase(found);
	++health.completed_details;
	if (request.listing != result.listing)
	{
		++health.stale_results;
		return;
	}
	P_char character = find_player_by_pid(static_cast<int>(request.actor_pid));
	if (!character)
	{
		++health.abandoned_offline;
		return;
	}
	if (!collector_accessible(character, true))
		return;
	switch (result.outcome)
	{
	case collector_listing_outcome::found:
		handle_found(character, request, result.detail);
		return;
	case collector_listing_outcome::not_found:
		send_to_char("That collector listing no longer exists.\r\n", character);
		return;
	case collector_listing_outcome::retryable_failure:
		send_to_char("The collector's records are temporarily unavailable.\r\n", character);
		return;
	case collector_listing_outcome::invalid_data:
		++health.rejected_details;
		persistence_alert(AVATAR, "collector", "listing", "redacted", "detail_read",
				  "invalid_data", "listing=%" PRIu64, request.listing);
		send_to_char("The collector found an invalid record and refused the request.\r\n",
			     character);
		return;
	case collector_listing_outcome::cancelled:
		return;
	}
}
} // namespace

void collector_service_command(P_char character, char *arguments, int command)
{
	if (command != CMD_COLLECTOR || !collector_accessible(character, true))
		return;
	char action[MAX_INPUT_LENGTH] = {};
	char remainder[MAX_STRING_LENGTH] = {};
	half_chop(arguments ? arguments : const_cast<char *>(""), action, remainder);
	if (isname(action, "list l"))
	{
		show_list(character);
		return;
	}
	if (isname(action, "inspect i") || isname(action, "buy b"))
	{
		uint64_t listing = 0;
		if (!parse_listing(remainder, &listing))
		{
			send_to_char("Which numeric collector listing do you mean?\r\n", character);
			return;
		}
		submit_detail(character, listing,
			      isname(action, "buy b") ? detail_intent::purchase :
							detail_intent::inspect);
		return;
	}
	show_help(character);
}

void collector_service_pulse(void)
{
	for (auto found = purchase_recoveries.begin(); found != purchase_recoveries.end();)
	{
		auto current = found++;
		P_char character = find_player_by_pid(current->second.actor_pid);
		if (!character || !current->second.payload ||
		    !materialize_purchase(character, current->second.result,
					  *current->second.payload))
			continue;
		send_to_char(
			"Your committed collector purchase is now available in your inventory.\r\n",
			character);
		purchase_recoveries.erase(current);
		clear_purchase_save_fence_if_recovered(character);
	}
	for (pending_purchase_recovery &recovery : purchase_fallback_recoveries)
	{
		if (!recovery.actor_pid)
			continue;
		P_char character = find_player_by_pid(recovery.actor_pid);
		if (!character || !recovery.payload ||
		    !materialize_purchase(character, recovery.result, *recovery.payload))
			continue;
		send_to_char(
			"Your committed collector purchase is now available in your inventory.\r\n",
			character);
		clear_purchase_recovery_slot(recovery);
		clear_purchase_save_fence_if_recovered(character);
	}
	collector_listing_result results[COLLECTOR_LISTING_MAX_COMPLETIONS] = {};
	const size_t count = collector_listing_pipeline_pulse_for(
		collector_listing_consumer::player, results, COLLECTOR_LISTING_MAX_COMPLETIONS);
	for (size_t index = 0; index < count; ++index)
		handle_detail_result(std::move(results[index]));
}

bool collector_service_player_busy(P_char character)
{
	return character && IS_PC(character) && GET_PID(character) > 0 &&
	       (player_has_pending_detail(static_cast<uint32_t>(GET_PID(character)), true) ||
		player_has_purchase_recovery(static_cast<uint32_t>(GET_PID(character))) ||
		player_has_purchase_save_fence(character));
}

collector_service_health collector_service_health_copy(void)
{
	collector_service_health snapshot = health;
	snapshot.pending_details = pending_details.size();
	return snapshot;
}

void collector_service_reset_for_tests(void)
{
	for (const auto &[request_id, request] : pending_details)
	{
		(void)request;
		collector_listing_pipeline_cancel(request_id);
	}
	pending_details.clear();
	purchase_recoveries.clear();
	for (pending_purchase_recovery &recovery : purchase_fallback_recoveries)
		clear_purchase_recovery_slot(recovery);
	health = {};
}

void collector_service_player_ready(P_char character, bool inventory_reloaded)
{
	if (!character || IS_NPC(character) || GET_PID(character) <= 0)
		return;
	// A payload-backed retry can be resolved immediately after the normal item
	// load.  The per-character reload bit covers only the case where even the
	// bounded payload paths could not retain a recovery.
	(void)recover_purchase_for_player(character);
	// A cold login has just hydrated the authoritative player_items snapshot.
	// A reconnect only reuses the existing graph, so clearing the fence there
	// would re-open the exact save race this guard is meant to prevent.
	const uint32_t pid = static_cast<uint32_t>(GET_PID(character));
	if (inventory_reloaded && !player_has_purchase_recovery(pid) &&
	    !player_has_fallback_purchase_recovery(pid))
	{
		REMOVE_BIT(character->runtime_flags, CHAR_RFLAG_COLLECTOR_SAVE_FENCE);
		REMOVE_BIT(character->runtime_flags, CHAR_RFLAG_COLLECTOR_SAVE_RELOAD);
	}
}

bool collector_service_recover_player(P_char character)
{
	return recover_purchase_for_player(character) && !player_has_purchase_save_fence(character);
}
