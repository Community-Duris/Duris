// Wizard character creation and approval commands.
#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "world/db.h"
#include "cmd/interp.h"
#include "core/utility.h"
#include "core/utils.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "account/account.h"
#include "account/creation_availability_config.h"
#include "world/epic.h"
#include "classes/epic_skills.h"
#include "guild/assocs.h"
#include "item/trophy.h"
#include "core/files.h"
#include "core/mm.h"
#include "magic/spells.h"
#include "sql/sql.h"
#include "sql/sql_player_identity.h"

extern int class_table[LAST_RACE + 1][CLASS_COUNT + 1];
extern Skill skills[];
extern epic_reward epic_rewards[];
extern struct race_names race_names_table[];
extern struct class_names class_names_table[];
extern struct mm_ds *dead_mob_pool;
extern struct mm_ds *dead_pconly_pool;
extern P_desc descriptor_list;
extern struct race_names race_names_table[];

// syntax: newchar <name> <race_id> <class_id> <level> <true|false>
// see: newchar help race, newchar help class
void do_newchar(P_char ch, char *argument, int /*cmd*/)
{
	extern int writeCharacter(P_char ch, int type, int room);
	extern void clear_char(P_char ch);
	extern void init_char(P_char ch);
	extern char *mysql_str(const char *str, char *buf);

	char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
	char arg3[MAX_INPUT_LENGTH], arg4[MAX_INPUT_LENGTH], arg5[MAX_INPUT_LENGTH];
	char buf[MAX_STRING_LENGTH];
	char name_lower[MAX_NAME_LENGTH + 1];
	int race_id, class_id, level, class_bitmask, class_index;
	int i, is_playable;
	bool max_skills;
	P_char newch;

	if (IS_NPC(ch))
		return;

	if (GET_LEVEL(ch) < OVERLORD)
	{
		send_to_char("you need to be an overlord to use this.\r\n", ch);
		return;
	}

	if (!ch->desc || !ch->desc->account)
	{
		send_to_char("you need a valid account connection.\r\n", ch);
		return;
	}

	half_chop(argument, arg1, buf);
	half_chop(buf, arg2, buf);
	half_chop(buf, arg3, buf);
	half_chop(buf, arg4, buf);
	half_chop(buf, arg5, buf);

	// handle help subcommands
	if (!*arg1 || !str_cmp(arg1, "help"))
	{
		if (!*arg2 || !str_cmp(arg2, "usage"))
		{
			send_to_char(
				"usage: newchar <name> <race_id> <class_id> <level> <true|false>\r\n",
				ch);
			send_to_char("       newchar help race   - list playable races\r\n", ch);
			send_to_char("       newchar help class  - list classes\r\n", ch);
			send_to_char("\r\nexample: newchar testnecro 3 11 56 true\r\n", ch);
			send_to_char(
				"creates a drow elf necromancer at level 56 with maxed skills.\r\n",
				ch);
			return;
		}

		if (!str_cmp(arg2, "race"))
		{
			send_to_char("playable races:\r\n", ch);
			send_to_char("id   race              faction\r\n", ch);
			send_to_char("---- ----------------- -------\r\n", ch);
			for (i = 0; playable_races[i].race_id != -1; i++)
			{
				snprintf(buf, MAX_STRING_LENGTH, "%-4d %-17s %s\r\n",
					 playable_races[i].race_id,
					 race_names_table[playable_races[i].race_id].normal,
					 playable_races[i].faction);
				send_to_char(buf, ch);
			}
			return;
		}

		if (!str_cmp(arg2, "class"))
		{
			send_to_char("classes:\r\n", ch);
			send_to_char("id   class\r\n", ch);
			send_to_char("---- --------------\r\n", ch);
			for (i = 1; i <= CLASS_COUNT; i++)
			{
				if (class_names_table[i].normal)
				{
					snprintf(buf, MAX_STRING_LENGTH, "%-4d %s\r\n", i,
						 class_names_table[i].normal);
					send_to_char(buf, ch);
				}
			}
			return;
		}

		send_to_char("unknown help topic. try: newchar help\r\n", ch);
		return;
	}

	// validate we have all required args
	if (!*arg2 || !*arg3 || !*arg4 || !*arg5)
	{
		send_to_char("usage: newchar <name> <race_id> <class_id> <level> <true|false>\r\n",
			     ch);
		return;
	}

	// validate name
	if (_parse_name(arg1, name_lower, true))
	{
		send_to_char("invalid name.\r\n", ch);
		return;
	}

	// capitalize properly
	name_lower[0] = toupper(name_lower[0]);
	for (i = 1; name_lower[i]; i++)
		name_lower[i] = tolower(name_lower[i]);

	if (sql_player_exists(name_lower))
	{
		send_to_char("that name is already taken.\r\n", ch);
		return;
	}

	// parse race_id
	race_id = atoi(arg2);
	if (race_id < 0 || race_id > LAST_RACE)
	{
		send_to_char("invalid race id. use 'newchar help race' to see valid ids.\r\n", ch);
		return;
	}

	// check if race is playable
	is_playable = 0;
	for (i = 0; playable_races[i].race_id != -1; i++)
	{
		if (playable_races[i].race_id == race_id)
		{
			is_playable = 1;
			break;
		}
	}
	if (!is_playable)
	{
		send_to_char("that race is not playable. use 'newchar help race'.\r\n", ch);
		return;
	}

	// parse class_id
	class_id = atoi(arg3);
	if (class_id < 1 || class_id > CLASS_COUNT)
	{
		send_to_char("invalid class id. use 'newchar help class' to see valid ids.\r\n",
			     ch);
		return;
	}

	// check race/class combo
	if (class_table[race_id][class_id] == 5)
	{
		send_to_char("that class is not available for that race.\r\n", ch);
		return;
	}

	// parse level
	level = atoi(arg4);
	if (level < 1 || level > 56)
	{
		send_to_char("level must be between 1 and 56.\r\n", ch);
		return;
	}

	// parse max_skills boolean
	if (!str_cmp(arg5, "true") || !str_cmp(arg5, "1") || !str_cmp(arg5, "yes"))
		max_skills = TRUE;
	else if (!str_cmp(arg5, "false") || !str_cmp(arg5, "0") || !str_cmp(arg5, "no"))
		max_skills = FALSE;
	else
	{
		send_to_char("last argument must be true or false.\r\n", ch);
		return;
	}

	// check account char limit
	if (ch->desc->account->num_chars >= MAX_CHARS_PER_ACCOUNT)
	{
		send_to_char("your account already has maximum characters.\r\n", ch);
		return;
	}

	// all validation passed - now allocate
	newch = (P_char)mm_get(dead_mob_pool);
	if (!newch)
	{
		send_to_char("memory allocation failed.\r\n", ch);
		return;
	}
	clear_char(newch);

	ensure_pconly_pool();
	newch->only.pc = (struct pc_only_data *)mm_get(dead_pconly_pool);
	if (!newch->only.pc)
	{
		free_char(newch);
		send_to_char("memory allocation failed.\r\n", ch);
		return;
	}
	memset(newch->only.pc, 0, sizeof(struct pc_only_data));
	newch->only.pc->aggressive = -1;

	// set name
	newch->player.name = str_dup(name_lower);

	// set race
	GET_RACE(newch) = race_id;

	// set class bitmask
	class_bitmask = 1 << (class_id - 1);
	newch->player.m_class = class_bitmask;
	class_index = flag2idx(class_bitmask) - 1;

	// set sex (default male)
	newch->player.sex = SEX_MALE;

	// set alignment based on race faction
	for (i = 0; playable_races[i].race_id != -1; i++)
	{
		if (playable_races[i].race_id == race_id)
		{
			if (!strcmp(playable_races[i].faction, "good"))
			{
				GET_ALIGNMENT(newch) = 1000;
				GET_RACEWAR(newch) = RACEWAR_GOOD;
			}
			else if (!strcmp(playable_races[i].faction, "evil"))
			{
				GET_ALIGNMENT(newch) = -1000;
				GET_RACEWAR(newch) = RACEWAR_EVIL;
			}
			else
			{
				// neutral race - default to evil
				GET_ALIGNMENT(newch) = -1000;
				GET_RACEWAR(newch) = RACEWAR_EVIL;
			}
			break;
		}
	}

	// set hometown
	GET_HOME(newch) = find_hometown(race_id, FALSE);
	GET_BIRTHPLACE(newch) = GET_HOME(newch);
	GET_ORIG_BIRTHPLACE(newch) = GET_HOME(newch);

	// set all stats to 100
	for (int stat_index = 0; stat_index < MAX_ATTRIBUTES; stat_index++)
		newch->base_stats[stat_index] = 100;
	newch->curr_stats = newch->base_stats;

	// init_char does pid assignment, skill setup, hp/mana/vitality etc
	init_char(newch);

	// override level after init_char (it may reset to 1)
	newch->player.level = level;

	// override pid to 0 to force db insert
	newch->only.pc->pid = 0;

	// set default player flags and prompt (init_char doesn't do this)
	newch->specials.act = (PLR_PETITION | PLR_ECHO | PLR_SNOTIFY | PLR_PAGING_ON | PLR_MAP);
	SET_BIT(newch->specials.act2, PLR2_NCHAT);
	SET_BIT(newch->specials.act2, PLR2_QUICKCHANT);
	SET_BIT(newch->specials.act2, PLR2_SPEC);
	SET_BIT(newch->specials.act2, PLR2_HINT_CHANNEL);
	SET_BIT(newch->specials.act2, PLR2_SHOW_QUEST);
	SET_BIT(newch->specials.act2, PLR2_BOON);
	SET_BIT(newch->specials.act2, PLR2_SHIPMAP);
	if (!GET_CLASS(newch, CLASS_PALADIN))
		SET_BIT(newch->specials.act, PLR_VICIOUS);

	newch->only.pc->wimpy = 10;
	newch->only.pc->aggressive = -1;
	newch->only.pc->prompt = (PROMPT_HIT | PROMPT_MAX_HIT | PROMPT_MOVE | PROMPT_MAX_MOVE |
				  PROMPT_TANK_NAME | PROMPT_TANK_COND | PROMPT_ENEMY |
				  PROMPT_ENEMY_COND | PROMPT_TWOLINE | PROMPT_STATUS);

	// set skills based on max_skills flag
	if (max_skills)
	{
		// regular skills
		for (i = FIRST_SKILL; i <= LAST_SKILL; i++)
		{
			if (class_index >= 0 && class_index < CLASS_COUNT)
			{
				int rlevel = skills[i].m_class[class_index].rlevel[0];
				int maxlearn = skills[i].m_class[class_index].maxlearn[0];
				if (rlevel > 0 && rlevel <= level)
				{
					newch->only.pc->skills[i].learned = maxlearn;
					newch->only.pc->skills[i].taught = maxlearn;
				}
			}
		}

		// epic skills
		for (i = 0; epic_rewards[i].type != 0; i++)
		{
			if (epic_rewards[i].type == EPIC_REWARD_SKILL)
			{
				int skill_num = epic_rewards[i].value;
				unsigned int allowed_classes = epic_rewards[i].classes;

				// check if class can have this epic skill (0 means all classes)
				if (allowed_classes == 0 || (allowed_classes & class_bitmask))
				{
					int maxlearn = epic_rewards[i].points_cost;
					newch->only.pc->skills[skill_num].learned = maxlearn;
					newch->only.pc->skills[skill_num].taught = maxlearn;
				}
			}
		}
	}

	// save character to db - this assigns auto-increment pid
	if (!writeCharacter(newch, RENT_QUIT, NOWHERE))
	{
		send_to_char("failed to save character to database.\r\n", ch);
		free_char(newch);
		return;
	}

	// now we have a valid pid from db auto-increment
	if (GET_PID(newch) <= 0)
	{
		send_to_char("failed to save character to database.\r\n", ch);
		free_char(newch);
		return;
	}

	// link to account via direct sql
	{
		char account_sql[MAX_STRING_LENGTH * 2 + 1];
		char name_sql[MAX_STRING_LENGTH * 2 + 1];

		mysql_str(ch->desc->account->acct_name, account_sql);
		mysql_str(newch->player.name, name_sql);

		if (!db_query(
			    "INSERT INTO account_characters "
			    "(account_name, pid, char_name, created_at, deleted_at) "
			    "VALUES('%s', %ld, '%s', NOW(), NULL) "
			    "ON DUPLICATE KEY UPDATE char_name = VALUES(char_name), deleted_at = NULL",
			    account_sql, GET_PID(newch), name_sql))
		{
			send_to_char("failed to link character to account.\r\n", ch);
			logit(LOG_DEBUG, "wiz_newchar: failed to link %s to account %s",
			      newch->player.name, ch->desc->account->acct_name);
			deleteCharacter(newch, FALSE);
			free_char(newch);
			return;
		}
	}

	// update in-memory account state for immediate visibility
	{
		struct acct_chars *c;
		CREATE(c, struct acct_chars, 1, MEM_TAG_OTHER);
		memset(c, 0, sizeof(struct acct_chars));
		c->charname = str_dup(newch->player.name);
		c->count = 1;
		c->last = time(NULL);
		c->racewar = (GET_RACEWAR(newch) == RACEWAR_EVIL) ? ACCT_EVIL : ACCT_GOOD;
		c->next = ch->desc->account->acct_character_list;
		ch->desc->account->acct_character_list = c;
		ch->desc->account->num_chars++;
	}

	// log it
	logit(LOG_WIZ, "%s created test character %s (race %d, class %d, level %d, maxskills %s)",
	      GET_NAME(ch), newch->player.name, race_id, class_id, level,
	      max_skills ? "true" : "false");
	wizlog(GET_LEVEL(ch), "%s created test character %s", GET_NAME(ch), newch->player.name);

	// notify the wizard
	snprintf(buf, MAX_STRING_LENGTH, "created %s - %s %s level %d (pid %d)\r\n",
		 newch->player.name, race_names_table[race_id].normal,
		 class_names_table[class_id].normal, level, GET_PID(newch));
	send_to_char(buf, ch);

	if (max_skills)
		send_to_char("all skills maxed.\r\n", ch);

	send_to_char("character saved. log out and select from account menu.\r\n", ch);

	// cleanup temp structures
	free_char(newch);
}

