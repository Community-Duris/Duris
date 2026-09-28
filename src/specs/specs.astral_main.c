/* Special procedures for Astral Main. */

#include "core/prototypes.h"
#include "core/structs.h"
#include "cmd/interp.h"
#include "world/specs.prototypes.h"

int astral_succubus(P_char ch, P_char tch, int cmd, char * /*arg*/)
{
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;
	if (!tch && !number(0, 10))
		return shout_and_hunt(ch, 10, "&+RCome, my sisters, we are under attack by %s!",
				      astral_succubus, NULL, 0, 0);
	return FALSE;
}
