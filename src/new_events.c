
/*
 * ***************************************************************************
 * *  File: events.c                                           Part of Duris *
 * *  Usage: manipulate various event lists
 * * *  Copyright  1994, 1995 - John Bashaw and Duris Systems Ltd.
 * *
 * ***************************************************************************
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <vector>
#ifndef _LINUX_SOURCE
#include <sys/types.h>
#endif

#include "prototypes.h"
#include "structs.h"
#include "comm.h"
#include "db.h"
#include "events.h"
#include "event_names.h"
#include "interp.h"
#include "utils.h"
#include "copyover.h"
#include "epic.h"
#include "justice.h"
#include "mm.h"
#include "objmisc.h"
#include "outposts.h"
#include "profile.h"
#include "redis.h"
#include "specs.prototypes.h"
#include "spells.h"
#include "vnum.obj.h"

#define FUNCTION_NAMES_FILE "lib/misc/event_names"
#define NEVENT_BUDGET_USEC_DEFAULT 25000L
#define NEVENT_MAX_CALLBACKS_DEFAULT 4000L
#define NEVENT_PRIORITY_NORMAL 0U
#define NEVENT_PRIORITY_PLAYER 1U
#define NEVENT_LIFECYCLE_ACTIVE 1U
#define NEVENT_LIFECYCLE_CANCEL_PENDING 2U
#define NEVENT_LIFECYCLE_DESTROYING 3U
#define NEVENT_LIFECYCLE_RELEASED 4U
#define NEVENT_MAX_DEFERRALS 0U
#define NEVENT_CATCHUP_WINDOW_PULSES 4
#define NEVENT_CATCHUP_MAX_EXTENSION_USEC_DEFAULT 5000L
#define NEVENT_CATCHUP_MAX_EXTRA_CALLBACKS_DEFAULT 4000L
#define NEVENT_CALLBACK_EWMA_SHIFT 4
#define NEVENT_ANALYTICS_CALLBACK_SLOTS 128
#define NEVENT_ANALYTICS_CALLBACK_NAME 96

/*
 * internal variables
 */
bool debug_event_list = FALSE;
P_nevent current_nevent = NULL;
long ne_event_counter = 0;
unsigned long long ne_event_tick = 0;
static unsigned long long ne_event_sequence = 0;

struct nevent_callback_analytics
{
	void *func;
	char name[NEVENT_ANALYTICS_CALLBACK_NAME];
	long long calls;
	long long total_us;
	long max_us;
	long long deferred;
};

struct nevent_analytics_data
{
	unsigned long long window_start_tick;
	long pulses;
	long long total_scanned;
	long long total_executed;
	long long total_deferred;
	long long total_us;
	long peak_scanned;
	long peak_executed;
	long peak_deferred;
	long peak_total_us;
	long peak_pending;
	unsigned long long peak_executed_tick;
	unsigned long long peak_total_us_tick;
	long budget_exhausted_pulses;
	long callback_overflow;
	struct nevent_callback_analytics callbacks[NEVENT_ANALYTICS_CALLBACK_SLOTS];
};

static struct nevent_analytics_data nevent_analytics;
static long nevent_catchup_debt = 0;
static int nevent_catchup_remaining = 0;
static long nevent_avg_callback_us = 50;
static long nevent_catchup_quota = 0;
static long nevent_catchup_extension_us = 0;
static long nevent_catchup_extra_callbacks = 0;
static std::vector<nevent_handle> nevent_pending_cancellations;

static void nevent_complete_deferred(P_nevent event);

static void nevent_destroy_raw_payload(void *data)
{
	FREE(data);
}

/*
 * this code was majorly redone by Tharkun, look for original events description
 * in events.c file
 */
struct mm_ds *ne_dead_event_pool = NULL;

/*
 * main array of pointers to lists, schedule is the 'master' controller,
 * has one element per pulse in a real minute.
 */
P_nevent ne_schedule[PULSES_IN_TICK];
P_nevent ne_schedule_tail[PULSES_IN_TICK];

/*
 * external variables
 */
extern Skill skills[];
/* if true, we have called Events() from the main loop this pulse already */
extern bool after_events_call;
extern P_char character_list;
extern P_index mob_index;
extern P_index obj_index;
extern P_obj object_list;
extern P_room world;
extern int errno;
extern int pulse;
extern int top_of_mobt;
extern int top_of_objt;
extern const int top_of_world;
extern int top_of_zone_table;
extern struct time_info_data time_info;
extern struct zone_data *zone;
extern struct zone_data *zone_table;
extern struct sector_data *sector_table;
extern const struct racial_data_type racial_data[LAST_RACE + 1];
void interaction_to_new_wrapper(P_char, P_char, char *);
void event_reset_zone(P_char ch, P_char victim, P_obj obj, void *data);
void register_func_call(void *func, double time);
const char *get_function_name(void *func);
void release_mob_mem(P_char ch, P_char victim, P_obj obj, void *data);
extern void event_mob_mundane(P_char, P_char, P_obj, void *);
extern void event_spellcast(P_char, P_char, P_obj, void *);
extern void event_memorize(P_char, P_char, P_obj, void *);
extern void event_wait(P_char, P_char, P_obj, void *);
extern void event_mana_regen(P_char, P_char, P_obj, void *);
extern void event_move_regen(P_char, P_char, P_obj, void *);
extern void event_hit_regen(P_char, P_char, P_obj, void *);
extern void event_ward_regen(P_char, P_char, P_obj, void *);
extern void event_balance_affects(P_char, P_char, P_obj, void *);
static long nevent_config_limit(const char *name, long fallback);

static const char *nevent_callback_label(event_func_type func)
{
	if (func == event_hit_regen)
		return "event_hit_regen";
	if (func == event_mana_regen)
		return "event_mana_regen";
	if (func == event_move_regen)
		return "event_move_regen";
	if (func == event_ward_regen)
		return "event_ward_regen";
	if (func == event_spellcast)
		return "event_spellcast";
	if (func == event_memorize)
		return "event_memorize";
	if (func == event_wait)
		return "event_wait";
	if (func == event_balance_affects)
		return "event_balance_affects";
	if (func == event_mob_mundane)
		return "event_mob_mundane";
	if (func == event_reset_zone)
		return "event_reset_zone";
	return get_function_name((void *)func);
}

static bool nevent_is_player_timed(event_func_type func, P_char ch)
{
	if (func == event_spellcast || func == event_memorize || func == event_balance_affects)
		return ch != NULL;
	/* event_wait clears the player's command gate set by CharWait().  If it
	 * misses its deadline, the player remains unable to issue commands after
	 * the visible action (cast/flee/combat action) has completed. */
	if (func == event_wait)
		return ch != NULL && (IS_PC(ch) || (IS_NPC(ch) && GET_MASTER(ch) &&
						    IS_AFFECTED5(GET_MASTER(ch), AFF5_ORDERING)));
	return ch && IS_PC(ch) &&
	       (func == event_mana_regen || func == event_move_regen || func == event_hit_regen);
}

static unsigned int nevent_priority(event_func_type func, P_char ch)
{
	static long player_priority = -1;
	if (player_priority < 0)
		player_priority = nevent_config_limit("DURIS_NEVENT_PLAYER_PRIORITY", 1);
	if (player_priority > 0 && nevent_is_player_timed(func, ch))
		return NEVENT_PRIORITY_PLAYER;
	return NEVENT_PRIORITY_NORMAL;
}

static void nevent_link_schedule(P_nevent event, int loc)
{
	P_nevent cursor;
	P_nevent last_player = NULL;

	if (!ne_schedule[loc])
	{
		ne_schedule[loc] = event;
		ne_schedule_tail[loc] = event;
		return;
	}

	if (!nevent_is_player_timed(event->func, event->ch))
	{
		event->prev_sched = ne_schedule_tail[loc];
		ne_schedule_tail[loc]->next_sched = event;
		ne_schedule_tail[loc] = event;
		return;
	}

	for (cursor = ne_schedule[loc]; cursor && nevent_is_player_timed(cursor->func, cursor->ch);
	     cursor = cursor->next_sched)
		last_player = cursor;

	if (!last_player)
	{
		event->next_sched = ne_schedule[loc];
		ne_schedule[loc]->prev_sched = event;
		ne_schedule[loc] = event;
		return;
	}

	event->prev_sched = last_player;
	event->next_sched = last_player->next_sched;
	last_player->next_sched = event;
	if (event->next_sched)
		event->next_sched->prev_sched = event;
	else
		ne_schedule_tail[loc] = event;
}

