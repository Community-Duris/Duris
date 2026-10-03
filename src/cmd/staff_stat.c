/* Staff statistics and world inspection command family. */

#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utility.h"
#include "core/utils.h"
#include "core/safe_format.h"
#include "net/comm.h"
#include "world/db.h"
#include "world/events.h"
#include "cmd/interp.h"
#include "economy/shop.h"
#include "guild/assocs.h"
#include "world/achievements.h"
#include "combat/damage.h"
#include "combat/training_dummy.h"
#include "world/epic.h"
#include "world/epic_transaction.h"
#include "core/files.h"
#include "combat/justice.h"
#include "world/map.h"
#include "core/mm.h"
#include "item/objmisc.h"
#include "persistence/persistence_mode.h"
#include "ships/ships.h"
#include "world/specs.prototypes.h"
#include "magic/spells.h"
#include "sql/sql.h"
#include "item/trophy.h"
#include "world/vnum.obj.h"
#include "world/weather.h"
#include "net/listen.h"
#include "net/gmcp.h"
#include "net/ws_handlers.h"
#include "item/item_movement_transaction.h"
#include "account/creation_availability_config.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

extern Skill skills[];
extern char *spells[];
extern P_desc descriptor_list;
extern P_index mob_index;
extern P_index obj_index;
extern P_obj object_list;
extern P_room world;
extern const char *justice_flags[];
extern const flagDef action_bits[];
extern const flagDef action2_bits[];
extern const flagDef aggro_bits[];
extern const flagDef aggro2_bits[];
extern const flagDef aggro3_bits[];
extern const char *apply_types[];
extern const struct class_names class_names_table[];
extern const char *command[];
extern const char *connected_types[];
extern const char *dirs[];
extern const char *drinks[];
extern const char *exit_bits[];
extern const flagDef extra_bits[];
extern const flagDef extra2_bits[];
extern const flagDef affected1_bits[];
extern const flagDef affected2_bits[];
extern const flagDef affected3_bits[];
extern const flagDef affected4_bits[];
extern const flagDef affected5_bits[];
extern const char *item_types[];
extern const char *shot_types[];
extern const char *player_bits[];
extern const char *player2_bits[];
extern const char *player3_bits[];
extern const char *player_prompt[];
extern const char *position_types[];
extern struct race_names race_names_table[];
extern const flagDef room_bits[];
extern const char *sector_types[];
extern const flagDef wear_bits[];
extern const char *zone_bits[];
extern const char *item_material[];
extern const struct stat_data stat_factor[];
extern const char *size_types[];
extern int number_of_shops;
extern int pulse;
extern const int top_of_world;
extern int top_of_zone_table;
extern int used_descs;
extern struct shop_data *shop_index;
extern struct str_app_type str_app[];
extern struct zone_data *zone_table;
extern struct link_description link_types[];
extern flagDef weapon_types[];
extern flagDef missile_types[];
extern const char *spldam_types[];
extern const char *craftsmanship_names[];
extern struct quest_data quest_index[];
extern float spell_pulse_data[LAST_RACE + 1];
extern int racial_shrug_data[LAST_RACE + 1];
extern const struct racial_data_type racial_data[];
extern const struct innate_data innates_data[];
extern float racial_exp_mods[LAST_RACE + 1];
extern float racial_exp_mod_victims[LAST_RACE + 1];
extern int damroll_cap;
extern const racewar_struct racewar_color[MAX_RACEWAR + 2];
extern struct continent_misfire_data continent_misfire;
extern struct misfire_properties_struct misfire_properties;
extern int race_lookup(char *raceStr);
extern float combat_by_race[LAST_RACE + 1][3];
extern int racial_values[LAST_RACE + 1][2];
extern char racial_innates[LAST_INNATE + 1][LAST_RACE + 1];
extern const char *specdata[][MAX_SPEC];
extern long new_exp_table[];
extern int spl_table[TOTALLVLS][MAX_CIRCLE];
extern struct time_info_data time_info;
extern const char *get_function_name(void *func);
extern void event_mob_mundane(P_char, P_char, P_obj, void *);
extern void shopping_stat(P_char, P_char, char *, int);

static void stat_race(P_char ch, char *arg);
static void stat_skill(P_char ch, char *arg);
static void stat_zone(P_char ch, char *arg);

static void stat_dam(P_char ch, char * /*arg*/)
{
	const char *race_name;
	char tmplate[512], buf[512], prop_name[512];
	float pulse, multiplier, mult_mod, damcap;
	int race;

	send_to_char("Race          &+WPulse&n  Mult  (&+WProp&n) DamCap\n", ch);
	send_to_char("-------------------------------------------\n", ch);
	for (race = 1; race <= LAST_RACE; race++)
	{
		race_name = race_names_table[race].ansi;
		pulse = combat_by_race[race][0];
		pulse += (int)(get_property("damage.pulse.class.all", 2));
		snprintf(prop_name, 512, "damage.totalOutput.racial.%s",
			 race_names_table[race].no_spaces);
		mult_mod = get_property(prop_name, 1.);
		multiplier = combat_by_race[race][1];
		damcap = combat_by_race[race][2];
		snprintf(buf, 512, "%%-%lds &+W%%2d&n  %%.3f  (&+W%%.3f&n) &+%%c%%.3f (%%d)&n\n",
			 (long)(strlen(race_name) - ansi_strlen(race_name) + 15));
		checked_snprintf_runtime(tmplate, 512, buf, race_name, (int)pulse, multiplier,
					 mult_mod, (damcap > 1) ? 'C' : 'c', damcap,
					 (int)(damcap * damroll_cap));
		send_to_char(tmplate, ch);
	}
}

static void stat_spldam(P_char ch, char * /*arg*/)
{
	char line[MAX_STRING_LENGTH], buf[512];
	int type, race;
	float val;

	line[0] = '\0';

	send_to_char("Spell Type Mods (offensive / defensive)\n", ch);
	send_to_char(
		"Race            Genrc Fire  Cold  Light Gas   Acid  Neg   Holy  Psi   Spirt Sound Earth\n",
		ch);
	send_to_char(
		"---------------------------------------------------------------------------------------\n",
		ch);
	// Skip RACE_NONE.
	for (race = 1; race <= LAST_RACE; race++)
	{
		// Start with racename.
		strcpy(line, pad_ansi(race_names_table[race].ansi, 16).c_str());

		// List modifier for each type of spell damage.
		for (type = 0; type < LAST_SPLDAM_TYPE; type++)
		{
			snprintf(buf, 512, "damage.spellTypeMod.offensive.racial.%s.%s",
				 race_names_table[race].no_spaces, spldam_types[type]);

			val = get_property(buf, 1.00);

			snprintf(buf, 512, "%1.3f ", val);
			strcat(line, buf);
		}

		strcat(line, "\n                ");

		for (type = 0; type < LAST_SPLDAM_TYPE; type++)
		{
			snprintf(buf, 512, "damage.spellTypeMod.defensive.racial.%s.%s",
				 race_names_table[race].no_spaces, spldam_types[type]);

			val = get_property(buf, 1.00);

			snprintf(buf, 512, "%1.3f ", val);
			strcat(line, buf);
		}

		strcat(line, "\n\n");
		send_to_char(line, ch);
	}
}

static void stat_pvp(P_char ch)
{
	for (int i = 0; i <= MAX_RACEWAR; i++)
	{
		send_to_char_f(ch, "misfire.pvp.maxAllies.%s: %d.\n", racewar_color[i].name,
			       misfire_properties.pvp_maxAllies[i]);
	}
}

void stat_game(P_char ch)
{
	P_desc d;
	P_char t_ch = NULL;
	char buf[MAX_STRING_LENGTH];
	float race[LAST_RACE + 1];
	float m_class[CLASS_COUNT + 1];
	int i;
	sh_int n, evils = 0, goods = 0, pundeads = 0;
	float x;

	buf[0] = '\0';
	x = used_descs;

	/* clear out counters */
	for (i = 0; i < static_cast<int>(ARRAY_SIZE(race)); i++)
		race[i] = 0.0;
	for (i = 0; i < static_cast<int>(ARRAY_SIZE(m_class)); i++)
		m_class[i] = 0.0;
	/* begin counting */
	for (d = descriptor_list; d; d = d->next)
	{
		if (d->character)
			t_ch = d->character;
		else
			t_ch = NULL;
		if (d->connected != CON_PLAYING)
			continue;
		if (t_ch)
		{
			race[GET_RACE(t_ch)]++;
			m_class[flag2idx(t_ch->player.m_class)]++;

			if (EVIL_RACE(t_ch))
				evils++;
			if (GOOD_RACE(t_ch))
				goods++;
			if (PUNDEAD_RACE(t_ch))
				pundeads++;
		}
	}
	/*  strcat(buf, "\n&+L(Values in approximate percentages currently online)&n\n");*/
	strcat(buf + strlen(buf), "\n&+B         RACES                          CLASSES\n\n");
	n = MAX(LAST_RACE, CLASS_COUNT);
	for (i = 0; i < n; i++)
	{
		if (i < LAST_RACE && race[i + 1])
		{
			APPENDF(buf, "%2d%%/%3d  %s", (int)((race[i + 1] / x) * 100 + .5),
				(int)race[i + 1],
				pad_ansi(race_names_table[i + 1].ansi, 15).c_str());
		}
		else if (i < (LAST_RACE - 1))
		{
			APPENDF(buf, " 0%%/  0  %s",
				pad_ansi(race_names_table[i + 1].ansi, 15).c_str());
		}
		else
			strcat(buf + strlen(buf), "               ");

		if (i < CLASS_COUNT && m_class[i + 1])
		{
			APPENDF(buf, "      %10d%%/%3d  %s\n",
				(int)((m_class[i + 1] / x) * 100 + .5), (int)m_class[i + 1],
				class_names_table[i + 1].ansi);
		}
		else if (i < (CLASS_COUNT - 1))
		{
			APPENDF(buf, "      %10d%%/%3d  %s \n", 0, 0,
				class_names_table[i + 1].ansi);
		}
		else
			strcat(buf + strlen(buf), "\n");
	}
	APPENDF(buf, "\nGood/Evil/Undead -raced players: %3d/%3d/%3d", goods, evils, pundeads);
	APPENDF(buf, "\nTotal playing          : %3d\n", used_descs);
	send_to_char(buf, ch);
}

#define STAT_SYNTAX                                                                                                                                                                              \
	"Syntax:\n   stat game\n   stat room <room #>\n   stat zone <zone #>\n   stat obj|item  #|'name'\n   stat char|mob #|'name'\n   stat trap 'name'\n   stat shop #|'name'\n   stat skill " \
	"#|'name'\n   stat damage\n   stat quest 'name'\n   stat quest 'race'\n"

// CMD = 555 is used for storing stat o string in db.

