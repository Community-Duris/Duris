/*
 * World activity policy for issue #299.
 *
 * Ordinary mobile activity is allowed to cool down only when a zone has no
 * direct player/corpse reason, no one-hop adjacent reason, and no recent
 * grace period.  The state is maintained from room and object lifecycle
 * hooks.  NPCs are indexed by zone so a promotion wakes only affected work.
 */

#include "world/world_activity.h"

#include "core/prototypes.h"
#include "core/utils.h"
#include "world/events.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <unordered_set>
#include <vector>

extern P_char character_list;
extern P_obj object_list;
extern P_room world;
extern P_index mob_index;
extern const int top_of_world;
extern int top_of_mobt;
extern unsigned long long ne_event_tick;
extern P_nevent current_nevent;

extern void event_mob_mundane(P_char, P_char, P_obj, void *);

namespace
{
constexpr int DEFAULT_DISTANT_SECONDS = 60;
constexpr int DEFAULT_GRACE_SECONDS = 90;
constexpr int DEFAULT_NEARBY_MULTIPLIER = PLAYERLESS_ZONE_SPEED_MODIFIER;
constexpr int MAX_ACTIVITY_SECONDS = 60 * 60;
constexpr int MAX_ACTIVITY_MULTIPLIER = 8;

struct activity_config
{
	bool enabled = false;
	int distant_pulses = DEFAULT_DISTANT_SECONDS * WAIT_SEC;
	int grace_pulses = DEFAULT_GRACE_SECONDS * WAIT_SEC;
	int nearby_multiplier = DEFAULT_NEARBY_MULTIPLIER;
	bool wake_enabled = true;
	bool diagnostics = false;
};

struct activity_zone
{
	uint32_t players = 0;
	uint32_t corpses = 0;
	uint32_t adjacent_players = 0;
	uint32_t adjacent_corpses = 0;
	uint64_t grace_until = 0;
	std::unordered_set<P_char> npcs;
	std::vector<int> neighbors;
};

activity_config config;
// Small zones share one region. Large zones are partitioned along actual exits
// into at most 64 rooms, so one traveller cannot activate an entire overworld.
constexpr int MAX_REGION_ROOMS = 64;
std::vector<activity_zone> zones;
std::vector<int> room_regions;
bool configuration_loaded = false;
bool ready = false;
bool bootstrapped = false;
uint64_t wake_promotions = 0;
uint64_t wake_events = 0;
uint64_t stale_wake_handles = 0;

bool valid_room(int room)
{
	return world && room >= 0 && room <= top_of_world;
}

int activity_zone_for_room(int room)
{
	if (!valid_room(room))
		return -1;

	if (static_cast<std::size_t>(room) >= room_regions.size())
		return -1;
	return room_regions[room];
}

void rebuild_regions()
{
	zones.clear();
	room_regions.assign(static_cast<std::size_t>(top_of_world) + 1, -1);
	std::array<int, MAX_ZONES> room_counts{};
	std::array<int, MAX_ZONES> small_regions{};
	small_regions.fill(-1);
	for (int room = 0; room <= top_of_world; ++room)
		if (world[room].zone < MAX_ZONES)
			++room_counts[world[room].zone];
	for (int room = 0; room <= top_of_world; ++room)
	{
		const int zone = world[room].zone;
		if (zone >= MAX_ZONES || room_regions[room] >= 0)
			continue;
		if (room_counts[zone] <= MAX_REGION_ROOMS && small_regions[zone] >= 0)
		{
			room_regions[room] = small_regions[zone];
			continue;
		}
		const int region = static_cast<int>(zones.size());
		zones.emplace_back();
		room_regions[room] = region;
		if (room_counts[zone] <= MAX_REGION_ROOMS)
		{
			small_regions[zone] = region;
			continue;
		}
		std::array<int, MAX_REGION_ROOMS> pending{};
		pending[0] = room;
		int count = 1;
		for (int at = 0; at < count && count < MAX_REGION_ROOMS; ++at)
			for (int door = 0; door < NUM_EXITS && count < MAX_REGION_ROOMS; ++door)
			{
				const auto *exit = world[pending[at]].dir_option[door];
				if (!exit || !valid_room(exit->to_room))
					continue;
				const int destination = exit->to_room;
				if (world[destination].zone != zone ||
				    room_regions[destination] >= 0)
					continue;
				room_regions[destination] = region;
				pending[count++] = destination;
			}
	}
	// Symmetric halo includes one-way exits: activity on either side of a
	// boundary keeps the other side nearby. Topology is built once per rebuild.
	for (int room = 0; room <= top_of_world; ++room)
		for (int door = 0; door < NUM_EXITS; ++door)
		{
			const auto *exit = world[room].dir_option[door];
			if (!exit)
				continue;
			const int own = activity_zone_for_room(room);
			const int neighbor = activity_zone_for_room(exit->to_room);
			if (own < 0 || neighbor < 0 || own == neighbor)
				continue;
			zones[own].neighbors.push_back(neighbor);
			zones[neighbor].neighbors.push_back(own);
		}
	for (auto &zone : zones)
	{
		auto &neighbors = zone.neighbors;
		std::sort(neighbors.begin(), neighbors.end());
		neighbors.erase(std::unique(neighbors.begin(), neighbors.end()), neighbors.end());
	}
}

template <typename Callback> void for_each_related_zone(int room, Callback &&callback)
{
	const int own_zone = activity_zone_for_room(room);
	if (own_zone < 0)
		return;

	callback(own_zone, false);

	for (int neighbor : zones[own_zone].neighbors)
		callback(neighbor, true);
}

bool direct_activity(const activity_zone &zone)
{
	// Corpse recovery protects the surrounding halo at the normal cadence too.
	// A predator crossing the region edge must not lose movement opportunities
	// while the independent corpse decay clock continues.
	return zone.players > 0 || zone.corpses > 0 || zone.adjacent_corpses > 0;
}

bool nearby_activity(const activity_zone &zone)
{
	return direct_activity(zone) || zone.adjacent_players > 0 || zone.adjacent_corpses > 0 ||
	       zone.grace_until > ne_event_tick;
}

void reset_activity_state()
{
	for (activity_zone &zone : zones)
	{
		zone.players = 0;
		zone.corpses = 0;
		zone.adjacent_players = 0;
		zone.adjacent_corpses = 0;
		zone.grace_until = 0;
		zone.npcs.clear();
	}
	wake_promotions = 0;
	wake_events = 0;
	stale_wake_handles = 0;
}

uint64_t bounded_tick_add(int pulses)
{
	const uint64_t amount = pulses > 0 ? static_cast<uint64_t>(pulses) : 0;
	if (std::numeric_limits<uint64_t>::max() - ne_event_tick < amount)
		return std::numeric_limits<uint64_t>::max();
	return ne_event_tick + amount;
}

void extend_grace(activity_zone &zone)
{
	zone.grace_until = std::max(zone.grace_until, bounded_tick_add(config.grace_pulses));
}

void wake_zone(int zone_number, bool force, int encounter_room = NOWHERE)
{
	if (!ready || (!config.enabled && !force) || (!config.wake_enabled && !force) ||
	    !nevent_is_game_thread() || zone_number < 0 ||
	    static_cast<std::size_t>(zone_number) >= zones.size())
		return;

	for (P_char mob : zones[zone_number].npcs)
	{
		if (!mob || !IS_ALIVE(mob) || !IS_NPC(mob) || mob->in_room == NOWHERE)
			continue;

		const nevent_handle scheduled = world_activity_mundane_event(mob);
		P_nevent event = scheduled.event;
		if (!event)
			continue;

		const int remaining = ne_event_time(event);
		if (remaining < 0)
		{
			++stale_wake_handles;
			continue;
		}

		/* Spread a promotion through at most one normal ambient interval. */
		const int target = mob->in_room == encounter_room ? 1 : number(1, PULSE_MOBILE);
		if (remaining > target &&
		    nevent_advance_by(nevent_handle_from_event(event),
				      static_cast<unsigned long long>(remaining - target)))
			++wake_events;
	}
}

void wake_encounter_room(int room)
{
	if (!ready || !config.enabled || !config.wake_enabled || !nevent_is_game_thread() ||
	    !valid_room(room))
		return;
	for (P_char mob = world[room].people; mob; mob = mob->next_in_room)
	{
		if (!IS_ALIVE(mob) || !IS_NPC(mob))
			continue;
		const auto handle = world_activity_mundane_event(mob);
		if (!handle.event)
			continue;
		const int remaining = ne_event_time(handle.event);
		if (remaining > 1 && nevent_advance_by(handle, remaining - 1))
			++wake_events;
	}
}

void adjust_reason(int room, bool player_reason, int delta)
{
	if (!config.enabled)
		return;

	for_each_related_zone(
		room,
		[&](int zone_number, bool adjacent)
		{
			activity_zone &zone = zones[zone_number];
			const bool was_nearby = nearby_activity(zone);
			const bool was_active = direct_activity(zone);

			uint32_t *counter = nullptr;
			if (player_reason)
				counter = adjacent ? &zone.adjacent_players : &zone.players;
			else
				counter = adjacent ? &zone.adjacent_corpses : &zone.corpses;

			if (delta > 0)
			{
				if (*counter < std::numeric_limits<uint32_t>::max())
					++*counter;
			}
			else if (delta < 0 && *counter > 0)
				--*counter;

			if (delta < 0)
				extend_grace(zone);

			if (ready && delta > 0 &&
			    ((!was_nearby && nearby_activity(zone)) ||
			     (!was_active && direct_activity(zone))))
			{
				++wake_promotions;
				wake_zone(zone_number, false, adjacent ? NOWHERE : room);
			}
		});
	if (delta > 0)
		wake_encounter_room(room);
}

bool pc_corpse(P_obj object)
{
	return object && object->type == ITEM_CORPSE && IS_SET(object->value[1], PC_CORPSE) &&
	       !IS_SET(object->value[1], NPC_CORPSE);
}

int effective_room(P_obj object)
{
	constexpr int MAX_PARENT_HOPS = 1024;
	for (int hops = 0; object && hops < MAX_PARENT_HOPS; ++hops)
	{
		if (OBJ_ROOM(object))
			return object->loc.room;
		if (OBJ_CARRIED(object))
			return object->loc.carrying ? object->loc.carrying->in_room : NOWHERE;
		if (OBJ_WORN(object))
			return object->loc.wearing ? object->loc.wearing->in_room : NOWHERE;
		if (!OBJ_INSIDE(object) || !object->loc.inside)
			return NOWHERE;
		object = object->loc.inside;
	}
	return NOWHERE;
}

bool sibling_cycle(P_obj object)
{
	P_obj slow = object, fast = object;
	for (int steps = 0; fast && fast->next_content; ++steps)
	{
		if (steps >= 65536)
			return true;
		slow = slow->next_content;
		fast = fast->next_content->next_content;
		if (slow == fast)
			return true;
	}
	return false;
}

void walk_corpse_subtree(P_obj object, int room, bool entering)
{
	// Fixed traversal storage avoids heap allocation and recursive stack
	// exhaustion. Parent validation and sibling-cycle checks reject malformed
	// recovery graphs before repeatedly counting the same corpse.
	struct frame
	{
		P_obj object;
		P_obj next_child;
	};
	std::array<frame, 1024> stack{};
	int depth = 0;
	unsigned int visited = 0;
	P_obj current = object;
	while (current && visited++ < 65536)
	{
		if (pc_corpse(current))
			adjust_reason(room, false, entering ? 1 : -1);
		P_obj child = current->contains;
		if (child && !sibling_cycle(child) && depth < static_cast<int>(stack.size()))
		{
			stack[depth++] = { current, child };
		}
		current = nullptr;
		while (depth > 0 && !current)
		{
			frame &parent = stack[depth - 1];
			P_obj next = parent.next_child;
			if (!next)
			{
				--depth;
				continue;
			}
			parent.next_child = next->next_content;
			if (OBJ_INSIDE(next) && next->loc.inside == parent.object)
				current = next;
		}
	}
}

void update_object_subtree(P_obj object, bool entering)
{
	if (!config.enabled || !object)
		return;
	/* Most object transfers are ordinary items with no contents.  Avoid even
	 * resolving their effective room; only a corpse or a non-empty subtree can
	 * change the activity policy. */
	if (!pc_corpse(object) && !object->contains)
		return;

	const int room = effective_room(object);
	if (!valid_room(room))
		return;

	walk_corpse_subtree(object, room, entering);
}

void update_character_objects(P_char ch, bool entering)
{
	if (!ch)
		return;
	if (!sibling_cycle(ch->carrying))
		for (P_obj object = ch->carrying; object; object = object->next_content)
			update_object_subtree(object, entering);
	for (int slot = 0; slot < MAX_WEAR; ++slot)
		if (ch->equipment[slot])
			update_object_subtree(ch->equipment[slot], entering);
}

void wake_all_indexed_mobs(bool force)
{
	if (!ready || !nevent_is_game_thread())
		return;
	for (int zone_number = 0; zone_number < static_cast<int>(zones.size()); ++zone_number)
		wake_zone(zone_number, force);
}

void load_configuration()
{
	activity_config next;
	next.enabled = get_property("world.activity.enabled", 0) != 0;
	next.distant_pulses =
		std::clamp(get_property("world.activity.distant.seconds", DEFAULT_DISTANT_SECONDS),
			   (PULSE_MOBILE + WAIT_SEC - 1) / WAIT_SEC, MAX_ACTIVITY_SECONDS) *
		WAIT_SEC;
	next.grace_pulses =
		std::clamp(get_property("world.activity.grace.seconds", DEFAULT_GRACE_SECONDS), 0,
			   MAX_ACTIVITY_SECONDS) *
		WAIT_SEC;
	next.nearby_multiplier = std::clamp(get_property("world.activity.nearby.multiplier",
							 DEFAULT_NEARBY_MULTIPLIER),
					    1, MAX_ACTIVITY_MULTIPLIER);
	next.wake_enabled = get_property("world.activity.wake.enabled", 1) != 0;
	next.diagnostics = get_property("world.activity.diagnostics", 0) != 0;

	const bool enabled_changed = configuration_loaded && config.enabled != next.enabled;
	const bool schedule_changed = configuration_loaded &&
				      (config.distant_pulses != next.distant_pulses ||
				       config.grace_pulses != next.grace_pulses ||
				       config.nearby_multiplier != next.nearby_multiplier ||
				       config.wake_enabled != next.wake_enabled);
	config = next;
	configuration_loaded = true;

	if (!ready)
	{
		if (bootstrapped && enabled_changed && config.enabled)
			world_activity_rebuild();
		return;
	}

	if (!config.enabled)
	{
		/* Disabling must not strand already-cold mundane events. */
		wake_all_indexed_mobs(true);
		reset_activity_state();
		ready = false;
	}
	else if (enabled_changed || schedule_changed)
	{
		if (enabled_changed)
			world_activity_rebuild();
		else
			wake_all_indexed_mobs(false);
	}
}

} // namespace

