/* Area-owned special procedures. */

#include "core/prototypes.h"
#include "world/difficulty.h"
#include "core/structs.h"
#include "net/comm.h"
#include "world/db.h"
#include "cmd/interp.h"
#include "core/utility.h"
#include "core/utils.h"
#include "world/specs.prototypes.h"
#include "magic/spells.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "world/events.h"
#include "combat/damage.h"

extern P_room world;

int undead_howl(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	P_char tch, vict = NULL;

	if (cmd == CMD_SET_PERIODIC)
		return FALSE;

	if (cmd)
		return FALSE;

	if (!ch)
		return FALSE;

	// Do it 5 % of the time
	if (number(0, 99) > 10)
		return FALSE;

	if (ch->in_room == NOWHERE)
		return FALSE;
	if (!GET_OPPONENT(ch))
		return FALSE;

	act("$n&+L unleashes a hellish, low &+whowl&+L; everything becomes a shade darker as it pierces your spirit.&n",
	    FALSE, ch, 0, vict, TO_NOTVICT);

	for (vict = world[ch->in_room].people; vict; vict = tch)
	{
		tch = vict->next_in_room;

		if (ch->group && vict->group && (ch->group == vict->group))
			continue;
		if (IS_TRUSTED(ch) || (ch == vict))
			continue;
		else
		{
			if (!NewSaves(vict, SAVING_FEAR,
				      (int)BOUNDED(0, (GET_LEVEL(ch) - GET_LEVEL(vict)) / 2, 10)))
			{
				act("$n&+L's soulless &+whowl&n strips your body of it's very soul...&n",
				    FALSE, ch, 0, vict, TO_VICT);
				die(vict, ch);
				if (!number(0, 2))
					return TRUE;
			}
		}
	}
	return TRUE;
}
