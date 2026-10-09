#include "economy/native_mobile_birth_cash_role_accounting.h"

#include <climits>
#include <new>
#include <utility>

bool native_mobile_birth_shared_shop_participant_valid(
	const native_mobile_birth_shared_shop_participant &value) noexcept
{
	for (const auto part : value.born_cash)
		if (part < 0)
			return false;
	int64_t cash = 0;
	return value.shop_id <= INT32_MAX && value.shop_after_present &&
	       (!value.shop_before_present ? !value.shop_revision_before :
					     value.shop_revision_before != 0) &&
	       value.shop_revision_after &&
	       (value.owner_before_present || !value.owner_revision_before) &&
	       (value.owner_after_present || !value.owner_revision_after) &&
	       economic_coin_value(value.born_cash, &cash) == economic_accounting_error::ok &&
	       cash >= 0 && cash <= INT_MAX;
}

namespace
{
using error = economic_accounting_error;
error metadata(const critical_command &command, economic_plan_metadata *output,
	       economic_frozen_intent *intent)
{
	auto status = economic_intent_decode(command.accounting_intent, intent);
	if (status != error::ok)
		return status;
	return economic_intent_plan_metadata(command, *intent, output);
}
}

economic_accounting_error
native_mobile_birth_cash_role_accounting_compile(const critical_command &command,
						 const economic_account_key &wallet,
						 economic_accounting_plan *output) noexcept
{
	if (!output)
		return error::corrupt_evidence;
	try
	{
		quest_mobile_native_image image;
		std::vector<native_mobile_birth_item_recipe> recipes;
		native_mobile_birth_cash_role_recipe role;
		auto status = native_mobile_birth_cash_role_command_decode(command, &image,
									   &recipes, &role);
		if (status != error::ok)
			return status;
		if (role.role != native_mobile_birth_cash_role::ordinary_wallet)
			return error::invalid_identity;
		economic_plan_metadata original_metadata;
		economic_frozen_intent intent;
		status = metadata(command, &original_metadata, &intent);
		if (status != error::ok)
			return status;
		critical_command original;
		status = native_mobile_birth_command_build(intent.admission.metadata, image,
							   recipes, role.original,
							   command.source_site,
							   command.accepted_at_usec, &original);
		if (status != error::ok)
			return status;
		economic_accounting_plan candidate;
		status = native_mobile_birth_accounting_compile(original, wallet, &candidate);
		if (status != error::ok)
			return status;
		// Preserve every original effect, including known-zero wallet revision1;
		// only metadata bindings change to the complete original NMB4 command.
		candidate.metadata = std::move(original_metadata);
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

economic_accounting_error native_mobile_birth_cash_role_accounting_compile(
	const critical_command &command,
	const native_mobile_birth_shared_shop_participant &participant,
	economic_accounting_plan *output) noexcept
{
	if (!output)
		return error::corrupt_evidence;
	try
	{
		quest_mobile_native_image image;
		std::vector<native_mobile_birth_item_recipe> recipes;
		native_mobile_birth_cash_role_recipe role;
		auto status = native_mobile_birth_cash_role_command_decode(command, &image,
									   &recipes, &role);
		if (status != error::ok)
			return status;
		if (role.role != native_mobile_birth_cash_role::shared_shopkeeper ||
		    !native_mobile_birth_shared_shop_participant_valid(participant) ||
		    participant.shop_id != static_cast<uint32_t>(role.original.reset_shop_index) ||
		    !image.cash || image.cash->revision != 1 ||
		    participant.born_cash != image.cash->denominations.amount ||
		    (!image.items.empty() && !participant.owner_after_present))
			return error::invalid_identity;
		if (image.items.size() > ECONOMIC_ACCOUNTING_MAX_ITEM_EVENTS ||
		    image.items.size() > ECONOMIC_ACCOUNTING_MAX_ITEM_WITNESSES)
			return error::capacity;
		economic_accounting_plan candidate;
		economic_frozen_intent intent;
		status = metadata(command, &candidate.metadata, &intent);
		if (status != error::ok)
			return status;
		// No fake wallet, treasury, virtual zero leg or cash issuance. The complete
		// native cash image and supplied SHOP facts are retained by the new result.
		candidate.items_before.reserve(image.items.size());
		candidate.items_after.reserve(image.items.size());
		candidate.item_events.reserve(image.items.size());
		for (size_t index = 0; index < image.items.size(); ++index)
		{
			const auto &item = image.items[index];
			economic_item_position after{};
			after.owner = { item_owner_type::shopkeeper,
					item_shopkeeper_owner_id(participant.shop_id), 0 };
			after.root_uid = item.object_uid;
			after.revision = 1;
			after.state = item_custody_state::active;
			// Original SHOP persistence preserves equipped roots. Its full physical
			// stock proof requires the same slot in custody and the native image.
			after.equipment_slot = item.equipment_slot;
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
			// Original DFS event ordering survives canonical UID witness sorting.
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
