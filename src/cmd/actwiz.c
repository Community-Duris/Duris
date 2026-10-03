/*
 * ***************************************************************************
 * *  File: actwiz.c                                           Part of Duris *
 * *  Usage: wizcommands
 * * *  Copyright  1990, 1991 - see 'license.doc' for complete information.
 *  * *  Copyright 1994 - 2008 - Duris Systems Ltd.
 * *
 * ***************************************************************************
 */

#include "core/prototypes.h"
#include "account/creation_availability_config.h"
#include "net/comm.h"
#include "world/db.h"
#include "world/events.h"
#include <errno.h>
#include <ctype.h>
#include <stdarg.h>
#include <signal.h>
#include <stdio.h>
#include <string>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "guild/assocs.h"
#include "economy/economic_gameplay_authority.h"
#include "economy/shop.h"
// #include "core/types.h"  // Not needed on modern Linux systems
#include "core/structs.h"
#include "cmd/interp.h"
#include "core/utility.h"
#include "core/utils.h"
#include "world/achievements.h"
#include "combat/damage.h"
#include "combat/training_dummy.h"
#include "world/epic.h"
#include "world/epic_transaction.h"
#include "core/files.h"
#include "net/gmcp.h"
#include "item/item_movement_transaction.h"
#include "combat/justice.h"
#include "net/listen.h"
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
#include "net/ws_handlers.h"
#include "core/safe_format.h"

/*
 * external variables
 */

extern Skill skills[];
extern char *spells[];
extern P_char character_list;
extern P_desc descriptor_list;
extern P_index mob_index;
extern P_index obj_index;
extern P_obj object_list;
extern P_room world;
extern ::byte create_locked;
extern ::byte locked;
extern int top_of_helpt;
extern FILE *help_fl;
extern const char *weapons[];
extern const char *justice_flags[];
extern const flagDef action_bits[];
extern const flagDef action2_bits[];
extern const flagDef aggro_bits[];
extern const flagDef aggro2_bits[];
extern const flagDef aggro3_bits[];
extern const flagDef anti_bits[];
extern const flagDef anti2_bits[];
extern const char *apply_types[];
extern const struct class_names class_names_table[];
extern const char *command[];
extern const char *connected_types[];
extern const char *dirs[];
extern const char *drinks[];
extern const char *equipment_types[];
extern const char *exit_bits[];
extern const flagDef extra_bits[];
extern const flagDef extra2_bits[];
extern const flagDef affected1_bits[];
extern const flagDef affected2_bits[];
extern const flagDef affected3_bits[];
extern const flagDef affected4_bits[];
extern const flagDef affected5_bits[];
extern const char *item_types[];
extern const char *missileweapons[];
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
extern const char *justice_obj_status[];
extern char *shutdown_message;
extern const char *item_material[];
extern const char *resource_list[];
extern const int shot_damage[];
extern const struct stat_data stat_factor[];
extern const char *size_types[];
extern const char *item_size_types[];
extern int number_of_shops;
extern int invitemode;
extern int pulse;
extern int shutdownflag, _reboot, _autoboot, _copyover, _pwipe;
extern int spl_table[TOTALLVLS][MAX_CIRCLE];
extern int top_of_mobt;
extern int top_of_objt;
extern const int top_of_world;
extern int top_of_zone_table;
extern int used_descs;
extern struct agi_app_type agi_app[];
extern struct ban_t *ban_list;
extern struct wizban_t *wizconnect;
extern struct bonus_stat bonus_stats[];
extern struct bonus_stat general_bonus_stats;
extern struct command_info cmd_info[MAX_CMD_LIST];
extern struct max_stat max_stats[];
extern struct shop_data *shop_index;
extern struct str_app_type str_app[];
extern struct zone_data *zone_table;
extern struct time_info_data time_info;
extern struct mm_ds *dead_mob_pool;
extern struct mm_ds *dead_pconly_pool;
extern int min_stats_for_class[][8];
extern char *specdata[][MAX_SPEC];
extern struct link_description link_types[];
void sprintbitde(ulong, const flagDef[], char *);
extern flagDef weapon_types[];
extern flagDef missile_types[];
extern float combat_by_class[][2];
extern float combat_by_race[][3];
extern long new_exp_table[]; // Arih: Fixed type mismatch bug - was int, should be long
extern const char *get_function_name(void *);
extern const char *spldam_types[];
extern const char *craftsmanship_names[];
extern int number_of_quests;
extern struct quest_data quest_index[];
extern const struct hold_data hold_index[];
extern float spell_pulse_data[LAST_RACE + 1];
extern int racial_shrug_data[LAST_RACE + 1];
extern const struct racial_data_type racial_data[];
extern int racial_values[LAST_RACE + 1][2];
extern bool racial_innates[][LAST_RACE + 1];
extern const struct innate_data innates_data[];
extern float racial_exp_mods[LAST_RACE + 1];
extern float racial_exp_mod_victims[LAST_RACE + 1];
extern int damroll_cap;
extern const racewar_struct racewar_color[MAX_RACEWAR + 2];
extern struct continent_misfire_data continent_misfire;
extern struct misfire_properties_struct misfire_properties;

typedef void cmd_func(P_char, char *, int);

void write_shutdown_info(const char *immortal_name, const char *reason)
{
	FILE *fp = fopen("logs/shutdown_info.txt", "w");
	if (fp)
	{
		fprintf(fp, "%s|%s\n", immortal_name, reason);
		fclose(fp);
	}
}

void apply_zone_modifier(P_char ch);
void shopping_stat(P_char ch, P_char keeper, char *arg, int cmd);
bool is_quested_item(P_obj obj);
void event_mob_mundane(P_char, P_char, P_obj, void *);

/*
 * Macros
 */
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(A) (sizeof(A) / sizeof(*(A)))
#endif

#define GET_VICTIM_ROOM(v, c, a) (v) = get_char_room_vis((c), (a))

/*
 * call with an object, recursively calls itself to build a string in the
 * format "in <container> in <container>... carried by <name>.  Only limit
 * to nesting depth, is MAX_STRING_LENGTH for all output.  Currently only
 * used by the new stat routine, probably add it to 'where' later, and the
 * god's version of locate spell.  JAB
 */
char *where_obj(P_obj w_obj, int flag)
{
	if (!flag)
		GS_buf1[0] = 0;

	if (!w_obj)
	{
		strcat(GS_buf1, "&+RLost in the bit bucket!&n");
		return (GS_buf1);
	}
	if (OBJ_ROOM(w_obj))
	{
		checked_snprintf(GS_buf1 + strlen(GS_buf1), MAX_STRING_LENGTH - strlen(GS_buf1),
				 "in [&+R%4d&+W:&+C%6d&n] &n%s", ROOM_ZONE_NUMBER(w_obj->loc.room),
				 world[w_obj->loc.room].number, world[w_obj->loc.room].name);
		return (GS_buf1);
	}
	if (OBJ_CARRIED(w_obj) && w_obj->loc.carrying)
	{
		checked_snprintf(GS_buf1 + strlen(GS_buf1), MAX_STRING_LENGTH - strlen(GS_buf1),
				 "in [&+R%4d&+W:&+C%6d&n] &+Ycarried by &n%s&n",
				 ((w_obj->loc.carrying->in_room != NOWHERE) ?
					  ROOM_ZONE_NUMBER(w_obj->loc.carrying->in_room) :
					  -1),
				 ((w_obj->loc.carrying->in_room != NOWHERE) ?
					  world[w_obj->loc.carrying->in_room].number :
					  -1),
				 GET_NAME(w_obj->loc.carrying));
		return (GS_buf1);
	}
	if (OBJ_WORN(w_obj) && w_obj->loc.wearing)
	{
		checked_snprintf(GS_buf1 + strlen(GS_buf1), MAX_STRING_LENGTH - strlen(GS_buf1),
				 "in [&+R%4d&+W:&+C%6d&n] &+Yequipped by &n%s&n",
				 ((w_obj->loc.wearing->in_room != NOWHERE) ?
					  ROOM_ZONE_NUMBER(w_obj->loc.wearing->in_room) :
					  -1),
				 ((w_obj->loc.wearing->in_room != NOWHERE) ?
					  world[w_obj->loc.wearing->in_room].number :
					  -1),
				 GET_NAME(w_obj->loc.wearing));
		return (GS_buf1);
	}
	if (OBJ_INSIDE(w_obj) && w_obj->loc.inside)
	{
		checked_snprintf(GS_buf1 + strlen(GS_buf1), MAX_STRING_LENGTH - strlen(GS_buf1),
				 "&+Yinside &n%s&+Y, ", w_obj->loc.inside->short_description);
		where_obj(w_obj->loc.inside, TRUE);
		return GS_buf1;
	}
	if (GS_buf1[0] == 0)
		strcat(GS_buf1, "&+RLost in the bit bucket! #2&N");

	return (GS_buf1);
}

/* Load a player manually from save files */
void do_read_player(P_char ch, char *arg, int /*cmd*/)
{
	P_char vict = NULL;
	int tmp;

	if (!*arg)
	{
		send_to_char("Syntax: load char <name> or load <m|c|o|i> <vnum>\n", ch);
		return;
	}

	vict = (P_char)mm_get(dead_mob_pool);
	clear_char(vict);
	vict->only.pc = (struct pc_only_data *)mm_get(dead_pconly_pool);
	vict->only.pc->aggressive = -1;
	vict->desc = NULL;
	if ((tmp = restoreCharOnly(vict, arg)) < 0)
	{
		send_to_char("&=RlDanger Will Robinson! Bad pfile!!&n\n", ch);
		return;
	}
	tmp = restoreItemsOnly(vict, 100);

	if (!strstr(GET_NAME(vict), ".locker"))
	{
		for (P_char it = character_list; it; it = it->next)
		{
			if (IS_PC(it) && !strstr(GET_NAME(it), ".locker") &&
			    GET_PID(it) == GET_PID(vict))
			{
				char buf[1024];
				snprintf(
					buf, ARRAY_SIZE(buf),
					"&=lWPID collision - characters %s and %s share PID %d&n\n",
					GET_NAME(it), GET_NAME(vict), GET_PID(it));
				send_to_char(buf, ch);
			}
		}
	}

	/* insert in list */
	vict->next = character_list;
	character_list = vict;

	/* saving info for teleport return command */
	vict->specials.was_in_room = vict->in_room;

	char_to_room(vict, ch->in_room, -2);
	update_ingame_racewar(GET_RACEWAR(vict));

	act("$N &+Bappears before you, greatly humbled by your power.", FALSE, ch, 0, vict,
	    TO_CHAR);
	act("$N &+Bappears before $n, greatly humbled by $s power.", TRUE, ch, 0, vict, TO_NOTVICT);

	logit(LOG_WIZ, "%s loaded %s's char into the game [%d]", GET_NAME(ch), GET_NAME(vict),
	      world[ch->in_room].number);
	sql_log(ch, WIZLOG, "Loaded char %s", GET_NAME(vict));
}

void do_release(P_char ch, char *argument, int /*cmd*/)
{
	char arg[MAX_STRING_LENGTH];
	char buf[MAX_STRING_LENGTH];
	P_desc d;
	int sdesc;

	if (!IS_TRUSTED(ch))
		return;

	one_argument(argument, arg);
	sdesc = atoi(arg);
	if ((sdesc == 0) && *arg)
	{
		P_char t_ch = NULL;

		for (d = descriptor_list; d; d = d->next)
		{
			if (d->character)
				t_ch = d->character;
			else
				continue;
			// hide higher level people
			if (GET_LEVEL(t_ch) > GET_LEVEL(ch))
				continue;
			// if this isn't the proper user, keep looking
			if ((!t_ch->player.name || !isname(t_ch->player.name, arg)))
				continue;
			sdesc = d->descriptor;
		}
	}
	if (!sdesc)
	{
		send_to_char("Illegal descriptor number or name not found.\n", ch);
		send_to_char("Usage: release {<#> | <name>}\n", ch);
		return;
	}
	for (d = descriptor_list; d; d = d->next)
	{
		if (d->descriptor == sdesc)
		{
			if (!d->character || CAN_SEE(ch, d->character))
			{
				close_socket(d);
				snprintf(buf, MAX_STRING_LENGTH,
					 "Closing socket to descriptor #%d\n", sdesc);
				send_to_char(buf, ch);
				if (GET_LEVEL(ch) < 62)
				{
					wizlog(GET_LEVEL(ch), "%s just released socket %d.",
					       GET_NAME(ch), sdesc);
					logit(LOG_WIZ, "%s just released socket %d.", GET_NAME(ch),
					      sdesc);
					sql_log(ch, WIZLOG, "Released socket %d.", sdesc);
				}
				return;
			}
		}
	}
	send_to_char("Descriptor not found!\n", ch);
}

/*
 ** This function now allows a player to transfer anyone
 ** at or below his level
 */

