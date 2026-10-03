/*
 * Staff "where" inspection command and its private helpers.
 */

#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utility.h"
#include "core/utils.h"
#include "net/comm.h"
#include "world/db.h"

#define OBJ_COLOR(rnum)                                                 \
	((obj_index[rnum].number <= 1)			       ? "+L" : \
	 (obj_index[rnum].number - 1 > obj_index[rnum].limit)  ? "+W" : \
	 (obj_index[rnum].number - 1 == obj_index[rnum].limit) ? "n" :  \
								 "+w")

extern P_char character_list;
extern P_desc descriptor_list;
extern P_obj object_list;
extern P_room world;
extern const int top_of_world;
extern int number_of_quests;
extern struct quest_data quest_index[];
bool is_quested_item(P_obj obj);
extern const racewar_struct racewar_color[MAX_RACEWAR + 2];
extern const char *apply_types[];

// Looks through the list of objects in game for those with supplied stats.
static void where_stat(P_char ch, char *argument)
{
	char arg1[MAX_STRING_LENGTH];
	char arg2[MAX_STRING_LENGTH];
	char buf[MAX_STRING_LENGTH];
	int amount;
	int flags;
	int i;
	bool greater, lesser;
	P_obj obj;

	argument_interpreter(argument, arg1, arg2);

	if (arg1[0] == '\0' || arg2[0] == '\0')
	{
		if (!strcmp(arg1, "options"))
		{
			send_to_char(
				"&=LWHardcoded options&n&+W:&+w strength&+W,&+w dexterity&+W,&+w intelligence&+W,&+w wisdom&+W,&+w"
				" constitution&+W,&+w sex&+W,&+w class&+W,&+w level&+W,&+w age&+W,&+w weight&+W,&+w height&+W,&+w mana&+W,&+w"
				" hitpoints&+W,&+w moves&+W,&+w gold&+W,&+w exp&+W,&+w armor&+W|&+wac&+W,&+w hitroll&+W,&+w damroll&+W,&+w"
				" para&+W,&+w rod&+W,&+w fear&+W,&+w breath&+W,&+w spell&+W,&+w pff&+W|&+wfire&+W,&+w agility&+W,&+w,"
				" power&+W,&+w charisma&+W,&+w karma&+W,&+w luck&+W,&+w maxstr&+W|&+wmax_str&+W,&+w"
				" maxdex&+W|&+wmax_dex&+W,&+w maxint&+W|&+wmax_int&+W,&+w maxwis&+W|&+wmax_wis&+W,&+w"
				" maxcon&+W|&+wmax_con&+W,&+w maxagi&+W|&+wmax_agi&+W,&+w maxpow&+W|&+wmax_pow&+W,&+w"
				" maxcha&+W|&+wmax_cha&+W,&+w maxkarma&+W|&+wmax_kar&+W,&+w maxluck&+W|&+wmaxluk&+W|&+wmax_luk&+W,&+w"
				" racestr&+W,&+w racedex&+W,&+w raceint&+W,&+w racewis&+W,&+w racecon&+W,&+w raceagi&+W,&+w racepow&+W,&+w"
				" racecha&+W,&+w racekarma&+W,&+w raceluck&+W|&+wraceluk&+W,&+w curse&+W,&+w skillgrant&+W,&+w"
				" skilladd&+W,&+w hitreg&+W,&+w movereg&+W,&+w manareg&+W,&+w pulsecombat&+W|&+wcombatpulse&+W,&+w"
				" pulsespell&+W|&+wspellpulse&+W.&n\n",
				ch);

			snprintf(buf, MAX_STRING_LENGTH, "&=LWList options&n&+W:&+w ");
			lesser = FALSE;
			for (flags = 0; *(apply_types[flags]) != '\n'; flags++)
			{
				if (lesser)
				{
					strcat(buf, "&+W,&+w ");
				}
				else
				{
					lesser = TRUE;
				}
				strcat(buf, apply_types[flags]);
			}
			strcat(buf, "&+W.&n\n");
			send_to_char(buf, ch);
		}
		else
		{
			send_to_char("&+WFormat: &+wwhere stat <flag> <amount>&+W.&n\n", ch);
			send_to_char(
				"i.e. '&+wwhere stat maxint 5&n' for all items with exactly 5maxint.\n",
				ch);
			send_to_char(
				"Note: You can add a '+' or '-' immediately after <amount> for all"
				" items with <amount> or {greater|lesser} of <flag>.\n\r",
				ch);
			send_to_char(
				"i.e. '&+wwhere stat str 15+&n' for all items with 15 or more str.\n",
				ch);
			send_to_char(
				"i.e. '&+wwhere stat str -15-&n' for all items with -15 or less str.\n",
				ch);
			send_to_char(
				"This command lists each of objects in game that fall under the given options.\n",
				ch);
			send_to_char(
				"For a full list of options (omg), use '&+wwhere stat options&n'.\n",
				ch);
		}
		return;
	}
	if ((amount = atoi(arg2)) == 0)
	{
		send_to_char("The second argument must be a non-zero number.\n", ch);
		return;
	}

	if (is_abbrev(arg1, "strength"))
	{
		flags = APPLY_STR;
	}
	else if (is_abbrev(arg1, "dexterity"))
	{
		flags = APPLY_DEX;
	}
	else if (is_abbrev(arg1, "intelligence"))
	{
		flags = APPLY_INT;
	}
	else if (is_abbrev(arg1, "wisdom"))
	{
		flags = APPLY_WIS;
	}
	else if (is_abbrev(arg1, "constitution"))
	{
		flags = APPLY_CON;
	}
	else if (is_abbrev(arg1, "sex"))
	{
		flags = APPLY_SEX;
	}
	else if (is_abbrev(arg1, "class"))
	{
		flags = APPLY_CLASS;
	}
	else if (is_abbrev(arg1, "level"))
	{
		flags = APPLY_LEVEL;
	}
	else if (is_abbrev(arg1, "age"))
	{
		flags = APPLY_AGE;
	}
	else if (is_abbrev(arg1, "weight"))
	{
		flags = APPLY_CHAR_WEIGHT;
	}
	else if (is_abbrev(arg1, "height"))
	{
		flags = APPLY_CHAR_HEIGHT;
	}
	else if (is_abbrev(arg1, "mana"))
	{
		flags = APPLY_MANA;
	}
	else if (is_abbrev(arg1, "hitpoints"))
	{
		flags = APPLY_HIT;
	}
	else if (is_abbrev(arg1, "moves"))
	{
		flags = APPLY_MOVE;
	}
	else if (is_abbrev(arg1, "gold"))
	{
		flags = APPLY_GOLD;
	}
	else if (is_abbrev(arg1, "exp"))
	{
		flags = APPLY_EXP;
	}
	else if (is_abbrev(arg1, "armor") || is_abbrev(arg1, "ac"))
	{
		flags = APPLY_AC;
	}
	else if (is_abbrev(arg1, "hitroll"))
	{
		flags = APPLY_HITROLL;
	}
	else if (is_abbrev(arg1, "damroll"))
	{
		flags = APPLY_DAMROLL;
	}
	else if (is_abbrev(arg1, "para"))
	{
		flags = APPLY_SAVING_PARA;
	}
	else if (is_abbrev(arg1, "rod"))
	{
		flags = APPLY_SAVING_ROD;
	}
	else if (is_abbrev(arg1, "fear"))
	{
		flags = APPLY_SAVING_FEAR;
	}
	else if (is_abbrev(arg1, "breath"))
	{
		flags = APPLY_SAVING_BREATH;
	}
	else if (is_abbrev(arg1, "spell"))
	{
		flags = APPLY_SAVING_SPELL;
	}
	else if (is_abbrev(arg1, "pff") || is_abbrev(arg1, "fire"))
	{
		flags = APPLY_FIRE_PROT;
	}
	else if (is_abbrev(arg1, "agility"))
	{
		flags = APPLY_AGI;
	}
	else if (is_abbrev(arg1, "power"))
	{
		flags = APPLY_POW;
	}
	else if (is_abbrev(arg1, "charisma"))
	{
		flags = APPLY_CHA;
	}
	else if (is_abbrev(arg1, "karma"))
	{
		flags = APPLY_KARMA;
	}
	else if (is_abbrev(arg1, "luck"))
	{
		flags = APPLY_LUCK;
	}
	else if (is_abbrev(arg1, "maxstr") || is_abbrev(arg1, "max_str"))
	{
		flags = APPLY_STR_MAX;
	}
	else if (is_abbrev(arg1, "maxdex") || is_abbrev(arg1, "max_dex"))
	{
		flags = APPLY_DEX_MAX;
	}
	else if (is_abbrev(arg1, "maxint") || is_abbrev(arg1, "max_int"))
	{
		flags = APPLY_INT_MAX;
	}
	else if (is_abbrev(arg1, "maxwis") || is_abbrev(arg1, "max_wis"))
	{
		flags = APPLY_WIS_MAX;
	}
	else if (is_abbrev(arg1, "maxcon") || is_abbrev(arg1, "max_con"))
	{
		flags = APPLY_CON_MAX;
	}
	else if (is_abbrev(arg1, "maxagi") || is_abbrev(arg1, "max_agi"))
	{
		flags = APPLY_AGI_MAX;
	}
	else if (is_abbrev(arg1, "maxpow") || is_abbrev(arg1, "max_pow"))
	{
		flags = APPLY_POW_MAX;
	}
	else if (is_abbrev(arg1, "maxcha") || is_abbrev(arg1, "max_cha"))
	{
		flags = APPLY_CHA_MAX;
	}
	else if (is_abbrev(arg1, "maxkarma") || is_abbrev(arg1, "max_kar"))
	{
		flags = APPLY_KARMA_MAX;
	}
	else if (is_abbrev(arg1, "maxluck") || is_abbrev(arg1, "maxluk") ||
		 is_abbrev(arg1, "max_luk"))
	{
		flags = APPLY_LUCK_MAX;
	}
	else if (is_abbrev(arg1, "racestr"))
	{
		flags = APPLY_STR_RACE;
	}
	else if (is_abbrev(arg1, "racedex"))
	{
		flags = APPLY_DEX_RACE;
	}
	else if (is_abbrev(arg1, "raceint"))
	{
		flags = APPLY_INT_RACE;
	}
	else if (is_abbrev(arg1, "racewis"))
	{
		flags = APPLY_WIS_RACE;
	}
	else if (is_abbrev(arg1, "racecon"))
	{
		flags = APPLY_CON_RACE;
	}
	else if (is_abbrev(arg1, "raceagi"))
	{
		flags = APPLY_AGI_RACE;
	}
	else if (is_abbrev(arg1, "racepow"))
	{
		flags = APPLY_POW_RACE;
	}
	else if (is_abbrev(arg1, "racecha"))
	{
		flags = APPLY_CHA_RACE;
	}
	else if (is_abbrev(arg1, "racekarma"))
	{
		flags = APPLY_KARMA_RACE;
	}
	else if (is_abbrev(arg1, "raceluck") || is_abbrev(arg1, "raceluk"))
	{
		flags = APPLY_LUCK_RACE;
	}
	else if (is_abbrev(arg1, "curse"))
	{
		flags = APPLY_CURSE;
	}
	else if (is_abbrev(arg1, "skillgrant"))
	{
		flags = APPLY_SKILL_GRANT;
	}
	else if (is_abbrev(arg1, "skilladd"))
	{
		flags = APPLY_SKILL_ADD;
	}
	else if (is_abbrev(arg1, "hitreg"))
	{
		flags = APPLY_HIT_REG;
	}
	else if (is_abbrev(arg1, "movereg"))
	{
		flags = APPLY_MOVE_REG;
	}
	else if (is_abbrev(arg1, "manareg"))
	{
		flags = APPLY_MANA_REG;
	}
	else if (is_abbrev(arg1, "combatpulse") || is_abbrev(arg1, "pulsecombat"))
	{
		flags = APPLY_COMBAT_PULSE;
	}
	else if (is_abbrev(arg1, "spellpulse") || is_abbrev(arg1, "pulsespell"))
	{
		flags = APPLY_SPELL_PULSE;
	}
	else
	{
		for (flags = 0; *(apply_types[flags]) != '\n'; flags++)
		{
			if (is_abbrev(arg1, apply_types[flags]))
			{
				break;
			}
		}
		if (*(apply_types[flags]) == '\n')
		{
			send_to_char("Could not find flag.  :(\n", ch);
			return;
		}
	}

	if (arg2[strlen(arg2) - 1] == '+')
	{
		greater = TRUE;
		lesser = FALSE;
	}
	else
	{
		greater = FALSE;
		if (arg2[strlen(arg2) - 1] == '-')
		{
			lesser = TRUE;
		}
		else
		{
			lesser = FALSE;
		}
	}

	// Header for list: shows the flag and amount to ch.
	snprintf(buf, MAX_STRING_LENGTH,
		 "&=LWFlag: '%s', Amount: %d, Greater/Lesser: '%c'&n\n     &+W*&n=arti"
		 "\n&-L( AMT)&n  &-LINGAME&n   &-LVNUM&n &-LOBJ-SHORT&n\n",
		 apply_types[flags], amount, greater ? '+' : (lesser ? '-' : '='));
	send_to_char(buf, ch);

	if (greater)
	{
		// For each object in the game..
		for (obj = object_list; obj; obj = obj->next)
		{
			// Check flags for match.
			for (i = 0; i < MAX_OBJ_AFFECT; i++)
			{
				if (obj->affected[i].location == flags &&
				    obj->affected[i].modifier >= amount)
				{
					// As if things could be easier to read...
					snprintf(buf, MAX_STRING_LENGTH,
						 "&%s(%+4d)&+W%c&%s%3d/%3d %6d &n'%s'\n",
						 OBJ_COLOR(obj->R_num), obj->affected[i].modifier,
						 IS_ARTIFACT(obj) ? '*' : ' ',
						 OBJ_COLOR(obj->R_num),
						 obj_index[obj->R_num].number,
						 obj_index[obj->R_num].limit,
						 obj_index[obj->R_num].virtual_number,
						 obj->short_description);
					send_to_char(buf, ch);
				}
			}
		}
	}
	else if (lesser)
	{
		// For each object in the game..
		for (obj = object_list; obj; obj = obj->next)
		{
			// Check flags for match.
			for (i = 0; i < MAX_OBJ_AFFECT; i++)
			{
				if (obj->affected[i].location == flags &&
				    obj->affected[i].modifier <= amount)
				{
					// As if things could be easier to read...
					snprintf(buf, MAX_STRING_LENGTH,
						 "&%s(%+4d)&+W%c&%s%3d/%3d %6d &n'%s'\n",
						 OBJ_COLOR(obj->R_num), obj->affected[i].modifier,
						 IS_ARTIFACT(obj) ? '*' : ' ',
						 OBJ_COLOR(obj->R_num),
						 obj_index[obj->R_num].number,
						 obj_index[obj->R_num].limit,
						 obj_index[obj->R_num].virtual_number,
						 obj->short_description);
					send_to_char(buf, ch);
				}
			}
		}
	}
	else
	{
		// For each object in the game..
		for (obj = object_list; obj; obj = obj->next)
		{
			// Check flags for match.
			for (i = 0; i < MAX_OBJ_AFFECT; i++)
			{
				if (obj->affected[i].location == flags &&
				    obj->affected[i].modifier == amount)
				{
					// As if things could be easier to read...
					snprintf(buf, MAX_STRING_LENGTH,
						 "&%s(%+4d)&+W%c&%s%3d/%3d %6d &n'%s'\n",
						 OBJ_COLOR(obj->R_num), obj->affected[i].modifier,
						 IS_ARTIFACT(obj) ? '*' : ' ',
						 OBJ_COLOR(obj->R_num),
						 obj_index[obj->R_num].number,
						 obj_index[obj->R_num].limit,
						 obj_index[obj->R_num].virtual_number,
						 obj->short_description);
					send_to_char(buf, ch);
				}
			}
		}
	}
}

