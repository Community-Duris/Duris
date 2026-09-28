/*
 * ***************************************************************************
 * *  File: specs.mobile.c                                     Part of Duris *
 * *  Usage: special procedures for mobiles                                    *
 * *  Copyright  1990, 1991 - see 'license.doc' for complete information.      *
 * *  Copyright 1994 - 2008 - Duris Systems Ltd.                             *
 * ***************************************************************************
 */

#include "core/prototypes.h"
#include "world/difficulty.h"
#include "core/structs.h"
#include "net/comm.h"
#include "world/db.h"
#include "world/events.h"
#include "cmd/interp.h"
#include "core/utility.h"
#include "core/utils.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include "guild/alliances.h"
#include "guild/assocs.h"
#include "combat/damage.h"
#include "world/epic.h"
#include "net/gmcp.h"
#include "combat/justice.h"
#include "world/map.h"
#include "classes/necromancy.h"
#include "economy/nexus_stones.h"
#include "economy/currency_transaction.h"
#include "economy/economic_gameplay_authority.h"
#include "combat/range.h"
#include "world/specs.prototypes.h"
#include "magic/spells.h"
#include "sql/sql.h"
#include "economy/tradeskill.h"
#include "world/vnum.obj.h"
#include "world/vnum.room.h"
#include "world/weather.h"
#include "world/world_quest.h"
#include "world/world_quest_policy_math.h"

/*
 * external variables
 */

extern P_char character_list;
extern P_desc descriptor_list;
extern P_index mob_index;
extern P_index obj_index;
extern P_room world;
extern const int top_of_world;
extern P_obj justice_items_list;
extern char *coin_names[];
extern const char *command[];
extern const char *dirs[];
// extern const char rev_dir[];
extern const struct stat_data stat_factor[];
extern int planes_room_num[];
extern int racial_base[];
extern int top_of_zone_table;
extern struct command_info cmd_info[MAX_CMD_LIST];
extern struct str_app_type str_app[];
extern struct time_info_data time_info;
extern struct zone_data *zone;
extern struct zone_data *zone_table;
extern const char *specdata[][MAX_SPEC];
extern struct class_names class_names_table[];
extern P_obj object_list;
extern void give_proper_stat(P_char);
extern void insectbite(P_char, P_char);
extern P_char guard_check(P_char, P_char);
extern P_char pick_target(P_char, unsigned int);
extern int cast_as_damage_area(P_char, void (*func)(int, P_char, char *, int, P_char, P_obj), int,
			       P_char, float, float);
extern struct quest_data quest_index[];

struct social_type
{
	char *cmd;
	int next_line;
};

struct obj_cost
{
	int total_cost;
	int no_carried;
	bool ok;
};

int silver_lady_shout(P_char ch, P_char tch, int cmd, char * /*arg*/)
{
	int helpers[] = { 11302, 11303, 11304, 11305, 11307, 11308, 11310, 11312, 11314, 0 };
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;
	if (!tch)
		return shout_and_hunt(ch, 30, "Help me mates!  We be under attack by %s!", NULL,
				      helpers, 0, 0);
	return FALSE;
}

int realms_master_shout(P_char ch, P_char tch, int cmd, char * /*arg*/)
{
	int helpers[] = { 11102, 11103, 11104, 11105, 11107, 11108, 11110, 11112, 11114, 0 };
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;
	if (!tch)
		return shout_and_hunt(ch, 30, "Help me mates!  We be under attack by %s!", NULL,
				      helpers, 0, 0);
	return FALSE;
}

int caranthazal_shout(P_char ch, P_char tch, int cmd, char * /*arg*/)
{
	int helpers[] = { 32835, 32836, 0 };
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;
	if (!tch && !number(0, 4))
		return shout_and_hunt(
			ch, 100, "&+MBrethren, we have been invaded, come to me and dispose of %s!",
			NULL, helpers, 0, 0);
	return FALSE;
}

int shadow_demon(P_char ch, P_char tch, int cmd, char *arg)
{
	P_char vict;
	int Mask;
	char buf[MAX_INPUT_LENGTH], password[MAX_INPUT_LENGTH];

	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return FALSE;

	if (cmd != CMD_WHISPER) /*
	                         * Whisper
	                         */
		return (FALSE);

	if ((!CAN_SEE(ch, tch) || !CAN_SEE(tch, ch)) || IS_FIGHTING(ch) || !IS_AWAKE(ch)) /*
	                                                                                   * if dragon can't see player
	                                                                                   */
		return (FALSE);

	if (cmd == CMD_WHISPER)
	{
		half_chop(arg, buf, password);
		if (!*buf || !*password || (!(vict = get_char_room_vis(tch, buf))) || (vict != ch))
			return (FALSE);

		if (str_cmp(password, "masque")) /*
		                                  * Password
		                                  */
			return (FALSE);

		if (CAN_SEE(ch, tch) && IS_PC(tch) && !GET_OPPONENT(tch))
		{
			if (tch->equipment[GUILD_INSIGNIA])
				Mask = obj_index[tch->equipment[GUILD_INSIGNIA]->R_num]
					       .virtual_number;
			else
				Mask = 0;
			if ((Mask == 8501) || (Mask == 8502) || (Mask == 8503))
			{
				act("$n whispers, 'Greetings follower of Mask!'\r\n"
				    "The demon utters an arcane magical phrase, casting a powerful incantation!\r\n"
				    "&+LThe room blackens with dark energy, shadows envelop the room........",
				    FALSE, ch, 0, 0, TO_ROOM);
				act("&+L$N is lost to the shadows of darkness and slowly slips from sight...",
				    FALSE, ch, 0, tch, TO_NOTVICT);
				snprintf(
					buf, MAX_INPUT_LENGTH,
					"&+LThe shadows lift for a moment as %s fades into exsistance..\r\n",
					GET_NAME(tch));
				send_to_room(buf, real_room(8450));
				send_to_char(
					"&+LA dark shadow envelops you as you fade from the room!\r\n"
					"The darkness vanishes and you stand inside the guild hall of the\r\n"
					"&+L<<=Shadowys of Dought=>>&N..\r\n",
					tch);
				char_from_room(tch);
				char_to_room(tch, real_room(8450), 0);
				act("The shadows retreat as the mighty demon goes back into hiding......",
				    FALSE, ch, 0, 0, TO_ROOM);
				return (TRUE);
			}
		}
	}
	return (FALSE);
}

int tiaka_ghoul(P_char ch, P_char tch, int cmd, char *arg)
{
	P_char vict;
	int Mask, GoodAlignment = 1; /*
	                                 * Define what tiaka consider a good align.
	                                 */
	char buf[MAX_INPUT_LENGTH], password[MAX_INPUT_LENGTH];

	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return FALSE;

	if (cmd != CMD_WHISPER)
		return (FALSE);

	if ((!CAN_SEE(ch, tch) || !CAN_SEE(tch, ch)) || IS_FIGHTING(ch) || !IS_AWAKE(ch)) /*
	                                                                                   * if dragon can't see player
	                                                                                   */
		return (FALSE);

	if (cmd == CMD_WHISPER)
	{
		half_chop(arg, buf, password);
		if (!*buf || !*password || (!(vict = get_char_room_vis(tch, buf))) || (vict != ch))
			return (FALSE);

		if (str_cmp(password, "blackend")) /*
		                                    * Password
		                                    */
			return (FALSE);

		if (CAN_SEE(ch, tch) && !IS_NPC(tch) && !GET_OPPONENT(tch))
		{
			if (tch->equipment[GUILD_INSIGNIA])
				Mask = obj_index[tch->equipment[GUILD_INSIGNIA]->R_num]
					       .virtual_number;
			else
				Mask = 0;
			if ((Mask == 1263) || (Mask == 1264))
			{
				if (GET_ALIGNMENT(tch) > GoodAlignment)
				{
					act("$n glances at $N, peering into $N's soul.", FALSE, ch,
					    0, tch, TO_NOTVICT);
					act("$n glances at you, scanning your soul.", FALSE, ch, 0,
					    tch, TO_VICT);
					act("$n grins at $N, and says 'Thou shall surly perish if thou stays here.'",
					    FALSE, ch, 0, tch, TO_NOTVICT);
					act("$n grins at you, and says 'Thou shall surly perish if thou stays here.'",
					    FALSE, ch, 0, tch, TO_VICT);
					return (FALSE);
				}
				act("$n says, 'Thou are worth to enter.'", FALSE, ch, 0, 0,
				    TO_ROOM);
				act("&+rTiaka&N bows to you, and opens the portal to the guildhall.",
				    FALSE, ch, 0, 0, TO_ROOM);
				act("&+RThe portal swirls around the room, shifting colors slightly.",
				    FALSE, ch, 0, 0, TO_ROOM);

				act("&+r$N is enveloped into the portal, and slowly fades out of exsistance.",
				    FALSE, ch, 0, tch, TO_NOTVICT);
				snprintf(
					buf, MAX_INPUT_LENGTH,
					"&+rA portal opens up inside the room and %s steps through it..\r\n",
					GET_NAME(tch));
				send_to_room(buf, real_room(8556));
				send_to_char(
					"You are partially blinded as you step through the portal.\r\n",
					tch);
				send_to_char(
					"The light of the portal fades slowly from the room.\r\n",
					tch);
				char_from_room(tch);
				char_to_room(tch, real_room(8556), 0);
				act("Tiaka closes the portal, and stands back at attention.", FALSE,
				    ch, 0, 0, TO_ROOM);
				return (TRUE);
			}
		}
	}
	return (FALSE);
}