// No args: Lists the deathobjects vnum along with short desc.
// With arg add: Adds to list of deathobjects and sets object's proc to kill any mortal.
// With arg remove: Removes argument from list of deathobjects and sets proc to NULL.
// Question: Are these procs read/set at boot and after regular procs set?
//   Otherwise, they won't persist through boots and would make this lame.
void do_deathobj(P_char ch, char *argument, int /*cmd*/)
{
	char vnum[15], out[MAX_STRING_LENGTH], Gbuf[MAX_STRING_LENGTH];
	char arg1[MAX_STRING_LENGTH], arg2[MAX_STRING_LENGTH];
	FILE *f;
	int count = 0, inserted = FALSE, removed = FALSE, newvnum, rn;
	P_obj obj;

	// Disabled for unknown reasons. - Lohrr 8/8/14
	send_to_char("do_deathobj: Command disabled.\n\r", ch);
	return;

	*out = '\0';
	*arg1 = '\0';
	*arg2 = '\0';

	f = fopen("Players/deathobjs", "r");
	if (!f)
	{
		send_to_char("Death object file is missing, no can do.\n", ch);
		return;
	}

	if (!*argument)
	{
		while (fgets(vnum, 15, f))
		{
			count++;
			vnum[strlen(vnum) - 1] = '\0';
			if ((obj = read_object(atoi(vnum), VIRTUAL)))
			{
				snprintf(Gbuf, MAX_STRING_LENGTH, "%d. [%s] %s\n", count, vnum,
					 obj->short_description);
				extract_obj(obj);
			}
			else
			{
				snprintf(Gbuf, MAX_STRING_LENGTH, "%d. [%s] No object found.\n",
					 count, vnum);
			}
			strcat(out, Gbuf);
		}
		fclose(f);
		send_to_char(out, ch);
		return;
	}
	else
	{
		argument_interpreter(argument, arg1, arg2);
		if (!*arg1 || !*arg2)
		{
			send_to_char("Usage: deathobj [add | remove] <vnum>\n", ch);
			fclose(f);
			return;
		}
		if (is_abbrev(arg1, "add"))
		{
			if (!(newvnum = atoi(arg2)))
			{
				send_to_char("Must be a valid vnum.\n", ch);
				fclose(f);
				return;
			}
			if (!(rn = real_object0(newvnum)))
			{
				send_to_char("No such object.\n", ch);
				fclose(f);
				return;
			}
			while (fgets(vnum, 15, f))
			{
				if (atoi(vnum) < newvnum || inserted)
					strcat(out, vnum);
				else if (atoi(vnum) == newvnum)
				{
					send_to_char("Vnum already on list.\n", ch);
					fclose(f);
					return;
				}
				else
				{
					snprintf(Gbuf, MAX_STRING_LENGTH, "%d\n", newvnum);
					strcat(out, Gbuf);
					strcat(out, vnum);
					inserted = TRUE;
				}
			}
			if (!inserted)
			{
				snprintf(Gbuf, MAX_STRING_LENGTH, "%d\n", newvnum);
				strcat(out, Gbuf);
			}
			obj_index[rn].func.obj = death_proc;
			fclose(f);
			if ((f = fopen("Players/deathobjs", "w")))
			{
				fprintf(f, "%s", out);
				fclose(f);
			}
			send_to_char("New vnum inserted.\n", ch);
			return;
		}
		else if (is_abbrev(arg1, "remove"))
		{
			if (!(newvnum = atoi(arg2)))
			{
				send_to_char("Must be a valid vnum.\n", ch);
				fclose(f);
				return;
			}
			while (fgets(vnum, 15, f))
			{
				if (atoi(vnum) == newvnum)
					removed = TRUE;
				else
					strcat(out, vnum);
			}
			fclose(f);
			if (removed)
			{
				if ((f = fopen("Players/deathobjs", "w")))
				{
					fprintf(f, "%s", out);
					fclose(f);
					obj_index[real_object0(newvnum)].func.obj = NULL;
					send_to_char("Object removed from list.\n", ch);
					return;
				}
				send_to_char("Error writing to file!\n", ch);
				return;
			}
			else
			{
				send_to_char("Vnum not found in list.\n", ch);
				return;
			}
		}
		else
		{
			send_to_char("Usage: deathobj [add | remove] <vnum>\n", ch);
			fclose(f);
			return;
		}
	}
}

void do_shutdow(P_char ch, char * /*argument*/, int /*cmd*/)
{
	send_to_char("If you want to shut something down - say so!\n", ch);
}

TimedShutdownData shutdownData = { 0, -1, TimedShutdownData::NONE, "", "" };

static const char *scheduled_shutdown_type_name(int shutdown_type, bool uppercase)
{
	switch (shutdown_type)
	{
	case TimedShutdownData::OK:
	case TimedShutdownData::PWIPE:
		return uppercase ? "SHUTDOWN" : "shutdown";
	case TimedShutdownData::COPYOVER:
	case TimedShutdownData::AUTOREBOOT_COPYOVER:
		return uppercase ? "COPYOVER" : "copyover";
	default:
		return uppercase ? "REBOOT" : "reboot";
	}
}

/** Execute an immediate shutdown or schedule the next countdown warning for an active request. */
void timedShutdown(P_char ch, P_char, P_obj, void * /*data*/)
{
	// timed shutdown event.  ch is the god who initiated the shutdown.
	//  data refers to the shutdown timer and shutdown type

	if (shutdownData.eShutdownType == TimedShutdownData::NONE)
	{ // silently return (without setting a new event)
		return;
	}

	char buf[500];
	// round a bit due to floating point errors...
	if (shutdownData.reboot_time == 0)
	{
		// perform the shutdown...
		switch (shutdownData.eShutdownType)
		{
		case TimedShutdownData::OK:
			snprintf(buf, 500, "\r\n%s grabs Duris by the balls and rips them off.\r\n",
				 shutdownData.IssuedBy);
			send_to_all(buf);
			logit(LOG_STATUS, "%s", buf);
			sql_log(ch, WIZLOG, "%s", buf);
			write_shutdown_info(shutdownData.IssuedBy, shutdownData.Reason);
			shutdownflag = 1;
			break;

		case TimedShutdownData::REBOOT:
			snprintf(buf, 500, "\r\n%s shreds the world around you.\r\n",
				 shutdownData.IssuedBy);
			send_to_all(buf);
			logit(LOG_STATUS, "%s", buf);
			sql_log(ch, WIZLOG, "%s", buf);
			write_shutdown_info(shutdownData.IssuedBy, shutdownData.Reason);
			shutdownflag = _reboot = 1;
			break;

		case TimedShutdownData::COPYOVER:
			snprintf(buf, 500,
				 "\r\n%s begins a copyover; your connection will be preserved.\r\n",
				 shutdownData.IssuedBy);
			send_to_all(buf);
			logit(LOG_STATUS, "%s", buf);
			sql_log(ch, WIZLOG, "%s", buf);
			write_shutdown_info(shutdownData.IssuedBy, shutdownData.Reason);
			shutdownflag = _copyover = 1;
			break;

		case TimedShutdownData::AUTOREBOOT_COPYOVER:
			snprintf(
				buf, 500,
				"\r\nDuris fades into nothing, as the world begins its reconstruction...\r\n");
			send_to_all(buf);
			logit(LOG_STATUS, "%s", buf);
			sql_log(ch, WIZLOG, "%s", buf);
			shutdownflag = _autoboot = _copyover = 1;
			break;

		case TimedShutdownData::AUTOREBOOT:
			snprintf(
				buf, 500,
				"\r\nDuris fades into nothing, as the world begins its reconstruction...\r\n");
			send_to_all(buf);
			logit(LOG_STATUS, "%s", buf);
			sql_log(ch, WIZLOG, "%s", buf);
			shutdownflag = _autoboot = 1;
			break;

		case TimedShutdownData::PWIPE:
			// Dunno why, but send_to_all isn't color coding here, maybe term type is erased in database or such? :(
			snprintf(
				buf, 500,
				"\r\n\033[1;5;44mDuris begins to fade into nothing.. So do you.. \033[0m\r\n\033[1;5;44mThis is really the "
				"end!!!\033[0m\r\n\033[1;5;44m............\033[0m\r\n\033[1;5;44m........\033[0m\r\n\033[1;5;44m......\033[0m\r\n\033[1;5;44m...\033[0m\r\n\033[1;5;44m.\033[0m\n\r");
			send_to_all(buf);
			logit(LOG_STATUS, "Shutdown pwipe called.");
			shutdownflag = _pwipe = 1;
			if (!persistence_prepare_pwipe())
			{
				send_to_all(
					"&=GlPersistence workers did not quiesce; aborting destructive wipe.&n\n\r");
				shutdownflag = _pwipe = 0;
				shutdownData.eShutdownType = TimedShutdownData::NONE;
				return;
			}
			if (!persistence_quarantine_fallback_events())
			{
				send_to_all(
					"&=GlFallback persistence log could not be quarantined; aborting destructive wipe.&n\n\r");
				shutdownflag = _pwipe = 0;
				shutdownData.eShutdownType = TimedShutdownData::NONE;
				return;
			}
			if (!sql_pwipe(1723699))
			{
				if (sql_pwipe_crossed_boundary())
				{
					send_to_all(
						"&+RSeason reset crossed its irreversible boundary and did not complete. The server will stop for operator recovery.&n\n\r");
					logit(LOG_STATUS,
					      "Pwipe failed after irreversible season boundary; forcing fenced shutdown.");
					write_shutdown_info(shutdownData.IssuedBy,
							    "incomplete irreversible season reset");
					shutdownflag = _pwipe = 1;
					break;
				}
				send_to_all(
					"&=GlSQL database not wiped clean.. Aborting shutdown wipe.&n\n\r&+WYou're still alive!  Yay!&n\n\r");
				shutdownflag = _pwipe = 0;
				shutdownData.eShutdownType = TimedShutdownData::NONE;
				return;
			}
			else
			{
				logit(LOG_STATUS, "Successful wipe of SQL stuff.");
			}
			write_shutdown_info(shutdownData.IssuedBy, shutdownData.Reason);
			shutdownflag = _pwipe = 1;
			break;

		default:
			wizlog(60, "WARNING:  Unknown shutdown type ABORTED!!");
			return;
		}
		if (shutdown_message)
		{
			FREE(shutdown_message);
		}
		shutdown_message = str_dup(buf);
	}
	else
	{
		P_char issuer = NULL;
		// find the god doing the shutdown.  If he/she loses link or logs off, the reboot will
		// be cancelled.
		for (P_desc d = descriptor_list; d; d = d->next)
		{
			if (d->character && isname(GET_NAME(d->character), shutdownData.IssuedBy))
			{
				issuer = d->character;
				break;
			}
		}
		/*   if(!ch)
		   {
		     // CANCEL the reboot.
		     shutdownData.eShutdownType = TimedShutdownData::NONE;
		     send_to_all("&+R*** Scheduled Reboot Cancelled ***&n\n");
		     snprintf(buf, 500, "Scheduled reboot cancelled: unable to locate %s", shutdownData.IssuedBy);
		     wizlog(60, buf);
		     return;
		   }
	   */
		// how much longer until a reboot?
		time_t secs = shutdownData.reboot_time - time(0);
		// special case:  going to reboot in less then ~1 second - force a restore all.
		if ((secs <= 1) && (issuer != NULL))
		{
			// do restoreall here...
			int old_level = GET_LEVEL(issuer);
			issuer->player.level = MAXLVL;
			do_restore(issuer, writable_arg(" all"), CMD_RESTORE);
			issuer->player.level = old_level;

			// setting the reboot_time to 0 forces the reboot to occur next event
			shutdownData.reboot_time = 0;
			secs = MAX(1, secs * WAIT_SEC);
			add_event(timedShutdown, secs, NULL, NULL, NULL, 0, NULL, 0);
			return;
		}
		else if (secs <= 1)
		{
			// no restore since no ch.
			// setting the reboot_time to 0 forces the reboot to occur next event
			shutdownData.reboot_time = 0;
			secs = MAX(1, secs * WAIT_SEC);
			add_event(timedShutdown, secs, NULL, NULL, NULL, 0, NULL, 0);
			return;
		}
		// if the next warning timer isn't set, set it now
		if (-1 == shutdownData.next_warning)
			shutdownData.next_warning = secs;

		// okay, see if a warning should be displayed...
		if (secs <= shutdownData.next_warning)
		{
			const char *type =
				scheduled_shutdown_type_name(shutdownData.eShutdownType, true);
			if (secs > 60)
				snprintf(buf, sizeof buf,
					 "&+R*** Scheduled %s in %ld minutes ***&n\n", type,
					 secs / 60);
			else
				snprintf(buf, sizeof buf,
					 "&+R*** Scheduled &-L%s&n&+R in %ld seconds ***&n\n", type,
					 secs);
			send_to_all(buf);
			// and set when the next warning should occur..

			if (secs <= 15)
			{
				shutdownData.next_warning -= 5;
			}
			else if (secs <= 60)
			{ // <= 1 minute - warn every 15 seconds
				shutdownData.next_warning -= 15;
			}
			else if (secs <= (5 * 60))
			{ // <= 5 minutes - warn every 1 minute
				shutdownData.next_warning -= 60;
			}
			else if (secs <= (15 * 60))
			{ // <= 15 minutes - warn every 2 minutes
				shutdownData.next_warning -= 120;
			}
			else
			{ // > 15 minutes, warn every 5 minutes
				shutdownData.next_warning -= (5 * 60);
			}
		}
		add_event(timedShutdown, WAIT_SEC, NULL, NULL, NULL, 0, NULL, 0);
	}
}

/** Display an active shutdown countdown, treating immediate and expired deadlines as zero. */
void displayShutdownMsg(P_char ch)
{
	// send_to_char() ch any pending reboot/shutdown message...
	if (shutdownData.eShutdownType == TimedShutdownData::NONE)
		return;

	char buf[200];
	time_t secs = shutdownData.reboot_time ? shutdownData.reboot_time - time(0) : 0;
	if (secs < 0)
		secs = 0;
	const char *type = scheduled_shutdown_type_name(shutdownData.eShutdownType, true);

	if (secs > 60)
		snprintf(buf, sizeof buf, "&+R*** Scheduled %s in %ld minute%s***&n\n", type,
			 secs / 60, (secs >= 120) ? "s " : " ");
	else
		snprintf(buf, sizeof buf, "&+R*** Scheduled &-L%s&n&+R in %ld seconds ***&n\n",
			 type, secs);
	send_to_char(buf, ch);
}

