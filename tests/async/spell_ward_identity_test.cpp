#include "combat/spell_wards.h"
#include "combat/spell_ward_identity_private.h"
#include "core/utils.h"
#include "magic/spells.h"
#include <cassert>
#include <cstring>
#include <initializer_list>

int main()
{
	assert(!spell_ward_is_managed(nullptr));
	const int supported[] = { SPELL_MINOR_GLOBE, SPELL_SPIRIT_WARD, SPELL_GREATER_SPIRIT_WARD,
				  SPELL_GLOBE };
	for (int spell : supported)
	{
		affected_type value{};
		value.type = spell;
		assert(!spell_ward_is_managed(&value));
		SET_BIT(value.flags, AFFTYPE_SPELL_WARD);
		unsigned char before[sizeof(value)];
		std::memcpy(before, &value, sizeof(value));
		assert(spell_ward_is_managed(&value));
		assert(!std::memcmp(&value, before, sizeof(value)));
	}
	for (int spell : { -1, 0, 1, 32767 })
	{
		affected_type value{};
		value.type = spell;
		SET_BIT(value.flags, AFFTYPE_SPELL_WARD);
		assert(!spell_ward_is_managed(&value));
	}
	assert(spell_ward_identity::WARD_KIND_COUNT == 4);
	for (int i = 0; i < spell_ward_identity::WARD_KIND_COUNT; ++i)
		assert(spell_ward_identity::ward_spells[i] == supported[i]);
}
