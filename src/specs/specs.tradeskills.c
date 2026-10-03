/* Trade skills area object special procedures. */

#include <time.h>

#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utils.h"
#include "net/comm.h"
#include "cmd/interp.h"
#include "magic/spells.h"

int thanksgiving_wings(P_obj obj, P_char ch, int cmd, char * /*argument*/)
{
	int curr_time;
	P_char temp_ch;

	if (cmd == CMD_SET_PERIODIC)
	{
		return TRUE;
	}

	if (!obj || !OBJ_WORN(obj) || cmd != CMD_PERIODIC)
	{
		return FALSE;
	}

	if (!(temp_ch = ch))
	{
		if (obj->loc.wearing)
		{
			temp_ch = obj->loc.wearing;
		}
		else
		{
			return FALSE;
		}
	}

	curr_time = time(NULL);

	if (obj->timer[0] + (int)200 <= curr_time)
	{
		act("&nYou suddenly feel the urge for &+yTURKEY! &nand take a nice &+rbite &nout of your $p!",
		    FALSE, temp_ch, obj, 0, TO_CHAR);
		act("$n suddenly looks &+Rravenous!&n and takes a huge &+rbite &nout of their $p&n!",
		    FALSE, temp_ch, obj, 0, TO_ROOM);
		spell_invigorate(30, temp_ch, 0, SPELL_TYPE_POTION, temp_ch, 0);
		obj->timer[0] = curr_time;
	}
	return FALSE;
}
