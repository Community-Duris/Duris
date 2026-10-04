/* Kobold Settlement special procedures. */

#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "world/db.h"
#include "world/events.h"
#include "cmd/interp.h"
#include "core/utility.h"
#include "core/utils.h"
#include "world/specs.prototypes.h"
#include "world/vnum.obj.h"
#include "world/vnum.room.h"
#include "magic/spells.h"

extern P_char character_list;
extern P_room world;
extern const int top_of_world;

/*
 * This is for Vaprak's kobold area.  When the mob dies, it becomes object
 * * 1438, a pile of stones, and its items and cash are placed within the
 * * container.
 * * Damon Silver, aka Oghma 8/3/94
 * * -- updated by DTS 2/9/95
 */

int stone_crumble(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	P_obj pile, money, obj, next_obj;
	int pos, room;

	if (cmd == CMD_SET_PERIODIC)
		return FALSE;

	if (cmd != CMD_DEATH || !ch || IS_PC(ch))
	{
		return FALSE;
	}

	// Where to put pile.
	room = (ch->in_room >= 0 && ch->in_room <= top_of_world) ? ch->in_room : NOWHERE;
	if (room == NOWHERE)
	{
		room = real_room(ch->specials.was_in_room);
	}
	// If we couldn't find a spot, just let the regular death code handle things.
	if (room == NOWHERE)
	{
		return FALSE;
	}

	if (!(pile = read_object(VOBJ_KOBOLD_DEATH_STONEPILE, VIRTUAL)))
	{
		logit(LOG_OBJ, "stone_crumble: Could not load stone pile.");
		return FALSE;
	}

	if (pile->type != ITEM_CONTAINER)
	{
		logit(LOG_OBJ, "stone_crumble: Object %d not type ITEM_CONTAINER!! Aborted.",
		      VOBJ_KOBOLD_DEATH_STONEPILE);
		return FALSE;
	}

	// Transfer inventory to pile
	for (obj = ch->carrying; obj; obj = next_obj)
	{
		next_obj = obj->next_content;
		obj_from_char(obj);
		obj_to_obj(obj, pile);
	}

	// Transfer equipment to pile
	for (pos = 0; pos < MAX_WEAR; pos++)
	{
		if (ch->equipment[pos])
		{
			obj_to_obj(unequip_char(ch, pos), pile);
		}
	}

	// Transfer money
	if (GET_MONEY(ch) > 0)
	{
		money = create_money(GET_COPPER(ch), GET_SILVER(ch), GET_GOLD(ch),
				     GET_PLATINUM(ch));
		obj_to_obj(money, pile);
	}

	obj_to_room(pile, room);

	// Clue players in
	act("$n stops fighting, and begins to shake.\r\n"
	    "$n silently crumbles into $p... a cloud of dust rises.",
	    TRUE, ch, pile, 0, TO_ROOM);
	return TRUE;
}

#define IMP_LIMIT 5

int kobold_priest(P_char ch, P_char pl, int cmd, char * /*arg*/)
{
	P_char i;
	P_char imp;
	int count = 0 /*
	                           * , flag = 0
	                           */
		;

	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
	{
		ch->only.npc->spec[0] = 0;
		return (TRUE);
	}
	if (!ch)
	{
		return (FALSE);
	}
	if (world[ch->in_room].number == 1481)
	{
		if (pl && ((cmd == CMD_NORTH) || (cmd == CMD_SOUTH) || (cmd == CMD_EAST)))
		{
			act("$N makes a strange gesture!\r\n"
			    "Suddenly, the way is blocked by an invisible wall of force!",
			    FALSE, pl, 0, ch, TO_CHAR);
			act("$N makes a strange gesture!\r\n"
			    "$n tries to leave the room but runs into an invisible barrier!",
			    FALSE, pl, 0, ch, TO_NOTVICT);
			return (TRUE);
		}
		else if (cmd == CMD_WEST)
		{
			if (pl)
			{
				act("$N cackles with glee as $e sees you foolishly\r\n"
				    "toss yourself off the dais and into the sacrificial pit!\r\n"
				    "You find yourself tumbling down...\r\n"
				    "                               ...down...\r\n"
				    "                                      ...down...\r\n"
				    "                                             ...into the pit.",
				    FALSE, pl, 0, ch, TO_CHAR);
				act("$N cackles with glee as $e watches $n toss $mself into the\r\n"
				    "sacrificial pit to the west!",
				    FALSE, pl, 0, ch, TO_NOTVICT);
				char_from_room(pl);
				char_to_room(pl, real_room(1484), 0);
				return (TRUE);
			}
			else
			{
				return (FALSE);
			}
		}
	}
	else
	{
		if (pl &&
		    ((cmd == CMD_NORTH) || (cmd == CMD_EAST) || (cmd == CMD_SOUTH) ||
		     (cmd == CMD_UP) || (cmd == CMD_DOWN)) &&
		    (!IS_TRUSTED(pl)) && (number(1, 100) > 20))
		{
			if (pl)
			{
				if (EXIT(pl, cmd_to_exitnumb(cmd)))
				{
					act("$N makes a strange gesture!\r\n"
					    "Suddenly, the way is blocked by an invisible wall of force!",
					    FALSE, pl, 0, ch, TO_CHAR);
					act("$N makes a strange gesture!\r\n"
					    "$n tries to leave the room but runs into an invisible barrier!",
					    FALSE, pl, 0, ch, TO_NOTVICT);
					return (TRUE);
				}
				else
				{
					return (FALSE);
				}
			}
			else
			{
				return (FALSE);
			}
		}
		else
		{
			return (FALSE);
		}
	}

	/*
	 * if it's some command besides a periodic event call, return
	 */
	if (cmd != 0)
	{
		return (FALSE);
	}
	if (ch->only.npc->spec[0] > 0)
	{
		ch->only.npc->spec[0]--;
		return (FALSE);
	}
	else
	{
		ch->only.npc->spec[0] = 4;
		if (IS_FIGHTING(ch))
		{
			/*
			 * attempt to "summon" purple imp...only possible if less than IMP_LIMIT
			 * * in world
			 */
			for (i = character_list; i; i = i->next)
			{
				if ((IS_NPC(i)) && (GET_VNUM(i) == 1440))
				{
					count++;
				}
			}
			if (count < IMP_LIMIT)
			{
				if (number(1, 100) < 90)
				{
					imp = read_mobile(1440, VIRTUAL);
					if (!imp)
					{
						logit(LOG_EXIT,
						      "assert: error in kobold_priest() proc");
						return FALSE;
					}
					act("&+M$n incants a powerful spell of summoning.\r\n"
					    "&+M$N arrives from the depths of &+RHell&+M to aid its master!\r\n",
					    FALSE, ch, 0, imp, TO_ROOM);
					char_to_room(imp, ch->in_room, 0);
					return (TRUE);
				}
				else
				{
					do_action(ch, 0, CMD_CURSE);
				}
			}
		}
	}

	return (FALSE);
}