static bool nevent_overdue_event(P_nevent event)
{
	/* Player-timed events remain normally prioritized, but ordinary events
	 * must not starve behind a continuously busy player prefix. */
	static_assert(NEVENT_MAX_DEFERRALS == 0U,
		      "Immediate overdue-event promotion is part of the scheduler contract");
	return event != NULL;
}

/* Keep the budget as the default, but allow one repeatedly deferred event to
 * run so every class of work continues making progress. */
static bool nevent_promote_overdue_event(P_nevent *next_event, P_nevent anchor)
{
	P_nevent candidate;
	P_nevent prior;
	P_nevent following;

	if (!next_event || !*next_event)
		return FALSE;

	for (candidate = *next_event; candidate; candidate = candidate->next_sched)
	{
		if (!nevent_overdue_event(candidate))
			continue;
		if (candidate == *next_event)
			return TRUE;

		prior = candidate->prev_sched;
		following = candidate->next_sched;
		prior->next_sched = following;
		if (following)
			following->prev_sched = prior;
		else
			ne_schedule_tail[pulse] = prior;

		candidate->prev_sched = anchor;
		if (anchor)
		{
			candidate->next_sched = anchor->next_sched;
			if (anchor->next_sched)
				anchor->next_sched->prev_sched = candidate;
			else
				ne_schedule_tail[pulse] = candidate;
			anchor->next_sched = candidate;
		}
		else
		{
			candidate->next_sched = ne_schedule[pulse];
			if (ne_schedule[pulse])
				ne_schedule[pulse]->prev_sched = candidate;
			else
				ne_schedule_tail[pulse] = candidate;
			ne_schedule[pulse] = candidate;
		}
		*next_event = candidate;
		return TRUE;
	}
	return FALSE;
}

static void nevent_detach_character(P_nevent event)
{
	P_char ch = event->ch;
	P_nevent cursor;

	if (!ch)
		return;
	if (ch->nevents == event)
	{
		ch->nevents = event->next_char_nev;
	}
	else
	{
		for (cursor = ch->nevents; cursor && cursor->next_char_nev != event;
		     cursor = cursor->next_char_nev)
			;
		if (cursor)
			cursor->next_char_nev = event->next_char_nev;
		else
			debug("nevent_detach_character: event sequence %llu is absent from its owner list",
			      event->sequence);
	}
	event->ch = NULL;
	event->next_char_nev = NULL;
}

static void nevent_detach_object(P_nevent event)
{
	P_obj obj = event->obj;
	P_nevent cursor;

	if (!obj)
		return;
	if (obj->nevents == event)
	{
		obj->nevents = event->next_obj_nev;
	}
	else
	{
		for (cursor = obj->nevents; cursor && cursor->next_obj_nev != event;
		     cursor = cursor->next_obj_nev)
			;
		if (cursor)
			cursor->next_obj_nev = event->next_obj_nev;
		else
			debug("nevent_detach_object: event sequence %llu is absent from its owner list",
			      event->sequence);
	}
	event->obj = NULL;
	event->next_obj_nev = NULL;
}

static void nevent_detach_owners(P_nevent event)
{
	if (event->cld && event->ch)
		remove_link(event->ch, event->cld);
	event->cld = NULL;
	event->victim = NULL;
	nevent_detach_object(event);
	nevent_detach_character(event);
}

static void nevent_unlink_schedule(P_nevent event)
{
	const unsigned int element = event->element;

	if (element >= PULSES_IN_TICK)
		panic_corruption("nevent", "event sequence %llu has invalid bucket %u",
				 event->sequence, element);
	if (event->prev_sched)
	{
		if (event->prev_sched->next_sched != event)
			panic_corruption("nevent", "event sequence %llu has a broken previous link",
					 event->sequence);
		event->prev_sched->next_sched = event->next_sched;
	}
	else
	{
		if (ne_schedule[element] != event)
			panic_corruption("nevent", "event sequence %llu is not its bucket head",
					 event->sequence);
		ne_schedule[element] = event->next_sched;
	}

	if (event->next_sched)
	{
		if (event->next_sched->prev_sched != event)
			panic_corruption("nevent", "event sequence %llu has a broken next link",
					 event->sequence);
		event->next_sched->prev_sched = event->prev_sched;
	}
	else
	{
		if (ne_schedule_tail[element] != event)
			panic_corruption("nevent", "event sequence %llu is not its bucket tail",
					 event->sequence);
		ne_schedule_tail[element] = event->prev_sched;
	}
	event->prev_sched = NULL;
	event->next_sched = NULL;
}

static bool nevent_destroy(P_nevent event)
{
	if (!event || (event->lifecycle_state != NEVENT_LIFECYCLE_ACTIVE &&
		       event->lifecycle_state != NEVENT_LIFECYCLE_CANCEL_PENDING))
		return FALSE;

	event->lifecycle_state = NEVENT_LIFECYCLE_DESTROYING;
	nevent_detach_owners(event);
	nevent_unlink_schedule(event);
	if (event->data)
	{
		if (!event->data_destroy)
			panic_corruption("nevent",
					 "event sequence %llu has data without a destructor",
					 event->sequence);
		event->data_destroy(event->data);
		event->data = NULL;
		event->data_destroy = NULL;
	}
	if (event->deferral_count > 0)
	{
		nevent_complete_deferred(event);
		event->deferral_count = 0;
	}
	if (ne_event_counter <= 0)
		panic_corruption("nevent", "destroying sequence %llu with counter %ld",
				 event->sequence, ne_event_counter);
	ne_event_counter--;
	event->func = NULL;
	event->timer = 0;
	event->lifecycle_state = NEVENT_LIFECYCLE_RELEASED;
	mm_release(ne_dead_event_pool, event);
	return TRUE;
}

nevent_handle nevent_handle_from_event(P_nevent event)
{
	return { event, event ? event->sequence : 0 };
}

nevent_cancel_result nevent_cancel(nevent_handle handle)
{
	P_nevent event = handle.event;

	if (!event || handle.sequence == 0)
		return nevent_cancel_result::invalid_handle;
	if (event->sequence != handle.sequence)
		return nevent_cancel_result::stale_handle;
	if (event->lifecycle_state == NEVENT_LIFECYCLE_CANCEL_PENDING ||
	    event->lifecycle_state == NEVENT_LIFECYCLE_DESTROYING ||
	    event->lifecycle_state == NEVENT_LIFECYCLE_RELEASED)
		return nevent_cancel_result::already_inactive;
	if (event->lifecycle_state != NEVENT_LIFECYCLE_ACTIVE)
		return nevent_cancel_result::invalid_handle;

	event->lifecycle_state = NEVENT_LIFECYCLE_CANCEL_PENDING;
	event->func = NULL;
	if (current_nevent)
	{
		nevent_detach_owners(event);
		nevent_pending_cancellations.push_back(handle);
		return nevent_cancel_result::deferred;
	}

	nevent_destroy(event);
	return nevent_cancel_result::canceled;
}

static void nevent_process_pending_cancellations()
{
	std::vector<nevent_handle> pending;
	pending.swap(nevent_pending_cancellations);
	for (const nevent_handle &handle : pending)
	{
		if (handle.event && handle.event->sequence == handle.sequence &&
		    handle.event->lifecycle_state == NEVENT_LIFECYCLE_CANCEL_PENDING)
			nevent_destroy(handle.event);
	}
}

// Returns true iff all the events in ch->nevents belong to ch.
bool check_ch_nevents(P_char ch)
{
	P_nevent e;

	if (!IS_ALIVE(ch))
	{
		return TRUE;
	}

	LOOP_EVENTS_CH(e, ch->nevents)
	{
		if (e->ch != NULL && e->ch != ch)
		{
			return FALSE;
		}
	}
	return TRUE;
}

// Returns true iff all the events in obj->nevents belong to obj.
bool check_obj_nevents(P_obj obj)
{
	P_nevent e;

	if (obj == NULL)
	{
		return TRUE;
	}

	LOOP_EVENTS_OBJ(e, obj->nevents)
	{
		if (e->obj != obj)
		{
			return FALSE;
		}
	}
	return TRUE;
}

// Compatibility wrappers now use the scheduler-owned cancellation lifecycle.
void disarm_single_event(P_nevent event)
{
	nevent_cancel(nevent_handle_from_event(event));
}

