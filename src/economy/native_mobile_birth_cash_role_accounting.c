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

#include <algorithm>
#include <type_traits>

namespace
{
bool cash_compile_add(size_t &bytes, size_t added) noexcept
{
	if (added > SIZE_MAX - bytes)
		return false;
	bytes += added;
	return true;
}
bool cash_compile_rows(size_t &bytes, size_t count, size_t width) noexcept
{
	return (!width || count <= SIZE_MAX / width) && cash_compile_add(bytes, count * width);
}
bool cash_compile_admit(size_t base, size_t extra, bool (*reserve)(size_t, void *) noexcept,
			void *context) noexcept
{
	return extra <= SIZE_MAX - base && reserve && reserve(base + extra, context);
}
bool cash_compile_plan_heap(const economic_accounting_plan &plan, size_t &bytes) noexcept
{
	bytes = 0;
	return cash_compile_rows(bytes, plan.accounts.capacity(),
				 sizeof(economic_account_effect)) &&
	       cash_compile_rows(bytes, plan.postings.capacity(), sizeof(economic_coin_posting)) &&
	       cash_compile_rows(bytes, plan.children.capacity(), sizeof(economic_child_link)) &&
	       cash_compile_rows(bytes, plan.items_before.capacity(),
				 sizeof(economic_item_snapshot)) &&
	       cash_compile_rows(bytes, plan.items_after.capacity(),
				 sizeof(economic_item_snapshot)) &&
	       cash_compile_rows(bytes, plan.item_events.capacity(), sizeof(economic_item_event));
}
bool cash_compile_command_heap(const critical_command &command, size_t &bytes) noexcept
{
	bytes = 0;
	return cash_compile_rows(bytes, command.keys.capacity(), sizeof(critical_entity_key)) &&
	       cash_compile_rows(bytes, command.expected_revisions.capacity(),
				 sizeof(critical_expected_revision)) &&
	       cash_compile_add(bytes, command.payload.capacity()) &&
	       cash_compile_add(bytes, command.accounting_intent.capacity());
}
struct cash_compile_workspace
{
	quest_mobile_native_image image;
	std::vector<native_mobile_birth_item_recipe> recipes;
	native_mobile_birth_cash_role_recipe role;
	economic_plan_metadata original_metadata;
	economic_frozen_intent intent;
	critical_command original;
	economic_accounting_plan candidate;
	economic_accounting_plan_allocation_profile profile;
	std::span<const uint8_t> intent_wire;
	std::span<const native_mobile_birth_item_recipe> recipe_values;
	size_t image_heap = 0, recipe_heap = 0;
};
struct cash_compile_live
{
	cash_compile_workspace &work;
	size_t base;
	bool bytes(size_t &out) const noexcept
	{
		out = base;
		size_t command_heap = 0, plan_heap = 0;
		return cash_compile_command_heap(work.original, command_heap) &&
		       cash_compile_plan_heap(work.candidate, plan_heap) &&
		       cash_compile_add(out, command_heap) && cash_compile_add(out, plan_heap) &&
		       cash_compile_add(out, work.image_heap) &&
		       cash_compile_add(out, work.recipe_heap) &&
		       cash_compile_add(out, work.intent.admission.facts.capacity());
	}
};
error cash_compile_metadata_bounded(const critical_command &command, cash_compile_workspace &work,
				    const cash_compile_live &live, economic_plan_metadata *output,
				    bool (*reserve)(size_t, void *) noexcept,
				    void *context) noexcept
{
	size_t current = 0;
	if (!live.bytes(current) ||
	    !cash_compile_admit(current, sizeof(std::span<const uint8_t>), reserve, context))
		return error::capacity;
	work.intent_wire = std::span<const uint8_t>(command.accounting_intent);
	auto status = economic_intent_decode_bounded(work.intent_wire, &work.intent, reserve,
						     context, current);
	if (status != error::ok)
		return status;
	if (!live.bytes(current))
		return error::capacity;
	return economic_intent_plan_metadata_bounded(command, work.intent, output, reserve, context,
						     current);
}
error cash_compile_normalize_bounded(cash_compile_workspace &work, const cash_compile_live &live,
				     bool (*reserve)(size_t, void *) noexcept,
				     void *context) noexcept
{
	size_t current = 0;
	if (!live.bytes(current) ||
	    !cash_compile_admit(current, economic_plan_allocation_preflight_working_bytes(),
				reserve, context))
		return error::capacity;
	auto status = economic_plan_allocation_preflight(work.candidate, &work.profile);
	if (status != error::ok)
		return status;
	if (!work.profile.storage_policy_supported ||
	    !cash_compile_admit(current, work.profile.normalize_working_bytes, reserve, context))
		return error::capacity;
	return economic_plan_normalize(&work.candidate);
}
}