#undef IMP_LIMIT

int stone_golem(P_char ch, P_char pl, int cmd, char * /*arg*/)
{
	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
	{
		return (FALSE);
	}
	if (!ch || !pl)
	{
		return (FALSE);
	}
	if (world[ch->in_room].number == 1482)
	{
		if (cmd == CMD_WEST && !IS_TRUSTED(pl) && (number(1, 100) > 20))
		{
			act("You try to leave the room but are shoved back by $N!", FALSE, pl, 0,
			    ch, TO_CHAR);
			act("$n tries to leave the room but is shoved back by $N!", FALSE, pl, 0,
			    ch, TO_NOTVICT);
			return (TRUE);
		}
	}
	return (FALSE);
}

int tako_demon(P_char ch, P_char pl, int cmd, char * /*arg*/)
{
	P_char k, victim = NULL;

	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
	{
		return (TRUE);
	}
	if (!ch)
	{
		return (FALSE);
	}
	if (world[ch->in_room].number == 1484)
	{
		if (pl && cmd == CMD_UP && !IS_TRUSTED(pl))
		{
			if (number(1, 100) > 50)
			{ /*
			   * caught!
			   */
				act("You try to leave but are grappled backwards by a snaky tentacle!",
				    FALSE, pl, 0, ch, TO_CHAR);
				act("$n tries to leave but is grappled backwards by a snaky tentacle!",
				    FALSE, pl, 0, ch, TO_NOTVICT);
				return (TRUE);
			}
		}
		else if (!cmd && !IS_FIGHTING(ch))
		{
			for (k = world[real_room(1483)].people; k; k = k->next_in_room)
			{
				if (IS_PC(k) && CAN_SEE(ch, k) && !IS_TRUSTED(k) && !IS_FIGHTING(k))
				{
					victim = k;
					break;
				}
			}
			if (victim && (world[victim->in_room].number == 1483))
			{
				if (number(0, 100) < 30)
				{
					act("You are suddenly &+BYANKED&n downward by a snaky tentacle!",
					    FALSE, victim, 0, 0, TO_CHAR);
					act("$n is suddenly &+BYANKED&n downwards by a snaky tentacle!",
					    TRUE, victim, 0, 0, TO_ROOM);
					char_from_room(victim);
					char_to_room(victim, real_room(1484), 0);
					act("$N is suddenly yanked here from above by $n!", TRUE,
					    ch, 0, victim, TO_NOTVICT);
					MobStartFight(ch, victim);
					return (TRUE);
				}
			}
		}
	}
	return (FALSE);
}

int chicken(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
	{
		return (TRUE);
	}
	if (!ch || cmd)
	{
		return (FALSE);
	}
	if (IS_AWAKE(ch) || !IS_FIGHTING(ch))
	{
		switch (number(1, 25))
		{
		case 1:
			act("$n clucks contently on $s nest.", TRUE, ch, 0, 0, TO_ROOM);
			break;
		case 2:
			act("$n becomes frightened and looks all around, sensing danger nearby.",
			    TRUE, ch, 0, 0, TO_ROOM);
			break;
		default:
			break;
		}
	}
	else if (IS_FIGHTING(ch))
	{
		switch (number(1, 4))
		{
		case 1:
			act("'SQUAAAAAAWWWK' screams $n as $e tries to run away from you!", TRUE,
			    ch, 0, 0, TO_ROOM);
			break;
		default:
			break;
		}
	}
	return (FALSE);
}
