/* Lava Tubes special procedures. */

#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "world/db.h"
#include "world/events.h"
#include "cmd/interp.h"
#include "core/utility.h"
#include "core/utils.h"
#include "combat/damage.h"
#include "combat/justice.h"
#include "world/map.h"
#include "world/specs.prototypes.h"
#include "magic/spells.h"

extern P_room world;

/*
 * The following pair of procs (xexos and agthrodos) are for the "same"
 * * monster, Xexos.  If in battle, Xexos uses the power of "alchemy" to
 * * transform himself into a monstrous agthrodos.  All inventory and equipment
 * * is transferred to its proper position.  When the agthrodos stops fighting,
 * * it will morph back, with all inventory and equipment in proper place.
 */

P_obj has_moonstone_fragment(P_char ch);
extern char arg1[MAX_STRING_LENGTH];
extern char arg2[MAX_STRING_LENGTH];
int xexos(P_char ch, P_char pl, int cmd, char *arg)
{
	P_char tempchar = NULL, was_fighting = NULL;
	P_obj item, next_item;
	int pos;

	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch || !IS_AWAKE(ch))
		return FALSE;

	if (IS_FIGHTING(ch))
	{
		was_fighting = GET_OPPONENT(ch);

		/*
		 * load agthrodos
		 */
		tempchar = read_mobile(12026, VIRTUAL);

		if (!tempchar)
		{
			logit(LOG_EXIT, "assert: mob load failed in xexos()");
			return FALSE;
		}
		/*
		 * stick agthrodos in same room
		 */
		char_to_room(tempchar, ch->in_room, -2);

		/*
		 * transfer inventory to agthrodos
		 */
		for (item = ch->carrying; item; item = next_item)
		{
			next_item = item->next_content;
			obj_from_char(item);
			obj_to_char(item, tempchar);
		}

		/*
		 * transfer equipment to agthrodos
		 */
		for (pos = 0; pos < MAX_WEAR; pos++)
		{
			if (ch->equipment[pos] != NULL)
			{
				item = unequip_char(ch, pos);
				equip_char(tempchar, item, pos, TRUE);
			}
		}

		/*
		 * let the player know what's going on
		 */
		mobsay(ch, "That was NOT a good idea!");
		act("$n pulls a vial from a hidden pocket and quickly quaffs it.", TRUE, ch, 0, 0,
		    TO_ROOM);
		act("Flesh rends and tears, reshaping $n into something monstrous!", TRUE, ch, 0, 0,
		    TO_ROOM);

		act("Whatever $n has become roars and charges to attack you!", FALSE, ch, 0,
		    GET_OPPONENT(ch), TO_VICT);

		act("Whatever $n has become roars and charges to attack $N!", FALSE, ch, 0,
		    GET_OPPONENT(ch), TO_NOTVICT);

		/*
		 * remove xexos
		 */
		extract_char(ch);
		ch = NULL;

		MobStartFight(tempchar, was_fighting);

		return (TRUE);
	}
	if (cmd == CMD_ASK)
	{
		if (has_moonstone_fragment(pl))
		{
			send_to_char("Xexos says 'So, you've killed that... thing.\r\n", pl);
			send_to_char(
				"  Fine, i admit it. I stole the moonstone from that old fool!\r\n",
				pl);
			send_to_char(
				"  But as i was escaping Sarmiz bay on a ship, we've come under pirates attack.\r\n",
				pl);
			send_to_char(
				"  In the heat of battle, the moonstone has been broken into three pieces!\r\n",
				pl);
			send_to_char(
				"  One fragment fell into ocean and i managed to escape to the land with another.\r\n",
				pl);
			send_to_char("  Pirates got the third one i guess...'\r\n\r\n", pl);
			send_to_char(
				"Xexos says 'I've tried to make an automaton with a single fragment, but its incomplete!\r\n",
				pl);
			send_to_char(
				"  Damn thing went berserk and i had to lock it down!'\r\n\r\n",
				pl);
			send_to_char(
				"Xexos signs 'And now i have to hide in this shithole from Erzul's revenge. So useless...'\r\n",
				pl);
			return TRUE;
		}
		else
		{
			half_chop(arg, arg1, arg2);
			if (*arg2)
			{
				if (isname(arg2, "automaton automatons erzul moonstone"))
				{
					send_to_char(
						"Xexos says 'I dont know what are you talking about, get lost!'\r\n",
						pl);
					return TRUE;
				}
			}
		}
	}

	return (FALSE);
}
int agthrodos(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	P_char tempchar = NULL;
	P_obj item, next_item;
	int pos;

	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch || !IS_AWAKE(ch))
		return FALSE;

	/*
	 * if it's something besides a periodic event call, return
	 */
	if (cmd != 0)
		return FALSE;

	/*
	 * check if agthrodos has stopped fighting
	 */
	if (!IS_FIGHTING(ch))
	{
		/*
		 * load Xexos
		 */
		tempchar = read_mobile(12025, VIRTUAL);

		if (!tempchar)
		{
			logit(LOG_EXIT, "assert: mob load failed in agthrodos()");
			return FALSE;
		}
		/*
		 * stick Xexos in same room
		 */
		char_to_room(tempchar, ch->in_room, 0);

		/*
		 * transfer inventory to Xexos
		 */
		for (item = ch->carrying; item; item = next_item)
		{
			next_item = item->next_content;
			obj_from_char(item);
			obj_to_char(item, tempchar);
		}

		/*
		 * transfer equipment to Xexos
		 */
		for (pos = 0; pos < MAX_WEAR; pos++)
		{
			if (ch->equipment[pos] != NULL)
			{
				item = unequip_char(ch, pos);
				equip_char(tempchar, item, pos, TRUE);
			}
		}

		/*
		 * let any watchers know what's going on
		 */
		act("$n chuffs angrily, and looks around for further threats.", TRUE, ch, 0, 0,
		    TO_ROOM);
		act("$n lets out a bellow, then reverts to its normal form, $N.", TRUE, ch, 0,
		    tempchar, TO_NOTVICT);

		/*
		 * remove agthrodos
		 */
		extract_char(ch);
		ch = NULL;

		return (TRUE);
	}
	else
	{
		return FALSE;
	}
}
int automaton_unblock(P_char ch, P_char pl, int cmd, char * /*arg*/)
{
	int flag = FALSE;
	P_char i;

	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch || pl || cmd)
		return FALSE;

	if (world[ch->in_room].number != 12159)
		return FALSE;

	for (i = world[ch->in_room].people; i; i = i->next_in_room)
		if (i != ch)
			flag = TRUE;

	if (!flag)
	{
		if (IS_SET(world[real_room(12158)].dir_option[DIR_DOWN]->exit_info, EX_BLOCKED))
			REMOVE_BIT(world[real_room(12158)].dir_option[DIR_DOWN]->exit_info,
				   EX_BLOCKED);
		if (IS_SET(world[real_room(12159)].dir_option[DIR_UP]->exit_info, EX_BLOCKED))
			REMOVE_BIT(world[real_room(12159)].dir_option[DIR_UP]->exit_info,
				   EX_BLOCKED);
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}
int phalanx(P_char ch, P_char pl, int cmd, char * /*arg*/)
{
	P_obj obj;
	sh_int temp = 0, temp2 = 0;

	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (cmd == CMD_DEATH)
	{
		obj = read_object(12000, VIRTUAL);
		if (!(obj))
		{
			logit(LOG_OBJ, "phalanx: could not load object 12000");
			return FALSE;
		}
		act("BOOM! $n explodes, leaving a $p, which\r\nfalls toward the ground, sparkling along the way.\r\n",
		    FALSE, ch, obj, 0, TO_ROOM);
		obj_to_room(obj, ch->in_room);
		return (FALSE);
	}
	if (pl)
	{
		if ((cmd == CMD_UP) && (pl != ch))
		{
			act("$n whirrs around the ceiling.", TRUE, ch, 0, 0, TO_ROOM);
			act("$N is preventing you.", FALSE, pl, 0, ch, TO_CHAR);
			return (TRUE);
		}
	}
	else
	{
		if (IS_FIGHTING(ch))
		{
			if ((GET_HIT(ch) < (GET_MAX_HIT(ch) / 4)) &&
			    (ch->in_room != real_room(12144)))
			{
				temp = ch->in_room;
				do_move(ch, 0, CMD_UP);
				if (ch->in_room != temp)
				{
					temp2 = ch->in_room;
					char_from_room(ch);
					char_to_room(ch, temp, 0);
					act("$n retreats upward.", TRUE, ch, 0, 0, TO_ROOM);
					ch->points.hit += ch->points.hit;
					char_from_room(ch);
					char_to_room(ch, temp2, 0);
					act("$n has arrived.", TRUE, ch, 0, 0, TO_ROOM);
				}
			}
		}
		else if (ch->points.base_armor == -50)
		{
			act("$n forms a new configuration.", TRUE, ch, 0, 0, TO_ROOM);
			ch->points.base_armor = -100;
		}
		else
		{
			switch (dice(3, 7))
			{
			case 20:
				act("$n splits apart to reorganize.", TRUE, ch, 0, 0, TO_ROOM);
				ch->points.base_armor = -50;
				break;
			case 19:
				act("$n makes some crackling noises.", FALSE, ch, 0, 0, TO_ROOM);
				break;
			case 7:
				act("A spark emanates from the interior of $n.", FALSE, ch, 0, 0,
				    TO_ROOM);
				break;
			default:
				break;
			}
		}
	}
	return (FALSE);
}
int snowbeast(P_char ch, P_char pl, int cmd, char * /*arg*/)
{
	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (pl)
	{
		return (0);
	}
	else
	{
		if (!IS_FIGHTING(ch))
		{
			switch (dice(3, 6))
			{
			case 3:
				mobsay(ch, "Yaargh, arrogha!!?!");
				break;
			case 4:
				mobsay(ch, "Hmmph.");
				break;
			case 5:
				act("The snowbeast scratches itself.", TRUE, ch, 0, 0, TO_ROOM);
				break;
			case 6:
				act("The snowbeast stares inquisitively at a point in space.", TRUE,
				    ch, 0, 0, TO_ROOM);
				break;
			default:
				break;
			}
		}
	}
	return (FALSE);
}
int snowvulture(P_char ch, P_char pl, int cmd, char *arg)
{
	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (pl)
	{
		return (0);
	}
	else
	{
		if (!IS_FIGHTING(ch))
		{
			switch (dice(3, 3))
			{
			case 3:
				act("$n squeaks, \"Skaaa? reet.\"", 0, ch, 0, 0, TO_ROOM);
				break;
			case 4:
				act("$n flaps about.", FALSE, ch, 0, 0, TO_ROOM);
				break;
			case 5:
				devour(ch, pl, cmd, arg);
				break;
			default:
				break;
			}
		}
	}
	return (FALSE);
}
int spiny(P_char ch, P_char pl, int cmd, char * /*arg*/)
{
	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (pl)
	{
		return (0);
	}
	else
	{
		if (!IS_FIGHTING(ch))
		{
			switch (dice(3, 2))
			{
			case 3:
				act("$n flips onto its back...or is that its front?", TRUE, ch, 0,
				    0, TO_ROOM);
				break;
			case 4:
				act("$n makes some clicking noises.", FALSE, ch, 0, 0, TO_ROOM);
				break;
			default:
				break;
			}
		}
	}
	return TRUE;
}
int spore_ball(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	P_char k, next;

	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return FALSE;

	if (cmd == CMD_DEATH)
	{
		act("$n crumples, a noxious gas escaping its interior.", FALSE, ch, 0, 0, TO_ROOM);
		if (CHAR_IN_SAFE_ROOM(ch))
			act("The gas dissipates harmlessly.", FALSE, ch, 0, 0, TO_ROOM);
		else
		{
			for (k = world[ch->in_room].people; k; k = next)
			{
				next = k->next_in_room;

				if (k != ch)
					spell_poison(24, ch, 0, 0, k, 0);
			}
		}

		return TRUE;
	}

	return (FALSE);
}

