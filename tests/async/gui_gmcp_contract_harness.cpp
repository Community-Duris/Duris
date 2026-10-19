#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utils.h"
#include "core/json_utils.h"
#include "net/gmcp.h"
#include "ships/ships.h"
#include "world/db.h"
#include "magic/spells.h"
#include "guild/assocs.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <vector>
#include <unordered_map>

static room_data rooms[3];
P_room world = rooms;
static zone_data zones[1];
zone_data *zone_table = zones;
const int top_of_world = 3;
P_desc descriptor_list;
ContactData contacts[MAXSHIPS];
const ShipTypeData ship_type_data[MAXSHIPCLASS] = {};
const ShipCrewData ship_crew_data[MAXCREWS] = {};
const ShipChiefData ship_chief_data[MAXCHIEFS] = {};
const WeaponData weapon_data[MAXWEAPON] = {};
const EquipmentData equipment_data[MAXEQUIPMENT] = {};
static ShipData vessel, enemy, second_vessel;
static bool second_ship_enabled;
static std::map<P_desc, std::vector<std::string>> wire;
static std::map<P_desc, std::map<std::string, std::string>> ws_wire;
int top_of_zone_table = 0;
index_data *mob_index, *obj_index;
Skill skills[MAX_SKILLS];
char *get_class_string(P_char, char *buf)
{
	std::strcpy(buf, "Warrior");
	return buf;
}
int get_vis_mode(P_char, int)
{
	return 0;
}
bool ac_can_see(P_char, P_char, bool)
{
	return true;
}
bool ac_can_see_obj(P_char, P_obj, int)
{
	return true;
}
static int websockets;
const class_names class_names_table[64] = { { "Warrior", "Warrior", "War", 'W' } };
extern const race_names race_names_table[LAST_RACE + 1] = { { "Human", "Human", "Human", "Hum" } };
long new_exp_table[100] = {};
static int group_limit = 13;
int get_property(const char *, int)
{
	return group_limit;
}
int GET_PRIME_CLASS(P_char, unsigned int)
{
	return 0;
}
int GET_SECONDARY_CLASS(P_char, unsigned int)
{
	return 0;
}
void update_groupies(P_char ch, bool)
{
	if (ch->desc)
		ch->desc->last_group_update = 1;
}
int flag2idx(int flags)
{
	(void)flags;
	return 0;
}

static int contact_count = 1;
static int contact_scans;
int is_desc_valid(P_desc d)
{
	return d != nullptr;
}
int write_to_descriptor_binary(P_desc d, const unsigned char *bytes, size_t size)
{
	wire[d].emplace_back(reinterpret_cast<const char *>(bytes), size);
	return 0;
}
int websocket_send_json(P_desc d, const char *package, const char *json)
{
	++websockets;
	ws_wire[d][package] = json;
	return 0;
}
P_ship get_ship_from_char(P_char)
{
	assert(false && "periodic GMCP must not scan all ship rooms per observer");
	return nullptr;
}
ShipObjHash::ShipObjHash()
	: table{}
	, sz(0)
{
}
ShipObjHash shipObjHash;
static int ship_visits;
bool ShipObjHash::get_first(visitor &vs)
{
	vs.curr = &vessel;
	++ship_visits;
	return true;
}
bool ShipObjHash::get_next(visitor &vs)
{
	if (vs.curr == &vessel && second_ship_enabled)
	{
		vs.curr = &second_vessel;
		++ship_visits;
		return true;
	}
	return false;
}
static int queued_looks;
void write_to_q(const char *text, struct txt_q *, const int)
{
	assert(std::string(text) == "look");
	++queued_looks;
}
int real_room(const int room)
{
	return room;
}
int getcontacts(P_ship ship, bool)
{
	++contact_scans;
	return ship == &second_vessel ? 0 : contact_count;
}
string strip_ansi(const char *text)
{
	return text ? text : "";
}
int ShipData::get_maxspeed(P_char ch) const
{
	return ch ? ch->points.mana : 20;
}
int ShipCrew::get_contact_range_mod() const
{
	return 0;
}
int ShipCrew::sail_mod()
{
	return 0;
}
int ShipCrew::guns_mod()
{
	return 0;
}
int ShipCrew::rpar_mod()
{
	return 0;
}
int num_people_in_ship(P_ship)
{
	return 2;
}
const char *get_ship_status(P_ship ship)
{
	return SHIP_DOCKED(ship) ? "docked" : "sailing";
}
const char *cargo_type_name(int)
{
	return "silk";
}

