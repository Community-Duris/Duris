/* Pet creation and ownership lifecycle helpers. */

#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "world/db.h"
#include "world/handler.h"
#include "core/utils.h"
#include "world/specs.prototypes.h"

extern P_room world;

/*
 * summon_creature - summons one particular type of creature to master
 *      (leaving stats of NPC summoned alone) can control max numb and
 *      duration - returns pointer to P_char summoned or NULL if some error
 */

P_char summon_creature(int mobnumb, P_char master, int max_summon, int dur, const char *appearsC,
		       const char *appears)
{
	P_char mob;
	struct follow_type *k;
	int i;

	if (!master || (mobnumb < 0) || (real_mobile(mobnumb) == -1))
	{
		return NULL;
	}

	if (dur <= 0)
		dur = (GET_LEVEL(master) * 4) + 10;

	if (CHAR_IN_SAFE_ROOM(master))
	{
		send_to_char("A mysterious force blocks your summoning!\r\n", master);
		return NULL;
	}

	if (max_summon)
	{
		for (k = master->followers, i = 0; k; k = k->next)
		{
			if (k->follower && IS_NPC(k->follower) &&
			    (GET_VNUM(k->follower) == mobnumb))
				i++;
		}

		if (i >= max_summon)
		{
			send_to_char("You cannot bind any more creatures to your control.\r\n",
				     master);
			return NULL;
		}
	}

	mob = read_mobile(real_mobile(mobnumb), REAL, false);
	if (!mob)
	{
		logit(LOG_DEBUG, "summon_creature(): mob %d not loadable", mobnumb);
		return NULL;
	}

	char_to_room(mob, master->in_room, 0);

	while (mob->affected)
		affect_remove(mob, mob->affected);

	if (!IS_SET(mob->specials.act, ACT_MEMORY))
		clearMemory(mob);

	balance_affects(mob);

	if (appears)
		act(appears, FALSE, master, 0, master, TO_ROOM);
	if (appearsC)
		act(appearsC, FALSE, master, 0, 0, TO_CHAR);

	setup_pet(mob, master, dur, 0);

	add_follower(mob, master);
	group_add_member(master, mob);

	return mob;
}

/*
 *  if vict is charmed by someone other than master, make em charmed
 *  by master (group em, too) - if madatOldMaster is set, add old master's
 *  name to memory
 */

int recharm_ch(P_char master, P_char vict, bool madatOldMaster, char *charmMsg)
{
	P_char oldmast = NULL, tmpch;

	if (IS_NPC(vict) && (IS_GREATER_DRACO(vict) || IS_GREATER_AVATAR(vict)))
		return FALSE;

	if (!master || !vict || (master == vict) || !IS_NPC(vict) || !IS_PC_PET(vict) ||
	    (vict->in_room != master->in_room))
		return FALSE;

	if (GET_MASTER(vict) == master)
		return FALSE; /* already charmed */

	/* we're successful, baby */

	if (charmMsg)
		act(charmMsg, TRUE, master, 0, vict, TO_ROOM);

	stop_fighting(vict);
	stop_fighting(master);

	/* stop all combat with new charmie */

	for (tmpch = world[vict->in_room].people; tmpch; tmpch = tmpch->next_in_room)
		if ((tmpch != vict) && (GET_OPPONENT(tmpch) == vict))
			stop_fighting(tmpch);

	if (vict->following)
	{
		oldmast = vict->following;
		stop_follower(vict);
	}

	add_follower(vict, master);

	setup_pet(vict, master, 24 * 18, 0);

	if (vict->group)
		group_remove_member(vict);

	group_add_member(master, vict);

	//  SET_BIT(vict->specials.act, ACT_AGGRESSIVE);
	SET_BIT(vict->only.npc->aggro_flags, AGGR_ALL);
	SET_BIT(vict->specials.act, ACT_SENTINEL);
	SET_BIT(vict->specials.act, ACT_PROTECTOR);

	if (madatOldMaster && oldmast)
		remember(vict, oldmast);

	return TRUE;
}
