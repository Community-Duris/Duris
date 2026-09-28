/*
 * Staff catalogue queries for rooms, zones, mobs, objects, races, and specs.
 */

#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utility.h"
#include "core/utils.h"
#include "core/safe_format.h"
#include "net/comm.h"
#include "world/db.h"
#include "world/specs.prototypes.h"
#include "item/objmisc.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern P_char character_list;
extern P_index mob_index;
extern P_index obj_index;
extern P_obj object_list;
extern P_room world;
extern int top_of_mobt;
extern int top_of_objt;
extern const int top_of_world;
extern int top_of_zone_table;
extern struct zone_data *zone_table;
extern flagDef weapon_types[];
extern const char *specdata[][MAX_SPEC];
extern const char *apply_types[];
extern const char *player_bits[];
extern const flagDef action_bits[];
extern const flagDef room_bits[];
extern const char *zone_bits[];
extern const flagDef wear_bits[];
extern const flagDef extra_bits[];
extern const flagDef extra2_bits[];
extern const flagDef affected1_bits[];
extern const flagDef affected2_bits[];
extern const flagDef affected3_bits[];
extern const flagDef affected4_bits[];
extern const flagDef affected5_bits[];
extern struct race_names race_names_table[];

#define OBJ_COLOR(rnum)                                                 \
	((obj_index[rnum].number <= 1)			       ? "+L" : \
	 (obj_index[rnum].number - 1 > obj_index[rnum].limit)  ? "+W" : \
	 (obj_index[rnum].number - 1 == obj_index[rnum].limit) ? "n" :  \
								 "+w")
#define WHICH_SYNTAX                                                                                            \
	"&+WSyntax:&n\n&+w   which room <zone flag>\n&+w   which zone <zone flag>\n"                            \
	"&+w   which char|mob <mobact flag>\n&+w   which race <race name|race number>\n"                        \
	"&+w   which obj|item <wear, extra(2), anti(2) or aff(2-6) flag>\n&+w   which stat <flag> <amount>&n\n" \
	"&+w   which spec <flag> <amount>&n\n"

static void which_race(P_char ch, char *argument);
static void which_stat(P_char ch, char *argument);
static void which_food(P_char ch, char *argument);
static void which_spec(P_char ch, char *argument);
static void which_armor(P_char ch, char *argument);
static void which_weapon(P_char ch, char *argument);
extern int race_lookup(char *raceStr);

