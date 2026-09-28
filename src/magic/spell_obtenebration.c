#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "core/utility.h"
#include "core/utils.h"
#include "core/defines.h"
#include "combat/damage.h"
#include "magic/spells.h"

static void spell_single_obtenebration(int level, P_char ch, char * /*arg*/, int /*type*/,
				       P_char victim, P_obj /*obj*/)
{
	int dam;
	struct damage_messages messages = {
		"&+LYour voice fills the living with dread and despair!&n",
		"&+LA wave of blackness sweeps over you, eating your flesh!&n",
		"&+LA pitch-darkness sweeps across the area, scalding the living!&n",
		"&+LYour voice disconnects every bit of $N's being.&n",
		"&+LA wave of blackness utterly consumes you.&n",
		"&+LA pitch-darkness consumes $N!&n"
	};

	if (!IS_ALIVE(ch) || !IS_ALIVE(victim))
	{
		return;
	}

	dam = 100 + level * 6 + number(1, 40);
	dam = dam * get_property("spell.area.damage.factor.obtenebration", 1.000);

	spell_damage(ch, victim, dam, SPLDAM_SOUND, 0, &messages);

	if (IS_ALIVE(ch) && number(1, 100) < 20)
	{
		act("&+LYou direct a cloud of &+yu&+Lm&+yb&+Lr&+ya&+Ll &+yd&+Lu&+ys&+Lt in $N's face, blinding them!&n",
		    FALSE, ch, 0, victim, TO_CHAR);
		act("&+LA cloud of &+yu&+Lm&+yb&+Lr&+ya&+Ll &+yd&+Lu&+ys&+Lt kicks up, blinding $N!",
		    FALSE, ch, 0, victim, TO_ROOM);
		act("&+LA cloud of &+yu&+Lm&+yb&+Lr&+ya&+Ll &+yd&+Lu&+ys&+Lt kicks up, blinding YOU!",
		    FALSE, ch, 0, victim, TO_VICT);
		blind(ch, victim, number(4, 12) * WAIT_SEC);
	}

	if (IS_ALIVE(ch) && number(1, 100) < 1)
	{
		act("&+LYou cause a rift in the spacetime continuum, which eats $N!&n", FALSE, ch,
		    0, victim, TO_CHAR);
		act("&+LA void of nothingness appears behind $N whom vanishes with a soft pop!&n",
		    FALSE, ch, 0, victim, TO_ROOM);
		act("&+LYour atoms explode and disperse, reappearing elsewhere!", FALSE, ch, 0,
		    victim, TO_VICT);
		spell_teleport(GET_LEVEL(ch), ch, 0, 0, victim, 0);
	}
}

void spell_obtenebration(int level, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
			 P_obj /*obj*/)
{
	{
		send_to_char("&+rYou begins chanting in a demonic voice.\n", ch);
		act("&+r$n begin chanting in a demonic voice!", FALSE, ch, 0, 0, TO_ROOM);
	}

	zone_spellmessage(ch->in_room, TRUE,
			  "&+LOff in the distance there is a &+wpiercing vibration.\n",
			  "&+LOff in the distance to the %s there is a &+wpiercing vibration.\n");

	cast_as_damage_area(ch, spell_single_obtenebration, level, victim,
			    get_property("spell.area.minChance.obtenebration", 50),
			    get_property("spell.area.chanceStep.obtenebration", 20));
}
