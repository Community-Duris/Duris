/*
 ***************************************************************************
 *  File: specs.object.c                                     Part of Duris *
 *  Usage: special procedures for objects                                    *
 *  Copyright  1990, 1991 - see 'license.doc' for complete information.      *
 *  Copyright 1994 - 2008 - Duris Systems Ltd.                             *
 ***************************************************************************
 */

#include "core/prototypes.h"
#include "item/objmisc.h"
#include "core/structs.h"
#include "net/comm.h"
#include "world/db.h"
#include "world/events.h"
#include "cmd/interp.h"
#include "core/utils.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include "guild/assocs.h"
#include "combat/damage.h"
#include "world/graph.h"
#include "combat/justice.h"
#include "world/specs.prototypes.h"
#include "magic/spells.h"
#include "mob/studioproclib.h"
#include "player/player_snapshot.h"
#include "economy/native_mobile_birth_recipe.h"
#include "world/weather.h"
#include "world/object_template.h"
#include <charconv>
#include <string_view>
#include <climits>

/*
   external variables
 */
extern P_char character_list;
extern P_desc descriptor_list;
extern P_index mob_index;
extern P_index obj_index;
extern P_room world;
extern char *coin_names[];
extern char *command[];
extern const char *dirs[];
extern const char *race_types[];
// extern const char rev_dir[];
extern const struct stat_data stat_factor[];
extern int innate_abilities[];
extern int planes_room_num[];
extern int top_of_zone_table;
extern struct command_info cmd_info[MAX_CMD_LIST];
extern struct dex_app_type dex_app[52];
extern struct time_info_data time_info;
extern struct zone_data *zone;
extern struct zone_data *zone_table;

P_nevent get_scheduled(P_obj obj, event_func func);

#define PROCLIB_PARAM_DELIM '&'

// some kind of 'find next token' function
#define ISQUOTE(a) ((((a) == '\'') || ((a) == '"')) ? a : 0)

// util function similar to one_argument, but will parse a string surrounded with single
// or double quotes as a single argument.
char *proclib_getNext_string(char *source, char *nextString)
{
	if (!nextString)
	{
		return source;
	}
	nextString[0] = '\0';
	if (!source)
		return NULL;

	char *p1 = source;
	while (*p1 && isspace(*p1))
		p1++;

	char quote = ISQUOTE(*p1);
	if (!quote)
	{
		// just return the next word
		return one_argument(source, nextString);
	}
	// find the matching quote or EOL
	int nIdx = 0;

	while (*(++p1))
	{
		if (quote == *p1)
		{
			p1++;
			break;
		}
		nextString[nIdx++] = *p1;
	}
	nextString[nIdx++] = '\0';
	return p1;
}

// actual proc for 'hummer'
int proclibobj_hummer(P_obj obj, P_char /*ch*/, int cmd, char * /*argument*/)
{
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;
	if (cmd)
		return FALSE;

	hummer(obj);
	return TRUE;
}

// param parser for actroom proc
char *proclibobj_parse_actroom(char *argument)
{
	char arg[MAX_STRING_LENGTH], params[MAX_STRING_LENGTH];
	int chance = 0;
	char *pRet = NULL;

	argument = proclib_getNext_string(argument, arg);
	if (arg[0])
	{
		chance = atoi(arg);
		if (chance)
		{
			argument = proclib_getNext_string(argument, arg);
			if (arg[0])
			{
				if (!strstr(arg, "%p") && !strstr(arg, "%q"))
					return NULL;

				while (strchr(arg, '%'))
					*(strchr(arg, '%')) = '$';

				checked_snprintf(params, MAX_STRING_LENGTH, "%d\xFF%s", chance,
						 arg);
				CREATE(pRet, char, strlen(params) + 1, MEM_TAG_EXDESCD);
				strcpy(pRet, params);
				return pRet;
			}
		}
	}
	return NULL;
}

// actual proc for actroom
int proclibobj_actroom(P_obj obj, P_char /*ch*/, int cmd, char *params)
{
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (cmd || !OBJ_ROOM(obj))
		return FALSE;

	char *pAct;
	long chance = strtol(params, &pAct, 10);

	if (chance && number(0, chance - 1))
		return FALSE;

	pAct = strchr(pAct, 0xFF);

	if (!chance || !pAct || !(*pAct))
	{
		char buf[500];
		snprintf(buf, 500, "Malformed _proclib_actroom description on object %d",
			 obj_index[obj->R_num].virtual_number);
		debug("%s", buf);
		wizlog(58, "%s", buf);
		return FALSE;
	}
	pAct++;
	// send the 'act' to the room
	act(pAct, TRUE, NULL, obj, 0, TO_ROOM);
	return TRUE;
}

// param parser for actworn
char *proclibobj_parse_actworn(char *argument)
{
	char arg1[MAX_STRING_LENGTH], arg2[MAX_STRING_LENGTH], params[MAX_STRING_LENGTH];
	int chance = 0;
	char *pRet = NULL;

	argument = proclib_getNext_string(argument, arg1);
	if (arg1[0])
	{
		chance = atoi(arg1);
		if (chance)
		{
			argument = proclib_getNext_string(argument, arg1);
			if (arg1[0])
			{
				if (!strstr(arg1, "%p") && !strstr(arg1, "%q"))
					return NULL;
				if (!strstr(arg1, "%n"))
					return NULL;

				while (strchr(arg1, '%'))
					*(strchr(arg1, '%')) = '$';

				argument = proclib_getNext_string(argument, arg2);
				if (arg2[0])
				{
					if (!strstr(arg2, "%p") && !strstr(arg2, "%q"))
						return NULL;

					while (strchr(arg2, '%'))
						*(strchr(arg2, '%')) = '$';

					checked_snprintf(params, MAX_STRING_LENGTH,
							 "%d\xFF%s\xFF%s", chance, arg1, arg2);
					CREATE(pRet, char, strlen(params) + 1, MEM_TAG_EXDESCD);
					strcpy(pRet, params);
					return pRet;
				}
			}
		}
	}
	return NULL;
}