void world_activity_reload()
{
	load_configuration();
	if (config.diagnostics)
		world_activity_log_diagnostics();
}

void world_activity_rebuild()
{
	bootstrapped = true;
	ready = false;
	reset_activity_state();
	if (!config.enabled)
	{
		ready = false;
		return;
	}
	if (!world || top_of_world < 0)
		return;
	rebuild_regions();

	for (P_char ch = character_list; ch; ch = ch->next)
	{
		ch->world_activity_mundane_event = NULL;
		ch->world_activity_mundane_event_sequence = 0;
		for (P_nevent event = ch->nevents; event; event = event->next_char_nev)
			if (event->func == event_mob_mundane)
			{
				world_activity_record_mundane_event(
					ch, nevent_handle_from_event(event));
				break;
			}

		if (!valid_room(ch->in_room))
			continue;
		if (IS_NPC(ch))
		{
			const int zone_number = activity_zone_for_room(ch->in_room);
			if (zone_number >= 0)
				zones[zone_number].npcs.insert(ch);
		}
		else if (GET_STAT(ch) > STAT_DEAD)
			adjust_reason(ch->in_room, true, 1);
	}

	/* Walk each top-level object exactly once; nested corpses are included. */
	for (P_obj object = object_list; object; object = object->next)
		if (!OBJ_INSIDE(object))
			update_object_subtree(object, true);

	ready = true;
	if (config.diagnostics)
		world_activity_log_diagnostics();
}

