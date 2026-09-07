#ifndef DURIS_WORLD_QUEST_POLICY_H
#define DURIS_WORLD_QUEST_POLICY_H

#include <vector>

#include "core/structs.h"

bool world_quest_policy_bootstrap();
bool world_quest_policy_zone_is_invalid(int zone_number);
void world_quest_policy_zone_list(P_char ch, std::vector<int> &valid_zones);
int world_quest_policy_select_zone(P_char ch, const std::vector<int> &valid_zones);
int world_quest_policy_suggest_mob(int zone_number, P_char ch, int quest_type,
				   int *target_probe_budget,
				   const std::vector<int> &excluded_targets = {});
int world_quest_policy_random_item(int zone_number);
int world_quest_policy_random_quest_item(int zone_number, int quest_level);

#endif // DURIS_WORLD_QUEST_POLICY_H
