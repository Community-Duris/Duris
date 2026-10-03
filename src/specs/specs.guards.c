/* Shared guard special procedures. */

#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "world/db.h"
#include "world/events.h"
#include "world/handler.h"
#include "cmd/interp.h"
#include "core/utility.h"
#include "core/utils.h"
#include "world/specs.prototypes.h"
#include <stdio.h>

extern P_room world;
extern const char *command[];

int good_city_guard(P_char ch, P_char tch, int cmd, char * /*arg*/)
{
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;
	if (!tch)
		return shout_and_hunt(ch, 100, "%s has dared to attack me!", good_city_guard, NULL,
				      0, 0);
	return FALSE;
}

int guild_guard(P_char ch, P_char pl, int cmd, char * /*arg*/)
{
	bool g_prot = FALSE, block = FALSE;
	char Gbuf1[MAX_STRING_LENGTH];
	int Guild_Eq;

	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return FALSE;

	if (!ch || !pl)
		return FALSE;

	if ((ch->in_room != real_room(GET_BIRTHPLACE(ch))) || (ch->in_room == NOWHERE))
		return FALSE;

	/*
	 * ok, for ease of addition, add the virtual room number that the guard loads
	 *
	 * in as a case to the following switch.  After the case, set g_prot = TRUE
	 * if this guard is supposed to use the guild_protector special (blast them,
	 * outlaw them, teleport them away).  Check class restrictions and set block
	 * = TRUE if the guard should block 'pl'. Note that guild guard's home is set
	 *
	 * to the room they load in, and this prevents them from doing ANYTHING
	 * special, when they aren't in that room. Charmed, fled, transed, etc.  JAB
	 */

	switch (world[ch->in_room].number)
	{
	case 17643:
		g_prot = FALSE;
		if (cmd == CMD_DOWN)
			block = TRUE;
		break;
	case 7528:
		g_prot = TRUE;
		if ((cmd == CMD_SOUTH) && !GET_CLASS(pl, CLASS_ROGUE))
			block = TRUE;
		break;
	case 7565:
		g_prot = TRUE;
		if (((cmd == CMD_EAST) || (cmd == CMD_WEST)) && !GET_CLASS(pl, CLASS_SHAMAN))
			block = TRUE;
		break;
	case 7578:
		g_prot = TRUE;
		if (((cmd == CMD_EAST) || (cmd == CMD_SOUTH)) && !GET_CLASS(pl, CLASS_WARRIOR))
			block = TRUE;
		break;
	case 7584:
		g_prot = TRUE;
		if ((cmd == CMD_NORTH) && !GET_CLASS(pl, CLASS_NECROMANCER) &&
		    !GET_CLASS(pl, CLASS_SUMMONER) && !GET_CLASS(pl, CLASS_SORCERER) &&
		    !GET_CLASS(pl, CLASS_CONJURER))
			block = TRUE;
		break;
	case 7588:
		g_prot = TRUE;
		if ((cmd == CMD_EAST) && !GET_CLASS(pl, CLASS_CLERIC))
			block = TRUE;
		break;
	case 9300:
	case 9301:
		if (cmd == CMD_DOWN)
		{
			if (pl->equipment[GUILD_INSIGNIA])
				Guild_Eq = obj_index[pl->equipment[GUILD_INSIGNIA]->R_num]
						   .virtual_number;
			else
				Guild_Eq = 0;
			if ((Guild_Eq == 9301) || (Guild_Eq == 9302))
				break;
			else
				block = TRUE;
		}
		break;
	case 16501:
		if (cmd == CMD_NORTH)
		{
			if (pl->equipment[GUILD_INSIGNIA])
				Guild_Eq = obj_index[pl->equipment[GUILD_INSIGNIA]->R_num]
						   .virtual_number;
			else
				Guild_Eq = 0;
			if (Guild_Eq == 9316)
				break;
			else
				block = TRUE;
		}
		break;
	case 11603:
		g_prot = TRUE;
		if ((cmd == CMD_WEST) && !GET_CLASS(pl, CLASS_WARRIOR))
			block = TRUE;
		break;
	case 11633:
		g_prot = TRUE;
		if ((cmd == CMD_WEST) && !GET_CLASS(pl, CLASS_SHAMAN))
			block = TRUE;
		break;
	case 11619:
		g_prot = TRUE;
		if ((cmd == CMD_SOUTH) && !GET_CLASS(pl, CLASS_MERCENARY))
			block = TRUE;
		break;
	case 8305:
		g_prot = TRUE;
		if ((cmd == CMD_EAST) && !GET_CLASS(pl, CLASS_RANGER))
			block = TRUE;
		break;
	case 8070:
	case 17550:
		g_prot = TRUE;
		if ((cmd == CMD_WEST) && !GET_CLASS(pl, CLASS_CLERIC))
			block = TRUE;
		break;
	case 8311:
		g_prot = TRUE;
		if ((cmd == CMD_SOUTH) && !GET_CLASS(pl, CLASS_NECROMANCER))
			block = TRUE;
		break;
	case 8318:
		g_prot = TRUE;
		if ((cmd == CMD_NORTH) && !GET_CLASS(pl, CLASS_BARD))
			block = TRUE;
		break;
	case 8200:
	case 17135:
		g_prot = TRUE;
		if ((cmd == CMD_WEST) && !GET_CLASS(pl, CLASS_ROGUE))
			block = TRUE;
		break;
	case 8137:
		g_prot = TRUE;
		if ((cmd == CMD_SOUTH) && !GET_CLASS(pl, CLASS_DRUID))
			block = TRUE;
		break;
	case 8113:
		g_prot = TRUE;
		if ((cmd == CMD_SOUTH) && !GET_CLASS(pl, CLASS_SORCERER))
			block = TRUE;
		break;
	case 8014:
	case 17221:
		g_prot = TRUE;
		if ((cmd == CMD_SOUTH) && !GET_CLASS(pl, CLASS_WARRIOR))
			block = TRUE;
		break;
	case 16056:
		g_prot = TRUE;
		if ((cmd == CMD_NORTH) && !GET_CLASS(pl, CLASS_CLERIC))
			block = TRUE;
		break;
	case 16392:
		g_prot = TRUE;
		if ((cmd == CMD_SOUTH) && !GET_CLASS(pl, CLASS_ROGUE))
			block = TRUE;
		break;
	case 16192:
		g_prot = TRUE;
		if ((cmd == CMD_NORTH) && !GET_CLASS(pl, CLASS_SORCERER))
			block = TRUE;
		break;
	case 16408:
		g_prot = TRUE;
		if ((cmd == CMD_NORTH) && !GET_CLASS(pl, CLASS_ROGUE))
			block = TRUE;
		break;
	case 16283:
		g_prot = TRUE;
		if ((cmd == CMD_SOUTH) && !GET_CLASS(pl, CLASS_SHAMAN))
			block = TRUE;
		break;
	case 16383:
		g_prot = TRUE;
		if ((cmd == CMD_SOUTH) && !GET_CLASS(pl, CLASS_SHAMAN))
			block = TRUE;
		break;
	case 16007:
		g_prot = TRUE;
		if ((cmd == CMD_WEST) && !GET_CLASS(pl, CLASS_WARRIOR))
			block = TRUE;
		break;
	case 16145:
		g_prot = TRUE;
		if ((cmd == CMD_EAST) && !GET_CLASS(pl, CLASS_SORCERER))
			block = TRUE;
		break;
	case 17086:
		g_prot = TRUE;
		if ((cmd == CMD_NORTH) && !GET_CLASS(pl, CLASS_MERCENARY))
			block = TRUE;
		break;
	case 17564:
		g_prot = TRUE;
		if ((cmd == CMD_EAST) && !GET_SPEC(pl, CLASS_ROGUE, SPEC_ASSASSIN))
			block = TRUE;
		break;
	case 139000:
	case 25001:
	case 25086:
	case 25201:
	case 19859:
	case 4128:
		if (cmd == CMD_NORTH)
			block = TRUE;
		break;
	case 8044:
	case 8046:
	case 11685:
		if (cmd == CMD_EAST)
			block = TRUE;
		break;
	case 8087:
		if (((cmd == CMD_EAST) && (GET_RACE(pl) != RACE_GREY) &&
		     (GET_RACE(pl) != RACE_HALFELF && (GET_RACE(pl) != RACE_CENTAUR))))
			block = TRUE;
		break;
	case 45017:
		if (((cmd == CMD_NORTH) && (GET_RACE(pl) != RACE_GREY) &&
		     (GET_RACE(pl) != RACE_HALFELF && (GET_RACE(pl) != RACE_CENTAUR))))
			block = TRUE;
		break;
	case 11812:
		if (cmd == CMD_UP)
			block = TRUE;
		break;
	case 11008:
	case 11208:
	case 19950:
	case 25320:
	case 25326:
	case 19951:
	case 19954:
		if (cmd == CMD_SOUTH)
			block = TRUE;
		break;
	case 8053:
		if (cmd == CMD_WEST)
			block = TRUE;
		break;
	case 17343:
		if (cmd == CMD_DOWN)
			block = TRUE;
		break;
	case 17345:
		if (cmd == CMD_DOWN)
			block = TRUE;
		break;
	case 17347:
		if (cmd == CMD_DOWN)
			block = TRUE;
		break;

		/*
			 * Ashrumite
			 */
	case 66065:
		g_prot = TRUE;
		if ((cmd == CMD_WEST) && !GET_CLASS(pl, CLASS_SHAMAN))
			block = TRUE;
		break;
	case 66088:
		g_prot = TRUE;
		if ((cmd == CMD_EAST) && !GET_CLASS(pl, CLASS_CLERIC))
			block = TRUE;
		break;
	case 66028:
		g_prot = TRUE;
		if ((cmd == CMD_SOUTH) && !GET_CLASS(pl, CLASS_ROGUE))
			block = TRUE;
		break;
	case 66084:
		g_prot = TRUE;
		if ((cmd == CMD_NORTH) && !GET_CLASS(pl, CLASS_SORCERER) &&
		    !GET_CLASS(pl, CLASS_SUMMONER) && !GET_CLASS(pl, CLASS_CONJURER))
			block = TRUE;
		break;
	case 66078:
		g_prot = TRUE;
		if ((cmd == CMD_SOUTH) && !GET_CLASS(pl, CLASS_WARRIOR))
			block = TRUE;
		break;
	} /*
	   * end switch
	   */

	/*
	 * code added to allow mobs which are hunting their "home" to pass.
	 */

	if (IS_NPC(pl))
	{
		P_nevent ev;

		LOOP_EVENTS_CH(ev, pl->nevents)
		{
			if (ev->func == event_mob_hunt)
			{
				break;
			}
		}
		if ((ev) && ((hunt_data *)(ev->data))->hunt_type == HUNT_ROOM &&
		    ((hunt_data *)(ev->data))->target_room == real_room(pl->player.birthplace))
			block = FALSE;
	}
	if (g_prot && IS_FIGHTING(ch) && (cmd == 0))
	{
		if (guild_protection(ch, GET_OPPONENT(ch)))
			return (TRUE);
	}
	if (block)
	{
		if (!IS_TRUSTED(pl))
		{
			act("$N humiliates you, and blocks your way.", FALSE, pl, 0, ch, TO_CHAR);
			act("$N humiliates $n, and blocks $s way.", FALSE, pl, 0, ch, TO_NOTVICT);
		}
		else
		{
			snprintf(Gbuf1, MAX_STRING_LENGTH,
				 "$N bows before you, saying 'Right this way, My %s'",
				 (GET_SEX(pl) == SEX_FEMALE) ? "Lady" : "Lord");
			act(Gbuf1, FALSE, pl, 0, ch, TO_CHAR);
			snprintf(Gbuf1, MAX_STRING_LENGTH,
				 "$N bows before $n, saying 'Right this way, My %s'",
				 (GET_SEX(pl) == SEX_FEMALE) ? "Lady" : "Lord");
			act(Gbuf1, FALSE, pl, 0, ch, TO_NOTVICT);
			return FALSE;
		}
		return TRUE;
	}
	return FALSE;
}

