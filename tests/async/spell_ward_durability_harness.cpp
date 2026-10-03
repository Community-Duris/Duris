#include "combat/spell_wards.h"
#include "combat/damage.h"
#include "core/prototypes.h"
#include "core/utils.h"
#include "magic/spells.h"
#include "net/comm.h"
#include "player/player_snapshot.h"
#include "player/player_snapshot_codec.h"
#include "world/events.h"
#include "world/falling.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>

// Production ward/spell/codec units run against deterministic event and world
// services. These fixtures do not claim to exercise SQL or the network loop.
unsigned long long ne_event_tick = 100;
P_nevent current_nevent = nullptr;
index_data indexes[2] = {};
P_index obj_index = indexes;
int top_of_objt = 1;
P_char character_list = nullptr;
P_room world = nullptr;
extern const int rev_dir[] = { 0, 0, 0, 0, 0, 0 };
extern const racial_data_type racial_data[LAST_RACE + 1] = {};
time_info_data age(P_char)
{
	return {};
}
void StartRegen(P_char, regen_resource) {}
void remove_disguise(P_char, bool) {}
int IS_MORPH(P_char)
{
	return 0;
}
P_char un_morph(P_char ch)
{
	return ch;
}
bool char_falling(P_char)
{
	return false;
}
falling_start_result falling_start(P_char)
{
	std::abort();
}
bool is_linked_to(P_char, P_char, ush_int)
{
	return true;
}
bool NewSaves(P_char, int, int)
{
	return false;
}
bool resists_spell(P_char, P_char)
{
	return false;
}
void logit(const char *, const char *, ...) {}
[[noreturn]] int panic_corruption_int(const char *, const char *, ...)
{
	std::abort();
}
void add_tag_to_char(P_char, int, int, int, int)
{
	std::abort();
}
void appear(P_char, bool) {}
void remember(P_char, P_char) {}
void MobStartFight(P_char, P_char) {}
void balance_affects(P_char) {}
bool ac_can_see(P_char, P_char, bool)
{
	return true;
}
int invoke_object_special(P_obj, P_char, int, char *)
{
	return 0;
}
int real_object(int)
{
	return 0;
}
int number(int low, int)
{
	return low;
}
void Decay(P_obj) {}
static int spirit_item_ticks = 0;
int get_property(const char *key, int fallback)
{
	return spirit_item_ticks && std::strcmp(key, "spell.ward.equipment.spiritLevel") == 0 ?
		       spirit_item_ticks :
		       fallback;
}
void persistence_assign_item_uid(P_obj obj, const char *)
{
	obj->obj_uid = 99;
}
void gmcp_char_affects(P_char) {}
void wear_off_message(P_char, affected_type *) {}
void act(const char *, int, P_char, P_obj, void *, int) {}
void do_point(P_char, P_char) {}
void send_to_char(const char *, P_char) {}
int vapor(P_obj, P_char, int, char *)
{
	return 0;
}
int GET_CLASS(P_char ch, uint mask)
{
	return (ch->player.m_class & mask) != 0;
}

