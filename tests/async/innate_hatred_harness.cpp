// The effect pool frees nodes immediately so invalid traversals fail under ASan.
#include "world/new_events.c"
#include "classes/necromancy.h"
#include "classes/disguise.h"
#include "world/falling.h"
#include "world/weather.h"
#include "net/gmcp.h"
#include "core/files.h"
#include <cassert>
#include <string>

int pulse = 0;
bool after_events_call = FALSE;
P_char character_list = nullptr;
P_obj object_list = nullptr;
static index_data mob_indexes[1]{};
P_index mob_index = mob_indexes;
P_index obj_index = nullptr;
static room_data rooms[2]{};
P_room world = rooms;
static zone_data zones[1]{};
zone_data *zone_table = zones;
extern const int top_of_world = 1;
Skill skills[MAX_AFFECT_TYPES + 1];
time_info_data time_info{};
extern const stat_data stat_factor[256]{};
extern const racial_data_type racial_data[LAST_RACE + 1]{};
hold_data TmpAffs{};
static mm_ds event_pool{}, affect_pool{};
mm_ds *dead_affect_pool = &affect_pool;
static int timer = WAIT_SEC;
static bool timer_configured = true;
static int activations, subsides;
static int short_size = sizeof(short), int_size = sizeof(int), long_size = sizeof(long);

bool nevent_periodic_begin(P_nevent)
{
	return false;
}
void nevent_periodic_complete(P_nevent) {}
void nevent_periodic_watchdog() {}
bool nevent_periodic_event_is_valid(P_nevent)
{
	return true;
}
long nevent_periodic_integrity_errors(bool)
{
	return 0;
}
const char *event_name_registry_lookup(const void *)
{
	return nullptr;
}
void release_mob_mem(P_char, P_char, P_obj, void *) {}
void event_hit_regen(P_char, P_char, P_obj, void *) {}
void event_mana_regen(P_char, P_char, P_obj, void *) {}
void event_ward_regen(P_char, P_char, P_obj, void *) {}
void event_wait(P_char, P_char, P_obj, void *) {}
void event_move_regen(P_char, P_char, P_obj, void *) {}
void event_spellcast(P_char, P_char, P_obj, void *) {}
void event_item_action_active(P_char, P_char, P_obj, void *) {}
void event_memorize(P_char, P_char, P_obj, void *) {}
void event_balance_affects(P_char, P_char, P_obj, void *) {}
void event_mob_mundane(P_char, P_char, P_obj, void *) {}
void event_reset_zone(P_char, P_char, P_obj, void *) {}
void event_autosave(P_char, P_char, P_obj, void *) {}
void event_smite_evil(P_char, P_char, P_obj, void *) {}
void event_halfling_check(P_char, P_char, P_obj, void *) {}
void event_righteous_aura_check(P_char, P_char, P_obj, void *) {}
void event_bleak_foeman_check(P_char, P_char, P_obj, void *) {}
void event_blood_tithe(P_char, P_char, P_obj, void *) {}
void event_short_affect(P_char, P_char, P_obj, void *) {}
void event_web_spray(P_char, P_char, P_obj, void *) {}
int IS_MORPH(P_char)
{
	return FALSE;
}
void StartRegen(P_char, regen_resource) {}
int number(int low, int)
{
	return low;
}
bool char_falling(P_char)
{
	return false;
}
time_info_data age(P_char)
{
	return {};
}
int GET_PRIME_CLASS(P_char, uint)
{
	return 0;
}
int GET_SECONDARY_CLASS(P_char, uint)
{
	return 0;
}
void remove_disguise(P_char, bool)
{
	assert(false);
}
P_char un_morph(P_char)
{
	assert(false);
	return nullptr;
}
falling_start_result falling_start(P_char)
{
	assert(false);
	return falling_start_result::not_applicable;
}
int GET_CHAR_SKILL_P(P_char, int)
{
	return 0;
}
void unlink_char_affect(P_char, affected_type *)
{
	assert(false);
}
void unlink_char_obj_affect(P_char, affected_type *)
{
	assert(false);
}
void *__malloc(size_t size, const char *, const char *, int)
{
	return std::malloc(size);
}
void __free(void *memory, const char *, int)
{
	std::free(memory);
}
void *_mm_get(mm_ds *pool, const char *, int)
{
	++pool->objs_used;
	return std::calloc(1, pool == &event_pool ? sizeof(nevent_data) : sizeof(affected_type));
}
void mm_release(mm_ds *pool, void *memory)
{
	assert(pool->objs_used > 0);
	--pool->objs_used;
	std::free(memory);
}
int GET_CLASS(P_char ch, unsigned int cls)
{
	return (ch->player.m_class & cls) != 0;
}
int get_property(const char *key, int fallback)
{
	return timer_configured && !std::strcmp(key, "innate.timer.hatred") ? timer : fallback;
}
float get_property(const char *, double fallback)
{
	return fallback;
}
void act(const char *text, int, P_char, P_obj, void *, int)
{
	if (std::strstr(text, "BURN"))
		++activations;
}
void send_to_char(const char *text, P_char)
{
	if (std::strstr(text, "hatred in your heart subside"))
		++subsides;
}
void affect_modify(int, int, unsigned long *, int);
void balance_affects(P_char ch)
{
	TmpAffs = {};
	for (auto *af = ch->affected; af; af = af->next)
		affect_modify(af->location, af->modifier, &af->bitvector, FALSE);
	ch->points.combat_pulse = TmpAffs.combat_pulse;
	ch->points.spell_pulse = TmpAffs.spell_pulse;
}
void all_affects(P_char ch, int)
{
	balance_affects(ch);
}
int char_light(P_char)
{
	return 0;
}
int room_light(int, int)
{
	return 0;
}
bool IS_TWILIGHT_ROOM(int)
{
	return false;
}
bool IS_OUTDOORS(int)
{
	return false;
}
bool is_linked_to(P_char, P_char, ush_int)
{
	return false;
}
void gmcp_char_affects(P_char) {}
bool rested_bonus_effect_active(const affected_type *)
{
	assert(false && "Hatred must not query rested-bonus state");
	return false;
}
mm_ds *mm_create(const char *, size_t, size_t, unsigned int)
{
	assert(false);
	return nullptr;
}
char_link_data *link_char(P_char, P_char, ush_int)
{
	assert(false);
	return nullptr;
}
void remove_link(P_char, char_link_data *)
{
	assert(false);
}

