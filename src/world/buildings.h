//
// Outposts and buildings are not attached to the mud, but is here in case someone wants to clean up and finish the code,
// which was started by Torgal in 2008 and then continued by Venthix in 2009.
// - Torgal 1/29/2010
//

#ifndef _BUILDINGS_H_
#define _BUILDINGS_H_

#include <vector>
using namespace std;

#include "core/structs.h"
#include "world/map.h"
#include "world/vnum.room.h"

#define BUILDING_START_ROOM 97800
#define BUILDING_END_ROOM 97899

#define IS_BUILDING_ROOM(room) \
	(world[room].number >= BUILDING_START_ROOM && world[room].number <= BUILDING_END_ROOM)

// Building types for building_types array
#define BUILDING_OUTPOST 1

// Building object vnums
#define BUILDING_RUBBLE 97800
#define BUILDING_PORTAL 97801

// Building structure mob vnums
#define OUTPOST_BUILDING_MOB 97800
// Normal mobs relating to outposts
#define OUTPOST_GATEGUARD_WAR 97801
#define MAX_OUTPOST_GATEGUARDS 2

#define IS_OP_GOLEM(ch) (IS_NPC(ch) && (GET_VNUM(ch) == OUTPOST_GATEGUARD_WAR))

int outpost_mob(P_char ch, P_char pl, int cmd, char *arg);

class Building
{
    public:
	Building();
	Building(P_Guild _guild, int _type, int _room_vnum, int _level);
	~Building();

	int load();
	int unload();
	bool sub_money(int p, int g, int s, int c);
	int gate_room()
	{
		if (rooms.size() < 1)
			return RROOM_VOID;
		if (!rooms[0])
			return RROOM_VOID;
		return real_room0(rooms[0]->number);
	}
	int location() { return real_room0(room_vnum); }
	int get_id() { return id; }
	int size() { return rooms.size(); }
	P_room get_room(int room_num) { return &(*rooms[room_num]); }
	bool is_loaded() { return loaded; }
	bool proc(P_char ch, P_char pl, int cmd, char *arg);
	P_Guild get_guild() { return guild; }
	P_char get_mob() { return mob; }
	void set_proc() { mob_proc = outpost_mob; }
	void add_room(P_room room) { rooms.push_back(room); }
	int get_level() { return level; }
	void set_dir(int dir) { golem_dir = dir; }
	int get_golem_room() { return golem_room; }
	int get_golem_dir() { return golem_dir; }
	void set_golem_room(int room) { golem_room = room; }
	bool load_gateguard(int location, int type, int golemnum);
	void update_outpost_owner(P_Guild new_guild);
	void set_guild(P_Guild new_guild) { guild = new_guild; }
	void clear_portal_op();
	bool generate_portals();

    protected:
	int id;
	static int next_id;
	P_Guild guild;
	int type;
	int room_vnum;
	int level;
	int golem_room;
	int golem_dir;
	bool loaded;
	P_char mob;
	mob_proc_type mob_proc;

	P_obj portal_op;
	P_obj portal_gh;

	P_char golems[MAX_OUTPOST_GATEGUARDS];

	vector<P_room> rooms;
};

// building generation functions
typedef int (*building_generator_type)(Building *);

struct BuildingType
{
	int type;
	int mob_vnum;
	int req_wood;
	int req_stone;
	int hitpoints;
	building_generator_type
		generator; // function to set up building, instantiate rooms, set flags, etc
};

// utility functions

int initialize_buildings(); // called from comm.c
BuildingType get_type(int type);
Building *get_building_from_gateguard(P_char ch);
Building *get_building_from_rubble(P_obj rubble);
Building *get_building_from_char(P_char ch);
Building *get_building_from_room(int rroom);
Building *get_building_from_id(int id);
Building *load_building(P_Guild guild, int type, int location, int level);
void do_build(P_char ch, char *argument, int cmd);
int building_mob_proc(P_char ch, P_char pl, int cmd, char *arg);
int check_outpost_death(P_char, P_char);

//
// individual building procs
//

// OUTPOST
int outpost_generate(Building *building);
int outpost_inside(int room, P_char ch, int cmd, char *arg);

#endif
