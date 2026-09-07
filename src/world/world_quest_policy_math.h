#ifndef DURIS_WORLD_QUEST_POLICY_MATH_H
#define DURIS_WORLD_QUEST_POLICY_MATH_H

#include <cmath>

constexpr int WORLD_QUEST_MIN_LEVEL = 11;
constexpr int WORLD_QUEST_MAPLESS_MIN_LEVEL = 41;
constexpr double WORLD_QUEST_LEVEL_FIT_SCALE = 6.0;
constexpr int WORLD_QUEST_MAX_TARGET_PROBES = 32;

inline bool world_quest_item_passes_floor(int quest_level, int itemvalue)
{
	return quest_level > 0 && itemvalue > 0 &&
	       (static_cast<long long>(itemvalue) * 2) >= quest_level;
}

inline double world_quest_zone_level_fit(int quest_level, double average_level)
{
	if (quest_level <= 0 || average_level < 0.0)
		return 0.0;
	return std::exp(-std::abs(average_level - static_cast<double>(quest_level)) /
			WORLD_QUEST_LEVEL_FIT_SCALE);
}

inline double world_quest_zone_raw_score(int quest_level, double average_level,
					 double average_ivalue, int eligible_item_count)
{
	if (quest_level <= 0 || average_ivalue <= 0.0 || eligible_item_count <= 0)
		return 0.0;
	return world_quest_zone_level_fit(quest_level, average_level) *
	       (average_ivalue / static_cast<double>(quest_level)) *
	       static_cast<double>(eligible_item_count);
}

#endif // DURIS_WORLD_QUEST_POLICY_MATH_H
