/* Alatorin special procedures. */

#include <string.h>
#include <time.h>

#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utils.h"
#include "net/comm.h"
#include "cmd/interp.h"
#include "world/handler.h"
#include "world/db.h"
#include "economy/mining.h"
#include "economy/currency_transaction.h"
#include "economy/economic_gameplay_authority.h"
#include "magic/spells.h"

int miners_helmet(P_obj obj, P_char ch, int cmd, char *argument)
{
	char *arg;
	int curr_time;

	if (cmd == CMD_SET_PERIODIC)
		return FALSE;

	if (!ch || !obj)
		return FALSE;

	if (!OBJ_WORN(obj))
		return FALSE;

	if (argument && (cmd == CMD_SAY))
	{
		arg = argument;

		while (*arg == ' ')
			arg++;

		if (!strcmp(arg, "paydirt"))
		{
			curr_time = time(NULL);

			// 10 minute timer = 600 sec.
			if (obj->timer[0] + 600 <= curr_time)
			{
				obj->timer[0] = curr_time;
				spell_lodestone_vision(60, ch, arg, SPELL_LODESTONE, ch, obj);
			}

			return TRUE;
		}
	}
	return FALSE;
}

/* Alatorin smelter interaction and its completion helper. */

void finish_smelt(P_char ch, P_char pl, int vnum);

