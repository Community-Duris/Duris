/* Shared mobile tracking special procedures. */

#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utils.h"
#include "world/db.h"
#include "cmd/interp.h"
#include "combat/range.h"
#include "world/specs.prototypes.h"

int range_scan_track(P_char ch, int distance, int type_scan);

int undeadcont_track(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	/*
	   check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return FALSE;

	if (cmd)
		return FALSE;

	if (GET_VITALITY(ch) < 10) /* ok dont get too tired */
		return TRUE;

	if (!IS_FIGHTING(ch) && (ch->in_room != NOWHERE) &&
	    (MIN_POS(ch, POS_STANDING + STAT_NORMAL)))
	{
		/* ok we check if there is any PC near */

		if (range_scan_track(ch, 3, SCAN_ANY))
		{
			InitNewMobHunt(ch);
			return TRUE;
		}
	}
	return FALSE;
}

/*
 * Underdark Mob Proc
 */
int underdark_track(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	/*
	   check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return FALSE;

	if (cmd)
		return FALSE;

	if (GET_VITALITY(ch) < 10) /* ok dont get too tired */
		return TRUE;

	if (!IS_FIGHTING(ch) && (ch->in_room != NOWHERE) &&
	    (MIN_POS(ch, POS_STANDING + STAT_NORMAL)))
	{
		/* ok we check if there is any PC near */

		if (range_scan_track(ch, 3, SCAN_ANY))
		{
			InitNewMobHunt(ch);
			return TRUE;
		}
	}
	return FALSE;
}
