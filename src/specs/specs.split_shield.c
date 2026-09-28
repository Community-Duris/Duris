/* Area-owned special procedures. */

#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "world/db.h"
#include "world/events.h"
#include "cmd/interp.h"
#include "core/utility.h"
#include "core/utils.h"
#include "world/specs.prototypes.h"
#include <string.h>

/*
 * ** used in the split shield area, copy of the guild guard changed
 * ** by Thomas Lowery 16 Jun 94
 */

int shady_man(P_char ch, P_char pl, int cmd, char * /*arg*/)
{
	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return FALSE;

	if (IS_TRUSTED(ch))
	{
		return FALSE;
	}

	if ((ch->in_room == real_room(10302)) && (cmd == CMD_SOUTH))
	{
		if (GET_LEVEL(pl) > 20 && GET_LEVEL(pl) < 57)
		{
			act("A shady old man whispers something to $n, stopping $m with his hand.",
			    FALSE, pl, 0, 0, TO_ROOM);
			send_to_char(
				"A shady old man whispers 'This area is far below you, unless you wish to fight me.'\r\n",
				pl);
			return TRUE;
		}
	}
	return FALSE;
}

int gate_guard(P_char ch, P_char pl, int cmd, char * /*arg*/)
{
	char Gbuf3[MAX_STRING_LENGTH], Gbuf4[MAX_STRING_LENGTH];

	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return FALSE;

	if (IS_TRUSTED(ch))
	{
		return FALSE;
	}
	strcpy(Gbuf4, "a gate guard whispers 'We don't want your type here, get lost.'\r\n");
	strcpy(Gbuf3, "a gate guard whispers something to $n, stopping $m with his hand.");

	if ((ch->in_room == real_room(10320)) && (cmd == CMD_SOUTH))
	{
		if (GET_LEVEL(pl) < 51)
		{
			act(Gbuf3, FALSE, pl, 0, 0, TO_ROOM);
			send_to_char(Gbuf4, pl);
			return TRUE;
		}
	}
	return FALSE;
}
