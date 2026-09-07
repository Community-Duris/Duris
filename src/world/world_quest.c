/****************************************************************************
 *
 *  File: world_quest.c                                      Part of Duris
 *  Usage: world_quest.c
 *  Copyright  1990, 1991 - see 'license.doc' for complete information.
 *  Copyright 1994 - 2008 - Duris Systems Ltd.
 *  Created by: Kvark 			Date: 2006-04-17
 * ***************************************************************************

todo:


Version 2
- Change so you can ask solo, small.



 */

#include <vector>
using namespace std;

#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "world/db.h"
#include "world/events.h"
#include "cmd/interp.h"
#include "core/utility.h"
#include "core/utils.h"
#include <ctype.h>
#include <fnmatch.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "world/epic.h"
#include "net/gmcp.h"
#include "combat/justice.h"
#include "world/map.h"
#include "item/objmisc.h"
#include "persistence/persistence_checkpoint.h"
#include "magic/spells.h"
#include "sql/sql.h"
#include "world/weather.h"
#include "world/world_quest.h"
#include "world/world_quest_policy.h"
#include "world/world_quest_policy_math.h"

/* * external variables */

extern char *target_locs[];
extern char *set_master_text[];
extern int mini_mode;
extern FILE *help_fl;
extern FILE *info_fl;
extern P_char character_list;
extern P_desc descriptor_list;
extern P_index mob_index;
extern P_index obj_index;
extern P_obj object_list;
extern P_room world;
extern struct class_names class_names_table[];
extern const char *color_liquid[];
extern const char *command[];
extern const char *connected_types[];
extern const char *craftsmanship_names[];
extern const char *dirs[];
extern const char *fullness[];
extern struct material_data materials[];
extern const char *month_name[];
extern const char *player_bits[];
extern const char *player_prompt[];
extern flagDef weapon_types[];
extern const char *weekdays[];
extern const int rev_dir[];
extern const long boot_time;
extern const struct stat_data stat_factor[];
extern const char *size_types[];
extern const char *item_size_types[];
extern long new_exp_table[]; // Arih: Fixed type mismatch bug - was int, should be long
extern int avail_descs;
extern int help_array[27][2];
extern int info_array[27][2];
extern int max_descs;
extern int max_users_playing;
extern int number_of_quests;
extern int number_of_shops;
extern int pulse;
extern int str_weightcarried, str_todam, str_tohit, dex_defense;
extern int top_of_helpt;
extern int top_of_infot;
extern int top_of_mobt;
extern int top_of_objt;
extern int top_of_zone_table;
extern int used_descs;
extern struct agi_app_type agi_app[];
extern struct command_info cmd_info[];
extern struct dex_app_type dex_app[];
extern struct info_index_element *info_index;
extern struct str_app_type str_app[];
extern struct time_info_data time_info;
extern struct zone_data *zone_table;
extern struct sector_data *sector_table;
extern char *specdata[][MAX_SPEC];
extern long sentbytes;
extern const struct race_names race_names_table[];
extern Skill skills[];
extern const mcname multiclass_names[];
extern void displayShutdownMsg(P_char);

void resetQuest(P_char ch)
{
	ch->only.pc->quest_active = 0;
	ch->only.pc->quest_mob_vnum = 0;
	ch->only.pc->quest_type = 0;
	ch->only.pc->quest_accomplished = 0;
	ch->only.pc->quest_started = 0;
	ch->only.pc->quest_zone_number = -1;
	ch->only.pc->quest_giver = 0;
	ch->only.pc->quest_level = 0;
	ch->only.pc->quest_receiver = 0;
	ch->only.pc->quest_shares_left = 0;
	ch->only.pc->quest_kill_how_many = 0;
	ch->only.pc->quest_kill_original = 0;
	ch->only.pc->quest_map_room = 0;
	ch->only.pc->quest_map_bought = 0;
}

