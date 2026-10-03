#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "world/db.h"
#include "core/utility.h"
#include "core/utils.h"
#include "core/defines.h"
#include "economy/economic_gameplay_authority.h"
#include "magic/spells.h"
#include "sql/sql.h"

extern P_index obj_index;
extern P_room world;

void spell_enchant_weapon(int level, P_char ch, char * /*arg*/, int /*type*/, P_char /*victim*/,
			  P_obj obj)
{
	int i;
	bool affected;

	if ((GET_ITEM_TYPE(obj) == ITEM_WEAPON) && !IS_SET(obj->extra2_flags, ITEM2_MAGIC))
	{
		SET_BIT(obj->extra2_flags, ITEM2_MAGIC);

		for (i = 0, affected = FALSE; i < MAX_OBJ_AFFECT; i++)
		{
			if (obj->affected[i].location != APPLY_NONE &&
			    obj->affected[i].modifier != 0)
			{
				affected = TRUE;
			}
		}
		// If it already has effects, but isn't flagged magic, add 1 or 2 (@ lvl 31+) hitroll (3 for Overlords).
		if (affected)
		{
			for (i = 0, affected = FALSE; i < MAX_OBJ_AFFECT; i++)
			{
				if (obj->affected[i].modifier == 0)
				{
					obj->affected[i].location = APPLY_HITROLL;
					obj->affected[i].modifier = 1 + (level / 31);
					break;
				}
			}
		}
		else
		{
			// At level 31: 2 hit, at 51: 3 hit (and 4 hit for Forgers+).
			obj->affected[0].location = APPLY_HITROLL;
			obj->affected[0].modifier = 1 + (level - 11) / 20;

			// At level 36: 2 dam, at 56: 3 dam.
			obj->affected[1].location = APPLY_DAMROLL;
			obj->affected[1].modifier = 1 + (level - 16) / 20;
		}

		if (IS_GOOD(ch))
		{
			act("&+B$p glows blue.", FALSE, ch, obj, 0, TO_CHAR);
		}
		else if (IS_EVIL(ch))
		{
			act("&+r$p glows red.", FALSE, ch, obj, 0, TO_CHAR);
		}
		else
		{
			act("&+Y$p glows yellow.", FALSE, ch, obj, 0, TO_CHAR);
		}
	}
	else
	{
		send_to_char("&+wNothing seems to happen.&n\n", ch);
	}
}

void spell_repair_one_item(int /*level*/, P_char ch, char * /*arg*/, int /*type*/,
			   P_char /*victim*/, P_obj obj)
{
	if (obj)
	{
		obj->condition = 100;
		act("$q glows a faint &+Yyellowish hue&n as a wave of energy covers its surface.",
		    FALSE, ch, obj, 0, TO_CHAR);
		act("$n's $q glows a faint &+Yyellowish hue&n as a wave of energy covers its surface.",
		    FALSE, ch, obj, 0, TO_ROOM);
	}
}

