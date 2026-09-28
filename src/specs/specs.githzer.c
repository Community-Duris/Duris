/* Object combat procedures for Githzer. */

#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utility.h"
#include "core/utils.h"
#include "net/comm.h"
#include "cmd/interp.h"
#include "combat/damage.h"
#include "magic/spells.h"
#include "world/specs.prototypes.h"

int lucky_weapon(P_obj obj, P_char ch, int cmd, char *arg)
{
	P_char vict;
	int i;
	int room;
	int dam = cmd / 1000;

	if (cmd == CMD_SET_PERIODIC)
	{
		return TRUE;
	}

	if (!dam || !IS_ALIVE(ch) || !(vict = legacy_proc_arg<P_char>(arg)))
	{
		return FALSE;
	}
	room = ch->in_room;
	if (!IS_ALIVE(vict) || vict->in_room != room)
	{
		return FALSE;
	}

	// 1/30 chance
	if (!number(0, 29))
	{
		send_to_char("&=LWYou score a CRITICAL HIT!!!&N\n", ch);
		make_bloodstain(ch);
		if (!number(0, 2))
		{
			spell_serendipity(40, ch, NULL, SPELL_TYPE_SPELL, ch, 0);
		}

		hit(ch, vict, obj);
	}
	else if (!number(0, 100))
	{
		send_to_char("&=LWYou score a REALLY LUCKY round of hits!!!!!&N\n", ch);
		spell_serendipity(60, ch, NULL, SPELL_TYPE_SPELL, ch, 0);
		make_bloodstain(ch);
		make_bloodstain(ch);
		make_bloodstain(ch);
		for (i = number(2, 6); i; i--)
		{
			if (is_char_in_room(ch, room) && is_char_in_room(vict, room))
			{
				hit(ch, vict, obj);
			}
		}
		return TRUE;
	}

	return FALSE;
}









// Item for learning skills.  Not in game as of 7/4/2015
