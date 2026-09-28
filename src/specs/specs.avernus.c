/* Avernus object procedures. */

#include <time.h>

#include "core/prototypes.h"
#include "combat/defense_resolution.h"
#include "core/structs.h"
#include "net/comm.h"
#include "world/db.h"
#include "world/events.h"
#include "cmd/interp.h"
#include "core/utility.h"
#include "core/utils.h"
#include "world/specs.prototypes.h"
#include "magic/spells.h"

extern P_room world;

int sinister_tactics_staff(P_obj obj, P_char ch, int cmd, char *arg)
{
	int dam = cmd / 1000;
	P_char victim;

	if (cmd == CMD_SET_PERIODIC)
	{
		return FALSE;
	}

	if (!dam || !IS_ALIVE(ch) || !OBJ_WORN(obj) || (obj->loc.wearing != ch))
	{
		return FALSE;
	}
	if (!(victim = legacy_proc_arg<P_char>(arg)))
	{
		return FALSE;
	}
	// 1/25 chance.
	if (!IS_ALIVE(victim) || number(0, 24))
	{
		return FALSE;
	}

	act("&+WYour $p &+Br&+be&+Bs&+bo&+Bn&+ba&+Bt&+be&+Bs &+Wwith the scream of &+Lt&+ror&+Rtu&+rre&+Ld &+csouls&+W!&N",
	    TRUE, ch, obj, victim, TO_CHAR);
	act("&+W$n's $p &+Br&+be&+Bs&+bo&+Bn&+ba&+Bt&+be&+Bs &+Was it strikes $N.", TRUE, ch, obj,
	    victim, TO_NOTVICT);
	act("&+WYour head is filled with the scream of &+Lt&+ror&+Rtu&+rre&+Ld &+csouls&+W!&N",
	    TRUE, ch, obj, victim, TO_VICT);
	if (!fear_check(victim))
	{
		do_flee(victim, 0, 2);
	}
	return TRUE;
}