int mystra_dragon(P_char ch, P_char tch, int cmd, char *arg)
{
	P_char vict;
	int Mask, EvilAlignment = -350;
	char buf[MAX_INPUT_LENGTH], password[MAX_INPUT_LENGTH];

	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return FALSE;

	if (cmd != CMD_WHISPER) /*
	                         * Whisper
	                         */
		return (FALSE);

	if ((!CAN_SEE(ch, tch) || !CAN_SEE(tch, ch)) || IS_FIGHTING(ch) || !IS_AWAKE(ch)) /*
	                                                                                   * if dragon can't see player
	                                                                                   */
		return (FALSE);

	if (cmd == CMD_WHISPER)
	{
		half_chop(arg, buf, password);
		if (!*buf || !*password || (!(vict = get_char_room_vis(tch, buf))) || (vict != ch))
			return (FALSE);

		if (str_cmp(password, "rovmelek")) /*
		                                    * Password
		                                    */
			return (FALSE);

		if (CAN_SEE(ch, tch) && IS_PC(tch) && !GET_OPPONENT(tch))
		{
			if (tch->equipment[GUILD_INSIGNIA])
				Mask = obj_index[tch->equipment[GUILD_INSIGNIA]->R_num]
					       .virtual_number;
			else
				Mask = 0;
			if ((Mask == 1240) || (Mask == 1241) || (Mask == 1242) || (Mask == 1243))
			{
				if (GET_ALIGNMENT(tch) < EvilAlignment)
				{
					act("$n looks at $N with a penetrating stare, scanning $M.\r\n"
					    "$n roars with rage at $N, \r\n"
					    "'Evil wretch! You cannot enter Mystra's holy sanctum!'",
					    FALSE, ch, 0, tch, TO_NOTVICT);
					act("$n looks at you with a penetrating stare, scanning you.\r\n"
					    "$n roars with rage at you, \r\n"
					    "'Evil wretch! You cannot enter Mystra's holy sanctum!'",
					    FALSE, ch, 0, tch, TO_VICT);
					return (FALSE);
				}
				act("$n roars, 'Hail to the faithful of Mystra!'\r\n"
				    "The dragon utters an arcane magical phrase, casting a powerful incantation!\r\n"
				    "&+BThe room crackles with mystical energy, blue sparks shimmer and dance about.",
				    FALSE, ch, 0, 0, TO_ROOM);
				act("&+b$N is enveloped in a blue aura, and slowly fades out of exsistance.",
				    FALSE, ch, 0, tch, TO_NOTVICT);
				snprintf(
					buf, MAX_INPUT_LENGTH,
					"&+bA soft aura of light fills the room as %s fades into exsistance..\r\n",
					GET_NAME(tch));
				send_to_room(buf, real_room(8512));
				send_to_char(
					"&+bA soft aura of light surrounds you as you fade from the room!\r\n"
					"The aura vanishes and you stand inside the holy temple of Mystra..\r\n",
					tch);
				char_from_room(tch);
				char_to_room(tch, real_room(8512), 0);
				act("The aura of magic ebbs, and the great dragon goes back to it's contemplation.",
				    FALSE, ch, 0, 0, TO_ROOM);
				return (TRUE);
			}
		}
	}
	return (FALSE);
}

int hunt_cat(P_char ch, P_char tch, int cmd, char *arg)
{
	P_char vict;
	int Mask, EvilAlignment = -350; /*
	                                    * Define what cat consider an evil
	                                    * align.
	                                    */
	char buf[MAX_INPUT_LENGTH], password[MAX_INPUT_LENGTH];

	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return FALSE;

	if ((cmd != CMD_WHISPER) && (cmd != CMD_MOUNT))
		return (FALSE);

	if ((!CAN_SEE(ch, tch) || !CAN_SEE(tch, ch)) || IS_FIGHTING(ch) || !IS_AWAKE(ch)) /*
	                                                                                   * if cat can't see player
	                                                                                   */
		return (FALSE);

	if (cmd == CMD_WHISPER)
	{
		half_chop(arg, buf, password);
		if (!*buf || !*password || (!(vict = get_char_room_vis(tch, buf))) || (vict != ch))
			return (FALSE);

		if (str_cmp(password, "eternal")) /*
		                                   * Password
		                                   */
			return (FALSE);

		if (CAN_SEE(ch, tch) && IS_PC(tch) && !GET_OPPONENT(tch))
		{
			if (tch->equipment[GUILD_INSIGNIA])
				Mask = obj_index[tch->equipment[GUILD_INSIGNIA]->R_num]
					       .virtual_number;
			else
				Mask = 0;
			if ((Mask == 8427) || (Mask == 8428) || (Mask == 8429))
			{
				if (GET_ALIGNMENT(tch) < EvilAlignment)
				{
					act("$n looks at $N with a penetrating stare, scanning $M.",
					    FALSE, ch, 0, tch, TO_NOTVICT);
					act("$n looks at you with a penetrating stare, scanning you.",
					    FALSE, ch, 0, tch, TO_VICT);
					act("$n roars with rage at $N, 'Evil wretch! You cannot enter &+RHouse Crimsonesti!!'",
					    FALSE, ch, 0, tch, TO_NOTVICT);
					act("$n roars at you, 'Evil wretch! You cannot enter &+RHouse Crimsonesti!!'",
					    FALSE, ch, 0, tch, TO_VICT);
					return (FALSE);
				}
				act("$n roars, 'Long life to the faithful of Labelas!'", FALSE, ch,
				    0, 0, TO_ROOM);
				act("The cat utters and arcane magical phrase, casting a powerful incantation!",
				    FALSE, ch, 0, 0, TO_ROOM);
				act("&+RThe room crackles with mystical energy, crimson sparks shimmer and dance about.",
				    FALSE, ch, 0, 0, TO_ROOM);
				act("&+R$N is enveloped in a crimson aura, and slowly fades out of exsistance.",
				    FALSE, ch, 0, tch, TO_NOTVICT);
				snprintf(
					buf, MAX_INPUT_LENGTH,
					"&+RA soft aura of light fills the room as %s fades into exsistance..\r\n",
					GET_NAME(tch));
				send_to_room(buf, real_room(8426));
				send_to_char(
					"&+RA soft aura of light surrounds you as you fade from the room!\r\n",
					tch);
				send_to_char(
					"The aura vanishes and you stand inside the house of Crimsonesti!..\r\n",
					tch);
				char_from_room(tch);
				char_to_room(tch, real_room(8426), 0);
				act("The aura of magic ebbs, and the great cat goes back to it's contemplation.",
				    FALSE, ch, 0, 0, TO_ROOM);
				return (TRUE);
			}
		}
	}
	else if (cmd == CMD_MOUNT)
	{
		/*
		 * okay, this is a stupid kludge, but it's basically so Tim can ride his **
		 *
		 * damned cat and mortals can't ride the one guarding house crimsonesti, **
		 *
		 * not that that's even used lately, but whatever...i'm too lazy to **
		 * duplicate the cat with a different v-number and reassign the proc ** -
		 * DTS 6/21/95
		 */
		if (GET_LEVEL(tch) < MINLVLIMMORTAL)
		{
			act("$n growls as you try to mount $m.  You rethink the idea.", FALSE, ch,
			    0, tch, TO_VICT);
			act("$n growls as $N tries to mount $m.  $N rethinks the idea.", TRUE, ch,
			    0, tch, TO_NOTVICT);
			return (TRUE);
		}
		return (FALSE);
	}
	return (FALSE);
}

