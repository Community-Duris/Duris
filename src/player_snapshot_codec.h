#ifndef PLAYER_SNAPSHOT_CODEC_H
#define PLAYER_SNAPSHOT_CODEC_H

#include "player_snapshot.h"

#include <cstdint>
#include <vector>

enum class player_snapshot_codec_result : uint8_t
{
	ok,
	invalid_value,
	truncated,
	limit_exceeded,
	unsupported_version,
	allocation_failure,
};

player_snapshot_codec_result player_snapshot_encode(const player_snapshot &snapshot,
						    std::vector<uint8_t> *encoded_out);
player_snapshot_codec_result player_snapshot_decode(const uint8_t *encoded, size_t encoded_size,
						    player_snapshot *snapshot_out);
player_snapshot_codec_result
player_item_snapshot_list_encode(const std::vector<player_item_snapshot> &items,
				 std::vector<uint8_t> *encoded_out);
player_snapshot_codec_result
player_item_snapshot_list_decode(const uint8_t *encoded, size_t encoded_size,
				 std::vector<player_item_snapshot> *items_out);
player_snapshot_codec_result
player_item_snapshot_extract_subtree(const std::vector<player_item_snapshot> &items,
				     uint64_t selected_uid,
				     std::vector<player_item_snapshot> *selected_out,
				     std::vector<player_item_snapshot> *remaining_out);

#endif
