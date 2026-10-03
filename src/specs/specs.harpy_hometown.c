/*
 * Special procedures for the Harpy Hometown.
 */

#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "world/db.h"
#include "cmd/interp.h"
#include "core/utils.h"
#include "world/specs.prototypes.h"
#include "world/vnum.obj.h"
#include <stdio.h>
#include <string.h>

int harpy_evil(P_char ch, P_char pl, int cmd, char *arg)
{
	char obj_name[MAX_INPUT_LENGTH], *argument;
	P_obj obj;

	if (cmd == CMD_SET_PERIODIC)
		return FALSE;

	if (!IS_AWAKE(ch))
		return FALSE;

	if (cmd == CMD_GIVE)
	{
		argument = arg;
		argument = one_argument(argument, obj_name);

		if (GET_RACEWAR(pl) != RACEWAR_NEUTRAL)
		{
			mobsay(ch, "You have already picked a path.");
			return TRUE;
		}

		obj = get_obj_in_list_vis(pl, obj_name, pl->carrying);
		if (!obj)
		{
			send_to_char("You do not seem to have anything like that.\r\n", pl);
			return TRUE;
		}
		if (OBJ_VNUM(obj) == VOBJ_HARPY_CHOOSE_FEATHER)
		{
			act("$n gives $p to $N.", 1, pl, obj, ch, TO_NOTVICT);
			act("$n gives you $p.", 0, pl, obj, ch, TO_VICT);
			send_to_char("Ok.\r\n", pl);

			extract_obj(obj, TRUE); // Not an arti.

			GET_RACE(pl) = RACE_HARPY;
			GET_RACEWAR(pl) = RACEWAR_EVIL;
			GET_ALIGNMENT(pl) = -1000;

			mobsay(ch,
			       "You have chosen the path of the darkness.  Now go slay some innocents.");

			return TRUE;
		}
	}

	return FALSE;
}

int harpy_good(P_char ch, P_char pl, int cmd, char *arg)
{
	char obj_name[MAX_INPUT_LENGTH], *argument;
	P_obj obj;

	if (cmd == CMD_SET_PERIODIC)
		return FALSE;

	if (!IS_AWAKE(ch))
		return FALSE;

	if (cmd == CMD_GIVE)
	{
		argument = arg;
		argument = one_argument(argument, obj_name);

		if (GET_RACEWAR(pl) != RACEWAR_NEUTRAL)
		{
			mobsay(ch, "You have already picked a path.");
			return TRUE;
		}

		obj = get_obj_in_list_vis(pl, obj_name, pl->carrying);
		if (!obj)
		{
			send_to_char("You do not seem to have anything like that.\r\n", pl);
			return TRUE;
		}
		if (OBJ_VNUM(obj) == VOBJ_HARPY_CHOOSE_FEATHER)
		{
			act("$n gives $p to $N.", 1, pl, obj, ch, TO_NOTVICT);
			act("$n gives you $p.", 0, pl, obj, ch, TO_VICT);
			send_to_char("Ok.\r\n", pl);

			extract_obj(obj, TRUE); // Not an arti.

			GET_RACE(pl) = RACE_HARPY;
			GET_RACEWAR(pl) = RACEWAR_GOOD;
			GET_ALIGNMENT(pl) = 1000;

			mobsay(ch,
			       "Always protect the innocent and be a beacon for justice and good.");
			return TRUE;
		}
	}

	return FALSE;
}

int gargoyle_master(P_char ch, P_char pl, int cmd, char *arg)
{
	int corpse_num = 0;
	P_obj obj, next_obj;
	char buf[1024];

	memset(buf, 0, sizeof(buf));

	if (cmd == CMD_SET_PERIODIC)
		return FALSE;

	if (!IS_AWAKE(ch))
		return FALSE;

	if (cmd != -2)
	{
		if (cmd && pl)
		{
			if (pl == ch)
				return FALSE;

			if (cmd == CMD_LOOK)
			{
				if (!isname(arg, "gargoyle"))
					return FALSE;
				mobsay(ch, "Ask me if you want to be undead.");
			}
			if (arg && cmd == CMD_ASK)
			{
				if (!isname(arg, "gargoyle undead") &&
				    !isname(arg, "Gargoyle undead"))
					return FALSE;

				if (GET_RACEWAR(pl) != RACEWAR_NEUTRAL)
				{
					mobsay(ch, "You have already picked a path.");
					return TRUE;
				}

				for (obj = world[ch->in_room].contents; obj; obj = next_obj)
				{
					next_obj = obj->next_content;
					if ((obj->type == ITEM_CORPSE) &&
					    IS_SET(obj->value[1], NPC_CORPSE))
						if (strstr(obj->name, "harpy"))
							corpse_num++;
				}

				if (corpse_num < 2)
				{
					corpse_num = 2 - corpse_num;

					if (corpse_num == 1)
						snprintf(buf, sizeof(buf),
							 "You need %d more corpse.", corpse_num);
					else
						snprintf(buf, sizeof(buf),
							 "You need %d more corpses.", corpse_num);

					mobsay(ch, buf);
				}
				else
				{ /*

					 for (obj = world[ch->in_room].contents; obj; obj = next_obj) {
					 next_obj = obj->next_content;
					 if ((obj->type == ITEM_CORPSE) && IS_SET(obj->value[1], NPC_CORPSE))
					 if(strstr(obj->name, "harpy"))
					 extract_obj(obj, TRUE); // Not an arti, but may contain one and is 'in game.'
					 }

					 send_to_char("&+LThe monsterous gargoyle grins evilly, then with lightning speed plunges his clawed hands into your chest.\n", pl);

					 send_to_char("&+rYou feel yourself falling to the ground.\n", pl);
					 send_to_char("&+rYour soul leaves your body in the cold sleep of death...\n", pl);

					 send_to_char("&+LYou slowly open your eyes, gazing out at onto the world through the glaze of undeath.\n", pl);

					 send_to_char("&+WYou have been reborn!\n", pl);

					 act("&+rYou hear the death cry of $n.", TRUE, pl, 0, ch, TO_NOTVICT);
					 act("&+rSuddenly, $n staggers to $s feet.", TRUE, pl, 0, ch, TO_NOTVICT);
					 act("&+W$n has been reborn!", TRUE, pl, 0, ch, TO_NOTVICT);

					 GET_RACE(pl) = RACE_GARGOYLE;
					 GET_RACEWAR(pl) = RACEWAR_UNDEAD;
					 GET_ALIGNMENT(pl) = -1000;

					 if(GET_CLASS(pl, CLASS_BARD))
					 pl->player.m_class = CLASS_SPIPER;
					 //              GET_CLASS(pl) = CLASS_SPIPER; */
				}

				return TRUE;
			}
		}
	}
	return FALSE;
}

int harpy_gate(P_obj obj, P_char ch, int cmd, char * /*arg*/)
{
	if (!obj || !ch)
		return FALSE;

	if (IS_NPC(ch) || IS_TRUSTED(ch))
		return FALSE;

	if (cmd == CMD_SOUTH)
	{
		if (GET_RACEWAR(ch) == RACEWAR_NEUTRAL)
		{
			send_to_char(
				"&+yYou must choose your life path before leaving the settlement!\n",
				ch);
			return TRUE;
		}
	}

	return FALSE;
}