#define GUILD_ITEM_START 8508
#define GUILD_ITEM_END 8513
#define GUILD_ITEM_POS GUILD_INSIGNIA

int mailed_fist_guardian(P_char ch, P_char vict, int cmd, char * /*arg*/)
{
	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (ch->in_room != real_room(8574))
		return FALSE;

	if (cmd == CMD_NORTH)
	{
		if (vict->equipment[GUILD_ITEM_POS] &&
		    (obj_index[vict->equipment[GUILD_ITEM_POS]->R_num].virtual_number >=
		     GUILD_ITEM_START) &&
		    (obj_index[vict->equipment[GUILD_ITEM_POS]->R_num].virtual_number <=
		     GUILD_ITEM_END))
		{
			act("The guard bows as $n enters the guild.", TRUE, ch, 0, 0, TO_ROOM);
			act("The guard bows as you enter the guild.", TRUE, ch, 0, 0, TO_CHAR);
			mobsay(ch, "Welcome, guildmember!");
			return FALSE;
		}
		else
		{
			mobsay(ch,
			       "You look way too shifty to be member of guild dedicated to justice and duty!");
			act("$n blocks your entrance to the guild.", TRUE, ch, 0, vict, TO_VICT);
			act("$n blocks $N's entrance to the guild.", TRUE, ch, 0, vict, TO_NOTVICT);
			act("You block $N's entrance to the guild.", TRUE, ch, 0, vict, TO_CHAR);
			return TRUE;
		}
	}
	if (vict)
		return FALSE;

	if (IS_FIGHTING(ch))
	{
		switch (number(1, 8))
		{
		case 3:
			mobsay(ch,
			       "If you're interested in upholding law and justice, and doing your duty..");
			mobsay(ch, "Contact nearest member of the guild.");
			break;
			/*
				 * Waiting for _N_ other new messages.. :P
				 */
		}
	}
	return FALSE;
}

#undef GUILD_ITEM_START
#undef GUILD_ITEM_END
#undef GUILD_ITEM_POS

int seas_coral_golem(P_char ch, P_char pl, int cmd, char * /*arg*/)
{
	int earring = 0;
	char Gbuf3[MAX_STRING_LENGTH];

	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if ((ch->in_room == real_room(20200)) && (cmd == CMD_SOUTH))
	{
		if (pl->equipment[GUILD_INSIGNIA])
			earring = obj_index[pl->equipment[GUILD_INSIGNIA]->R_num].virtual_number;

		if (earring == 20202)
		{ /*
		   * abalone earring
		   */

			act("The coral golem bows before $n as $e enters the cave.", FALSE, pl, 0,
			    0, TO_ROOM);
			send_to_char("The coral golem bows before you as you enter.\r\n", pl);
			act("$N leaves east, entering the cave.", FALSE, ch, 0, pl, TO_NOTVICT);
			snprintf(Gbuf3, MAX_STRING_LENGTH, "%s arrives from the west.\r\n",
				 (GET_NAME(pl)));
			send_to_room(Gbuf3, real_room(20201));
			char_from_room(pl);
			char_to_room(pl, real_room(20201), 0);
			return TRUE;
		}
		else
		{
			act("The coral golem blocks $n's entry into the cave.", FALSE, pl, 0, 0,
			    TO_ROOM);
			send_to_char("The coral golem blocks your entry into the cave.\r\n", pl);
			send_to_char(
				"The coral golem says 'Only those of the Underground Seas may enter here.'\r\n",
				pl);
			return TRUE;
		}
	}
	if (pl)
	{
		return (0);
	}
	else if (MIN_POS(ch, POS_STANDING + STAT_NORMAL))
	{
		switch (dice(2, 6))
		{
		case 2:
			act("$n seems to come to life as it smiles to you.", TRUE, ch, 0, 0,
			    TO_ROOM);
			break;
		case 3:
			mobsay(ch, "Hail Valkur God of the Winds and the Oceans!");
			break;
		case 4:
			mobsay(ch, "Humans and barbarians!");
			mobsay(ch, "Speak to Prime about the Underground Seas!");
			break;
		case 5:
			act("$n keeps a close eye on you.", TRUE, ch, 0, 0, TO_ROOM);
			break;
		case 6:
			mobsay(ch, "Humans and Barbarians!");
			mobsay(ch,
			       "Speak to an Overseer about membership to the Underground Seas!");
			break;
		default:
			break;
		}
	}
	return FALSE;
}

/*
 * social GENERAL PROCEDURES
 *
 * If first letter of the command is '!' this will mean that the following
 * command will be executed immediately.
 *
 * "G", n      : Sets next line to n
 * "g", n      : Sets next line relative to n, fx. line+=n
 * "m<dir>", n : move to <dir>, <dir> is 0, 1, 2, 3, 4 or 5
 * "w", n      : Wake up and set standing (if possible)
 * "c<txt>", n : Look for a person named <txt> in the room
 * "o<txt>", n : Look for an object named <txt> in the room
 * "r<int>", n : Test if the npc in room number <int>?
 * "s", n      : Go to sleep, return false if can't go sleep
 * "e<txt>", n : echo <txt> to the room, can use $o/$p/$N depending on
 * contents of the **thing
 * "E<txt>", n : Send <txt> to person pointed to by thing
 * "B<txt>", n : Send <txt> to room, except to thing
 * "?<num>", n : <num> in [1..99]. A random chance of <num>% success rate.
 * Will as usual advance one line upon sucess, and change
 * relative n lines upon failure.
 * "O<txt>", n : Open <txt> if in sight.
 * "C<txt>", n : Close <txt> if in sight.
 * "L<txt>", n : Lock <txt> if in sight.
 * "U<txt>", n : Unlock <txt> if in sight.
 */

/*
 * Execute a social command.
 */
void exec_social(P_char npc, char *cmd, int next_line, int *cur_line, void **thing)
{
	bool ok;

	if (IS_FIGHTING(npc))
		return;

	ok = TRUE;

	switch (*cmd)
	{
	case 'G':
		*cur_line = next_line;
		return;

	case 'g':
		*cur_line += next_line;
		return;

	case 'e':
		act(cmd + 1, FALSE, npc, (struct obj_data *)*thing, *thing, TO_ROOM);
		break;

	case 'E':
		act(cmd + 1, FALSE, npc, 0, *thing, TO_VICT);
		break;

	case 'B':
		act(cmd + 1, FALSE, npc, 0, *thing, TO_NOTVICT);
		break;

	case 'm':
		do_move(npc, 0, exitnumb_to_cmd(*(cmd + 1) - '0'));
		break;

	case 'w':
		if (IS_AWAKE(npc))
			ok = FALSE;
		else
			SET_POS(npc, POS_STANDING + STAT_NORMAL);
		break;

	case 's':
		if (!IS_AWAKE(npc))
			ok = FALSE;
		else
			SET_POS(npc, GET_POS(npc) + STAT_SLEEPING);
		break;

	case 'c': /*
		           * Find char in room
		           */
		*thing = get_char_room_vis(npc, cmd + 1);
		ok = (*thing != 0);
		break;

	case 'o': /*
		           * Find object in room
		           */
		*thing = get_obj_in_list_vis(npc, cmd + 1, world[npc->in_room].contents);
		ok = (*thing != 0);
		break;

	case 'r': /*
		           * Test if in a certain room
		           */
		ok = (npc->in_room == atoi(cmd + 1));
		break;

	case 'O': /*
		           * Open something
		           */
		do_open(npc, cmd + 1, 0);
		break;

	case 'C': /*
		           * Close something
		           */
		do_close(npc, cmd + 1, 0);
		break;

	case 'L': /*
		           * Lock something
		           */
		do_lock(npc, cmd + 1, 0);
		break;

	case 'U': /*
		           * UnLock something
		           */
		do_unlock(npc, cmd + 1, 0);
		break;

	case '?': /*
		           * Test a random number
		           */
		if (atoi(cmd + 1) <= number(1, 100))
			ok = FALSE;
		break;

	default:
		break;
	} /*
	   * End Switch
	   */

	if (ok)
		(*cur_line)++;
	else
		(*cur_line) += next_line;
}

