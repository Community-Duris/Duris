#include "core/prototypes.h"
#include "core/utils.h"
#include "world/world_singletons.h"
#include "persistence/copyover.h"
#include "world/world_recovery_codec.h"
#include "world/generated_npc_state.h"
#include "world/graph.h"
#include "persistence/persistence_mode.h"
#include <cassert>
#include <cstdarg>
#include <cstring>
#include <unordered_map>
#include <vector>

P_char character_list = nullptr;
P_desc descriptor_list = nullptr;
P_obj object_list = nullptr;
index_data mob_indexes[4] = {};
index_data object_indexes[5] = {};
P_index mob_index = mob_indexes;
P_index obj_index = object_indexes;
room_data rooms[6] = {};
P_room world = rooms;
int top_of_world = 5;
int top_of_mobt = 3;
int top_of_objt = 4;
int top_of_zone_table = -1;
zone_data *zone_table = nullptr;
zone_data *zone = nullptr;
shop_data shops[2] = {};
shop_data *shop_index = shops;
int number_of_shops = 2;
const char *dirs[] = { "north", "east", "south", "west", "up", "down" };
const char *dirs2[] = { "north", "east", "south", "west", "up", "down" };
extern const int rev_dir[] = { 2, 3, 0, 1, 5, 4 };
std::unordered_map<P_char, P_char> mounts;
std::unordered_map<P_char, P_char> masters;
struct scheduled_event
{
	event_func callback;
	P_char rider;
};
std::unordered_map<P_char, scheduled_event> scheduled;
int next_id = 1;
int bfs_cur_marker = 1;
int players_landed = 0;
bool shop_save_succeeds = true;
int shops_saved = 0;
int saved_shop_rooms[2] = { -1, -1 };
persistence_mode persistence_mode_get()
{
	return PERSISTENCE_MODE_MARIADB_PRIMARY;
}
bool sql_save_shopkeeper(P_char ch, int shop)
{
	++shops_saved;
	saved_shop_rooms[shop] = shop_index[shop].shop_is_roaming ? world[ch->in_room].number :
								    shop_index[shop].in_room;
	return shop_save_succeeds;
}
bool sql_save_dirty_shopkeepers(bool)
{
	return true;
}

