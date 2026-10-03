#ifndef CHAOS_POUCH_LEDGER_H
#define CHAOS_POUCH_LEDGER_H

#include "player/player_snapshot.h"
#include "combat/chaos_pouch_types.h"

#include <cstdint>
#include <span>

enum class chaos_pouch_ledger_result : uint8_t
{
	ok,
	invalid,
	overflow,
	capacity,
};

// Construct an immutable after-image without changing the live pouch. Its UID,
// topology and all non-ledger state remain identical to the before-image.
chaos_pouch_ledger_result
chaos_pouch_ledger_prepare(const player_item_snapshot &before,
			   std::span<const chaos_material_pouch_usage> usage,
			   chaos_pouch_usage_mode mode, player_item_snapshot *after);

// Native owners verify the frozen mutation against their locked before-image.
// The canonical snapshot comparison also rejects unrelated payload changes.
chaos_pouch_ledger_result
chaos_pouch_ledger_verify(const player_item_snapshot &before, const player_item_snapshot &after,
			  std::span<const chaos_material_pouch_usage> usage,
			  chaos_pouch_usage_mode mode);

// Apply only the counter mutation to a locked native item. Other native fields
// may have advanced since capture and must not be replaced by the live image.
chaos_pouch_ledger_result chaos_pouch_ledger_apply_native(
	const player_item_snapshot &frozen_before, const player_item_snapshot &frozen_after,
	std::span<const chaos_material_pouch_usage> usage, chaos_pouch_usage_mode mode,
	const player_item_snapshot &native_before, player_item_snapshot *native_after);

// Recover authoritative counters without replacing other snapshot attributes.
chaos_pouch_ledger_result chaos_pouch_ledger_overlay(const player_item_snapshot &ledger_source,
						     const player_item_snapshot &current,
						     player_item_snapshot *overlaid);

bool chaos_pouch_ledger_counters_equal(const player_item_snapshot &left,
				       const player_item_snapshot &right);

#endif
