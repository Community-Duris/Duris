/* Torg special procedures. */

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

#define INVASION_ROOM_VNUM 29116
#define INVASION_LEADER_VNUM 28975

int timoro_die(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	P_char i;
	char dir[] = "down";

	if (cmd != CMD_DEATH)
		return FALSE;

	act("\nA banging sound can be heard as if someone is pounding the walls.\n"
	    "All of a sudden large &+ycracks&n form along the ceiling and chunks of &+Lstone&n slam in the floor.\n"
	    "A large band of dwarves pour out of the hole, faces grim.\n",
	    TRUE, ch, 0, 0, TO_ROOM);

	for (i = world[real_room(INVASION_ROOM_VNUM)].people; i; i = i->next_in_room)
	{
		if ((IS_NPC(i)) && (GET_VNUM(i) == INVASION_LEADER_VNUM))
		{
			command_interpreter(i, dir);
			break;
		}
	}
	return FALSE;
}