void do_decline(P_char ch, char *arg, int /*cmd*/)
{
	char Gbuf2[MAX_STRING_LENGTH], f_a[MAX_STRING_LENGTH], Gbuf1[MAX_STRING_LENGTH];
	P_desc d, i;

	arg = skip_spaces(arg);
	if (!*arg)
	{
		send_to_char("Usage: decline <charname> [reason]\n", ch);
		return;
	}
	arg = one_argument(arg, f_a);
	for (d = descriptor_list; d; d = d->next)
		if (STATE(d) == CON_ACCEPTWAIT && d->character &&
		    !str_cmp(GET_NAME(d->character), f_a))
		{
			if (!arg || !*arg)
			{
				SEND_TO_Q(
					"\n\nYour new character application has been declined. It is highly probable\n",
					d);
				SEND_TO_Q(
					"that either your name does not suit fantasy theme, or something else is\n"
					"inappropriate to the theme Duris strives to maintain.\n\n",
					d);
				SEND_TO_Q("Please enter another, more suitable fantasy name:", d);
			}
			else
			{
				SEND_TO_Q(
					"\n\nYour new character application has been declined.\nReason supplied was:",
					d);
				SEND_TO_Q(arg, d);
				SEND_TO_Q("\n\nPlease enter another, more suitable fantasy name:",
					  d);
			}
			d->character->only.pc->prestige = 0;
			logit(LOG_NEWCHAR, "%s declined new char %s (%s): %s.", GET_NAME(ch),
			      GET_NAME(d->character), (*d->host ? d->host : "UNKNOWN"),
			      (arg ? arg : "NO REASON GIVEN"));
			snprintf(Gbuf1, MAX_STRING_LENGTH,
				 "&+c*** STATUS: %s declined new player %s. (%s)\n", GET_NAME(ch),
				 GET_NAME(d->character), arg);
			snprintf(Gbuf2, MAX_STRING_LENGTH,
				 "&+c*** STATUS: Someone declined new player %s. (%s)\n",
				 GET_NAME(d->character), arg);
			for (i = descriptor_list; i; i = i->next)
				if (!i->connected && i->character &&
				    IS_SET(i->character->specials.act, PLR_PETITION) &&
				    IS_TRUSTED(i->character))
				{
					if (!CAN_SEE(i->character, ch))
						send_to_char(Gbuf2, i->character);
					else
						send_to_char(Gbuf1, i->character);
					//    deny_name(GET_NAME(d->character));
					STATE(d) = CON_NEW_NAME;
				}

			deny_name(GET_NAME(d->character));

			return;
		}
	send_to_char("Decline what new player's application?\n", ch);
}