void quest_epic_reward(P_char ch, int /*type*/)
{
	if (GET_LEVEL(ch) > 45 && ch->only.pc->quest_level > 45)
	{
		// snprintf(Gbuf1, MAX_STRING_LENGTH, "You gain some epic experience.");
		// act(Gbuf1, FALSE, quest_mob, 0, ch, TO_VICT);

		if (ch->only.pc->quest_level == 46)
			gain_epic(ch, EPIC_QUEST, 0, number(1, 3));

		if (ch->only.pc->quest_level == 47)
			gain_epic(ch, EPIC_QUEST, 0, number(2, 4));

		if (ch->only.pc->quest_level == 48)
			gain_epic(ch, EPIC_QUEST, 0, number(3, 5));

		if (ch->only.pc->quest_level == 49)
			gain_epic(ch, EPIC_QUEST, 0, number(4, 6));

		if (ch->only.pc->quest_level == 50)
			gain_epic(ch, EPIC_QUEST, 0, number(5, 7));

		if (ch->only.pc->quest_level == 51)
			gain_epic(ch, EPIC_QUEST, 0, number(6, 8));

		if (ch->only.pc->quest_level == 52)
			gain_epic(ch, EPIC_QUEST, 0, number(7, 9));

		if (ch->only.pc->quest_level == 53)
			gain_epic(ch, EPIC_QUEST, 0, number(8, 10));

		if (ch->only.pc->quest_level == 54)
			gain_epic(ch, EPIC_QUEST, 0, number(9, 10));

		if (ch->only.pc->quest_level == 55)
			gain_epic(ch, EPIC_QUEST, 0, number(10, 11));

		if (ch->only.pc->quest_level > 55)
			gain_epic(ch, EPIC_QUEST, 0, 15);
	}
}

int quest_exp_reward(P_char ch, int /*type*/)
{
	int exp_gain = 0;
	if (GET_LEVEL(ch) <= 30)
		exp_gain = EXP_NOTCH(ch) * get_property("world.quest.exp.level.30.andUnder", 1.300);
	else if (GET_LEVEL(ch) <= 40)
		exp_gain = EXP_NOTCH(ch) * get_property("world.quest.exp.level.40.andUnder", 1.000);
	else if (GET_LEVEL(ch) <= 50)
		exp_gain = EXP_NOTCH(ch) * get_property("world.quest.exp.level.50.andUnder", 0.800);
	else if (GET_LEVEL(ch) <= 55)
		exp_gain = EXP_NOTCH(ch) * get_property("world.quest.exp.level.55.andUnder", 0.500);
	else
		exp_gain = EXP_NOTCH(ch) * get_property("world.quest.exp.level.other", 1.000);
	return exp_gain;
}

P_obj quest_item_reward(P_char ch)
{
	const int quest_level = ch->only.pc->quest_level > 0 ? ch->only.pc->quest_level :
							       GET_LEVEL(ch);
	P_obj reward = read_object(real_object(getQuestItemFromZone(
					   real_zone(ch->only.pc->quest_zone_number), quest_level)),
				   REAL);

	if (!reward)
	{
		reward = create_random_eq_new(ch, ch, -1, -1);
	}

	if (reward)
	{
		wizlog(56, "quest_item_reward: %s reward was: %s [%d] ivalue %d.", GET_NAME(ch),
		       reward->short_description, OBJ_VNUM(reward), itemvalue(reward));

		REMOVE_BIT(reward->extra_flags, ITEM_SECRET);
		REMOVE_BIT(reward->extra_flags, ITEM_INVISIBLE);
		SET_BIT(reward->extra_flags, ITEM_NOREPAIR);
		REMOVE_BIT(reward->extra_flags, ITEM_NODROP);
	}
	return reward;
}

