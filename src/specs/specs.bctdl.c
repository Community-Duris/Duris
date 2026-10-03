/* BCTDL object special procedures. */

#include <time.h>

#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utils.h"
#include "net/comm.h"
#include "cmd/interp.h"
#include "combat/damage.h"
#include "classes/reavers.h"
#include "magic/spells.h"

int bel_sword(P_obj obj, P_char ch, int cmd, char *arg)
{
	P_char vict;
	int dam = cmd / 1000, curr_time;
	struct damage_messages messages = {
		"&+RV&+ra&+Rmp&+ri&+Rr&+ri&+Rc &+renergy &+Linfuses into your body and up your arms, seeping into your &+Cs&+co&+Cu&+cl&+L!&n",
		"&+LYou feel yourself becoming &+Rweaker &+Las your &+Wl&+wi&+Wf&+we&+Wf&+wo&+Wrc&+we &+Lis &+rdrained &+Lout of you!&n",
		"&+rS&+Ra&+rt&+Ra&+rn&+Ri&+rc &+Renergy &+Linfuses into $n's &+Lhand and up $s arm, greatly strengthening $m.&n",
		"",
		"",
		"",
		0,
		obj
	};

	if (cmd == CMD_SET_PERIODIC)
	{
		return FALSE;
	}

	if (!IS_ALIVE(ch) || !OBJ_WORN(obj))
	{
		return FALSE;
	}
	ch = obj->loc.wearing;

	if (arg && (cmd == CMD_REMOVE))
	{
		if (isname(arg, obj->name) || isname(arg, "all"))
		{
			if (affected_by_spell(ch, SPELL_ILIENZES_FLAME_SWORD))
			{
				affect_from_char(ch, SPELL_ILIENZES_FLAME_SWORD);
			}
		}
	}

	if (arg && (cmd == CMD_SAY))
	{
		if (isname(arg, "eld"))
		{
			curr_time = time(NULL);
			// 4 min timer.
			if (obj->timer[0] + 240 <= curr_time)
			{
				act("You say 'eld'", FALSE, ch, 0, 0, TO_CHAR);
				act("&+LYour $q &+rg&+Rl&+Yo&+Rw&+rs &+Lbriefly.&n", FALSE, ch, obj,
				    obj, TO_CHAR);

				act("$n says 'eld'", TRUE, ch, obj, NULL, TO_ROOM);
				act("&+LThe $q &+Lcarried by $n &+rg&+Rl&+Yo&+Rw&+rs&n &+Lbriefly.&n",
				    TRUE, ch, obj, NULL, TO_ROOM);
				act("&+rS&+Ra&+rt&+Ra&+rn&+Ri&+rc &+Rf&+rl&+Ra&+rm&+Re&+rs &+Lengulf $n's &+Lblade as it comes alive with the power of &+rDa&+Rrag&+ror&+L!&n",
				    TRUE, ch, obj, NULL, TO_ROOM);
				spell_ilienzes_flame_sword(51, ch, 0, SPELL_TYPE_SPELL, ch, 0);

				obj->timer[0] = curr_time;

				return TRUE;
			}
		}
	}

	if (!dam)
	{
		return FALSE;
	}

	vict = legacy_proc_arg<P_char>(arg);
	if (!IS_ALIVE(vict) || (GET_RACE(vict) != RACE_DEMON && GET_RACE(vict) != RACE_DEVIL))
	{
		return FALSE;
	}
	// 1/20 chance.
	if (number(0, 19))
	{
		return FALSE;
	}

	dam = MIN((GET_HIT(vict) + 9), 100);
	act("&+LYour $q &+binfuses &+Lwith &+rs&+Ra&+rt&+Ra&+rn&+Ri&+rc &+Renergy &+Las it slashes into $N!&n",
	    FALSE, ch, obj, vict, TO_CHAR);
	act("$n's $q &+bglows &+rcrimson &+Las it slashes into $N!&n", FALSE, ch, obj, vict,
	    TO_NOTVICT);
	act("$n's $q &+bglows &+rcrimson &+Las it slices into you!&n", FALSE, ch, obj, vict,
	    TO_VICT);
	spell_damage(ch, vict, 400, SPLDAM_NEGATIVE,
		     SPLDAM_NODEFLECT | SPLDAM_NOSHRUG | RAWDAM_NOKILL, &messages);

	vamp(ch, dam / 2, (int)(GET_MAX_HIT(ch) * VAMPPERCENT(ch)));

	return TRUE;
}