void disarm_char_nevents(P_char ch, event_func_type func)
{
	P_nevent event, next;

	if (!ch)
		return;
	for (event = ch->nevents; event; event = next)
	{
		next = event->next_char_nev;
		if (!func || event->func == func)
			nevent_cancel(nevent_handle_from_event(event));
	}
}

void disarm_obj_nevents(P_obj obj, event_func_type func)
{
	P_nevent event, next;

	if (!obj)
		return;
	for (event = obj->nevents; event; event = next)
	{
		next = event->next_obj_nev;
		if (!func || event->func == func)
			nevent_cancel(nevent_handle_from_event(event));
	}
}

static void add_event_internal(event_func func, int delay, P_char ch, P_char victim, P_obj obj,
			       const void *data, int data_size, void *owned_payload,
			       nevent_payload_destroy_type owned_payload_destroy)
{
	P_nevent event, e;
	char *data_buf;
	int loc;
	auto discard_owned_payload = [&]()
	{
		if (owned_payload && owned_payload_destroy)
			owned_payload_destroy(owned_payload);
	};

	if (!func)
	{
		debug("add_event: No function!");
		discard_owned_payload();
		return;
	}

	if (delay < 0)
	{
		debug("add_event: Delay (%d) les than zero?!", delay);
		discard_owned_payload();
		return;
	}

	if (ch && !IS_ALIVE(ch) && func != release_mob_mem)
	{
		logit(LOG_DEBUG, "add_event: dead ch '%s' in room r%d/v%d function %s",
		      GET_NAME(ch), ch->in_room, ROOM_VNUM(ch->in_room),
		      get_function_name((void *)func));
		debug("add_event: dead ch '%s' in room r%d/v%d function %s", GET_NAME(ch),
		      ch->in_room, ROOM_VNUM(ch->in_room), get_function_name((void *)func));
		discard_owned_payload();
		return;
	}

	// No reason an event can't have an object and a ch/victim. - Lohrr
	// Ok, here's the reason... we have an object-events list and a char-events list.
	// Well, now we have two lists: obj->nevents->next_obj_nev ... and ch->nevents->next_char_nev.
	/*
	  if( obj && ch )
	  {
	    // Logging them anyway, just to test..
	    debug( "add_event: func: '%s' has ch: %s %d and obj: %s %d.",
	      (func == NULL) ? "NULL" : get_function_name((void*)func),
	      ch ? J_NAME(ch) : "NULL", ch ? GET_ID(ch) : -1,
	      obj ? OBJ_SHORT(obj) : "NULL", obj ? OBJ_VNUM(obj) : -1 );
	    return;
	  }
	*/

	// Should it be possible for an object to have a victim w/out a ch?
	if (victim && !ch)
	{
		debug("add_event: victim '%s' & !ch, func: %s, obj: %s %d.", J_NAME(victim),
		      (func == NULL) ? "NULL" : get_function_name((void *)func),
		      obj ? OBJ_SHORT(obj) : "NULL", obj ? OBJ_VNUM(obj) : -1);
		discard_owned_payload();
		return;
	}

	if ((delay == 0) && after_events_call)
	{
		delay++;
	}

	event = (P_nevent)mm_get(ne_dead_event_pool);

	event->prev_sched = event->next_sched = NULL;
	event->next_char_nev = event->next_obj_nev = NULL;
	event->ch = ch;
	event->victim = victim;
	event->obj = obj;
	event->func = func;
	event->data = NULL;
	event->data_destroy = NULL;
	event->priority = nevent_priority(func, ch);
	event->scheduled_tick = ne_event_tick + (unsigned long long)delay;
	event->sequence = ++ne_event_sequence;
	event->lifecycle_state = NEVENT_LIFECYCLE_ACTIVE;

	if (ch && victim && ch != victim)
		event->cld = link_char(ch, victim, LNK_EVENT);
	else
		event->cld = NULL;

	if (owned_payload)
	{
		if (!owned_payload_destroy)
			panic_corruption("nevent", "owned payload for %s has no destructor",
					 get_function_name((void *)func));
		event->data = owned_payload;
		event->data_destroy = owned_payload_destroy;
	}
	else if (data && data_size > 0)
	{
		CREATE(data_buf, char, data_size, MEM_TAG_EVTBUF);
		// data_buf = (char*)malloc(data_size);
		event->data = memcpy(data_buf, data, data_size);
		event->data_destroy = nevent_destroy_raw_payload;
	}

	loc = (delay + pulse) % PULSES_IN_TICK;

	event->timer = (delay / PULSES_IN_TICK) + 1;
	event->element = loc;

	if (ch)
	{
		if (ch->nevents != NULL)
		{
			// Put event at the end.
			// Loop through ch's events and find the last one.
			LOOP_EVENTS_CH(e, ch->nevents)
			{
				if (e->next_char_nev == NULL)
					break;
			}
			// Put last->next to event.
			e->next_char_nev = event;
		}
		else
			ch->nevents = event;
		// Event at end of ch->nevents, so terminate list.
		event->next_char_nev = NULL;
	}

	if (obj)
	{
		event->next_obj_nev = obj->nevents;
		obj->nevents = event;
	}

	nevent_link_schedule(event, loc);
	ne_event_counter++;

	if (debug_event_list)
	{
		check_nevents();
	}

	return;
}

void add_event(event_func func, int delay, P_char ch, P_char victim, P_obj obj, int /*flag*/,
	       const void *data, int data_size)
{
	add_event_internal(func, delay, ch, victim, obj, data, data_size, NULL, NULL);
}

void add_event_owned_payload(event_func func, int delay, P_char ch, P_char victim, P_obj obj,
			     int /*flag*/, void *payload,
			     nevent_payload_destroy_type payload_destroy)
{
	add_event_internal(func, delay, ch, victim, obj, NULL, 0, payload, payload_destroy);
}

// Returns the time left (in pulses) in the e1 event
int ne_event_time(P_nevent e1)
{
	const int event_element = static_cast<int>(e1->element);
	int time_left;

	time_left = (e1->timer - 1) * PULSES_IN_TICK + event_element - pulse;
	if (event_element < pulse)
		time_left += PULSES_IN_TICK;

	return time_left;
}

/*  Not sure what this does, but commenting out because it seems unnecessary.
void nevent_from_char( P_nevent old_nevent )
{
  P_char ch;
  P_nevent e;

  if( !old_nevent || !(ch = old_nevent->ch) )
    return;

  // Pull from list
  if( ch->nevents == old_nevent )
  {
    ch->nevents = ch->nevents->next_char_nev;
    if( ch->nevents && ch != ch->nevents->ch && ch->nevents->ch != NULL )
    {
      debug( "nevent_from_char: ch '%s' %d not ch in ch->nevents, func %s.",
        IS_ALIVE(ch) ? J_NAME(ch) : GET_NAME(ch), GET_ID(ch), get_function_name((void *)ch->nevents->func) );
      ch->nevents = NULL;
    }
  }
  else
  {
    if( ch->nevents && ch->nevents->ch != ch && ch->nevents->ch != NULL )
    {
      debug( "nevent_from_char: ch '%s' %d not ch in ch->nevents, func %s.",
        IS_ALIVE(ch) ? J_NAME(ch) : GET_NAME(ch), GET_ID(ch), get_function_name((void *)ch->nevents->func) );
      ch->nevents = NULL;
    }
    LOOP_EVENTS_CH( e, ch->nevents )
    {
      if( e->next_char_nev && e->next_char_nev->ch && e->next_char_nev->ch != ch )
      {
        debug( "nevent_from_char: ch '%s' %d is not ch in sub-event e->next_char_nev, func %s.",
          IS_ALIVE(ch) ? J_NAME(ch) : GET_NAME(ch), GET_ID(ch), get_function_name((void *)e->next_char_nev->func) );
        e->next_char_nev = NULL;
        break;
      }
      if( e->next_char_nev == old_nevent )
      {
        e->next_char_nev = old_nevent->next_char_nev;
        old_nevent->next_char_nev = NULL;
        if( e->next_char_nev && ch != e->next_char_nev->ch && e->next_char_nev->ch != NULL )
        {
          debug( "nevent_from_char: ch '%s' %d not ch in ch->nevents, func %s.",
            IS_ALIVE(ch) ? J_NAME(ch) : GET_NAME(ch), GET_ID(ch), get_function_name((void *)e->next_char_nev->func) );
          e->next_char_nev = NULL;
        }
        break;
      }
    }
  }
}
*/

