/* Foggy Woods object procedures. */

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

int fw_ruby_monocle(P_obj obj, P_char /*ch*/, int cmd, char * /*arg*/)
{
	int randroom;

	/* check for periodic event calls */
	if (cmd == CMD_SET_PERIODIC)
	{
		return TRUE;
	}

	if (!OBJ_ROOM(obj) || cmd != CMD_PERIODIC)
	{
		return FALSE;
	}

	// If obj is in a good room, and zone just reset.
	if ((world[obj->loc.room].number >= 90124) && (world[obj->loc.room].number <= 90142) &&
	    (zone_table[world[obj->loc.room].zone].age == 0))
	{
		randroom = number(90124, 90142);
		obj_from_room(obj);
		obj_to_room(obj, real_room(randroom));
		return TRUE;
	}
	return FALSE;
}