void quest_full_reward(P_char ch, P_char quest_mob, int type)
{
	char Gbuf1[MAX_STRING_LENGTH];

	if (!IS_ALIVE(ch) || IS_NPC(ch))
	{
		return;
	}

	P_obj reward = quest_item_reward(ch);
	if (reward)
	{
		act("$n gives you $q ", TRUE, quest_mob, reward, ch, TO_VICT);
		act("$n gives $N $q.", FALSE, quest_mob, reward, ch, TO_NOTVICT);
		obj_to_char(reward, ch);
	}

	if (GET_CLASS(ch, CLASS_MERCENARY))
	{
		if (GET_LEVEL(ch) > 24)
		{
			int temp = GET_LEVEL(ch) * 1256 + number(1, 500);
			mobsay(quest_mob, "I know you mercenaries don't work for free. Take this.");
			snprintf(Gbuf1, MAX_STRING_LENGTH, "You receive %s.\r\n",
				 coin_stringv(temp));
			send_to_char(Gbuf1, ch);
			ADD_MONEY(ch, temp);
		}
		else
		{
			mobsay(quest_mob,
			       "Sorry, but it's hard to take a little pipsqueek like you seriously as a mercenary. Grow up a bit and you'll start earning your keep.");
		}
	}

	quest_epic_reward(ch, type);

	int exp_gain = quest_exp_reward(ch, type);
	gain_exp(ch, NULL, exp_gain, EXP_WORLD_QUEST);

	snprintf(Gbuf1, MAX_STRING_LENGTH, "&+WYou gain some experience.&n");
	act(Gbuf1, FALSE, quest_mob, 0, ch, TO_VICT);

	sql_world_quest_finished(ch, reward);
	mark_player_dirty_components(GET_PID(ch), PLAYER_COMPONENT_STATUS);

	resetQuest(ch);
	gmcp_quest_status(ch);
}

void quest_ask(P_char ch, P_char quest_mob)
{
	if (ch->only.pc->quest_active != 1)
		return;

	if (IS_PC(quest_mob))
		return;

	if ((GET_VNUM(quest_mob) != ch->only.pc->quest_mob_vnum))
		return;

	if (ch->only.pc->quest_type != FIND_AND_ASK)
		return;

	wizlog(56, "%s finished quest @%s (ask quest)", GET_NAME(ch),
	       quest_mob->player.short_descr);
	do_action(quest_mob, 0, CMD_NOD);

	mobsay(quest_mob,
	       "Thanks for bringing me the message. Here is a little something for your trouble.");

	quest_full_reward(ch, quest_mob, FIND_AND_ASK);
}

void quest_kill(P_char ch, P_char quest_mob)
{
	if (!IS_ALIVE(ch) || !(quest_mob) || IS_NPC(ch))
	{
		return;
	}

	if (ch->only.pc->quest_active != 1 || IS_PC(quest_mob) ||
	    (GET_VNUM(quest_mob) != ch->only.pc->quest_mob_vnum))
	{
		return;
	}

	if (ch->only.pc->quest_accomplished == 1)
	{
		send_to_char(
			"This quest is already finished, go visit your quest master for your reward!\r\n",
			ch);
		return;
	}

	ch->only.pc->quest_kill_how_many++;

	int exp_gain = quest_exp_reward(ch, FIND_AND_KILL);

	if (ch->only.pc->quest_kill_original == 0)
	{
		send_to_char("&-RMEMORY ERROR! quest_kill_original is zero, inform a god!&n\r\n",
			     ch);
		wizlog(56, "MEMORY ERROR: quest_kill_original is zero!");
		ch->only.pc->quest_kill_original = ch->only.pc->quest_kill_how_many;
	}

	gain_exp(ch, NULL, exp_gain / ch->only.pc->quest_kill_original, EXP_WORLD_QUEST);
	if (number(1, ch->only.pc->quest_kill_original + 1) <= 2)
	{
		P_obj reward = quest_item_reward(ch);
		if (reward)
			obj_to_char(reward, quest_mob);
	}

	if (ch->only.pc->quest_kill_how_many - ch->only.pc->quest_kill_original == 0)
	{
		send_to_char("&+WCongratulations&n&n&+W, you finished your quest!&n\r\n", ch);
		wizlog(56, "%s finished quest @%s (kill quest)", GET_NAME(ch),
		       quest_mob->player.short_descr);

		if (GET_CLASS(ch, CLASS_MERCENARY) && GET_LEVEL(ch) > 24)
		{
			int temp = GET_LEVEL(ch) * 1256 + number(1, 500);
			act("A sneaky gremlin appears near $n and whispers something.", TRUE, ch, 0,
			    0, TO_ROOM);
			act("A sneaky gremlin appears beside you.", FALSE, ch, 0, 0, TO_CHAR);
			act("A sneaky gremlin whispers '&+LMy master sent me with your payment&N'.",
			    FALSE, ch, 0, 0, TO_CHAR);
			send_to_char_f(ch, "You receive %s.\r\n", coin_stringv(temp));
			ADD_MONEY(ch, temp);
		}

		quest_epic_reward(ch, FIND_AND_KILL);
		sql_world_quest_finished(ch, 0);
		mark_player_dirty_components(GET_PID(ch), PLAYER_COMPONENT_STATUS);
		resetQuest(ch);
		gmcp_quest_status(ch);
	}
	else
	{
		send_to_char(
			"&+YCongratulations&+y, you found the right mob, but you're not done yet.\r\n",
			ch);
		gmcp_quest_status(ch);
	}
}