#include "production.inc"

static void heartbeat()
{
	ne_events();
	nevent_advance_tick();
}

static int watcher_count(P_char ch)
{
	int count = 0;
	for (auto *event = ch->nevents; event; event = event->next_char_nev)
		count += event->func == event_hatred_check;
	return count;
}

struct scene
{
	char_data dwarf{}, target{};
	pc_only_data player{}, other_player{};
	scene()
	{
		dwarf.only.pc = &player;
		target.only.pc = &other_player;
		SET_POS(&dwarf, STAT_NORMAL + POS_STANDING);
		SET_POS(&target, STAT_NORMAL + POS_STANDING);
		dwarf.player.race = RACE_MOUNTAIN;
		dwarf.player.level = 21;
		dwarf.in_room = target.in_room = 0;
		dwarf.next_in_room = &target;
		rooms[0].people = &dwarf;
		target.player.race = RACE_HUMAN;
		activations = subsides = 0;
	}
	~scene()
	{
		disarm_char_nevents(&dwarf, nullptr);
		while (dwarf.affected)
			affect_remove(&dwarf, dwarf.affected);
		assert(event_pool.objs_used == 0 && affect_pool.objs_used == 0);
		rooms[0].people = nullptr;
	}
	void check()
	{
		disarm_char_nevents(&dwarf, event_hatred_check);
		event_hatred_check(&dwarf, nullptr, nullptr, nullptr);
	}
	void add(int location, int modifier = -1, int type = TAG_HATRED)
	{
		affected_type af{};
		af.type = type;
		af.flags = AFFTYPE_NOSAVE | AFFTYPE_NODISPEL;
		af.location = location;
		af.modifier = modifier;
		af.duration = -1;
		affect_to_char(&dwarf, &af);
	}
	int count()
	{
		int n = 0;
		for (auto *af = dwarf.affected; af; af = af->next)
			n += af->type == TAG_HATRED;
		return n;
	}
	void pair()
	{
		assert(count() == 2);
		assert(dwarf.points.combat_pulse == -1 && dwarf.points.spell_pulse == -1);
		for (auto *af = dwarf.affected; af; af = af->next)
			if (af->type == TAG_HATRED)
				assert(af->duration == -1 &&
				       af->flags == (AFFTYPE_NOSAVE | AFFTYPE_NODISPEL));
	}
};

