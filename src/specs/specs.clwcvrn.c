#include "core/prototypes.h"
#include "combat/damage.h"
#include "core/structs.h"
#include "net/comm.h"
#include "world/db.h"
#include "world/events.h"
#include "cmd/interp.h"
#include "core/utils.h"
#include <stdio.h>
#include "world/specs.prototypes.h"
#include "magic/spells.h"
#include "world/vnum.obj.h"

/*
   extern variables
 */

extern P_room world;
extern struct zone_data *zone_table;

/*
 * **Should be a decent death proc for mobs vnum 80706-80710,
 * **80713, 80715-80724, 80728-80734.  If it can be assigned to
 * **that many.
 */

int clwcvrn_crys_die(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return FALSE;

	if (cmd == CMD_DEATH)
	{
		act("$n shatters into a million sharp fragments that fly everywhere.", TRUE, ch, 0,
		    0, TO_ROOM);
	}
	return (FALSE);
}

/*
 * **Ok, here is one I am worried about.  Played with this
 * **one a bit more than I would like.  The mob is supposed to
 * **die and leave behind a pile of crystal shards that contains
 * **everything it was carrying.  It is also mostly copied from
 * **another proc, I would like to put event_decay in here
 * **someplace so the pile decays like a corpse, and I am not
 * **real certain how to do it.  Should be assigned to mob 80735.
 */

int clwcvrn_golem_shatter(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	P_obj pile, money, obj, next_obj;
	int pos;

	// Check for periodic event calls
	if (cmd == CMD_SET_PERIODIC)
		return FALSE;

	if (!ch || IS_PC(ch))
		return FALSE;

	// Spec die
	if (cmd == CMD_DEATH)
	{
		pile = read_object(VOBJ_CLWCVRN_CRYSTAL_SHARDS, VIRTUAL);
		if (!pile)
		{
			logit(LOG_OBJ, "clwcvrn_golem_shatter: could not load object %d",
			      VOBJ_CLWCVRN_CRYSTAL_SHARDS);
			return FALSE;
		}
		if (pile->type == ITEM_CONTAINER)
		{
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

			// Stick object in same room unless something's weird
			if (ch->in_room == NOWHERE)
			{
				if (real_room(ch->specials.was_in_room) != NOWHERE)
				{
					obj_to_room(pile, real_room(ch->specials.was_in_room));
				}
				else
				{
					extract_obj(
						pile,
						TRUE); // No good place to put it (never 'in game', but contents may have been).
					pile = NULL;
					return FALSE;
				}
			}
			else
			{
				obj_to_room(pile, ch->in_room);
			}

			// Clue players in
			act("$n emits a lood pitched wail and begins to shake.\r\n"
			    "$n shatters and falls into $p... a cloud of dust rises.",
			    TRUE, ch, pile, 0, TO_ROOM);
			return TRUE;
		}
		else
		{
			logit(LOG_OBJ,
			      "Object %d not CONTAINER for clwcvrn_golem_shatter()!!  Aborted.",
			      VOBJ_CLWCVRN_CRYSTAL_SHARDS);
			return FALSE;
		}
	}
	// Was from some command
	return FALSE;
}

/*
 * ** Copy of SS shady old man for the clawed caverns protector.
 * ** Should be assigned to mob vnum 80739.
 */

int clwcvrn_protect(P_char ch, P_char pl, int cmd, char * /*arg*/)
{
	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return FALSE;

	if (IS_TRUSTED(ch))
	{
		return FALSE;
	}

	if ((ch->in_room == real_room(80700)) && (cmd == CMD_NORTH))
	{
		if (GET_LEVEL(pl) > 20 && GET_LEVEL(pl) < 51)
		{
			act("The giant stone golem rumbles something to $n, stopping $m with its hand.",
			    FALSE, pl, 0, 0, TO_ROOM);
			send_to_char(
				"The giant stone golem rumbles \"Murderers such as you must go elsewhere.\"\r\n",
				pl);
			return TRUE;
		}
	}
	return FALSE;
}