int thief(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	P_char cons, next;

	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (cmd)
		return FALSE;

	if (!MIN_POS(ch, POS_STANDING + STAT_NORMAL) || IS_FIGHTING(ch))
		return FALSE;

	for (cons = world[ch->in_room].people; cons; cons = next)
	{
		next = cons->next_in_room;

		if (IS_PC(cons) && !IS_TRUSTED(cons) && !IS_FIGHTING(cons) && number(0, 1) &&
		    CAN_SEE(ch, cons))
		{
			npc_steal(ch, cons);
			return TRUE;
		}
	}

	return FALSE;
}

struct ticket_info_data
{
	int in_room;
	int item_id;
	int ship_id;
} ticket_info[] = {

	{
     5313,  5341,   11100 /*
   * WD, to Caer Corwell, Realms Master
   */						   },
	{
     5399,  5341,   11300 /*
   * WD, to Caer Corwell, Silver Lady
   */						   },
	{
     26200, 26240, 11100 /*
 * Port of Caer Corwell, to WD, Realms Master
 */							},
	{
     26200, 26240, 11300 /*
 * Port of Caer Corwell, to WD, Silver Lady
 */							},
	{    0,     0,							  0}
};

int ticket_taker(P_char ch, P_char pl, int cmd, char *arg)
{
	P_obj obj;
	char name[MAX_INPUT_LENGTH];
	bool no_ticket, found;
	int i;
	P_obj obj_entered;

	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return FALSE;

	if ((cmd != CMD_ENTER) || !ch || !pl)
		return FALSE;
	if (!IS_AWAKE(ch))
		return FALSE;
	if (!CAN_SEE(ch, pl))
		return FALSE;

	/*
	 * check for ticket on char.  If ticket not on char, then check for ticket
	 * given to mob.
	 */
	one_argument(arg, name);
	obj_entered = get_obj_in_list_vis(ch, name, world[ch->in_room].contents);
	if (!obj_entered)
		return FALSE;
	found = 0;
	no_ticket = 1;
	for (i = 0; ticket_info[(int)i].in_room != 0; i++)
	{
		if ((ch->in_room == real_room(ticket_info[(int)i].in_room)) &&
		    (obj_entered->R_num == real_object(ticket_info[(int)i].ship_id)))
		{
			no_ticket = 0;

			for (obj = pl->carrying; obj; obj = obj->next_content)
			{
				if (obj_index[obj->R_num].virtual_number ==
				    ticket_info[(int)i].item_id)
				{
					found = 1;
					act("$N tears up the ticket in your hand.", FALSE, pl, 0,
					    ch, TO_CHAR);
					act("$N tears up the ticket in $n's hand.", FALSE, pl, 0,
					    ch, TO_ROOM);
					obj_from_char(obj);
					extract_obj(obj, TRUE); // Not an arti, but 'in game.'
					obj = NULL;
					break;
				}
			}
			if (!found)
			{
				for (obj = ch->carrying; obj; obj = obj->next_content)
				{
					if (obj_index[obj->R_num].virtual_number ==
					    ticket_info[(int)i].item_id)
					{
						found = 1;
						act("$n tears up the ticket.", FALSE, ch, 0, 0,
						    TO_ROOM);
						obj_from_char(obj);
						extract_obj(obj,
							    TRUE); // Not an arti, but 'in game.'
						break;
					}
				}
			}
			if (found)
			{
				act("Then $E says to you, 'You may proceed.'", FALSE, pl, 0, ch,
				    TO_CHAR);
				act("Then $E says to $n, 'You may proceed.'", FALSE, pl, 0, ch,
				    TO_ROOM);
				return FALSE;
			}
		}
	}
	if (!no_ticket)
	{
		act("$N says, 'you must have a ticket to proceed.'", FALSE, pl, 0, ch, TO_CHAR);
		return TRUE;
	}
	if (!found) /*
	             * this ship is not restricted from being
	             * boarded
	             */
		return FALSE;
	return TRUE;
}

/*
 *    Fun procs - SAM 6-94
 */

/*
 * If the automaton is alone in its room and the trapdoor is blocked, unblock
 * * the door, so that more people can come into their deaths...>8^)
 * * -- DTS 2/22/95
 */

int brass_dragon(P_char ch, P_char pl, int cmd, char * /*arg*/)
{
	char Gbuf2[MAX_STRING_LENGTH], Gbuf4[MAX_STRING_LENGTH];

	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return FALSE;

	if ((cmd > CMD_DOWN) || (cmd < CMD_NORTH))
		return FALSE;

	strcpy(Gbuf4, "The brass dragon humiliates you, and blocks your way.\r\n");
	strcpy(Gbuf2, "The brass dragon humiliates $n, and blocks $s way.");

	if ((ch->in_room == real_room(5065)) && (cmd == CMD_WEST))
	{
		act(Gbuf2, FALSE, pl, 0, 0, TO_ROOM);
		send_to_char(Gbuf4, pl);
		return TRUE;
	}
	return FALSE;
}

int janitor(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	P_obj i;
	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (cmd || !IS_AWAKE(ch))
		return (FALSE);

	for (i = world[ch->in_room].contents; i; i = i->next_content)
	{
		if (CAN_GET_OBJ(ch, i, rider) && (CAN_CARRY_W(ch) <= GET_OBJ_WEIGHT(i)) &&
		    ((i->type == ITEM_DRINKCON) || (i->type == ITEM_TRASH) ||
		     (i->type == ITEM_OTHER) || (i->type == ITEM_FOOD) || (i->cost < 20)))
		{
			act("$n picks up some trash.", FALSE, ch, 0, 0, TO_ROOM);

			obj_from_room(i);
			obj_to_char(i, ch);
			return (TRUE);
		}
	}
	return (FALSE);
}

/*
 * A special for the Knife Shop Proprieter (mob-based)
 */

int clyde(P_char ch, P_char pl, int cmd, char * /*arg*/)
{
	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!pl)
	{
		if (!MIN_POS(ch, POS_STANDING + STAT_RESTING))
			return (FALSE);
		switch (dice(2, 5))
		{
		case 1:
			act("An evil grin crosses $n's face.", FALSE, ch, 0, 0, TO_ROOM);
			break;
		case 2:
			act("$n whistles a tune.", FALSE, ch, 0, 0, TO_ROOM);
			break;
		default:
			break;
		};
	}
	else if ((ch->in_room == real_room(12595)) && (cmd == CMD_SOUTH) &&
		 !GET_CLASS(pl, CLASS_ROGUE) && (MIN_POS(ch, POS_STANDING + STAT_NORMAL)))
	{
		act("With a gentle, but firm hand, Clyde guides you away from the curtain.", FALSE,
		    pl, 0, 0, TO_CHAR);
		act("Clyde skillfully redirects $n from going behind the curtain.", FALSE, pl, 0, 0,
		    TO_ROOM);
		return (TRUE);
	}
	else if ((cmd == CMD_BUY) || (cmd == CMD_SELL) || (cmd == CMD_LIST) || (cmd == CMD_VALUE) ||
		 (cmd == CMD_PERUSE))
	{
		mobsay(ch, "I'm not open for business right now.");
		/*
		 * in fact, he's never open...
		 */
	}
	return (FALSE);
}

/*
 * Another mob-based special...
 */