static long nevent_config_limit(const char *name, long default_value)
{
	const char *raw = getenv(name);
	char *end = NULL;
	long value;

	if (!raw || !*raw)
		return default_value;
	value = strtol(raw, &end, 10);
	if (!end || *end != '\0' || value < 0)
	{
		logit(LOG_STATUS, "Invalid %s='%s'; using %ld", name, raw, default_value);
		return default_value;
	}
	return value;
}

static long nevent_budget_usec(void)
{
	static long configured = -1;
	if (configured < 0)
		configured =
			nevent_config_limit("DURIS_NEVENT_BUDGET_USEC", NEVENT_BUDGET_USEC_DEFAULT);
	return configured;
}

static long nevent_max_callbacks(void)
{
	static long configured = -1;
	if (configured < 0)
		configured = nevent_config_limit("DURIS_NEVENT_MAX_CALLBACKS",
						 NEVENT_MAX_CALLBACKS_DEFAULT);
	return configured;
}

static long nevent_catchup_max_extension_us(void)
{
	static long configured = -1;

	if (configured < 0)
		configured = nevent_config_limit("DURIS_NEVENT_CATCHUP_MAX_EXTENSION_USEC",
						 NEVENT_CATCHUP_MAX_EXTENSION_USEC_DEFAULT);
	return configured;
}

static long nevent_catchup_max_extra_callbacks(void)
{
	static long configured = -1;

	if (configured < 0)
		configured = nevent_config_limit("DURIS_NEVENT_CATCHUP_MAX_EXTRA_CALLBACKS",
						 NEVENT_CATCHUP_MAX_EXTRA_CALLBACKS_DEFAULT);
	return configured;
}

static void nevent_record_callback_cost(long callback_us)
{
	if (callback_us < 1)
		callback_us = 1;
	if (nevent_avg_callback_us < 1)
		nevent_avg_callback_us = callback_us;
	else
		nevent_avg_callback_us += (callback_us - nevent_avg_callback_us) >>
					  NEVENT_CALLBACK_EWMA_SHIFT;
	if (nevent_avg_callback_us < 1)
		nevent_avg_callback_us = 1;
}

static void nevent_add_catchup_debt(long deferred)
{
	if (deferred <= 0)
		return;
	nevent_catchup_debt += deferred;
	if (nevent_catchup_remaining <= 0)
		nevent_catchup_remaining = NEVENT_CATCHUP_WINDOW_PULSES;
}

static void nevent_complete_deferred(P_nevent event)
{
	if (event && event->deferral_count > 0 && nevent_catchup_debt > 0)
		nevent_catchup_debt--;
}

static void nevent_prepare_catchup(long base_budget_usec, long base_max_callbacks,
				   long *effective_budget_usec, long *effective_max_callbacks)
{
	long max_extra_callbacks = nevent_catchup_max_extra_callbacks();
	long max_extension_usec = nevent_catchup_max_extension_us();
	long callback_budget;

	nevent_catchup_quota = 0;
	nevent_catchup_extra_callbacks = 0;
	nevent_catchup_extension_us = 0;
	*effective_budget_usec = base_budget_usec;
	*effective_max_callbacks = base_max_callbacks;

	if (nevent_catchup_debt <= 0 || nevent_catchup_remaining <= 0)
		return;

	nevent_catchup_quota =
		(nevent_catchup_debt + nevent_catchup_remaining - 1) / nevent_catchup_remaining;
	nevent_catchup_extra_callbacks = MIN(nevent_catchup_quota, max_extra_callbacks);
	callback_budget = nevent_catchup_extra_callbacks * MAX(1L, nevent_avg_callback_us);
	nevent_catchup_extension_us = MIN(max_extension_usec, callback_budget);
	*effective_budget_usec += nevent_catchup_extension_us;
	if (base_max_callbacks > 0)
		*effective_max_callbacks += nevent_catchup_extra_callbacks;
}

static void nevent_finish_catchup_pulse(void)
{
	if (nevent_catchup_debt <= 0)
	{
		nevent_catchup_debt = 0;
		nevent_catchup_remaining = 0;
		return;
	}
	if (nevent_catchup_remaining > 0)
		nevent_catchup_remaining--;
	if (nevent_catchup_remaining <= 0)
		nevent_catchup_remaining = NEVENT_CATCHUP_WINDOW_PULSES;
}

static bool nevent_trace_player(void)
{
	return nevent_config_limit("DURIS_NEVENT_TRACE_PLAYER", 0) > 0;
}

static bool nevent_analytics_enabled(void)
{
	static long enabled = -1;
	if (enabled < 0)
		enabled = nevent_config_limit("DURIS_NEVENT_ANALYTICS", 0) > 0;
	return enabled > 0;
}

static void nevent_analytics_reset(unsigned long long start_tick)
{
	memset(&nevent_analytics, 0, sizeof(nevent_analytics));
	nevent_analytics.window_start_tick = start_tick;
}

static struct nevent_callback_analytics *nevent_analytics_callback_slot(void *func,
									const char *name)
{
	int free_slot = -1;
	int i;

	if (!func)
		return NULL;
	for (i = 0; i < NEVENT_ANALYTICS_CALLBACK_SLOTS; i++)
	{
		if (nevent_analytics.callbacks[i].func == func)
		{
			if (name && !nevent_analytics.callbacks[i].name[0])
				snprintf(nevent_analytics.callbacks[i].name,
					 sizeof(nevent_analytics.callbacks[i].name), "%s", name);
			return &nevent_analytics.callbacks[i];
		}
		if ((free_slot < 0) && !nevent_analytics.callbacks[i].func)
			free_slot = i;
	}
	if (free_slot < 0)
	{
		nevent_analytics.callback_overflow++;
		return NULL;
	}
	nevent_analytics.callbacks[free_slot].func = func;
	if (name)
		snprintf(nevent_analytics.callbacks[free_slot].name,
			 sizeof(nevent_analytics.callbacks[free_slot].name), "%s", name);
	return &nevent_analytics.callbacks[free_slot];
}

static void nevent_analytics_record_callback(event_func_type func, const char *name,
					     long callback_us)
{
	struct nevent_callback_analytics *callback;

	if (!nevent_analytics_enabled())
		return;
	callback = nevent_analytics_callback_slot((void *)func, name);
	if (!callback)
		return;
	callback->calls++;
	callback->total_us += callback_us;
	if (callback_us > callback->max_us)
		callback->max_us = callback_us;
}

static void nevent_analytics_record_deferred(P_nevent event)
{
	struct nevent_callback_analytics *callback;

	if (!nevent_analytics_enabled() || !event || !event->func)
		return;
	callback = nevent_analytics_callback_slot((void *)event->func, NULL);
	if (callback)
		callback->deferred++;
}

static void nevent_analytics_emit_callbacks(void)
{
	int i;

	for (i = 0; i < NEVENT_ANALYTICS_CALLBACK_SLOTS; i++)
	{
		struct nevent_callback_analytics *callback = &nevent_analytics.callbacks[i];
		const char *callback_name;
		if (!callback->func)
			continue;
		callback_name = callback->name[0] ? callback->name :
						    get_function_name(callback->func);
		logit(LOG_STATUS,
		      "NEVENT ANALYTICS CALLBACK: window_start_tick=%llu func=%p name=%s calls=%lld total_us=%lld avg_us=%.2f max_us=%ld deferred=%lld",
		      nevent_analytics.window_start_tick, callback->func,
		      callback_name ? callback_name : "unknown", callback->calls,
		      callback->total_us,
		      callback->calls ? (double)callback->total_us / (double)callback->calls : 0.0,
		      callback->max_us, callback->deferred);
	}
	if (nevent_analytics.callback_overflow > 0)
		logit(LOG_STATUS,
		      "NEVENT ANALYTICS CALLBACK OVERFLOW: window_start_tick=%llu dropped=%ld slots=%d",
		      nevent_analytics.window_start_tick, nevent_analytics.callback_overflow,
		      NEVENT_ANALYTICS_CALLBACK_SLOTS);
}

