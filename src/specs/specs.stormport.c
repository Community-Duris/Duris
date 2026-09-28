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
#include "world/epic.h"
#include "economy/economic_gameplay_authority.h"
#include "economy/nexus_stones.h"
#include "economy/tradeskill.h"
#include "sql/sql.h"

int clear_epic_task_spec(P_char npc, P_char ch, int cmd, char *arg)
{
	char askFor[MAX_STRING_LENGTH];
	char buffer[MAX_STRING_LENGTH];
	P_obj nexus;

	if (cmd == CMD_SET_PERIODIC || cmd == CMD_PERIODIC)
	{
		return FALSE;
	}

	if (!ch || !arg || !npc)
	{
		return FALSE;
	}

	arg = one_argument(arg, askFor);
	arg = one_argument(arg, askFor);

	if (cmd == CMD_ASK && !str_cmp(askFor, "task"))
	{
		if (!CAN_SPEAK(npc))
		{
			return FALSE;
		}

		mobsay(npc,
		       "Ah, another soul who feels unable to meet the harsh demands of the Gods.");
		mobsay(npc, "My child, I can help.");
		mobsay(npc,
		       "I could prepare a &+Lprayer&n for you, and perhaps the Gods would listen.");
		return TRUE;
	}
	else if (cmd == CMD_ASK && !str_cmp(askFor, "prayer"))
	{
		if (!CAN_SPEAK(npc))
		{
			return FALSE;
		}

		if (!CAN_SEE(npc, ch))
		{
			mobsay(npc, "How can I help you if I cannot see you?");
			return TRUE;
		}
		struct affected_type *afp;
		afp = get_epic_task(ch);
		if (!afp)
		{
			mobsay(npc, "Whaat? The Gods haven't given you a task! Begone!");
			return TRUE;
		}
		if (economic_gameplay_authority::active())
		{
			send_to_char(
				"Epic task clearing is unavailable while active accounting is enabled.\r\n",
				ch);
			return TRUE;
		}

		// Count money
		int price = get_property("mobspecs.epicTaskClear.price", 10000000);
		int task = abs(afp->modifier);
		if (task == SPILL_BLOOD)
		{
			mobsay(npc,
			       "The gods are upset with your prayer to clear your &+Rspilling blood&n task.");
			price *= 2;
		}
		// a nexus stone
		else if ((task > SPILL_BLOOD) && (task <= SPILL_BLOOD + NEXUS_STONE_LAST))
		{
			nexus = get_nexus_stone(task - SPILL_BLOOD);
			if (!nexus)
			{
				debug("clear_epic_task_spec(): error, can't find nexus");
				send_to_char("Can't clear a bugged task, please ask an imm.\r\n",
					     ch);
				return TRUE;
			}
			if ((IS_RACEWAR_GOOD(ch) && STONE_ALIGN(nexus) < STONE_ALIGN_GOOD) ||
			    (IS_RACEWAR_EVIL(ch) && STONE_ALIGN(nexus) > STONE_ALIGN_EVIL))
			{
				price = (price * 3) / 2;
			}
		}

		if (GET_MONEY(ch) < price)
		{
			mobsay(npc, "I can't pray a proper prayer on an empty stomach!");
			snprintf(buffer, MAX_STRING_LENGTH, "You need at least %s&n more!",
				 coin_stringv(price - GET_MONEY(ch)));
			mobsay(npc, buffer);
			return TRUE;
		}

		act("$n begins to chant in a deep voice, starting quietly and then raising $s voice \n"
		    "slowly until the entire room is shaking. Your conscience -- and your wallet -- suddenly \n"
		    "feel much lighter!",
		    FALSE, npc, 0, ch, TO_VICT);
		act("$n begins to chant in a deep voice, starting quietly and then raising $s voice \n"
		    "slowly until the entire room is shaking. $N suddenly looks like a huge weight was taken \n"
		    "off $S shoulders.",
		    FALSE, npc, 0, ch, TO_NOTVICT);
		affect_remove(ch, afp);
		SUB_MONEY(ch, price, 0);

		wizlog(AVATAR, "%s cleared his epic task at %s", GET_NAME(ch), GET_NAME(npc));
		logit(LOG_PLAYER, "%s cleared his epic task at %s", GET_NAME(ch), GET_NAME(npc));
		sql_log(ch, PLAYERLOG, "Cleared epic task at %s", GET_NAME(npc));
		return TRUE;
	}
	return FALSE;
}
