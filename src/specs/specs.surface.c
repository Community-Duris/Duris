/* Special procedures for the surface area. */

#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utils.h"
#include "net/comm.h"
#include "cmd/interp.h"
#include "world/db.h"
#include "world/specs.prototypes.h"

extern P_char character_list;

#define GUARDIAN_HELPER_LIMIT 30

int goodie_guardian(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	P_char i;
	int count = 0;
	P_char guardian;

	if (cmd == CMD_SET_PERIODIC)
	{
		return TRUE;
	}
	if (!ch)
	{
		return FALSE;
	}

	if (cmd == CMD_DEATH)
	{
		debug("&+WGuardian death called.&n");
		act("&nThe guard moves to attack one final time, but instead chokes on his own &+rblood&n and falls to the ground, lifeless.\n&n",
		    FALSE, ch, 0, 0, TO_ROOM);

		return true;
	}

	if (cmd != 0)
	{
		return FALSE;
	}

	if (IS_FIGHTING(ch))
	{
		/*
		 * attempt to "summon" an elite tharn soldier...only possible if less than GUARDIAN_HELPER_LIMIT
		 * * in world
		 */
		for (i = character_list; i; i = i->next)
		{
			if ((IS_NPC(i)) && (GET_VNUM(i) == 446))
			{
				count++;
			}
		}
		if (count < GUARDIAN_HELPER_LIMIT)
		{
			if (number(1, 100) < 35)
			{
				guardian = read_mobile(446, VIRTUAL);
				if (!guardian)
				{
					logit(LOG_MOB,
					      "goodie_guardian: could not load helper mob 446");
					return FALSE;
				}
				act("$n &nyells 'To arms brothers! Help me destroy this threat to our &+glands&n!&n\r\n"
				    "&+WAn elite guard &ncharges into the fray, assisting his comrade...&n\r\n",
				    FALSE, ch, 0, guardian, TO_ROOM);
				char_to_room(guardian, ch->in_room, 0);
				return TRUE;
			}
		}
	}

	return FALSE;
}

#undef GUARDIAN_HELPER_LIMIT
