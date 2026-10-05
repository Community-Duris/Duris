#ifndef QUEST_MOBILE_NATIVE_H
#define QUEST_MOBILE_NATIVE_H

#include "economy/economic_accounting_plan.h"
#include "world/quest_mobile_native_reference.h"
#include "player/player_snapshot_codec.h"
#include "player/player_snapshot_capture.h"

#include <array>
#include <span>

constexpr size_t QUEST_MOBILE_NATIVE_IMAGE_OVERHEAD = 216;

enum class quest_mobile_lifetime_state : uint8_t
{
	live = 1,
	retired = 2
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
