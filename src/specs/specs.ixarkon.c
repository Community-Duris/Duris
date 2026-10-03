/* Special procedures for Ixarkon. */

#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utils.h"
#include "net/comm.h"
#include "cmd/interp.h"
#include "world/db.h"
#include "world/specs.prototypes.h"

extern P_room world;

// Match the number of array entries below - 1
#define MAX_SQUID_ROOM 24
int illithid_teleport_veil(P_obj obj, P_char ch, int cmd, char *arg)
{
	int r_room = -1;
	int to_room[MAX_SQUID_ROOM + 1] = { 2368,  3404,  4108,	 4109,	4437,  6900,  11545,
					    12528, 12535, 12536, 12540, 12541, 15273, 19022,
					    19275, 23805, 23812, 25458, 25459, 25484, 36171,
					    96563, 96569, 96803, 96909 };

	if (cmd == CMD_SET_PERIODIC)
	{
		return FALSE;
	}

	if (cmd != CMD_ENTER || !obj || !IS_ALIVE(ch) || !arg || (str_cmp(" veil", arg)))
	{
		return FALSE;
	}

	act("$p &+Wsuddenly glows brightly!", FALSE, ch, obj, 0, TO_ROOM);
	act("$n steps through, and is gone!", FALSE, ch, obj, 0, TO_ROOM);
	char_from_room(ch);
	while (r_room == -1)
	{
		r_room = real_room(to_room[number(0, MAX_SQUID_ROOM)]);
	}

	act("You enter $p and reappear elsewhere...", FALSE, ch, obj, 0, TO_CHAR);
	char_to_room(ch, r_room, -1);
	do_restore(ch, GET_NAME(ch), -4);
	act("A crackle of energy is felt, and $n appears.", FALSE, ch, 0, 0, TO_ROOM);
	CharWait(ch, 3);

	return TRUE;
}
#undef MAX_SQUID_ROOM
