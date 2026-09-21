#include "combat/spell_wards.h"

#include "combat/damage.h"
#include "core/config.h"
#include "core/prototypes.h"
#include "core/utils.h"
#include "item/objmisc.h"
#include "magic/spells.h"
#include "net/gmcp.h"
#include "world/db.h"
#include "world/events.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <stdarg.h>

extern unsigned long long ne_event_tick;
extern P_nevent current_nevent;

namespace
{
constexpr int WARD_REFRESH_FRACTION = 2;

enum ward_kind
{
	WARD_KIND_MINOR = 0,
	WARD_KIND_SPIRIT,
	WARD_KIND_GREATER_SPIRIT,
	WARD_KIND_GLOBE,
	WARD_KIND_COUNT
};

struct ward_source
{
	P_obj object;
	uint64_t uid;
};

const int ward_spells[WARD_KIND_COUNT] = {
	SPELL_MINOR_GLOBE,
	SPELL_SPIRIT_WARD,
	SPELL_GREATER_SPIRIT_WARD,
	SPELL_GLOBE};

const unsigned int ward_flags[WARD_KIND_COUNT] = {
	SPLDAM_MINORGLOBE,
	SPLDAM_SPIRITWARD,
	SPLDAM_GRSPIRIT,
	SPLDAM_GLOBE};

const char *ward_names[WARD_KIND_COUNT] = {
	"Minor Globe",
	"Spirit Ward",
	"Greater Spirit Ward",
	"Globe"};

bool is_ward_spell(int spell)
{
	for (int i = 0; i < WARD_KIND_COUNT; ++i)
		if (ward_spells[i] == spell)
			return true;
	return false;
}

int ward_kind_for_spell(int spell)
{
	for (int i = 0; i < WARD_KIND_COUNT; ++i)
		if (ward_spells[i] == spell)
			return i;
	return -1;
}

int ward_budget_per_tick(int spell)
{
	switch (spell)
	{
	case SPELL_MINOR_GLOBE:
		return std::max(1, get_property("spell.ward.damagePerTick.minorGlobe", 150));
	case SPELL_SPIRIT_WARD:
		return std::max(1, get_property("spell.ward.damagePerTick.spiritWard", 225));
	case SPELL_GREATER_SPIRIT_WARD:
		return std::max(1, get_property("spell.ward.damagePerTick.greaterSpiritWard", 300));
	case SPELL_GLOBE:
		return std::max(1, get_property("spell.ward.damagePerTick.globe", 400));
	default:
		return 0;
	}
}

int equipment_duration_ticks(int spell)
{
	switch (spell)
	{
	case SPELL_MINOR_GLOBE:
		return 6;
	case SPELL_GLOBE:
		return 8;
	case SPELL_SPIRIT_WARD:
		return std::max(1, get_property("spell.ward.equipment.spiritLevel", 10));
	case SPELL_GREATER_SPIRIT_WARD:
		return std::max(1, get_property("spell.ward.equipment.greaterSpiritLevel", 12));
	default:
		return 0;
	}
}

int duration_pulses(int ticks)
{
	return std::max(1, ticks) * PULSES_IN_TICK;
}

uint64_t object_uid(P_obj object)
{
	if (!object)
		return 0;
	if (!object->obj_uid)
		persistence_assign_item_uid(object, "spell_ward_source");
	if (object->obj_uid)
		return static_cast<uint64_t>(object->obj_uid);
	/* Creation candidates should have a UID, but do not collapse duplicate
	 * source objects if an allocator is unavailable during a test/bootstrap. */
	return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(object));
}

bool equipment_slot_is_eligible(P_char ch, int slot)
{
	if (!ch || slot < 0 || slot >= MAX_WEAR || !ch->equipment[slot])
		return false;

	P_obj object = ch->equipment[slot];
	if ((slot == HOLD || slot == WIELD || slot == WIELD2 || slot == WIELD3 ||
	     slot == WIELD4) &&
	    object->type != ITEM_WEAPON && object->type != ITEM_FIREWEAPON &&
	    (object->wear_flags & ~(ITEM_TAKE | ITEM_HOLD | ITEM_ATTACH_BELT)))
		return false;
	if (slot == HOLD &&
	    (object->type == ITEM_WEAPON || object->type == ITEM_FIREWEAPON))
		return false;
	if ((slot == WEAR_ATTACH_BELT_2 || slot == WEAR_ATTACH_BELT_3) &&
	    !IS_ARTIFACT(object))
		return false;
	return true;
}