static void nevent_analytics_record(long scanned, long executed, long deferred,
				    long catchup_executed, long max_deferral_seen,
				    long max_late_ticks, const char *max_late_name,
				    unsigned long long max_late_scheduled, long max_late_deferral,
				    long loop_us, bool budget_exhausted)
{
	if (!nevent_analytics_enabled())
		return;

	if (nevent_analytics.pulses == 0)
		nevent_analytics.window_start_tick = ne_event_tick;

	nevent_analytics.pulses++;
	nevent_analytics.total_scanned += scanned;
	nevent_analytics.total_executed += executed;
	nevent_analytics.total_deferred += deferred;
	nevent_analytics.total_us += loop_us;
	if (budget_exhausted)
		nevent_analytics.budget_exhausted_pulses++;

	if (scanned > nevent_analytics.peak_scanned)
		nevent_analytics.peak_scanned = scanned;
	if (executed > nevent_analytics.peak_executed)
	{
		nevent_analytics.peak_executed = executed;
		nevent_analytics.peak_executed_tick = ne_event_tick;
	}
	if (deferred > nevent_analytics.peak_deferred)
		nevent_analytics.peak_deferred = deferred;
	if (loop_us > nevent_analytics.peak_total_us)
	{
		nevent_analytics.peak_total_us = loop_us;
		nevent_analytics.peak_total_us_tick = ne_event_tick;
	}
	if (ne_event_counter > nevent_analytics.peak_pending)
		nevent_analytics.peak_pending = ne_event_counter;

	logit(LOG_STATUS,
	      "NEVENT ANALYTICS PULSE: tick=%llu scanned=%ld executed=%ld deferred=%ld pending=%ld budget_exhausted=%d catchup_debt=%ld catchup_quota=%ld catchup_executed=%ld max_deferral=%ld max_late_ticks=%ld max_late_name=%s max_late_scheduled=%llu max_late_deferral=%ld catchup_extension_us=%ld avg_callback_us=%ld total_us=%ld",
	      ne_event_tick, scanned, executed, deferred, ne_event_counter,
	      budget_exhausted ? 1 : 0, nevent_catchup_debt, nevent_catchup_quota, catchup_executed,
	      max_deferral_seen, max_late_ticks, max_late_name, max_late_scheduled,
	      max_late_deferral, nevent_catchup_extension_us, nevent_avg_callback_us, loop_us);

	if (nevent_analytics.pulses >= PULSES_IN_TICK)
	{
		double pulses = (double)nevent_analytics.pulses;
		logit(LOG_STATUS,
		      "NEVENT ANALYTICS MINUTE: start_tick=%llu end_tick=%llu pulses=%ld avg_scanned=%.2f avg_executed=%.2f avg_deferred=%.2f avg_total_us=%.2f peak_scanned=%ld peak_executed=%ld peak_executed_tick=%llu peak_deferred=%ld peak_total_us=%ld peak_total_us_tick=%llu peak_pending=%ld budget_exhausted_pulses=%ld",
		      nevent_analytics.window_start_tick, ne_event_tick, nevent_analytics.pulses,
		      nevent_analytics.total_scanned / pulses,
		      nevent_analytics.total_executed / pulses,
		      nevent_analytics.total_deferred / pulses, nevent_analytics.total_us / pulses,
		      nevent_analytics.peak_scanned, nevent_analytics.peak_executed,
		      nevent_analytics.peak_executed_tick, nevent_analytics.peak_deferred,
		      nevent_analytics.peak_total_us, nevent_analytics.peak_total_us_tick,
		      nevent_analytics.peak_pending, nevent_analytics.budget_exhausted_pulses);
		nevent_analytics_emit_callbacks();
		nevent_analytics_reset(ne_event_tick + 1);
	}
}

static long nevent_elapsed_us(const struct timespec *started, const struct timespec *finished)
{
	return (finished->tv_sec - started->tv_sec) * 1000000L +
	       (finished->tv_nsec - started->tv_nsec) / 1000L;
}

/* Move the unscanned but due events to the front of the next pulse.  Leaving
 * them in the current ring bucket would delay already-due work for a full cycle.
 * Events that are not due yet stay put, but still get credited the revolution
 * the scan never gave them. */
static long nevent_defer_suffix(P_nevent deferred_head, long *new_debt)
{
	P_nevent event, next;
	P_nevent moved_head = NULL;
	P_nevent moved_tail = NULL;
	int next_pulse;
	long deferred = 0;

	if (new_debt)
		*new_debt = 0;
	if (!deferred_head)
		return 0;

	next_pulse = (pulse + 1) % PULSES_IN_TICK;

	for (event = deferred_head; event; event = next)
	{
		next = event->next_sched;

		/* Scheduled for a later ring traversal.  The scan never reached it, so
		 * decrement here; otherwise it silently loses a whole revolution. */
		if (event->timer > 1)
		{
			event->timer--;
			continue;
		}

		/* Unlink from this bucket. */
		if (event->prev_sched)
			event->prev_sched->next_sched = event->next_sched;
		else
			ne_schedule[pulse] = event->next_sched;
		if (event->next_sched)
			event->next_sched->prev_sched = event->prev_sched;
		else
			ne_schedule_tail[pulse] = event->prev_sched;

		event->prev_sched = NULL;
		event->next_sched = NULL;
		event->element = next_pulse;
		event->timer = 1;
		if (event->deferral_count == 0 && new_debt)
			(*new_debt)++;
		event->deferral_count++;
		nevent_analytics_record_deferred(event);
		deferred++;

		if (!moved_head)
		{
			moved_head = moved_tail = event;
		}
		else
		{
			moved_tail->next_sched = event;
			event->prev_sched = moved_tail;
			moved_tail = event;
		}
	}

	if (!moved_head)
		return 0;

	/* Prepend the moved run to the next pulse, preserving their order. */
	if (ne_schedule[next_pulse])
	{
		moved_tail->next_sched = ne_schedule[next_pulse];
		ne_schedule[next_pulse]->prev_sched = moved_tail;
	}
	else
	{
		ne_schedule_tail[next_pulse] = moved_tail;
	}
	ne_schedule[next_pulse] = moved_head;

	return deferred;
}