int waiter(P_char ch, P_char pl, int cmd, char *arg)
{
	int check1, check2, check3, check4, check5;
	P_obj i;

	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return FALSE;

	if ((cmd == CMD_EAST) && (ch->in_room == real_room(12576)))
	{
		check1 = real_object(12530);
		check2 = real_object(12531);
		check3 = real_object(12532);
		check4 = -1;
		check5 = -1; /*
		              * For when I add other menu items
		              */
		for (i = pl->carrying; i; i = i->next_content)
		{
			if ((i->R_num == check1) || (i->R_num == check2) || (i->R_num == check3) ||
			    (i->R_num == check4) || (i->R_num == check5))
			{
				act("The waiter prevents you from leaving.", FALSE, pl, 0, 0,
				    TO_CHAR);
				act("The waiter prevents $n from leaving.", FALSE, pl, 0, 0,
				    TO_ROOM);
				mobsay(ch, "You must finish your meal here!");
				return (TRUE);
			}
		}
	}
	return (shop_keeper(ch, pl, cmd, arg));
}

int barmaid(P_char ch, P_char pl, int cmd, char *arg)
{
	/*
	 * Do special stuff, then call regular shop routine
	 */

	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return FALSE;

	return (shop_keeper(ch, pl, cmd, arg));
}

int cookie(P_char ch, P_char pl, int cmd, char * /*arg*/)
{
	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!pl)
	{
		switch (number(1, 15))
		{
		case 2:
			do_action(ch, 0, CMD_BURP);
			switch (number(1, 3))
			{
			case 1:
				mobsay(ch, "Mmm! Mammoth!");
				break;
			case 2:
				mobsay(ch, "Mmm! Yak liver!");
				break;
			case 3:
				mobsay(ch, "Hmm. Can't quite place that one.");
				break;
			default:
				break;
			}
		default:
			break;
		}
	}
	return (FALSE);
}

int neophyte(P_char ch, P_char pl, int cmd, char * /*arg*/)
{
	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (pl)
	{
		if (ch->in_room == real_room(12587))
		{
			if ((cmd == CMD_WEST) && !GET_CLASS(pl, CLASS_CLERIC))
			{
				mobsay(ch, "Only the chosen get to see the master!");
				return (TRUE);
			}
		}
	}
	else
	{
		switch (number(1, 15))
		{
		case 1:
			do_action(ch, 0, CMD_STARE);
			break;
		default:
			break;
		}
	}
	return (FALSE);
}

int guru_anapest(P_char ch, P_char pl, int cmd, char *arg)
{ /*
   * If in floating position, rotate, slow flips,
   *
   * etc...
   */
	P_char who;
	char Gbuf1[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH];

	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (pl)
	{
		if (!IS_AWAKE(ch))
			return (FALSE);
		argument_interpreter(arg, Gbuf1, Gbuf2);
		who = get_char_room(Gbuf1, ch->in_room);
		switch (cmd)
		{
		case CMD_WORSHIP:
			if (who == ch)
			{
				do_action(pl, arg, CMD_WORSHIP);
				mobsay(ch,
				       "Don't worship me, for none are worthy of such respect.");
				return (TRUE);
			}
			break;
		case CMD_NUDGE:
			if (who == ch)
			{
				do_action(pl, arg, CMD_NUDGE);
				strcpy(Gbuf1, GET_NAME(pl));
				do_action(ch, Gbuf1, CMD_WINK);
				return (TRUE);
			};
			break;
		case CMD_WINK:
			if (who == ch)
			{
				do_action(pl, arg, CMD_WINK);
				strcpy(Gbuf1, GET_NAME(pl));
				do_action(ch, Gbuf1, CMD_NUDGE);
				return (TRUE);
			};
			break;
		}
	}
	else if (IS_FIGHTING(ch))
	{
		if (GET_HIT(ch) < (GET_MAX_HIT(ch) / 4))
		{
			spell_teleport(GET_LEVEL(ch), ch, 0, 0, ch, 0);
		}
	}
	else
	{
		switch (number(0, 25))
		{
		case 1:
			mobsay(ch, "Existence is suffering.");
			break;
		case 2:
			mobsay(ch, "Suffering is the end result of greed.");
			break;
		case 3:
			mobsay(ch, "Information complicates our lives.");
			break;
		case 4:
			mobsay(ch, "The pinnacle of existence is nothingness.");
			break;
		default:
			break;
		}
	}
	return FALSE;
}

int confess_figure(P_char ch, P_char pl, int cmd, char * /*arg*/)
{
	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (pl)
	{
		if (cmd == CMD_TELL)
		{
			return (TRUE);
		}
	}
	else if (IS_FIGHTING(ch))
	{
		switch (number(1, 11))
		{
		case 1:
			mobsay(ch, "I hope your conscience bothers you!");
			break;
		default:
			break;
		}
		return (FALSE);
	}
	else
	{
		if (ch->only.npc->spec[0])
		{
		}
		else
			switch (number(1, 10))
			{
			case 1:
				do_action(ch, 0, CMD_COUGH);
				break;
			default:
				break;
			}
	}
	return (FALSE);
}

int taxman(P_char ch, P_char pl, int cmd, char *arg)
{
	P_char who;
	char Gbuf1[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH];

	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!IS_AWAKE(ch))
		return (FALSE);
	if (pl)
	{
		argument_interpreter(arg, Gbuf1, Gbuf2);
		who = get_char_room(Gbuf1, ch->in_room);
		switch (cmd)
		{
		case CMD_BACKSTAB:
			if (who == ch)
			{
				mobsay(ch, "Oh no you don't!");
				strcpy(Gbuf1, GET_NAME(pl));
				do_action(ch, Gbuf1, CMD_SPANK);
			}
			break;
		default:
			break;
		}
	}
	else
	{
		switch (number(1, 15))
		{
		case 1:
			do_action(ch, 0, CMD_CACKLE);
			break;
		default:
			break;
		}
	}
	return (FALSE);
}

int albert(P_char ch, P_char pl, int cmd, char * /*arg*/)
{
	char Gbuf4[MAX_STRING_LENGTH];

	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!pl)
	{
		if (IS_SET(ch->specials.act, ACT_SENTINEL) && (GET_HIT(ch) == GET_MAX_HIT(ch)))
		{
			if (GET_STAT(ch) == STAT_SLEEPING)
				do_wake(ch, 0, 0);
			else if (!MIN_POS(ch, POS_STANDING + STAT_RESTING))
				do_stand(ch, 0, 0);
			REMOVE_BIT(ch->specials.act, ACT_SENTINEL);
		}
		else if ((world[ch->in_room].number == 12613) ||
			 (world[ch->in_room].number == 12614))
		{
			if (IS_FIGHTING(ch))
			{
				do_action(ch, 0, CMD_SCREAM);
				do_flee(ch, 0, 0);
			}
		}
		else if (GET_HIT(ch) != GET_MAX_HIT(ch))
		{
			strcpy(Gbuf4, "rub ring");
			command_interpreter(ch, Gbuf4);
			if (world[ch->in_room].number != 12613)
			{
				mobsay(ch, "What happened to my ring?");
			}
			else
			{
				SET_BIT(ch->specials.act, ACT_SENTINEL);
			}
		}
	}
	return (FALSE);
}

int mage_anapest(P_char ch, P_char pl, int cmd, char * /*arg*/)
{
	P_char i, temp;
	char Gbuf4[MAX_STRING_LENGTH];

	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!IS_AWAKE(ch))
		return (FALSE);
	if (pl)
	{
		switch (cmd)
		{
		case CMD_BACKSTAB:
			do_action(ch, 0, CMD_GROWL);
			spell_teleport(GET_LEVEL(ch), ch, 0, 0, pl, 0);
			act("She apparently doesn't like that.\r\n", FALSE, pl, 0, 0, TO_CHAR);
			return (TRUE);
			break;
		default:
			break;
		}
	}
	else
	{
		if (ch->in_room == real_room(12581))
		{
			for (i = world[ch->in_room].people; i; i = temp)
			{
				temp = i->next_in_room;
				if ((i != ch) && !GET_CLASS(i, CLASS_SORCERER))
				{
					mobsay(ch, "Be gone!");
					/*
					 * Get rid of them
					 */
					spell_teleport(GET_LEVEL(ch), ch, 0, 0, i, NULL);
				}
				else if ((i != ch) && (GET_ALIGNMENT(i) > 350))
				{
					snprintf(Gbuf4, MAX_STRING_LENGTH,
						 "%s I don't think I like you!", i->player.name);
					do_tell(ch, Gbuf4, 0);
				}
			}
		}
	}

	return FALSE;
}

