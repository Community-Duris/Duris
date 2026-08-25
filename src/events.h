/* ***************************************************************************
 *  File: events.h                                           Part of Duris *
 *  Usage: defines, macros and structures for event handling.                *
 *  Copyright  1994, 1995 - John Bashaw and Duris Systems Ltd.             *
 *************************************************************************** */

#ifndef _SOJ_EVENTS_H_
#define _SOJ_EVENTS_H_

#include <sys/types.h>

#ifndef _SOJ_STRUCTS_H
#ifdef _LINUX_SOURCE
#include "structs.h"
#endif
#endif

#define T_PULSES 1
#define T_SECS 2
#define T_ROUNDS 3
#define T_MINS 4
#define T_HOURS 5
#define T_DAYS 6

/* Removed a bunch of event types, eventually wana have only 1 event
 * type where you give directly a function to be called after
 * given time. the signature will be
 * void func(P_char ch, P_char victim, P_obj obj, void *data)
 * - Tharkun
 */
/* A NULL event, figured it would be a wise thing to include. */
#define EVENT_NONE 0
/*
 * Unset actor's COMMAND flag (replaces old WAIT_STATE, now active for all chars,
 * PCs and NPCs equally). The delay setting function checks and uses the longer of
 * the current and new delays. If a char has 6 pulses left from casting a spell and
 * is then bashed, the bash delay replaces the current 6-pulse delay.
 */
#define EVENT_COMMAND_WAIT 2
#define EVENT_HIT_REGEN 3 /* change a_ch's current hitpoint total */
#define EVENT_MOVE_REGEN 4 /* change a_ch's current moves */
#define EVENT_MANA_REGEN 5 /* change a_ch's current mana */
#define EVENT_MOB_MUNDANE 6 /* check to see if a_ch wants to move */
/* Check whether a_ch wants to act; with EVENT_MOB_MUNDANE, replaces PULSE_MOBILE. */
#define EVENT_MOB_SPECIAL 7
/* Check whether an object wants to act; currently only the Waterdeep clock tower. */
#define EVENT_OBJ_SPECIAL 8
/* Fall one room; if a_ch does not hit bottom, schedule another event next pulse. */
#define EVENT_FALLING_CHAR 10
/* Make a_obj fall one room; currently unused. */
#define EVENT_FALLING_OBJ 11
/* Zone resets are staggered at boot and cyclical so they do not pile up. */
#define EVENT_RESET_ZONE 12
#define EVENT_OBJ_AFFECT 13 /* wearing off of obj affects */
/* Call a void function unrelated to a specific mob, object, or room (WD city noises). */
#define EVENT_SPECIAL 15

/* Note: All of the following (except scribe) could use just
   EVENT_CHAR_EXECUTE with diff. subfunction, _but_ for sake of
   debugging help, I leave them as their seperate event types. */

#define EVENT_TRACK_DECAY 19 /* tracks in rooms */
#define EVENT_SPELL_MEM 21 /* spell memorization */
#define EVENT_SPELLCAST 22 /* spell delayed casting */
/* Execute a function passed to AddEvent with a char as its only parameter. */
#define EVENT_CHAR_EXECUTE 26
#define EVENT_BALANCE_AFFECTS 32 /* event to wear off berserk skill */
/* Move HUNTER mobs toward their victims. */
#define EVENT_MOB_HUNT 33

#define EVENT_SWIMMING 35 /* Tires them out, eventually may drown them */
#define EVENT_RESET_ZONE_COMPLETE 38
#define EVENT_BALANCE_AFFECTS_NODIE 42 /* like BALANCE_AFFECTS but ch never dies */
#define EVENT_SHORT_AFFECT 46
#define EVENT_ROOM_AFFECT 47
#define EVENT_UNDEAD_MEM 50
#define EVENT_ARTIFACT 52 /* handles artifact feeding */
#define EVENT_SACKING 53 /* handles artifact feeding */
#define EVENT_INTERACTION 64
#define EVENT_WARD_REGEN 65

#define LAST_EVENT 66
/* used by the event_sub_list and
                   event_counter arrays, must always be 1
                   more than that last defined EVENT_* type */

/* useful macros */

#define AddEvent(type, time, flag, actor, target) \
	Schedule((type), (long)(time), (flag), (void *)(actor), (void *)(target))

#define FIND_EVENT_TYPE(var, etype)                                \
	for ((var) = event_list; (var); (var) = (var)->next_event) \
		if ((var)->type == (etype))

#define LOOP_EVENTS_CH(var, e_list) for ((var) = (e_list); (var); (var) = (var)->next_char_nev)

#define LOOP_EVENTS_OBJ(var, e_list) for ((var) = (e_list); (var); (var) = (var)->next_obj_nev)

#endif /* _SOJ_EVENTS_H_ */
