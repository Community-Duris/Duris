#include "core/prototypes.h"
#include "core/utils.h"
#include "magic/spells.h"
#include "world/character_maintenance.h"
#include "world/events.h"

#include <algorithm>

extern P_char character_list;
extern P_nevent current_nevent;
extern unsigned long long ne_event_tick;
extern struct time_info_data time_info;

static constexpr unsigned long long BODY_PERIOD = 20 * WAIT_SEC;
static constexpr unsigned long long NPC_CHECK_PERIOD = 5 * WAIT_SEC;
static unsigned long long first_body_tick = 0;
static bool maintenance_ready = false;

static nevent_handle maintenance_handle(P_char character)
{
	const nevent_handle handle = { character->character_maintenance_event,
				       character->character_maintenance_event_sequence };
	if (nevent_handle_is_active(handle) && handle.event->ch == character &&
	    handle.event->owner_runtime_id == character->runtime_id &&
	    handle.event->func == generic_char_event)
		return handle;
	return { nullptr, 0 };
}

void character_maintenance_leave(P_char character)
{
	if (!character || !nevent_require_game_thread("character_maintenance_leave"))
		return;
	const nevent_handle handle = maintenance_handle(character);
	character->character_maintenance_in_world = false;
	character->character_maintenance_event = nullptr;
	character->character_maintenance_event_sequence = 0;
	if (handle.event)
		nevent_cancel(handle);
}

void character_maintenance_changed(P_char character)
{
	if (!character || !maintenance_ready ||
	    !nevent_require_game_thread("character_maintenance_changed"))
		return;
	if (!character->character_maintenance_in_world)
		return;
	if (!IS_ALIVE(character))
	{
		const nevent_handle handle = maintenance_handle(character);
		character->character_maintenance_event = nullptr;
		character->character_maintenance_event_sequence = 0;
		if (handle.event)
			nevent_cancel(handle);
		return;
	}
	const nevent_handle existing = maintenance_handle(character);
	// A dispatched event is destroyed by the scheduler after its callback.
	// State changes during that callback must not create duplicate successors.
	if (existing.event == current_nevent && existing.event)
		return;

	auto &body_due = character->character_maintenance_body_due;
	if (!body_due)
		body_due = first_body_tick + character->runtime_id % BODY_PERIOD;
	// A pending callback may be due or budget-deferred. A state notification
	// must not move its body deadline past the work it still owes.
	if (!existing.event && body_due <= ne_event_tick)
		body_due += ((ne_event_tick - body_due) / BODY_PERIOD + 1) * BODY_PERIOD;
	unsigned long long delay = body_due > ne_event_tick ? body_due - ne_event_tick : 1;
	if (IS_NPC(character))
	{
		// NPC structural checks remain five-second work, even on idle NPCs.
		// Consecutive runtime identities distribute mass admission over all
		// eligible pulses instead of hashing addresses into four large waves.
		const auto phase = character->runtime_id % NPC_CHECK_PERIOD;
		const auto now_phase = ne_event_tick % NPC_CHECK_PERIOD;
		const auto check_delay =
			(phase + NPC_CHECK_PERIOD - now_phase - 1) % NPC_CHECK_PERIOD + 1;
		delay = std::min(delay, check_delay);
	}
	if (existing.event)
	{
		// Morphing a PC into an NPC may introduce the shorter safety cadence.
		if (IS_NPC(character) && existing.event->due_tick > ne_event_tick + delay)
			nevent_reschedule_after(existing, delay);
		return;
	}
	const auto scheduled =
		add_event(generic_char_event, static_cast<int>(delay), character, 0, 0, 0, 0, 0);
	if (!scheduled.was_scheduled())
		panic_corruption("character-maintenance", "cannot arm runtime %llu (status=%u)",
				 static_cast<unsigned long long>(character->runtime_id),
				 static_cast<unsigned int>(scheduled.status));
	character->character_maintenance_event = scheduled.handle.event;
	character->character_maintenance_event_sequence = scheduled.handle.sequence;
}

void character_maintenance_enter(P_char character)
{
	if (!character || !character->runtime_id ||
	    !nevent_require_game_thread("character_maintenance_enter"))
		return;
	character->character_maintenance_in_world = true;
	character_maintenance_changed(character);
}

void character_maintenance_init()
{
	if (!nevent_require_game_thread("character_maintenance_init"))
		return;
	first_body_tick = ne_event_tick + BODY_PERIOD;
	maintenance_ready = true;
	// One boot reconciliation also covers characters restored before events
	// were initialized. Steady-state discovery uses scheduler owner links.
	for (P_char character = character_list; character; character = character->next)
		character_maintenance_enter(character);
}

