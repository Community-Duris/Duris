#include "economy/native_mobile_birth_cash_role_recipe.h"

#include <algorithm>
#include <new>
#include <utility>

bool native_mobile_birth_cash_role_recipe_valid(
	const native_mobile_birth_cash_role_recipe &value) noexcept
{
	if (value.original.wire_version !=
		    NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_ALCHEMIST_VERSION ||
	    !native_mobile_birth_constructor_recipe_valid(value.original) ||
	    value.original.reset_room_vnum <= 0)
		return false;
	return (value.role == native_mobile_birth_cash_role::ordinary_wallet &&
		value.configured_shop_matches == 0 && value.original.reset_shop_index == -1) ||
	       (value.role == native_mobile_birth_cash_role::shared_shopkeeper &&
		value.configured_shop_matches == 1 && value.original.reset_shop_index >= 0);
}

bool native_mobile_birth_cash_role_recipe_encode(
	const native_mobile_birth_cash_role_recipe &value,
	native_mobile_birth_cash_role_recipe_bytes *output) noexcept
{
	if (!output || !native_mobile_birth_cash_role_recipe_valid(value))
		return false;
	try
	{
		std::vector<uint8_t> original;
		if (!native_mobile_birth_constructor_recipe_encode_blob(value.original,
									&original) ||
		    original.size() != NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_ALCHEMIST_BYTES)
			return false;
		native_mobile_birth_cash_role_recipe_bytes candidate{};
		candidate[0] = 'N';
		candidate[1] = 'B';
		candidate[2] = 'C';
		candidate[3] = '4';
		candidate[4] = NATIVE_MOBILE_BIRTH_CASH_ROLE_RECIPE_VERSION;
		std::copy(original.begin(), original.end(), candidate.begin() + 8);
		constexpr size_t role_offset =
			8 + NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_ALCHEMIST_BYTES;
		candidate[role_offset] = static_cast<uint8_t>(value.role);
		for (size_t i = 0; i < 4; ++i)
			candidate[role_offset + 1 + i] =
				static_cast<uint8_t>(value.configured_shop_matches >> (8 * i));
		*output = candidate;
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	catch (...)
	{
		return false;
	}
}

bool native_mobile_birth_cash_role_recipe_decode(
	std::span<const uint8_t> bytes, native_mobile_birth_cash_role_recipe *output) noexcept
{
	if (!output || bytes.size() != NATIVE_MOBILE_BIRTH_CASH_ROLE_RECIPE_BYTES ||
	    bytes[0] != 'N' || bytes[1] != 'B' || bytes[2] != 'C' || bytes[3] != '4' ||
	    bytes[4] != NATIVE_MOBILE_BIRTH_CASH_ROLE_RECIPE_VERSION || bytes[5] || bytes[6] ||
	    bytes[7])
		return false;
	native_mobile_birth_cash_role_recipe candidate;
	if (!native_mobile_birth_constructor_recipe_decode(
		    bytes.subspan(8, NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_ALCHEMIST_BYTES),
		    &candidate.original))
		return false;
	constexpr size_t role_offset = 8 + NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_ALCHEMIST_BYTES;
	candidate.role = static_cast<native_mobile_birth_cash_role>(bytes[role_offset]);
	candidate.configured_shop_matches = 0;
	for (size_t i = 0; i < 4; ++i)
		candidate.configured_shop_matches |= uint32_t(bytes[role_offset + 1 + i])
						     << (8 * i);
	if (!native_mobile_birth_cash_role_recipe_valid(candidate))
		return false;
	*output = candidate;
	return true;
}
