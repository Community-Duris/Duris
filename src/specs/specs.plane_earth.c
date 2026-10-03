/* Plane Earth equipment procedures. */

#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "world/events.h"
#include "cmd/interp.h"
#include "core/utility.h"
#include "core/utils.h"
#include "combat/damage.h"
#include "world/specs.prototypes.h"
#include "magic/spells.h"

int earthquake_gauntlet(P_obj obj, P_char ch, int cmd, char * /*arg*/)
{
	P_char vict = NULL;

	if (cmd == CMD_SET_PERIODIC)
	{
		return TRUE;
	}

	if (!OBJ_WORN_POS(obj, WEAR_HANDS) || cmd != CMD_PERIODIC || ch)
	{
		return FALSE;
	}
	ch = obj->loc.wearing;
	if (!IS_ALIVE(ch))
	{
		return FALSE;
	}

	// 1/10 chance
	if (IS_FIGHTING(ch) && !number(0, 9))
	{
		act("&+y$n's $q &+ydrive into the ground causing the earth to buckle and break!&N",
		    TRUE, ch, obj, vict, TO_ROOM);
		act("&+yYour $q &+ydrive into the ground causing the earth to buckle and break!&N",
		    TRUE, ch, obj, vict, TO_CHAR);
		spell_earthquake(50, ch, 0, SPELL_TYPE_SPELL, vict, 0);
		return TRUE;
	}

	return FALSE;
}

int blind_boots(P_obj obj, P_char ch, int cmd, char * /*arg*/)
{
	P_char vict = NULL;

	if (cmd == CMD_SET_PERIODIC)
	{
		return TRUE;
	}

	if (!OBJ_WORN_POS(obj, WEAR_FEET) || ch || cmd != CMD_PERIODIC)
	{
		return FALSE;
	}
	ch = obj->loc.wearing;
	if (!IS_ALIVE(ch))
	{
		return FALSE;
	}

	// 1/25 chance == 4%
	if (IS_FIGHTING(ch) && !number(0, 24))
	{
		vict = GET_OPPONENT(ch);

		if (affected_by_spell(vict, SPELL_BLINDNESS) || (GET_RACE(vict) == RACE_DRAGON) ||
		    (GET_RACE(vict) == RACE_DEMON) || (GET_RACE(vict) == RACE_DEVIL) ||
		    IS_IMMATERIAL(vict) || IS_ELEMENTAL(vict) || EYELESS(vict) || IS_ELITE(vict))
		{
			return TRUE;
		}

		act("&+y$n &+ykicks up a cloud of dust, obscuring your vision!&N", TRUE, ch, obj,
		    vict, TO_VICT);
		act("&+y$n &+ykicks up a cloud of dust at $N.&N", TRUE, ch, obj, vict, TO_NOTVICT);
		act("&+yYou &+ykick up a cloud of dust at $N!&N", TRUE, ch, obj, vict, TO_CHAR);
		blind(ch, vict, number(2, 10) * WAIT_SEC);
		return TRUE;
	}

	return FALSE;
}
