/*
   ***************************************************************************
   *  File: studioproclib.h                                   Part of Duris *
   *  Usage: the 'sayresponse' and 'transporter' object proclibs            *
   *                                                                        *
   *  Objects authored offline already carry _proclib_sayresponse /         *
   *  _proclib_transporter extra-descriptions.  With those names missing    *
   *  from object_proc_libs[], proclibObj_add() returns -1 and              *
   *  read_object() keeps the description as plain text (db.c:2940) - the   *
   *  objects load, nothing fires, nothing is logged.  This module makes    *
   *  the two names real.                                                   *
   *                                                                        *
   *  Kept in its own translation unit so that specs.library.c gains only   *
   *  an include, two registry rows and the prototype bridge.               *
   ***************************************************************************
 */

#ifndef _STUDIOPROCLIB_H_
#define _STUDIOPROCLIB_H_

#include "core/structs.h"

char *proclibobj_parse_sayresponse(char *argument);
int proclibobj_sayresponse(P_obj obj, P_char ch, int cmd, char *argument);

char *proclibobj_parse_transporter(char *argument);
int proclibobj_transporter(P_obj obj, P_char ch, int cmd, char *argument);

/* Prototype-level bridge that lets special() reach INSTANCE proclibs.
   Installed by proclibObj_add() on the vnum of any object that gains a
   proclib, so 'enter <keyword>' reaches proclibobj_transporter without
   patching interp.c at all. */
int proclib_obj_cmd_bridge(P_obj obj, P_char ch, int cmd, char *argument);

/* Remember the object proc the bridge displaced on this vnum, so the
   bridge can call it first instead of the vnum having to choose between
   its existing proc and its instance proclibs. */
void proclib_chain_install(int rnum, int (*prev)(P_obj, P_char, int, char *));

/* Help text for the object_proc_libs[] registry rows in specs.library.c,
   hoisted here so each row stays one line. */
#define PROCLIB_SAYRESPONSE_HELP                                                     \
	"        Params: 'keywords' 'reply text'\n"                                  \
	"          keywords: quoted, space separated; replies when a player's say\n" \
	"                    contains any of them (object in room or held).\n"       \
	"          reply text: quoted; sent to the room as: <object> replies, '...'\n"

#define PROCLIB_TRANSPORTER_HELP                                                     \
	"        Params: keyword roomvnum\n"                                         \
	"          keyword: what the player must type after 'enter'.\n"              \
	"          roomvnum: destination room (validated when fired).  The object\n" \
	"                    must be on the ground in the actor's room.\n"

#endif /* _STUDIOPROCLIB_H_ */