void do_stat(P_char ch, char *argument, int cmd)
{
	P_char k = 0, t_mob = 0, shopkeeper, mob;
	P_obj j = 0, t_obj = 0;
	P_room rm = 0;
	char arg1[MAX_STRING_LENGTH], arg2[MAX_STRING_LENGTH], *rest;
	char buf1[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];
	char buf[MAX_STRING_LENGTH], o_buf[MAX_STRING_LENGTH];
	char *timestr;
	char time_left[128], showed_vals = FALSE;
	int i = 0, i2, i3, i4, m_virtual, num_tr, num_pr, x, qi;
	Memory *mem;
	struct affected_type *aff;
	struct extra_descr_data *desc;
	struct follow_type *fol;
	struct time_info_data playing_time;
	float fragnum = 0;
	time_t now;

	if (IS_NPC(ch))
		return;

	/* for mortals, reroute command to do_att, in case they are used to
	   muds using stat */

	if (cmd != 555)
	{
		if (!IS_TRUSTED(ch))
		{
			for (shopkeeper = world[ch->in_room].people; shopkeeper;
			     shopkeeper = shopkeeper->next_in_room)
				if (IS_SHOPKEEPER(shopkeeper))
				{
					shopping_stat(ch, shopkeeper, argument, cmd);
					return;
				}
			do_attributes(ch, argument, cmd);
			return;
		}
	}

	//  argument_interpreter(argument, arg1, arg2);
	rest = lohrr_chop(argument, arg1);
	rest = lohrr_chop(rest, arg2);

	if (!*arg1)
	{
		send_to_char(STAT_SYNTAX, ch);
		return;
	}
	o_buf[0] = '\0'; /* output string, so we can page it */
	buf[0] = '\0';
	buf1[0] = '\0';
	buf2[0] = '\0';

	/* stats on game */
	if ((*arg1 == 'g') || (*arg1 == 'G'))
	{
		stat_game(ch);
		return;
	}

	// Stats on pvp table
	if ((*arg1 == 'p') || (*arg1 == 'P'))
	{
		stat_pvp(ch);
		return;
	}

	/* stats on room  */
	if ((*arg1 == 'r') || (*arg1 == 'R'))
	{
		if (*(arg1 + 1) == 'a' || *(arg1 + 1) == 'A')
		{
			// Skip the 'race' argument and any spaces following it.
			stat_race(ch, skip_spaces(argument + strlen(arg1)));
			return;
		}
		// TODO: add guildhall room stats?
		if (!*arg2)
			i = ch->in_room;
		else
		{
			/* accept a room number as second arg  */
			if (!is_number(arg2) || ((i = real_room(atoi(arg2))) < 0) ||
			    (i > top_of_world))
			{
				send_to_char("Room not in world.\n", ch);
				return;
			}
		}

		rm = &world[i];

		sprinttype(rm->sector_type, sector_types, buf2);
		checked_snprintf(o_buf, MAX_STRING_LENGTH,
				 "&+YRoom: [&N%d&+Y](&N%d&+Y)  Zone: &N%d&+Y  Sector type: &N%s\n",
				 rm->number, i, zone_table[rm->zone].number, buf2);

		checked_snprintf(o_buf + strlen(o_buf), MAX_STRING_LENGTH - strlen(o_buf),
				 "&+YName: &N%s\n", rm->name);

		sprintbitde(rm->room_flags, room_bits, buf2);
		checked_snprintf(o_buf + strlen(o_buf), MAX_STRING_LENGTH - strlen(o_buf),
				 "&+YRoom flags:&N %s\n", buf2);

		checked_snprintf(o_buf + strlen(o_buf), MAX_STRING_LENGTH - strlen(o_buf),
				 "&+YWeather sector: &N%d\n",
				 in_weather_sector(real_room0(rm->number)));

		if (rm->continent)
		{
			checked_snprintf(o_buf + strlen(o_buf), MAX_STRING_LENGTH - strlen(o_buf),
					 "&+YContinent: &n%s\n", continent_name(rm->continent));
			for (int racewar = 1; racewar <= MAX_RACEWAR; racewar++)
			{
				checked_snprintf(
					o_buf + strlen(o_buf), MAX_STRING_LENGTH - strlen(o_buf),
					"  &+%c%7s &+Yplayers: &N%d, &+Ymisfire: &N%s.\n",
					racewar_color[racewar].color, racewar_color[racewar].name,
					continent_misfire.players[rm->continent][racewar],
					YESNO(continent_misfire.misfiring[rm->continent][racewar]));
			}
		}

		checked_snprintf(o_buf + strlen(o_buf), MAX_STRING_LENGTH - strlen(o_buf),
				 "&+YJustice Patrol:&N %s \n",
				 town_name_list[(int)rm->justice_area]);

		checked_snprintf(o_buf + strlen(o_buf), MAX_STRING_LENGTH - strlen(o_buf),
				 "&+YSpecial procedure:&N %s\n",
				 (rm->funct) ? get_function_name((void *)rm->funct) : "None");

		checked_snprintf(
			o_buf + strlen(o_buf), MAX_STRING_LENGTH - strlen(o_buf),
			"&+YCurrent: (&N%d&+Y)-(&N%d)&+Y  Chance of falling:&N %d&+Y%%  Light sources:&N %d &+YSunShine:&N %s\n&+YDescription:&N\n",
			rm->current_speed, rm->current_direction, rm->chance_fall, rm->light,
			YESNO(IS_SUNLIT(i)));

		checked_snprintf(o_buf + strlen(o_buf), MAX_STRING_LENGTH - strlen(o_buf),
				 "&+YSection: &N%d  &+YX = &N%d  &+YY = &N%d  &+YZ = &N%d&N\n",
				 rm->map_section, rm->x_coord, rm->y_coord, rm->z_coord);

		if (rm->description)
			strcat(o_buf, rm->description);

		if (rm->ex_description)
		{
			strcpy(buf, "\n&+YExtra description keywords(s):\n");
			for (desc = rm->ex_description; desc; desc = desc->next)
			{
				strcat(buf, desc->keyword);
				strcat(buf, "\n");
			}
			strcat(buf, "\n");
			strcat(o_buf, buf);
		}
		strcpy(buf, "&+Y------- Chars present -------\n");
		for (k = rm->people; k; k = k->next_in_room)
		{
			if (IS_PC(k) && !CAN_SEE(ch, k))
				continue;
			strcat(buf, IS_PC(k) ? " &+Y(PC)&N " : "&+R(NPC)&N ");
			strcat(buf, GET_NAME(k));
			strcat(buf, "\n");
		}
		strcat(buf, "\n");
		strcat(o_buf, buf);

		if (rm->contents)
		{
			strcpy(buf, "&+Y--------- Contents ---------\n");
			for (j = rm->contents; j; j = j->next_content)
			{
				strcat(buf, j->short_description);
				strcat(buf, "\n");
			}
			strcat(buf, "\n");
			strcat(o_buf, buf);
		}
		strcat(o_buf, "&+Y------- Exits defined -------\n");
		for (i = 0; i <= (NUM_EXITS - 1); i++)
		{
			if (rm->dir_option[i])
			{
				sprintbit((ulong)rm->dir_option[i]->exit_info, exit_bits, buf2);
				checked_snprintf(
					buf, MAX_STRING_LENGTH,
					"&+YDirection &+R%5s  &+YKeyword: &+G%s  &+YKey:&N %d  &+YExit flag: &N%s\n&+YTo room: [&N%d&+Y](&N%d&+Y)&N  %s\n\n",
					dirs[i], rm->dir_option[i]->keyword, rm->dir_option[i]->key,
					buf2,
					(rm->dir_option[i]->to_room != NOWHERE) ?
						world[rm->dir_option[i]->to_room].number :
						-1,
					rm->dir_option[i]->to_room,
					(rm->dir_option[i]->general_description) ?
						rm->dir_option[i]->general_description :
						"UNDEFINED");
				strcat(o_buf, buf);
			}
		}
		page_string(ch->desc, o_buf, 1);
		return;
	}
	else if ((*arg1 == 'z') || (*arg1 == 'Z'))
	{
		stat_zone(ch, arg2);
		/* stat all zones, for exits */
	}
	else if ((*arg1 == 'w') || (*arg1 == 'W'))
	{
		send_to_char("broken, leave me alone.\n", ch);
		return;

		/*
		    for (x = 0; x <= top_of_zone_table; x++)
		    {
		      zone = &zone_table[x];
		      snprintf(o_buf, MAX_STRING_LENGTH, "&+YZone: (&N%d&+Y)  Name:&N %s\n",
		              world[zone->real_bottom].zone, zone->name);

		      for (i3 = 0, i = zone->real_bottom;
		           (i != NOWHERE) && (i <= zone->real_top); i++)
		        for (i2 = 0; i2 < NUM_EXITS; i2++)
		          if(world[i].dir_option[i2])
		          {
		            if((world[i].dir_option[i2]->to_room == NOWHERE) ||
		                (world[world[i].dir_option[i2]->to_room].zone !=
		                 world[i].zone))
		            {
		              if(!i3)
		                i3 = 1;
		              if(world[i].dir_option[i2]->to_room == NOWHERE)
		                snprintf(o_buf + strlen(o_buf), MAX_STRING_LENGTH - strlen(o_buf),
		                        " &+Y[&n%5d&+Y](&n%5d&+Y)&n &+R%-5s&n to &+WNOWHERE\n",
		                        world[i].number, i, dirs[i2]);
		              else
		                snprintf(o_buf + strlen(o_buf), MAX_STRING_LENGTH - strlen(o_buf),
		                        " &+Y[&n%5d&+Y]&n &+R%-5s&n to &+Y[&+R%3d&n:&+Y%5d&+Y]&n %s\n",
		                        world[i].number, dirs[i2],
		                        zone_table[world[world[i].dir_option[i2]->to_room].zone].number,
		                        world[world[i].dir_option[i2]->to_room].number,
		                        world[world[i].dir_option[i2]->to_room].name);
		            }
		          }
		      send_to_char(o_buf, ch);
		    }
		    return;
		*/
		/* stat on object  */
	}
	else if ((*arg1 == 'o') || (*arg1 == 'O') || (*arg1 == 'i') || (*arg1 == 'I'))
	{
		if (!*arg2)
		{
			send_to_char(STAT_SYNTAX, ch);
			return;
		}

		if (is_number(arg2))
		{
			if ((i = real_object(atoi(arg2))) < 0)
			{
				send_to_char("Illegal object number.\n", ch);
				return;
			}
			/* load one to stat, extract after statting  */
			t_obj = read_object(i, REAL);
			if (!t_obj)
			{
				logit(LOG_DEBUG, "do_stat(): obj %d [%d] not loadable", i,
				      obj_index[i].virtual_number);
				return;
			}
			// If there's more than one in the game, pull t_obj.
			if (obj_index[t_obj->R_num].number > 1)
			{
				extract_obj(t_obj);
				t_obj = NULL;
			}
		}
		/*
		    if(cmd == 555) //special code for web eq stats
		    {
		        if(j = get_obj_vis(ch, arg2))
		        {
		          if(!strcmp(j->name, arg2))
		            break;
		          else
		          j = NULL;
		        }

		        if(j == NULL)
		         return;

		    }
		    else
		*/
		if (!(j = get_obj_vis(ch, arg2)))
		{
			send_to_char("No such object.\n", ch);
			if (t_obj)
			{
				extract_obj(t_obj);
			}
			return;
		}

		m_virtual = (j->R_num >= 0) ? obj_index[j->R_num].virtual_number : 0;

		sprinttype(GET_ITEM_TYPE(j), item_types, buf2);
		checked_snprintf(
			o_buf, MAX_STRING_LENGTH,
			"&+YObject:\n&+YNumber: [&N%d&+Y](&N%d&+Y)  Type: &N%s  &+YName: &N%s\n",
			m_virtual, j->R_num, buf2,
			((j->short_description) ? j->short_description : "None"));

		checked_snprintf(o_buf + strlen(o_buf), MAX_STRING_LENGTH - strlen(o_buf),
				 "&+YKeywords: &N%s\n&+YLong description:\n%s\n",
				 ((j->name) ? j->name : "None"),
				 ((j->description) ? j->description : "None"));

		if (j->ex_description)
		{
			strcpy(buf, "&+YExtra description keyword(s):\n&+Y----------\n");
			for (desc = j->ex_description; desc; desc = desc->next)
			{
				strcat(buf, desc->keyword);
				strcat(buf, "\n");
			}
			strcat(buf, "&+Y----------\n");
			strcat(o_buf, buf);
		}
		checked_snprintf(o_buf + strlen(o_buf), MAX_STRING_LENGTH - strlen(o_buf),
				 "&+YNumber in game : &N%d\n",
				 (obj_index[j->R_num].number - ((t_obj != NULL) ? 1 : 0)));

		sprintbitde(j->wear_flags, wear_bits, buf2);
		checked_snprintf(o_buf + strlen(o_buf), MAX_STRING_LENGTH - strlen(o_buf),
				 "&+YCan be worn on : &N%s\n", buf2);

		if (j->bitvector)
		{
			sprintbitde(j->bitvector, affected1_bits, buf2);
			checked_snprintf(o_buf + strlen(o_buf), MAX_STRING_LENGTH - strlen(o_buf),
					 "&+YSet char bits 1: &N%s\n", buf2);
		}

		if (j->bitvector2)
		{
			sprintbitde(j->bitvector2, affected2_bits, buf2);
			checked_snprintf(o_buf + strlen(o_buf), MAX_STRING_LENGTH - strlen(o_buf),
					 "&+YSet char bits 2: &N%s\n", buf2);
		}

		if (j->bitvector3)
		{
			sprintbitde(j->bitvector3, affected3_bits, buf2);
			checked_snprintf(o_buf + strlen(o_buf), MAX_STRING_LENGTH - strlen(o_buf),
					 "&+YSet char bits 3: &N%s\n", buf2);
		}

		if (j->bitvector4)
		{
			sprintbitde(j->bitvector4, affected4_bits, buf2);
			checked_snprintf(o_buf + strlen(o_buf), MAX_STRING_LENGTH - strlen(o_buf),
					 "&+YSet char bits 4: &N%s\n", buf2);
		}

		if (j->bitvector5)
		{
			sprintbitde(j->bitvector5, affected5_bits, buf2);
			checked_snprintf(o_buf + strlen(o_buf), MAX_STRING_LENGTH - strlen(o_buf),
					 "&+YSet char bits 5: &N%s\n", buf2);
		}

		if (j->extra_flags)
		{
			sprintbitde(j->extra_flags, extra_bits, buf2);
			checked_snprintf(o_buf + strlen(o_buf), MAX_STRING_LENGTH - strlen(o_buf),
					 "&+YExtra flags    : &N%s (%d)\n", buf2, j->extra_flags);
		}

		if (j->extra2_flags)
		{
			sprintbitde(j->extra2_flags, extra2_bits, buf2);
			checked_snprintf(o_buf + strlen(o_buf), MAX_STRING_LENGTH - strlen(o_buf),
					 "&+YExtra2 flags   : &N%s\n", buf2);
		}

		if (j->anti_flags)
		{
			*buf2 = '\0';
			for (x = 0; x < CLASS_COUNT; x++)
				if (j->anti_flags & (((unsigned long)1) << x))
					checked_snprintf(buf2 + strlen(buf2),
							 MAX_STRING_LENGTH - strlen(buf2), "%s ",
							 class_names_table[x + 1].normal);
			checked_snprintf(o_buf + strlen(o_buf), MAX_STRING_LENGTH - strlen(o_buf),
					 "&+Y%s : &N%s\n",
					 IS_SET(j->extra_flags, ITEM_ALLOWED_CLASSES) ?
						 "Allowed classes" :
						 "Denied classes",
					 buf2);
		}

		if (j->anti2_flags)
		{
			*buf2 = '\0';
			for (x = 0; x < RACE_PLAYER_MAX; x++)
				if (j->anti2_flags & (((unsigned long)1) << x))
					checked_snprintf(buf2 + strlen(buf2),
							 MAX_STRING_LENGTH - strlen(buf2), "%s ",
							 race_names_table[x + 1].no_spaces);
			checked_snprintf(o_buf + strlen(o_buf), MAX_STRING_LENGTH - strlen(o_buf),
					 "&+Y%s  : &N%s\n",
					 IS_SET(j->extra_flags, ITEM_ALLOWED_RACES) ?
						 "Allowed races" :
						 "Denied races",
					 buf2);
		}

		checked_snprintf(
			o_buf + strlen(o_buf), MAX_STRING_LENGTH - strlen(o_buf),
			"&+YWeight: &N%d&+Y lbs   Value: &N%s   &+YCondition: &N%d   &+YItem Value: &N%d\n", //%d(%d%%)\n",
			j->weight, comma_string((long)(j->cost)), j->condition, itemvalue(j));
		//, j->max_condition, (int) (((float) j->condition / j->max_condition) * 100)); wipe2011

		checked_snprintf(
			o_buf + strlen(o_buf), MAX_STRING_LENGTH - strlen(o_buf),
			"&+YT0: &n%d&+Y  T1: &n%d&+Y  T2: &n%d&+Y  T3: &n%d&+Y  T4: &n%d&+Y  T5: &n%d\n",
			(int)j->timer[0], (int)j->timer[1], (int)j->timer[2], (int)j->timer[3],
			(int)j->timer[4], (int)j->timer[5]);

		checked_snprintf(o_buf + strlen(o_buf), MAX_STRING_LENGTH - strlen(o_buf),
				 "&+YCraftsmanship: &n%s\n", craftsmanship_names[j->craftsmanship]);

		sprinttype(j->material, item_material, buf2);
		checked_snprintf(o_buf + strlen(o_buf), MAX_STRING_LENGTH - strlen(o_buf),
				 "&+YMaterial: &n%s\n", buf2);

		if (!t_obj)
		{
			strcat(o_buf, "&+YLocation: ");
			strcat(o_buf, where_obj(j, FALSE));
			strcat(o_buf, "\n");
		}
		switch (j->type)
		{
		case ITEM_LIGHT:
			snprintf(buf, MAX_STRING_LENGTH,
				 "&+YColor: [&N%d&+Y]  Type: [&N%d&+Y]  Hours: [&N%d&+Y]",
				 j->value[0], j->value[1], j->value[2]);
			break;
		case ITEM_POTION:
		case ITEM_SCROLL:
			snprintf(buf, MAX_STRING_LENGTH, "&+Y Level: &N%d&+Y  Spells:&N ",
				 j->value[0]);

			for (i = 1; (i < 4) && (j->value[i] > 0); i++)
			{
				sprinttype(j->value[i], (const char **)spells, buf2);
				checked_snprintf(buf, MAX_STRING_LENGTH, "%s%d) &+C%s [%d]&+Y, ",
						 buf, j->value[i], buf2, GetCircle(j->value[i]));
			}

			i = strlen(buf);
			if (buf[i - 2] != ',')
				strcat(buf, "&+RBUGGED!&N\n");
			else
			{
				buf[i - 2] = ',';
				buf[i - 1] = '\0';
			}

			break;
		case ITEM_STAFF:
		case ITEM_WAND:
			if (j->value[3] > 0)
				sprinttype(j->value[3], (const char **)spells, buf2);
			else
				strcpy(buf2, "&+RBUGGED!&");
			checked_snprintf(buf, MAX_STRING_LENGTH,
					 "%d(%d)&+Y charges, Level &N%d&+Y spell: %d) &+C%s [%d]&N",
					 j->value[1], j->value[2], j->value[0], j->value[3], buf2,
					 GetCircle(j->value[3]));
			break;
		case ITEM_FIREWEAPON:
			if ((j->value[3] < 1) || (j->value[3] > 6))
				strcpy(buf2, "&+RBUGGED!&N");
			else
				sprinttype(j->value[3] - 1, shot_types, buf2);
			snprintf(buf, MAX_STRING_LENGTH,
				 "&+YRange: &N%d  &+YRate of fire: &N%d  &+YMissile type: &N%d",
				 j->value[1], j->value[0], j->value[3]);
			break;
		case ITEM_WEAPON:
		{
			int spells[3];
			char spell_list[512];

			spells[0] = j->value[5] % 1000;
			spells[1] = j->value[5] % 1000000 / 1000;
			spells[2] = j->value[5] % 1000000000 / 1000000;

			if ((j->value[0] < 1) || (j->value[0] > WEAPON_NUMCHUCKS))
				strcpy(buf2, "&+RBUGGED!&N");
			else
				strcpy(buf2, weapon_types[j->value[0]].flagLong);

			if (obj_index[j->R_num].func.obj == NULL && j->value[5])
			{
				if (skills[spells[0]].name)
					strcpy(spell_list, skills[spells[0]].name);
				if (spells[1] && skills[spells[1]].name)
					APPENDF(spell_list, "&n, &+W%s", skills[spells[1]].name);
				if (spells[2] && skills[spells[2]].name)
					APPENDF(spell_list, "&n, &+W%s", skills[spells[2]].name);

				if (j->value[5] / 1000000000)
					snprintf(
						buf1, MAX_STRING_LENGTH,
						"&+YProcs one of &+W%d&+Y level &+W%s &+Yat &+W1/%d&+Y chance\n",
						j->value[6], spell_list, j->value[7]);
				else
					snprintf(
						buf1, MAX_STRING_LENGTH,
						"&+YProcs all of &+W%d&+Y level &+W%s &+Yat &+W1/%d&+Y chance\n",
						j->value[6], spell_list, j->value[7]);
			}
			else
				*buf1 = 0;
			checked_snprintf(buf, MAX_STRING_LENGTH,
					 "%s&+YType: &n%s &+Ydice: &N%dD%d&N %s", buf1, buf2,
					 j->value[1], j->value[2],
					 j->value[4] ? "&+g(Poisoned)&n" : "");
			break;
		}
		case ITEM_QUIVER:
			snprintf(
				buf, MAX_STRING_LENGTH,
				"&+YMax Capacity: &N%d  &+YCurrent No. Arrows. &N%d  &+YContainer Flags: &N%d  &+YMissile type: &N%d",
				j->value[0], j->value[3], j->value[1], j->value[2]);
			break;
		case ITEM_MISSILE:
			if ((j->value[3] < 1) || (j->value[3] > 6))
				strcpy(buf2, "&+RBUGGED!&N");
			else
				sprinttype(j->value[3] - 1, shot_types, buf2);
			snprintf(buf, MAX_STRING_LENGTH,
				 "&+YDamage: &N%dd%d&N &+YMissile Type: &n%s", j->value[1],
				 j->value[2], missile_types[j->value[3] - 1].flagLong);
			break;
		case ITEM_ARMOR:
			snprintf(buf, MAX_STRING_LENGTH,
				 "&+YAC-apply: &N%d  &+rWarmth: &N%d  &+YPrestige: &N%d",
				 j->value[0], j->value[1], j->value[2]);
			break;
		case ITEM_SHIELD:
			snprintf(buf, MAX_STRING_LENGTH, "&+YAC-apply: &N%d", j->value[3]);
			break;
		case ITEM_CONTAINER:
		case ITEM_STORAGE:
			snprintf(
				buf, MAX_STRING_LENGTH,
				"&+YHolds: &N%d  &+YLocktype: &N%d  &+YKey: &N%d  &+YSize hold: &N%d",
				j->value[0], j->value[1], j->value[2], j->value[3]);
			break;
		case ITEM_CORPSE:
			if (IS_SET(j->value[1], PC_CORPSE))
				snprintf(buf, MAX_STRING_LENGTH,
					 "&+mPlayer Corpse&n &+YHolding:&n %d &+Ylbs&N",
					 j->value[0]);
			else
				snprintf(buf, MAX_STRING_LENGTH,
					 "&+bNPC Corpse&n&+Y (&n%d&+Y) Holding:&n %d &+Ylbs&n",
					 j->value[3], j->value[0]);
			break;
		case ITEM_DRINKCON:
			sprinttype(j->value[2], drinks, buf2);
			checked_snprintf(
				buf, MAX_STRING_LENGTH,
				"&+YHolds: &N%d  &+YContains:&N %d  &+YPoisoned:&N %d  &+YLiquid:&N %s",
				j->value[0], j->value[1], j->value[3], buf2);
			break;
		case ITEM_NOTE:
			snprintf(buf, MAX_STRING_LENGTH, "&+YTongue:&N %d", j->value[0]);
			break;
		case ITEM_KEY:
			snprintf(buf, MAX_STRING_LENGTH,
				 "&+YKeytype:&N %3d   &+YBreak Percentage:&n %d%%", j->value[0],
				 j->value[1]);
			break;
		case ITEM_FOOD:
			snprintf(buf, MAX_STRING_LENGTH, "&+YMakes full:&N %d  &+YPoisoned:&N %d",
				 j->value[0], j->value[3]);
			break;
		case ITEM_MONEY:
			snprintf(
				buf, MAX_STRING_LENGTH,
				"&+YCopper:&N %d  &+YSilver:&N %d  &+YGold:&N %d  &+YPlatinum:&N %d",
				j->value[0], j->value[1], j->value[2], j->value[3]);
			break;
		case ITEM_WORN:
			snprintf(buf, MAX_STRING_LENGTH,
				 "&+rWarmth:&N %d  &+YPrestige:&N %d  &+YMaterial:&n %d",
				 j->value[1], j->value[2], j->value[3]);
			break;
		case ITEM_TELEPORT:
			i = real_room(j->value[0]);
			snprintf(buf, MAX_STRING_LENGTH,
				 "&+YTo room: [&N%d&+Y]&N %s\n"
				 "&+YCommand #: [&N%d&+Y]  Charges: [&N%d&+Y]  Zone-to: [&N%d&+Y]",
				 j->value[0],
				 ((i > 1) && (i <= top_of_world)) ?
					 world[real_room(j->value[0])].name :
					 "",
				 j->value[1], j->value[2], j->value[3]);
			break;
		case ITEM_BANDAGE:
			snprintf(buf, MAX_STRING_LENGTH, "&+YHeals : &n%d&n", j->value[0]);
			break;
		default:
			snprintf(
				buf, MAX_STRING_LENGTH,
				"&+YValues 0-7: [&N%d&+Y] [&N%d&+Y] [&N%d&+Y] [&N%d&+Y] [&n%d&+Y] [&n%d&+Y] [&n%d&+Y] [&n%d&+Y]",
				j->value[0], j->value[1], j->value[2], j->value[3], j->value[4],
				j->value[5], j->value[6], j->value[7]);
			showed_vals = TRUE;

			break;
		}
		strcat(o_buf, buf);

		if (!showed_vals)
			snprintf(
				buf, MAX_STRING_LENGTH,
				"\n&+YValues 0-7: [&N%d&+Y] [&N%d&+Y] [&N%d&+Y] [&N%d&+Y] [&n%d&+Y] [&n%d&+Y] [&n%d&+Y] [&n%d&+Y]",
				j->value[0], j->value[1], j->value[2], j->value[3], j->value[4],
				j->value[5], j->value[6], j->value[7]);
		else
			buf[0] = '\0';

		strcat(o_buf, buf);

		snprintf(buf, MAX_STRING_LENGTH, "\n&+YSpecial procedure:&N ");
		if (j->R_num >= 0)
			strcat(buf,
			       (obj_index[j->R_num].func.obj ?
					get_function_name((void *)obj_index[j->R_num].func.obj) :
					"No"));
		else
			strcat(buf, "No");
		strcat(buf, "\n");

		strcat(o_buf, buf);
		/*
		strcat(buf, "\n&+YGod procedure:&N ");
		if(j->R_num >= 0)
		  strcat(buf, (obj_index[j->R_num].god_func ? "exists\n" : "No\n"));
		else
		  strcat(buf, "No\n");

		*/

		for (i = 0; i < MAX_OBJ_AFFECT; i++)
		{
			if (j->affected[i].location != APPLY_NONE)
			{
				sprinttype(j->affected[i].location, apply_types, buf2);
				checked_snprintf(o_buf + strlen(o_buf),
						 MAX_STRING_LENGTH - strlen(o_buf),
						 "   &+YAffects: &+c%s&+y By &N%d\n", buf2,
						 j->affected[i].modifier);
			}
		}
		if (j->affects)
		{
			struct obj_affect *o_af;

			strcat(o_buf, "&+YAffected by: \n");
			for (o_af = j->affects; o_af; o_af = o_af->next)
			{
				if (o_af->extra2)
				{
					checked_snprintf(o_buf + strlen(o_buf),
							 MAX_STRING_LENGTH - strlen(o_buf),
							 "   &n%s &+Yfor&n %d &+Ygranting:&n ",
							 skills[o_af->type].name, (int)o_af->data);
					sprintbitde(o_af->extra2, extra2_bits,
						    o_buf + strlen(o_buf));
				}
				else
					checked_snprintf(o_buf + strlen(o_buf),
							 MAX_STRING_LENGTH - strlen(o_buf),
							 "   &n%s &+Yfor&n %d&n",
							 skills[o_af->type].name, (int)o_af->data);
				strcat(o_buf, "\n");
			}
		}
		if (j->nevents)
		{
			P_nevent ne;
			strcat(o_buf, "&+YEvents:\n&+Y-------\n");

			LOOP_EVENTS_OBJ(ne, j->nevents)
			{
				checked_snprintf(o_buf + strlen(o_buf),
						 MAX_STRING_LENGTH - strlen(o_buf),
						 "%6d&+Y seconds,&n %s&+Y.\n",
						 ne_event_time(ne) / WAIT_SEC,
						 get_function_name((void *)ne->func));
				if (ne->func == event_mob_mundane)
				{
					checked_snprintf(o_buf + strlen(o_buf),
							 MAX_STRING_LENGTH - strlen(o_buf),
							 "  &+YOffending mob: &n%s&N %d&+Y.\n",
							 (ne->ch) ? J_NAME(ne->ch) : "NULL",
							 IS_ALIVE(ne->ch) ? GET_ID(ne->ch) : -1);
				}
			}
			strcat(o_buf, "\n");
		}

		/* Since quality of an item can have some meaning now, let's add it to stat command -Alver */
		{
			int craft = j->craftsmanship;
			snprintf(buf, MAX_STRING_LENGTH, "\n&+YQuality:&N ");

			if (craft < OBJCRAFT_LOWEST || craft > OBJCRAFT_HIGHEST)
			{
				strcat(buf, "BUGGY!\n");
			}
			else
			{
				strcat(buf, craftsmanship_names[craft]);
				strcat(buf, "\n");
			}
			if (IS_ARTIFACT(j))
			{
				strcat(buf, "&+YIn game since: &n");
				strcat(buf, asctime(localtime(&(j->timer[5]))));
				strcat(buf, "\n");
			}
			strcat(o_buf, buf);
		}

		// Insert item into db
		if (cmd == 555)
		{
			sql_insert_item(ch, j, o_buf);
		}
		else
		{
			if (j->contains)
				strcat(o_buf, "\n&+YContains:\n");

			page_string(ch->desc, o_buf, 1);

			if (j->contains)
				list_obj_to_char(j->contains, ch, LISTOBJ_SHORTDESC | LISTOBJ_STATS,
						 TRUE);
		}

		if (t_obj)
		{
			extract_obj(t_obj);
			t_obj = NULL;
		}
		return;
	}
	else if ((*arg1 == 'c') || (*arg1 == 'C') || (*arg1 == 'm') || (*arg1 == 'M'))
	{
		/* mobile in world  */

		if (!*arg2)
		{
			send_to_char(STAT_SYNTAX, ch);
			return;
		}
		if (is_number(arg2))
		{
			if ((i = real_mobile(atoi(arg2))) == -1)
			{
				send_to_char("Illegal mob number.\n", ch);
				return;
			}
			/* load one to stat, extract after statting  */
			t_mob = read_mobile(i, REAL);
			if (!t_mob)
			{
				logit(LOG_DEBUG, "do_stat(): mob %d [%d] not loadable", i,
				      mob_index[i].virtual_number);
				send_to_char("error loading mob to stat.\n", ch);
				return;
			}
			else
			{
				char_to_room(t_mob, 0, -2);
			}
			if (t_mob->player.name)
				strcpy(arg2, t_mob->player.name);
		}
		if (!(k = get_char_vis(ch, arg2)))
		{
			send_to_char("No such character.\n", ch);
			if (t_mob)
			{
				extract_char(t_mob);
				t_mob = NULL;
			}
			return;
		}
		switch (k->player.sex)
		{
		case SEX_NEUTRAL:
			strcpy(buf, "Neuter");
			break;
		case SEX_MALE:
			strcpy(buf, "&+BMale&N");
			break;
		case SEX_FEMALE:
			strcpy(buf, "&+RFemale&N");
			break;
		default:
			strcpy(buf, "&+MILLEGAL-SEX!!&N");
			break;
		}

		snprintf(buf1, MAX_STRING_LENGTH,
			 "  &+YIn room: [&N%d&+Y] Zone: [&n%d&+Y](&n%d&+Y) %s",
			 world[k->in_room].number, zone_table[world[k->in_room].zone].number,
			 world[k->in_room].zone, zone_table[world[k->in_room].zone].name);
		checked_snprintf(buf2, MAX_STRING_LENGTH, "%s %s%s  ", buf,
				 (IS_PC(k) ? "&+YPC" : (IS_PC(k) ? "&+RNPC" : "&+GMOB")),
				 (t_mob) ? "" : buf1);
		if (IS_NPC(k))
		{
			snprintf(buf, MAX_STRING_LENGTH,
				 "Numbers: &N%d&+Y-V &N%d&+Y-R &N%d&+Y-I   # in game: &n%d\n",
				 mob_index[GET_RNUM(k)].virtual_number, GET_RNUM(k), GET_IDNUM(k),
				 (mob_index[GET_RNUM(k)].number));
		}
		else
		{
#ifdef USE_ACCOUNT
			snprintf(buf, MAX_STRING_LENGTH,
				 " &+YName:&n &N%s  &+YID numb: &n%d  &+YAccount Name:&n %s\n",
				 GET_NAME(k), GET_PID(k), get_account_name_safe(k));
#else
			snprintf(buf, MAX_STRING_LENGTH, " &+YName:&n &N%s  &+YID numb: &n%d\n",
				 GET_NAME(k), GET_PID(k));
#endif
		}
		strcat(buf2, buf);
		strcpy(o_buf, buf2);

		if (IS_NPC(k))
			snprintf(buf, MAX_STRING_LENGTH,
				 "&+YName: &N%s\n&+YKeywords: &N%s\n&+YDescription:\n%s\n",
				 k->player.short_descr, GET_NAME(k), k->player.long_descr);
		else
			snprintf(
				buf, MAX_STRING_LENGTH,
				"&+YTitle: &N%s\n&+YShort Description:&n%s\n&+YLong Description:\n%s\n",
				/*              k->only.pc->title ? k->only.pc->title : "&+rNone", */
				k->player.title ? k->player.title : "&+rNone",
				k->player.short_descr ? k->player.short_descr : "&+rNone",
				k->player.description ? k->player.description : "&+rNone");
		strcat(o_buf, buf);

		if (IS_NPC(k))
		{
			// snprintf(buf2, MAX_STRING_LENGTH, "&+Y+(&N%s&+Y)", comma_string((long) (GET_LEVEL(k) * GET_HIT(k) * .4)));
			buf2[0] = '\0';
		}
		else
		{
			if (k->player.m_class == 0 || k->player.m_class > (1 << CLASS_COUNT) - 1 ||
			    GET_LEVEL(k) < 1 || IS_TRUSTED(k))
				strcpy(buf1, "Unknown");
			else
				strcpy(buf1, comma_string((long)(new_exp_table[GET_LEVEL(k) + 1] -
								 GET_EXP(k))));
			checked_snprintf(buf2, MAX_STRING_LENGTH, "&+Y Exp to Level: &N%s",
					 IS_TRUSTED(k) ? "Unknown" : buf1);
		}

		checked_snprintf(
			buf, MAX_STRING_LENGTH,
			"&+YLevel: &N%d&+Y(&n%d&+Y)&n  &+YExperience: &N%s %s  &+YAlignment [&N%d&+Y] Assoc:&n %d %s\n",
			k->player.level, IS_PC(k) ? k->only.pc->highest_level : GET_LEVEL(k),
			comma_string((int)GET_EXP(k)), buf2, GET_ALIGNMENT(k),
			(GET_ASSOC(k) == NULL) ? -1 : GET_ASSOC(k)->get_id(),
			(GET_ASSOC(k) == NULL) ? "" : GET_ASSOC(k)->get_name().c_str());
		strcat(o_buf, buf);

		snprintf(buf, MAX_STRING_LENGTH, "&+YRace: &N%s  &+YClass: &N",
			 race_names_table[k->player.race].ansi);
		get_class_string(k, buf2);
		strcat(buf, buf2);
		APPENDF(buf, " &+YRacewar: ");
		if (IS_NPC(k))
			APPENDF(buf, "&+wNPC&n");
		else if (GET_RACEWAR(k) >= 0 && GET_RACEWAR(k) <= MAX_RACEWAR)
			APPENDF(buf, "&+%c%s&N", racewar_color[GET_RACEWAR(k)].color,
				racewar_color[GET_RACEWAR(k)].name);
		else
			APPENDF(buf, "&+RINVALID&n");

		snprintf(
			buf2, MAX_STRING_LENGTH,
			"\n&+YHometown: &N%d  &+YBirthplace: &N%d  &+YOrig BP: &n%d &+YSpell Pulse: &n%+.2f\n",
			GET_HOME(k), GET_BIRTHPLACE(k), GET_ORIG_BIRTHPLACE(k),
			spell_pulse_data[GET_RACE(k)] * SPELL_PULSE(k));
		strcat(buf, buf2);
		strcat(o_buf, buf);
		if (IS_PC(k))
			fragnum = (float)k->only.pc->frags;
		else
			fragnum = 0;
		fragnum /= 100;

		snprintf(
			buf, MAX_STRING_LENGTH,
			"&+YPulse: &N%4d&+Y  Current Pulse: &N%4d&+Y  Dam Multiplier: &N%1.2f  &+YFrags:&n %+.02f\n",
			(int)k->specials.base_combat_round, k->specials.combat_tics,
			k->specials.damage_mod, fragnum);
		strcat(o_buf, buf);

		strcat(o_buf, "\n");

		if (IS_PC(k))
		{
			struct affected_type *paf = get_spell_from_char(ch, TAG_EPICS_GAINED);

			snprintf(buf, MAX_STRING_LENGTH,
				 "&+YEpic points: &n%ld&+Y  Total epics gained: &n%d\n",
				 k->only.pc->epics, paf ? paf->modifier : 0);
			strcat(o_buf, buf);

			snprintf(
				buf, MAX_STRING_LENGTH,
				"&+YAge: &N%4d &+Yyears  &N%2d &+Ymonths  &N%2d &+Ydays  &N%2d &+YHours\n",
				age(k).year, age(k).month, age(k).day, age(k).hour);
			strcat(o_buf, buf);

			playing_time =
#ifndef EQ_WIPE
				real_time_passed((long)(k->player.time.played +
							(time(0) - k->player.time.logon)),
						 0);
#else
				real_time_passed((long)(k->player.time.played - EQ_WIPE +
							(time(0) - k->player.time.logon)),
						 0);
#endif
			snprintf(buf, MAX_STRING_LENGTH,
				 "&+YPlayed:  &N%3d &+Ydays  &N%2d &+Yhours  &N%2d &+Yminutes\n",
				 playing_time.day, playing_time.hour, playing_time.minute);
			strcat(o_buf, buf);

			playing_time = real_time_passed(time(0), k->player.time.logon);
			snprintf(buf, MAX_STRING_LENGTH,
				 "&+YSession: &N%3d &+Ydays  &N%2d &+Yhours  &N%2d &+Yminutes\n",
				 playing_time.day, playing_time.hour, playing_time.minute);
			strcat(o_buf, buf);
		}
		else
		{
			playing_time = real_time_passed(time(0), k->player.time.birth);
			snprintf(buf, MAX_STRING_LENGTH,
				 "&+YLived: &N%2d &+Ydays  &N%2d &+Yhours  &N%2d &+Yminutes\n",
				 playing_time.day, playing_time.hour, playing_time.minute);
			strcat(o_buf, buf);
		}

		strcat(o_buf, "      &+gCur (Bas)      Cur (Bas)\n");

		for (i = 0, i3 = 0; i < MAX_WEAR; i++)
			if (k->equipment[i])
				i3++;
		i2 = GET_HEIGHT(k);
		i = i2 / 12;
		i2 -= i * 12;

		snprintf(
			buf, MAX_STRING_LENGTH,
			"&+YStr: &n%3d&+Y (&n%3d&+Y)    Pow: &n%3d&+Y (&n%3d&+Y)    Height: &n%3d&+Y\' &n%2d&+Y\" (&n%d&+Yin)\n",
			GET_C_STR(k), k->base_stats.Str, GET_C_POW(k), k->base_stats.Pow, i, i2,
			GET_HEIGHT(k));
		strcat(o_buf, buf);

		snprintf(
			buf, MAX_STRING_LENGTH,
			"&+YDex: &n%3d&+Y (&n%3d&+Y)    Int: &n%3d&+Y (&n%3d&+Y)    Weight: &n%3d&+Y lbs\n",
			GET_C_DEX(k), k->base_stats.Dex, GET_C_INT(k), k->base_stats.Int,
			GET_WEIGHT(k));
		strcat(o_buf, buf);

		sprinttype(GET_ALT_SIZE(k), size_types, buf2);
		checked_snprintf(
			buf, MAX_STRING_LENGTH,
			"&+YAgi: &n%3d&+Y (&n%3d&+Y)    Wis: &n%3d&+Y (&n%3d&+Y)    Size: &n%s&+Y\n",
			GET_C_AGI(k), k->base_stats.Agi, GET_C_WIS(k), k->base_stats.Wis, buf2);
		strcat(o_buf, buf);

		snprintf(
			buf, MAX_STRING_LENGTH,
			"&+YCon: &n%3d&+Y (&n%3d&+Y)    Cha: &n%3d&+Y (&n%3d&+Y)    Equipped Items:&n%3d&+Y     Carried weight:&n%5d\n",
			GET_C_CON(k), k->base_stats.Con, GET_C_CHA(k), k->base_stats.Cha, i3,
			total_carried_weight(k));
		strcat(o_buf, buf);

		snprintf(
			buf, MAX_STRING_LENGTH,
			"&+YKar: &n%3d&+Y (&n%3d&+Y)    Luc: &n%3d&+Y (&n%3d&+Y)    Carried Items: &n%3d&+Y   Max Carry Weight:&n%5d\n",
			GET_C_KAR(k), k->base_stats.Kar, GET_C_LUK(k), k->base_stats.Luk,
			IS_CARRYING_N(k), CAN_CARRY_W(k));
		strcat(o_buf, buf);

		i = GET_C_STR(k) + GET_C_DEX(k) + GET_C_AGI(k) + GET_C_CON(k) + GET_C_POW(k) +
		    GET_C_INT(k) + GET_C_WIS(k) + GET_C_CHA(k);

		i2 = k->base_stats.Str + k->base_stats.Dex + k->base_stats.Agi + k->base_stats.Con +
		     k->base_stats.Pow + k->base_stats.Int + k->base_stats.Wis + k->base_stats.Cha;

		snprintf(
			buf, MAX_STRING_LENGTH,
			"&+YAvg: &n%3d&+Y (&n%3d&+Y)  Total mod: (&n%3d&+Y)              Load modifer: &n%3d\n\n",
			(int)(i / 8), (int)(i2 / 8), (i - i2), load_modifier(k));
		strcat(o_buf, buf);

		/*
		 * Print out NPC spell slot information: # spells left in each circle and # of
		 * spells left to regain overall. - SKB 7 Apr 1995
		 */

		if (IS_NPC(k) || IS_PUNDEAD(k) || GET_CLASS(k, CLASS_DRUID))
		{
			snprintf(buf, MAX_STRING_LENGTH,
				 "&+mSpells left in circles:  (%d to regain)\n&+m",
				 k->specials.undead_spell_slots[0]);
			strcat(o_buf, buf);

			for (i4 = 1; i4 < MAX_CIRCLE + 1; i4++)
			{
				snprintf(buf, MAX_STRING_LENGTH, "%d:%d/%d", i4,
					 k->specials.undead_spell_slots[i4],
					 spl_table[GET_LEVEL(k)][i4 - 1]);
				checked_snprintf(o_buf + strlen(o_buf),
						 MAX_STRING_LENGTH - strlen(o_buf), "%-8s", buf);
			}
			strcat(o_buf, "\n\n");
		}
		snprintf(buf, MAX_STRING_LENGTH,
			 "&+YHits: [&N%5d&+Y/&N%5d&+Y/&N%5d&+Y+&N%3d&+Y]&+W   Pcoins: &N%5d",
			 GET_HIT(k), GET_MAX_HIT(k), k->points.base_hit, hit_regen(k, TRUE),
			 GET_PLATINUM(k));
		if (IS_PC(k))
			checked_snprintf(buf, MAX_STRING_LENGTH, "%s  &+WPbank: &N%5d\n", buf,
					 GET_BALANCE_PLATINUM(k));
		else
			checked_snprintf(buf, MAX_STRING_LENGTH, "%-52s  &+YTimer: &N%d\n", buf,
					 k->specials.timer);
		strcat(o_buf, buf);

		snprintf(buf, MAX_STRING_LENGTH,
			 "&+YMana: [&N%5d&+Y/&N%5d&+Y/&N%5d&+Y+&N%3d&+Y]   Gcoins: &N%5d",
			 GET_MANA(k), GET_MAX_MANA(k), k->points.base_mana, mana_regen(k, TRUE),
			 GET_GOLD(k));

		if (IS_PC(k))
			checked_snprintf(buf, MAX_STRING_LENGTH, "%s  &+YGbank: &N%5d\n", buf,
					 GET_BALANCE_GOLD(k));
		else
			checked_snprintf(buf, MAX_STRING_LENGTH, "%-50s  &+YSpecial: &N%s\n", buf,
					 (mob_index[GET_RNUM(k)].func.mob ?
						  get_function_name(
							  (void *)mob_index[GET_RNUM(k)].func.mob) :
						  "None"));
		strcat(o_buf, buf);

		snprintf(buf, MAX_STRING_LENGTH,
			 "&+YMove: [&N%5d&+Y/&N%5d&+Y/&N%5d&+Y+&N%3d&+Y]&n   Scoins: %5d",
			 GET_VITALITY(k), GET_MAX_VITALITY(k), vitality_limit(k),
			 move_regen(k, TRUE), GET_SILVER(k));
		if (IS_PC(k))
			checked_snprintf(buf, MAX_STRING_LENGTH, "%s  &nSbank: %5d\n", buf,
					 GET_BALANCE_SILVER(k));
		else if ((qi = find_quester_id(GET_RNUM(k))) >= 0)
		{
			checked_snprintf(buf, MAX_STRING_LENGTH, "%-52s  &+YQuest: &N%s\n", buf,
					 mob_index[GET_RNUM(k)].qst_func ?
						 (has_quest_complete(qi) ?
							  "&+BComplete&N" :
							  (has_quest_ask(qi) ? "&+RAsk&N" :
									       "&+RRoomMsg&n")) :
						 "None");
		}
		strcat(o_buf, buf);

		snprintf(buf, MAX_STRING_LENGTH, "                                &+yCcoins: &N%5d",
			 GET_COPPER(k));
		if (IS_PC(k))
			checked_snprintf(buf, MAX_STRING_LENGTH, "%s  &+yCbank: &N%5d\n", buf,
					 GET_BALANCE_COPPER(k));
		else
			strcat(buf, "\n");
		strcat(o_buf, buf);

		//    i = calculate_ac(k, FALSE);
		//    snprintf(buf, MAX_STRING_LENGTH, "&+cAgility Armor Class: &+Y%d&n  ", i);
		//    strcat(o_buf, buf);

		i = calculate_ac(k); //, TRUE);  wipe 2011

		if (i > 0)
			snprintf(
				buf, MAX_STRING_LENGTH,
				"&+cTotal Armor Class: &+Y%d&n,  Increases melee damage by &+W%+.2f&n percent.\n",
				i, (double)(i * 0.10));
		else
			snprintf(
				buf, MAX_STRING_LENGTH,
				"&+cTotal Armor Class: &+Y%d&n,  Reduces melee damage by &+W%+.2f&n.\n",
				i, (double)(i * 0.10));

		strcat(o_buf, buf);

		i2 = calculate_thac_zero(k, 100); // Assumes 100 weapon skill.

		snprintf(buf, MAX_STRING_LENGTH, "&+Y thAC0: &N%d &+Y  +Hit: &N%d", i2,
			 GET_HITROLL(k) + str_app[STAT_INDEX(GET_C_STR(k))].tohit);
		if (IS_NPC(k) || (GET_CLASS(k, CLASS_MONK) && !k->equipment[WIELD] &&
				  !k->equipment[WEAR_SHIELD] && !k->equipment[HOLD] &&
				  !k->equipment[SECONDARY_WEAPON]))
		{
			checked_snprintf(buf, MAX_STRING_LENGTH,
					 "%s   &+YUnarmed damage: &N%d&+Yd&N%d  &+Y+Dam: &N%d\n",
					 buf, k->points.damnodice, k->points.damsizedice,
					 TRUE_DAMROLL(k));
		}
		else
		{
			checked_snprintf(buf, MAX_STRING_LENGTH, "%s  &+Y+Dam: &N%d+%d = %d\n", buf,
					 GET_DAMROLL(k), str_app[STAT_INDEX(GET_C_STR(k))].todam,
					 TRUE_DAMROLL(k));
		}
		strcat(o_buf, buf);

		strcat(o_buf, "&+YSaves:    Para   Wands  Fear   Breath Spell\n");
		snprintf(
			buf, MAX_STRING_LENGTH,
			"&+Y (actual) [&N%3d&+Y]  [&N%3d&+Y]  [&N%3d&+Y]  [&N%3d&+Y]  [&N%3d&+Y]\n",
			BOUNDED(1,
				(find_save(k, SAVING_PARA) + k->specials.apply_saving_throw[0] * 5),
				100),
			BOUNDED(1,
				(find_save(k, SAVING_ROD) + k->specials.apply_saving_throw[1] * 5),
				100),
			BOUNDED(1,
				(find_save(k, SAVING_FEAR) + k->specials.apply_saving_throw[2] * 5),
				100),
			BOUNDED(1,
				(find_save(k, SAVING_BREATH) +
				 k->specials.apply_saving_throw[3] * 5),
				100),
			BOUNDED(1,
				(find_save(k, SAVING_SPELL) + k->specials.apply_saving_throw[4] * 5),
				100));
		strcat(o_buf, buf);
		snprintf(
			buf, MAX_STRING_LENGTH,
			"&+Y (mods)   [&N%3d&+Y]  [&N%3d&+Y]  [&N%3d&+Y]  [&N%3d&+Y]  [&N%3d&+Y]\n",
			k->specials.apply_saving_throw[0], k->specials.apply_saving_throw[1],
			k->specials.apply_saving_throw[2], k->specials.apply_saving_throw[3],
			k->specials.apply_saving_throw[4]);
		strcat(o_buf, buf);

		strcat(o_buf, "\n");

		if (IS_PC(k))
		{
			if (k->desc)
				sprinttype(k->desc->connected, connected_types, buf2);
			else
				strcpy(buf2, "");
			checked_snprintf(
				buf, MAX_STRING_LENGTH,
				"&+YHunger: &N%2d  &+YThirst: &N%2d  &+YDrunk: &N%2d &+Y%s%s\n",
				k->specials.conditions[FULL], k->specials.conditions[THIRST],
				k->specials.conditions
					[DRUNK], // k->only.pc->justice_level, /* &+YJustice Level: &N%d */  wipe2011
				(k->desc) ? "Connected: " : "Linkdead", buf2);
			strcat(o_buf, buf);
		}
		else
		{
			strcat(o_buf, "&+YValues: ");

			for (i4 = 0; i4 < NUMB_CHAR_VALS; i4++)
			{
				snprintf(buf, MAX_STRING_LENGTH, "&+Y[&n%d&+Y] ",
					 k->only.npc->value[i4]);
				strcat(o_buf, buf);
			}

			strcat(o_buf, "\n\n");
		}

		sprinttype(GET_POS(k), position_types, buf1);
		strcat(buf1, " ");
		sprintbit((GET_STAT(k) * 4), position_types, buf1 + strlen(buf1));
		if (IS_NPC(k))
		{
			sprinttype((k->only.npc->default_pos & 3), position_types, buf2);
			strcat(buf2, " ");
			sprintbit(((k->only.npc->default_pos & STAT_MASK) * 4), position_types,
				  buf2 + strlen(buf2));
			checked_snprintf(buf, MAX_STRING_LENGTH,
					 "&+YPosition/Default: &N%s&+Y/&N%s", buf1, buf2);
		}
		else
			checked_snprintf(buf, MAX_STRING_LENGTH, "&+YPosition: &N%s", buf1);
		checked_snprintf(buf1, MAX_STRING_LENGTH, "%s  &+YFighting:&n %s", buf,
				 ((GET_OPPONENT(k)) ? GET_NAME(k->specials.fighting) : "---"));
		if (IS_NPC(k))
		{
			strcat(buf1, "\n");
			strcpy(buf, buf1);
		}
		else
			checked_snprintf(buf, MAX_STRING_LENGTH, "%-61s  &+YTimer: &N%d\n", buf1,
					 k->specials.timer);
		strcat(o_buf, buf);

		if (IS_NPC(k))
		{
			snprintf(buf, MAX_STRING_LENGTH, "&+YJustice hooks: &N%d\n",
				 k->only.npc->spec[2]);
			strcat(o_buf, buf);
			sprintbitde(k->specials.act, action_bits, buf2);
			APPENDF(buf, "&+YACT flags: &N%s\n", buf2);
			sprintbitde(k->specials.act2, action2_bits, buf2);
			APPENDF(buf, "&+YACT2 flags: &N%s\n", buf2);
			sprintbitde(k->only.npc->aggro_flags, aggro_bits, buf2);
			APPENDF(buf, "&+YAggro    : &n%s\n", buf2);
			sprintbitde(k->only.npc->aggro2_flags, aggro2_bits, buf2);
			APPENDF(buf, "&+YAggro2   : &n%s\n", buf2);
			sprintbitde(k->only.npc->aggro3_flags, aggro3_bits, buf2);
			APPENDF(buf, "&+YAggro3   : &n%s\n", buf2);
			strcat(o_buf, buf);
		}
		else
		{
			sprintbit(k->only.pc->prompt, player_prompt, buf2);
			checked_snprintf(buf, MAX_STRING_LENGTH, "&+YPrompt: &N%s\n", buf2);
			strcat(o_buf, buf);
			sprintbit(k->specials.act, player_bits, buf2);
			checked_snprintf(buf, MAX_STRING_LENGTH, "&+YAct1: &N%s\n", buf2);
			strcat(o_buf, buf);
			sprintbit(k->specials.act2, player2_bits, buf2);
			checked_snprintf(buf, MAX_STRING_LENGTH, "&+YAct2: &N%s\n", buf2);
			strcat(o_buf, buf);
			sprintbit(k->specials.act3, player3_bits, buf2);
			checked_snprintf(buf, MAX_STRING_LENGTH, "&+YAct3: &N%s\n", buf2);
			strcat(o_buf, buf);
		}
		if (k->specials.affected_by)
		{
			sprintbitde(k->specials.affected_by, affected1_bits, buf2);
			checked_snprintf(buf, MAX_STRING_LENGTH,
					 "&+YAffected by (1):&n %10lu - %s\n",
					 k->specials.affected_by, buf2);
			strcat(o_buf, buf);
		}

		if (k->specials.affected_by2)
		{
			sprintbitde(k->specials.affected_by2, affected2_bits, buf2);
			checked_snprintf(buf, MAX_STRING_LENGTH,
					 "&+YAffected by (2):&n %10lu - %s\n",
					 k->specials.affected_by2, buf2);
			strcat(o_buf, buf);
		}

		if (k->specials.affected_by3)
		{
			sprintbitde(k->specials.affected_by3, affected3_bits, buf2);
			checked_snprintf(buf, MAX_STRING_LENGTH,
					 "&+YAffected by (3):&n %10lu - %s\n",
					 k->specials.affected_by3, buf2);
			strcat(o_buf, buf);
		}

		if (k->specials.affected_by4)
		{
			sprintbitde(k->specials.affected_by4, affected4_bits, buf2);
			checked_snprintf(buf, MAX_STRING_LENGTH,
					 "&+YAffected by (4):&n %10lu - %s\n",
					 k->specials.affected_by4, buf2);
			strcat(o_buf, buf);
		}

		if (k->specials.affected_by5)
		{
			sprintbitde(k->specials.affected_by5, affected5_bits, buf2);
			checked_snprintf(buf, MAX_STRING_LENGTH,
					 "&+YAffected by (5):&n %10lu - %s\n",
					 k->specials.affected_by5, buf2);
			strcat(o_buf, buf);
		}

		snprintf(buf, MAX_STRING_LENGTH,
			 "&+YFollowers:           &+YMaster is: &N%s   &+YRank: &n%s\n",
			 ((k->following) ? GET_NAME(k->following) : "---"),
			 (k->group ? (IS_BACKRANKED(k) ? "Back" : "Front") : "---"));
		strcat(o_buf, buf);
		for (fol = k->followers; fol; fol = fol->next)
		{
			snprintf(buf, MAX_STRING_LENGTH, "  %s\n",
				 IS_NPC(fol->follower) ? fol->follower->player.short_descr :
							 GET_NAME(fol->follower));
			strcat(o_buf, buf);
		}

		/* Show player on mobs piss list */
		if (IS_NPC(k) && IS_SET(k->specials.act, ACT_MEMORY))
		{
			snprintf(buf, MAX_STRING_LENGTH,
				 "&+RPissed List&n:\n&+Y--------------&n\n");

			// rebuild this for new memory system
			mem = k->only.npc->memory;
			while (mem)
			{
				snprintf(buf2, MAX_STRING_LENGTH, "  %10u\n", mem->pcID);
				strcat(buf, buf2);

				mem = mem->next;
			}

			strcat(buf, "\n");
			strcat(o_buf, buf);
		}

		strcat(o_buf, "\n");

		if (IS_PC(k))
		{
			snprintf(
				buf, MAX_STRING_LENGTH,
				"&+YTimers: T[0] = &N%10ld&+Y, T[1] = &N%10ld&+Y, T[2] = &N%10ld&+Y, T[3] = &N%10ld&+Y, T[4] = &N%10ld&+Y,\n"
				"&+Y        T[5] = &N%10ld&+Y, T[6] = &N%10ld&+Y, T[7] = &N%10ld&+Y, T[8] = &N%10ld&+Y, T[9] = &N%10ld&+Y.&N\n",
				k->only.pc->pc_timer[0], k->only.pc->pc_timer[1],
				k->only.pc->pc_timer[2], k->only.pc->pc_timer[3],
				k->only.pc->pc_timer[4], k->only.pc->pc_timer[5],
				k->only.pc->pc_timer[6], k->only.pc->pc_timer[7],
				k->only.pc->pc_timer[8], k->only.pc->pc_timer[9]);
			strcat(o_buf, buf);

			now = time(NULL);
			snprintf(
				buf, MAX_STRING_LENGTH,
				"&+YTimers(left): T[STAT_POOL] = &N%8ld&+Y, T[FLEE]    = &N%8ld&+Y, T[HEAVEN] = &N%8ld&+Y,\n"
				"&+Y              T[AVATAR]    = &N%8ld&+Y, T[SBEACON] = &N%8ld&+Y.\n\n",
				(k->only.pc->pc_timer[PC_TIMER_STAT_POOL] > now) ?
					k->only.pc->pc_timer[PC_TIMER_STAT_POOL] - now :
					0,
				(k->only.pc->pc_timer[PC_TIMER_FLEE] > now) ?
					k->only.pc->pc_timer[PC_TIMER_FLEE] - now :
					0,
				(k->only.pc->pc_timer[PC_TIMER_HEAVEN] > now) ?
					k->only.pc->pc_timer[PC_TIMER_HEAVEN] - now :
					0,
				(k->only.pc->pc_timer[PC_TIMER_AVATAR] > now) ?
					k->only.pc->pc_timer[PC_TIMER_AVATAR] - now :
					0,
				(k->only.pc->pc_timer[PC_TIMER_SBEACON] > now) ?
					k->only.pc->pc_timer[PC_TIMER_SBEACON] - now :
					0);
			strcat(o_buf, buf);
		}

		if (k->affected)
		{
			strcat(o_buf, "&+YAffecting Spells:\n&+Y-----------------\n");
			for (aff = k->affected; aff; aff = aff->next)
			{
				if (aff->type == TAG_MEMORIZE)
				{
					snprintf(buf, MAX_STRING_LENGTH,
						 "  %sMEMORIZED &+Yspell&n %s%s&n\n",
						 (aff->flags & AFFTYPE_CUSTOM1) ? "  " : "UN",
						 (aff->flags & AFFTYPE_CUSTOM1) ? "&+W" : "&+w",
						 skills[aff->modifier].name);
					strcat(o_buf, buf);
					continue;
				}

				snprintf(buf, MAX_STRING_LENGTH,
					 "%13s &+Yby &N%4d &+Yfor &N%3d &+Yfrom &N'%s'",
					 IS_SET(aff->flags, AFFTYPE_NOAPPLY) ?
						 "NONE" :
						 apply_types[(int)aff->location],
					 aff->modifier, aff->duration,
					 (skills[aff->type].name) ? skills[aff->type].name :
								    "Nameless Type");
				*buf2 = '\0';

				if (aff->bitvector)
					sprintbitde(aff->bitvector, affected1_bits, buf2);

				if (aff->bitvector2)
				{
					sprintbitde(aff->bitvector2, affected2_bits, buf1);
					strcat(buf2, buf1);
				}

				if (aff->bitvector3)
				{
					sprintbitde(aff->bitvector3, affected3_bits, buf1);
					strcat(buf2, buf1);
				}

				if (aff->bitvector4)
				{
					sprintbitde(aff->bitvector4, affected4_bits, buf1);
					strcat(buf2, buf1);
				}

				if (aff->bitvector5)
				{
					sprintbitde(aff->bitvector5, affected5_bits, buf1);
					strcat(buf2, buf1);
				}

				if (*buf2 != '\0' && !IS_SET(aff->flags, AFFTYPE_NOAPPLY))
				{
					buf1[0] = 0;
					checked_snprintf(buf1, MAX_STRING_LENGTH,
							 "%-61s &+YSets: &N%s\n\n", buf, buf2);
					strcat(o_buf, buf1);
				}
				else
				{
					strcat(buf, "\n\n");
					strcat(o_buf, buf);
				}
			}
		}
		if (k->nevents)
		{
			P_nevent ne;
			strcat(o_buf, "&+YEvents:\n&+Y-------\n");

			LOOP_EVENTS_CH(ne, k->nevents)
			{
				checked_snprintf(o_buf + strlen(o_buf),
						 MAX_STRING_LENGTH - strlen(o_buf),
						 "%6d&+Y seconds,&n %s",
						 ne_event_time(ne) / WAIT_SEC,
						 get_function_name((void *)ne->func));
				if (ne->func == event_short_affect)
					checked_snprintf(
						o_buf + strlen(o_buf),
						MAX_STRING_LENGTH - strlen(o_buf), " - %s&+Y.\n",
						(ne->data == NULL ||
						 ((event_short_affect_data *)ne->data)->af ==
							 NULL) ?
							"No affect" :
							skills[((event_short_affect_data *)ne->data)
								       ->af->type]
								.name);
				else
					checked_snprintf(o_buf + strlen(o_buf),
							 MAX_STRING_LENGTH - strlen(o_buf),
							 "&+Y.\n");
			}
			strcat(o_buf, "\n");
		}

		if (k->linking || k->linked || k->obj_linked)
		{
			struct char_link_data *link;
			struct char_obj_link_data *olink;

			strcat(o_buf, "&+YLinks:\n&+Y-------\n");
			for (link = k->linking; link; link = link->next_linking)
			{
				checked_snprintf(o_buf + strlen(o_buf),
						 MAX_STRING_LENGTH - strlen(o_buf),
						 "%s (%s): &+Ylinked to&n %s.\n",
						 link_types[link->type].name, "master",
						 link->linked->player.name);
			}
			for (link = k->linked; link; link = link->next_linked)
			{
				checked_snprintf(o_buf + strlen(o_buf),
						 MAX_STRING_LENGTH - strlen(o_buf),
						 "%s (%s): &+Ylinked to&n %s.\n",
						 link_types[link->type].name, "slave",
						 link->linking->player.name);
			}
			for (olink = k->obj_linked; olink; olink = olink->next)
			{
				checked_snprintf(o_buf + strlen(o_buf),
						 MAX_STRING_LENGTH - strlen(o_buf),
						 "%s: &+Ylinked to&n %s - %s.\n",
						 link_types[olink->type].name,
						 OBJ_SHORT(olink->obj),
						 (olink->affect == NULL) ?
							 "no affect" :
							 ((skills[olink->affect->type].name) ?
								  skills[olink->affect->type].name :
								  "Nameless Type"));
			}
			strcat(o_buf, "\n");
		}
		if (IS_PC(k))
		{
			strcat(o_buf, "&+YGuild:\n&+Y-------\n");

			if (GET_TIME_LEFT_GUILD(k) > 0)
			{
				timestr = asctime(localtime(&(GET_TIME_LEFT_GUILD(k))));
				*(timestr + 10) = 0;
				strcpy(time_left, timestr);
			}
			else
			{
				strcpy(time_left, "None");
			}
			snprintf(buf, MAX_STRING_LENGTH, "&+YDate left last guild : &N%s\n",
				 time_left);
			strcat(o_buf, buf);
			snprintf(buf, MAX_STRING_LENGTH, "&+YNumber of guild left : &N%d\n",
				 GET_NB_LEFT_GUILD(k));
			strcat(o_buf, buf);
			strcat(o_buf, "\n");
		}
		if (IS_PC(k) && IS_DISGUISE(k))
		{
			strcat(o_buf, "&+YDisguise:\n&+Y-------\n");
			snprintf(buf, MAX_STRING_LENGTH, "&+mDisguise as : &N%s\n",
				 k->disguise.name);
			strcat(o_buf, buf);
		}
		page_string(ch->desc, o_buf, 1);
		if (t_mob)
		{
			extract_char(t_mob);
			t_mob = NULL;
		}
		/* Trap data. Rather than clog do_stat anymore, we'll just pass info on */
	}
	else if (LOWER(arg1[0]) == 't' && LOWER(arg1[1]) == 'r')
	{
		do_trapstat(ch, arg2, 0);
	}
	// old guildhalls (deprecated)
	//  else if((*arg1 == 'h') || (*arg1 == 'H'))
	//  {
	//    do_stathouse(ch, arg2, 0);
	//  }
	else if ((*arg1 == 's') || (*arg1 == 'S'))
	{
		if ((arg1[1] == 'k') || (arg1[1] == 'K'))
		{
			stat_skill(ch, arg2);
			return;
		}

		/* shop data on a mobile in world, similar to statting a mobile,
		   but gives info on the shop proc, rather than the mob.  Due to
		   the way things are setup, the mob must have been loaded by a
		   zone command at bootup (or the shop is not setup properly).
		   Even if mob has been killed/purged, you can stat-by-number. */

		if (!*arg2)
		{
			send_to_char(STAT_SYNTAX, ch);
			return;
		}
		if (is_number(arg2))
		{
			if ((i = real_mobile0(atoi(arg2))) == 0)
			{
				send_to_char("Illegal mob number.\n", ch);
				return;
			}
			/* load one to stat, extract after statting  */
			t_mob = read_mobile(i, REAL);

			if (!t_mob)
			{
				logit(LOG_DEBUG, "do_stat(): mob %d [%d] not loadable", i,
				      mob_index[i].virtual_number);
				send_to_char("error loading mob to stat.\n", ch);
				return;
			}
			else
			{
				char_to_room(t_mob, 0, -2);
			}

			if (t_mob->player.name)
				strcpy(arg2, t_mob->player.name);
		}
		i2 = FALSE;
		if (!(k = get_char_vis(ch, arg2)) || !IS_NPC(k))
		{
			send_to_char("No such character.\n", ch);
			i2 = TRUE;
		}
		else
		{
			for (i = 0; (i < number_of_shops) && (shop_index[i].keeper != GET_RNUM(k));
			     i++)
			{
				;
			}
			if (((mob_index[GET_RNUM(k)].func.mob != shop_keeper) &&
			     (mob_index[GET_RNUM(k)].qst_func != shop_keeper)) ||
			    (i >= number_of_shops))
			{
				send_to_char("No shop data for this mob.\n", ch);
				i2 = TRUE;
			}
		}

		if (i2)
		{
			if (t_mob)
			{
				extract_char(t_mob);
				t_mob = NULL;
			}
			return;
		}
		if (shop_index[i].shop_new_options)
		{
			num_tr = shop_index[i].number_types_traded;
			num_pr = shop_index[i].number_items_produced;
		}
		else
		{
			for (i2 = 0, num_tr = 0, num_pr = 0; i2 < 5; i2++)
			{
				if (SHOP_BUYTYPE(i, i2) != -1)
					num_tr++;
				if (shop_index[i].producing[i2] != -1)
					num_pr++;
			}
		}
		snprintf(o_buf, MAX_STRING_LENGTH,
			 "&+Y%s %sShop, Number: &N%d&+Y  for [&N%d&+Y](&n%d&+Y)&N %s\n\n",
			 shop_index[i].shop_new_options ? "New" : "Old",
			 shop_index[i].shop_is_roaming ? "Roaming " : "", i,
			 mob_index[GET_RNUM(k)].virtual_number, GET_RNUM(k),
			 k->player.short_descr ? k->player.short_descr : "&+rNone");
		checked_snprintf(
			o_buf + strlen(o_buf), MAX_STRING_LENGTH - strlen(o_buf),
			"&+YHours: &N%d&+Y-&N%d&+Y,&N %d&+Y-&N%d  &+YAttackable?: %c  Allow Casting?: %c\n",
			shop_index[i].open1, shop_index[i].close1, shop_index[i].open2,
			shop_index[i].close2, shop_index[i].shop_killable ? 'Y' : 'N',
			shop_index[i].magic_allowed ? 'Y' : 'N');
		checked_snprintf(
			o_buf + strlen(o_buf), MAX_STRING_LENGTH - strlen(o_buf),
			"&+YBuys for: &N%d%%&+Y, Sells for: &N%d%%&+Y, Produces &N%d &+YItems, Trades in &N%d &+YTypes\n",
			(int)(shop_index[i].buy_percent * 100),
			(int)(shop_index[i].sell_percent * 100), num_pr, num_tr);

		/* various messages that shop has stored. */

		checked_snprintf(o_buf + strlen(o_buf), MAX_STRING_LENGTH - strlen(o_buf),
				 "&+YRacist       :&N %s\n",
				 shop_index[i].racist_message ? shop_index[i].racist_message :
								"&+R<NONE>");
		checked_snprintf(o_buf + strlen(o_buf), MAX_STRING_LENGTH - strlen(o_buf),
				 "&+YOpening      :&N %s\n",
				 shop_index[i].open_message ? shop_index[i].open_message :
							      "&+R<NONE>");
		checked_snprintf(o_buf + strlen(o_buf), MAX_STRING_LENGTH - strlen(o_buf),
				 "&+YClosing      :&N %s\n",
				 shop_index[i].close_message ? shop_index[i].close_message :
							       "&+R<NONE>");
		checked_snprintf(o_buf + strlen(o_buf), MAX_STRING_LENGTH - strlen(o_buf),
				 "&+YDon't have   :&N %s\n",
				 shop_index[i].no_such_item1 ? shop_index[i].no_such_item1 :
							       "&+R<NONE>");
		checked_snprintf(o_buf + strlen(o_buf), MAX_STRING_LENGTH - strlen(o_buf),
				 "&+YCh don't have:&N %s\n",
				 shop_index[i].no_such_item2 ? shop_index[i].no_such_item2 :
							       "&+R<NONE>");
		checked_snprintf(o_buf + strlen(o_buf), MAX_STRING_LENGTH - strlen(o_buf),
				 "&+YToo poor     :&N %s\n",
				 shop_index[i].missing_cash1 ? shop_index[i].missing_cash1 :
							       "&+R<NONE>");
		checked_snprintf(o_buf + strlen(o_buf), MAX_STRING_LENGTH - strlen(o_buf),
				 "&+YCh too poor  :&N %s\n",
				 shop_index[i].missing_cash2 ? shop_index[i].missing_cash2 :
							       "&+R<NONE>");
		checked_snprintf(o_buf + strlen(o_buf), MAX_STRING_LENGTH - strlen(o_buf),
				 "&+YWrong Type   :&N %s\n",
				 shop_index[i].do_not_buy ? shop_index[i].do_not_buy : "&+R<NONE>");
		checked_snprintf(o_buf + strlen(o_buf), MAX_STRING_LENGTH - strlen(o_buf),
				 "&+YSOLD!        :&N %s\n",
				 shop_index[i].message_buy ? shop_index[i].message_buy :
							     "&+R<NONE>");
		checked_snprintf(o_buf + strlen(o_buf), MAX_STRING_LENGTH - strlen(o_buf),
				 "&+YBought       :&N %s\n",
				 shop_index[i].message_sell ? shop_index[i].message_sell :
							      "&+R<NONE>");

		strcat(o_buf, "\n&+YItems traded: &N");
		for (i2 = 0; (i2 < shop_index[i].number_types_traded) && SHOP_BUYTYPE(i, i2); i2++)
		{
			strcat(o_buf, item_types[(int)SHOP_BUYTYPE(i, i2)]);
			strcat(o_buf, " ");
		}
		/**** Out for a bit ***
		    for (i2 = 0; SHOP_BUYTYPE(i, i2) != NOTHING; i2++) {
		      if(i2)
		        strcat(buf, ", ");
		      snprintf(buf1, MAX_STRING_LENGTH, "%s (#%d) ", item_types[SHOP_BUYTYPE(i, i2)],
		              SHOP_BUYTYPE(i, i2));
		      if(SHOP_BUYWORD(i, i2))
		        snprintf(END_OF(buf1), MAX_STRING_LENGTH, "[%s]", SHOP_BUYWORD(i, i2));
		      else
		        strcat(buf1, "[all]");
		      strcat(o_buf
		             }
		****/

		strcat(o_buf, "\n\n&+YItems produced:\n");

		for (i2 = 0; i2 < shop_index[i].number_items_produced; i2++)
		{
			if (shop_index[i].producing[i2] != -1)
			{
				if ((t_obj = read_object(shop_index[i].producing[i2], REAL)))
				{
					m_virtual = (t_obj->R_num >= 0) ?
							    obj_index[t_obj->R_num].virtual_number :
							    0;

					checked_snprintf(o_buf + strlen(o_buf),
							 MAX_STRING_LENGTH - strlen(o_buf),
							 "&+Y[&N%5d&+Y] (&N%5d&+Y)&N %12s %s\n",
							 m_virtual, t_obj->R_num,
							 item_types[(int)t_obj->type],
							 ((t_obj->short_description) ?
								  t_obj->short_description :
								  "None"));
					extract_obj(t_obj);
				}
				else
				{
					logit(LOG_DEBUG,
					      "do_stat(): obj %d [%d] not loadable (shop stat)",
					      shop_index[i].producing[i2],
					      obj_index[shop_index[i].producing[i2]].virtual_number);
					checked_snprintf(o_buf + strlen(o_buf),
							 MAX_STRING_LENGTH - strlen(o_buf),
							 "&+RNon-existant object: &N%d\n",
							 shop_index[i].producing[i2]);
				}
			}
		}

		page_string(ch->desc, o_buf, 1);
		if (t_mob)
		{
			extract_char(t_mob);
			t_mob = NULL;
		}
	}
	else if ((*arg1 == 'd') || (*arg1 == 'D'))
	{
		stat_dam(ch, arg2);
		send_to_char("\n", ch);
		stat_spldam(ch, arg2);
	}
	else if ((*arg1 == 'q') || (*arg1 == 'Q'))
	{
		if (!(mob = get_char_vis(ch, arg2)) || !IS_NPC(mob))
		{
			checked_snprintf(buf, MAX_STRING_LENGTH,
					 "'%s' not found or is not a NPC.\n", arg2);
			send_to_char(buf, ch);
			return;
		}
		qi = find_quester_id(GET_RNUM(mob));
		struct quest_complete_data *qdata = quest_index[qi].quest_complete;
		struct goal_data *goals;

		if (!qdata)
		{
			snprintf(buf, MAX_STRING_LENGTH, "'%s' is not a quest complete mob.\n",
				 mob->player.short_descr);
			send_to_char(buf, ch);
		}
		else
		{
			snprintf(buf, MAX_STRING_LENGTH, "'%s' has quest:\n",
				 mob->player.short_descr);
			send_to_char(buf, ch);

			while (qdata)
			{
				snprintf(buf, MAX_STRING_LENGTH, "'%s'\n", qdata->message);
				send_to_char(buf, ch);
				if (qdata->receive)
				{
					for (goals = qdata->receive; goals; goals = goals->next)
					{
						snprintf(buf, MAX_STRING_LENGTH,
							 "Receive: '%c' %d\n", goals->goal_type,
							 goals->number);
						send_to_char(buf, ch);
					}
				}
				if (qdata->give)
				{
					for (goals = qdata->give; goals; goals = goals->next)
					{
						snprintf(buf, MAX_STRING_LENGTH, "Give: '%c' %d\n",
							 goals->goal_type, goals->number);
						send_to_char(buf, ch);
					}
				}

				qdata = qdata->next;
			}
		}
	}
	else
		send_to_char(STAT_SYNTAX, ch);
}

