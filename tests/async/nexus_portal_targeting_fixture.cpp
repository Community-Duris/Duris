#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utils.h"
#include "cmd/interp.h"
#include "net/comm.h"
#include "world/map.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <initializer_list>
#include <string>
#include <vector>

// Rooms, visibility, movement, output and the RNG are doubled. Keyword parsing,
// object targeting (generic_find) and the nexus proc itself are production code.
P_room world = nullptr;
struct zone_data *zone_table = nullptr;
P_index obj_index = nullptr;
int top_of_world = 0;

static std::vector<std::string> output;
static std::deque<int> rolls;

bool ac_can_see_obj(P_char, P_obj, int)
{
	return true;
}
void act(const char *text, int, P_char, P_obj, void *, int)
{
	output.emplace_back(text);
}
void send_to_char(const char *text, P_char)
{
	output.emplace_back(text);
}
int number(int low, int high)
{
	assert(!rolls.empty());
	const int result = rolls.front();
	rolls.pop_front();
	assert(result >= low && result <= high);
	return result;
}
void stop_fighting(P_char ch)
{
	ch->specials.fighting = nullptr;
}
void char_from_room(P_char ch)
{
	ch->in_room = NOWHERE;
}
bool char_to_room(P_char ch, int room, int)
{
	ch->in_room = room;
	return true;
}
void logit(const char *, const char *, ...) {}
int panic_corruption_int(const char *, const char *, ...)
{
	abort();
}
// generic_find() only reaches these for flags nexus never passes.
P_char get_char_room_vis(P_char, const char *)
{
	abort();
}
P_char get_char_vis(P_char, const char *)
{
	abort();
}
P_obj get_obj_vis(P_char, char *, int)
{
	abort();
}
P_obj get_obj_vis_no_tracks(P_char, char *, int)
{
	abort();
}

// PRODUCTION_FUNCTIONS

enum
{
	NEXUS_ROOM,
	PRIVATE_ROOM,
	PRIVATE_ZONE_ROOM,
	NO_TELEPORT_ROOM,
	OCEAN_MAP_ROOM,
	FIXED_DESTINATION_ROOM,
	SURFACE_MAP_ROOM,
	ROOM_COUNT
};

static room_data rooms[ROOM_COUNT];
static zone_data zones[2];
static index_data prototypes[3];
static obj_data nexus_portal, green_portal, chest;
static char portal_name[] = "portal";
static char green_name[] = "portal";
static char chest_name[] = "chest";

static void place(std::initializer_list<P_obj> contents)
{
	P_obj *link = &rooms[NEXUS_ROOM].contents;
	for (P_obj obj : contents)
	{
		*link = obj;
		link = &obj->next_content;
	}
	*link = nullptr;
}

enum outcome
{
	DECLINED,
	SHIFTED
};

// A shifted character must land on the surface map, never in the portal's fixed
// value[0] room. A declined command must leave everything for the next handler.
static void enter(char_data &ch, const char *argument, outcome expected,
		  std::initializer_list<int> choices = { SURFACE_MAP_ROOM })
{
	char arg[MAX_INPUT_LENGTH];
	snprintf(arg, sizeof(arg), "%s", argument);
	output.clear();
	rolls.assign(choices);
	ch.in_room = NEXUS_ROOM;
	rooms[NEXUS_ROOM].people = &ch;
	const bool taken = nexus(&nexus_portal, &ch, CMD_ENTER, arg);
	if (taken != (expected == SHIFTED))
	{
		fprintf(stderr, "%s: 'enter %s' was %s\n", GET_NAME(&ch), argument,
			taken ? "taken" : "declined");
		abort();
	}
	if (taken)
	{
		assert(ch.in_room == SURFACE_MAP_ROOM);
		assert(IS_SURFACE_MAP(ch.in_room));
		assert(rolls.empty());
		assert(output.size() == 5);
		assert(output[2].find("you are elsewhere") != std::string::npos);
	}
	else
	{
		assert(ch.in_room == NEXUS_ROOM);
		assert(output.empty());
		assert(rolls.size() == choices.size());
	}
}

int main()
{
	const int vnums[ROOM_COUNT] = { 7481, 600001, 600002, 600003, 600004, 7497, 508086 };
	for (int room = 0; room < ROOM_COUNT; ++room)
		rooms[room].number = vnums[room];
	SET_BIT(rooms[PRIVATE_ROOM].room_flags, ROOM_PRIVATE);
	rooms[PRIVATE_ZONE_ROOM].zone = 1;
	SET_BIT(zones[1].flags, ZONE_PRIVATE);
	SET_BIT(rooms[NO_TELEPORT_ROOM].room_flags, ROOM_NO_TELEPORT);
	rooms[OCEAN_MAP_ROOM].sector_type = SECT_OCEAN;
	world = rooms;
	zone_table = zones;
	top_of_world = ROOM_COUNT - 1;

	prototypes[0].virtual_number = 7371;
	prototypes[1].virtual_number = 59074;
	prototypes[2].virtual_number = 1000;
	obj_index = prototypes;
	for (obj_data *obj : { &nexus_portal, &green_portal, &chest })
	{
		obj->loc_p = LOC_ROOM;
		obj->loc.room = NEXUS_ROOM;
	}
	nexus_portal.R_num = 0;
	nexus_portal.name = portal_name;
	nexus_portal.type = ITEM_TELEPORT;
	nexus_portal.value[0] = 7497;
	nexus_portal.value[1] = CMD_ENTER;
	green_portal.R_num = 1;
	green_portal.name = green_name;
	green_portal.type = ITEM_TELEPORT;
	green_portal.value[0] = 59165;
	green_portal.value[1] = CMD_ENTER;
	chest.R_num = 2;
	chest.name = chest_name;

	char mortal_name[] = "Mortal", god_name[] = "Builder";
	char_data mortal{}, god{};
	mortal.player.name = mortal_name;
	mortal.player.level = 50;
	god.player.name = god_name;
	god.player.level = 62;

	place({ &nexus_portal, &chest });
	// The destination filter skips private, private-zone, no-teleport, ocean and
	// off-map rooms, including the fixed value[0] room, before settling.
	enter(mortal, "portal", SHIFTED,
	      { PRIVATE_ROOM, PRIVATE_ZONE_ROOM, NO_TELEPORT_ROOM, OCEAN_MAP_ROOM,
		FIXED_DESTINATION_ROOM, SURFACE_MAP_ROOM });
	enter(mortal, "the portal", SHIFTED);
	// The reported bug: numbered and vnum targeting used to fall through to the
	// generic ITEM_TELEPORT handler and land in value[0], room 7497.
	enter(mortal, "1.portal", SHIFTED);
	enter(god, "7371", SHIFTED);
	enter(mortal, "7371", DECLINED);
	enter(mortal, "chest", DECLINED);
	enter(mortal, "2.portal", DECLINED);
	const bool looked = nexus(&nexus_portal, &mortal, CMD_LOOK, portal_name);
	assert(!looked);

	// Room 59169 also holds green portals under the same keyword. Numbered
	// targeting picks exactly one object, so another portal keeps its own
	// destination; the bare keyword still means the nexus, as it always has.
	place({ &green_portal, &nexus_portal, &chest });
	enter(mortal, "1.portal", DECLINED);
	enter(mortal, "2.portal", SHIFTED);
	enter(mortal, "3.portal", DECLINED);
	enter(mortal, "portal", SHIFTED);

	puts("nexus portal targeting regression passed");
	return 0;
}
