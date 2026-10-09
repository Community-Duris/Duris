/*
 * World activity policy for issue #299.
 *
 * Ordinary mobile activity is allowed to cool down only when a zone has no
 * direct player/corpse reason, no one-hop adjacent reason, and no recent
 * grace period.  The state is maintained from room and object lifecycle
 * hooks.  NPCs are indexed by zone so a promotion wakes only affected work.
 */

#include "world/world_activity.h"
#include "player/player_snapshot.h"

#include "core/prototypes.h"
#include "core/utils.h"
#include "world/events.h"
#include "world/ferry.h"
#include "world/specs.prototypes.h"
#include "ships/ships.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <unordered_map>
#include <unordered_set>
#include <vector>

extern P_char character_list;
extern P_obj object_list;
extern P_room world;
extern P_index mob_index;
extern P_index obj_index;
extern const int top_of_world;
extern int top_of_mobt;
extern int top_of_objt;
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
	uint64_t last_departure_tick = std::numeric_limits<uint64_t>::max();
	std::unordered_set<P_char> npcs;
	std::vector<int> neighbors;
};

activity_config config;
// Small zones share one region. Large zones are partitioned along actual exits
// into at most 64 rooms, so one traveller cannot activate an entire overworld.
constexpr int MAX_REGION_ROOMS = 64;
std::vector<activity_zone> zones;
std::vector<int> room_regions;
std::array<std::vector<int>, MAX_ZONES> zone_regions;
std::vector<std::array<int, NUM_EXITS>> room_exits;
// Count physical connections, so removing one of two exits (or a portal) does
// not remove their shared halo. Only the first/last connection changes reasons.
std::unordered_map<uint64_t, uint32_t> connections;
struct object_connection
{
	int room;
	std::vector<int> destinations;
};
std::unordered_map<P_obj, object_connection> object_connections;
std::unordered_map<P_char, int> controlled_rooms;
bool configuration_loaded = false;
bool ready = false;
bool bootstrapped = false;
uint64_t wake_promotions = 0;
uint64_t wake_events = 0;
uint64_t stale_wake_handles = 0;
uint64_t repaired_mundane_handles = 0;
uint64_t topology_changes = 0;
std::vector<int> rebuild_encounters;
void change_connection(int from_room, int to_room, int delta);

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
	connections.clear();
	object_connections.clear();
	for (auto &regions : zone_regions)
		regions.clear();
	room_regions.assign(static_cast<std::size_t>(top_of_world) + 1, -1);
	room_exits.resize(room_regions.size());
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
		zone_regions[zone].push_back(room);
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
	// boundary keeps the other side nearby. Live publishers update this cache.
	for (int room = 0; room <= top_of_world; ++room)
		for (int door = 0; door < NUM_EXITS; ++door)
		{
			const auto *exit = world[room].dir_option[door];
			const int destination = exit && valid_room(exit->to_room) ? exit->to_room :
										    NOWHERE;
			room_exits[room][door] = destination;
			change_connection(room, destination, 1);
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

bool activity_promoted(const activity_zone &zone, bool was_nearby, bool was_active)
{
	// A leave/enter pair in the same region must not repeatedly advance all its
	// NPCs. Their previous promotion already bounded the schedule; the arrival
	// room still receives the independent next-pulse encounter wake.
	if (zone.last_departure_tick == ne_event_tick)
		return false;
	return (!was_nearby && nearby_activity(zone)) || (!was_active && direct_activity(zone));
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
		zone.last_departure_tick = std::numeric_limits<uint64_t>::max();
		zone.npcs.clear();
	}
	wake_promotions = 0;
	wake_events = 0;
	stale_wake_handles = 0;
	repaired_mundane_handles = 0;
	topology_changes = 0;
	controlled_rooms.clear();
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

	nevent_reschedule_batch batch;
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
	nevent_reschedule_batch batch;
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

void change_connection(int from_room, int to_room, int delta)
{
	const int own = activity_zone_for_room(from_room);
	const int neighbor = activity_zone_for_room(to_room);
	if (own < 0 || neighbor < 0 || own == neighbor)
		return;
	const uint64_t key = (static_cast<uint64_t>(std::min(own, neighbor)) << 32) |
			     static_cast<uint32_t>(std::max(own, neighbor));
	if (delta > 0)
	{
		if (++connections[key] != 1)
			return;
		zones[own].neighbors.push_back(neighbor);
		zones[neighbor].neighbors.push_back(own);
	}
	else
	{
		auto found = connections.find(key);
		if (found == connections.end() || --found->second != 0)
			return;
		connections.erase(found);
		for (auto pair : { std::pair{ own, neighbor }, std::pair{ neighbor, own } })
		{
			auto &neighbors = zones[pair.first].neighbors;
			neighbors.erase(std::remove(neighbors.begin(), neighbors.end(),
						    pair.second),
					neighbors.end());
		}
	}
	if (ready)
		++topology_changes;
	for (auto pair : { std::pair{ own, neighbor }, std::pair{ neighbor, own } })
	{
		const activity_zone &source = zones[pair.first];
		activity_zone &target = zones[pair.second];
		const bool was_nearby = nearby_activity(target);
		const bool was_active = direct_activity(target);
		if (delta > 0)
		{
			target.adjacent_players += source.players;
			target.adjacent_corpses += source.corpses;
		}
		else
		{
			target.adjacent_players -=
				std::min(target.adjacent_players, source.players);
			target.adjacent_corpses -=
				std::min(target.adjacent_corpses, source.corpses);
			if (source.players || source.corpses)
				extend_grace(target);
		}
		if (delta < 0 && ((was_active && !direct_activity(target)) ||
				  (was_nearby && !nearby_activity(target))))
			target.last_departure_tick = ne_event_tick;
		if (ready && delta > 0 && activity_promoted(target, was_nearby, was_active))
		{
			++wake_promotions;
			wake_zone(pair.second, false,
				  pair.second == neighbor ? to_room : from_room);
		}
	}
}

void update_object_connections(P_obj object, bool entering)
{
	if (!config.enabled || !object)
		return;
	auto old = object_connections.find(object);
	if (old != object_connections.end())
	{
		for (int destination : old->second.destinations)
			change_connection(old->second.room, destination, -1);
		object_connections.erase(old);
	}
	if (!entering || !OBJ_ROOM(object) || !valid_room(object->loc.room))
		return;
	object_connection next{ object->loc.room, {} };
	if (object->type == ITEM_TELEPORT ||
	    (obj_index && object->R_num >= 0 && object->R_num <= top_of_objt &&
	     (obj_index[object->R_num].func.obj == portal_door ||
	      obj_index[object->R_num].func.obj == portal_wormhole ||
	      obj_index[object->R_num].func.obj == portal_etherportal)))
	{
		if (object->value[0] >= 0)
			next.destinations.push_back(real_room(object->value[0]));
		else
		{
			// Random-zone teleporters have no single destination. Conservatively
			// protect every region in the possible destination zone.
			const int zone_room = real_room(object->value[3]);
			if (valid_room(zone_room) && world[zone_room].zone < MAX_ZONES)
				next.destinations = zone_regions[world[zone_room].zone];
		}
	}
	else if (object->type == ITEM_SHIP || object->type == ITEM_BOAT)
	{
		if (P_ship ship = shipObjHash.find(object))
			for (int i = 0; i < ship->room_count; ++i)
				next.destinations.push_back(real_room(ship->room[i].roomnum));
		if (Ferry *ferry = get_ferry_from_obj(object->R_num); ferry && ferry->obj == object)
			next.destinations.insert(next.destinations.end(), ferry->rooms.begin(),
						 ferry->rooms.end());
	}
	if (next.destinations.empty())
		return;
	for (int destination : next.destinations)
		change_connection(next.room, destination, 1);
	object_connections.emplace(object, std::move(next));
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
			if (delta < 0 && ((was_active && !direct_activity(zone)) ||
					  (was_nearby && !nearby_activity(zone))))
				zone.last_departure_tick = ne_event_tick;

			if (ready && delta > 0 && activity_promoted(zone, was_nearby, was_active))
			{
				++wake_promotions;
				wake_zone(zone_number, false, adjacent ? NOWHERE : room);
			}
		});
	if (delta > 0)
	{
		if (!ready)
			rebuild_encounters.push_back(room);
		else
			wake_encounter_room(room);
	}
}

