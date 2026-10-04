#include "combat/spell_wards.h"
#include "combat/damage.h"
#include "cmd/interp.h"
#include "core/prototypes.h"
#include "core/utils.h"
#include "magic/spells.h"
#include "net/comm.h"
#include "player/player_snapshot.h"
#include "player/player_snapshot_codec.h"
#include "world/events.h"
#include "world/falling.h"
#include "world/specs.prototypes.h"
#include "world/vnum.obj.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

// Production ward/spell/codec units run against deterministic event and world
// services. These fixtures do not claim to exercise SQL or the network loop.
unsigned long long ne_event_tick = 100;
P_nevent current_nevent = nullptr;
index_data indexes[8] = {};
P_index obj_index = indexes;
int top_of_objt = 7;
P_char character_list = nullptr;
P_room world = nullptr;
extern const int top_of_world = 1;
extern const int rev_dir[] = { 2, 3, 0, 1, 5, 4, 8, 9, 6, 7 };
extern const racial_data_type racial_data[LAST_RACE + 1] = {};
Skill skills[MAX_SKILLS] = {};
static bool dispel_consent = true;
static bool dispel_save = false;
static bool dispel_resist = false;
static int dispel_save_checks = 0;
static int dispel_damage_rolls = 0;
static int portal_coin = 0;
static int item_destroy_roll = 0;
static int legacy_dispel_calls = 0;
static P_char anchor_owner = nullptr;
static P_obj portal_action_object = nullptr;
static int portal_action_command = 0;
static std::vector<std::pair<int, int>> number_requests;
static std::map<P_char, std::string> transcript;
static std::set<P_obj> decayed_objects;
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
	return dispel_consent;
}
bool NewSaves(P_char, int, int)
{
	++dispel_save_checks;
	return dispel_save;
}
bool resists_spell(P_char, P_char)
{
	return dispel_resist;
}
void logit(const char *, const char *, ...) {}
void debug(const char *, ...) {}
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
int invoke_object_special(P_obj obj, P_char ch, int cmd, char *arg)
{
	if (cmd == CMD_DISPEL)
		++legacy_dispel_calls;
	return obj_index[obj->R_num].func.obj(obj, ch, cmd, arg);
}
int real_object(int vnum)
{
	for (int i = 0; i <= top_of_objt; ++i)
		if (indexes[i].virtual_number == vnum)
			return i;
	return -1;
}
int real_room(int vnum)
{
	for (int i = 0; i <= top_of_world; ++i)
		if (world[i].number == vnum)
			return i;
	return NOWHERE;
}
int number(int low, int high)
{
	number_requests.emplace_back(low, high);
	if (low == 0 && high == 1)
		return portal_coin;
	if (low == 0 && high == 4)
		return item_destroy_roll;
	return low;
}
int is_Raidable(P_char, char *, int)
{
	return false; // Portal actions must remain available to non-raidable characters.
}
char *get_player_name_from_pid(int pid)
{
	static char name[] = "anchor owner";
	return anchor_owner && GET_PID(anchor_owner) == pid ? name : nullptr;
}
P_char get_char_online(char *, bool)
{
	return anchor_owner;
}
P_char find_player_by_pid(int pid)
{
	return anchor_owner && GET_PID(anchor_owner) == pid ? anchor_owner : nullptr;
}
obj_affect *get_obj_affect(P_obj obj, int type)
{
	for (auto af = obj->affects; af; af = af->next)
		if (af->type == type)
			return af;
	return nullptr;
}
void set_obj_affected(P_obj obj, int duration, sh_int spell, sh_int)
{
	auto af = new obj_affect{};
	af->type = spell;
	af->next = obj->affects;
	obj->affects = af;
	add_event(event_obj_affect, duration, nullptr, nullptr, obj, 0, &af, sizeof(af));
}
int portal_general_internal(P_obj obj, P_char, int cmd, char *, portal_action_messages *messages)
{
	assert(cmd != CMD_DISPEL);
	assert(messages);
	portal_action_object = obj;
	portal_action_command = cmd;
	return TRUE;
}
int dice(int count, int sides)
{
	assert(count > 0 && sides == 6);
	++dispel_damage_rolls;
	return count * 3;
}
void Decay(P_obj obj)
{
	assert(decayed_objects.insert(obj).second);
	if (OBJ_ROOM(obj) && obj_index[obj->R_num].func.obj)
		invoke_object_special(obj, nullptr, CMD_DECAY, nullptr);
	for (auto timer = obj->nevents; timer;)
	{
		auto next = timer->next_obj_nev;
		if (timer != current_nevent)
			nevent_cancel(nevent_handle_from_event(timer));
		timer = next;
	}
	if (OBJ_ROOM(obj))
	{
		auto &room = world[obj->loc.room];
		P_obj *link = &room.contents;
		while (*link && *link != obj)
			link = &(*link)->next_content;
		assert(*link == obj);
		*link = obj->next_content;
		if (obj->R_num == real_object(VOBJ_WALLS))
			room.dir_option[obj->value[1]]->exit_info &= ~(EX_WALLED | EX_BREAKABLE);
	}
}
void event_obj_affect(P_char, P_char, P_obj obj, void *)
{
	Decay(obj);
}
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
void act(const char *message, int, P_char actor, P_obj, void *target, int audience)
{
	if (audience == TO_CHAR)
		transcript[actor] += message;
	else if (audience == TO_VICT)
		transcript[static_cast<P_char>(target)] += message;
}
void do_point(P_char, P_char) {}
void send_to_char(const char *message, P_char ch)
{
	transcript[ch] += message;
}
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
	if (ch)
	{
		event->next_char_nev = ch->nevents;
		ch->nevents = event;
	}
	else
	{
		assert(obj);
		event->next_obj_nev = obj->nevents;
		obj->nevents = event;
	}
	return { nevent_schedule_status::scheduled, { event, 1 } };
}
nevent_handle nevent_handle_from_event(P_nevent event)
{
	return { event, 1 };
}
nevent_cancel_result nevent_cancel(nevent_handle handle)
{
	auto event = handle.event;
	P_nevent *link = event->ch ? &event->ch->nevents : &event->obj->nevents;
	while (*link && *link != event)
		link = event->ch ? &(*link)->next_char_nev : &(*link)->next_obj_nev;
	assert(*link == event);
	*link = event->ch ? event->next_char_nev : event->next_obj_nev;
	std::free(event->data);
	delete event;
	return nevent_cancel_result::canceled;
}
int ne_event_time(P_nevent event)
{
	return event->due_tick > ne_event_tick ? static_cast<int>(event->due_tick - ne_event_tick) :
						 0;
}
bool nevent_reschedule_after(nevent_handle handle, unsigned long long delay)
{
	handle.event->due_tick = ne_event_tick + delay;
	return true;
}
void event_short_affect(P_char ch, P_char, P_obj, void *data)
{
	auto af = static_cast<event_short_affect_data *>(data)->af;
	if (spell_ward_is_managed(af))
		spell_ward_expire(ch, af);
	else
		affect_remove(ch, af);
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

static void test_dispel_durations(P_char caster, P_char victim)
{
	skills[SPELL_ARMOR].name = "armor";
	skills[SPELL_HASTE].name = "haste";
	dispel_consent = false;
	for (bool saved : { true, false })
	{
		dispel_save = saved;
		dispel_resist = !saved;
		affected_type prototype{};
		prototype.type = SPELL_ARMOR;
		prototype.duration = 30;
		auto first = affect_to_char(victim, &prototype);
		prototype.type = SPELL_HASTE;
		prototype.duration = 9;
		auto haste = affect_to_char(victim, &prototype);
		prototype.type = SPELL_ARMOR;
		auto second = affect_to_char(victim, &prototype);
		dispel_save_checks = 0;
		transcript.clear();
		spell_dispel_magic(56, caster, nullptr, SPELL_TYPE_SPELL, victim, nullptr);
		assert(dispel_save_checks == 2); // One check covers even non-adjacent rows.
		assert(first->duration == 27 && second->duration == 8 && haste->duration == 8);
		assert(transcript[caster].find("armor") != std::string::npos);
		assert(transcript[victim].find("haste") != std::string::npos);
		assert(transcript[caster].find("fail miserably") == std::string::npos);
		dispel_save = dispel_resist = false;
		spell_dispel_magic(56, caster, nullptr, SPELL_TYPE_SPELL, victim, nullptr);
		assert(!victim->affected); // Removal must not leave a dangling iteration pointer.
	}

	dispel_save = true;
	affected_type prototype{};
	prototype.type = SPELL_ARMOR;
	prototype.duration = 100 * PULSES_IN_TICK;
	prototype.flags = AFFTYPE_SHORT;
	auto timed = affect_to_char(victim, &prototype);
	// The stored duration is stale; wear must use the actual remaining event time.
	advance(victim, ne_event_tick + 80 * PULSES_IN_TICK);
	spell_dispel_magic(56, caster, nullptr, SPELL_TYPE_SPELL, victim, nullptr);
	assert(timed->duration == 18 * PULSES_IN_TICK);
	assert(ne_event_time(victim->nevents) == 18 * PULSES_IN_TICK);
	advance(victim, ne_event_tick + 18 * PULSES_IN_TICK - 1);
	assert(victim->affected == timed);
	advance(victim, ne_event_tick + 1);
	assert(!victim->affected && !victim->nevents);

	for (int flags : { 0, static_cast<int>(AFFTYPE_SHORT) })
	{
		prototype.flags = flags;
		prototype.duration = flags ? PULSES_IN_TICK / 2 : 1;
		affect_to_char(victim, &prototype);
		transcript.clear();
		spell_dispel_magic(56, caster, nullptr, SPELL_TYPE_SPELL, victim, nullptr);
		assert(!victim->affected && !victim->nevents);
		assert(transcript[caster].find("exhausts") != std::string::npos);
		assert(transcript[victim].find("exhausts") != std::string::npos);
	}
	prototype.flags = AFFTYPE_NODISPEL;
	prototype.duration = 10;
	auto protected_spell = affect_to_char(victim, &prototype);
	prototype.type = SPELL_HASTE;
	prototype.flags = 0;
	prototype.duration = -1;
	auto permanent = affect_to_char(victim, &prototype);
	dispel_save_checks = 0;
	spell_dispel_magic(56, caster, nullptr, SPELL_TYPE_SPELL, victim, nullptr);
	assert(dispel_save_checks == 1);
	assert(permanent->duration == -1 && protected_spell->duration == 10);
	clear(victim);
	dispel_consent = true;
	dispel_save = false;
}

static void test_dispel_walls(P_char caster)
{
	room_data rooms[2]{};
	room_direction_data exits[2]{};
	pc_only_data pc{};
	pc.pid = 777;
	caster->only.pc = &pc;
	world = rooms;
	rooms[0].number = 101;
	rooms[1].number = 102;
	rooms[0].dir_option[0] = &exits[0];
	rooms[1].dir_option[2] = &exits[1];
	exits[0].to_room = 1;
	exits[1].to_room = 0;
	for (int kind = WALL_OF_FLAMES; kind <= WALL_OF_AIR; ++kind)
	{
		if (kind == WALL_OUTPOST)
			continue; // The untimed outpost case below covers its one-point wear.
		for (int trial : { 0, 1, 2 })
		{
			obj_data walls[2]{}, unrelated{};
			obj_affect expiry[2]{};
			decayed_objects.clear();
			for (int i = 0; i < 2; ++i)
			{
				walls[i].loc_p = LOC_ROOM;
				walls[i].loc.room = i;
				walls[i].value[0] = rooms[1 - i].number;
				walls[i].value[1] = i ? 2 : 0;
				walls[i].value[2] = trial == 2 ? 10 : 400;
				walls[i].value[3] = kind;
				walls[i].value[4] = 100;
				walls[i].value[5] = trial == 1 ? pc.pid : 123;
				rooms[i].contents = &walls[i];
				exits[i].exit_info = EX_WALLED | EX_BREAKABLE;
				expiry[i].type = TAG_OBJ_DECAY;
				walls[i].affects = &expiry[i];
				auto payload = &expiry[i];
				add_event(event_obj_affect, 1800, nullptr, nullptr, &walls[i], 0,
					  &payload, sizeof(payload));
			}
			unrelated = walls[1];
			unrelated.nevents = nullptr;
			unrelated.affects = nullptr;
			unrelated.value[5] = 999;
			unrelated.next_content = &walls[1];
			rooms[1].contents = &unrelated;
			caster->in_room = 0;
			transcript.clear();
			spell_dispel_magic(56, caster, nullptr, SPELL_TYPE_SPELL, nullptr,
					   &walls[0]);
			if (trial == 0)
			{
				assert(decayed_objects.empty());
				assert(walls[0].value[2] == 386 && walls[1].value[2] == 386);
				assert(ne_event_time(walls[0].nevents) == 1500);
				assert(ne_event_time(walls[1].nevents) == 1500);
				assert(transcript[caster].find("weakens") != std::string::npos);
				ne_event_tick += 100;
				caster->in_room = 1;
				spell_dispel_magic(56, caster, nullptr, SPELL_TYPE_SPELL, nullptr,
						   &walls[1]);
				assert(walls[0].value[2] == 372 && walls[1].value[2] == 372);
				assert(ne_event_time(walls[0].nevents) == 1100 &&
				       ne_event_time(walls[1].nevents) == 1100);
				nevent_reschedule_after(nevent_handle_from_event(walls[0].nevents),
							100);
				nevent_reschedule_after(nevent_handle_from_event(walls[1].nevents),
							100);
				spell_dispel_magic(56, caster, nullptr, SPELL_TYPE_SPELL, nullptr,
						   &walls[1]);
			}
			assert(decayed_objects.size() == 2);
			assert(decayed_objects.count(&walls[0]) &&
			       decayed_objects.count(&walls[1]));
			assert(!walls[0].nevents && !walls[1].nevents);
			assert(!(exits[0].exit_info & (EX_WALLED | EX_BREAKABLE)));
			assert(!(exits[1].exit_info & (EX_WALLED | EX_BREAKABLE)));
			assert(rooms[1].contents == &unrelated);
			assert(transcript[caster].find(trial == 1 ? "dispels" : "breaks") !=
			       std::string::npos);
		}
	}
	// Untimed physical walls retain exactly their legacy strength damage;
	// Dispel Magic does not create a decay event or add another damage packet.
	for (int kind : { WALL_OUTPOST, WALL_OF_STONE })
	{
		obj_data walls[2]{};
		decayed_objects.clear();
		number_requests.clear();
		for (int i = 0; i < 2; ++i)
		{
			walls[i].loc_p = LOC_ROOM;
			walls[i].loc.room = i;
			walls[i].value[0] = rooms[1 - i].number;
			walls[i].value[1] = i ? 2 : 0;
			walls[i].value[2] = 400;
			walls[i].value[3] = kind;
			walls[i].value[4] = 100;
			walls[i].value[5] = 123;
			rooms[i].contents = &walls[i];
		}
		spell_dispel_magic(56, caster, nullptr, SPELL_TYPE_SPELL, nullptr, &walls[0]);
		const int legacy_damage = kind == WALL_OUTPOST ? 1 : 14;
		assert(walls[0].value[2] == 400 - legacy_damage);
		assert(walls[1].value[2] == 400 - legacy_damage);
		assert(!walls[0].nevents && !walls[1].nevents && decayed_objects.empty());
		assert(number_requests.size() == (kind == WALL_OUTPOST ? 1 : 2));
	}
	obj_data mundane{};
	mundane.R_num = 7;
	mundane.affected[0].modifier = 12;
	number_requests.clear();
	spell_dispel_magic(56, caster, nullptr, SPELL_TYPE_SPELL, nullptr, &mundane);
	assert(mundane.affected[0].modifier == 12 && !mundane.nevents);
	assert(number_requests.empty() && decayed_objects.empty());
	caster->only.pc = nullptr;
	caster->in_room = 0;
	world = nullptr;
}

static void test_dispel_portals(P_char caster)
{
	room_data rooms[2]{};
	world = rooms;
	rooms[0].number = 101;
	rooms[1].number = 102;
	const int old_level = GET_LEVEL(caster);
	for (int kind : { 2, 3, 4 })
	{
		for (int trial = 0; trial < 11; ++trial)
		{
			obj_data portals[2]{}, unrelated{}, duplicate{};
			obj_affect expiry[2]{};
			decayed_objects.clear();
			transcript.clear();
			number_requests.clear();
			legacy_dispel_calls = 0;
			portal_coin = trial == 2 ? 1 : 0;
			caster->player.level = trial == 0			       ? 45 :
					       trial == 1 || trial == 9 || trial == 10 ? 46 :
					       trial == 2			       ? 49 :
											 50;
			for (int i = 0; i < 2; ++i)
			{
				portals[i].R_num = kind;
				portals[i].loc_p = LOC_ROOM;
				portals[i].loc.room = i;
				portals[i].extra2_flags = ITEM2_MAGIC;
				portals[i].value[0] = rooms[1 - i].number;
				portals[i].value[1] = RACE_HUMAN;
				portals[i].value[2] = 20;
				portals[i].value[3] = 56;
				portals[i].value[4] = 3;
				portals[i].value[5] = 2;
				portals[i].value[6] = 1;
				portals[i].value[7] = 777;
				portals[i].timer[0] = 999;
				portals[i].timer[1] = 1000;
				rooms[i].contents = &portals[i];
				if (trial != 9)
				{
					expiry[i].type = TAG_OBJ_DECAY;
					portals[i].affects = &expiry[i];
					auto af = &expiry[i];
					add_event(event_obj_affect,
						  trial == 10 ? 1 :
						  i	      ? 160 :
								120,
						  nullptr, nullptr, &portals[i], 0, &af,
						  sizeof(af));
				}
			}
			unrelated = portals[1];
			unrelated.nevents = nullptr;
			unrelated.affects = nullptr;
			unrelated.value[7] = 888;
			unrelated.next_content = &portals[1];
			rooms[1].contents = &unrelated;
			if (trial == 5)
				portals[0].value[0] =
					9999; // Invalid destination must not index world[-1].
			if (trial == 6)
				portals[1].value[0] =
					9999; // Same ID without a reciprocal link is not a pair.
			if (trial == 7)
				unrelated.next_content =
					nullptr; // Missing counterpart must not fall through.
			if (trial == 8)
			{
				duplicate = portals[1];
				duplicate.nevents = nullptr;
				duplicate.affects = nullptr;
				duplicate.next_content = rooms[1].contents;
				rooms[1].contents = &duplicate;
			}
			// The old thresholds depend on character level, not the spell's level.
			spell_dispel_magic(1, caster, nullptr, SPELL_TYPE_SPELL, nullptr,
					   &portals[0]);
			const bool blocked = trial == 0 || (trial >= 5 && trial <= 8) || trial == 9;
			if (blocked)
			{
				assert(decayed_objects.empty());
				if (trial != 9)
				{
					assert(ne_event_time(portals[0].nevents) == 120);
					assert(ne_event_time(portals[1].nevents) == 160);
				}
				else
					assert(!portals[0].nevents && !portals[1].nevents);
			}
			else if (trial == 1)
			{
				assert(decayed_objects.empty());
				assert(ne_event_time(portals[0].nevents) == 108);
				assert(ne_event_time(portals[1].nevents) == 108);
				assert(transcript[caster].find("shortening") != std::string::npos);
				ne_event_tick += 10;
				spell_dispel_magic(56, caster, nullptr, SPELL_TYPE_SPELL, nullptr,
						   &portals[1]);
				assert(ne_event_time(portals[0].nevents) == 88);
				assert(ne_event_time(portals[1].nevents) == 88);
			}
			else
			{
				assert(decayed_objects.size() == 2);
				assert(decayed_objects.count(&portals[0]) &&
				       decayed_objects.count(&portals[1]));
				assert(!portals[0].nevents && !portals[1].nevents);
				assert(transcript[caster].find(trial == 10 ? "exhausts" :
									     "dispels") !=
				       std::string::npos);
			}
			assert(!decayed_objects.count(&unrelated) &&
			       !decayed_objects.count(&duplicate));
			assert(legacy_dispel_calls == 0);
			for (auto &portal : portals)
			{
				assert(portal.extra2_flags == ITEM2_MAGIC);
				assert(portal.value[2] == 20 && portal.value[3] == 56);
				assert(portal.value[4] == 3 && portal.value[5] == 2 &&
				       portal.value[6] == 1);
				assert(portal.timer[0] == 999 && portal.timer[1] == 1000);
				while (portal.nevents)
					nevent_cancel(nevent_handle_from_event(portal.nevents));
			}
			for (auto request : number_requests)
				assert(request.first == 0 && request.second == 1);
			if (GET_LEVEL(caster) == 50 || trial == 0)
				assert(number_requests.empty());
		}
	}
	caster->player.level = old_level;
	portal_coin = 0;
	world = nullptr;
}

static void test_portal_actions_without_raid_equipment()
{
	char_data traveler{};
	traveler.specials.position = POS_STANDING | STAT_NORMAL;
	assert(!is_Raidable(&traveler, nullptr, 0));
	obj_data portal{};
	char argument[] = "portal";
	for (auto handler : { portal_door, portal_wormhole, portal_etherportal })
	{
		for (int command : { CMD_ENTER, CMD_LOOK })
		{
			transcript.clear();
			portal_action_object = nullptr;
			portal_action_command = 0;
			assert(handler(&portal, &traveler, command, argument) == TRUE);
			assert(portal_action_object == &portal && portal_action_command == command);
			assert(transcript[&traveler].empty());
		}
	}
}

static void test_dispel_stone_anchors(P_char caster)
{
	room_data rooms[2]{};
	world = rooms;
	char_data owner{};
	pc_only_data pc{};
	pc.pid = 555;
	owner.only.pc = &pc;
	owner.specials.position = POS_STANDING | STAT_NORMAL;
	anchor_owner = &owner;
	for (int kind : { 5, 6 })
	{
		for (int remaining : { 3000, 300, 80, 1, -1, -2 })
		{
			obj_data anchor{};
			obj_affect expiry{};
			anchor.R_num = kind;
			anchor.loc_p = LOC_ROOM;
			anchor.loc.room = 0;
			anchor.value[0] = pc.pid;
			rooms[0].contents = &anchor;
			decayed_objects.clear();
			transcript.clear();
			number_requests.clear();
			legacy_dispel_calls = 0;
			const int spell = kind == 5 ? SPELL_MOONSTONE : SPELL_BLOODSTONE;
			affected_type tracking{};
			tracking.type = spell;
			tracking.flags = AFFTYPE_NODISPEL | AFFTYPE_NOSAVE | AFFTYPE_NOAPPLY;
			tracking.duration = 10;
			affect_to_char(&owner, &tracking);
			if (remaining != -2)
			{
				expiry.type = TAG_OBJ_DECAY;
				anchor.affects = &expiry;
				if (remaining >= 0)
				{
					auto af = &expiry;
					add_event(event_obj_affect, remaining, nullptr, nullptr,
						  &anchor, 0, &af, sizeof(af));
				}
			}
			spell_dispel_magic(56, caster, nullptr, SPELL_TYPE_SPELL, nullptr, &anchor);
			assert(legacy_dispel_calls == 0 && number_requests.empty());
			if (remaining == 1)
				assert(decayed_objects.count(&anchor));
			else
			{
				const int expected = remaining == 3000 ? 300 :
						     remaining == 300  ? 270 :
						     remaining == 80   ? 72 :
									 1;
				assert(ne_event_time(anchor.nevents) == expected);
				assert(transcript[caster].find("shortening") != std::string::npos);
				// The shortened deadline runs the normal object decay callback,
				// which notifies the owner and removes the tracking affect.
				auto timer = anchor.nevents;
				ne_event_tick = timer->due_tick;
				current_nevent = timer;
				timer->func(nullptr, nullptr, &anchor, timer->data);
				current_nevent = nullptr;
				nevent_cancel(nevent_handle_from_event(timer));
			}
			assert(!owner.affected && !anchor.nevents);
			assert(transcript[&owner].find("fades to nothingness") !=
			       std::string::npos);
			if (remaining == -2)
				delete anchor
					.affects; // Only this fixture asks set_obj_affected to allocate.
		}
		// Preserve the deliberate ordinary-enchantment continuation and its
		// one-in-five destruction roll for anchors bearing ITEM2_MAGIC.
		for (int destruction_roll : { 0, 1 })
		{
			obj_data anchor{};
			obj_affect expiry{};
			anchor.R_num = kind;
			anchor.loc_p = LOC_ROOM;
			anchor.loc.room = 0;
			anchor.value[0] = pc.pid;
			anchor.extra2_flags = ITEM2_MAGIC;
			anchor.affected[0].modifier = 12;
			rooms[0].contents = &anchor;
			expiry.type = TAG_OBJ_DECAY;
			anchor.affects = &expiry;
			auto af = &expiry;
			add_event(event_obj_affect, 3000, nullptr, nullptr, &anchor, 0, &af,
				  sizeof(af));
			decayed_objects.clear();
			item_destroy_roll = destruction_roll;
			spell_dispel_magic(56, caster, nullptr, SPELL_TYPE_SPELL, nullptr, &anchor);
			if (destruction_roll == 0)
				assert(decayed_objects.count(&anchor) && !anchor.nevents);
			else
			{
				assert(decayed_objects.empty() &&
				       ne_event_time(anchor.nevents) == 300);
				assert(!(anchor.extra2_flags & ITEM2_MAGIC) &&
				       anchor.affected[0].modifier == 0);
				nevent_cancel(nevent_handle_from_event(anchor.nevents));
			}
		}
	}
	item_destroy_roll = 0;
	anchor_owner = nullptr;
	world = nullptr;
}

void run_portal_owner_check(P_char, P_obj, bool);
static void test_portal_owner_lifetime()
{
	room_data rooms[2]{};
	rooms[0].number = 101;
	rooms[1].number = 102;
	world = rooms;
	char_data owner{};
	pc_only_data pc{};
	pc.pid = 555;
	owner.only.pc = &pc;
	anchor_owner = &owner;
	for (int trial : { 0, 1, 2, 3 })
	{
		obj_data portal{};
		portal.R_num = 2;
		portal.loc_p = LOC_ROOM;
		portal.loc.room = 0;
		portal.value[0] = 102;
		rooms[0].contents = &portal;
		decayed_objects.clear();
		owner.in_room = trial == 0 ? 0 : trial == 1 || trial == 3 ? 1 : NOWHERE;
		// The other end has disappeared; the surviving owner's check must
		// not dereference an extracted portal. Either destination remains an
		// allowed owner position until this side's own expiry runs.
		auto other = new obj_data{};
		other->R_num = 2;
		other->loc_p = LOC_ROOM;
		other->loc.room = 1;
		rooms[1].contents = other;
		Decay(other);
		delete other;
		decayed_objects.clear();
		run_portal_owner_check(&owner, &portal, trial == 3);
		if (trial < 2)
		{
			assert(decayed_objects.empty() &&
			       ne_event_time(portal.nevents) == WAIT_SEC);
			nevent_cancel(nevent_handle_from_event(portal.nevents));
		}
		else
			assert(decayed_objects.count(&portal) && !portal.nevents);
	}
	anchor_owner = nullptr;
	world = nullptr;
}

int main()
{
	indexes[0].virtual_number = VOBJ_WALLS;
	indexes[2].virtual_number = 751;
	indexes[2].func.obj = portal_door;
	indexes[3].virtual_number = 770;
	indexes[3].func.obj = portal_wormhole;
	indexes[4].virtual_number = 780;
	indexes[4].func.obj = portal_etherportal;
	indexes[5].virtual_number = 419;
	indexes[5].func.obj = moonstone;
	indexes[6].virtual_number = 433;
	indexes[6].func.obj = moonstone;
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
	// order, a successful dispel removes both protections and preserves the cycle.
	attacker.specials.position = victim.specials.position = POS_STANDING | STAT_NORMAL;
	for (int spell :
	     { SPELL_MINOR_GLOBE, SPELL_SPIRIT_WARD, SPELL_GREATER_SPIRIT_WARD, SPELL_GLOBE })
	{
		gear.bitvector = spell == SPELL_MINOR_GLOBE ? AFF_MINOR_GLOBE : 0;
		gear.bitvector2 = spell == SPELL_GLOBE ? AFF2_GLOBE : 0;
		gear.bitvector3 = spell == SPELL_SPIRIT_WARD	     ? AFF3_SPIRIT_WARD :
				  spell == SPELL_GREATER_SPIRIT_WARD ? AFF3_GR_SPIRIT_WARD :
								       0;
		for (bool equipment_first : { false, true })
		{
			dispel_consent = equipment_first; // Cover hostile successes and consent.
			dispel_save_checks = 0;
			if (!equipment_first)
				cast(&victim, spell, 8);
			victim.equipment[WEAR_BODY] = &gear;
			spell_ward_equipment_sync(&victim);
			equipment = find(&victim, spell, SPELL_WARD_SOURCE_EQUIPMENT);
			if (equipment_first)
				cast(&victim, spell, 8);
			const int refresh = equipment->ward_refresh_remaining;
			const auto source_uid = equipment->ward_source_uid;
			transcript.clear();
			spell_dispel_magic(60, &attacker, nullptr, SPELL_TYPE_SPELL, &victim,
					   nullptr);
			assert(dispel_save_checks == (dispel_consent ? 0 : 2));
			assert(!find(&victim, spell, SPELL_WARD_SOURCE_CAST));
			assert(find(&victim, spell, SPELL_WARD_SOURCE_EQUIPMENT) == equipment);
			assert(!spell_ward_is_active(equipment) && equipment->ward_capacity == 0);
			assert(equipment->ward_refresh_remaining == refresh &&
			       equipment->ward_source_uid == source_uid);
			assert(transcript[&attacker].find("dispels") != std::string::npos);
			assert(transcript[&victim].find("dispels") != std::string::npos);
			assert(transcript[&attacker].find("(cast)") != std::string::npos);
			assert(transcript[&attacker].find("(equipment)") != std::string::npos);
			all_affects(&victim, TRUE);
			assert(!spell_ward_is_active(equipment));
			assert(!spell_ward_item_callback_allowed(&victim, spell));
			advance(&victim, ne_event_tick + refresh - 1);
			assert(!spell_ward_is_active(equipment));
			advance(&victim, ne_event_tick + 1);
			assert(spell_ward_is_active(equipment));
			clear(&victim);
			victim.equipment[WEAR_BODY] = nullptr;
		}
	}
	assert(dispel_damage_rolls == 0); // A successful dispel does not roll damage wear.

	// Saving throws and spell resistance each leave partial wear on every
	// active cast/item ward. The wear matches the existing damage calculation.
	dispel_consent = false;
	for (bool saved : { true, false })
	{
		dispel_save = saved;
		dispel_resist = !saved;
		gear.bitvector = AFF_MINOR_GLOBE;
		gear.bitvector2 = AFF2_GLOBE;
		gear.bitvector3 = AFF3_SPIRIT_WARD | AFF3_GR_SPIRIT_WARD;
		victim.equipment[WEAR_BODY] = &gear;
		spell_ward_equipment_sync(&victim);
		for (int spell : { SPELL_MINOR_GLOBE, SPELL_SPIRIT_WARD, SPELL_GREATER_SPIRIT_WARD,
				   SPELL_GLOBE })
			cast(&victim, spell, 8);
		std::map<affected_type *, affected_type> before;
		for (auto af = victim.affected; af; af = af->next)
			before.emplace(af, *af);
		dispel_save_checks = dispel_damage_rolls = 0;
		transcript.clear();
		victim.points.hit = 100;
		spell_dispel_magic(56, &attacker, nullptr, SPELL_TYPE_SPELL, &victim, nullptr);
		assert(dispel_save_checks == 8 && dispel_damage_rolls == 8);
		assert(victim.points.hit == 100);
		for (const auto &[af, original] : before)
		{
			assert(spell_ward_is_active(af));
			assert(af->ward_capacity ==
			       original.ward_capacity - 120 * SPELL_WARD_CAPACITY_SCALE);
			assert(af->duration ==
			       static_cast<int>(
				       std::ceil(static_cast<long double>(af->ward_capacity) *
						 af->ward_full_duration / af->ward_capacity_max)));
			assert(af->ward_refresh_remaining == original.ward_refresh_remaining);
			assert(af->ward_source_uid == original.ward_source_uid);
		}
		assert(transcript[&attacker].find("weakens") != std::string::npos);
		assert(transcript[&victim].find("weakens") != std::string::npos);
		assert(transcript[&attacker].find("fail miserably") == std::string::npos);
		clear(&victim);
		victim.equipment[WEAR_BODY] = nullptr;
	}
	gear.bitvector = gear.bitvector3 = 0;
	dispel_save = true;
	dispel_resist = false;
	cast(&victim, SPELL_GLOBE, 8);
	spell_ward_absorb(&attacker, &victim, 3140, SPLDAM_GLOBE);
	victim.equipment[WEAR_BODY] = &gear;
	spell_ward_equipment_sync(&victim);
	equipment = find(&victim, SPELL_GLOBE, SPELL_WARD_SOURCE_EQUIPMENT);
	const int broken_refresh = equipment->ward_refresh_remaining;
	transcript.clear();
	spell_dispel_magic(56, &attacker, nullptr, SPELL_TYPE_SPELL, &victim, nullptr);
	assert(!find(&victim, SPELL_GLOBE, SPELL_WARD_SOURCE_CAST));
	assert(equipment->ward_capacity == (3200 - 120) * SPELL_WARD_CAPACITY_SCALE);
	assert(equipment->ward_refresh_remaining == broken_refresh);
	assert(victim.points.hit == 100); // Dispel wear never overflows into HP damage.
	assert(transcript[&attacker].find("breaks") != std::string::npos);
	assert(transcript[&victim].find("breaks") != std::string::npos);
	spell_ward_absorb(&attacker, &victim, 3020, SPLDAM_GLOBE);
	assert(equipment->ward_capacity == 60 * SPELL_WARD_CAPACITY_SCALE);
	transcript.clear();
	spell_dispel_magic(56, &attacker, nullptr, SPELL_TYPE_SPELL, &victim, nullptr);
	assert(equipment->ward_capacity == 0 && !spell_ward_is_active(equipment));
	assert(equipment->ward_refresh_remaining == broken_refresh);
	assert(transcript[&attacker].find("breaks") != std::string::npos);
	assert(transcript[&victim].find("breaks") != std::string::npos);
	assert(victim.points.hit == 100);
	dispel_save_checks = dispel_damage_rolls = 0;
	spell_dispel_magic(56, &attacker, nullptr, SPELL_TYPE_SPELL, &victim, nullptr);
	assert(dispel_save_checks == 0 && dispel_damage_rolls == 0);
	assert(equipment->ward_refresh_remaining == broken_refresh);
	advance(&victim, ne_event_tick + broken_refresh);
	assert(spell_ward_is_active(equipment));
	victim.equipment[WEAR_BODY] = nullptr;
	spell_ward_equipment_sync(&victim);
	spell_dispel_magic(56, &attacker, nullptr, SPELL_TYPE_SPELL, &victim, nullptr);
	assert(dispel_save_checks == 0 && dispel_damage_rolls == 0);
	assert(equipment->ward_capacity == equipment->ward_capacity_max);
	clear(&victim);
	dispel_consent = true;
	dispel_save = false;
	test_dispel_durations(&attacker, &victim);
	test_dispel_walls(&attacker);
	test_dispel_portals(&attacker);
	test_portal_actions_without_raid_equipment();
	test_dispel_stone_anchors(&attacker);
	test_portal_owner_lifetime();

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
		<< "ward durability, dispel duration/barrier wear, equipment cycles and snapshot recovery passed\n";
}