static bool check_flagsde(int *value, const flagDef flagNames[], const char *flagName)
{
	int i;

	for (i = 0; (flagNames[i].flagShort != NULL) && str_cmp(flagNames[i].flagShort, flagName);
	     i++)
		;

	if (flagNames[i].flagShort == NULL)
		return FALSE;

	*value = i;

	return TRUE;
}
static bool check_apply(int *value, const char *flagName)
{
	int i;

	for (i = 0; apply_types[i] != NULL && apply_types[i][0] != '\n' &&
		    str_cmp(apply_types[i], flagName);
	     i++)
		;

	if (apply_types[i] == NULL || apply_types[i][0] == '\n')
		return FALSE;

	*value = i;

	return TRUE;
}
// Lists all mob/object types that match the search string.
void do_which(P_char ch, char *args, int /*cmd*/)
{
	typedef enum _whichObjFlagsEnum
	{
		wear = 0,
		extra,
		extra2,
		anti,
		anti2,
		aff,
		aff2,
		aff3,
		aff4,
		aff5,
		apply
	} whichObjFlagsEnum;

	P_char t_ch = NULL;
	P_obj t_obj = NULL;
	char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH], *rest;
	char arg3[MAX_INPUT_LENGTH], arg4[MAX_INPUT_LENGTH];
	int sc_min, sc_max = 0;
	char o_buf[MAX_STRING_LENGTH], buf1[MAX_STRING_LENGTH];
	int i, j = 0, t, room_nr, zone_nr, o_len, found;
	bool stat_check = FALSE;
	whichObjFlagsEnum whichObjFlags;
	uint w_bit = 0;

	// setbit_parseArgument is the same as one_argument(), but allows 'two word' args.
	// rest and args = the rest of the arguments after arg1 without any leading spaces.
	rest = args = skip_spaces(setbit_parseArgument(args, arg1));
	// Here skip_spaces isn't necessary.
	args = setbit_parseArgument(args, arg2);
	args = one_argument(args, arg3);
	one_argument(args, arg4);
	sc_min = atoi(arg3);
	sc_max = atoi(arg4);

	if (sc_min)
		stat_check = TRUE;

	if (!*arg1 || (*arg1 != 'f' && *arg1 != 'F' && !*arg2))
	{
		send_to_char(WHICH_SYNTAX, ch);
		return;
	}
	o_buf[0] = '\0'; /* output string, so we can page it */
	o_len = 50; /* saves us a bunch of strlen() calls */
	buf1[0] = '\0';

	if ((*arg1 == 'r') || (*arg1 == 'R'))
	{
		if (LOWER(arg1[1]) == 'a')
		{
			which_race(ch, arg2);
			return;
		}
		/* room and zone flags are the easiest, just run down the list  */
		for (i = 0; room_bits[i].flagShort && str_cmp(room_bits[i].flagShort, arg2); i++)
		{
			;
		}
		if (!room_bits[i].flagShort)
		{
			send_to_char("Unknown flag, valid options are:\n", ch);
			buf1[0] = '\0'; // Initialize buf1 before use
			for (j = 0; room_bits[j].flagShort; j++)
			{
				if (j && !(j % 3))
					strcat(buf1, "\n");

				APPENDF(buf1, "%-20s", room_bits[j].flagShort);
			}
			strcat(buf1, "\n");
			send_to_char(buf1, ch);
			return;
		}
		w_bit = 1 << i;
		for (room_nr = 0; room_nr < top_of_world; room_nr++)
		{
			if (IS_ROOM(room_nr, w_bit))
			{
				snprintf(buf1, MAX_STRING_LENGTH, "&+Y[&n%5d&+Y](&n%5d&+Y)&n %s\n",
					 world[room_nr].number, room_nr, world[room_nr].name);
				o_len += strlen(buf1);
				if (o_len > MAX_STRING_LENGTH)
				{
					strcat(o_buf, "And so on, and so forth...\n");
					break;
				}
				else
				{
					strcat(o_buf, buf1);
				}
			}
		}
	}
	else if ((*arg1 == 'z') || (*arg1 == 'Z'))
	{
		for (i = 0; str_cmp(zone_bits[i], arg2) && (zone_bits[i][0] != '\n'); i++)
			;
		if (zone_bits[i][0] == '\n')
		{
			send_to_char("Unknown flag, valid options are:\n", ch);
			buf1[0] = '\0'; // Initialize buf1 before use
			for (j = 0; zone_bits[j][0] != '\n'; j++)
			{
				if (j && !(j % 3))
					strcat(buf1, "\n");

				APPENDF(buf1, "%-20s", zone_bits[j]);
			}
			strcat(buf1, "\n");
			send_to_char(buf1, ch);
			return;
		}
		w_bit = 1 << i;
		for (zone_nr = 0; zone_nr <= top_of_zone_table; zone_nr++)
			if (zone_table[zone_nr].flags & w_bit)
			{
				snprintf(buf1, MAX_STRING_LENGTH, "&+Y[&n%3d&+Y]&n %s\n", zone_nr,
					 zone_table[zone_nr].name);
				o_len += strlen(buf1);
				if (o_len > MAX_STRING_LENGTH)
				{
					strcat(o_buf, "And so on, and so forth...\n");
					break;
				}
				else
				{
					strcat(o_buf, buf1);
				}
			}
	}
	else if ((*arg1 == 'o') || (*arg1 == 'O') || (*arg1 == 'i') || (*arg1 == 'I'))
	{
		/*
		 * obj flags are slightly different, because we have to check for
		 * many flags
		 */

		if (check_flagsde(&i, wear_bits, arg2))
			whichObjFlags = wear;
		else if (check_flagsde(&i, extra_bits, arg2))
			whichObjFlags = extra;
		else if (check_flagsde(&i, extra2_bits, arg2))
			whichObjFlags = extra2;
		/*
		   else if(check_flagsde(&i, anti_bits, arg2))
		   whichObjFlags = anti;
		   else if(check_flagsde(&i, anti2_bits, arg2))
		   whichObjFlags = anti2; */
		else if (check_flagsde(&i, affected1_bits, arg2))
			whichObjFlags = aff;
		else if (check_flagsde(&i, affected2_bits, arg2))
			whichObjFlags = aff2;
		else if (check_flagsde(&i, affected3_bits, arg2))
			whichObjFlags = aff3;
		else if (check_flagsde(&i, affected4_bits, arg2))
			whichObjFlags = aff4;
		else if (check_flagsde(&i, affected5_bits, arg2))
			whichObjFlags = aff5;
		else if (check_apply(&i, arg2))
			whichObjFlags = apply;
		else
		{
			// assume we won't go over max_string_length?  let's see what happens :)

			strcpy(buf1, "Unknown flag, valid options are:\n\n");

			concat_which_flagsde("Wear", wear_bits, buf1);
			concat_which_flagsde("Extra", extra_bits, buf1);
			concat_which_flagsde("Extra2", extra2_bits, buf1);
			//      concat_which_flagsde("Anti", anti_bits, buf1);
			//      concat_which_flagsde("Anti2", anti2_bits, buf1);
			concat_which_flagsde("Aff1", affected1_bits, buf1);
			concat_which_flagsde("Aff2", affected2_bits, buf1);
			concat_which_flagsde("Aff3", affected3_bits, buf1);
			concat_which_flagsde("Aff4", affected4_bits, buf1);
			concat_which_flagsde("Aff5", affected5_bits, buf1);
			// add applies to objects here..

			page_string(ch->desc, buf1, 1);
			return;
		}
		w_bit = 1 << i;
		for (t_obj = object_list; t_obj; t_obj = t_obj->next)
		{
			found = 0;

			if (IS_NOSHOW(t_obj))
				continue;

			switch (whichObjFlags)
			{
			case wear:
				found = (t_obj->wear_flags & w_bit);
				break;

			case extra:
				found = (t_obj->extra_flags & w_bit);
				break;

			case extra2:
				found = (t_obj->extra2_flags & w_bit);
				break;

			case anti:
				found = (t_obj->anti_flags & w_bit);
				break;

			case anti2:
				found = (t_obj->anti2_flags & w_bit);
				break;

			case aff:
				found = (t_obj->bitvector & w_bit);
				break;

			case aff2:
				found = (t_obj->bitvector2 & w_bit);
				break;

			case aff3:
				found = (t_obj->bitvector3 & w_bit);
				break;

			case aff4:
				found = (t_obj->bitvector4 & w_bit);
				break;

			case aff5:
				found = (t_obj->bitvector5 & w_bit);
				break;

			case apply:
				for (j = 0; j < 3; j++)
				{
					if ((found = (t_obj->affected[j].location == i)))
						break;
				}
				break;
			default:
				found = FALSE; // shrug
			}

			if (found && !stat_check)
			{
				snprintf(buf1, MAX_STRING_LENGTH, "[%5d] %-30s - %s\n",
					 obj_index[t_obj->R_num].virtual_number,
					 t_obj->short_description, where_obj(t_obj, FALSE));

				o_len += strlen(buf1);
				if (o_len > MAX_STRING_LENGTH)
				{
					strcat(o_buf, "And so on, and so forth...\n");
					break;
				}
				else
				{
					strcat(o_buf, buf1);
				}
			}
			else if (found && stat_check && whichObjFlags == apply)
			{
				if (sc_min <= t_obj->affected[j].modifier &&
				    sc_max >= t_obj->affected[j].modifier)
				{
					char temp[MAX_STRING_LENGTH];
					char temp2[MAX_STRING_LENGTH];
					temp[0] = '\0'; // Initialize temp buffer before use
					for (t = 0; t < MAX_OBJ_AFFECT; t++)
					{
						if (t_obj->affected[t].location != APPLY_NONE)
						{
							sprinttype(t_obj->affected[t].location,
								   apply_types, temp2);
							checked_snprintf(
								temp + strlen(temp),
								MAX_STRING_LENGTH - strlen(temp),
								"   &+YAffects: &+c%s&+Y By &+c%d\n",
								temp2, t_obj->affected[t].modifier);
						}
					}
					checked_snprintf(buf1, MAX_STRING_LENGTH,
							 "[%5d] %-30s - %s\n%s\n",
							 obj_index[t_obj->R_num].virtual_number,
							 t_obj->short_description,
							 where_obj(t_obj, FALSE), temp);
					o_len += strlen(buf1);
					if (o_len > MAX_STRING_LENGTH)
					{
						strcat(o_buf, "And so on, and so forth...\n");
						break;
					}
					else
					{
						strcat(o_buf, buf1);
					}
				}
			}
		}
	}
	else if ((*arg1 == 'c') || (*arg1 == 'C') || (*arg1 == 'm') || (*arg1 == 'M'))
	{
		/*
		 * char flags are slightly different, because we have to check for
		 * pcact, or npcact
		 */

		int which = 0;
		for (i = 0; action_bits[i].flagShort && str_cmp(action_bits[i].flagShort, arg2);
		     i++)
			;
		if (!action_bits[i].flagShort)
		{
			which = 1;
			for (i = 0; str_cmp(player_bits[i], arg2) && (player_bits[i][0] != '\n');
			     i++)
				;
			if (player_bits[i][0] == '\n')
			{
				send_to_char("Unknown flag, valid options are:\n", ch);
				buf1[0] = '\0'; // Initialize buf1 before use
				for (j = 0; action_bits[j].flagShort; j++)
				{
					if (j && !(j % 3))
						strcat(buf1, "\n");

					APPENDF(buf1, "%-20s", action_bits[j].flagShort);
				}
				strcat(buf1, "\n");

				for (j = 0; player_bits[j][0] != '\n'; j++)
				{
					if (j && !(j % 3))
						strcat(buf1, "\n");

					APPENDF(buf1, "%-20s", player_bits[j]);
				}
				strcat(buf1, "\n");

				send_to_char(buf1, ch);
				return;
			}
		}
		w_bit = 1 << i;
		for (t_ch = character_list; t_ch; t_ch = t_ch->next)
		{
			if (!CAN_SEE(ch, t_ch))
				continue;
			found = 0;
			if (which)
			{
				if (IS_PC(t_ch) && (t_ch->specials.act & w_bit))
					found = TRUE;
			}
			else
			{
				if (IS_NPC(t_ch) && (t_ch->specials.act & w_bit))
					found = TRUE;
			}

			if (found)
			{
				if (IS_NPC(t_ch))
					snprintf(buf1, MAX_STRING_LENGTH,
						 "%-30s- &+Y[&n%5d&+Y]&n %s\n",
						 t_ch->player.short_descr,
						 world[t_ch->in_room].number,
						 world[t_ch->in_room].name);
				else
					snprintf(buf1, MAX_STRING_LENGTH,
						 "%-30s- &+Y[&n%5d&+Y]&n %s\n", t_ch->player.name,
						 world[t_ch->in_room].number,
						 world[t_ch->in_room].name);
				o_len += strlen(buf1);
				if (o_len > MAX_STRING_LENGTH)
				{
					strcat(o_buf, "And so on, and so forth...\n");
					break;
				}
				else
				{
					strcat(o_buf, buf1);
				}
			}
		}
	}
	else if ((*arg1 == 's') || (*arg1 == 'S'))
	{
		if (is_abbrev(arg1, "spec"))
			which_spec(ch, rest);
		else if (is_abbrev(arg1, "stat"))
			which_stat(ch, rest);
		else
			send_to_char(WHICH_SYNTAX, ch);
		return;
	}
	else if ((*arg1 == 'f') || (*arg1 == 'F'))
	{
		which_food(ch, rest);
		return;
	}
	else if (is_abbrev(arg1, "weapon"))
	{
		which_weapon(ch, rest);
		return;
	}
	else if (is_abbrev(arg1, "armor"))
	{
		which_armor(ch, rest);
		return;
	}
	if (!*o_buf)
		send_to_char("No matches.\n", ch);
	else
		page_string(ch->desc, o_buf, 1);
}
// Displays each type of mob with race corresponding to argument.
static void which_race(P_char ch, char *argument)
{
	char arg[MAX_STRING_LENGTH];
	char buf[MAX_STRING_LENGTH];
	char oBuf[MAX_STRING_LENGTH];
	int mobRace, mobVnum, mobZone, raceIndex, count, i, j, oBufLength;
	P_char mob;
	P_index mobIndex;

	one_argument(argument, arg);
	raceIndex = -1;
	oBufLength = 0;
	oBuf[0] = '\0';

	// No argument or ? -> display format.
	if (arg[0] == '\0' || arg[0] == '?')
	{
		send_to_char("Format: which race < race name | race number >\n", ch);
		send_to_char("i.e. 'which race < human | 1 >' for all mob types which are human.\n",
			     ch);
		return;
	}

	raceIndex = race_lookup(arg);

	// If we couldn't identify the race to look for.. (allowing RACE_NONE since this is a Imm command).
	if (raceIndex < 0 || raceIndex > LAST_RACE)
	{
		checked_snprintf(
			buf, MAX_STRING_LENGTH,
			"Race '%s' not found.  Please enter a number between 0 and %d or a valid race name.\n\r",
			arg, LAST_RACE);
		send_to_char(buf, ch);
		return;
	}

	snprintf(buf, MAX_STRING_LENGTH, "Race: %s (%d) listing: \n\r",
		 race_names_table[raceIndex].ansi, raceIndex);
	send_to_char(buf, ch);

	count = 0;
	// Walk through the mob_index table...
	for (i = 0; i < top_of_mobt; i++)
	{
		// If we don't have a valid mob index, or an incomplete one.
		if ((mobIndex = &(mob_index[i])) == NULL || mobIndex->virtual_number == 0)
		{
			debug("which_race: Mob index %d %s.", i,
			      (mobIndex == NULL) ? "is NULL" : "has virtual number 0.");
			continue;
		}
		// If we fail to load an instance of the mob.
		if ((mob = read_mobile(mobIndex->virtual_number, VIRTUAL)) == NULL)
		{
			continue;
		}
		mobRace = GET_RACE(mob);
		mobVnum = GET_VNUM(mob);
		// If we have a match, create a line.
		if (mobRace == raceIndex)
		{
			mobZone = 0;
			// Walk through the list of zones.
			for (j = 1; j <= top_of_zone_table; j++)
			{
				// When we reach the first zone where the vnum should be in the zone before it,
				if (mobVnum < zone_table[j].number * 100)
				{
					// If the vnum does fit in the zone before,
					if (mobVnum >= zone_table[j - 1].number * 100)
					{
						// Set the zone number correctly.
						mobZone = j - 1;
					}
					// Otherwise, we have some buggy s*** going on.
					else
					{
						debug("which_race: mob '%s' (%d) does not have a home zone?!?",
						      J_NAME(mob), mobVnum);
						// Set the zone number to Heavens to prevent crashes.
						mobZone = 0;
					}
					break;
				}
			}
			extract_char(mob);
			snprintf(buf, MAX_STRING_LENGTH,
				 "%3d)&+W%c&n%6d %s &n-%c&+c%2d&n/&+C%2d&n  %s\n", ++count,
				 (mobIndex->func.mob == NULL) ? ' ' : '*', mobVnum,
				 pad_ansi((mobIndex->desc2 == NULL) ? "(NULL)" : mobIndex->desc2,
					  30, TRUE)
					 .c_str(),
				 (mobIndex->qst_func == NULL) ? ' ' : 'Q', mobIndex->number,
				 mobIndex->limit, zone_table[mobZone].name);
			// If the next line exceeds size of return buffer (-30 for terminating char + "And the list goes on...\n\r").
			if (oBufLength + strlen(buf) > MAX_STRING_LENGTH - 30)
			{
				strcat(oBuf, "And the list goes on...\n\r");
				break;
			}
			strcat(oBuf, buf);
			oBufLength += strlen(buf);
		}
		else
		{
			extract_char(mob);
		}
	}
	if (count > 0)
	{
		snprintf(
			buf, MAX_STRING_LENGTH,
			"Num)   Vnum          Name                &+cInGame&n/&+CMax&n Home Zone\n\r"
			"    &+W*&n=Special Func                         -Q=Quest Mob\n\r");
		send_to_char(buf, ch);
		page_string(ch->desc, oBuf, 1);
	}
	else
	{
		snprintf(buf, MAX_STRING_LENGTH, "No mobs of race '%s' (%d) found.\n\r",
			 race_names_table[raceIndex].ansi, raceIndex);
		send_to_char(buf, ch);
	}
}
// This looks through each item type for the supplied flag/amounts.
static void which_stat(P_char ch, char *argument)
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
			send_to_char("&+WFormat: &+wwhich stat <flag> <amount>&+W.&n\n", ch);
			send_to_char(
				"i.e. '&+wwhich stat maxint 5&n' for all items with exactly 5maxint.\n",
				ch);
			send_to_char(
				"Note: You can add a '+' or '-' immediately after <amount> for all"
				" items with <amount> or {greater|lesser} of <flag>.\n\r",
				ch);
			send_to_char(
				"i.e. '&+wwhich stat str 15+&n' for all items with 15 or more str.\n",
				ch);
			send_to_char(
				"i.e. '&+wwhich stat str -15-&n' for all items with -15 or less str.\n",
				ch);
			send_to_char(
				"For a full list of options (omg), use '&+wwhich stat options&n'.\n",
				ch);
			send_to_char(
				"This command lists the types of objects that fall under the given options.\n",
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
		// For each real object..
		for (int r_num = 0; r_num <= top_of_objt; r_num++)
		{
			// Load a copy of object.
			obj = read_object(r_num, REAL);

			// Check flags for match.
			for (i = 0; i < MAX_OBJ_AFFECT; i++)
			{
				if (obj->affected[i].location == flags &&
				    obj->affected[i].modifier >= amount)
				{
					// As if things could be easier to read...
					snprintf(buf, MAX_STRING_LENGTH,
						 "&%s(%+4d)&+W%c&%s%3d/%3d %6d &n'%s'\n",
						 OBJ_COLOR(r_num), obj->affected[i].modifier,
						 IS_ARTIFACT(obj) ? '*' : ' ', OBJ_COLOR(r_num),
						 obj_index[r_num].number - 1,
						 obj_index[r_num].limit,
						 obj_index[r_num].virtual_number,
						 obj->short_description);
					send_to_char(buf, ch);
				}
			}

			// Free up the object copy.
			extract_obj(obj, FALSE);
		}
	}
	else if (lesser)
	{
		// For each real object..
		for (int r_num = 0; r_num <= top_of_objt; r_num++)
		{
			// Load a copy of object.
			obj = read_object(r_num, REAL);

			// Check flags for match.
			for (i = 0; i < MAX_OBJ_AFFECT; i++)
			{
				if (obj->affected[i].location == flags &&
				    obj->affected[i].modifier <= amount)
				{
					// As if things could be easier to read...
					snprintf(buf, MAX_STRING_LENGTH,
						 "&%s(%+4d)&+W%c&%s%3d/%3d %6d &n'%s'\n",
						 OBJ_COLOR(r_num), obj->affected[i].modifier,
						 IS_ARTIFACT(obj) ? '*' : ' ', OBJ_COLOR(r_num),
						 obj_index[r_num].number - 1,
						 obj_index[r_num].limit,
						 obj_index[r_num].virtual_number,
						 obj->short_description);
					send_to_char(buf, ch);
				}
			}

			// Free up the object copy.
			extract_obj(obj, FALSE);
		}
	}
	else
	{
		// For each real object..
		for (int r_num = 0; r_num <= top_of_objt; r_num++)
		{
			// Load a copy of object.
			obj = read_object(r_num, REAL);

			// Check flags for match.
			for (i = 0; i < MAX_OBJ_AFFECT; i++)
			{
				if (obj->affected[i].location == flags &&
				    obj->affected[i].modifier == amount)
				{
					// As if things could be easier to read...
					snprintf(buf, MAX_STRING_LENGTH,
						 "&%s(%+4d)&+W%c&%s%3d/%3d %6d &n'%s'\n",
						 OBJ_COLOR(r_num), obj->affected[i].modifier,
						 IS_ARTIFACT(obj) ? '*' : ' ', OBJ_COLOR(r_num),
						 obj_index[r_num].number - 1,
						 obj_index[r_num].limit,
						 obj_index[r_num].virtual_number,
						 obj->short_description);
					send_to_char(buf, ch);
				}
			}

			// Free up the object copy.
			extract_obj(obj, FALSE);
		}
	}
}
// This looks through each item type for food-types and lists them along with their benefits.
static void which_food(P_char ch, char * /*argument*/)
{
	int count = 0;
	char buf[MAX_STRING_LENGTH];
	P_obj obj;

	// Display the Header:
	// "  1)    0/   0     13 a *huge* valium tablet measuri - Unknown."
	snprintf(buf, MAX_STRING_LENGTH,
		 "&=LWNum) INGAME      VNUM Description                    - Modifiers&n\n"
		 "         &=LW/ LIMIT&n\n");
	send_to_char(buf, ch);

	for (int r_num = 0; r_num <= top_of_objt; r_num++)
	{
		// Load a copy of object.
		obj = read_object(r_num, REAL);

		if (obj->type == ITEM_FOOD)
		{
			snprintf(buf, MAX_STRING_LENGTH, "%3d) &%s%4d/%4d %6d&n %s&n - %s.\n",
				 ++count, OBJ_COLOR(r_num), obj_index[r_num].number - 1,
				 obj_index[r_num].limit, obj_index[r_num].virtual_number,
				 pad_ansi(obj->short_description, 30, TRUE).c_str(),
				 food_modifiers(obj));
			send_to_char(buf, ch);
		}
		// Free up the object copy.
		extract_obj(obj, FALSE);
	}
}
// This looks through each mob type for the supplied class-spec.
static void which_spec(P_char ch, char *argument)
{
	char specname[MAX_STRING_LENGTH], *tmp;
	int mclass, spec, R_num, count;
	bool found;
	P_char mob;

	// Make the argument all lowercase.
	tmp = argument;
	while (*tmp != '\0')
	{
		*tmp = LOWER(*tmp);
		tmp++;
	}

	found = FALSE;
	for (mclass = CLASS_NONE; mclass <= CLASS_COUNT; mclass++)
	{
		for (spec = 0; spec < MAX_SPEC; spec++)
		{
			// Get the ansi-less spec name..
			snprintf(specname, MAX_STRING_LENGTH, "%s",
				 strip_ansi(specdata[mclass][spec]).c_str());
			// And make it all lower case.
			tmp = specname;
			while (*tmp != '\0')
			{
				*tmp = LOWER(*tmp);
				tmp++;
			}
			// If we found a perfect match.
			if (!strcmp(argument, specname))
			{
				found = TRUE;
				break;
			}
		}
		if (found)
			break;
	}
	if (!found)
	{
		send_to_char_f(
			ch,
			"&+YCould not find spec '&+w%s&+Y'.&n\n&+YDid you enter the full spec name?&n\n",
			argument);
		return;
	}

	send_to_char_f(ch, "&=LWList of mobs with spec:&N %s&N\n", specdata[mclass][spec]);

	// Convert from count to bitvector.
	mclass = 1 << (mclass - 1);
	// Convert from subscript (0-3) to actual count (1-4).
	spec++;

	// Walk through each mob type (skip mob 0 = prototype).
	for (R_num = 1, count = 0; R_num <= top_of_mobt; R_num++)
	{
		mob = read_mobile(R_num, REAL);
		if (GET_SPEC(mob, mclass, spec))
			send_to_char_f(ch, "%2d) %6d %s\n", ++count, GET_VNUM(mob),
				       mob->player.short_descr);
		extract_char(mob);
	}

	if (count == 0)
		send_to_char("&+YNone found.&n\n", ch);
}
static char *weapon_modifiers(P_obj weapon)
{
	static char mod_string[MAX_STRING_LENGTH];
	int hit, dam, i;

	hit = dam = 0;
	for (i = 0; i < MAX_OBJ_AFFECT; i++)
	{
		if (weapon->affected[i].location == APPLY_HITROLL)
		{
			hit += weapon->affected[i].modifier;
		}
		else if (weapon->affected[i].location == APPLY_DAMROLL)
		{
			dam += weapon->affected[i].modifier;
		}
	}

	snprintf(mod_string, MAX_STRING_LENGTH, "%dd%d %d/%d", weapon->value[1], weapon->value[2],
		 hit, dam);

	return mod_string;
}
static char *armor_modifiers(P_obj armor)
{
	static char mod_string[MAX_STRING_LENGTH];

	snprintf(mod_string, MAX_STRING_LENGTH, "&+YAC-apply: &N%d", armor->value[0]);

	return mod_string;
}
static void which_armor(P_char ch, char *argument)
{
	int count;
	char buf[MAX_STRING_LENGTH];
	char arg1[100];
	char arg2[10];
	P_obj obj;

	argument_interpreter(argument, arg1, arg2);

	if (!is_number(arg1))
	{
		send_to_char("Please an AC (value0) value to compare.\n", ch);
		return;
	}

	int acValue = atoi(arg1);

	typedef enum
	{
		OP_LESS_THAN = 0,
		OP_EQUAL = 1,
		OP_GREATER_THAN = 2
	} Operand;

	Operand op = OP_EQUAL;

	if (!strcmp(arg2, "-"))
	{
		op = OP_LESS_THAN;
	}
	else if (!strcmp(arg2, "+"))
	{
		op = OP_GREATER_THAN;
	}

	// Display the Header:
	// "  1)    0/   0     13 a *huge* valium tablet measuri - Unknown."
	snprintf(buf, MAX_STRING_LENGTH,
		 "&=LWNum) INGAME      VNUM Description                    - Modifiers&n\n"
		 "         &=LW/ LIMIT&n\n");
	send_to_char(buf, ch);
	count = 0;
	for (int r_num = 0; r_num <= top_of_objt; r_num++)
	{
		// Load a copy of object.
		obj = read_object(r_num, REAL);

		if (obj->type == ITEM_ARMOR)
		{
			if ((op == OP_EQUAL && obj->value[0] == acValue) ||
			    (op == OP_LESS_THAN && obj->value[0] < acValue) ||
			    (op == OP_GREATER_THAN && obj->value[0] > acValue))
			{
				snprintf(buf, MAX_STRING_LENGTH,
					 "%3d) &%s%4d/%4d %6d&n %s&n - %s.\n", ++count,
					 OBJ_COLOR(r_num), obj_index[r_num].number - 1,
					 obj_index[r_num].limit, obj_index[r_num].virtual_number,
					 pad_ansi(obj->short_description, 30, TRUE).c_str(),
					 armor_modifiers(obj));
				send_to_char(buf, ch);
			}
		}
		// Free up the object copy.
		extract_obj(obj, FALSE);
	}
}
static void which_weapon(P_char ch, char *argument)
{
	int type, count;
	char buf[MAX_STRING_LENGTH];
	P_obj obj;

	if (!*argument)
	{
		send_to_char("Please supply a weapon type (ie shortsword).\n", ch);
		return;
	}
	for (type = 0; type <= WEAPON_HIGHEST; type++)
	{
		if (is_abbrev(argument, weapon_types[type].flagLong))
		{
			break;
		}
	}
	if (is_abbrev(argument, "twohanded") || is_abbrev(argument, "two-handed") ||
	    is_abbrev(argument, "twohands") || is_abbrev(argument, "two-hands"))
	{
		type = -1;
	}
	if (type > WEAPON_HIGHEST)
	{
		send_to_char_f(ch, "'%s' is not a valid weapon type.\n", argument);
		send_to_char_f(ch, "Valid weapon types are: %s", weapon_types[type].flagLong);
		for (type = 1; type <= WEAPON_HIGHEST; type++)
		{
			send_to_char(", ", ch);
			send_to_char(weapon_types[type].flagLong, ch);
		}
		send_to_char(".\n", ch);
		return;
	}
	if (type >= 0)
	{
		type = weapon_types[type].defVal;
	}

	// Display the Header:
	// "  1)    0/   0     13 a *huge* valium tablet measuri - Unknown."
	snprintf(buf, MAX_STRING_LENGTH,
		 "&=LWNum) INGAME      VNUM Description                    - Modifiers&n\n"
		 "         &=LW/ LIMIT&n\n");
	send_to_char(buf, ch);
	count = 0;
	for (int r_num = 0; r_num <= top_of_objt; r_num++)
	{
		// Load a copy of object.
		obj = read_object(r_num, REAL);

		if ((obj->type == ITEM_WEAPON) &&
		    ((type == -1 && IS_SET(obj->extra_flags, ITEM_TWOHANDS)) ||
		     (obj->value[0] == type)))
		{
			snprintf(buf, MAX_STRING_LENGTH, "%3d) &%s%4d/%4d %6d&n %s&n - %s.\n",
				 ++count, OBJ_COLOR(r_num), obj_index[r_num].number - 1,
				 obj_index[r_num].limit, obj_index[r_num].virtual_number,
				 pad_ansi(obj->short_description, 30, TRUE).c_str(),
				 weapon_modifiers(obj));
			send_to_char(buf, ch);
		}
		// Free up the object copy.
		extract_obj(obj, FALSE);
	}
}

#undef OBJ_COLOR
#undef WHICH_SYNTAX