// actual proc for actworn
int proclibobj_actworn(P_obj obj, P_char ch, int cmd, char *params)
{
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;
	if (cmd || !OBJ_WORN(obj))
		return FALSE;
	ch = obj->loc.wearing;

	char *pAct;
	long chance = strtol(params, &pAct, 10);
	char buf[500]; // need a buffer to hold the first (to room) string

	if (chance && number(0, chance - 1))
		return FALSE;

	pAct = strchr(pAct, 0xFF);
	if (pAct)
	{
		pAct++;
		// parse out that first string *somehow* into buf
		char *delim = strchr(pAct, 0xFF);
		if (chance && delim && ((delim - pAct) < 500))
		{
			*delim = '\0'; // temp make the delim be EOS
			strcpy(buf, pAct); // copy it
			*delim = static_cast<char>(0xFF); // put delimiter back
			// note: the above method is faster then using strncpy and then manually adding a null

			pAct = delim + 1;
			while (*pAct && isspace(*pAct))
				pAct++;

			// we are all set!
			if (*pAct)
			{
				act(buf, TRUE, ch, obj, NULL, TO_ROOM);
				act(pAct, TRUE, ch, obj, 0, TO_CHAR);
				return TRUE;
			}
		}
	}
	snprintf(buf, 500, "Malformed _proclib_actworn description on object %d",
		 obj_index[obj->R_num].virtual_number);
	debug("%s", buf);
	wizlog(58, "%s", buf);
	return FALSE;
}

// default parameter parser
char *proclibobj_parse_default(char *)
{
	char *pRet;
	CREATE(pRet, char, 2, MEM_TAG_EXDESCD);
	*pRet = ' ';
	*(pRet + 1) = '\0';
	return pRet;
}

struct ObjProcLib
{
	int (*func)(P_obj, P_char, int, char *);
	char *(*parse_params)(char *arguments);
	const char procName[20];
	const char procDesc[100];
	const char procHelp[300];
} object_proc_libs[] = {
	{ proclibobj_actroom, proclibobj_parse_actroom, "actroom",
	  "Object 'acts' to the room when on the ground.",
	  "        Params: chance act_room\n"
	  "          chance: Act will have 1 in 'chance' odds of occurring apprx every 5 seconds.\n"
	  "          act_room: Actor string sent to room which must contain %p or %q." },
	{ proclibobj_actworn, proclibobj_parse_actworn, "actworn",
	  "Object 'acts' while being worn/equiped by a character.",
	  "        Params: chance act_room act_wearer\n"
	  "          chance: Act will have 1 in 'chance' odds of occurring apprx every 5 seconds.\n"
	  "          act_room: Actor string sent to room which must contain %p or %q, AND %n.\n"
	  "          act_wearer: Actor string sent to room which must contain %p or %q.\n" },
	{ proclibobj_hummer, proclibobj_parse_default, "hummer",
	  "Items 'hums' - similar to some artifact weapons.", "        No parameters." },
	{ proclibobj_sayresponse, proclibobj_parse_sayresponse, "sayresponse",
	  "Object replies when a player says a keyword.", PROCLIB_SAYRESPONSE_HELP },
	{ proclibobj_transporter, proclibobj_parse_transporter, "transporter",
	  "'enter <keyword>' teleports the actor to a room.", PROCLIB_TRANSPORTER_HELP },
};

bool proclib_saved_binding_eligible(P_obj object, bool *eligible) noexcept
{
	if (!object || !eligible)
		return false;
	bool found = false;
	size_t description_count = 0;
	if (IS_SET(object->extra_flags, ITEM_PROCLIB))
		for (const extra_descr_data *description = object->ex_description; description;
		     description = description->next)
		{
			if (++description_count > PLAYER_SNAPSHOT_MAX_ROWS)
				return false;
			if (!description->keyword || strn_cmp(description->keyword, "_proclib_", 9))
				continue;
			if (!description->description)
				return false;
			const size_t length = strnlen(description->description,
						      PLAYER_SNAPSHOT_MAX_STRING_BYTES + 1);
			if (!length || length > PLAYER_SNAPSHOT_MAX_STRING_BYTES)
				return false;
			const ObjProcLib *library = nullptr;
			for (size_t i = ARRAY_SIZE(object_proc_libs); i-- > 0;)
				if (object_proc_libs[i].func &&
				    !strn_cmp(description->keyword + 9,
					      object_proc_libs[i].procName,
					      strlen(object_proc_libs[i].procName)))
				{
					library = &object_proc_libs[i];
					break;
				}
			if (!library || !library->parse_params)
				return false;
			// A saved flag is constructor-success evidence only under the
			// owner's original locked literal/UID proof. Validate its existing
			// post-parse storage shape; never parse user arguments again.
			const std::string_view value(description->description, length);
			const size_t split = value.find(static_cast<char>(0xff));
			const auto integer = [](std::string_view text, int &result)
			{
				const auto parsed = std::from_chars(
					text.data(), text.data() + text.size(), result);
				return !text.empty() && parsed.ec == std::errc{} &&
				       parsed.ptr == text.data() + text.size();
			};
			if (library->func == proclibobj_hummer)
			{
				if (value != " ")
					return false;
			}
			else if (library->func == proclibobj_sayresponse)
			{
				if (split == std::string_view::npos || !split ||
				    split + 1 >= value.size())
					return false;
			}
			else if (library->func == proclibobj_transporter)
			{
				int destination = 0;
				if (split == std::string_view::npos || !split ||
				    !integer(value.substr(split + 1), destination) ||
				    destination <= 0)
					return false;
			}
			else if (library->func == proclibobj_actroom ||
				 library->func == proclibobj_actworn)
			{
				int chance = 0;
				if (split == std::string_view::npos ||
				    !integer(value.substr(0, split), chance) || !chance ||
				    split + 1 >= value.size())
					return false;
				if (library->func == proclibobj_actworn)
				{
					const size_t second =
						value.find(static_cast<char>(0xff), split + 1);
					if (second == std::string_view::npos ||
					    second == split + 1 || second + 1 >= value.size())
						return false;
				}
			}
			else
				return false;
			found = true;
		}
	if (IS_SET(object->extra_flags, ITEM_PROCLIB) && !found)
		return false;
	*eligible = found;
	return true;
}