bool object_has_ward(P_obj object, int spell)
{
	if (!object)
		return false;
	switch (spell)
	{
	case SPELL_MINOR_GLOBE:
		return IS_SET(object->bitvector, AFF_MINOR_GLOBE);
	case SPELL_SPIRIT_WARD:
		return IS_SET(object->bitvector3, AFF3_SPIRIT_WARD);
	case SPELL_GREATER_SPIRIT_WARD:
		return IS_SET(object->bitvector3, AFF3_GR_SPIRIT_WARD);
	case SPELL_GLOBE:
		return IS_SET(object->bitvector2, AFF2_GLOBE);
	default:
		return false;
	}
}

ward_source find_equipment_source(P_char ch, int spell)
{
	ward_source result = {NULL, 0};
	for (int slot = 0; slot < MAX_WEAR; ++slot)
	{
		if (!equipment_slot_is_eligible(ch, slot) ||
		    !object_has_ward(ch->equipment[slot], spell))
			continue;
		ward_source candidate = {ch->equipment[slot], object_uid(ch->equipment[slot])};
		if (!result.object || candidate.uid < result.uid)
			result = candidate;
	}
	return result;
}

struct affected_type *find_ward_affect(P_char ch, int spell, int source_type)
{
	if (!ch)
		return NULL;
	for (struct affected_type *af = ch->affected; af; af = af->next)
	{
		if (!spell_ward_is_managed(af) || af->type != spell)
			continue;
		if (source_type == SPELL_WARD_SOURCE_NONE ||
		    af->ward_source_type == source_type)
			return af;
	}
	return NULL;
}

bool affect_is_live(P_char ch, const struct affected_type *needle)
{
	if (!ch || !needle)
		return false;
	for (const struct affected_type *af = ch->affected; af; af = af->next)
		if (af == needle)
			return true;
	return false;
}

P_nevent find_short_event(P_char ch, const struct affected_type *af)
{
	if (!ch || !af)
		return NULL;
	P_nevent event;
	LOOP_EVENTS_CH(event, ch->nevents)
	{
		if (event->func != event_short_affect || !event->data)
			continue;
		if (static_cast<struct event_short_affect_data *>(event->data)->af == af)
			return event;
	}
	return NULL;
}

bool refresh_event_matches(P_nevent event, const struct affected_type *af)
{
	if (!event || event->func != event_spell_ward_refresh || !event->data)
		return false;
	return *static_cast<struct affected_type **>(event->data) == af;
}

void cancel_short_event(P_char ch, struct affected_type *af)
{
	if (!ch || !af)
		return;
	P_nevent event;
	P_nevent next;
	for (event = ch->nevents; event; event = next)
	{
		next = event->next_char_nev;
		if (event == current_nevent)
			continue;
		if (event->func == event_short_affect && event->data &&
		    static_cast<struct event_short_affect_data *>(event->data)->af == af)
			nevent_cancel(nevent_handle_from_event(event));
	}
}

void cancel_refresh_event(P_char ch, struct affected_type *af)
{
	if (!ch || !af)
		return;
	P_nevent event;
	P_nevent next;
	for (event = ch->nevents; event; event = next)
	{
		next = event->next_char_nev;
		if (event == current_nevent)
			continue;
		if (refresh_event_matches(event, af))
			nevent_cancel(nevent_handle_from_event(event));
	}
}

void schedule_short_event(P_char ch, struct affected_type *af)
{
	if (!ch || !af || !spell_ward_is_active(af) || af->duration <= 0)
		return;
	if (find_short_event(ch, af))
		return;
	struct event_short_affect_data data = {ch, af};
	add_event(event_short_affect, std::max(1, af->duration), ch, NULL, NULL, 0, &data,
		  sizeof(data));
}