#define EVIL_CONT 0
#define GOOD_CONT 1
#define UNDER_DARK 2

int get_map_room(int zone_id)
{
	int i2;
	int i;
	struct zone_data *zone = 0;

	if (zone_id < 0)
		return -1;

	zone = &zone_table[zone_id];

	for (i = zone->real_bottom; (i != NOWHERE) && (i <= zone->real_top); i++)
		for (i2 = 0; i2 < NUM_EXITS; i2++)
			if (world[i].dir_option[i2])
			{
				if ((world[i].dir_option[i2]->to_room == NOWHERE) ||
				    (world[world[i].dir_option[i2]->to_room].zone != world[i].zone))
				{
					if (world[i].dir_option[i2]->to_room == NOWHERE)
						;
					else
					{
						if (IS_MAP_ROOM(world[i].dir_option[i2]->to_room))
						{
							/*
						   snprintf(o_buf, MAX_STRING_LENGTH,
						   " &+Y[&n%5d&+Y](&n%5d&+Y)&n &+R%-5s&n to &+Y[&+R%3d&n:&+Y%5d&+Y](&n%5d&+Y)&n %s\n",
						   world[i].number, i, dirs[i2],
						   world[world[i].dir_option[i2]->to_room].zone,
						   world[world[i].dir_option[i2]->to_room].number,
						   world[i].dir_option[i2]->to_room,
						   world[world[i].dir_option[i2]->to_room].name);
						   wizlog(56, "%s", o_buf);
						 */
							return world[i].dir_option[i2]->to_room;
						}
					}
				}
			}
	return -1;
}

