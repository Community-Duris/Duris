#ifndef DURIS_COLLECTOR_COLLECTION_IMAGE_H
#define DURIS_COLLECTOR_COLLECTION_IMAGE_H

#include "economy/collector_command.h"
#include "player/player_snapshot_codec.h"

#include <limits>
#include <new>
#include <span>
#include <utility>

// Prepare the literal collected singleton from owned state. The native owner
// still proves custody/topology and releases the direct children; this image
// grants no authority to detach or transfer the selected item.
inline bool collector_collection_prepare_image(player_item_snapshot selected,
					       std::span<const int64_t> direct_child_weights,
					       std::vector<uint8_t> *blob)
{
	if (!blob)
		return false;
	int64_t children_weight = 0;
	for (int64_t weight : direct_child_weights)
	{
		if (weight < 0 || weight > std::numeric_limits<int64_t>::max() - children_weight)
			return false;
		children_weight += weight;
	}
	if (selected.weight < children_weight)
		return false;
	const int64_t own_weight = static_cast<int64_t>(selected.weight) - children_weight;
	if (own_weight < 0 || own_weight > std::numeric_limits<int32_t>::max())
		return false;
	selected.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	selected.equipment_slot = 0;
	selected.weight = static_cast<int32_t>(own_weight);
	std::vector<player_item_snapshot> exact;
	try
	{
		exact.push_back(std::move(selected));
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	return player_item_snapshot_list_encode(exact, blob) == player_snapshot_codec_result::ok &&
	       !blob->empty() && blob->size() <= COLLECTOR_COMMAND_ITEM_BLOB_MAX_BYTES;
}

#endif
