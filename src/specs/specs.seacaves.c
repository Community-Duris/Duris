/* Sea Caves equipment procedures. */

#include "combat/damage.h"
#include "magic/spells.h"

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

int cutting_dagger(P_obj obj, P_char ch, int cmd, char *arg)
{
	struct damage_messages messages = {
		"&+wThe &+Lblade &+won &N$q &+rcu&+Rts &+wyour finger drawing a little &+rb&+Rl&+roo&+Rd&+w.&N",
		0,
		"&+wThe &+Lblade &+won &N$n's &N$q &+rcu&+Rts &+wyour finger drawing a little &+rb&+Rl&+roo&+Rd&+w.&N",
		0,
		0,
		0,
		0,
		obj
	};

	if (IS_ALIVE(ch) && IS_PC(ch) && OBJ_ROOM(obj) && cmd == CMD_GET &&
	    obj == get_obj_in_list_vis(ch, arg, world[ch->in_room].contents))
	{
		spell_damage(ch, ch, 4, SPLDAM_GENERIC, 0, &messages);
	}

	return FALSE;
}