// Lists objects and characters that are not in the normal world locations.
// TODO: Subcategories: char and obj that just list chars or just objs.
static void where_nowhere(P_char ch, char * /*args*/)
{
	char buf[MAX_STRING_LENGTH];
	int count;
	P_obj obj;
	P_char t_ch;

	send_to_char("&-LObjects in NOWHERE:&n\n", ch);
	send_to_char_f(ch, "Nowhere: %d, Worn: %d, Carried: %d, Room: %d, Inside: %d.\n",
		       LOC_NOWHERE, LOC_WORN, LOC_CARRIED, LOC_ROOM, LOC_INSIDE);
	// For each object in the game..
	for (count = 0, obj = object_list; obj; obj = obj->next)
	{
		// If it's in nowhere, in room nowhere (or out of bounds room vnum), or on NULL char or in a NULL obj
		if (OBJ_NOWHERE(obj) || (OBJ_ROOM(obj) && (ROOM_VNUM(obj->loc.room) == NOWHERE)) ||
		    (OBJ_WORN(obj) && (obj->loc.wearing == NULL)) ||
		    (OBJ_CARRIED(obj) && (obj->loc.carrying == NULL)) ||
		    (OBJ_INSIDE(obj) && (obj->loc.inside == NULL)))
		{
			snprintf(buf, MAX_STRING_LENGTH, "%3d) %d %6d %s&n\n", ++count, obj->loc_p,
				 OBJ_VNUM(obj), OBJ_SHORT(obj));
			send_to_char(buf, ch);
		}
	}
	if (count > 0)
		send_to_char("&+yTo collect these objects, type '&+wget nowhere&+y'.&n\n", ch);

	send_to_char("\n&-LCharacters in NOWHERE:&n\n", ch);
	// For each character in the game..
	for (t_ch = character_list; t_ch; t_ch = t_ch->next)
	{
		if (t_ch->in_room == NOWHERE)
		{
			snprintf(buf, MAX_STRING_LENGTH, "%3d)%6d %s&n\n", ++count, GET_ID(t_ch),
				 J_NAME(t_ch));
			send_to_char(buf, ch);
		}
	}
}

