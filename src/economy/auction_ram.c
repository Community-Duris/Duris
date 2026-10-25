#include "economy/auction_ram.h"
#include "item/item_transfer_command.h"

#include <algorithm>
#include <cerrno>
#include <climits>
#include <strings.h>

namespace
{
unsigned int wallet_delta(const currency_command_result &before, int64_t delta,
			  auction_command_result *result)
{
	constexpr int64_t values[]{ 1, 10, 100, 1000 };
	int64_t total = 0;
	for (size_t i = 0; i < 4; ++i)
	{
		if (before.wallet.amount[i] < 0 || before.wallet.amount[i] > INT_MAX)
			return ERANGE;
		total += before.wallet.amount[i] * values[i];
	}
	if (delta < -total)
		return ENOSPC;
	if (delta > INT64_MAX - total || before.wallet_revision == UINT64_MAX ||
	    before.bank_revision == UINT64_MAX)
		return ERANGE;
	total += delta;
	for (size_t i = 4; i-- > 0;)
	{
		result->wallet.amount[i] = total / values[i];
		total %= values[i];
		if (result->wallet.amount[i] > INT_MAX)
			return ERANGE;
	}
	result->wallet_value_delta = delta;
	result->wallet_revision = before.wallet_revision + 1;
	result->bank_revision = before.bank_revision + 1;
	return 0;
}

int64_t proceeds(int64_t price, uint32_t fee)
{
	return price - (price / 10000) * fee - ((price % 10000) * fee) / 10000;
}
} // namespace

