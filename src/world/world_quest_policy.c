#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utils.h"
#include "economy/tradeskill.h"
#include "item/objmisc.h"
#include "magic/spells.h"
#include "world/db.h"
#include "world/map.h"
#include "world/world_quest_policy.h"
#include "world/world_quest_policy_math.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <exception>
#include <cstdio>
#include <string>
#include <vector>

extern P_index mob_index;
extern P_index obj_index;
extern P_room world;
extern struct zone_data *zone_table;
extern int top_of_mobt;
extern int top_of_objt;
extern int top_of_zone_table;
extern int top_of_world;

extern int get_map_room(int zone_id);

namespace
{
constexpr int WORLD_QUEST_MAX_ZONE_NUMBER = 4000;

constexpr int WORLD_QUEST_EXPLICIT_DENY_ZONES[] = {
	0,
	292,
	536,
};

struct quest_mob_profile
{
	int rnum = -1;
	int vnum = -1;
	int level = -1;
	bool can_speak = false;
	bool invisible = false;
	bool hidden = false;
};

struct quest_reward_profile
{
	int vnum = -1;
	int ivalue = 0;
};

struct zone_interval
{
	int bottom = 0;
	int top = 0;
	int zone = -1;
};

struct quest_zone_profile
{
	int number = -1;
	int runtime_average_level = -1;
	int64_t mob_level_sum = 0;
	int64_t mob_level_count = 0;
	double average_level = -1.0;
	int map_room = -1;
	std::vector<quest_mob_profile> mobs;
	std::vector<quest_reward_profile> rewards;
	std::vector<int> reward_vnums;
	std::array<std::vector<int>, TOTALLVLS> reward_vnums_by_level;
	std::array<int, TOTALLVLS> reward_count{};
	std::array<double, TOTALLVLS> reward_average{};
	std::array<double, TOTALLVLS> zone_score{};
};

std::vector<quest_zone_profile> quest_zones;
std::vector<zone_interval> quest_intervals;
bool quest_catalog_ready = false;
bool quest_catalog_failed = false;
bool quest_catalog_building = false;

struct mobile_probe_guard
{
	P_char value;
	~mobile_probe_guard()
	{
		if (value)
			extract_char(value);
	}
};

struct object_probe_guard
{
	P_obj value;
	~object_probe_guard()
	{
		if (value)
			extract_obj(value, FALSE);
	}
};

bool explicit_deny(int zone_number)
{
	for (const int denied : WORLD_QUEST_EXPLICIT_DENY_ZONES)
	{
		if (zone_number == denied)
			return true;
	}
	return false;
}

bool normal_zone(int zone_index)
{
	if (!zone_table || zone_index < 0 || zone_index > top_of_zone_table)
		return false;
	const int zone_number = zone_table[zone_index].number;
	return zone_number >= 0 && zone_number < WORLD_QUEST_MAX_ZONE_NUMBER;
}

bool valid_room_interval(int zone_index)
{
	if (!normal_zone(zone_index))
		return false;
	const zone_data &zone = zone_table[zone_index];
	return zone.real_bottom != NOWHERE && zone.real_top != NOWHERE && zone.real_bottom >= 0 &&
	       zone.real_top >= zone.real_bottom && zone.real_top <= top_of_world;
}

int zone_for_vnum(int vnum)
{
	if (quest_intervals.empty())
		return -1;
	const auto found = std::upper_bound(quest_intervals.begin(), quest_intervals.end(), vnum,
					    [](int value, const zone_interval &interval)
					    { return value < interval.bottom; });
	if (found == quest_intervals.begin())
		return -1;
	const auto candidate = found - 1;
	return vnum <= candidate->top ? candidate->zone : -1;
}

bool source_eligible_reward(P_obj obj)
{
	if (!obj)
		return false;
	if (IS_SET(obj->extra_flags, ITEM_NORENT) || IS_SET(obj->str_mask, STRUNG_KEYS) ||
	    !IS_SET(obj->wear_flags, ITEM_TAKE) || IS_SET(obj->extra_flags, ITEM_TRANSIENT))
		return false;
	if (obj->type != ITEM_WAND && obj->type != ITEM_STAFF && obj->type != ITEM_ARMOR &&
	    obj->type != ITEM_WORN && obj->type != ITEM_BOOK && obj->type != ITEM_QUIVER &&
	    obj->type != ITEM_INSTRUMENT && obj->type != ITEM_SPELLBOOK &&
	    obj->type != ITEM_TOTEM && obj->type != ITEM_SHIELD && obj->type != ITEM_FIREWEAPON &&
	    obj->type != ITEM_WEAPON && obj->type != ITEM_POTION)
		return false;
	if (IS_SET(obj->bitvector, AFF_STONE_SKIN) || IS_SET(obj->bitvector, AFF_HIDE) ||
	    IS_SET(obj->bitvector, AFF_SNEAK) || IS_SET(obj->bitvector, AFF_FLY) ||
	    IS_SET(obj->bitvector, AFF4_NOFEAR) || IS_SET(obj->bitvector2, AFF2_AIR_AURA) ||
	    IS_SET(obj->bitvector2, AFF2_EARTH_AURA) ||
	    IS_SET(obj->bitvector3, AFF3_INERTIAL_BARRIER) ||
	    IS_SET(obj->bitvector3, AFF3_REDUCE) || IS_SET(obj->bitvector2, AFF2_GLOBE) ||
	    IS_SET(obj->bitvector, AFF_HASTE) || IS_SET(obj->bitvector, AFF_DETECT_INVISIBLE) ||
	    IS_SET(obj->bitvector4, AFF4_DETECT_ILLUSION))
		return false;
	if (IS_NOSHOW(obj) || GET_OBJ_WEIGHT(obj) > 99 || IS_ARTIFACT(obj) ||
	    isname("_noquest_", obj->name) || IS_OBJ_STAT2(obj, ITEM2_QUESTITEM))
		return false;
	return true;
}

void build_intervals()
{
	quest_intervals.clear();
	quest_intervals.reserve(static_cast<size_t>(top_of_zone_table + 1));
	for (int zone = 0; zone <= top_of_zone_table; ++zone)
	{
		if (!valid_room_interval(zone))
			continue;
		quest_intervals.push_back({
			world[zone_table[zone].real_bottom].number,
			world[zone_table[zone].real_top].number,
			zone,
		});
	}
	std::sort(quest_intervals.begin(), quest_intervals.end(),
		  [](const zone_interval &left, const zone_interval &right)
		  {
			  if (left.bottom != right.bottom)
				  return left.bottom < right.bottom;
			  return left.top < right.top;
		  });
}

void load_mobile_profiles()
{
	for (int rnum = 0; rnum <= top_of_mobt; ++rnum)
	{
		const int zone = zone_for_vnum(mob_index[rnum].virtual_number);
		if (zone < 0)
			continue;

		mobile_probe_guard probe{ read_mobile_probe(mob_index[rnum].virtual_number,
							    VIRTUAL) };
		if (!probe.value)
			continue;

		quest_mob_profile profile;
		profile.rnum = rnum;
		profile.vnum = mob_index[rnum].virtual_number;
		profile.level = GET_LEVEL(probe.value);
		profile.can_speak = CAN_SPEAK(probe.value);
		profile.invisible = IS_AFFECTED(probe.value, AFF_INVISIBLE) ||
				    IS_AFFECTED2(probe.value, AFF2_CONCEALMENT) ||
				    IS_AFFECTED3(probe.value, AFF3_ECTOPLASMIC_FORM);
		profile.hidden = IS_AFFECTED(probe.value, AFF_HIDE);
		quest_zones[zone].mobs.push_back(profile);
		++quest_zones[zone].mob_level_count;
		quest_zones[zone].mob_level_sum += profile.level;
		quest_zones[zone].average_level =
			static_cast<double>(quest_zones[zone].mob_level_sum) /
			static_cast<double>(quest_zones[zone].mob_level_count);
	}
}

void load_reward_profiles()
{
	for (int rnum = 0; rnum <= top_of_objt; ++rnum)
	{
		const int zone = zone_for_vnum(obj_index[rnum].virtual_number);
		if (zone < 0)
			continue;

		object_probe_guard probe{ read_object(obj_index[rnum].virtual_number, VIRTUAL) };
		if (!probe.value)
			continue;
		if (source_eligible_reward(probe.value))
		{
			quest_reward_profile profile;
			profile.vnum = obj_index[rnum].virtual_number;
			profile.ivalue = itemvalue(probe.value);
			quest_zones[zone].rewards.push_back(profile);
			quest_zones[zone].reward_vnums.push_back(profile.vnum);
		}
	}

	for (quest_zone_profile &zone : quest_zones)
	{
		for (const quest_reward_profile &reward : zone.rewards)
		{
			for (int level = 1; level < TOTALLVLS; ++level)
			{
				if (!world_quest_item_passes_floor(level, reward.ivalue))
					continue;
				zone.reward_vnums_by_level[level].push_back(reward.vnum);
				zone.reward_count[level]++;
				zone.reward_average[level] += reward.ivalue;
			}
		}
		for (int level = 1; level < TOTALLVLS; ++level)
		{
			if (zone.reward_count[level] > 0)
				zone.reward_average[level] /= zone.reward_count[level];
		}
	}
}

void load_zone_scores()
{
	for (quest_zone_profile &zone : quest_zones)
		for (int level = 1; level < TOTALLVLS; ++level)
			zone.zone_score[level] = world_quest_zone_raw_score(
				level, zone.average_level, zone.reward_average[level],
				zone.reward_count[level]);
}

bool static_zone_eligibility(int zone, P_char ch)
{
	if (!ch || zone < 0 || zone > top_of_zone_table || !normal_zone(zone))
		return false;
	const quest_zone_profile &profile = quest_zones[zone];
	if (explicit_deny(profile.number) || IS_SET(zone_table[zone].flags, ZONE_TOWN) ||
	    zone_table[zone].hometown != 0 || profile.runtime_average_level < 0)
		return false;
	const int level = GET_LEVEL(ch);
	if (level < WORLD_QUEST_MIN_LEVEL || level >= TOTALLVLS)
		return false;
	if (!world_quest_zone_level_window_accepts(level, profile.runtime_average_level))
		return false;
	if (profile.map_room < 0 && level < WORLD_QUEST_MAPLESS_MIN_LEVEL)
		return false;
	return profile.reward_count[level] > 0;
}

int select_cached_mob(const quest_zone_profile &zone, P_char ch, int quest_type,
		      int *target_probe_budget, const std::vector<int> &excluded_targets)
{
	const int player_level = GET_LEVEL(ch);
	const int kind_of_quest = quest_type == 0 ? number(1, 2) : quest_type;
	std::vector<const quest_mob_profile *> candidates;
	candidates.reserve(zone.mobs.size());

	for (const quest_mob_profile &mob : zone.mobs)
	{
		if (std::find(excluded_targets.begin(), excluded_targets.end(), mob.vnum) !=
		    excluded_targets.end())
			continue;
		if (mob.rnum < 0 || mob.rnum > top_of_mobt)
			continue;
		if (kind_of_quest == FIND_AND_KILL)
		{
			if (mob.level < player_level - 4 || mob.level > player_level + 5 ||
			    mob_index[mob.rnum].number < 2)
				continue;
		}
		else
		{
			if (mob.level < player_level - 4 || mob.level > player_level + 5 ||
			    mob.level >= MAXLVL || mob_index[mob.rnum].number != 1 ||
			    !mob.can_speak || (player_level < 31 && (mob.invisible || mob.hidden)))
				continue;
		}
		candidates.push_back(&mob);
	}

	if (candidates.empty())
		return -1;
	if (kind_of_quest == FIND_AND_KILL)
	{
		return candidates[number(0, static_cast<int>(candidates.size()) - 1)]->vnum;
	}

	if (!target_probe_budget || *target_probe_budget <= 0)
		return -1;
	int probes = 0;
	while (!candidates.empty() && probes < WORLD_QUEST_MAX_TARGET_PROBES &&
	       *target_probe_budget > 0)
	{
		--*target_probe_budget;
		const int index = number(0, static_cast<int>(candidates.size()) - 1);
		const quest_mob_profile *selected = candidates[index];
		candidates.erase(candidates.begin() + index);
		++probes;

		mobile_probe_guard probe{ read_mobile_probe(selected->vnum, VIRTUAL) };
		if (!probe.value)
			continue;
		REMOVE_BIT(probe.value->specials.act, ACT_SPEC);
		if (probe.value->in_room == NOWHERE)
			char_to_room(probe.value, 1, -2);
		const bool aggressive = aggressive_to(probe.value, ch);
		if (!aggressive)
			return selected->vnum;
	}
	return -1;
}
} // namespace

