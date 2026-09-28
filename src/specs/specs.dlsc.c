/* Domain of Lost Souls equipment procedures. */

#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "world/events.h"
#include "cmd/interp.h"
#include "core/utility.h"
#include "core/utils.h"
#include "combat/damage.h"
#include "world/specs.prototypes.h"
#include "magic/spells.h"

int kvasir_dagger(P_obj obj, P_char ch, int cmd, char *arg)
{
	P_char victim;

	if (cmd == CMD_SET_PERIODIC)
	{
		return FALSE;
	}

	victim = legacy_proc_arg<P_char>(arg);
	// 1/30 chance.
	if (cmd != CMD_MELEE_HIT || !IS_ALIVE(ch) || !OBJ_WORN_BY(obj, ch) || !IS_ALIVE(victim) ||
	    number(0, 29))
	{
		return FALSE;
	}

	switch (number(0, 3))
	{
	case 0:
		act("&+WFrost runs down the blade of a&n $q\n"
		    "&+Was the spirit of the &+Cice dragon &+Wstirs in its eternal prison...&n",
		    TRUE, ch, obj, victim, TO_NOTVICT);
		act("&+WFrost runs down the blade of a&n $q\n"
		    "&+Was the spirit of the &+Cice dragon &+Wstirs in its eternal prison...&n",
		    TRUE, ch, obj, victim, TO_CHAR);
		act("&+WFrost runs down the blade of a&n $q\n"
		    "&+Was the spirit of the &+Cice dragon &+Wstirs in its eternal prison...&n",
		    TRUE, ch, obj, victim, TO_VICT);
		spell_cone_of_cold(60, ch, NULL, SPELL_TYPE_SPELL, victim, obj);
		break;

	case 1:
		act("&+WFrost runs down the blade of a&n $q\n"
		    "&+Was the spirit of the &+Cice dragon &+Wstirs in its eternal prison...&n",
		    TRUE, ch, obj, victim, TO_NOTVICT);
		act("&+WFrost runs down the blade of a&n $q\n"
		    "&+Was the spirit of the &+Cice dragon &+Wstirs in its eternal prison...&n",
		    TRUE, ch, obj, victim, TO_CHAR);
		act("&+WFrost runs down the blade of a&n $q\n"
		    "&+Was the spirit of the &+Cice dragon &+Wstirs in its eternal prison...&n",
		    TRUE, ch, obj, victim, TO_VICT);
		spell_ice_storm(60, ch, NULL, 0, victim, obj);
		break;

	case 2:
		act("&+RFlames &+Wcrackle along the blade of a&n $q\n"
		    "&+Was the spirit of the &+Rfire dragon &+Wstirs in its eternal prison...&n",
		    TRUE, ch, obj, victim, TO_NOTVICT);
		act("&+RFlames &+Wcrackle along the blade of a&n $q\n"
		    "&+Was the spirit of the &+Rfire dragon &+Wstirs in its eternal prison...&n",
		    TRUE, ch, obj, victim, TO_CHAR);
		act("&+RFlames &+Wcrackle along the blade of a&n $q\n"
		    "&+Was the spirit of the &+Rfire dragon &+Wstirs in its eternal prison...&n",
		    TRUE, ch, obj, victim, TO_VICT);
		spell_immolate(60, ch, NULL, 0, victim, obj);
		break;

	case 3:
		act("&+RFlames &+Wcrackle along the blade of a&n $q\n"
		    "&+Was the spirit of the &+Rfire dragon &+Wstirs in its eternal prison...&n",
		    TRUE, ch, obj, victim, TO_NOTVICT);
		act("&+RFlames &+Wcrackle along the blade of a&n $q\n"
		    "&+Was the spirit of the &+Rfire dragon &+Wstirs in its eternal prison...&n",
		    TRUE, ch, obj, victim, TO_CHAR);
		act("&+RFlames &+Wcrackle along the blade of a&n $q\n"
		    "&+Was the spirit of the &+Rfire dragon &+Wstirs in its eternal prison...&n",
		    TRUE, ch, obj, victim, TO_VICT);
		spell_firestorm(60, ch, NULL, 0, victim, obj);
		break;
	}
	return TRUE;
}

int critical_attack_proc(P_obj obj, P_char ch, int cmd, char *arg)
{
	P_char victim;
	int dam = cmd / 1000;

	if (cmd == CMD_SET_PERIODIC)
	{
		return FALSE;
	}

	if (!IS_ALIVE(ch))
	{
		if (OBJ_WORN(obj) && IS_ALIVE(obj->loc.wearing))
		{
			ch = obj->loc.wearing;
		}
		else
		{
			return FALSE;
		}
	}

	if (!dam)
	{
		return FALSE;
	}

	victim = legacy_proc_arg<P_char>(arg);
	// 1/30 chance.
	if (!IS_ALIVE(victim) || IS_DRAGON(victim) || number(0, 29))
	{
		return FALSE;
	}

	switch (number(0, 2))
	{
	case 0:
		act("&nYou slam the &+yhandle&n of your $q in $N's face!&n", FALSE, ch, obj, victim,
		    TO_CHAR);
		act("$n makes a quick move and slams the &+yhandle&n of $s $q in your face!&n",
		    FALSE, ch, obj, victim, TO_VICT);
		act("$n makes a quick move and slams the &+yhandle&n of $s $q in $N's face!&n",
		    FALSE, ch, obj, victim, TO_NOTVICT);
		blind(ch, victim, number(2, 10) * WAIT_SEC);
		break;
	case 1:
		act("&nYou point your&n $q at&n $N and utter a word of &+rc&+Ro&+rm&+Rm&+ra&+Rn&+rd&n!",
		    FALSE, ch, obj, victim, TO_CHAR);
		act("$n points $s $q at &+L_YOU_&n and utters a word of &+rc&+Ro&+rm&+Rm&+ra&+Rn&+rd&n!",
		    FALSE, ch, obj, victim, TO_VICT);
		act("$n points $s $q at $N and utters a word of &+rc&+Ro&+rm&+Rm&+ra&+Rn&+rd&n!",
		    FALSE, ch, obj, victim, TO_NOTVICT);
		Stun(victim, ch, PULSE_VIOLENCE, FALSE);
		break;
	case 2:
		act("&nYou &+rhook&n your $q around $N's leg and pull $M to the ground!&n", FALSE,
		    ch, obj, victim, TO_CHAR);
		act("$n suddenly &+rhooks&n $s $q around your leg and pulls you to the ground!&n",
		    FALSE, ch, obj, victim, TO_VICT);
		act("$n suddenly &+rhooks&n $s $q around&n $N's leg and pulls $M to the ground!&n",
		    FALSE, ch, obj, victim, TO_NOTVICT);
		SET_POS(victim, POS_SITTING + GET_STAT(victim));
		break;
	}
	return TRUE;
}