bool pc_corpse(P_obj object)
{
	return object && object->type == ITEM_CORPSE && IS_SET(object->value[1], PC_CORPSE) &&
	       !IS_SET(object->value[1], NPC_CORPSE);
}

bool controlled_presence(P_char ch)
{
	if (!ch || !IS_NPC(ch) || !ch->only.npc || !IS_ALIVE(ch))
		return false;
	const P_char original = MORPH_ORIG(ch);
	const P_char master = GET_MASTER(ch);
	const P_char rider = get_linking_char(ch, LNK_RIDING);
	return ch->desc || (original && IS_PC(original)) || (master && IS_PC(master)) ||
	       (rider && IS_PC(rider));
}

void refresh_controlled_presence(P_char ch, bool leaving = false)
{
	if (!config.enabled || !ch || !IS_NPC(ch))
		return;
	const bool controlled = !leaving && valid_room(ch->in_room) && controlled_presence(ch);
	auto previous = controlled_rooms.find(ch);
	if (previous != controlled_rooms.end())
	{
		if (controlled && previous->second == ch->in_room)
			return;
		adjust_reason(previous->second, true, -1);
		controlled_rooms.erase(previous);
	}
	if (controlled)
	{
		controlled_rooms.emplace(ch, ch->in_room);
		adjust_reason(ch->in_room, true, 1);
	}
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
	// Every live frame is fully assigned before use; do not clear 16 KiB for
	// each ordinary container transfer.
	std::array<frame, 1024> stack;
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
	next.enabled = get_property("world.activity.enabled", 1) != 0;
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
	// Cold schedules are only safe when approaching players/corpses can wake them.
	next.enabled = next.enabled && next.wake_enabled;
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
	rebuild_encounters.clear();
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
			refresh_controlled_presence(ch);
		}
		else if (GET_STAT(ch) > STAT_DEAD)
			adjust_reason(ch->in_room, true, 1);
	}

	/* Walk each top-level object exactly once; nested corpses are included. */
	for (P_obj object = object_list; object; object = object->next)
		if (!OBJ_INSIDE(object))
		{
			update_object_connections(object, true);
			update_object_subtree(object, true);
		}

	ready = true;
	for (int region = 0; region < static_cast<int>(zones.size()); ++region)
		if (nearby_activity(zones[region]))
			wake_zone(region, false);
	std::sort(rebuild_encounters.begin(), rebuild_encounters.end());
	rebuild_encounters.erase(std::unique(rebuild_encounters.begin(), rebuild_encounters.end()),
				 rebuild_encounters.end());
	for (int room : rebuild_encounters)
		wake_encounter_room(room);
	rebuild_encounters.clear();
	if (config.diagnostics)
		world_activity_log_diagnostics();
}