void logit(const char *, const char *, ...) {}
char *str_dup(const char *text)
{
	return strdup(text);
}
void str_free(const char *text)
{
	free(const_cast<char *>(text));
}
char affect_total(P_char, int)
{
	std::abort();
}
[[noreturn]] int panic_corruption_int(const char *, const char *, ...)
{
	std::abort();
}
std::string strip_ansi(const char *s)
{
	return s;
}
int real_room(int v)
{
	for (int i = 0; i <= top_of_world; ++i)
		if (rooms[i].number == v)
			return i;
	return -1;
}
int real_room0(int v)
{
	return real_room(v);
}
int real_mobile(int v)
{
	for (int i = 0; i <= top_of_mobt; ++i)
		if (mob_indexes[i].virtual_number == v)
			return i;
	return -1;
}
int real_mobile0(int v)
{
	return real_mobile(v);
}
P_char get_linked_char(P_char ch, ush_int type)
{
	if (type == LNK_PET && masters.count(ch))
		return masters.at(ch);
	return type == LNK_RIDING && mounts.count(ch) ? mounts.at(ch) : nullptr;
}
P_char get_linking_char(P_char ch, ush_int type)
{
	for (auto [rider, mount] : mounts)
		if (type == LNK_RIDING && mount == ch)
			return rider;
	return nullptr;
}
char_link_data *link_char(P_char rider, P_char mount, ush_int)
{
	mounts[rider] = mount;
	return nullptr;
}
void unlink_char(P_char rider, P_char, ush_int)
{
	mounts.erase(rider);
	++players_landed;
}
P_char find_player_by_name(const char *name)
{
	for (P_char ch = character_list; ch; ch = ch->next)
		if (!IS_NPC(ch) && !strcmp(GET_NAME(ch), name))
			return ch;
	return nullptr;
}
void char_from_room(P_char ch)
{
	if (ch->in_room >= 0)
	{
		P_char *at = &world[ch->in_room].people;
		while (*at && *at != ch)
			at = &(*at)->next_in_room;
		if (*at)
			*at = ch->next_in_room;
	}
	ch->in_room = -1;
	ch->next_in_room = nullptr;
}
bool char_to_room(P_char ch, int room, int)
{
	assert(room >= 0 && room <= top_of_world);
	ch->in_room = room;
	ch->next_in_room = world[room].people;
	world[room].people = ch;
	if (P_char rider = GET_RIDER(ch))
	{
		char_from_room(rider);
		char_to_room(rider, room, -1);
	}
	return true;
}
P_char read_mobile(int v, int mode)
{
	int r = mode == VIRTUAL ? real_mobile(v) : v;
	if (r < 0)
		return nullptr;
	P_char ch = new char_data{};
	ch->specials.act = ACT_ISNPC;
	ch->only.npc = new npc_only_data{};
	ch->only.npc->R_num = r;
	ch->only.npc->idnum = next_id++;
	ch->only.npc->shopkeeper_shop_id = -1;
	ch->in_room = -1;
	ch->next = character_list;
	character_list = ch;
	++mob_index[r].number;
	return ch;
}
P_obj read_object(int v, int mode)
{
	int r = v;
	if (mode == VIRTUAL)
	{
		r = -1;
		for (int i = 0; i <= top_of_objt; ++i)
			if (obj_index[i].virtual_number == v)
				r = i;
	}
	if (r < 0 || r > top_of_objt)
		return nullptr;
	P_obj obj = new obj_data{};
	obj->R_num = r;
	obj->loc_p = LOC_NOWHERE;
	obj->next = object_list;
	object_list = obj;
	++obj_index[r].number;
	return obj;
}
void obj_to_room(P_obj obj, int room)
{
	obj->loc_p = LOC_ROOM;
	obj->loc.room = room;
	obj->next_content = world[room].contents;
	world[room].contents = obj;
}
void obj_to_char(P_obj obj, P_char ch)
{
	obj->loc_p = LOC_CARRIED;
	obj->loc.carrying = ch;
	obj->next_content = ch->carrying;
	ch->carrying = obj;
}
void obj_from_char(P_obj obj)
{
	P_obj *at = &obj->loc.carrying->carrying;
	while (*at != obj)
		at = &(*at)->next_content;
	*at = obj->next_content;
	obj->next_content = nullptr;
	obj->loc_p = LOC_NOWHERE;
}
void obj_from_obj(P_obj obj)
{
	P_obj *at = &obj->loc.inside->contains;
	while (*at != obj)
		at = &(*at)->next_content;
	*at = obj->next_content;
	obj->next_content = nullptr;
	obj->loc_p = LOC_NOWHERE;
}
void equip_char(P_char ch, P_obj obj, int slot, int)
{
	ch->equipment[slot] = obj;
	obj->loc_p = LOC_WORN;
	obj->loc.wearing = ch;
}
P_obj unequip_char(P_char ch, int slot, bool)
{
	P_obj obj = ch->equipment[slot];
	ch->equipment[slot] = nullptr;
	obj->loc_p = LOC_NOWHERE;
	return obj;
}
void extract_obj(P_obj obj, int)
{
	if (OBJ_ROOM(obj))
	{
		P_obj *at = &world[obj->loc.room].contents;
		while (*at != obj)
			at = &(*at)->next_content;
		*at = obj->next_content;
	}
	if (obj->loc_p == LOC_CARRIED)
		obj_from_char(obj);
	P_obj *at = &object_list;
	while (*at != obj)
		at = &(*at)->next;
	*at = obj->next;
	--obj_index[obj->R_num].number;
	delete obj;
}
void extract_char(P_char ch)
{
	assert(!GET_RIDER(ch));
	scheduled.erase(ch);
	while (ch->carrying)
		extract_obj(ch->carrying);
	for (int i = 0; i < MAX_WEAR; ++i)
		if (ch->equipment[i])
			extract_obj(unequip_char(ch, i));
	char_from_room(ch);
	P_char *at = &character_list;
	while (*at != ch)
		at = &(*at)->next;
	*at = ch->next;
	if (IS_NPC(ch))
	{
		--mob_index[GET_RNUM(ch)].number;
		delete ch->only.npc;
	}
	delete ch;
}
int shop_producing(P_obj obj, int shop)
{
	for (int i = 0; i < shop_index[shop].number_items_produced; ++i)
		if (shop_index[shop].producing[i] == obj->R_num)
			return 1;
	return 0;
}
bool dijkstra(int from, int to, valid_edge_func *, std::vector<int> &path)
{
	path = from < to ? std::vector<int>{ 1, 1 } : std::vector<int>{ 3, 3 };
	return true;
}
int flying_transport(P_char, P_char, int, char *)
{
	return 0;
}
nevent_handle nevent_find_next(P_char ch, event_func_type f)
{
	return { scheduled.count(ch) && scheduled.at(ch).callback == f ?
			 reinterpret_cast<P_nevent>(ch) :
			 nullptr,
		 0 };
}
nevent_cancel_result nevent_cancel(nevent_handle h)
{
	if (h.event)
		scheduled.erase(reinterpret_cast<P_char>(h.event));
	return {};
}
nevent_schedule_result add_event(event_func f, int, P_char ch, P_char rider, P_obj, int,
				 const void *, int)
{
	assert(!scheduled.count(ch));
	scheduled[ch] = { f, rider };
	return {};
}
void act(const char *, int, P_char, P_obj, void *, int) {}
void do_dismount(P_char rider, char *, int)
{
	mounts.erase(rider);
}

