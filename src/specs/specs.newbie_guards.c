/* Newbie area boundary guard procedures. */

#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "world/db.h"
#include "world/events.h"
#include "cmd/interp.h"
#include "core/utils.h"
#include "world/specs.prototypes.h"

// NEWBIEGUARD
int newbie_guard_north(P_char ch, P_char pl, int cmd, char * /*arg*/)
{
	int allowed = 0;
	P_char rider;

	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch)
		return 0;

	if (!pl)
		return 0;

	if (!(cmd == CMD_NORTH))
		return 0;

	rider = get_linking_char(pl, LNK_RIDING);

	if (rider && (GET_LEVEL(rider) >= 26 || GET_LEVEL(ch) >= 26))
	{
	}
	else if (GET_LEVEL(pl) < 26)
	{
		allowed = 1;

		if (IS_NPC(pl) && IS_SET(pl->specials.act, ACT_MOUNT) && rider &&
		    GET_LEVEL(rider) > 25 && !IS_TRUSTED(rider))
			allowed = 0;
	}
	else if (IS_TRUSTED(pl))
		allowed = 1;
	else
		allowed = 0;

	if (allowed)
	{
		act("$N nods, stands aside and lets $n pass.", FALSE, pl, 0, ch, TO_ROOM);
		act("$N nods and stands aside to let you pass.", FALSE, pl, 0, ch, TO_CHAR);
		return (FALSE);
	}
	/*
	 * BLOCK!
	 */
	act("$N says '&+ROver my dead body!&n.", FALSE, pl, 0, ch, TO_CHAR);
	act("$N says '&+ROver my dead body!&n.", FALSE, pl, 0, ch, TO_NOTVICT);
	return (TRUE);
}

int newbie_guard_east(P_char ch, P_char pl, int cmd, char * /*arg*/)
{
	int allowed = 0;
	P_char rider;

	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch)
		return 0;

	if (!pl)
		return 0;

	if (!(cmd == CMD_EAST))
		return 0;

	rider = get_linking_char(pl, LNK_RIDING);

	if (rider && (GET_LEVEL(rider) >= 26 || GET_LEVEL(ch) >= 26))
	{
	}
	else if (GET_LEVEL(pl) < 26)
	{
		allowed = 1;

		if (IS_NPC(pl) && IS_SET(pl->specials.act, ACT_MOUNT) && rider &&
		    GET_LEVEL(rider) > 25 && !IS_TRUSTED(rider))
			allowed = 0;
	}
	else if (IS_TRUSTED(pl))
		allowed = 1;
	else
		allowed = 0;

	if (allowed)
	{
		act("$N nods, stands aside and lets $n pass.", FALSE, pl, 0, ch, TO_ROOM);
		act("$N nods and stands aside to let you pass.", FALSE, pl, 0, ch, TO_CHAR);
		return (FALSE);
	}
	/*
	 * BLOCK!
	 */
	act("$N says '&+ROver my dead body!&n.", FALSE, pl, 0, ch, TO_CHAR);
	act("$N says '&+ROver my dead body!&n.", FALSE, pl, 0, ch, TO_NOTVICT);
	return (TRUE);
}

int newbie_guard_south(P_char ch, P_char pl, int cmd, char * /*arg*/)
{
	int allowed = 0;
	P_char rider;

	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch)
		return 0;

	if (!pl)
		return 0;

	if (!(cmd == CMD_SOUTH))
		return 0;

	rider = get_linking_char(pl, LNK_RIDING);

	if (rider && (GET_LEVEL(rider) >= 26 || GET_LEVEL(ch) >= 26))
	{
	}
	else if (GET_LEVEL(pl) < 26)
	{
		allowed = 1;

		if (IS_NPC(pl) && IS_SET(pl->specials.act, ACT_MOUNT) && rider &&
		    GET_LEVEL(rider) > 25 && !IS_TRUSTED(rider))
			allowed = 0;
	}
	else if (IS_TRUSTED(pl))
		allowed = 1;
	else
		allowed = 0;

	if (allowed)
	{
		act("$N nods, stands aside and lets $n pass.", FALSE, pl, 0, ch, TO_ROOM);
		act("$N nods and stands aside to let you pass.", FALSE, pl, 0, ch, TO_CHAR);
		return (FALSE);
	}
	/*
	 * BLOCK!
	 */
	act("$N says '&+ROver my dead body!&n.", FALSE, pl, 0, ch, TO_CHAR);
	act("$N says '&+ROver my dead body!&n.", FALSE, pl, 0, ch, TO_NOTVICT);
	return (TRUE);
}

int newbie_guard_west(P_char ch, P_char pl, int cmd, char * /*arg*/)
{
	int allowed = 0;
	P_char rider;

	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch)
		return 0;

	if (!pl)
		return 0;

	if (!(cmd == CMD_WEST))
		return 0;

	rider = get_linking_char(pl, LNK_RIDING);

	if (rider && (GET_LEVEL(rider) >= 26 || GET_LEVEL(ch) >= 26))
	{
	}
	else if (GET_LEVEL(pl) < 26)
	{
		allowed = 1;

		if (IS_NPC(pl) && IS_SET(pl->specials.act, ACT_MOUNT) && rider &&
		    GET_LEVEL(rider) > 25 && !IS_TRUSTED(rider))
			allowed = 0;
	}
	else if (IS_TRUSTED(pl))
		allowed = 1;
	else
		allowed = 0;

	if (allowed)
	{
		act("$N nods, stands aside and lets $n pass.", FALSE, pl, 0, ch, TO_ROOM);
		act("$N nods and stands aside to let you pass.", FALSE, pl, 0, ch, TO_CHAR);
		return (FALSE);
	}
	/*
	 * BLOCK!
	 */
	act("$N says '&+ROver my dead body!&n.", FALSE, pl, 0, ch, TO_CHAR);
	act("$N says '&+ROver my dead body!&n.", FALSE, pl, 0, ch, TO_NOTVICT);
	return (TRUE);
}
// END NEWBIE GUARD