static bool character_has_body_work(P_char character)
{
	if (IS_PC(character) || GET_HIT(character) < GET_MAX_HIT(character) ||
	    GET_WARD(character) < GET_MAX_WARD(character) || character->specials.z_cord ||
	    IS_AFFECTED3(character, AFF3_SWIMMING))
		return true;
	if ((IS_AFFECTED2(character, AFF2_POISONED) && GET_LEVEL(character) > 30 &&
	     GET_CLASS(character, CLASS_DRUID)) ||
	    affected_by_spell(character, SPELL_PLEASANTRY))
		return true;
	for (int slot = PRIMARY_WEAPON; slot < WEAR_EYES; ++slot)
		if (character->equipment[slot] && character->equipment[slot]->type == ITEM_LIGHT &&
		    character->equipment[slot]->value[2] > 0)
			return true;
	return false;
}

static void maintain_character(P_char ch)
{
	int n;
	if (ch->in_room != NOWHERE && !IS_BLOODLUST && has_innate(ch, INNATE_VULN_SUN))
	{
		sun_damage_check(ch);
	}

	if (GET_CLASS(ch, CLASS_DRUID) && (GET_LEVEL(ch) > 30) && (IS_AFFECTED2(ch, AFF2_POISONED)))
	{
		if (poison_common_remove(ch) && current_nevent->ch == ch)
		{
			send_to_char("You neutralize the poison in your bloodstream!\r\n", ch);
		}
	}

	if (current_nevent->ch != ch)
		return;

	// that wonderful god spell...
	if (affected_by_spell(ch, SPELL_PLEASANTRY))
	{
		pleasantry(ch);
	}

	if (current_nevent->ch != ch)
		return;

	/* repair munged flyers/swimmers */
	if (ch->in_room != NOWHERE && ch->specials.z_cord > 0 && !OUTSIDE(ch))
	{
		ch->specials.z_cord = 0;
	}
	else if (ch->in_room != NOWHERE && ch->specials.z_cord < 0 && !IS_WATER_ROOM(ch->in_room))
	{
		ch->specials.z_cord = 0;
	}
	if (ch->in_room != NOWHERE && IS_SET(ch->specials.affected_by3, AFF3_SWIMMING) &&
	    !IS_WATER_ROOM(ch->in_room))
	{
		REMOVE_BIT(ch->specials.affected_by3, AFF3_SWIMMING);
	}

	/* keep taught/learned proper */
	if (IS_PC(ch) && !IS_MORPH(ch))
	{
		for (n = FIRST_SKILL; n <= LAST_SKILL; n++)
		{
			if (ch->only.pc->skills[n].taught < ch->only.pc->skills[n].learned)
				ch->only.pc->skills[n].learned = ch->only.pc->skills[n].taught;
		}
	}

	/* light sources, et al */
	update_char_objects(ch);

	/* since fights stop healing, lets make sure we restart it */
	if (GET_HIT(ch) < GET_MAX_HIT(ch))
	{
		StartRegen(ch, regen_resource::hit);
	}
	if (GET_WARD(ch) < GET_MAX_WARD(ch))
	{
		StartRegen(ch, regen_resource::ward);
	}
}

void generic_char_event(P_char ch, P_char /*victim*/, P_obj /*obj*/, void * /*data*/)
{
	// The owner link is detached synchronously on extraction/cancellation.
	// Validate the captured runtime identity before touching character state.
	if (!current_nevent || !ch || current_nevent->ch != ch ||
	    current_nevent->owner_runtime_id != ch->runtime_id ||
	    !ch->character_maintenance_in_world)
		return;
	if (!IS_ALIVE(ch))
	{
		character_maintenance_changed(ch);
		return;
	}
	if (IS_NPC(ch) && !ch->only.npc && !IS_MORPH(ch))
	{
		wizlog(AVATAR,
		       "&=LRDanger! Mob without only.npc struct! Attempting to neutralize!");
		// GET_RNUM cannot be used when only.npc is missing.
		logit(LOG_DEBUG, "mob runtime %llu (%s) without only.npc struct",
		      static_cast<unsigned long long>(ch->runtime_id), ch->player.long_descr);
		extract_char(ch);
		return;
	}
	if (ne_event_tick >= ch->character_maintenance_body_due)
	{
		ch->character_maintenance_body_due = ne_event_tick + BODY_PERIOD;
		// Re-read legacy direct stat/equipment writes when due. An idle NPC
		// still receives its structural check, but needs no maintenance body.
		if (character_has_body_work(ch))
			maintain_character(ch);
	}
	if (current_nevent->ch != ch)
		return;
	ch->character_maintenance_event = nullptr;
	ch->character_maintenance_event_sequence = 0;
	character_maintenance_changed(ch);
}
