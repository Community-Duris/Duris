/* Nyneth special procedures. */

#include <time.h>
#include <string.h>

#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "world/db.h"
#include "world/events.h"
#include "cmd/interp.h"
#include "core/utils.h"
#include "combat/damage.h"
#include "combat/justice.h"
#include "world/specs.prototypes.h"
#include "magic/spells.h"
#include "world/weather.h"
#include "world/bloodstains.h"

extern P_room world;

int construct(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	P_char vict = NULL;

	if (cmd == CMD_SET_PERIODIC)
		return FALSE;

	if (cmd)
		return FALSE;

	if (!ch)
		return FALSE;

	// Do it 50 % of the time
	if (number(1, 100) < 50)
		return FALSE;

	if (ch->in_room == NOWHERE)
		return FALSE;
	if (!GET_OPPONENT(ch))
		return FALSE;
	for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
	{
		if (ch->group && vict->group && (ch->group == vict->group))
			continue;
		if (IS_TRUSTED(ch) || (ch == vict))
			continue;
		else
		{
			if ((number(0, (SIZE_GARGANTUAN + 2)) - 2) > GET_ALT_SIZE(vict))
			{
				act("$n&+L picks up $N &+Land tosses $M &+Lagainst the wall!&n",
				    FALSE, ch, 0, vict, TO_NOTVICT);
				act("$n&+L picks you up and tosses you against the wall!&n", FALSE,
				    ch, 0, vict, TO_VICT);
				SET_POS(vict, POS_PRONE + GET_STAT(vict));
				stop_fighting(vict);
				CharWait(vict, PULSE_VIOLENCE);
			}
		}
	}
	return TRUE;
}
int nyneth(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	P_char fury;

	if (cmd == CMD_SET_PERIODIC)
		return FALSE;

	if (cmd)
		return FALSE;

	if (!ch)
		return FALSE;

	if (GET_OPPONENT(ch))
	{
		if (number(1, 100) < 10)
		{
			fury = read_mobile(38735, VIRTUAL);
			if (!fury)
			{
				logit(LOG_EXIT, "assert: error in nyneth proc");
				return FALSE;
			}
			act("A &+rFuRy&n enters from somewhere.\r\n", FALSE, ch, 0, fury, TO_ROOM);
			char_to_room(fury, ch->in_room, 0);
			return TRUE;
		}
	}
	return FALSE;
}
int hammer_titans(P_obj obj, P_char ch, int cmd, char *arg)
{
	int dam = cmd / 1000;
	P_char victim;

	if (cmd == CMD_SET_PERIODIC)
	{
		return FALSE;
	}

	if (!dam || !IS_ALIVE(ch) || !OBJ_WORN_BY(obj, ch))
	{
		return FALSE;
	}
	victim = legacy_proc_arg<P_char>(arg);
	// 1/30 chance.
	if (!IS_ALIVE(victim) || number(0, 29))
	{
		return FALSE;
	}

	act("&+W$n's&N $q &+Bsurges with electricity!&N", TRUE, ch, obj, victim, TO_NOTVICT);
	act("&+WYour&N $q &+Bsurges with electricity!&N", TRUE, ch, obj, victim, TO_CHAR);
	act("&+W$n's&N $q &+Bsurges with electricity!&N", TRUE, ch, obj, victim, TO_VICT);
	spell_lightning_bolt(60, ch, 0, SPELL_TYPE_SPELL, victim, 0);
	if (char_in_list(victim))
	{
		spell_lightning_bolt(60, ch, 0, SPELL_TYPE_SPELL, victim, 0);
	}
	return TRUE;
}
int stormbringer(P_obj obj, P_char ch, int cmd, char *arg)
{
	int dam = cmd / 1000;
	P_char victim;

	if (cmd == CMD_SET_PERIODIC)
	{
		return FALSE;
	}

	victim = legacy_proc_arg<P_char>(arg);
	// 1/30 chance
	if (!dam || !IS_ALIVE(ch) || !IS_ALIVE(victim) || !OBJ_WORN_BY(obj, ch) || number(0, 29))
	{
		return FALSE;
	}

	act("&+W$n's&N $q &+Bsummons up a storm!&N", TRUE, ch, obj, victim, TO_NOTVICT);
	act("&+WYour&N $q &+Bsummons up a storm!!&N", TRUE, ch, obj, victim, TO_CHAR);
	act("&+W$n's&N $q &+Wsummons up a storm!!&N", TRUE, ch, obj, victim, TO_VICT);
	/*act("&+cA &+Wforce&+c of &+Cwinds&+c throws you backwards.&n", TRUE, ch,
	    obj, victim, TO_VICT);
	act("&+cA &+Wforce&+c of &+Cwinds&+c throws $N backwards.&n", TRUE, ch, obj,
	    victim, TO_CHAR);
	act("&+cA &+Wforce&+c of &+Cwinds&+c throws $n backwards.&n", TRUE, victim,
	    obj, ch, TO_NOTVICT);
	Stun(victim, ch, (dice(1, 2) * PULSE_VIOLENCE), FALSE);*/

	spell_cyclone(30, ch, 0, SPELL_TYPE_SPELL, victim, 0);
	if (!IS_ALIVE(ch) || !IS_ALIVE(victim))
	{
		return TRUE;
	}

	switch (world[ch->in_room].sector_type)
	{
	case SECT_AIR_PLANE:
		act("&+cThe energies of the &+CAir Plane&+c fill $q &+cwith power!&n", TRUE, ch,
		    obj, victim, TO_VICT);
		act("&+cThe energies of the &+CAir Plane&+c fill $q &+cwith power!&n", TRUE, ch,
		    obj, victim, TO_CHAR);
		act("&+cThe energies of the &+CAir Plane&+c fill $q &+cwith power!&n", TRUE, victim,
		    obj, ch, TO_NOTVICT);
		spell_chain_lightning(50, ch, 0, SPELL_TYPE_SPELL, victim, 0);
		break;
	case SECT_FIELD:
	case SECT_HILLS:
	case SECT_FOREST:
		act("&+LA powerful thundercloud appears on the horizon.&n", TRUE, ch, obj, victim,
		    TO_VICT);
		act("&+LA powerful thundercloud appears on the horizon.&n", TRUE, ch, obj, victim,
		    TO_CHAR);
		act("&+LA powerful thundercloud appears on the horizon.&n", TRUE, victim, obj, ch,
		    TO_NOTVICT);
		cast_call_lightning(50, ch, 0, SPELL_TYPE_SPELL, victim, 0);
		break;
	case SECT_DESERT:
		act("&+rA great &+Rhot &+rsandstorm blows in on the horizon...&n", TRUE, ch, obj,
		    victim, TO_VICT);
		act("&+rA great &+Rhot &+rsandstorm blows in on the horizon...&n", TRUE, ch, obj,
		    victim, TO_CHAR);
		act("&+rA great &+Rhot &+rsandstorm blows in on the horizon...&n", TRUE, victim,
		    obj, ch, TO_NOTVICT);
		spell_firestorm(50, ch, 0, SPELL_TYPE_SPELL, victim, 0);
		break;
	}
	return TRUE;
}

