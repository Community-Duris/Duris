/* Prison special procedures. */

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

#define AZER 7359

int warden_shout(P_char ch, P_char tch, int cmd, char * /*arg*/)
{
	int helpers[] = { 7334, 7335, 7368, 7369, 7367, 7315, 7314, 7317, 0 };
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;
	if (!tch && !number(0, 7))
		return shout_and_hunt(ch, 100, "&+MGuards!!!  Come at once! Destroy %s!", NULL,
				      helpers, 0, 0);
	return FALSE;
}

int flaming_axe_of_azer(P_obj /*obj*/, P_char ch, int cmd, char *arg)
{
	P_char tmp_ch, vict = NULL;
	int room, level;
	int dam = cmd / 1000;

	if (cmd == CMD_SET_PERIODIC)
	{
		return TRUE;
	}

	if (!dam || !IS_ALIVE(ch) || !(room = ch->in_room) ||
	    !(vict = legacy_proc_arg<P_char>(arg)))
	{
		return FALSE;
	}

	// 1/30 chance
	if (!number(0, 29))
	{
		level = BOUNDED(10, GET_LEVEL(ch) + number(-5, 5), 60);

		if (!(tmp_ch = read_mobile(AZER, VIRTUAL)))
		{
			send_to_char(
				"&+WSerious screw-up in your weapon's proc. Tell a coder immidiately.&n\n",
				ch);
			return FALSE;
		}
		char_to_room(tmp_ch, room, -1);
		act("&+rIn a column of &+RROARING&+r fire, $N&+r makes $S entrance.&n", FALSE, ch,
		    0, tmp_ch, TO_CHAR);
		act("&+rIn a column of &+RROARING&+r fire, $N&+r makes $S entrance.&n", FALSE, ch,
		    0, tmp_ch, TO_ROOM);
		group_add_member(ch, tmp_ch);

		if (is_char_in_room(ch, room) && is_char_in_room(vict, room))
		{
			switch (number(0, 3))
			{
			case 0:
				spell_fireball(level, tmp_ch, NULL, 0, vict, 0);
				break;
			case 1:
				spell_magma_burst(level, tmp_ch, 0, 0, vict, 0);
				break;
			case 2:
				spell_molten_spray(level, tmp_ch, 0, 0, vict, 0);
				break;
			case 3:
				spell_immolate(level, tmp_ch, NULL, 0, vict, 0);
				break;
			default:
				break;
			}
		}

		act("&+rAfter helping $S master, $n&+r departs hastily.", FALSE, tmp_ch, 0, ch,
		    TO_NOTVICT);
		act("&+rAfter helping you, $n&+r departs hastily.", FALSE, tmp_ch, 0, ch, TO_VICT);
		char_from_room(tmp_ch);
		char_to_room(tmp_ch, real_room(1), -1);
		extract_char(tmp_ch);
		// If we killed'm then don't execute attack.
		if (!IS_ALIVE(vict))
		{
			return TRUE;
		}
	}
	// for to execute normal hit
	return FALSE;
}