bool world_quest_policy_bootstrap()
{
	if (quest_catalog_ready || quest_catalog_failed)
		return quest_catalog_ready;
	if (quest_catalog_building || !zone_table || !world || !mob_index || !obj_index ||
	    top_of_zone_table < 0 || top_of_mobt < 0 || top_of_objt < 0)
		return false;

	quest_catalog_building = true;
	const auto started = std::chrono::steady_clock::now();
	try
	{
		quest_zones.clear();
		quest_zones.resize(static_cast<size_t>(top_of_zone_table + 1));
		for (int zone = 0; zone <= top_of_zone_table; ++zone)
			quest_zones[zone].number = zone_table[zone].number;
		build_intervals();
		load_mobile_profiles();
		for (quest_zone_profile &zone : quest_zones)
		{
			if (zone.mob_level_count > 0)
			{
				zone.average_level = static_cast<double>(zone.mob_level_sum) /
						     static_cast<double>(zone.mob_level_count);
				zone.runtime_average_level = world_quest_truncated_average_level(
					zone.mob_level_sum, zone.mob_level_count);
			}
			if (zone.runtime_average_level >= 0)
				zone_table[&zone - quest_zones.data()].avg_mob_level =
					zone.runtime_average_level;
			else if (normal_zone(static_cast<int>(&zone - quest_zones.data())))
				zone_table[&zone - quest_zones.data()].avg_mob_level = -1;
		}
		for (int zone = 0; zone <= top_of_zone_table; ++zone)
		{
			if (!valid_room_interval(zone))
				continue;
			quest_zones[zone].map_room = get_map_room(zone);
		}
		load_reward_profiles();
		load_zone_scores();
		quest_catalog_ready = true;
		const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::steady_clock::now() - started);
		size_t mob_profiles = 0;
		size_t reward_profiles = 0;
		int mapless_zones = 0;
		for (const quest_zone_profile &zone : quest_zones)
		{
			mob_profiles += zone.mobs.size();
			reward_profiles += zone.rewards.size();
			if (zone.map_room < 0)
				++mapless_zones;
		}
		logit(LOG_STATUS,
		      "World quest catalog ready: zones=%d mobile_profiles=%zu reward_profiles=%zu "
		      "mapless_zones=%d elapsed_ms=%lld",
		      top_of_zone_table + 1, mob_profiles, reward_profiles, mapless_zones,
		      static_cast<long long>(elapsed.count()));
		fprintf(stderr,
			"World quest catalog ready: zones=%d mobile_profiles=%zu reward_profiles=%zu "
			"mapless_zones=%d elapsed_ms=%lld\n",
			top_of_zone_table + 1, mob_profiles, reward_profiles, mapless_zones,
			static_cast<long long>(elapsed.count()));
	}
	catch (const std::exception &error)
	{
		quest_zones.clear();
		quest_intervals.clear();
		quest_catalog_failed = true;
		logit(LOG_EXIT, "World quest catalog failed closed: %s", error.what());
		fprintf(stderr, "World quest catalog failed closed: %s\n", error.what());
	}
	quest_catalog_building = false;
	return quest_catalog_ready;
}

