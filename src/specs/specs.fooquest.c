/* Fooquest special procedures. */

#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "world/db.h"
#include "world/handler.h"
#include "world/events.h"
#include "cmd/interp.h"
#include "core/utils.h"
#include "world/specs.prototypes.h"
#include "magic/spells.h"

extern P_char character_list;

int fooquest_boss(P_char ch, P_char pl, int cmd, char * /*arg*/)
{
	P_char i;
	P_char dragon;
	int count = 0;

	if (cmd == CMD_SET_PERIODIC)
	{
		return TRUE;
	}
	if (!ch)
	{
		return FALSE;
	}
	if (IS_FIGHTING(ch))
	{
		/*
		 * attempt to "summon" a silver dragon...only possible if less than BAHAMUT_HELP_LIMIT
		 * * in world
		 */
		for (i = character_list; i; i = i->next)
		{
			if ((IS_NPC(i)) && (GET_VNUM(i) == 65014))
			{
				count++;
			}
		}
		if (count < 5)
		{
			if (number(1, 100) < 50)
			{
				dragon = read_mobile(65014, VIRTUAL);
				if (!dragon)
				{
					logit(LOG_EXIT, "assert: error in bahamut() proc");
					return FALSE;
				}
				act("&+LThe air before you seems to rend and tear, revealing a black rift.&N\r\n"
				    "An &+MIllithid&N stumbles out of &+Lthe wormhole&N.",
				    FALSE, ch, 0, dragon, TO_ROOM);
				char_to_room(dragon, ch->in_room, 0);
				return TRUE;
			}
		}
	}
	if (pl)
	{
		if (cmd == CMD_FLEE || cmd == CMD_RETREAT)
		{
			act("$n &+Llooks at you and you feel your limbs numb, unable to carry you from the fight!&N",
			    TRUE, ch, 0, pl, TO_VICT);
			act("$N &+Lprevents $n&+L from running with a mental blast!&N", TRUE, pl, 0,
			    ch, TO_NOTVICT);
			return TRUE;
		}
	}

	return FALSE;
}

int fooquest_mob(P_char ch, P_char pl, int cmd, char *arg)
{
	P_char tempchar = NULL;
	P_obj item;

	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch || !IS_AWAKE(ch))
		return FALSE;

	/*
	 * if it's some command besides a periodic event call, return
	 */
	if (!pl)
		return FALSE;

	if (IS_TRUSTED(pl))
	{
		if (cmd != CMD_USE)
			return FALSE;

		if (!isname(arg, "transform"))
			return FALSE;

		/*
		 * load agthrodos
		 */
		tempchar = read_mobile(65013, VIRTUAL);

		if (!tempchar)
		{
			logit(LOG_EXIT, "assert: mob load failed in xexos()");
			return FALSE;
		}
		/*
		 * stick agthrodos in same room
		 */
		char_to_room(tempchar, ch->in_room, 0);

		/*
		 * transfer inventory to agthrodos
		 */
		if (IS_RACEWAR_EVIL(ch))
		{
			item = read_object(21, VIRTUAL);
		}
		else
		{
			item = read_object(22, VIRTUAL);
		}
		obj_to_char(item, tempchar);

		/*
		 * let the player know what's going on
		 */
		mobsay(ch, "You can DIE!");
		act("$n starts removing his disguise.", TRUE, ch, 0, 0, TO_ROOM);
		act("An &+MIllithid&N invades your mind with 'Now I have the artifact, I don't need you anymore.'",
		    TRUE, ch, 0, 0, TO_ROOM);

		/*
		 * remove xexos
		 */
		extract_char(ch);
		ch = NULL;
		return (TRUE);
	}
	else
	{
		return (FALSE);
	}
}

int newbie_quest(P_char ch, P_char pl, int cmd, char *arg)
{
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;
	if (!ch || !cmd || !arg || !pl)
		return FALSE;

	if ((cmd == CMD_ASK && GET_LEVEL(pl) > 36) || (IS_TRUSTED(pl) && cmd == CMD_ASK))
	{
		mobsay(ch,
		       "&+wI am a questmob for the godquest for levels 1-35 only. Please let them be&+w for this is one of the rare occasions that a quest designed for low levels is&+w run and this "
		       "quest will allow them to become more familiar with the mud to&+w become better players in the future.!");
		return TRUE;
	}
	return FALSE;
}

// Gellz Added 060316 GELLZ

/* THIS IS NEWBIE ZONE  STREAM OF LIFE*/

int dragonslayer(P_obj obj, P_char ch, int cmd, char *arg)
{
	int dam = cmd / 1000;
	P_char victim;

	if (cmd == CMD_SET_PERIODIC)
	{
		return FALSE;
	}

	if (!OBJ_WORN(obj) && !OBJ_CARRIED(obj))
	{
		return FALSE;
	}

	if (OBJ_WORN(obj))
	{
		ch = obj->loc.wearing;
	}
	else if (OBJ_CARRIED(obj))
	{
		ch = obj->loc.carrying;
	}
	else
	{
		return FALSE;
	}
	if (!IS_ALIVE(ch))
	{
		return FALSE;
	}

	if (GET_LEVEL(ch) > 36 && !IS_TRUSTED(ch))
	{
		act("&+GYour $q zaaaps your experienced hands&n!", TRUE, ch, obj, 0, TO_CHAR);
		act("&+G$n's $q zaaaps $s experienced hands&n!", TRUE, ch, obj, 0, TO_NOTVICT);
		act("&+GYour $q drops to the ground with a soft thud.", TRUE, ch, obj, 0, TO_CHAR);
		act("&+G$n's $q drops to the ground with a soft thud.", TRUE, ch, obj, 0,
		    TO_NOTVICT);
		obj_from_char(obj);
		obj_to_room(obj, ch->in_room);
		return FALSE;
	}

	if (!dam || !OBJ_WORN(obj))
	{
		return FALSE;
	}
	victim = legacy_proc_arg<P_char>(arg);
	// 1/20 chance.
	if (!IS_ALIVE(victim) || !IS_DRAGON(victim) || number(0, 19))
	{
		return FALSE;
	}

	act("&+GYour $q &+Wglows &+Rbright &+rred &+Gat the sight of a &+gdr&+Gag&+gon&N.", TRUE,
	    ch, obj, victim, TO_CHAR);
	act("&+G$n's $q &+Wglows &+Rbright &+rred &+Gat the sight of a &+gdr&+Gag&+gon&N.&N", TRUE,
	    ch, obj, victim, TO_NOTVICT);
	spell_burning_hands(40, ch, NULL, 0, victim, obj);
	spell_stone_skin(26, ch, 0, 0, ch, 0);
	spell_haste(26, ch, 0, 0, ch, 0);
	return TRUE;
}