/*
 * this is a copy of guild_guard proc with a nasty twist.  Each PULSE_MOBILE
 * the guardian check the room it's supposed to be guarding, if there are
 * people in the room, it will attack them.  This is for use by mobs guarding
 * vaults and such.  And they will attack ANYONE in the room (including other
 * mobs) (excluding only gods).  They also block, like normal guild_guards.
 * -JAB
 */

int guardian(P_char ch, P_char pl, int cmd, char * /*arg*/)
{
	int block_dir = 0, i;
	P_char t_ch;
	char Gbuf1[MAX_STRING_LENGTH];

	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return FALSE;

	if (!ch)
		return FALSE;

	if ((ch->in_room != real_room(GET_HOME(ch))) || (ch->in_room == NOWHERE))
	{
		/*
		 * don't let them get out of position, unless charmed, which shouldn't
		 * happen with these guys anyway.
		 */
		if (!IS_AWAKE(ch) || IS_FIGHTING(ch))
			return FALSE;

		act("$N looks around frantically, then vanishes in a small puff of smoke", FALSE,
		    ch, 0, 0, TO_ROOM);
		char_from_room(ch);
		char_to_room(ch, real_room(GET_HOME(ch)), -1);

		return FALSE;
	}
	/*
	 * ok, for ease of addition, add the virtual room number that the guard loads
	 *
	 * in as a case to the following switch.  After the case, set attack = TRUE
	 * if this guard is supposed to use the guild_protector special (blast them,
	 * outlaw them, teleport them away).  Check class restrictions and set block
	 * = TRUE if the guard should block 'pl'. Note that guild guard's home is set
	 *
	 * to the room they load in, and this prevents them from doing ANYTHING
	 * special, when they aren't in that room. Charmed, fled, transed, etc.  JAB
	 */

	switch (world[ch->in_room].number)
	{
	case 8044:
		block_dir = 2;
		break;
	case 8046:
		block_dir = 2;
		break;
	case 8053:
		block_dir = 4;
		break;
	case 11008:
		block_dir = 3;
		break;
	case 11208:
		block_dir = 3;
		break;
	case 25001:
		block_dir = 1;
		break;
	case 25086:
		block_dir = 1;
		break;
	case 25201:
		block_dir = 1;
		break;
	case 97126:
	case 97242:
		block_dir = 3;
		break;
	}

	if (pl && cmd)
	{
		if (cmd == block_dir)
		{
			if (!IS_TRUSTED(pl))
			{
				act("$N humiliates you, and block your way.", FALSE, pl, 0, ch,
				    TO_CHAR);
				act("$N humiliates $n, and blocks $s way.", FALSE, pl, 0, ch,
				    TO_NOTVICT);
			}
			else
			{
				snprintf(Gbuf1, MAX_STRING_LENGTH,
					 "$N bows before you, saying 'Right this way, My %s'",
					 (GET_SEX(pl) == SEX_FEMALE) ? "Lady" : "Lord");
				act(Gbuf1, FALSE, pl, 0, ch, TO_CHAR);
				snprintf(Gbuf1, MAX_STRING_LENGTH,
					 "$N bows before $n, saying 'Right this way, My %s'",
					 (GET_SEX(pl) == SEX_FEMALE) ? "Lady" : "Lord");
				act(Gbuf1, FALSE, pl, 0, ch, TO_ROOM);
				return FALSE;
			}
			return TRUE;
		}
		return FALSE;
	}
	else if (pl && !cmd)
	{
		/*
		 * the twist part
		 */
		if (IS_FIGHTING(ch) || (block_dir < 1))
			return FALSE;

		/*    block_dir--;*/
		block_dir = cmd_to_exitnumb(block_dir);

		if (!EXIT(ch, block_dir) || (EXIT(ch, block_dir)->to_room == NOWHERE))
		{
			logit(LOG_MOB, "bogus room to guard in guardian() for %s, in %d (%s)",
			      ch->player.short_descr, world[ch->in_room].number,
			      command[exitnumb_to_cmd(block_dir)]);
			REMOVE_BIT(ch->specials.act, ACT_SPEC);
			return FALSE;
		}
		i = EXIT(ch, block_dir)->to_room;
		t_ch = world[i].people;

		if (!t_ch)
			return FALSE;

		act("$n snarls angrily, and vanishes in a puff of smoke!", FALSE, ch, 0, 0,
		    TO_ROOM);
		char_from_room(ch);
		char_to_room(ch, i, -1);
		act("Snarling in rage, $n appears and attacks!", FALSE, ch, 0, 0, TO_ROOM);
		MobStartFight(ch, t_ch);
		return TRUE;
	}
	else
	{
		logit(LOG_MOB, "%s guardian special called in room %d with cmd and no target",
		      ch->player.short_descr, world[ch->in_room].number);
	}
	return FALSE;
}

