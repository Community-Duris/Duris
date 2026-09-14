#include "economy/collector_presence.h"

#include "core/prototypes.h"
#include "core/utility.h"
#include "core/utils.h"
#include "economy/auction_room_registry.h"
#include "economy/collector_catalog_cache.h"
#include "economy/collector_runtime.h"
#include "world/vnum.mob.h"

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <limits>

extern P_char character_list;
extern P_index mob_index;
extern P_room world;
extern const int top_of_world;

namespace
{
using clock_type = std::chrono::steady_clock;
constexpr auto RECONCILE_INTERVAL = std::chrono::seconds(30);

int collector_rnum = -1;
clock_type::time_point next_reconcile = {};
uint64_t observed_catalog_revision = std::numeric_limits<uint64_t>::max();
size_t observed_available_count = std::numeric_limits<size_t>::max();
bool observed_ready = false;
collector_presence_health health = {};

int collector_special(P_char collector, P_char, int, char *)
{
	(void)collector;
	return FALSE;
}

bool presence_desired()
{
	return health.initialized && collector_catalog_cache_ready() &&
	       collector_runtime_available_count() > 0;
}

void protect(P_char collector)
{
	if (!collector_presence_is_npc(collector))
		return;
	SET_BIT(collector->specials.act, ACT_SPEC | ACT_SENTINEL | ACT_ISNPC | ACT_NICE_THIEF |
						 ACT_NO_SUMMON | ACT_NO_BASH | ACT_IGNORE);
	SET_BIT(collector->specials.act2, ACT2_NO_LURE);
	collector->only.npc->aggro_flags = 0;
	collector->only.npc->aggro2_flags = 0;
	collector->only.npc->aggro3_flags = 0;
	SET_POS(collector, POS_STANDING | STAT_NORMAL);
	collector->only.npc->default_pos = POS_STANDING | STAT_NORMAL;
	if (IS_FIGHTING(collector))
		stop_fighting(collector);
	if (collector->following)
		stop_follower(collector);
}

std::array<int, AUCTION_HOUSE_REGISTERED_ROOM_COUNT> resolve_rooms(size_t *valid_count)
{
	std::array<int, AUCTION_HOUSE_REGISTERED_ROOM_COUNT> rooms = {};
	rooms.fill(NOWHERE);
	*valid_count = 0;
	for (size_t index = 0; index < rooms.size(); ++index)
	{
		int room_vnum = NOWHERE;
		if (!auction_house_registered_room_vnum(index, &room_vnum))
			continue;
		const int room_rnum = real_room(room_vnum);
		if (room_rnum < 0 || room_rnum > top_of_world)
			continue;
		rooms[index] = room_rnum;
		++*valid_count;
	}
	return rooms;
}

int room_slot(int room_rnum, const std::array<int, AUCTION_HOUSE_REGISTERED_ROOM_COUNT> &rooms)
{
	for (size_t index = 0; index < rooms.size(); ++index)
		if (rooms[index] == room_rnum)
			return static_cast<int>(index);
	return -1;
}

void remove_collector(P_char collector)
{
	if (!collector_presence_is_npc(collector))
		return;
	extract_char(collector);
	++health.removed;
}

void reconcile()
{
	++health.reconciliations;
	health.desired = presence_desired();
	health.active_rooms = 0;
	health.live_collectors = 0;
	size_t registered_rooms = 0;
	const auto rooms = resolve_rooms(&registered_rooms);
	health.registered_rooms = registered_rooms;
	std::array<bool, AUCTION_HOUSE_REGISTERED_ROOM_COUNT> occupied = {};

	for (P_char collector = character_list, next = nullptr; collector; collector = next)
	{
		next = collector->next;
		if (!collector_presence_is_npc(collector))
			continue;
		if (!health.desired)
		{
			remove_collector(collector);
			continue;
		}
		const int slot = room_slot(collector->in_room, rooms);
		if (slot < 0)
		{
			++health.off_registry_removed;
			remove_collector(collector);
			continue;
		}
		if (occupied[static_cast<size_t>(slot)])
		{
			++health.duplicates_removed;
			remove_collector(collector);
			continue;
		}
		occupied[static_cast<size_t>(slot)] = true;
		protect(collector);
		++health.active_rooms;
		++health.live_collectors;
	}

	if (health.desired)
		for (size_t index = 0; index < rooms.size(); ++index)
		{
			if (rooms[index] == NOWHERE || occupied[index])
				continue;
			P_char collector = read_mobile(VMOB_COLLECTOR_ANTIQUITIES, VIRTUAL);
			if (!collector)
			{
				++health.spawn_failures;
				continue;
			}
			protect(collector);
			// char_to_room() extracts the mobile itself when placement fails.
			if (!char_to_room(collector, rooms[index], -1))
			{
				++health.spawn_failures;
				continue;
			}
			occupied[index] = true;
			++health.spawned;
			++health.active_rooms;
			++health.live_collectors;
		}

	next_reconcile = clock_type::now() + RECONCILE_INTERVAL;
}
} // namespace