// Isolate world/mechanics dependencies while exercising production serializers.

int sql_world_quest_can_do_another(P_char)
{
	return 2;
}
static char_data quest_mob;
static int quest_loads;
P_char read_mobile(int, int)
{
	++quest_loads;
	return &quest_mob;
}
int real_mobile(const int)
{
	return 0;
}
int real_zone0(const int)
{
	return 0;
}
void extract_char(P_char) {}
time_info_data time_info;
const char *month_name[] = { "Moon" };
void char_from_room(P_char)
{
	assert(false && "GMCP map refresh must not run movement hooks");
}
bool char_to_room(P_char, int, int)
{
	assert(false && "GMCP map refresh must not run movement hooks");
	return false;
}
void display_map_room(P_char ch, int room, int radius, int mode, int package)
{
	assert(ch->in_room == room && room == 2);
	assert(radius == 20 && mode == 999 && package == 1);
	gmcp_send_quest_map(ch, "quest map");
}

#include "production.inc"

static std::string payload(P_desc d, const char *package)
{
	if (d->websocket)
	{
		assert(ws_wire[d].contains(package) && "missing WebSocket GMCP package");
		return ws_wire[d][package];
	}
	const std::string prefix = std::string("\xff\xfa\xc9", 3) + package + " ";
	for (auto i = wire[d].rbegin(); i != wire[d].rend(); ++i)
	{
		if (i->starts_with(prefix))
		{
			assert(i->ends_with(std::string("\xff\xf0", 2)));
			return i->substr(prefix.size(), i->size() - prefix.size() - 2);
		}
	}
	assert(false && "missing GMCP package");
	return "";
}
static void emit(P_desc d, const char *package, const char *phase = "snapshot")
{
	printf("%s %s %s\n", phase, package, payload(d, package).c_str());
}
static void flush()
{
	gmcp_flush_dirty_ship_contacts();
	gmcp_flush_dirty_ship_info();
}
int main()
{
	static char_data a, b, victim;
	static pc_only_data apc, bpc;
	static descriptor_data ad, bd;
	a.only.pc = &apc;
	b.only.pc = &bpc;
	a.desc = &ad;
	b.desc = &bd;
	ad.character = &a;
	bd.character = &b;
	ad.connected = bd.connected = CON_PLAYING;
	ad.gmcp_enabled = bd.gmcp_enabled = 1;
	a.in_room = b.in_room = 1;
	a.points.mana = 18;
	b.points.mana = 23;
	descriptor_list = &ad;
	vessel.flags = LOADED;
	vessel.bridge = 1;
	vessel.location = 2;
	vessel.name = const_cast<char *>("Bronze Voyager");
	vessel.id = const_cast<char *>("AA");
	vessel.heading = 90;
	vessel.speed = 12;
	vessel.armor[0] = 80;
	vessel.maxarmor[0] = 100;
	vessel.internal[0] = 40;
	vessel.maxinternal[0] = 50;
	vessel.slot[0].type = SLOT_CARGO;
	vessel.slot[0].val0 = 3;
	rooms[2].zone = 0;
	zones[0].flags = ZONE_MAP;
	zones[0].mapx = 100;
	zones[0].mapy = 100;
	enemy.name = const_cast<char *>("Enemy vessel");
	enemy.id = const_cast<char *>("BB");
	enemy.race = EVILSHIP;
	enemy.heading = 50;
	enemy.speed = 8;
	enemy.target = &vessel;
	vessel.target = &enemy;
	contacts[0].ship = &enemy;
	contacts[0].range = 5;
	contacts[0].bearing = 70;
	std::strcpy(contacts[0].arc, "port");
	flush();
	assert(wire[&ad].size() == 2);
	emit(&ad, "Ship.Info");
	emit(&ad, "Ship.Contacts");
	auto json = payload(&ad, "Ship.Contacts");
	assert(json.find("\"worldX\"") == std::string::npos);
	assert(json.find("\"you_targeting\":true") != std::string::npos);
	assert(json.find("\"heading\":90") != std::string::npos);
	wire.clear();
	flush();
	assert(wire.empty());
	static descriptor_data land_descriptors[128];
	static char_data land_players[128];
	for (int i = 0; i < 128; ++i)
	{
		land_players[i].desc = &land_descriptors[i];
		land_players[i].in_room = 0;
		land_descriptors[i].character = &land_players[i];
		land_descriptors[i].connected = CON_PLAYING;
		land_descriptors[i].gmcp_enabled = 1;
		land_descriptors[i].next = i < 127 ? &land_descriptors[i + 1] : nullptr;
	}
	ad.next = land_descriptors;
	ship_visits = contact_scans = 0;
	flush();
	assert(ship_visits == 2 && contact_scans == 1 && wire.empty());
	ad.next = &bd;
	contact_scans = 0;
	flush();
	assert(contact_scans == 1); // one all-ships scan for both bridge observers
	assert(wire[&ad].empty() && wire[&bd].size() == 2); // stationary newcomer
	assert(payload(&bd, "Ship.Info").find("\"maxSpeed\":23") != std::string::npos);
	bd.durisweb_verified = 1;
	wire.clear();
	contact_scans = 0;
	flush();
	assert(contact_scans == 1);
	assert(payload(&bd, "Ship.Contacts").find("\"worldX\"") != std::string::npos);
	assert(wire[&ad].empty());
	bd.durisweb_verified = 0;
	flush();
	assert(payload(&bd, "Ship.Contacts").find("\"worldX\"") == std::string::npos);
	second_vessel.flags = LOADED;
	second_vessel.bridge = 0;
	second_vessel.location = 2;
	second_ship_enabled = true;
	b.in_room = 0;
	contact_scans = 0;
	gmcp_flush_dirty_ship_contacts();
	assert(contact_scans == 2); // independent contact set for each occupied ship
	assert(payload(&bd, "Ship.Contacts").find("\"contacts\":[]") != std::string::npos);
	second_ship_enabled = false;
	b.in_room = 1;
	flush();
	vessel.flags &= ~LOADED;
	flush();
	assert(payload(&ad, "Ship.Info") == "{}");
	assert(payload(&bd, "Ship.Contacts") == "{\"contacts\":[]}");
	vessel.flags |= LOADED;
	flush();
	assert(payload(&ad, "Ship.Info").find("Bronze Voyager") != std::string::npos);
	wire.clear();
	b.points.mana = 24;
	flush();
	assert(wire[&ad].empty() && wire[&bd].size() == 1); // personalized change
	wire.clear();
	vessel.flags |= DOCKED;
	flush();
	assert(payload(&ad, "Ship.Contacts").find("\"contacts\":[]") != std::string::npos);
	wire.clear();
	a.in_room = 0;
	flush();
	assert(payload(&ad, "Ship.Info") == "{}");
	assert(payload(&ad, "Ship.Contacts") == "{\"contacts\":[]}");
	wire.clear();
	a.in_room = 1;
	flush();
	assert(wire[&ad].size() == 2);
	wire.clear();
	a.specials.act3 |= PLR3_NOGMCP;
	vessel.speed = 4;
	flush();
	assert(wire[&ad].empty());
	a.specials.act3 &= ~PLR3_NOGMCP;
	wire.clear();
	ad.gmcp_enabled = 0;
	gmcp_handle_negotiation(&ad, TELNET_DO);
	assert(wire.empty());
	gmcp_flush_initial_states();
	for (const char *pkg : { "Char.Status", "Char.Vitals", "Char.Affects", "Group.Status",
				 "Room.Info", "Quest.Status", "Combat.Update" })
		payload(&ad, pkg);
	wire.clear();
	gmcp_handle_negotiation(&ad, TELNET_DO);
	gmcp_flush_initial_states();
	assert(wire.empty());
	gmcp_send_room_map(&a, "&+G@&n\n");
	emit(&ad, "Room.Map");
	assert(websockets == 0); // Telnet framing retained
	a.player.name = const_cast<char *>("Tester");
	victim.player.name = const_cast<char *>("wolf");
	victim.points.hit = 40;
	victim.points.max_hit = 100;
	applied_damage_notice(&a, &victim, 8);
	emit(&ad, "Combat.Update");
	static char_data primary;
	primary.player.name = const_cast<char *>("primary");
	primary.points.hit = 90;
	primary.points.max_hit = 100;
	a.specials.fighting = &primary;
	applied_damage_notice(&a, &victim, 8);
	assert(payload(&ad, "Combat.Update").find("\"name\":\"primary\"") != std::string::npos);
	assert(payload(&ad, "Combat.Update").find("\"round\"") == std::string::npos);
	a.specials.fighting = &victim;
	applied_damage_notice(&a, &victim, 8);
	assert(payload(&ad, "Combat.Update").find("\"name\":\"wolf\"") != std::string::npos);
	assert(payload(&ad, "Combat.Update").find("\"damage\":8") != std::string::npos);
	a.specials.fighting = nullptr;
	gmcp_combat_end(&a);
	assert(payload(&ad, "Combat.Update") == "{\"target\":null}");
	zones[0].flags = 0;
	zones[0].number = 228;
	zones[0].name = const_cast<char *>("Test Zone");
	rooms[1].number = 22800;
	rooms[1].name = const_cast<char *>("Gate of \"Dawn\"");
	gmcp_room_info(&a);
	emit(&ad, "Room.Info");
	cJSON *room_json = cJSON_Parse(payload(&ad, "Room.Info").c_str());
	assert(std::string(cJSON_GetObjectItem(room_json, "name")->valuestring) == rooms[1].name);
	cJSON_Delete(room_json);
	gmcp_char_status(&a);
	emit(&ad, "Char.Status");
	affected_type affect{};
	affect.type = 1;
	affect.duration = 3;
	a.affected = &affect;
	skills[1].name = "bless";
	gmcp_char_affects(&a);
	emit(&ad, "Char.Affects");
	quest_mob.player.short_descr = const_cast<char *>("bronze guardian");
	apc.quest_active = 1;
	apc.quest_type = FIND_AND_KILL;
	apc.quest_kill_original = 5;
	apc.quest_kill_how_many = 2;
	gmcp_quest_status(&a);
	emit(&ad, "Quest.Status");
	apc.quest_map_bought = 1;
	apc.quest_map_room = 2;
	apc.quest_zone_number = 228;
	ad.gmcp_quest_map_sent = 1;
	quest_loads = 0;
	wire.clear();
	for (int i = 0; i < 500; ++i)
	{
		gmcp_handle_negotiation(&ad, TELNET_DONT);
		gmcp_handle_negotiation(&ad, TELNET_DO);
	}
	assert(wire.empty() && quest_loads == 0);
	gmcp_flush_initial_states();
	assert(quest_loads == 1 && ad.gmcp_quest_map_sent == 1);
	assert(payload(&ad, "Quest.Map") == "{\"map\":\"quest map\",\"zoneNumber\":228}");
	assert(a.in_room == 1);
	wire.clear();
	gmcp_flush_initial_states();
	assert(wire.empty() && quest_loads == 1);
	gmcp_handle_negotiation(&ad, TELNET_DONT);
	gmcp_handle_negotiation(&ad, TELNET_DO);
	gmcp_handle_negotiation(&ad, TELNET_DONT);
	gmcp_flush_initial_states();
	assert(wire.empty()); // disabling before the flush cancels the request
	gmcp_handle_negotiation(&ad, TELNET_DO);
	gmcp_flush_initial_states();
	assert(quest_loads == 2); // later legitimate re-negotiation is not lost
	payload(&ad, "Quest.Map");
	wire.clear();
	gmcp_send_initial_state(&a); // toggle/reconnect also resends an already-sent map
	payload(&ad, "Quest.Map");
	static char_data morph;
	static npc_only_data morph_npc;
	morph.player.name = const_cast<char *>("morph body");
	morph.specials.act = ACT_ISNPC;
	morph.only.npc = &morph_npc;
	morph_npc.orig_char = &a;
	morph.desc = &ad;
	morph.in_room = 0;
	morph.points.hit = 37;
	morph.points.max_hit = 80;
	morph.affected = &affect;
	morph.specials.fighting = &victim;
	rooms[0].name = const_cast<char *>("Morph room");
	ad.original = &a;
	ad.character = &morph;
	a.desc = nullptr;
	wire.clear();
	gmcp_send_initial_state(&morph);
	assert(payload(&ad, "Char.Status").find("\"name\":\"Tester\"") != std::string::npos);
	assert(payload(&ad, "Char.Vitals").find("\"hp\":37") != std::string::npos);
	assert(payload(&ad, "Char.Affects").find("bless") != std::string::npos);
	assert(payload(&ad, "Room.Info").find("Morph room") != std::string::npos);
	assert(payload(&ad, "Combat.Update").find("wolf") != std::string::npos);
	assert(payload(&ad, "Quest.Status").find("\"active\":true") != std::string::npos);
	assert(payload(&ad, "Quest.Map").find("\"zoneNumber\":228") != std::string::npos);
	payload(&ad, "Group.Status");
	assert(morph.in_room == 0 && a.in_room == 1 && !a.desc && ad.character == &morph);
	a.specials.act3 |= PLR3_NOGMCP;
	wire.clear();
	gmcp_send_initial_state(&morph);
	gmcp_quest_status(&morph);
	assert(wire.empty()); // the original player's preference controls the morph
	a.specials.act3 &= ~PLR3_NOGMCP;
	gmcp_send_initial_state(&morph);
	payload(&ad, "Quest.Map");
	ad.original = nullptr;
	ad.character = &a;
	a.desc = &ad;
	morph.desc = nullptr;
	apc.quest_map_bought = 0;
	gmcp_comm_channel_ex(&a, "nchat", "Tester", "hello", "good");
	emit(&ad, "Comm.Channel");
	group_list member_b = { &b, nullptr };
	group_list member_a = { &a, &member_b };
	a.group = b.group = &member_a;
	b.player.name = const_cast<char *>("Ally");
	ad.websocket = 1;
	ad.ws_handshake_done = 0;
	ws_send_full_game_state(&ad);
	assert(ws_wire.empty() && queued_looks == 1);
	ad.ws_handshake_done = 1;
	a.specials.fighting = &victim;
	apc.quest_map_bought = ad.gmcp_quest_map_sent = 1;
	ws_send_full_game_state(&ad);
	assert(queued_looks == 2);
	for (const char *pkg :
	     { "Char.Status", "Char.Vitals", "Char.Affects", "Room.Info", "Quest.Status",
	       "Quest.Map", "Group.Status", "Combat.Update", "Ship.Info", "Ship.Contacts" })
		payload(&ad, pkg);
	assert(payload(&ad, "Group.Status").find("Ally") != std::string::npos);
	assert(payload(&ad, "Combat.Update").find("wolf") != std::string::npos);
	assert(payload(&ad, "Ship.Info") == "{}");
	flush();
	assert(payload(&ad, "Ship.Info").find("Bronze Voyager") != std::string::npos);
	a.specials.fighting = nullptr;
	a.group = nullptr;
	ws_wire.clear();
	ws_send_full_game_state(&ad);
	assert(payload(&ad, "Combat.Update") == "{\"target\":null}");
	assert(payload(&ad, "Group.Status").find("\"members\":[]") != std::string::npos);
	ad.websocket = 0;
	apc.quest_map_bought = 0;
	a.group = &member_a;
	gmcp_send_group_status(&a);
	emit(&ad, "Group.Status");
	assert(payload(&ad, "Group.Status").find("\"maxSize\":13") != std::string::npos);
	a.points.hit = 70;
	a.points.max_hit = 100;
	a.points.vitality = 80;
	a.points.max_vitality = 100;
	gmcp_char_vitals(&a);
	emit(&ad, "Char.Vitals");
	group_limit = 9;
	a.player.racewar = RACEWAR_GOOD;
	gmcp_send_group_status(&a);
	assert(payload(&ad, "Group.Status").find("\"maxSize\":9") != std::string::npos);
	a.group = nullptr;
	gmcp_send_group_status(&a);
	emit(&ad, "Group.Status", "clear");
	a.affected = nullptr;
	gmcp_char_affects(&a);
	emit(&ad, "Char.Affects", "clear");
	apc.quest_active = 0;
	gmcp_quest_status(&a);
	emit(&ad, "Quest.Status", "clear");
	gmcp_combat_end(&a);
	emit(&ad, "Combat.Update", "clear");
	flush();
	a.in_room = 0;
	flush();
	emit(&ad, "Ship.Info", "clear");
	emit(&ad, "Ship.Contacts", "clear");
	assert(payload(&ad, "Group.Status").find("\"members\":[]") != std::string::npos);
	ad.gmcp_enabled = 0;
	wire.clear();
	gmcp_send_room_map(&a, "hidden");
	assert(wire.empty());
}
