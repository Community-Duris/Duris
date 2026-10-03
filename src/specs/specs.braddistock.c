/* Braddistock area procedures. */

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

int jet_black_maul(P_obj obj, P_char ch, int cmd, char *arg)
{
	int curr_time;

	/*
	   check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
	{
		return TRUE;
	}

	if (!IS_ALIVE(ch))
		return FALSE;

	if ((cmd == CMD_REMOVE) && arg)
	{
		if (isname(arg, obj->name) || isname(arg, "all"))
		{
			if (affected_by_spell(ch, SPELL_STRENGTH))
			{
				affect_from_char(ch, SPELL_STRENGTH);
				send_to_char("You feel less &+Cstrong&n.\r\n", ch);
			}
			if (affected_by_spell(ch, SPELL_ENHANCED_STR))
			{
				affect_from_char(ch, SPELL_ENHANCED_STR);
				send_to_char("&+CYour muscles return to normal.&n\r\n", ch);
			}
		}
	}

	if (!OBJ_WORN_POS(obj, WIELD))
	{
		return FALSE;
	}

	if (arg && (cmd == CMD_SAY))
	{
		if (isname(arg, "titan"))
		{
			curr_time = time(NULL);

			if (curr_time >= obj->timer[0] + 60)
			{
				act("You say 'titan'", FALSE, ch, obj, 0, TO_CHAR);
				act("Your $q hums briefly.", FALSE, ch, obj, obj, TO_CHAR);

				act("$n says 'titan'", TRUE, ch, obj, NULL, TO_ROOM);
				act("$n's $q hums briefly.", TRUE, ch, obj, NULL, TO_ROOM);
				spell_enhanced_strength(50, ch, 0, SPELL_TYPE_SPELL, ch, 0);
				spell_strength(50, ch, 0, SPELL_TYPE_SPELL, ch, 0);
				obj->timer[0] = curr_time;

				return TRUE;
			}
		}
	}
	return FALSE;
}

int braddistock(P_char ch, P_char pl, int cmd, char * /*arg*/)
{
	// Check for periodic event calls
	if (cmd == CMD_SET_PERIODIC)
	{
		return FALSE;
	}

	// MobCombat() will call this function with pl, cmd, and arg set to null
	// Immortals aren't blocked.
	if (!pl || IS_TRUSTED(pl))
	{
		return FALSE;
	}

	if (cmd == CMD_NORTH && pl != ch && (GET_LEVEL(pl) > 14))
	{
		send_to_char(
			"The spirit of Lord Braddistock says 'We don't want your kind around here!'",
			ch);
		act("$N says 'We don't want your kind around here!'", TRUE, pl, 0, ch, TO_NOTVICT);
		act("$N blocks your passage.", TRUE, pl, 0, ch, TO_CHAR);
		act("$N blocks $n.", TRUE, pl, 0, ch, TO_NOTVICT);
		return TRUE;
	}

	return FALSE;
}
