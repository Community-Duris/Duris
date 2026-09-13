#include "world/weather.h"
#include "magic/spells.h"
#include "world/vnum.obj.h"
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
bool affected_by_spell(P_char, int)
{
	return 0;
}
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