void do_shutdown(P_char ch, char *argument, int /*cmd*/)
{
	char buf[100], arg[MAX_STRING_LENGTH];

	if (IS_NPC(ch))
		return;

	argument = one_argument(argument, arg);

	int mins_to_reboot = 0;
	const char *type = "reboot";
	char reason[MAX_STRING_LENGTH];
	strcpy(reason, "No reason given"); // Default reason

	type = scheduled_shutdown_type_name(shutdownData.eShutdownType, false);

	// Parse: shutdown <type> [minutes] [reason...]
	char temp_arg[MAX_INPUT_LENGTH];
	const char *reason_start = argument;

	if (argument && *argument)
	{
		reason_start = one_argument(argument, temp_arg);

		// If temp_arg is numeric, it's minutes
		if (isdigit(temp_arg[0]))
		{
			mins_to_reboot = atol(temp_arg);

			// Skip whitespace to get to reason
			while (isspace(*reason_start))
				reason_start++;

			// If there's text after minutes, that's the reason
			if (*reason_start)
				strlcpy(reason, reason_start, sizeof reason);
		}
		// If temp_arg is NOT numeric, entire argument is the reason
		else
			strlcpy(reason, argument, sizeof reason);
	}

	if (shutdownData.eShutdownType == TimedShutdownData::AUTOREBOOT)
	{
		if (!str_cmp(arg, "copyover"))
		{
			shutdownData.eShutdownType = TimedShutdownData::AUTOREBOOT_COPYOVER;
			send_to_all("&+R*** Code will copyover at auto reboot. ***&N\n");
		}
		return; // AutoReboots cannot be cycled.
	}
	if (shutdownData.eShutdownType == TimedShutdownData::AUTOREBOOT_COPYOVER)
	{
		if (!str_cmp(arg, "copyover"))
		{
			shutdownData.eShutdownType = TimedShutdownData::AUTOREBOOT;
			send_to_all("&+R*** Code will no longer copyover at auto reboot. ***&N\n");
		}
		return; // AutoReboots cannot be cycled.
	}

	// if there is a pending shutdown, cancel it now...
	if ((shutdownData.eShutdownType != TimedShutdownData::NONE))
	{
		snprintf(buf, 100, "&+R*** Scheduled %s cancelled ***&n\n", type);
		send_to_all(buf);
		snprintf(buf, 100, "Scheduled %s cancelled by %s", type, GET_NAME(ch));
		shutdownData.eShutdownType = TimedShutdownData::NONE;
		wizlog(60, "%s", buf);
		sql_log(ch, WIZLOG, "Shutdown cancelled by %s", GET_NAME(ch));
		// Delete shutdown info file when cancelled
		unlink("logs/shutdown_info.txt");
	}

	if (!*arg)
	{
		send_to_char(
			"Syntax: shutdown <ok | reboot | copyover> [minutes] [reason]\r\n"
			"\r\n"
			"Examples:\r\n"
			"  shutdown reboot                    - Immediate reboot (no reason given)\r\n"
			"  shutdown reboot 5                  - Reboot in 5 minutes (no reason given)\r\n"
			"  shutdown reboot updating new code  - Immediate reboot with reason\r\n"
			"  shutdown reboot 5 emergency patch  - Reboot in 5 minutes with reason\r\n"
			"  shutdown copyover fixing bug       - Hot restart with reason\r\n"
			"  shutdown ok server maintenance     - Shutdown (no restart) with reason\r\n"
			"\r\n"
			"Notes:\r\n"
			"  - Scheduled shutdowns are cancelled when the scheduler leaves the game\r\n"
			"    or uses the shutdown command again\r\n"
			"  - Reason is optional but recommended for tracking purposes\r\n",
			ch);
		return;
	}

	if (!str_cmp(arg, "ok"))
	{
		shutdownData.eShutdownType = TimedShutdownData::OK;
	}
	else if (!str_cmp(arg, "reboot"))
	{
		shutdownData.eShutdownType = TimedShutdownData::REBOOT;
	}
	else if (!str_cmp(arg, "autoreboot"))
	{
		shutdownData.eShutdownType = TimedShutdownData::AUTOREBOOT;
	}
	else if (!str_cmp(arg, "copyover"))
	{
		shutdownData.eShutdownType = TimedShutdownData::COPYOVER;
	}
	else if (!str_cmp(arg, "pwipe"))
	{
		argument = one_argument(argument, arg);
		if (!str_cmp(arg, "confirm"))
		{
			argument = one_argument(argument, arg);
			if (!str_cmp(arg, "yes"))
			{
				send_to_char(
					"&-RYou've done it now.. the world is really going away!!!&n\n\r",
					ch);
				if (GET_LEVEL(ch) < OVERLORD)
				{
					send_to_char(
						"&=RLThe world resists your attempt to destroy it!  You must attain a higher level to do this.&n\n\r",
						ch);
					return;
				}
				send_to_char(
					"&=glYou have _five_ minutes to reconsider and cancel the shutdown.&n\n\r",
					ch);
				shutdownData.eShutdownType = TimedShutdownData::PWIPE;
				mins_to_reboot = 5;
			}
			else
			{
				send_to_char(
					"&=RWYou must &n&=RL'shutdown pwipe confirm yes'&=RW to do this, as it's going to DELETE everything!!!&n\n\r",
					ch);
				return;
			}
		}
		else
		{
			send_to_char(
				"&-rYou must &+W'shutdown pwipe confirm'&n&-r to do this, as it's going to DELETE everything!!!&n\n\r",
				ch);
			send_to_char(
				"&-rNot to be used for a partial wipe, and will wipe files you didn't know you had!!!&n\n\r",
				ch);
			return;
		}
	}
	else if (!str_cmp(arg, "segfault"))
	{
		sql_log(ch, WIZLOG, "Shutdown - SIGSEGV by %s", GET_NAME(ch));
		panic_corruption("actwiz", "shutdown segfault requested by %s", GET_NAME(ch));
	}
	else
	{
		send_to_char("Go shut down someone your own size.\n", ch);
		return;
	}
	type = scheduled_shutdown_type_name(shutdownData.eShutdownType, false);
	strcpy(shutdownData.IssuedBy, GET_NAME(ch));
	strlcpy(shutdownData.Reason, reason, sizeof shutdownData.Reason);
	shutdownData.next_warning = -1;
	shutdownData.reboot_time = (time(0) + (mins_to_reboot * 60));
	snprintf(buf, sizeof buf, "Scheduled %s initiated by %s in %d minutes.", type, GET_NAME(ch),
		 mins_to_reboot);
	wizlog(60, "%s", buf);
	sql_log(ch, WIZLOG, "%s initiated by %s in %d minutes.", type, GET_NAME(ch),
		mins_to_reboot);
	// calling the event will start the event
	timedShutdown(NULL, NULL, NULL, NULL);
}

namespace
{
// do_load used to hand the new object straight to the wizard with obj_to_char() and
// submit no transfer, so it arrived carrying a uid the ownership ledger had never heard
// of. The next save wrote a player_items row with no item_current_owner row - the orphan
// that used to make the character permanently unloadable. Establish ownership the way
// every other grant does, and let the completion do the live move.
struct wizard_load_context
{
	uint64_t item_uid;
	int32_t room;
	int32_t to_room;
};

P_obj find_object_by_uid(uint64_t item_uid)
{
	if (!item_uid)
		return NULL;
	for (P_obj object = object_list; object; object = object->next)
		if (object->obj_uid == item_uid)
			return object;
	return NULL;
}

void wizard_load_completion(P_char actor, bool committed, const item_transfer_result &,
			    unsigned int, const uint8_t *encoded, size_t encoded_size)
{
	wizard_load_context context = {};
	if (!actor || !encoded || encoded_size != sizeof(context))
		return;
	memcpy(&context, encoded, sizeof(context));
	P_obj object = find_object_by_uid(context.item_uid);
	if (!object)
		return;
	if (!committed)
	{
		// Nothing owns it and nothing ever will; leaving it in play would recreate
		// exactly the orphan this submission exists to prevent.
		send_to_char(
			"The ownership authority did not commit; the object was discarded.\r\n",
			actor);
		if (OBJ_NOWHERE(object))
			extract_obj(object, FALSE);
		return;
	}
	if (!OBJ_NOWHERE(object) || context.room <= NOWHERE || context.room > top_of_world)
	{
		logit(LOG_FILE, "wizard load committed but live publication was stale (uid=%llu)",
		      (unsigned long long)context.item_uid);
		send_to_char("The ownership authority committed, but live publication failed.\r\n",
			     actor);
		return;
	}
	act("$n makes a strange magical gesture.", TRUE, actor, 0, 0, TO_ROOM);
	act("$n has created $p!", TRUE, actor, object, 0, TO_ROOM);
	act("You have created $p!", FALSE, actor, object, 0, TO_CHAR);
	if (context.to_room)
		obj_to_room(object, context.room);
	else
		obj_to_char(object, actor);
}

// Same-owner establish: the object has no ledger row yet, so source and destination are
// both the owner it is about to have.
bool submit_wizard_load_establish(P_char actor, P_obj object, bool to_room)
{
	if (!actor || !object || !OBJ_NOWHERE(object) || actor->in_room <= NOWHERE ||
	    actor->in_room > top_of_world)
		return false;
	const item_owner_identity owner =
		to_room ? item_owner_identity{ item_owner_type::room,
					       static_cast<uint64_t>(world[actor->in_room].number),
					       0 } :
			  item_owner_identity{ item_owner_type::player,
					       static_cast<uint64_t>(GET_PID(actor)), 0 };
	const wizard_load_context context = { object->obj_uid, actor->in_room, to_room ? 1 : 0 };
	return item_movement_transaction_submit(
		actor, object, NULL, owner, owner, item_transfer_reason::creation,
		object->R_num >= 0 ? obj_index[object->R_num].virtual_number : 0,
		wizard_load_completion, &context, sizeof(context), NULL, NULL, nullptr,
		economic_source_kind::administrator);
}
} // namespace

void do_load(P_char ch, char *argument, int /*cmd*/)
{
	P_char mob;
	P_obj obj;
	char type[MAX_INPUT_LENGTH], num[MAX_INPUT_LENGTH];
	int l_num, r_num;

	if (IS_NPC(ch))
		return;

	argument_interpreter(argument, type, num);

	if (*type && !isdigit(*num) && (GET_LEVEL(ch) >= GREATER_G || god_check(GET_NAME(ch))))
		if (is_abbrev(type, "char"))
		{
			do_read_player(ch, num, 0);
			return;
		}
	if (!*type || !*num || !isdigit(*num))
	{
		send_to_char("Syntax:\nload <'char' | 'obj'> <number>.\n", ch);
		return;
	}
	if ((l_num = atoi(num)) < 0)
	{
		send_to_char("A NEGATIVE number??\n", ch);
		return;
	}
	if (is_abbrev(type, "char") || is_abbrev(type, "mobile"))
	{
		if ((r_num = real_mobile(l_num)) < 0)
		{
			send_to_char("There is no monster with that number.\n", ch);
			return;
		}
		mob = read_mobile(r_num, REAL);
		if (!mob)
		{
			logit(LOG_DEBUG, "do_load(): mob %d [%d] not loadable", r_num,
			      mob_index[r_num].virtual_number);
			return;
		}
		GET_BIRTHPLACE(mob) = world[ch->in_room].number;
		apply_zone_modifier(mob);
		char_to_room(mob, ch->in_room, 0);

		if (GET_HIT(mob) > GET_MAX_HIT(mob))
			GET_HIT(mob) = GET_MAX_HIT(mob);

		if (GET_LEVEL(ch) < OVERLORD)
			wizlog(GET_LEVEL(ch), "%s loaded mob %s '%s' in [%d]", GET_NAME(ch), num,
			       GET_NAME(mob), world[ch->in_room].number);
		logit(LOG_WIZLOAD, "%s loaded mob %s '%s' in [%d]", GET_NAME(ch), num,
		      GET_NAME(mob), world[ch->in_room].number);
		sql_log(ch, WIZLOG, "Loaded mob %s &n[%s]", J_NAME(mob), num);
		act("$n makes a quaint, magical gesture with one hand.", TRUE, ch, 0, 0, TO_ROOM);
		act("$n has created $N!", FALSE, ch, 0, mob, TO_ROOM);
		act("You have created $N!", FALSE, ch, 0, mob, TO_CHAR);
	}
	else if (is_abbrev(type, "object") || is_abbrev(type, "item"))
	{
		if ((r_num = real_object(l_num)) < 0)
		{
			send_to_char("There is no object with that number.\n", ch);
			return;
		}
		obj = read_object(r_num, REAL);
		if (!obj)
		{
			logit(LOG_DEBUG, "do_load(): obj %d [%d] not loadable", r_num,
			      obj_index[r_num].virtual_number);
			return;
		}
		if (GET_LEVEL(ch) < OVERLORD)
			wizlog(GET_LEVEL(ch), "%s loaded obj %s '%s' in [%d]", GET_NAME(ch), num,
			       obj->short_description, world[ch->in_room].number);
		logit(LOG_WIZLOAD, "%s loaded obj %s '%s' [%d]", GET_NAME(ch), num,
		      obj->short_description, world[ch->in_room].number);
		sql_log(ch, WIZLOG, "Loaded obj %s &n[%s]", obj->short_description, num);
		obj->z_cord = ch->specials.z_cord;
		// An object with no uid never reaches the ownership ledger at all, so there is
		// nothing to establish and nothing to orphan.
		const bool to_room = !IS_SET(obj->wear_flags, ITEM_TAKE);
		if (!obj->obj_uid)
		{
			act("$n makes a strange magical gesture.", TRUE, ch, 0, 0, TO_ROOM);
			act("$n has created $p!", TRUE, ch, obj, 0, TO_ROOM);
			act("You have created $p!", FALSE, ch, obj, 0, TO_CHAR);
			if (to_room)
				obj_to_room(obj, ch->in_room);
			else
				obj_to_char(obj, ch);
		}
		else if (!submit_wizard_load_establish(ch, obj, to_room))
		{
			send_to_char("Item ownership is busy; nothing was created.\r\n", ch);
			extract_obj(obj, FALSE);
		}
	}
	else
		send_to_char("That'll have to be either 'char' or 'obj'.\n", ch);
}

