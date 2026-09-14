#include "economy/collector_presence.h"

#include "core/prototypes.h"
#include "core/utils.h"
#include "economy/auction_room_registry.h"
#include "world/db.h"
#include "world/vnum.mob.h"

#include <array>
#include <cassert>
#include <cstdarg>
#include <cstdlib>

extern P_char character_list;
extern P_room world;
extern const int top_of_world;

namespace
{
constexpr size_t registered_count = AUCTION_HOUSE_REGISTERED_ROOM_COUNT;
std::array<room_data, registered_count + 1> rooms = {};
std::array<index_data, 1> indexes = {};
bool cache_ready = false;
uint64_t catalog_revision = 0;
size_t available_count = 0;
int missing_room_index = -1;
bool fail_next_placement = false;
size_t alerts = 0;
size_t stopped_fights = 0;
size_t stopped_followers = 0;

size_t live_collectors()
{
	size_t count = 0;
	for (P_char character = character_list; character; character = character->next)
		if (collector_presence_is_npc(character))
			++count;
	return count;
}

size_t collectors_in_room(int room)
{
	size_t count = 0;
	for (P_char character = world[room].people; character; character = character->next_in_room)
		if (collector_presence_is_npc(character))
			++count;
	return count;
}

P_char collector_in_room(int room)
{
	for (P_char character = world[room].people; character; character = character->next_in_room)
		if (collector_presence_is_npc(character))
			return character;
	return nullptr;
}

void unlink_room(P_char character)
{
	if (!character || character->in_room < 0 || character->in_room > top_of_world)
		return;
	P_char *link = &world[character->in_room].people;
	while (*link && *link != character)
		link = &(*link)->next_in_room;
	if (*link == character)
		*link = character->next_in_room;
	character->next_in_room = nullptr;
	character->in_room = NOWHERE;
}

void relocate(P_char character, int room)
{
	unlink_room(character);
	character->in_room = room;
	character->next_in_room = world[room].people;
	world[room].people = character;
}

int conflicting_special(P_char, P_char, int, char *)
{
	return FALSE;
}
} // namespace

P_char character_list = nullptr;
P_index mob_index = indexes.data();
P_room world = rooms.data();
extern const int top_of_world = static_cast<int>(registered_count);

bool collector_catalog_cache_ready()
{
	return cache_ready;
}

uint64_t collector_runtime_catalog_revision()
{
	return catalog_revision;
}

size_t collector_runtime_available_count()
{
	return available_count;
}

int real_mobile(const int vnum)
{
	return vnum == VMOB_COLLECTOR_ANTIQUITIES ? 0 : -1;
}

int real_room(const int vnum)
{
	for (size_t index = 0; index < registered_count; ++index)
	{
		int registered_vnum = NOWHERE;
		assert(auction_house_registered_room_vnum(index, &registered_vnum));
		if (vnum == registered_vnum)
			return static_cast<int>(index) == missing_room_index ?
				       NOWHERE :
				       static_cast<int>(index);
	}
	return NOWHERE;
}

P_char read_mobile(int vnum, int mode)
{
	assert(vnum == VMOB_COLLECTOR_ANTIQUITIES);
	assert(mode == VIRTUAL);
	P_char character = new char_data{};
	character->only.npc = new npc_only_data{};
	character->only.npc->R_num = 0;
	character->specials.act = ACT_ISNPC;
	character->specials.position = POS_STANDING | STAT_NORMAL;
	character->in_room = NOWHERE;
	character->next = character_list;
	character_list = character;
	return character;
}

bool char_to_room(P_char character, int room, int)
{
	assert(character);
	if (fail_next_placement)
	{
		fail_next_placement = false;
		extract_char(character);
		return false;
	}
	assert(character->in_room == NOWHERE);
	assert(room >= 0 && room <= top_of_world);
	character->in_room = room;
	character->next_in_room = world[room].people;
	world[room].people = character;
	return true;
}

void extract_char(P_char character)
{
	assert(character);
	unlink_room(character);
	P_char *link = &character_list;
	while (*link && *link != character)
		link = &(*link)->next;
	assert(*link == character);
	*link = character->next;
	delete character->only.npc;
	delete character;
}

void stop_fighting(P_char character)
{
	assert(character);
	character->specials.fighting = nullptr;
	++stopped_fights;
}

void stop_follower(P_char character)
{
	assert(character);
	character->following = nullptr;
	++stopped_followers;
}

void persistence_alert(int, const char *, const char *, const char *, const char *, const char *,
		       const char *, ...)
{
	++alerts;
}

[[noreturn]] int panic_corruption_int(const char *, const char *, ...)
{
	std::abort();
}