extern int approve_mode;

#define APPROVE_OFF 0
#define APPROVE_ON 1
void do_approve(P_char ch, char *arg, int /*cmd*/)
{
	int count;
	P_desc d1, d2;
	char Gbuf1[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH];
	const char *approve_modes[] = { "off", "on", "\n" };

	if (!arg || !*arg)
	{
		/* list characters needing approval:  */
		count = 0;
		snprintf(Gbuf1, MAX_STRING_LENGTH,
			 "&+cAC: Post-creation approval system is now %s.\n",
			 approve_modes[approve_mode]);
		send_to_char(Gbuf1, ch);
		if (approve_mode == APPROVE_ON)
		{
			send_to_char("List of characters needing approval:\n", ch);
			for (d1 = descriptor_list; d1; d1 = d1->next)
			{
				if (STATE(d1) == CON_ACCEPTWAIT)
				{
					snprintf(
						Gbuf1, MAX_STRING_LENGTH,
						"%d. %s (%s %s) %s - Rolled for %ld:%02ld, Socket: %d, Idle: %d:%02d.\n",
						++count, GET_NAME(d1->character),
						get_class_string(d1->character, Gbuf2),
						race_names_table[(int)GET_RACE(d1->character)].ansi,
						*d1->host ? d1->host : "UNKNOWN",
						d1->character->only.pc->pc_timer[PC_TIMER_HEAVEN] /
							60,
						d1->character->only.pc->pc_timer[PC_TIMER_HEAVEN] %
							60,
						d1->descriptor, (d1->wait / WAIT_SEC) / 60,
						(d1->wait / WAIT_SEC) % 60);
					send_to_char(Gbuf1, ch);
				}
			}
			if (count == 0)
			{
				send_to_char("None.\n\n", ch);
			}
		}
		send_to_char("Usage: approve <charname|on|off>\n", ch);
		return;
	}
	arg = skip_spaces(arg);
	switch (search_block(arg, approve_modes, FALSE))
	{
	case 0:
		if (GET_LEVEL(ch) < 62)
		{
			send_to_char("Sorry, that option is overlord only.\n", ch);
			return;
		}
		approve_mode = APPROVE_OFF;

		snprintf(Gbuf1, MAX_STRING_LENGTH, "&+cAC: %s set newchar application system %s.\n",
			 GET_NAME(ch), approve_modes[APPROVE_OFF]);
		snprintf(Gbuf2, MAX_STRING_LENGTH,
			 "&+CAC: Someone set newchar application system %s.\n",
			 approve_modes[APPROVE_OFF]);
		logit(LOG_WIZ, "%s", Gbuf1);
		for (d1 = descriptor_list; d1; d1 = d1->next)
		{
			if ((d1->connected == CON_PLAYING) && d1->character &&
			    IS_SET(d1->character->specials.act, PLR_PETITION) &&
			    IS_TRUSTED(d1->character))
			{
				if (!CAN_SEE(d1->character, ch))
					send_to_char(Gbuf2, d1->character);
				else
					send_to_char(Gbuf1, d1->character);
			}
		}
		break;
	case 1:
		if (GET_LEVEL(ch) < 62)
		{
			send_to_char("Sorry, that option is overlord only.\n", ch);
			return;
		}
		approve_mode = APPROVE_ON;

		snprintf(Gbuf1, MAX_STRING_LENGTH, "&+cAC: %s set newchar application system %s.\n",
			 GET_NAME(ch), approve_modes[APPROVE_ON]);
		logit(LOG_WIZ, "%s", Gbuf1);
		snprintf(Gbuf2, MAX_STRING_LENGTH,
			 "&+cAC: Someone set newchar application system %s.\n",
			 approve_modes[APPROVE_ON]);
		for (d1 = descriptor_list; d1; d1 = d1->next)
		{
			if ((d1->connected == CON_PLAYING) && d1->character &&
			    IS_SET(d1->character->specials.act, PLR_PETITION) &&
			    IS_TRUSTED(d1->character))
			{
				if (!CAN_SEE(d1->character, ch))
					send_to_char(Gbuf2, d1->character);
				else
					send_to_char(Gbuf1, d1->character);
			}
		}
		break;
	default:
		for (d1 = descriptor_list; d1; d1 = d1->next)
		{
			if (STATE(d1) == CON_ACCEPTWAIT && d1->character &&
			    !str_cmp(GET_NAME(d1->character), arg))
			{
				logit(LOG_NEWCHAR, "%s approved new char %s from %s.", GET_NAME(ch),
				      GET_NAME(d1->character), (*d1->host ? d1->host : "UNKNOWN"));
				snprintf(Gbuf1, MAX_STRING_LENGTH,
					 "&+c*** STATUS: %s approved new player %s from %s.\n",
					 GET_NAME(ch), GET_NAME(d1->character),
					 *d1->host ? d1->host : "&+WUNKNOWN&n");
				snprintf(Gbuf2, MAX_STRING_LENGTH,
					 "&+c*** STATUS: Someone approved new player %s from %s.\n",
					 GET_NAME(d1->character),
					 *d1->host ? d1->host : "&+WUNKNOWN&n");

				for (d2 = descriptor_list; d2; d2 = d2->next)
				{
					if ((d2->connected == CON_PLAYING) && d2->character &&
					    IS_SET(d2->character->specials.act, PLR_PETITION) &&
					    IS_TRUSTED(d2->character))
					{
						if (!CAN_SEE(d2->character, ch))
							send_to_char(Gbuf2, d2->character);
						else
							send_to_char(Gbuf1, d2->character);
					}
				}

				SEND_TO_Q(
					"\nYour application for character has been approved. Welcome into ranks of\nthe players of Duris!\n\n",
					d1);
				SEND_TO_Q("\n*** PRESS RETURN:\n", d1);
				STATE(d1) = CON_WELCOME;
				approve_name(GET_NAME(d1->character));
				schedule_chaos_new_character_kit_before_entry(d1->character);

				return;
			}
		}

		if (approve_mode == APPROVE_OFF)
		{
			approve_name(arg);
			statuslog(GET_WIZINVIS(ch),
				  "Name '%s' added to the approved names list by %s", arg,
				  GET_NAME(ch));
		}
		else
		{
			send_to_char("No such player in the newplayer-queue! \n", ch);
		}
		break;
	}
}