#undef STAT_SYNTAX

static void stat_single_race(P_char ch, int race)
{
	char buf[MAX_STRING_LENGTH];
	bool first;
	int i;

	// Note: Right now the longest race name is "Water Elemental" (42) at 15 chars. 6/5/2015
	snprintf(buf, MAX_STRING_LENGTH, "\n\rRace: %s&n (%2d) %15s %10s %2s\n\r",
		 pad_ansi(race_names_table[race].ansi, 15).c_str(), race,
		 race_names_table[race].normal, race_names_table[race].no_spaces,
		 race_names_table[race].code);
	send_to_char(buf, ch);

	snprintf(buf, MAX_STRING_LENGTH,
		 "Strength    : &+c%3d&n | Power       : &+c%3d&n\n\r"
		 "Dexterity   : &+c%3d&n | Intelligence: &+c%3d&n\n\r"
		 "Agility     : &+c%3d&n | Wisdom      : &+c%3d&n\n\r"
		 "Constitution: &+c%3d&n | Charisma    : &+c%3d&n\n\r"
		 "Luck        : &+c%3d&n | Karma       : &+c%3d&n\n\r"
		 "&+wCombatPulse : &+c%3.0f&+w | SpellPulse  : &+c%1.2f&n\n\r"
		 "&+wTotalDamMod : &+c%1.2f&+w| DamrollMod  : &+c%1.2f&n\n\r",
		 stat_factor[race].Str, stat_factor[race].Pow, stat_factor[race].Dex,
		 stat_factor[race].Int, stat_factor[race].Agi, stat_factor[race].Wis,
		 stat_factor[race].Con, stat_factor[race].Cha, stat_factor[race].Kar,
		 stat_factor[race].Luk, combat_by_race[race][0], spell_pulse_data[race],
		 combat_by_race[race][1], combat_by_race[race][2]);
	send_to_char(buf, ch);

	snprintf(buf, MAX_STRING_LENGTH,
		 "Base Age: &+c%3d&n, Max Age: &+c%4d&n, HP Bonus: &+c%2d&n\n\r"
		 "Base Moves: &+c%3d&n, Base Mana: &+c%3d&n, Max Mana: &+c%3d&n\n\r"
		 "Base Height: &+c%3d&n, Base Weight: &+c%3d&n\n\r",
		 racial_data[race].base_age, racial_data[race].max_age, racial_data[race].hp_bonus,
		 racial_data[race].base_vitality, racial_data[race].base_mana,
		 racial_data[race].max_mana, racial_values[race][0], racial_values[race][1]);
	send_to_char(buf, ch);

	snprintf(buf, MAX_STRING_LENGTH,
		 "&+MShrug&n: &+c%2d&n, ExpFactor: &+c%1.3f&n, VictimExpFactor: &+c%1.3f&n\n\r",
		 racial_shrug_data[race], racial_exp_mods[race], racial_exp_mod_victims[race]);
	send_to_char(buf, ch);

	send_to_char("&+CInnates&n: ", ch);
	first = TRUE;
	for (i = 0; i <= LAST_INNATE; i++)
	{
		if (racial_innates[i][race])
		{
			if (!first)
			{
				send_to_char(", ", ch);
			}
			first = FALSE;
			send_to_char_f(ch, "%s (%d)", innates_data[i].name,
				       racial_innates[i][race]);
		}
	}
	send_to_char(".\n\r", ch);
}

