#include "combat/spell_wards.h"
#include "combat/spell_ward_identity_private.h"
#include "core/utils.h"
#include "magic/spells.h"

namespace spell_ward_identity
{
const int ward_spells[WARD_KIND_COUNT] = { SPELL_MINOR_GLOBE, SPELL_SPIRIT_WARD,
					   SPELL_GREATER_SPIRIT_WARD, SPELL_GLOBE };

bool is_ward_spell(int spell)
{
	for (int i = 0; i < WARD_KIND_COUNT; ++i)
		if (ward_spells[i] == spell)
			return true;
	return false;
}
}

using spell_ward_identity::is_ward_spell;

bool spell_ward_is_managed(const struct affected_type *af)
{
	return af && IS_SET(af->flags, AFFTYPE_SPELL_WARD) && is_ward_spell(af->type);
}