int staff_shadow_summoning(P_obj obj, P_char ch, int cmd, char *arg)
{
	int curr_time;
	P_char shadow;
	int sum;
	bool summoned = FALSE;
	bool aggsummoned = FALSE;

	if (cmd == CMD_SET_PERIODIC)
	{
		return FALSE;
	}

	if (!IS_ALIVE(ch) || !OBJ_WORN(obj))
		return FALSE;

	if (arg && (cmd == CMD_TAP))
	{
		if (isname(arg, "staff"))
		{
			if (IS_FIGHTING(ch))
			{
				send_to_char(
					"You're too busy fighting for your life to worry about that.\n",
					ch);
				return FALSE;
			}
			curr_time = time(NULL);
			if (!IS_TRUSTED(ch))
			{
				if (obj->value[7] > 100)
				{
					obj->value[7] = 100;
				}

				if (obj->value[7] > 0)
				{
					// Every 12 min.
					if (obj->timer[0] + (60 * 12) <= curr_time)
					{
						obj->value[7]--;
						shadow = read_mobile(92076, VIRTUAL);
						act("&+LYou tap your $q&+L on the ground... Shadows quickly engulf the area.&N",
						    FALSE, ch, obj, obj, TO_CHAR);
						act("&+LAs $n taps $s $q&+L on the ground..... Shadows quickly engulf the area.&N",
						    FALSE, ch, obj, obj, TO_ROOM);
						summoned = TRUE;
					}
					else
					{
						act("&+LAs you tap your $q&+L..... Nothing Happens!&N",
						    FALSE, ch, obj, obj, TO_CHAR);
						act("&+LAs $n taps $s $q&+L..... Nothing Happens!&N",
						    FALSE, ch, obj, obj, TO_ROOM);
						return TRUE;
					}
				}
				// Every 15 sec .. oh aggro.
				else if (obj->timer[0] + 15 <= curr_time)
				{
					obj->value[7]--;
					shadow = read_mobile(200, VIRTUAL);
					act("&+LYou tap your $q&+L on the ground... Shadows quickly engulf the area.&N",
					    FALSE, ch, obj, obj, TO_CHAR);
					act("&+LAs $n taps $s $q&+L on the ground..... Shadows quickly engulf the area.&N",
					    FALSE, ch, obj, obj, TO_ROOM);
					summoned = TRUE;
					aggsummoned = TRUE;
				}
				else
				{
					act("&+LAs you tap your $q&+L..... Nothing Happens!&N",
					    FALSE, ch, obj, obj, TO_CHAR);
					act("&+LAs $n taps $s $q&+L..... Nothing Happens!&N", FALSE,
					    ch, obj, obj, TO_ROOM);
					return TRUE;
				}
			}
			else
			{
				shadow = read_mobile(92076, VIRTUAL);
				act("&+LYou tap your $q &+Lon the ground... Shadows quickly engulf the area.&N",
				    FALSE, ch, obj, obj, TO_CHAR);
				act("&+LAs $n taps $s $q &+Lon the ground..... Shadows quickly engulf the area.&N",
				    FALSE, ch, obj, obj, TO_ROOM);
				summoned = TRUE;
			}

			if (summoned)
			{
				if (!shadow)
				{
					act("&+WTHERE IS NO SHADOW, TELL A GOD!!&N", FALSE, ch, obj,
					    obj, TO_CHAR);
					return FALSE;
				}
				char_to_room(shadow, ch->in_room, 0);

				GET_SIZE(shadow) = SIZE_MEDIUM;
				shadow->player.m_class = CLASS_WARRIOR;
				shadow->player.level = 40;
				sum = dice(GET_LEVEL(shadow) * 4, 8) + (GET_LEVEL(shadow) * 3);
				while (shadow->affected)
				{
					affect_remove(shadow, shadow->affected);
				}
				if (!IS_SET(shadow->specials.act, ACT_MEMORY))
				{
					clearMemory(shadow);
				}
				SET_BIT(shadow->specials.affected_by, AFF_INFRAVISION);
				remove_plushit_bits(shadow);
				GET_MAX_HIT(shadow) = GET_HIT(shadow) = shadow->points.base_hit =
					sum;
				shadow->points.base_hitroll = shadow->points.hitroll =
					GET_LEVEL(shadow) / 3;
				shadow->points.base_damroll = shadow->points.damroll =
					GET_LEVEL(shadow) / 3;
				/* This does nothing because shadow is not a Monk.
				        MonkSetSpecialDie(shadow);
				*/
				GET_EXP(shadow) = 0;
				if (!aggsummoned)
				{
					REMOVE_BIT(shadow->only.npc->aggro_flags, AGGR_ALL);
					balance_affects(shadow);
					setup_pet(shadow, ch, 1500, PET_NOCASH);
					add_follower(shadow, ch);
				}
				obj->timer[0] = curr_time;
				if (obj->value[7] < 0)
				{
					obj->value[7] = 0;
				}
				return TRUE;
			}
		}
	}
	return FALSE;
}

int shard_frozen_styx_water(P_obj obj, P_char ch, int cmd, char *arg)
{
	int dam = cmd / 1000;
	P_char victim;

	if (cmd == CMD_SET_PERIODIC)
	{
		return FALSE;
	}

	if (!dam || !IS_ALIVE(ch) || !OBJ_WORN(obj) || (obj->loc.wearing != ch))
	{
		return FALSE;
	}
	if (!(victim = legacy_proc_arg<P_char>(arg)))
	{
		return FALSE;
	}
	// 1/25 chance.
	if (!IS_ALIVE(victim) || number(0, 24))
	{
		return FALSE;
	}

	act("&+LYour $p &+Lgets &+Cice &+Wcold&+L causing $N&+L's head to &+Bfreeze&+L!&N", TRUE,
	    ch, obj, victim, TO_CHAR);
	act("&+L$n's $p &+Lgets &+Cice &+Wcold&+L causing $N&+L's head to &+Bfreeze&+L!&N", TRUE,
	    ch, obj, victim, TO_NOTVICT);
	act("&+L$n's $p &+Lgets &+Cice &+Wcold&+L causing your head to &+Bfreeze&+L!&N", TRUE, ch,
	    obj, victim, TO_VICT);
	spell_feeblemind(35, ch, NULL, 0, victim, 0);
	return (TRUE);
}