void do_quest(P_char ch, char *args, int /*cmd*/)
{
	P_char q_mob;
	P_char q_giver;
	P_char victim;

	char buf[MAX_STRING_LENGTH];
	char q_name[MAX_STRING_LENGTH];

	char name[MAX_INPUT_LENGTH], who[MAX_STRING_LENGTH];

	if (IS_NPC(ch))
	{
		return;
	}

	// Allow Illithids to do bartender quests.
	/* if( IS_ILLITHID(ch) )
	 {
	   send_to_char("No quests for you!", ch);
	   return;
	 }*/

	half_chop(args, name, who);
	if (*name)
	{
		if (isname(name, "reset") && IS_TRUSTED(ch))
		{
			if (!*who)
			{
				send_to_char("Whos quest do you want to reset?\r\n", ch);
				return;
			}

			victim = ParseTarget(ch, who);
			if (!victim)
			{
				send_to_char("Hmm, who do you mean?\r\n", ch);
				return;
			}

			resetQuest(victim);

			send_to_char("Someone magically cleared your quest.\r\n", victim);
			send_to_char("You've cleared his quest.\r\n", ch);
			return;
		}
		else if (isname(name, "stat") && IS_TRUSTED(ch))
		{
			pc_only_data *pcdata;

			if (!*who)
			{
				send_to_char("Whos quest do you want to examine?\r\n", ch);
				return;
			}
			if (!(victim = ParseTarget(ch, who)))
			{
				send_to_char("Hmm, nobody around here by that name.\r\n", ch);
				return;
			}
			if (IS_NPC(victim))
			{
				send_to_char("Hmm, mobs don't have quests.  Try again.\n\r", ch);
				return;
			}
			pcdata = victim->only.pc;
			// One big string.. rawr.
			snprintf(
				buf, MAX_STRING_LENGTH,
				"Quest for %s (PID %d): \n\rActive: %s, Level: %d, Type: %s, Started: %s, Accomplished: %s\n\r"
				"Bartender Vnum: %d, Shares left: %d, Original Questor PID: %d, Can share: %s\n\r"
				"Target Mob Vnum: %d, Number Killed: %d out of %d needed.\n\r"
				"Zone: %s (%d), Map room: %s (%d), Map purchased: %s.\n\r",
				J_NAME(victim), GET_PID(victim), YESNO(pcdata->quest_active),
				pcdata->quest_level,
				(pcdata->quest_type == FIND_AND_ASK)	   ? "ASK" :
				(pcdata->quest_type == FIND_AND_KILL)	   ? "KILL" :
				(pcdata->quest_type == FIND_AND_SOMETHING) ? "SOMETHING" :
									     "UNKNOWN",
				YESNO(pcdata->quest_started), YESNO(pcdata->quest_accomplished),
				pcdata->quest_giver, pcdata->quest_shares_left,
				pcdata->quest_receiver,
				YESNO(GET_PID(victim) == pcdata->quest_receiver),
				pcdata->quest_mob_vnum, pcdata->quest_kill_how_many,
				pcdata->quest_kill_original,
				zone_table[real_zone0(pcdata->quest_zone_number)].name,
				pcdata->quest_zone_number,
				world[real_room0(pcdata->quest_map_room)].name,
				pcdata->quest_map_room, YESNO(pcdata->quest_map_bought));
			send_to_char(buf, ch);

			return;
		}
	}

	if (ch->only.pc->quest_active == 0)
	{
		send_to_char("You don't have a quest!\r\n", ch);
		return;
	}

	if (*name)
		if (isname(name, "share"))
		{
			if (ch->only.pc->quest_receiver != GET_PID(ch))
			{
				send_to_char("Only the quest receiver can share the quest.\r\n",
					     ch);
				return;
			}

			/*if(ch->only.pc->quest_kill_how_many != 0)
			{
			  send_to_char("You already started the quest alone, so finish it alone!\r\n", ch);
			  return;
			}*/
			if (ch->only.pc->quest_shares_left == 0)
			{
				send_to_char("You cant share this quest with more people.\r\n", ch);
				return;
			}

			if (!*who)
			{
				send_to_char("Who do you want to share your quest with?\r\n", ch);
				return;
			}

			victim = ParseTarget(ch, who);
			if (!victim)
			{
				send_to_char("Hmm, who do you mean?\r\n", ch);
				return;
			}

			if (victim->only.pc->quest_active)
			{
				send_to_char("Your fellow adventurer is already on a task.\r\n",
					     ch);
				return;
			}

			if (!is_linked_to(ch, victim, LNK_CONSENT))
			{
				send_to_char(
					"Your fellow adventurer needs to consent you first.\r\n",
					ch);
				return;
			}

			if (ch->only.pc->quest_accomplished == 1)
			{
				send_to_char(
					"Hehe, sharing it so they get reward without work?!\r\n",
					ch);
				return;
			}

			if (ch->only.pc->quest_level <
			    GET_LEVEL(victim) - get_property("world.quest.level.high.value", 6))
			{
				send_to_char("They are much too experienced for that quest!\r\n",
					     ch);
				return;
			}

			if (ch->only.pc->quest_level >
			    GET_LEVEL(victim) + get_property("world.quest.level.low.value", 6))
			{
				send_to_char("They are too inexperienced for that quest!\r\n", ch);
				return;
			}

			if (sql_world_quest_can_do_another(victim) < 1)
			{
				send_to_char(
					"You are unable to receive a new quest, maybe you have already done a few recently?\r\n",
					victim);
				send_to_char(
					"They are unable to receive your quest, you need to find another friend to join you.\r\n",
					ch);
				return;
			}

			if (sql_world_quest_done_already(victim, ch->only.pc->quest_mob_vnum))
			{
				send_to_char(
					"You are unable to receive a new quest, maybe you have already done this quest??\r\n",
					victim);
				send_to_char("They've already done this quest!\r\n", ch);
				return;
			}
			resetQuest(victim);

			ch->only.pc->quest_shares_left--;

			send_to_char("You receive a quest.\r\n", victim);

			send_to_char("You've shared your quest.\r\n", ch);

			victim->only.pc->quest_active = ch->only.pc->quest_active;
			victim->only.pc->quest_mob_vnum = ch->only.pc->quest_mob_vnum;
			victim->only.pc->quest_type = ch->only.pc->quest_type;
			victim->only.pc->quest_accomplished = ch->only.pc->quest_accomplished;
			victim->only.pc->quest_started = ch->only.pc->quest_started;
			victim->only.pc->quest_zone_number = ch->only.pc->quest_zone_number;
			victim->only.pc->quest_giver = ch->only.pc->quest_giver;
			victim->only.pc->quest_level = ch->only.pc->quest_level;
			victim->only.pc->quest_receiver = ch->only.pc->quest_receiver;
			victim->only.pc->quest_kill_how_many = ch->only.pc->quest_kill_how_many;
			victim->only.pc->quest_kill_original = ch->only.pc->quest_kill_original;
			do_quest(victim, writable_arg(""), 0);
			gmcp_quest_status(victim);

			return;
		}

	if ((q_giver = read_mobile(real_mobile(ch->only.pc->quest_giver), REAL)))
	{
		snprintf(q_name, MAX_STRING_LENGTH, "%s", q_giver->player.short_descr);
		checked_snprintf(buf, MAX_STRING_LENGTH, "%s gave you the following quest:\r\n",
				 q_name);
		send_to_char(buf, ch);

		if (q_giver)
		{
			extract_char(q_giver);
			q_giver = NULL;
		}
	}
	else
	{
		wizlog(56, "UNABLE TO LOAD Q MOB:%d for char %s reseting his quest.",
		       ch->only.pc->quest_giver, GET_NAME(ch));
		send_to_char(
			"HMMMMM (bug), Seems like you forgot your quest already, go grab a new one, if this one dont work, try another bartender around the world. Sorry we working on a fix ASAP.!\r\n",
			ch);
		resetQuest(ch);
		return;
	}

	// wizlog(56, "questmob num:%d",ch->only.pc->quest_mob_vnum);
	if ((q_mob = read_mobile(real_mobile(ch->only.pc->quest_mob_vnum), REAL)))
	{
		snprintf(q_name, MAX_STRING_LENGTH, "%s", q_mob->player.short_descr);

		if (ch->only.pc->quest_type == FIND_AND_ASK)
		{
			checked_snprintf(
				buf, MAX_STRING_LENGTH, "Go ask %s in %s about the %s.\r\n", q_name,
				zone_table[real_zone0(ch->only.pc->quest_zone_number)].name,
				month_name[time_info.month]);
			send_to_char(buf, ch);
		}
		else if (ch->only.pc->quest_type == FIND_AND_KILL)
		{
			checked_snprintf(
				buf, MAX_STRING_LENGTH, "Go kill %d %s (%d left) in %s!\r\n",
				ch->only.pc->quest_kill_original, q_name,
				(ch->only.pc->quest_kill_original -
				 ch->only.pc->quest_kill_how_many),
				zone_table[real_zone0(ch->only.pc->quest_zone_number)].name);
			send_to_char(buf, ch);
			wizlog(56,
			       "%s got a quest to go kill %d of mob vnum %d; they have %d left.",
			       GET_NAME(ch), ch->only.pc->quest_kill_original,
			       ch->only.pc->quest_mob_vnum,
			       (ch->only.pc->quest_kill_original -
				ch->only.pc->quest_kill_how_many));
		}

		if (q_mob)
		{
			extract_char(q_mob);
			q_mob = NULL;
		}
	}
	else
	{
		send_to_char(
			"HMMMMMMMMMM, something's broken with your quest, tell a god ):(2)\r\n",
			ch);
		return;
	}
	// always call this

	if (ch->only.pc->quest_map_bought == 1)
	{
		send_to_char("You also got some additional information from the quest master:\r\n",
			     ch);
		show_map_at(ch, real_room(ch->only.pc->quest_map_room));
	}

	if (ch->only.pc->quest_accomplished == 1)
	{
		send_to_char("This quest is already finished, go talk to the quest master!\r\n",
			     ch);
	}

	if (ch->only.pc->quest_receiver != GET_PID(ch))
	{
		send_to_char("&+RThis quest was shared with you&n.\r\n", ch);
	}
	else
	{
		send_to_char("&+RThis quest was given to you&n.\r\n", ch);
		snprintf(buf, MAX_STRING_LENGTH,
			 "You can share this quest with %d more people.\r\n",
			 ch->only.pc->quest_shares_left);
		send_to_char(buf, ch);
	}

	send_to_char(
		"If you're unable to finish it, go to the quest master and ask him to &+Wabandon&n your quest!\r\n",
		ch);
}