unsigned int auction_ram_prepare(const critical_command &command,
				 const auction_ram_listing *current,
				 const currency_command_result &balances,
				 const auction_ram_pickup &pickup, uint64_t player_owner_revision,
				 uint64_t auction_owner_revision, auction_ram_listing *next,
				 auction_command_result *result)
{
	auction_command_payload payload{};
	if (!next || !result || !auction_command_decode_payload(command, &payload) ||
	    payload.closing_fee_basis_points > 10000 || !command.accepted_at_usec)
		return EINVAL;
	if (payload.actor_pid && ((payload.expected_wallet_revision != UINT64_MAX &&
				   payload.expected_wallet_revision != balances.wallet_revision) ||
				  (payload.expected_bank_revision != UINT64_MAX &&
				   payload.expected_bank_revision != balances.bank_revision)))
		return ESTALE;
	*result = {};
	result->action = payload.action;
	result->wallet = balances.wallet;
	result->bank = balances.bank;
	result->wallet_revision = balances.wallet_revision;
	result->bank_revision = balances.bank_revision;
	*next = current ? *current : auction_ram_listing{};
	const uint64_t now = command.accepted_at_usec / 1000000;
	if (payload.action == auction_action::claim_money)
	{
		if (pickup.amount <= 0 || pickup.amount > UINT_MAX)
			return ENOENT;
		if (pickup.revision == UINT64_MAX)
			return ERANGE;
		if (auto error = wallet_delta(balances, pickup.amount, result))
			return error;
		result->auction_revision = result->wallet_revision;
		result->event_type = auction_event_type::money_claimed;
		return 0;
	}
	if (payload.action == auction_action::list)
	{
		if (current)
			return EEXIST;
		if (payload.start_price < 0 || payload.start_price > UINT_MAX ||
		    payload.buy_price < 0 || payload.buy_price > INT_MAX ||
		    (payload.buy_price && payload.buy_price < payload.start_price) ||
		    payload.listing_fee < 0 || !payload.object_blob_size ||
		    payload.end_time <= now || payload.end_time > INT32_MAX)
			return EINVAL;
		next->id = auction_command_listing_id(command.operation_id);
		next->seller = payload.actor_pid;
		next->seller_account = payload.account_name.data();
		next->status = 1;
		next->price = payload.start_price;
		next->buy_price = payload.buy_price;
		next->revision = 1;
		next->end_time = payload.end_time;
		next->object_blob.assign(payload.object_blob.begin(),
					 payload.object_blob.begin() + payload.object_blob_size);
		for (size_t i = 0; i < payload.item_count; ++i)
			next->items.push_back({ payload.items[i].item_uid,
						payload.items[i].expected_item_revision,
						payload.items[i].vnum, 0, false });
		if (auto error = wallet_delta(balances, -payload.listing_fee, result))
			return error;
		result->event_type = auction_event_type::listed;
	}
	else
	{
		if (!current)
			return ENOENT;
		if (current->revision == UINT64_MAX)
			return ERANGE;
		if (payload.action == auction_action::bid)
		{
			if (current->status != 1 || current->end_time <= now ||
			    current->seller == payload.actor_pid ||
			    !strcasecmp(current->seller_account.c_str(),
					payload.account_name.data()))
				return EACCES;
			int64_t bid = payload.value;
			if (current->buy_price > 0 && bid >= current->buy_price)
				bid = current->buy_price;
			if (bid <= 0 || bid > UINT_MAX ||
			    (!current->winner && bid < current->price) ||
			    (current->winner && bid <= current->price))
				return EINVAL;
			const int64_t debit =
				current->winner == payload.actor_pid ? bid - current->price : bid;
			if (auto error = wallet_delta(balances, -debit, result))
				return error;
			result->previous_bidder_pid = current->winner;
			if (current->winner && current->winner != payload.actor_pid)
				result->refund = current->price;
			next->winner = payload.actor_pid;
			next->price = bid;
			const bool sold = current->buy_price && bid >= current->buy_price;
			if (sold)
			{
				next->status = 2;
				result->seller_proceeds =
					proceeds(bid, payload.closing_fee_basis_points);
				for (auto &item : next->items)
					if (!item.claimed && !item.claimant)
						item.claimant = payload.actor_pid;
			}
			else if (current->winner != payload.actor_pid)
			{
				if (current->end_time > UINT64_MAX - payload.bid_extension_seconds)
					return ERANGE;
				next->end_time += payload.bid_extension_seconds;
				if (next->end_time > INT32_MAX)
					return ERANGE;
			}
			result->event_type = sold ? auction_event_type::sold :
						    auction_event_type::bid_placed;
		}
		else if (payload.action == auction_action::remove ||
			 payload.action == auction_action::finalize)
		{
			if (current->status != 1)
				return EALREADY;
			if (payload.action == auction_action::finalize && current->end_time > now)
				return EAGAIN;
			const bool removed = payload.action == auction_action::remove;
			next->status = removed ? 3 : 2;
			if (current->winner)
			{
				if (removed)
					result->refund = current->price;
				else
					result->seller_proceeds = proceeds(
						current->price, payload.closing_fee_basis_points);
			}
			for (auto &item : next->items)
				if (!item.claimed && !item.claimant)
					item.claimant = removed || !current->winner ?
								current->seller :
								current->winner;
			result->event_type = removed	     ? auction_event_type::removed :
					     current->winner ? auction_event_type::sold :
							       auction_event_type::expired;
		}
		else if (payload.action != auction_action::claim_item)
			return EINVAL;
		++next->revision;
	}
	if (payload.action == auction_action::list || payload.action == auction_action::claim_item)
	{
		item_transfer_payload transfer{};
		if (!auction_item_payload(payload, next->id, &transfer))
			return EINVAL;
		if (payload.action == auction_action::claim_item)
		{
			auction_command_payload stored{};
			if (next->object_blob.size() >= stored.object_blob.size())
				return EINVAL;
			stored.object_blob_size = next->object_blob.size();
			std::copy(next->object_blob.begin(), next->object_blob.end(),
				  stored.object_blob.begin());
			const uint8_t *old_text, *old_runtime, *text, *runtime;
			size_t old_size, old_runtime_size, text_size, runtime_size;
			if (!auction_blob_parts(stored, &old_text, &old_size, &old_runtime,
						&old_runtime_size) ||
			    !auction_blob_parts(payload, &text, &text_size, &runtime,
						&runtime_size) ||
			    old_size != text_size ||
			    !std::equal(old_text, old_text + old_size, text) ||
			    (old_runtime_size &&
			     (old_runtime_size != runtime_size ||
			      !std::equal(old_runtime, old_runtime + old_runtime_size, runtime))))
				return ESTALE;
		}
		if (player_owner_revision == UINT64_MAX || auction_owner_revision == UINT64_MAX)
			return ERANGE;
		for (size_t i = 0; i < payload.item_count; ++i)
		{
			const auto &entry = payload.items[i];
			auto found = std::find_if(next->items.begin(), next->items.end(),
						  [&](const auto &item)
						  { return item.uid == entry.item_uid; });
			if (found == next->items.end() ||
			    found->revision != entry.expected_item_revision ||
			    found->vnum != entry.vnum || found->revision == UINT64_MAX ||
			    (payload.action == auction_action::claim_item &&
			     (found->claimed || found->claimant != payload.actor_pid)))
				return ESTALE;
			for (size_t prior = 0; prior < i; ++prior)
				if (payload.items[prior].item_uid == entry.item_uid)
					return EINVAL;
			result->item_uids[i] = found->uid;
			result->item_revisions[i] = ++found->revision;
			if (payload.action == auction_action::claim_item)
				found->claimed = true;
		}
		result->item_count = payload.item_count;
		result->player_owner_revision = player_owner_revision + 1;
		result->auction_owner_revision = auction_owner_revision + 1;
		if (payload.action == auction_action::claim_item)
			result->event_type = auction_event_type::item_claimed;
	}
	result->auction_id = next->id;
	result->status = next->status;
	result->seller_pid = next->seller;
	result->winner_pid = payload.action == auction_action::claim_item ? payload.actor_pid :
									    next->winner;
	// Listing and item claims historically leave final_price at zero.
	if (payload.action == auction_action::bid || payload.action == auction_action::finalize ||
	    payload.action == auction_action::remove)
		result->final_price = next->price;
	result->auction_revision = next->revision;
	result->end_time = next->end_time;
	return 0;
}
