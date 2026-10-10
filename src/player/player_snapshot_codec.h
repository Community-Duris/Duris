#ifndef PLAYER_SNAPSHOT_CODEC_H
#define PLAYER_SNAPSHOT_CODEC_H

#include "player/player_snapshot.h"

#include <cstdint>
#include <span>
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
// The passive wire scan's two simultaneously live named objects. Callback
// closures and scalar call frames follow the existing codec storage policy.
size_t player_item_snapshot_list_preflight_object_bytes() noexcept;

// Output remains unchanged on failure. A successful scan covers the full span
// before any item allocation; the existing semantic decode is still required.
player_snapshot_codec_result player_item_snapshot_list_preflight(
	const uint8_t *encoded, size_t encoded_size,
	player_item_snapshot_list_allocation_profile *profile_out) noexcept;

// Allocation-free profile of the actual live item-list encoder before it copies
// or allocates bytes. Models the same append sequence and fresh validation decode
// as the original encoder; original semantic encode/decode remains authoritative.
// Input/prior output/profile objects are caller-owned outer-live storage. Reserve
// the scan object's footprint before profiling, then retain the admitted peak
// through codec output transfer. Unsupported request policies cannot admit work.
size_t player_item_snapshot_list_encoder_preflight_object_bytes() noexcept;
player_snapshot_codec_result player_item_snapshot_list_encoder_preflight(
	const std::vector<player_item_snapshot> &,
	player_item_snapshot_list_allocation_profile *) noexcept;
// Same allocation-free scan for a contiguous range, including a normalized
// singleton before initializer-list/vector copies. The caller owns the input
// span and must admit it before construction; both overloads use the same
// private scan object's footprint and preserve strong profile output.
player_snapshot_codec_result player_item_snapshot_list_encoder_preflight(
	const std::span<const player_item_snapshot> &,
	player_item_snapshot_list_allocation_profile *) noexcept;
// Original encoder's maximum simultaneous object/request bytes, excluding its
// caller-owned input, prior output and profile. Includes both validation vectors,
// the decoder object, decoded rows/strings and relationship-validation scratch.
bool player_item_snapshot_list_encoder_working_bytes(
	const player_item_snapshot_list_allocation_profile &, size_t *) noexcept;
player_snapshot_codec_result player_item_snapshot_list_encode_bounded(
	const std::vector<player_item_snapshot> &, std::vector<uint8_t> *,
	bool (*reserve_scratch_peak)(size_t, void *) noexcept, void *context,
	size_t outer_live_scratch) noexcept;

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

// Full original unversioned item-list decoder with prospective real vector and
// string requests, all row/depth limits and strong output. Outer includes
// authentic input, prior output and every other live owner; excludes this
// callee's private workspace. No execution/native authority is granted.
player_snapshot_codec_result player_item_snapshot_list_decode_bounded(
	const uint8_t *, size_t, std::vector<player_item_snapshot> *,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer_live,
	size_t *retained_item_heap_bytes = nullptr) noexcept;

// Passive source/copy storage observations for the supported libstdc++13
// C++11 ABI. Values exclude the row inline object and allocator metadata.
// Current uses genuine capacities; a fresh copy uses size-based requests.
// No validation, custody, replay or native authority; strong scalar output.
bool player_item_snapshot_current_heap_bytes(const player_item_snapshot &, size_t *) noexcept;
bool player_item_snapshot_fresh_copy_request_bytes(const player_item_snapshot &, size_t *) noexcept;
// Source-declared copy/move/destruction and observer frames, not emitted stack.
// Caller construction of an outer vector/initializer-list is separately owned.
size_t player_item_snapshot_copy_frame_bytes() noexcept;
// Genuine complete private row copy then nonthrowing strong-output move. Outer
// includes actual source, prior destination and every other live caller owner;
// excludes this callee's prospective private row/frame. No runtime selection.
player_snapshot_codec_result
player_item_snapshot_clone_bounded(const player_item_snapshot &, player_item_snapshot *,
				   bool (*reserve)(size_t, void *) noexcept, void *context,
				   size_t outer_live) noexcept;

#endif