void schedule_refresh_event(P_char ch, struct affected_type *af)
{
	if (!ch || !af || !spell_ward_is_equipment(af) || !af->ward_source_worn ||
	    af->ward_refresh_remaining <= 0)
		return;
	P_nevent event;
	LOOP_EVENTS_CH(event, ch->nevents)
	{
		if (event != current_nevent && refresh_event_matches(event, af))
			return;
	}
	struct affected_type *payload = af;
	add_event(event_spell_ward_refresh, std::max(1, af->ward_refresh_remaining), ch, NULL, NULL,
		  0, &payload, sizeof(payload));
}

void set_ward_bits(P_char ch, const struct affected_type *af, bool enabled)
{
	if (!ch || !af)
		return;
	if (enabled)
	{
		ch->specials.affected_by |= af->bitvector;
		ch->specials.affected_by2 |= af->bitvector2;
		ch->specials.affected_by3 |= af->bitvector3;
		ch->specials.affected_by4 |= af->bitvector4;
		ch->specials.affected_by5 |= af->bitvector5;
	}
	else
	{
		/* A cast and an equipment source may coexist. Do not clear a shared
		 * aggregate bit when only one source has broken or expired. */
		unsigned long clear_bits[5] = {af->bitvector, af->bitvector2, af->bitvector3,
					       af->bitvector4, af->bitvector5};
		for (const struct affected_type *other = ch->affected; other; other = other->next)
		{
			if (other == af || !spell_ward_is_active(other))
				continue;
			clear_bits[0] &= ~other->bitvector;
			clear_bits[1] &= ~other->bitvector2;
			clear_bits[2] &= ~other->bitvector3;
			clear_bits[3] &= ~other->bitvector4;
			clear_bits[4] &= ~other->bitvector5;
		}
		REMOVE_BIT(ch->specials.affected_by, clear_bits[0]);
		REMOVE_BIT(ch->specials.affected_by2, clear_bits[1]);
		REMOVE_BIT(ch->specials.affected_by3, clear_bits[2]);
		REMOVE_BIT(ch->specials.affected_by4, clear_bits[3]);
		REMOVE_BIT(ch->specials.affected_by5, clear_bits[4]);
	}
}

void activate_equipment_ward(P_char ch, struct affected_type *af)
{
	if (!ch || !af || af->ward_capacity <= 0 || af->duration <= 0 || !af->ward_source_worn)
		return;
	REMOVE_BIT(af->flags, AFFTYPE_NOAPPLY);
	SET_BIT(af->flags, AFFTYPE_SHORT);
	af->ward_active = 1;
	set_ward_bits(ch, af, true);
	schedule_short_event(ch, af);
}

void deactivate_equipment_ward(P_char ch, struct affected_type *af, bool preserve_duration)
{
	if (!ch || !af)
		return;
	cancel_short_event(ch, af);
	set_ward_bits(ch, af, false);
	af->ward_active = 0;
	af->flags |= AFFTYPE_NOAPPLY;
	af->flags &= ~AFFTYPE_SHORT;
	if (!preserve_duration)
		af->duration = 0;
}

void refresh_equipment_ward(P_char ch, struct affected_type *af)
{
	if (!ch || !af || !spell_ward_is_equipment(af) || !af->ward_source_worn)
		return;
	cancel_short_event(ch, af);
	af->ward_capacity = af->ward_capacity_max;
	af->duration = af->ward_full_duration;
	af->ward_refresh_remaining = std::max(1, af->ward_full_duration / WARD_REFRESH_FRACTION);
	af->ward_active = 1;
	REMOVE_BIT(af->flags, AFFTYPE_NOAPPLY);
	SET_BIT(af->flags, AFFTYPE_SHORT);
	set_ward_bits(ch, af, true);
	schedule_short_event(ch, af);
	schedule_refresh_event(ch, af);
	gmcp_char_affects(ch);
}

