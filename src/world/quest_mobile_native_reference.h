#ifndef QUEST_MOBILE_NATIVE_REFERENCE_H
#define QUEST_MOBILE_NATIVE_REFERENCE_H

#include "economy/economic_source_event.h"

#include <array>
#include <span>

enum class player_snapshot_codec_result : uint8_t;
constexpr uint16_t QUEST_MOBILE_NATIVE_VERSION = 1;
constexpr size_t QUEST_MOBILE_NATIVE_REFERENCE_BYTES = 148;

enum class quest_mobile_birth_provenance : uint8_t
{
	reset = 1,
	spawn = 2
};
// Caller-supplied original native facts, not authenticated by these value codecs.
// A fresh native birth owner obtains mobile_instance_id from the reserved UID
// allocator; restore keeps that original value. This module never issues IDs.
struct quest_mobile_native_reference
{
	uint64_t mobile_instance_id = 0;
	critical_operation_id birth_operation = {};
	// Exact existing source/generation/sequence/slot semantics. For reset,
	// slot identifies the original reset command; no source decision is made here.
	economic_source_event birth_source = {};
	int32_t mobile_vnum = 0;
	int32_t birthplace_vnum = 0; // Preserve native zero/NOWHERE facts verbatim.
	int32_t reset_zone_vnum = -1; // Nonnegative for reset; -1 for spawn.
	quest_mobile_birth_provenance provenance = {};
	uint64_t mobile_revision = 0;
	uint64_t stock_revision = 0;
};

// Value validation only; no native birth/source/custody or ACK authority.
bool quest_mobile_native_reference_valid(const quest_mobile_native_reference &) noexcept;

player_snapshot_codec_result quest_mobile_native_reference_encode(
	const quest_mobile_native_reference &,
	std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES> *output) noexcept;
player_snapshot_codec_result
quest_mobile_native_reference_decode(std::span<const uint8_t>,
				     quest_mobile_native_reference *output) noexcept;
// Complete original fixed reference codecs/validation with genuine prospective
// source-event/checksum/SHA frames. Same 148 bytes, digest, original version and
// provenance laws; no native UID/source/custody/admission authority or selection.
// Outer includes real input/prior output/caller state; strong output on refusal.
// Codecs distinguish checked-size overflow (limit_exceeded), unsupported
// profile/reserve refusal (allocation_failure), and invalid native values.
// Public bool validation keeps its existing false-on-refusal contract.
bool quest_mobile_native_reference_valid_bounded(const quest_mobile_native_reference &,
						 bool (*reserve)(size_t, void *) noexcept,
						 void *context, size_t outer) noexcept;
player_snapshot_codec_result quest_mobile_native_reference_encode_bounded(
	const quest_mobile_native_reference &,
	std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES> *,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer) noexcept;
player_snapshot_codec_result quest_mobile_native_reference_decode_bounded(
	std::span<const uint8_t>, quest_mobile_native_reference *,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer) noexcept;

#endif
