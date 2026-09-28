/* Area-owned special procedures. */

#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "world/db.h"
#include "cmd/interp.h"
#include "core/utility.h"
#include "core/utils.h"
#include "world/specs.prototypes.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "world/events.h"
#include "economy/currency_transaction.h"

int jindo_ticket_master(P_char ch, P_char pl, int cmd, char *arg)
{
	if (cmd == CMD_SET_PERIODIC)
	{
		return FALSE;
	}

	// MobCombat() will call this function with pl, cmd, and arg set to null
	if (!pl)
	{
		return FALSE;
	}

	if (cmd == CMD_WEST)
	{
		if (IS_TRUSTED(pl))
		{
			act("$n bows to your &+WGodliness&n.", TRUE, pl, 0, ch, TO_VICT);
			return FALSE;
		}
		act("$n says 'If you would like to enter the carnival, please buy a ticket.'", TRUE,
		    ch, 0, pl, TO_VICT);
		act("$n says 'Tickets are 5 gold pieces each.'", TRUE, ch, 0, pl, TO_VICT);
		act("$n blocks your passage.", TRUE, pl, 0, ch, TO_VICT);
		act("$N says to $n 'If you would like to enter the carnival, please buy a ticket.'",
		    TRUE, pl, 0, ch, TO_NOTVICT);
		act("$N blocks $m passage.", TRUE, pl, 0, ch, TO_NOTVICT);
		return TRUE;
	}

	if ((cmd == CMD_BUY) && *arg)
	{
		if (isname(arg, "ticket"))
		{
			// Trying to cheese this huh?  Ticketmaster no like!
			if (IS_PC_PET(ch))
			{
				act("$n says, 'I don't do &+Ycheese&n!'", TRUE, ch, 0, pl, TO_VICT);
				act("$n grabs your wrist and flips you to the ground pinning you!",
				    TRUE, ch, 0, pl, TO_VICT);
				SET_POS(pl, POS_PRONE + GET_STAT(pl));
				act("$n hits your abdomen leaving you breathless!", TRUE, ch, 0, pl,
				    TO_VICT);
				CharWait(pl, PULSE_VIOLENCE * 3);
				return TRUE;
			}

			if (transact(pl, NULL, ch, 500))
			{
				act("$n says, 'Thank you for your purchase, enjoy the carnival.'",
				    TRUE, ch, 0, pl, TO_VICT);
				act("$n hands $N a ticket and thanks $S for $M patronage.", TRUE,
				    ch, 0, pl, TO_NOTVICT);
				char_from_room(pl);
				char_to_room(pl, real_room(82062), 0);
				act("$n is whisked away into the delights and the joys of the carnival.",
				    TRUE, pl, 0, ch, TO_NOTVICT);

				return TRUE;
			}
		}
	}
	return FALSE;
}
