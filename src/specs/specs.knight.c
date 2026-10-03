/* Knight object procedures. */

#include <time.h>

#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "world/db.h"
#include "world/events.h"
#include "cmd/interp.h"
#include "core/utility.h"
#include "core/utils.h"
#include "world/specs.prototypes.h"
#include "magic/spells.h"

extern P_room world;

int pathfinder(P_obj obj, P_char ch, int cmd, char *argument)
{
	char *arg;
	int curr_time;

	if (cmd == CMD_SET_PERIODIC)
	{
		return FALSE;
	}

	if (!IS_ALIVE(ch) || !obj || !OBJ_WORN(obj))
	{
		return FALSE;
	}

	// Any powers activated by keywords? Right here, bud.
	if (argument && (cmd == CMD_SAY))
	{
		arg = argument;

		while (*arg == ' ')
		{
			arg++;
		}

		if (!strcmp(arg, "path"))
		{
			if (!say(ch, arg))
			{
				return TRUE;
			}
			curr_time = time(NULL);

			// Every 800 sec.
			if (obj->timer[0] + 800 <= curr_time)
			{
				act("Your $q &+ghums&n briefly and points you in a direction.",
				    FALSE, ch, obj, obj, TO_CHAR);
				act("$n's $q &+ghums&n briefly.", TRUE, ch, obj, NULL, TO_ROOM);
				spell_pass_without_trace(30, ch, 0, SPELL_TYPE_SPELL, ch, 0);
				obj->timer[0] = curr_time;
			}
			return TRUE;
		}
	}
	return FALSE;
}