int cityguard(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	P_char tch, tar_ch = NULL;
	int tar_align, a_flag, magnitude;

	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (cmd || !IS_AWAKE(ch) || (IS_FIGHTING(ch)))
		return (FALSE);

	a_flag = (IS_GOOD(ch) ? 1001 : IS_EVIL(ch) ? -1001 : 0);
	tar_align = a_flag;
	magnitude = 0;

	for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
	{
		if (GET_OPPONENT(tch) && CAN_SEE(ch, tch) &&
		    (IS_PC(tch) || (GET_VNUM(ch) != GET_VNUM(tch))))
		{
			switch (a_flag)
			{
			case -1001: /*
				             * guard is evil, nuke most good
				             */
				if (GET_ALIGNMENT(tch) > tar_align)
				{
					tar_align = GET_ALIGNMENT(tch);
					tar_ch = tch;
				}
				break;
			case 0: /*
				         * guard is neutral, get most divergent
				         */
				if (((GET_ALIGNMENT(tch) > 0) &&
				     (GET_ALIGNMENT(tch) > magnitude)) ||
				    ((GET_ALIGNMENT(tch) < 0) && (GET_ALIGNMENT(tch) < -magnitude)))
				{
					tar_ch = tch;
					magnitude = GET_ALIGNMENT(tch);
					if (GET_ALIGNMENT(tch) < 0)
						magnitude = -magnitude;
				}
				break;
			case 1001: /*
				            * guard is good, nuke most evil
				            */
				if (GET_ALIGNMENT(tch) < tar_align)
				{
					tar_align = GET_ALIGNMENT(tch);
					tar_ch = tch;
				}
				break;
			}
		}
	}

	if (tar_ch)
	{
		switch (a_flag)
		{
		case -1001:
			act("$n screams 'PURGE THE INNOCENT! BANZAI! CHARGE! ARARAGGGHH!'", FALSE,
			    ch, 0, 0, TO_ROOM);
			break;
		case 0:
			act("$n screams 'PRESERVE THE BALANCE! BANZAI! CHARGE! ARARAGGGHH!'", FALSE,
			    ch, 0, 0, TO_ROOM);
			break;
		case 1001:
			act("$n screams 'PROTECT THE INNOCENT! BANZAI! CHARGE! ARARAGGGHH!'", FALSE,
			    ch, 0, 0, TO_ROOM);
			break;
		}
		MobStartFight(ch, tar_ch);
		return (TRUE);
	}
	return false;
}
