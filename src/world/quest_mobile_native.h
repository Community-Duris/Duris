#ifndef QUEST_MOBILE_NATIVE_H
#define QUEST_MOBILE_NATIVE_H

#include "economy/economic_accounting_plan.h"
#include "player/player_snapshot_codec.h"
#include "player/player_snapshot_capture.h"

#include <array>
#include <span>

constexpr uint16_t QUEST_MOBILE_NATIVE_VERSION = 1;
constexpr size_t QUEST_MOBILE_NATIVE_REFERENCE_BYTES = 148;
constexpr size_t QUEST_MOBILE_NATIVE_IMAGE_OVERHEAD = 216;

enum class quest_mobile_birth_provenance : uint8_t
{
	reset = 1,
	spawn = 2
};
enum class quest_mobile_lifetime_state : uint8_t
{
	live = 1,
	retired = 2
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

struct quest_mobile_native_image
{
	quest_mobile_native_reference reference;
	quest_mobile_lifetime_state state = {};
	critical_operation_id last_transition_operation = {};
	// Equipment roots in ascending one-based slot order, then carried roots in
	// native linked-list order; each complete subtree is contiguous depth-first.
	// The existing parent/slot representation preserves order without a second
	// UID/custody catalog. Every row freezes all four literal strings.
	std::vector<player_item_snapshot> items;
};

player_snapshot_codec_result quest_mobile_native_reference_encode(
	const quest_mobile_native_reference &,
	std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES> *output) noexcept;
player_snapshot_codec_result
quest_mobile_native_reference_decode(std::span<const uint8_t>,
				     quest_mobile_native_reference *output) noexcept;
player_snapshot_codec_result
quest_mobile_native_image_encode(const quest_mobile_native_image &,
				 std::vector<uint8_t> *output) noexcept;
player_snapshot_codec_result
quest_mobile_native_image_decode(std::span<const uint8_t>,
				 quest_mobile_native_image *output) noexcept;

// Read-only complete NPC forest, including NORENT items: this is native stock,
// not a player save/filtering decision. No UID adoption, runtime/custody write,
// birth decision, rebind, retirement or ACK. The supplied reference/operation
// remain caller facts; matching prototype/birthplace is not durable authority.
// Capture requires explicit LIVE input. RETIRED images are values constructed
// by the eventual native transition owner and must have an empty forest.
// Every output remains unchanged on failure. Game-thread serialization remains
// the caller's obligation, as for existing literal tree capture.
player_snapshot_capture_result quest_mobile_native_capture(
	P_char, const quest_mobile_native_reference &, quest_mobile_lifetime_state,
	const critical_operation_id &last_transition, quest_mobile_native_image *output) noexcept;

#endif
