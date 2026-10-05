#ifndef DURIS_SHOP_TRADE_DESTINATION_WEIGHT_H
#define DURIS_SHOP_TRADE_DESTINATION_WEIGHT_H

#include "core/defines.h"
#include "player/player_snapshot.h"
#include <climits>
#include <cstdint>

// Original game-thread native facts only; values alone grant no authority.
// direct_contents_weight includes every direct child, including unsaved NORENT.
// Shell is the actual original native probe result, never a worker template.
struct shop_trade_destination_weight
{
	int32_t before = 0, direct_contents = 0, shell = 0, after = 0;
	bool operator==(const shop_trade_destination_weight &) const = default;
};

inline bool shop_trade_destination_weight_compute(const player_item_snapshot &destination,
						  int32_t selected_weight, int32_t direct_contents,
						  int32_t shell,
						  shop_trade_destination_weight *output,
						  int32_t *correction = nullptr) noexcept
{
	if (!output || destination.type != ITEM_CONTAINER || destination.equipment_slot ||
	    destination.parent_index != PLAYER_SNAPSHOT_NO_PARENT)
		return false;
	const int32_t percentage = destination.values[4] <= 0 ? 0 :
				   destination.values[4] > 90 ? 90 :
								destination.values[4];
	const int64_t intermediate = int64_t(destination.weight) + selected_weight;
	if (intermediate < INT32_MIN || intermediate > INT32_MAX)
		return false;
	int64_t after = intermediate;
	if (!percentage)
	{
		if (direct_contents || shell)
			return false;
	}
	else
	{
		int64_t contents = int64_t(direct_contents) + selected_weight;
		if (contents < INT32_MIN || contents > INT32_MAX)
			return false;
		if (contents > 0)
			contents -= contents * percentage / 100;
		after = int64_t(shell) + contents;
	}
	const int64_t delta = after - intermediate;
	if (after < INT32_MIN || after > INT32_MAX || delta < INT32_MIN || delta > INT32_MAX)
		return false;
	const shop_trade_destination_weight staged{ destination.weight, direct_contents, shell,
						    static_cast<int32_t>(after) };
	*output = staged;
	if (correction)
		*correction = static_cast<int32_t>(delta);
	return true;
}

inline bool shop_trade_destination_weight_verify(const player_item_snapshot &destination,
						 int32_t selected_weight,
						 const shop_trade_destination_weight &frozen,
						 int32_t *correction = nullptr) noexcept
{
	shop_trade_destination_weight calculated;
	int32_t delta = 0;
	if (!shop_trade_destination_weight_compute(destination, selected_weight,
						   frozen.direct_contents, frozen.shell,
						   &calculated, &delta) ||
	    calculated != frozen)
		return false;
	if (correction)
		*correction = delta;
	return true;
}

#endif
