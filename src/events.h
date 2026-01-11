/* ***************************************************************************
 *  File: events.h                                           Part of Duris *
 *  Usage: defines, macros and structures for event handling.                *
 *  Copyright  1994, 1995 - John Bashaw and Duris Systems Ltd.             *
 *************************************************************************** */

#ifndef _SOJ_EVENTS_H_
#define _SOJ_EVENTS_H_

#include <sys/types.h>

#ifndef _SOJ_STRUCTS_H
# ifdef _LINUX_SOURCE
#  include "structs.h"
# endif
#endif

#define T_PULSES  1
#define T_SECS    2
#define T_ROUNDS  3
#define T_MINS    4
#define T_HOURS   5
#define T_DAYS    6

/* Removed a bunch of event types, eventually wana have only 1 event
 * type where you give directly a function to be called after
 * given time. the signature will be 
 * void func(P_char ch, P_char victim, P_obj obj, void *data)
 * - Tharkun
 */
#define EVENT_NONE            0	/* a NULL event, figured it would be a wise
				   thing to include. */
#define EVENT_COMMAND_WAIT    2	/* unset actor's COMMAND flag (replaces old
				   WAIT_STATE, now active for all chars, PCs
				   and NPCs equally.  The delay setting
				   function checks and uses the longer of:
				   current delay/new delay, so if a char
				   had 6 pulses to go from casting a spell
				   and is then bashed, the bash delay replaces
				   the current 6 pulse delay. */
#define EVENT_HIT_REGEN       3	/* change a_ch's current hitpoint total */
#define EVENT_MOVE_REGEN      4	/* change a_ch's current moves */
#define EVENT_MANA_REGEN      5	/* change a_ch's current mana */
#define EVENT_MOB_MUNDANE     6	/* check to see if a_ch wants to move */
#define EVENT_MOB_SPECIAL     7	/* check to see if a_ch wants to do something
				   (these two replace old PULSE_MOBILE calls)
				   */
#define EVENT_OBJ_SPECIAL     8	/* check to see if an object wants to do
				   something (currently only the Waterdeep
				   clock tower) */
#define EVENT_FALLING_CHAR   10	/* a_ch falls one room, replaces the
				   check_fall_all() function, if they don't
				   hit bottom, this EVENT schedules another
				   one next PULSE (fall 4 rooms/sec) */
#define EVENT_FALLING_OBJ    11	/* a_obj falls one room, not used yet, but I
				   can think of some neat thngs to do with
				   this, so I added it. */
#define EVENT_RESET_ZONE     12	/* zone resets are now staggered out over time
				   so they don't pile up.  These are set at
				   bootup and are cyclical. */
#define EVENT_OBJ_AFFECT     13	/* wearing off of obj affects */
#define EVENT_SPECIAL        15	/* call a void function to do something
				   unrelated to any specific mob/obj/room,
				   (WD city noises) */

/* Note: All of the following (except scribe) could use just
   EVENT_CHAR_EXECUTE with diff. subfunction, _but_ for sake of
   debugging help, I leave them as their seperate event types. */

#define EVENT_TRACK_DECAY    19	/* tracks in rooms */
#define EVENT_SPELL_MEM      21	/* spell memorization */
#define EVENT_SPELLCAST      22	/* spell delayed casting */
#define EVENT_CHAR_EXECUTE   26	/* generic execution of function that is given
				   as other parameter to AddEvent with char as
				   only parameter. */
#define EVENT_BALANCE_AFFECTS 32 /* event to wear off berserk skill */
#define EVENT_MOB_HUNT       33  /* used to move HUNTER mobs to their
                                    victims */

#define EVENT_SWIMMING       35 /* Tires them out, eventually may drown them */
#define EVENT_RESET_ZONE_COMPLETE 38
#define EVENT_BALANCE_AFFECTS_NODIE 42 /* like BALANCE_AFFECTS but ch never dies */
#define EVENT_SHORT_AFFECT 46
#define EVENT_ROOM_AFFECT  47
#define EVENT_UNDEAD_MEM    50
#define EVENT_ARTIFACT      52 /* handles artifact feeding */
#define EVENT_SACKING      53 /* handles artifact feeding */
#define EVENT_INTERACTION 64
#define EVENT_WARD_REGEN  65


#define LAST_EVENT         66
/* used by the event_sub_list and
				   event_counter arrays, must always be 1
				   more than that last defined EVENT_* type */

/* useful macros */

#define AddEvent(type, time, flag, actor, target) \
  Schedule((type), (long)(time), (flag), (void *)(actor), (void *)(target))

#define FIND_EVENT_TYPE(var, etype)  \
  for ((var) = event_list; (var); (var) = (var)->next_event) \
		if ((var)->type == (etype))

#define LOOP_EVENTS_CH( var, e_list )  \
  for( (var) = (e_list); (var); (var) = (var)->next_char_nev )

#define LOOP_EVENTS_OBJ( var, e_list )  \
  for( (var) = (e_list); (var); (var) = (var)->next_obj_nev )

#endif  /* _SOJ_EVENTS_H_ */
