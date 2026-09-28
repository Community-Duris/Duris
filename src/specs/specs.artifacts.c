/* Shared wearable artifact procedures. */

#include <string.h>
#include <time.h>

#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utility.h"
#include "core/utils.h"
#include "net/comm.h"
#include "cmd/interp.h"
#include "world/db.h"
#include "world/specs.prototypes.h"
#include "magic/spells.h"

extern P_room world;
extern bool has_skin_spell(P_char ch);

/* I have horrbly twisted this function to be called only from
   equip_char, if passed cmd == -1 it activates stone -Zod*/
// This works just fine but I don't see any reference to -1 in the code anywhere?
int artifact_stone(P_obj obj, P_char ch, int cmd, char * /*argument*/)
{
	int curr_time;

	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!OBJ_WORN(obj) || cmd != CMD_PERIODIC)
	{
		return FALSE;
	}

	if (!ch)
	{
		if (obj->loc.wearing)
		{
			ch = obj->loc.wearing;
		}
		else
		{
			return FALSE;
		}
	}
	else
	{
		return FALSE;
	}

	// validate ch is a valid character (protect against use-after-free)
	if (!char_in_list(ch))
	{
		// stale pointer - clear the object's location
		logit(LOG_DEBUG, "artifact_stone: stale loc.wearing pointer for obj %d, clearing",
		      OBJ_VNUM(obj));
		obj->loc_p = LOC_NOWHERE;
		obj->loc.wearing = NULL;
		return FALSE;
	}

	curr_time = time(NULL);

	if (!has_skin_spell(ch) &&
	    obj->timer[0] + (int)get_property("timer.stoneskin.generic", 60) <= curr_time)
	{
		spell_stone_skin(30, ch, 0, SPELL_TYPE_POTION, ch, 0);
		obj->timer[0] = curr_time;
	}

	return FALSE;
}

int artifact_hide(P_obj obj, P_char ch, int cmd, char *argument)
{
	char *arg;
	int curr_time, room;

	if (cmd == CMD_SET_PERIODIC) /*
	                   Events have priority
	                 */
		return FALSE;

	if (!ch || !obj) /*
	                    If the player ain't here, why are we?
	                  */
		return FALSE;

	if (!OBJ_WORN(obj)) /*
	                       Most things don't work in a sack...
	                     */
		return FALSE;

	/*
	   Any powers activated by keywords? Right here, bud.
	 */

	if (argument && (cmd == CMD_SAY))
	{
		arg = argument;

		while (*arg == ' ')
			arg++;

		if (!strcmp(arg, "hide"))
		{
			if (!say(ch, arg))
				return TRUE;

			curr_time = time(NULL);

			if (obj->timer[0] + 60 <= curr_time)
			{
				act("Your $q hums briefly.", FALSE, ch, obj, obj, TO_CHAR);

				act("$n's $q hums briefly.", TRUE, ch, obj, NULL, TO_ROOM);

				if (!(room = ch->in_room))
					return true;

				if (IS_WATER_ROOM(room) || world[room].sector_type == SECT_OCEAN)
				{
					send_to_char(
						"It's very &+Bwet&n here; too &+Bwet&n to hide behind anything.\r\n",
						ch);
					return true;
				}
				else
				{
					SET_BIT(ch->specials.affected_by, AFF_HIDE);
				}

				obj->timer[0] = curr_time;
			}
			return TRUE;
		}
	}
	return FALSE;
}

int artifact_invisible(P_obj obj, P_char ch, int cmd, char *argument)
{
	char *arg;
	int curr_time;

	if (cmd == CMD_SET_PERIODIC) /*
	                   Events have priority
	                 */
		return FALSE;

	if (!ch || !obj) /*
	                    If the player ain't here, why are we?
	                  */
		return FALSE;

	if (!OBJ_WORN(obj)) /*
	                       Most things don't work in a sack...
	                     */
		return FALSE;

	/*
	   Any powers activated by keywords? Right here, bud.
	 */

	if (argument && (cmd == CMD_SAY))
	{
		arg = argument;

		while (*arg == ' ')
			arg++;

		if (!strcmp(arg, "invisible"))
		{
			if (!say(ch, arg))
				return TRUE;

			curr_time = time(NULL);

			if (obj->timer[0] + 60 <= curr_time)
			{
				act("Your $q hums briefly.", FALSE, ch, obj, obj, TO_CHAR);

				act("$n's $q hums briefly.", TRUE, ch, obj, NULL, TO_ROOM);
				spell_invisibility(55, ch, 0, SPELL_TYPE_SPELL, ch, 0);

				obj->timer[0] = curr_time;
			}

			return TRUE;
		}
	}

	return FALSE;
}

int splinter(P_obj obj, P_char ch, int cmd, char *arg)
{
	int curr_time;

	if (cmd == CMD_SET_PERIODIC)
	{
		return FALSE;
	}

	if (!IS_ALIVE(ch) || !OBJ_WORN_BY(obj, ch))
	{
		return FALSE;
	}

	if (arg && (cmd == CMD_RUB))
	{
		if (isname(arg, "splinter"))
		{
			curr_time = time(NULL);
			// Every 15 minutes.
			if (obj->timer[0] + (60 * 15) <= curr_time)
			{
				act("Your $q hums briefly.", FALSE, ch, obj, obj, TO_CHAR);
				act("$n's $q hums briefly.", TRUE, ch, obj, NULL, TO_ROOM);
				spell_cure_critic(45, ch, 0, SPELL_TYPE_SPELL, ch, 0);
				spell_cure_critic(45, ch, 0, SPELL_TYPE_SPELL, ch, 0);

				obj->timer[0] = curr_time;
				return TRUE;
			}
		}
	}
	return FALSE;
}
