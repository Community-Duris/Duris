#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "core/utils.h"
#include "core/defines.h"
#include "magic/spells.h"
#include <strings.h>

extern Skill skills[];

void spell_chaos_shield(int /*level*/, P_char ch, char * /*arg*/, [[maybe_unused]] int type,
			P_char /*victim*/, P_obj /*obj*/)
{
	if (affected_by_spell(ch, SPELL_CHAOS_SHIELD))
	{
		send_to_char("You're already protected by &+rchaotic forces&n.\n", ch);
		return;
	}

	struct affected_type af;
	bzero(&af, sizeof(af));
	af.type = SPELL_CHAOS_SHIELD;
	af.duration = GET_LEVEL(ch) / 3;
	affect_to_char(ch, &af);

	act("&+LA wild magical aura swirls up and around you.&n", TRUE, ch, 0, 0, TO_CHAR);
	act("&+LA wild magical aura swirlds up and around $n.&n", TRUE, ch, 0, 0, TO_NOTVICT);
}

#define MAX_CHAOSSHIELD_SPELL 15
const int randomspell[MAX_CHAOSSHIELD_SPELL] = { SPELL_STRENGTH,
						 SPELL_DEXTERITY,
						 SPELL_COLDSHIELD,
						 SPELL_FIRESHIELD,
						 SPELL_HASTE,
						 SPELL_FLY,
						 SPELL_GLOBE,
						 SPELL_FEEBLEMIND,
						 SPELL_STONE_SKIN,
						 SPELL_BALADORS_PROTECTION,
						 SPELL_LIGHTNINGSHIELD,
						 SPELL_FARSEE,
						 SPELL_DETECT_INVISIBLE,
						 SPELL_SLOW,
						 SPELL_WITHER };

int parse_chaos_shield(P_char victim, P_char ch)
{
	int chance;

	if (!affected_by_spell(ch, SPELL_CHAOS_SHIELD))
		return FALSE;

	chance = (int)get_property("spell.chaosshield.perc", 10.000);

	if (chance < number(0, 101))
		return FALSE;

	act("&+LThe chaotic shield surrounding $n flares as it absorbs some of $N's spell.&n",
	    FALSE, ch, 0, victim, TO_NOTVICTROOM);
	act("&+LThe chaotic shield surrounding $n flares as it absorbs some of your spell.&n",
	    FALSE, ch, 0, victim, TO_VICT);
	act("&+LThe chaotic shield surrounding you flares as it absorbs some of $N's spell.&n",
	    FALSE, ch, 0, victim, TO_CHAR);

	((*skills[randomspell[number(0, MAX_CHAOSSHIELD_SPELL - 1)]].spell_pointer)(
		(int)GET_LEVEL(ch), ch, 0, SPELL_TYPE_SPELL, ch, 0));

	return TRUE;
}
