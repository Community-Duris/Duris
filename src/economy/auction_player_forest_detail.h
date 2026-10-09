#ifndef AUCTION_PLAYER_FOREST_DETAIL_H
#define AUCTION_PLAYER_FOREST_DETAIL_H

#include "economy/auction_command.h"
#include "player/player_snapshot_codec.h"
#include <array>
#include <set>
#include <span>
#include <utility>
#include <vector>

// One original pure range validator shared by the server's publication owner
// and standalone player-forest provider. No world/source/ACK authority.
namespace auction_player_forest_detail
{
inline bool selected_ranges(const auction_command_payload &payload,
			    std::span<const player_item_snapshot> selected,
			    std::vector<std::pair<size_t, size_t>> *out)
{
	if (!out || selected.size() > PLAYER_SNAPSHOT_MAX_OBJECTS)
		return false;
	std::vector<player_item_snapshot> copy(selected.begin(), selected.end());
	std::vector<uint8_t> encoded;
	if (player_item_snapshot_list_encode(copy, &encoded) != player_snapshot_codec_result::ok)
		return false;
	std::vector<std::pair<size_t, size_t>> ranges;
	std::set<uint64_t> unique;
	std::array<size_t, PLAYER_SNAPSHOT_MAX_DEPTH> ancestors{};
	size_t depth = 0;
	for (size_t i = 0; i < selected.size(); ++i)
	{
		const auto &row = selected[i];
		if (!row.object_uid || row.object_uid == UINT64_MAX || row.equipment_slot ||
		    row.string_mask != 15 || !unique.insert(row.object_uid).second)
			return false;
		if (row.parent_index == PLAYER_SNAPSHOT_NO_PARENT)
		{
			const size_t slot = ranges.size();
			if (slot >= payload.item_count ||
			    row.object_uid != payload.items[slot].item_uid ||
			    row.vnum != payload.items[slot].vnum)
				return false;
			if (!ranges.empty())
				ranges.back().second = i;
			ranges.emplace_back(i, selected.size());
			depth = 1;
			ancestors[0] = i;
		}
		else
		{
			if (row.parent_index < 0 || static_cast<size_t>(row.parent_index) >= i)
				return false;
			while (depth &&
			       ancestors[depth - 1] != static_cast<size_t>(row.parent_index))
				--depth;
			if (!depth || depth == ancestors.size())
				return false;
			ancestors[depth++] = i;
		}
	}
	if (ranges.size() != payload.item_count)
		return false;
	*out = std::move(ranges);
	return true;
}
}

#endif