bool proclib_saved_periodic_probe(P_obj object, size_t description_index, bool *periodic) noexcept
{
	if (!object || !periodic || description_index >= PLAYER_SNAPSHOT_MAX_ROWS)
		return false;
	try
	{
		// The native owner has just proved the exact ordered literal descriptions
		// in its complete fresh world/SQL cut. This helper supplies no authority.
		const extra_descr_data *description = object->ex_description;
		for (size_t index = 0; index < description_index && description; ++index)
			description = description->next;
		if (!description)
			return false;
		bool requested = false;
		if (description->keyword && description->description &&
		    !strn_cmp(description->keyword, "_proclib_", 9))
			for (size_t index = 0; index < ARRAY_SIZE(object_proc_libs); ++index)
				if (object_proc_libs[index].func &&
				    !strn_cmp(description->keyword + 9,
					      object_proc_libs[index].procName,
					      strlen(object_proc_libs[index].procName)))
				{
					// Exactly the existing constructor eligibility probe. In particular
					// command-only sayresponse/transporter return FALSE. Already saved
					// parsed parameters are never parsed again or added to the object.
					requested = object_proc_libs[index].func(
						object, nullptr, CMD_SET_PERIODIC, nullptr);
					break;
				}
		*periodic = requested;
		return true;
	}
	catch (...)
	{
		return false;
	}
}

int proclib_obj_proc(P_obj obj, P_char ch, int cmd, char *argument);

// event func - just redirects to generic proclib_obj_proc (which is in obj proc format)
void proclib_obj_event(P_char, P_char, P_obj obj, void *)
{
	proclib_obj_proc(obj, NULL, 0, NULL);
}

// the "hub" for all proclib object procs.  This function dispatches proclibs
// for all objects
int proclib_obj_proc(P_obj obj, P_char ch, int cmd, char *argument)
{
	if (item_restricted_for_player_pet(ch, obj))
		return FALSE;
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!obj)
		return FALSE;

	int bRet = FALSE, bResetPeriodic = FALSE;

	struct extra_descr_data *ed = obj->ex_description;
	while (ed)
	{
		if (ed->keyword && ed->description)
		{
			if (!strn_cmp(ed->keyword, "_proclib_", 9))
			{
				for (size_t i = 0; i < ARRAY_SIZE(object_proc_libs); i++)
				{
					if (!strn_cmp(ed->keyword + 9, object_proc_libs[i].procName,
						      strlen(object_proc_libs[i].procName)) &&
					    object_proc_libs[i].func)
					{
						if (!cmd)
						{
							if (object_proc_libs[i].func(
								    obj, ch, cmd, ed->description))
							{
								bRet = bResetPeriodic = TRUE;
								break;
							}
							else if (!bResetPeriodic &&
								 object_proc_libs[i].func(
									 obj, NULL, -10, NULL))
								bResetPeriodic = TRUE;
						}
						else if (object_proc_libs[i].func(obj, ch, cmd,
										  argument))
							return TRUE; // no need to break the for loop - this wasn't a periodic
								// call, so don't need to reset it.
					}
				}
			}
		}
		ed = ed->next;
	}
	if (bResetPeriodic)
		add_event(proclib_obj_event, PULSE_MOBILE + number(-4, 4), NULL, NULL, obj, 0, NULL,
			  0);
	return bRet;
}

// sends simple 'usage' info for object procs
void proclibUsage(P_char ch)
{
	if (ch)
		send_to_char(
			"Usage: proclib <mob|obj|room> <target> <add|del> <procname> [proc params]\n",
			ch);
}

// generic function for adding a proclib to an object.  This is NOT an interactive
// function, but is designed to be called from both the read_object() interface, as well
// as the in-mud proclib command.
//
// 0 - success.  <0 is a procname issue,  >0 is a params issue (and the retVal is the idx+1
int quest_mobile_native_original_proclib::prepare(P_obj obj, char *procName, char *args,
						  size_t *library_index)
{
	int libIdx = -1;
	for (libIdx = (sizeof(object_proc_libs) / sizeof(ObjProcLib)) - 1; libIdx >= 0; libIdx--)
	{
		if (!strn_cmp(procName, object_proc_libs[libIdx].procName,
			      strlen(object_proc_libs[libIdx].procName)) &&
		    object_proc_libs[libIdx].func)
			break;
	}
	if (-1 == libIdx)
		return -1;

	char *params = object_proc_libs[libIdx].parse_params(args);
	if (!params)
		return (libIdx + 1);

	// find a suffix to use...
	int suffix = 0;
	struct extra_descr_data *ed = obj->ex_description;
	while (ed)
	{
		if (ed->keyword)
		{
			if (!strn_cmp(ed->keyword, "_proclib_", 9) &&
			    !strn_cmp(ed->keyword + 9, object_proc_libs[libIdx].procName,
				      strlen(object_proc_libs[libIdx].procName)))
			{
				int tempSuff =
					atoi(ed->keyword +
					     (9 + strlen(object_proc_libs[libIdx].procName)));
				if (tempSuff > suffix)
					suffix = tempSuff;
			}
		}
		ed = ed->next;
	}
	char keyword[50];
	snprintf(keyword, 50, "_proclib_%s%d", object_proc_libs[libIdx].procName, suffix + 1);

	CREATE(ed, struct extra_descr_data, 1, MEM_TAG_EXDESCD);
	ed->next = obj->ex_description;
	obj->ex_description = ed;
	CREATE(ed->keyword, char, strlen(keyword) + 1, MEM_TAG_EXDESCD);
	strcpy(ed->keyword, keyword);
	ed->description = params;
	obj->str_mask |= STRUNG_EDESC;
	SET_BIT(obj->extra_flags, ITEM_PROCLIB);

	*library_index = static_cast<size_t>(libIdx);
	return 0;
}
bool quest_mobile_native_original_proclib::probe(P_obj object, size_t index,
						 bool *periodic) noexcept
{
	if (!object || !periodic || index >= ARRAY_SIZE(object_proc_libs) ||
	    !object_proc_libs[index].func)
		return false;
	const auto function = object_proc_libs[index].func;
	// These existing probes return before all other callback work. An unknown
	// future library needs its original constructor participant, not prediction.
	if (function != proclibobj_hummer && function != proclibobj_actroom &&
	    function != proclibobj_actworn && function != proclibobj_sayresponse &&
	    function != proclibobj_transporter)
		return false;
	try
	{
		const bool requested =
			object_proc_libs[index].func(object, nullptr, CMD_SET_PERIODIC, nullptr);
		*periodic = requested;
		return true;
	}
	catch (...)
	{
		return false;
	}
}

