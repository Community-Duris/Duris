/* Kimordril mobile special procedures. */

#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utils.h"
#include "cmd/interp.h"

int kimordril_shout(P_char ch, P_char tch, int cmd, char * /*arg*/)
{
	int helpers[] = { 95505, 95532, 0 };
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;
	if (!tch && !number(0, 4))
		return shout_and_hunt(ch, 100,
				      "&+WHelp me elite guard, we are being attacked by %s!", NULL,
				      helpers, 0, 0);
	return FALSE;
}