economic_accounting_error native_mobile_birth_cash_role_accounting_compile_bounded(
	const critical_command &command, const economic_account_key &wallet,
	economic_accounting_plan *output, bool (*reserve)(size_t, void *) noexcept, void *context,
	size_t outer_live, size_t *retained_plan_heap_bytes) noexcept
{
	if (!output)
		return error::corrupt_evidence;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	(void)command;
	(void)wallet;
	(void)reserve;
	(void)context;
	(void)outer_live;
	(void)retained_plan_heap_bytes;
	return error::unresolved;
#else
	size_t base = outer_live;
	if (!cash_compile_add(base, sizeof(cash_compile_workspace)) ||
	    !cash_compile_add(base, sizeof(cash_compile_live)) ||
	    !cash_compile_admit(base, 0, reserve, context))
		return error::capacity;
	try
	{
		cash_compile_workspace work;
		cash_compile_live live{ work, base };
		auto status = native_mobile_birth_cash_role_command_decode_bounded(
			command, &work.image, &work.recipes, &work.role, reserve, context, base,
			&work.image_heap, &work.recipe_heap);
		if (status != error::ok)
			return status;
		if (work.role.role != native_mobile_birth_cash_role::ordinary_wallet)
			return error::invalid_identity;
		status = cash_compile_metadata_bounded(command, work, live, &work.original_metadata,
						       reserve, context);
		if (status != error::ok)
			return status;
		size_t current = 0;
		if (!live.bytes(current) ||
		    !cash_compile_admit(current,
					sizeof(std::span<const native_mobile_birth_item_recipe>),
					reserve, context))
			return error::capacity;
		work.recipe_values = std::span<const native_mobile_birth_item_recipe>(work.recipes);
		status = native_mobile_birth_command_build_bounded(
			work.intent.admission.metadata, work.image, work.recipe_values,
			work.role.original, command.source_site, command.accepted_at_usec,
			&work.original, reserve, context, current);
		if (status != error::ok)
			return status;
		if (!live.bytes(current))
			return error::capacity;
		status = native_mobile_birth_accounting_compile_v3_bounded(
			work.original, wallet, &work.candidate, reserve, context, current);
		if (status != error::ok)
			return status;
		// Every original wallet effect is retained; only complete NMB4 metadata is rebound.
		work.candidate.metadata = std::move(work.original_metadata);
		status = cash_compile_normalize_bounded(work, live, reserve, context);
		if (status != error::ok)
			return status;
		size_t retained = 0;
		if (!cash_compile_plan_heap(work.candidate, retained))
			return error::capacity;
		static_assert(std::is_nothrow_move_assignable_v<economic_accounting_plan>);
		*output = std::move(work.candidate);
		if (retained_plan_heap_bytes)
			*retained_plan_heap_bytes = retained;
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
#endif
}

economic_accounting_error native_mobile_birth_cash_role_accounting_compile_bounded(
	const critical_command &command,
	const native_mobile_birth_shared_shop_participant &participant,
	economic_accounting_plan *output, bool (*reserve)(size_t, void *) noexcept, void *context,
	size_t outer_live, size_t *retained_plan_heap_bytes) noexcept
{
	if (!output)
		return error::corrupt_evidence;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	(void)command;
	(void)participant;
	(void)reserve;
	(void)context;
	(void)outer_live;
	(void)retained_plan_heap_bytes;
	return error::unresolved;
#else
	size_t base = outer_live;
	if (!cash_compile_add(base, sizeof(cash_compile_workspace)) ||
	    !cash_compile_add(base, sizeof(cash_compile_live)) ||
	    !cash_compile_admit(base, 0, reserve, context))
		return error::capacity;
	try
	{
		cash_compile_workspace work;
		cash_compile_live live{ work, base };
		auto &image = work.image;
		auto &candidate = work.candidate;
		auto status = native_mobile_birth_cash_role_command_decode_bounded(
			command, &image, &work.recipes, &work.role, reserve, context, base,
			&work.image_heap, &work.recipe_heap);
		if (status != error::ok)
			return status;
		if (work.role.role != native_mobile_birth_cash_role::shared_shopkeeper ||
		    !native_mobile_birth_shared_shop_participant_valid(participant) ||
		    participant.shop_id !=
			    static_cast<uint32_t>(work.role.original.reset_shop_index) ||
		    !image.cash || image.cash->revision != 1 ||
		    participant.born_cash != image.cash->denominations.amount ||
		    (!image.items.empty() && !participant.owner_after_present))
			return error::invalid_identity;
		if (image.items.size() > ECONOMIC_ACCOUNTING_MAX_ITEM_EVENTS ||
		    image.items.size() > ECONOMIC_ACCOUNTING_MAX_ITEM_WITNESSES)
			return error::capacity;
		status = cash_compile_metadata_bounded(command, work, live, &candidate.metadata,
						       reserve, context);
		if (status != error::ok)
			return status;
		// No wallet/posting/cash issuance is introduced by shared SHOP birth.
		size_t current = 0, item_requests = 0;
		const size_t item_temporary =
			image.items.empty() ?
				size_t{ 0 } :
				sizeof(economic_item_position) +
					std::max(sizeof(item_owner_identity),
						 std::max(sizeof(economic_item_snapshot),
							  sizeof(economic_item_event)));
		if (!cash_compile_rows(item_requests, image.items.size(),
				       2 * sizeof(economic_item_snapshot) +
					       sizeof(economic_item_event)) ||
		    !cash_compile_add(item_requests, item_temporary) || !live.bytes(current) ||
		    !cash_compile_admit(current, item_requests, reserve, context))
			return error::capacity;
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
			candidate.item_events.push_back(
				{ static_cast<uint32_t>(index), 0, item.object_uid, {}, after });
		}
		status = cash_compile_normalize_bounded(work, live, reserve, context);
		if (status != error::ok)
			return status;
		size_t retained = 0;
		if (!cash_compile_plan_heap(candidate, retained))
			return error::capacity;
		static_assert(std::is_nothrow_move_assignable_v<economic_accounting_plan>);
		*output = std::move(candidate);
		if (retained_plan_heap_bytes)
			*retained_plan_heap_bytes = retained;
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
#endif
}
