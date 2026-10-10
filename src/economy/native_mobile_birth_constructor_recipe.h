#ifndef NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_H
#define NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_H

#include "core/random.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

using quest_mobile_native_constructor_digest = std::array<uint8_t, 32>;
enum class quest_mobile_native_constructor_binding : uint8_t
{
	none,
	thief,
	teacher,
	shop_keeper,
	quester,
	arbitrary = 5
};
// Retained original external alchemist decision only; values grant no authority.
enum class original_alchemist_choice : uint8_t
{
	not_attempted = 0,
	missed = 1,
	selected = 2
};
// Original constructor inputs only, not an NPC-state ledger or publication permit.
// The original birth owner must supply the actual compiled build witness.
struct quest_mobile_native_constructor_recipe
{
	int32_t mobile_vnum = 0, constructor_birthplace_vnum = 0;
	bool apply_mob_gold = false;
	native_mobile_birth_random_recipe random;
	std::array<int64_t, 2> clock_values{};
	uint64_t template_bytes = 0;
	quest_mobile_native_constructor_digest build_digest{}, template_digest{},
		effective_inputs_digest{}, cached_strings_digest{};
	std::array<quest_mobile_native_constructor_digest, 4> string_digests{};
	quest_mobile_native_constructor_binding binding_before{}, binding_after{}, quest_binding{};
	// NBC1 remains the default and cannot silently omit successor inputs.
	uint16_t wire_version = 1;
	quest_mobile_native_constructor_digest procedure_before{}, procedure_after{}, reset_tail{};
	int32_t reset_room_vnum = 0, reset_shop_index = -1;
	// Selected with UID0 retains a returned failed grant, not permission to grant.
	original_alchemist_choice alchemist_choice = original_alchemist_choice::not_attempted;
	uint64_t alchemist_grant_uid = 0;
};

constexpr uint16_t NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_VERSION = 1;
constexpr size_t NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_BYTES = 372;
constexpr uint16_t NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_SUCCESSOR_VERSION = 2;
constexpr size_t NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_SUCCESSOR_BYTES = 476;
constexpr uint16_t NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_ALCHEMIST_VERSION = 3;
constexpr size_t NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_ALCHEMIST_BYTES = 485;
constexpr size_t NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_MAX_BYTES =
	NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_ALCHEMIST_BYTES;
using native_mobile_birth_constructor_recipe_bytes =
	std::array<uint8_t, NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_BYTES>;

// Pure fixed-value shape only. Matching the actual compiled build, template,
// bindings, conversion inputs and strings remains the original factory's job.
// No RNG advance, clocks, callbacks, database, runtime or publication authority.
bool native_mobile_birth_constructor_recipe_valid(
	const quest_mobile_native_constructor_recipe &) noexcept;

// NBC1: canonical little-endian fixed-width values, never raw struct/padding.
// Exact size, zero reserved header, canonical bool and stable binding tags.
// Allocation-free; every output remains unchanged on any refusal.
bool native_mobile_birth_constructor_recipe_encode(
	const quest_mobile_native_constructor_recipe &,
	native_mobile_birth_constructor_recipe_bytes *) noexcept;
// Canonical bounded NBC1/NBC2/NBC3 transport. NBC2 appends actual procedure-before,
// procedure-after and reset-tail witnesses plus original room/shop selection.
// NBC3 additionally retains the original alchemist decision and optional grant UID.
// Values are shape only; the original owners still prove actual inputs/outcomes.
// Allocation failure preserves output. NBC1/NBC2 refuse nondefault NBC3 data.
bool native_mobile_birth_constructor_recipe_encode_blob(
	const quest_mobile_native_constructor_recipe &, std::vector<uint8_t> *) noexcept;

// Exact version/magic/length correlation; no inferred successor values for NBC1.
bool native_mobile_birth_constructor_recipe_decode(
	std::span<const uint8_t>, quest_mobile_native_constructor_recipe *) noexcept;

// Prospective companions for the original complete NBC1/NBC2/NBC3 codecs.
// Caller owns/admit inputs, old outputs and inline output objects in outer_live,
// and retains the callback's absolute simultaneous peak through return/transfer.
// Encoder admission includes actual original writer/range-vector phases; decoder
// includes its actual allocation-free named objects. Fresh vector request policy
// requires GCC13 libstdc++ C++11 ABI; unsupported policy refuses. Strong output.
// A callback refusal returns false (ENOBUFS); semantic false is not authority.
bool native_mobile_birth_constructor_recipe_encode_blob_bounded(
	const quest_mobile_native_constructor_recipe &, std::vector<uint8_t> *,
	bool (*)(size_t, void *) noexcept, void *, size_t outer_live) noexcept;
bool native_mobile_birth_constructor_recipe_decode_bounded(const std::span<const uint8_t> &,
							   quest_mobile_native_constructor_recipe *,
							   bool (*)(size_t, void *) noexcept,
							   void *, size_t outer_live) noexcept;

// Structured passive result for the full original allocation-free decoder.
// Checked-add/null callback/admission refusal is capacity even before callback;
// unsupported request policy is unresolved; malformed NBC1/2/3 is corrupt_evidence.
// Strong output, genuine complete candidate/frame admission, no errno inference.
enum class economic_accounting_error : uint8_t;
economic_accounting_error native_mobile_birth_constructor_recipe_decode_status_bounded(
	const std::span<const uint8_t> &, quest_mobile_native_constructor_recipe *,
	bool (*)(size_t, void *) noexcept, void *, size_t outer_live) noexcept;

#endif
