/* Object combat procedures for Githzer. */

#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utility.h"
#include "core/utils.h"
#include "combat/attack_continuation.h"
#include "net/comm.h"
#include "cmd/interp.h"
#include "combat/damage.h"
#include "magic/spells.h"
#include "world/bloodstains.h"
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

	if (!dam || !char_in_list(ch) || !IS_ALIVE(ch) || !(vict = legacy_proc_arg<P_char>(arg)))
	{
		return FALSE;
	}
	room = ch->in_room;
	if (!char_in_list(vict) || !IS_ALIVE(vict) || vict->in_room != room)
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
		const attack_continuation luck_continuation =
			begin_attack_continuation(ch, vict, obj);
		spell_serendipity(60, ch, NULL, SPELL_TYPE_SPELL, ch, 0);
		const attack_continuation_result after_luck =
			check_attack_continuation(luck_continuation);
		if (!after_luck.can_continue())
			return TRUE;
		ch = after_luck.actor;
		vict = after_luck.target;
		obj = after_luck.weapon;
		make_bloodstain(ch);
		make_bloodstain(ch);
		make_bloodstain(ch);
		for (i = number(2, 6); i; i--)
		{
			if (is_char_in_room(ch, room) && is_char_in_room(vict, room))
			{
				const attack_continuation continuation =
					begin_attack_continuation(ch, vict, obj);
				hit(ch, vict, obj);
				const attack_continuation_result after_hit =
					check_attack_continuation(continuation);
				if (!after_hit.can_continue())
					break;
				ch = after_hit.actor;
				vict = after_hit.target;
				obj = after_hit.weapon;
			}
		}
		return TRUE;
	}

	return FALSE;
}

// Item for learning skills.  Not in game as of 7/4/2015