int proclibObj_add(P_obj obj, char *procName, char *args)
{
	int libIdx = -1;
	for (libIdx = (sizeof(object_proc_libs) / sizeof(ObjProcLib)) - 1; libIdx >= 0; libIdx--)
	{
		if (!strn_cmp(procName, object_proc_libs[libIdx].procName,
			      strlen(object_proc_libs[libIdx].procName)) &&
		    object_proc_libs[libIdx].func)
			break;
	}
	if (-1 == libIdx)
		return -1;

	char *params = object_proc_libs[libIdx].parse_params(args);
	if (!params)
		return (libIdx + 1);

	// find a suffix to use...
	int suffix = 0;
	struct extra_descr_data *ed = obj->ex_description;
	while (ed)
	{
		if (ed->keyword)
		{
			if (!strn_cmp(ed->keyword, "_proclib_", 9) &&
			    !strn_cmp(ed->keyword + 9, object_proc_libs[libIdx].procName,
				      strlen(object_proc_libs[libIdx].procName)))
			{
				int tempSuff =
					atoi(ed->keyword +
					     (9 + strlen(object_proc_libs[libIdx].procName)));
				if (tempSuff > suffix)
					suffix = tempSuff;
			}
		}
		ed = ed->next;
	}
	char keyword[50];
	snprintf(keyword, 50, "_proclib_%s%d", object_proc_libs[libIdx].procName, suffix + 1);

	CREATE(ed, struct extra_descr_data, 1, MEM_TAG_EXDESCD);
	ed->next = obj->ex_description;
	obj->ex_description = ed;
	CREATE(ed->keyword, char, strlen(keyword) + 1, MEM_TAG_EXDESCD);
	strcpy(ed->keyword, keyword);
	ed->description = params;
	obj->str_mask |= STRUNG_EDESC;
	SET_BIT(obj->extra_flags, ITEM_PROCLIB);

	if ((NULL == get_scheduled(obj, proclib_obj_event)) &&
	    object_proc_libs[libIdx].func(obj, NULL, CMD_SET_PERIODIC, NULL))
	{
		add_event(proclib_obj_event, PULSE_MOBILE + number(-4, 4), NULL, NULL, obj, 0, NULL,
			  0);
	}

	/* Forward real commands to this vnum's instance proclibs.  A vnum that
	   already owns a proc keeps it: proclib_chain_install() remembers it and
	   the bridge calls it FIRST, so the incumbent still wins and a proclib
	   added at runtime is still reachable. */
	if ((obj->R_num >= 0) && obj_index[obj->R_num].func.obj != proclib_obj_cmd_bridge)
	{
		const auto recovery_before = obj_index[obj->R_num].func.obj;
		if (obj_index[obj->R_num].func.obj)
			proclib_chain_install(obj->R_num, obj_index[obj->R_num].func.obj);
		obj_index[obj->R_num].func.obj = proclib_obj_cmd_bridge;
		shop_trade_original_procedure_binding_stage::observe_normal_binding(
			obj->R_num, recovery_before, proclib_obj_cmd_bridge);
	}

	return 0;
}

// object specific version of proclib cmd processing
void do_proclibObj(P_char ch, char *argument)
{
	// parameters should be: target_obj add|del procname [proc_params]

	bool bAdd = false; // false means that its a delete

	char argBuf[MAX_STRING_LENGTH];

	wizlog(GET_LEVEL(ch), "%s: proclibObj %s", GET_NAME(ch), argument);
	logit(LOG_WIZ, "%s: proclibObj %s", GET_NAME(ch), argument);

	// parse object
	argument = one_argument(argument, argBuf);
	// find the object referenced by argBuf
	P_obj obj;

	if (!(obj = get_obj_in_list_vis(ch, argBuf, ch->carrying)))
	{
		if (!(obj = get_obj_in_list_vis(ch, argBuf, world[ch->in_room].contents)))
		{
			if (ch)
				send_to_char("Unable to find the specified object\n", ch);
			return;
		}
	}
	// parse cmd: add|delete
	argument = one_argument(argument, argBuf);
	if (is_abbrev(argBuf, "add"))
		bAdd = true;
	else if (!is_abbrev(argBuf, "delete"))
	{
		proclibUsage(ch);
		return;
	}

	if (!bAdd)
	{
		send_to_char("deleting of proclibs isn't *yet* supported\n", ch);
		return;
	}
	// parse out the proc name
	argument = one_argument(argument, argBuf);

	int addRet = proclibObj_add(obj, argBuf, argument);
	if (addRet < 0)
	{ // error with proc name
		proclibUsage(ch);
		send_to_char("Available proclibs for objects: \n", ch);
		for (int libIdx = (sizeof(object_proc_libs) / sizeof(ObjProcLib)) - 1; libIdx >= 0;
		     libIdx--)
		{
			send_to_char(object_proc_libs[libIdx].procName, ch);
			send_to_char(" - ", ch);
			send_to_char(object_proc_libs[libIdx].procDesc, ch);
			send_to_char("\n", ch);
		}
		send_to_char("\n", ch);
		return;
	}
	if (addRet)
	{
		send_to_char("Error in proc parameters.\n", ch);
		send_to_char(object_proc_libs[addRet - 1].procHelp, ch);
		send_to_char("\n", ch);
		return;
	}
	send_to_char("Proc successfully added.\n", ch);
}

