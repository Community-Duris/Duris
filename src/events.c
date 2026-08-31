/*
 * ***************************************************************************
 * *  File: events.c                                           Part of Duris *
 * *  Usage: nevent callbacks and callback-specific helpers
 * * *  Copyright  1994, 1995 - John Bashaw and Duris Systems Ltd.
 * *
 * ***************************************************************************
 */

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _LINUX_SOURCE
#include <sys/types.h>
#endif

#include "prototypes.h"
#include "structs.h"
#include "net/comm.h"
#include "db.h"
#include "events.h"
#include "utils.h"
#include "epic.h"
#include "net/gmcp.h"
#include "combat/justice.h"
#include "mm.h"
#include "objmisc.h"
#include "profile.h"
#include "specs.prototypes.h"
#include "magic/spells.h"

void event_memorize(P_char, P_char, P_obj, void *);

extern int pulse;
extern unsigned long long ne_event_tick;
extern P_nevent current_nevent;
extern P_room world;
extern struct zone_data *zone_table;

struct regen_event_state
{
	float accumulated;
	unsigned long long last_tick;
};

static struct regen_event_state regen_state_from_data(void *data)
{
	struct regen_event_state state;

	state.accumulated = 0.0f;
	state.last_tick = ne_event_tick;
	if (data)
		state = *((struct regen_event_state *)data);
	if (state.last_tick > ne_event_tick)
		state.last_tick = ne_event_tick;
	return state;
}

static unsigned long long regen_elapsed_ticks(struct regen_event_state *state)
{
	unsigned long long elapsed = ne_event_tick - state->last_tick;

	if (elapsed == 0)
		elapsed = 1;
	state->last_tick = ne_event_tick;
	return elapsed;
}

static bool zone_reset_trace_enabled(void)
{
	static int enabled = -1;
	if (enabled < 0)
		enabled = getenv("DURIS_ZONE_RESET_TRACE") &&
			  atoi(getenv("DURIS_ZONE_RESET_TRACE")) > 0;
	return enabled > 0;
}

void calculate_regen_values(int reg, int *per_pulse, int *delay)
{
	int sign;

	*per_pulse = 1;

	if (reg < 0)
	{
		sign = -1;
		reg = -reg;
	}
	else
		sign = 1;

	while (reg > PULSES_IN_TICK)
	{
		(*per_pulse)++;
		reg -= PULSES_IN_TICK;
	}

	reg = MAX(1, ((PULSES_IN_TICK / reg) + number(0, 1)));

	if (*per_pulse > 1)
	{
		*per_pulse = sign * (*per_pulse - 1 + (pulse % reg ? 0 : 1));
		*delay = 1;
	}
	else
	{
		*delay = reg;
		*per_pulse = sign;
	}
}

#define MOB_MANA_REGEN_DELAY 5
// codemod
void event_mana_regen(P_char ch, P_char /*victim*/, P_obj /*obj*/, void *data)
{
	struct regen_event_state state = regen_state_from_data(data);
	unsigned long long elapsed_ticks = regen_elapsed_ticks(&state);
	int regen_value_int;
	int per_tick = 0;

	per_tick = IS_AFFECTED(ch, AFF_MEDITATE) ? ((GET_C_POW(ch) + GET_C_INT(ch)) / .2) :
						   ((GET_C_POW(ch) + GET_C_INT(ch)) / 2.5);
	if ((per_tick == 0) || (GET_MANA(ch) == GET_MAX_MANA(ch) && per_tick > 0) ||
	    (GET_MANA(ch) < 0 && per_tick < 0))
		return;

	state.accumulated += ((float)per_tick * (float)elapsed_ticks) / (float)PULSES_IN_TICK;
	regen_value_int = (int)state.accumulated;
	if (regen_value_int >= 1 || regen_value_int <= -1)
	{
		GET_MANA(ch) += regen_value_int;
		if (GET_MANA(ch) > GET_MAX_MANA(ch))
			GET_MANA(ch) = GET_MAX_MANA(ch);
		state.accumulated -= (float)regen_value_int;
		gmcp_char_vitals(ch);
		if (IS_PC(ch) && IS_AFFECTED(ch, AFF_MEDITATE) &&
		    GET_MANA(ch) == GET_MAX_MANA(ch) && GET_CLASS(ch, CLASS_PSIONICIST))
		{
			send_to_char("&+LYour mana reserves are now full.&n\r\n", ch);
			stop_meditation(ch);
		}
	}

	add_event(event_mana_regen, IS_PC(ch) ? 1 : MOB_MANA_REGEN_DELAY, ch, 0, 0, 0, &state,
		  sizeof(state));
}

