/* Graves special procedures. */

#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "world/db.h"
#include "cmd/interp.h"
#include "core/utility.h"
#include "core/utils.h"
#include "world/map.h"
#include "world/specs.prototypes.h"

extern P_room world;
extern struct zone_data *zone;

int random_tomb(P_obj /*obj*/, P_char ch, int cmd, char * /*arg*/)
{
	P_obj tmp_object = NULL;
	bool have_one = FALSE;

	/*
	   check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return FALSE;

	if (!ch || ((cmd != CMD_DIG) && (cmd != CMD_SEARCH)))
		return FALSE;

	if (IS_NPC(ch))
		return TRUE;

	if (world[ch->in_room].sector_type == 0)
	{
		act("You already found a secret zone here, but sure!", FALSE, ch, 0, 0, TO_CHAR);
		return FALSE;
	}

	if (cmd == CMD_DIG)
	{
		if (ch->equipment[HOLD])
		{
			tmp_object = ch->equipment[HOLD];
			if (isname("shovel", tmp_object->name) || isname("hoe", tmp_object->name) ||
			    isname("pick", tmp_object->name))
				have_one = TRUE;
		}

		if (!have_one)
		{
			send_to_char("Using what? Your fingers?\n", ch);
			return FALSE;
		}

		if (number(0, 7))
		{
			act("You dig up absolutely nothing!", FALSE, ch, 0, 0, TO_CHAR);
			act("$n digs up absolutely nothing!", FALSE, ch, 0, 0, TO_ROOM);
		}
		else
		{
			act("You dig into a pieces of bone.", FALSE, ch, 0, 0, TO_CHAR);
			act("$n digs some bones.", FALSE, ch, 0, 0, TO_ROOM);
			do_search(ch, NULL, 0);
			world[ch->in_room].sector_type = 0;
		}
	}
	else if (cmd == CMD_SEARCH)
	{
		send_to_char("You don't find anything you didn't see before.\n", ch);
	}

	CharWait(ch, 3);

	return TRUE;
}

int random_glass(P_obj /*obj*/, P_char ch, int cmd, char * /*arg*/)
{
	P_obj tmp_object = NULL;
	bool have_one = FALSE;

	/*
	   check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return FALSE;

	if (!ch || ((cmd != CMD_DIG) && (cmd != CMD_SEARCH)))
		return FALSE;

	if (IS_NPC(ch))
		return TRUE;

	if (world[ch->in_room].sector_type == 0)
	{
		act("You already found a secret zone here, but sure!", FALSE, ch, 0, 0, TO_CHAR);
		return FALSE;
	}

	if (cmd == CMD_DIG)
	{
		if (ch->equipment[HOLD])
		{
			tmp_object = ch->equipment[HOLD];
			if (isname("shovel", tmp_object->name) || isname("hoe", tmp_object->name) ||
			    isname("pick", tmp_object->name))
				have_one = TRUE;
		}

		if (!have_one)
		{
			send_to_char("Using what? Your fingers?\n", ch);
			return FALSE;
		}

		if (number(0, 7))
		{
			act("You dig up absolutely nothing!", FALSE, ch, 0, 0, TO_CHAR);
			act("$n digs up absolutely nothing!", FALSE, ch, 0, 0, TO_ROOM);
		}
		else
		{
			act("You dig into a pieces of glass.", FALSE, ch, 0, 0, TO_CHAR);
			act("$n digs some bones.", FALSE, ch, 0, 0, TO_ROOM);
			do_search(ch, NULL, 0);
			world[ch->in_room].sector_type = 0;
		}
	}
	else if (cmd == CMD_SEARCH)
	{
		send_to_char("You don't find anything you didn't see before.\n", ch);
	}

	CharWait(ch, 3);

	return TRUE;
}

int random_slab(P_obj /*obj*/, P_char ch, int cmd, char * /*arg*/)
{
	P_obj tmp_object = NULL;
	bool have_one = FALSE;

	/*
	   check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return FALSE;

	if (!ch || ((cmd != CMD_DIG) && (cmd != CMD_SEARCH)))
		return FALSE;

	if (IS_NPC(ch))
		return TRUE;

	if (world[ch->in_room].sector_type == 0)
	{
		act("You already found a secret zone here, but sure!", FALSE, ch, 0, 0, TO_CHAR);
		return FALSE;
	}

	if (cmd == CMD_DIG)
	{
		if (ch->equipment[HOLD])
		{
			tmp_object = ch->equipment[HOLD];
			if (isname("shovel", tmp_object->name) || isname("hoe", tmp_object->name) ||
			    isname("pick", tmp_object->name))
				have_one = TRUE;
		}

		if (!have_one)
		{
			send_to_char("Using what? Your fingers?\n", ch);
			return FALSE;
		}

		if (number(0, 7))
		{
			act("You dig up absolutely nothing!", FALSE, ch, 0, 0, TO_CHAR);
			act("$n digs up absolutely nothing!", FALSE, ch, 0, 0, TO_ROOM);
		}
		else
		{
			do_search(ch, NULL, 0);
			world[ch->in_room].sector_type = 0;
		}
	}
	else if (cmd == CMD_SEARCH)
	{
		send_to_char("You don't find anything you didn't see before.\n", ch);
	}

	CharWait(ch, 3);

	return TRUE;
}