/* clean a room of all mobiles and objects */
void do_purge(P_char ch, char *argument, int /*cmd*/)
{
	P_char vict, next_v;
	P_obj obj, next_o;
	int level;
	char name[MAX_INPUT_LENGTH];
	char buf[512], buf2[512];
	time_t laston, timegone;
	FILE *flist, *f;
	P_ship temp;

	if (IS_NPC(ch))
		return;

	level = MIN(62, GET_LEVEL(ch));
	one_argument(argument, name);

	// Argument supplied. destroy single object or char
	if (*name)
	{
		if ((vict = get_char_room_vis(ch, name)))
		{
			if ((GET_LEVEL(vict) >= level) && IS_PC(vict))
			{
				send_to_char("Oh no you don't!\n", ch);
				return;
			}
			if ((IS_PC(vict) || IS_MORPH(vict)) && (level < GREATER_G))
			{
				send_to_char("You are too lame to be able to purge chars!\n", ch);
				return;
			}
			act("$n disintegrates $N, who is reduced to a small pile of ashes.", FALSE,
			    ch, 0, vict, TO_NOTVICT);

			wizlog(GET_LEVEL(ch), "%s has purged %s [%d]", GET_NAME(ch), GET_NAME(vict),
			       world[ch->in_room].number);
			logit(LOG_WIZ, "%s has purged %s [%d]", GET_NAME(ch), GET_NAME(vict),
			      world[ch->in_room].number);

			sql_log(ch, WIZLOG, "Purged %s", GET_NAME(vict));

			if (IS_NPC(vict))
			{
				extract_char(vict);
				vict = NULL;
			}
			else
			{
				// If it's not an immortal.
				if (GET_LEVEL(ch) < MINLVLIMMORTAL)
				{
					update_ingame_racewar(-GET_RACEWAR(ch));
				}

				/* player will lose all objects! */
				extract_char(vict);
				writeCharacter(vict, 2, NOWHERE);
				if (vict->desc)
					close_socket(vict->desc);
				else
					free_char(vict);
				vict->desc = 0;
				vict = NULL;
			}
		}
		else if ((obj = get_obj_in_list_vis(ch, name, world[ch->in_room].contents)))
		{
			if (obj->R_num == real_object(VOBJ_WALLS))
			{
				send_to_char("Use 'dispel magic' to get rid of walls.\n", ch);
				return;
			}
			// Use storage command to delete these items.
			if (GET_ITEM_TYPE(obj) == ITEM_STORAGE)
			{
				send_to_char(
					"Use the storage command if you want to get rid of this item.\n",
					ch);
				return;
			}
			act("$n destroys $p.", TRUE, ch, obj, 0, TO_ROOM);

			wizlog(GET_LEVEL(ch), "%s has purged %s [%d]", GET_NAME(ch),
			       obj->short_description, world[ch->in_room].number);
			logit(LOG_WIZ, "%s has purged %s [%d]", GET_NAME(ch),
			      obj->short_description, world[ch->in_room].number);
			sql_log(ch, WIZLOG, "Purged %s", obj->short_description);

			if (obj_index[obj->R_num].virtual_number == VOBJ_ALL_SHIPS)
			{
				temp = shipObjHash.find(obj);
				if (!temp)
					return;
				shipObjHash.erase(temp);
				delete_ship(temp);
			}
			else
			{
				if (IS_ARTIFACT(obj))
				{
					act("You purged artifact $p; not removing it from the artifact list.",
					    FALSE, ch, obj, 0, TO_CHAR);
					snprintf(
						buf, 512,
						"&+WIf you wish to clear the artifact entry, use '&+wartifact clear %d&+W'.&n\n\r",
						OBJ_VNUM(obj));
				}
				extract_obj(obj);
				obj = NULL;
			}
		}
		else if (!str_cmp("pfiles", name))
		{
			flist = fopen("Players/pfiles", "r");
			if (!flist)
			{
				return;
			}
			while (fscanf(flist, " %s \n", buf) != EOF)
			{
				checked_snprintf(buf2, sizeof buf2, "Players/%c/%s", LOWER(*buf),
						 buf);
				f = fopen(buf2, "r");
				vict = (struct char_data *)mm_get(dead_mob_pool);
				ensure_pconly_pool();
				vict->only.pc = (struct pc_only_data *)mm_get(dead_pconly_pool);
				if (restoreCharOnly(vict, skip_spaces(buf)) < 0 || !vict)
				{
					if (vict)
						free_char(vict);
					continue;
				}
				laston = vict->player.time.saved;

				/* can't be too careful */

				if ((time(0) - laston) >= 0)
					timegone = (time(0) - laston) / 60;
				else
					timegone = 0;

				if (((timegone > 1440) && ((timegone / 1440) > 60)) &&
				    (GET_LEVEL(vict) <= 56))
				{
					deleteCharacter(vict);
				}
				if (vict)
					free_char(vict);
				fclose(f);
			}
			fclose(flist);
		}
		else
		{
			send_to_char("I don't know anyone or anything by that name.\n", ch);
			return;
		}

		send_to_char("Ok.\n", ch);
	}
	else
	{ /* no argument. clean out the room */
		if (IS_NPC(ch))
		{
			send_to_char("Don't... You would only kill yourself...\n", ch);
			return;
		}
		wizlog(GET_LEVEL(ch), "%s has purged the room %d", GET_NAME(ch),
		       world[ch->in_room].number);
		logit(LOG_WIZ, "%s has purged the room %d", GET_NAME(ch),
		      world[ch->in_room].number);
		sql_log(ch, WIZLOG, "Purged room");
		act("$n gestures... you are surrounded by godly power!", FALSE, ch, 0, 0, TO_ROOM);
		send_to_room("The world seems a little cleaner.\n", ch->in_room);

		for (vict = world[ch->in_room].people; vict; vict = next_v)
		{
			next_v = vict->next_in_room;
			if (IS_NPC(vict) && !IS_MORPH(vict))
			{
				extract_char(vict);
				vict = NULL;
			}
		}

		for (obj = world[ch->in_room].contents; obj; obj = next_o)
		{
			next_o = obj->next_content;

			if (obj->R_num == real_object(VOBJ_WALLS))
			{
				continue;
			}

			if (IS_ARTIFACT(obj))
			{
				act("Skipping artifact $p.  You must purge that item by name if you want it gone.",
				    FALSE, ch, obj, 0, TO_ROOM);
				continue;
			}

			if ((obj->wear_flags & ITEM_TAKE) ||
			    (obj->type == ITEM_CORPSE && !obj->contains))
			{
				extract_obj(obj);
				obj = NULL;
			}
		}
	}
}

/*
   Krov: major modifications to roll_basic_attributes are
   - fixed "stat points" plus a bit of luck for roll fanatics
   - class restrictions are obeyed, since new nanny fixes class before
   rolling the stats
   - kept the range system, modified for total stats: flag=-1 means
   punishment, 0 is standard, 1..3 better and better and >=4
   means at random (don't use last one...)
   - added karma and luck to total points, players can't see them at
   roll time, so they can't control their stats completely
 */

// This function now assigns a static low level base value to all
// "normal" stats(not luck or karma). - Jexni

void roll_basic_attributes(P_char ch, int type)
{
	int faces, rolls, base, value;
	/* screw 'bell curves' and stat totalling, let's keep it simple */

	/*
	  int i, rolls[10], statp, temp, total, sides;
	  // this gives the total points distributed on _10_ stats, according to type+1
	  static int totlim[5] =
	  {
	    250, 550, 600, 650, 700
	  };

	  // Set minimum of points for class, 25 min in all stats,
	  //   except for luck&karma (invis to player)
	  temp = 0;
	  for (i = 0; i < 8; i++) {
	    // class requirements?
	    rolls[i] = min_stats_for_class[GET_CLASS(ch)][i];
	    // min stat
	    if (rolls[i] < 25)
	      rolls[i] = 25;
	    temp += rolls[i];
	  }

	  // determine how many points will be distributed
	  if (type >= 4)
	    total = number(temp, 800);
	  else {
	    total = totlim[type + 1];
	    total += (dice(3, 21) + 17);
	  }

	  if (temp > total)
	    // probably punished, no additional points
	    total = 0;
	  else
	    total -= temp;

	  // distribute the rest of the points, choose random
	     stat and add with a pseudo-bell made by 3 dices
	  while (total) {
	    temp = number(0, 7);
	    sides = (100 - rolls[temp]) / 3;
	    if (sides)
	      statp = dice(3, sides);
	    else
	      statp = 100 - rolls[temp];
	    if (statp < total) {
	      rolls[temp] += statp;
	      total -= statp;
	    } else {
	      rolls[temp] += total;
	      total = 0;
	    }
	  }

	  ch->base_stats.Str = ch->curr_stats.Str = rolls[0];
	  ch->base_stats.Dex = ch->curr_stats.Dex = rolls[1];
	  ch->base_stats.Agi = ch->curr_stats.Agi = rolls[2];
	  ch->base_stats.Con = ch->curr_stats.Con = rolls[3];
	  ch->base_stats.Pow = ch->curr_stats.Pow = rolls[4];
	  ch->base_stats.Int = ch->curr_stats.Int = rolls[5];
	  ch->base_stats.Wis = ch->curr_stats.Wis = rolls[6];
	  ch->base_stats.Cha = ch->curr_stats.Cha = rolls[7];
	  ch->base_stats.Kar = ch->curr_stats.Kar = number(1, 100);
	  ch->base_stats.Luk = ch->curr_stats.Luk = number(1, 100);
	*/

	// Bad rolls - 44 to 80.
	if (type == ROLL_BAD)
	{
		rolls = 4;
		faces = 10;
		base = 40;
	}
	// Normal rolls - 80 to 95.
	else if (type == ROLL_NORMAL)
	{
		// Used to be 5 8 53, then 3 6 77.
		rolls = 1;
		faces = 4;
		base = 86;
	}
	// Normal mob rolls - 68 - 98.
	else if (type == ROLL_MOB_NORMAL)
	{
		rolls = 3;
		faces = 11;
		base = 65;
	}
	// Good mob rolls - 80 - 98.
	else if (type == ROLL_MOB_GOOD)
	{
		rolls = 3;
		faces = 7;
		base = 77;
	}
	// Elite mob rolls - 91 - 100
	else if (type == ROLL_MOB_ELITE)
	{
		rolls = 3;
		faces = 4;
		base = 88;
	}
	// Buggy, give 51 to 100.
	else
	{
		debug("roll_basic_attributes: Bad type (%d) on char '%s' %d.", type, J_NAME(ch),
		      GET_ID(ch));
		logit(LOG_WIZ, "roll_basic_attributes: Bad type (%d) on char '%s' %d.", type,
		      J_NAME(ch), GET_ID(ch));
		rolls = 1;
		faces = 50;
		base = 50;
	}

	if (type != ROLL_MOB_NORMAL && type != ROLL_MOB_GOOD && type != ROLL_MOB_ELITE)
	{
		for (int i = 0; i < MAX_ATTRIBUTES; i++)
			ch->base_stats[i] = ch->curr_stats[i] = dice(rolls, faces) + base;
	}
	// Mobs may have a 'lower limit' already entered in base_stats.
	else
	{
		for (int i = 0; i < MAX_ATTRIBUTES; i++)
			if ((value = dice(rolls, faces) + base) > ch->base_stats[i])
				ch->base_stats[i] = ch->curr_stats[i] = value;
	}
}

void do_advance(P_char ch, char *argument, int /*cmd*/)
{
	P_char victim;
	char name[MAX_INPUT_LENGTH], level[MAX_INPUT_LENGTH];
	int newlevel = 0, oldlevel;

	if (IS_NPC(ch))
		return;

	argument_interpreter(argument, name, level);

	if (*name)
	{
		if (!(victim = get_char_vis(ch, name)))
		{
			send_to_char("That player is not here.\n", ch);
			return;
		}
	}
	else
	{
		send_to_char("Advance who?\n", ch);
		return;
	}

	if (IS_NPC(victim))
	{
		send_to_char("NO! Not on NPC's.\n", ch);
		return;
	}
	oldlevel = GET_LEVEL(victim);

	if (oldlevel == 0)
		;
	else if (!*level)
	{
		send_to_char("You must supply a level number.\n", ch);
		return;
	}
	else
	{
		if (!isdigit(*level))
		{
			send_to_char("Second argument must be a positive integer.\n", ch);
			return;
		}
		newlevel = atoi(level);

		if (newlevel <= oldlevel)
		{
			send_to_char("Can't dimnish a players status.\n", ch);
			return;
		}
	}

	if (newlevel > GET_LEVEL(ch))
	{
		send_to_char("So sorry Charlie!  You may not advance someone past your level.\n",
			     ch);
		return;
	}
	if (newlevel > OVERLORD)
	{
		send_to_char("62 is the highest possible level.\n", ch);
		return;
	}

	if (newlevel >= MINLVLIMMORTAL && GET_LEVEL(ch) < OVERLORD)
	{
		send_to_char("You aren't allowed to create new gods.\n", ch);
		return;
	}

	if (newlevel != 1)
		send_to_char("You feel generous.\n", ch);

	act("$n makes some strange gestures.\n"
	    "A strange feeling comes upon you.   Like a giant hand, light comes down\n"
	    "from above, grabbing your body, which begins to pulse with colored lights\n"
	    "from inside.  Your head seems to be filled with demons from another plane\n"
	    "as your body dissolves to the elements of time and space itself.\n"
	    "Suddenly a silent explosion of light snaps you back to reality.  You feel\n"
	    "improved!",
	    FALSE, ch, 0, victim, TO_VICT);

	if (oldlevel == 0)
	{
		do_start(victim, 0);
		return;
	}

	advance_to_level(victim, newlevel);

	GET_EXP(victim) = 1;

	if (newlevel >= MINLVLIMMORTAL && oldlevel < MINLVLIMMORTAL)
	{
		send_to_char("A sense of timelessness overwhelms you.  "
			     "You feel the myriad aches and pains\n"
			     "of mortal flesh drop away.  Power courses through your body "
			     "and you seem to\nactually be glowing!\n\n"
			     "Welcome to immortality!\n\n",
			     victim);
		victim->specials.act |= (PLR_PETITION | PLR_AGGIMMUNE | PLR_WIZLOG | PLR_STATUS |
					 PLR_VNUM | PLR_NAMES);
		REMOVE_BIT(victim->specials.act, PLR_ANONYMOUS);
		victim->only.pc->prompt |= (PROMPT_VIS);
	}

	wizlog(GET_LEVEL(ch), " %s has advanced %s to level %d", GET_NAME(ch), GET_NAME(victim),
	       GET_LEVEL(victim));
	logit(LOG_WIZ, "%s advanced %s to level %d", GET_NAME(ch), GET_NAME(victim),
	      GET_LEVEL(victim));
	sql_log(ch, WIZLOG, "Advanced %s to level %d", GET_NAME(victim), GET_LEVEL(victim));
	do_restore(ch, name, -4);
}

