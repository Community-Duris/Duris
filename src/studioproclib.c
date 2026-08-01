/*
   ***************************************************************************
   *  File: studioproclib.c                                   Part of Duris *
   *  Usage: the 'sayresponse' and 'transporter' object proclibs            *
   ***************************************************************************
   *
   * Ported in behaviour from a 2020-era uncommitted patch that carried
   * both procs inside specs.library.c, with three corrections:
   *
   *  1. sprintf() -> snprintf() on both parameter builders.  The originals
   *     wrote two MAX_STRING_LENGTH inputs into one fixed buffer; the
   *     build runs -D_FORTIFY_SOURCE=0, so nothing would have caught it.
   *  2. proclibobj_transporter called char_light()/room_light() after
   *     char_to_room(); char_to_room() already does the light bookkeeping,
   *     and the second call on a NOWHERE char is a crash.  The redundant
   *     calls are gone.
   *  3. The original reached these procs by patching special() in
   *     interp.c to walk every ground object on every command.  Instead
   *     proclibObj_add() installs proclib_obj_cmd_bridge() as the
   *     prototype proc of any vnum that gains a proclib, so the engine's
   *     OWN dispatch (interp.c:2229, "special in object present?") does
   *     the work and interp.c is not touched.  It is also cheaper: only
   *     vnums that actually carry a proclib are ever consulted.
   */

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "prototypes.h"
#include "structs.h"
#include "comm.h"
#include "db.h"
#include "interp.h"
#include "utils.h"
#include "utility.h"
#include "studioproclib.h"

extern P_index obj_index;
extern P_room  world;

/* defined in specs.library.c */
extern char *proclib_getNext_string(char *source, char *nextString);
extern int   proclib_obj_proc(P_obj obj, P_char ch, int cmd, char *argument);

/* ------------------------------------------------------------------ */
/* sayresponse: 'keywords' 'reply text'                                */
/* ------------------------------------------------------------------ */

char *proclibobj_parse_sayresponse(char *argument)
{
	char  arg1[MAX_STRING_LENGTH], arg2[MAX_STRING_LENGTH];
	char  params[MAX_STRING_LENGTH * 2 + 2];
	char *pRet = NULL;

	argument = proclib_getNext_string(argument, arg1);
	if (arg1[0])
	{
		argument = proclib_getNext_string(argument, arg2);
		if (arg2[0])
		{
			snprintf(params, sizeof(params), "%s\xFF%s", arg1, arg2);
			CREATE(pRet, char, strlen(params) + 1, MEM_TAG_EXDESCD);
			strcpy(pRet, params);
			return pRet;
		}
	}
	return NULL;
}

/* Object in the room (or held by the speaker) replies when a player says
   any of the keywords.  Reached from studioproc_speech() with CMD_SAY,
   AFTER the say text has landed, so the reply reads as a reply. */
int proclibobj_sayresponse(P_obj obj, P_char ch, int cmd, char *argument)
{
	struct extra_descr_data *ed;
	char                     low[MAX_STRING_LENGTH];
	int                      room = -1, li, replied = FALSE;

	if (cmd == CMD_SET_PERIODIC)
		return FALSE;                        /* command-driven only */
	if (cmd != CMD_SAY || !obj || !ch || !argument || !*argument)
		return FALSE;

	if (OBJ_ROOM(obj))
		room = obj->loc.room;
	else if (OBJ_CARRIED(obj) && obj->loc.carrying)
		room = obj->loc.carrying->in_room;
	else if (OBJ_WORN(obj) && obj->loc.wearing)
		room = obj->loc.wearing->in_room;
	if (room < 0 || room != ch->in_room)
		return FALSE;

	for (li = 0; argument[li] && li < (int)sizeof(low) - 1; li++)
		low[li] = LOWER(argument[li]);
	low[li] = '\0';

	for (ed = obj->ex_description; ed; ed = ed->next)
	{
		char       *delim;
		char        kw[MAX_INPUT_LENGTH];
		const char *p;
		int         hit = FALSE, k;
		char        buf[MAX_STRING_LENGTH];

		if (!ed->keyword || !ed->description || strn_cmp(ed->keyword, "_proclib_sayresponse", 20))
			continue;

		delim = strchr(ed->description, '\xFF');
		if (!delim || !*(delim + 1))
			continue;

		p = ed->description;
		while (p < delim && !hit)
		{
			while (p < delim && *p == ' ')
				p++;
			for (k = 0; p < delim && *p != ' ' && k < (int)sizeof(kw) - 1; p++, k++)
				kw[k] = LOWER(*p);
			kw[k] = '\0';
			if (k && strstr(low, kw))
				hit = TRUE;
		}
		if (!hit)
			continue;

		snprintf(buf, sizeof(buf) - 3, "%s replies, '%s'", obj->short_description ? obj->short_description : "something", delim + 1);
		CAP(buf);
		strcat(buf, "\r\n");
		send_to_room(buf, room);
		replied = TRUE;
	}
	return replied;
}

