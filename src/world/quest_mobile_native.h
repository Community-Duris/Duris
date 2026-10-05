#ifndef QUEST_MOBILE_NATIVE_H
#define QUEST_MOBILE_NATIVE_H

#include "economy/economic_accounting_plan.h"
#include "economy/currency_command.h"
#include "world/quest_mobile_native_reference.h"
#include "player/player_snapshot_codec.h"
#include "player/player_snapshot_capture.h"

#include <array>
#include <optional>
#include <span>

// Keep the original image and 148-byte reference formats readable verbatim.
constexpr size_t QUEST_MOBILE_NATIVE_IMAGE_OVERHEAD = 216;
constexpr uint16_t QUEST_MOBILE_NATIVE_CASH_IMAGE_VERSION = 2;
constexpr size_t QUEST_MOBILE_NATIVE_CASH_IMAGE_OVERHEAD = 256;

enum class quest_mobile_lifetime_state : uint8_t
{
	live = 1,
	retired = 2
};

struct quest_mobile_native_cash
{
	uint64_t revision = 0;
	currency_vector denominations{};
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
	// Absent in historical v1 images: unknown cash, never an authoritative zero.
	// A v2 image freezes the actual native denominations, not converted value.
	std::optional<quest_mobile_native_cash> cash;
};

player_snapshot_codec_result
quest_mobile_native_image_encode(const quest_mobile_native_image &,
				 std::vector<uint8_t> *output) noexcept;
player_snapshot_codec_result
quest_mobile_native_image_decode(std::span<const uint8_t>,
				 quest_mobile_native_image *output) noexcept;

// Pure native cash revision policy, not admitted source/transition authority.
// Missing BEFORE means birth; unknown historical cash cannot be adopted here.
// A cash change advances both its revision and mobile revision exactly once.
bool quest_mobile_native_cash_transition_valid(const quest_mobile_native_image *before,
					       const quest_mobile_native_image &after) noexcept;

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

// Cash-aware capture for the original economic owner. The caller supplies its
// retained cash revision; this reads the actual post-conversion native wallet.
// The compatibility overload above still returns a v1 image with unknown cash.
player_snapshot_capture_result
quest_mobile_native_capture(P_char, const quest_mobile_native_reference &,
			    quest_mobile_lifetime_state,
			    const critical_operation_id &last_transition, uint64_t cash_revision,
			    quest_mobile_native_image *output) noexcept;

#endif