void normalize_legacy_cast_wards(P_char ch)
{
	if (!ch)
		return;
	for (struct affected_type *af = ch->affected; af; af = af->next)
	{
		if (spell_ward_is_managed(af) || !is_ward_spell(af->type))
			continue;
		const int duration = af->duration > 0 ? af->duration : equipment_duration_ticks(af->type);
		const int ticks = IS_SET(af->flags, AFFTYPE_SHORT) ?
			std::max(1, duration / PULSES_IN_TICK) : std::max(1, duration);
		af->flags |= AFFTYPE_SPELL_WARD | AFFTYPE_SHORT;
		af->ward_source_type = SPELL_WARD_SOURCE_CAST;
		af->ward_source_worn = 1;
		af->ward_active = 1;
		af->ward_full_duration = duration_pulses(ticks);
		af->ward_capacity_max = static_cast<int64_t>(ticks) * ward_budget_per_tick(af->type);
		af->ward_capacity = af->ward_capacity_max;
		af->ward_refresh_remaining = 0;
		af->ward_last_tick = ne_event_tick;
		af->duration = af->ward_full_duration;
		cancel_short_event(ch, af);
		schedule_short_event(ch, af);
	}
}

bool eligible_for_flags(const struct affected_type *af, unsigned int flags)
{
	if (!spell_ward_is_active(af))
		return false;
	const int kind = ward_kind_for_spell(af->type);
	return kind >= 0 && (flags & ward_flags[kind]);
}

struct affected_type *find_absorb_candidate(P_char victim, unsigned int flags)
{
	/* Preserve the historical precedence: spirit wards before globes, with
	 * the lesser ward selected before the greater ward in the same family. */
	for (int kind : {WARD_KIND_SPIRIT, WARD_KIND_GREATER_SPIRIT, WARD_KIND_MINOR,
			 WARD_KIND_GLOBE})
	{
		if (!(flags & ward_flags[kind]))
			continue;
		for (struct affected_type *af = victim->affected; af; af = af->next)
			if (af->type == ward_spells[kind] && eligible_for_flags(af, flags))
				return af;
	}
	return NULL;
}

bool has_available_for_flags(P_char victim, unsigned int flags)
{
	return victim && find_absorb_candidate(victim, flags) != NULL;
}

void append_status(char *buffer, size_t buffer_size, size_t *used, const char *format, ...)
{
	if (!buffer || !used || *used >= buffer_size)
		return;
	va_list args;
	va_start(args, format);
	int written = vsnprintf(buffer + *used, buffer_size - *used, format, args);
	va_end(args);
	if (written < 0)
		return;
	*used += std::min(static_cast<size_t>(written), buffer_size - *used - 1);
}
} // namespace

bool spell_ward_is_managed(const struct affected_type *af)
{
	return af && IS_SET(af->flags, AFFTYPE_SPELL_WARD) && is_ward_spell(af->type);
}

bool spell_ward_is_equipment(const struct affected_type *af)
{
	return spell_ward_is_managed(af) && af->ward_source_type == SPELL_WARD_SOURCE_EQUIPMENT;
}

bool spell_ward_is_active(const struct affected_type *af)
{
	if (!spell_ward_is_managed(af) || !af->ward_active || af->ward_capacity <= 0 ||
	    IS_SET(af->flags, AFFTYPE_NOAPPLY))
		return false;
	return af->ward_source_type == SPELL_WARD_SOURCE_CAST ||
	       (af->ward_source_type == SPELL_WARD_SOURCE_EQUIPMENT && af->ward_source_worn);
}

void spell_ward_mask_equipment_bits(unsigned long bitvectors[5])
{
	if (!bitvectors)
		return;
	REMOVE_BIT(bitvectors[0], AFF_MINOR_GLOBE);
	REMOVE_BIT(bitvectors[1], AFF2_GLOBE);
	REMOVE_BIT(bitvectors[2], AFF3_SPIRIT_WARD | AFF3_GR_SPIRIT_WARD);
}

void spell_ward_cancel_events(P_char ch, struct affected_type *af)
{
	cancel_short_event(ch, af);
	cancel_refresh_event(ch, af);
}

