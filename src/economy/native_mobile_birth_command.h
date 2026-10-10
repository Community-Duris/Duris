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

// Prospective v3 companions for aggregate caller-owned scratch admission.
// Inputs, prior outputs and caller inline output objects belong to outer_live;
// reserve observes absolute simultaneous object/request bytes and must retain
// the admitted maximum through output transfer. Supported requests require
// GCC13 libstdc++ C++11 ABI; unsupported policy refuses without mutation.
// These structural codecs grant no source, factory, execution or ACK authority.
economic_accounting_error native_mobile_birth_command_build_bounded(
	const economic_operation_metadata &, const quest_mobile_native_image &,
	const std::span<const native_mobile_birth_item_recipe> &,
	const quest_mobile_native_constructor_recipe &, critical_source_site,
	uint64_t accepted_at_usec, critical_command *, bool (*)(size_t, void *) noexcept, void *,
	size_t outer_live) noexcept;
// Requires genuine v3. Optional scalars are exact transferred heap requests,
// excluding caller inline image/recipe-vector objects; all outputs are strong.
economic_accounting_error native_mobile_birth_command_decode_bounded(
	const critical_command &, quest_mobile_native_image *,
	std::vector<native_mobile_birth_item_recipe> *, quest_mobile_native_constructor_recipe *,
	bool (*)(size_t, void *) noexcept, void *, size_t outer_live,
	size_t *retained_image_heap_bytes = nullptr,
	size_t *retained_recipe_heap_bytes = nullptr) noexcept;

// Genuine historical v2/v3 recipe decode. V1 has no retained recipes and refuses.
// Preserves full original metadata, keys/revisions, intent/image/recipe binding
// and byte-for-byte canonical reconstruction; outputs are strong on refusal.
// Uses the same bounded storage policy and caller-owned outer contract above.
economic_accounting_error native_mobile_birth_command_decode_bounded(
	const critical_command &, quest_mobile_native_image *,
	std::vector<native_mobile_birth_item_recipe> *, bool (*)(size_t, void *) noexcept, void *,
	size_t outer_live, size_t *retained_image_heap_bytes = nullptr,
	size_t *retained_recipe_heap_bytes = nullptr) noexcept;

// Original IMAGE-only historical command path, including binding and canonical
// command reconstruction. Requires payload v1; no recipe/constructor invented.
// Strong output and optional transferred heap. Outer owns inputs/prior outputs.
// Pure accessors below cover this TU's named scalar/inline storage ONLY; complete
// lower image/intent/STL source and native qualification remain separate gates.
economic_accounting_error native_mobile_birth_command_decode_v1_bounded(
	const critical_command &, quest_mobile_native_image *, bool (*)(size_t, void *) noexcept,
	void *, size_t outer_live, size_t *retained_image_heap_bytes = nullptr) noexcept;
bool native_mobile_birth_command_v1_decode_initial_inline_bytes(size_t *) noexcept;
bool native_mobile_birth_command_v1_decode_own_source_frame_bytes(size_t *) noexcept;
constexpr size_t native_mobile_birth_command_v1_decode_query_frame_bytes() noexcept
{
	return sizeof(size_t *) + sizeof(bool);
}

// Original two-argument image-only decoder across genuine v1/v2/v3. Discarded
// recipes and constructor remain real local values; fixed proof/freeze paths
// preserve original digests and canonical command equality. No v4 invented.
economic_accounting_error native_mobile_birth_command_decode_bounded(
	const critical_command &, quest_mobile_native_image *, bool (*)(size_t, void *) noexcept,
	void *, size_t outer_live, size_t *retained_image_heap_bytes = nullptr) noexcept;
economic_accounting_error native_mobile_birth_command_historical_decode_fixed_bounded(
	const critical_command &, quest_mobile_native_image *,
	std::vector<native_mobile_birth_item_recipe> *, bool (*)(size_t, void *) noexcept, void *,
	size_t outer_live, size_t *retained_image_heap_bytes = nullptr,
	size_t *retained_recipe_heap_bytes = nullptr) noexcept;
