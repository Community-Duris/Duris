/* Dragon Pit equipment procedures. */

#include <time.h>

#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "world/db.h"
#include "world/events.h"
#include "cmd/interp.h"
#include "combat/damage.h"
#include "core/utility.h"
#include "core/utils.h"
#include "world/specs.prototypes.h"
#include "magic/spells.h"

extern P_room world;

extern void bard_dragons(int, P_char, P_char, int);
int mace_dragondeath(P_obj obj, P_char /*ch*/, int cmd, char *arg)
{
	P_char temp_ch;
	P_char vict;
	int dam, curr_time;

	if (cmd == CMD_SET_PERIODIC)
	{
		return TRUE;
	}

	if (cmd != CMD_PERIODIC && cmd != CMD_MELEE_HIT)
	{
		return FALSE;
	}

	if (!obj || !OBJ_WORN(obj) || !(temp_ch = obj->loc.wearing))
	{
		return FALSE;
	}

	if (!(obj == temp_ch->equipment[WIELD]))
	{
		return FALSE;
	}

	if (cmd == CMD_PERIODIC && !IS_ROOM(temp_ch->in_room, ROOM_NO_MAGIC))
	{
		curr_time = time(NULL);

		if (obj->timer[0] + 60 <= curr_time)
		{
			obj->timer[0] = curr_time;
			// 1/10 chance each minute
			if (!number(0, 9))
			{
				act("&+wYour $q &+wbegins to vibrate softly.\n"
				    "&+wSuddenly, it flares &+Rbright red&+w and a &+rcrimson&+w cloud of &+Lsmoke&+w pours forth.\n"
				    "&+wThe spirits of the &+LDragons&+w trapped within infuse you with power to combat\n"
				    "&+wtheir bretheren!",
				    FALSE, temp_ch, obj, 0, TO_CHAR);

				act("&+w$n&+w's $q &+wbegins to vibrate softly.\n"
				    "&+wSuddenly, it flares &+Rbright red&+w and a &+rcrimson&+w cloud of &+Lsmoke&+w pours forth.\n"
				    "&+wThe spirits of the &+LDragons&+w trapped within infuse you with power to combat\n"
				    "&+wtheir bretheren!",
				    FALSE, temp_ch, obj, 0, TO_ROOM);

				bard_dragons(60, temp_ch, temp_ch, 0);
				return FALSE;
			}
		}
	}

	// 1/10 chance on a hit
	if (cmd == CMD_MELEE_HIT && !number(0, 9))
	{
		if (!(vict = legacy_proc_arg<P_char>(arg)))
			return FALSE;

		if (IS_DRAGON(vict) || (GET_RACE(vict) == RACE_DRAGONKIN))
		{
			act("&+LYour $q &+Lbites deeply into $N!", FALSE, temp_ch, obj, vict,
			    TO_CHAR);
			act("&+L$n&+L grins as $p&+L bites deeply into $N!", FALSE, temp_ch, obj,
			    vict, TO_NOTVICT);
			act("&+LYou double over in pain as $p&+L bites deeply into your flesh!",
			    FALSE, temp_ch, obj, vict, TO_VICT);

			dam = number(400, 500);
			melee_damage(temp_ch, vict, dam, PHSDAM_NOSHIELDS | PHSDAM_TOUCH, 0);
			return FALSE;
		}
	}
	return FALSE;
}