#define MOB_WARD_REGEN_DELAY 3
// codemod
void event_ward_regen(P_char ch, P_char /*victim*/, P_obj /*obj*/, void *data)
{
	struct regen_event_state state = regen_state_from_data(data);
	unsigned long long elapsed_ticks = regen_elapsed_ticks(&state);
	int regen_value_int;
	int per_tick = ward_regen(ch, FALSE);

	if ((per_tick == 0) || (GET_WARD(ch) == GET_MAX_WARD(ch) && per_tick > 0))
		return;

	state.accumulated += ((float)per_tick * (float)elapsed_ticks) / (float)PULSES_IN_TICK;
	regen_value_int = (int)state.accumulated;
	if (regen_value_int >= 1 || regen_value_int <= -1)
	{
		GET_WARD(ch) += regen_value_int;
		if (GET_WARD(ch) > GET_MAX_WARD(ch))
			GET_WARD(ch) = GET_MAX_WARD(ch);
		state.accumulated -= (float)regen_value_int;
		gmcp_char_vitals(ch);
	}

	add_event(event_ward_regen, IS_PC(ch) ? 1 : MOB_WARD_REGEN_DELAY, ch, 0, 0, 0, &state,
		  sizeof(state));
}

#define MOB_MOVE_REGEN_DELAY 10

void event_move_regen(P_char ch, P_char /*victim*/, P_obj /*obj*/, void *data)
{
	struct regen_event_state state = regen_state_from_data(data);
	unsigned long long elapsed_ticks = regen_elapsed_ticks(&state);
	int regen_value_int;
	int per_tick = move_regen(ch, FALSE);

#if defined(CTF_MUD) && (CTF_MUD == 1)
	affected_type *af;
	if ((af = get_spell_from_char(ch, TAG_CTF_BONUS)) != NULL && af->modifier >= 10)
		per_tick *= 2;
#endif

	if ((per_tick == 0) || (GET_VITALITY(ch) == GET_MAX_VITALITY(ch) && per_tick > 0) ||
	    (GET_VITALITY(ch) < 0 && per_tick < 0))
		return;

	state.accumulated += ((float)per_tick * (float)elapsed_ticks) / (float)PULSES_IN_TICK;
	regen_value_int = (int)state.accumulated;
	if (regen_value_int >= 1 || regen_value_int <= -1)
	{
		GET_VITALITY(ch) += regen_value_int;
		if (GET_VITALITY(ch) > GET_MAX_VITALITY(ch))
			GET_VITALITY(ch) = GET_MAX_VITALITY(ch);
		state.accumulated -= (float)regen_value_int;
		gmcp_char_vitals(ch);
	}

	add_event(event_move_regen, IS_PC(ch) ? 1 : MOB_MOVE_REGEN_DELAY, ch, 0, 0, 0, &state,
		  sizeof(state));
}

