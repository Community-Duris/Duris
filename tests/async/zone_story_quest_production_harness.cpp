#include "core/structs.h"
#include "world/zone_story_quest_production.h"

#include <cstdlib>
#include <iostream>
#include <string>

P_index mob_index;
int number_of_quests = 0;
struct quest_data quest_index[1];
struct zone_data *zone_table = nullptr;
int top_of_zone_table = -1;

namespace
{
void require(bool condition, const char *message)
{
	if (!condition)
	{
		std::cerr << message << '\n';
		std::exit(1);
	}
}
} // namespace

int main()
{
	index_data mobs[1] = {};
	mob_index = mobs;
	mobs[0].virtual_number = 17;
	char mob_name[] = "the archivist";
	mobs[0].desc2 = mob_name;
	zone_data zones[1] = {};
	char area_name[] = "The First Heavens";
	zones[0].number = 1;
	zones[0].name = area_name;
	zone_table = zones;
	top_of_zone_table = 0;

	goal_data give{ .goal_type = QUEST_GOAL_ITEM, .number = 24402, .next = nullptr };
	goal_data second_give{ .goal_type = QUEST_GOAL_ITEM, .number = 24404, .next = nullptr };
	goal_data receive{ .goal_type = QUEST_GOAL_ITEM, .number = 24403, .next = nullptr };
	quest_complete_data first{ .message = nullptr,
				   .receive = &receive,
				   .give = &give,
				   .disappear = false,
				   .disappear_message = nullptr,
				   .echoAll = false,
				   .next = nullptr };
	quest_complete_data second = first;
	second.give = &second_give;
	quest_complete_data duplicate = first;
	first.next = &second;
	second.next = &duplicate;
	quest_index[0] = { .quester = 0, .quest_message = nullptr, .quest_complete = &first };
	number_of_quests = 1;

	std::string error;
	const auto catalog = zone_story_quest_production::build_runtime_catalog(1, &error);
	require(error.empty() && catalog.definitions.size() == 2,
		"runtime production catalog did not deduplicate identical Q blocks");
	require(catalog.definitions[0].zone_number == 1 && catalog.definitions[0].giver_vnum == 17,
		"low-vnum quester was not assigned to its valid zone");
	require(catalog.definitions[0].giver_name == "the archivist" &&
			catalog.definitions[0].zone_name == "The First Heavens" &&
			!catalog.definitions[0].display_name.empty() &&
			!catalog.definitions[0].objective.empty(),
		"runtime catalog did not populate player-facing quest metadata");
	require(zone_story_quest_production::bootstrap(1, &error),
		"runtime production catalog failed to bootstrap");
	require(zone_story_quest_production::ready(),
		"runtime production catalog was not marked ready");
	const std::string *first_id = zone_story_quest_production::definition_id_for(&first);
	const std::string *second_id = zone_story_quest_production::definition_id_for(&second);
	const std::string *duplicate_id =
		zone_story_quest_production::definition_id_for(&duplicate);
	require(first_id && second_id && duplicate_id && *first_id != *second_id &&
			*first_id == *duplicate_id,
		"Q completion blocks did not preserve deduplicated stable identities");
	require(zone_story_quest_production::canonical_completion_key(first).find("give=I:24402") !=
			std::string::npos,
		"canonical completion key omitted give goals");

	std::cout << "zone-story runtime production catalog regression passed\n";
	return 0;
}
