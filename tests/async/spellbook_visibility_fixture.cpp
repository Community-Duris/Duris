#include "core/prototypes.h"
#include "core/utils.h"
#include "core/utility.h"
#include <cassert>
#include <cstdio>
#include <initializer_list>

// Visibility is the only mocked policy: reproduce mortal secret, invisible,
// and blind rejection. Lookup, description validation and bitmap reads are real.
bool ac_can_see_obj(P_char ch, P_obj obj, int)
{
	return !IS_SET(obj->extra_flags, ITEM_SECRET | ITEM_INVISIBLE) &&
	       !IS_AFFECTED(ch, AFF_BLIND);
}
P_obj find_gh_library_book_obj(P_char)
{
	return nullptr;
}
int real_object(int number)
{
	return number;
}
bool isname(const char *, const char *)
{
	return false;
}

// PRODUCTION_FUNCTIONS

int main()
{
	char name[] = "Reader";
	char keyword[] = { 3, 1, 3, 0 };
	char bitmap[32]{};
	constexpr int spell = 8;
	bitmap[spell / 8] = 1 << (spell % 8);
	extra_descr_data description{};
	description.keyword = keyword;
	description.description = bitmap;
	const int personal = SBOOK_MODE_IN_INV | SBOOK_MODE_AT_HAND | SBOOK_MODE_ON_BELT;
	int cases = 0;
	for (int spec = 0; spec <= MAX_SPEC; ++spec)
		for (int state = 0; state < 4; ++state)
			for (int slot :
			     { -1, WIELD, HOLD, WIELD2, WIELD3, WIELD4, WEAR_ATTACH_BELT_1,
			       WEAR_ATTACH_BELT_2, WEAR_ATTACH_BELT_3 })
			{
				char_data owner{};
				owner.player.name = name;
				owner.player.level = 8;
				owner.player.m_class = CLASS_NECROMANCER;
				owner.player.spec = spec;
				obj_data book{};
				book.type = ITEM_SPELLBOOK;
				book.R_num = 19408;
				book.value[0] = TONGUE_MAGIC;
				book.value[1] = CLASS_NECROMANCER;
				book.ex_description = &description;
				if (state == 1)
					SET_BIT(book.extra_flags, ITEM_SECRET);
				if (state == 2)
					SET_BIT(book.extra_flags, ITEM_INVISIBLE);
				if (state == 3)
					SET_BIT(owner.specials.affected_by, AFF_BLIND);
				int mode;
				if (slot == -1)
				{
					owner.carrying = &book;
					book.loc_p = LOC_CARRIED;
					book.loc.carrying = &owner;
					mode = SBOOK_MODE_IN_INV;
				}
				else
				{
					owner.equipment[slot] = &book;
					book.loc_p = LOC_WORN;
					book.loc.wearing = &owner;
					mode = slot >= WEAR_ATTACH_BELT_1 &&
							       slot <= WEAR_ATTACH_BELT_3 ?
						       SBOOK_MODE_ON_BELT :
						       SBOOK_MODE_AT_HAND;
				}
				assert(CAN_SEE_OBJ(&owner, &book) == (state == 0));
				assert(FindSpellBookWithSpell(&owner, spell, mode) == &book);
				// do_spells uses personal slots; do_memorize also allows guild libraries.
				assert(SpellInSpellBook(&owner, spell, personal) == &book);
				assert(SpellInSpellBook(&owner, spell,
							personal | SBOOK_MODE_ON_GROUND) == &book);
				assert(!SpellInSpellBook(&owner, spell + 1, personal));
				assert(!SpellInSpellBook(&owner, spell,
							 personal | SBOOK_MODE_NO_BOOK));
				assert(!SpellInSpellBook(&owner, spell, personal & ~mode));
				assert(SpellInSpellBook(&owner, spell,
							personal | SBOOK_MODE_NO_SCROLL) == &book);
				book.value[0] = TONGUE_USING;
				assert(!SpellInSpellBook(&owner, spell, personal));
				book.value[0] = TONGUE_MAGIC;
				book.value[1] = 0;
				assert(!SpellInSpellBook(&owner, spell, personal));
				book.value[1] = CLASS_NECROMANCER;
				book.ex_description = nullptr;
				assert(!SpellInSpellBook(&owner, spell, personal));
				book.ex_description = &description;
				// Scroll visibility remains a separate rule.
				if (slot == -1)
				{
					book.type = ITEM_SCROLL;
					book.value[1] = spell;
					assert(bool(SpellInSpellBook(&owner, spell, personal)) ==
					       (state == 0));
					assert(!SpellInSpellBook(&owner, spell,
								 personal | SBOOK_MODE_NO_SCROLL));
				}
				owner.carrying = nullptr;
				if (slot != -1)
					owner.equipment[slot] = nullptr;
				assert(!SpellInSpellBook(&owner, spell, personal));
				++cases;
			}
	std::printf("%d mortal spellbook location/visibility/specialization cases passed\n", cases);
}