nevent_schedule_result add_event(event_func func, int delay, P_char ch, P_char victim, P_obj obj,
				 int, const void *data, int size)
{
	auto event = new nevent_data{};
	event->func = func;
	event->ch = ch;
	event->victim = victim;
	event->obj = obj;
	event->due_tick = ne_event_tick + delay;
	event->data = std::malloc(size);
	std::memcpy(event->data, data, size);
	event->next_char_nev = ch->nevents;
	ch->nevents = event;
	return { nevent_schedule_status::scheduled, { event, 1 } };
}
nevent_handle nevent_handle_from_event(P_nevent event)
{
	return { event, 1 };
}
nevent_cancel_result nevent_cancel(nevent_handle handle)
{
	auto event = handle.event;
	P_nevent *link = &event->ch->nevents;
	while (*link && *link != event)
		link = &(*link)->next_char_nev;
	assert(*link == event);
	*link = event->next_char_nev;
	std::free(event->data);
	delete event;
	return nevent_cancel_result::canceled;
}
void event_short_affect(P_char ch, P_char, P_obj, void *data)
{
	spell_ward_expire(ch, static_cast<event_short_affect_data *>(data)->af);
}
affected_type *affect_to_char(P_char ch, affected_type *prototype)
{
	auto af = new affected_type(*prototype);
	af->ward_last_tick = ne_event_tick;
	af->next = ch->affected;
	ch->affected = af;
	ch->specials.affected_by |= af->bitvector;
	ch->specials.affected_by2 |= af->bitvector2;
	ch->specials.affected_by3 |= af->bitvector3;
	if (af->flags & AFFTYPE_SHORT)
	{
		event_short_affect_data data = { ch, af };
		add_event(event_short_affect, af->duration, ch, nullptr, nullptr, 0, &data,
			  sizeof(data));
	}
	return af;
}
void affect_remove(P_char ch, affected_type *af)
{
	spell_ward_cancel_events(ch, af);
	affected_type **link = &ch->affected;
	while (*link && *link != af)
		link = &(*link)->next;
	assert(*link == af);
	*link = af->next;
	ch->specials.affected_by &= ~af->bitvector;
	ch->specials.affected_by2 &= ~af->bitvector2;
	ch->specials.affected_by3 &= ~af->bitvector3;
	for (auto other = ch->affected; other; other = other->next)
	{
		if (!spell_ward_is_active(other))
			continue;
		ch->specials.affected_by |= other->bitvector;
		ch->specials.affected_by2 |= other->bitvector2;
		ch->specials.affected_by3 |= other->bitvector3;
	}
	delete af;
}
void all_affects(P_char ch, int mode)
{
	spell_ward_sync_timers(ch);
	if (mode)
		spell_ward_equipment_sync(ch);
}
bool affected_by_spell(P_char ch, int spell)
{
	for (auto af = ch->affected; af; af = af->next)
		if (af->type == spell && (!spell_ward_is_managed(af) || spell_ward_is_active(af)))
			return true;
	return false;
}
static affected_type *find(P_char ch, int spell, int source)
{
	for (auto af = ch->affected; af; af = af->next)
		if (af->type == spell && af->ward_source_type == source)
			return af;
	return nullptr;
}
static void clear(P_char ch)
{
	while (ch->affected)
		affect_remove(ch, ch->affected);
	while (ch->nevents)
		nevent_cancel(nevent_handle_from_event(ch->nevents));
}
static void advance(P_char ch, unsigned long long target)
{
	while (true)
	{
		P_nevent next = nullptr;
		for (auto ev = ch->nevents; ev; ev = ev->next_char_nev)
			if (ev->due_tick <= target && (!next || ev->due_tick < next->due_tick))
				next = ev;
		if (!next)
			break;
		ne_event_tick = next->due_tick;
		current_nevent = next;
		next->func(ch, next->victim, next->obj, next->data);
		current_nevent = nullptr;
		nevent_cancel(nevent_handle_from_event(next));
	}
	ne_event_tick = target;
	spell_ward_sync_timers(ch);
}
static affected_type *cast(P_char ch, int spell, int ticks)
{
	affected_type prototype = {};
	prototype.type = spell;
	if (spell == SPELL_MINOR_GLOBE)
		prototype.bitvector = AFF_MINOR_GLOBE;
	if (spell == SPELL_GLOBE)
		prototype.bitvector2 = AFF2_GLOBE;
	if (spell == SPELL_SPIRIT_WARD)
		prototype.bitvector3 = AFF3_SPIRIT_WARD;
	if (spell == SPELL_GREATER_SPIRIT_WARD)
		prototype.bitvector3 = AFF3_GR_SPIRIT_WARD;
	return spell_ward_apply_cast(ch, &prototype, ticks);
}

