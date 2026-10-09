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
// Allocation-free scan of the existing unversioned item-list wire format.
// Counts and logical encoded size are portable. Storage request totals describe
// fresh decoder vectors/strings under the supported libstdc++ 13 C++11 ABI policy
// only; callers must check fresh_decode_storage_policy_supported. Row storage
// includes nested container objects through sizeof(row), not heap metadata.
// No UID/relationship/semantic authority or allocator metadata accounting.
struct player_item_snapshot_list_allocation_profile
{
	size_t item_count = 0;
	size_t dynamic_affect_count = 0;
	size_t extra_description_count = 0;
	size_t spell_id_count = 0;
	size_t string_count = 0;
	size_t string_content_bytes = 0;
	size_t decoded_row_storage_bytes = 0;
	size_t decoded_string_storage_bytes = 0;
	size_t decoded_payload_bytes = 0;
	size_t relationship_scratch_bytes = 0;
	size_t canonical_encoded_bytes = 0;
	// Capacity and old+new reallocation peak for the fresh byte encoder vector,
	// excluding its inline object and semantic validation storage. Valid only
	// when canonical_encoder_storage_policy_supported is true.
	// Actual inline private encoder footprint; no guessed vector+flag padding.
	size_t canonical_encoder_object_bytes = 0;
	// Actual decoder inline object, distinct from its local decoded vector.
	size_t item_codec_decoder_object_bytes = 0;
	size_t canonical_encoded_capacity_bytes = 0;
	size_t canonical_encoded_reallocation_peak_bytes = 0;
	bool fresh_decode_storage_policy_supported = false;
	bool canonical_encoder_storage_policy_supported = false;
};

// Passive actual inline decoder footprint; no allocation or authority.
size_t player_item_snapshot_list_decoder_object_bytes() noexcept;

// Output remains unchanged on failure. A successful scan covers the full span
// before any item allocation; the existing semantic decode is still required.
player_snapshot_codec_result player_item_snapshot_list_preflight(
	const uint8_t *encoded, size_t encoded_size,
	player_item_snapshot_list_allocation_profile *profile_out) noexcept;

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
