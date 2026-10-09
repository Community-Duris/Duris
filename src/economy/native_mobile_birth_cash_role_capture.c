#include "economy/native_mobile_birth_cash_role_recipe.h"

#include "core/prototypes.h"
#include "core/structs.h"
#include "economy/shop.h"

extern P_index mob_index;
extern P_room world;
extern int top_of_mobt;
extern int top_of_world;
extern struct shop_data *shop_index;
extern int number_of_shops;

bool native_mobile_birth_cash_role_recipe_capture(
	const quest_mobile_native_constructor_recipe &original,
	native_mobile_birth_cash_role_recipe *output) noexcept
{
	if (!output ||
	    original.wire_version != NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_ALCHEMIST_VERSION ||
	    !native_mobile_birth_constructor_recipe_valid(original) || original.mobile_vnum <= 0 ||
	    original.reset_room_vnum <= 0 || !nevent_is_game_thread() || !mob_index || !world ||
	    number_of_shops < 0 || (number_of_shops > 0 && !shop_index))
		return false;
	const int mobile = real_mobile(original.mobile_vnum);
	const int room = real_room(original.reset_room_vnum);
	if (mobile < 0 || mobile > top_of_mobt || room < 0 || room > top_of_world ||
	    mob_index[mobile].virtual_number != original.mobile_vnum ||
	    world[room].number != original.reset_room_vnum)
		return false;
	// Preserve the exact original selector inputs and authentic candidate scan.
	// Neither a procedure tag nor a matching VNUM alone certifies a keeper.
	uint32_t matches = 0;
	int selected = -1;
	for (int shop = 0; shop < number_of_shops; ++shop)
		if (shop_index[shop].keeper == mobile &&
		    shop_index[shop].in_room == original.reset_room_vnum)
		{
			++matches;
			selected = shop;
		}
	if (matches > 1 || (matches == 0 ? -1 : selected) != original.reset_shop_index)
		return false;
	shop_native_mobile_birth_reset_selection actual;
	if (!shop_native_mobile_birth_reset_tail_selection(original.mobile_vnum,
							   original.reset_room_vnum,
							   original.reset_shop_index, &actual) ||
	    actual.index != original.reset_shop_index)
		return false;
	native_mobile_birth_cash_role_recipe candidate;
	candidate.original = original;
	candidate.configured_shop_matches = matches;
	candidate.role = matches ? native_mobile_birth_cash_role::shared_shopkeeper :
				   native_mobile_birth_cash_role::ordinary_wallet;
	if (!native_mobile_birth_cash_role_recipe_valid(candidate))
		return false;
	*output = candidate;
	return true;
}