// I changed Steelgrip's smelting quest into a proc for $ purposes.
int smelter(P_char ch, P_char pl, int cmd, char *argument)
{
	char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH], *rest;
	char buf[MAX_STRING_LENGTH];
	int amount = 0, type;
	P_obj obj = NULL;

	if (cmd == CMD_SET_PERIODIC)
	{
		return FALSE;
	}

	if (cmd == CMD_ASK || cmd == CMD_TELL)
	{
		rest = one_argument(argument, arg1);
		if (!(ch == get_char_room_vis(pl, arg1)))
			return FALSE;
		one_argument(rest, arg2);
		while (*rest == ' ')
		{
			rest++;
		}
		if (isname(arg2, "smelting smelt"))
		{
			snprintf(buf, MAX_STRING_LENGTH, "You ask $N '%s'", rest);
			act(buf, FALSE, pl, 0, ch, TO_CHAR);
			snprintf(buf, MAX_STRING_LENGTH, "$n asks $N '%s'", rest);
			act(buf, TRUE, pl, 0, ch, TO_ROOM);
			act("&n$n says to $N '&+YSmelting &Nhappens to be my specialty!'", TRUE, ch,
			    0, pl, TO_ROOM);
			act("&n$n claps his hands, smiling.", FALSE, ch, 0, NULL, TO_ROOM);
			do_say(ch,
			       writable_arg(
				       "I can take two small pieces of the same ore and make them a medium.  I can also take"
				       " two medium pieces of the same ore and make it a large.  Same price for either smelt!  &+W10"
				       " platinum &Nfor &+ciron&n, &+Ctin&N, &+ycopper&N, or silver.  &+W50 platinum &Nfor &+Ygold&N,"
				       " &+Wplatinum&n, or &+mmithril&n, and &+W100 platinum&N for &+Madamantium&n."),
			       CMD_SAY);
			do_say(ch,
			       writable_arg(
				       "Remember, I only deal in &+Wplatinum&n now, so keep your other coins."),
			       CMD_SAY);
			return TRUE;
		}
	}

	if (cmd == CMD_GIVE)
	{
		// Smelting can accept coins or ore before charging the NPC-held cash.
		if (economic_gameplay_authority::active())
		{
			send_to_char("The smelter is unavailable while active accounting is "
				     "enabled.\r\n",
				     pl);
			return TRUE;
		}
		rest = one_argument(argument, arg1);
		rest = one_argument(rest, arg2);
		// If it's coins.
		if (is_number(arg1) && strlen(arg1) <= 7)
		{
			// Trying to give a negative number of coins?
			if ((amount = atoi(arg1)) < 0)
			{
				return FALSE;
			}
			type = coin_type(arg2);
			// Bad coin type or not enough coins.
			if ((type < 0) || (pl->points.cash[type] < amount))
			{
				return FALSE;
			}
			// Is pl giving to ch?
			rest = one_argument(rest, arg1);
			if (!*arg1 || !(ch == get_char_room_vis(pl, arg1)))
			{
				return FALSE;
			}
			pl->points.cash[type] -= amount;
			ch->points.cash[type] += amount;
			snprintf(buf, MAX_STRING_LENGTH, "You give %d %s to $N.", amount,
				 (type == 0) ? "copper" :
				 (type == 1) ? "silver" :
				 (type == 2) ? "gold" :
					       "platinum");
			act(buf, FALSE, pl, obj, ch, TO_CHAR);
			finish_smelt(ch, pl, 0);
			return TRUE;
		}
		else if ((obj = get_obj_in_list_vis(pl, arg1, pl->carrying)))
		{
			// If pl isn't giving to ch, or is trying to give a cursed/souldbound item.
			if ((ch != get_char_room_vis(pl, arg2)) ||
			    IS_SET(obj->extra_flags, ITEM_NODROP) ||
			    IS_OBJ_STAT2(obj, ITEM2_SOULBIND))
			{
				return FALSE;
			}
			// If it's not a valid ore vnum.
			if (OBJ_VNUM(obj) < LOWEST_ORE_VNUM ||
			    OBJ_VNUM(obj) > LOWEST_ORE_VNUM + NUMBER_ORE_TYPES * 3 - 1)
			{
				return FALSE;
			}

			if ((OBJ_VNUM(obj) - LOWEST_ORE_VNUM) % 3 == 2)
			{
				act("$n says '$p is too big to smelt into something bigger.'",
				    FALSE, ch, obj, NULL, TO_ROOM);
				return TRUE;
			}

			if (vnum_in_inv(ch, OBJ_VNUM(obj)) > 1)
			{
				act("$n says 'I already have enough of $p to smelt.'", FALSE, ch,
				    obj, NULL, TO_ROOM);
				type = (OBJ_VNUM(obj) - LOWEST_ORE_VNUM) / 3;
				switch (type)
				{
				case 0:
				case 1:
				case 2:
				case 3:
					amount += 10;
					break;
				case 4:
				case 5:
				case 6:
					amount += 50;
					break;
				case 7:
					amount += 100;
					break;
				}
				if (amount - GET_PLATINUM(ch) > 0)
				{
					snprintf(
						buf, MAX_STRING_LENGTH,
						"But I need %d more &+Wplatinum&n to complete this transaction.",
						amount - GET_PLATINUM(ch));
					do_say(ch, buf, CMD_SAY);
					return TRUE;
				}
				// Otherwise we have the cash and the ore so finish.
				else
				{
					if (SUB_MONEY(ch, amount * 1000, 0) != 0)
						return TRUE;
					finish_smelt(ch, pl, OBJ_VNUM(obj));
					return TRUE;
				}
			}
			else
			{
				obj_from_char(obj);
				act("$n gives $p to $N.", TRUE, pl, obj, ch, TO_ROOM);
				act("You give $p to $N.", FALSE, pl, obj, ch, TO_CHAR);
				obj_to_char(obj, ch);
				// If we have enough ores to smelt.
				if (vnum_in_inv(ch, OBJ_VNUM(obj)) > 1)
				{
					switch ((OBJ_VNUM(obj) - LOWEST_ORE_VNUM) / 3)
					{
					case 0:
					case 1:
					case 2:
					case 3:
						amount = 10;
						break;
					case 4:
					case 5:
					case 6:
						amount = 50;
						break;
					case 7:
						amount = 100;
						break;
					}
					// If the smelter doesn't need more cash.
					if (amount - GET_PLATINUM(ch) <= 0)
					{
						if (SUB_MONEY(ch, amount * 1000, 0) != 0)
							return TRUE;
						finish_smelt(ch, pl, OBJ_VNUM(obj));
						return TRUE;
					}
					else
					{
						snprintf(
							buf, MAX_STRING_LENGTH,
							"I need %d more &+Wplatinum&n to complete this transaction.",
							amount - GET_PLATINUM(ch));
						do_say(ch, buf, CMD_SAY);
					}
				}
				return TRUE;
			}
		}
	}
	return FALSE;
}

