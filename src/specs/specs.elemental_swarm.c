/* Elemental swarm mobile special procedures. */

#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "cmd/interp.h"
#include "core/utils.h"
#include "magic/spells.h"
#include "world/specs.prototypes.h"

int elemental_swarm_fire(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	P_char vict = NULL;

	if (!ch)
		return FALSE;
	if (cmd == CMD_SET_PERIODIC && GET_OPPONENT(ch))
		return FALSE;
	// let's junk it on anything if not fighitng!
	if (cmd && GET_OPPONENT(ch))
		return FALSE;

	if (IS_FIGHTING(ch) && (cmd == 0) && (number(1, 15) == 1))
	{
		vict = GET_OPPONENT(ch);
		act("$n &+rlets forth a guttural &=LRROAR&+r!&n", TRUE, ch, 0, 0, TO_ROOM);
		spell_flamestrike(GET_LEVEL(ch), ch, NULL, SPELL_TYPE_SPELL, vict, 0);
	}
	return FALSE;
}

int elemental_swarm_earth(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	P_char vict = NULL;

	if (!ch)
		return FALSE;
	if (cmd == CMD_SET_PERIODIC && GET_OPPONENT(ch))
		return FALSE;
	// let's junk it on anything if not fighitng!
	if (cmd && GET_OPPONENT(ch))
		return FALSE;

	if (IS_FIGHTING(ch) && (cmd == 0) && (number(1, 15) == 1))
	{
		vict = GET_OPPONENT(ch);
		act("$n &+rlets forth a guttural &=LRROAR&+r!&n", TRUE, ch, 0, 0, TO_ROOM);
		spell_earthen_maul(GET_LEVEL(ch), ch, NULL, SPELL_TYPE_SPELL, vict, 0);
	}
	return FALSE;
}

int elemental_swarm_air(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	P_char vict = NULL;

	if (!ch)
		return FALSE;
	if (cmd == CMD_SET_PERIODIC && GET_OPPONENT(ch))
		return FALSE;
	// let's junk it on anything if not fighitng!
	if (cmd && GET_OPPONENT(ch))
		return FALSE;

	if (IS_FIGHTING(ch) && (cmd == 0) && (number(1, 15) == 1))
	{
		vict = GET_OPPONENT(ch);
		act("$n &+rlets forth a guttural &=LRROAR&+r!&n", TRUE, ch, 0, 0, TO_ROOM);
		spell_cyclone(GET_LEVEL(ch), ch, NULL, SPELL_TYPE_SPELL, vict, 0);
	}
	return FALSE;
}

int elemental_swarm_water(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	P_char vict = NULL;

	if (!ch)
		return FALSE;
	if (cmd == CMD_SET_PERIODIC && GET_OPPONENT(ch))
		return FALSE;
	// let's junk it on anything if not fighitng!
	if (cmd && GET_OPPONENT(ch))
		return FALSE;

	if (IS_FIGHTING(ch) && (cmd == 0) && (number(1, 15) == 1))
	{
		vict = GET_OPPONENT(ch);
		act("$n &+rlets forth a guttural &=LRROAR&+r!&n", TRUE, ch, 0, 0, TO_ROOM);
		spell_dread_wave(GET_LEVEL(ch), ch, NULL, SPELL_TYPE_SPELL, vict, 0);
	}
	return FALSE;
}
