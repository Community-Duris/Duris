#include "world/zone_story_quest_production.h"

#include "core/structs.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <map>
#include <set>
#include <sstream>
#include <utility>
#include <vector>

extern P_index mob_index;
extern int number_of_quests;
extern struct quest_data quest_index[];
extern struct zone_data *zone_table;
extern int top_of_zone_table;

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

std::string compact_player_text(const char *value)
{
	if (!value || !*value)
		return {};
	std::string output;
	bool pending_space = false;
	for (const unsigned char character : std::string_view(value))
	{
		if (std::isspace(character))
		{
			if (!output.empty())
				pending_space = true;
			continue;
		}
		if (pending_space && !output.empty())
			output.push_back(' ');
		pending_space = false;
		output.push_back(static_cast<char>(character));
	}
	if (output.size() >= 2 && output.front() == '"' && output.back() == '"')
		output = output.substr(1, output.size() - 2);
	if (output.size() > 240)
		output.resize(237), output += "...";
	return output;
}

bool is_system_quest_message(const char *keywords)
{
	return keywords && (!std::strncmp(keywords, "qc_", 3) || !std::strncmp(keywords, "QC_", 3));
}

std::string zone_name_for_number(int zone_number)
{
	if (!zone_table || top_of_zone_table < 0)
		return {};
	for (int index = 0; index <= top_of_zone_table; ++index)
	{
		if (zone_table[index].number == zone_number && zone_table[index].name)
			return compact_player_text(zone_table[index].name);
	}
	return {};
}

std::string giver_name_for_index(int quester_rnum)
{
	if (quester_rnum < 0 || !mob_index)
		return {};
	const char *name = mob_index[quester_rnum].desc2;
	if (!name || !*name)
		name = mob_index[quester_rnum].keys;
	return compact_player_text(name);
}

std::string objective_for_quest(const quest_data &quest, std::string_view giver_name)
{
	for (const quest_msg_data *message = quest.quest_message; message; message = message->next)
	{
		if (!message->message || !*message->message ||
		    is_system_quest_message(message->key_words))
			continue;
		const std::string text = compact_player_text(message->message);
		if (!text.empty())
			return text;
	}
	for (const quest_complete_data *completion = quest.quest_complete; completion;
	     completion = completion->next)
	{
		const std::string text = compact_player_text(completion->message);
		if (!text.empty())
			return text;
	}
	if (!giver_name.empty())
		return "Complete the request from " + std::string(giver_name) + ".";
	return "Complete the assigned quest.";
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
			definition.giver_name = giver_name_for_index(quester_rnum);
			definition.zone_name = zone_name_for_number(zone_number);
			definition.display_name = definition.giver_name.empty() ?
							  "Daily quest" :
							  "A request from " + definition.giver_name;
			definition.objective =
				objective_for_quest(quest_index[quest], definition.giver_name);
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
