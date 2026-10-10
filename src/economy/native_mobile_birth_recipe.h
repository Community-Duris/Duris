#ifndef NATIVE_MOBILE_BIRTH_RECIPE_H
#define NATIVE_MOBILE_BIRTH_RECIPE_H

#include "economy/economic_accounting_types.h"
#include "player/player_snapshot.h"

#include <cstdint>
#include <span>
#include <vector>

// Stable identities for the original detached factory's supported bindings.
// These are persisted values; never serialize function pointers or runtime indexes.
enum class native_mobile_birth_binding_form : uint8_t
{
	direct = 1,
	bridge = 2,
};
enum class native_mobile_birth_procedure : uint8_t
{
	none = 0,
	spell_pool = 1,
	super_cannon = 2,
	vecna_deathportal = 3,
	blood_stains = 4,
	zombies_game = 5,
	item_switch = 6,
	proclib_obj_proc = 7,
};
enum class native_mobile_birth_library : uint8_t
{
	actroom = 1,
	actworn = 2,
	hummer = 3,
	sayresponse = 4,
	transporter = 5,
};
struct native_mobile_birth_library_recipe
{
	native_mobile_birth_library library = {};
	// Zero-based index of the actual created parsed row in the FINAL literal image.
	uint32_t extra_description_index = 0;
	bool periodic_requested = false;
	int32_t delay = 0;
};
struct native_mobile_birth_item_recipe
{
	uint64_t object_uid = 0;
	native_mobile_birth_binding_form binding_form = {};
	native_mobile_birth_procedure procedure = {};
	// Original successful preparation order, not final linked-list traversal order.
	std::vector<native_mobile_birth_library_recipe> libraries;
	bool general_periodic = false;
	int32_t general_delay = 0;
	bool random_exit_requested = false;
	int16_t trap_eff = 0, trap_dam = 0, trap_charge = 0, trap_level = 0;
};

constexpr uint16_t NATIVE_MOBILE_BIRTH_RECIPE_VERSION = 1;

// Exactly one recipe for every item row, in the same complete forest order.
// Pure value validation: no parsing, probes, RNG, bindings or publication.
bool native_mobile_birth_recipe_valid(std::span<const player_item_snapshot>,
				      std::span<const native_mobile_birth_item_recipe>) noexcept;

// Versioned canonical little-endian recipe. Strings and parsed parameters stay
// in the existing literal image; the enclosing command owns its combined limit.
// All outputs remain unchanged on every refusal, including allocation failure.
economic_accounting_error
native_mobile_birth_recipe_encode(std::span<const player_item_snapshot>,
				  std::span<const native_mobile_birth_item_recipe>,
				  std::vector<uint8_t> *) noexcept;
economic_accounting_error
native_mobile_birth_recipe_decode(std::span<const uint8_t>, std::span<const player_item_snapshot>,
				  std::vector<native_mobile_birth_item_recipe> *) noexcept;

// Allocation-free profiles of the existing canonical NBR1 codec. Counts and
// wire length are portable; fresh allocation requests require the policy bits.
// Nested library-vector objects are already included in sizeof(recipe rows).
// These are explicit object/request bytes, not allocator metadata or frame padding.
struct native_mobile_birth_recipe_allocation_profile
{
	size_t item_count = 0, library_count = 0, wire_bytes = 0;
	size_t decoded_row_storage_bytes = 0, decoded_library_storage_bytes = 0;
	size_t decoded_payload_bytes = 0, encoded_capacity_bytes = 0;
	size_t validation_inline_storage_bytes = 0, preflight_inline_storage_bytes = 0;
	size_t encoder_inline_storage_bytes = 0, decoder_inline_storage_bytes = 0;
	bool fresh_decode_storage_policy_supported = false;
	bool fresh_encode_storage_policy_supported = false;
};

// Reserve this explicit profile/preflight footprint BEFORE invoking a profile.
// Input spans/forests/wire, the profile output and all prior codec outputs belong
// to the caller's outer-live charge. Keep admitted payload/object requests live
// through the original codec call and output transfer; no lifetime is granted here.
size_t native_mobile_birth_recipe_profile_inline_storage_bytes() noexcept;
economic_accounting_error native_mobile_birth_recipe_encode_profile(
	std::span<const player_item_snapshot>, std::span<const native_mobile_birth_item_recipe>,
	native_mobile_birth_recipe_allocation_profile *) noexcept;
economic_accounting_error native_mobile_birth_recipe_decode_profile(
	std::span<const uint8_t>, std::span<const player_item_snapshot>,
	native_mobile_birth_recipe_allocation_profile *) noexcept;

// SOURCE companions for the unchanged NBR1 codec/profile algorithms. The old
// profile already owns spans, DTOs, seen arrays, four named descriptor views and
// exact fresh vector requests. Retain ONLY the returned supplement alongside
// that old admission; full SOURCE is the same scalar/algorithm supplement.
// Initial-inline describes the existing first profile/preflight object phase,
// for transient admission before constructing it, not a second retained charge.
bool native_mobile_birth_recipe_encode_source_frame_bytes(size_t *) noexcept;
bool native_mobile_birth_recipe_decode_source_frame_bytes(size_t *) noexcept;
bool native_mobile_birth_recipe_encode_source_supplement_frame_bytes(size_t *) noexcept;
bool native_mobile_birth_recipe_decode_source_supplement_frame_bytes(size_t *) noexcept;
bool native_mobile_birth_recipe_encode_initial_inline_bytes(size_t *) noexcept;
bool native_mobile_birth_recipe_decode_initial_inline_bytes(size_t *) noexcept;
// Actual retaining caller's recipe-vector default/move/cleanup path, including
// nested library-vector destruction; no input/output capacity is included.
bool native_mobile_birth_recipe_value_lifecycle_source_frame_bytes(size_t *) noexcept;
bool native_mobile_birth_recipe_valid_source_frame_bytes(size_t *) noexcept;
constexpr size_t native_mobile_birth_recipe_source_query_frame_bytes() noexcept
{
	return sizeof(size_t *) + 2 * sizeof(bool);
}

#endif
