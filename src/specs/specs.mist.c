#include "prototypes.h"
#include "structs.h"
#include "comm.h"
#include "db.h"
#include "events.h"
#include "interp.h"
#include "utils.h"
#include <stdio.h>
#include "specs.prototypes.h"
#include "spells.h"

/*
   extern variables
 */

extern P_room world;
extern struct zone_data *zone_table;

/*
 * Nothing fancy here, just ripped off the block proc for the
 * clawed cavern golems. Thanks previous coders!
 */

int mist_protect(P_char ch, P_char pl, int cmd, char * /*arg*/)
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

	if ((ch->in_room == real_room(6300)) && (cmd == CMD_NORTH))
	{
		if (GET_LEVEL(pl) > 36 && !IS_TRUSTED(pl))
		{
			act("&+LThe guardian druid solemnly blocks $n&+L's passage into the forest.&n",
			    FALSE, pl, 0, 0, TO_ROOM);
			send_to_char(
				"&+LThe guardian druid looks at you solemnly, shaking his head.&n\r\n",
				pl);
			return TRUE;
		}
	}
	return FALSE;
}
