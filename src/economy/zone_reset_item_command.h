#ifndef ZONE_RESET_ITEM_COMMAND_H
#define ZONE_RESET_ITEM_COMMAND_H

#include "economy/economic_accounting_intent.h"
#include "economy/native_mobile_birth_recipe.h"
#include <optional>

constexpr uint32_t ECONOMIC_WRITER_ZONE_RESET_ITEM_BIRTH = 16;
constexpr uint16_t ZONE_RESET_ITEM_PAYLOAD_VERSION = 1;
constexpr uint16_t ZONE_RESET_ITEM_PLACEMENT_PAYLOAD_VERSION = 2;

// Original synchronous placement cut. This records the actual short-circuit
// decision, not an eventual falling destination or any admission authority.
struct zone_reset_room_placement_recipe
{
	uint64_t root_uid = 0;
	int32_t room_vnum = 0;
	int32_t original_sector_type = 0;
	int32_t original_chance_fall = 0;
	int32_t original_z_cord = 0;
	bool original_levitates = false;
	bool fall_roll_drawn = false;
	uint32_t fall_roll = 0;
	bool fall_selected = false;
	bool operator==(const zone_reset_room_placement_recipe &) const = default;
};
bool zone_reset_room_placement_recipe_valid(const zone_reset_room_placement_recipe &) noexcept;

struct zone_reset_coin_output
{
	uint64_t item_uid = 0;
	economic_coin_vector denominations = {};
};

struct zone_reset_item_image
{
	critical_operation_id operation_id = {};
	economic_source_event reset_source = {};
	int32_t zone_vnum = -1;
	int32_t room_vnum = 0;
	uint64_t season_epoch = 0;
	uint64_t expected_room_revision = 0;
	// One original O root followed by its complete children; literal strings and
	// actual factory decisions are retained without template reconstruction.
	std::vector<player_item_snapshot> items;
	std::vector<native_mobile_birth_item_recipe> recipes;
	std::vector<zone_reset_coin_output> coins;
	std::optional<zone_reset_room_placement_recipe> placement;
};

// Pure canonical command construction/correlation. A source value, UID or
// decoded command grants no source admission, SQL, UID issuance, production
// authority, artifact ownership, live publication or recovery entitlement.
// The original reset owner must supply its actual invocation and command slot;
// atomic participants separately prove absent outputs, current season/room,
// source claim, supply/custody and room literal retention in the same root.
// All outputs remain unchanged on every refusal, including allocation failure.
economic_accounting_error zone_reset_item_command_build(const economic_operation_metadata &,
							const zone_reset_item_image &,
							uint64_t accepted_at_usec,
							critical_command *) noexcept;
economic_accounting_error zone_reset_item_command_decode(const critical_command &,
							 zone_reset_item_image *) noexcept;

// Prospective scratch admission for this original compiler only. Caller owns
// metadata/image/input/profile outputs and the old command output in outer_live.
// The callback admits an absolute peak before any original compiler allocation
// and must retain its high-water through return/output transfer. No source,
// factory, execution, activation or publication permission is granted.
economic_accounting_error zone_reset_item_command_build_bounded(
	const economic_operation_metadata &, const zone_reset_item_image &, uint64_t,
	critical_command *, bool (*reserve_scratch_peak)(size_t, void *) noexcept,
	void *context, size_t outer_live_scratch) noexcept;

// Passive staged bounded companion to the original complete command decoder.
// Caller includes complete command/input, prior image output and other retained
// storage in outer_live. Absolute reservations precede each allocating stage;
// caller retains the maximum through nested calls and final output transfer.
// Full literal/recipe/source/intent/canonical validation remains required and
// success grants no lifetime, factory, admission, publication or ACK authority.
economic_accounting_error zone_reset_item_command_decode_bounded(
	const critical_command &, zone_reset_item_image *,
	bool (*reserve_scratch_peak)(size_t, void *) noexcept, void *context,
	size_t outer_live_scratch) noexcept;

// Exact passive value closure for this image's generated default/move-assignment/
// cleanup, its item/recipe vectors, trivial coin vector and trivial optional.
// Caller owns actual input/prior output objects and all heap capacities, including
// old-output cleanup through genuine vector move temporaries. No codec/authority.
// Unsupported/null/overflow leaves scalar output untouched.
bool zone_reset_item_image_lifetime_source_frame_bytes(size_t *) noexcept;
constexpr size_t zone_reset_item_image_lifetime_source_query_frame_bytes() noexcept
{
	// Public output/three local results/return; both genuine lower queries;
	// checked add(ref,amount,result). Accessor's N result is caller-owned.
	return 4 * sizeof(void *) + 4 * sizeof(size_t) + 6 * sizeof(bool);
}

#include "player/player_snapshot_codec.h"

// Complete passive fixed companion to the original ROOM decoder. Original
// APIs/wire/error semantics stay authoritative; this companion substitutes the
// equivalent fixed binding/freezing proof at the same full canonical cuts.
// Caller owns input/prior image output and all retained foreign heaps. Pre-admit
// genuine query, then full Source + initial transiently. The child owns its
// actual missing Source internally and the external supplement is zero; neither
// Source nor successful decoding grants admission/publication/activation rights.
economic_accounting_error
zone_reset_item_command_decode_fixed_bounded(const critical_command &, zone_reset_item_image *,
					     bool (*)(size_t, void *) noexcept, void *,
					     size_t outer_live) noexcept;
bool zone_reset_item_command_decode_source_frame_bytes(size_t *) noexcept;
bool zone_reset_item_command_decode_source_supplement_frame_bytes(size_t *) noexcept;
bool zone_reset_item_command_decode_initial_inline_bytes(size_t *) noexcept;
constexpr size_t zone_reset_item_command_decode_source_query_frame_bytes() noexcept
{
	// Public output/full/retained/return; exact internal two outputs/15N/
	// returned bool/policy; checked add/max and actual valid-frame result.
	// Each genuine lower pure getter's complete typed query is composed here.
	// This accessor's own N result remains with its caller.
	return 7 * sizeof(void *) + 19 * sizeof(size_t) + 5 * sizeof(bool) +
	       player_item_snapshot_list_encode_source_query_frame_bytes() +
	       player_item_snapshot_list_decode_source_query_frame_bytes() +
	       player_item_snapshot_list_preflight_source_query_frame_bytes() +
	       3 * native_mobile_birth_recipe_source_query_frame_bytes() +
	       zone_reset_item_image_lifetime_source_query_frame_bytes() +
	       economic_operation_metadata_validate_source_query_frame_bytes() +
	       2 * economic_intent_decode_source_query_frame_bytes() +
	       economic_intent_freeze_fixed_source_query_frame_bytes() +
	       economic_intent_verify_binding_fixed_source_query_frame_bytes() +
	       critical_command_startup_codec_complete_source_query_frame_bytes();
}

#endif