void spell_knock(int /*cmd*/, P_char ch, char *argument, [[maybe_unused]] int type,
		 P_char /*victim*/, P_obj obj)
{
	int percent, chance, retval;
	char Gbuf2[MAX_STRING_LENGTH], Gbuf3[MAX_STRING_LENGTH];
	P_obj found_obj;
	P_char found_char;

	argument_interpreter(argument, Gbuf2, Gbuf3);

	chance = GET_LEVEL(ch);
	percent = number(1, 100);

	if (!*Gbuf2)
	{
		send_to_char("What requires unlocking here again?\n", ch);
		return;
	}

	if (IS_TRUSTED(ch))
	{
		retval = generic_find(argument, FIND_OBJ_INV | FIND_OBJ_ROOM, ch, &found_char,
				      &found_obj);
	}
	else
	{
		retval = generic_find(argument, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_NO_TRACKS, ch,
				      &found_char, &found_obj);
	}

	if (retval != 0)
	{
		if ((found_obj->type != ITEM_CONTAINER) && (found_obj->type != ITEM_STORAGE) &&
		    (found_obj->type != ITEM_QUIVER))
			send_to_char("That's not a container.\n", ch);
		else if (!IS_SET(found_obj->value[1], CONT_CLOSED))
			send_to_char("Unlocking something that's not even closed - how droll!\n",
				     ch);
		else if (found_obj->value[2] < 0)
			send_to_char("Odd - you can't seem to find a keyhole.\n", ch);
		else if (!IS_SET(found_obj->value[1], CONT_LOCKED))
			send_to_char("Hey, it's not even locked!\n", ch);
		else
		{
			if (IS_SET(found_obj->value[1], CONT_PICKPROOF))
			{
				send_to_char(
					"You focus your magic, but the container resists all attempts to open it.  You'd better find the key.\n",
					ch);
				return;
			}

			if ((percent > chance))
			{
				send_to_char(
					"You hear the brief sound of tumblers and pins turning, but alas, you lose concentration...\n",
					ch);
				CharWait(ch, 8);
				return;
			}
			else
			{
				REMOVE_BIT(found_obj->value[1], CONT_LOCKED);
				send_to_char(
					"The sound of tumblers and pins clicking into place is music to your ears.\n",
					ch);
				act("$p rattles loudly, the sound of tumblers and pins locking into place!",
				    FALSE, ch, obj, 0, TO_ROOM);
				CharWait(ch, 6);
				return;
			}
		}
	}
	return;
}

void spell_create_water(int level, P_char ch, char * /*arg*/, int /*type*/, P_char /*victim*/,
			P_obj obj)
{
	int water;

	if (GET_ITEM_TYPE(obj) == ITEM_DRINKCON)
	{
		if ((obj->value[2] != LIQ_WATER) && (obj->value[1] != 0))
		{
			name_from_drinkcon(obj);
			obj->value[2] = LIQ_SLIME;
			name_to_drinkcon(obj, LIQ_SLIME);
		}
		else
		{
			water = 2 * level;

			/* Calculate water it can contain, or water created */
			water = MIN(obj->value[0] - obj->value[1], water);

			if (water > 0)
			{
				obj->value[2] = LIQ_WATER;
				obj->value[1] += water;

				weight_change_object(obj, water);

				name_from_drinkcon(obj);
				name_to_drinkcon(obj, LIQ_WATER);
				act("$p is filled.", FALSE, ch, obj, 0, TO_CHAR);
			}
		}
	}
	else
	{
		send_to_char("It is unable to hold water.\n", ch);
		return;
	}
}

