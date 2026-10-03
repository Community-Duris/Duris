#ifndef PLAYER_SNAPSHOT_CODEC_H
#define PLAYER_SNAPSHOT_CODEC_H

#include "player/player_snapshot.h"

#include <cstdint>
#include <string>
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
constexpr size_t PLAYER_ITEM_PROPERTIES_MAX_BYTES = 16 + PLAYER_SNAPSHOT_MAX_ROWS * 12;
constexpr size_t PLAYER_ITEM_PROPERTIES_MAX_HEX_BYTES = PLAYER_ITEM_PROPERTIES_MAX_BYTES * 2;

player_snapshot_codec_result player_item_properties_encode(
	uint32_t extra2_flags,
	const std::vector<player_item_dynamic_affect_snapshot> &dynamic_affects,
	std::string *encoded_hex_out);
player_snapshot_codec_result player_item_properties_decode(
	const std::string &encoded_hex, uint32_t *extra2_flags_out,
	std::vector<player_item_dynamic_affect_snapshot> *dynamic_affects_out);
const char *player_item_properties_sql_column_suffix();
player_snapshot_codec_result player_item_properties_sql_value_suffix(
	uint32_t extra2_flags,
	const std::vector<player_item_dynamic_affect_snapshot> &dynamic_affects,
	std::string *value_suffix_out);
player_snapshot_codec_result player_item_properties_decode_sql_row(
	const char *encoded_hex, const char *octet_length, uint32_t *extra2_flags_out,
	std::vector<player_item_dynamic_affect_snapshot> *dynamic_affects_out,
	bool *has_payload_out);
player_snapshot_codec_result
player_item_snapshot_extract_subtree(const std::vector<player_item_snapshot> &items,
				     uint64_t selected_uid,
				     std::vector<player_item_snapshot> *selected_out,
				     std::vector<player_item_snapshot> *remaining_out);
player_snapshot_codec_result
player_item_snapshot_extract_forest(const std::vector<player_item_snapshot> &items,
				    const std::vector<uint64_t> &selected_root_uids,
				    std::vector<player_item_snapshot> *selected_out,
				    std::vector<player_item_snapshot> *remaining_out);

#endif
