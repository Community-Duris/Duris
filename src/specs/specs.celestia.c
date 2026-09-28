/* Celestia special procedures. */

#include <time.h>

#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "world/db.h"
#include "world/events.h"
#include "cmd/interp.h"
#include "core/utils.h"
#include "world/specs.prototypes.h"
#include "magic/spells.h"
#include "combat/damage.h"

extern P_room world;

int Malevolence(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	P_char tch, vapor, vict = NULL;
	P_obj item, next_item;
	int pos;
	int randroom;

	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return FALSE;

	if (cmd != CMD_PERIODIC)
		return FALSE;

	if (!IS_ALIVE(ch))
	{
		return false;
	}

	if (!IS_FIGHTING(ch))
		return FALSE;

	if (!GET_OPPONENT(ch))
		return FALSE;

	/* loop number of PC, pick someone */
	if (number(0, 49) == 0)
	{
		for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
		{
			if (IS_PC(tch) && !IS_TRUSTED(tch))
			{
				if (number(0, 10) >= 5)
				{
					vict = tch;
					act("$n screams out in an unearthly howl '&+rI control reality! I control you! Begone from my domain and die within Celestia!' &n",
					    FALSE, ch, 0, vict, TO_VICT);
					act("$n screams out in an unearthly howl '&+rI control reality! I control $N! Begone from my domain and die within Celestia!' &n",
					    FALSE, ch, 0, vict, TO_NOTVICT);
					randroom = number(45500, 45520);
					char_from_room(vict);
					char_to_room(vict, real_room(randroom), -1);
				}
			}
		}
		return TRUE;
	}

	if (GET_HIT(ch) < (GET_MAX_HIT(ch) / 3))
	{
		vapor = read_mobile(45571, VIRTUAL);
		if (!vapor)
		{
			logit(LOG_EXIT, "assert: mob load failed in Malevolence()");
			return FALSE;
		}
		GET_HIT(vapor) = GET_MAX_HIT(vapor) = vapor->points.base_hit =
			MAX(GET_HIT(ch) * 4, 1500);
		char_to_room(vapor, ch->in_room, 0);

		for (item = ch->carrying; item; item = next_item)
		{
			next_item = item->next_content;
			obj_from_char(item);
			obj_to_char(item, vapor); /*
			                           * transfer any eq and inv
			                           */
		}
		for (pos = 0; pos < MAX_WEAR; pos++)
		{
			if (ch->equipment[pos] != NULL)
			{
				item = unequip_char(ch, pos);
				equip_char(vapor, item, pos, TRUE);
			}
		}
		act("$n is dead! R.I.P.", TRUE, ch, 0, 0, TO_ROOM);
		act("The corpse of &+wMa&+Llev&n&+rolen&n&+wce, the en&+Ltity of hav&n&+roc&n shudders in a spasm of death then glows with a blindling light.",
		    FALSE, ch, 0, vapor, TO_NOTVICT);
		char_from_room(ch);
		char_to_room(ch, real_room(1), -1);
		act("$n screams out in an unearthly howl '&+rI LIVE!' &n", FALSE, vapor, 0, vapor,
		    TO_NOTVICT);
		die(ch, ch);
	}

	return FALSE;
}
int Malevolence_vapor(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	P_char tch, vict = NULL;

	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return FALSE;

	if (cmd)
		return FALSE;

	if (!IS_FIGHTING(ch))
		return FALSE;

	if (!GET_OPPONENT(ch))
		return FALSE;

	if (number(0, 24))
		return FALSE;

	/* loop number of PC, pick someone */

	switch (number(0, 9))
	{
	case 0:
	case 1:
		for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
		{
			if (IS_PC(tch) && !IS_TRUSTED(tch))
			{
				if (number(0, 10) >= 5)
				{
					vict = tch;
					act("$n stares at $N as $e utters some uneartly incantations. &n",
					    FALSE, ch, 0, vict, TO_NOTVICT);
					act("$n stares at you while uttering some unearthly incantations. &n",
					    FALSE, ch, 0, vict, TO_VICT);
					spell_chaotic_ripple(60, ch, 0, 0, vict, 0);
				}
			}
		}
		return TRUE;
	case 2:
	case 3:
		act("Your hands glow as you lay your hands on yourself.", FALSE, ch, 0, 0, TO_CHAR);
		act("$n's hands glow as $e lays $s hands on $mself.", FALSE, ch, 0, 0, TO_ROOM);
		GET_HIT(ch) += MIN(GET_LEVEL(ch) * 20, GET_MAX_HIT(ch) - GET_HIT(ch));
		return TRUE;
	case 4:
	case 5:
	{
		const uint64_t actor_runtime_id = ch->runtime_id;
		const int actor_room = ch->in_room;
		const int actor_height = ch->specials.z_cord;

		for (vict = world[actor_room].people; vict; vict = tch)
		{
			tch = vict->next_in_room;

			if (ch == vict)
				continue;

			if (ch->group && vict->group && (ch->group == vict->group))
				continue;

			if (!CAN_SEE(ch, vict))
				continue;

			const uint64_t next_victim_runtime_id = tch ? tch->runtime_id : 0;
			hit(ch, vict, ch->equipment[PRIMARY_WEAPON]);

			ch = find_character_by_runtime_id(actor_runtime_id);
			if (!ch || !IS_ALIVE(ch) || ch->in_room != actor_room ||
			    ch->specials.z_cord != actor_height)
				return TRUE;

			if (next_victim_runtime_id)
			{
				tch = find_character_by_runtime_id(next_victim_runtime_id);
				if (!tch || !is_char_in_room(tch, actor_room))
					return TRUE;
			}
			else
				tch = NULL;
		}
		return TRUE;
	}
	case 6:
	case 7:
		if (!GET_OPPONENT(ch))
			break;

		vict = GET_OPPONENT(ch);

		act("$n &+Ltouches $N&+L, draining his lifeforce and leaving $M&+L collapsed at $s feet.&n",
		    FALSE, ch, 0, vict, TO_NOTVICT);
		act("$n &+Ltouches you. &+WOUCH!!!&n", FALSE, ch, 0, vict, TO_VICT);
		act("&+LYou feed upon $N&+L's blood.&n", FALSE, ch, 0, vict, TO_CHAR);

		GET_HIT(ch) += GET_HIT(vict);
		GET_HIT(vict) = -5;
		GET_VITALITY(vict) = 0;
		GET_MANA(vict) = 0;

		return TRUE;
	case 8:
	case 9:
		act("$n utters a word of power.&n", FALSE, ch, 0, 0, TO_ROOM);
		act("You utter a word of power.&n", FALSE, ch, 0, 0, TO_CHAR);
		spell_shadow_shield(60, ch, 0, 0, ch, 0);
		spell_stone_skin(60, ch, 0, 0, ch, 0);
		spell_biofeedback(60, ch, 0, 0, ch, 0);
		spell_inertial_barrier(60, ch, 0, 0, ch, 0);

		return TRUE;
	default:
		return FALSE;
	}

	return FALSE;
}
int celestia_pulsar(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return FALSE;

	if (cmd)
		return FALSE;

	if (!IS_FIGHTING(ch))
		return FALSE;

	if (!number(0, 3))
	{
		spell_nova(60, ch, NULL, SPELL_TYPE_SPELL, 0, 0);
		return TRUE;
	}

	return FALSE;
}

