#include "economy/shop_trade_publication.h"

#include "economy/currency_transaction.h"
#include "item/item_ownership_runtime.h"
#include "core/prototypes.h"
#include "economy/shop_trade_runtime.h"
#include "core/utils.h"

#include <algorithm>
#include <cerrno>
#include <new>
#include <map>

namespace
{
struct pending_trade
{
	uint32_t player_pid = 0;
	shop_trade_payload payload = {};
	shop_trade_completion_fn completion = nullptr;
	bool completion_ready = false;
	critical_completion completed = {};
	shop_trade_physical_publication_fn publication = nullptr;
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
	entry.publishing = true;
	try
	{
		if (committed && !entry.revision_published &&
		    !shop_trade_runtime_can_advance(entry.payload.shop_id,
						    entry.payload.expected_shop_revision,
						    result.shop_revision))
		{
			entry.publishing = false;
			return false;
		}
		if (decoded && (!entry.balances_published ||
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
		if (committed && !entry.custody_published)
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
		if (committed && !entry.revision_published)
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
		if (committed && !entry.physical_published)
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
} // namespace

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
		if (entry.completion_ready && !same_receipt(entry.completed, completions[index]))
		{
			const bool original_committed =
				entry.completed.outcome == critical_apply_outcome::applied ||
				entry.completed.outcome == critical_apply_outcome::already_applied;
			if (original_committed || entry.publishing || entry.blocked)
			{
				entry.blocked = true;
				continue;
			}
		}
		// An exact readback may say already applied; keep the first committed
		// receipt and projection stages while accepting that equivalent proof.
		if (!entry.completion_ready ||
		    (entry.completed.outcome != critical_apply_outcome::applied &&
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
		if (P_char character = find_player_by_pid(entry.player_pid))
			publish(found, character);
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

void shop_trade_transaction_reset_for_tests(void)
{
	pending.clear();
	notifying = 0;
}