// fullReset -> Do we reset epic skills/tradeskills?
void NewbySkillSet(P_char ch, bool fullReset)
{
	int i;

	// Walk through skills..
	for (i = FIRST_SKILL; i <= LAST_SKILL; i++)
	{
		if (!fullReset && (IS_EPIC_SKILL(i) || IS_TRADESKILL(i)))
		{
			continue;
		}
		// if they have the skill..
		if (SKILL_DATA_ALL(ch, i).rlevel[0] > 0 &&
		    SKILL_DATA_ALL(ch, i).rlevel[0] <= GET_LEVEL(ch) && !IS_SPELL(i))
		{
			ch->only.pc->skills[i].learned = number(5, 20);
			ch->only.pc->skills[i].taught = SKILL_DATA_ALL(ch, i).maxlearn[0] - 10;
		}
		else if (SKILL_DATA_ALL(ch, i).rlevel[0] && IS_SPELL(i) &&
			 SKILL_DATA_ALL(ch, i).rlevel[0] <= GET_LEVEL(ch) && praying_class(ch))
		{
			ch->only.pc->skills[i].learned = 100;
		}
		else
		{
			ch->only.pc->skills[i].learned = 0;
		}
	}
}

// If nomsg == 0, send a message and assume a new char.
// If nomsg == CMD_MULTICLASS, this is a new multiclassed char.
static void do_start_impl(P_char ch, int nomsg, bool grant_newbie_kit)
{
	int i;

	if (IS_NPC(ch))
	{
		return;
	}

	ch->player.level = 1;
	ch->points.base_hit = 1;
	ch->points.base_mana = 1;

	if (!nomsg)
	{
		send_to_char("Welcome. This is now your character on Duris.\n"
			     "May your journey here never end.....\n\n"
			     " NOTE:  Type TOGGLE and HELP for useful information!\n",
			     ch);
	}

	clear_title(ch);

	if (grant_newbie_kit && !(GET_CLASS(ch, CLASS_AVENGER | CLASS_DREADLORD)) &&
	    !((GET_RACE(ch) == RACE_DUERGAR || GET_RACE(ch) == RACE_MOUNTAIN) &&
	      GET_CLASS(ch, CLASS_BERSERKER)))
	{
		load_obj_to_newbies(ch);
	}

	if (nomsg == CMD_MULTICLASS)
	{
		/* Clear the skills array */
		for (i = 0; i < MAX_SKILLS; i++)
		{
			// We don't reset epic skills nor tradeskills when multiclassing.
			if (!IS_EPIC_SKILL(i) && !IS_TRADESKILL(i))
			{
				ch->only.pc->skills[i].learned = 0;
			}
		}
	}
	else
	{
		/* Clear the skills array */
		for (i = 0; i < MAX_SKILLS; i++)
		{
			ch->only.pc->skills[i].learned = 0;
		}
	}

	clear_zone_trophy(ch);

	ch->only.pc->prestige = 0;
	if (nomsg == CMD_MULTICLASS)
	{
		// Set ch to parole if guilded.
		if (GET_ASSOC(ch) != NULL)
		{
			SET_PAROLE(GET_A_BITS(ch));
		}
	}
	else
	{
		ch->specials.guild = 0;
		ch->specials.guild_status = 0;
	}

	/* These legacy standalone classes were replaced by Rogue specializations,
	   so their specialization menus are intentionally empty. Preserve their
	   old class-specific abilities when direct creation is explicitly open. */
	if (creation_all_classes_enabled() && !ch->player.spec)
	{
		if (ch->player.m_class == CLASS_ASSASSIN)
			ch->player.spec = SPEC_ASSMASTER;
		else if (ch->player.m_class == CLASS_THIEF)
			ch->player.spec = SPEC_CUTPURSE;
	}

	NewbySkillSet(ch, (nomsg != CMD_MULTICLASS) ? TRUE : FALSE);

	GET_EXP(ch) = 1;

	if (isname("Tyrus", GET_NAME(ch)) || god_check(GET_NAME(ch)))
	{
		ch->player.level = OVERLORD;
	}

	GET_MAX_HIT(ch) = GET_HIT(ch) = ch->points.base_hit;

	GET_MAX_MANA(ch) = GET_MANA(ch) = ch->points.base_mana;

	GET_MAX_VITALITY(ch) = GET_VITALITY(ch) = vitality_limit(ch);

	if (GET_RACE(ch) != RACE_ILLITHID)
	{
		GET_COND(ch, THIRST) = 96;
		GET_COND(ch, DRUNK) = 0;
	}
	else
	{
		GET_COND(ch, THIRST) = -1;
		GET_COND(ch, DRUNK) = -1;
	}

	GET_COND(ch, FULL) = 96;

	if (nomsg != CMD_MULTICLASS)
	{
		/* set some defaults. */
		ch->specials.act =
			(PLR_PETITION | PLR_ECHO | PLR_SNOTIFY | PLR_PAGING_ON | PLR_MAP);

		/* preserve hardcore and newbie bits */
		ch->specials.act2 &= (PLR2_HARDCORE_CHAR | PLR2_NEWBIE);

		SET_BIT(ch->specials.act2, PLR2_NCHAT);
		SET_BIT(ch->specials.act2, PLR2_QUICKCHANT);
		SET_BIT(ch->specials.act2, PLR2_SPEC);
		SET_BIT(ch->specials.act2, PLR2_HINT_CHANNEL);
		SET_BIT(ch->specials.act2, PLR2_SHOW_QUEST);
		SET_BIT(ch->specials.act2, PLR2_BOON);
		SET_BIT(ch->specials.act2, PLR2_SHIPMAP);
		if (!GET_CLASS(ch, CLASS_PALADIN))
		{
			SET_BIT(ch->specials.act, PLR_VICIOUS);
		}
		ch->only.pc->wimpy = 10;
		ch->only.pc->aggressive = -1;

		ch->only.pc->prompt = (PROMPT_HIT | PROMPT_MAX_HIT | PROMPT_MOVE | PROMPT_MAX_MOVE |
				       PROMPT_TANK_NAME | PROMPT_TANK_COND | PROMPT_ENEMY |
				       PROMPT_ENEMY_COND | PROMPT_TWOLINE | PROMPT_STATUS);
	}
	// New multi'd Paladins don't get vicious on.
	else if (GET_CLASS(ch, CLASS_PALADIN))
	{
		REMOVE_BIT(ch->specials.act, PLR_VICIOUS);
	}

	if (USES_MANA(ch))
	{
		ch->only.pc->prompt |= (PROMPT_MANA | PROMPT_MAX_MANA);
	}

	/*  if(!GET_CLASS(ch, CLASS_PSIONICIST) && !GET_CLASS(ch, CLASS_MINDFLAYER))
	    ch->only.pc->prompt =
	      PROMPT_HIT | PROMPT_MAX_HIT | PROMPT_MOVE | PROMPT_MAX_MOVE;
	  else
	    ch->only.pc->prompt = PROMPT_HIT | PROMPT_MAX_HIT | PROMPT_MANA |
	      PROMPT_MAX_MANA | PROMPT_MOVE | PROMPT_MAX_MOVE; */

#ifndef EQ_WIPE
	ch->player.time.played = 0;
#else
	ch->player.time.played = EQ_WIPE;
#endif
	ch->player.time.logon = time(0);
}

void do_start(P_char ch, int nomsg)
{
	do_start_impl(ch, nomsg, true);
}

void do_start_deferred_newbie_kit(P_char ch, int nomsg)
{
	do_start_impl(ch, nomsg, false);
}
