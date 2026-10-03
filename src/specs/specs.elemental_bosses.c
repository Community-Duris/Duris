/* Elemental-plane boss special procedures. */

#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "world/db.h"
#include "world/events.h"
#include "cmd/interp.h"
#include "core/utils.h"
#include "combat/guard.h"
#include "combat/damage.h"
#include "world/specs.prototypes.h"
#include "magic/spells.h"

int menzellon_shout(P_char ch, P_char tch, int cmd, char * /*arg*/)
{
	int helpers[] = { 12401, 12410, 12420, 12430, 12440, 12450, 12460, 0 };
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;
	if (!tch && !number(0, 4))
		return shout_and_hunt(ch, 100,
				      "&+BYou shall DIE!  Denizens of the Ether, come absorb %s!",
				      NULL, helpers, 0, 0);
	return FALSE;
}

int ogremoch_shout(P_char ch, P_char tch, int cmd, char * /*arg*/)
{
	int helpers[] = { 23801, 23802, 23803, 23804, 23807, 23927, 23862, 0 };
	if (cmd == CMD_SET_PERIODIC)
	{
		return TRUE;
	}
	if (!tch && !number(0, 4))
	{
		return shout_and_hunt(ch, 50,
				      "&+YCreatures of the earth!  Come to my aid and crush %s!",
				      NULL, helpers, 0, 0);
	}
	return FALSE;
}

int olhydra_shout(P_char ch, P_char tch, int cmd, char * /*arg*/)
{
	int helpers[] = { 23200, 23210, 23215, 23220, 23230, 23250, 0 };
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;
	if (!tch && !number(0, 4))
		return shout_and_hunt(ch, 100,
				      "&+bBeings of Water!  Assemble at once and destroy %s!", NULL,
				      helpers, 0, 0);
	return FALSE;
}

int yancbin_shout(P_char ch, P_char tch, int cmd, char * /*arg*/)
{
	int helpers[] = { 24400, 24410, 24415, 24420, 24430, 24450, 0 };
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;
	if (!tch && !number(0, 4))
		return shout_and_hunt(ch, 100, "&+CDenizens of air, destroy %s!", NULL, helpers, 0,
				      0);
	return FALSE;
}

int earth_treant(P_char ch, P_char /*tch*/, int cmd, char * /*arg*/)
{
	P_char victim;

	if (cmd == CMD_SET_PERIODIC)
		return FALSE;

	if (cmd)
		return FALSE;

	if (!IS_FIGHTING(ch))
		return FALSE;

	if (!number(0, 6))
	{
		if (!(victim = pick_target(ch, PT_CASTER | PT_STANDING | PT_SMALLER | PT_TOLERANT)))
			return FALSE;
		else
		{
			if (GET_OPPONENT(ch) != victim)
			{
				stop_fighting(ch);
				act("$n switches targets...", FALSE, ch, 0, 0, TO_ROOM);
				victim = guard_check(ch, victim);
				set_fighting(ch, victim);
				send_to_char("You switch opponents!\n", ch);
			}
			if (GET_POS(victim) < POS_STANDING)
				hit(ch, victim, ch->equipment[PRIMARY_WEAPON]);
			else
				branch(ch, victim);
			return TRUE;
		}
	}

	return FALSE;
}

int glades_dagger(P_obj obj, P_char ch, int cmd, char *arg)
{
	P_char vict = ch;
	int dam = cmd / 1000;

	if (cmd == CMD_SET_PERIODIC)
	{
		return TRUE;
	}

	if (!obj || !IS_ALIVE(ch))
		if (!IS_ALIVE(ch))
		{
			return FALSE;
		}

	if ((IS_UNDEAD(ch) || IS_UNDEADRACE(ch)) && OBJ_WORN_BY(obj, ch) && !number(0, 2))
	{
		act("&+B$p briefly flashes with &N&+Yintense light&N&+B, burning your hand severely!&N",
		    TRUE, ch, obj, vict, TO_CHAR);
		act("$N screams in pain, as $S $p burns into $S skin!", FALSE, ch, obj, vict,
		    TO_ROOM);
		spell_damage(ch, ch, number(120, 240), SPLDAM_HOLY, SPLDAM_NOSHRUG, 0);
		return FALSE;
	}

	if (!dam || !(vict = legacy_proc_arg<P_char>(arg)))
	{
		return FALSE;
	}
	if (!IS_ALIVE(vict))
	{
		return FALSE;
	}

	// 1/30 chance
	if (!number(0, 29))
	{
		act("&+BYour $p briefly flashes as its blade touches $N and burns $S skin!&n", TRUE,
		    ch, obj, vict, TO_CHAR);
		act("&+B$p briefly flashes as its blade touches $N and burns $S skin!&n", TRUE, ch,
		    obj, vict, TO_NOTVICT);
		act("&+BOUCH! $n just burned you with his $p!&n", TRUE, ch, obj, vict, TO_VICT);

		if (IS_UNDEAD(vict))
		{
			spell_destroy_undead(50, ch, NULL, 0, vict, 0);
		}
		else if (IS_EVIL(vict))
		{
			spell_dispel_evil(50, ch, NULL, SPELL_TYPE_SPELL, vict, 0);
		}
		else
		{
			spell_cause_critical(50, ch, NULL, SPELL_TYPE_SPELL, vict, 0);
		}
		return FALSE;
	}
	return FALSE;
}