/* Additional Lava Tubes procedures. */

int skeleton(P_char ch, P_char pl, int cmd, char * /*arg*/)
{
	P_char temp;

	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return FALSE;

	if (ch == pl)
		return FALSE;

	if (cmd == CMD_DEATH)
	{
		if (!ch->only.npc->spec[0])
			ch->only.npc->spec[0] = 3;
		else if (!--ch->only.npc->spec[0])
			return (FALSE);

		temp = read_mobile(GET_RNUM(ch), REAL);
		if (temp)
		{
			char_to_room(temp, ch->in_room, 0);
			temp->only.npc->spec[0] = ch->only.npc->spec[0];
		}
		temp = read_mobile(GET_RNUM(ch), REAL);
		if (temp)
		{
			char_to_room(temp, ch->in_room, 0);
			temp->only.npc->spec[0] = ch->only.npc->spec[0];
		}
		act("The bones of the skeleton split apart and reform into two new skeletons.",
		    TRUE, ch, 0, 0, TO_ROOM);
	}
	if (!pl || !CAN_SEE(ch, pl))
		return FALSE;

	if (cmd == CMD_FLEE)
	{
		if (!number(0, 19))
		{
			act("As you turn to flee, a skeleton trips you!", FALSE, pl, 0, 0, TO_CHAR);
			act("$N turns to run, but a skeleton trips $M!", TRUE, ch, 0, pl,
			    TO_NOTVICT);
			SET_POS(pl, GET_STAT(pl) + POS_PRONE);
			CharWait(pl, 4);
			return TRUE;
		}
	}
	else if ((cmd == CMD_NORTH) || (cmd == CMD_SOUTH) || (cmd == CMD_EAST) ||
		 (cmd == CMD_WEST) || (cmd == CMD_UP) || (cmd == CMD_DOWN))
	{
		if (!number(0, 19))
		{
			act("As you try to leave, a skeleton leaps in front of you!", FALSE, pl, 0,
			    0, TO_CHAR);
			act("$N trys to leave, but a skeleton blocks $M!", TRUE, ch, 0, pl,
			    TO_NOTVICT);
			CharWait(pl, 2);
			return TRUE;
		}
	}
	return (FALSE);
}

