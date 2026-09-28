/*
 ***************************************************************************
 *  File: specs.exit_barriers.c                             Part of Duris  *
 *  Usage: shared exit-barrier procedures                                  *
 ***************************************************************************
 */

#include <stdio.h>
#include <string.h>

#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utility.h"
#include "core/utils.h"
#include "net/comm.h"
#include "cmd/interp.h"
#include "world/db.h"
#include "combat/death_messages.h"
#include "world/specs.prototypes.h"

extern P_room world;
extern struct quest_data quest_index[];

int block_dir(P_char ch, P_char pl, int cmd, char * /*arg*/)
{
	if (cmd == CMD_SET_PERIODIC || cmd == CMD_MOB_MUNDANE)
		return FALSE;

	if (!ch || !pl)
		return FALSE;

	bool allowed = TRUE;

	if (cmd == CMD_NORTH && isname("_block_north_", GET_NAME(ch)))
	{
		allowed = FALSE;
	}
	else if (cmd == CMD_EAST && isname("_block_east_", GET_NAME(ch)))
	{
		allowed = FALSE;
	}
	else if (cmd == CMD_SOUTH && isname("_block_south_", GET_NAME(ch)))
	{
		allowed = FALSE;
	}
	else if (cmd == CMD_WEST && isname("_block_west_", GET_NAME(ch)))
	{
		allowed = FALSE;
	}
	else if (cmd == CMD_UP && isname("_block_up_", GET_NAME(ch)))
	{
		allowed = FALSE;
	}
	else if (cmd == CMD_DOWN && isname("_block_down_", GET_NAME(ch)))
	{
		allowed = FALSE;
	}
	else if (cmd == CMD_NORTHWEST && isname("_block_northwest_", GET_NAME(ch)))
	{
		allowed = FALSE;
	}
	else if (cmd == CMD_SOUTHWEST && isname("_block_southwest_", GET_NAME(ch)))
	{
		allowed = FALSE;
	}
	else if (cmd == CMD_NORTHEAST && isname("_block_northeast_", GET_NAME(ch)))
	{
		allowed = FALSE;
	}
	else if (cmd == CMD_SOUTHEAST && isname("_block_southeast_", GET_NAME(ch)))
	{
		allowed = FALSE;
	}

	if (!allowed)
	{
		if (IS_TRUSTED(pl))
		{
			act("$n bows in deference as you pass by.", FALSE, ch, 0, pl, TO_VICT);
			return FALSE;
		}
		else
		{
			act("$n blocks you.", FALSE, ch, 0, pl, TO_VICT);
			act("$n blocks $N.", FALSE, ch, 0, pl, TO_NOTVICT);
			return TRUE;
		}
	}

	return FALSE;
}

int unblock_on_death(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	int rroom, dir, qi;
	struct quest_msg_data *qdata;
	char direction[32];

	// Includes periodic.
	if (cmd != CMD_DEATH)
	{
		return FALSE;
	}

	if ((qi = find_quester_id(GET_RNUM(ch))) < 0)
	{
		return FALSE;
	}

	for (qdata = quest_index[qi].quest_message; qdata; qdata = qdata->next)
	{
		if (sscanf(qdata->key_words, QC_UNBLOCK " %d %s", &rroom, direction) == 2)
		{
			break;
		}
	}
	if (!qdata)
	{
		return FALSE;
	}

	if ((rroom = real_room(rroom)) == NOWHERE)
	{
		return FALSE;
	}
	if ((dir = dir_from_keyword(direction)) == -1)
	{
		return FALSE;
	}
	if (world[rroom].dir_option[dir] == NULL)
	{
		return FALSE;
	}

	REMOVE_BIT(world[rroom].dir_option[dir]->exit_info, EX_BLOCKED);

	act("$n is dead! &+RR.I.P.&n", TRUE, ch, 0, 0, TO_ROOM);
	act("&-L&+rYou feel yourself falling to the ground.&n", FALSE, ch, 0, 0, TO_CHAR);
	act("&-L&+rYour soul leaves your body in the cold sleep of death...&n", FALSE, ch, 0, 0,
	    TO_CHAR);

	if (!CAN_SPEAK(ch))
	{
		death_rattle(ch);
	}
	else
	{
		death_cry(ch);
	}

	return FALSE;
}
