// The Python runner inserts production code; these doubles model only its I/O.
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

constexpr int MAX_WEAR = 4, MAX_OBJ_AFFECT = 4, NOWHERE = -1;
constexpr int VIRTUAL = 0, REAL = 1, LOG_DEBUG = 0, LOG_MOB = 1;
constexpr int LOC_CARRIED = 1, LOC_INSIDE = 2;
constexpr int STRUNG_KEYS = 1, STRUNG_DESC2 = 2, STRUNG_DESC1 = 4, STRUNG_DESC3 = 8;
struct Character;
struct Object;
using P_char = Character *;
using P_obj = Object *;
struct Object
{
	int R_num = 0, weight = 0, cost = 0, value[8] = {}, str_mask = 0, loc_p = 0;
	long timer[1] = {};
	unsigned long extra_flags = 0;
	char *name = nullptr, *short_description = nullptr, *description = nullptr,
	     *action_description = nullptr;
	struct
	{
		int location = 0, modifier = 0;
	} affected[MAX_OBJ_AFFECT];
	P_obj next_content = nullptr, contains = nullptr;
	struct
	{
		P_obj inside = nullptr;
		P_char carrying = nullptr;
	} loc;
};
struct Character
{
	int rnum = 0, in_room = NOWHERE, birthplace = 0, affects = 0;
	bool npc = true, shopkeeper = true;
	P_char next = nullptr, next_in_room = nullptr;
	P_obj carrying = nullptr, equipment[MAX_WEAR] = {};
};
struct affected_type
{
	int type, duration, modifier, location;
	unsigned long bitvector, bitvector2, bitvector3, bitvector4, bitvector5;
};
struct
{
	int virtual_number = 100, number = 0, limit = 0;
} mob_index[3];
struct
{
	int number = 200;
	P_char people = nullptr;
} world[3];
struct
{
	int keeper = 0, dirty = 0, number_items_produced = 0, producing[4] = {};
} shop_index[4];
int number_of_shops = 4, top_of_world = 2;
P_char character_list = nullptr;
std::vector<P_obj> objects;
int births = 0;
#define GET_RNUM(ch) ((ch)->rnum)
#define GET_BIRTHPLACE(ch) ((ch)->birthplace)
#define IS_NPC(ch) ((ch)->npc)
#define IS_SHOPKEEPER(ch) ((ch)->npc && (ch)->shopkeeper)
void logit(int, const char *, ...) {}
int real_mobile(int vnum)
{
	return vnum >= 100 && vnum <= 102 ? vnum - 100 : -1;
}
int real_room(int vnum)
{
	return vnum >= 200 && vnum <= 202 ? vnum - 200 : NOWHERE;
}
int real_object(int vnum)
{
	return vnum;
}
P_char read_mobile(int id, int type)
{
	const int rnum = type == VIRTUAL ? real_mobile(id) : id;
	if (rnum < 0)
		return nullptr;
	P_char ch = new Character;
	ch->rnum = rnum;
	ch->next = character_list;
	character_list = ch;
	++mob_index[rnum].number;
	++births;
	return ch;
}
void unlink_room(P_char ch)
{
	if (ch->in_room == NOWHERE)
		return;
	P_char *link = &world[ch->in_room].people;
	while (*link != ch)
	{
		assert(*link);
		link = &(*link)->next_in_room;
	}
	*link = ch->next_in_room;
	ch->in_room = NOWHERE;
}
void extract_char(P_char ch)
{
	unlink_room(ch);
	P_char *link = &character_list;
	while (*link != ch)
	{
		assert(*link);
		link = &(*link)->next;
	}
	*link = ch->next;
	--mob_index[ch->rnum].number;
	delete ch;
}
void char_to_room(P_char ch, int room, int)
{
	assert(ch->in_room == NOWHERE);
	ch->next_in_room = world[room].people;
	world[room].people = ch;
	ch->in_room = room;
}
P_obj read_object(int rnum, int)
{
	P_obj obj = new Object;
	obj->R_num = rnum;
	objects.push_back(obj);
	return obj;
}
void equip_char(P_char ch, P_obj obj, int slot, int)
{
	ch->equipment[slot] = obj;
}
void obj_to_char(P_obj obj, P_char ch)
{
	obj->next_content = ch->carrying;
	ch->carrying = obj;
	obj->loc_p = LOC_CARRIED;
	obj->loc.carrying = ch;
}
void affect_to_char(P_char ch, affected_type *)
{
	++ch->affects;
}
bool obj_can_nest(P_obj obj, P_obj parent)
{
	return obj != parent;
}
char *str_dup(const char *str)
{
	return strdup(str);
}

// Fake MySQL results own strings until mysql_free_result(), as the real API does.
using Rows = std::vector<std::vector<std::string>>;
Rows saved_keepers, saved_items, saved_affects;
struct MYSQL_RES
{
	Rows rows;
	size_t index = 0;
	std::vector<char *> cells;
};
using MYSQL_ROW = char **;
bool DB = true;
MYSQL_RES *db_query(const char *query, ...)
{
	Rows rows;
	if (strstr(query, "SELECT shop_id"))
		rows = saved_keepers;
	else if (strstr(query, "FROM shopkeeper_items si"))
		rows = saved_items;
	else if (strstr(query, "FROM shopkeeper_affects sa"))
		rows = saved_affects;
	return new MYSQL_RES{ rows, 0, {} };
}
MYSQL_ROW mysql_fetch_row(MYSQL_RES *result)
{
	if (result->index == result->rows.size())
		return nullptr;
	result->cells.clear();
	for (auto &value : result->rows[result->index++])
		result->cells.push_back(value.data());
	return result->cells.data();
}
void mysql_free_result(MYSQL_RES *result)
{
	delete result;
}
bool sql_run_query(const char *)
{
	return true;
}

