#include "world/weather.h"
#include "magic/spells.h"
#include "world/vnum.obj.h"
#include "world/epic.h"
#include "item/objmisc.h"
static zone_data test_zones[1]{};
zone_data *zone_table = test_zones;
time_info_data time_info{};
static int world_visibility = 2;
int get_vis_mode(P_char, int)
{
	return world_visibility;
}
bool IS_TWILIGHT_ROOM(int)
{
	return false;
}
bool IS_OUTDOORS(int)
{
	return true;
}
int check_visible_wall(P_char, int)
{
	return 0;
}
P_obj get_wall_dir(P_char, int)
{
	return nullptr;
}
int GET_CLASS(P_char ch, uint cls)
{
	return (ch->player.m_class & cls) != 0;
}
bool has_innate(P_char, int)
{
	return false;
}
room_affect *get_spell_from_room(P_room, int)
{
	return nullptr;
}
bool affected_by_spell(P_char ch, int spell)
{
	for (auto *affect = ch->affected; affect; affect = affect->next)
		if (affect->type == spell)
			return true;
	return false;
}
struct affected_type *affect_to_char(P_char ch, struct affected_type *prototype)
{
	auto *affect = new affected_type(*prototype);
	affect->next = ch->affected;
	ch->affected = affect;
	ch->specials.affected_by2 |= affect->bitvector2;
	return affect;
}
const char *where[MAX_WEAR]{};
const char *set_master_text[1]{};
int get_numb_free_hands(P_char) { return 2; }
int wield_item_size(P_char, P_obj) { return 1; }
bool has_eq_slot(P_char, int) { return true; }
int epic_stone(P_obj, P_char, int, char *) { return 0; }
int in_weather_sector(int)
{
	return 7;
}
int ageCorpse(P_char, P_obj, char *)
{
	return 0;
}
int is_illusion_obj(P_obj)
{
	return 0;
}
obj_affect *get_obj_affect(P_obj, int)
{
	return nullptr;
}
void artifact_timer_sql(int, char *) {}
char *item_condition(P_obj)
{
	static char empty[] = "";
	return empty;
}
void page_string(P_desc d, char *text, int)
{
	if (d)
		send_to_char(text, d->character);
}
