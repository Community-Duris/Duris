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

namespace
{
[[maybe_unused]] bool reset_compile_add(size_t &total, size_t amount) noexcept
{
	if (amount > SIZE_MAX - total)
		return false;
	total += amount;
	return true;
}
[[maybe_unused]] bool reset_compile_rows(size_t &total, size_t count, size_t width) noexcept
{
	return (!width || count <= SIZE_MAX / width) && reset_compile_add(total, count * width);
}
[[maybe_unused]] bool reset_compile_string(size_t &total, const std::string &value) noexcept
{
	// Supported C++11 string uses its inline 15-character buffer. Actual dynamic
	// capacity, including the NUL request, is retained after the bounded decode.
	return value.capacity() <= 15 ||
	       (value.capacity() < SIZE_MAX && reset_compile_add(total, value.capacity() + 1));
}
[[maybe_unused]] bool reset_compile_image_heap(const zone_reset_item_image &image,
					       size_t *output) noexcept
{
	size_t bytes = 0;
	if (!reset_compile_rows(bytes, image.items.capacity(), sizeof(player_item_snapshot)) ||
	    !reset_compile_rows(bytes, image.recipes.capacity(),
				sizeof(native_mobile_birth_item_recipe)) ||
	    !reset_compile_rows(bytes, image.coins.capacity(), sizeof(zone_reset_coin_output)))
		return false;
	for (const auto &item : image.items)
	{
		if (!reset_compile_string(bytes, item.name) ||
		    !reset_compile_string(bytes, item.short_description) ||
		    !reset_compile_string(bytes, item.description) ||
		    !reset_compile_string(bytes, item.action_description) ||
		    !reset_compile_rows(bytes, item.dynamic_affects.capacity(),
					sizeof(player_item_dynamic_affect_snapshot)) ||
		    !reset_compile_rows(bytes, item.extra_descriptions.capacity(),
					sizeof(player_item_extra_description_snapshot)))
			return false;
		for (const auto &extra : item.extra_descriptions)
			if (!reset_compile_string(bytes, extra.keyword) ||
			    !reset_compile_string(bytes, extra.description) ||
			    !reset_compile_rows(bytes, extra.spell_ids.capacity(), sizeof(int32_t)))
				return false;
	}
	for (const auto &recipe : image.recipes)
		if (!reset_compile_rows(bytes, recipe.libraries.capacity(),
					sizeof(native_mobile_birth_library_recipe)))
			return false;
	*output = bytes;
	return true;
}
[[maybe_unused]] bool reset_compile_plan_heap(const economic_accounting_plan &plan,
					      size_t *output) noexcept
{
	size_t bytes = 0;
	if (!reset_compile_rows(bytes, plan.accounts.capacity(), sizeof(economic_account_effect)) ||
	    !reset_compile_rows(bytes, plan.postings.capacity(), sizeof(economic_coin_posting)) ||
	    !reset_compile_rows(bytes, plan.children.capacity(), sizeof(economic_child_link)) ||
	    !reset_compile_rows(bytes, plan.items_before.capacity(),
				sizeof(economic_item_snapshot)) ||
	    !reset_compile_rows(bytes, plan.items_after.capacity(),
				sizeof(economic_item_snapshot)) ||
	    !reset_compile_rows(bytes, plan.item_events.capacity(), sizeof(economic_item_event)))
		return false;
	*output = bytes;
	return true;
}
struct reset_compile_workspace
{
	zone_reset_item_image image;
	economic_frozen_intent intent;
	economic_accounting_plan candidate;
	economic_accounting_plan_allocation_profile profile;
	std::span<const uint8_t> intent_bytes;
	size_t image_heap = 0, candidate_heap = 0, live = 0, peak = 0, retained_heap = 0;
};
} // namespace

