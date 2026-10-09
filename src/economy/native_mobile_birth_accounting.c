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

#include <type_traits>

namespace
{
bool birth_compile_add(size_t &bytes, size_t added) noexcept
{
	if (added > SIZE_MAX - bytes)
		return false;
	bytes += added;
	return true;
}
bool birth_compile_rows(size_t &bytes, size_t count, size_t width) noexcept
{
	return (!width || count <= SIZE_MAX / width) && birth_compile_add(bytes, count * width);
}
bool birth_compile_admit(size_t base, size_t extra, bool (*reserve)(size_t, void *) noexcept,
			 void *context) noexcept
{
	return extra <= SIZE_MAX - base && reserve && reserve(base + extra, context);
}
bool birth_compile_plan_heap(const economic_accounting_plan &plan, size_t &bytes) noexcept
{
	bytes = 0;
	return birth_compile_rows(bytes, plan.accounts.capacity(),
				  sizeof(economic_account_effect)) &&
	       birth_compile_rows(bytes, plan.postings.capacity(), sizeof(economic_coin_posting)) &&
	       birth_compile_rows(bytes, plan.children.capacity(), sizeof(economic_child_link)) &&
	       birth_compile_rows(bytes, plan.items_before.capacity(),
				  sizeof(economic_item_snapshot)) &&
	       birth_compile_rows(bytes, plan.items_after.capacity(),
				  sizeof(economic_item_snapshot)) &&
	       birth_compile_rows(bytes, plan.item_events.capacity(), sizeof(economic_item_event));
}
bool birth_compile_account_push(size_t size, size_t capacity, size_t &extra) noexcept
{
	extra = sizeof(economic_account_effect);
	if (size != capacity)
		return true;
	const size_t growth = std::max(size, size_t{ 1 });
	return growth <= SIZE_MAX - size &&
	       birth_compile_rows(extra, size + growth, sizeof(economic_account_effect));
}
struct birth_compile_workspace
{
	quest_mobile_native_image image;
	economic_frozen_intent intent;
	economic_accounting_plan candidate;
	economic_coin_vector delta{};
	economic_accounting_plan_allocation_profile profile;
	std::span<const uint8_t> intent_wire;
	size_t image_heap = 0;
};
struct birth_compile_live
{
	birth_compile_workspace &work;
	size_t base;
	bool bytes(size_t &out) const noexcept
	{
		out = base;
		size_t plan_heap = 0;
		return birth_compile_add(out, work.image_heap) &&
		       birth_compile_add(out, work.intent.admission.facts.capacity()) &&
		       birth_compile_plan_heap(work.candidate, plan_heap) &&
		       birth_compile_add(out, plan_heap);
	}
};
}