int crystal_spike(P_obj obj, P_char ch, int cmd, char * /*arg*/)
{
	if (cmd == CMD_SET_PERIODIC)
	{
		return FALSE;
	}

	if (!IS_ALIVE(ch) || !OBJ_WORN_POS(obj, HOLD))
	{
		return FALSE;
	}

	if (obj->loc.wearing == ch)
	{
		if (cmd == CMD_CAST)
		{
			obj->value[0]--;
			if (obj->value[0] == 0)
			{
				act("Your $q fades slowly, and disappears into nothing.", FALSE,
				    obj->loc.wearing, obj, 0, TO_CHAR);
				act("$n's $q fades slowly, and disappears into nothing.", FALSE,
				    obj->loc.wearing, obj, 0, TO_ROOM);
				unequip_char(obj->loc.wearing, HOLD);
				extract_obj(obj, TRUE); // Not an arti, but 'in game.'
				obj = NULL;
			}
		}
	}
	return FALSE;
}

int automaton_lever(P_obj obj, P_char ch, int cmd, char *arg)
{
	P_char tempch;
	P_obj tempobj;

	if (cmd == CMD_SET_PERIODIC)
	{
		return FALSE;
	}

	if (!obj || !IS_ALIVE(ch) || !arg)
	{
		return FALSE;
	}

	// No tracks are autmaton levers.
	generic_find(arg, FIND_OBJ_INV | FIND_OBJ_EQUIP | FIND_OBJ_ROOM | FIND_NO_TRACKS, ch,
		     &tempch, &tempobj);
	if (tempobj != obj)
	{
		return FALSE;
	}

	if (cmd == CMD_PULL)
	{
		if (IS_SET(world[ch->in_room].dir_option[DIR_UP]->exit_info, EX_BLOCKED))
		{
			act("You pull $p.", FALSE, ch, obj, 0, TO_CHAR);
			act("$n pulls $p.", TRUE, ch, obj, 0, TO_ROOM);
			send_to_room("A loud metallic scraping sounds, followed by a clunk.\n",
				     ch->in_room);
			send_to_room("The trapdoor appears to hang ever so slightly lower.\n",
				     ch->in_room);
			REMOVE_BIT(world[ch->in_room].dir_option[DIR_UP]->exit_info, EX_BLOCKED);
			REMOVE_BIT(world[real_room(12158)].dir_option[DIR_DOWN]->exit_info,
				   EX_BLOCKED);
			return TRUE;
		}
		else
		{
			send_to_char("Nothing seems to happen.\n", ch);
			return TRUE;
		}
	}
	else
	{
		return FALSE;
	}
}