int burn_touch_obj(P_obj obj, P_char ch, int cmd, char *arg)
{
	int dam;

	if (cmd == CMD_SET_PERIODIC)
		return FALSE;

	if (!ch)
		return FALSE;

	if (IS_TRUSTED(ch) || !CAN_SEE_OBJ(ch, obj))
		return FALSE;

	switch (cmd)
	{
	case CMD_TOUCH:
	case CMD_PULL:
	case CMD_PUSH:
	case CMD_MOVE:
	case CMD_USE:
	case CMD_TUG:
	case CMD_OPEN:
	case CMD_CLOSE:
	case CMD_RUB:
	case CMD_HOLD:
		act("YOW!  $p burns the hell out of you as you touch it!", FALSE, ch, obj, 0,
		    TO_CHAR);
		act("$n recoils in pain as $e is burned by touching $p.", FALSE, ch, obj, 0,
		    TO_ROOM);

		dam = 20;
		if (IS_AFFECTED(ch, AFF_PROT_FIRE))
			dam >>= 1;
		if (GET_RACE(ch) == RACE_TROLL)
			dam <<= 2;

		if (damage(ch, ch, dam, TYPE_UNDEFINED))
			return TRUE;

		if (GET_ITEM_TYPE(obj) == ITEM_SWITCH)
			return item_switch(obj, ch, cmd, arg);
		else
			return FALSE;

	default:
		return FALSE;
	}

	return FALSE;
}

/*
 * acerlade, the big meany in the transparent tower who turns PC charmees
 * into NPC charmees..  WATCH OUT KIDS!
 */
int claw_cavern_drow_mage(P_char ch, P_char pl, int cmd, char *arg)
{
	char obj_name[MAX_INPUT_LENGTH], *argument;
	P_obj obj;

	/*
	 * Check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return FALSE;

	if (!pl || (IS_TRUSTED(pl) && (cmd != CMD_GIVE)))
		return FALSE;

	if (IS_AGG_CMD(cmd) && (cmd != CMD_WILL) && (cmd != CMD_CAST))
	{
		send_to_char("The walls hum as an invisible force pushes you away.\r\n", pl);
		act("The walls hum as $n is pushed away by an invisible field.", FALSE, pl, 0, 0,
		    TO_ROOM);
		mobsay(ch,
		       "Fools!  Do you think I would even allow you to stand in my presence if I had a choice?");

		return TRUE;
	}

	if ((cmd == CMD_WILL) || (cmd == CMD_CAST))
	{
		send_to_char(
			"The walls glow as you feel your magic drained into the building around you.\r\n",
			pl);
		act("$n looks bewildered as the walls glow and somehow prevent the casting of $s spell.",
		    FALSE, pl, 0, 0, TO_ROOM);
		return TRUE;
	}

	if (cmd == CMD_GIVE)
	{
		argument = arg;
		argument = one_argument(argument, obj_name);

		obj = get_obj_in_list_vis(pl, obj_name, pl->carrying);
		if (!obj)
		{
			send_to_char("You do not seem to have anything like that.\r\n", pl);
			return TRUE;
		}

		if (OBJ_VNUM(obj) == VOBJ_CLWCVRN_RAINBOW_KEY)
		{
			act("$n gives $p to $N.", 1, pl, obj, ch, TO_NOTVICT);
			act("$n gives you $p.", 0, pl, obj, ch, TO_VICT);
			send_to_char("Ok.\r\n", pl);

			obj_from_char(obj);
			extract_obj(obj, TRUE); // Not an arti, but 'in game.'

			mobsay(ch, "Yes!  Now those fools shall bow to their true master!");
			do_action(ch, 0, CMD_CACKLE);
			act("The mage begins a complex incantation and the key begins to glow.",
			    TRUE, ch, 0, 0, TO_ROOM);
			act("$n screams as the walls begin to hum at a high pitch and the key starts to shiver.",
			    TRUE, ch, 0, 0, TO_ROOM);
			act("The crystal key shakes and glows, finally exploding in a burst of searing light.",
			    TRUE, ch, 0, 0, TO_ROOM);

			obj = read_object(VOBJ_CLWCVRN_RAINBOW_SHARDS, VIRTUAL);
			if (!obj)
				mobsay(ch, "Oh damn, there is a bug in the proc!  Tell a god!");
			else
				obj_to_room(obj, ch->in_room);

			extract_char(ch);

			return TRUE;
		}

		return FALSE; // let normal give handle it
	}

	return FALSE;
}