bool world_quest_policy_zone_is_invalid(int zone_number)
{
	if (!world_quest_policy_bootstrap())
		return true;
	for (int zone = 0; zone <= top_of_zone_table; ++zone)
	{
		if (quest_zones[zone].number != zone_number)
			continue;
		return !normal_zone(zone) || explicit_deny(zone_number) ||
		       IS_SET(zone_table[zone].flags, ZONE_TOWN) ||
		       zone_table[zone].hometown != 0 ||
		       quest_zones[zone].runtime_average_level < 0;
	}
	return true;
}

void world_quest_policy_zone_list(P_char ch, std::vector<int> &valid_zones)
{
	valid_zones.clear();
	if (!world_quest_policy_bootstrap() || !ch)
		return;
	for (int zone = 0; zone <= top_of_zone_table; ++zone)
	{
		if (static_zone_eligibility(zone, ch))
			valid_zones.push_back(zone);
	}
}

int world_quest_policy_select_zone(P_char ch, const std::vector<int> &valid_zones)
{
	if (!ch || valid_zones.empty() || !world_quest_policy_bootstrap())
		return -1;
	const int level = GET_LEVEL(ch);
	if (level < 1 || level >= TOTALLVLS)
		return -1;
	double total = 0.0;
	for (const int zone : valid_zones)
	{
		if (zone < 0 || zone > top_of_zone_table)
			continue;
		const quest_zone_profile &profile = quest_zones[zone];
		total += profile.zone_score[level];
	}
	if (total <= 0.0)
		return -1;

	double roll = static_cast<double>(number(1, 1000000)) / 1000000.0 * total;
	for (const int zone : valid_zones)
	{
		if (zone < 0 || zone > top_of_zone_table)
			continue;
		const quest_zone_profile &profile = quest_zones[zone];
		const double weight = profile.zone_score[level];
		if (weight <= 0.0)
			continue;
		if (roll <= weight)
			return zone;
		roll -= weight;
	}
	return valid_zones.back();
}

