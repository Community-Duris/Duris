/* Character falling policy execution and world effects. */

#include "world/falling.h"

#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utils.h"
#include "magic/spells.h"
#include "net/comm.h"
#include "world/db.h"
#include "world/events.h"
#include "world/falling_policy.h"
#include "world/vnum.obj.h"

#include <cstdint>

extern P_room world;

namespace
{
enum class falling_injury_result
{
	survived,
	relocated,
	removed
};

bool character_survived(P_char ch, std::uint64_t removal_before)
{
	if (!ch)
		return false;
	if (removal_before != character_removal_generation && !char_in_list(ch))
		return false;
	return IS_ALIVE(ch);
}

nevent_schedule_result schedule_falling_event(P_char ch, int speed, int delay)
{
	const nevent_schedule_result scheduled =
		add_event(event_falling_char, delay, ch, NULL, NULL, 0, &speed, sizeof(speed));
	if (!scheduled.was_scheduled())
		logit(LOG_DEBUG, "Falling event schedule rejected with status %d.",
		      static_cast<int>(scheduled.status));
	return scheduled;
}

falling_injury_result apply_falling_injury(P_char victim, int amount, bool allow_death,
					   int expected_room)
{
	if (allow_death)
	{
		const std::uint64_t removal_before = character_removal_generation;
		if (damage(victim, victim, amount, DAMAGE_FALLING))
			return falling_injury_result::removed;
		if (!character_survived(victim, removal_before))
			return falling_injury_result::removed;
	}
	else
	{
		GET_HIT(victim) = MAX(GET_HIT(victim) - amount, -8);
		update_pos(victim);
	}

	if (victim->in_room != expected_room)
		return falling_injury_result::relocated;

	const int injury_percent = falling_injury_percent(amount, GET_MAX_HIT(victim));
	SET_POS(victim, number(0, 2) + GET_STAT(victim));
	const std::uint64_t stun_removal_before = character_removal_generation;
	Stun(victim, victim, injury_percent, FALSE);
	if (!character_survived(victim, stun_removal_before))
		return falling_injury_result::removed;
	if (victim->in_room != expected_room)
		return falling_injury_result::relocated;

	if (number(1, injury_percent) >
	    number(STAT_INDEX(GET_C_CON(victim)) / 2, STAT_INDEX(GET_C_CON(victim)) * 3))
	{
		const std::uint64_t knockout_removal_before = character_removal_generation;
		KnockOut(victim, number(2, MAX(2, (100 - GET_C_CON(victim)))));
		if (!character_survived(victim, knockout_removal_before))
			return falling_injury_result::removed;
		if (victim->in_room != expected_room)
			return falling_injury_result::relocated;
	}

	return falling_injury_result::survived;
}

falling_step_result checked_look(P_char ch, int expected_room, int mode)
{
	const std::uint64_t removal_before = character_removal_generation;
	do_look(ch, 0, mode);
	if (!character_survived(ch, removal_before))
		return falling_step_result::actor_removed;
	if (ch->in_room != expected_room)
		return falling_step_result::relocated;
	return falling_step_result::continued;
}
} // namespace

void event_falling_char(P_char ch, P_char /*victim*/, P_obj /*obj*/, void *data)
{
	if (!data)
	{
		logit(LOG_DEBUG, "Falling event received a null speed payload.");
		return;
	}
	falling_step(ch, *static_cast<const int *>(data));
}

