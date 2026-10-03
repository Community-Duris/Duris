/* Dungeon equipment procedures. */

#include <time.h>

#include "core/prototypes.h"
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

int blue_sword_armor(P_obj obj, P_char ch, int cmd, char *arg)
{
	int curr_time;

	if (cmd == CMD_SET_PERIODIC)
	{
		return TRUE;
	}

	if (!IS_ALIVE(ch))
	{
		return FALSE;
	}

	if ((cmd == CMD_REMOVE) && arg)
	{
		if (isname(arg, obj->name) || isname(arg, "all"))
		{
			if (affected_by_spell(ch, SPELL_ARMOR))
			{
				affect_from_char(ch, SPELL_ARMOR);
				send_to_char("You feel less &+Wprotected&n.\r\n", ch);
			}
		}
	}

	if (!OBJ_WORN_POS(obj, WIELD))
	{
		return FALSE;
	}

	if (arg && (cmd == CMD_SAY))
	{
		if (isname(arg, "armor"))
		{
			curr_time = time(NULL);

			if (curr_time >= obj->timer[0] + 60)
			{
				act("You say 'armor'", FALSE, ch, 0, 0, TO_CHAR);
				act("Your $q hums briefly.", FALSE, ch, obj, obj, TO_CHAR);

				act("$n says 'armor'", TRUE, ch, obj, NULL, TO_ROOM);
				act("$n's $q hums briefly.", TRUE, ch, obj, NULL, TO_ROOM);
				spell_armor(60, ch, 0, SPELL_TYPE_SPELL, ch, 0);
				obj->timer[0] = curr_time;

				return TRUE;
			}
		}
	}
	return FALSE;
}
