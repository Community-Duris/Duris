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

int ghore_paradise(P_char ch, P_char pl, int cmd, char *arg)
{
	int beforecash, aftercash;

	/*
	 * check for periodic event call
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch))
		return FALSE;

	if (ch->in_room == real_room(11666))
	{
		if (cmd == CMD_UP)
		{
			if (IS_PC(pl))
			{
				act("$n stops you.", FALSE, ch, 0, pl, TO_VICT);
				act("$n stops $N.", TRUE, ch, 0, pl, TO_NOTVICT);
				mobsay(ch, "Paradise is off limits to freeloaders!");
				mobsay(ch, "The price to enter is 10 Ghorean platinum!");
				return TRUE;
			}
			else
			{
				act("$N gives $n some money.", TRUE, ch, 0, pl, TO_NOTVICT);

				mobsay(ch, "Welcome to the Paradise.");
				act("$n steps aside to let $N up the passage", TRUE, ch, 0, pl,
				    TO_NOTVICT);
				char_from_room(pl);
				char_to_room(pl, real_room(11667), 0);
				act("$n arrives from the passage below.", TRUE, pl, 0, 0, TO_ROOM);
				return TRUE;
			}
		}
		else if (cmd == CMD_GIVE)
		{
			beforecash = GET_MONEY(ch);
			do_give(pl, arg, cmd);
			aftercash = GET_MONEY(ch);
			if ((aftercash - beforecash) < 10000)
			{
				mobsay(ch, "The cost is 10 Ghorean platinum, worm!  Try again.");
				do_action(ch, 0, CMD_SPIT);
				return TRUE;
			}
			else
			{
				mobsay(ch, "Welcome to Paradise.");
				act("$n steps aside to let you continue up the passage.", FALSE, ch,
				    0, pl, TO_VICT);
				act("$n steps aside to let $N further up the passage.", TRUE, ch, 0,
				    pl, TO_NOTVICT);
				char_from_room(pl);
				char_to_room(pl, real_room(11667), 0);
				act("$n arrives from the passage below.", TRUE, pl, 0, 0, TO_ROOM);
				return TRUE;
			}
		}
		else if (cmd)
		{
			return FALSE;
		}
		else
		{
			switch (number(0, 80))
			{
			case 0:
				mobsay(ch, "Welcome to Paradise!");
				do_action(ch, 0, CMD_CACKLE);
				return TRUE;
			case 1:
				do_action(ch, 0, CMD_SING);
				mobsay(ch, "Gimme lots of money, or I'm gonna eat you....");
				return TRUE;
			case 2:
				mobsay(ch, "I will not let you pass unless you pay the tribute!");
				return TRUE;
			case 3:
				mobsay(ch, "Paradise lies ahead...");
				do_action(ch, 0, CMD_SMILE);
				return TRUE;
			case 4:
				act("$n holds out $s hand for some coins.", TRUE, ch, 0, 0,
				    TO_ROOM);
				return TRUE;
			case 5:
				mobsay(ch, "Pay now, or die now; 'tis a simple choice, is it not?");
				do_action(ch, 0, CMD_CACKLE);
				[[fallthrough]];
			default:
				return FALSE;
			}
		}
	}
	else
	{
		return FALSE;
	}
}
