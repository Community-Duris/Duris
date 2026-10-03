/* Tikitt special procedures. */

#include <time.h>

#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "world/db.h"
#include "world/events.h"
#include "cmd/interp.h"
#include "core/utility.h"
#include "core/utils.h"
#include "combat/attack_continuation.h"
#include "combat/damage.h"
#include "magic/spells.h"
#include "world/specs.prototypes.h"

void unmulti(P_char ch, P_obj obj);

int madman_shield(P_obj obj, P_char ch, int cmd, char *arg)
{
	struct proc_data *data;
	P_char vict;

	if (cmd == CMD_SET_PERIODIC)
	{
		return FALSE;
	}

	// 1/10 chance.
	if (cmd != CMD_GOTHIT || number(0, 9))
	{
		return FALSE;
	}

	// important! can do this cast (next line) ONLY if cmd was CMD_GOTHIT or CMD_GOTNUKED
	if (!(data = legacy_proc_arg<struct proc_data *>(arg)))
	{
		return FALSE;
	}
	if (!(vict = data->victim))
	{
		return FALSE;
	}

	act("$N is burned by magical fire as he strikes the $q!", FALSE, ch, obj, vict, TO_NOTVICT);
	act("Your $q pulsates with &+Wmagical fire&n burning $N!", FALSE, ch, obj, vict, TO_CHAR);
	act("$n's $q flares up and burns you with magical fire.", FALSE, ch, obj, vict, TO_VICT);
	damage(ch, vict, (40 + number(0, 40)), TYPE_UNDEFINED);
	return TRUE;
}

int madman_mangler(P_obj obj, P_char ch, int cmd, char *arg)
{
	int ripostes;
	P_char victim;
	struct proc_data *data;

	if (cmd == CMD_SET_PERIODIC)
	{
		return FALSE;
	}

	if (!char_in_list(ch) || !IS_ALIVE(ch) || !OBJ_WORN(obj) || (obj->loc.wearing != ch))
	{
		return FALSE;
	}

	if (cmd == CMD_REMOVE && isname(arg, obj->name))
	{
		affect_from_char(ch, SPELL_BLUR);
		REMOVE_BIT(ch->specials.affected_by3, AFF3_BLUR);
		return FALSE;
	}

	// 1/20 chance.
	if (cmd == CMD_GOTHIT && !number(0, 19))
	{
		if (!(data = legacy_proc_arg<struct proc_data *>(arg)))
		{
			return FALSE;
		}
		victim = data->victim;
		if (!victim || !char_in_list(victim) || !IS_ALIVE(victim))
		{
			return FALSE;
		}

		if (number(0, 1))
		{
			ripostes = 1;
		}
		else if (number(0, 4) >= 2)
		{
			ripostes = 2;
		}
		else
		{
			ripostes = 3;
		}

		act("$n's $q deflects $N's blow and strikes $M!", TRUE, ch, obj, victim,
		    TO_NOTVICT | ACT_NOTTERSE);
		act("$n's $q deflects your blow and strikes YOU!", TRUE, ch, obj, victim,
		    TO_VICT | ACT_NOTTERSE);
		act("Your $q deflects $N's blow and strikes $M!", TRUE, ch, obj, victim,
		    TO_CHAR | ACT_NOTTERSE);
		do
		{
			const attack_continuation continuation =
				begin_attack_continuation(ch, victim, obj);
			hit(ch, victim, obj);
			const attack_continuation_result after_hit =
				check_attack_continuation(continuation);
			if (!after_hit.can_continue() || !OBJ_WORN(after_hit.weapon) ||
			    after_hit.weapon->loc.wearing != after_hit.actor)
				break;
			ch = after_hit.actor;
			victim = after_hit.target;
			obj = after_hit.weapon;
		} while (--ripostes && char_in_list(victim) && char_in_list(ch));

		return TRUE;
	}

	if (cmd != CMD_MELEE_HIT)
	{
		return FALSE;
	}
	victim = legacy_proc_arg<P_char>(arg);
	// 1/30 chance.
	if (!IS_ALIVE(victim) || number(0, 29))
	{
		return FALSE;
	}
	act("&+L$n's&N $q &n&+Lgoes &+rmad...&N", TRUE, ch, obj, victim, TO_NOTVICT);
	act("&+LYour&N $q &n&+Lgoes &+rmad...&N", TRUE, ch, obj, victim, TO_CHAR);
	act("&+L$n's&N $q &n&+Lgoes &+rmad...&N", TRUE, ch, obj, victim, TO_VICT);
	spell_blur(60, ch, 0, 0, ch, 0);
	return TRUE;
}

int unmulti_altar(P_obj obj, P_char ch, int cmd, char *arg)
{
	if (cmd != CMD_PRAY || !ch)
		return FALSE;

	if (!strstr(arg, "altar"))
		return FALSE;

	if (!IS_MULTICLASS_PC(ch))
	{
		send_to_char("&+WThe Gods ignore your prayers!\n", ch);
		return TRUE;
	}

	unmulti(ch, obj);
	return TRUE;
}

void event_mentality_mace_vibrate(P_char /*ch*/, P_char /*victim*/, P_obj obj, void * /*data*/)
{
	if (OBJ_WORN(obj) && obj->loc.wearing)
	{
		act("$p vibrates softly.", FALSE, obj->loc.wearing, obj, 0, TO_CHAR);
	}
}

int mentality_mace(P_obj obj, P_char ch, int cmd, char *arg)
{
	int curr_time;
	struct follow_type *k, *p;

	if (cmd == CMD_SET_PERIODIC)
	{
		return FALSE;
	}

	if (!IS_ALIVE(ch) || !OBJ_WORN(obj) || !OBJ_WORN_BY(obj, ch))
	{
		return FALSE;
	}

	if (arg && (cmd == CMD_REMOVE))
	{
		if (isname(arg, obj->name) || isname(arg, "all"))
		{
			for (k = ch->followers; k; k = p)
			{
				P_char tch = k->follower;
				p = k->next;
				if (tch && IS_NPC(tch) && GET_VNUM(tch) == 250)
				{
					stop_fighting(tch);
					StopAllAttackers(tch);
					extract_char(tch);
				}
			}
		}
	}

	if (arg && (cmd == CMD_SAY))
	{
		if (isname(arg, "mentality"))
		{
			curr_time = time(NULL);
			// 5 min timer.
			if (IS_TRUSTED(ch) || (obj->timer[0] + 300 <= curr_time))
			{
				act("$p hums loudly, and shatters your psyche!", FALSE, ch, obj, 0,
				    TO_CHAR);
				act("$p hums loudly, and shatters $n's psyche!", FALSE, ch, obj, 0,
				    TO_ROOM);

				spell_reflection(50, ch, writable_arg(""), 0, ch, 0);

				obj->timer[0] = curr_time;

				disarm_obj_nevents(obj, event_mentality_mace_vibrate);
				add_event(event_mentality_mace_vibrate, 300 * WAIT_SEC, 0, 0, obj,
					  0, 0, 0);

				return TRUE;
			}
			else
			{
				act("$p hums loudly, giving you a splitting headache!", FALSE, ch,
				    obj, 0, TO_CHAR);
				act("$p hums loudly, and $n looks pained for a second.", FALSE, ch,
				    obj, 0, TO_ROOM);

				spell_damage(ch, ch, 100, SPLDAM_GENERIC,
					     SPLDAM_NODEFLECT | SPLDAM_NOSHRUG | RAWDAM_NOKILL, 0);
				return TRUE;
			}
		}
	}

	return FALSE;
}
