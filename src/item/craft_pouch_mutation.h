#ifndef CRAFT_POUCH_MUTATION_H
#define CRAFT_POUCH_MUTATION_H

#include "combat/chaos_pouch_ledger.h"

constexpr size_t CRAFT_POUCH_MUTATION_MAX_BYTES = 128 * 1024;
constexpr size_t CRAFT_POUCH_MUTATION_MAX_MATERIALS = 219;

struct craft_pouch_mutation
{
	chaos_pouch_usage_mode mode = chaos_pouch_usage_mode::generated;
	std::vector<chaos_material_pouch_usage> usage;
	player_item_snapshot before = {};
	player_item_snapshot after = {};
};

// One retained pouch, with normalized standalone snapshot topology. Actual
// custody topology/revision stays in the enclosing command's item entry.
bool craft_pouch_mutation_encode(const craft_pouch_mutation &mutation,
				 std::vector<uint8_t> *encoded);
bool craft_pouch_mutation_decode(std::span<const uint8_t> encoded, craft_pouch_mutation *mutation);

// A command without this continuation has no retained pouch (UID zero).
// Collection must retire the exact material counts recorded in the ledger.
bool craft_pouch_mutation_from_payload(const item_transfer_payload &payload,
				       craft_pouch_mutation *mutation);

// Complete original canonical pouch wire companions. Outer retains authentic
// input, prior output and all other caller-owned live storage. Each actual
// nested codec/singleton/candidate allocation is prospectively owned; no native
// effects or replay authority. from_payload is a separate owning dependency.
bool craft_pouch_mutation_encode_bounded(const craft_pouch_mutation &, std::vector<uint8_t> *,
					 bool (*reserve)(size_t, void *) noexcept, void *context,
					 size_t outer_live) noexcept;
bool craft_pouch_mutation_decode_bounded(std::span<const uint8_t>, craft_pouch_mutation *,
					 bool (*reserve)(size_t, void *) noexcept, void *context,
					 size_t outer_live) noexcept;

// Complete original pouch continuation proof, including full craft-recipe
// decode and genuine complete payload copy, map counts and canonical decode.
// Authentic outer includes input/prior output/all caller live owners. No new
// authority or native side effects; strong refusal preserves output.
bool craft_pouch_mutation_from_payload_bounded(const item_transfer_payload &,
					       craft_pouch_mutation *,
					       bool (*reserve)(size_t, void *) noexcept,
					       void *context, size_t outer_live) noexcept;

#endif
