#include "economy/shop_trade_recovery_image.h"

#include "player/player_snapshot_codec.h"
#include <climits>
#include <map>
#include <utility>

#ifndef __NO_MYSQL__
bool shop_trade_recovery_image_reconstruct(const shop_trade_recovery_forest_binding &binding,
					   shop_trade_recovery_forest_role role,
					   const shop_item_runtime_image &image,
					   std::vector<player_item_snapshot> *output) noexcept
{
	if (!output || !binding.present || binding.ordered_item_uids.size() != image.size() ||
	    image.size() > PLAYER_SNAPSHOT_MAX_OBJECTS)
		return false;
	try
	{
		std::vector<player_item_snapshot> candidate;
		std::vector<uint64_t> roots;
		std::map<uint64_t, int32_t> current_positions;
		candidate.reserve(image.size());
		roots.reserve(image.size());
		for (const auto uid : binding.ordered_item_uids)
		{
			const auto found = image.find(uid);
			if (!uid || uid == UINT64_MAX || found == image.end())
				return false;
			const auto &row = found->second;
			if (!row.payload_present || !row.id || row.id > INT_MAX ||
			    row.parent_id > INT_MAX || !row.revision ||
			    row.item.object_uid != uid || row.slot != row.item.equipment_slot)
				return false;
			const auto index = static_cast<int32_t>(candidate.size());
			// Resolve only from parents already present in the retained original
			// order. Self, later, missing and cross-image edges cannot be repaired.
			int32_t parent = PLAYER_SNAPSHOT_NO_PARENT;
			uint64_t root_uid = uid;
			if (row.parent_id)
			{
				const auto parent_row = current_positions.find(row.parent_id);
				if (parent_row == current_positions.end() || row.slot)
					return false;
				parent = parent_row->second;
				root_uid = roots[static_cast<size_t>(parent)];
			}
			if (row.root_uid != root_uid ||
			    !current_positions.emplace(row.id, index).second)
				return false;
			candidate.push_back(row.item);
			candidate.back().parent_index = parent;
			roots.push_back(root_uid);
		}
		std::vector<uint8_t> canonical;
		if (player_item_snapshot_list_encode(candidate, &canonical) !=
			    player_snapshot_codec_result::ok ||
		    !shop_trade_recovery_forest_verify(canonical, role, binding))
			return false;
		*output = std::move(candidate);
		return true;
	}
	catch (...)
	{
		return false;
	}
}
#endif