bool collector_presence_init(void)
{
	collector_presence_shutdown();
	collector_rnum = real_mobile(VMOB_COLLECTOR_ANTIQUITIES);
	if (collector_rnum < 0)
	{
		persistence_alert(AVATAR, "collector", "presence", "none", "none",
				  "prototype_missing", "mob_vnum=%d", VMOB_COLLECTOR_ANTIQUITIES);
		return false;
	}
	if (mob_index[collector_rnum].func.mob &&
	    mob_index[collector_rnum].func.mob != collector_special)
	{
		++health.prototype_conflicts;
		persistence_alert(AVATAR, "collector", "presence", "none", "none",
				  "prototype_conflict", "mob_vnum=%d", VMOB_COLLECTOR_ANTIQUITIES);
		collector_rnum = -1;
		return false;
	}
	mob_index[collector_rnum].func.mob = collector_special;
	health.initialized = true;
	observed_catalog_revision = std::numeric_limits<uint64_t>::max();
	observed_available_count = std::numeric_limits<size_t>::max();
	observed_ready = false;
	next_reconcile = {};
	return true;
}

void collector_presence_pulse(void)
{
	if (!health.initialized)
		return;
	const bool ready = collector_catalog_cache_ready();
	const uint64_t revision = collector_runtime_catalog_revision();
	const size_t available = collector_runtime_available_count();
	const auto now = clock_type::now();
	if (ready != observed_ready || revision != observed_catalog_revision ||
	    available != observed_available_count || next_reconcile == clock_type::time_point{} ||
	    now >= next_reconcile)
	{
		observed_ready = ready;
		observed_catalog_revision = revision;
		observed_available_count = available;
		reconcile();
	}
}

void collector_presence_shutdown(void)
{
	if (collector_rnum >= 0)
	{
		for (P_char collector = character_list, next = nullptr; collector; collector = next)
		{
			next = collector->next;
			if (collector_presence_is_npc(collector))
				extract_char(collector);
		}
		if (mob_index && mob_index[collector_rnum].func.mob == collector_special)
			mob_index[collector_rnum].func.mob = nullptr;
	}
	collector_rnum = -1;
	next_reconcile = {};
	observed_catalog_revision = std::numeric_limits<uint64_t>::max();
	observed_available_count = std::numeric_limits<size_t>::max();
	observed_ready = false;
	health = {};
}

bool collector_presence_is_npc(P_char character)
{
	return collector_rnum >= 0 && character && IS_NPC(character) && character->only.npc &&
	       GET_RNUM(character) == collector_rnum;
}

bool collector_presence_room_active(int room_rnum)
{
	if (!presence_desired() || room_rnum < 0 || room_rnum > top_of_world ||
	    !auction_house_is_registered_room_vnum(ROOM_VNUM(room_rnum)))
		return false;
	size_t count = 0;
	for (P_char character = world[room_rnum].people; character;
	     character = character->next_in_room)
		if (collector_presence_is_npc(character) && ++count > 1)
			return false;
	return count == 1;
}

collector_presence_health collector_presence_health_copy(void)
{
	return health;
}
