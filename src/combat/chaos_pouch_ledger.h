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

// Genuine prospective counterparts. The caller outer includes full input
// snapshots/usage/prior output and all sibling owners; each physical heap is
// included once even when references alias. The implementation owns candidate,
// deep-copy initializer-list/vector, chunk/description/string and codec frames.
// Strong after output; no reserve/callback follows successful transfer. Original
// methods/native apply/overlay and selected gameplay remain unchanged. Supported
// allocator requests are pinned libstdc++13 C++11 ABI; no unbounded fallback.
chaos_pouch_ledger_result
chaos_pouch_ledger_prepare_bounded(const player_item_snapshot &,
				   std::span<const chaos_material_pouch_usage>,
				   chaos_pouch_usage_mode, player_item_snapshot *,
				   bool (*)(size_t, void *) noexcept, void *, size_t) noexcept;
chaos_pouch_ledger_result
chaos_pouch_ledger_verify_bounded(const player_item_snapshot &, const player_item_snapshot &,
				  std::span<const chaos_material_pouch_usage>,
				  chaos_pouch_usage_mode, bool (*)(size_t, void *) noexcept, void *,
				  size_t) noexcept;

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

// Genuine controller present before the original first admit callback. The
// later prepare_frame/verify_frame and all Source/library work are separate;
// this is not the mixed original own allowance or any retained heap baseline.
bool chaos_pouch_ledger_prepare_initial_inline_bytes(size_t *) noexcept;
constexpr size_t chaos_pouch_ledger_prepare_initial_query_frame_bytes() noexcept
{
	// Getter output pointer and returned bool; scalar result is caller-owned.
	return sizeof(size_t *) + sizeof(bool);
}
bool chaos_pouch_ledger_verify_initial_inline_bytes(size_t *) noexcept;
constexpr size_t chaos_pouch_ledger_verify_initial_query_frame_bytes() noexcept
{
	// Getter output pointer and returned bool; scalar result is caller-owned.
	return sizeof(size_t *) + sizeof(bool);
}

// Pure, strong-output Source profiles for the actual unchanged bounded ledger
// methods. Callers own inputs/usage/prior output and physical alias heaps once.
// Admit full+initial transiently before entry; retain only the positive genuine
// supplement in outer during the original child. Historical library inventories
// are not full providers. Queries allocate nothing and observe no live objects.
bool chaos_pouch_ledger_prepare_source_frame_bytes(size_t *) noexcept;
bool chaos_pouch_ledger_prepare_source_supplement_frame_bytes(size_t *) noexcept;
bool chaos_pouch_ledger_verify_source_frame_bytes(size_t *) noexcept;
bool chaos_pouch_ledger_verify_source_supplement_frame_bytes(size_t *) noexcept;
// Export output+return; helper output/two flags/return+two local size_t;
// selected lower query output/policy/return; checked-add ref/size/return.
constexpr size_t chaos_pouch_ledger_prepare_source_query_frame_bytes() noexcept
{
	return 4 * sizeof(void *) + 3 * sizeof(size_t) + 7 * sizeof(bool);
}
constexpr size_t chaos_pouch_ledger_verify_source_query_frame_bytes() noexcept
{
	return 4 * sizeof(void *) + 3 * sizeof(size_t) + 7 * sizeof(bool);
}

#endif