int world_quest_policy_suggest_mob(int zone_number, P_char ch, int quest_type,
				   int *target_probe_budget,
				   const std::vector<int> &excluded_targets)
{
	if (!ch || !world_quest_policy_bootstrap() || zone_number < 0 ||
	    zone_number > top_of_zone_table)
		return -1;
	return select_cached_mob(quest_zones[zone_number], ch, quest_type, target_probe_budget,
				 excluded_targets);
}

int world_quest_policy_random_item(int zone_number)
{
	if (!world_quest_policy_bootstrap() || zone_number < 0 || zone_number > top_of_zone_table)
		return -1;
	const std::vector<int> &items = quest_zones[zone_number].reward_vnums;
	if (items.empty())
		return -1;
	return items[number(0, static_cast<int>(items.size()) - 1)];
}

int world_quest_policy_random_quest_item(int zone_number, int quest_level)
{
	if (!world_quest_policy_bootstrap() || zone_number < 0 || zone_number > top_of_zone_table ||
	    quest_level <= 0 || quest_level >= TOTALLVLS)
		return -1;
	const std::vector<int> &items = quest_zones[zone_number].reward_vnums_by_level[quest_level];
	if (items.empty())
		return -1;
	return items[number(0, static_cast<int>(items.size()) - 1)];
}