#define REROLL_SYNTAX \
	"Syntax:  reroll <name> [<modifier>]\n\
  Modifier is a number:\n\
    0 - miserable\n\
    1 - normal (default)\n\
    2 - good, still in 'normal' range\n\
    3 - great, can be above normal\n\
    4 - exceptional, well above average\n"

void do_reroll(P_char ch, char *argument, int /*cmd*/)
{
	P_char victim;
	int flag;
	char name[MAX_INPUT_LENGTH], modifier[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];

	if (IS_NPC(ch))
		return;

	argument_interpreter(argument, name, modifier);
	if (!*name)
	{
		send_to_char(REROLL_SYNTAX, ch);
		return;
	}
	else if (!(victim = get_char_vis(ch, name)))
	{
		send_to_char("No-one by that name in the world.\n", ch);
		return;
	}
	if (!*modifier || (*modifier == '1'))
		flag = ROLL_NORMAL;
	else if (*modifier == '0')
		flag = ROLL_BAD;
	else if (*modifier == '2')
		flag = ROLL_MOB_NORMAL;
	else if (*modifier == '3')
		flag = ROLL_MOB_GOOD;
	else if (*modifier == '4')
		flag = ROLL_MOB_ELITE;
	else
	{
		send_to_char(REROLL_SYNTAX, ch);
		return;
	}

	roll_basic_attributes(victim, flag);
	if (!IS_TRUSTED(ch))
		send_to_char("Rerolled...\n", ch);
	else
	{
		snprintf(
			buf, MAX_STRING_LENGTH,
			"%s Rerolled:     Avg:(%d)\n  S:%3d  D:%3d  A:%3d  C:%3d\n  P:%3d  I:%3d  W:%3d Ch:%3d\n",
			GET_NAME(victim),
			(GET_C_STR(victim) + GET_C_DEX(victim) + GET_C_AGI(victim) +
			 GET_C_CON(victim) + GET_C_POW(victim) + GET_C_INT(victim) +
			 GET_C_WIS(victim) + GET_C_CHA(victim)) /
				8,
			GET_C_STR(victim), GET_C_DEX(victim), GET_C_AGI(victim), GET_C_CON(victim),
			GET_C_POW(victim), GET_C_INT(victim), GET_C_WIS(victim), GET_C_CHA(victim));
		send_to_char(buf, ch);

		if (affect_total(victim, TRUE))
			return; /* whoops */
	}
	if (IS_TRUSTED(ch))
	{
		wizlog(GET_LEVEL(ch), "%s has rerolled %s %s.", GET_NAME(ch), GET_NAME(victim),
		       (flag == ROLL_BAD)	 ? " miserably" :
		       (flag == ROLL_NORMAL)	 ? "normal" :
		       (flag == ROLL_MOB_NORMAL) ? "mob-normal" :
		       (flag == ROLL_MOB_GOOD)	 ? " mob-good" :
						   " mob-elite");
		logit(LOG_WIZ, "%s has rerolled %s %s.", GET_NAME(ch), GET_NAME(victim),
		      (flag == ROLL_BAD)	? " miserably" :
		      (flag == ROLL_NORMAL)	? "normal" :
		      (flag == ROLL_MOB_NORMAL) ? "mob-normal" :
		      (flag == ROLL_MOB_GOOD)	? " mob-good" :
						  " mob-elite");
	}
}

#undef REROLL_SYNTAX

void do_reinitphys(P_char ch, char *arg, int /*cmd*/)
{
	P_char vict;
	P_desc p;

	/**** temp ****/
	char buf[MAX_STRING_LENGTH];

	one_argument(arg, buf);

	if (!*buf)
		send_to_char("Who do you wish to reinitialize skills for?\n", ch);
	else if (!str_cmp("all", buf))
	{
		for (p = descriptor_list; p; p = p->next)
			if (!p->connected)
			{
				vict = p->character;
				if (!IS_TRUSTED(vict))
					NewbySkillSet(vict, TRUE);
				send_to_char(
					"&+LA haze of powdery dust falls from the heavens, clogging your breathing, choking your lungs, blurring your vision. As it begins to subside, you realize all your "
					"wordly knowledge is somehow.... different.\n",
					vict);
			}
		send_to_char("Resetting of all player skills completed.\n", ch);
	}
	else if (!(vict = get_char_vis(ch, buf)))
		send_to_char("No-one by that name in the world.\n", ch);
	else if (ch == vict || !IS_TRUSTED(vict))
	{
		NewbySkillSet(vict, TRUE);
		snprintf(buf, MAX_STRING_LENGTH, "Resetting of $N's skills completed.");
		act(buf, FALSE, ch, 0, vict, TO_CHAR);
	}
	else
		send_to_char("No gods please.\n", ch);
	return;
	/****/

	if (GET_LEVEL(ch) <= MAXLVLMORTAL)
	{ /*
	   * mortals can only set
	   * their own attr
	   */
		send_to_char("Your height and weight have been re-randomed!\n", ch);
		init_height_weight(ch);
		return;
	}
	if (!*arg)
	{
		send_to_char("usage:\n   reinitphys <targetname>\n", ch);
		return;
	}
	if (!strcmp(arg, "all"))
	{
		for (p = descriptor_list; p; p = p->next)
			if (!p->connected)
				if (CAN_SEE(ch, p->character))
					if (strcmp(GET_NAME(ch), GET_NAME(p->character)))
						do_reinitphys(ch, GET_NAME(ch), CMD_REINITPHYS);
		send_to_char("Ok, you re-random everyone's physical stats (except yourself's.\n",
			     ch);
		return;
	}
	vict = get_char_vis(ch, arg);
	if (!vict)
	{
		send_to_char("You see no char with name like that in the game!\n", ch);
		return;
	}
	act("Ok, $N's weight and height rerolled.", TRUE, ch, 0, vict, TO_CHAR);
	act("Your height and weight have been randomly rolled again by $N!", TRUE, vict, 0, ch,
	    TO_CHAR);
	set_char_height_weight(vict);
}

/* Used to demote player to level 1 (and level 1 only) */

namespace
{
struct demote_context
{
	uint32_t administrator_pid;
};

void demote_committed(P_char victim, bool committed, const epic_command_result &, unsigned int,
		      const uint8_t *raw_context, size_t context_size)
{
	if (!committed)
	{
		send_to_char("The administrative epic reset failed; demotion was not applied.\n",
			     victim);
		return;
	}
	for (int skill = 0; skill < MAX_SKILLS; ++skill)
		victim->only.pc->skills[skill].learned = 0;
	NewbySkillSet(victim, TRUE);
	victim->points.max_mana = 0;
	victim->points.max_vitality = 0;
	victim->specials.conditions[0] = 24;
	victim->specials.conditions[1] = 24;
	victim->specials.conditions[2] = 0;
	do_start(victim, 0);
	send_to_char("You have just been demoted to level one... oh well.\n", victim);
	logit(LOG_WIZ, "%s was demoted to level 1 after a committed epic reset", GET_NAME(victim));
	if (context_size == sizeof(demote_context))
	{
		demote_context context = {};
		memcpy(&context, raw_context, sizeof(context));
		P_char administrator = find_player_by_pid(context.administrator_pid);
		if (administrator)
			send_to_char("The demotion and epic reset committed successfully.\n",
				     administrator);
	}
}
} // namespace

void do_demote(P_char ch, char *argument, int /*cmd*/)
{
	char person[MAX_STRING_LENGTH];
	P_char victim;

	if (IS_NPC(ch))
		return;

	one_argument(argument, person);

	if (!*person)
	{
		send_to_char("Syntax: demote person\n", ch);
		return;
	}
	if (!(victim = get_char_vis(ch, person)))
	{
		send_to_char("No one by that name here...\n", ch);
		return;
	}
	if (IS_NPC(victim))
	{
		send_to_char("Monsters cannot be demoted.\n", ch);
		return;
	}
	if (GET_LEVEL(victim) >= MIN(62, GET_LEVEL(ch)))
	{
		send_to_char("Oh no you don't!\n", ch);
		return;
	}

	const demote_context context = { static_cast<uint32_t>(GET_PID(ch)) };
	if (victim->only.pc->epics > 0 &&
	    !epic_transaction_submit(
		    victim, -victim->only.pc->epics, epic_reason_type::admin_adjustment,
		    GET_PID(ch), EPIC_COMMAND_REQUIRE_FUNDS, critical_source_site::operator_repair,
		    critical_deadline_class::terminal, demote_committed, &context, sizeof(context)))
	{
		send_to_char("The epic transaction service is busy; demotion was not applied.\n",
			     ch);
		return;
	}
	if (victim->only.pc->epics == 0)
		demote_committed(victim, true, {}, 0, reinterpret_cast<const uint8_t *>(&context),
				 sizeof(context));
}

void do_secret(P_char ch, char *argument, int /*cmd*/)
{
	P_obj obj = NULL;
	P_char dummy;
	char buf[MAX_STRING_LENGTH];

	one_argument(argument, buf);
	if (generic_find(buf, FIND_OBJ_INV | FIND_OBJ_ROOM, ch, &dummy, &obj))
	{
		if (IS_SET(obj->extra_flags, ITEM_SECRET))
		{
			REMOVE_BIT(obj->extra_flags, ITEM_SECRET);
			send_to_char("SECRET bit removed.\n", ch);
		}
		else
		{
			SET_BIT(obj->extra_flags, ITEM_SECRET);
			send_to_char("SECRET bit set.\n", ch);
		}
	}
	else
		send_to_char("Secretize what?\n", ch);
}

/*
 * Look up an object based on a keyword
 *
 * Syntax : "lookup <room | mob | obj | random> <search_string>"
 */

// Same as GetMIA but includes seconds and no "  (" to start..
/* clone stuff - Valkur */

struct obj_data *clone_obj(P_obj obj)
{
	P_obj ocopy;
	size_t i;

	ocopy = read_object(obj->R_num, REAL);
	/* copy  */
	if (obj->name)
	{
		if (IS_SET(obj->str_mask, STRUNG_KEYS))
		{
			ocopy->name = str_dup(obj->name);
			SET_BIT(ocopy->str_mask, STRUNG_KEYS);
		}
	}
	if (obj->short_description)
	{
		if (IS_SET(obj->str_mask, STRUNG_DESC2))
		{
			ocopy->short_description = str_dup(obj->short_description);
			SET_BIT(ocopy->str_mask, STRUNG_DESC2);
		}
	}
	if (obj->description)
	{
		if (IS_SET(obj->str_mask, STRUNG_DESC1))
		{
			ocopy->description = str_dup(obj->description);
			SET_BIT(ocopy->str_mask, STRUNG_DESC1);
		}
	}
	if (obj->action_description)
	{
		if (IS_SET(obj->str_mask, STRUNG_DESC3))
		{
			ocopy->action_description = str_dup(obj->action_description);
			SET_BIT(ocopy->str_mask, STRUNG_DESC3);
		}
	}

	for (i = 0; i < ARRAY_SIZE(obj->value); i++)
		ocopy->value[i] = obj->value[i];

	for (i = 0; i < ARRAY_SIZE(obj->timer); i++)
		ocopy->timer[i] = obj->timer[i];

	ocopy->wear_flags = obj->wear_flags;
	ocopy->extra_flags = obj->extra_flags;
	ocopy->anti_flags = obj->anti_flags;
	ocopy->anti2_flags = obj->anti2_flags;
	ocopy->extra2_flags = obj->extra2_flags;
	ocopy->weight = obj->weight;
	ocopy->cost = obj->cost;
	ocopy->trap_eff = obj->trap_eff;
	ocopy->trap_dam = obj->trap_dam;
	ocopy->trap_charge = obj->trap_charge;
	ocopy->trap_level = obj->trap_level;
	ocopy->condition = obj->condition;
	// ocopy->max_condition = obj->max_condition; wipe2011
	ocopy->material = obj->material;
	ocopy->craftsmanship = obj->craftsmanship;
	ocopy->bitvector = obj->bitvector;
	ocopy->bitvector2 = obj->bitvector2;
	ocopy->bitvector3 = obj->bitvector3;
	ocopy->bitvector4 = obj->bitvector4;
	ocopy->bitvector5 = obj->bitvector5;

	for (i = 0; i < MAX_OBJ_AFFECT; i++)
	{
		ocopy->affected[i].location = obj->affected[i].location;
		ocopy->affected[i].modifier = obj->affected[i].modifier;
	}

	return ocopy;
}

void clone_container_obj(P_obj to, P_obj obj)
{
	P_obj tmp, ocopy;

	for (tmp = obj->contains; tmp; tmp = tmp->next_content)
	{
		ocopy = clone_obj(tmp);
		if (tmp->contains)
			clone_container_obj(ocopy, tmp);
		obj_to_obj(ocopy, to);
	}
}

