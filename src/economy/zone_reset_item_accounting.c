#include "economy/zone_reset_item_accounting.h"

#include <algorithm>
#include <new>
#include <utility>

economic_accounting_error
zone_reset_item_accounting_compile(const critical_command &command,
				   economic_accounting_plan *output) noexcept
{
	using error = economic_accounting_error;
	if (!output)
		return error::corrupt_evidence;
	try
	{
		zone_reset_item_image image;
		auto status = zone_reset_item_command_decode(command, &image);
		if (status != error::ok)
			return status;
		if (image.items.size() > ECONOMIC_ACCOUNTING_MAX_ITEM_EVENTS ||
		    image.items.size() > ECONOMIC_ACCOUNTING_MAX_ITEM_WITNESSES ||
		    image.coins.size() >= ECONOMIC_ACCOUNTING_MAX_ACCOUNTS ||
		    image.coins.size() > ECONOMIC_ACCOUNTING_MAX_POSTINGS / 2)
			return error::capacity;
		economic_frozen_intent intent;
		status = economic_intent_decode(command.accounting_intent, &intent);
		if (status != error::ok)
			return status;
		economic_accounting_plan candidate;
		status = economic_intent_plan_metadata(command, intent, &candidate.metadata);
		if (status != error::ok)
			return status;
		candidate.items_before.reserve(image.items.size());
		candidate.items_after.reserve(image.items.size());
		candidate.item_events.reserve(image.items.size());
		for (size_t index = 0; index < image.items.size(); ++index)
		{
			const auto &item = image.items[index];
			economic_item_position after{};
			after.owner = { item_owner_type::room,
					static_cast<uint64_t>(image.room_vnum), 0 };
			after.root_uid = image.items.front().object_uid;
			after.revision = 1;
			after.state = item_custody_state::active;
			if (item.parent_index != PLAYER_SNAPSHOT_NO_PARENT)
			{
				if (item.parent_index < 0 ||
				    static_cast<size_t>(item.parent_index) >= index)
					return error::topology;
				after.parent_uid = image.items[item.parent_index].object_uid;
			}
			candidate.items_before.push_back({ item.object_uid, {} });
			candidate.items_after.push_back({ item.object_uid, after });
			candidate.item_events.push_back(
				{ static_cast<uint32_t>(index), 0, item.object_uid, {}, after });
		}
		// Pile identity is the original UID lifetime; no player wallet or mapping
		// can stand in for the world-generation issuance counterparty.
		candidate.accounts.reserve(image.coins.size() + 1);
		candidate.postings.reserve(image.coins.size() * 2);
		size_t issuance_index = SIZE_MAX;
		for (size_t index = 0; index < image.coins.size(); ++index)
		{
			const auto &coin = image.coins[index];
			const size_t pile_index = candidate.accounts.size();
			candidate.accounts.push_back(
				{ { candidate.metadata.lineage, economic_account_kind::pile,
				    coin.item_uid, 0 },
				  {},
				  coin.denominations,
				  0,
				  1 });
			economic_coin_vector delta{}, opposite{};
			status = economic_coin_delta({}, coin.denominations, &delta);
			if (status != error::ok)
				return status;
			if (std::all_of(delta.begin(), delta.end(),
					[](int64_t part) { return !part; }))
				continue; // Explicit zero still creates the ordinary pile at revision1.
			status = economic_coin_delta(coin.denominations, {}, &opposite);
			if (status != error::ok)
				return status;
			int64_t copper = 0;
			status = economic_coin_value(delta, &copper);
			if (status != error::ok)
				return status;
			if (issuance_index == SIZE_MAX)
			{
				issuance_index = candidate.accounts.size();
				candidate.accounts.push_back(
					{ { candidate.metadata.lineage,
					    economic_account_kind::issuance, 1, 0 },
					  {},
					  {},
					  0,
					  0 });
			}
			candidate.postings.push_back(
				{ static_cast<uint32_t>(candidate.postings.size()),
				  static_cast<uint16_t>(pile_index), 0, delta, copper });
			candidate.postings.push_back(
				{ static_cast<uint32_t>(candidate.postings.size()),
				  static_cast<uint16_t>(issuance_index), 0, opposite, -copper });
		}
		status = economic_plan_normalize(&candidate);
		if (status != error::ok)
			return status;
		*output = std::move(candidate);
		return error::ok;
	}
	catch (const std::bad_alloc &)
	{
		return error::capacity;
	}
	catch (...)
	{
		return error::corrupt_evidence;
	}
}
