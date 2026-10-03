/* Area-owned special procedures. */

#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "world/db.h"
#include "world/events.h"
#include "cmd/interp.h"
#include "core/utility.h"
#include "core/utils.h"
#include "world/specs.prototypes.h"
#include "combat/damage.h"
#include "combat/range.h"
#include "magic/spells.h"

int do_fetid_breath(P_char ch);
void hyena_bite(P_char ch, P_char victim);
extern void insectbite(P_char ch, P_char victim);

int block_up(P_char ch, P_char pl, int cmd, char * /*arg*/)
{
	int allowed = 0;

	if (cmd == CMD_SET_PERIODIC)
		return TRUE;
	allowed = 0;
	if (!ch)
		return 0;
	if (!pl)
		return 0;

	if (cmd != CMD_UP)
		return FALSE;

	if (IS_TRUSTED(pl) || IS_NPC(pl))
		allowed = 1;
	else
		allowed = 0;

	if (allowed)
	{
		act("$N nods, stands aside and lets $n pass.", FALSE, pl, 0, ch, TO_ROOM);
		act("$N nods and stands aside to let you pass.", FALSE, pl, 0, ch, TO_CHAR);
		return FALSE;
	}

	/* BLOCK! */
	act("$N &+yjumps in your path blocking the exit!&n.", FALSE, pl, 0, ch, TO_CHAR);
	act("$N &+yjumps in the way of $n blocking the exit!&n.", FALSE, pl, 0, ch, TO_NOTVICT);
	return TRUE;
}

int whirlwind_of_teetch(P_char ch, int targets)
{
	P_char victim;

	if (!IS_FIGHTING(ch))
		return 0;

	act("$n&+W sends our a deep howl, and charges at $s&+W foes!", TRUE, ch, 0, 0, TO_ROOM);
	send_to_char("&+WYou send out a deep howl as you charge at your foes!", ch);

	for (int i = targets; i && IS_ALIVE(ch); i--)
	{
		if ((victim = pick_target(ch, PT_TOLERANT)))
			insectbite(ch, victim);
	}
	return 0;
}

int yeenoghu(P_char ch, P_char /*tch*/, int cmd, char * /*arg*/)
{
	P_char victim;

	if (cmd == CMD_SET_PERIODIC)
		return FALSE;

	if (cmd)
		return FALSE;

	if (!IS_FIGHTING(ch))
		return FALSE;

	if (!number(0, 9) && GET_HIT(ch) < GET_MAX_HIT(ch) / 2)
	{
		act("$n&+L raises his clawed hands and emits a screeching scream!", TRUE, ch, 0, 0,
		    TO_ROOM);
		spell_full_heal(GET_LEVEL(ch), ch, 0, 0, ch, 0);
	}
	else if (!number(0, 5) && GET_HIT(ch) < GET_MAX_HIT(ch) / 4)
	{
		act("$n&+L raises his clawed hands and emits a screeching scream!", TRUE, ch, 0, 0,
		    TO_ROOM);
		spell_full_heal(GET_LEVEL(ch), ch, 0, 0, ch, 0);
	}
	else if (!number(0, 6))
		do_fetid_breath(ch);
	else if (!number(0, 5))
		whirlwind_of_teetch(ch, dice(2, 6));
	else if (!number(0, 3))
	{
		if ((victim = pick_target(ch, PT_NUKETARGET | PT_WEAKEST)))
			hyena_bite(ch, victim);
	}
	return 0;
}
