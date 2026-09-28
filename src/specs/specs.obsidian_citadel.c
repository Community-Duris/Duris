/* Special procedures for the Obsidian Citadel. */

#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "world/db.h"
#include "world/events.h"
#include "cmd/interp.h"
#include "core/utils.h"
#include "world/specs.prototypes.h"
#include "magic/spells.h"

extern P_room world;

/*
 * satar ghulan in obsidian citadel summons up to two flesh golems
 */

int obsid_cit_satar_ghulan(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return FALSE;

	if (cmd)
		return FALSE;

	if (!ch)
		return FALSE; /* boggle */

	/* summon beastie #75648, max of 2, let the func set duration itself */

	if (summon_creature(75648, ch, 2, 0, NULL,
			    "&+MWith an arcane gesture, &n$N&+M suddenly "
			    "summons a flesh golem to do $S bidding!"))
		return TRUE;
	else
		return FALSE;
}

/*
 * death knight proc for obsidian citadel, make em cast fireball and
 * incend cloud now and then
 */

int obsid_cit_death_knight(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	P_char tch, vict = NULL;
	int numbPCs = 0, luckyPC = 0, currPC = 0, numb;

	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return FALSE;

	if (cmd)
		return FALSE;

	if (!IS_FIGHTING(ch))
		return FALSE;

	if (!GET_OPPONENT(ch))
		return FALSE;

	/* count number of PCs, pick someone */

	for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
	{
		if (IS_PC(tch) && !IS_TRUSTED(tch))
			numbPCs++;
	}

	if (!numbPCs)
		return FALSE; /* doh */

	if (numbPCs == 1)
		luckyPC = 0;
	else
		luckyPC = number(0, numbPCs - 1);

	for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
	{
		if (IS_PC(tch) && !IS_TRUSTED(tch))
		{
			if (currPC == luckyPC)
			{
				vict = tch;
				break;
			}
			else
				currPC++;
		}
	}

	if (!vict) /* hmm, error */
	{
		act("$n says 'bug in my proc!  tell a god!'", 1, ch, 0, 0, TO_ROOM);
		return FALSE;
	}

	numb = number(1, 100);

	/* 5% chance of incendiary cloud of death */

	if (numb <= 5)
	{
		act("$n&n's body suddenly &+Rglows brightly&n!", 1, ch, 0, 0, TO_ROOM);
		spell_incendiary_cloud((int)(GET_LEVEL(ch) * 1.5), ch, 0, SPELL_TYPE_SPELL, vict,
				       0);
		return TRUE;
	}

	/* 25% chance of fireball */

	else if (numb >= 75)
	{
		act("$n&n's body suddenly &+Rglows brightly&n!", 1, ch, 0, 0, TO_ROOM);
		spell_fireball((int)(GET_LEVEL(ch) * 1.5), ch, 0, SPELL_TYPE_SPELL, vict, 0);
		return TRUE;
	}

	else
		return FALSE;
}