void spell_ward_sync_timers(P_char ch)
{
	if (!ch)
		return;
	const unsigned long long now = ne_event_tick;
	for (struct affected_type *af = ch->affected; af; af = af->next)
	{
		if (!spell_ward_is_managed(af))
			continue;
		if (af->ward_last_tick == 0)
			af->ward_last_tick = now;
		else
		{
			const unsigned long long elapsed = now > af->ward_last_tick ?
				now - af->ward_last_tick : 0;
			if (spell_ward_is_equipment(af) && af->ward_source_worn && elapsed > 0 &&
			    af->ward_refresh_remaining > 0)
			{
				const int spent = static_cast<int>(std::min<unsigned long long>(
					 elapsed, static_cast<unsigned long long>(af->ward_refresh_remaining)));
				af->ward_refresh_remaining -= spent;
			}
			af->ward_last_tick = now;
		}

		if (spell_ward_is_active(af))
		{
			P_nevent event = find_short_event(ch, af);
			if (event)
				af->duration = ne_event_time(event);
		}
	}
}

void spell_ward_equipment_sync(P_char ch)
{
	if (!ch || IS_NPC(ch))
		return;
	normalize_legacy_cast_wards(ch);

	for (int kind = 0; kind < WARD_KIND_COUNT; ++kind)
	{
		const int spell = ward_spells[kind];
		ward_source source = find_equipment_source(ch, spell);
		struct affected_type *af = find_ward_affect(ch, spell, SPELL_WARD_SOURCE_EQUIPMENT);

		if (!af && source.object)
		{
			struct affected_type prototype = {};
			prototype.type = spell;
			prototype.flags = AFFTYPE_SPELL_WARD | AFFTYPE_NOSHOW | AFFTYPE_NODISPEL |
					  AFFTYPE_NOAPPLY;
			af = affect_to_char(ch, &prototype);
			const int ticks = equipment_duration_ticks(spell);
			af->ward_source_type = SPELL_WARD_SOURCE_EQUIPMENT;
			af->ward_source_uid = source.uid;
			af->ward_source_worn = 1;
			af->ward_full_duration = duration_pulses(ticks);
			af->ward_capacity_max = static_cast<int64_t>(ticks) * ward_budget_per_tick(spell);
			af->ward_capacity = af->ward_capacity_max;
			af->ward_refresh_remaining =
				std::max(1, af->ward_full_duration / WARD_REFRESH_FRACTION);
			af->duration = af->ward_full_duration;
			af->ward_last_tick = ne_event_tick;
			af->ward_active = 0;
		}

		if (!af)
			continue;

		if (!source.object)
		{
			af->ward_source_worn = 0;
			deactivate_equipment_ward(ch, af, true);
			cancel_refresh_event(ch, af);
			continue;
		}

		af->ward_source_uid = source.uid;
		af->ward_source_worn = 1;
		if (af->ward_full_duration <= 0 || af->ward_capacity_max <= 0)
		{
			const int ticks = equipment_duration_ticks(spell);
			af->ward_full_duration = duration_pulses(ticks);
			af->ward_capacity_max = static_cast<int64_t>(ticks) * ward_budget_per_tick(spell);
			if (af->ward_refresh_remaining <= 0)
				af->ward_refresh_remaining =
					std::max(1, af->ward_full_duration / WARD_REFRESH_FRACTION);
		}

		if (af->ward_refresh_remaining <= 0)
		{
			refresh_equipment_ward(ch, af);
		}
		else if (af->ward_capacity > 0 && af->duration > 0)
		{
			activate_equipment_ward(ch, af);
		}
		else
		{
			deactivate_equipment_ward(ch, af, true);
		}
		schedule_refresh_event(ch, af);
	}
}