int farmer(P_char ch, P_char pl, int cmd, char *arg)
{
	P_char who;
	const char *str = NULL;
	char Gbuf1[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH];

	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (pl)
	{
		if (!IS_AWAKE(ch))
			return (FALSE);
		argument_interpreter(arg, Gbuf1, Gbuf2);
		who = get_char_room(Gbuf1, ch->in_room);
		switch (cmd)
		{
		case CMD_TELL:
			if (who == ch)
			{
				do_tell(pl, arg, 0);
				do_action(ch, 0, CMD_NOD);
				switch (number(1, 20))
				{
				case 1:
					str = " I ain't got no problem with that - it's your opinion.";
					break;
				case 2:
					str = " Hehehe.";
					break;
				case 3:
					str = " You're just SO much smarter than simple little farmers like us...NOT!";
					break;
				default:
					break;
				}
				if (str)
				{
					Gbuf2[0] = 0;
					strcat(Gbuf2, GET_NAME(pl));
					strcat(Gbuf2, str);
					do_tell(ch, Gbuf2, 0);
				}
			}
		}
	}
	else
	{
		switch (number(1, 13))
		{
		case 1:
			do_action(ch, 0, CMD_YODEL);
			break;
		case 2:
			if (IS_ROOM(ch->in_room, ROOM_INDOORS))
				mobsay(ch, "Ya know, I really like being outside.");
			else
			{
				act("$n examines the ground for its agricultural potential.", TRUE,
				    ch, 0, 0, TO_ROOM);
				if (world[ch->in_room].sector_type <= SECT_CITY)
					mobsay(ch, "Pbbbbbt!");
			}
			break;
		default:
			break;
		}
	}
	return (FALSE);
}

int animated_skeleton(P_char ch, P_char pl, int cmd, char * /*arg*/)
{
	P_char undead, ch2;
	struct follow_type *followers;
	int num;

	if (cmd == CMD_SET_PERIODIC)
		return FALSE;

	if (ch == pl)
		return FALSE;

	if (!number(0, 3))
		return FALSE;

	if (ch->following)
		ch2 = ch->following;
	else
		ch2 = ch;

	if (cmd == CMD_DEATH)
	{
		num = 0;
		for (followers = ch2->followers; followers; followers = followers->next)
			if (followers->follower && IS_NPC(followers->follower) &&
			    (GET_VNUM(followers->follower) == 1201))
				num++;
		if (num > (GET_LEVEL(ch2) - 10) || number(0, 2))
		{
			act("$n shatters into a pile of useless bones!", TRUE, ch, 0, 0, TO_ROOM);
			return TRUE;
		}
		undead = read_mobile(GET_RNUM(ch), REAL);
		if (undead)
		{
			SET_BIT(undead->specials.act,
				ACT_SENTINEL | ACT_ISNPC | ACT_SPEC | ACT_SPEC_DIE);
			if (!IS_SET(undead->specials.act, ACT_MEMORY))
			{
				clearMemory(undead);
			}
			GET_RACE(undead) = RACE_UNDEAD;
			GET_SEX(undead) = SEX_NEUTRAL;
			//      GET_CLASS(undead) = GET_CLASS(ch);
			undead->player.m_class = CLASS_WARRIOR; // needs to be fixed..
			GET_ALIGNMENT(undead) = -500;
			//      GET_LEVEL(undead) = BOUNDED(1, (GET_LEVEL(ch) - 1), 10);
			undead->player.level = BOUNDED(1, (GET_LEVEL(ch) - 1), 10);
			undead->only.npc->str_mask = (STRUNG_KEYS | STRUNG_DESC1 | STRUNG_DESC2);
			undead->player.name = str_dup(ch->player.name);
			undead->player.short_descr = str_dup(ch->player.short_descr);
			undead->player.long_descr = str_dup(ch->player.long_descr);
			undead->points.damnodice = ch->points.damnodice - 1;
			undead->points.base_hitroll = undead->points.hitroll =
				GET_LEVEL(undead) / 4;
			undead->points.base_damroll = undead->points.damroll =
				GET_LEVEL(undead) / 4;
			undead->points.mana = undead->points.base_mana = 0;
			GET_PLATINUM(undead) = 0;
			GET_GOLD(undead) = 0;
			GET_SILVER(undead) = 0;
			GET_COPPER(undead) = 0;
			while (undead->affected)
				affect_remove(undead, undead->affected);
			GET_EXP(undead) = 0;
			mob_index[GET_RNUM(undead)].func.mob = animated_skeleton;
			char_to_room(undead, ch->in_room, 0);
			balance_affects(undead);
		}
		undead = read_mobile(GET_RNUM(ch), REAL);
		if (undead)
		{
			SET_BIT(undead->specials.act,
				ACT_SENTINEL | ACT_ISNPC | ACT_SPEC | ACT_SPEC_DIE);
			if (!IS_SET(undead->specials.act, ACT_MEMORY))
				clearMemory(undead);
			GET_RACE(undead) = RACE_UNDEAD;
			GET_SEX(undead) = SEX_NEUTRAL;
			//    GET_CLASS(undead) = GET_CLASS(ch);
			undead->player.m_class = CLASS_WARRIOR; // needs to be fixed
			GET_ALIGNMENT(undead) = -500;
			//      GET_LEVEL(undead) = BOUNDED(1, (GET_LEVEL(ch) - 1), 10);
			undead->player.level = BOUNDED(1, (GET_LEVEL(ch) - 1), 10);
			undead->only.npc->str_mask = (STRUNG_KEYS | STRUNG_DESC1 | STRUNG_DESC2);
			undead->player.name = str_dup(ch->player.name);
			undead->player.short_descr = str_dup(ch->player.short_descr);
			undead->player.long_descr = str_dup(ch->player.long_descr);
			undead->points.damnodice = ch->points.damnodice - 1;
			undead->points.base_hitroll = undead->points.hitroll =
				GET_LEVEL(undead) / 4;
			undead->points.base_damroll = undead->points.damroll =
				GET_LEVEL(undead) / 4;
			undead->points.mana = undead->points.base_mana = 0;
			GET_PLATINUM(undead) = 0;
			GET_GOLD(undead) = 0;
			GET_SILVER(undead) = 0;
			GET_COPPER(undead) = 0;
			while (undead->affected)
				affect_remove(undead, undead->affected);
			GET_EXP(undead) = 0;
			mob_index[GET_RNUM(undead)].func.mob = animated_skeleton;
			char_to_room(undead, ch->in_room, 0);
			balance_affects(undead);
		}
		act("The bones of the skeleton split apart and reform into two new skeletons.",
		    TRUE, ch, 0, 0, TO_ROOM);
		return TRUE;
	}
	return FALSE;
}

int bridge_troll(P_char ch, P_char pl, int cmd, char *arg)
{
	int gold;
	P_char k;
	char Gbuf1[MAX_STRING_LENGTH];

	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (pl && (pl != ch))
	{
		if (cmd == CMD_GIVE)
		{
			gold = GET_MONEY(ch);
			do_give(pl, arg, 0);
			if ((gold = (GET_MONEY(ch) - gold)))
			{
				if (gold < 500)
					mobsay(ch,
					       "You STILL need to pay me 500 copper coins, pal.");
				else
				{
					strcpy(Gbuf1, GET_NAME(pl));
					do_action(ch, Gbuf1, CMD_SMILE); /*
					                                  * smile
					                                  */
					k = world[ch->in_room].people;
					while ((k != ch) && (k != pl) && k)
						k = k->next_in_room;
					if (!k)
					{ /*
					   * Should not happen!
					   */
						logit(LOG_DEBUG, "Troll error 1!");
						return (TRUE);
					}
					act("$N picks you and tosses you to the other side of the bridge!",
					    FALSE, pl, 0, ch, TO_CHAR);
					act("$N throws $n to the other side of the bridge!", TRUE,
					    pl, 0, ch, TO_NOTVICT);
					char_from_room(pl);
					if (k == ch)
						if (ch->in_room == real_room(1863)) /*
						                                     * calimport troll
						                                     */
							char_to_room(pl, real_room(1862), 0);
						else
							char_to_room(pl, real_room(14236), 0);
					else
					{
						if (ch->in_room == real_room(1863)) /*
						                                     * calimport troll
						                                     */
							char_to_room(pl, real_room(1864), 0);
						else
							char_to_room(pl, real_room(14238), 0);
					}
					act("$n lands in a pile here from the direction of the bridge!",
					    TRUE, pl, 0, 0, TO_ROOM);
					SET_POS(pl, POS_SITTING + GET_STAT(pl));
				}
			}
			return (TRUE);
		}
	}
	else
	{
		ch->only.npc->spec[0]++;
		if (ch->only.npc->spec[0] == 6)
		{
			ch->only.npc->spec[0] = 0;
			GET_HIT(ch) = MIN(GET_HIT(ch) + 6, ch->points.base_hit);
		}
	}
	return (FALSE);
}