economic_accounting_error native_mobile_birth_command_decode_constructor_fixed_bounded(
	const critical_command &, quest_mobile_native_image *,
	std::vector<native_mobile_birth_item_recipe> *, quest_mobile_native_constructor_recipe *,
	bool (*)(size_t, void *) noexcept, void *, size_t outer_live,
	size_t *retained_image_heap_bytes = nullptr,
	size_t *retained_recipe_heap_bytes = nullptr) noexcept;
economic_accounting_error native_mobile_birth_command_build_fixed_bounded(
	const economic_operation_metadata &, const quest_mobile_native_image &,
	const std::span<const native_mobile_birth_item_recipe> &,
	const quest_mobile_native_constructor_recipe &, critical_source_site,
	uint64_t accepted_at_usec, critical_command *, bool (*)(size_t, void *) noexcept, void *,
	size_t outer_live) noexcept;
// LOCAL entry/formal profiles only, not full recipe/constructor/STL/lower SOURCE.
bool native_mobile_birth_command_historical_decode_own_source_frame_bytes(size_t *) noexcept;
bool native_mobile_birth_command_historical_decode_initial_inline_bytes(size_t *) noexcept;
bool native_mobile_birth_command_general_decode_own_source_frame_bytes(size_t *) noexcept;
bool native_mobile_birth_command_general_decode_initial_inline_bytes(size_t *) noexcept;
bool native_mobile_birth_command_constructor_build_own_source_frame_bytes(size_t *) noexcept;
bool native_mobile_birth_command_constructor_build_initial_inline_bytes(size_t *) noexcept;
constexpr size_t native_mobile_birth_command_constructor_build_query_frame_bytes() noexcept
{
	return sizeof(size_t *) + sizeof(bool);
}
constexpr size_t native_mobile_birth_command_historical_decode_query_frame_bytes() noexcept
{
	return sizeof(size_t *) + sizeof(bool);
}
constexpr size_t native_mobile_birth_command_general_decode_query_frame_bytes() noexcept
{
	return sizeof(size_t *) + sizeof(bool);
}

// Genuine complete prospective SOURCE contracts; inputs/prior outputs remain
// outer. Full SOURCE+initial is transient preentry. Child owns actual local
// SOURCE and retains only named lower supplements/lifetimes, once.
bool native_mobile_birth_command_v1_decode_source_frame_bytes(size_t *) noexcept;
bool native_mobile_birth_command_constructor_build_source_frame_bytes(size_t *) noexcept;
bool native_mobile_birth_command_constructor_decode_source_frame_bytes(size_t *) noexcept;
bool native_mobile_birth_command_historical_decode_source_frame_bytes(size_t *) noexcept;
bool native_mobile_birth_command_general_decode_source_frame_bytes(size_t *) noexcept;
constexpr size_t native_mobile_birth_command_source_query_frame_bytes() noexcept
{
	// Actual full-profile/public-wrapper/add/policy carriers, plus the genuine
	// reached pure lower query closures. Sequential scopes conservatively sum.
	return 2 * sizeof(void *) + 3 * sizeof(size_t) + sizeof(uint8_t) + 4 * sizeof(bool) +
	       sizeof(void *) + sizeof(size_t) + sizeof(bool) +
	       economic_operation_metadata_validate_source_query_frame_bytes() +
	       quest_mobile_native_reference_valid_source_query_frame_bytes() +
	       2 * native_mobile_birth_constructor_recipe_query_frame_bytes() +
	       2 * quest_mobile_native_image_source_query_frame_bytes() +
	       quest_mobile_native_image_lifetime_source_query_frame_bytes() +
	       3 * native_mobile_birth_recipe_source_query_frame_bytes() +
	       economic_intent_freeze_fixed_source_query_frame_bytes() +
	       economic_intent_decode_source_query_frame_bytes() +
	       economic_intent_verify_binding_fixed_source_query_frame_bytes() +
	       critical_command_startup_codec_source_query_frame_bytes();
}

#endif
