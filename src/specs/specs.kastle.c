/* Kastle equipment procedures. */

#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "world/events.h"
#include "cmd/interp.h"
#include "core/utility.h"
#include "core/utils.h"
#include "combat/damage.h"
#include "world/specs.prototypes.h"
#include "magic/spells.h"

int nightcrawler_dagger(P_obj obj, P_char ch, int cmd, char *arg)
{
	int dam = cmd / 1000;
	P_char victim;

	if (cmd == CMD_SET_PERIODIC)
	{
		return FALSE;
	}

	if (!dam || !IS_ALIVE(ch) || !OBJ_WORN(obj) || (obj->loc.wearing != ch))
	{
		return FALSE;
	}

	victim = legacy_proc_arg<P_char>(arg);
	// 1/30 chance.
	if (!IS_ALIVE(victim) || number(0, 29))
	{
		return FALSE;
	}

	act("&+LA beam of d&+barkne&+Lss erupts from the tip of $n's&N $q!&N", TRUE, ch, obj,
	    victim, TO_NOTVICT);
	act("&+LA beam of d&+barkne&+Lss erupts from the tip of your&N $q!&N", TRUE, ch, obj,
	    victim, TO_CHAR);
	act("&+LA beam of d&+barkne&+Lss erupts from the tip of $n's&N $q!&N&N", TRUE, ch, obj,
	    victim, TO_VICT);
	spell_devitalize(50, ch, NULL, SPELL_TYPE_SPELL, victim, 0);
	return TRUE;
}

int zarthos_vampire_slayer(P_obj obj, P_char ch, int cmd, char *arg)
{
	P_char vict;
	int dam = cmd / 1000;

	if (cmd == CMD_SET_PERIODIC)
	{
		return FALSE;
	}

	if (!IS_ALIVE(ch))
	{
		if (OBJ_WORN(obj) && IS_ALIVE(obj->loc.wearing))
		{
			ch = obj->loc.wearing;
		}
		else
		{
			return FALSE;
		}
	}

	if (!dam)
	{
		return FALSE;
	}
	vict = legacy_proc_arg<P_char>(arg);
	if (!IS_ALIVE(vict) || !(IS_UNDEAD(vict) || IS_AFFECTED(vict, AFF_WRAITHFORM)))
	{
		return FALSE;
	}
	// 1/25 chance.
	if (number(0, 24))
	{
		return FALSE;
	}

	act("&+LYour $q &+Lradiates &+Wdivine &+Lpower as it slashes into $N!&n", FALSE, ch, obj,
	    vict, TO_CHAR);
	act("$n's $q &+Lemanates &+Ws&+wp&+We&+wc&+Wt&+wr&+Wa&+wl &+Llight as it slashes into $N!&n",
	    FALSE, ch, obj, vict, TO_NOTVICT);
	act("$n's $q &+Lradiates with &+Wdivine &+Lpower as it slices into you!&n", FALSE, ch, obj,
	    vict, TO_VICT);
	spell_destroy_undead(39, ch, 0, SPELL_TYPE_SPELL, vict, 0);
	act("&+LThe $q &+Lbathes you in &+Chealing &+Llight.&n", TRUE, ch, obj, vict, TO_CHAR);
	spell_cure_serious(39, ch, 0, SPELL_TYPE_SPELL, ch, 0);

	return TRUE;
}