void world_activity_log_diagnostics()
{
	if (!config.diagnostics)
		return;
	const world_activity_health health = world_activity_get_health();
	logit(LOG_STATUS,
	      "WORLD ACTIVITY: enabled=%d ready=%d indexed_npcs=%llu players=%llu corpses=%llu "
	      "adjacent=%llu promotions=%llu wakes=%llu stale_wake_handles=%llu "
	      "handle_repairs=%llu topology_changes=%llu",
	      health.enabled ? 1 : 0, health.ready ? 1 : 0,
	      static_cast<unsigned long long>(health.indexed_npcs),
	      static_cast<unsigned long long>(health.player_reasons),
	      static_cast<unsigned long long>(health.corpse_reasons),
	      static_cast<unsigned long long>(health.adjacent_reasons),
	      static_cast<unsigned long long>(health.wake_promotions),
	      static_cast<unsigned long long>(health.wake_events),
	      static_cast<unsigned long long>(health.stale_wake_handles),
	      static_cast<unsigned long long>(health.repaired_mundane_handles),
	      static_cast<unsigned long long>(health.topology_changes));
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
	health.repaired_mundane_handles = repaired_mundane_handles;
	health.topology_changes = topology_changes;
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

// Reconstruct already-returned birth membership. The existing birth owner
// proves original SQL/native/source/custody authority before the room stage
// reaches this private seam. It must latch the returned room substep before
// any subsequent fallible work. Ordinary enter/leave and promotion stay intact.
bool world_activity_native_birth_restore_owner::enter(P_char ch) noexcept
{
	if (!nevent_is_game_thread() || !ch || !IS_NPC(ch) || !ch->only.npc || !IS_ALIVE(ch) ||
	    !ch->runtime_id || !valid_room(ch->in_room) || controlled_presence(ch) ||
	    controlled_rooms.contains(ch))
		return false;
	if (!config.enabled)
		return true;
	try
	{
		// The complete forest is already authenticated by the birth owner;
		// independently check its physical links before counting PC corpses.
		// All traversal/allocation precedes any activity projection mutation.
		struct entry
		{
			P_obj object;
			P_obj parent;
			size_t depth;
			int slot;
		};
		std::vector<entry> pending;
		std::unordered_set<P_obj> seen;
		std::unordered_set<uint64_t> uids;
		for (int slot = 0; slot < MAX_WEAR; ++slot)
			if (ch->equipment[slot])
			{
				if (ch->equipment[slot]->next_content)
					return false;
				pending.push_back({ ch->equipment[slot], nullptr, 1, slot + 1 });
			}
		if (ch->carrying)
			pending.push_back({ ch->carrying, nullptr, 1, 0 });
		uint32_t corpses = 0;
		while (!pending.empty())
		{
			const auto current = pending.back();
			pending.pop_back();
			const auto object = current.object;
			if (current.depth > PLAYER_SNAPSHOT_MAX_DEPTH ||
			    seen.size() >= PLAYER_SNAPSHOT_MAX_OBJECTS ||
			    !seen.insert(object).second || !object->obj_uid ||
			    object->obj_uid == UINT64_MAX || !uids.insert(object->obj_uid).second)
				return false;
			if (current.parent)
			{
				if (object->loc_p != LOC_INSIDE ||
				    object->loc.inside != current.parent)
					return false;
			}
			else if (current.slot)
			{
				if (object->loc_p != LOC_WORN || object->loc.wearing != ch)
					return false;
			}
			else if (object->loc_p != LOC_CARRIED || object->loc.carrying != ch)
				return false;
			if (pc_corpse(object))
				++corpses;
			if (!current.slot && object->next_content)
				pending.push_back(
					{ object->next_content, current.parent, current.depth, 0 });
			if (object->contains)
				pending.push_back(
					{ object->contains, object, current.depth + 1, 0 });
		}
		const int own = activity_zone_for_room(ch->in_room);
		if (own < 0)
			return true; // Original enter has no region contribution at this cut.
		if (static_cast<size_t>(own) >= zones.size())
			return false;
		std::unordered_set<int> neighbors;
		for (int neighbor : zones[own].neighbors)
		{
			if (neighbor < 0 || static_cast<size_t>(neighbor) >= zones.size() ||
			    neighbor == own || !neighbors.insert(neighbor).second)
				return false;
		}
		for (const auto &zone : zones)
			if (zone.npcs.contains(ch))
				return false; // This room substep cannot count an enrolled body twice.
		// unordered_set insertion has the strong guarantee. After this succeeds,
		// only no-throw counter assignments remain: no wake/RNG/scheduler or
		// rebuild_encounters allocation can split activity reconstruction.
		if (!zones[own].npcs.insert(ch).second)
			return false;
		const auto add = [corpses](uint32_t &counter) noexcept
		{
			const auto remaining = std::numeric_limits<uint32_t>::max() - counter;
			counter += std::min(corpses, remaining);
		};
		add(zones[own].corpses);
		for (int neighbor : zones[own].neighbors)
			add(zones[neighbor].adjacent_corpses);
		return true;
	}
	catch (...)
	{
		return false;
	}
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
		refresh_controlled_presence(ch);
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
		refresh_controlled_presence(ch, true);
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

static nevent_handle validated_mundane_event(P_char ch, bool *complete)
{
	if (complete)
		*complete = true;
	if (!ch)
		return { NULL, 0 };

	const nevent_handle event = { ch->world_activity_mundane_event,
				      ch->world_activity_mundane_event_sequence };
	if (nevent_handle_is_active(event) && event.event->func == event_mob_mundane &&
	    event.event->ch == ch)
		return event;
	if (event.event)
		++stale_wake_handles;
	ch->world_activity_mundane_event = NULL;
	ch->world_activity_mundane_event_sequence = 0;
	// Only a missing/stale cache takes this bounded fallback. The indexed wake
	// hot path validates one handle; it never searches a healthy owner's list.
	unsigned int remaining = 64;
	P_nevent pending = ch->nevents;
	for (; pending && remaining; pending = pending->next_char_nev, --remaining)
	{
		const nevent_handle candidate = nevent_handle_from_event(pending);
		if (pending->func == event_mob_mundane && pending->ch == ch &&
		    nevent_handle_is_active(candidate))
		{
			world_activity_record_mundane_event(ch, candidate);
			++repaired_mundane_handles;
			return candidate;
		}
	}
	if (pending)
	{
		++stale_wake_handles;
		if (complete)
			*complete = false;
	}
	return { NULL, 0 };
}

nevent_handle world_activity_mundane_event(P_char ch)
{
	return validated_mundane_event(ch, nullptr);
}

void world_activity_room_exits_changed(int room)
{
	if (!config.enabled || !ready || !valid_room(room) ||
	    static_cast<std::size_t>(room) >= room_exits.size())
		return;
	for (int door = 0; door < NUM_EXITS; ++door)
	{
		const auto *exit = world[room].dir_option[door];
		const int destination = exit && valid_room(exit->to_room) ? exit->to_room : NOWHERE;
		int &previous = room_exits[room][door];
		if (previous == destination)
			continue;
		if (activity_zone_for_room(previous) == activity_zone_for_room(destination))
		{
			previous = destination;
			continue;
		}
		change_connection(room, previous, -1);
		previous = destination;
		change_connection(room, destination, 1);
	}
}

void world_activity_promote_character(P_char ch)
{
	if (!config.enabled || !ready || !ch || !valid_room(ch->in_room))
		return;
	refresh_controlled_presence(ch);
	wake_encounter_room(ch->in_room);
}

void world_activity_object_enter(P_obj object)
{
	update_object_connections(object, true);
	update_object_subtree(object, true);
}

void world_activity_object_leave(P_obj object)
{
	update_object_connections(object, false);
	update_object_subtree(object, false);
}

bool world_activity_mob_is_timing_sensitive(P_char ch)
{
	if (!ch || !IS_NPC(ch))
		return true;
	if (controlled_presence(ch))
		return true;
	if (valid_room(ch->in_room) && world[ch->in_room].funct)
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
		refresh_controlled_presence(ch);
		const auto tier = world_activity_tier_for_room(ch->in_room);
		if (world_activity_mob_is_timing_sensitive(ch))
		{
			// Exempt NPCs keep their legacy cadence outside protected regions.
			// Enabling an idle-work throttle must not speed up remote scripts.
			if (tier == world_activity_tier::active)
				base = PULSE_MOBILE;
		}
		else
		{
			switch (tier)
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
	bool complete = true;
	const nevent_handle existing = validated_mundane_event(ch, &complete);
	if (!complete && (!current_nevent || current_nevent->ch != ch ||
			  current_nevent->func != event_mob_mundane))
		return; // An unsearched pending event must not acquire a duplicate.
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

namespace
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI
bool activity_storage_add(size_t &bytes, size_t amount) noexcept
{
	if (amount > SIZE_MAX - bytes)
		return false;
	bytes += amount;
	return true;
}
bool activity_storage_rows(size_t &bytes, size_t count, size_t width) noexcept
{
	return (!width || count <= SIZE_MAX / width) && activity_storage_add(bytes, count * width);
}
template <class Container>
bool activity_storage_hash_heap(const Container &value, size_t &bytes) noexcept
{
	using node =
		std::__detail::_Hash_node<typename Container::value_type,
					  std::__cache_default<typename Container::key_type,
							       typename Container::hasher>::value>;
	// GCC13 singleton bucket is embedded in the already-counted container object.
	return activity_storage_rows(bytes, value.size(), sizeof(node)) &&
	       (value.bucket_count() == 1 ||
		activity_storage_rows(bytes, value.bucket_count(),
				      sizeof(std::__detail::_Hash_node_base *)));
}
#endif
}

bool world_activity_storage_bytes(size_t *output) noexcept
{
	if (!output || !nevent_is_game_thread())
		return false;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	return false;
#else
	// Actual owning inline registries/configuration/counters plus every live heap
	// request. Nested container inline objects already reside in nodes/zone backing.
	size_t bytes = sizeof(config) + sizeof(zones) + sizeof(room_regions) +
		       sizeof(zone_regions) + sizeof(room_exits) + sizeof(connections) +
		       sizeof(object_connections) + sizeof(controlled_rooms) +
		       sizeof(configuration_loaded) + sizeof(ready) + sizeof(bootstrapped) +
		       sizeof(wake_promotions) + sizeof(wake_events) + sizeof(stale_wake_handles) +
		       sizeof(repaired_mundane_handles) + sizeof(topology_changes) +
		       sizeof(rebuild_encounters);
	if (!activity_storage_rows(bytes, zones.capacity(), sizeof(activity_zone)) ||
	    !activity_storage_rows(bytes, room_regions.capacity(), sizeof(int)) ||
	    !activity_storage_rows(bytes, room_exits.capacity(),
				   sizeof(std::array<int, NUM_EXITS>)) ||
	    !activity_storage_rows(bytes, rebuild_encounters.capacity(), sizeof(int)) ||
	    !activity_storage_hash_heap(connections, bytes) ||
	    !activity_storage_hash_heap(object_connections, bytes) ||
	    !activity_storage_hash_heap(controlled_rooms, bytes))
		return false;
	for (const auto &zone : zones)
		if (!activity_storage_hash_heap(zone.npcs, bytes) ||
		    !activity_storage_rows(bytes, zone.neighbors.capacity(), sizeof(int)))
			return false;
	for (const auto &region : zone_regions)
		if (!activity_storage_rows(bytes, region.capacity(), sizeof(int)))
			return false;
	for (const auto &entry : object_connections)
		if (!activity_storage_rows(bytes, entry.second.destinations.capacity(),
					   sizeof(int)))
			return false;
	*output = bytes;
	return true;
#endif
}

#include "net/comm.h"
#include <initializer_list>
#include <new>
namespace
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI
struct activity_enter_budget
{
	bool (*reserve)(size_t, void *) noexcept;
	void *context;
	size_t base;
	std::vector<int> *transient = nullptr;
	bool live(size_t *result, size_t scope = 0)
	{
		size_t activity, pending, output;
		if (!world_activity_storage_bytes(&activity) ||
		    !nevent_native_reschedule_storage_bytes(&pending) ||
		    !diagnostic_output_storage_bytes(&output))
			return false;
		size_t bytes = base;
		if (!activity_storage_add(bytes, activity) ||
		    !activity_storage_add(bytes, pending) || !activity_storage_add(bytes, output) ||
		    !activity_storage_add(bytes, scope) ||
		    (transient &&
		     !activity_storage_rows(bytes, transient->capacity(), sizeof(int))))
			return false;
		*result = bytes;
		return true;
	}
	bool admit(size_t extra, size_t scope = 0)
	{
		size_t bytes;
		if (!live(&bytes, scope))
			return false;
		return activity_storage_add(bytes, extra) && reserve(bytes, context);
	}
	template <class Map, class Key> bool insert(const Map &map, const Key &key, size_t scope)
	{
		if (map.find(key) != map.end())
			return true;
		struct scan
		{
			std::__detail::_Prime_rehash_policy policy;
			std::pair<bool, size_t> growth;
			size_t bytes;
		};
		if (!admit(sizeof(scan), scope))
			return false;
		scan work{};
		// These ORIGINAL private registries have default load-factor one, never
		// changed by any original owning method. Reproduce its actual reset threshold.
		if (map.max_load_factor() != 1.0f)
			return false;
		work.policy._M_reset(map.bucket_count() == 1 ? 0 : map.bucket_count());
		work.growth = work.policy._M_need_rehash(map.bucket_count(), map.size(), 1);
		using node = std::__detail::_Hash_node<
			typename Map::value_type,
			std::__cache_default<typename Map::key_type, typename Map::hasher>::value>;
		work.bytes = sizeof(node);
		if (work.growth.first &&
		    !activity_storage_rows(work.bytes, work.growth.second,
					   sizeof(std::__detail::_Hash_node_base *)))
			return false;
		return admit(work.bytes,
			     scope); // Scanner dies before original node/bucket requests.
	}
	bool grow(const std::vector<int> &values, size_t count, size_t scope)
	{
		if (count <= values.capacity() - values.size())
			return true;
		if (values.size() > SIZE_MAX - std::max(values.size(), count))
			return false;
		const size_t capacity = values.size() + std::max(values.size(), count);
		if (capacity > SIZE_MAX / sizeof(int))
			return false;
		return admit(capacity * sizeof(int),
			     scope); // Existing request remains in current live.
	}
};
bool activity_wake_zone_bounded(activity_enter_budget &, size_t, int, bool, int = NOWHERE);
bool activity_wake_encounter_bounded(activity_enter_budget &, size_t, int);
bool activity_change_connection_bounded(activity_enter_budget &, size_t, int, int, int);
bool activity_adjust_reason_bounded(activity_enter_budget &, size_t, int, bool, int);
bool activity_wake_zone_bounded(activity_enter_budget &budget, size_t scope, int zone_number,
				bool force, int encounter_room)
{
	if (!ready || (!config.enabled && !force) || (!config.wake_enabled && !force) ||
	    !nevent_is_game_thread() || zone_number < 0 ||
	    static_cast<std::size_t>(zone_number) >= zones.size())
		return true;

	const size_t frame =
		sizeof(nevent_native_reschedule_batch) + 4 * sizeof(nevent_handle) + sizeof(size_t);
	if (!activity_storage_add(scope, frame))
		return false;
	if (!budget.admit(0, scope))
		return false;
	nevent_native_reschedule_batch batch;
	size_t current;
	if (!budget.live(&current, scope) ||
	    !batch.begin_bounded(budget.reserve, budget.context, current))
		return false;
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
		if (remaining > target)
		{
			const nevent_handle selected = nevent_handle_from_event(event);
			bool advanced;
			if (!budget.live(&current, scope) ||
			    !nevent_advance_by_bounded(
				    selected, static_cast<unsigned long long>(remaining - target),
				    &advanced, budget.reserve, budget.context, current))
				return false;
			if (advanced)
				++wake_events;
		}
	}
	if (!budget.live(&current, scope) ||
	    !batch.finish_bounded(budget.reserve, budget.context, current))
		return false;

	return true;
}
bool activity_wake_encounter_bounded(activity_enter_budget &budget, size_t scope, int room)
{
	if (!ready || !config.enabled || !config.wake_enabled || !nevent_is_game_thread() ||
	    !valid_room(room))
		return true;
	const size_t frame =
		sizeof(nevent_native_reschedule_batch) + 4 * sizeof(nevent_handle) + sizeof(size_t);
	if (!activity_storage_add(scope, frame))
		return false;
	if (!budget.admit(0, scope))
		return false;
	nevent_native_reschedule_batch batch;
	size_t current;
	if (!budget.live(&current, scope) ||
	    !batch.begin_bounded(budget.reserve, budget.context, current))
		return false;
	for (P_char mob = world[room].people; mob; mob = mob->next_in_room)
	{
		if (!IS_ALIVE(mob) || !IS_NPC(mob))
			continue;
		const auto handle = world_activity_mundane_event(mob);
		if (!handle.event)
			continue;
		const int remaining = ne_event_time(handle.event);
		if (remaining > 1)
		{
			bool advanced;
			if (!budget.live(&current, scope) ||
			    !nevent_advance_by_bounded(handle, remaining - 1, &advanced,
						       budget.reserve, budget.context, current))
				return false;
			if (advanced)
				++wake_events;
		}
	}
	if (!budget.live(&current, scope) ||
	    !batch.finish_bounded(budget.reserve, budget.context, current))
		return false;

	return true;
}
bool activity_change_connection_bounded(activity_enter_budget &budget, size_t scope, int from_room,
					int to_room, int delta)
{
	const int own = activity_zone_for_room(from_room);
	const int neighbor = activity_zone_for_room(to_room);
	if (own < 0 || neighbor < 0 || own == neighbor)
		return true;
	const size_t pair_frame = sizeof(std::initializer_list<std::pair<int, int>>) +
				  sizeof(std::array<std::pair<int, int>, 2>) +
				  sizeof(std::pair<int, int>);
	if (!activity_storage_add(scope, pair_frame))
		return false;
	if (!budget.admit(0, scope))
		return false;
	const uint64_t key = (static_cast<uint64_t>(std::min(own, neighbor)) << 32) |
			     static_cast<uint32_t>(std::max(own, neighbor));
	if (delta > 0)
	{
		if (!budget.insert(connections, key, scope))
			return false;
		if (++connections[key] != 1)
			return true;
		if (!budget.grow(zones[own].neighbors, 1, scope))
			return false;
		zones[own].neighbors.push_back(neighbor);
		if (!budget.grow(zones[neighbor].neighbors, 1, scope))
			return false;
		zones[neighbor].neighbors.push_back(own);
	}
	else
	{
		auto found = connections.find(key);
		if (found == connections.end() || --found->second != 0)
			return true;
		connections.erase(found);
		for (auto pair : { std::pair{ own, neighbor }, std::pair{ neighbor, own } })
		{
			auto &neighbors = zones[pair.first].neighbors;
			neighbors.erase(std::remove(neighbors.begin(), neighbors.end(),
						    pair.second),
					neighbors.end());
		}
	}
	if (ready)
		++topology_changes;
	for (auto pair : { std::pair{ own, neighbor }, std::pair{ neighbor, own } })
	{
		const activity_zone &source = zones[pair.first];
		activity_zone &target = zones[pair.second];
		const bool was_nearby = nearby_activity(target);
		const bool was_active = direct_activity(target);
		if (delta > 0)
		{
			target.adjacent_players += source.players;
			target.adjacent_corpses += source.corpses;
		}
		else
		{
			target.adjacent_players -=
				std::min(target.adjacent_players, source.players);
			target.adjacent_corpses -=
				std::min(target.adjacent_corpses, source.corpses);
			if (source.players || source.corpses)
				extend_grace(target);
		}
		if (delta < 0 && ((was_active && !direct_activity(target)) ||
				  (was_nearby && !nearby_activity(target))))
			target.last_departure_tick = ne_event_tick;
		if (ready && delta > 0 && activity_promoted(target, was_nearby, was_active))
		{
			++wake_promotions;
			if (!activity_wake_zone_bounded(budget, scope, pair.second, false,
							pair.second == neighbor ? to_room :
										  from_room))
				return false;
		}
	}

	return true;
}
bool activity_update_connections_bounded(activity_enter_budget &budget, size_t scope, P_obj object,
					 bool entering)
{
	if (!config.enabled || !object)
		return true;
	auto old = object_connections.find(object);
	if (old != object_connections.end())
	{
		for (int destination : old->second.destinations)
			if (!activity_change_connection_bounded(budget, scope, old->second.room,
								destination, -1))
				return false;
		object_connections.erase(old);
	}
	if (!entering || !OBJ_ROOM(object) || !valid_room(object->loc.room))
		return true;
	struct transient_guard
	{
		activity_enter_budget &budget;
		~transient_guard() { budget.transient = nullptr; }
	};
	if (!activity_storage_add(scope, sizeof(object_connection) + sizeof(transient_guard)))
		return false;
	if (!budget.admit(0, scope))
		return false;
	object_connection next{ object->loc.room, {} };
	transient_guard guard{ budget };
	budget.transient = &next.destinations;
	if (object->type == ITEM_TELEPORT ||
	    (obj_index && object->R_num >= 0 && object->R_num <= top_of_objt &&
	     (obj_index[object->R_num].func.obj == portal_door ||
	      obj_index[object->R_num].func.obj == portal_wormhole ||
	      obj_index[object->R_num].func.obj == portal_etherportal)))
	{
		if (object->value[0] >= 0)
		{
			if (!budget.grow(next.destinations, 1, scope))
				return false;
			next.destinations.push_back(real_room(object->value[0]));
		}
		else
		{
			// Random-zone teleporters have no single destination. Conservatively
			// protect every region in the possible destination zone.
			const int zone_room = real_room(object->value[3]);
			if (valid_room(zone_room) && world[zone_room].zone < MAX_ZONES)
			{
				const auto &source = zone_regions[world[zone_room].zone];
				if (source.size() > SIZE_MAX / sizeof(int))
					return false;
				if (!budget.admit(source.size() * sizeof(int), scope))
					return false;
				next.destinations = source;
			}
		}
	}
	else if (object->type == ITEM_SHIP || object->type == ITEM_BOAT)
	{
		if (P_ship ship = shipObjHash.find(object))
			for (int i = 0; i < ship->room_count; ++i)
			{
				if (!budget.grow(next.destinations, 1, scope))
					return false;
				next.destinations.push_back(real_room(ship->room[i].roomnum));
			}
		if (Ferry *ferry = get_ferry_from_obj(object->R_num); ferry && ferry->obj == object)
		{
			if (!budget.grow(next.destinations, ferry->rooms.size(), scope))
				return false;
			next.destinations.insert(next.destinations.end(), ferry->rooms.begin(),
						 ferry->rooms.end());
		}
	}
	if (next.destinations.empty())
		return true;
	for (int destination : next.destinations)
		if (!activity_change_connection_bounded(budget, scope, next.room, destination, 1))
			return false;
	if (!budget.insert(object_connections, object, scope))
		return false;
	object_connections.emplace(object, std::move(next));

	return true;
}
bool activity_adjust_one_bounded(activity_enter_budget &budget, size_t scope, int room,
				 bool player_reason, int delta, int zone_number, bool adjacent)
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
	if (delta < 0 &&
	    ((was_active && !direct_activity(zone)) || (was_nearby && !nearby_activity(zone))))
		zone.last_departure_tick = ne_event_tick;

	if (ready && delta > 0 && activity_promoted(zone, was_nearby, was_active))
	{
		++wake_promotions;
		if (!activity_wake_zone_bounded(budget, scope, zone_number, false,
						adjacent ? NOWHERE : room))
			return false;
	}

	return true;
}
struct activity_adjust_callback
{
	activity_enter_budget &budget;
	size_t scope;
	int room;
	bool player_reason;
	int delta;
	bool operator()(int zone, bool adjacent)
	{
		return activity_adjust_one_bounded(budget, scope, room, player_reason, delta, zone,
						   adjacent);
	}
};
bool activity_adjust_reason_bounded(activity_enter_budget &budget, size_t scope, int room,
				    bool player_reason, int delta)
{
	if (!config.enabled)
		return true;
	if (!activity_storage_add(scope, sizeof(activity_adjust_callback)))
		return false;
	if (!budget.admit(0, scope))
		return false;
	activity_adjust_callback callback{ budget, scope, room, player_reason, delta };
	const int own_zone = activity_zone_for_room(room);
	if (own_zone >= 0)
	{
		if (!callback(own_zone, false))
			return false;
		for (int neighbor : zones[own_zone].neighbors)
			if (!callback(neighbor, true))
				return false;
	}
	if (delta > 0)
	{
		if (!ready)
		{
			if (!budget.grow(rebuild_encounters, 1, scope))
				return false;
			rebuild_encounters.push_back(room);
		}
		else if (!activity_wake_encounter_bounded(budget, scope, room))
			return false;
	}

	return true;
}
bool activity_walk_subtree_bounded(activity_enter_budget &budget, size_t scope, P_obj object,
				   int room, bool entering)
{
	// Fixed traversal storage avoids heap allocation and recursive stack
	// exhaustion. Parent validation and sibling-cycle checks reject malformed
	// recovery graphs before repeatedly counting the same corpse.
	struct frame
	{
		P_obj object;
		P_obj next_child;
	};
	// Every live frame is fully assigned before use; do not clear 16 KiB for
	// each ordinary container transfer.
	if (!activity_storage_add(scope, sizeof(std::array<frame, 1024>) + sizeof(frame)))
		return false;
	if (!budget.admit(0, scope))
		return false;
	std::array<frame, 1024> stack;
	int depth = 0;
	unsigned int visited = 0;
	P_obj current = object;
	while (current && visited++ < 65536)
	{
		if (pc_corpse(current))
			if (!activity_adjust_reason_bounded(budget, scope, room, false,
							    entering ? 1 : -1))
				return false;
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

	return true;
}
bool activity_update_subtree_bounded(activity_enter_budget &budget, size_t scope, P_obj object,
				     bool entering)
{
	if (!config.enabled || !object)
		return true;
	/* Most object transfers are ordinary items with no contents.  Avoid even
	 * resolving their effective room; only a corpse or a non-empty subtree can
	 * change the activity policy. */
	if (!pc_corpse(object) && !object->contains)
		return true;

	const int room = effective_room(object);
	if (!valid_room(room))
		return true;

	if (!activity_walk_subtree_bounded(budget, scope, object, room, entering))
		return false;

	return true;
}
#endif
}
bool world_activity_object_enter_bounded(P_obj object, bool (*reserve)(size_t, void *) noexcept,
					 void *context, size_t outer_live) noexcept
{
	if (!reserve || !nevent_is_game_thread())
		return false;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	(void)object;
	(void)context;
	(void)outer_live;
	return false;
#else
	if (sizeof(activity_enter_budget) > SIZE_MAX - outer_live ||
	    !reserve(outer_live + sizeof(activity_enter_budget), context))
		return false;
	size_t activity, pending, output;
	if (!world_activity_storage_bytes(&activity) ||
	    !nevent_native_reschedule_storage_bytes(&pending) ||
	    !diagnostic_output_storage_bytes(&output) || activity > outer_live ||
	    pending > outer_live - activity || output > outer_live - activity - pending)
		return false;
	activity_enter_budget budget{ reserve, context,
				      outer_live - activity - pending - output +
					      sizeof(activity_enter_budget) };
	try
	{
		if (!activity_update_connections_bounded(budget, 0, object, true))
			return false;
		if (!activity_update_subtree_bounded(budget, 0, object, true))
			return false;
		return true;
	}
	catch (...)
	{
		return false;
	}
#endif
}
