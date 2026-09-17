#include "world/zone_story_quest_production.h"

#include "core/structs.h"

#include <algorithm>
#include <map>
#include <set>
#include <sstream>
#include <utility>
#include <vector>

extern P_index mob_index;
extern int number_of_quests;
extern struct quest_data quest_index[];

namespace zone_story_quest_production
{
namespace
{
zone_story_quest_catalog::catalog published_catalog;
std::map<const quest_complete_data *, std::string> completion_bindings;
bool catalog_ready = false;

char goal_kind(char goal_type)
{
	switch (goal_type)
	{
	case QUEST_GOAL_ITEM:
		return 'I';
	case QUEST_GOAL_ITEM_TYPE:
		return 'T';
	case QUEST_GOAL_COINS:
		return 'C';
	case QUEST_GOAL_SKILL:
		return 'S';
	case QUEST_GOAL_EXP:
		return 'E';
	default:
		return '?';
	}
}

std::string hex_encode(std::string_view value)
{
	const char *digits = "0123456789abcdef";
	std::string encoded;
	encoded.reserve(value.size() * 2);
	for (unsigned char byte : value)
	{
		encoded.push_back(digits[byte >> 4]);
		encoded.push_back(digits[byte & 0x0f]);
	}
	return encoded;
}

std::vector<std::pair<char, int>> sorted_goals(const goal_data *goals)
{
	std::vector<std::pair<char, int>> values;
	for (const goal_data *goal = goals; goal; goal = goal->next)
		values.emplace_back(goal_kind(goal->goal_type), goal->number);
	std::sort(values.begin(), values.end());
	return values;
}

std::string goals_string(const std::vector<std::pair<char, int>> &goals)
{
	std::ostringstream output;
	for (size_t index = 0; index < goals.size(); ++index)
	{
		if (index)
			output << ',';
		output << goals[index].first << ':' << goals[index].second;
	}
	return output.str();
}
} // namespace

int zone_for_giver_vnum(int giver_vnum)
{
	return giver_vnum > 0 ? std::max(1, giver_vnum / 100) : 0;
}

std::string canonical_completion_key(const quest_complete_data &completion)
{
	return "give=" + goals_string(sorted_goals(completion.give)) +
	       ";receive=" + goals_string(sorted_goals(completion.receive)) +
	       ";disappear=" + (completion.disappear ? "1" : "0");
}

zone_story_quest_catalog::catalog build_runtime_catalog(uint32_t content_revision,
							std::string *error)
{
	zone_story_quest_catalog::catalog result;
	result.content_revision = content_revision;
	if (content_revision == 0)
	{
		if (error)
			*error = "zone-story production content revision must be positive";
		return result;
	}
	if (!mob_index || number_of_quests < 0)
	{
		if (error)
			*error = "quest index is not booted";
		return result;
	}
	std::set<std::pair<int, std::string>> seen_contracts;
	for (int quest = 0; quest < number_of_quests; ++quest)
	{
		const int quester_rnum = quest_index[quest].quester;
		if (quester_rnum < 0)
			continue;
		const int giver_vnum = mob_index[quester_rnum].virtual_number;
		const int zone_number = zone_for_giver_vnum(giver_vnum);
		if (zone_number <= 0)
			continue;
		for (const quest_complete_data *completion = quest_index[quest].quest_complete;
		     completion; completion = completion->next)
		{
			const std::string key = canonical_completion_key(*completion);
			const auto occurrence_key = std::make_pair(giver_vnum, key);
			if (!seen_contracts.emplace(occurrence_key).second)
				continue;
			const std::string encoded_key = hex_encode(key);
			zone_story_quest_tracking::quest_definition definition;
			definition.definition_id =
				"zone-story:qst:" + std::to_string(giver_vnum) + ":" + encoded_key;
			definition.source_system =
				zone_story_quest_tracking::ZONE_STORY_QUEST_SOURCE_SYSTEM;
			definition.zone_number = zone_number;
			definition.source_area = "runtime-qst";
			definition.giver_vnum = giver_vnum;
			definition.completion_key = encoded_key;
			definition.active = true;
			definition.eligible_for_zone_completion = true;
			definition.repeatable = true;
			definition.content_revision = content_revision;
			result.definitions.push_back(std::move(definition));
		}
	}
	std::sort(result.definitions.begin(), result.definitions.end(),
		  [](const auto &left, const auto &right)
		  { return left.definition_id < right.definition_id; });
	std::vector<zone_story_quest_catalog::diagnostic> diagnostics;
	if (!zone_story_quest_catalog::validate(result, &diagnostics) && error)
		*error = diagnostics.empty() ?
				 "runtime zone-story catalog is invalid" :
				 diagnostics.front().code + ": " + diagnostics.front().message;
	return result;
}

bool bootstrap(uint32_t content_revision, std::string *error)
{
	std::string build_error;
	zone_story_quest_catalog::catalog candidate =
		build_runtime_catalog(content_revision, &build_error);
	std::vector<zone_story_quest_catalog::diagnostic> diagnostics;
	if (!zone_story_quest_catalog::validate(candidate, &diagnostics))
	{
		catalog_ready = false;
		if (error)
			*error = diagnostics.empty() ? build_error :
						       diagnostics.front().code + ": " +
							       diagnostics.front().message;
		return false;
	}
	completion_bindings.clear();
	for (int quest = 0; quest < number_of_quests; ++quest)
	{
		const int quester_rnum = quest_index[quest].quester;
		if (quester_rnum < 0)
			continue;
		const int giver_vnum = mob_index[quester_rnum].virtual_number;
		for (const quest_complete_data *completion = quest_index[quest].quest_complete;
		     completion; completion = completion->next)
		{
			const std::string key = canonical_completion_key(*completion);
			const std::string definition_id =
				"zone-story:qst:" + std::to_string(giver_vnum) + ":" +
				hex_encode(key);
			completion_bindings.emplace(completion, definition_id);
		}
	}
	published_catalog = std::move(candidate);
	catalog_ready = true;
	return true;
}

const zone_story_quest_catalog::catalog &runtime_catalog()
{
	return published_catalog;
}

const std::string *definition_id_for(const quest_complete_data *completion)
{
	if (!completion)
		return nullptr;
	const auto found = completion_bindings.find(completion);
	return found == completion_bindings.end() ? nullptr : &found->second;
}

bool ready()
{
	return catalog_ready;
}
} // namespace zone_story_quest_production