// mobile specific version of proclib cmd processing
void do_proclibMob(P_char ch, char * /*argument*/)
{
	send_to_char("Mob proclibs not yet supported\n", ch);
}

// room specific version of proclib cmd processing
void do_proclibRoom(P_char ch, char * /*argument*/)
{
	send_to_char("Room proclibs not yet supported\n", ch);
}

// main proclib cmd (in mud) processer
void do_proclib(P_char ch, char *argument, int /*cmd*/)
{
	char type[MAX_STRING_LENGTH];

	argument = one_argument(argument, type);
	if (!*type)
		proclibUsage(ch);
	else if (is_abbrev(type, "object"))
	{
		do_proclibObj(ch, argument);
	}
	else if (is_abbrev(type, "character") || is_abbrev(type, "mobile"))
	{
		do_proclibMob(ch, argument);
	}
	else if (is_abbrev(type, "room"))
	{
		do_proclibRoom(ch, argument);
	}
	else
	{
		proclibUsage(ch);
	}
}

bool quest_mobile_native_original_proclib::retained_library(
	size_t index, native_mobile_birth_library *output) noexcept
{
	if (!output || index >= ARRAY_SIZE(object_proc_libs))
		return false;
	const auto function = object_proc_libs[index].func;
	native_mobile_birth_library tag;
	if (function == proclibobj_actroom)
		tag = native_mobile_birth_library::actroom;
	else if (function == proclibobj_actworn)
		tag = native_mobile_birth_library::actworn;
	else if (function == proclibobj_hummer)
		tag = native_mobile_birth_library::hummer;
	else if (function == proclibobj_sayresponse)
		tag = native_mobile_birth_library::sayresponse;
	else if (function == proclibobj_transporter)
		tag = native_mobile_birth_library::transporter;
	else
		return false;
	*output = tag;
	return true;
}
bool quest_mobile_native_original_proclib::retained_index(native_mobile_birth_library tag,
							  size_t *output) noexcept
{
	if (!output)
		return false;
	for (size_t index = 0; index < ARRAY_SIZE(object_proc_libs); ++index)
	{
		native_mobile_birth_library actual;
		if (retained_library(index, &actual) && actual == tag)
		{
			*output = index;
			return true;
		}
	}
	return false;
}

#include "core/utility.h"
#include <algorithm>
#include <cerrno>
#include <initializer_list>

namespace
{
using original_proclib_current = bool (*)(size_t *, void *) noexcept;
using original_proclib_reserve = bool (*)(size_t, void *) noexcept;

struct original_proclib_budget
{
	original_proclib_current current;
	original_proclib_reserve reserve;
	void *context;
	size_t outer, fixed, retained = 0;