void world_activity_log_diagnostics()
{
	const world_activity_health health = world_activity_get_health();
	logit(LOG_STATUS,
	      "WORLD ACTIVITY: enabled=%d ready=%d indexed_npcs=%llu players=%llu corpses=%llu "
	      "adjacent=%llu promotions=%llu wakes=%llu stale_wake_handles=%llu",
	      health.enabled ? 1 : 0, health.ready ? 1 : 0,
	      static_cast<unsigned long long>(health.indexed_npcs),
	      static_cast<unsigned long long>(health.player_reasons),
	      static_cast<unsigned long long>(health.corpse_reasons),
	      static_cast<unsigned long long>(health.adjacent_reasons),
	      static_cast<unsigned long long>(health.wake_promotions),
	      static_cast<unsigned long long>(health.wake_events),
	      static_cast<unsigned long long>(health.stale_wake_handles));
}

world_activity_health world_activity_get_health()
{
	world_activity_health health{};
	health.enabled = config.enabled;
	health.ready = ready;
	for (const activity_zone &zone : zones)
	{
		health.indexed_npcs += zone.npcs.size();
		health.player_reasons += zone.players;
		health.corpse_reasons += zone.corpses;
		health.adjacent_reasons += zone.adjacent_players;
		health.adjacent_reasons += zone.adjacent_corpses;
	}
	health.wake_promotions = wake_promotions;
	health.wake_events = wake_events;
	health.stale_wake_handles = stale_wake_handles;
	return health;
}

