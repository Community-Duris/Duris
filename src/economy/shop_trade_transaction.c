#include "economy/shop_trade_transaction.h"

#include "economy/currency_transaction.h"
#include "item/item_ownership_runtime.h"
#include "core/prototypes.h"
#include "economy/shop_trade_runtime.h"
#include "core/utils.h"

#include <algorithm>
#include <cerrno>
#include <new>
#include <string>
#include <unordered_map>

namespace
{
struct pending_trade
{
	uint32_t player_pid = 0;
	shop_trade_payload payload = {};
	shop_trade_completion_fn completion = nullptr;
	bool completion_ready = false;
	bool balances_published = false;
	bool ownership_published = false;
	bool shop_revision_published = false;
	bool publication_alerted = false;
	critical_completion completed = {};
};

std::unordered_map<std::string, pending_trade> pending;

std::string operation_key(const critical_operation_id &operation_id)
{
	return std::string(reinterpret_cast<const char *>(operation_id.bytes.data()),
			   operation_id.bytes.size());
}

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

bool publish(std::unordered_map<std::string, pending_trade>::iterator found, P_char character)
{
	pending_trade &entry = found->second;
	shop_trade_result result = {};
	const bool decoded = shop_trade_command_decode_result(entry.completed.result_payload.data(),
							      entry.completed.result_size, &result);
	const bool committed = decoded &&
			       (entry.completed.outcome == critical_apply_outcome::applied ||
				entry.completed.outcome == critical_apply_outcome::already_applied);
	const char *failure = nullptr;
	if (committed && !entry.shop_revision_published &&
	    !shop_trade_runtime_can_advance(entry.payload.shop_id,
					    entry.payload.expected_shop_revision,
					    result.shop_revision))
		failure = "shop_revision";
	if (!failure && committed && !entry.balances_published)
	{
		if (!currency_transaction_publish_balances(
			    character, entry.payload.account_name.data(), entry.payload.racewar,
			    result.wallet, result.bank, result.wallet_revision,
			    result.bank_revision))
			failure = "balances";
		else
			entry.balances_published = true;
	}
	if (!failure && committed && !entry.ownership_published)
	{
		if (!publish_ownership(entry.payload, result))
			failure = "ownership";
		else
			entry.ownership_published = true;
	}
	if (!failure && committed && !entry.shop_revision_published)
	{
		if (!shop_trade_runtime_advance(entry.payload.shop_id,
						entry.payload.expected_shop_revision,
						result.shop_revision))
			failure = "shop_revision";
		else
			entry.shop_revision_published = true;
	}
	if (!failure &&
	    !critical_command_coordinator_acknowledge_publication(entry.completed.operation_id))
		failure = "acknowledgement";
	if (failure)
	{
		if (!entry.publication_alerted)
		{
			logit(LOG_FILE,
			      "shop_trade: committed publication retained shop=%u pid=%u phase=%s",
			      entry.payload.shop_id, entry.player_pid, failure);
			entry.publication_alerted = true;
		}
		return false;
	}
	const auto completion = entry.completion;
	const shop_trade_payload payload = entry.payload;
	const unsigned int error_code = decoded ? entry.completed.error_code : EBADMSG;
	pending.erase(found);
	if (completion)
		completion(character, committed, decoded ? result : shop_trade_result{}, error_code,
			   payload);
	return committed;
}
} // namespace

bool shop_trade_transaction_submit(P_char character, const shop_trade_payload &payload,
				   shop_trade_completion_fn completion)
{
	if (!character || IS_NPC(character) || GET_PID(character) <= 0 ||
	    static_cast<uint32_t>(GET_PID(character)) != payload.player_pid ||
	    pending.size() >= SHOP_TRADE_PENDING_MAX || player_pending(payload.player_pid))
		return false;
	critical_operation_id operation_id = {};
	critical_command command = {};
	if (!critical_operation_id_generate(&operation_id) ||
	    !shop_trade_command_build(&command, operation_id, payload,
				      critical_source_site::command,
				      critical_deadline_class::interactive))
		return false;
	const std::string key = operation_key(operation_id);
	try
	{
		pending.emplace(key, pending_trade{ .player_pid = payload.player_pid,
						    .payload = payload,
						    .completion = completion });
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	const auto submitted =
		critical_command_coordinator_submit_for_publication(std::move(command));
	if (!critical_submit_result_keeps_operation(submitted))
	{
		pending.erase(key);
		return false;
	}
	return true;
}

void shop_trade_transaction_handle_completions(const critical_completion *completions, size_t count)
{
	if (count && !completions)
		return;
	for (size_t index = 0; index < count; ++index)
	{
		auto found = pending.find(operation_key(completions[index].operation_id));
		if (found == pending.end())
			continue;
		found->second.completed = completions[index];
		found->second.completion_ready = true;
	}
	for (auto found = pending.begin(); found != pending.end();)
	{
		auto current = found++;
		if (current->second.completion_ready)
			if (P_char character = find_player_by_pid(current->second.player_pid))
				publish(current, character);
	}
}

void shop_trade_transaction_player_ready(P_char character)
{
	if (!character || IS_NPC(character) || GET_PID(character) <= 0)
		return;
	for (auto found = pending.begin(); found != pending.end();)
	{
		auto current = found++;
		if (current->second.player_pid == static_cast<uint32_t>(GET_PID(character)) &&
		    current->second.completion_ready)
			publish(current, character);
	}
}

bool shop_trade_transaction_player_busy(P_char character)
{
	return character && !IS_NPC(character) && GET_PID(character) > 0 &&
	       player_pending(static_cast<uint32_t>(GET_PID(character)));
}

void shop_trade_transaction_reset_for_tests(void)
{
	pending.clear();
}