	bool charge(size_t prospective = 0) const noexcept
	{
		size_t global = 0, total = outer;
		if (!current || !reserve || !current(&global, context))
		{
			errno = ENOBUFS;
			return false;
		}
		for (const size_t bytes : { fixed, retained, prospective, global })
		{
			if (bytes > SIZE_MAX - total)
			{
				errno = ENOBUFS;
				return false;
			}
			total += bytes;
		}
		if (!reserve(total, context))
		{
			errno = ENOBUFS;
			return false;
		}
		return true;
	}
	bool finish(int error) const noexcept
	{
		if (!charge())
			return false;
		errno = error;
		return !error;
	}
	void *allocate(size_t bytes) noexcept
	{
		if (!bytes || bytes > SIZE_MAX - malloc_header())
		{
			errno = ENOBUFS;
			return nullptr;
		}
		const size_t storage = bytes + malloc_header();
		if (!charge(storage))
			return nullptr;
		void *result = __try_malloc(bytes, MEM_TAG_EXDESCD, __FILE__, __LINE__);
		if (!result)
		{
			errno = ENOMEM;
			return nullptr;
		}
		retained += storage; // charge proved this addition cannot overflow.
		if (!charge())
		{
			__free(result, __FILE__, __LINE__);
			retained -= storage;
			return nullptr;
		}
		return result;
	}
	void release(void *allocation, size_t bytes) noexcept
	{
		if (!allocation)
			return;
		__free(allocation, __FILE__, __LINE__);
		retained -= bytes + malloc_header();
	}
	static constexpr size_t malloc_header() noexcept
	{
#ifdef MEMCHK
		return sizeof(ALLOCATION_HEADER);
#else
		return 0;
#endif
	}
};

struct original_proclib_log_relay
{
	original_proclib_budget *budget;
	size_t initial_global;
	static bool reserve_current(size_t request, void *opaque) noexcept
	{
		auto &relay = *static_cast<original_proclib_log_relay *>(opaque);
		size_t global = 0;
		if (request < relay.initial_global ||
		    !relay.budget->current(&global, relay.budget->context))
		{
			errno = ENOBUFS;
			return false;
		}
		request -= relay.initial_global;
		if (global > SIZE_MAX - request ||
		    !relay.budget->reserve(request + global, relay.budget->context))
		{
			errno = ENOBUFS;
			return false;
		}
		return true;
	}
};

// These are actual companion buffers, not a prediction of a compiler frame.
// They accommodate each of the five original parser layouts, including the
// sayresponse parser's MAX_STRING_LENGTH * 2 + 2 parameter destination.
struct original_proclib_parse_buffers
{
	char first[MAX_STRING_LENGTH], second[MAX_STRING_LENGTH];
	char params[MAX_STRING_LENGTH * 2 + 2];
};

// Declaration-level inventories of the original allocation-free token helpers:
// getNext(source,nextString,p1,quote,nIdx,result), one_argument(argument,first,
// begin,look_at,result), fill_word(argument,result), search_block(arg,list,exact,
// i,l,result), strn_cmp(arg1,arg2,n,chk,i,result), and is_number(str,result).
constexpr size_t original_proclib_token_declarations =
	4 * sizeof(char *) + sizeof(char) + sizeof(int) + 3 * sizeof(char *) + 2 * sizeof(int) +
	sizeof(char *) + sizeof(int) + sizeof(char *) + sizeof(const char **) + 4 * sizeof(int) +
	2 * sizeof(char *) + sizeof(unsigned int) + 3 * sizeof(int) + sizeof(char *) + sizeof(bool);
// Actual literal format-companion declarations: budget/destination/format,
// longest pack(int,char*,char*), capacity/required/request/rendered/available/
// copy_size/observed/error; malloc/free/snprintf/memcpy/fprintf call carriers.
// The original checked_vsnprintf va_list objects are NOT called or counted.
// Native libc implementation qualification remains the existing libc policy.
constexpr size_t original_proclib_format_declarations =
	6 * sizeof(void *) + 4 * sizeof(size_t) + 3 * sizeof(int) + sizeof(bool) +
	10 * sizeof(void *) + 4 * sizeof(size_t) + 4 * sizeof(int);
// __try_malloc/__free and the real MEMCHK init/increment/decrement paths are
// allocation-free for MEMCHK <= 1. Their literal header is charged separately.
constexpr size_t original_proclib_malloc_declarations =
	3 * sizeof(void *) + sizeof(size_t) + sizeof(int) + 2 * sizeof(void *) +
	2 * sizeof(void *) + sizeof(int) + sizeof(void *) + 2 * sizeof(const char *) +
	2 * sizeof(size_t) + 4 * sizeof(int);
constexpr size_t original_proclib_control_declarations =
	// charge(this,prospective,global,total,range array/begin/end/bytes,result),
	// allocate(this,bytes,storage,result) and finish(this,error,result).
	3 * sizeof(void *) + 9 * sizeof(size_t) + sizeof(bool) +
	sizeof(std::initializer_list<size_t>) + 2 * sizeof(void *) + 2 * sizeof(size_t) +
	sizeof(void *) + sizeof(int) + sizeof(bool) +
	// Real logging relay object and reserve_current(request,opaque,relay,
	// global,result), plus next_bounded(source/destination/budget/first,
	// quote/length/cursor/global/live/range array/begin/end/bytes/result).
	sizeof(original_proclib_log_relay) + 2 * sizeof(void *) + 2 * sizeof(size_t) +
	sizeof(bool) + 5 * sizeof(void *) + sizeof(char) + 8 * sizeof(size_t) + 2 * sizeof(void *) +
	sizeof(bool) + sizeof(std::initializer_list<size_t>) +
	// repeated strcpy/strlen/strchr/strstr/atoi leaf call declarations.
	12 * sizeof(void *) + 2 * sizeof(size_t) + 3 * sizeof(int);

// Use the ORIGINAL token routine whenever it is safe and allocation-free.
// Its unquoted oversized-input branch originally logs and returns no token;
// reproduce that exact diagnostic through the existing bounded log owner.
bool original_proclib_next_bounded(char *&source, char *destination,
				   original_proclib_budget &budget) noexcept
{
	if (source)
	{
		char *first = source;
		while (*first && isspace(*first))
			++first;
		const char quote = ISQUOTE(*first);
		if (quote)
		{
			size_t length = 0;
			for (const char *cursor = first + 1; *cursor && *cursor != quote; ++cursor)
				if (++length >= MAX_STRING_LENGTH)
				{
					// The original fixed array would overflow; no defined parser
					// result is replaced by accepting a truncated token.
					errno = EOVERFLOW;
					return false;
				}
		}
		else if (strlen(source) >= MAX_INPUT_LENGTH)
		{
			// diagnostic_logit_bounded reserves through the authentic caller.
			// Its callback receives a total including the fresh global bytes.
			size_t global = 0;
			if (!budget.charge() || !budget.current(&global, budget.context))
			{
				errno = ENOBUFS;
				return false;
			}
			size_t live = budget.outer;
			for (const size_t bytes : { budget.fixed, budget.retained, global })
			{
				if (bytes > SIZE_MAX - live)
				{
					errno = ENOBUFS;
					return false;
				}
				live += bytes;
			}
			original_proclib_log_relay relay{ &budget, global };
			if (!diagnostic_logit_bounded(original_proclib_log_relay::reserve_current,
						      &relay, live, LOG_SYS,
						      "one_argument: argument too long."))
				return false;
			destination[0] = '\0';
			source = nullptr;
			return budget.charge();
		}
	}
	source = proclib_getNext_string(source, destination);
	return budget.charge();
}

// Literal owning companion of checked_vsnprintf's measure/malloc/render/copy/
// warning/free algorithm. The actual separate malloc is admitted before it is
// made. Preserve the exact original destination truncation and stderr warning.
// These parser destinations never alias their arguments. Native libc/stdio
// qualification is still part of the existing combined major-plan gate.
template <typename... Arguments>
bool original_proclib_format_bounded(original_proclib_budget &budget, char *destination,
				     size_t capacity, const char *format,
				     Arguments... arguments) noexcept
{
	const int required = snprintf(nullptr, 0, format, arguments...);
	if (required < 0)
	{
		errno = EINVAL;
		return false;
	}
	if (!budget.charge(static_cast<size_t>(required) + 1))
		return false;
	const size_t request = static_cast<size_t>(required) + 1;
	char *rendered = static_cast<char *>(malloc(request));
	if (!rendered)
		return budget.finish(ENOMEM);
	budget.retained += request;
	if (!budget.charge())
	{
		free(rendered);
		budget.retained -= request;
		budget.finish(ENOBUFS);
		return false;
	}
	snprintf(rendered, request, format, arguments...);
	if (capacity)
	{
		const size_t available = capacity - 1;
		const size_t copy_size = static_cast<size_t>(required) < available ?
						 static_cast<size_t>(required) :
						 available;
		memcpy(destination, rendered, copy_size);
		destination[copy_size] = '\0';
	}
	if (static_cast<size_t>(required) >= capacity)
		fprintf(stderr,
			"checked_snprintf: output requires %d bytes but destination holds %zu; truncated.\n",
			required, capacity ? capacity - 1 : 0);
	const bool observed = budget.charge();
	const int error = observed ? 0 : ENOBUFS;
	free(rendered);
	budget.retained -= request;
	if (!budget.finish(error))
		return false;
	return true;
}

char *original_proclib_parse_bounded(const ObjProcLib &library, char *argument,
				     original_proclib_budget &budget,
				     size_t &allocation_bytes) noexcept
{
	original_proclib_parse_buffers buffers;
	char *arg1 = buffers.first, *arg2 = buffers.second, *params = buffers.params;
	int chance = 0;
	size_t capacity = 0;
	if (library.parse_params == proclibobj_parse_default)
	{
		char *result = static_cast<char *>(budget.allocate(2));
		if (!result)
			return nullptr;
		result[0] = ' ';
		result[1] = '\0';
		allocation_bytes = 2;
		return result;
	}
	if (!original_proclib_next_bounded(argument, arg1, budget))
		return nullptr;
	if (!arg1[0])
	{
		errno = EINVAL;
		return nullptr;
	}
	if (library.parse_params == proclibobj_parse_actroom ||
	    library.parse_params == proclibobj_parse_actworn)
	{
		chance = atoi(arg1);
		if (!chance || !original_proclib_next_bounded(argument, arg1, budget))
		{
			if (!chance)
				errno = EINVAL;
			return nullptr;
		}
		if (!arg1[0] || (!strstr(arg1, "%p") && !strstr(arg1, "%q")))
		{
			errno = EINVAL;
			return nullptr;
		}
		if (library.parse_params == proclibobj_parse_actworn && !strstr(arg1, "%n"))
		{
			errno = EINVAL;
			return nullptr;
		}
		while (strchr(arg1, '%'))
			*strchr(arg1, '%') = '$';
		capacity = MAX_STRING_LENGTH;
		if (library.parse_params == proclibobj_parse_actworn)
		{
			if (!original_proclib_next_bounded(argument, arg2, budget))
				return nullptr;
			if (!arg2[0] || (!strstr(arg2, "%p") && !strstr(arg2, "%q")))
			{
				errno = EINVAL;
				return nullptr;
			}
			while (strchr(arg2, '%'))
				*strchr(arg2, '%') = '$';
			if (!original_proclib_format_bounded(budget, params, capacity,
							     "%d\xFF%s\xFF%s", chance, arg1, arg2))
				return nullptr;
		}
		else if (!original_proclib_format_bounded(budget, params, capacity, "%d\xFF%s",
							  chance, arg1))
			return nullptr;
	}
	else if (library.parse_params == proclibobj_parse_sayresponse)
	{
		if (!original_proclib_next_bounded(argument, arg2, budget))
			return nullptr;
		if (!arg2[0])
		{
			errno = EINVAL;
			return nullptr;
		}
		capacity = MAX_STRING_LENGTH * 2 + 2;
		if (!original_proclib_format_bounded(budget, params, capacity, "%s\xFF%s", arg1,
						     arg2))
			return nullptr;
	}
	else if (library.parse_params == proclibobj_parse_transporter)
	{
		if (is_number(arg1))
		{
			errno = EINVAL;
			return nullptr;
		}
		if (!original_proclib_next_bounded(argument, arg2, budget))
			return nullptr;
		if (!arg2[0] || !is_number(arg2) || atoi(arg2) <= 0)
		{
			errno = EINVAL;
			return nullptr;
		}
		capacity = MAX_STRING_LENGTH + 16;
		if (!original_proclib_format_bounded(budget, params, capacity, "%s\xFF%d", arg1,
						     atoi(arg2)))
			return nullptr;
	}
	else
	{
		// A future parser needs its actual owning companion, never prediction.
		errno = ENOTSUP;
		return nullptr;
	}
	allocation_bytes = strlen(params) + 1;
	char *result = static_cast<char *>(budget.allocate(allocation_bytes));
	if (!result)
		return nullptr;
	strcpy(result, params);
	return result;
}
}