// Attempts to create a quest for ch (a PC), given by giver (a NPC).
// Returns TRUE if successful, and FALSE if failed.
bool createQuest(P_char ch, P_char giver, quest_creation_failure *failure)
{
	int quest_zone = -1;
	int quest_mob = -1;
	int quest_type = -1;
	int target_probe_budget = WORLD_QUEST_MAX_TARGET_PROBES;
	int history_check_budget = WORLD_QUEST_MAX_HISTORY_CHECKS;
	vector<int> tried_targets;

	if (failure)
		*failure = QUEST_CREATION_NO_FAILURE;

	// Fail if missing an arg, or ch not a PC, or giver not an NPC or God.
	if (!giver || !ch || IS_NPC(ch) || (IS_PC(giver) && !IS_TRUSTED(giver)))
	{
		if (failure)
			*failure = QUEST_CREATION_INVALID_ACTOR;
		return FALSE;
	}

	vector<int> valid_zones;
	getQuestZoneList(ch, valid_zones);
	if (valid_zones.empty())
	{
		wizlog(56, "Unable to find a quest zone for %s", GET_NAME(ch));
		if (failure)
			*failure = QUEST_CREATION_NO_ELIGIBLE_ZONE;
		return FALSE;
	}

	// Visit each zone once, trying distinct targets before moving on. The
	// catalog and its cached scores are never rebuilt on the command path.
	vector<int> remaining_zones = valid_zones;
	while (!remaining_zones.empty() && history_check_budget > 0)
	{
		quest_zone = world_quest_policy_select_zone(ch, remaining_zones);
		if (quest_zone < 0)
			break;
		for (vector<int>::iterator zone = remaining_zones.begin();
		     zone != remaining_zones.end(); ++zone)
		{
			if (*zone == quest_zone)
			{
				remaining_zones.erase(zone);
				break;
			}
		}

		const int first_type = GET_LEVEL(ch) > 49 ? FIND_AND_KILL : number(1, 2);
		const int second_type =
			first_type == FIND_AND_KILL ?
				FIND_AND_ASK :
				(first_type == FIND_AND_ASK ? FIND_AND_KILL : FIND_AND_ASK);
		const int type_attempts = GET_LEVEL(ch) > 49 ? 1 : 2;
		for (int attempt = 0; attempt < type_attempts; ++attempt)
		{
			quest_type = attempt == 0 ? first_type : second_type;
			while (history_check_budget > 0)
			{
				quest_mob = world_quest_policy_suggest_mob(quest_zone, ch,
									   quest_type,
									   &target_probe_budget,
									   tried_targets);
				if (quest_mob <= 0)
					break;
				tried_targets.push_back(quest_mob);
				--history_check_budget;

				const int mob_rnum = real_mobile(quest_mob);
				if (mob_rnum < 0 ||
				    (quest_type == FIND_AND_KILL &&
				     mob_index[mob_rnum].number < 2) ||
				    (quest_type == FIND_AND_ASK && mob_index[mob_rnum].number < 1))
				{
					quest_mob = -1;
					continue;
				}

				const int completed = sql_world_quest_done_already(ch, quest_mob);
				if (completed < 0)
				{
					// A failed read stops the request; it must not grant a quest
					// or trigger more history queries against a failing backend.
					if (failure)
						*failure = QUEST_CREATION_NO_ELIGIBLE_TARGET;
					return FALSE;
				}
				if (completed > 0)
				{
					quest_mob = -1;
					continue;
				}
				break;
			}
			if (quest_mob > 0)
				break;
		}
		if (quest_mob > 0)
			break;
	}

	if (quest_mob <= 0)
	{
		wizlog(56, "Unable to find a valid quest target for %s", GET_NAME(ch));
		if (failure)
			*failure = QUEST_CREATION_NO_ELIGIBLE_TARGET;
		return FALSE;
	}

	const int rnum = real_mobile(quest_mob);
	if (rnum < 0)
	{
		if (failure)
			*failure = QUEST_CREATION_NO_ELIGIBLE_TARGET;
		return FALSE;
	}

	const int quest_kill_original = MIN(number(7, 9), mob_index[rnum].number - 1);
	ch->only.pc->quest_shares_left = 4;
	ch->only.pc->quest_active = 1;
	ch->only.pc->quest_mob_vnum = quest_mob;
	ch->only.pc->quest_type = quest_type;
	ch->only.pc->quest_accomplished = 0;
	ch->only.pc->quest_started = time(NULL);
	ch->only.pc->quest_zone_number = zone_table[quest_zone].number;
	ch->only.pc->quest_giver = GET_VNUM(giver);
	ch->only.pc->quest_level = GET_LEVEL(ch);
	ch->only.pc->quest_receiver = GET_PID(ch);
	ch->only.pc->quest_kill_how_many = 0;
	ch->only.pc->quest_kill_original = quest_kill_original;
	debug("Quest Kill Original Value: %d, mob_index number: %d, mob_index limit: %d, mob_vnum: %d",
	      ch->only.pc->quest_kill_original, mob_index[rnum].number, mob_index[rnum].limit - 1,
	      mob_index[rnum].virtual_number);

	return TRUE;
}