// PRODUCTION_RESTORE
// PRODUCTION_HELPER

struct
{
	char command = 'M';
	int arg1 = 0, arg2 = 1, arg3 = 0, arg4 = 100;
} command;
#define ZCMD command
int number(int low, int)
{
	return low;
}
void apply_zone_modifier(P_char) {}
void reset_mobile(int force_item_repop, bool expect_skip)
{
	int zone = 0, last_cmd = 1, last_mob_load = 1;
	P_char mob = character_list, last_mob = mob, tmp_mob = mob, last_mob_followable = mob;
	(void)zone;
	(void)last_cmd;
	(void)last_mob_load;
	(void)last_mob;
	(void)tmp_mob;
	(void)last_mob_followable;
	switch (ZCMD.command)
	{
		// PRODUCTION_RESET
	}
	if (expect_skip)
	{
		assert(!last_cmd && !last_mob_load);
		assert(!mob && !last_mob && !tmp_mob && !last_mob_followable);
	}
}
P_char spawn(int room, int rnum = 0)
{
	P_char ch = read_mobile(rnum, REAL);
	char_to_room(ch, room, 0);
	return ch;
}
void add_item(int keeper, int vnum, int slot = 0)
{
	std::vector<std::string> row(21, "0");
	row[0] = std::to_string(saved_items.size() + 1);
	row[1] = std::to_string(keeper);
	row[2] = std::to_string(vnum);
	row[3] = std::to_string(slot);
	for (int i = 16; i <= 19; ++i)
		row[i] = "";
	saved_items.push_back(row);
}
bool has_item(P_char ch, int rnum)
{
	for (P_obj obj = ch->carrying; obj; obj = obj->next_content)
		if (obj->R_num == rnum)
			return true;
	return false;
}
int main(int argc, char **argv)
{
	assert(argc == 2);
	const std::string scenario = argv[1];
	for (int i = 0; i < 3; ++i)
	{
		world[i].number = 200 + i;
		mob_index[i].virtual_number = 100 + i;
	}
	if (scenario == "reset")
	{
		P_char incumbent = spawn(0);
		obj_to_char(read_object(90, REAL), incumbent);
		for (int force : { 1, 2, 0 })
		{
			// Ordinary resets also must respect room occupancy below the global limit.
			command.arg2 = force ? 1 : 10;
			reset_mobile(force, true);
			assert(births == 1 && world[0].people == incumbent &&
			       has_item(incumbent, 90));
		}
		command.arg3 = 1;
		reset_mobile(1, false); // A keeper elsewhere does not occupy this room.
		assert(births == 2 && world[1].people);
		extract_char(world[1].people);
		reset_mobile(0, false); // A killed keeper can respawn.
		assert(births == 3);
		world[1].people->shopkeeper = false;
		reset_mobile(1, false); // Non-shop NPC population behavior is unchanged.
		assert(births == 4);
		command.arg3 = NOWHERE;
		reset_mobile(1, false); // Existing invalid-room handling still rejects placement.
	}
	else if (scenario == "shared")
	{
		spawn(0);
		spawn(0);
		spawn(1);
		P_char elsewhere = spawn(2);
		P_char player = spawn(0);
		player->npc = false;
		saved_keepers = { { "0", "10", "100", "200" }, { "1", "11", "100", "201" } };
		add_item(10, 70);
		add_item(11, 71);
		add_item(11, 72, 1);
		saved_affects = { { "11", "1", "2", "3", "4", "0", "0", "0", "0", "0" } };
		shop_index[0].number_items_produced = shop_index[1].number_items_produced = 1;
		shop_index[0].producing[0] = 80;
		shop_index[1].producing[0] = 81;
		sql_restore_shopkeepers();
		P_char first = world[0].people, second = world[1].people;
		assert(first->next_in_room == player && !player->next_in_room);
		assert(!second->next_in_room && world[2].people == elsewhere);
		assert(has_item(first, 70) && has_item(first, 80) && !has_item(first, 81));
		assert(has_item(second, 71) && has_item(second, 81) && !has_item(second, 80));
		assert(second->equipment[0]->R_num == 72 && second->affects == 1);
		assert(shop_index[0].dirty && shop_index[1].dirty);
		assert(mob_index[0].number == 4);
	}
	else if (scenario == "duplicate")
	{
		// DB orders newest snapshot first; only its stock should materialize.
		saved_keepers = { { "1", "11", "100", "200" }, { "0", "10", "100", "200" } };
		add_item(11, 71);
		add_item(10, 70);
		sql_restore_shopkeepers();
		assert(births == 1 && objects.size() == 1 && !world[0].people->next_in_room);
		assert(has_item(world[0].people, 71) && !has_item(world[0].people, 70));
		assert(shop_index[1].dirty && !shop_index[0].dirty);
	}
	else if (scenario == "invalid")
	{
		saved_keepers = { { "0", "10", "100", "200" }, { "-1", "11", "100", "201" },
				  { "4", "12", "100", "201" }, { "1", "13", "101", "201" },
				  { "2", "14", "100", "999" }, { "3", "15", "999", "201" } };
		for (int id = 10; id <= 15; ++id)
			add_item(id, id + 50);
		sql_restore_shopkeepers();
		assert(births == 1 && objects.size() == 1 && has_item(world[0].people, 60));
		assert(!world[1].people && !world[2].people);
	}
	else
		assert(false);
	while (character_list)
		extract_char(character_list);
	for (P_obj obj : objects)
	{
		free(obj->name);
		free(obj->short_description);
		free(obj->description);
		free(obj->action_description);
		delete obj;
	}
}
