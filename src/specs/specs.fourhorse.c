/* Four Horsemen equipment procedures. */

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

int brainripper(P_obj obj, P_char ch, int cmd, char *arg)
{
	int dam = cmd / 1000;
	P_char victim;
	int rand;

	if (cmd == CMD_SET_PERIODIC)
	{
		return FALSE;
	}

	if (!dam || !IS_ALIVE(ch) || !OBJ_WORN_BY(obj, ch))
	{
		return FALSE;
	}

	victim = legacy_proc_arg<P_char>(arg);
	// 1/30 chance.
	if (!IS_ALIVE(victim) || number(0, 29))
	{
		return FALSE;
	}

	rand = number(1, 10);
	// 70% chance.
	if (rand < 8)
	{
		act("&+W$n's&N $q &+Wglows!&N", TRUE, ch, obj, victim, TO_NOTVICT);
		act("&+WYour&N $q &+Wglows!!&N", TRUE, ch, obj, victim, TO_CHAR);
		act("&+W$n's&N $q &+Wglows!!&N", TRUE, ch, obj, victim, TO_VICT);
		spell_inflict_pain(40, ch, 0, 0, victim, obj);
		spell_feeblemind(60, ch, NULL, 0, victim, 0);
	}
	else
	{
		act("&+W$n's&N $q &+Lglows!&N", TRUE, ch, obj, victim, TO_NOTVICT);
		act("&+WYour&N $q &+Lglows!!&N", TRUE, ch, obj, victim, TO_CHAR);
		act("&+W$n's&N $q &+Lglows!!&N", TRUE, ch, obj, victim, TO_VICT);
		berserk(ch, 6 * PULSE_VIOLENCE);
	}
	return (TRUE);
}

int mankiller(P_obj obj, P_char ch, int cmd, char *arg)
{
	int dam = cmd / 1000;
	P_char victim;

	if (cmd == CMD_SET_PERIODIC)
	{
		return FALSE;
	}

	if (!dam || !IS_ALIVE(ch) || !OBJ_WORN(obj) || (obj->loc.wearing != ch))
	{
		return FALSE;
	}

	victim = legacy_proc_arg<P_char>(arg);
	// 1/15 chance.
	if (!IS_ALIVE(victim) || GET_SEX(victim) != SEX_MALE || number(0, 14))
	{
		return FALSE;
	}
	/* Dunno why we skip this, but ok...
	  act("&+w$n's&N $q &Nscreams out with a female voice &+W'&+MBegone you filthy male!&+W'&N", TRUE, ch, obj, victim, TO_NOTVICT);
	  act("&+wYour $q &Nscreams out with a female voice &+W'&+MBegone you filthy male!&+W'&N", TRUE, ch, obj, victim, TO_CHAR);
	  act("&+w$n's&N $q &Nscreams out with a female voice &+W'&+MBegone you filthy male!&+W'&N", TRUE, ch, obj, victim, TO_VICT);
	*/
	switch (number(0, 2))
	{
	case 0:
		act("&+w$n's&N $q &Nlets out a fearsome &+MSCREAM&n, directed at $N!&N", TRUE, ch,
		    obj, victim, TO_NOTVICT);
		act("You grin slightly, as $q&N &+wspirit's &+MSCREAM&n causes $N to writhe in agony!&N",
		    TRUE, ch, obj, victim, TO_CHAR);
		act("&+w$n's&N $q pierces your mind with a &+wsupernatural&n &+MSCREAM&n, stunning you!&N",
		    TRUE, ch, obj, victim, TO_VICT);
		Stun(victim, ch, (int)(PULSE_VIOLENCE * 0.5), FALSE);
		return TRUE;
		break;
	case 1:
		act("&+w$n's&N $q &Nscreams out with a female voice &+W'&+MBegone, you filthy male!&+W'&N",
		    TRUE, ch, obj, victim, TO_NOTVICT);
		act("&+w$n's&N $q &Nscreams out with a female voice &+W'&+MBegone, you filthy male!&+W'&N",
		    TRUE, ch, obj, victim, TO_CHAR);
		act("&+w$n's&N $q &Nscreams out with a female voice &+W'&+MBegone, you filthy male!&+W'&N",
		    TRUE, ch, obj, victim, TO_VICT);
		spell_nightmare(60, ch, NULL, 0, victim, 0);
		return TRUE;
		break;
	case 2:
		act("&+w$n's $q&n emits an ear piercing &+RSHRIEK&n!&N", TRUE, ch, obj, victim,
		    TO_NOTVICT);
		act("$q&n emits an ear piercing &+RSHRIEK&n! You instinctively cover your ears.&N",
		    TRUE, ch, obj, victim, TO_CHAR);
		act("&+RARGH!&n As $n's $q&n emits a piercing &+RSHRIEK&n, you are filled with excruciating pain!&N",
		    TRUE, ch, obj, victim, TO_VICT);
		spell_shatter(56, ch, NULL, 0, victim, 0);
		return TRUE;
		break;
	}
	return FALSE;
}
