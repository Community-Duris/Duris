/* Special procedures for Moonshae. */

#include "core/prototypes.h"
#include "core/structs.h"
#include "cmd/interp.h"
#include "world/specs.prototypes.h"
#include "core/utils.h"

int sister_knight(P_char ch, P_char tch, int cmd, char * /*arg*/)
{
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;
	if (!tch && !number(0, 8))
		return shout_and_hunt(ch, 100, "Come, my sisters, we are under attack by %s!",
				      sister_knight, NULL, 0, 0);
	return FALSE;
}

int cc_fisherffolk(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
		return FALSE;

	switch (number(1, 100))
	{
	case 1:
	{
		mobsay(ch, "I hear the zoo keeper is looking for feathers again.");
		return TRUE;
	}
	case 2:
	{
		mobsay(ch, "I wish these darned fish would start biting.");
		return TRUE;
	}
	case 3:
	{
		mobsay(ch, "Darn, I'm out of worms again.");
		return TRUE;
	}
	default:
		return FALSE;
	}
}

int cc_female_ffolk(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
		return FALSE;

	switch (number(1, 100))
	{
	case 1:
	{
		mobsay(ch, "Those sharkteeth trinkets some of the ffolk have sure are pretty.");
		return TRUE;
	}
	case 2:
	{
		mobsay(ch, "I wish I had a sharktooth necklace.");
		return TRUE;
	}
	case 3:
	{
		mobsay(ch, "I wonder where to get a sharktooth necklace at?");
		return TRUE;
	}
	default:
		return FALSE;
	}
}
