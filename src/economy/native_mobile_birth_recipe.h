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

#endif