void event_hit_regen(P_char ch, P_char /*victim*/, P_obj /*obj*/, void *data)
{
	struct regen_event_state state = regen_state_from_data(data);
	unsigned long long elapsed_ticks = regen_elapsed_ticks(&state);
	int regen_value_int;
	int per_tick = hit_regen(ch, FALSE);
#if defined(CTF_MUD) && (CTF_MUD == 1)
	affected_type *af;
	if ((af = get_spell_from_char(ch, TAG_CTF_BONUS)) != NULL && af->modifier >= 10)
		per_tick *= 2;
#endif

	if (per_tick == 0)
		return;

	state.accumulated += ((float)per_tick * (float)elapsed_ticks) / (float)PULSES_IN_TICK;
	regen_value_int = (int)state.accumulated;
	if (regen_value_int >= 1 || regen_value_int <= -1)
	{
		GET_HIT(ch) += regen_value_int;
		if (GET_HIT(ch) < -10)
		{
			if (!char_in_list(ch))
				return;
			if (IS_PC(ch))
			{
				logit(LOG_DEATH, "%s killed in %d (< -10 hits)", GET_NAME(ch),
				      (ch->in_room == NOWHERE) ? -1 : world[ch->in_room].number);
				statuslog(ch->player.level, "%s died in %d ( < -10 hps).",
					  GET_NAME(ch),
					  ((ch->in_room == NOWHERE) ? -1 :
								      world[ch->in_room].number));
			}
			die(ch, ch);
			return;
		}
		if (GET_HIT(ch) > GET_MAX_HIT(ch))
			GET_HIT(ch) = GET_MAX_HIT(ch);
		state.accumulated -= (float)regen_value_int;
		gmcp_char_vitals(ch);
	}

	update_pos(ch);
	add_event(event_hit_regen, 1, ch, 0, 0, 0, &state, sizeof(state));
}

void StartRegen(P_char ch, regen_resource resource)
{
	event_func_type func;
	int delay, per_tick;

	if (resource == regen_resource::vitality)
	{
		func = event_move_regen;
		if (get_scheduled(ch, func))
			return;
		per_tick = move_regen(ch, FALSE);
		delay = IS_PC(ch) ? 1 : MOB_MOVE_REGEN_DELAY;
	}
	else if (resource == regen_resource::hit)
	{
		func = event_hit_regen;
		if (get_scheduled(ch, func))
			return;
		per_tick = hit_regen(ch, FALSE);
		delay = 1;
	}
	else if (resource == regen_resource::mana)
	{
		func = event_mana_regen;
		if (get_scheduled(ch, func))
			return;
		// codemod remove multiplicate of mana_regen
		per_tick = mana_regen(ch, FALSE);

		delay = IS_PC(ch) ? 1 : MOB_MANA_REGEN_DELAY;
	}
	else if (resource == regen_resource::ward)
	{
		func = event_ward_regen;
		if (get_scheduled(ch, func))
			return;
		per_tick = ward_regen(ch, FALSE);
		delay = IS_PC(ch) ? 1 : MOB_WARD_REGEN_DELAY;
	}
	else
		return;

	if (per_tick == 0)
		return;

	struct regen_event_state state;
	state.accumulated = 0.0f;
	state.last_tick = ne_event_tick;
	add_event(func, delay, ch, 0, 0, 0, &state, sizeof(state));
}

void event_wait(P_char ch, P_char /*victim*/, P_obj /*obj*/, void * /*data*/)
{
	if (!ch)
	{
		logit(LOG_EXIT, "event_wait called in events.c with no ch");
		return;
	}
	if (ch) // Just making sure.
	{
		if (ch->specials.act2 & PLR2_WAIT)
		{
			REMOVE_BIT(ch->specials.act2, PLR2_WAIT);
		}
		if (ch->in_room != NOWHERE)
		{
			update_pos(ch);
		}
	}
}

void DelayCommune(P_char ch, int delay)
{
	if (USES_SPELL_SLOTS(ch))
	{
		P_nevent e = get_scheduled(ch, event_memorize);
		if (e)
		{
			const unsigned long long old_time =
				static_cast<unsigned long long>(ne_event_time(e));
			const unsigned long long extension =
				delay > 0 ? static_cast<unsigned long long>(delay) : 0;
			const unsigned long long new_delay = extension > ULLONG_MAX - old_time ?
								     ULLONG_MAX :
								     old_time + extension;
			if (!nevent_reschedule_after(nevent_handle_from_event(e), new_delay))
				logit(LOG_EXIT,
				      "DelayCommune: failed to reschedule memorize event");
		}
	}
}

