#ifndef DURIS_SHOP_TRADE_ITEM_PAYLOAD_H
#define DURIS_SHOP_TRADE_ITEM_PAYLOAD_H

#include "economy/shop_trade_command.h"
#include "player/player_snapshot_codec.h"
#include "core/defines.h"
#include <algorithm>
#include <new>
#include <utility>

// Pure v6 after-image derivation. Neither this original snapshot nor the
// frozen status fields prove native authority: the transaction must verify
// both locked preimages. Historical proof uses these original fields only.
// No template defaults, current player level or new UID are consulted.
inline bool shop_trade_accounted_after_items(const shop_trade_payload &payload,
					     std::vector<player_item_snapshot> *output)
{
	if (!output || !payload.expected_player_save_revision || !payload.expected_player_level ||
	    payload.expected_player_level > UINT8_MAX || !payload.item_blob_size ||
	    payload.item_blob_size > payload.item_blob.size() || !payload.item_count ||
	    payload.item_count > payload.items.size())
		return false;
	try
	{
		std::vector<uint8_t> encoded;
		if (!(payload.recovery_manifest_recorded ?
			      shop_trade_command_encode_recovery_payload(payload, &encoded) :
		      payload.native_destination_weight_recorded ?
			      shop_trade_command_encode_native_payload(payload, &encoded) :
			      shop_trade_command_encode_accounted_payload(payload, &encoded)))
			return false;
		std::vector<player_item_snapshot> candidate;
		if (player_item_snapshot_list_decode(payload.item_blob.data(),
						     payload.item_blob_size, &candidate) !=
			    player_snapshot_codec_result::ok ||
		    candidate.size() != payload.item_count || candidate.empty() ||
		    candidate.front().object_uid != payload.selected_item_uid ||
		    candidate.front().parent_index != -1)
			return false;
		std::array<bool, SHOP_TRADE_MAX_ITEMS> matched = {};
		for (size_t index = 0; index < candidate.size(); ++index)
		{
			const auto &item = candidate[index];
			if (item.equipment_slot || (index && item.parent_index < 0))
				return false;
			const auto found = std::find_if(
				payload.items.begin(), payload.items.begin() + payload.item_count,
				[&](const auto &entry)
				{ return entry.item_uid == item.object_uid; });
			if (found == payload.items.begin() + payload.item_count ||
			    found->vnum != item.vnum ||
			    found->root_item_uid != payload.selected_item_uid)
				return false;
			const size_t position = static_cast<size_t>(found - payload.items.begin());
			if (matched[position] ||
			    (index ? (item.parent_index < 0 ||
				      static_cast<size_t>(item.parent_index) >= index) :
				     item.parent_index != -1))
				return false;
			const uint64_t parent_uid =
				index ? candidate[static_cast<size_t>(item.parent_index)]
						.object_uid :
					0;
			if (found->parent_item_uid != parent_uid)
				return false;
			matched[position] = true;
		}
		if (!std::all_of(matched.begin(), matched.begin() + payload.item_count,
				 [](bool present) { return present; }))
			return false;
		if (payload.action == shop_trade_action::buy_existing ||
		    payload.action == shop_trade_action::buy_produced)
		{
			candidate.front().extra2_flags |= ITEM2_STOREITEM;
			if (!candidate.front().generated_key &&
			    payload.expected_player_level < 57 && payload.player_pid < 10000000)
				candidate.front().generated_key = 1;
		}
		*output = std::move(candidate);
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

#endif
