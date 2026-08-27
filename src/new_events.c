
/*
 * ***************************************************************************
 * *  File: events.c                                           Part of Duris *
 * *  Usage: manipulate various event lists
 * * *  Copyright  1994, 1995 - John Bashaw and Duris Systems Ltd.
 * *
 * ***************************************************************************
 */

#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <map>
#include <new>
#include <thread>
#include <unordered_set>
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
#define NEVENT_CONFIG_MAX_BUDGET_USEC 1000000L
#define NEVENT_CONFIG_MAX_CALLBACKS 1000000L
#define NEVENT_UNLIMITED 0L
#define NEVENT_PRIORITY_NORMAL 0U
#define NEVENT_PRIORITY_PLAYER 1U
#define NEVENT_PRIORITY_AGED_NORMAL 2U
#define NEVENT_LIFECYCLE_ACTIVE 1U
#define NEVENT_LIFECYCLE_CANCEL_PENDING 2U
#define NEVENT_LIFECYCLE_DESTROYING 3U
#define NEVENT_LIFECYCLE_RELEASED 4U
#define NEVENT_NORMAL_AGING_DEFERRALS 2U
#define NEVENT_NORMAL_AGING_TICKS 2ULL
#define NEVENT_CATCHUP_WINDOW_PULSES 4
#define NEVENT_CATCHUP_MAX_EXTENSION_USEC_DEFAULT 5000L
#define NEVENT_CATCHUP_MAX_EXTRA_CALLBACKS_DEFAULT 4000L
#define NEVENT_CALLBACK_EWMA_SHIFT 4
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
	long callback_allocation_failures;
	long long lateness_on_time;
	long long lateness_one_tick;
	long long lateness_two_to_three;
	long long lateness_four_to_fifteen;
	long long lateness_sixteen_plus;
	std::map<void *, struct nevent_callback_analytics> callbacks;
};

static struct nevent_analytics_data nevent_analytics;
static long nevent_catchup_debt = 0;
static int nevent_catchup_remaining = 0;
static long nevent_avg_callback_us = 50;
static long nevent_catchup_quota = 0;
static long nevent_catchup_extension_us = 0;
static long nevent_catchup_extra_callbacks = 0;
static unsigned long long nevent_catchup_debt_estimated_us = 0;
static std::map<unsigned long long, long> nevent_deferred_due_counts;
static std::vector<nevent_handle> nevent_pending_cancellations;
static std::thread::id nevent_game_thread;
static bool nevent_game_thread_bound = false;
static long nevent_last_pulse_total_us = 0;

static void nevent_register_deferred(P_nevent event);
static void nevent_complete_deferred(P_nevent event);

void nevent_bind_game_thread()
{
	const std::thread::id caller = std::this_thread::get_id();

	if (!nevent_game_thread_bound)
	{
		nevent_game_thread = caller;
		nevent_game_thread_bound = true;
		return;
	}
	if (nevent_game_thread != caller)
		panic_corruption("nevent",
				 "attempted to rebind scheduler ownership from another thread");
}

bool nevent_is_game_thread()
{
	return nevent_game_thread_bound && nevent_game_thread == std::this_thread::get_id();
}

bool nevent_require_game_thread(const char *operation)
{
	if (nevent_is_game_thread())
		return true;
#ifndef NDEBUG
	panic_corruption("nevent", "%s called outside the bound game thread", operation);
#else
	logit(LOG_EXIT, "nevent: %s rejected outside the bound game thread", operation);
#endif
	return false;
}

static void nevent_destroy_raw_payload(void *data)
{
	FREE(data);
}

/*
 * this code was majorly redone by Tharkun, look for original events description
 * in events.c file
 */
struct mm_ds *ne_dead_event_pool = NULL;

static void nevent_assert_pool_accounting(const char *operation)
{
	if (!ne_dead_event_pool || ne_event_counter < 0 ||
	    static_cast<size_t>(ne_event_counter) != ne_dead_event_pool->objs_used)
		panic_corruption("nevent", "%s left counter=%ld but pool usage=%zu", operation,
				 ne_event_counter,
				 ne_dead_event_pool ? ne_dead_event_pool->objs_used : 0);
}

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
/* False before the event pass; true during and after it for the current tick. */
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
static long nevent_config_limit(const char *name, long fallback, long maximum);

static unsigned int nevent_bucket_for_tick(unsigned long long tick)
{
	return static_cast<unsigned int>(tick % PULSES_IN_TICK);
}

static unsigned long long nevent_add_ticks(unsigned long long tick, unsigned long long delay)
{
	if (delay > ULLONG_MAX - tick)
		return ULLONG_MAX;
	return tick + delay;
}

static unsigned long long nevent_first_eligible_tick()
{
	return nevent_add_ticks(ne_event_tick, after_events_call ? 1ULL : 0ULL);
}

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
	if (!ch)
		return FALSE;
	if (func == event_spellcast || func == event_memorize || func == event_balance_affects)
		return IS_PC(ch);
	/* event_wait clears the player's command gate set by CharWait().  If it
	 * misses its deadline, the player remains unable to issue commands after
	 * the visible action (cast/flee/combat action) has completed. */
	if (func == event_wait)
		return IS_PC(ch) || (IS_NPC(ch) && GET_MASTER(ch) &&
				     IS_AFFECTED5(GET_MASTER(ch), AFF5_ORDERING));
	return IS_PC(ch) && (func == event_mana_regen || func == event_move_regen ||
			     func == event_hit_regen || func == event_ward_regen);
}

static unsigned int nevent_priority(event_func_type func, P_char ch)
{
	static long player_priority = -1;
	if (player_priority < 0)
		player_priority = nevent_config_limit("DURIS_NEVENT_PLAYER_PRIORITY", 1, 1);
	if (player_priority > 0 && nevent_is_player_timed(func, ch))
		return NEVENT_PRIORITY_PLAYER;
	return NEVENT_PRIORITY_NORMAL;
}

static unsigned int nevent_effective_priority(P_nevent event)
{
	if (event->priority == NEVENT_PRIORITY_NORMAL &&
	    (event->deferral_count >= NEVENT_NORMAL_AGING_DEFERRALS ||
	     (ne_event_tick > event->due_tick &&
	      ne_event_tick - event->due_tick >= NEVENT_NORMAL_AGING_TICKS)))
		return NEVENT_PRIORITY_AGED_NORMAL;
	return event->priority;
}

static bool nevent_sorts_before(P_nevent left, P_nevent right)
{
	const unsigned int left_priority = nevent_effective_priority(left);
	const unsigned int right_priority = nevent_effective_priority(right);

	if (left->due_tick != right->due_tick)
		return left->due_tick < right->due_tick;
	if (left_priority != right_priority)
		return left_priority > right_priority;
	return left->sequence < right->sequence;
}

static void nevent_link_schedule(P_nevent event, int loc)
{
	P_nevent cursor;

	for (cursor = ne_schedule[loc]; cursor && !nevent_sorts_before(event, cursor);
	     cursor = cursor->next_sched)
		;

	if (!cursor)
	{
		event->prev_sched = ne_schedule_tail[loc];
		event->next_sched = NULL;
		if (ne_schedule_tail[loc])
			ne_schedule_tail[loc]->next_sched = event;
		else
			ne_schedule[loc] = event;
		ne_schedule_tail[loc] = event;
		return;
	}

	event->prev_sched = cursor->prev_sched;
	event->next_sched = cursor;
	if (cursor->prev_sched)
		cursor->prev_sched->next_sched = event;
	else
		ne_schedule[loc] = event;
	cursor->prev_sched = event;
}

static void nevent_detach_character(P_nevent event)
{
	P_char ch = event->ch;

	if (!ch)
		return;
	if (event->prev_char_nev)
	{
		if (event->prev_char_nev->next_char_nev != event)
			panic_corruption("nevent",
					 "event sequence %llu has a broken character previous link",
					 event->sequence);
		event->prev_char_nev->next_char_nev = event->next_char_nev;
	}
	else
	{
		if (ch->nevents != event)
			panic_corruption("nevent",
					 "event sequence %llu is not its character-list head",
					 event->sequence);
		ch->nevents = event->next_char_nev;
	}
	if (event->next_char_nev)
	{
		if (event->next_char_nev->prev_char_nev != event)
			panic_corruption("nevent",
					 "event sequence %llu has a broken character next link",
					 event->sequence);
		event->next_char_nev->prev_char_nev = event->prev_char_nev;
	}
	else
	{
		if (ch->nevents_tail != event)
			panic_corruption("nevent",
					 "event sequence %llu is not its character-list tail",
					 event->sequence);
		ch->nevents_tail = event->prev_char_nev;
	}
	event->ch = NULL;
	event->prev_char_nev = NULL;
	event->next_char_nev = NULL;
}