void finish_smelt(P_char ch, P_char pl, int vnum)
{
	char buf[MAX_STRING_LENGTH], oreType[32];
	int price;
	P_obj ore, obj;

	ore = obj = NULL;

	if (vnum == 0)
	{
		for (obj = ch->carrying; obj; obj = obj->next_content)
		{
			// If it's a small/medium adamantium ore..
			if (OBJ_VNUM(obj) == LOWEST_ORE_VNUM + 7 * 3 ||
			    OBJ_VNUM(obj) == LOWEST_ORE_VNUM + 7 * 3 + 1)
			{
				// Look through the rest of the list for the same vnum.
				for (ore = obj->next_content; ore; ore = ore->next_content)
				{
					// If we've found a match.
					if (OBJ_VNUM(obj) == OBJ_VNUM(ore))
					{
						vnum = OBJ_VNUM(obj);
						price = 100;
						break;
					}
				}
				if (vnum > 0)
				{
					break;
				}
			}
		}
		if (vnum == 0)
		{
			for (obj = ch->carrying; obj; obj = obj->next_content)
			{
				// If it's a small/medium gold/plat/mith ore..
				if (((OBJ_VNUM(obj) - LOWEST_ORE_VNUM) % 3 == 0 ||
				     (OBJ_VNUM(obj) - LOWEST_ORE_VNUM) % 3 == 1) &&
				    OBJ_VNUM(obj) >= LOWEST_ORE_VNUM + 4 * 3 &&
				    OBJ_VNUM(obj) <= LOWEST_ORE_VNUM + 6 * 3 + 1)
				{
					// Look through the rest of the list for the same vnum.
					for (ore = obj->next_content; ore; ore = ore->next_content)
					{
						// If we've found a match.
						if (OBJ_VNUM(obj) == OBJ_VNUM(ore))
						{
							vnum = OBJ_VNUM(obj);
							price = 50;
							break;
						}
					}
					if (vnum > 0)
					{
						break;
					}
				}
			}
		}
		if (vnum == 0)
		{
			for (obj = ch->carrying; obj; obj = obj->next_content)
			{
				// If it's a small/medium iron/tin/copper/silver ore..
				if ((((OBJ_VNUM(obj) - LOWEST_ORE_VNUM) % 3 == 0 ||
				      (OBJ_VNUM(obj) - LOWEST_ORE_VNUM)) %
					     3 ==
				     1) &&
				    OBJ_VNUM(obj) >= LOWEST_ORE_VNUM &&
				    OBJ_VNUM(obj) <= LOWEST_ORE_VNUM + 3 * 3 + 1)
				{
					// Look through the rest of the list for the same vnum.
					for (ore = obj->next_content; ore; ore = ore->next_content)
					{
						// If we've found a match.
						if (OBJ_VNUM(obj) == OBJ_VNUM(ore))
						{
							vnum = OBJ_VNUM(obj);
							price = 10;
							break;
						}
					}
					if (vnum > 0)
					{
						break;
					}
				}
			}
		}
		// If we didn't find one, return
		if (vnum == 0)
		{
			return;
		}
		if (GET_PLATINUM(ch) < price)
		{
			snprintf(buf, MAX_STRING_LENGTH,
				 "I need %d more &+Wplatinum&n to complete this transaction.",
				 price - GET_PLATINUM(ch));
			do_say(ch, buf, CMD_SAY);
			return;
		}
		if (SUB_MONEY(ch, price * 1000, 0) != 0)
			return;
	}
	else
	{
		for (obj = ch->carrying; obj; obj = obj->next_content)
		{
			if (OBJ_VNUM(obj) == vnum)
			{
				break;
			}
		}
		// If we didn't find even one, just return.
		if (!obj)
		{
			return;
		}
		for (ore = obj->next_content; ore; ore = ore->next_content)
		{
			if (OBJ_VNUM(ore) == vnum)
			{
				break;
			}
		}
		// If we didn't find a match, return.
		if (!ore)
		{
			return;
		}
	}

	price = ore->cost + obj->cost;
	obj_from_char(ore);
	obj_from_char(obj);

	ore = read_object(vnum + 1, VIRTUAL);
	ore->cost = price;
	obj_to_char(ore, pl);

	switch ((OBJ_VNUM(ore) - LOWEST_ORE_VNUM) / 3)
	{
	case 0:
		snprintf(oreType, sizeof oreType, "&+ciron&n");
		break;
	case 1:
		snprintf(oreType, sizeof oreType, "&+Ctin&n");
		break;
	case 2:
		snprintf(oreType, sizeof oreType, "&+ycopper&n");
		break;
	case 3:
		snprintf(oreType, sizeof oreType, "&nsilver");
		break;
	case 4:
		snprintf(oreType, sizeof oreType, "&+Ygold&n");
		break;
	case 5:
		snprintf(oreType, sizeof oreType, "&+Wplatinum&n");
		break;
	case 6:
		snprintf(oreType, sizeof oreType, "&+mmithril&n");
		break;
	case 7:
		snprintf(oreType, sizeof oreType, "&+Madamantium&n");
		break;
	default:
		snprintf(oreType, sizeof oreType, "&+Lmetal&n");
		break;
	}
	snprintf(buf, MAX_STRING_LENGTH, "$n &+RSMELTS&n some %s!", oreType);
	act(buf, FALSE, ch, NULL, NULL, TO_ROOM);
	act("$n gives $p to $N.", TRUE, ch, ore, pl, TO_ROOM);
}
