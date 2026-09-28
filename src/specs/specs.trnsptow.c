/* Transport Tower special procedures. */

#include <stdio.h>

#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "world/db.h"
#include "world/events.h"
#include "cmd/interp.h"
#include "core/utility.h"
#include "core/utils.h"
#include "world/map.h"
#include "world/specs.prototypes.h"
#include "magic/spells.h"
#include "world/vnum.obj.h"
#include "world/vnum.room.h"

extern P_room world;
extern bool has_skin_spell(P_char);

int transp_tow_acerlade(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	P_char tch, next;
	char didit = FALSE;
	char buf[256];

	if (cmd == CMD_SET_PERIODIC)
		return FALSE;

	if (cmd)
		return FALSE;

	if (!ch)
		return FALSE;

	/* hell, go for broke, try em all */

	if (ch->in_room == NOWHERE)
		return FALSE;

	snprintf(buf, 256, "&+LWith a wicked grin, Aceralde brutally steals control of &n$N&+L!");
	for (tch = world[ch->in_room].people; tch; tch = next)
	{
		next = tch->next_in_room;

		if (recharm_ch(ch, tch, (bool)TRUE, buf))
			didit = TRUE;
	}

	return didit;
}

#ifdef THARKUN_ARTIS
int trans_tower_shadow_globe(P_obj obj, P_char ch, int cmd, char * /*arg*/)
{
	P_char vict;
	int curr_time = time(NULL);

	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (ch || cmd || !OBJ_WORN(obj))
		return FALSE;

	ch = obj->loc.wearing;

	if (!has_skin_spell(ch) &&
	    obj->timer[0] + get_property("timer.stoneskin.generic", 60) < curr_time)
	{
		hummer(obj);
		spell_stone_skin(45, ch, 0, SPELL_TYPE_POTION, ch, 0);
		obj->timer[0] = curr_time;
	}

	if (IS_FIGHTING(ch))
	{
		if (!number(0, 3))
		{
			act("&+LYour $p throbs as an inky black darkness flows from it!&N", FALSE,
			    obj->loc.wearing, obj, 0, TO_CHAR);
			act("&+L$n's $p pulses as an inky black darkness flows from it!&N", FALSE,
			    obj->loc.wearing, obj, 0, TO_ROOM);
			for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
				if (!grouped(vict, ch) && !IS_TRUSTED(vict) && ch != vict)
					spell_wither(60, ch, 0, SPELL_TYPE_SPELL, vict, 0);
		}
	}

	return FALSE;
}

#else

int trans_tower_shadow_globe(P_obj obj, P_char ch, int cmd, char *arg)
{
	P_char vict, temp;
	int curr_time;

	vict = legacy_proc_arg<P_char>(arg);

	if (cmd == CMD_SET_PERIODIC)
	{
		return TRUE;
	}
	if (ch || cmd)
		return FALSE;

	if (!OBJ_WORN(obj)) /* Most things don't work in a sack... */
		return FALSE;

	ch = obj->loc.wearing;

	if (!has_skin_spell(ch))
	{
		hummer(obj);
		spell_stone_skin(45, ch, 0, SPELL_TYPE_POTION, ch, 0);
	}

	if (arg && (cmd == CMD_SAY))
	{
		if (isname(arg, "invisible"))
		{
			curr_time = time(NULL);

			if (obj->timer[0] + 60 <= curr_time)
			{
				act("You say 'invisible'", FALSE, ch, 0, 0, TO_CHAR);
				act("Your $q hums briefly.", FALSE, ch, obj, obj, TO_CHAR);

				act("$n says 'invisible'", TRUE, ch, obj, NULL, TO_ROOM);
				act("$n's $q hums briefly.", TRUE, ch, obj, NULL, TO_ROOM);
				spell_invisibility(55, ch, 0, SPELL_TYPE_SPELL, ch, 0);

				obj->timer[0] = curr_time;

				return TRUE;
			}
		}
	}

	if (IS_FIGHTING(ch))
	{ /* Check again, for the halibut */
		if (!number(0, 3))
		{
			act("&+LYour $p throbs as an inky black darkness flows from it!&N", FALSE,
			    obj->loc.wearing, obj, 0, TO_CHAR);
			act("&+L$n's $p pulses as an inky black darkness flows from it!&N", FALSE,
			    obj->loc.wearing, obj, 0, TO_ROOM);
			for (vict = world[ch->in_room].people; vict; vict = temp)
			{
				temp = vict->next_in_room;
				if (((vict->group != ch->group) && !IS_TRUSTED(vict)) ||
				    (!ch->group))
					if (ch != vict)
						spell_wither(60, ch, 0, SPELL_TYPE_SPELL, vict, 0);
			}
		}
	}

	return FALSE;
}

#endif
