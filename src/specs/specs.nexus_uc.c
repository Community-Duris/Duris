/* Nexus Upper City object procedures. */

#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "world/db.h"
#include "world/events.h"
#include "cmd/interp.h"
#include "core/utility.h"
#include "core/utils.h"
#include "world/specs.prototypes.h"

extern P_room world;

int obj_tp_no_high_levels(P_obj obj, P_char ch, int cmd, char *arg)
{
	char arg1[MAX_INPUT_LENGTH];

	// Not a periodic proc, nor can we check an object that doesn't exist, nor stop a char that isn't alive.
	if (cmd == CMD_SET_PERIODIC || !obj || !IS_ALIVE(ch))
	{
		return FALSE;
	}

	// If the command isn't the trigger command.
	if (cmd != obj->value[1])
	{
		return FALSE;
	}

	one_argument(arg, arg1);
	// If we're not triggering the object.
	if (obj != get_obj_in_list(arg1, world[ch->in_room].contents))
	{
		return FALSE;
	}

	// If they're too high level (and not a god).
	if ((GET_LEVEL(ch) > obj->value[3]) && !IS_TRUSTED(ch))
	{
		act("$p is not powerful enough to transport you.", FALSE, ch, obj, NULL, TO_CHAR);
		return TRUE;
	}
	return FALSE;
}
