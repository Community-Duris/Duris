/* Shared combat-equipment procedures. */

#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utils.h"
#include "net/comm.h"
#include "cmd/interp.h"
#include "world/specs.prototypes.h"

int generic_parry_proc(P_obj obj, P_char ch, int cmd, char *arg)
{
	struct proc_data *data;
	P_char vict;

	if (cmd == CMD_SET_PERIODIC)
	{
		return FALSE;
	}

	// 1/20 chance.
	if (cmd != CMD_GOTHIT || number(0, 19))
	{
		return FALSE;
	}
	// important! can do this cast (next line) ONLY if cmd was CMD_GOTHIT or CMD_GOTNUKED
	if (!(data = legacy_proc_arg<struct proc_data *>(arg)))
	{
		return FALSE;
	}
	vict = data->victim;
	if (!IS_ALIVE(vict))
	{
		return FALSE;
	}
	act("Your $q parries $N's lunge at you.", FALSE, ch, obj, vict, TO_CHAR | ACT_NOTTERSE);
	act("$n's $q parries your futile lunge at $m.", FALSE, ch, obj, vict,
	    TO_VICT | ACT_NOTTERSE);
	act("$n's $q parries $N's lunge at $n.", FALSE, ch, obj, vict, TO_NOTVICT | ACT_NOTTERSE);

	return TRUE;
}