static void stat_zone(P_char ch, char *arg)
{
	struct zone_data *zone = 0;
	int zone_id, zone_number;
	char buf[MAX_STRING_LENGTH], o_buf[MAX_STRING_LENGTH];
	char buf2[MAX_STRING_LENGTH];

	if (!*arg)
	{
		zone_id = world[ch->in_room].zone;
		zone = &zone_table[zone_id];
	}
	else if (is_number(arg))
	{
		// accept a zone number as second arg
		if (((zone_number = atoi(arg)) > -1) && ((zone_id = real_zone(zone_number)) >= 0))
		{
			zone = &zone_table[zone_id];
		}
	}
	else if (!strcmp(arg, "portable"))
	{
		if (IS_MAP_ROOM(ch->in_room))
		{
			send_to_char(
				"&+rThis command is not available in a map zone, there are too many rooms.\n",
				ch);
			return;
		}

		send_to_char("&+YPortable rooms in current zone:\n", ch);

		zone_id = world[ch->in_room].zone;
		zone = &zone_table[zone_id];
		for (int i = zone->real_bottom; i < zone->real_top; i++)
		{
			// If the room is teleportable, display it w/room vnum.
			if (!IS_ROOM(i, ROOM_NO_TELEPORT))
			{
				snprintf(buf, MAX_STRING_LENGTH, "[&+C%d&n] %s\n", world[i].number,
					 world[i].name);
				send_to_char(buf, ch);
			}
		}
		return;
	}

	if (!zone)
	{
		send_to_char("Invalid zone number. Type 'world zones' to see list.\n", ch);
		return;
	}

	snprintf(o_buf, MAX_STRING_LENGTH,
		 "&+YZone: [&N%d&+Y](&N%d&+Y)  Name:&N %s&n  &+YFilename:&n %s\n", zone->number,
		 zone_id, zone->name, zone->filename);

	int maproom = maproom_of_zone(zone_id);
	if (maproom > 0)
	{
		checked_snprintf(o_buf + strlen(o_buf), MAX_STRING_LENGTH - strlen(o_buf),
				 "&+YConnects to map room: [&N%d&+Y]\n", maproom);
	}

	checked_snprintf(
		o_buf + strlen(o_buf), MAX_STRING_LENGTH - strlen(o_buf),
		"&+YRooms: &N%d  &+YRange: [&N%d&+Y](&N%d&+Y) to [&N%d&+Y](&N%d&+Y)  Top: &N%d\n",
		zone->real_top - zone->real_bottom + 1, world[zone->real_bottom].number,
		zone->real_bottom, world[zone->real_top].number, zone->real_top, zone->top);

	checked_snprintf(o_buf + strlen(o_buf), MAX_STRING_LENGTH - strlen(o_buf),
			 "&+YDifficulty: &N%d ", zone->difficulty);
	checked_snprintf(o_buf + strlen(o_buf), MAX_STRING_LENGTH - strlen(o_buf),
			 "&+YAvg mob level: &N%d ", zone->avg_mob_level);
	checked_snprintf(o_buf + strlen(o_buf), MAX_STRING_LENGTH - strlen(o_buf),
			 "&+YLifespan: &N%d  &+YAge: &N%d  &+R", zone->lifespan, zone->age);

	switch (zone->reset_mode)
	{
	case 0:
		strcat(o_buf, "Zone never resets.\n");
		break;
	case 1:
		strcat(o_buf, "Zone resets when empty.\n");
		break;
	case 2:
		strcat(o_buf, "Zone resets regardless.\n");
		break;
	default:
		strcat(o_buf, "Invalid reset mode!\n");
		break;
	}

	checked_snprintf(o_buf + strlen(o_buf), MAX_STRING_LENGTH - strlen(o_buf),
			 "&+YFull reset lifespan: &n%d  &+YFull reset age: &n%d\n",
			 zone->fullreset_lifespan, zone->fullreset_age);

	if (IS_SET(zone->flags, ZONE_MAP))
	{
		checked_snprintf(o_buf + strlen(o_buf), MAX_STRING_LENGTH - strlen(o_buf),
				 "&+YMap size:&n %d&+Yx&n%d\n", zone->mapx, zone->mapy);
	}

	checked_snprintf(o_buf + strlen(o_buf), MAX_STRING_LENGTH - strlen(o_buf),
			 "&+YControlling town:&N %s\n", town_name_list[zone->hometown]);
	sprintbit(zone->hometown ? hometowns[zone->hometown - 1].flags : 0, justice_flags, buf2);
	checked_snprintf(o_buf + strlen(o_buf), MAX_STRING_LENGTH - strlen(o_buf),
			 "&+YJustice:&N %s\n", buf2);
	sprintbit(zone->flags, zone_bits, buf);
	checked_snprintf(o_buf + strlen(o_buf), MAX_STRING_LENGTH - strlen(o_buf),
			 "&+YZone flags:&N %s\n", buf);

	struct zone_info zinfo;
	if (get_zone_info(zone->number, &zinfo))
	{
		string buff;

		checked_snprintf(o_buf + strlen(o_buf), MAX_STRING_LENGTH - strlen(o_buf),
				 "\n&+GZone Info\n");

		checked_snprintf(
			o_buf + strlen(o_buf), MAX_STRING_LENGTH - strlen(o_buf),
			"&+gTask zone: &+G%s  &+gQuest zone:  &+G%s  &+gTrophy zone:  &+G%s\n",
			YESNO(zinfo.task_zone), YESNO(zinfo.quest_zone), YESNO(zinfo.trophy_zone));

		if (zinfo.epic_type)
		{
			if (zinfo.epic_level)
			{
				checked_snprintf(o_buf + strlen(o_buf),
						 MAX_STRING_LENGTH - strlen(o_buf),
						 "&+gGrants epic level: &+G%d\n", zinfo.epic_level);
			}

			checked_snprintf(o_buf + strlen(o_buf), MAX_STRING_LENGTH - strlen(o_buf),
					 "&+gRarity: &+G%1.3f  ", zinfo.frequency_mod);
			checked_snprintf(o_buf + strlen(o_buf), MAX_STRING_LENGTH - strlen(o_buf),
					 "&+gZone frequency multiplier: &+G%1.3f\n",
					 zinfo.zone_freq_mod);

			checked_snprintf(o_buf + strlen(o_buf), MAX_STRING_LENGTH - strlen(o_buf),
					 "&+gEpic value: &+G%d  &+gSuggested group size: &+G%d\n",
					 zinfo.epic_payout, zinfo.suggested_group_size);

			checked_snprintf(o_buf + strlen(o_buf), MAX_STRING_LENGTH - strlen(o_buf),
					 "&+gEpic stone(s):\n");

			for (P_obj tobj = object_list; tobj; tobj = tobj->next)
			{
				if (obj_zone_id(tobj) != zone_id)
				{
					continue;
				}

				int obj_vnum = obj_index[tobj->R_num].virtual_number;

				if (obj_vnum != EPIC_SMALL_STONE && obj_vnum != EPIC_LARGE_STONE &&
				    obj_vnum != EPIC_MONOLITH)
				{
					continue;
				}

				int obj_room_vnum = world[obj_room_id(tobj)].number;
				if (obj_room_vnum < 0)
				{
					continue;
				}
				checked_snprintf(o_buf + strlen(o_buf),
						 MAX_STRING_LENGTH - strlen(o_buf),
						 " %s &nin &+W[&n%d&+W]\n", tobj->short_description,
						 obj_room_vnum);
			}
		}

		checked_snprintf(o_buf + strlen(o_buf), MAX_STRING_LENGTH - strlen(o_buf),
				 "&+YRacewar Info:&N\n");
		zone_data *zdata = &(zone_table[zone_id]);
		// Skip RACEWAR_NONE.
		for (int rw = 1; rw <= MAX_RACEWAR; rw++)
		{
			checked_snprintf(o_buf + strlen(o_buf), MAX_STRING_LENGTH - strlen(o_buf),
					 "&+%c%7s&+Y Count: &N%2d&+Y, Misfiring: &N%s&+Y.&N\n",
					 racewar_color[rw].color, racewar_color[rw].name,
					 zdata->players[rw], YESNO(zdata->misfiring[rw]));
		}
	}
	checked_snprintf(o_buf + strlen(o_buf), MAX_STRING_LENGTH - strlen(o_buf),
			 "\n&+YExits from this zone:\n");

	int exits_shown = 0;
	int i, i2, i3;
	for (i3 = 0, i = zone->real_bottom;
	     (i != NOWHERE && i <= zone->real_top && exits_shown < 1000); i++)
	{
		for (i2 = 0; i2 < NUM_EXITS; i2++)
		{
			if (world[i].dir_option[i2])
			{
				if ((world[i].dir_option[i2]->to_room == NOWHERE) ||
				    (world[world[i].dir_option[i2]->to_room].zone != world[i].zone))
				{
					if (!i3)
					{
						i3 = 1;
					}
					if (world[i].dir_option[i2]->to_room == NOWHERE)
					{
						checked_snprintf(
							o_buf + strlen(o_buf),
							MAX_STRING_LENGTH - strlen(o_buf),
							" &+Y[&n%5d&+Y]&n &+R%-5s&n to &+WNOWHERE\n",
							world[i].number, dirs[i2]);
						exits_shown++;
					}
					else
					{
						checked_snprintf(
							o_buf + strlen(o_buf),
							MAX_STRING_LENGTH - strlen(o_buf),
							" &+Y[&n%5d&+Y]&n &+R%-5s&n to &+Y[&+R%3d&n:&+C%5d&+Y]&n %s\n",
							world[i].number, dirs[i2],
							zone_table[world[world[i].dir_option[i2]
										 ->to_room]
									   .zone]
								.number,
							world[world[i].dir_option[i2]->to_room]
								.number,
							world[world[i].dir_option[i2]->to_room]
								.name);
						exits_shown++;
					}
				}
			}
		}
	}

	if (!i3)
	{
		strcat(o_buf, "&+RNONE!&n\n");
	}
	if (exits_shown >= 1000)
	{
		strcat(o_buf, " (and many more)\n");
	}
	page_string(ch->desc, o_buf, 1);
	return;
}

