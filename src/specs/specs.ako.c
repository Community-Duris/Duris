/* Special procedures for Ako Village. */

#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "world/db.h"
#include "world/events.h"
#include "cmd/interp.h"
#include "core/utils.h"
#include "world/specs.prototypes.h"

/* ako procs */

/* mob 3715 */
int ako_hypersquirrel(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;
	if (!ch || !IS_AWAKE(ch) || cmd)
		return FALSE;

	switch (number(0, 100))
	{
	case 0:
		act("$n runs around all over the place!", TRUE, ch, 0, 0, TO_ROOM);
		return TRUE;
	case 1:
		act("$n runs runs up your leg, up your back, around your neck, then leaps off and runs around some more.",
		    TRUE, ch, 0, 0, TO_ROOM);
		return TRUE;
	case 2:
		act("$n runs really fast, slamming right into a tree!", TRUE, ch, 0, 0, TO_ROOM);
		return TRUE;
	case 3:
		act("$n makes some soft squirrel noises.", TRUE, ch, 0, 0, TO_ROOM);
		return TRUE;
	}
	return FALSE;
}

/* mob 3701 */
int ako_songbird(P_char ch, P_char pl, int cmd, char *arg)
{
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (cmd && pl)
	{
		if (pl == ch)
			return FALSE;
		switch (cmd)
		{
		case CMD_PET:
			do_action(pl, arg, CMD_PET);
			act("$n makes a soft soothing sound then flies away.'.", 1, ch, 0, pl,
			    TO_VICT);
		}
	}

	if (!ch || !IS_AWAKE(ch) || cmd)
		return FALSE;

	switch (number(0, 100))
	{
	case 0:
		act("$n sings a beautiful song.", TRUE, ch, 0, 0, TO_ROOM);
		return TRUE;
	case 1:
		act("$n pecks at the ground.", TRUE, ch, 0, 0, TO_ROOM);
		return TRUE;
	case 2:
		act("$n flies really high up in the air, dive bombs at the ground and lands flawlessly!",
		    TRUE, ch, 0, 0, TO_ROOM);
		do_action(ch, 0, CMD_FLEX);
		return TRUE;
	case 3:
		act("$n sings a little more.", TRUE, ch, 0, 0, TO_ROOM);
		return TRUE;
	}
	return FALSE;
}

/* mob 3716 */
int ako_vulture(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;
	if (!ch || !IS_AWAKE(ch) || cmd)
		return FALSE;

	switch (number(0, 100))
	{
	case 0:
		act("$n rips a bit of flesh out of the corpse and swallows it whole.", TRUE, ch, 0,
		    0, TO_ROOM);
		return TRUE;
	case 1:
		act("$n pulls out a large maggot and eats it.", TRUE, ch, 0, 0, TO_ROOM);
		return TRUE;
	case 2:
		act("$n looks in your direction, you feel a little uneasy about that.", TRUE, ch, 0,
		    0, TO_ROOM);
		return TRUE;
	}
	return FALSE;
}

/* mob 3721 */
int ako_wildmare(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch || !IS_AWAKE(ch) || cmd)
		return FALSE;

	switch (number(0, 100))
	{
	case 0:
		act("$n swats its tail at a fly.", TRUE, ch, 0, 0, TO_ROOM);
		return TRUE;
	case 1:
		act("$n takes a mouthful of grass and starts chewing.", TRUE, ch, 0, 0, TO_ROOM);
		return TRUE;
	}
	return FALSE;
}

/* mob 3720 */
int ako_cow(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;
	if (!ch || !IS_AWAKE(ch) || cmd)
		return FALSE;

	switch (number(0, 100))
	{
	case 0:
		do_action(ch, 0, CMD_COW);
		return TRUE;
	case 1:
		do_action(ch, 0, CMD_COW);
		return TRUE;
	case 2:
		do_action(ch, 0, CMD_COW);
		return TRUE;
	case 3:
		do_action(ch, 0, CMD_COW);
		return TRUE;
	case 4:
		do_action(ch, 0, CMD_COW);
		return TRUE;
	case 5:
		do_action(ch, 0, CMD_COW);
		return TRUE;
	case 6:
		do_action(ch, 0, CMD_COW);
		return TRUE;
	case 7:
		do_action(ch, 0, CMD_COW);
		return TRUE;
	case 8:
		do_action(ch, 0, CMD_COW);
		return TRUE;
	case 9:
		act("$n moos at you!.", TRUE, ch, 0, 0, TO_ROOM);
		return TRUE;
	case 10:
		act("$n swats its tail at a fly.", TRUE, ch, 0, 0, TO_ROOM);
		return TRUE;
	}
	return FALSE;
}