bool world_activity_is_enabled()
{
	return config.enabled;
}

world_activity_tier world_activity_tier_for_room(int room)
{
	if (!config.enabled || !ready)
		return world_activity_tier::active;

	const int zone_number = activity_zone_for_room(room);
	if (zone_number < 0)
		return world_activity_tier::active;
	if (direct_activity(zones[zone_number]))
		return world_activity_tier::active;
	if (nearby_activity(zones[zone_number]))
		return world_activity_tier::nearby;
	return world_activity_tier::distant;
}

void world_activity_character_enter(P_char ch)
{
	if (!config.enabled || !ch || !valid_room(ch->in_room))
		return;
	if (IS_NPC(ch))
	{
		const int zone_number = activity_zone_for_room(ch->in_room);
		if (zone_number >= 0)
			zones[zone_number].npcs.insert(ch);
	}
	update_character_objects(ch, true);
}

void world_activity_character_leave(P_char ch)
{
	if (!config.enabled || !ch || !valid_room(ch->in_room))
		return;
	if (IS_NPC(ch))
	{
		const int zone_number = activity_zone_for_room(ch->in_room);
		if (zone_number >= 0)
			zones[zone_number].npcs.erase(ch);
	}
	update_character_objects(ch, false);
}

void world_activity_player_enter(P_char ch)
{
	if (!config.enabled || !ch || IS_NPC(ch) || !valid_room(ch->in_room))
		return;
	adjust_reason(ch->in_room, true, 1);
}