int blob(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	P_obj i, temp, next_obj;

	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
	{
		ch->only.npc->spec[0] = 0;
		return TRUE;
	}
	if (cmd || !IS_AWAKE(ch) || IS_FIGHTING(ch))
		return (FALSE);

	i = world[ch->in_room].contents;
	/*
	 * Check for takeable item to absorb.
	 */
	if ((i != NULL) && (IS_SET(i->wear_flags, ITEM_TAKE)) && !IS_ARTIFACT(i))
	{
		act("$n absorbs $p with a squish.", FALSE, ch, i, 0, TO_ROOM);
		obj_from_room(i);
		obj_to_char(i, ch);
		return (TRUE); /*
		                * Absorption happened. Happy blob! =)
		                */
	}
	else if (ch->only.npc->spec[0] != 0)
	{
		ch->only.npc->spec[0]--;
		return (FALSE);
	}
	else
	{
		ch->only.npc->spec[0] = 5; /*
		                            * >5 turns before digestion tried again.
		                            */
		/*
		 * No items or item not takeable. See if there's anything to digest.
		 */
		i = ch->carrying;
		if (i != NULL)
		{
			if ((GET_ITEM_TYPE(i) == ITEM_CONTAINER) ||
			    (GET_ITEM_TYPE(i) == ITEM_STORAGE) ||
			    (GET_ITEM_TYPE(i) == ITEM_QUIVER) || (GET_ITEM_TYPE(i) == ITEM_CORPSE))
				for (temp = i->contains; temp; temp = next_obj)
				{
					next_obj = temp->next_content;
					obj_from_obj(temp);
					obj_to_char(temp, ch);
				} /*
				   * if a container is digested, leave contents
				   * for later digestion
				   */
			act("$n digests $p.", FALSE, ch, i, 0, TO_ROOM);
			obj_from_char(i);
			extract_obj(i, TRUE); // Shouldn't be an arti, but 'in game.'
			return (TRUE);
		}
		return (FALSE);
	}
}

int cc_warehouse_man(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
		return FALSE;

	switch (number(1, 100))
	{
	case 1:
	{
		act("$n flexes $s muscles as $e moves around some boxes.", TRUE, ch, 0, 0, TO_ROOM);
		return TRUE;
	}
	case 2:
	{
		act("$n mumbles something about $s foreman.", TRUE, ch, 0, 0, TO_ROOM);
		return TRUE;
	}
	case 3:
	{
		mobsay(ch, "I swear I am going to quit this damn job!");
		return TRUE;
	}
	default:
		return FALSE;
	}
}

int cc_warehouse_foreman(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
		return FALSE;

	switch (number(1, 100))
	{
	case 1:
	{
		act("$n yells, 'Get back to work!'", TRUE, ch, 0, 0, TO_ROOM);
		return TRUE;
	}
	case 2:
	{
		act("$n yells, 'One more remark like that and you're fired!'", TRUE, ch, 0, 0,
		    TO_ROOM);
		return TRUE;
	}
	case 3:
	{
		act("$n yells, 'Hurry up!  These crates gotta ship today!'", TRUE, ch, 0, 0,
		    TO_ROOM);
		return TRUE;
	}
	default:
		return FALSE;
	}
}

int rentacleric(P_char ch, P_char vict, int cmd, char *argument)
{
	int i, cost, spl;
	P_obj obj = NULL, next_obj;
	char buf[MAX_STRING_LENGTH];
	struct price_info
	{
		short int number;
		char name[50];
		char tobuy[50];
		int price;
	} prices[] = {
		/* Spell Num (defined)      Name shown                               Name           Price  */
		{ SPELL_CURE_CRITIC, "&+WCure critical wounds&n     ", "cure critical wounds",
		  250 },
		{ SPELL_FULL_HEAL, "&+WFull heal&n                ", "full heal", 500 },
		{ SPELL_ARMOR, "&+wBenevolent armor&n         ", "benevolent armor", 100 },
		{ SPELL_BLESS, "&+WBlessing &+Lof the &+RGods&n     ", "blessing of the gods",
		  100 },
		{ SPELL_REMOVE_POISON, "&+GAntidote&n                 ", "antidote", 600 },
		{ SPELL_CURE_DISEASE, "&+yDisease &+wremoval&n          ", "disease removal", 650 },
		{ SPELL_REMOVE_CURSE, "&+rCurse &+wremoval&n            ", "curse removal", 700 },
		{ SPELL_CURE_BLIND, "&+WCure of &+Lblindness&n        ", "cure of blindness", 500 },
		{ SPELL_ACCEL_HEALING, "&+YAccelerated &+Whealing&n      ", "accelerated healing",
		  2500 },
		{ SPELL_RESURRECT, "&+WResurrection&n             ", "resurrection", 5000 },
		{ -1, "\r\n", "", -1 },
	};

	if (cmd == CMD_SET_PERIODIC)
	{
		// So they can cast spells.
		if (GET_LEVEL(ch) < 56)
			ch->player.level = 56;
		return TRUE;
	}

	if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch))
		return FALSE;

	if (cmd == CMD_BUY)
	{
		argument = one_argument(argument, buf);
		if (*buf)
		{
			if (economic_gameplay_authority::active())
			{
				send_to_char(
					"Cleric services are unavailable while active accounting is enabled.\r\n",
					vict);
				return TRUE;
			}
			for (i = 0; prices[i].number > SPELL_RESERVED_DBC; i++)
				if (is_abbrev(buf, prices[i].tobuy))
				{
					/* resur is special case. Just find any corpse and raise it :) */
					if (prices[i].number == SPELL_RESURRECT)
					{
						// In case 'order follower buy resurrect' etc.
						if (IS_NPC(vict))
						{
							mobsay(ch,
							       "I only raise PC corpses, maybe you should talk to Melmba");
							return TRUE;
						}
						for (obj = world[ch->in_room].contents; obj;
						     obj = next_obj)
						{
							next_obj = obj->next_content;
							if ((obj->type == ITEM_CORPSE) &&
							    IS_SET(obj->value[1], PC_CORPSE) &&
							    isname(GET_NAME(vict),
								   obj->action_description))
								break;
						}
						if (!obj)
						{
							mobsay(ch,
							       "Did you perhaps forget to bring your friend?");
							return TRUE;
						}
					}
					cost = prices[i].price * (GET_LEVEL(vict) < 36 ?
									  GET_LEVEL(vict) / 4 :
									  GET_LEVEL(vict));
					spl = prices[i].number;
					if (transact(vict, NULL, ch, cost))
					{
						/* make em broke, as clerics should be */
						GET_PLATINUM(ch) = GET_GOLD(ch) = GET_SILVER(ch) =
							GET_COPPER(ch) = 0;
						StopCasting(ch);
						if (!(spl == SPELL_RESURRECT))
						{
							MobCastSpell(ch, vict, NULL, spl,
								     GET_LEVEL(vict) < 20 ?
									     60 :
									     GET_LEVEL(vict));
							return TRUE;
						}
						else
						{
							MobCastSpell(ch, vict, obj, spl, 60);
							return TRUE;
						}
					}
					else
						return TRUE;
				}
			mobsay(ch, "Sorry, I don't know of that spell.");
			return TRUE;
		}
		else
		{
			act("$n tells you, 'Here is a listing of the prices for my services.'",
			    FALSE, ch, 0, vict, TO_VICT);
			for (i = 0; prices[i].number > SPELL_RESERVED_DBC; i++)
			{
				cost = prices[i].price * (GET_LEVEL(vict) < 36 ?
								  GET_LEVEL(vict) / 4 :
								  GET_LEVEL(vict));
				snprintf(buf, MAX_STRING_LENGTH, "%s%s\r\n", prices[i].name,
					 coin_stringv(cost));
				send_to_char(buf, vict);
			}
			return TRUE;
		}
	}
	else if (cmd == CMD_LIST)
	{
		act("$n tells you, 'Here is a listing of the prices for my services.'", FALSE, ch,
		    0, vict, TO_VICT);
		for (i = 0; prices[i].number > SPELL_RESERVED_DBC; i++)
		{
			cost = prices[i].price *
			       (GET_LEVEL(vict) < 36 ? GET_LEVEL(vict) / 4 : GET_LEVEL(vict));
			snprintf(buf, MAX_STRING_LENGTH, "%s%s\r\n", prices[i].name,
				 coin_stringv(cost));
			send_to_char(buf, vict);
		}
		return TRUE;
	}
	return FALSE;
}

