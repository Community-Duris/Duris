/* Special procedures for Olympus. */

#include <stdio.h>
#include <time.h>

#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utils.h"
#include "net/comm.h"
#include "cmd/interp.h"
#include "world/db.h"
#include "world/specs.prototypes.h"

extern P_room world;
extern P_index obj_index;
extern struct zone_data *zone_table;

int olympus_portal(P_obj obj, P_char /*ch*/, int cmd, char * /*arg*/)
{
	int to_room, base, real_top, real_bottom, origin_portal, origin_room;
	P_obj portal = NULL;
	char Gbuf1[MAX_STRING_LENGTH];

	if (cmd == CMD_SET_PERIODIC)
	{
		return TRUE;
	}

	// Every 30 sec..
	if (!obj || cmd != CMD_PERIODIC || obj->timer[0] + 30 > time(NULL))
	{
		return FALSE;
	}

	switch (obj_index[obj->R_num].virtual_number)
	{
	case 99801:
		base = 140000;
		origin_portal = real_object(99800);
		origin_room = real_room(99805);
		break;
	case 99803:
		base = 150000;
		origin_portal = real_object(99802);
		origin_room = real_room(99806);
		break;
	case 99805:
		base = 160000;
		origin_portal = real_object(99804);
		origin_room = real_room(99807);
		break;
	case 99807:
		base = 170000;
		origin_portal = real_object(99806);
		origin_room = real_room(99808);
		break;
	default:
		return FALSE;
	}
	real_bottom = zone_table[world[real_room0(base)].zone].real_bottom;
	real_top = zone_table[world[real_room0(base)].zone].real_top;
	do
	{
		to_room = number(real_bottom, real_top);
	} while (IS_SET(world[to_room].sector_type, SECT_OCEAN));

	if (OBJ_ROOM(obj))
	{
		snprintf(Gbuf1, MAX_STRING_LENGTH,
			 "&+WThe air shifts slighty as %s&+W folds up and vanishes!\n",
			 obj->short_description);
		send_to_room(Gbuf1, obj->loc.room);
		obj_from_room(obj);
		obj_to_room(obj, to_room);
		snprintf(Gbuf1, MAX_STRING_LENGTH,
			 "&+WA slight breeze wafts by as %s&+W materializes in the room!\n",
			 obj->short_description);
		send_to_room(Gbuf1, obj->loc.room);
		if (origin_room > 0 && origin_portal > 0)
		{
			portal = get_obj_in_list_num(origin_portal, world[origin_room].contents);
			if (portal)
			{
				portal->value[0] = world[obj->loc.room].number;
			}
		}
		obj->timer[0] = time(NULL);
		return TRUE;
	}

	return FALSE;
}