int main()
{
	indexes[0].virtual_number = VMOB_COLLECTOR_ANTIQUITIES;
	for (size_t index = 0; index < registered_count; ++index)
	{
		int vnum = NOWHERE;
		assert(auction_house_registered_room_vnum(index, &vnum));
		rooms[index].number = vnum;
	}
	rooms[registered_count].number = 999999;

	assert(collector_presence_init());
	assert(indexes[0].func.mob != nullptr);
	assert(collector_presence_health_copy().initialized);
	collector_presence_pulse();
	assert(live_collectors() == 0);

	cache_ready = true;
	available_count = 1;
	catalog_revision = 1;
	collector_presence_pulse();
	assert(live_collectors() == registered_count);
	for (size_t room = 0; room < registered_count; ++room)
	{
		assert(collectors_in_room(static_cast<int>(room)) == 1);
		assert(collector_presence_room_active(static_cast<int>(room)));
		P_char collector = collector_in_room(static_cast<int>(room));
		assert(collector_presence_is_npc(collector));
		assert(IS_SET(collector->specials.act, ACT_SPEC));
		assert(IS_SET(collector->specials.act, ACT_SENTINEL));
		assert(IS_SET(collector->specials.act, ACT_NICE_THIEF));
		assert(IS_SET(collector->specials.act, ACT_NO_SUMMON));
		assert(IS_SET(collector->specials.act, ACT_NO_BASH));
		assert(IS_SET(collector->specials.act, ACT_IGNORE));
		assert(IS_SET(collector->specials.act2, ACT2_NO_LURE));
	}
	assert(!collector_presence_room_active(static_cast<int>(registered_count)));
	auto health = collector_presence_health_copy();
	assert(health.desired);
	assert(health.registered_rooms == registered_count);
	assert(health.active_rooms == registered_count);
	assert(health.spawned == registered_count);

	P_char duplicate = read_mobile(VMOB_COLLECTOR_ANTIQUITIES, VIRTUAL);
	assert(char_to_room(duplicate, 0, -1));
	assert(!collector_presence_room_active(0));
	++catalog_revision;
	collector_presence_pulse();
	assert(live_collectors() == registered_count);
	assert(collector_presence_room_active(0));
	assert(collector_presence_health_copy().duplicates_removed == 1);

	P_char displaced = collector_in_room(1);
	assert(displaced);
	relocate(displaced, static_cast<int>(registered_count));
	++catalog_revision;
	collector_presence_pulse();
	assert(live_collectors() == registered_count);
	assert(collectors_in_room(static_cast<int>(registered_count)) == 0);
	assert(collector_presence_health_copy().off_registry_removed == 1);

	P_char unprotected = collector_in_room(2);
	assert(unprotected);
	unprotected->specials.act = ACT_ISNPC;
	unprotected->specials.act2 = 0;
	unprotected->only.npc->aggro_flags = 123;
	unprotected->only.npc->aggro2_flags = 456;
	unprotected->only.npc->aggro3_flags = 789;
	char_data dummy = {};
	unprotected->specials.fighting = &dummy;
	unprotected->following = &dummy;
	++catalog_revision;
	collector_presence_pulse();
	assert(IS_SET(unprotected->specials.act, ACT_SENTINEL | ACT_NO_SUMMON | ACT_IGNORE));
	assert(IS_SET(unprotected->specials.act2, ACT2_NO_LURE));
	assert(!unprotected->specials.fighting);
	assert(!unprotected->following);
	assert(!unprotected->only.npc->aggro_flags);
	assert(!unprotected->only.npc->aggro2_flags);
	assert(!unprotected->only.npc->aggro3_flags);
	assert(stopped_fights == 1);
	assert(stopped_followers == 1);

	missing_room_index = 3;
	++catalog_revision;
	collector_presence_pulse();
	assert(live_collectors() == registered_count - 1);
	health = collector_presence_health_copy();
	assert(health.registered_rooms == registered_count - 1);
	assert(health.active_rooms == registered_count - 1);
	missing_room_index = -1;
	fail_next_placement = true;
	++catalog_revision;
	collector_presence_pulse();
	assert(live_collectors() == registered_count - 1);
	assert(collector_presence_health_copy().spawn_failures == 1);
	++catalog_revision;
	collector_presence_pulse();
	assert(live_collectors() == registered_count);

	available_count = 0;
	++catalog_revision;
	collector_presence_pulse();
	assert(live_collectors() == 0);
	assert(!collector_presence_room_active(0));
	health = collector_presence_health_copy();
	assert(!health.desired);
	assert(health.active_rooms == 0);

	collector_presence_shutdown();
	assert(indexes[0].func.mob == nullptr);
	assert(!collector_presence_health_copy().initialized);
	indexes[0].func.mob = conflicting_special;
	assert(!collector_presence_init());
	assert(indexes[0].func.mob == conflicting_special);
	assert(alerts == 1);
	health = collector_presence_health_copy();
	assert(!health.initialized);
	assert(health.prototype_conflicts == 1);
	indexes[0].func.mob = nullptr;
	collector_presence_shutdown();
}
