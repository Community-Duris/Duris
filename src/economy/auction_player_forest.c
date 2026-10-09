#include "economy/auction_native_publication.h"
#include "economy/auction_player_forest_detail.h"
#include <algorithm>
#include <set>
#include <utility>

namespace
{
using auction_player_forest_detail::selected_ranges;
bool same_selected_values(std::span<const player_item_snapshot> observed,
			  std::span<const player_item_snapshot> selected)
{
	if (observed.size() != selected.size())
		return false;
	for (const auto &row : observed)
	{
		auto found = std::find_if(selected.begin(), selected.end(), [&](const auto &x)
					  { return x.object_uid == row.object_uid; });
		if (found == selected.end())
			return false;
		const uint64_t a_parent =
			row.parent_index < 0 ? 0 : observed[row.parent_index].object_uid;
		const uint64_t b_parent =
			found->parent_index < 0 ? 0 : selected[found->parent_index].object_uid;
		if (a_parent != b_parent)
			return false;
		std::vector<player_item_snapshot> a{ row }, b{ *found };
		a[0].parent_index = b[0].parent_index = PLAYER_SNAPSHOT_NO_PARENT;
		std::vector<uint8_t> x, y;
		if (player_item_snapshot_list_encode(a, &x) != player_snapshot_codec_result::ok ||
		    player_item_snapshot_list_encode(b, &y) != player_snapshot_codec_result::ok ||
		    x != y)
			return false;
	}
	return true;
}
}

bool auction_native_selected_forest_valid(const auction_command_payload &payload,
					  std::span<const player_item_snapshot> selected) noexcept
{
	try
	{
		std::vector<std::pair<size_t, size_t>> ranges;
		return selected_ranges(payload, selected, &ranges);
	}
	catch (...)
	{
		return false;
	}
}

bool auction_native_expected_player_forest(const auction_command_payload &payload,
					   std::span<const player_item_snapshot> original_before,
					   std::span<const player_item_snapshot> original_selected,
					   bool rejected, uint32_t original_actor_level,
					   std::vector<player_item_snapshot> *out) noexcept
{
	try
	{
		if (!out || original_before.size() > PLAYER_SNAPSHOT_MAX_OBJECTS ||
		    payload.item_count > AUCTION_COMMAND_MAX_ITEMS)
			return false;
		std::vector<player_item_snapshot> before(original_before.begin(),
							 original_before.end());
		std::vector<uint8_t> encoded;
		if (player_item_snapshot_list_encode(before, &encoded) !=
		    player_snapshot_codec_result::ok)
			return false;
		std::set<uint64_t> identities;
		for (const auto &row : before)
			if (!row.object_uid || !identities.insert(row.object_uid).second)
				return false;
		const bool listing = payload.action == auction_action::list,
			   claiming = payload.action == auction_action::claim_item;
		if ((!listing && !claiming) || rejected)
		{
			*out = std::move(before);
			return true;
		}
		if (!payload.actor_pid || !original_actor_level || !payload.item_count)
			return false;
		std::vector<std::pair<size_t, size_t>> ranges;
		if (!selected_ranges(payload, original_selected, &ranges))
			return false;
		if (listing)
		{
			std::vector<uint64_t> roots;
			for (const auto &range : ranges)
				roots.push_back(original_selected[range.first].object_uid);
			std::sort(roots.begin(), roots.end());
			std::vector<player_item_snapshot> removed, after;
			if (player_item_snapshot_extract_forest(before, roots, &removed, &after) !=
				    player_snapshot_codec_result::ok ||
			    !same_selected_values(removed, original_selected))
				return false;
			*out = std::move(after);
			return true;
		}
		if (before.size() > PLAYER_SNAPSHOT_MAX_OBJECTS - original_selected.size())
			return false;
		std::vector<player_item_snapshot> after = std::move(before);
		for (const auto &[begin, end] : ranges)
		{
			const auto &root = original_selected[begin];
			size_t head = after.size(), same = after.size();
			for (size_t i = 0; i < after.size(); ++i)
				if (after[i].parent_index == PLAYER_SNAPSHOT_NO_PARENT &&
				    !after[i].equipment_slot)
				{
					if (head == after.size())
						head = i;
					if (after[i].vnum == root.vnum)
					{
						same = i;
						break;
					}
				}
			const size_t position = same != after.size() ? same : head;
			const size_t count = end - begin;
			for (auto &row : after)
				if (row.parent_index >= 0 &&
				    static_cast<size_t>(row.parent_index) >= position)
					row.parent_index += static_cast<int32_t>(count);
			std::vector<player_item_snapshot> tree;
			tree.reserve(count);
			for (size_t i = begin; i < end; ++i)
			{
				player_item_snapshot row = original_selected[i];
				if (!identities.insert(row.object_uid).second)
					return false;
				if (row.parent_index >= 0)
					row.parent_index = static_cast<int32_t>(position) +
							   row.parent_index -
							   static_cast<int32_t>(begin);
				// Only obj_to_char's placed root receives the original g_key rule;
				// existing descendants retain their own exact literal metadata.
				if (i == begin && !row.generated_key && original_actor_level < 57 &&
				    payload.actor_pid < 10000000)
					row.generated_key = 1;
				tree.push_back(std::move(row));
			}
			after.insert(after.begin() + position, tree.begin(), tree.end());
		}
		if (player_item_snapshot_list_encode(after, &encoded) !=
		    player_snapshot_codec_result::ok)
			return false;
		*out = std::move(after);
		return true;
	}
	catch (...)
	{
		return false;
	}
}