economic_accounting_error native_mobile_birth_accounting_compile_v3_bounded(
	const critical_command &command, const economic_account_key &original_native_wallet,
	economic_accounting_plan *output, bool (*reserve)(size_t, void *) noexcept, void *context,
	size_t outer_live, size_t *retained_plan_heap_bytes) noexcept
{
	using error = economic_accounting_error;
	if (!output)
		return error::corrupt_evidence;
	// This companion serves the genuine cash-role projection; historical formats
	// still use the unchanged original compiler and are not claimed bounded here.
	if (command.payload_version != NATIVE_MOBILE_BIRTH_CONSTRUCTOR_PAYLOAD_VERSION)
		return error::unresolved;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	(void)original_native_wallet;
	(void)reserve;
	(void)context;
	(void)outer_live;
	(void)retained_plan_heap_bytes;
	return error::unresolved;
#else
	size_t base = outer_live;
	if (!birth_compile_add(base, sizeof(birth_compile_workspace)) ||
	    !birth_compile_add(base, sizeof(birth_compile_live)) ||
	    !birth_compile_admit(base, 0, reserve, context))
		return error::capacity;
	try
	{
		birth_compile_workspace work;
		birth_compile_live live{ work, base };
		auto &image = work.image;
		auto &intent = work.intent;
		auto &candidate = work.candidate;
		auto &delta = work.delta;
		error status;
		{
			// Original image-only decode discards recipes/constructor after complete
			// command proof. Keep that same lifetime rather than retain dead payloads.
			constexpr size_t decode_outputs =
				sizeof(std::vector<native_mobile_birth_item_recipe>) +
				sizeof(quest_mobile_native_constructor_recipe);
			if (!birth_compile_admit(base, decode_outputs, reserve, context))
				return error::capacity;
			std::vector<native_mobile_birth_item_recipe> recipes;
			quest_mobile_native_constructor_recipe constructor;
			size_t decode_outer = base;
			if (!birth_compile_add(decode_outer, decode_outputs))
				return error::capacity;
			status = native_mobile_birth_command_decode_bounded(
				command, &image, &recipes, &constructor, reserve, context,
				decode_outer, &work.image_heap);
		}
		if (status != error::ok)
			return status;
		if (!image.cash || image.cash->revision != 1)
			return error::corrupt_evidence;
		if (image.items.size() > ECONOMIC_ACCOUNTING_MAX_ITEM_EVENTS ||
		    image.items.size() > ECONOMIC_ACCOUNTING_MAX_ITEM_WITNESSES)
			return error::capacity;
		size_t current = 0;
		if (!live.bytes(current) ||
		    !birth_compile_admit(current, sizeof(std::span<const uint8_t>), reserve,
					 context))
			return error::capacity;
		work.intent_wire = std::span<const uint8_t>(command.accounting_intent);
		status = economic_intent_decode_bounded(work.intent_wire, &intent, reserve, context,
							current);
		if (status != error::ok)
			return status;
		if (!live.bytes(current))
			return error::capacity;
		status = economic_intent_plan_metadata_bounded(command, intent, &candidate.metadata,
							       reserve, context, current);
		if (status != error::ok)
			return status;
		if (!economic_account_key_valid(original_native_wallet) ||
		    original_native_wallet.lineage.bytes != candidate.metadata.lineage.bytes ||
		    original_native_wallet.kind != economic_account_kind::wallet ||
		    original_native_wallet.context_id != ECONOMIC_NATIVE_MOBILE_WALLET_CONTEXT)
			return error::invalid_identity;
		const auto &cash = image.cash->denominations.amount;
		// Zero argument temporary and economic_coin_delta's own result coexist;
		// the output delta is already a named workspace object.
		if (!live.bytes(current) ||
		    !birth_compile_admit(current, 2 * sizeof(economic_coin_vector), reserve,
					 context))
			return error::capacity;
		status = economic_coin_delta({}, cash, &delta);
		if (status != error::ok)
			return status;
		size_t extra = 0;
		if (!live.bytes(current) ||
		    !birth_compile_account_push(candidate.accounts.size(),
						candidate.accounts.capacity(), extra) ||
		    !birth_compile_admit(current, extra, reserve, context))
			return error::capacity;
		candidate.accounts.push_back(
			{ original_native_wallet, {}, cash, 0, image.cash->revision });
		if (std::any_of(delta.begin(), delta.end(), [](int64_t part) { return part != 0; }))
		{
			if (!live.bytes(current) ||
			    !birth_compile_admit(current, 3 * sizeof(economic_coin_vector), reserve,
						 context))
				return error::capacity;
			economic_coin_vector opposite{};
			// The admitted local opposite survives the zero argument/callee result.
			status = economic_coin_delta(cash, {}, &opposite);
			if (status != error::ok)
				return status;
			int64_t copper = 0;
			status = economic_coin_value(delta, &copper);
			if (status != error::ok)
				return status;
			if (!live.bytes(current) || !birth_compile_add(current, sizeof(opposite)) ||
			    !birth_compile_account_push(candidate.accounts.size(),
							candidate.accounts.capacity(), extra) ||
			    !birth_compile_admit(current, extra, reserve, context))
				return error::capacity;
			candidate.accounts.push_back({ { candidate.metadata.lineage,
							 economic_account_kind::issuance, 1, 0 },
						       {},
						       {},
						       0,
						       0 });
			if (!live.bytes(current) || !birth_compile_add(current, sizeof(opposite)))
				return error::capacity;
			extra = sizeof(std::initializer_list<economic_coin_posting>) +
				sizeof(std::array<economic_coin_posting, 2>);
			if (!birth_compile_rows(extra, 2, sizeof(economic_coin_posting)) ||
			    !birth_compile_admit(current, extra, reserve, context))
				return error::capacity;
			candidate.postings = { { 0, 0, 0, delta, copper },
					       { 1, 1, 0, opposite, -copper } };
		}
		size_t item_requests = 0;
		const size_t item_temporary =
			image.items.empty() ?
				size_t{ 0 } :
				sizeof(economic_item_position) +
					std::max(sizeof(item_owner_identity),
						 std::max(sizeof(economic_item_snapshot),
							  sizeof(economic_item_event)));
		if (!birth_compile_rows(item_requests, image.items.size(),
					2 * sizeof(economic_item_snapshot) +
						sizeof(economic_item_event)) ||
		    !birth_compile_add(item_requests, item_temporary) || !live.bytes(current) ||
		    !birth_compile_admit(current, item_requests, reserve, context))
			return error::capacity;
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
			candidate.item_events.push_back(
				{ static_cast<uint32_t>(index), 0, item.object_uid, {}, after });
		}
		if (!live.bytes(current) ||
		    !birth_compile_admit(current,
					 economic_plan_allocation_preflight_working_bytes(),
					 reserve, context))
			return error::capacity;
		status = economic_plan_allocation_preflight(candidate, &work.profile);
		if (status != error::ok)
			return status;
		if (!work.profile.storage_policy_supported ||
		    !birth_compile_admit(current, work.profile.normalize_working_bytes, reserve,
					 context))
			return error::capacity;
		status = economic_plan_normalize(&candidate);
		if (status != error::ok)
			return status;
		size_t retained = 0;
		if (!birth_compile_plan_heap(candidate, retained))
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
