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

#include "player/player_snapshot_codec.h"

// PRIVATE additive complete entry contract. Not selected until complete
// per-entry retained-own Source and the owning implementations are sealed.
// Public full is the actual prospective owning graph, including nested
// self-owning children. Parent retains its own Source once; supplement0
// describes the new self-owning entry only, never the original old bounded API.
bool craft_pouch_mutation_encode_complete_bounded(const craft_pouch_mutation &,
						  std::vector<uint8_t> *,
						  bool (*)(size_t, void *) noexcept, void *,
						  size_t) noexcept;
bool craft_pouch_mutation_decode_complete_bounded(std::span<const uint8_t>, craft_pouch_mutation *,
						  bool (*)(size_t, void *) noexcept, void *,
						  size_t) noexcept;
bool craft_pouch_mutation_from_payload_complete_bounded(const item_transfer_payload &,
							craft_pouch_mutation *,
							bool (*)(size_t, void *) noexcept, void *,
							size_t) noexcept;
bool craft_pouch_mutation_encode_complete_source_frame_bytes(size_t *) noexcept;
bool craft_pouch_mutation_encode_complete_source_supplement_frame_bytes(size_t *) noexcept;
bool craft_pouch_mutation_encode_complete_initial_inline_bytes(size_t *) noexcept;
bool craft_pouch_mutation_decode_complete_source_frame_bytes(size_t *) noexcept;
bool craft_pouch_mutation_decode_complete_source_supplement_frame_bytes(size_t *) noexcept;
bool craft_pouch_mutation_decode_complete_initial_inline_bytes(size_t *) noexcept;
bool craft_pouch_mutation_from_payload_complete_source_frame_bytes(size_t *) noexcept;
bool craft_pouch_mutation_from_payload_complete_source_supplement_frame_bytes(size_t *) noexcept;
bool craft_pouch_mutation_from_payload_complete_initial_inline_bytes(size_t *) noexcept;

constexpr size_t craft_pouch_mutation_encode_complete_source_query_frame_bytes() noexcept
{
	// Public getter owns outputP and ledger/list/total3N plus returnB.
	// Both genuine checked-add occurrences are conservatively retained;
	// their by-value N is separate from the caller's live returned scalars.
	return sizeof(size_t *) + 3 * sizeof(size_t) + sizeof(bool) +
	       chaos_pouch_ledger_verify_source_query_frame_bytes() +
	       player_item_snapshot_list_encode_source_query_frame_bytes() +
	       2 * (sizeof(void *) + sizeof(size_t) + sizeof(bool));
}
constexpr size_t craft_pouch_mutation_decode_complete_source_query_frame_bytes() noexcept
{
	return sizeof(size_t *) + 3 * sizeof(size_t) + sizeof(bool) +
	       player_item_snapshot_list_decode_bounded_source_query_frame_bytes() +
	       craft_pouch_mutation_encode_complete_source_query_frame_bytes() +
	       2 * (sizeof(void *) + sizeof(size_t) + sizeof(bool));
}
constexpr size_t craft_pouch_mutation_from_payload_complete_source_query_frame_bytes() noexcept
{
	// Public usage/total2N; internal usage-only getter child/total2N.
	// Both output/ref/results belong to actual distinct getter scopes.
	return sizeof(size_t *) + 2 * sizeof(size_t) + sizeof(bool) + sizeof(size_t *) +
	       2 * sizeof(size_t) + sizeof(bool) +
	       craft_pouch_mutation_decode_complete_source_query_frame_bytes() +
	       2 * (sizeof(void *) + sizeof(size_t) + sizeof(bool));
}

#endif
