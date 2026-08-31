/* Add keleks procs here, or i will swallow your soul, and seif will swallow your load! */
#include <ctype.h>
#include <list>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <time.h>
using namespace std;

#include "prototypes.h"
#include "structs.h"
#include "net/comm.h"
#include "world/db.h"
#include "world/events.h"
#include "cmd/interp.h"
#include "utils.h"
#include "guild/assocs.h"
#include "combat/damage.h"
#include "world/graph.h"
#include "combat/justice.h"
#include "classes/reavers.h"
#include "specs/specs.keleks.h"
#include "world/specs.prototypes.h"
#include "magic/spells.h"
#include "world/weather.h"

int deliverer_hammer(P_obj obj, P_char ch, int cmd, char *arg)
{
	P_char victim;
	int dam;

	if (cmd == CMD_SET_PERIODIC)
	{
		return FALSE;
	}

	if (!IS_ALIVE(ch) || !OBJ_WORN_POS(obj, WIELD))
	{
		return FALSE;
	}

	if (cmd == CMD_MELEE_HIT)
	{
		victim = legacy_proc_arg<P_char>(arg);

		if (!IS_ALIVE(victim) ||
		    !(IS_UNDEAD(victim) || IS_AFFECTED(victim, AFF_WRAITHFORM)))
		{
			return FALSE;
		}

		// 1/20 chance.
		if (!number(0, 19))
		{
			act("&+WYour $q&+W is surrouned by a holy glow as it strikes $N&+W!&n",
			    FALSE, ch, obj, victim, TO_CHAR);
			act("&+W$n&+W's glows with a &+wholy &+Maura&+W!&n", FALSE, ch, obj, victim,
			    TO_ROOM);
			dam = number(50, 250);
			spell_damage(ch, victim, dam, SPLDAM_HOLY,
				     SPLDAM_NOSHRUG | SPLDAM_NODEFLECT, 0);
		}
	}
	return FALSE;
}