affected_type *affect_to_char(P_char, affected_type *)
{
	return nullptr;
}

// TRANSPORT_PRODUCTION

P_char mob_at(int rnum, int room)
{
	P_char ch = read_mobile(rnum, REAL);
	char_to_room(ch, room, -1);
	GET_BIRTHPLACE(ch) = world[room].number;
	return ch;
}
P_char dragon()
{
	for (P_char ch = character_list; ch; ch = ch->next)
		if (IS_NPC(ch) && GET_RNUM(ch) == 0)
			return ch;
	return nullptr;
}
void run_event(P_char ch)
{
	auto event = scheduled.at(ch);
	scheduled.erase(ch);
	event.callback(ch, event.rider, nullptr, nullptr);
}
void clear_world()
{
	mounts.clear();
	masters.clear();
	while (character_list)
		extract_char(character_list);
	while (object_list)
		extract_obj(object_list);
}

int main()
{
	rooms[0].number = 543662;
	rooms[1].number = 543663;
	rooms[2].number = 519043;
	rooms[3].number = 29437;
	rooms[4].number = 29438;
	rooms[5].number = 99999;
	room_direction_data east[2] = {}, west[2] = {};
	for (int i = 0; i < 2; ++i)
	{
		east[i].to_room = i + 1;
		rooms[i].dir_option[1] = &east[i];
		west[i].to_room = i;
		rooms[i + 1].dir_option[3] = &west[i];
	}
	mob_indexes[0].virtual_number = 47030;
	mob_indexes[1].virtual_number = 47028;
	mob_indexes[2].virtual_number = 29429;
	mob_indexes[3].virtual_number = 1234;
	for (int i = 0; i < 5; ++i)
		obj_index[i].virtual_number = 420 + i;
	shops[0].keeper = shops[1].keeper = 2;
	shops[0].in_room = 29437;
	shops[0].shop_is_roaming = 0;
	shops[1].in_room = 29438;
	shops[1].shop_is_roaming = 0;
	shops[0].producing[0] = 1;
	shops[1].producing[0] = 2;
	shops[0].number_items_produced = shops[1].number_items_produced = 1;

	// Cold boot and repeated initializer calls, including a missing sign.
	initialize_transport();
	initialize_transport();
	assert(mob_index[0].number == 1 && mob_index[1].number == 1 && obj_index[0].number == 2);
	extract_obj(world[0].contents);
	initialize_transport();
	assert(obj_index[0].number == 2);
	P_char keeper = mob_at(2, 3);
	P_obj stock = read_object(3, REAL);
	obj_to_char(stock, keeper);
	P_obj equipment = read_object(4, REAL);
	equip_char(keeper, equipment, 1, 0);
	P_char other = mob_at(2, 4);
	P_char ordinary = mob_at(2, 5);
	masters[ordinary] = keeper; // controlled template copy is never a shop candidate
	char_from_room(keeper);
	char_to_room(keeper, 5, -1);
	bind_shopkeeper(keeper, 0);
	char_from_room(other);
	char_to_room(other, 5, -1);
	GET_BIRTHPLACE(other) = world[5].number;
	assert(singleton_shop_id(keeper) == 0);
	assert(singleton_shop_id(other) < 0); // unbound fixed shop away from home
	bind_shopkeeper(other, 1);
	assert(singleton_shop_id(other) == 1); // explicit binding survives off-home room
	P_char controlled = mob_at(2, 5);
	masters[controlled] = keeper;
	assert(singleton_shop_id(controlled) < 0);
	assert(singleton_shop_id(ordinary) < 0);
	shops[0].shop_is_roaming = 1;
	shops[0].in_room = 0; // room 0 is configuration, not the live roaming room
	assert(snapshot_shopkeepers_for_copyover() && shops_saved == 2);
	assert(saved_shop_rooms[0] == world[5].number && saved_shop_rooms[1] == shops[1].in_room);
	shop_save_succeeds = false;
	assert(!snapshot_shopkeepers_for_copyover());
	shop_save_succeeds = true;

	// Five successful Redis clean/crash generations and fallback reconciliation.
	for (int cycle = 0; cycle < 5; ++cycle)
	{
		remember_boot_shopkeepers();
		for (int duplicate = 0; duplicate < 5; ++duplicate)
		{
			mob_at(2, 3);
			mob_at(0, 0);
			obj_to_room(read_object(0, REAL), 0);
		}
		reconcile_shopkeepers(false);
		initialize_transport();
		assert(mob_index[0].number == 1 && obj_index[0].number == 2 &&
		       mob_index[2].number == 4);
		assert(keeper->equipment[1] == equipment && stock->loc.carrying == keeper);
		assert(singleton_shop_id(other) == 1 && ordinary->in_room == 5);
	}
	remember_boot_shopkeepers();
	reconcile_shopkeepers(false);
	initialize_transport();

	// Already duplicated file snapshots retain distinct stock and equipment.
	remember_boot_shopkeepers();
	clear_world();
	remember_boot_shopkeepers();
	keeper = mob_at(2, 3);
	obj_to_char(read_object(3, REAL), keeper);
	P_char duplicate = mob_at(2, 3);
	obj_to_char(read_object(4, REAL), duplicate);
	reconcile_shopkeepers(true);
	assert(mob_index[2].number == 1 && obj_index[3].number == 1 && obj_index[4].number == 1);
	keeper = world[3].people;
	equip_char(keeper, read_object(2, REAL), 1, 0);
	for (int cycle = 0; cycle < 5; ++cycle)
	{
		char snapshot[4096] = {};
		const int bytes = copyover_write_mob_to_buffer(keeper, snapshot, sizeof(snapshot));
		assert(bytes > 0);
		extract_char(keeper);
		size_t consumed = 0;
		keeper = copyover_restore_mob_from_buffer(snapshot, bytes, &consumed);
		assert(keeper && consumed == static_cast<size_t>(bytes));
		reconcile_shopkeepers(true);
		assert(mob_index[2].number == 1 && obj_index[3].number == 1 &&
		       obj_index[4].number == 1);
		assert(keeper->equipment[1] && keeper->equipment[1]->R_num == 2);
	}

	// A passenger survives five encoded recovery cycles away from the origin.
	initialize_transport();
	char player_name[] = "Passenger";
	P_char rider = new char_data{};
	rider->player.name = player_name;
	rider->in_room = -1;
	rider->next = character_list;
	character_list = rider;
	char_to_room(rider, 1, -1);
	P_char flight = dragon();
	char_from_room(flight);
	char_to_room(flight, 1, -1);
	link_char(rider, flight, LNK_RIDING);
	TRANSPORT_STATE(flight) = TRANSPORT_STATE_MOVING;
	TRANSPORT_ROUTE(flight) = 16;
	TRANSPORT_STEP(flight) = 1;
	for (int cycle = 0; cycle < 5; ++cycle)
	{
		char snapshot[4096] = {};
		const int bytes = copyover_write_mob_to_buffer(flight, snapshot, sizeof(snapshot));
		assert(bytes > 0);
		unsigned char wire[1024] = {};
		size_t size = 0;
		assert(world_recovery_encode_record(world_recovery_record_type::mob,
						    reinterpret_cast<unsigned char *>(snapshot),
						    bytes, wire, sizeof(wire), &size));
		std::vector<unsigned char> restored;
		assert(world_recovery_decode_record(world_recovery_record_type::mob, wire, size,
						    &restored));
		copyover_mob decoded = {};
		memcpy(&decoded, restored.data(), sizeof(decoded));
		assert(!strcmp(decoded.transport.rider, player_name) &&
		       decoded.transport.step == 1);
		mounts.clear();
		extract_char(flight);
		size_t consumed = 0;
		flight = copyover_restore_mob_from_buffer(
			reinterpret_cast<const char *>(restored.data()), restored.size(),
			&consumed);
		assert(flight && consumed == restored.size());
		mob_at(0, 0);
		initialize_transport();
		initialize_transport();
		assert(dragon() == flight && GET_RIDER(flight) == rider && rider->in_room == 1);
		assert(mob_index[0].number == 1 && scheduled.size() == 1);
	}
	run_event(flight);
	assert(flight->in_room == 2 && rider->in_room == 2);
	run_event(flight);
	assert(!GET_MOUNT(rider)); // arrival, return scheduled
	run_event(flight);
	run_event(flight);
	run_event(flight);
	assert(flight->in_room == 0 && TRANSPORT_STATE(flight) == TRANSPORT_STATE_WAITING &&
	       scheduled.empty());

	// Old snapshots have no route metadata; safely return that dragon home.
	char_from_room(flight);
	char_to_room(flight, 1, -1);
	transport_restore(flight, {});
	initialize_transport();
	assert(flight->in_room == 0 && TRANSPORT_ROUTE(flight) == -1);

	// Two occupied duplicates: land the extra passenger before extraction.
	P_char rider2 = new char_data{};
	rider2->in_room = -1;
	rider2->next = character_list;
	character_list = rider2;
	duplicate = mob_at(0, 1);
	char_to_room(rider2, 1, -1);
	link_char(rider2, duplicate, LNK_RIDING);
	char_from_room(rider);
	char_to_room(rider, 0, -1);
	link_char(rider, flight, LNK_RIDING);
	initialize_transport();
	assert(mob_index[0].number == 1 && players_landed > 0);
	assert((!GET_MOUNT(rider) && rider->in_room == 0) ||
	       (!GET_MOUNT(rider2) && rider2->in_room == 0));
	clear_world();
	puts("singleton counts, shared shops, stock, five recovery cycles, riders and travel passed");
}