void spell_remove_curse(int /*level*/, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
			P_obj obj)
{
	P_obj t_obj, nextobj;
	int e_pos;

	if (IS_NPC(ch) && (GET_VNUM(ch) == 63))
		return;
	if (!obj && victim && economic_gameplay_authority::active())
	{
		bool would_drop = false;
		for (int slot = 0; slot < MAX_WEAR; ++slot)
			would_drop |= victim->equipment[slot] &&
				      IS_SET(victim->equipment[slot]->extra_flags, ITEM_NODROP);
		for (P_obj held = victim->carrying; held; held = held->next_content)
			would_drop |= IS_SET(held->extra_flags, ITEM_NODROP);
		if (would_drop)
		{
			send_to_char("That curse cannot release items right now.\r\n", ch);
			return;
		}
	}

	if (obj)
	{
		if (IS_SET(obj->extra_flags, ITEM_NODROP))
		{
			act("&+B$p briefly glows blue.", TRUE, ch, obj, 0, TO_CHAR);

			REMOVE_BIT(obj->extra_flags, ITEM_NODROP);
		}
	}
	else
	{
		/*
		 * Then it is a PC | NPC
		 */
		if (affected_by_spell(victim, SPELL_CURSE))
		{
			act("$n briefly &+rglows red, &+bthen blue.", FALSE, victim, 0, 0, TO_ROOM);
			act("&+WYou feel better.", FALSE, victim, 0, 0, TO_CHAR);
			affect_from_char(victim, SPELL_CURSE);
		}
		/*
		 * the nifty new bit, cursed items worn by target get zapped off,
		 * but remain cursed. -JAB
		 */

		if (!is_linked_to(ch, victim, LNK_CONSENT) && (ch != victim) && !IS_TRUSTED(ch) &&
		    IS_PC(ch))
		{
			send_to_char("target must consent to you to zap items in equip.\n", ch);
			return;
		}
		for (e_pos = 0; e_pos < MAX_WEAR; e_pos++)
			if ((t_obj = victim->equipment[e_pos]) &&
			    IS_SET(t_obj->extra_flags, ITEM_NODROP))
			{
				act("Your $q &+Bflashes blue&n and falls to the ground.", FALSE,
				    victim, t_obj, 0, TO_CHAR);
				act("$n's $q &+Bflashes blue&n and falls to the ground.", FALSE,
				    victim, t_obj, 0, TO_ROOM);
				if (obj_index[t_obj->R_num].virtual_number == 67243)
				{
					act("&+LUpon being torn from $n&+L, $p &+Llets out a ghastly &n&+rSHRIEK!!!",
					    FALSE, victim, t_obj, 0, TO_ROOM);
					act("&+LUpon being torn from you, $p &+Llets out a ghastly &n&+rSHRIEK!!!",
					    FALSE, victim, t_obj, 0, TO_CHAR);
					affect_from_char(ch, SPELL_VAMPIRE);
					spell_dispel_magic(60, victim, 0, SPELL_TYPE_SPELL, victim,
							   0);
				}
				if (IS_TRUSTED(victim))
				{
					wizlog(GET_LEVEL(victim), "%s drops %s [%d]",
					       GET_NAME(victim), t_obj->short_description,
					       world[victim->in_room].number);
					logit(LOG_WIZ, "%s drops %s [%d]", GET_NAME(victim),
					      t_obj->short_description,
					      world[victim->in_room].number);
					sql_log(victim, WIZLOG, "Dropped %s",
						t_obj->short_description);
				}
				obj_to_room(unequip_char(victim, e_pos), victim->in_room);
			}
		for (t_obj = victim->carrying; t_obj; t_obj = nextobj)
		{
			nextobj = t_obj->next_content;

			if (IS_SET(t_obj->extra_flags, ITEM_NODROP))
			{
				act("Your $q &+Bflashes blue&n and falls to the ground.", FALSE,
				    victim, t_obj, 0, TO_CHAR);
				act("$n's $q &+Bflashes blue&n and falls to the ground.", FALSE,
				    victim, t_obj, 0, TO_ROOM);
				if (obj_index[t_obj->R_num].virtual_number == 67243)
				{
					act("&+LUpon being torn from $n&+L, $p &+Llets out a ghastly &n&+rSHRIEK!!!",
					    FALSE, victim, t_obj, 0, TO_ROOM);
					act("&+LUpon being torn from you, $p &+Llets out a ghastly &n&+rSHRIEK!!!",
					    FALSE, victim, t_obj, 0, TO_CHAR);
					affect_from_char(ch, SPELL_VAMPIRE);
					spell_dispel_magic(60, victim, 0, SPELL_TYPE_SPELL, victim,
							   0);
				}
				if (IS_TRUSTED(victim))
				{
					wizlog(GET_LEVEL(victim), "%s drops %s [%d]",
					       GET_NAME(victim), t_obj->short_description,
					       world[victim->in_room].number);
					logit(LOG_WIZ, "%s drops %s [%d]", GET_NAME(victim),
					      t_obj->short_description,
					      world[victim->in_room].number);
					sql_log(victim, WIZLOG, "Dropped %s",
						t_obj->short_description);
				}
				obj_from_char(t_obj);
				obj_to_room(t_obj, victim->in_room);
			}
		}
	}
}
