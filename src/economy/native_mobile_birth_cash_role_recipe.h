#ifndef NATIVE_MOBILE_BIRTH_CASH_ROLE_RECIPE_H
#define NATIVE_MOBILE_BIRTH_CASH_ROLE_RECIPE_H

#include "economy/native_mobile_birth_constructor_recipe.h"

// Prospective reset cash classification, separate from historical NBC1/2/3.
// These values never authenticate a factory, epoch, source claim or mapping.
enum class native_mobile_birth_cash_role : uint8_t
{
	ordinary_wallet = 1,
	shared_shopkeeper = 2
};
struct native_mobile_birth_cash_role_recipe
{
	quest_mobile_native_constructor_recipe original;
	native_mobile_birth_cash_role role{};
	uint32_t configured_shop_matches = UINT32_MAX;
};
constexpr uint16_t NATIVE_MOBILE_BIRTH_CASH_ROLE_RECIPE_VERSION = 4;
constexpr size_t NATIVE_MOBILE_BIRTH_CASH_ROLE_RECIPE_BYTES =
	8 + NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_ALCHEMIST_BYTES + 1 + 4;
using native_mobile_birth_cash_role_recipe_bytes =
	std::array<uint8_t, NATIVE_MOBILE_BIRTH_CASH_ROLE_RECIPE_BYTES>;

// Exact bounded NBC4 wrapper retains the original NBC3 capsule byte-for-byte.
// Old command/result readers refuse NBC4; no historical wallet policy changes.
// Counts above one are unresolved and cannot encode as an ordinary wallet.
bool native_mobile_birth_cash_role_recipe_valid(
	const native_mobile_birth_cash_role_recipe &) noexcept;
bool native_mobile_birth_cash_role_recipe_encode(
	const native_mobile_birth_cash_role_recipe &,
	native_mobile_birth_cash_role_recipe_bytes *) noexcept;
bool native_mobile_birth_cash_role_recipe_decode(std::span<const uint8_t>,
						 native_mobile_birth_cash_role_recipe *) noexcept;

// Observe the actual original configured keeper-RNUM/destination-room selector
// on the game thread; distinguish real absence from ambiguity before freezing.
// Original procedure/tail/constructor inputs still need genuine owner proof.
// No binding, callbacks, RNG, clocks, UID allocation, write or ACK. Strong output.
bool native_mobile_birth_cash_role_recipe_capture(
	const quest_mobile_native_constructor_recipe &original,
	native_mobile_birth_cash_role_recipe *) noexcept;

// Full original NBC4 wrapper. Inputs and previous output stay outer; private
// candidate/vector and source are retained by the callee. Strong output.
enum class economic_accounting_error : uint8_t;
economic_accounting_error native_mobile_birth_cash_role_recipe_encode_fixed_bounded(
	const native_mobile_birth_cash_role_recipe &, native_mobile_birth_cash_role_recipe_bytes *,
	bool (*)(size_t, void *) noexcept, void *, size_t) noexcept;
economic_accounting_error native_mobile_birth_cash_role_recipe_decode_fixed_bounded(
	const std::span<const uint8_t> &, native_mobile_birth_cash_role_recipe *,
	bool (*)(size_t, void *) noexcept, void *, size_t) noexcept;
bool native_mobile_birth_cash_role_recipe_source_frame_bytes(size_t *) noexcept;
bool native_mobile_birth_cash_role_recipe_initial_inline_bytes(size_t *) noexcept;
constexpr size_t native_mobile_birth_cash_role_recipe_query_frame_bytes() noexcept
{
	// source getter (P+2N+B), role_fixed_query (2P+N+2B),
	// constructor valid-source getter/profile (P+2B), and checked add (P+N+B).
	// Returned size_t stays caller-owned; no encoded/decoded DTO belongs here.
	return 5 * sizeof(size_t *) + 4 * sizeof(size_t) + 6 * sizeof(bool);
}

#endif
