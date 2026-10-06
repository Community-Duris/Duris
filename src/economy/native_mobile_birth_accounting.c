#include "economy/native_mobile_birth_accounting.h"

#include <algorithm>
#include <new>
#include <utility>

economic_accounting_error
native_mobile_birth_accounting_compile(const critical_command &command,
				       const economic_account_key &original_native_wallet,
				       economic_accounting_plan *output) noexcept
{
	using error = economic_accounting_error;
	if (!output)
		return error::corrupt_evidence;
	try
	{
		quest_mobile_native_image image;
		auto status = native_mobile_birth_command_decode(command, &image);
		if (status != error::ok)
			return status;
		if (!image.cash || image.cash->revision != 1)
			return error::corrupt_evidence; // Historical unknown cash is never zero.
		if (image.items.size() > ECONOMIC_ACCOUNTING_MAX_ITEM_EVENTS ||
		    image.items.size() > ECONOMIC_ACCOUNTING_MAX_ITEM_WITNESSES)
			return error::capacity;

		economic_frozen_intent intent;
		status = economic_intent_decode(command.accounting_intent, &intent);
		if (status != error::ok)
			return status;
		economic_accounting_plan candidate;
		status = economic_intent_plan_metadata(command, intent, &candidate.metadata);
		if (status != error::ok)
			return status;

		// The original atomic owner supplies its created/locked mapping lifetime.
		// Native UID and mapping ID use distinct namespaces; never infer this key.
		if (!economic_account_key_valid(original_native_wallet) ||
		    original_native_wallet.lineage.bytes != candidate.metadata.lineage.bytes ||
		    original_native_wallet.kind != economic_account_kind::wallet ||
		    original_native_wallet.context_id != ECONOMIC_NATIVE_MOBILE_WALLET_CONTEXT)
			return error::invalid_identity;

		const auto &cash = image.cash->denominations.amount;
		economic_coin_vector delta{};
		status = economic_coin_delta({}, cash, &delta);
		if (status != error::ok)
			return status;
		candidate.accounts.push_back(
			{ original_native_wallet, {}, cash, 0, image.cash->revision });
		if (std::any_of(delta.begin(), delta.end(), [](int64_t part) { return part != 0; }))
		{
			economic_coin_vector opposite{};
			status = economic_coin_delta(cash, {}, &opposite);
			if (status != error::ok)
				return status;
			int64_t copper = 0;
			status = economic_coin_value(delta, &copper);
			if (status != error::ok)
				return status;
			candidate.accounts.push_back({ { candidate.metadata.lineage,
							 economic_account_kind::issuance, 1, 0 },
						       {},
						       {},
						       0,
						       0 });
			candidate.postings = { { 0, 0, 0, delta, copper },
					       { 1, 1, 0, opposite, -copper } };
		}
		// Known zero still creates the ordinary wallet at revision1. Its unchanged
		// balance needs no posting; an unreferenced zero issuance account is invalid.

		candidate.items_before.reserve(image.items.size());
		candidate.items_after.reserve(image.items.size());
		candidate.item_events.reserve(image.items.size());
		for (size_t index = 0; index < image.items.size(); ++index)
		{
			const auto &item = image.items[index];
			economic_item_position after{};
			after.owner = { item_owner_type::native_mobile,
					image.reference.mobile_instance_id, 0 };
			after.root_uid = item.object_uid;
			after.revision = 1;
			after.state = item_custody_state::active;
			after.equipment_slot = static_cast<uint16_t>(item.equipment_slot);
			if (item.parent_index != PLAYER_SNAPSHOT_NO_PARENT)
			{
				if (item.parent_index < 0 ||
				    static_cast<size_t>(item.parent_index) >= index ||
				    item.equipment_slot)
					return error::topology;
				const size_t parent = static_cast<size_t>(item.parent_index);
				after.root_uid = candidate.items_after[parent].position.root_uid;
				after.parent_uid = image.items[parent].object_uid;
			}
			candidate.items_before.push_back({ item.object_uid, {} });
			candidate.items_after.push_back({ item.object_uid, after });
			// Event indices retain original equipment/carry DFS order. Shared
			// normalization sorts witnesses by UID without changing this order.
			candidate.item_events.push_back(
				{ static_cast<uint32_t>(index), 0, item.object_uid, {}, after });
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