void do_clone(P_char ch, char *argument, int /*cmd*/)
{
	P_char mob, mcopy;
	P_obj obj, ocopy;
	char type[MAX_STRING_LENGTH];
	char name[MAX_STRING_LENGTH];
	char buf[MAX_STRING_LENGTH];
	int i, j, count, where;

	if (IS_NPC(ch))
	{
		send_to_char("Uh, no you can't clone something as a mob.\n", ch);
		return;
	}
	argument = one_argument(argument, type);
	if (!*type)
	{
		send_to_char("Usage: clone <mob|obj> <name> <count>\n", ch);
		return;
	}
	argument = one_argument(argument, name);
	if (!*name)
	{
		send_to_char("Usage: clone <mob|obj> <name> <count>\n", ch);
		return;
	}
	argument = one_argument(argument, buf);
	if (!*buf)
		count = 1;
	else
		count = atoi(buf);
	if (!count)
	{
		send_to_char("No count specified!  Assuming 1.\n", ch);
		count = 1;
	}
	/* keep them from doing too many on accident ;)  */
	if (count > 100)
	{
		send_to_char("Max count set to 100 so Pook can't do it 2000 times. ;)\n", ch);
		return;
	}
	if (is_abbrev(type, "mobile") || is_abbrev(type, "character"))
	{
		if ((mob = get_char_room_vis(ch, name)) == 0)
		{
			send_to_char("Can't find any such mobile!\n", ch);
			return;
		}
		if (!training_dummy_clone_target_allowed(mob))
		{
			send_to_char("You can't clone a training dummy.\n", ch);
			return;
		}
		if (IS_PC(mob))
		{
			send_to_char("Cloning a PC?!?!? Buahahahaha!!!!\n", ch);
			send_to_char(GET_NAME(ch), mob);
			send_to_char(" just tried to clone YOU...*laugh*\n", mob);
			return;
		}
		if (GET_RNUM(mob) < 0)
		{
			send_to_char("You can't clone that mob!!\n", ch);
			return;
		}
		for (i = 0; i < count; i++)
		{
			if (!(mcopy = read_mobile(GET_RNUM(mob), REAL)))
				break;

			/* copy    */
			if (mob->player.name)
			{
				if (IS_SET(mob->only.npc->str_mask, STRUNG_KEYS))
				{
					mcopy->player.name = str_dup(mob->player.name);
					SET_BIT(mcopy->only.npc->str_mask, STRUNG_KEYS);
				}
			}
			if (mob->player.short_descr)
			{
				if (IS_SET(mob->only.npc->str_mask, STRUNG_DESC2))
				{
					mcopy->player.short_descr =
						str_dup(mob->player.short_descr);
					SET_BIT(mcopy->only.npc->str_mask, STRUNG_DESC2);
				}
			}
			if (mob->player.long_descr)
			{
				if (IS_SET(mob->only.npc->str_mask, STRUNG_DESC1))
				{
					mcopy->player.long_descr = str_dup(mob->player.long_descr);
					SET_BIT(mcopy->only.npc->str_mask, STRUNG_DESC1);
				}
			}
			if (mob->player.description)
			{
				if (IS_SET(mob->only.npc->str_mask, STRUNG_DESC3))
				{
					mcopy->player.description =
						str_dup(mob->player.description);
					SET_BIT(mcopy->only.npc->str_mask, STRUNG_DESC3);
				}
			}
			/* clone EQ equiped  */
			for (j = 0; j < MAX_WEAR; j++)
			{
				if (mob->equipment[j])
				{
					/* clone mob->equipment[j]  */
					ocopy = clone_obj(mob->equipment[j]);
					if (mob->equipment[j]->contains)
					{
						clone_container_obj(ocopy, mob->equipment[j]);
					}
					equip_char(mcopy, ocopy, j, 0);
				}
			}
			/* clone EQ carried  */
			if (mob->carrying)
				for (obj = mob->carrying; obj; obj = obj->next_content)
				{
					ocopy = clone_obj(obj);
					if (obj->contains)
						clone_container_obj(ocopy, obj);
					/* move obj to cloned mobs carrying  */
					obj_to_char(ocopy, mcopy);
				}
			char_to_room(mcopy, ch->in_room, -1);
			act("$n has just made a clone of $N!", FALSE, ch, 0, mob, TO_ROOM);
			act("You make a clone of $N.", FALSE, ch, 0, mob, TO_CHAR);
		}
		if (IS_TRUSTED(ch) && GET_LEVEL(ch) < OVERLORD)
		{
			wizlog(GET_LEVEL(ch), "%s just cloned %s %d times [&+C%d&N]", GET_NAME(ch),
			       mob->player.short_descr, count, world[ch->in_room].number);
			logit(LOG_WIZ, "%s cloned %s %d times [&+C%d&N]", GET_NAME(ch),
			      mob->player.short_descr, count, world[ch->in_room].number);
			sql_log(ch, WIZLOG, "Cloned %s &n%d times", mob->player.short_descr, count);
		}
	}
	else if (is_abbrev(type, "object"))
	{
		if ((obj = get_obj_in_list_vis(ch, name, ch->carrying)))
			where = 1;
		else if ((obj = get_obj_in_list_vis(ch, name, world[ch->in_room].contents)))
			where = 2;
		else
		{
			send_to_char("Can't find any such object!!\n", ch);
			return;
		}
		if (obj->R_num < 0)
		{
			send_to_char("You can't clone that object!!\n", ch);
			return;
		}
		for (i = 0; i < count; i++)
		{
			ocopy = clone_obj(obj);
			ocopy->type = obj->type;

			if (obj->contains)
				clone_container_obj(ocopy, obj);
			act("$n has just made a clone of $p!", FALSE, ch, obj, 0, TO_ROOM);
			act("You make a clone of $p.", FALSE, ch, obj, 0, TO_CHAR);
			if (where == 1)
				obj_to_char(ocopy, ch);
			else
				obj_to_room(ocopy, ch->in_room);
		}
		if (IS_TRUSTED(ch) && GET_LEVEL(ch) < OVERLORD)
		{
			wizlog(GET_LEVEL(ch), "%s just cloned %s %d times [&+C%d&N]", GET_NAME(ch),
			       obj->short_description, count, world[ch->in_room].number);
			logit(LOG_WIZ, "%s cloned %s %d times [&+C%d&N]", GET_NAME(ch),
			      obj->short_description, count, world[ch->in_room].number);
			sql_log(ch, WIZLOG, "Cloned %s &n%d times", obj->short_description, count);
		}
	}
	else
	{
		send_to_char("Usage: clone <mob|obj> <name> <count>\n", ch);
		return;
	}
	return;
}

#if 0 /* don't do anything with this yet (neb)  */
void do_proc(P_char ch, char *args, int cmd)
{
  /*
   * interface for listing, loading, and unloading libraries of special
   * procedures
   *
   * proc [list | [load | unload <libname>]]
   *
   * with no parameter, generate a syntax message
   *
   * list - lists all libs the mud knows about, and if they are loaded
   * or not
   *
   * load - (only usable by 59+) loads <libname> and assigns function
   * pointers.  (if <libname> is already loaded, returns error)
   *
   * unload - (only usable by 59+) unloads <libname> and assigns all
   * used function pointers to stub functions. (if <libname> isn't
   * loaded, returns an error).
   *
   */
#ifndef SHLIB
  send_to_char("The mud wasn't compiled with dynamic proc loading enabled\n",
               ch);
#else

  char     func[MAX_STRING_LENGTH];
  char     libname[MAX_STRING_LENGTH];
  int      i;

  if(!ch || !args || IS_NPC(ch) || !IS_TRUSTED(ch))
    return;

  if(!*args)
  {

    send_to_char("Usage: proc \n", ch);
    send_to_char
      ("            list          - list proc libraries the mud knows about\n",
       ch);
    if(GET_LEVEL(ch) < 59)
      return;
    send_to_char
      ("            load <name>   - load the proc library called <name>\n",
       ch);
    send_to_char
      ("            unload <name> - unload the proc library called <name>\n",
       ch);
    return;
  }
  args = one_argument(args, func);
  if(!str_cmp(func, "list"))
  {
    /*
     * list libs, their status, and exit
     */

    send_to_char("\nLib name                Status"
                 "\n--------                ------\n", ch);

    for (i = 0; dynam_proc_list[i].name; i++)
    {
      snprintf(libname, MAX_STRING_LENGTH, "%-23s %-8s\n", dynam_proc_list[i].name,
              dynam_proc_list[i].handle ? "Loaded" : "Unloaded");
      send_to_char(libname, ch);
    }
    return;
  }
  one_argument(args, libname);
  if(GET_LEVEL(ch) >= 59)
    if(!str_cmp(func, "load"))
    {
      for (i = 0; dynam_proc_list[i].name; i++)
        if(!str_cmp(dynam_proc_list[i].name, libname))
        {
          if(dynam_proc_list[i].handle)
          {
            send_to_char("That lib is already loaded!\n", ch);
          }
          else
          {
            if(load_proc_lib(libname))
            {
              send_to_char("Library loaded successfully!\n", ch);
              /*
               * LOG ME!!
               */
            }
            else
            {
              send_to_char("Unable to load lib!\n", ch);
              /*
               * LOG ME!!
               */
            }
          }
          return;
        }
      send_to_char("No such library exists.\n", ch);
      return;
    }
    else if(!str_cmp(func, "unload"))
    {
      for (i = 0; dynam_proc_list[i].name; i++)
        if(!str_cmp(dynam_proc_list[i].name, libname))
        {
          if(!dynam_proc_list[i].handle)
          {
            send_to_char("That lib isn't loaded!\n", ch);
          }
          else
          {
            if(unload_proc_lib(libname))
            {
              send_to_char("Library unloaded successfully!\n", ch);
              /*
               * LOG ME!!
               */
            }
            else
            {
              send_to_char("Unable to unload lib!\n", ch);
              /*
               * LOG ME!!
               */
            }
          }
          return;
        }
      send_to_char("No such library exists.\n", ch);
      return;
    }
  send_to_char("Bad command.  Try \"proc\" with no paramters for usage\n",
               ch);

#endif
  /* SHLIB  */

  return;
}

#endif

void do_terminate(P_char ch, char *argument, int /*cmd*/)
{
	P_char victim = NULL;
	char buf[MAX_STRING_LENGTH];

	if (IS_NPC(ch))
		return;

	one_argument(argument, buf);

	if (!*buf)
	{
		send_to_char("But oh Mighty One, whom do you wish to utterly annihilate?\n", ch);
		return;
	}
	if (!(victim = get_char_room_vis(ch, buf)))
	{
		send_to_char("You see no person by that name here.\n", ch);
		return;
	}
	if (IS_NPC(victim))
	{
		send_to_char("Who cares about NPC's...\n", ch);
		return;
	}
	if ((GET_LEVEL(victim) >= GET_LEVEL(ch)) || (GET_LEVEL(victim) >= 62))
	{
		send_to_char("Go away jackass!\n", ch);
		return;
	}
	send_to_char("&+rYou raise your hands and feel the power of life and death\n", ch);
	send_to_char("&+rflowing through your soul!  You unleash the power in a \n", ch);
	act("&+Lblack beam&N &+rof death upon $N!!!", FALSE, ch, 0, victim, TO_CHAR);
	act("&+r$N is completely obliterated into a million bits!", FALSE, ch, 0, victim, TO_CHAR);
	act("&+rYou smile as $N will never again challenge your greatness!", FALSE, ch, 0, victim,
	    TO_CHAR);

	act("&+r$n is surrounded by a red aura, and $s &+Weyes&n &+rglow.", FALSE, ch, 0, victim,
	    TO_NOTVICT);
	act("&+r$n raises $s hands and points them at $N and a&n &+Lblack ", FALSE, ch, 0, victim,
	    TO_NOTVICT);
	act("&+Lbeam&n &+r shoots out to strike at $N!", FALSE, ch, 0, victim, TO_NOTVICT);
	act("&+r$N cries out and $E shatters into a million bits!", FALSE, ch, 0, victim,
	    TO_NOTVICT);

	act("&+r$n is surrounded by a red aura, and $s &+Weyes&n &+rglow.", FALSE, ch, 0, victim,
	    TO_VICT);
	act("&+r$n raises $s hands and points them at YOU! and a&n &+Lblack ", FALSE, ch, 0, victim,
	    TO_VICT);
	act("&+Lbeam&n &+r shoots out to strike at YOU!", FALSE, ch, 0, victim, TO_VICT);
	act("&+rYou feel your soul being invaded by $n as $e begins to", FALSE, ch, 0, victim,
	    TO_VICT);
	act("&+runravel the mysteries of your existance! You feel every part", FALSE, ch, 0, victim,
	    TO_VICT);
	act("&+rof your body being torn to shreds, and you suddenly cry out in", FALSE, ch, 0,
	    victim, TO_VICT);
	act("&+rutter terror as $n stands over the remains of you in righteous", FALSE, ch, 0,
	    victim, TO_VICT);
	act("&+rjustice!  You fall away into complete&n &+Lblackness.......", FALSE, ch, 0, victim,
	    TO_VICT);

	statuslog(ch->player.level, "%s's existence on Duris was just terminated by %s!",
		  GET_NAME(victim), GET_NAME(ch));

	logit(LOG_WIZ, "%s's existence on Duris was just terminated by %s!", GET_NAME(victim),
	      GET_NAME(ch));

	sql_log(ch, WIZLOG, "Terminated %s's existence on Duris.", GET_NAME(victim));

	if (victim->desc)
	{
		close_socket(victim->desc);
	}
	/*
	    victim->desc->connected = CON_DELETE;
	    extract_char(victim);
	    return;
	  } else {
	*/
	// If it's not an immortal.
	if (GET_LEVEL(ch) < MINLVLIMMORTAL)
	{
		update_ingame_racewar(-GET_RACEWAR(ch));
	}
	deleteCharacter(victim);
	extract_char(victim); // extract_char also calls free_char
	victim = NULL;
	return;
	/*  }*/
	logit(LOG_DEBUG, "Somehow reached the end of do_terminate()");
	return;
}

