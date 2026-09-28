/* Teka2 special procedures. */

#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "world/db.h"
#include "cmd/interp.h"
#include "core/utils.h"
#include "combat/damage.h"
#include "magic/spells.h"
#include "world/specs.prototypes.h"

/* flaming mace of the Ruzdo #75561 */

int flaming_mace_ruzdo(P_obj obj, P_char ch, int cmd, char *arg)
{
	int dam = cmd / 1000;
	P_char vict;

	/*
	   check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return FALSE;

	if (!dam) /*
	             if dam is not 0, we have been called when
	             weapon hits someone
	           */
		return (FALSE);

	if (!ch)
		return (FALSE);

	if (!OBJ_WORN_POS(obj, WIELD))
		return (FALSE);

	vict = legacy_proc_arg<P_char>(arg);

	if (!vict)
		return (FALSE);

	if (obj->loc.wearing == ch)
	{
		if (!number(0, 30))
		{
			act("Your $q glows brightly as a &+Yblast of pure light&N streaks out of it.",
			    FALSE, obj->loc.wearing, obj, 0, TO_CHAR);
			act("$n's $q glows brightly as a &+Yblast of pure light&N streaks out of it!",
			    FALSE, obj->loc.wearing, obj, 0, TO_ROOM);
			spell_sunray(30, ch, 0, SPELL_TYPE_SPELL, vict, 0);
		}
		else
		{
			if (!GET_OPPONENT(ch))
				set_fighting(ch, vict);
		}
	}
	if (GET_OPPONENT(ch))
		return (FALSE); /*
		                   do the normal hit damage as well
		                 */
	else
		return (TRUE);
}