// Execute events!
void ne_events(void)
{
	static long count = 0;
	P_nevent next_event;
	struct timespec loop_started, callback_started, callback_finished, loop_finished;
	long scanned = 0, executed = 0, catchup_executed = 0, max_deferral_seen = 0;
	long max_late_ticks = 0, max_late_deferral = 0, slowest_us = 0, deferred = 0;
	long new_debt = 0;
	const char *max_late_name = "none";
	unsigned long long max_late_scheduled = 0;
	long base_budget_usec = nevent_budget_usec();
	long base_max_callbacks = nevent_max_callbacks();
	long budget_usec = base_budget_usec;
	long max_callbacks = base_max_callbacks;
	bool budget_exhausted = FALSE;
	bool priority_promotion_used = FALSE;
	const char *slowest_name = "none";

	if ((pulse < 0) || (pulse >= PULSES_IN_TICK))
	{
		panic_corruption("ne_events", "pulse (%d) out of range", pulse);
	}

	if (debug_event_list)
	{
		check_nevents();
	}

	nevent_prepare_catchup(base_budget_usec, base_max_callbacks, &budget_usec, &max_callbacks);
	clock_gettime(CLOCK_MONOTONIC, &loop_started);
	PROFILE_START(event_loop);
	for (current_nevent = ne_schedule[pulse]; current_nevent; current_nevent = next_event)
	{
		scanned++;
		next_event = current_nevent->next_sched;

		if (--(current_nevent->timer) > 0)
		{
			if (budget_usec > 0 && !(scanned % 64))
			{
				clock_gettime(CLOCK_MONOTONIC, &loop_finished);
				budget_exhausted = nevent_elapsed_us(&loop_started,
								     &loop_finished) >= budget_usec;
			}
			if (budget_exhausted && next_event && !priority_promotion_used &&
			    nevent_promote_overdue_event(&next_event, current_nevent))
			{
				priority_promotion_used = TRUE;
				continue;
			}
			if (budget_exhausted && next_event)
			{
				deferred = nevent_defer_suffix(next_event, &new_debt);
				nevent_add_catchup_debt(new_debt);
				break;
			}
			continue;
		}

		if ((long)current_nevent->deferral_count > max_deferral_seen)
			max_deferral_seen = (long)current_nevent->deferral_count;
		if (ne_event_tick > current_nevent->scheduled_tick &&
		    (long)(ne_event_tick - current_nevent->scheduled_tick) > max_late_ticks)
		{
			max_late_ticks = (long)(ne_event_tick - current_nevent->scheduled_tick);
			max_late_name = current_nevent->func ?
						nevent_callback_label(current_nevent->func) :
						"neutered";
			max_late_scheduled = current_nevent->scheduled_tick;
			max_late_deferral = (long)current_nevent->deferral_count;
		}

		// If this event has a function to execute (hasn't been neutered)
		if (current_nevent->func)
		{
			event_func_type callback_func = current_nevent->func;
			const char *callback_name = nevent_callback_label(callback_func);
			clock_gettime(CLOCK_MONOTONIC, &callback_started);
#ifdef DO_PROFILE
			PROFILE_START(event_func);
			(callback_func)(current_nevent->ch, current_nevent->victim,
					current_nevent->obj, current_nevent->data);
			PROFILE_END(event_func);
			PROFILE_REGISTER_CALL(callback_func,
					      event_func_profile_end - event_func_profile_beg)
#else
			(callback_func)(current_nevent->ch, current_nevent->victim,
					current_nevent->obj, current_nevent->data);
#endif
			clock_gettime(CLOCK_MONOTONIC, &callback_finished);
			executed++;
			long callback_us = nevent_elapsed_us(&callback_started, &callback_finished);
			nevent_record_callback_cost(callback_us);
			if (callback_us > slowest_us)
			{
				slowest_us = callback_us;
				slowest_name = callback_name;
			}
			nevent_analytics_record_callback(callback_func, callback_name, callback_us);
		}

		if (nevent_is_player_timed(current_nevent->func, current_nevent->ch) &&
		    nevent_trace_player())
		{
			long long late_pulses =
				(ne_event_tick > current_nevent->scheduled_tick) ?
					(long long)(ne_event_tick -
						    current_nevent->scheduled_tick) :
					0;
			logit(LOG_STATUS,
			      "PLAYER EVENT TIMING: func=%s sequence=%llu ch_pid=%ld due_tick=%llu actual_tick=%llu late_pulses=%lld scheduled=%ld",
			      nevent_callback_label(current_nevent->func), current_nevent->sequence,
			      current_nevent->ch ? (long)GET_ID(current_nevent->ch) : -1L,
			      current_nevent->scheduled_tick, ne_event_tick, late_pulses,
			      ne_event_counter);
		}

		if (current_nevent->deferral_count > 0)
			catchup_executed++;
		nevent_destroy(current_nevent);

		if (max_callbacks > 0 && executed >= max_callbacks)
			budget_exhausted = TRUE;
		if (budget_usec > 0)
		{
			clock_gettime(CLOCK_MONOTONIC, &loop_finished);
			if (nevent_elapsed_us(&loop_started, &loop_finished) >= budget_usec)
				budget_exhausted = TRUE;
		}
		if (budget_exhausted && next_event && !priority_promotion_used &&
		    nevent_promote_overdue_event(&next_event, NULL))
		{
			priority_promotion_used = TRUE;
			continue;
		}
		if (budget_exhausted && next_event)
		{
			deferred = nevent_defer_suffix(next_event, &new_debt);
			nevent_add_catchup_debt(new_debt);
			break;
		}
	}
	current_nevent = NULL;
	nevent_process_pending_cancellations();
	PROFILE_END(event_loop);
	clock_gettime(CLOCK_MONOTONIC, &loop_finished);
	long loop_us = nevent_elapsed_us(&loop_started, &loop_finished);
	if (deferred > 0)
	{
		logit(LOG_STATUS,
		      "NEVENT BUDGET: pulse=%d total_us=%ld scanned=%ld executed=%ld deferred=%ld catchup_debt=%ld catchup_quota=%ld catchup_executed=%ld max_deferral=%ld max_late_ticks=%ld max_late_name=%s max_late_scheduled=%llu max_late_deferral=%ld catchup_extension_us=%ld avg_callback_us=%ld slowest=%s slowest_us=%ld scheduled=%ld",
		      pulse, loop_us, scanned, executed, deferred, nevent_catchup_debt,
		      nevent_catchup_quota, catchup_executed, max_deferral_seen, max_late_ticks,
		      max_late_name, max_late_scheduled, max_late_deferral,
		      nevent_catchup_extension_us, nevent_avg_callback_us,
		      slowest_name ? slowest_name : "unknown", slowest_us, ne_event_counter);
	}
	if (nevent_catchup_quota > 0 || new_debt > 0)
	{
		logit(LOG_STATUS,
		      "NEVENT CATCHUP: pulse=%d debt=%ld remaining_pulses=%d quota=%ld executed=%ld extension_us=%ld avg_callback_us=%ld new_debt=%ld",
		      pulse, nevent_catchup_debt, nevent_catchup_remaining, nevent_catchup_quota,
		      catchup_executed, nevent_catchup_extension_us, nevent_avg_callback_us,
		      new_debt);
	}
	if (loop_us >= 50000)
	{
		logit(LOG_STATUS,
		      "NEVENT SLOW: pulse=%d total_us=%ld scanned=%ld executed=%ld slowest=%s slowest_us=%ld scheduled=%ld",
		      pulse, loop_us, scanned, executed, slowest_name ? slowest_name : "unknown",
		      slowest_us, ne_event_counter);
	}
	nevent_analytics_record(scanned, executed, deferred, catchup_executed, max_deferral_seen,
				max_late_ticks, max_late_name, max_late_scheduled,
				max_late_deferral, loop_us, budget_exhausted);
	nevent_finish_catchup_pulse();
	count++;
	ne_event_tick++;
}

// Returns the first instance of an event with func as the event function.
// Note: This is only useful if there is only one such event, since the first instance
//   may not be the next to occur.
P_nevent get_scheduled(event_func func)
{
	P_nevent pEvent;

	for (int i = 0; i < PULSES_IN_TICK; i++)
	{
		pEvent = ne_schedule[i];
		while (pEvent != NULL)
		{
			if (pEvent->func == func)
			{
				return pEvent;
			}
			pEvent = pEvent->next_sched;
		}
	}
	return NULL;
}

P_nevent get_scheduled(P_char ch, event_func func)
{
	P_nevent e;

	LOOP_EVENTS_CH(e, ch->nevents)
	{
		if (e->func == func)
		{
			return e;
		}
	}

	return NULL;
}

P_nevent get_scheduled_excluding_current(P_char ch, event_func func)
{
	P_nevent event;

	LOOP_EVENTS_CH(event, ch->nevents)
	{
		if (event != current_nevent && event->func == func)
			return event;
	}

	return NULL;
}

P_nevent get_scheduled(P_obj obj, event_func func)
{
	P_nevent e;

	LOOP_EVENTS_OBJ(e, obj->nevents)
	{
		if (e->func == func)
		{
			return e;
		}
	}

	return NULL;
}

P_nevent get_next_scheduled_char(P_nevent e, event_func func)
{
	if (!e)
	{
		return NULL;
	}
	// Start with the next event in ch's list and look for func.
	LOOP_EVENTS_CH(e, e->next_char_nev)
	{
		if (e->func == func)
		{
			return e;
		}
	}
	return NULL;
}

P_nevent get_next_scheduled_obj(P_nevent e, event_func func)
{
	if (!e)
	{
		return NULL;
	}
	LOOP_EVENTS_OBJ(e, e->next_obj_nev)
	{
		if (e->func == func)
		{
			return e;
		}
	}
	return NULL;
}

void ne_init_event_pool(void)
{
	pulse = 0;
	current_nevent = NULL;
	ne_event_counter = 0;
	ne_event_tick = 0;
	ne_event_sequence = 0;
	nevent_catchup_debt = 0;
	nevent_catchup_remaining = 0;
	nevent_pending_cancellations.clear();
	memset(ne_schedule, 0, sizeof(ne_schedule));
	memset(ne_schedule_tail, 0, sizeof(ne_schedule_tail));
	ne_dead_event_pool = mm_create("NEVENTS", sizeof(struct nevent_data),
				       offsetof(struct nevent_data, next_sched), 11);
}