int main(int argc, char **argv)
{
	assert(argc == 2);
	nevent_bind_game_thread();
	ne_dead_event_pool = &event_pool;
	assign_innates();
	skills[TAG_HATRED].wear_off_char[0] = "You feel the hatred in your heart subside.";
	if (!std::strcmp(argv[1], "bounds"))
	{
		scene s;
		s.check();
		assert(!s.count() && !activations && !subsides);
	}
	else if (!std::strcmp(argv[1], "cleanup"))
	{
		scene s;
		s.dwarf.next_in_room = nullptr;
		s.add(APPLY_COMBAT_PULSE);
		s.add(APPLY_SPELL_PULSE);
		s.check();
		assert(!s.count() && subsides == 1);
		s.check();
		assert(subsides == 1);
	}
	else if (!std::strcmp(argv[1], "partial"))
	{
		scene s;
		s.target.player.race = RACE_ORC;
		s.add(APPLY_COMBAT_PULSE);
		s.check();
		s.pair();
		assert(!activations);
	}
	else if (!std::strcmp(argv[1], "matching"))
	{
		for (int race = 0; race <= LAST_RACE + 1; ++race)
		{
			scene s;
			s.target.player.race = race;
			s.check();
			bool hated = race == RACE_ORC || race == RACE_DUERGAR || race == RACE_DROW;
			assert(s.count() == (hated ? 2 : 0) && activations == hated);
		}
		{
			scene s;
			s.target.player.race = static_cast<ubyte>(-1);
			s.check();
			assert(!s.count());
		}
		for (int race : { RACE_ORC, RACE_DUERGAR, RACE_DROW })
		{
			scene s;
			s.target.player.race = race;
			s.check();
			s.pair();
			assert(activations == 1);
			s.check();
			s.pair();
			assert(activations == 1);
			s.dwarf.next_in_room = nullptr;
			s.check();
			assert(!s.count() && subsides == 1);
			s.dwarf.next_in_room = &s.target;
			s.check();
			s.pair();
			assert(activations == 2);
		}
		scene s;
		npc_only_data npc{};
		s.target.only.npc = &npc;
		s.target.specials.act = ACT_ISNPC;
		s.target.player.race = RACE_ORC;
		s.check();
		s.pair();
		s.dwarf.next_in_room = nullptr;
		s.target.in_room = 1;
		rooms[1].people = &s.target;
		s.check();
		assert(!s.count() && subsides == 1);
		rooms[1].people = nullptr;

		// Multiple enemies and occupant order keep one pair and one activation.
		char_data other{};
		pc_only_data other_pc{};
		other.only.pc = &other_pc;
		SET_POS(&other, STAT_NORMAL + POS_STANDING);
		other.player.race = RACE_DROW;
		other.in_room = 0;
		s.target.in_room = 0;
		s.dwarf.next_in_room = &s.target;
		s.target.next_in_room = &other;
		s.check();
		s.pair();
		int announced = activations;
		s.dwarf.next_in_room = &other;
		other.next_in_room = &s.target;
		s.target.next_in_room = nullptr;
		s.check();
		s.pair();
		assert(activations == announced);
		other.next_in_room = nullptr;
		s.dwarf.next_in_room = nullptr;
	}
	else if (!std::strcmp(argv[1], "visibility"))
	{
		scene s;
		s.target.player.race = RACE_ORC;
		s.target.specials.affected_by = AFF_INVISIBLE;
		s.check();
		assert(!s.count());
		s.dwarf.specials.affected_by = AFF_DETECT_INVISIBLE;
		s.check();
		s.pair();
		s.target.specials.affected_by = AFF_HIDE;
		s.check();
		assert(!s.count() && subsides == 1);
		s.target.specials.affected_by = 0;
		s.dwarf.specials.affected_by = AFF_BLIND;
		s.check();
		assert(!s.count());
		s.dwarf.specials.affected_by = 0;
		SET_POS(&s.dwarf, STAT_SLEEPING + POS_PRONE);
		s.check();
		assert(!s.count());
		SET_POS(&s.dwarf, STAT_NORMAL + POS_STANDING);
		s.check();
		s.pair();
		assert(activations == 2);
	}
	else if (!std::strcmp(argv[1], "eligibility"))
	{
		for (int race = 0; race <= LAST_RACE; ++race)
		{
			scene s;
			s.target.player.race = RACE_ORC;
			s.dwarf.player.race = race;
			s.check();
			assert(s.count() == (race == RACE_MOUNTAIN ? 2 : 0));
		}
		scene s;
		s.target.player.race = RACE_ORC;
		s.dwarf.player.level = 20;
		s.check();
		assert(!s.count());
		s.dwarf.player.level = 21;
		s.check();
		s.pair();
		s.dwarf.disguise.active_shapechange = true;
		s.dwarf.disguise.race = RACE_HUMAN;
		s.check();
		assert(!s.count() && subsides == 1);
		s.dwarf.disguise.active_shapechange = false;
		s.check();
		s.pair();
		// A human borrowing mountain-dwarf innates still has no actual-race rule.
		s.dwarf.player.race = RACE_HUMAN;
		s.dwarf.disguise.active_shapechange = true;
		s.dwarf.disguise.race = RACE_MOUNTAIN;
		s.check();
		assert(!s.count());
		s.dwarf.disguise.active_shapechange = false;
		s.add(APPLY_NONE, RACE_MOUNTAIN, TAG_RACE_CHANGE);
		s.add(APPLY_NONE, CORPSEFORM_REG, SPELL_CORPSEFORM);
		assert(has_innate(&s.dwarf, INNATE_HATRED));
		s.check();
		assert(!s.count());
		affect_from_char(&s.dwarf, TAG_RACE_CHANGE);
		affect_from_char(&s.dwarf, SPELL_CORPSEFORM);
		s.check();
		s.pair();
		s.dwarf.in_room = NOWHERE;
		s.check();
		assert(!s.count());
		event_hatred_check(nullptr, nullptr, nullptr, nullptr);
	}
	else if (!std::strcmp(argv[1], "normalization"))
	{
		for (int location : { APPLY_COMBAT_PULSE, APPLY_SPELL_PULSE, APPLY_HITROLL })
		{
			scene s;
			s.target.player.race = RACE_ORC;
			s.add(location, -7);
			s.add(APPLY_NONE, 0, SPELL_BLESS);
			s.add(APPLY_COMBAT_PULSE);
			s.add(APPLY_COMBAT_PULSE);
			s.add(APPLY_SPELL_PULSE);
			s.check();
			s.pair();
			assert(!activations && !subsides);
			assert(get_spell_from_char(&s.dwarf, SPELL_BLESS));
			s.dwarf.next_in_room = nullptr;
			s.check();
			assert(!s.count() && subsides == 1);
			assert(get_spell_from_char(&s.dwarf, SPELL_BLESS));
			assert(s.dwarf.points.combat_pulse == 0 && s.dwarf.points.spell_pulse == 0);
		}
	}
	else if (!std::strcmp(argv[1], "scheduler"))
	{
		for (int setting : { -9, 0, 1, WAIT_SEC, 2 * PULSES_IN_TICK })
		{
			scene s;
			timer = setting;
			schedule_pc_events(&s.dwarf);
			auto *autosave = get_scheduled(&s.dwarf, event_autosave);
			assert(autosave);
			schedule_pc_events(&s.dwarf);
			start_hatred_check(&s.dwarf);
			start_hatred_check(&s.dwarf);
			assert(watcher_count(&s.dwarf) == 1);
			assert(get_scheduled(&s.dwarf, event_autosave) == autosave);
			const int delay = setting > 0 ? setting : WAIT_SEC;
			assert(get_scheduled(&s.dwarf, event_hatred_check)->due_tick ==
			       ne_event_tick + delay);
			s.dwarf.player.level = 20;
			s.target.player.race = RACE_ORC;
			for (int n = 0; n <= delay; ++n)
				heartbeat();
			assert(!s.count() && watcher_count(&s.dwarf) == 1);
			s.dwarf.player.level = 21;
			for (int n = 0; n <= delay; ++n)
				heartbeat();
			s.pair();
			assert(activations == 1 && watcher_count(&s.dwarf) == 1);
			s.dwarf.disguise.active_shapechange = true;
			s.dwarf.disguise.race = RACE_HUMAN;
			for (int n = 0; n <= delay; ++n)
				heartbeat();
			assert(!s.count() && watcher_count(&s.dwarf) == 1);
			s.dwarf.disguise.active_shapechange = false;
			for (int n = 0; n <= delay; ++n)
				heartbeat();
			s.pair();
			assert(activations == 2 && watcher_count(&s.dwarf) == 1);
		}
		timer = WAIT_SEC;
		timer_configured = false;
		{
			scene s;
			start_hatred_check(&s.dwarf);
			assert(get_scheduled(&s.dwarf, event_hatred_check)->due_tick ==
			       ne_event_tick + WAIT_SEC);
		}
		timer_configured = true;
		{
			scene s;
			s.target.player.race = RACE_ORC;
			s.check();
			s.pair();
			auto sequence = ne_event_sequence;
			ne_event_sequence = ULLONG_MAX;
			// Exercise refusal inside the queued callback, including self-cancellation.
			for (int n = 0; n <= WAIT_SEC; ++n)
				heartbeat();
			assert(!s.count() && watcher_count(&s.dwarf) == 0);
			assert(activations == 1);
			assert(subsides == 1);
			ne_event_sequence = sequence;
			SET_POS(&s.dwarf, STAT_DEAD + POS_PRONE);
			assert(!start_hatred_check(&s.dwarf));
		}
		assert(!start_hatred_check(nullptr));
		auto *owner = new char_data{};
		SET_POS(owner, STAT_NORMAL + POS_STANDING);
		assert(start_hatred_check(owner));
		disarm_char_nevents(owner, nullptr);
		delete owner;
		for (int n = 0; n <= WAIT_SEC; ++n)
			heartbeat();
		assert(event_pool.objs_used == 0);
	}
	else if (!std::strcmp(argv[1], "timing"))
	{
		scene s;
		s.target.player.race = RACE_DROW;
		timer = 3 * PULSES_IN_TICK;
		s.check();
		s.pair();
		character_list = &s.dwarf;
		for (int n = 0; n <= 5 * PULSES_IN_TICK; ++n)
		{
			if (!(n % PULSES_IN_TICK))
				affect_update();
			heartbeat();
			s.pair();
		}
		assert(activations == 1 && subsides == 0);
		character_list = nullptr;
		timer = WAIT_SEC;
	}
	else if (!std::strcmp(argv[1], "serialization"))
	{
		scene s;
		s.target.player.race = RACE_ORC;
		s.check();
		s.pair();
		char saved[4096]{};
		assert(writeAffects(saved, s.dwarf.affected) ==
		       1 + static_cast<int>(sizeof(short)));
		char *cursor = saved;
		assert(GET_BYTE(cursor) == SAV_AFFVERS);
		assert(GET_SHORT(cursor) == 0);
		affected_type ordinary{};
		ordinary.type = SPELL_BLESS;
		ordinary.duration = 3;
		affect_to_char(&s.dwarf, &ordinary);
		writeAffects(saved, s.dwarf.affected);
		cursor = saved;
		assert(GET_BYTE(cursor) == SAV_AFFVERS);
		assert(GET_SHORT(cursor) == 1);
		assert(GET_BYTE(cursor) == 0);
		assert(GET_SHORT(cursor) == SPELL_BLESS);
	}
	else
		assert(false);
	std::printf("Hatred production case passed: %s\n", argv[1]);
}