void do_sacrifice(P_char ch, char *argument, int /*cmd*/)
{
	P_char victim = NULL;
	char buf[MAX_STRING_LENGTH];

	if (IS_NPC(ch))
		return;

	one_argument(argument, buf);

	if (!*buf)
	{
		send_to_char("Who do you feel like sacrificing today?\n", ch);
		return;
	}
	if (!(victim = get_char_room_vis(ch, buf)))
	{
		send_to_char("You see no person by that name here.\n", ch);
		return;
	}
	if (IS_NPC(victim))
	{
		send_to_char("Who cares about NPC's...\n", ch);
		return;
	}
	if (victim == ch)
	{
		send_to_char("How do you propose to sacrifice yourself?", ch);
		return;
	}
	if (GET_LEVEL(victim) >= MIN(62, GET_LEVEL(ch)))
	{
		send_to_char("Sorry, you can't sacrifice your peers or superiors!\n", ch);
		return;
	}
	act("$n whips out a huge dagger out of nowhere and stabs you in the heart!", FALSE, ch, 0,
	    victim, TO_VICT);
	act("You conjure a dagger out of nowhere and stab $N in the heart!", FALSE, ch, 0, victim,
	    TO_CHAR);
	act("$n whips out a huge dagger out of nowhere and stabs $N in the heart!", FALSE, ch, 0,
	    victim, TO_NOTVICT);
	statuslog(ch->player.level, "%s was just sacrificed by %s.", GET_NAME(victim),
		  GET_NAME(ch));
	logit(LOG_WIZ, "%s was sacrificed by %s.", GET_NAME(victim), GET_NAME(ch));
	die(victim, ch);
	return;
}

int vnum_mobile(char *searchname, struct char_data *ch)
{
	int i, found = 0, count = 0, length = 0;
	char pattern[MAX_INPUT_LENGTH], mobile_name[MAX_INPUT_LENGTH];
	char buff[MAX_STRING_LENGTH], buf[MAX_STRING_LENGTH];
	P_char t_mob;

	strcpy(pattern, searchname);
	strToLower(pattern);

	strcpy(buff, "&+C__________Mobiles__________&n\n");
	for (i = 0; i <= top_of_mobt; i++)
		if ((t_mob = read_mobile(mob_index[i].virtual_number, VIRTUAL)))
		{
			if (IS_SET(t_mob->specials.act, ACT_SPEC))
				REMOVE_BIT(t_mob->specials.act, ACT_SPEC);
			char_to_room(t_mob, ch->in_room, -2);
			strcpy(mobile_name, /*strip_color */ (t_mob->player.short_descr));
			strToLower(mobile_name);
			snprintf(buf, MAX_STRING_LENGTH, "%6d  %5d  %-s\n",
				 mob_index[i].virtual_number, mob_index[i].number - 1,
				 (t_mob->player.short_descr) ? t_mob->player.short_descr : "None");
			count++;
			if (t_mob)
			{
				extract_char(t_mob);
				t_mob = NULL;
			}
			else
			{
				logit(LOG_EXIT, "GLITCH 1");
				panic_corruption("actwiz", "GLITCH 1 in mobile list rendering");
			}
			if ((strlen(buf) + length + 40) < MAX_STRING_LENGTH)
			{
				strcat(buff, buf);
				length += strlen(buf);
			}
			else
			{
				strcat(buff, "Too many mobiles to list...\n");
				break;
			}
		}
		else
			logit(LOG_DEBUG, "vnum_mobile(): mob %d not loadable",
			      mob_index[i].virtual_number);
	strcat(buff, "\n");
	checked_snprintf(buff + strlen(buff), MAX_STRING_LENGTH - strlen(buff),
			 "&+LTotal mobs of this name in database:&n %d\n", count);
	page_string(ch->desc, buff, 1);

	return (found);
}

#if 0
int vnum_object(char *searchname, struct char_data *ch)
{
  int      nr, found = 0;
  char     pattern[MAX_INPUT_LENGTH], object_name[MAX_INPUT_LENGTH];
  char     buf[MAX_STRING_LENGTH];

  strcpy(pattern, searchname);
  strToLower(pattern);

  strcpy(buf, "&+C__________Objects__________&n\n");
  for (nr = 0; nr <= top_of_objt; nr++)
  {

    strcpy(object_name, strip_color(obj_index[nr].short_description));
    strToLower(object_name);

    if(strstr(object_name, pattern) != NULL)
    {
      if(strlen(buf) > MAX_STRING_LENGTH - 50)
      {
        checked_snprintf(buf, MAX_STRING_LENGTH, "%s\n...and the list goes on...\n", buf);
        break;
      }
      else
      {
        checked_snprintf(buf, MAX_STRING_LENGTH, "%s%3d. &+B[%5d] &n%s\n", buf, ++found,
                obj_index[nr].virtual_number,
                obj_index[nr].short_description);
      }
    }
  }
  page_string(ch->desc, buf, 1);

  return (found);
}
#endif
int vnum_room(char *searchname, struct char_data *ch)
{
	int nr, found = 0;
	char pattern[MAX_INPUT_LENGTH], room_name[MAX_INPUT_LENGTH];
	char buf[MAX_STRING_LENGTH];

	strcpy(pattern, searchname);
	strToLower(pattern);

	strcpy(buf, "&+C__________Rooms__________&n\n");

	for (nr = 0; nr <= top_of_world; nr++)
	{
		strcpy(room_name, strip_color(world[nr].name));
		strToLower(room_name);

		if (strstr(room_name, pattern) != NULL)
		{
			if (strlen(buf) > MAX_STRING_LENGTH - 50)
			{
				checked_snprintf(buf, MAX_STRING_LENGTH,
						 "%s\n...and the list goes on...\n", buf);
				break;
			}
			else
			{
				checked_snprintf(buf, MAX_STRING_LENGTH, "%s%3d. &+B[%5d]&n %s\n",
						 buf, ++found, world[nr].number, world[nr].name);
			}
		}
	}
	page_string(ch->desc, buf, 1);

	return (found);
}

void do_unspec(P_char ch, char *argument, int /*cmd*/)
{
	P_char victim = NULL;
	char buf[MAX_STRING_LENGTH];

	if (IS_NPC(ch))
		return;

	one_argument(argument, buf);

	if (!*buf)
	{
		send_to_char("Who do you want to unspec?\n", ch);
		return;
	}
	if (!(victim = get_char_room_vis(ch, buf)))
	{
		send_to_char("Who do you want to unspec?\n", ch);
		return;
	}
	if (IS_NPC(victim))
		return;

	update_skills(ch);

	if (!IS_SPECIALIZED(victim))
	{
		victim->only.pc->time_unspecced = 0;
		send_to_char("They are not specialized.\n", ch);
		return;
	}
	victim->player.spec = 0;
	send_to_char("You are no longer specialized.\n", victim);
	send_to_char("They are no longer specialized.\n", ch);
}

void do_RemoveSpecTimer(P_char ch, char *argument, int /*cmd*/)
{
	P_char victim = NULL;
	char buf[MAX_STRING_LENGTH];

	if (IS_NPC(ch))
		return;

	one_argument(argument, buf);

	if (!*buf)
	{
		send_to_char("Who's timer do you want to remove?\n", ch);
		return;
	}
	if (!(victim = get_char_room_vis(ch, buf)))
	{
		send_to_char("Who's timer do you want to remove?\n", ch);
		return;
	}
	if (IS_NPC(victim))
		return;
	if (!IS_SET(victim->specials.act2, PLR2_SPEC_TIMER))
	{
		send_to_char("They're timer isn't set.\n", ch);
		return;
	}
	REMOVE_BIT(ch->specials.act2, PLR2_SPEC_TIMER);
	send_to_char("Thier timer has been removed.\n", ch);
}

namespace
{
enum class flat_storage_action : uint8_t
{
	establish = 1,
	destroy,
	remove_child,
	remove_root,
};

struct flat_storage_context
{
	uint64_t storage_uid;
	uint64_t item_uid;
	int32_t room;
	flat_storage_action action;
};

P_obj find_flat_storage_object(uint64_t item_uid)
{
	for (P_obj object = object_list; object; object = object->next)
		if (object->obj_uid == item_uid)
			return object;
	return NULL;
}

bool flat_storage_room_owner(P_obj storage, int32_t room, item_owner_identity *owner = nullptr)
{
	if (!storage || storage->type != ITEM_STORAGE || !OBJ_ROOM(storage) ||
	    storage->loc.room != room || room <= NOWHERE || room > top_of_world)
		return false;
	if (owner)
		*owner = { item_owner_type::room, static_cast<uint64_t>(world[room].number), 0 };
	return true;
}

void log_flat_storage_change(P_char actor, const char *verb, const std::string &description,
			     int32_t room)
{
	if (!actor || !verb)
		return;
	wizlog(GET_LEVEL(actor), "%s %s storage item %s in room %d.", J_NAME(actor), verb,
	       description.c_str(), world[room].number);
	logit(LOG_WIZ, "%s %s storage item %s in room %d.", J_NAME(actor), verb,
	      description.c_str(), world[room].number);
}

bool submit_flat_storage_remove_next(P_char actor, P_obj storage, int32_t room);

void flat_storage_completion(P_char actor, bool committed, const item_transfer_result &,
			     unsigned int, const uint8_t *encoded, size_t encoded_size)
{
	flat_storage_context context = {};
	if (encoded && encoded_size == sizeof(context))
		memcpy(&context, encoded, sizeof(context));
	if (!actor || !encoded || encoded_size != sizeof(context))
		return;
	P_obj storage = find_flat_storage_object(context.storage_uid);
	if (!committed)
	{
		send_to_char("The storage change did not commit; the live object was retained.\r\n",
			     actor);
		if (context.action == flat_storage_action::establish && storage &&
		    OBJ_NOWHERE(storage))
			extract_obj(storage, FALSE);
		return;
	}
	if (context.action == flat_storage_action::establish)
	{
		if (!storage || !OBJ_NOWHERE(storage) || context.room <= NOWHERE ||
		    context.room > top_of_world)
		{
			logit(LOG_FILE,
			      "storage establish committed but live publication was stale (uid=%llu)",
			      (unsigned long long)context.storage_uid);
			send_to_char(
				"The storage authority committed, but live publication failed.\r\n",
				actor);
			return;
		}
		const std::string description =
			storage->short_description ? storage->short_description : "(unnamed)";
		obj_to_room(storage, context.room);
		send_to_char(
			"This object now loads here without being in a .zon file.  Please remove it from the .zon file if necessessary to prevent double loading and confusion.\n",
			actor);
		log_flat_storage_change(actor, "loads", description, context.room);
		return;
	}
	if (!flat_storage_room_owner(storage, context.room))
	{
		logit(LOG_FILE,
		      "storage mutation committed but live publication was stale (uid=%llu)",
		      (unsigned long long)context.storage_uid);
		send_to_char("The storage authority committed, but live publication failed.\r\n",
			     actor);
		return;
	}
	if (context.action == flat_storage_action::remove_child)
	{
		P_obj item = find_flat_storage_object(context.item_uid);
		if (!item || !OBJ_INSIDE(item) || item->loc.inside != storage)
		{
			logit(LOG_FILE,
			      "storage child move committed but live topology was stale (uid=%llu)",
			      (unsigned long long)context.item_uid);
			send_to_char("A storage item committed, but live publication failed.\r\n",
				     actor);
			return;
		}
		obj_from_obj(item);
		obj_to_room(item, context.room);
		if (!submit_flat_storage_remove_next(actor, storage, context.room))
			send_to_char(
				"The moved contents are safe, but the remaining storage change is busy.\r\n",
				actor);
		return;
	}
	if (context.action == flat_storage_action::remove_root && storage->contains)
	{
		logit(LOG_FILE,
		      "empty storage removal committed while live contents remained (uid=%llu)",
		      (unsigned long long)context.storage_uid);
		send_to_char("The storage authority committed, but live contents changed.\r\n",
			     actor);
		return;
	}
	const std::string description = storage->short_description ? storage->short_description :
								     "(unnamed)";
	const char *verb = context.action == flat_storage_action::destroy ? "deletes" : "removes";
	log_flat_storage_change(actor, verb, description, context.room);
	extract_obj(storage, context.action == flat_storage_action::destroy);
}

bool submit_flat_storage_destroy(P_char actor, P_obj storage, int32_t room,
				 flat_storage_action action)
{
	item_owner_identity source = {};
	if (!actor || !flat_storage_room_owner(storage, room, &source) ||
	    (action != flat_storage_action::destroy && action != flat_storage_action::remove_root))
		return false;
	const item_owner_identity destination = { item_owner_type::destruction, 0, 0 };
	const flat_storage_context context = { storage->obj_uid, storage->obj_uid, room, action };
	return item_movement_transaction_submit(actor, storage, NULL, source, destination,
						item_transfer_reason::destruction,
						static_cast<int64_t>(storage->obj_uid),
						flat_storage_completion, &context, sizeof(context),
						NULL, NULL, nullptr,
						economic_source_kind::intentional_destruction);
}

bool submit_flat_storage_remove_next(P_char actor, P_obj storage, int32_t room)
{
	item_owner_identity owner = {};
	if (!actor || !flat_storage_room_owner(storage, room, &owner))
		return false;
	if (!storage->contains)
		return submit_flat_storage_destroy(actor, storage, room,
						   flat_storage_action::remove_root);
	P_obj item = storage->contains;
	const flat_storage_context context = { storage->obj_uid, item->obj_uid, room,
					       flat_storage_action::remove_child };
	return item_movement_transaction_submit(actor, item, NULL, owner, owner,
						item_transfer_reason::operator_repair,
						static_cast<int64_t>(storage->obj_uid),
						flat_storage_completion, &context, sizeof(context));
}

bool submit_flat_storage_establish(P_char actor, P_obj storage, int32_t room)
{
	if (!actor || !storage || storage->type != ITEM_STORAGE || !OBJ_NOWHERE(storage) ||
	    !storage->obj_uid || room <= NOWHERE || room > top_of_world)
		return false;
	const item_owner_identity owner = { item_owner_type::room,
					    static_cast<uint64_t>(world[room].number), 0 };
	const flat_storage_context context = { storage->obj_uid, storage->obj_uid, room,
					       flat_storage_action::establish };
	return item_movement_transaction_submit(actor, storage, NULL, owner, owner,
						item_transfer_reason::operator_repair,
						world[room].number, flat_storage_completion,
						&context, sizeof(context), NULL, NULL, nullptr,
						economic_source_kind::administrator);
}
} // namespace

