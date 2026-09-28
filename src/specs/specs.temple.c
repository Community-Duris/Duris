/* Temple special procedures. */

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

int temple_illyn(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	/*
	 * check for periodic event call
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
		return FALSE;

	switch (number(0, 30))
	{
	case 0:
		mobsay(ch, "Fire is the key!");
		do_action(ch, 0, CMD_GRUMBLE);
		return TRUE;
	case 1:
		do_action(ch, 0, CMD_GRUMBLE);
		return TRUE;
	}
	return FALSE;
}
