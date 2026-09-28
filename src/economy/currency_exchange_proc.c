/* Currency exchange special procedure for money-changer mobiles.
 * Retired in favor of zero-surcharge bank coin conversion.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cmd/interp.h"
#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utils.h"
#include "net/comm.h"
#include "world/specs.prototypes.h"

int money_changer(P_char me, P_char ch, int cmd, char * /*arg*/)
{
	/* Check for periodic event calls */
	if (cmd == CMD_SET_PERIODIC)
	{
		return FALSE;
	}
	if (!me || !ch || !IS_AWAKE(me) || IS_FIGHTING(me))
		return FALSE;

	if ((cmd != CMD_EXCHANGE) && (cmd != CMD_LIST))
		return FALSE;

	if (!CAN_SEE(me, ch))
	{
		mobsay(me, "How may I be of help if I cannot see you?");
		return TRUE;
	}
	if (cmd == CMD_LIST)
	{
		mobsay(me, "The Royal Bank now handles all coin exchanges with zero surcharge.");
		mobsay(me, "Please visit any town bank teller to deposit or withdraw coins.");
		return TRUE;
	}
	/* cmd == CMD_EXCHANGE */
	mobsay(me,
	       "We no longer exchange coins here. The Royal Bank handles all coin exchanges with zero surcharge; please visit a bank teller.");
	return TRUE;
}