/* Storage command:
 * Item needs to be a type of ITEM_CONTAINER or ITEM_STORAGE
 * storage new <item vnum> - loads a new object setup to store through boots in room.
 * storage delete <item name> - deletes a storage object and all contents within.
 * storage remove <item name> - deletes a storage object and drops all contents to room.
 */

void do_storage(P_char ch, char *arg, int /*cmd*/)
{
	char subcmd[MAX_STRING_LENGTH], objarg[MAX_STRING_LENGTH];
	P_obj s_obj, tmpobj, next_obj;

	arg = one_argument(arg, subcmd);
	arg = one_argument(arg, objarg);

	if (economic_gameplay_authority::active() &&
	    persistence_mode_get() != PERSISTENCE_MODE_FLATFILE_PRIMARY &&
	    (!strcmp(subcmd, "new") || !strcmp(subcmd, "delete") || !strcmp(subcmd, "remove")))
	{
		send_to_char("Storage changes are unavailable right now.\r\n", ch);
		return;
	}

	if (!strcmp(subcmd, "new"))
	{
		if (!(s_obj = read_object(atoi(objarg), VIRTUAL)))
		{
			send_to_char("&+RError&n: Please choose a valid object vnum.\n", ch);
			return;
		}
		if ((s_obj->type != ITEM_STORAGE) && (s_obj->type != ITEM_CONTAINER))
		{
			send_to_char(
				"&+RError&n: The item on file must be of type STORAGE or CONTAINER\n",
				ch);
			extract_obj(s_obj, FALSE);
			return;
		}
		if (s_obj->type == ITEM_CONTAINER)
		{
			s_obj->type = ITEM_STORAGE;
		}
		if (s_obj->type == ITEM_STORAGE)
		{
			REMOVE_BIT(s_obj->wear_flags, ITEM_TAKE);
			// Just making sure.
			REMOVE_BIT(s_obj->extra_flags, ITEM_ARTIFACT);
			if (persistence_mode_get() == PERSISTENCE_MODE_FLATFILE_PRIMARY)
			{
				if (!submit_flat_storage_establish(ch, s_obj, ch->in_room))
				{
					send_to_char(
						"The storage authority is busy; no object was created.\r\n",
						ch);
					extract_obj(s_obj, FALSE);
				}
				else
					send_to_char("The storage creation is being committed.\r\n",
						     ch);
				return;
			}
			obj_to_room(s_obj, ch->in_room);
			writeSavedItem(s_obj);
			send_to_char(
				"This object now loads here without being in a .zon file.  Please remove it from the .zon file if necessessary to prevent double loading and confusion.\n",
				ch);
			wizlog(GET_LEVEL(ch), "%s loads %s as a storage unit in room %d.",
			       J_NAME(ch), s_obj->short_description, world[ch->in_room].number);
			logit(LOG_WIZ, "%s loads %s as a storage unit in room %d.", J_NAME(ch),
			      s_obj->short_description, world[ch->in_room].number);
			return;
		}
		else
		{
			extract_obj(s_obj);
		}
		send_to_char(
			"Syntax: storage new <item vnum> - load a new object for storage\n"
			"        storage delete <item name> - delete a storage item and all contents within\n"
			"        storage remove <item name> - delete a storage item and drop contents to room\n",
			ch);
		return;
	}
	else if (!strcmp(subcmd, "delete"))
	{
		if ((s_obj = get_obj_in_list(objarg, world[ch->in_room].contents)) &&
		    (s_obj->type == ITEM_STORAGE))
		{
			if (persistence_mode_get() == PERSISTENCE_MODE_FLATFILE_PRIMARY)
			{
				if (!submit_flat_storage_destroy(ch, s_obj, ch->in_room,
								 flat_storage_action::destroy))
					send_to_char(
						"The storage authority is busy; nothing was deleted.\r\n",
						ch);
				else
					send_to_char("The storage deletion is being committed.\r\n",
						     ch);
				return;
			}
			wizlog(GET_LEVEL(ch), "%s deletes storage item %s from room %d.",
			       J_NAME(ch), s_obj->short_description, world[ch->in_room].number);
			logit(LOG_WIZ, "%s deletes storage item %s from room %d.", J_NAME(ch),
			      s_obj->short_description, world[ch->in_room].number);
			extract_obj(s_obj);
			return;
		}
		else
		{
			send_to_char(
				"&+RError&n: You don't see a storage item with that name in this room.\n",
				ch);
			return;
		}
	}
	else if (!strcmp(subcmd, "remove"))
	{
		if ((s_obj = get_obj_in_list(objarg, world[ch->in_room].contents)) &&
		    (s_obj->type == ITEM_STORAGE))
		{
			if (persistence_mode_get() == PERSISTENCE_MODE_FLATFILE_PRIMARY)
			{
				if (!submit_flat_storage_remove_next(ch, s_obj, ch->in_room))
					send_to_char(
						"The storage authority is busy; nothing was removed.\r\n",
						ch);
				else
					send_to_char("The storage removal is being committed.\r\n",
						     ch);
				return;
			}
			for (tmpobj = s_obj->contains; tmpobj; tmpobj = next_obj)
			{
				next_obj = tmpobj->next_content;
				obj_from_obj(tmpobj);
				obj_to_room(tmpobj, ch->in_room);
			}
			wizlog(GET_LEVEL(ch), "%s removes storage item %s from room %d.",
			       J_NAME(ch), s_obj->short_description, world[ch->in_room].number);
			logit(LOG_WIZ, "%s remove storage item %s from room %d.", J_NAME(ch),
			      s_obj->short_description, world[ch->in_room].number);
			extract_obj(s_obj, FALSE);
			return;
		}
		else
		{
			send_to_char(
				"&+RError&n: You don't see a storage item with that name in this room.\n",
				ch);
			return;
		}
	}
	else
	{
		send_to_char(
			"Syntax: storage new <item vnum> - load a new object for storage\n"
			"        storage delete <item name> - delete a storage item and all contents within\n"
			"        storage remove <item name> - delete a storage item and drop contents to room\n",
			ch);
		return;
	}
}

// May be kinda slow since we're walking through the whole mob table and
//   creating/destroying a mob of each type to find its race/zone/etc.

#ifdef USE_ACCOUNT
void show_account_info(P_char ch, P_char target)
{
	if (target->desc)
	{
		send_to_char("\n", ch);
		display_account_information_to_char(ch, target->desc->account);
		display_character_list_to_char(ch, target->desc->account);
		send_to_char("\n", ch);
	}
	else
	{
		send_to_char("\nCharacter has no descriptor attached (link dead).\n", ch);
	}
}

void remove_account_char(P_char ch, P_char /*target*/)
{
	send_to_char("Not supported yet.\n", ch);
}

void add_account_char(P_char ch, P_char /*target*/)
{
	send_to_char("Not supported yet.\n", ch);
}

void change_account_email(P_char ch, P_char /*target*/, const char * /*newEmail*/)
{
	send_to_char("Not supported yet.\n", ch);
}

void lookup_account(P_char ch, const char * /*arguments*/)
{
	send_to_char("Not supported yet.\n", ch);
}

#define ACCOUNT_SYNTAX ""

void do_account(P_char ch, char *arg, int /*cmd*/)
{
	char arg1[MAX_STRING_LENGTH], arg2[MAX_STRING_LENGTH], newEmail[MAX_STRING_LENGTH], *rest;
	P_char target;

	if (IS_NPC(ch) || !IS_TRUSTED(ch))
		return;

	rest = lohrr_chop(arg, arg1);
	rest = lohrr_chop(rest, arg2);

	if (!*arg1)
	{
		send_to_char(ACCOUNT_SYNTAX, ch);
		return;
	}

	if ((*arg1 == 'l') || (*arg1 == 'L'))
	{
		lookup_account(ch, rest);
		return;
	}
	else if (!(target = get_char_vis(ch, arg2)) || IS_NPC(target))
	{
		send_to_char("No such character.\n", ch);
		return;
	}

	if ((*arg1 == 'i') || (*arg1 == 'I'))
	{
		show_account_info(ch, target);
		return;
	}
	else if ((*arg1 == 'r') || (*arg1 == 'R'))
	{
		remove_account_char(ch, target);
		return;
	}
	else if ((*arg1 == 'a') || (*arg1 == 'A'))
	{
		add_account_char(ch, target);
		return;
	}
	else if ((*arg1 == 'c') || (*arg1 == 'C'))
	{
		rest = lohrr_chop(rest, newEmail);
		change_account_email(ch, target, newEmail);
		return;
	}
	else
	{
		send_to_char(ACCOUNT_SYNTAX, ch);
		return;
	}
}
#endif

/* is_desc_valid() moved to comm.c as a global function */

/*
 * Extract ghost characters from the game.
 * Handles both true linkdead (desc=NULL) and dangling pointer ghosts
 * (desc points to freed memory not in descriptor_list).
 *
 * Usage: extractlink <name> - extract specific ghost character
 *        extractlink all    - extract all ghost characters
 */
static bool extractlink_attempt(P_char ch, P_char vict)
{
	char victim_name[MAX_INPUT_LENGTH];
	char buf[MAX_STRING_LENGTH];
	const bool dangling_descriptor = vict->desc && !is_desc_valid(vict->desc);
	const char *name = GET_NAME(vict);

	snprintf(victim_name, sizeof(victim_name), "%s", name ? name : "<unnamed>");
	snprintf(buf, sizeof(buf), "Attempting ghost extraction: %s (%s).\r\n", victim_name,
		 dangling_descriptor ? "invalid descriptor" : "linkdead");
	send_to_char(buf, ch);

	/* A pointer outside descriptor_list is never safe to retain or dereference. */
	if (dangling_descriptor)
		vict->desc = NULL;

	if (!persistence_save_character_terminal(vict, RENT_LINKDEAD))
	{
		snprintf(buf, sizeof(buf),
			 "Retained ghost %s: terminal save was not durable, so no extraction "
			 "was performed. Resolve the persistence failure and retry.\r\n",
			 victim_name);
		send_to_char(buf, ch);
		wizlog(GET_LEVEL(ch),
		       "%s could not extract ghost character %s: terminal save was not "
		       "durable; character retained",
		       GET_NAME(ch), victim_name);
		logit(LOG_WIZ,
		      "%s could not extract ghost character %s: terminal save was not "
		      "durable; character retained",
		      GET_NAME(ch), victim_name);
		return false;
	}

	extract_char_after_terminal_save(vict);
	wizlog(GET_LEVEL(ch), "%s extracted ghost character %s", GET_NAME(ch), victim_name);
	logit(LOG_WIZ, "%s extracted ghost character %s", GET_NAME(ch), victim_name);
	snprintf(buf, sizeof(buf), "Extracted ghost: %s.\r\n", victim_name);
	send_to_char(buf, ch);
	return true;
}

void do_extractlink(P_char ch, char *argument, int /*cmd*/)
{
	P_char vict, next_vict;
	char name[MAX_INPUT_LENGTH];
	char buf[MAX_STRING_LENGTH];
	int matches = 0;
	int ghosts = 0;
	int extracted = 0;
	int retained = 0;
	int connected = 0;
	int excluded = 0;

	if (!IS_TRUSTED(ch))
		return;

	one_argument(argument, name);

	if (!*name)
	{
		send_to_char("Usage:\r\n", ch);
		send_to_char(
			"  extractlink <name>  Save and extract a matching disconnected player\r\n",
			ch);
		send_to_char("  extractlink all     Save and extract every disconnected player\r\n",
			     ch);
		send_to_char("A character is retained when its terminal save is not durable.\r\n",
			     ch);
		send_to_char(
			"Detects both linkdead characters and invalid descriptor pointers.\r\n",
			ch);
		return;
	}

	const bool extract_all = !strcasecmp(name, "all");
	for (vict = character_list; vict; vict = next_vict)
	{
		next_vict = vict->next;

		if (IS_NPC(vict))
			continue;
		if (!extract_all && !isname(name, GET_NAME(vict)))
			continue;
		if (!extract_all)
			matches++;

		if (vict == ch)
		{
			if (!extract_all)
			{
				excluded++;
				send_to_char("Cannot extract yourself with extractlink.\r\n", ch);
			}
			continue;
		}

		const bool is_ghost = !vict->desc || !is_desc_valid(vict->desc);
		if (!is_ghost)
		{
			if (!extract_all)
			{
				connected++;
				snprintf(buf, MAX_STRING_LENGTH,
					 "Cannot extract %s: character has a valid connection.\r\n",
					 GET_NAME(vict));
				send_to_char(buf, ch);
			}
			continue;
		}

		ghosts++;
		if (extractlink_attempt(ch, vict))
			extracted++;
		else
			retained++;
	}

	if (extract_all)
	{
		if (ghosts == 0)
			send_to_char("extractlink all complete: no ghost characters found.\r\n",
				     ch);
		else
		{
			snprintf(buf, sizeof(buf),
				 "extractlink all complete: %d ghost%s found; %d extracted; %d "
				 "retained after save failure.\r\n",
				 ghosts, ghosts == 1 ? "" : "s", extracted, retained);
			send_to_char(buf, ch);
		}
		return;
	}

	if (matches == 0)
	{
		snprintf(buf, sizeof(buf), "No player character matching '%s' was found.\r\n",
			 name);
		send_to_char(buf, ch);
		return;
	}

	snprintf(buf, sizeof(buf),
		 "extractlink result: %d match%s; %d ghost%s found; %d extracted; %d "
		 "retained after save failure; %d connected; %d excluded.\r\n",
		 matches, matches == 1 ? "" : "es", ghosts, ghosts == 1 ? "" : "s", extracted,
		 retained, connected, excluded);
	send_to_char(buf, ch);
}