falling_start_result falling_start(P_char ch)
{
	P_nevent event;

	if (!ch)
	{
		logit(LOG_EXIT, "falling_start with NULL char.");
		return falling_start_result::not_applicable;
	}
	if (!IS_ALIVE(ch) || ch->in_room == NOWHERE)
		return falling_start_result::not_applicable;

	LOOP_EVENTS_CH(event, ch->nevents)
	{
		if (event->func == event_falling_char)
			return falling_start_result::not_applicable;
	}

	/* Grounded chance-fall rooms may have a blocked downward exit. */
	if ((world[ch->in_room].sector_type != SECT_NO_GROUND) &&
	    (world[ch->in_room].sector_type != SECT_UNDRWLD_NOGROUND) &&
	    !VIRTUAL_CAN_GO(ch->in_room, DIR_DOWN) && ch->specials.z_cord == 0)
		return falling_start_result::not_applicable;
	if (ch->specials.z_cord < 0)
		return falling_start_result::not_applicable;

	if (world[ch->in_room].dir_option[DIR_DOWN] &&
	    IS_SET(world[ch->in_room].dir_option[DIR_DOWN]->exit_info, EX_BREAKABLE))
		return falling_start_result::not_applicable;

	if (IS_TRUSTED(ch) || IS_AFFECTED(ch, AFF_LEVITATE) || IS_AFFECTED(ch, AFF_FLY))
	{
		send_to_char(
			"You grin as you realize you're floating, with no floor in the room.\n",
			ch);
		return falling_start_result::not_applicable;
	}
	if (IS_RIDING(ch) &&
	    (IS_AFFECTED(GET_MOUNT(ch), AFF_LEVITATE) || IS_AFFECTED(GET_MOUNT(ch), AFF_FLY)))
	{
		send_to_char("You grin as you realize you're floating upon a flying mount.", ch);
		return falling_start_result::not_applicable;
	}

	const bool climb_active = affected_by_spell(ch, SKILL_CLIMB);
	if (climb_active &&
	    falling_climb_catches(true, GET_CHAR_SKILL(ch, SKILL_CLIMB), number(1, 100)))
	{
		send_to_char("You start to slip, but catch yourself.\n", ch);
		return falling_start_result::caught;
	}

	act("$n has just realized $e has no visible means of support!", TRUE, ch, 0, 0, TO_ROOM);
	if (GET_STAT(ch) > STAT_SLEEPING)
		send_to_char("You rediscover the law of gravity...\n...the hard way!\n", ch);
	else if (GET_STAT(ch) > STAT_INCAP)
		send_to_char("You get a sinking feeling.\n", ch);
	else
		send_to_char("Just when it seemed things couldn't get any worse...\n", ch);

	if (CAN_GO(ch, DIR_DOWN) || ch->specials.z_cord > 0)
	{
		const nevent_schedule_result scheduled = schedule_falling_event(ch, 1, 0);
		return scheduled.was_scheduled() ? falling_start_result::scheduled :
						   falling_start_result::schedule_rejected;
	}

	send_to_char("But wait!  Saved by a bug!\n", ch);
	act("$n is granted a reprieve, and breathes a prayer of thanks", FALSE, ch, 0, 0, TO_ROOM);
	logit(LOG_DEBUG, "Room (%d) Name: (%s) is NO_GROUND but has no valid 'down' exit",
	      world[ch->in_room].number, GET_NAME(ch));
	world[ch->in_room].sector_type = SECT_INSIDE;
	return falling_start_result::not_applicable;
}

