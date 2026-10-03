/* Object procedures for Cosmic. */

#include <time.h>

#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utility.h"
#include "core/utils.h"
#include "net/comm.h"
#include "cmd/interp.h"
#include "world/specs.prototypes.h"
#include "magic/spells.h"

int proc_whirlwinds(P_obj obj, P_char ch, int cmd, char *arg)
{
	int curr_time = time(NULL);

	if (cmd == CMD_SET_PERIODIC)
	{
		return TRUE;
	}

	if ((!OBJ_WORN_POS(obj, WEAR_WRIST_R)) && (!OBJ_WORN_POS(obj, WEAR_WRIST_L)) &&
	    (!OBJ_WORN_POS(obj, WEAR_WRIST_LR)) && (!OBJ_WORN_POS(obj, WEAR_WRIST_LL)))
	{
		obj->extra_flags &= ~ITEM_HUM;
		return FALSE;
	}

	if (cmd == CMD_TAP && obj == get_obj_equipped(ch, arg))
	{
		if (!IS_SET(obj->extra_flags, ITEM_HUM))
		{
			send_to_char("Nothing seems to happen.\n", ch);
			return TRUE;
		}
		else
		{
			act("As you tap on $p an immense rush of &+Cvibrating energy&n flows down your limbs!",
			    TRUE, ch, obj, 0, TO_CHAR);
			act("$n performs a slight gesture around $s forearm and suddenly gains &+Cspeed&n and &+Baccuracy&n!",
			    TRUE, ch, obj, 0, TO_ROOM);
			obj->timer[0] = curr_time;
			obj->extra_flags &= ~ITEM_HUM;
			set_short_affected_by(ch, SKILL_WHIRLWIND, 3 * PULSE_VIOLENCE);
			return TRUE;
		}
	}

	if (cmd == CMD_PERIODIC && !IS_SET(obj->extra_flags, ITEM_HUM) &&
	    obj->timer[0] + get_property("timer.proc.bracerOfWhirlwinds", 300) <= curr_time)
	{
		act("Your $q starts vibrating and humming quietly.", TRUE, obj->loc.wearing, obj, 0,
		    TO_CHAR);
		obj->extra_flags |= ITEM_HUM;
	}

	return FALSE;
}