int transparent_blade(P_obj obj, P_char ch, int cmd, char *arg)
{
	P_char victim;
	struct damage_messages messages = {
		"Your $q &Nbu&n&+Brsts in&+Wto a haz&n&+be of blue light, enveloping $N i&+Bn an ether&+Weal cloud.&n",
		"&+W$n's&N $q bu&n&+Brsts in&+Wto a haz&n&+be of blue light, enveloping you i&+Bn an ether&+Weal cloud.&n",
		"&+W$n's&N $q bu&n&+Brsts in&+Wto a haz&n&+be of blue light, enveloping $N i&+Bn an ether&+Weal cloud.&N",
		"Your $q &Nbu&n&+Brsts in&+Wto a haz&n&+be of blue light, killing $N wi&+Bth an ether&+Weal cloud.&n",
		"&+W$n's&N $q bu&n&+Brsts in&+Wto a haz&n&+be of blue light, your soul is torn directly from your dying body..",
		"&+W$n's&N $q bu&n&+Brsts in&+Wto a haz&n&+be of blue light, killing $N wi&+Bth an ether&+Weal cloud.&N",
		0,
		obj
	};

	if (cmd == CMD_SET_PERIODIC)
	{
		return FALSE;
	}

	if (cmd != CMD_MELEE_HIT || !IS_ALIVE(ch) || !OBJ_WORN(obj) || (obj->loc.wearing != ch))
	{
		return FALSE;
	}
	victim = legacy_proc_arg<P_char>(arg);
	// 1/30 chance.
	if (!IS_ALIVE(victim) || number(0, 29))
	{
		return FALSE;
	}
	spell_damage(ch, victim, 75 * 4, SPLDAM_SPIRIT, SPLDAM_NOSHRUG | SPLDAM_NODEFLECT,
		     &messages);
	return TRUE;
}