falling_step_result falling_step(P_char ch, int speed)
{
	if (!ch)
	{
		logit(LOG_EXIT, "falling_step with NULL char.");
		return falling_step_result::actor_removed;
	}
	if (!IS_ALIVE(ch) || ch->in_room == NOWHERE)
		return falling_step_result::actor_removed;

	const bool allow_lethal_damage = speed != 0;
	const int source_room = ch->in_room;
	int new_room = source_room;
	bool completed_vertical_descent = false;

	const auto *source_down = world[source_room].dir_option[DIR_DOWN];
	const bool open_down = source_down && source_down->to_room != NOWHERE &&
			       !IS_SET(source_down->exit_info, EX_CLOSED) &&
			       !IS_SET(source_down->exit_info, EX_BREAKABLE);
	const falling_route route = falling_choose_route(ch->specials.z_cord > 0, open_down);

	if (route != falling_route::stay)
	{
		if (route == falling_route::vertical)
		{
			--ch->specials.z_cord;
			completed_vertical_descent = ch->specials.z_cord == 0;
			new_room = source_room;
		}
		else
			new_room = source_down->to_room;

		if (speed < 45)
			act("$n drops from sight.", TRUE, ch, 0, 0, TO_ROOM);
		else if (speed < 90)
			act("Someone drops from sight.", TRUE, ch, 0, 0, TO_ROOM);
		else
			act("A large (screaming) object drops from sight!", TRUE, ch, 0, 0,
			    TO_ROOM);

		std::uint64_t removal_before = character_removal_generation;
		char_from_room(ch);
		if (!character_survived(ch, removal_before))
			return falling_step_result::actor_removed;
		if (ch->in_room != NOWHERE)
		{
			logit(LOG_DEBUG, "Falling movement was rejected while leaving room %d.",
			      source_room);
			return falling_step_result::movement_rejected;
		}

		removal_before = character_removal_generation;
		const bool entered = char_to_room(ch, new_room, -2);
		if (!character_survived(ch, removal_before))
			return falling_step_result::actor_removed;
		if (!entered || ch->in_room == NOWHERE)
		{
			logit(LOG_DEBUG, "Falling movement failed while entering room %d.",
			      new_room);
			if (ch->in_room == NOWHERE)
			{
				removal_before = character_removal_generation;
				const bool restored = char_to_room(ch, source_room, -2);
				if (!character_survived(ch, removal_before))
					return falling_step_result::actor_removed;
				if (!restored || ch->in_room != source_room)
					logit(LOG_DEBUG,
					      "Falling movement could not restore rejected actor to room %d.",
					      source_room);
			}
			return falling_step_result::movement_rejected;
		}
		new_room = ch->in_room;

		const falling_speed_decision speed_decision = falling_advance_speed(
			speed, IS_AFFECTED(ch, AFF_LEVITATE), IS_AFFECTED(ch, AFF_FLY));
		speed = speed_decision.speed;
		if (speed_decision.motion == falling_motion::stopped)
		{
			send_to_char("You slow to a stop.  WHEW!\n", ch);
			act("$n floats in from above.", TRUE, ch, 0, 0, TO_ROOM);
			const falling_step_result look = checked_look(ch, new_room, -2);
			return look == falling_step_result::continued ?
				       falling_step_result::stopped :
				       look;
		}
		if (IS_AFFECTED(ch, AFF_LEVITATE) || IS_AFFECTED(ch, AFF_FLY))
			send_to_char("You slow down a little.\n", ch);
	}

	const auto *down = world[new_room].dir_option[DIR_DOWN];
	const bool should_land = falling_should_land(down != NULL, down && down->to_room != NOWHERE,
						     down && IS_SET(down->exit_info, EX_CLOSED),
						     down && IS_SET(down->exit_info, EX_BREAKABLE),
						     completed_vertical_descent);

	if (should_land)
	{
		const int impact_roll = number(80, 120);
		const int safe_fall_skill = GET_CHAR_SKILL(ch, SKILL_SAFE_FALL);
		const int safe_fall_roll = safe_fall_skill ? number(1, 101) : 0;
		const int impact_damage = falling_impact_damage(GET_MAX_HIT(ch), speed,
								GET_C_AGI(ch), impact_roll,
								safe_fall_skill, safe_fall_roll);
		const int impact_room = ch->in_room;

		if (down && IS_SET(down->exit_info, EX_BREAKABLE))
		{
			P_obj wall;
			for (wall = world[impact_room].contents; wall; wall = wall->next_content)
			{
				if (wall->R_num == real_object(VOBJ_WALLS) && wall->value[1] == 5)
					break;
			}

			if (wall && falling_breaks_floor(speed, wall->value[2]))
			{
				act("You slam into $p!", FALSE, ch, wall, 0, TO_CHAR);
				act("$n falls from above and slams into $p!", FALSE, ch, wall, 0,
				    TO_ROOM);
				std::uint64_t removal_before = character_removal_generation;
				if (damage(ch, ch, impact_damage, TYPE_UNDEFINED))
					return falling_step_result::actor_removed;
				if (!character_survived(ch, removal_before))
					return falling_step_result::actor_removed;
				if (ch->in_room != impact_room)
					return falling_step_result::relocated;

				removal_before = character_removal_generation;
				spell_dispel_magic(70, ch, NULL, SPELL_TYPE_SPELL, 0, wall);
				if (!character_survived(ch, removal_before))
					return falling_step_result::actor_removed;
				if (ch->in_room != impact_room)
					return falling_step_result::relocated;

				speed /= 2;
				const nevent_schedule_result scheduled =
					schedule_falling_event(ch, speed, 0);
				return scheduled.was_scheduled() ?
					       falling_step_result::continued :
					       falling_step_result::schedule_rejected;
			}
			if (wall)
				wall->value[2] /= 2;
		}

		if (IS_WATER_ROOM(impact_room))
		{
			send_to_char("With a splash, you plunge into the waters!\n", ch);
			const falling_step_result look = checked_look(ch, impact_room, -2);
			if (look != falling_step_result::continued)
				return look;
			act("$n drops in from above with a loud splash.", TRUE, ch, 0, 0, TO_ROOM);
			return falling_step_result::landed;
		}

		send_to_char("You land with stunning force!\n", ch);
		act("$n falls in from above, landing in a crumpled heap!", TRUE, ch, 0, 0, TO_ROOM);
		if (ch->specials.z_cord > 0)
			ch->specials.z_cord = 0;

		P_char rider = get_linking_char(ch, LNK_RIDING);
		if (rider && (!char_in_list(rider) || !IS_ALIVE(rider)))
			rider = NULL;
		const bool rider_at_impact = rider && rider->in_room == impact_room;
		if (rider)
			unlink_char(rider, ch, LNK_RIDING);

		const falling_injury_result actor_injury = apply_falling_injury(
			ch, impact_damage, allow_lethal_damage && !rider, impact_room);
		if (actor_injury == falling_injury_result::removed)
			return falling_step_result::actor_removed;
		if (actor_injury == falling_injury_result::relocated)
			return falling_step_result::relocated;

		if (rider_at_impact && (!char_in_list(rider) || !IS_ALIVE(rider)))
			rider = NULL;
		if (rider_at_impact && rider && ch->in_room == rider->in_room)
		{
			send_to_char("You land with stunning force!\n", rider);
			send_to_char("Your rider suffers a similar fate!\n", ch);
			act("$n, riding $N, is thrown off $s mount and slams into the ground!",
			    TRUE, rider, 0, ch, TO_NOTVICT);
			if (rider->specials.z_cord > 0)
				rider->specials.z_cord = 0;

			const std::uint64_t actor_removal_before = character_removal_generation;
			const falling_injury_result rider_injury = apply_falling_injury(
				rider, impact_damage, allow_lethal_damage, impact_room);
			if (rider_injury != falling_injury_result::survived)
				return falling_step_result::landed;
			if (!character_survived(ch, actor_removal_before))
				return falling_step_result::actor_removed;
			if (ch->in_room != impact_room)
				return falling_step_result::relocated;
		}

		return falling_step_result::landed;
	}

	if (speed < 45)
	{
		act("$n falls in from above.", TRUE, ch, 0, 0, TO_ROOM);
		const falling_step_result look = checked_look(ch, new_room, -2);
		if (look != falling_step_result::continued)
			return look;
	}
	else if (speed < 90)
	{
		act("Someone hurtles in from above.", TRUE, ch, 0, 0, TO_ROOM);
		if (IS_PC(ch))
		{
			const int saved_act = ch->specials.act;
			SET_BIT(ch->specials.act, PLR_BRIEF);
			const falling_step_result look = checked_look(ch, new_room, -2);
			if (look != falling_step_result::actor_removed)
				ch->specials.act = saved_act;
			if (look != falling_step_result::continued)
				return look;
		}
	}
	else
	{
		act("A large (screaming) object plummets in from above!", TRUE, ch, 0, 0, TO_ROOM);
		send_to_char("You fall, shapes and sounds shredding past you.\n", ch);
	}

	const nevent_schedule_result scheduled =
		schedule_falling_event(ch, speed, falling_event_delay(speed));
	return scheduled.was_scheduled() ? falling_step_result::continued :
					   falling_step_result::schedule_rejected;
}
