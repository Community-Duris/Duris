/* Negplane special procedures. */

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "world/db.h"
#include "world/events.h"
#include "cmd/interp.h"
#include "core/utility.h"
#include "core/utils.h"
#include "combat/damage.h"
#include "world/map.h"
#include "world/specs.prototypes.h"
#include "magic/spells.h"
#include "world/vnum.obj.h"
#include "world/vnum.room.h"

extern P_room world;
extern bool has_skin_spell(P_char);

int neg_pocket(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	P_char vict, next;
	int dam;

	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (cmd == CMD_DEATH)
	{ /*
	   * explode upon death
	   */
		act("$n &N&+LEXPLODES, engulfing the area in a dark layer of death!", 0, ch, 0, 0,
		    TO_ROOM);
		for (vict = world[ch->in_room].people; vict; vict = next)
		{
			next = vict->next_in_room;
			if ((ch == vict) || IS_TRUSTED(vict) || IS_NPC(vict))
				continue;

			dam = 250;

			if ((GET_HIT(vict) - dam) < -10)
			{
				act("&+LYou are engulfed into the darkness!&N", FALSE, ch, 0, 0,
				    TO_CHAR);
				act("&+L$n&+L is engulfed completely by the darkness!&N", TRUE, ch,
				    0, 0, TO_ROOM);
				logit(LOG_DEATH, "%s died from neg_pocket() explosion in room %d.",
				      GET_NAME(vict), world[vict->in_room].number);
				die(vict, ch);
			}
			else
				GET_HIT(vict) -= dam;
		}
		return TRUE;
	}
	return FALSE;
}

int transp_tow_misty_gloves(P_obj obj, P_char ch, int cmd, char *argument)
{
	int curr_time;

	if (cmd == CMD_SET_PERIODIC)
	{
		return TRUE;
	}

	if (cmd == CMD_PERIODIC)
	{
		if (OBJ_WORN(obj) && (ch = obj->loc.wearing))
		{
			if (!IS_ALIVE(ch))
			{
				return FALSE;
			}
			curr_time = time(NULL);
			// Every 30 sec.
			if (obj->timer[0] + 30 <= curr_time && !has_skin_spell(ch))
			{
				spell_stone_skin(50, ch, 0, SPELL_TYPE_SPELL, ch, 0);
				obj->timer[0] = curr_time;
				return TRUE;
			}
		}

		hummer(obj);
		return TRUE;
	}

	// If ch not wearing obj on hands.
	if (!IS_ALIVE(ch) || !OBJ_WORN_BY(obj, ch) ||
	    obj->loc.wearing->equipment[WEAR_HANDS] != obj)
	{
		return FALSE;
	}

	if (argument && (cmd == CMD_RUB))
	{
		curr_time = time(NULL);

		act("You rub your gloved hands together.", FALSE, ch, 0, 0, TO_CHAR);
		act("$n rubs $s gloved hands together.", FALSE, ch, 0, 0, TO_ROOM);
		// 2 min timer.
		if (obj->timer[1] + 120 <= curr_time)
		{
			act("Your hands tingle for a brief moment.", FALSE, ch, obj, obj, TO_CHAR);

			spell_invisibility(55, ch, 0, SPELL_TYPE_SPELL, ch, 0);
			SET_BIT(ch->specials.affected_by, AFF_HIDE);

			obj->timer[1] = curr_time;
			return TRUE;
		}
	}

	return FALSE;
}

int neg_orb(P_obj obj, P_char ch, int cmd, char *arg)
{
	P_char victim;
	struct damage_messages messages = {
		"&+WYour&N $q &+Wvibrates as a tendril of &+Lnegative energy &+Wsnakes out toward $N!&n",
		"&+W$n's&N $q &+Wvibrates as a tendril of &+Lnegative energy &+Wsnakes out toward you!&n",
		"&+W$n's&N $q &+Wvibrates as a tendril of &+Lnegative energy &+Wsnakes out toward $N!&n",
		"&+WYour&N $q &+Wvibrates as a tendril of &+Lnegative energy &+Wsnakes out and slays $N!&n",
		"&+W$n's&N $q &+Wvibrates as a tendril of &+Lnegative energy &+Wsnakes out toward you claiming your life!&n",
		"&+W$n's&N $q &+Wvibrates as a tendril of &+Lnegative energy &+Wsnakes out and slays $N!&n",
		0
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

	messages.obj = obj;
	spell_damage(ch, victim, 75 * 4, SPLDAM_GENERIC, SPLDAM_NOSHRUG | SPLDAM_NODEFLECT,
		     &messages);
	return TRUE;
}

int sanguine(P_obj obj, P_char ch, int cmd, char *argument)
{
	char *arg;
	int curr_time;

	if (cmd == CMD_SET_PERIODIC)
	{
		return FALSE;
	}

	if (!IS_ALIVE(ch) || !obj || !OBJ_WORN(obj))
	{
		return FALSE;
	}

	// Any powers activated by keywords? Right here, bud.
	if (argument && (cmd == CMD_SAY))
	{
		arg = argument;

		while (*arg == ' ')
		{
			arg++;
		}

		if (!strcmp(arg, "sanguine"))
		{
			if (!say(ch, arg))
			{
				return TRUE;
			}
			curr_time = time(NULL);

			// 5 min timer.
			if (obj->timer[0] + 300 <= curr_time)
			{
				act("$n's $q hums briefly.", TRUE, ch, obj, NULL, TO_ROOM);
				spell_armor(50, ch, 0, SPELL_TYPE_SPELL, ch, 0);
				spell_bless(50, ch, 0, SPELL_TYPE_SPELL, ch, 0);

				obj->timer[0] = curr_time;
			}
			return TRUE;
		}
	}
	return FALSE;
}

int orb_of_destruction(P_obj obj, P_char ch, int cmd, char *arg)
{
	P_char victim;

	if (cmd == CMD_SET_PERIODIC)
	{
		return FALSE;
	}

	if (cmd != CMD_MELEE_HIT || !IS_ALIVE(ch) || !OBJ_WORN(obj) || (obj->loc.wearing != ch))
	{
		return FALSE;
	}

	// 1/16 chance.
	if (!(victim = legacy_proc_arg<P_char>(arg)) || number(0, 15))
	{
		return FALSE;
	}

	act("&+L$n's&N $q &n&+bdraws &+Benergy&n&+b from the surroundings and &+LBLASTS $N!&N",
	    TRUE, ch, obj, victim, TO_NOTVICT);
	act("&+LYour&N $q &n&+bdraws &+Benergy&n&+b from the surroundings and &+LBLASTS $N!&N",
	    TRUE, ch, obj, victim, TO_CHAR);
	act("&+L$n's&N $q &n&+bdraws &+Benergy&n&+b from the surroundings and &+LBLASTS YOU!&N",
	    TRUE, ch, obj, victim, TO_VICT);
	spell_disintegrate(40, ch, NULL, SPELL_TYPE_SPELL, victim, obj);
	return TRUE;
}
