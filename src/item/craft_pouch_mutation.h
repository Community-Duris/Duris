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

#endif