void world_activity_player_leave(P_char ch)
{
	if (!config.enabled || !ch || IS_NPC(ch) || !valid_room(ch->in_room))
		return;
	adjust_reason(ch->in_room, true, -1);
}

void world_activity_record_mundane_event(P_char ch, nevent_handle event)
{
	if (!ch)
		return;
	ch->world_activity_mundane_event = event.event;
	ch->world_activity_mundane_event_sequence = event.sequence;
}

nevent_handle world_activity_mundane_event(P_char ch)
{
	if (!ch || !ch->world_activity_mundane_event ||
	    ch->world_activity_mundane_event_sequence == 0)
		return { NULL, 0 };

	const nevent_handle event = { ch->world_activity_mundane_event,
				      ch->world_activity_mundane_event_sequence };
	if (!nevent_handle_is_active(event) || event.event->func != event_mob_mundane ||
	    event.event->ch != ch)
	{
		ch->world_activity_mundane_event = NULL;
		ch->world_activity_mundane_event_sequence = 0;
		return { NULL, 0 };
	}
	return event;
}

void world_activity_promote_character(P_char ch)
{
	if (!config.enabled || !ready || !ch || !valid_room(ch->in_room))
		return;
	wake_encounter_room(ch->in_room);
}

void world_activity_object_enter(P_obj object)
{
	update_object_subtree(object, true);
}