/* ------------------------------------------------------------------ */
/* transporter: keyword roomvnum                                       */
/* ------------------------------------------------------------------ */

char *proclibobj_parse_transporter(char *argument)
{
	char  arg1[MAX_STRING_LENGTH], arg2[MAX_STRING_LENGTH];
	char  params[MAX_STRING_LENGTH + 16];
	char *pRet = NULL;

	argument = proclib_getNext_string(argument, arg1);
	if (arg1[0] && !is_number(arg1))
	{
		argument = proclib_getNext_string(argument, arg2);
		if (arg2[0] && is_number(arg2) && atoi(arg2) > 0)
		{
			snprintf(params, sizeof(params), "%s\xFF%d", arg1, atoi(arg2));
			CREATE(pRet, char, strlen(params) + 1, MEM_TAG_EXDESCD);
			strcpy(pRet, params);
			return pRet;
		}
	}
	return NULL;
}

/* 'enter <keyword>' teleports the actor to the configured room, which is
   validated at fire time (an area can be renumbered under our feet). */
int proclibobj_transporter(P_obj obj, P_char ch, int cmd, char *argument)
{
	struct extra_descr_data *ed;
	char                     word[MAX_INPUT_LENGTH];

	if (cmd == CMD_SET_PERIODIC)
		return FALSE;                        /* command-driven only */
	if (cmd != CMD_ENTER || !obj || !ch || !argument)
		return FALSE;

	/* the transporter must be on the ground in the actor's room */
	if (!OBJ_ROOM(obj) || obj->loc.room != ch->in_room)
		return FALSE;

	one_argument(argument, word);
	if (!*word)
		return FALSE;

	for (ed = obj->ex_description; ed; ed = ed->next)
	{
		char *delim;
		char  kw[MAX_INPUT_LENGTH];
		int   klen, rnum;

		if (!ed->keyword || !ed->description || strn_cmp(ed->keyword, "_proclib_transporter", 20))
			continue;

		delim = strchr(ed->description, '\xFF');
		if (!delim || !*(delim + 1))
			continue;

		klen = (int)(delim - ed->description);
		if (klen <= 0 || klen >= (int)sizeof(kw))
			continue;
		strncpy(kw, ed->description, klen);
		kw[klen] = '\0';
		if (str_cmp(word, kw))
			continue;

		rnum = real_room(atoi(delim + 1));
		if (rnum < 0)
		{
			logit(LOG_STATUS, "proclib transporter: obj %d keyword '%s' leads to missing room %d", obj_index[obj->R_num].virtual_number, kw, atoi(delim + 1));
			send_to_char("It doesn't seem to lead anywhere.\r\n", ch);
			return TRUE;
		}
		if (rnum == ch->in_room)
		{
			send_to_char("You are already there.\r\n", ch);
			return TRUE;
		}

		act("$n steps into $p and vanishes.", TRUE, ch, obj, 0, TO_ROOM);
		act("You step into $p...", FALSE, ch, obj, 0, TO_CHAR);
		char_from_room(ch);
		if (!char_to_room(ch, rnum, -1))
		{
			act("$n arrives in a swirl of mist.", TRUE, ch, 0, 0, TO_ROOM);
			if (IS_PC(ch))
			{
				char empty[2];

				empty[0] = '\0';
				do_look(ch, empty, CMD_LOOK);
			}
		}
		return TRUE;
	}
	return FALSE;
}

/* ------------------------------------------------------------------ */
/* prototype bridge                                                    */
/* ------------------------------------------------------------------ */

/* Installed on the vnum of any object that gains a proclib, so that
   interp.c's existing "special in object present?" walk delivers real
   commands to instance proclibs.  Deliberately silent for:
     cmd <= 0   - CMD_PERIODIC / CMD_SET_PERIODIC etc.  Periodic ticking
                  is already owned by proclib_obj_event (scheduled inside
                  proclibObj_add); returning TRUE here would make db.c
                  schedule a SECOND periodic event and double-tick.
     CMD_SAY    - speech is delivered from studioproc_speech() after the
                  say text has landed, so replies read as replies and the
                  player's say is never swallowed. */
int proclib_obj_cmd_bridge(P_obj obj, P_char ch, int cmd, char *argument)
{
	if (!obj || cmd <= 0 || cmd == CMD_SAY)
		return FALSE;
	if (!IS_SET(obj->extra_flags, ITEM_PROCLIB))
		return FALSE;
	return proclib_obj_proc(obj, ch, cmd, argument);
}