static void stat_race(P_char ch, char *arg)
{
	int race;
	char buf[MAX_STRING_LENGTH];

	if (!strcmp(arg, "all"))
	{
		for (race = 0; race < LAST_RACE; race++)
		{
			stat_single_race(ch, race);
		}
		return;
	}

	race = race_lookup(arg);
	// If we couldn't identify the race to look for.. (allowing RACE_NONE since this is a Imm command).
	if (race < 0 || race > LAST_RACE)
	{
		snprintf(
			buf, MAX_STRING_LENGTH,
			"Race '%s' not found.  Please enter a number between 0 and %d or a valid race name.\n\r",
			arg, LAST_RACE);
		send_to_char(buf, ch);
		return;
	}

	stat_single_race(ch, race);
}

static int lookup_skill(char *skill_name)
{
	int skl;
	for (skl = FIRST_SPELL; skl <= LAST_SPELL; skl++)
	{
		if (is_abbrev(skill_name, skills[skl].name))
		{
			return skl;
		}
	}
	for (skl = FIRST_SKILL; skl <= LAST_SKILL; skl++)
	{
		if (is_abbrev(skill_name, skills[skl].name))
		{
			return skl;
		}
	}
	return SKILL_NONE;
}

static void stat_skill(P_char ch, char *arg)
{
	int skl;

	if (is_number(arg))
	{
		skl = atoi(arg);
		if (!IS_SKILL(skl))
		{
			send_to_char_f(ch, "That's not a valid skill number (Range: %d to %d).\n",
				       FIRST_SKILL, LAST_SKILL);
			return;
		}
	}
	else if ((skl = lookup_skill(arg)) == SKILL_NONE)
	{
		send_to_char_f(ch, "'%s' is not recognized as a skill.\n", arg);
		return;
	}

	send_to_char_f(ch, "&+YSkill: '&n%s&+Y'&N %d\n", skills[skl].name, skl);
	for (int cls = 1; class_names_table[cls].code != NULL; cls++)
	{
		if (skills[skl].m_class[cls - 1].rlevel[0] > 0)
		{
			send_to_char_f(ch, "Class: %s : @lvl %d Max %d.\n",
				       class_names_table[cls].code,
				       skills[skl].m_class[cls - 1].rlevel[0],
				       skills[skl].m_class[cls - 1].maxlearn[0]);
		}
		for (int spec = 1; spec < MAX_SPEC; spec++)
		{
			if (skills[skl].m_class[cls - 1].rlevel[spec] !=
			    skills[skl].m_class[cls - 1].rlevel[0])
			{
				send_to_char_f(ch, "Spec: %d %s : @lvl %d Max %d.\n", spec,
					       specdata[cls][spec - 1],
					       skills[skl].m_class[cls - 1].rlevel[spec],
					       skills[skl].m_class[cls - 1].maxlearn[spec]);
			}
		}
	}
}