int quest_mobile_native_original_proclib::prepare_bounded(
	P_obj obj, char *procName, char *args, size_t *library_index,
	bool (*current_global)(size_t *, void *) noexcept,
	bool (*reserve_scratch_peak)(size_t, void *) noexcept, void *context,
	size_t outer_live_scratch) noexcept
{
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	_GLIBCXX_USE_CXX11_ABI != 1 || !defined(__linux__) || !defined(__GLIBC__) ||            \
	!defined(__x86_64__) || (defined(MEMCHK) && MEMCHK > 1)
	(void)obj;
	(void)procName;
	(void)args;
	(void)library_index;
	(void)current_global;
	(void)reserve_scratch_peak;
	(void)context;
	(void)outer_live_scratch;
	errno = ENOTSUP;
	return -1;
#else
	if (sizeof(void *) != 8 || sizeof(size_t) != 8 || sizeof(int) != 4)
	{
		errno = ENOTSUP;
		return -1;
	}
	// Actual entry parameters/locals, nested parser's real buffers and typed
	// helper inventories. Input/old object/old description storage is caller-owned.
	constexpr size_t fixed =
		sizeof(original_proclib_budget) + sizeof(original_proclib_parse_buffers) +
		// obj/name/args/index/current/reserve/context, outer, libIdx,
		// params/node/keyword, their allocation lengths, suffix/tempSuff,
		// traversal cursor, keyword[50], status/error and parser locals.
		11 * sizeof(void *) + 5 * sizeof(size_t) + 5 * sizeof(int) + 50 +
		6 * sizeof(void *) + 3 * sizeof(size_t) + sizeof(int) +
		original_proclib_token_declarations + original_proclib_format_declarations +
		original_proclib_malloc_declarations + original_proclib_control_declarations;
	original_proclib_budget budget{ current_global, reserve_scratch_peak, context,
					outer_live_scratch, fixed };
	if (!budget.charge())
		return -1;
	if (!obj || !procName || !library_index)
	{
		budget.finish(EINVAL);
		return -1;
	}
	int libIdx = -1;
	for (libIdx = (sizeof(object_proc_libs) / sizeof(ObjProcLib)) - 1; libIdx >= 0; --libIdx)
		if (!strn_cmp(procName, object_proc_libs[libIdx].procName,
			      strlen(object_proc_libs[libIdx].procName)) &&
		    object_proc_libs[libIdx].func)
			break;
	if (libIdx == -1)
	{
		budget.finish(EINVAL);
		return -1;
	}
	size_t params_bytes = 0, keyword_bytes = 0;
	char *params = original_proclib_parse_bounded(object_proc_libs[libIdx], args, budget,
						      params_bytes);
	if (!params)
	{
		budget.finish(errno ? errno : EINVAL);
		return libIdx + 1;
	}
	int suffix = 0;
	for (extra_descr_data *description = obj->ex_description; description;
	     description = description->next)
		if (description->keyword && !strn_cmp(description->keyword, "_proclib_", 9) &&
		    !strn_cmp(description->keyword + 9, object_proc_libs[libIdx].procName,
			      strlen(object_proc_libs[libIdx].procName)))
		{
			const int tempSuff = atoi(description->keyword +
						  (9 + strlen(object_proc_libs[libIdx].procName)));
			if (tempSuff > suffix)
				suffix = tempSuff;
		}
	if (suffix == INT_MAX)
	{
		budget.release(params, params_bytes);
		budget.finish(EOVERFLOW);
		return libIdx + 1;
	}
	char keyword[50];
	snprintf(keyword, 50, "_proclib_%s%d", object_proc_libs[libIdx].procName, suffix + 1);
	keyword_bytes = strlen(keyword) + 1;
	auto *ed = static_cast<extra_descr_data *>(budget.allocate(sizeof(extra_descr_data)));
	char *saved_keyword = ed ? static_cast<char *>(budget.allocate(keyword_bytes)) : nullptr;
	if (!ed || !saved_keyword)
	{
		const int error = errno ? errno : ENOMEM;
		budget.release(ed, sizeof(extra_descr_data));
		budget.release(params, params_bytes);
		budget.finish(error);
		return libIdx + 1;
	}
	strcpy(saved_keyword, keyword);
	ed->keyword = saved_keyword;
	ed->description = params;
	ed->next = obj->ex_description;
	// Every possible refusal occurs before the original one-way descriptor/flag
	// mutation. Reobserve genuine G immediately before transferring ownership.
	if (!budget.charge())
	{
		const int error = errno;
		budget.release(saved_keyword, keyword_bytes);
		budget.release(ed, sizeof(extra_descr_data));
		budget.release(params, params_bytes);
		budget.finish(error);
		return libIdx + 1;
	}
	obj->ex_description = ed;
	obj->str_mask |= STRUNG_EDESC;
	SET_BIT(obj->extra_flags, ITEM_PROCLIB);
	*library_index = static_cast<size_t>(libIdx);
	// Allocation-free final attachment does not alter global storage; caller
	// reobserves its private object now, before any next mutation/callback.
	errno = 0;
	return 0;
#endif
}