int ring_of_regeneration(P_obj obj, P_char ch, int cmd, char *arg)
{
	int curr_time = time(NULL);
	char first_arg[256];

	if (cmd == CMD_SET_PERIODIC)
	{
		return TRUE;
	}

	if (!OBJ_WORN_POS(obj, WEAR_FINGER_R) && !OBJ_WORN_POS(obj, WEAR_FINGER_L))
	{
		return FALSE;
	}

	one_argument(arg, first_arg);

	if (cmd == CMD_RUB && !strcmp("ring", first_arg))
	{
		if (!IS_SET(obj->extra_flags, ITEM_GLOW))
		{
			return FALSE;
		}
		else
		{
			act("$n's $q &nhums softly.", TRUE, ch, obj, 0, TO_VICT);
			act("$n's $q &nhums softly.", TRUE, ch, obj, 0, TO_NOTVICT);
			act("Your $q &nhums softly.", TRUE, ch, obj, 0, TO_CHAR);
			obj->timer[0] = curr_time;
			obj->extra_flags &= ~ITEM_GLOW;
			spell_heal(60, ch, 0, 0, ch, 0);
			return TRUE;
		}
	}

	if (cmd == CMD_PERIODIC && !IS_SET(obj->extra_flags, ITEM_GLOW) &&
	    obj->timer[0] + get_property("timer.proc.ringOfRegeneration", 150) <= curr_time)
	{
		act("Your $q &+Wglows&n with a soft light.", TRUE, obj->loc.wearing, obj, 0,
		    TO_CHAR);
		obj->extra_flags |= ITEM_GLOW;
	}

	return FALSE;
}

void event_lifereaver(P_char ch, P_char /*victim*/, P_obj /*obj*/, void *data)
{
	int dam;
	int count = *((int *)data);

	dam = dice(3, 6);

	if ((GET_HIT(ch) - dam) > 0)
	{
		GET_HIT(ch) -= dam;
		send_to_char("&+RYour wound bleeds openly!&N\n", ch);
		act("$n&+R bleeds all over the place.&N", TRUE, ch, NULL, NULL, TO_NOTVICT);
		make_bloodstain(ch);
	}

	if (count >= 0)
	{
		count--;
		add_event(event_lifereaver, PULSE_VIOLENCE, ch, 0, 0, 0, &count, sizeof(count));
	}
	else
	{
		send_to_char("&+rThe bleeding appears to have stopped.&N\n", ch);
	}
}

int lifereaver(P_obj obj, P_char ch, int cmd, char * /*arg*/)
{
	P_char vict;
	int numb;

	if (cmd == CMD_SET_PERIODIC)
	{
		return TRUE;
	}

	if (!OBJ_WORN(obj))
	{
		return FALSE;
	}
	if (OBJ_WORN(obj))
		ch = obj->loc.wearing;
	else if (OBJ_CARRIED(obj))
		ch = obj->loc.carrying;
	else
		return FALSE;

	if (ch == NULL)
		return FALSE;

	if (cmd)
	{
		return FALSE;
	}
	else if (IS_FIGHTING(ch))
	{
		if (!number(0, 3) && !IS_UNDEADRACE(GET_OPPONENT(ch)))
		{
			vict = GET_OPPONENT(ch);
			act("&+LYour $q &+Lslices into $N&+L, making him &+rBLEED&+L!&N", TRUE, ch,
			    obj, vict, TO_CHAR);
			act("&+L$ns $q &+Lslices into $N&+L, making him &+rBLEED&+L!&N", TRUE, ch,
			    obj, vict, TO_NOTVICT);
			act("&+L&ns $q &+Lslices into you, making you &+rBLEED&+L!&N", TRUE, ch,
			    obj, vict, TO_VICT);
			numb = number(4, 7);
			add_event(event_lifereaver, PULSE_VIOLENCE, vict, 0, 0, 0, &numb,
				  sizeof(numb));
			return TRUE;
		}
	}
	return FALSE;
}