int necro_specpet_blood(P_char /*ch*/, P_char /*pl*/, int /*cmd*/, char * /*arg*/)
{
	return 0;
}

int conj_specpet_salamander(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	P_char vict;

	if (cmd == CMD_SET_PERIODIC)
	{
		return TRUE;
	}

	if (IS_FIGHTING(ch) && cmd == CMD_MOB_MUNDANE && (number(1, 10) == 1) &&
	    world[ch->in_room].sector_type != SECT_WATER_PLANE)
	{
		vict = GET_OPPONENT(ch);
		if (vict->in_room != ch->in_room)
		{
			return FALSE;
		}
		act("$n &+ropens its &+Rjaws &+rspewing a &+Rf&+rl&+Ra&+rm&+Ri&+rn&+Rg mass at $N!",
		    FALSE, ch, 0, vict, TO_NOTVICT);
		act("$n &+ropens its &+Rjaws &+rspewing a &+Rf&+rl&+Ra&+rm&+Ri&+rn&+Rg mass at you!",
		    FALSE, ch, 0, vict, TO_VICT);
		spell_immolate(50, ch, NULL, 0, vict, 0);
		if (number(1, 5) == 5)
		{
			act("&+RSucking &+rin another &+Rbreath &+r$n &+rcovers &+R$N &+rwith &+Wwhite&+r-&+Rhot &+rmagma!",
			    FALSE, ch, 0, vict, TO_NOTVICT);
			act("&+RSucking &+rin another &+Rbreath &+r$n &+rcovers &+Ryou &+rwith &+Wwhite&+r-&+Rhot &+rmagma!",
			    FALSE, ch, 0, vict, TO_VICT);
			spell_magma_burst(60, ch, NULL, 0, vict, 0);
		}
	}
	return FALSE;
}

// When Timoro dies, a bunch of dwarven invaders led by mob vnum (above), come down
//   from room vnum (above) to avenge the death.

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:74001
   *Name:Common woman
 */

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7102
   *Name:Little brat
 */

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7103
   *Name:Holyman
 */

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7104
   *Name:merchant
 */

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7105    7308
   *Name:wino    wino second
 */

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7106
   *Name:watcher
 */

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7107
   *Name:guard
 */

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7108
   *Name:squire
 */

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7110    7111     7112       7141
   *Name:vrock   hezrou   glabrezu   lurker
 */

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7113
   *Name:timid prisoner
 */

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7114
   *Name:shady prisoner
 */

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7115
   *Name:sinister prisoner
 */

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7116
   *Name:menacing prisoner
 */

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7117
   *Name:executioner
 */

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7118
   *Name:baron
 */

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7120
   *Name:sparrow
 */

/*
  Bloodstone Zone 71 Mob proc
   *Mob#:7121        7122
   *Name:squirrel    huge squirrel
 */

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7123
   *Name:crow
 */

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7124
   *Name:mountainman
 */

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7125
   *Name:salesman
 */

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7126
   *Name:nomad
 */

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7127
   *Name:insane woman
 */

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7128
   *Name:homeless man
 */

/*
  Bloodstone Zone 71 Mob proc
  *Mob#:7129
  *Name:baron's servant
*/

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7140
   *Name:wolf
 */

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7143
   *Name:gnoll
 */

/*
  Bloodstone Zone 71 Mob proc
  *Mob#:7144
  *Name:ettin
*/

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7147
   *Name:griffon
 */

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7152
   *Name:wereboar
 */

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7153
   *Name:manticore cub
 */

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7154
   *Name:manticore fierce
 */

/*
   Bloodstone Zone 71 Mob proc
   *Mob#:7160
   *Name:stirge
 */

int monk_remort(P_char ch, P_char pl, int cmd, char *arg)
{
	P_char tch;
	char name[MAX_STRING_LENGTH], msg[MAX_STRING_LENGTH];
	char Gbuf[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH];
	int epiccost, plat;

	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	argument_interpreter(arg, name, msg);

	if (!pl && (cmd == CMD_MOB_MUNDANE))
	{
		LOOP_THRU_PEOPLE(tch, ch)
		{
			if (!number(0, 3) && GET_CLASS(tch, CLASS_CLERIC) && !IS_MULTICLASS_PC(tch))
			{
				do_say(ch,
				       writable_arg(
					       "A cleric eh?  Have you heard the rumors of clerics becoming powerful monks?"),
				       -4);
				return FALSE;
			}
		}
	}

	if (pl && !IS_PC(pl))
		return FALSE;

	if (cmd != CMD_ASK)
		return FALSE;

	epiccost = (int)get_property("remort.monk.epic.cost", 1000.000);
	plat = (int)get_property("remort.monk.cost", 1000000.00);

	if (!strcmp(msg, "monk"))
	{
		do_say(ch,
		       writable_arg(
			       "Yes Monks are a powerful kind indeed.  If you seek to become one, I can teach you for a price."),
		       -4);
		snprintf(Gbuf, MAX_STRING_LENGTH,
			 "It will cost you %s, and you must posses %d epics.", coin_stringv(plat),
			 epiccost);
		do_say(ch, Gbuf, -4);
		do_say(ch, writable_arg("Ask me 'remort' to confirm."), -4);
		return TRUE;
	}
	if (!strcmp(msg, "remort"))
	{
		if (pl && economic_gameplay_authority::active())
		{
			send_to_char(
				"Monk remort is unavailable while economic accounting is active.\n",
				pl);
			return TRUE;
		}
		if (IS_TRUSTED(pl))
		{
			send_to_char("That would be very dumb.\n", pl);
			return TRUE;
		}

		if (!GET_CLASS(pl, CLASS_CLERIC))
		{
			send_to_char(
				"You do not posses the correct class to obtain my teachings.\n",
				pl);
			return TRUE;
		}

		if ((GET_RACE(pl) != RACE_HUMAN) && (GET_RACE(pl) != RACE_GNOME) &&
		    (GET_RACE(pl) != RACE_GITHZERAI))
		{
			send_to_char("I do not teach your kind!  Be gone!\n", pl);
			return TRUE;
		}

		if (pl->only.pc->epics < (int)get_property("remort.monk.epic.cost", 1000.000))
		{
			send_to_char("You are not epic enough!\n", pl);
			return TRUE;
		}

		if (GET_MONEY(pl) < plat)
		{
			send_to_char("You can't afford it!\n", pl);
			return TRUE;
		}

		// PASSED!

		SUB_MONEY(pl, plat, 0);
		snprintf(Gbuf, MAX_STRING_LENGTH, "%s takes your money.\n", ch->player.short_descr);
		send_to_char(Gbuf, pl);

		forget_spells(pl, -1);
		pl->player.spec = 0;
		pl->player.secondary_class = 0;
		pl->player.m_class = CLASS_MONK;
		do_start(pl, 1);

		snprintf(Gbuf2, MAX_STRING_LENGTH, "You begin listening to %s as he begins\n",
			 ch->player.short_descr);
		send_to_char(Gbuf2, pl);
		send_to_char("describing the ways of the &+LM&+won&+Lk&n to you.\n", pl);
		send_to_char("Before too long, you begin to forget your priesthood.\n", pl);
		CharWait(pl, WAIT_SEC * 30);
		return TRUE;
	}
	return 0;
}