void ne_init_events(void)
{
	int j = 0, i = 0;

	ne_init_event_pool();

	logit(LOG_STATUS, "assigning room specials events.");
	for (j = 0; j < top_of_world; j++)
		if (world[j].funct && (*world[j].funct)(j, 0, CMD_SET_PERIODIC, 0))
		{
			add_event(room_event, PULSE_MOBILE + number(-4, 4), 0, 0, 0, 0, &j,
				  sizeof(j));
		}

	/*
	 * do boot_time reset of zones and fire up their reset events, it
	 * staggers them over the entire pulse, because zone resets are
	 * heavy-duty CPU hogs so we don't want them all at once.
	 */

	logit(LOG_STATUS, "Boot time reset of all zones.");

	for (i = 0, j = 0; j <= top_of_zone_table; i++, j++)
	{
		i = i % PULSES_IN_TICK;

		logit(LOG_STATUS, "Zone %3d:(%5d-%5d) %s", j, j ? (zone_table[j - 1].top + 1) : 0,
		      zone_table[j].top, zone_table[j].name);

		// schedule zone reset events (always needed)
		if (zone_table[j].reset_mode)
		{
			add_event(event_reset_zone, i, 0, 0, 0, 0, &j, sizeof(j));
		}

		// skip zone reset during copyover/crash_recovery - mobs preserved from before
		// but still initialize lifespan so zone timers work
		if (copyover_boot || crash_recovery_boot)
		{
			// just set lifespan without spawning mobs
			// for crash recovery, zone ages will be restored from redis later
			if (!crash_recovery_boot)
			{
				if (zone_table[j].lifespan_min != zone_table[j].lifespan_max)
					zone_table[j].lifespan = number(zone_table[j].lifespan_min,
									zone_table[j].lifespan_max);
				else
					zone_table[j].lifespan = zone_table[j].lifespan_min;
				zone_table[j].age = 0;
			}
		}
		else
		{
			reset_zone(j, 2);
		}
	}

	/* special cases now */

	/* game clock */
	// This is where we set the initial hour mud-tick.
	add_event(event_another_hour, 125 * WAIT_SEC - pulse, NULL, NULL, NULL, 0, NULL, 0);
	// AddEvent(EVENT_SPECIAL, 500 - pulse, FALSE, another_hour, 0);

	/* timed house control stuff */
	// old guildhalls (deprecated)
	// add_event(event_housekeeping, 500, NULL, NULL, NULL, 0, NULL, 0);
	// AddEvent(EVENT_SPECIAL, 500 - pulse, FALSE, do_housekeeping, 0);

	/* sunrise, sunset, etc informer */
	add_event(event_astral_clock, 125 * WAIT_SEC, NULL, NULL, NULL, 0, NULL, 0);
	// AddEvent(EVENT_SPECIAL, 500, TRUE, astral_clock, NULL);

	/* sector weather */
	// Why do we have 100 events where the weather changes instead of just one?
	for (j = 0; j < 100; j++)
	{
		// We take 2 ticks before we start changing the weather.
		add_event(event_weather_change, 125 * WAIT_SEC + number(-9, 9), NULL, NULL, NULL, 0,
			  &j, sizeof(j));
		// AddEvent(EVENT_SPECIAL, 500 + number(-9, 9), TRUE, weather_change, Gbuf1);
	}

	/* Statistic logging functionality */
	// AddEvent(EVENT_SPECIAL, 60, TRUE, write_statistic, NULL);

	/* miscellaneous character looping */
	add_event(generic_char_event, 20 * WAIT_SEC, NULL, NULL, NULL, 0, NULL, 0);
	// AddEvent(EVENT_SPECIAL, 20 * 4, FALSE, generic_char_event, 0);

	// Checks to see if artifact souls are ready to merge.
	add_event(event_artifact_check_bind_sql, 15 * WAIT_SEC, NULL, NULL, NULL, 0, NULL, 0);

	// Makes artifacts fight and lose time on timers (penalty for multiple artis).
	add_event(event_artifact_wars_sql, 20 * WAIT_SEC, NULL, NULL, NULL, 0, NULL, 0);

	// Checks ALL artis rented and non for negative timers..
	add_event(event_artifact_check_poof_sql, 35 * WAIT_SEC, NULL, NULL, NULL, 0, NULL, 0);

	// Upkeep costs for outposts
	add_event(event_outposts_upkeep, SECS_PER_MUD_HOUR * WAIT_SEC, NULL, NULL, NULL, 0, NULL,
		  0);

	// Increases and notifies people if they've ranked up in feudal surname.
	add_event(event_update_surnames, 45 * WAIT_SEC, NULL, NULL, NULL, 0, NULL, 0);

	// Revisioned local player checkpoints do not depend on Redis availability.
	add_event(event_flush_dirty_players, 5 * WAIT_SEC, NULL, NULL, NULL, 0, NULL, 0);

	// redis donation message polling
	if (redis_enabled)
		add_event(event_check_donation_messages, 1 * WAIT_SEC, NULL, NULL, NULL, 0, NULL,
			  0);

	// redis world state saves for crash recovery
	if (redis_enabled && redis_world_state_enabled && !crash_recovery_boot)
		add_event(event_save_world_state, 30 * WAIT_SEC, NULL, NULL, NULL, 0, NULL, 0);

	logit(LOG_STATUS, "Done scheduling events.\n");
}

void zone_purge(int zone_number)
{
	P_char vict, next_v;
	P_obj obj, next_o;
	struct zone_data to_purge = zone_table[zone_number];
	int k;

	for (k = to_purge.real_bottom; k != NOWHERE && k <= to_purge.real_top; k++)
	{
		for (vict = world[k].people; vict; vict = next_v)
		{
			next_v = vict->next_in_room;
			if (IS_NPC(vict) && !IS_MORPH(vict))
			{
				extract_char(vict);
				vict = NULL;
			}
		}

		for (obj = world[k].contents; obj; obj = next_o)
		{
			next_o = obj->next_content;

			if (obj->R_num == real_object(VOBJ_WALLS))
				continue;

			if (obj->type == ITEM_CORPSE &&
			    !obj->contains) // Don't purge corpses w/ contents
			{
			}
			// Don't purge artis either.
			else if (!IS_ARTIFACT(obj))
			{
				extract_obj(obj, TRUE); // Does not include artis.
				obj = NULL;
			}
		}
	}
}

// This function is very CPU intensive.  Do _NOT_ leave it toggled on if you're not having issues.
void check_nevents()
{
	P_char ch;
	bool shown = FALSE;
	P_nevent e1, e2;

	// For each event in the game,
	for (int i = 0; i < PULSES_IN_TICK; i++)
	{
		for (e1 = ne_schedule[i]; e1; e1 = e1->next_sched)
		{
			// If the event has a ch,
			if ((ch = e1->ch))
			{
				// If the head of the list isn't equal to the second.
				if (ch->nevents && ch->nevents->ch && ch->nevents->ch != ch)
				{
					debug("check_nevents: ch '%s' %d not ch in ch->nevents, func %s.",
					      IS_ALIVE(ch) ? J_NAME(ch) : GET_NAME(ch), GET_ID(ch),
					      get_function_name((void *)ch->nevents->func));
					shown = TRUE;
					ch->nevents = NULL;
					continue;
				}
				// Make sure all of ch's events belong to ch.
				LOOP_EVENTS_CH(e2, ch->nevents)
				{
					if (e2->next_char_nev && e2->next_char_nev->ch &&
					    e2->next_char_nev->ch != ch)
					{
						debug("check_nevents: ch '%s' %d is not ch in sub-event e->next_char_nev, func %s.",
						      IS_ALIVE(ch) ? J_NAME(ch) : GET_NAME(ch),
						      GET_ID(ch),
						      get_function_name(
							      (void *)e2->next_char_nev->func));
						shown = TRUE;
						e2->next_char_nev = NULL;
						break;
					}
				}
			}
		}
	}
	if (shown)
		debug("%ld is current time.", time(NULL));
}

void event_broken(struct char_link_data *cld)
{
	P_char ch = cld->linking;
	P_nevent e;

	if (!ch)
	{
		return;
	}

	LOOP_EVENTS_CH(e, ch->nevents)
	{
		if (e->cld == cld)
		{
			nevent_handle handle = nevent_handle_from_event(e);
			e->cld = NULL;
			e->victim = NULL;
			if (e->lifecycle_state == NEVENT_LIFECYCLE_ACTIVE)
				nevent_cancel(handle);
			return;
		}
	}
	if (debug_event_list)
	{
		debug("event_broken: couldn't find cld in cld->ch's event list, ch: '%s' %d",
		      J_NAME(ch), GET_ID(ch));
	}
}

void *get_executable_base_address()
{
	FILE *f;
	char line[256];
	void *base_address = NULL;
	// Open the maps file for the current process
	f = fopen("/proc/self/maps", "r");
	if (f == NULL)
	{
		perror("fopen");
		return NULL;
	}

	// Read the first line, which usually describes the main executable's text segment
	if (fgets(line, sizeof(line), f) != NULL)
	{
		// The address range is at the beginning of the line, e.g., "55b1f9f23000-55b1f9f24000 r-xp..."
		char *dash_pos = strchr(line, '-');
		if (dash_pos != NULL)
		{
			*dash_pos = '\0'; // Temporarily terminate the string at the dash
			base_address = (void *)strtoul(line, NULL, 16);
		}
	}

	fclose(f);
	return base_address;
}