static void nevent_detach_object(P_nevent event)
{
	P_obj obj = event->obj;

	if (!obj)
		return;
	if (event->prev_obj_nev)
	{
		if (event->prev_obj_nev->next_obj_nev != event)
			panic_corruption("nevent",
					 "event sequence %llu has a broken object previous link",
					 event->sequence);
		event->prev_obj_nev->next_obj_nev = event->next_obj_nev;
	}
	else
	{
		if (obj->nevents != event)
			panic_corruption("nevent",
					 "event sequence %llu is not its object-list head",
					 event->sequence);
		obj->nevents = event->next_obj_nev;
	}
	if (event->next_obj_nev)
	{
		if (event->next_obj_nev->prev_obj_nev != event)
			panic_corruption("nevent",
					 "event sequence %llu has a broken object next link",
					 event->sequence);
		event->next_obj_nev->prev_obj_nev = event->prev_obj_nev;
	}
	else
	{
		if (obj->nevents_tail != event)
			panic_corruption("nevent",
					 "event sequence %llu is not its object-list tail",
					 event->sequence);
		obj->nevents_tail = event->prev_obj_nev;
	}
	event->obj = NULL;
	event->prev_obj_nev = NULL;
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
	event->lifecycle_state = NEVENT_LIFECYCLE_RELEASED;
	mm_release(ne_dead_event_pool, event);
	nevent_assert_pool_accounting("nevent_destroy");
	return TRUE;
}

nevent_handle nevent_handle_from_event(P_nevent event)
{
	if (!nevent_require_game_thread("nevent_handle_from_event"))
		return { NULL, 0 };
	return { event, event ? event->sequence : 0 };
}

