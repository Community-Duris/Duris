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

#endif