struct affected_type *spell_ward_apply_cast(P_char victim,
						   const struct affected_type *prototype,
						   int duration_ticks)
{
	if (!victim || !prototype || !is_ward_spell(prototype->type))
		return NULL;
	/* NPC ward bits are native protection state. Keep their legacy affect
	 * shape and duration semantics separate from player durability. */
	if (IS_NPC(victim))
	{
		for (struct affected_type *af = victim->affected; af; af = af->next)
		{
			if (af->type == prototype->type)
			{
				af->duration = duration_ticks;
				return af;
			}
		}
		struct affected_type native = *prototype;
		native.duration = duration_ticks;
		return affect_to_char(victim, &native);
	}
	if (IS_PC(victim))
		normalize_legacy_cast_wards(victim);

	const int spell = prototype->type;
	struct affected_type *af = find_ward_affect(victim, spell, SPELL_WARD_SOURCE_CAST);
	const int ticks = std::max(1, duration_ticks);
	const int pulses = duration_pulses(ticks);
	const int64_t capacity = static_cast<int64_t>(ticks) * ward_budget_per_tick(spell);

	if (af)
	{
		spell_ward_sync_timers(victim);
		spell_ward_cancel_events(victim, af);
		all_affects(victim, FALSE);
		af->flags = prototype->flags | AFFTYPE_SPELL_WARD | AFFTYPE_SHORT;
		REMOVE_BIT(af->flags, AFFTYPE_NOAPPLY);
		af->bitvector = prototype->bitvector;
		af->bitvector2 = prototype->bitvector2;
		af->bitvector3 = prototype->bitvector3;
		af->bitvector4 = prototype->bitvector4;
		af->bitvector5 = prototype->bitvector5;
		af->duration = pulses;
		af->ward_full_duration = pulses;
		af->ward_capacity = capacity;
		af->ward_capacity_max = capacity;
		af->ward_refresh_remaining = 0;
		af->ward_last_tick = ne_event_tick;
		af->ward_source_type = SPELL_WARD_SOURCE_CAST;
		af->ward_source_worn = 1;
		af->ward_active = 1;
		set_ward_bits(victim, af, true);
		all_affects(victim, TRUE);
		schedule_short_event(victim, af);
		gmcp_char_affects(victim);
		return af;
	}

	struct affected_type cast = *prototype;
	cast.flags |= AFFTYPE_SPELL_WARD | AFFTYPE_SHORT;
	REMOVE_BIT(cast.flags, AFFTYPE_NOAPPLY);
	cast.duration = pulses;
	cast.ward_full_duration = pulses;
	cast.ward_capacity = capacity;
	cast.ward_capacity_max = capacity;
	cast.ward_refresh_remaining = 0;
	cast.ward_last_tick = ne_event_tick;
	cast.ward_source_type = SPELL_WARD_SOURCE_CAST;
	cast.ward_source_worn = 1;
	cast.ward_active = 1;
	return affect_to_char(victim, &cast);
}

void spell_ward_expire(P_char ch, struct affected_type *af)
{
	if (!ch || !af || !spell_ward_is_managed(af))
		return;
	if (spell_ward_is_equipment(af))
	{
		spell_ward_sync_timers(ch);
		deactivate_equipment_ward(ch, af, false);
		gmcp_char_affects(ch);
		return;
	}
	wear_off_message(ch, af);
	affect_remove(ch, af);
}

spell_ward_absorb_result spell_ward_absorb(P_char attacker, P_char victim, double damage,
							 unsigned int flags)
{
	spell_ward_absorb_result result = {damage, 0.0, false};
	if (!attacker || !victim || attacker == victim || damage <= 0.0)
		return result;

	spell_ward_sync_timers(victim);
	if (IS_PC(victim))
		normalize_legacy_cast_wards(victim);
	struct affected_type *af = find_absorb_candidate(victim, flags);
	if (!af)
		return result;

	const double available = static_cast<double>(std::max<int64_t>(0, af->ward_capacity));
	result.blocked = std::min(damage, available);
	result.remaining = damage - result.blocked;
	if (result.blocked <= 0.0)
		return result;

	const int64_t debit = static_cast<int64_t>(std::ceil(result.blocked));
	af->ward_capacity = std::max<int64_t>(0, af->ward_capacity - debit);
	result.fully_blocked = result.remaining <= 0.000001;
	if (af->ward_capacity <= 0)
	{
		if (spell_ward_is_equipment(af))
		{
			deactivate_equipment_ward(victim, af, true);
			gmcp_char_affects(victim);
		}
		else if (affect_is_live(victim, af))
		{
			wear_off_message(victim, af);
			affect_remove(victim, af);
		}
	}
	else
	{
		gmcp_char_affects(victim);
	}
	return result;
}

