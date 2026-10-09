#ifndef SMITH_NATIVE_COMPOUND_IMAGES_H
#define SMITH_NATIVE_COMPOUND_IMAGES_H

#include "item/smith_native_compound_codec.h"
#include "world/quest_mobile_native.h"

// Pure full-image transform. The original owner separately authenticates and
// retains these BEFORE values, save ACK, live generations and complete custody.
// Current SQL must check the original frozen carrier, never manufacture it.
struct smith_native_compound_images
{
	quest_mobile_native_image native_before, native_after;
	std::vector<player_item_snapshot> player_before, player_after;
};

// Original runtime grouping facts, independently authenticated by the owner
// against its actual indexed PC/output bodies and retained save/factory cut.
// R_num is not inferred from VNUM. These values alone authorize no publication.
struct smith_native_carried_root
{
	uint64_t object_uid = 0;
	int32_t original_rnum = -1;
};

struct smith_native_player_grant_projection
{
	uint32_t original_player_level = 0;
	int32_t original_output_rnum = -1;
	std::vector<smith_native_carried_root> original_carried_roots;
};

inline player_snapshot_codec_result
smith_native_compound_prepare_images(const smith_native_compound_terms &terms,
				     const quest_mobile_native_image &native_before,
				     const std::vector<player_item_snapshot> &player_before,
				     const smith_native_player_grant_projection &original_grant,
				     smith_native_compound_images *output) noexcept
{
	if (!output)
		return player_snapshot_codec_result::invalid_value;
	try
	{
		std::vector<uint8_t> checked_terms, checked_native, checked_player;
		auto result = smith_native_compound_encode(terms, &checked_terms);
		if (result != player_snapshot_codec_result::ok)
			return result;
		result = quest_mobile_native_image_encode(native_before, &checked_native);
		if (result != player_snapshot_codec_result::ok)
			return result;
		result = player_item_snapshot_list_encode(player_before, &checked_player);
		if (result != player_snapshot_codec_result::ok)
			return result;
		// The full physical carrier uses the original equipment/carry DFS order.
		// Generic snapshot validity alone permits reopened trees or equipment
		// after a carried root, which cannot describe original obj_to_char.
		if (!smith_native_compound_codec_detail::literal_forest(player_before) ||
		    original_grant.original_output_rnum < 0 ||
		    original_grant.original_carried_roots.size() > player_before.size())
			return player_snapshot_codec_result::invalid_value;
		int16_t last_equipment_slot = 0;
		bool inventory_started = false;
		size_t carried_index = 0;
		for (const auto &row : player_before)
		{
			if (row.parent_index != PLAYER_SNAPSHOT_NO_PARENT)
			{
				if (row.equipment_slot)
					return player_snapshot_codec_result::invalid_value;
				continue;
			}
			if (row.equipment_slot < 0 ||
			    row.equipment_slot > ITEM_TRANSFER_MAX_EQUIPMENT_SLOT)
				return player_snapshot_codec_result::invalid_value;
			if (row.equipment_slot)
			{
				if (inventory_started || row.equipment_slot <= last_equipment_slot)
					return player_snapshot_codec_result::invalid_value;
				last_equipment_slot = row.equipment_slot;
			}
			else
			{
				inventory_started = true;
				if (carried_index == original_grant.original_carried_roots.size())
					return player_snapshot_codec_result::invalid_value;
				const auto &original =
					original_grant.original_carried_roots[carried_index++];
				if (original.object_uid != row.object_uid ||
				    original.original_rnum < 0)
					return player_snapshot_codec_result::invalid_value;
			}
		}
		if (carried_index != original_grant.original_carried_roots.size())
			return player_snapshot_codec_result::invalid_value;
		std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES> expected{}, observed{};
		if (native_before.state != quest_mobile_lifetime_state::live ||
		    !native_before.cash ||
		    quest_mobile_native_reference_encode(terms.native_before.reference,
							 &expected) !=
			    player_snapshot_codec_result::ok ||
		    quest_mobile_native_reference_encode(native_before.reference, &observed) !=
			    player_snapshot_codec_result::ok ||
		    expected != observed ||
		    native_before.cash->revision != terms.native_before.cash_revision ||
		    native_before.cash->denominations.amount != terms.native_before.denominations ||
		    player_before.size() > PLAYER_SNAPSHOT_MAX_OBJECTS ||
		    terms.frozen_outputs.size() >
			    PLAYER_SNAPSHOT_MAX_OBJECTS - player_before.size())
			return player_snapshot_codec_result::invalid_value;

		// Complete current images must not disagree on any native/PC/output UID.
		// Foreign-world claims remain an additional original authority check.
		std::vector<uint64_t> all_uids;
		all_uids.reserve(native_before.items.size() + player_before.size() +
				 terms.frozen_outputs.size());
		for (const auto &row : native_before.items)
			all_uids.push_back(row.object_uid);
		for (const auto &row : player_before)
		{
			if (!row.object_uid || row.object_uid == UINT64_MAX)
				return player_snapshot_codec_result::invalid_value;
			all_uids.push_back(row.object_uid);
		}
		for (const auto &row : terms.frozen_outputs)
			all_uids.push_back(row.object_uid);
		std::sort(all_uids.begin(), all_uids.end());
		if (std::adjacent_find(all_uids.begin(), all_uids.end()) != all_uids.end())
			return player_snapshot_codec_result::invalid_value;

		// Preserve native forest order; generic extract_forest groups by sorted
		// root UID and therefore cannot supply this original selected order.
		std::vector<player_item_snapshot> selected, remaining;
		std::vector<int32_t> translated(native_before.items.size(),
						PLAYER_SNAPSHOT_NO_PARENT);
		bool selecting = false;
		for (size_t i = 0; i < native_before.items.size(); ++i)
		{
			const auto &before = native_before.items[i];
			if (before.parent_index == PLAYER_SNAPSHOT_NO_PARENT)
				selecting = std::find(terms.selected_root_order.begin(),
						      terms.selected_root_order.end(),
						      before.object_uid) !=
					    terms.selected_root_order.end();
			auto row = before;
			if (row.parent_index != PLAYER_SNAPSHOT_NO_PARENT)
			{
				if (row.parent_index < 0 || size_t(row.parent_index) >= i ||
				    translated[row.parent_index] < 0)
					return player_snapshot_codec_result::invalid_value;
				row.parent_index = translated[row.parent_index];
			}
			auto &forest = selecting ? selected : remaining;
			translated[i] = static_cast<int32_t>(forest.size());
			forest.push_back(std::move(row));
		}
		std::vector<uint8_t> actual_selection, frozen_selection;
		if (player_item_snapshot_list_encode(selected, &actual_selection) !=
			    player_snapshot_codec_result::ok ||
		    player_item_snapshot_list_encode(terms.selected_native_items,
						     &frozen_selection) !=
			    player_snapshot_codec_result::ok ||
		    actual_selection != frozen_selection)
			return player_snapshot_codec_result::invalid_value;

		smith_native_compound_images candidate;
		candidate.native_before = native_before;
		candidate.native_after = native_before;
		candidate.native_after.items = std::move(remaining);
		++candidate.native_after.reference.mobile_revision;
		++candidate.native_after.reference.stock_revision;
		candidate.native_after.last_transition_operation = terms.operation_id;
		// Preserve actual NPC cash and its clock. Only stock/mobile advance.
		if (!quest_mobile_native_cash_transition_valid(&candidate.native_before,
							       candidate.native_after))
			return player_snapshot_codec_result::invalid_value;
		std::vector<uint8_t> after_native;
		result = quest_mobile_native_image_encode(candidate.native_after, &after_native);
		if (result != player_snapshot_codec_result::ok)
			return result;

		candidate.player_before = player_before;
		// Original obj_to_char inserts before the first carried root with the
		// same actual R_num; if there is no match, it prepends to carried roots.
		// Equipment and preceding unrelated carried trees retain their order.
		size_t insertion = player_before.size();
		uint64_t before_uid = 0;
		for (const auto &root : original_grant.original_carried_roots)
			if (root.original_rnum == original_grant.original_output_rnum)
			{
				before_uid = root.object_uid;
				break;
			}
		if (!before_uid && !original_grant.original_carried_roots.empty())
			before_uid = original_grant.original_carried_roots.front().object_uid;
		for (size_t i = 0; i < player_before.size(); ++i)
			if (player_before[i].parent_index == PLAYER_SNAPSHOT_NO_PARENT &&
			    !player_before[i].equipment_slot &&
			    player_before[i].object_uid == before_uid)
			{
				insertion = i;
				break;
			}
		const int32_t inserted = static_cast<int32_t>(terms.frozen_outputs.size());
		candidate.player_after.reserve(player_before.size() + terms.frozen_outputs.size());
		for (size_t i = 0; i <= player_before.size(); ++i)
		{
			if (i == insertion)
				for (auto row : terms.frozen_outputs)
				{
					if (row.parent_index == PLAYER_SNAPSHOT_NO_PARENT &&
					    !row.generated_key &&
					    original_grant.original_player_level < 57 &&
					    terms.player_pid < 10000000)
						row.generated_key = 1;
					if (row.parent_index != PLAYER_SNAPSHOT_NO_PARENT)
						row.parent_index += static_cast<int32_t>(insertion);
					candidate.player_after.push_back(std::move(row));
				}
			if (i == player_before.size())
				break;
			auto row = player_before[i];
			if (row.parent_index != PLAYER_SNAPSHOT_NO_PARENT &&
			    size_t(row.parent_index) >= insertion)
				row.parent_index += inserted;
			candidate.player_after.push_back(std::move(row));
		}
		std::vector<uint8_t> after_player;
		result = player_item_snapshot_list_encode(candidate.player_after, &after_player);
		if (result != player_snapshot_codec_result::ok)
			return result;
		*output = std::move(candidate);
		return player_snapshot_codec_result::ok;
	}
	catch (...)
	{
		return player_snapshot_codec_result::allocation_failure;
	}
}

#endif