economic_accounting_error zone_reset_item_accounting_compile_bounded(
	const critical_command &command, economic_accounting_plan *output,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer_live,
	size_t *retained_plan_heap_bytes) noexcept
{
	using error = economic_accounting_error;
	if (!output)
		return error::corrupt_evidence;
	if (!reserve)
		return error::capacity;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	(void)command;
	(void)context;
	(void)outer_live;
	(void)retained_plan_heap_bytes;
	return error::capacity;
#else
	try
	{
		size_t base = outer_live;
		if (!reset_compile_add(base, sizeof(reset_compile_workspace)) ||
		    !reserve(base, context))
			return error::capacity;
		reset_compile_workspace work;
		auto &image = work.image;
		auto &intent = work.intent;
		auto &candidate = work.candidate;
		auto status = zone_reset_item_command_decode_bounded(command, &image, reserve,
								     context, base);
		if (status != error::ok)
			return status;
		if (image.items.size() > ECONOMIC_ACCOUNTING_MAX_ITEM_EVENTS ||
		    image.items.size() > ECONOMIC_ACCOUNTING_MAX_ITEM_WITNESSES ||
		    image.coins.size() >= ECONOMIC_ACCOUNTING_MAX_ACCOUNTS ||
		    image.coins.size() > ECONOMIC_ACCOUNTING_MAX_POSTINGS / 2)
			return error::capacity;
		if (!reset_compile_image_heap(image, &work.image_heap))
			return error::capacity;
		work.live = base;
		if (!reset_compile_add(work.live, work.image_heap))
			return error::capacity;
		work.intent_bytes = command.accounting_intent;
		status = economic_intent_decode_bounded(work.intent_bytes, &intent, reserve,
							context, work.live);
		if (status != error::ok)
			return status;
		if (!reset_compile_add(work.live, intent.admission.facts.capacity()))
			return error::capacity;
		status = economic_intent_plan_metadata_bounded(command, intent, &candidate.metadata,
							       reserve, context, work.live);
		if (status != error::ok)
			return status;
		// Original fresh reserves below have exact requests on the supported ABI.
		// Account/posting capacities remain live through normalization even when
		// zero piles leave their actual sizes smaller than these reservations.
		if (!reset_compile_rows(work.candidate_heap, image.items.size(),
					2 * sizeof(economic_item_snapshot) +
						sizeof(economic_item_event)) ||
		    !reset_compile_rows(work.candidate_heap, image.coins.size() + 1,
					sizeof(economic_account_effect)) ||
		    !reset_compile_rows(work.candidate_heap, image.coins.size(),
					2 * sizeof(economic_coin_posting)))
			return error::capacity;
		work.peak = work.live;
		const size_t item_temporaries =
			sizeof(economic_item_position) +
			std::max(sizeof(economic_item_snapshot), sizeof(economic_item_event));
		const size_t coin_temporaries = std::max(
			4 * sizeof(economic_coin_vector),
			2 * sizeof(economic_coin_vector) + std::max(sizeof(economic_account_effect),
								    sizeof(economic_coin_posting)));
		if (!reset_compile_add(work.peak, work.candidate_heap) ||
		    !reset_compile_add(work.peak,
				       std::max(item_temporaries, image.coins.empty() ?
									  size_t{ 0 } :
									  coin_temporaries)) ||
		    !reserve(work.peak, context))
			return error::capacity;
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
		if (!reset_compile_plan_heap(candidate, &work.candidate_heap) ||
		    !reset_compile_add(work.live, work.candidate_heap))
			return error::capacity;
		work.peak = work.live;
		if (!reset_compile_add(work.peak,
				       economic_plan_allocation_preflight_working_bytes()) ||
		    !reserve(work.peak, context))
			return error::capacity;
		status = economic_plan_allocation_preflight(candidate, &work.profile);
		if (status != error::ok)
			return status;
		work.peak = work.live;
		if (!work.profile.storage_policy_supported ||
		    !reset_compile_add(work.peak, work.profile.normalize_working_bytes) ||
		    !reserve(work.peak, context))
			return error::capacity;
		status = economic_plan_normalize(&candidate);
		if (status != error::ok)
			return status;
		if (!reset_compile_plan_heap(candidate, &work.retained_heap))
			return error::capacity;
		*output = std::move(candidate);
		if (retained_plan_heap_bytes)
			*retained_plan_heap_bytes = work.retained_heap;
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