void CharWait(P_char ch, int delay)
{
	P_nevent e = NULL;
	nevent_schedule_result scheduled = { nevent_schedule_status::invalid_replace_target,
					     { NULL, 0 } };

	if (!ch)
	{
		logit(LOG_EXIT, "CharWait called in events.c with no ch");
		return;
	}

	if (!IS_ALIVE(ch))
	{
		REMOVE_BIT(ch->specials.act2, PLR2_WAIT);
		debug("CharWait: Dead char: %s", J_NAME(ch));
		return;
	}

	// A negative delay is refused by add_event(), which would leave PLR2_WAIT set
	//   with nothing scheduled to clear it -- the character could never act again.
	if (delay < 0)
	{
		debug("CharWait: negative delay (%d) for %s, clamping to 0.", delay, J_NAME(ch));
		delay = 0;
	}

	if (!CAN_ACT(ch))
	{
		// The event event_wait just turns off the PLR2_WAIT bit (and updates position).
		e = get_scheduled(ch, event_wait);
		if (e)
		{
			// If the new delay is shorter than the current, ignore new.
			if (ne_event_time(e) >= delay)
			{
				return;
			}
		}
	}
	// An absurd delay is almost always a caller bug, and it gates the character for
	//   the whole time.  Log it so the caller can be found, but honour it.
	if (delay > PULSES_IN_TICK)
	{
		logit(LOG_DEBUG, "CharWait: %s given a %d pulse (%d sec) wait.", J_NAME(ch), delay,
		      delay / WAIT_SEC);
	}

	if (e)
		scheduled = nevent_replace(nevent_handle_from_event(e), event_wait, delay, ch, NULL,
					   NULL, 0, NULL, 0);
	else
		scheduled = add_event(event_wait, delay, ch, NULL, NULL, 0, NULL, 0);

	if (!scheduled)
	{
		debug("CharWait: event_wait schedule failed for %s (status %u)", J_NAME(ch),
		      static_cast<unsigned int>(scheduled.status));
		if (!e)
			REMOVE_BIT(ch->specials.act2, PLR2_WAIT);
		return;
	}

	if (!IS_TRUSTED(ch))
	{
		const unsigned long long grace = static_cast<unsigned long long>(2 * WAIT_SEC);
		SET_BIT(ch->specials.act2, PLR2_WAIT);
		// Hard deadline for the command gate.  event_wait normally clears it first,
		// but the gate must still recover if the event is lost or delayed.
		ch->specials.wait_until_pulse = scheduled.handle.event->due_tick >
								ULLONG_MAX - grace ?
							ULLONG_MAX :
							scheduled.handle.event->due_tick + grace;
	}
}

void event_reset_zone(P_char /*ch*/, P_char /*victim*/, P_obj /*obj*/, void *data)
{
	int zone = *((int *)data);
	int age_before = zone_table[zone].age;
	bool will_reset = age_before + 1 >= zone_table[zone].lifespan &&
			  (zone_table[zone].reset_mode == 2 || ::is_empty(zone));

	if (zone_reset_trace_enabled())
		logit(LOG_STATUS,
		      "ZONE RESET TRACE: zone_rnum=%d zone_vnum=%d name=%s fired_tick=%llu due_tick=%llu lateness_ticks=%lld event_bucket=%d sequence=%llu age_before=%d lifespan=%d age_after=%d will_reset=%d reset_mode=%d pulse=%d",
		      zone, zone_table[zone].number, zone_table[zone].name, ne_event_tick,
		      current_nevent ? current_nevent->due_tick : 0,
		      current_nevent ?
			      (long long)ne_event_tick - (long long)current_nevent->due_tick :
			      0,
		      current_nevent ? current_nevent->element : -1,
		      current_nevent ? current_nevent->sequence : 0, age_before,
		      zone_table[zone].lifespan, age_before + 1, will_reset ? 1 : 0,
		      zone_table[zone].reset_mode, pulse);

	zone_table[zone].age++;

	if (will_reset)
	{
		reset_zone(zone, 0);
	}
	else if (!zone_table[zone].reset_mode)
	{
		no_reset_zone_reset(zone);
		return;
	}

	add_event(event_reset_zone, PULSES_IN_TICK, 0, 0, 0, 0, &zone, sizeof(zone));
}

void room_event(P_char /*ch*/, P_char /*victim*/, P_obj /*obj*/, void *data)
{
	P_room room = &world[*((int *)data)];

	if (room && room->funct)
	{
		(room->funct)(room->number, 0, 0, 0);
		add_event(room_event, PULSE_MOBILE + number(-4, 4), 0, 0, 0, 0, data, sizeof(int));
	}
}
