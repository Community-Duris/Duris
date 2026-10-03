/* Battlefields object special procedures. */

#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utils.h"
#include "net/comm.h"
#include "cmd/interp.h"
#include "magic/spells.h"

int righteous_blade(P_obj obj, P_char ch, int cmd, char *arg)
{
	P_char victim;

	if (cmd == CMD_SET_PERIODIC)
	{
		return FALSE;
	}

	if (cmd != CMD_MELEE_HIT || !IS_ALIVE(ch) || !(victim = legacy_proc_arg<P_char>(arg)))
	{
		return FALSE;
	}
	if (!IS_ALIVE(victim))
	{
		return FALSE;
	}

	// 1/30 chance.
	if (!number(0, 29))
	{
		if (!affected_by_spell(ch, SPELL_VIRTUE))
		{
			if ((IS_EVIL(ch) && !IS_EVIL(victim)) || (IS_GOOD(ch) && !IS_GOOD(victim)))
			{
				act("&+wA &+Wbright &+Wpu&+wl&+Ws&+wa&+Wt&+wi&+Wng &n&+Cg&+Wlo&+Cw&n&+w surrounds $q&+w, and after short while it spreads over whole $n's &+wbody!",
				    FALSE, ch, obj, victim, TO_ROOM);
				act("&+wA &+Wbright &+Cg&+Wlo&+Ww&n&+w surrounds your $q&+w, and then it spreads over all of you!",
				    FALSE, ch, obj, victim, TO_CHAR);
				spell_virtue(50, ch, 0, SPELL_TYPE_SPELL, ch, 0);
				return FALSE;
			}
		}
		// 1/30 * 1/4 == 1/120 chance.
		if (number(0, 3))
		{
			act("$n&+W's weapon briefly glows and vibrates upon striking $N&+W...",
			    FALSE, ch, obj, victim, TO_ROOM);
			act("$n&+W's weapon briefly glows and vibrates upon striking you...", FALSE,
			    ch, obj, victim, TO_VICT);
			act("&+WYour $q&+W briefly glows and vibrates as it strikes $N&+W...",
			    FALSE, ch, obj, victim, TO_CHAR);
			spell_life_bolt(60, ch, 0, SPELL_TYPE_SPELL, victim, 0);
			return FALSE;
		}
		else
		{
			act("$n&+W's weapon glows &+Rred&n&+W and vibrates upon striking $N&+W...",
			    FALSE, ch, obj, victim, TO_ROOM);
			act("$n&+W's weapon glows &+Rred&n&+W and vibrates upon striking you&+W...",
			    FALSE, ch, obj, victim, TO_VICT);
			act("&+WYour $q&+W glows &+Rred&n&+W and vibrates as it strikes $N&+W...",
			    FALSE, ch, obj, victim, TO_CHAR);

			if (IS_UNDEAD(victim))
			{
				spell_destroy_undead(50, ch, 0, SPELL_TYPE_SPELL, victim, 0);
			}
			else
			{
				// dispel_lifeforce does no damage.
				for (int i = 1 + GET_LEVEL(ch) / 17;
				     i && !affected_by_spell(victim, SPELL_DISPEL_LIFEFORCE); i--)
				{
					spell_dispel_lifeforce(50, ch, 0, SPELL_TYPE_SPELL, victim,
							       0);
				}
			}
		}
	}

	return FALSE;
}
