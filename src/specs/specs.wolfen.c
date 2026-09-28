/* Object combat procedures for Wolfen. */

#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utils.h"
#include "net/comm.h"
#include "cmd/interp.h"
#include "combat/damage.h"
#include "world/specs.prototypes.h"

int generic_riposte_proc(P_obj obj, P_char ch, int cmd, char *arg)
{
	struct proc_data *data;
	P_char victim;

	if (cmd == CMD_SET_PERIODIC)
	{
		return FALSE;
	}

	// 1/20 chance.
	if (cmd != CMD_GOTHIT || number(0, 19))
	{
		return FALSE;
	}

	if (!(data = legacy_proc_arg<struct proc_data *>(arg)))
	{
		return FALSE;
	}
	victim = data->victim;
	if (!IS_ALIVE(victim))
	{
		return FALSE;
	}

	act("$n's $q deflects $N's blow and strikes $M!", TRUE, ch, obj, victim,
	    TO_NOTVICT | ACT_NOTTERSE);
	act("$n's $q deflects your blow and strikes YOU!", TRUE, ch, obj, victim,
	    TO_VICT | ACT_NOTTERSE);
	act("Your $q deflects $N's blow and strikes $M!", TRUE, ch, obj, victim,
	    TO_CHAR | ACT_NOTTERSE);
	hit(ch, victim, obj);

	return TRUE;
}
