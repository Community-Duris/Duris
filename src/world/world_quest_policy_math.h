#ifndef DURIS_WORLD_QUEST_POLICY_MATH_H
#define DURIS_WORLD_QUEST_POLICY_MATH_H

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <vector>

constexpr int WORLD_QUEST_MIN_LEVEL = 11;
constexpr int WORLD_QUEST_MAPLESS_MIN_LEVEL = 41;
constexpr double WORLD_QUEST_LEVEL_FIT_SCALE = 6.0;
constexpr int WORLD_QUEST_MAX_TARGET_PROBES = 32;
constexpr int WORLD_QUEST_MAX_HISTORY_CHECKS = 32;

inline int world_quest_truncated_average_level(int64_t level_sum, int64_t sample_count)
{
	if (level_sum < 0 || sample_count <= 0)
		return -1;
	const int64_t average = level_sum / sample_count;
	return average > std::numeric_limits<int>::max() ? std::numeric_limits<int>::max() :
							   static_cast<int>(average);
}

inline bool world_quest_zone_level_window_accepts(int player_level, int runtime_average_level)
{
	return runtime_average_level > player_level - 7 && runtime_average_level < player_level + 5;
}

inline bool world_quest_item_passes_floor(int quest_level, int itemvalue)
{
	return quest_level > 0 && itemvalue > 0 &&
	       (static_cast<long long>(itemvalue) * 2) >= quest_level;
}

// Bartenders never hand out the most valuable share of a zone's reward items. The top
// ceil(count * withheld_percent / 100) items by itemvalue are withheld, and any item worth
// as much as the cheapest of those is withheld with it, so the rule acts as a ceiling: an
// item is offered only when its value is below the returned figure. Returns INT_MAX when
// nothing is withheld.
inline int world_quest_reward_value_ceiling(std::vector<int> values, int withheld_percent)
{
	if (values.empty() || withheld_percent <= 0)
		return std::numeric_limits<int>::max();
	const long long percent = withheld_percent > 100 ? 100 : withheld_percent;
	const long long count = static_cast<long long>(values.size());
	const long long withheld = (count * percent + 99) / 100;
	std::sort(values.begin(), values.end(), std::greater<int>());
	return values[static_cast<size_t>(withheld - 1)];
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
