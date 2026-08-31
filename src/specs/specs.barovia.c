/* Made by Zion, to house all of the procs for Barovia, zone written by Fotenak 2007 */
#include <ctype.h>
#include <list>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <time.h>
using namespace std;

#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "world/db.h"
#include "world/events.h"
#include "cmd/interp.h"
#include "core/utils.h"
#include "guild/assocs.h"
#include "combat/damage.h"
#include "world/graph.h"
#include "combat/justice.h"
#include "classes/reavers.h"
#include "specs/specs.barovia.h"
#include "world/specs.prototypes.h"
#include "magic/spells.h"
#include "world/weather.h"

int barovia_undead_necklace(P_obj obj, P_char ch, int cmd, char *arg)
{
	struct proc_data *data;
	P_char victim;

	if (cmd == CMD_SET_PERIODIC)
	{
		return FALSE;
	}
	// 1/20 chance.
	if (cmd != CMD_GOTHIT || number(0, 19))
	{
		return FALSE;
	}
	if (!(data = legacy_proc_arg<struct proc_data *>(arg)))
	{
		return FALSE;
	}
	victim = data->victim;
	if (!IS_ALIVE(victim) || !IS_UNDEAD(victim))
	{
		return FALSE;
	}
	act("&+WBright white light surrounds $n's $q, &+Wand it sends out a wave of holy energy!&N",
	    TRUE, ch, obj, victim, TO_NOTVICT);
	act("&+WBright white light surrounds $n's $q, &+Wand it sends out a wave of holy energy!&N",
	    TRUE, ch, obj, victim, TO_VICT);
	act("&+WBright white light surrounds your $q, &+Wand it sends out a wave of holy energy!&N",
	    TRUE, ch, obj, victim, TO_CHAR);
	switch (number(0, 2))
	{
	case 0:
		spell_destroy_undead(60, ch, NULL, 0, victim, 0);
		break;
	case 1:
		spell_damage(ch, victim, 200, SPLDAM_HOLY, SPLDAM_NOSHRUG | SPLDAM_NODEFLECT, 0);
		break;
	case 2:
		if (!fear_check(victim))
		{
			do_flee(victim, 0, 1);
		}
		else
		{
			act("&+wSeeing the first attempt fail, you send a new wave of &+Wholy energy&n&+w at $N.",
			    FALSE, ch, 0, victim, TO_CHAR);
			act("&+wSeeing the first attempt fail, $n &n&+wsends a new wave of &+Wholy energy&n&+w at $N.",
			    TRUE, ch, 0, victim, TO_NOTVICT);
			act("&+wSeeing the first attempt fail, $n &n&+wsends a new wave of &+Wholy energy&n&+w at you!.",
			    TRUE, ch, 0, victim, TO_VICT);
			spell_destroy_undead(20, ch, NULL, 0, victim, 0);
		}
		break;
	}
	return TRUE;
}