nevent_cancel_result nevent_cancel(nevent_handle handle)
{
	P_nevent event = handle.event;

	if (!nevent_require_game_thread("nevent_cancel"))
		return nevent_cancel_result::wrong_thread;
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

bool nevent_reschedule_at(nevent_handle handle, unsigned long long due_tick)
{
	P_nevent event = handle.event;

	if (!nevent_require_game_thread("nevent_reschedule_at"))
		return FALSE;
	const unsigned long long first_eligible_tick = nevent_first_eligible_tick();
	if (!event || handle.sequence == 0 || event->sequence != handle.sequence ||
	    event->lifecycle_state != NEVENT_LIFECYCLE_ACTIVE || current_nevent)
		return FALSE;

	if (due_tick < first_eligible_tick)
		due_tick = first_eligible_tick;

	nevent_unlink_schedule(event);
	if (event->deferral_count > 0)
	{
		nevent_complete_deferred(event);
		event->deferral_count = 0;
	}
	event->due_tick = due_tick;
	event->element = nevent_bucket_for_tick(due_tick);
	nevent_link_schedule(event, static_cast<int>(event->element));
	return TRUE;
}

bool nevent_reschedule_after(nevent_handle handle, unsigned long long delay)
{
	if (!nevent_require_game_thread("nevent_reschedule_after"))
		return FALSE;
	return nevent_reschedule_at(handle, nevent_add_ticks(ne_event_tick, delay));
}

bool nevent_advance_by(nevent_handle handle, unsigned long long ticks)
{
	P_nevent event = handle.event;

	if (!nevent_require_game_thread("nevent_advance_by"))
		return FALSE;
	if (!event || handle.sequence == 0 || event->sequence != handle.sequence)
		return FALSE;
	return nevent_reschedule_at(handle, ticks >= event->due_tick ? 0 : event->due_tick - ticks);
}

// Returns true iff all the events in ch->nevents belong to ch.
bool check_ch_nevents(P_char ch)
{
	P_nevent e, previous = NULL;

	if (!nevent_require_game_thread("check_ch_nevents"))
		return FALSE;
	if (!IS_ALIVE(ch))
	{
		return TRUE;
	}

	LOOP_EVENTS_CH(e, ch->nevents)
	{
		if ((e->ch != NULL && e->ch != ch) || e->prev_char_nev != previous)
		{
			return FALSE;
		}
		previous = e;
	}
	return ch->nevents_tail == previous;
}

// Returns true iff all the events in obj->nevents belong to obj.
bool check_obj_nevents(P_obj obj)
{
	P_nevent e, previous = NULL;

	if (!nevent_require_game_thread("check_obj_nevents"))
		return FALSE;
	if (obj == NULL)
	{
		return TRUE;
	}

	LOOP_EVENTS_OBJ(e, obj->nevents)
	{
		if (e->obj != obj || e->prev_obj_nev != previous)
		{
			return FALSE;
		}
		previous = e;
	}
	return obj->nevents_tail == previous;
}

// Compatibility wrappers now use the scheduler-owned cancellation lifecycle.
void disarm_single_event(P_nevent event)
{
	if (!nevent_require_game_thread("disarm_single_event"))
		return;
	nevent_cancel(nevent_handle_from_event(event));
}

void disarm_char_nevents(P_char ch, event_func_type func)
{
	P_nevent event, next;

	if (!nevent_require_game_thread("disarm_char_nevents"))
		return;
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

	if (!nevent_require_game_thread("disarm_obj_nevents"))
		return;
	if (!obj)
		return;
	for (event = obj->nevents; event; event = next)
	{
		next = event->next_obj_nev;
		if (!func || event->func == func)
			nevent_cancel(nevent_handle_from_event(event));
	}
}

static nevent_schedule_result nevent_schedule_failure(nevent_schedule_status status)
{
	return { status, { NULL, 0 } };
}

static nevent_schedule_result add_event_internal(event_func func, int delay, P_char ch,
						 P_char victim, P_obj obj, const void *data,
						 int data_size, void *owned_payload,
						 nevent_payload_destroy_type owned_payload_destroy)
{
	P_nevent event;
	char *data_buf;
	int loc;
	auto discard_owned_payload = [&]()
	{
		if (owned_payload && owned_payload_destroy)
			owned_payload_destroy(owned_payload);
	};
	if (!nevent_require_game_thread("add_event"))
	{
		discard_owned_payload();
		return nevent_schedule_failure(nevent_schedule_status::wrong_thread);
	}

	if (!func)
	{
		debug("add_event: No function!");
		discard_owned_payload();
		return nevent_schedule_failure(nevent_schedule_status::null_callback);
	}

	if (delay < 0)
	{
		debug("add_event: Delay (%d) les than zero?!", delay);
		discard_owned_payload();
		return nevent_schedule_failure(nevent_schedule_status::negative_delay);
	}

	if (ch && !IS_ALIVE(ch) && func != release_mob_mem)
	{
		logit(LOG_DEBUG, "add_event: dead ch '%s' in room r%d/v%d function %s",
		      GET_NAME(ch), ch->in_room, ROOM_VNUM(ch->in_room),
		      get_function_name((void *)func));
		debug("add_event: dead ch '%s' in room r%d/v%d function %s", GET_NAME(ch),
		      ch->in_room, ROOM_VNUM(ch->in_room), get_function_name((void *)func));
		discard_owned_payload();
		return nevent_schedule_failure(nevent_schedule_status::dead_owner);
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
		return nevent_schedule_failure(nevent_schedule_status::victim_without_owner);
	}
	if (data_size < 0 || (data_size > 0 && !data) || (owned_payload && !owned_payload_destroy))
	{
		debug("add_event: invalid payload for function %s",
		      get_function_name((void *)func));
		discard_owned_payload();
		return nevent_schedule_failure(nevent_schedule_status::invalid_payload);
	}
	if (ne_event_sequence == ULLONG_MAX)
	{
		logit(LOG_EXIT, "add_event: scheduler sequence space exhausted");
		discard_owned_payload();
		return nevent_schedule_failure(nevent_schedule_status::sequence_exhausted);
	}

	event = (P_nevent)mm_get(ne_dead_event_pool);

	event->prev_sched = event->next_sched = NULL;
	event->prev_char_nev = event->next_char_nev = NULL;
	event->prev_obj_nev = event->next_obj_nev = NULL;
	event->ch = ch;
	event->victim = victim;
	event->obj = obj;
	event->owner_runtime_id = ch ? ch->runtime_id : 0;
	event->victim_runtime_id = victim ? victim->runtime_id : 0;
	event->diagnostic_obj_vnum = obj ? OBJ_VNUM(obj) : 0;
	event->func = func;
	event->data = NULL;
	event->data_destroy = NULL;
	event->priority = nevent_priority(func, ch);
	event->deferral_count = 0;
	event->periodic_job_id = 0;
	event->deferred_cost_us = 0;
	event->due_tick = nevent_add_ticks(ne_event_tick, static_cast<unsigned long long>(delay));
	if (event->due_tick < nevent_first_eligible_tick())
		event->due_tick = nevent_first_eligible_tick();
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

	loc = static_cast<int>(nevent_bucket_for_tick(event->due_tick));
	event->element = loc;

	if (ch)
	{
		event->prev_char_nev = ch->nevents_tail;
		if (ch->nevents_tail)
			ch->nevents_tail->next_char_nev = event;
		else
			ch->nevents = event;
		ch->nevents_tail = event;
	}

	if (obj)
	{
		event->prev_obj_nev = obj->nevents_tail;
		if (obj->nevents_tail)
			obj->nevents_tail->next_obj_nev = event;
		else
			obj->nevents = event;
		obj->nevents_tail = event;
	}

	nevent_link_schedule(event, loc);
	ne_event_counter++;
	nevent_assert_pool_accounting("add_event");

	if (debug_event_list)
	{
		check_nevents();
	}

	return { nevent_schedule_status::scheduled, nevent_handle_from_event(event) };
}

nevent_schedule_result add_event(event_func func, int delay, P_char ch, P_char victim, P_obj obj,
				 int /*flag*/, const void *data, int data_size)
{
	return add_event_internal(func, delay, ch, victim, obj, data, data_size, NULL, NULL);
}

nevent_schedule_result add_event_owned_payload(event_func func, int delay, P_char ch, P_char victim,
					       P_obj obj, int /*flag*/, void *payload,
					       nevent_payload_destroy_type payload_destroy)
{
	return add_event_internal(func, delay, ch, victim, obj, NULL, 0, payload, payload_destroy);
}

bool nevent_handle_is_active(nevent_handle existing)
{
	if (!nevent_require_game_thread("nevent_handle_is_active"))
		return false;
	return existing.event && existing.sequence != 0 &&
	       existing.event->sequence == existing.sequence &&
	       existing.event->lifecycle_state == NEVENT_LIFECYCLE_ACTIVE;
}

static nevent_schedule_result nevent_finish_replace(nevent_handle existing,
						    nevent_schedule_result scheduled)
{
	if (!scheduled)
		return scheduled;
	const nevent_cancel_result canceled = nevent_cancel(existing);
	if (canceled == nevent_cancel_result::canceled ||
	    canceled == nevent_cancel_result::deferred)
		return scheduled;
	nevent_cancel(scheduled.handle);
	return nevent_schedule_failure(nevent_schedule_status::invalid_replace_target);
}

nevent_schedule_result nevent_replace(nevent_handle existing, event_func func, int delay, P_char ch,
				      P_char victim, P_obj obj, int flag, const void *data,
				      int data_size)
{
	if (!nevent_require_game_thread("nevent_replace"))
		return nevent_schedule_failure(nevent_schedule_status::wrong_thread);
	if (!nevent_handle_is_active(existing))
		return nevent_schedule_failure(nevent_schedule_status::invalid_replace_target);
	return nevent_finish_replace(existing, add_event(func, delay, ch, victim, obj, flag, data,
							 data_size));
}

nevent_schedule_result nevent_replace_owned_payload(nevent_handle existing, event_func func,
						    int delay, P_char ch, P_char victim, P_obj obj,
						    int flag, void *payload,
						    nevent_payload_destroy_type payload_destroy)
{
	if (!nevent_require_game_thread("nevent_replace_owned_payload"))
	{
		if (payload && payload_destroy)
			payload_destroy(payload);
		return nevent_schedule_failure(nevent_schedule_status::wrong_thread);
	}
	if (!nevent_handle_is_active(existing))
	{
		if (payload && payload_destroy)
			payload_destroy(payload);
		return nevent_schedule_failure(nevent_schedule_status::invalid_replace_target);
	}
	return nevent_finish_replace(existing,
				     add_event_owned_payload(func, delay, ch, victim, obj, flag,
							     payload, payload_destroy));
}

// Returns the time left (in pulses) in the e1 event
int ne_event_time(P_nevent e1)
{
	unsigned long long time_left;

	if (!nevent_require_game_thread("ne_event_time"))
		return 0;
	if (!e1 || e1->lifecycle_state != NEVENT_LIFECYCLE_ACTIVE || e1->due_tick <= ne_event_tick)
		return 0;
	time_left = e1->due_tick - ne_event_tick;
	return time_left > static_cast<unsigned long long>(INT_MAX) ? INT_MAX :
								      static_cast<int>(time_left);
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

static long nevent_config_limit(const char *name, long default_value, long maximum)
{
	const char *raw = getenv(name);
	char *end = NULL;
	long value;

	if (!raw || !*raw)
		return default_value;
	errno = 0;
	value = strtol(raw, &end, 10);
	if (errno == ERANGE || !end || *end != '\0' || value < 0 || value > maximum)
	{
		logit(LOG_STATUS, "Invalid %s='%s' (allowed 0..%ld); using %ld", name, raw, maximum,
		      default_value);
		return default_value;
	}
	return value;
}

static long nevent_saturating_add_long(long left, long right)
{
	if (right > LONG_MAX - left)
		return LONG_MAX;
	return left + right;
}

static unsigned long long nevent_saturating_add_ull(unsigned long long left,
						    unsigned long long right)
{
	if (right > ULLONG_MAX - left)
		return ULLONG_MAX;
	return left + right;
}

static unsigned long long nevent_ceil_div_ull(unsigned long long value, unsigned long long divisor)
{
	return value / divisor + (value % divisor != 0);
}

static long nevent_budget_usec(void)
{
	static long configured = -1;
	if (configured < 0)
		configured = nevent_config_limit("DURIS_NEVENT_BUDGET_USEC",
						 NEVENT_BUDGET_USEC_DEFAULT,
						 NEVENT_CONFIG_MAX_BUDGET_USEC);
	return configured;
}

static long nevent_max_callbacks(void)
{
	static long configured = -1;
	if (configured < 0)
		configured = nevent_config_limit("DURIS_NEVENT_MAX_CALLBACKS",
						 NEVENT_MAX_CALLBACKS_DEFAULT,
						 NEVENT_CONFIG_MAX_CALLBACKS);
	return configured;
}

static long nevent_catchup_max_extension_us(void)
{
	static long configured = -1;

	if (configured < 0)
		configured = nevent_config_limit("DURIS_NEVENT_CATCHUP_MAX_EXTENSION_USEC",
						 NEVENT_CATCHUP_MAX_EXTENSION_USEC_DEFAULT,
						 NEVENT_CONFIG_MAX_BUDGET_USEC);
	return configured;
}

static long nevent_catchup_max_extra_callbacks(void)
{
	static long configured = -1;

	if (configured < 0)
		configured = nevent_config_limit("DURIS_NEVENT_CATCHUP_MAX_EXTRA_CALLBACKS",
						 NEVENT_CATCHUP_MAX_EXTRA_CALLBACKS_DEFAULT,
						 NEVENT_CONFIG_MAX_CALLBACKS);
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

static void nevent_register_deferred(P_nevent event)
{
	if (!event || event->deferral_count > 0)
		return;
	if (nevent_catchup_debt == LONG_MAX)
		panic_corruption("nevent", "catch-up debt counter overflow");
	event->deferred_cost_us = static_cast<unsigned long long>(MAX(1L, nevent_avg_callback_us));
	nevent_catchup_debt++;
	nevent_catchup_debt_estimated_us = nevent_saturating_add_ull(
		nevent_catchup_debt_estimated_us, event->deferred_cost_us);
	nevent_deferred_due_counts[event->due_tick]++;
	if (nevent_catchup_remaining <= 0)
		nevent_catchup_remaining = NEVENT_CATCHUP_WINDOW_PULSES;
}

static void nevent_complete_deferred(P_nevent event)
{
	std::map<unsigned long long, long>::iterator due;

	if (!event || event->deferral_count == 0)
		return;
	if (nevent_catchup_debt <= 0)
		panic_corruption("nevent", "deferred event sequence %llu has no catch-up debt",
				 event->sequence);
	due = nevent_deferred_due_counts.find(event->due_tick);
	if (due == nevent_deferred_due_counts.end() || due->second <= 0)
		panic_corruption("nevent", "deferred event sequence %llu has no due-tick debt",
				 event->sequence);
	if (--due->second == 0)
		nevent_deferred_due_counts.erase(due);
	nevent_catchup_debt--;
	if (event->deferred_cost_us > nevent_catchup_debt_estimated_us)
		panic_corruption("nevent", "deferred event sequence %llu has invalid cost debt",
				 event->sequence);
	nevent_catchup_debt_estimated_us -= event->deferred_cost_us;
	event->deferred_cost_us = 0;
}

static unsigned long long nevent_oldest_deferred_due_tick()
{
	return nevent_deferred_due_counts.empty() ? 0 : nevent_deferred_due_counts.begin()->first;
}

static void nevent_prepare_catchup(long base_budget_usec, long base_max_callbacks,
				   long *effective_budget_usec, long *effective_max_callbacks)
{
	long max_extra_callbacks = nevent_catchup_max_extra_callbacks();
	long max_extension_usec = nevent_catchup_max_extension_us();
	unsigned long long callback_quota;
	unsigned long long cost_quota;

	nevent_catchup_quota = 0;
	nevent_catchup_extra_callbacks = 0;
	nevent_catchup_extension_us = 0;
	*effective_budget_usec = base_budget_usec;
	*effective_max_callbacks = base_max_callbacks;

	if (nevent_catchup_debt <= 0 || nevent_catchup_remaining <= 0)
		return;

	/* Deferred records retain older due ticks and therefore sort ahead of new
	 * arrivals.  This extension is enabled only by registered debt: old work
	 * receives the base capacity first, while the added capacity makes that
	 * repayment a net reduction even at the sustainable base arrival rate. */
	callback_quota =
		nevent_ceil_div_ull(static_cast<unsigned long long>(nevent_catchup_debt),
				    static_cast<unsigned long long>(nevent_catchup_remaining));
	cost_quota = nevent_ceil_div_ull(nevent_catchup_debt_estimated_us,
					 static_cast<unsigned long long>(nevent_catchup_remaining));
	nevent_catchup_quota = static_cast<long>(
		MIN(callback_quota, static_cast<unsigned long long>(NEVENT_CONFIG_MAX_CALLBACKS)));
	if (base_max_callbacks != NEVENT_UNLIMITED)
	{
		nevent_catchup_extra_callbacks = MIN(nevent_catchup_quota, max_extra_callbacks);
		*effective_max_callbacks = nevent_saturating_add_long(
			base_max_callbacks, nevent_catchup_extra_callbacks);
	}
	if (base_budget_usec != NEVENT_UNLIMITED)
	{
		nevent_catchup_extension_us = static_cast<long>(
			MIN(cost_quota, static_cast<unsigned long long>(max_extension_usec)));
		*effective_budget_usec =
			nevent_saturating_add_long(base_budget_usec, nevent_catchup_extension_us);
	}
}

static void nevent_finish_catchup_pulse(void)
{
	if (nevent_catchup_debt <= 0)
	{
		if (!nevent_deferred_due_counts.empty() || nevent_catchup_debt_estimated_us != 0)
			panic_corruption("nevent",
					 "zero catch-up count disagrees with debt metadata");
		nevent_catchup_debt = 0;
		nevent_catchup_remaining = 0;
		return;
	}
	if (nevent_deferred_due_counts.empty())
		panic_corruption("nevent", "catch-up debt has no deferred due ticks");
	if (nevent_catchup_remaining > 0)
		nevent_catchup_remaining--;
	if (nevent_catchup_remaining <= 0)
		nevent_catchup_remaining = NEVENT_CATCHUP_WINDOW_PULSES;
}

static bool nevent_trace_player(void)
{
	static const bool enabled = nevent_config_limit("DURIS_NEVENT_TRACE_PLAYER", 0, 1) > 0;
	return enabled;
}

static bool nevent_analytics_enabled(void)
{
	static long enabled = -1;
	if (enabled < 0)
		enabled = nevent_config_limit("DURIS_NEVENT_ANALYTICS", 0, 1) > 0;
	return enabled > 0;
}

static void nevent_analytics_reset(unsigned long long start_tick)
{
	nevent_analytics = {};
	nevent_analytics.window_start_tick = start_tick;
}

static struct nevent_callback_analytics *nevent_analytics_callback_slot(void *func,
									const char *name)
{
	if (!func)
		return NULL;
	try
	{
		auto [entry, inserted] = nevent_analytics.callbacks.try_emplace(func);
		struct nevent_callback_analytics &callback = entry->second;
		if (inserted)
			callback.func = func;
		if (name && !callback.name[0])
			snprintf(callback.name, sizeof(callback.name), "%s", name);
		return &callback;
	}
	catch (const std::bad_alloc &)
	{
		nevent_analytics.callback_allocation_failures++;
		return NULL;
	}
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

static void nevent_analytics_record_lateness(unsigned long long lateness)
{
	if (!nevent_analytics_enabled())
		return;
	if (lateness == 0)
		nevent_analytics.lateness_on_time++;
	else if (lateness == 1)
		nevent_analytics.lateness_one_tick++;
	else if (lateness <= 3)
		nevent_analytics.lateness_two_to_three++;
	else if (lateness <= 15)
		nevent_analytics.lateness_four_to_fifteen++;
	else
		nevent_analytics.lateness_sixteen_plus++;
}

static void nevent_analytics_emit_callbacks(void)
{
	for (auto &[func, callback] : nevent_analytics.callbacks)
	{
		const char *callback_name;
		callback_name = callback.name[0] ? callback.name : get_function_name(func);
		logit(LOG_STATUS,
		      "NEVENT ANALYTICS CALLBACK: window_start_tick=%llu func=%p name=%s calls=%lld total_us=%lld avg_us=%.2f max_us=%ld deferred=%lld",
		      nevent_analytics.window_start_tick, func,
		      callback_name ? callback_name : "unknown", callback.calls, callback.total_us,
		      callback.calls ? (double)callback.total_us / (double)callback.calls : 0.0,
		      callback.max_us, callback.deferred);
	}
}

static void nevent_analytics_emit_window()
{
	const double pulses = (double)nevent_analytics.pulses;

	logit(LOG_STATUS,
	      "NEVENT ANALYTICS WINDOW: start_tick=%llu end_tick=%llu pulses=%ld avg_scanned=%.2f avg_executed=%.2f avg_deferred=%.2f avg_total_us=%.2f peak_scanned=%ld peak_executed=%ld peak_executed_tick=%llu peak_deferred=%ld peak_total_us=%ld peak_total_us_tick=%llu peak_pending=%ld budget_exhausted_pulses=%ld callback_allocation_failures=%ld lateness_on_time=%lld lateness_1=%lld lateness_2_3=%lld lateness_4_15=%lld lateness_16_plus=%lld",
	      nevent_analytics.window_start_tick, ne_event_tick, nevent_analytics.pulses,
	      nevent_analytics.total_scanned / pulses, nevent_analytics.total_executed / pulses,
	      nevent_analytics.total_deferred / pulses, nevent_analytics.total_us / pulses,
	      nevent_analytics.peak_scanned, nevent_analytics.peak_executed,
	      nevent_analytics.peak_executed_tick, nevent_analytics.peak_deferred,
	      nevent_analytics.peak_total_us, nevent_analytics.peak_total_us_tick,
	      nevent_analytics.peak_pending, nevent_analytics.budget_exhausted_pulses,
	      nevent_analytics.callback_allocation_failures, nevent_analytics.lateness_on_time,
	      nevent_analytics.lateness_one_tick, nevent_analytics.lateness_two_to_three,
	      nevent_analytics.lateness_four_to_fifteen, nevent_analytics.lateness_sixteen_plus);
}

static bool nevent_analytics_record(long scanned, long executed, long deferred, long loop_us,
				    bool budget_exhausted)
{
	if (!nevent_analytics_enabled())
		return false;

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

	return nevent_analytics.pulses >= PULSES_IN_TICK;
}

static void nevent_analytics_extend_last_pulse(long recorded_us, long measured_us)
{
	if (!nevent_analytics_enabled() || measured_us <= recorded_us)
		return;
	nevent_analytics.total_us += measured_us - recorded_us;
	if (measured_us > nevent_analytics.peak_total_us)
	{
		nevent_analytics.peak_total_us = measured_us;
		nevent_analytics.peak_total_us_tick = ne_event_tick;
	}
}

static long nevent_elapsed_us(const struct timespec *started, const struct timespec *finished)
{
	return (finished->tv_sec - started->tv_sec) * 1000000L +
	       (finished->tv_nsec - started->tv_nsec) / 1000L;
}

/* Move every unscanned due event into the next bucket.  Reinsertion uses the
 * authoritative due/priority/aging order; future revolutions stay put and the
 * original due tick remains unchanged. */
static long nevent_defer_suffix(P_nevent deferred_head, long *new_debt)
{
	P_nevent event, next;
	unsigned int next_bucket;
	long deferred = 0;

	if (new_debt)
		*new_debt = 0;
	if (!deferred_head)
		return 0;

	next_bucket = nevent_bucket_for_tick(nevent_add_ticks(ne_event_tick, 1));

	for (event = deferred_head; event; event = next)
	{
		next = event->next_sched;

		if (event->due_tick > ne_event_tick)
			continue;

		nevent_unlink_schedule(event);
		event->element = next_bucket;
		if (event->deferral_count == 0)
		{
			nevent_register_deferred(event);
			if (new_debt)
				(*new_debt)++;
		}
		event->deferral_count++;
		nevent_analytics_record_deferred(event);
		deferred++;
		nevent_link_schedule(event, static_cast<int>(next_bucket));
	}

	return deferred;
}

static void nevent_warn_if_unbounded(long base_budget_usec, long base_max_callbacks)
{
	static bool warned = FALSE;

	if (!warned && base_budget_usec == NEVENT_UNLIMITED &&
	    base_max_callbacks == NEVENT_UNLIMITED)
	{
		warned = TRUE;
		logit(LOG_STATUS,
		      "NEVENT CONFIG WARNING: both callback and time limits are zero; the scheduler is intentionally unbounded");
	}
}

// Execute events!
void ne_events(void)
{
	static long count = 0;
	if (!nevent_require_game_thread("ne_events"))
		return;
	P_nevent next_event;
	struct timespec loop_started, callback_started, callback_finished, loop_finished;
	long scanned = 0, executed = 0, catchup_executed = 0, max_deferral_seen = 0;
	long max_late_ticks = 0, max_late_deferral = 0, slowest_us = 0, deferred = 0;
	long new_debt = 0;
	const char *max_late_name = "none";
	unsigned long long max_late_due = 0;
	unsigned long long pass_sequence;
	long base_budget_usec;
	long base_max_callbacks;
	long budget_usec;
	long max_callbacks;
	bool budget_exhausted = FALSE;
	const char *slowest_name = "none";

	clock_gettime(CLOCK_MONOTONIC, &loop_started);
	base_budget_usec = nevent_budget_usec();
	base_max_callbacks = nevent_max_callbacks();
	budget_usec = base_budget_usec;
	max_callbacks = base_max_callbacks;
	if ((pulse < 0) || (pulse >= PULSES_IN_TICK))
	{
		panic_corruption("ne_events", "pulse (%d) out of range", pulse);
	}
	if (static_cast<unsigned int>(pulse) != nevent_bucket_for_tick(ne_event_tick))
		panic_corruption("ne_events", "pulse %d disagrees with scheduler tick %llu", pulse,
				 ne_event_tick);
	if (after_events_call)
		panic_corruption("ne_events", "event pass repeated for scheduler tick %llu",
				 ne_event_tick);
	after_events_call = TRUE;
	pass_sequence = ne_event_sequence;
	nevent_warn_if_unbounded(base_budget_usec, base_max_callbacks);

	if (debug_event_list)
	{
		check_nevents();
	}

	nevent_prepare_catchup(base_budget_usec, base_max_callbacks, &budget_usec, &max_callbacks);
	PROFILE_START(event_loop);
	for (current_nevent = ne_schedule[pulse]; current_nevent; current_nevent = next_event)
	{
		scanned++;
		next_event = current_nevent->next_sched;

		if (current_nevent->sequence > pass_sequence ||
		    current_nevent->due_tick > ne_event_tick)
		{
			if (budget_usec > 0 && !(scanned % 64))
			{
				clock_gettime(CLOCK_MONOTONIC, &loop_finished);
				budget_exhausted = nevent_elapsed_us(&loop_started,
								     &loop_finished) >= budget_usec;
			}
			if (budget_exhausted && next_event)
			{
				deferred = nevent_defer_suffix(next_event, &new_debt);
				break;
			}
			continue;
		}

		if ((long)current_nevent->deferral_count > max_deferral_seen)
			max_deferral_seen = (long)current_nevent->deferral_count;
		const unsigned long long current_lateness =
			ne_event_tick > current_nevent->due_tick ?
				ne_event_tick - current_nevent->due_tick :
				0;
		const long current_lateness_long = static_cast<long>(
			MIN(current_lateness, static_cast<unsigned long long>(LONG_MAX)));
		nevent_analytics_record_lateness(current_lateness);
		if (current_lateness_long > max_late_ticks)
		{
			max_late_ticks = current_lateness_long;
			max_late_name = current_nevent->func ?
						nevent_callback_label(current_nevent->func) :
						"neutered";
			max_late_due = current_nevent->due_tick;
			max_late_deferral = (long)current_nevent->deferral_count;
		}

		// If this event has a function to execute (hasn't been neutered)
		if (current_nevent->func)
		{
			event_func_type callback_func = current_nevent->func;
			const char *callback_name = nevent_callback_label(callback_func);
			const bool periodic_callback = nevent_periodic_begin(current_nevent);
			clock_gettime(CLOCK_MONOTONIC, &callback_started);
#ifdef DO_PROFILE
			PROFILE_START(event_func);
			(callback_func)(current_nevent->ch, current_nevent->victim,
					current_nevent->obj, current_nevent->data);
			if (periodic_callback)
				nevent_periodic_complete(current_nevent);
			PROFILE_END(event_func);
			PROFILE_REGISTER_CALL(callback_func,
					      event_func_profile_end - event_func_profile_beg)
#else
			(callback_func)(current_nevent->ch, current_nevent->victim,
					current_nevent->obj, current_nevent->data);
			if (periodic_callback)
				nevent_periodic_complete(current_nevent);
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
			long long late_pulses = static_cast<long long>(
				MIN(current_lateness, static_cast<unsigned long long>(LLONG_MAX)));
			logit(LOG_STATUS,
			      "PLAYER EVENT TIMING: func=%s sequence=%llu ch_pid=%ld due_tick=%llu actual_tick=%llu late_pulses=%lld scheduled=%ld",
			      nevent_callback_label(current_nevent->func), current_nevent->sequence,
			      current_nevent->ch ? (long)GET_ID(current_nevent->ch) : -1L,
			      current_nevent->due_tick, ne_event_tick, late_pulses,
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
		if (budget_exhausted && next_event)
		{
			deferred = nevent_defer_suffix(next_event, &new_debt);
			break;
		}
	}
	current_nevent = NULL;
	nevent_process_pending_cancellations();
	nevent_periodic_watchdog();
	PROFILE_END(event_loop);
	clock_gettime(CLOCK_MONOTONIC, &loop_finished);
	long loop_us = nevent_elapsed_us(&loop_started, &loop_finished);
	if (deferred > 0)
	{
		logit(LOG_STATUS,
		      "NEVENT BUDGET: pulse=%d total_us=%ld scanned=%ld executed=%ld deferred=%ld catchup_debt=%ld catchup_debt_estimated_us=%llu catchup_oldest_due=%llu catchup_quota=%ld catchup_executed=%ld max_deferral=%ld max_late_ticks=%ld max_late_name=%s max_late_due=%llu max_late_deferral=%ld catchup_extension_us=%ld avg_callback_us=%ld slowest=%s slowest_us=%ld scheduled=%ld",
		      pulse, loop_us, scanned, executed, deferred, nevent_catchup_debt,
		      nevent_catchup_debt_estimated_us, nevent_oldest_deferred_due_tick(),
		      nevent_catchup_quota, catchup_executed, max_deferral_seen, max_late_ticks,
		      max_late_name, max_late_due, max_late_deferral, nevent_catchup_extension_us,
		      nevent_avg_callback_us, slowest_name ? slowest_name : "unknown", slowest_us,
		      ne_event_counter);
	}
	if (nevent_catchup_quota > 0 || new_debt > 0)
	{
		logit(LOG_STATUS,
		      "NEVENT CATCHUP: pulse=%d debt=%ld debt_estimated_us=%llu oldest_due=%llu remaining_pulses=%d quota=%ld extra_callbacks=%ld executed=%ld extension_us=%ld avg_callback_us=%ld new_debt=%ld",
		      pulse, nevent_catchup_debt, nevent_catchup_debt_estimated_us,
		      nevent_oldest_deferred_due_tick(), nevent_catchup_remaining,
		      nevent_catchup_quota, nevent_catchup_extra_callbacks, catchup_executed,
		      nevent_catchup_extension_us, nevent_avg_callback_us, new_debt);
	}
	nevent_finish_catchup_pulse();
	/* Include scheduler preparation, cleanup, and diagnostics already emitted
	 * above in the observed wall time. */
	clock_gettime(CLOCK_MONOTONIC, &loop_finished);
	loop_us = nevent_elapsed_us(&loop_started, &loop_finished);
	const bool analytics_window_ready =
		nevent_analytics_record(scanned, executed, deferred, loop_us, budget_exhausted);
	if (analytics_window_ready)
	{
		nevent_analytics_emit_callbacks();
		clock_gettime(CLOCK_MONOTONIC, &loop_finished);
		const long callback_diagnostic_us =
			nevent_elapsed_us(&loop_started, &loop_finished);
		nevent_analytics_extend_last_pulse(loop_us, callback_diagnostic_us);
		nevent_analytics_emit_window();
		nevent_analytics_reset(ne_event_tick + 1);
	}
	clock_gettime(CLOCK_MONOTONIC, &loop_finished);
	nevent_last_pulse_total_us = nevent_elapsed_us(&loop_started, &loop_finished);
	if (nevent_last_pulse_total_us >= 50000)
	{
		logit(LOG_STATUS,
		      "NEVENT SLOW: pulse=%d total_us=%ld scanned=%ld executed=%ld slowest=%s slowest_us=%ld scheduled=%ld",
		      pulse, nevent_last_pulse_total_us, scanned, executed,
		      slowest_name ? slowest_name : "unknown", slowest_us, ne_event_counter);
	}
	nevent_assert_pool_accounting("ne_events");
	count++;
}

void nevent_advance_tick()
{
	if (!nevent_require_game_thread("nevent_advance_tick"))
		return;
	if (!after_events_call)
		panic_corruption("nevent_advance_tick",
				 "scheduler tick %llu has not run its event pass", ne_event_tick);
	if (ne_event_tick == ULLONG_MAX)
		panic_corruption("nevent_advance_tick", "scheduler tick overflow");
	ne_event_tick++;
	pulse = static_cast<int>(nevent_bucket_for_tick(ne_event_tick));
	after_events_call = FALSE;
}

static P_nevent nevent_earlier_match(P_nevent best, P_nevent candidate, event_func func,
				     P_nevent excluded)
{
	if (!candidate || candidate == excluded || candidate->func != func ||
	    candidate->lifecycle_state != NEVENT_LIFECYCLE_ACTIVE)
		return best;
	return !best || nevent_sorts_before(candidate, best) ? candidate : best;
}

nevent_handle nevent_find_next(event_func func)
{
	P_nevent best = NULL;

	if (!nevent_require_game_thread("nevent_find_next"))
		return { NULL, 0 };
	for (int i = 0; i < PULSES_IN_TICK; i++)
		for (P_nevent event = ne_schedule[i]; event; event = event->next_sched)
			best = nevent_earlier_match(best, event, func, NULL);
	return nevent_handle_from_event(best);
}

nevent_handle nevent_find_next(P_char ch, event_func func)
{
	P_nevent best = NULL;

	if (!nevent_require_game_thread("nevent_find_next_character"))
		return { NULL, 0 };
	if (!ch)
		return { NULL, 0 };
	for (P_nevent event = ch->nevents; event; event = event->next_char_nev)
		best = nevent_earlier_match(best, event, func, NULL);
	return nevent_handle_from_event(best);
}

nevent_handle nevent_find_next_excluding_current(P_char ch, event_func func)
{
	P_nevent best = NULL;

	if (!nevent_require_game_thread("nevent_find_next_excluding_current"))
		return { NULL, 0 };
	if (!ch)
		return { NULL, 0 };
	for (P_nevent event = ch->nevents; event; event = event->next_char_nev)
		best = nevent_earlier_match(best, event, func, current_nevent);
	return nevent_handle_from_event(best);
}

nevent_handle nevent_find_next(P_obj obj, event_func func)
{
	P_nevent best = NULL;

	if (!nevent_require_game_thread("nevent_find_next_object"))
		return { NULL, 0 };
	if (!obj)
		return { NULL, 0 };
	for (P_nevent event = obj->nevents; event; event = event->next_obj_nev)
		best = nevent_earlier_match(best, event, func, NULL);
	return nevent_handle_from_event(best);
}

P_nevent get_scheduled(event_func func)
{
	return nevent_find_next(func).event;
}

P_nevent get_scheduled(P_char ch, event_func func)
{
	return nevent_find_next(ch, func).event;
}

P_nevent get_scheduled_excluding_current(P_char ch, event_func func)
{
	return nevent_find_next_excluding_current(ch, func).event;
}

P_nevent get_scheduled(P_obj obj, event_func func)
{
	return nevent_find_next(obj, func).event;
}

P_nevent get_next_scheduled_char(P_nevent e, event_func func)
{
	P_nevent best = NULL;

	if (!nevent_require_game_thread("get_next_scheduled_char"))
		return NULL;
	if (!e || !e->ch)
		return NULL;
	for (P_nevent candidate = e->ch->nevents; candidate; candidate = candidate->next_char_nev)
		if (candidate != e && nevent_sorts_before(e, candidate))
			best = nevent_earlier_match(best, candidate, func, NULL);
	return best;
}

P_nevent get_next_scheduled_obj(P_nevent e, event_func func)
{
	P_nevent best = NULL;

	if (!nevent_require_game_thread("get_next_scheduled_obj"))
		return NULL;
	if (!e || !e->obj)
		return NULL;
	for (P_nevent candidate = e->obj->nevents; candidate; candidate = candidate->next_obj_nev)
		if (candidate != e && nevent_sorts_before(e, candidate))
			best = nevent_earlier_match(best, candidate, func, NULL);
	return best;
}

void ne_init_event_pool(void)
{
	nevent_bind_game_thread();
	pulse = 0;
	current_nevent = NULL;
	ne_event_counter = 0;
	ne_event_tick = 0;
	ne_event_sequence = 0;
	after_events_call = FALSE;
	nevent_catchup_debt = 0;
	nevent_catchup_remaining = 0;
	nevent_catchup_debt_estimated_us = 0;
	nevent_deferred_due_counts.clear();
	nevent_pending_cancellations.clear();
	nevent_periodic_reset();
	memset(ne_schedule, 0, sizeof(ne_schedule));
	memset(ne_schedule_tail, 0, sizeof(ne_schedule_tail));
	ne_dead_event_pool = mm_create("NEVENTS", sizeof(struct nevent_data),
				       offsetof(struct nevent_data, next_sched), 11);
	nevent_assert_pool_accounting("ne_init_event_pool");
}

static void nevent_register_periodic_job(const char *key, event_func_type callback,
					 unsigned long long initial_delay,
					 unsigned long long interval, nevent_periodic_policy policy,
					 bool enabled)
{
	const nevent_periodic_result result =
		nevent_periodic_register(key, callback, initial_delay, interval, policy, enabled);
	if (result != nevent_periodic_result::registered)
		logit(LOG_EXIT, "NEVENT PERIODIC: registration failed for %s (status=%u)", key,
		      static_cast<unsigned int>(result));
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
	nevent_register_periodic_job("game-clock", event_another_hour, 125 * WAIT_SEC - pulse,
				     PULSES_IN_TICK, nevent_periodic_policy::fixed_rate, true);
	// AddEvent(EVENT_SPECIAL, 500 - pulse, FALSE, another_hour, 0);

	/* timed house control stuff */
	// old guildhalls (deprecated)
	// add_event(event_housekeeping, 500, NULL, NULL, NULL, 0, NULL, 0);
	// AddEvent(EVENT_SPECIAL, 500 - pulse, FALSE, do_housekeeping, 0);

	/* sunrise, sunset, etc informer */
	nevent_register_periodic_job("astral-clock", event_astral_clock, 125 * WAIT_SEC,
				     PULSES_IN_TICK, nevent_periodic_policy::fixed_rate, true);
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
	nevent_register_periodic_job("generic-character-sweep", generic_char_event, 20 * WAIT_SEC,
				     5 * WAIT_SEC, nevent_periodic_policy::fixed_delay, true);
	// AddEvent(EVENT_SPECIAL, 20 * 4, FALSE, generic_char_event, 0);

	// Checks to see if artifact souls are ready to merge.
	nevent_register_periodic_job("artifact-bind", event_artifact_check_bind_sql, 15 * WAIT_SEC,
				     7 * 60 * WAIT_SEC, nevent_periodic_policy::fixed_delay, true);

	// Makes artifacts fight and lose time on timers (penalty for multiple artis).
	nevent_register_periodic_job("artifact-wars", event_artifact_wars_sql, 20 * WAIT_SEC,
				     30 * 60 * WAIT_SEC, nevent_periodic_policy::fixed_delay, true);

	// Checks ALL artis rented and non for negative timers..
	nevent_register_periodic_job("artifact-expiry", event_artifact_check_poof_sql,
				     35 * WAIT_SEC, 12 * WAIT_SEC,
				     nevent_periodic_policy::fixed_delay, true);

	// Upkeep costs for outposts
	nevent_register_periodic_job("outpost-upkeep", event_outposts_upkeep,
				     SECS_PER_MUD_HOUR * WAIT_SEC, SECS_PER_REAL_HOUR * WAIT_SEC,
				     nevent_periodic_policy::fixed_delay, true);

	// Increases and notifies people if they've ranked up in feudal surname.
	nevent_register_periodic_job("surname-update", event_update_surnames, 45 * WAIT_SEC,
				     300 * WAIT_SEC, nevent_periodic_policy::fixed_delay, true);

	// Revisioned local player checkpoints do not depend on Redis availability.
	nevent_register_periodic_job("dirty-player-checkpoint", event_flush_dirty_players,
				     5 * WAIT_SEC, 30 * WAIT_SEC,
				     nevent_periodic_policy::fixed_delay, true);

	// redis donation message polling
	nevent_register_periodic_job("donation-message-poll", event_check_donation_messages,
				     1 * WAIT_SEC, 1 * WAIT_SEC,
				     nevent_periodic_policy::fixed_delay, redis_enabled);

	// redis world state saves for crash recovery
	nevent_register_periodic_job("world-state-save", event_save_world_state, 30 * WAIT_SEC,
				     30 * WAIT_SEC, nevent_periodic_policy::fixed_delay,
				     redis_enabled && redis_world_state_enabled &&
					     !crash_recovery_boot);

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

struct nevent_integrity_report
{
	long wheel_count;
	long character_links;
	long object_links;
	long deferred_count;
	long periodic_errors;
	unsigned long long deferred_cost_us;
	long errors;
};

static void nevent_integrity_problem(nevent_integrity_report *report, const char *format, ...)
{
	char message[512];
	va_list arguments;

	if (!report)
		return;
	report->errors++;
	va_start(arguments, format);
	vsnprintf(message, sizeof(message), format, arguments);
	va_end(arguments);
	debug("check_nevents: %s", message);
}

static bool nevent_character_link_present(P_char character, struct char_link_data *expected,
					  bool linking_list)
{
	std::unordered_set<struct char_link_data *> visited;
	struct char_link_data *link = linking_list ? character->linking : character->linked;

	for (; link; link = linking_list ? link->next_linking : link->next_linked)
	{
		if (!visited.insert(link).second)
			return false;
		if (link == expected)
			return true;
	}
	return false;
}

static nevent_integrity_report nevent_inspect_invariants(bool emit_summary)
{
	nevent_integrity_report report = {};
	std::unordered_set<P_nevent> wheel_events;
	std::unordered_set<unsigned long long> sequences;
	std::unordered_set<P_char> live_characters;
	std::unordered_set<P_obj> live_objects;
	std::unordered_set<P_nevent> character_events;
	std::unordered_set<P_nevent> object_events;
	std::map<unsigned long long, long> deferred_due_counts;

	for (P_char character = character_list; character; character = character->next)
		live_characters.insert(character);
	for (P_obj object = object_list; object; object = object->next)
		live_objects.insert(object);

	for (int bucket = 0; bucket < PULSES_IN_TICK; ++bucket)
	{
		P_nevent previous = NULL;
		for (P_nevent event = ne_schedule[bucket]; event; event = event->next_sched)
		{
			if (!wheel_events.insert(event).second)
			{
				nevent_integrity_problem(
					&report,
					"event pointer %p appears more than once in the wheel",
					(void *)event);
				break;
			}
			report.wheel_count++;
			if (event->prev_sched != previous)
				nevent_integrity_problem(
					&report,
					"sequence %llu has a non-reciprocal wheel previous link",
					event->sequence);
			if (event->element != static_cast<unsigned int>(bucket))
				nevent_integrity_problem(
					&report, "sequence %llu claims bucket %u but is in %d",
					event->sequence, event->element, bucket);
			if (event->deferral_count == 0 && nevent_bucket_for_tick(event->due_tick) !=
								  static_cast<unsigned int>(bucket))
				nevent_integrity_problem(
					&report,
					"sequence %llu due tick %llu disagrees with bucket %d",
					event->sequence, event->due_tick, bucket);
			if (!event->sequence || !sequences.insert(event->sequence).second)
				nevent_integrity_problem(
					&report, "event has a zero or duplicate sequence %llu",
					event->sequence);
			if (event->lifecycle_state != NEVENT_LIFECYCLE_ACTIVE &&
			    event->lifecycle_state != NEVENT_LIFECYCLE_CANCEL_PENDING)
				nevent_integrity_problem(
					&report,
					"sequence %llu has invalid live lifecycle state %u",
					event->sequence, event->lifecycle_state);
			if ((event->data == NULL) != (event->data_destroy == NULL))
				nevent_integrity_problem(
					&report, "sequence %llu has mismatched payload ownership",
					event->sequence);
			if (!nevent_periodic_event_is_valid(event))
				nevent_integrity_problem(
					&report,
					"sequence %llu has invalid periodic registry metadata",
					event->sequence);
			if (event->deferral_count > 0)
			{
				report.deferred_count++;
				report.deferred_cost_us = nevent_saturating_add_ull(
					report.deferred_cost_us, event->deferred_cost_us);
				deferred_due_counts[event->due_tick]++;
			}
			previous = event;
		}
		if (ne_schedule_tail[bucket] != previous)
			nevent_integrity_problem(&report, "bucket %d has an inconsistent tail",
						 bucket);
	}

	for (P_char character : live_characters)
	{
		P_nevent previous = NULL;
		std::unordered_set<P_nevent> local;
		for (P_nevent owned = character->nevents; owned; owned = owned->next_char_nev)
		{
			if (!wheel_events.count(owned) || !local.insert(owned).second)
			{
				nevent_integrity_problem(
					&report,
					"character owner id %llu has an unknown or cyclic event link",
					character->runtime_id);
				break;
			}
			report.character_links++;
			character_events.insert(owned);
			if (owned->ch != character || owned->prev_char_nev != previous)
				nevent_integrity_problem(
					&report,
					"sequence %llu has inconsistent character ownership links",
					owned->sequence);
			previous = owned;
		}
		if (character->nevents_tail != previous)
			nevent_integrity_problem(
				&report, "character owner id %llu has an inconsistent event tail",
				character->runtime_id);
	}

	for (P_obj object : live_objects)
	{
		P_nevent previous = NULL;
		std::unordered_set<P_nevent> local;
		for (P_nevent owned = object->nevents; owned; owned = owned->next_obj_nev)
		{
			if (!wheel_events.count(owned) || !local.insert(owned).second)
			{
				nevent_integrity_problem(
					&report,
					"object owner vnum %d has an unknown or cyclic event link",
					object->R_num >= 0 ? OBJ_VNUM(object) : -1);
				break;
			}
			report.object_links++;
			object_events.insert(owned);
			if (owned->obj != object || owned->prev_obj_nev != previous)
				nevent_integrity_problem(
					&report,
					"sequence %llu has inconsistent object ownership links",
					owned->sequence);
			previous = owned;
		}
		if (object->nevents_tail != previous)
			nevent_integrity_problem(
				&report, "object owner vnum %d has an inconsistent event tail",
				object->R_num >= 0 ? OBJ_VNUM(object) : -1);
	}

	for (P_nevent event : wheel_events)
	{
		if (event->ch)
		{
			if (!live_characters.count(event->ch) && event->func != release_mob_mem)
				nevent_integrity_problem(
					&report,
					"sequence %llu has a non-live character owner id %llu",
					event->sequence, event->owner_runtime_id);
			else if (live_characters.count(event->ch) && event->owner_runtime_id &&
				 event->ch->runtime_id != event->owner_runtime_id)
			{
				nevent_integrity_problem(
					&report,
					"sequence %llu character identity changed from %llu to %llu",
					event->sequence, event->owner_runtime_id,
					event->ch->runtime_id);
			}
		}
		if (event->obj)
		{
			if (!live_objects.count(event->obj))
				nevent_integrity_problem(
					&report,
					"sequence %llu has a non-live object owner vnum %d",
					event->sequence, event->diagnostic_obj_vnum);
		}
		if (event->victim)
		{
			const bool victim_live =
				live_characters.count(event->victim) &&
				(!event->victim_runtime_id ||
				 event->victim->runtime_id == event->victim_runtime_id);
			if (!victim_live)
				nevent_integrity_problem(
					&report,
					"sequence %llu has a non-live or reused victim id %llu",
					event->sequence, event->victim_runtime_id);
			else if (event->ch == event->victim)
			{
				if (event->cld)
					nevent_integrity_problem(
						&report,
						"sequence %llu has a self-target victim link",
						event->sequence);
			}
			else if (!event->ch)
				nevent_integrity_problem(
					&report, "sequence %llu has a victim without an owner",
					event->sequence);
			else if (live_characters.count(event->ch))
			{
				const bool owner_has_link =
					event->cld &&
					nevent_character_link_present(event->ch, event->cld, true);
				const bool victim_has_link =
					event->cld && nevent_character_link_present(
							      event->victim, event->cld, false);
				if (!owner_has_link || !victim_has_link)
					nevent_integrity_problem(
						&report,
						"sequence %llu is absent from a victim-link list",
						event->sequence);
				else if (event->cld->type != LNK_EVENT ||
					 event->cld->linking != event->ch ||
					 event->cld->linked != event->victim)
					nevent_integrity_problem(
						&report,
						"sequence %llu has inconsistent victim-link ownership",
						event->sequence);
			}
		}
		else if (event->cld)
			nevent_integrity_problem(&report,
						 "sequence %llu has a victim link without a victim",
						 event->sequence);
	}

	for (P_nevent event : wheel_events)
	{
		if (event->ch && live_characters.count(event->ch) && !character_events.count(event))
			nevent_integrity_problem(
				&report, "sequence %llu is absent from its character owner list",
				event->sequence);
		if (event->obj && live_objects.count(event->obj) && !object_events.count(event))
			nevent_integrity_problem(
				&report, "sequence %llu is absent from its object owner list",
				event->sequence);
	}

	if (report.wheel_count != ne_event_counter)
		nevent_integrity_problem(&report, "wheel count %ld differs from counter %ld",
					 report.wheel_count, ne_event_counter);
	if (!ne_dead_event_pool ||
	    report.wheel_count != static_cast<long>(ne_dead_event_pool->objs_used))
		nevent_integrity_problem(&report, "wheel count %ld differs from pool usage %zu",
					 report.wheel_count,
					 ne_dead_event_pool ? ne_dead_event_pool->objs_used : 0);
	if (report.deferred_count != nevent_catchup_debt ||
	    report.deferred_cost_us != nevent_catchup_debt_estimated_us ||
	    deferred_due_counts != nevent_deferred_due_counts)
		nevent_integrity_problem(
			&report,
			"deferred metadata disagrees with live records (count=%ld/%ld cost=%llu/%llu)",
			report.deferred_count, nevent_catchup_debt, report.deferred_cost_us,
			nevent_catchup_debt_estimated_us);
	report.periodic_errors = nevent_periodic_integrity_errors(emit_summary);
	report.errors += report.periodic_errors;

	if (emit_summary || report.errors)
		debug("check_nevents: errors=%ld wheel=%ld pool=%zu counter=%ld character_links=%ld object_links=%ld deferred=%ld periodic_errors=%ld at %ld",
		      report.errors, report.wheel_count,
		      ne_dead_event_pool ? ne_dead_event_pool->objs_used : 0, ne_event_counter,
		      report.character_links, report.object_links, report.deferred_count,
		      report.periodic_errors, time(NULL));
	return report;
}

// Expensive by design, but observation-only: diagnostics never sever or repair links.
bool check_nevents()
{
	if (!nevent_require_game_thread("check_nevents"))
		return false;
	return nevent_inspect_invariants(true).errors == 0;
}

void event_broken(struct char_link_data *cld)
{
	if (!nevent_require_game_thread("event_broken"))
		return;
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

static void show_world_periodic_events(P_char ch)
{
	nevent_periodic_health jobs[32];
	char output[MAX_STRING_LENGTH];
	const size_t total = nevent_periodic_copy_health(jobs, ARRAY_SIZE(jobs));
	const size_t shown = MIN(total, ARRAY_SIZE(jobs));

	snprintf(
		output, sizeof(output),
		"Periodic nevent jobs: %zu\n\n key                         | state    | policy | next due   | remain | last ok    | runs | fail | missed | watch | dup\n"
		"-----------------------------|----------|--------|------------|--------|------------|------|------|--------|-------|----\n",
		total);
	for (size_t index = 0; index < shown; ++index)
	{
		const nevent_periodic_health &job = jobs[index];
		const char *state = !job.enabled ? "disabled" :
				    job.running	 ? "running" :
				    job.armed	 ? "armed" :
						   "missing";
		const char *policy = job.policy == nevent_periodic_policy::fixed_rate ? "rate" :
											"delay";
		const unsigned long long remaining = job.enabled && job.next_due_tick >
									     ne_event_tick ?
							     job.next_due_tick - ne_event_tick :
							     0;
		char line[384];
		char last_success[32];
		if (job.has_succeeded)
			snprintf(last_success, sizeof(last_success), "%llu", job.last_success_tick);
		else
			snprintf(last_success, sizeof(last_success), "-");
		checked_snprintf(
			line, sizeof(line),
			" %-28s | %-8s | %-6s | %-10llu | %-6llu | %-10s | %-4llu | %-4llu | %-6llu | %-5llu | %-3llu\n",
			job.key, state, policy, job.next_due_tick, remaining, last_success,
			job.total_runs, job.callback_failures + job.schedule_failures,
			job.missed_runs, job.watchdog_rearms, job.duplicates_suppressed);
		strlcat(output, line, sizeof(output));
		if (job.last_failure[0])
		{
			checked_snprintf(line, sizeof(line), "   last failure: %s\n",
					 job.last_failure);
			strlcat(output, line, sizeof(output));
		}
	}
	page_string(ch->desc, output, 1);
}

void show_world_events(P_char ch, const char *arg)
{
	long count = 0;
	char buf[MAX_STRING_LENGTH];
	if (!nevent_require_game_thread("show_world_events"))
		return;
	if (!arg || arg[0] == '\0')
	{
		const nevent_integrity_report integrity = nevent_inspect_invariants(false);
		const nevent_periodic_summary periodic = nevent_periodic_summary_copy();
		snprintf(
			buf, MAX_STRING_LENGTH,
			"NEvent health: wheel=%ld counter=%ld pool=%zu integrity_errors=%ld last_pulse_total_us=%ld.\nPeriodic jobs: registered=%lu enabled=%lu armed=%lu running=%lu unhealthy=%lu failures=%llu missed=%llu watchdog_rearms=%llu duplicates_suppressed=%llu.\nSpecify a function name, or 'periodic' for job health.\n",
			integrity.wheel_count, ne_event_counter,
			ne_dead_event_pool ? ne_dead_event_pool->objs_used : 0, integrity.errors,
			nevent_last_pulse_total_us, periodic.registered, periodic.enabled,
			periodic.armed, periodic.running, periodic.unhealthy,
			periodic.callback_failures + periodic.schedule_failures,
			periodic.missed_runs, periodic.watchdog_rearms,
			periodic.duplicates_suppressed);
		send_to_char(buf, ch);
		return;
	}
	if (!str_cmp(arg, "periodic"))
	{
		show_world_periodic_events(ch);
		return;
	}

	snprintf(buf, MAX_STRING_LENGTH, "Event function: %s\n\n", arg);
	strcat(buf,
	       " bucket | due tick   | remain | late | pri | defer | sequence   | owner id/live | victim id/live | obj vnum/live\n");
	strcat(buf,
	       "--------|------------|--------|------|-----|-------|------------|---------------|----------------|--------------\n");
	for (int i = 0; i < PULSES_IN_TICK; i++)
		if (ne_schedule[i])
		{
			for (P_nevent ev = ne_schedule[i]; ev; ev = ev->next_sched)
			{
				if (strcmp(get_function_name((void *)ev->func), arg))
					continue;
				char line[256];
				const unsigned long long lateness =
					ne_event_tick > ev->due_tick ?
						ne_event_tick - ev->due_tick :
						0;
				const bool owner_live = ev->owner_runtime_id &&
							find_character_by_runtime_id(
								ev->owner_runtime_id) == ev->ch;
				const bool victim_live =
					ev->victim_runtime_id &&
					find_character_by_runtime_id(ev->victim_runtime_id) ==
						ev->victim;
				bool object_live = false;
				for (P_obj object = object_list; object; object = object->next)
					if (object == ev->obj)
					{
						object_live = true;
						break;
					}

				count++;
				checked_snprintf(
					line, sizeof line,
					" %-6u | %-10llu | %-6d | %-4llu | %-3u | %-5u | %-10llu | %8llu/%-4s | %9llu/%-4s | %8d/%-4s\n",
					ev->element, ev->due_tick, ne_event_time(ev), lateness,
					nevent_effective_priority(ev), ev->deferral_count,
					ev->sequence,
					static_cast<unsigned long long>(ev->owner_runtime_id),
					owner_live ? "yes" : "no",
					static_cast<unsigned long long>(ev->victim_runtime_id),
					victim_live ? "yes" : "no", ev->diagnostic_obj_vnum,
					object_live ? "yes" : "no");

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