void world_activity_object_leave(P_obj object)
{
	update_object_subtree(object, false);
}

bool world_activity_mob_is_timing_sensitive(P_char ch)
{
	if (!ch || !IS_NPC(ch))
		return true;
	if (IS_FIGHTING(ch) || GET_OPPONENT(ch) || IS_CASTING(ch) || GET_MASTER(ch) ||
	    get_linking_char(ch, LNK_RIDING))
		return true;
	if (IS_PATROL(ch) || TRUSTED_NPC(ch) || IS_SET(ch->specials.act, ACT_SPEC) ||
	    IS_SET(ch->specials.act, ACT_SENTINEL) || (HAS_MEMORY(ch) && GET_MEMORY(ch)))
		return true;
	if (!ch->only.npc || !mob_index || GET_RNUM(ch) < 0 || GET_RNUM(ch) > top_of_mobt ||
	    mob_index[GET_RNUM(ch)].qst_func || mob_index[GET_RNUM(ch)].func.mob)
		return true;
	if (IS_AFFECTED2(ch, AFF2_CASTING | AFF2_MEMORIZING | AFF2_SCRIBING | AFF2_HUNTER) ||
	    GET_POS(ch) != POS_STANDING || GET_HIT(ch) < GET_MAX_HIT(ch))
		return true;
	return false;
}

int world_activity_mundane_delay(P_char ch, bool quick_retry, bool legacy_zone_occupied)
{
	if (quick_retry)
		return PULSE_VIOLENCE;

	int base = legacy_zone_occupied ? PULSE_MOBILE :
					  PULSE_MOBILE * PLAYERLESS_ZONE_SPEED_MODIFIER;
	if (config.enabled && ready && ch)
	{
		if (world_activity_mob_is_timing_sensitive(ch))
			base = PULSE_MOBILE;
		else
		{
			switch (world_activity_tier_for_room(ch->in_room))
			{
			case world_activity_tier::active:
				base = PULSE_MOBILE;
				break;
			case world_activity_tier::nearby:
				base = PULSE_MOBILE * config.nearby_multiplier;
				break;
			case world_activity_tier::distant:
				base = config.distant_pulses;
				break;
			}
		}
	}
	return std::max(1, base + number(-4, 4));
}

void world_activity_schedule_mundane_after(P_char ch, int delay)
{
	if (!ch)
		return;
	const nevent_handle existing = world_activity_mundane_event(ch);
	// The scheduler destroys the dispatched event after its callback. Its
	// successor must be a new event; only pending events can be rescheduled.
	if (existing.event && existing.event != current_nevent)
	{
		nevent_reschedule_after(existing, std::max(0, delay));
		return;
	}
	const nevent_schedule_result result =
		add_event(event_mob_mundane, std::max(0, delay), ch, 0, 0, 0, 0, 0);
	if (result.was_scheduled())
		world_activity_record_mundane_event(ch, result.handle);
}

void world_activity_schedule_mundane(P_char ch, bool quick_retry, bool legacy_zone_occupied)
{
	world_activity_schedule_mundane_after(
		ch, world_activity_mundane_delay(ch, quick_retry, legacy_zone_occupied));
}