bool quest_mobile_native_original_proclib::probe_bounded(
	P_obj object, size_t index, bool *periodic,
	bool (*current_global)(size_t *, void *) noexcept,
	bool (*reserve_scratch_peak)(size_t, void *) noexcept, void *context,
	size_t outer_live_scratch) noexcept
{
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	_GLIBCXX_USE_CXX11_ABI != 1 || !defined(__linux__) || !defined(__GLIBC__) ||            \
	!defined(__x86_64__) || (defined(MEMCHK) && MEMCHK > 1)
	(void)object;
	(void)index;
	(void)periodic;
	(void)current_global;
	(void)reserve_scratch_peak;
	(void)context;
	(void)outer_live_scratch;
	errno = ENOTSUP;
	return false;
#else
	if (sizeof(void *) != 8 || sizeof(size_t) != 8 || sizeof(int) != 4)
	{
		errno = ENOTSUP;
		return false;
	}
	// Actual entry carrier object, selected function and returned bool. Known
	// original CMD_SET_PERIODIC bodies return before further callback work.
	// sayresponse/transporter declare their first buffers BEFORE that test;
	// count their real declarations even though the periodic branch does not
	// use them. No assumption that the compiler optimizes those arrays away.
	constexpr size_t fixed =
		sizeof(original_proclib_budget) + 8 * sizeof(void *) + 2 * sizeof(size_t) +
		sizeof(bool) + 3 * sizeof(void *) + 2 * sizeof(int) +
		std::max(MAX_STRING_LENGTH + sizeof(extra_descr_data *) + 3 * sizeof(int),
			 MAX_INPUT_LENGTH + sizeof(extra_descr_data *) + sizeof(int)) +
		original_proclib_control_declarations;
	original_proclib_budget budget{ current_global, reserve_scratch_peak, context,
					outer_live_scratch, fixed };
	if (!budget.charge())
		return false;
	if (!object || !periodic || index >= ARRAY_SIZE(object_proc_libs) ||
	    !object_proc_libs[index].func)
		return budget.finish(EINVAL);
	const auto function = object_proc_libs[index].func;
	if (function != proclibobj_hummer && function != proclibobj_actroom &&
	    function != proclibobj_actworn && function != proclibobj_sayresponse &&
	    function != proclibobj_transporter)
		return budget.finish(ENOTSUP);
	try
	{
		const bool requested = function(object, nullptr, CMD_SET_PERIODIC, nullptr);
		if (!budget.finish(0))
			return false;
		*periodic = requested;
		return true;
	}
	catch (...)
	{
		return budget.finish(EINVAL);
	}
#endif
}