// Sort the displayed players by room vnum for the where command.
struct line_info
{
	char line[512]; // Shouldn't be more than 512 chars per line.
	int room_number;
};

static int where_compare(const void *line1, const void *line2)
{
	return ((line_info *)line1)->room_number > ((line_info *)line2)->room_number;
}

// Looks up players, mobiles, and objects in the current world.
// This is an immortal-only command, as registered in interp.c.
void do_where(P_char ch, char *argument, int /*cmd*/)
{
	char buf[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];
	char *args;
	int length = 0, count = 0, o_count = 0, v_num;
	bool flag;
	P_char i;
	P_obj k;
	P_desc d;
	P_char t_ch;

	int line_count;
	struct line_info lines[MAX_CONNECTIONS];

	buf[0] = 0;
	length = 0;
	line_count = 0;
	flag = FALSE;

	while (*argument == ' ')
	{
		argument++;
	}

	if (!strcmp(argument, "?"))
	{
		send_to_char(
			"&+YValid arguments: &+Wevils, goods, undeads, neutrals, <vnum>, zone, trap, stat, nowhere.\n\r",
			ch);
		return;
	}

	if (!*argument)
	{
		strcpy(buf, "Players:\n--------\n");

		for (d = descriptor_list; d; d = d->next)
		{
			t_ch = d->character;
			if ((!t_ch) && d->original)
				t_ch = d->original;

			if (t_ch && IS_PC(t_ch) && (d->connected == CON_PLAYING) &&
			    (d->character->in_room != NOWHERE) && CAN_SEE(ch, t_ch))
			{
				if (d->original) /* If switched */
				{
					snprintf(
						lines[line_count].line,
						sizeof(lines[line_count].line),
						"&+%c%-20s &+Y- &n[&+R%4d&+W:&+C%6d&n] %s &n(In body of %s&n)\n",
						IS_TRUSTED(t_ch) ?
							'w' :
							racewar_color[GET_RACEWAR(t_ch)].color,
						t_ch->player.name,
						ROOM_ZONE_NUMBER(d->character->in_room),
						world[d->character->in_room].number,
						world[d->character->in_room].name,
						FirstWord(d->character->player.name));
					lines[line_count].room_number =
						ROOM_VNUM(d->character->in_room);
				}
				else
				{
					snprintf(lines[line_count].line,
						 sizeof(lines[line_count].line),
						 "&+%c%-20s - &n[&+R%4d&+W:&+C%6d&n] %s&n\n",
						 IS_TRUSTED(t_ch) ?
							 'w' :
							 racewar_color[GET_RACEWAR(t_ch)].color,
						 t_ch->player.name,
						 ROOM_ZONE_NUMBER(d->character->in_room),
						 world[d->character->in_room].number,
						 world[d->character->in_room].name);
					lines[line_count].room_number =
						ROOM_VNUM(d->character->in_room);
				}
				// We increment here because we want to increment before the break.
				if (strlen(lines[line_count++].line) + length + 512 >
				    MAX_STRING_LENGTH)
				{
					snprintf(lines[line_count].line,
						 sizeof(lines[line_count].line),
						 "   ...the list is too long...\n");
					// Max zone number is 9999999.
					lines[line_count++].room_number = 10000000;
					break;
				}
				// We use -1 here, because we already incremented.
				length += strlen(lines[line_count - 1].line);
			}
		}
		// Sort
		qsort(lines, line_count, sizeof(line_info), where_compare);
		// Send.
		for (int i = 0; i < line_count; i++)
		{
			send_to_char(lines[i].line, ch);
		}
		return;
	}

	if (isname(argument, "evils") || isname(argument, "goods") || isname(argument, "undeads") ||
	    isname(argument, "neutrals"))
	{
		int racewar = 0;

		if (isname(argument, "goods"))
		{
			racewar = RACEWAR_GOOD;
			strcpy(buf, "Players (goods):\n--------------\n");
		}
		else if (isname(argument, "evils"))
		{
			racewar = RACEWAR_EVIL;
			strcpy(buf, "Players (evils):\n--------------\n");
		}
		else if (isname(argument, "undeads"))
		{
			racewar = RACEWAR_UNDEAD;
			strcpy(buf, "Players (undeads):\n--------------\n");
		}
		else if (isname(argument, "neutrals"))
		{
			racewar = RACEWAR_NEUTRAL;
			strcpy(buf, "Players (neutrals):\n--------------\n");
		}
		else
		{
			racewar = RACEWAR_NONE;
			strcpy(buf, "Players (&+Rbuggy&n):\n--------------\n");
		}

		for (d = descriptor_list; d; d = d->next)
		{
			t_ch = d->character;
			if ((!t_ch) && d->original)
			{
				t_ch = d->original;
			}

			if (t_ch && IS_PC(t_ch) && (d->connected == CON_PLAYING) &&
			    (d->character->in_room != NOWHERE) && CAN_SEE(ch, t_ch) &&
			    t_ch->player.racewar == racewar)
			{
				if (d->original) /* If switched */
				{
					snprintf(
						lines[line_count].line,
						sizeof(lines[line_count].line),
						"&+%c%-20s &+Y- &n[&+R%4d&+W:&+C%6d&n] %s &n(In body of %s&n)\n",
						IS_TRUSTED(t_ch) ?
							'w' :
							racewar_color[GET_RACEWAR(t_ch)].color,
						t_ch->player.name,
						ROOM_ZONE_NUMBER(d->character->in_room),
						world[d->character->in_room].number,
						world[d->character->in_room].name,
						FirstWord(d->character->player.name));
					lines[line_count].room_number =
						ROOM_VNUM(d->character->in_room);
				}
				else
				{
					snprintf(lines[line_count].line,
						 sizeof(lines[line_count].line),
						 "&+%c%-20s - &n[&+R%4d&+W:&+C%6d&n] %s&n\n",
						 IS_TRUSTED(t_ch) ?
							 'w' :
							 racewar_color[GET_RACEWAR(t_ch)].color,
						 t_ch->player.name,
						 ROOM_ZONE_NUMBER(d->character->in_room),
						 world[d->character->in_room].number,
						 world[d->character->in_room].name);
					lines[line_count].room_number =
						ROOM_VNUM(d->character->in_room);
				}
				// We increment here because we want to increment before the break.
				if (strlen(lines[line_count++].line) + length + 512 >
				    MAX_STRING_LENGTH)
				{
					snprintf(lines[line_count].line,
						 sizeof(lines[line_count].line),
						 "   ...the list is too long...\n");
					// Max zone number is 9999999.
					lines[line_count++].room_number = 10000000;
					break;
				}
				// We use -1 here, because we already incremented.
				length += strlen(lines[line_count - 1].line);
			}
		}
		// Sort
		qsort(lines, line_count, sizeof(line_info), where_compare);
		// Send.
		for (int i = 0; i < line_count; i++)
		{
			send_to_char(lines[i].line, ch);
		}
		return;
	}

	/*
	 * This chunk of code allows "where <v-number>" if the argument is a
	 * number.  It will return all mobs/objects with that v-number.
	 * Otherwise it defaults to treating the argument as a string.
	 */
	if (is_number(argument))
	{
		v_num = atoi(argument);

		/* mobs */
		for (i = character_list; i && !flag; i = i->next)
		{
			if (IS_NPC(i) && CAN_SEE(ch, i) && (v_num == GET_VNUM(i)))
			{
				if ((i->in_room != NOWHERE) &&
				    (IS_TRUSTED(ch) ||
				     (world[i->in_room].zone == world[ch->in_room].zone)))
				{
					count++;
					snprintf(buf2, MAX_STRING_LENGTH,
						 "%3d. [%6d] %s &+Y- &n[&+R%4d&+W:&+C%6d&n] %s&n\n",
						 count, v_num,
						 pad_ansi(i->player.short_descr, 40).c_str(),
						 ROOM_ZONE_NUMBER(i->in_room),
						 world[i->in_room].number, world[i->in_room].name);
					if ((length + strlen(buf2) + 35) > MAX_STRING_LENGTH)
					{
						strcpy(buf2, "   ...the list is too long...\n");
						flag = TRUE;
					}
					strcat(buf, buf2);
					length += strlen(buf2);
				}
			}
		}

		if (count && !flag)
		{
			strcat(buf, "\n\n"); /* extra lines between mobs/objs */
		}

		/* objects */
		for (k = object_list; k && !flag; k = k->next)
		{
			if (v_num == OBJ_VNUM(k))
			{
				// wizinvis checks
				P_obj tobj = k;
				if ((k->affected[0].location == APPLY_LEVEL ||
				     k->affected[1].location == APPLY_LEVEL) &&
				    GET_LEVEL(ch) <= 59)
				{
					continue;
				}
				while (OBJ_INSIDE(tobj))
				{
					tobj = tobj->loc.inside;
				}
				if ((OBJ_WORN(tobj) && WIZ_INVIS(ch, tobj->loc.wearing)) ||
				    (OBJ_CARRIED(tobj) && WIZ_INVIS(ch, tobj->loc.carrying)))
				{
					continue;
				}

				o_count++;
				count++;

				snprintf(buf2, MAX_STRING_LENGTH, "%3d. [%6d] %s &+Y-&n %s\n",
					 o_count, v_num, pad_ansi(k->short_description, 40).c_str(),
					 where_obj(k, FALSE));

				if ((strlen(buf2) + length + 35) > MAX_STRING_LENGTH)
				{
					strcpy(buf2, "   ...the list is too long...\n");
					flag = TRUE;
				}
				strcat(buf, buf2);
				length += strlen(buf2);
			}
		}
		if (!count)
		{
			send_to_char("Nothing found.\n", ch);
		}
		else
		{
			page_string(ch->desc, buf, 1);
		}
		return;
	}
	/*
	 * "where zone" -- added by DTS 7/6/95
	 */
	if (is_abbrev(argument, "zone"))
	{
		strcpy(buf, "Players in this zone:\n---------------------\n");

		for (d = descriptor_list; d; d = d->next)
		{
			if (d->character && IS_PC(d->character) && (d->connected == CON_PLAYING) &&
			    (d->character->in_room != NOWHERE) && CAN_SEE(ch, d->character))
			{
				if (world[d->character->in_room].zone == world[ch->in_room].zone)
				{
					if (d->original)
					{
						snprintf(
							buf2, MAX_STRING_LENGTH,
							"%-20s &+Y- &n[&+C%6d&n] &n%s (In body of %s&n)\n",
							d->original->player.name,
							world[d->character->in_room].number,
							world[d->character->in_room].name,
							FirstWord(d->character->player.name));
					}
					else
					{
						snprintf(buf2, MAX_STRING_LENGTH,
							 "%-20s &+Y- &n[&+C%6d&n] %s&n\n",
							 d->character->player.name,
							 world[d->character->in_room].number,
							 world[d->character->in_room].name);
					}
					if (strlen(buf2) + length + 35 > MAX_STRING_LENGTH)
					{
						snprintf(buf2, MAX_STRING_LENGTH,
							 "   ...the list is too long...\n");
						strcat(buf, buf2);
					}
					strcat(buf, buf2);
					length = strlen(buf);
				}
			}
		}
		page_string(ch->desc, buf, 1);
		return;
	}
	else if (is_abbrev(argument, "trap"))
		do_traplist(ch, argument, 0);

	args = one_argument(argument, buf);
	if (is_abbrev(buf, "stat"))
	{
		where_stat(ch, args);
		return;
	}

	if (is_abbrev(buf, "nowhere"))
	{
		where_nowhere(ch, args);
		return;
	}

	/* mobs */
	buf[0] = '\0';
	for (i = character_list; i && !flag; i = i->next)
	{
		if ((isname(argument, GET_NAME(i)) ||
		     (IS_NPC(i) && isname(argument, i->player.short_descr))) &&
		    CAN_SEE(ch, i))
		{
			if ((i->in_room != NOWHERE) &&
			    (IS_TRUSTED(ch) || (world[i->in_room].zone == world[ch->in_room].zone)))
			{
				count++;
				if (IS_NPC(i))
					snprintf(buf2, MAX_STRING_LENGTH,
						 "%3d. [%6d] %s &+Y- &n[&+R%4d&+W:&+C%6d&n] %s&n ",
						 count, GET_VNUM(i),
						 pad_ansi(i->player.short_descr, 40).c_str(),
						 ROOM_ZONE_NUMBER(i->in_room),
						 world[i->in_room].number, world[i->in_room].name);
				else
					snprintf(buf2, MAX_STRING_LENGTH,
						 "%3d. %s &+Y- &n[&+R%4d&+W:&+C%6d&n] %s&n ", count,
						 pad_ansi(i->player.name, 40).c_str(),
						 ROOM_ZONE_NUMBER(i->in_room),
						 world[i->in_room].number, world[i->in_room].name);

				strcat(buf2, "\n");

				if ((length + strlen(buf2) + 35) > MAX_STRING_LENGTH)
				{
					strcpy(buf2, "   ...the list is too long...\n");
					flag = TRUE;
				}
				strcat(buf, buf2);
				length += strlen(buf2);

				if (!IS_TRUSTED(ch))
					flag = TRUE;
			}
		}
	}

	if (count && !flag)
		strcat(buf, "\n\n"); /*
		                      * extra lines between mobs/objs
		                      */

	for (k = object_list; k && !flag; k = k->next)
	{
		if ((isname(argument, k->name) || isname(argument, k->short_description)) &&
		    CAN_SEE_OBJ(ch, k))
		{
			// wizinvis checks
			P_obj tobj = k;
			while (OBJ_INSIDE(tobj))
				tobj = tobj->loc.inside;
			if (OBJ_WORN(tobj) && WIZ_INVIS(ch, tobj->loc.wearing))
				continue;
			if (OBJ_CARRIED(tobj) && WIZ_INVIS(ch, tobj->loc.carrying))
				continue;
			if ((k->affected[0].location == APPLY_LEVEL ||
			     k->affected[1].location == APPLY_LEVEL) &&
			    GET_LEVEL(ch) <= 60)
				continue;

			o_count++;
			count++;
			snprintf(buf2, MAX_STRING_LENGTH, "%3d. [%6d] %s &+Y- &n%s\n", o_count,
				 OBJ_VNUM(k), pad_ansi(k->short_description, 40).c_str(),
				 where_obj(k, FALSE));
			if ((strlen(buf2) + length + 35) > MAX_STRING_LENGTH)
			{
				strcpy(buf2, "   ...the list is too long...\n");
				flag = TRUE;
			}
			strcat(buf, buf2);
			length += strlen(buf2);
		}
	}

	if (!count)
		send_to_char("Nothing found.\n", ch);
	else
		page_string(ch->desc, buf, 1);
}

