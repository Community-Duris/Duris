/* Ceothia equipment procedures. */

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

int ogre_warlords_sword(P_obj /*obj*/, P_char ch, int cmd, char * /*arg*/)
{
	int dam = cmd / 1000;

	if (cmd == CMD_SET_PERIODIC)
	{
		return TRUE;
	}

	if (!dam || !IS_ALIVE(ch))
	{
		return FALSE;
	}

	// 1/30 chance
	if (!number(0, 29))
	{
		act("$n &+bsuddenly fills with an ogrish fury!", TRUE, ch, 0, 0, TO_ROOM);
		send_to_char("&+rThe fury of ogre kin starts to grow within you...&n\n", ch);
		berserk(ch, 6 * PULSE_VIOLENCE);
	}

	return FALSE;
}