const char *get_function_name(void *func)
{
	const char *name;

	if (func == NULL)
	{
		return "NULL";
	}

	name = event_name_registry_lookup(func);
	if (name)
		return name;

	// Shouldn't this event be in the function_names array?
	if ((void *)event_autosave == func)
		return "event_autosave";

	return "unknown function";
}

// file FUNCTION_NAMES_FILE should be created with
// nm --demangle dms | grep " T " before each boot,
// preferably with a mainboot script
void load_event_names()
{
	void *base_address = get_executable_base_address();
	event_name_load_result result = event_name_registry_load(
		FUNCTION_NAMES_FILE, reinterpret_cast<uintptr_t>(base_address));

	if (!result.opened)
	{
		logit(LOG_SYS, "load_event_names: could not open %s: %s", FUNCTION_NAMES_FILE,
		      strerror(result.error_number));
		return;
	}
	if (result.read_error)
	{
		logit(LOG_SYS, "load_event_names: read failed for %s: %s", FUNCTION_NAMES_FILE,
		      strerror(result.error_number));
		return;
	}
	if (result.malformed_lines > 0)
	{
		logit(LOG_SYS, "load_event_names: ignored %zu malformed record(s) in %s",
		      result.malformed_lines, FUNCTION_NAMES_FILE);
	}
	logit(LOG_STATUS, "Loaded %zu event callback names (%zu duplicate addresses).",
	      result.symbols_loaded, result.duplicate_addresses);
}

void show_world_events(P_char ch, const char *arg)
{
	int count = 0;
	char buf[MAX_STRING_LENGTH];
	if (!arg || arg[0] == '\0')
	{
		for (int i = 0; i < PULSES_IN_TICK; i++)
			if (ne_schedule[i])
			{
				for (P_nevent ev = ne_schedule[i]; ev; ev = ev->next_sched)
				{
					count++;
				}
			}
		snprintf(
			buf, MAX_STRING_LENGTH,
			"There are currently %d events scheduled on the system.\nSpecify a function name to see more information about that particular event.\n",
			count);
		send_to_char(buf, ch);
		return;
	}

	snprintf(buf, MAX_STRING_LENGTH, "Event function: %s\n\n", arg);
	strcat(buf, "    pulse | timer |  char name   |  vict name   | obj vnum |  data ptr  \n");
	strcat(buf, "   -------|-------|--------------|--------------|----------|------------\n");
	for (int i = 0; i < PULSES_IN_TICK; i++)
		if (ne_schedule[i])
		{
			for (P_nevent ev = ne_schedule[i]; ev; ev = ev->next_sched)
			{
				if (strcmp(get_function_name((void *)ev->func), arg))
					continue;
				char line[128];

				count++;
				/* Format one row into its own bounded buffer and
				   append it, rather than writing at buf + strlen(buf)
				   with a size the compiler cannot narrow. */
				checked_snprintf(
					line, sizeof line,
					"    %-5d | %-5d | %-12.12s | %-12.12s | %-8d | %p\n",
					ev->element, ev->timer,
					ev->ch ? GET_NAME(ev->ch) : "   none",
					ev->victim ? GET_NAME(ev->victim) : "   none",
					ev->obj ? OBJ_VNUM(ev->obj) : 0, ev->data);

				if (strlen(buf) + sizeof line >= sizeof buf)
				{
					i = PULSES_IN_TICK;
					break;
				}
				strlcat(buf, line, sizeof buf);
			}
		}
	if (!count)
		strcat(buf, "\n   No events matched that function name.\n");

	page_string(ch->desc, buf, 1);
}

#ifdef DO_PROFILE

PROFILES(DEFINE);
bool do_profile = FALSE;

void save_profile_data(const char *name, double total_inside, double total_outside, unsigned total)
{
	logit(LOG_FILE,
	      "Profile info for \"%s\": inside = %.0f, outside = %.0f, total_calls = %d, average = %.0f, share = %6.3f%%",
	      name, total_inside, total_outside, total,
	      (total != 0) ? (total_inside / (double)total) : 0,
	      (total_inside + total_outside != 0) ?
		      (total_inside / (total_inside + total_outside) * 100.0) :
		      0);
	statuslog(
		56,
		"Profile info for \"%s\": inside = %.0f, outside = %.0f, total_calls = %d, average = %.0f, share = %6.3f%%",
		name, total_inside, total_outside, total,
		(total != 0) ? (total_inside / (double)total) : 0,
		(total_inside + total_outside != 0) ?
			(total_inside / (total_inside + total_outside) * 100.0) :
			0);
}

struct FuncCallInfo
{
	const char *name;
	const void *addr;
	unsigned calls;
	double time;
	FuncCallInfo *next;
	FuncCallInfo *prev;
};

static std::vector<FuncCallInfo> func_call_info;

static void add_func_call_info(const void *func, const char *name, void *context)
{
	auto *entries = static_cast<std::vector<FuncCallInfo> *>(context);
	entries->push_back({ name, func, 0, 0, nullptr, nullptr });
}

void reset_func_call_info()
{
	if (func_call_info.empty())
		return;
	FuncCallInfo *curr = func_call_info.data();
	do
	{
		curr->calls = 0;
		curr->time = 0;
		curr = curr->next;
	} while (curr != func_call_info.data());
}

void init_func_call_info()
{
	func_call_info.clear();
	func_call_info.reserve(event_name_registry_size() + 1);
	func_call_info.push_back({ "Zero or unknown", nullptr, 0, 0, nullptr, nullptr });
	event_name_registry_visit(add_func_call_info, &func_call_info);

	for (size_t i = 0; i < func_call_info.size(); i++)
	{
		const size_t next = (i + 1) % func_call_info.size();
		const size_t prev = (i + func_call_info.size() - 1) % func_call_info.size();
		func_call_info[i].next = &func_call_info[next];
		func_call_info[i].prev = &func_call_info[prev];
	}
	reset_func_call_info();
}

void save_func_call_info()
{
	unsigned total_calls = 0;
	double total_time = 0;

	if (func_call_info.empty())
		return;
	FuncCallInfo *curr = func_call_info.data();
	do
	{
		total_calls += curr->calls;
		total_time += curr->time;
		curr = curr->next;
	} while (curr != func_call_info.data());

	curr = func_call_info.data();
	do
	{
		logit(LOG_FILE,
		      "Profile info for function \"%-30s\": total calls = %9d (%7.3f%%)  total time = %9.0f (%7.3f%%)",
		      curr->name, curr->calls,
		      (total_calls != 0) ? ((double)curr->calls / (double)total_calls * 100.0) : 0,
		      curr->time / 1000.,
		      (total_time != 0) ? (curr->time / total_time * 100.0) : 0);
		statuslog(
			56,
			"Profile info for function \"%-30s\": total calls = %9d (%7.3f%%)  total time = %9.0f (%7.3f%%)",
			curr->name, curr->calls,
			(total_calls != 0) ? ((double)curr->calls / (double)total_calls * 100.0) :
					     0,
			curr->time / 1000.,
			(total_time != 0) ? (curr->time / total_time * 100.0) : 0);
		curr = curr->next;
	} while (curr != func_call_info.data() && curr->calls != 0);

	logit(LOG_FILE,
	      "Profile info for function \"%-30s\": total calls = %9d (%7.3f%%)  total time = %9.0f (%7.3f%%)",
	      "TOTAL", total_calls, 100.0, total_time / 1000., 100.0);
}

void register_func_call(void *func, double time)
{
	if (func_call_info.empty())
		return;
	FuncCallInfo *unknown = func_call_info.data();
	for (FuncCallInfo *curr = unknown->next; curr != unknown; curr = curr->next)
	{
		if (curr->addr == func)
		{
			double wallClockInSec = time / (double)CLOCKS_PER_SEC;
			if (wallClockInSec > 0.05)
			{
				statuslog(56, "LONG EVENT \"%-30s\": took %f seconds", curr->name,
					  wallClockInSec);
			}
			curr->calls++;
			curr->time += time;
			FuncCallInfo *prev = curr->prev;
			if (prev != unknown && curr->calls > prev->calls)
			{
				while (prev != unknown && prev->calls < curr->calls)
					prev = prev->prev;
				curr->next->prev = curr->prev;
				curr->prev->next = curr->next;
				curr->next = prev->next;
				curr->prev = prev;
				prev->next = curr;
				curr->next->prev = curr;
			}
			return;
		}
	}
	unknown->calls++;
	unknown->time += time;
}

#endif