bool spell_ward_has_available(P_char victim, int spell)
{
	if (!victim)
		return false;
	const int circle = GetLowestSpellCircle_p(spell);
	unsigned int flags = 0;
	if (circle < 4)
		flags |= SPLDAM_MINORGLOBE;
	if (circle < 5)
		flags |= SPLDAM_SPIRITWARD;
	if (circle < 6)
		flags |= SPLDAM_GRSPIRIT;
	/* Keep the NPC/prediction helper's historical Negative Energy Barrier
	 * exception. The damage wrapper still supplies its explicit Greater Spirit
	 * Ward flag when the attack is actually resolved. */
	if (circle < 7 && spell != SPELL_DETONATE && spell != SPELL_NEG_ENERGY_BARRIER)
		flags |= SPLDAM_GLOBE;
	if (spell == SPELL_NEG_ENERGY_BARRIER)
		flags |= SPLDAM_GRSPIRIT;
	return has_available_for_flags(victim, flags);
}

bool spell_ward_item_callback_allowed(P_char victim, int spell)
{
	if (!victim || !is_ward_spell(spell))
		return true;
	for (const struct affected_type *af = victim->affected; af; af = af->next)
	{
		if (!spell_ward_is_equipment(af) || af->type != spell || !af->ward_source_worn)
			continue;
		if (spell_ward_is_active(af))
			return true;
		/* Keep a persistent source down until its scheduled renewal unless an
		 * operator explicitly opts into the legacy immediate callback. */
		return get_property("spell.ward.callbacks.ignoreDowntime", 0) != 0;
	}
	return true;
}

void spell_ward_status(P_char ch, char *buffer, size_t buffer_size)
{
	if (!buffer || buffer_size == 0)
		return;
	buffer[0] = '\0';
	if (!ch)
		return;
	spell_ward_sync_timers(ch);
	size_t used = 0;
	for (int kind = 0; kind < WARD_KIND_COUNT; ++kind)
	{
		struct affected_type *af = find_ward_affect(ch, ward_spells[kind], SPELL_WARD_SOURCE_NONE);
		if (!af)
			continue;
		const bool active = spell_ward_is_active(af);
		if (used > 0)
			append_status(buffer, buffer_size, &used, "; ");
		if (active)
		{
			append_status(buffer, buffer_size, &used, "%s %lld/%lld damage, %ds",
				       ward_names[kind], static_cast<long long>(af->ward_capacity),
				       static_cast<long long>(af->ward_capacity_max),
				       std::max(0, af->duration / WAIT_SEC));
		}
		else if (spell_ward_is_equipment(af))
		{
			append_status(buffer, buffer_size, &used, "%s equipment ward broken, refresh %ds",
				       ward_names[kind], std::max(0, af->ward_refresh_remaining / WAIT_SEC));
		}
		else
		{
			append_status(buffer, buffer_size, &used, "%s inactive", ward_names[kind]);
		}
	}
}

void event_spell_ward_refresh(P_char ch, P_char /*victim*/, P_obj /*obj*/, void *data)
{
	if (!ch || !data)
		return;
	struct affected_type *af = *static_cast<struct affected_type **>(data);
	if (!affect_is_live(ch, af) || !spell_ward_is_equipment(af))
		return;
	spell_ward_sync_timers(ch);
	ward_source source = find_equipment_source(ch, af->type);
	if (!source.object)
	{
		af->ward_source_worn = 0;
		deactivate_equipment_ward(ch, af, true);
		return;
	}
	af->ward_source_worn = 1;
	af->ward_source_uid = source.uid;
	if (af->ward_refresh_remaining <= 0)
		refresh_equipment_ward(ch, af);
	else
		schedule_refresh_event(ch, af);
}
