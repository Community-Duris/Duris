#ifndef NATIVE_MOBILE_BIRTH_COMMAND_H
#define NATIVE_MOBILE_BIRTH_COMMAND_H

#include "economy/economic_accounting_intent.h"
#include "world/quest_mobile_native.h"
#include "economy/native_mobile_birth_recipe.h"
#include "economy/native_mobile_birth_constructor_recipe.h"

// Append-only writer number after the existing SHOP writer14. Registration,
// original generation admission and the atomic store participant are separate.
constexpr uint32_t ECONOMIC_WRITER_NATIVE_MOBILE_BIRTH = 15;
// Historical image-only commands remain readable and retain their exact bytes.
constexpr uint16_t NATIVE_MOBILE_BIRTH_PAYLOAD_VERSION = 1;
constexpr uint16_t NATIVE_MOBILE_BIRTH_RECIPE_PAYLOAD_VERSION = 2;
constexpr uint16_t NATIVE_MOBILE_BIRTH_CONSTRUCTOR_PAYLOAD_VERSION = 3;
constexpr bool native_mobile_birth_payload_version_supported(uint16_t version) noexcept
{
	return version == NATIVE_MOBILE_BIRTH_PAYLOAD_VERSION ||
	       version == NATIVE_MOBILE_BIRTH_RECIPE_PAYLOAD_VERSION ||
	       version == NATIVE_MOBILE_BIRTH_CONSTRUCTOR_PAYLOAD_VERSION;
}

// Pure construction from the original generation owner's final staged image.
// Retains actual post-conversion cash and complete initial equipment/carrying,
// including an actually empty forest and an explicitly known zero wallet.
// Source values, a reserved UID or this command do not authenticate a birth.
// No ID issuance, source claim, SQL, live publication, recovery or ACK occurs.
// All outputs are unchanged on refusal, including allocation failure.
economic_accounting_error
native_mobile_birth_command_build(const economic_operation_metadata &,
				  const quest_mobile_native_image &,
				  critical_source_site original_source_site,
				  uint64_t accepted_at_usec, critical_command *) noexcept;

// Canonical exact command/intent/image correlation only. The atomic owner must
// separately prove the admitted original generation, absent native/cash/item
// lifetimes and current epoch, then persist the native image with its ledger.
economic_accounting_error native_mobile_birth_command_decode(const critical_command &,
							     quest_mobile_native_image *) noexcept;

// The actual original factory supplies one complete retained recipe per stock
// row. This v2 wrapper preserves the original native-image codec and binds the
// exact constructor choices into the immutable command/accounting intent.
economic_accounting_error native_mobile_birth_command_build(
	const economic_operation_metadata &, const quest_mobile_native_image &,
	std::span<const native_mobile_birth_item_recipe>, critical_source_site,
	uint64_t accepted_at_usec, critical_command *) noexcept;

// Historical v1 returns no recipes, never an inferred constructor decision.
// Cold publication must separately require v2. Both outputs remain unchanged
// on refusal; existing image-only decode still validates the complete v2 body.
economic_accounting_error
native_mobile_birth_command_decode(const critical_command &, quest_mobile_native_image *,
				   std::vector<native_mobile_birth_item_recipe> *) noexcept;

// V3 binds an explicitly versioned NBC1/NBC2/NBC3 constructor capsule alongside the unchanged
// native image and full stock recipes. NBC3 binds any nonzero alchemist grant UID
// to exactly one original poison-vial image row, without imposing its placement.
// The original owners separately prove the actual
// running build, template, bindings and replay inputs before restoring an actor.
economic_accounting_error native_mobile_birth_command_build(
	const economic_operation_metadata &, const quest_mobile_native_image &,
	std::span<const native_mobile_birth_item_recipe>,
	const quest_mobile_native_constructor_recipe &, critical_source_site,
	uint64_t accepted_at_usec, critical_command *) noexcept;

// This overload requires v3; historical commands cannot supply inferred NPC
// constructor choices. All outputs remain unchanged on refusal. The existing
// decode overloads also validate the complete v3 capsule and intent binding.
economic_accounting_error
native_mobile_birth_command_decode(const critical_command &, quest_mobile_native_image *,
				   std::vector<native_mobile_birth_item_recipe> *,
				   quest_mobile_native_constructor_recipe *) noexcept;

#endif