#undef OBJ_COLOR

void do_questwhere(P_char ch, char *arg, int /*cmd*/)
{
	P_obj obj;
	char buf[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];
	int count = 0, length = 0;
	bool stop = FALSE;

	if (!ch)
		return;

	while (*arg == ' ')
		arg++;
	if (!(*arg))
	{
		send_to_char("The questwhere command requires an argument.\n", ch);
		return;
	}

	buf[0] = '\0';

	for (obj = object_list; obj && !stop; obj = obj->next)
	{
		if (isname(arg, obj->name) && is_quested_item(obj))
		{
			snprintf(buf2, MAX_STRING_LENGTH, "%3d. [%6d] %s - ", ++count,
				 obj_index[obj->R_num].virtual_number,
				 pad_ansi(obj->short_description, 40).c_str());
			strcat(buf2, where_obj(obj, FALSE));
			strcat(buf2, "\n");
			if (strlen(buf2) + length + 35 > MAX_STRING_LENGTH)
			{
				strcpy(buf2, " ... The list is too long.\n");
				stop = TRUE;
			}
			strcat(buf, buf2);
			length += strlen(buf2);
		}
	}
	if (!count)
		send_to_char("No items found.\n", ch);
	else
		page_string(ch->desc, buf, 1);
}

bool is_quested_item(P_obj obj)
{
	struct quest_complete_data *qcp;
	struct goal_data *gp;
	int vnum = obj_index[obj->R_num].virtual_number;
	int count = 0;

	// For each quest on the mud.
	for (qcp = quest_index[0].quest_complete; count < number_of_quests;
	     qcp = quest_index[++count].quest_complete)
	{
		// If there is quest completion data, look through it
		if (qcp)
			for (gp = qcp->receive; gp; gp = gp->next)
			{
				// If the item received from the quest has the right vnum
				if (gp->goal_type == QUEST_GOAL_ITEM)
				{
					if (gp->number == vnum)
						return TRUE;
				}
			}
	}
	return FALSE;
}