int main()
{
	char_data attacker{}, victim{}, second{};
	for (int spell :
	     { SPELL_MINOR_GLOBE, SPELL_SPIRIT_WARD, SPELL_GREATER_SPIRIT_WARD, SPELL_GLOBE })
	{
		unsigned int flag = spell == SPELL_MINOR_GLOBE	       ? SPLDAM_MINORGLOBE :
				    spell == SPELL_SPIRIT_WARD	       ? SPLDAM_SPIRITWARD :
				    spell == SPELL_GREATER_SPIRIT_WARD ? SPLDAM_GRSPIRIT :
									 SPLDAM_GLOBE;
		auto af = cast(&victim, spell, 6);
		const auto full = af->ward_capacity;
		assert(spell_ward_absorb(&victim, &victim, 20, flag).blocked == 0);
		assert(spell_ward_absorb(&attacker, &victim, 0, flag).blocked == 0);
		assert(spell_ward_absorb(&attacker, &victim, 20, 0).blocked == 0);
		assert(af->ward_capacity == full);
		for (int i = 0; i < 100; ++i)
			assert(spell_ward_absorb(&attacker, &victim, 0.25, flag).fully_blocked);
		assert(af->ward_capacity == full - 25 * SPELL_WARD_CAPACITY_SCALE);
		assert(af->duration < af->ward_full_duration);
		cast(&victim, spell, 6);
		assert(af->ward_capacity == full && af->duration == af->ward_full_duration);
		advance(&victim, ne_event_tick + 3 * PULSES_IN_TICK);
		assert(af->ward_capacity == full / 2);
		const auto result = spell_ward_absorb(
			&attacker, &victim, double(full) / SPELL_WARD_CAPACITY_SCALE, flag);
		assert(result.blocked == double(full / 2) / SPELL_WARD_CAPACITY_SCALE);
		assert(result.remaining == result.blocked && !victim.affected);
		cast(&victim, spell, 6);
		advance(&victim, ne_event_tick + 6 * PULSES_IN_TICK);
		assert(!victim.affected);
	}
	auto minor = cast(&victim, SPELL_MINOR_GLOBE, 6);
	for (int i = 0; i < 900; ++i)
		assert(spell_ward_absorb(&attacker, &victim, 1, SPLDAM_MINORGLOBE).fully_blocked);
	assert(!victim.affected);
	minor = cast(&victim, SPELL_MINOR_GLOBE, 6);
	auto greater = cast(&victim, SPELL_GREATER_SPIRIT_WARD, 56);
	const auto minor_full = minor->ward_capacity;
	assert(greater->ward_capacity == 3600 * SPELL_WARD_CAPACITY_SCALE);
	spell_ward_absorb(&attacker, &victim, 4000, SPLDAM_GRSPIRIT | SPLDAM_MINORGLOBE);
	assert(minor->ward_capacity ==
	       minor_full); // Same-hit overflow never charges a second ward.
	clear(&victim);

	obj_data gear{}, duplicate{};
	gear.obj_uid = 10;
	gear.bitvector2 = AFF2_GLOBE;
	gear.type = ITEM_ARMOR;
	duplicate = gear;
	duplicate.obj_uid = 20;
	victim.equipment[WEAR_BODY] = &gear;
	spell_ward_equipment_sync(&victim);
	auto equipment = find(&victim, SPELL_GLOBE, SPELL_WARD_SOURCE_EQUIPMENT);
	assert(equipment && IS_AFFECTED2(&victim, AFF2_GLOBE));
	const auto start = ne_event_tick;
	const int interval = 4 * PULSES_IN_TICK;
	spell_ward_absorb(&attacker, &victim, 4000, SPLDAM_GLOBE);
	assert(!spell_ward_is_active(equipment) &&
	       !spell_ward_item_callback_allowed(&victim, SPELL_GLOBE));
	victim.equipment[WEAR_BODY] = nullptr;
	spell_ward_equipment_sync(&victim);
	victim.equipment[WEAR_BODY] = &duplicate;
	spell_ward_equipment_sync(&victim);
	assert(equipment->ward_source_uid == 20 && equipment->ward_capacity == 0);
	advance(&victim, start + interval - 1);
	assert(!spell_ward_is_active(equipment));
	spell_globe(56, &attacker, nullptr, 0, &victim, nullptr);
	assert(equipment->ward_refresh_remaining == 1);
	advance(&victim, start + interval);
	assert(spell_ward_is_active(equipment) &&
	       equipment->ward_capacity == equipment->ward_capacity_max);
	for (int i = 1; i <= 4; ++i)
	{
		advance(&victim, start + (i + 1) * interval - 1);
		assert(spell_ward_is_active(equipment));
		advance(&victim, start + (i + 1) * interval);
		assert(equipment->ward_capacity == equipment->ward_capacity_max);
	}
	clear(&victim);
	victim.equipment[WEAR_BODY] = nullptr;

	// Ordinary affect ticks must retain an expired equipment pool until its
	// original renewal deadline, and leave an unequipped pool's lifetime paused.
	victim.equipment[WEAR_BODY] = &gear;
	spell_ward_equipment_sync(&victim);
	equipment = find(&victim, SPELL_GLOBE, SPELL_WARD_SOURCE_EQUIPMENT);
	const auto renewal = ne_event_tick + equipment->ward_refresh_remaining;
	spell_ward_absorb(&attacker, &victim, 3199, SPLDAM_GLOBE);
	advance(&victim, ne_event_tick + equipment->duration);
	assert(equipment->duration == 0 && equipment->ward_capacity == 0);
	character_list = &victim;
	affect_update();
	assert(find(&victim, SPELL_GLOBE, SPELL_WARD_SOURCE_EQUIPMENT) == equipment);
	spell_ward_equipment_sync(&victim);
	assert(!spell_ward_is_active(equipment));
	advance(&victim, renewal);
	assert(spell_ward_is_active(equipment));
	victim.equipment[WEAR_BODY] = nullptr;
	spell_ward_equipment_sync(&victim);
	const int paused_duration = equipment->duration;
	const int paused_refresh = equipment->ward_refresh_remaining;
	for (int tick = 0; tick < 10; ++tick)
	{
		advance(&victim, ne_event_tick + PULSES_IN_TICK);
		affect_update();
	}
	assert(equipment->duration == paused_duration);
	assert(equipment->ward_refresh_remaining == paused_refresh);
	character_list = nullptr;
	clear(&victim);

	// Equipment can precede or follow a cast in the affect list. In either
	// order, a successful dispel removes only the cast and preserves the cycle.
	attacker.specials.position = victim.specials.position = POS_STANDING | STAT_NORMAL;
	for (bool equipment_first : { false, true })
	{
		if (!equipment_first)
			cast(&victim, SPELL_GLOBE, 8);
		victim.equipment[WEAR_BODY] = &gear;
		spell_ward_equipment_sync(&victim);
		equipment = find(&victim, SPELL_GLOBE, SPELL_WARD_SOURCE_EQUIPMENT);
		if (equipment_first)
			cast(&victim, SPELL_GLOBE, 8);
		const auto capacity = equipment->ward_capacity;
		const int refresh = equipment->ward_refresh_remaining;
		spell_dispel_magic(60, &attacker, nullptr, SPELL_TYPE_SPELL, &victim, nullptr);
		assert(!find(&victim, SPELL_GLOBE, SPELL_WARD_SOURCE_CAST));
		assert(find(&victim, SPELL_GLOBE, SPELL_WARD_SOURCE_EQUIPMENT) == equipment);
		assert(equipment->ward_capacity == capacity &&
		       equipment->ward_refresh_remaining == refresh);
		clear(&victim);
		victim.equipment[WEAR_BODY] = nullptr;
	}

	// Score identifies both sources even if the broken equipment pool is first.
	cast(&victim, SPELL_GLOBE, 8);
	victim.equipment[WEAR_BODY] = &gear;
	spell_ward_equipment_sync(&victim);
	equipment = find(&victim, SPELL_GLOBE, SPELL_WARD_SOURCE_EQUIPMENT);
	spell_ward_expire(&victim, equipment);
	char status[1024];
	spell_ward_status(&victim, status, sizeof(status));
	assert(std::strstr(status, "Globe cast:") &&
	       std::strstr(status, "Globe equipment: broken"));
	victim.equipment[WEAR_BODY] = nullptr;
	spell_ward_equipment_sync(&victim);
	spell_ward_status(&victim, status, sizeof(status));
	assert(std::strstr(status, "Globe equipment: paused"));
	clear(&victim);

	// An odd source duration schedules a fractional-tick half interval.
	spirit_item_ticks = 15;
	gear.bitvector2 = 0;
	gear.bitvector3 = AFF3_SPIRIT_WARD;
	victim.equipment[WEAR_BODY] = &gear;
	spell_ward_equipment_sync(&victim);
	equipment = find(&victim, SPELL_SPIRIT_WARD, SPELL_WARD_SOURCE_EQUIPMENT);
	assert(equipment->ward_refresh_remaining == 15 * PULSES_IN_TICK / 2);
	const auto fractional_due = ne_event_tick + equipment->ward_refresh_remaining;
	advance(&victim, fractional_due - 1);
	assert(equipment->ward_capacity < equipment->ward_capacity_max);
	advance(&victim, fractional_due);
	assert(equipment->ward_capacity == equipment->ward_capacity_max);
	clear(&victim);
	victim.equipment[WEAR_BODY] = nullptr;
	spirit_item_ticks = 0;
	gear.bitvector3 = 0;
	gear.R_num = 1;
	indexes[1].func.obj = vapor;
	victim.equipment[WEAR_BODY] = &gear;
	spell_ward_equipment_sync(&victim);
	assert(find(&victim, SPELL_GLOBE, SPELL_WARD_SOURCE_EQUIPMENT));
	spell_ward_absorb(&attacker, &victim, 9999, SPLDAM_GLOBE);
	assert(!spell_ward_item_callback_allowed(&victim, SPELL_GLOBE));
	clear(&victim);
	victim.equipment[WEAR_BODY] = nullptr;
	attacker.specials.position = victim.specials.position = POS_STANDING | STAT_NORMAL;
	attacker.player.m_class = CLASS_SHAMAN;
	spell_spirit_ward(56, &attacker, nullptr, 0, &victim, nullptr);
	assert(victim.affected->duration == 56 * PULSES_IN_TICK);
	assert(victim.affected->ward_capacity == 2250 * SPELL_WARD_CAPACITY_SCALE);
	spell_spirit_ward(56, &attacker, nullptr, 0, &victim, nullptr);
	assert(!victim.affected->next);
	clear(&victim);
	spell_greater_spirit_ward(56, &attacker, nullptr, 0, &victim, nullptr);
	assert(victim.affected->duration == 56 * PULSES_IN_TICK);
	assert(victim.affected->ward_capacity == 3600 * SPELL_WARD_CAPACITY_SCALE);
	clear(&victim);
	attacker.player.m_class = CLASS_CONJURER;
	group_list leader{}, follower{};
	leader.ch = &victim;
	leader.next = &follower;
	follower.ch = &second;
	attacker.group = &leader;
	spell_group_globe(56, &attacker, nullptr, 0, nullptr, nullptr);
	spell_ward_absorb(&attacker, &victim, 100, SPLDAM_GLOBE);
	spell_group_globe(56, &attacker, nullptr, 0, nullptr, nullptr);
	assert(victim.affected->duration == 15 * PULSES_IN_TICK);
	assert(second.affected->duration == 15 * PULSES_IN_TICK);
	assert(victim.affected->ward_capacity == second.affected->ward_capacity);
	clear(&victim);
	clear(&second);

	for (uint32_t schema :
	     { PLAYER_SNAPSHOT_SCHEMA_VERSION, PLAYER_SNAPSHOT_CRAFT_RECEIPT_SCHEMA_VERSION })
	{
		player_snapshot snapshot{};
		snapshot.schema_version = schema;
		snapshot.pid = 42;
		snapshot.revision = 5;
		snapshot.components = PLAYER_CHECKPOINT_COMPONENT_ALL;
		snapshot.encoded_size_bound = PLAYER_SNAPSHOT_MAX_BYTES;
		if (schema == PLAYER_SNAPSHOT_CRAFT_RECEIPT_SCHEMA_VERSION)
		{
			player_craft_receipt_snapshot receipt{};
			receipt.operation_id.bytes[0] = 1;
			receipt.discipline = 1;
			receipt.experience = 3;
			snapshot.craft_receipts.push_back(receipt);
		}
		player_affect_snapshot row{};
		row.type = SPELL_GLOBE;
		row.duration = 900;
		row.flags = AFFTYPE_SPELL_WARD;
		row.ward_source_type = SPELL_WARD_SOURCE_EQUIPMENT;
		row.ward_source_uid = 20;
		row.ward_full_duration = 2400;
		row.ward_capacity = 0;
		row.ward_capacity_max = 3200 * SPELL_WARD_CAPACITY_SCALE;
		row.ward_refresh_remaining = 123;
		row.ward_source_worn = 1;
		snapshot.affects.push_back(row);
		std::vector<uint8_t> bytes;
		assert(player_snapshot_encode(snapshot, &bytes) ==
		       player_snapshot_codec_result::ok);
		assert(bytes[0] == schema + PLAYER_SNAPSHOT_WARD_WIRE_OFFSET);
		player_snapshot restored;
		assert(player_snapshot_decode(bytes.data(), bytes.size(), &restored) ==
		       player_snapshot_codec_result::ok);
		assert(restored.schema_version == schema);
		assert(restored.affects[0].ward_capacity == 0 &&
		       restored.affects[0].ward_refresh_remaining == 123);
		assert(restored.affects[0].ward_source_uid == 20);
		assert(restored.craft_receipts.size() == snapshot.craft_receipts.size());
	}
	std::cout
		<< "ward damage, fractional wear, expiry, overlap, recast, group, equipment cycles and snapshot recovery passed\n";
}