int serpent_of_miracles(P_obj obj, P_char ch, int cmd, char *arg)
{
	int rand;
	int curr_time;
	P_char victim = NULL;

	if (cmd == CMD_SET_PERIODIC)
	{
		return TRUE;
	}

	if (!IS_ALIVE(ch) || !OBJ_WORN(obj) || (obj->loc.wearing != ch))
	{
		return FALSE;
	}

	curr_time = time(NULL);
	if (arg && (cmd == CMD_SAY))
	{
		if (isname(arg, "wish"))
		{
			// 2min 40sec timer.
			if (obj->timer[0] + 160 <= curr_time)
			{
				rand = number(0, 7);
				switch (rand)
				{
				case 0:
					act("&n&+WThe writh&n&+Ring serpent of mi&n&+rracles tightens ar&+Wound your a&+Rrm and infuse&n&+rs you with magic.&n",
					    TRUE, ch, obj, victim, TO_CHAR);
					spell_globe(20, ch, 0, 0, ch, 0);
					break;
				case 1:
					act("&n&+WThe writh&n&+Ring serpent of mi&n&+rracles tightens ar&+Wound your a&+Rrm and infuse&n&+rs you with magic.&n",
					    TRUE, ch, obj, victim, TO_CHAR);
					spell_detect_invisibility(20, ch, NULL, SPELL_TYPE_SPELL,
								  ch, 0);
					break;
				case 2:
					act("&n&+WThe writh&n&+Ring serpent of mi&n&+rracles tightens ar&+Wound your a&+Rrm and infuse&n&+rs you with magic.&n",
					    TRUE, ch, obj, victim, TO_CHAR);
					spell_haste(20, ch, 0, 0, ch, 0);
					break;
				case 3:
					act("&n&+WThe writh&n&+Ring serpent of mi&n&+rracles tightens ar&+Wound your a&+Rrm and infuse&n&+rs you with magic.&n",
					    TRUE, ch, obj, victim, TO_CHAR);
					spell_fly(20, ch, NULL, 0, ch, 0);
					break;
				case 4:
					act("&n&+WThe writh&n&+Ring serpent of mi&n&+rracles tightens ar&+Wound your a&+Rrm and ble&n&+rsses you with ho&+Wly power.&n",
					    TRUE, ch, obj, victim, TO_CHAR);
					spell_regeneration(20, ch, NULL, 0, ch, 0);
					break;
				case 5:
					act("&n&+WThe writh&n&+Ring serpent of mi&n&+rracles tightens ar&+Wound your a&+Rrm and ble&n&+rsses you with ho&+Wly power.&n",
					    TRUE, ch, obj, victim, TO_CHAR);
					spell_armor(20, ch, 0, 0, ch, 0);
					break;
				case 6:
					act("&n&+WThe writh&n&+Ring serpent of mi&n&+rracles tightens ar&+Wound your a&+Rrm and ble&n&+rsses you with ho&+Wly power.&n",
					    TRUE, ch, obj, victim, TO_CHAR);
					spell_soulshield(20, ch, 0, 0, ch, 0);
					break;
				case 7:
					act("&n&+WThe writh&n&+Ring serpent of mi&n&+rracles tightens ar&+Wound your a&+Rrm and ble&n&+rsses you with ho&+Wly power.&n",
					    TRUE, ch, obj, victim, TO_CHAR);
					spell_pantherspeed(20, ch, NULL, SPELL_TYPE_SPELL, ch, 0);
					break;
				}
				obj->timer[0] = curr_time;
				return TRUE;
			}
		}
	}
	return FALSE;
}
