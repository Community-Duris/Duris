/* Aracguil special procedures. */

#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utils.h"
#include "net/comm.h"
#include "cmd/interp.h"
#include "world/db.h"
#include "world/events.h"
#include "world/handler.h"
#include "world/specs.prototypes.h"
#include "combat/damage.h"
#include "classes/reavers.h"
#include "magic/spells.h"

int rod_of_zarbon(P_obj obj, P_char ch, int cmd, char *arg)
{
	P_char vict;
	P_obj curse;
	char e_pos;
	int bad_owner;
	int dam = (dice(8, 10) * 6);
	struct damage_messages messages = {
		"&+L$N &+Lturns pale as your $q &+Ldrains $S lifeforce, transferring it to you!&n",
		"&+LYour soul feels hollow, as the power of $n&+L's $q&+L saps your lifeforce!&n",
		"&+L$N &+Lscreams out in pain, as $S lifeforce is drained by $n&+L!&n",
		"",
		"",
		"",
		0,
		obj
	};

	vict = legacy_proc_arg<P_char>(arg);

	if (cmd == CMD_SET_PERIODIC)
	{
		return FALSE;
	}

	if (!IS_ALIVE(ch) || !OBJ_WORN(obj) || cmd != CMD_MELEE_HIT)
	{
		return FALSE;
	}

	/* Check class and sex for the extra special stuff */
	if (!GET_CLASS(ch, CLASS_CLERIC) || (GET_SEX(ch) != SEX_FEMALE) ||
	    GET_RACE(ch) != RACE_DROW)
	{
		bad_owner = TRUE;
	}
	else
	{
		bad_owner = FALSE;
	}

	e_pos = ((obj->loc.wearing->equipment[WIELD] == obj)		? WIELD :
		 (obj->loc.wearing->equipment[SECONDARY_WEAPON] == obj) ? SECONDARY_WEAPON :
									  0);

	/* must be wielded */
	if (!e_pos)
	{
		return FALSE;
	}

	/* The extra-special fancy-dancy power */
	// 1/100 chance.
	if (!bad_owner && !number(0, 99))
	{
		curse = read_object(36761, VIRTUAL);
		if (curse)
		{
			obj_to_char(curse, vict);
		}
	}
	/* If we scored a _really_ nice hit, let's do some spell shit */
	else if (CheckMultiProcTiming(ch) && !number(0, 32)) // 3%
	{
		act("Your $q flares up upon hitting $N!", FALSE, ch, obj, vict, TO_CHAR);
		act("$n's $q flares up upon hitting $N!", FALSE, ch, obj, vict, TO_NOTVICT);
		act("$n's $q flares up upon hitting you!", FALSE, ch, obj, vict, TO_VICT);

		spell_damage(ch, vict, dam, SPLDAM_NEGATIVE,
			     SPLDAM_NOSHRUG | SPLDAM_NODEFLECT | RAWDAM_NOKILL, &messages);

		vamp(ch, dam / 4, (int)(GET_MAX_HIT(ch) * VAMPPERCENT(ch)));

		if (GET_VITALITY(vict) >= 25 && !number(0, 2))
		{
			act("&+rYour $q &+mFLARES&n&+r, tapping the vigor of $N&+r!&n", FALSE, ch,
			    obj, vict, TO_CHAR);
			act("&+r$n&+r's $q &+mFLARES&n&+r, draining $N&+r's vigor!&n", FALSE, ch,
			    obj, vict, TO_NOTVICT);
			act("&+r$n&+r's $q &+mFLARES&n&+r, draining your vigor!&n", FALSE, ch, obj,
			    vict, TO_VICT);

			GET_VITALITY(vict) -= (dam / 9);
			GET_VITALITY(ch) += (dam / 9);

			StartRegen(ch, regen_resource::vitality);
			StartRegen(vict, regen_resource::vitality);
		}
	}
	return TRUE;
}
