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
extern P_index obj_index;
extern FILE *mob_f;
extern FILE *obj_f;
extern int top_of_objt;
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

bool positive_item_goal(const std::pair<char, int> &goal)
{
	return goal.first == 'I' && goal.second > 0;
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
	const std::string_view text(value);
	for (size_t index = 0; index < text.size(); ++index)
	{
		const unsigned char character = text[index];
		if (character == '&' && index + 1 < text.size())
		{
			const size_t code = index + (text[index + 1] == '+' ? 2 : 1);
			if (code < text.size() &&
			    std::isalnum(static_cast<unsigned char>(text[code])))
			{
				index = code;
				continue;
			}
		}
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

std::string prototype_label(P_index index, int number, FILE *file)
{
	if (!index || number < 0)
		return {};
	if (index[number].desc2)
		return compact_player_text(index[number].desc2);
	// Prototype text is lazy in db.c. Read just the keyword/short-description
	// fields without instantiating an NPC/item or minting an ownership UID.
	if (!file)
		return compact_player_text(index[number].keys);
	const long previous = ftell(file);
	if (previous < 0 || fseek(file, index[number].pos, SEEK_SET))
		return {};
	std::string label;
	for (int field = 0; field < 2; ++field)
	{
		std::string text;
		int character;
		while ((character = fgetc(file)) != EOF && character != '~' && text.size() < 65536)
			text.push_back(static_cast<char>(character));
		if (character != '~')
		{
			label.clear();
			break;
		}
		if (field == 1)
			label = std::move(text);
	}
	if (fseek(file, previous, SEEK_SET))
		return {};
	return compact_player_text(label.c_str());
}

std::string giver_name_for_index(int quester_rnum)
{
	return prototype_label(mob_index, quester_rnum, mob_f);
}

std::string objective_for_completion(const quest_complete_data &completion)
{
	std::ostringstream output;
	output << "Bring ";
	bool first = true;
	std::map<std::pair<char, int>, size_t> goals;
	for (const auto &goal : sorted_goals(completion.give))
		++goals[goal];
	for (const auto &[goal, count] : goals)
	{
		if (!first)
			output << ", ";
		first = false;
		if (goal.first == 'C')
			output << goal.second << " copper";
		else if (goal.first == 'I')
		{
			std::string name;
			for (int index = 0; obj_index && index <= top_of_objt; ++index)
				if (obj_index[index].virtual_number == goal.second)
				{
					name = prototype_label(obj_index, index, obj_f);
					break;
				}
			output << count << " x " << (name.empty() ? "requested item" : name);
		}
		else
			output << "the requested offering";
	}
	if (first)
		return "Speak with the quest giver to complete this request.";
	return output.str() + " to the quest giver.";
}
} // namespace

int zone_for_giver_vnum(int giver_vnum)
{
	if (giver_vnum <= 0 || !zone_table)
		return -1;
	int32_t previous_top = -1;
	for (int index = 0; index <= top_of_zone_table; ++index)
	{
		const auto &zone = zone_table[index];
		if (giver_vnum > previous_top && giver_vnum <= zone.top)
			return zone.number;
		previous_top = zone.top;
	}
	return -1;
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
	int32_t previous_top = -1;
	for (int index = 0; zone_table && index <= top_of_zone_table; ++index)
	{
		const auto &zone = zone_table[index];
		if (zone.number >= 0)
			result.zones.push_back(
				{ .zone_number = zone.number,
				  .name = compact_player_text(zone.name),
				  .source_area = zone.filename ? zone.filename : "runtime-zone",
				  .first_vnum = previous_top + 1,
				  .last_vnum = zone.top,
				  .discoverable = zone.number > 0 });
		previous_top = zone.top;
	}
	for (int quest = 0; quest < number_of_quests; ++quest)
	{
		const int quester_rnum = quest_index[quest].quester;
		if (quester_rnum < 0)
			continue;
		const int giver_vnum = mob_index[quester_rnum].virtual_number;
		const int zone_number = zone_for_giver_vnum(giver_vnum);
		if (zone_number < 0)
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
			for (const auto &zone : result.zones)
				if (zone.zone_number == zone_number)
					definition.source_area = zone.source_area;
			definition.giver_vnum = giver_vnum;
			definition.completion_key = encoded_key;
			definition.giver_name = giver_name_for_index(quester_rnum);
			definition.zone_name = zone_name_for_number(zone_number);
			definition.display_name = definition.giver_name.empty() ?
							  "Daily quest" :
							  "A request from " + definition.giver_name;
			definition.objective = objective_for_completion(*completion);
			definition.active = true;
			definition.eligible_for_zone_completion = zone_number > 0;
			bool resettable = false;
			for (int index = 0; index <= top_of_zone_table; ++index)
				if (zone_table[index].number == zone_number)
					resettable = zone_table[index].reset_mode != 0;
			definition.repeatable = !completion->disappear || resettable;
			const auto give = sorted_goals(completion->give);
			const auto receive = sorted_goals(completion->receive);
			const bool item_offering =
				std::any_of(give.begin(), give.end(), positive_item_goal);
			const bool returns_offering =
				std::any_of(give.begin(), give.end(),
					    [&](const auto &goal) {
						    return goal.first == 'I' &&
							   std::find(receive.begin(), receive.end(),
								     goal) != receive.end();
					    });
			const bool durable_offering =
				give.size() <= ZONE_STORY_QUEST_MAX_DURABLE_OFFERINGS &&
				std::all_of(give.begin(), give.end(), positive_item_goal);
			definition.daily_eligible = definition.repeatable && zone_number > 0 &&
						    item_offering && !returns_offering &&
						    durable_offering;
			if (!definition.daily_eligible)
				definition.daily_exclusion =
					zone_number <= 0       ? "Administrative content" :
					!definition.repeatable ? "Story-only quest" :
					returns_offering       ? "Item exchange" :
					!item_offering	       ? "No repeatable item offering" :
								 "Unsupported durable offering";
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