void show_map_at(P_char ch, int room)
{
	if (!IS_ALIVE(ch) || IS_NPC(ch))
	{
		return;
	}

	if (!(room))
	{
		send_to_char(
			"&+W* There is a bug with the map for this quest please contact an Admin.&n.\r\n",
			ch);
		logit(LOG_DEBUG, "World quest: (%s) failed in show_map_at for room (%d).",
		      GET_NAME(ch), room);
		return;
	}

	if (ch->only.pc->quest_map_bought == 1)
	{
		if (room == -1)
		{
			send_to_char(
				"&+W* This zone is not connected to the overland maps of duris&n.\r\n",
				ch);
			return;
		}

		int old_room = ch->in_room;
		char_from_room(ch);
		char_to_room(ch, room, -2);
		display_map(ch, 20, 999);
		char_from_room(ch);
		char_to_room(ch, old_room, -2);
		return;
	}
}

int quest_buy_map(P_char ch)
{
	if (!IS_ALIVE(ch) || IS_NPC(ch))
	{
		return 0;
	}

	if (ch->only.pc->quest_map_bought == 1)
	{
		send_to_char(
			"Your memory is bad, you dont have to pay me for this, you can just type 'quest'\r\n.",
			ch);
		return 0;
	}

	ch->only.pc->quest_map_bought = 1;
	send_to_char(
		"The 'quest' command will now show you additional information about the quest.\r\n",
		ch);

	int map_room = get_map_room(real_zone(ch->only.pc->quest_zone_number));

	// debug("quest_zone_number: %d, real_zone: %d, map_room: %d", ch->only.pc->quest_zone_number, real_zone(ch->only.pc->quest_zone_number), map_room);

	if (map_room > 0)
	{
		ch->only.pc->quest_map_room = world[map_room].number;
		// debug("quest_map_room: %d", ch->only.pc->quest_map_room);
		show_map_at(ch, map_room);

		/* Send quest map via GMCP for WebSocket clients */
		gmcp_quest_map(ch);
	}

	return 1;
}

// Return random vnum for an item from a zone's boot-time cache.
int getItemFromZone(int zone)
{
	return world_quest_policy_random_item(zone);
}

int getQuestItemFromZone(int zone, int quest_level)
{
	return world_quest_policy_random_quest_item(zone, quest_level);
}

bool isInvalidQuestZone(int zone_number)
{
	return world_quest_policy_zone_is_invalid(zone_number);
}

void getQuestZoneList(P_char ch, vector<int> &valid_zones)
{
	world_quest_policy_zone_list(ch, valid_zones);
}

int suggestQuestMob(int zone_num, P_char ch, int QUEST_TYPE)
{
	int target_probe_budget = WORLD_QUEST_MAX_TARGET_PROBES;
	return world_quest_policy_suggest_mob(zone_num, ch, QUEST_TYPE, &target_probe_budget);
}

// Called from comm.c to populate the boot-time world-quest catalog and avg_mob_level.
int calc_zone_mob_level()
{
	return world_quest_policy_bootstrap() ? 0 : -1;
}
