#include "core/structs.h"
#include "world/zone_story_quest_production.h"

#include <cstdlib>
#include <algorithm>
#include <iostream>
#include <string>
#include <filesystem>
#include <fstream>

P_index mob_index;
P_index obj_index = nullptr;
FILE *mob_f = nullptr;
FILE *obj_f = nullptr;
int top_of_objt = -1;
int top_of_mobt = 0;
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

int main(int argc, char **argv)
{
	index_data mobs[1] = {};
	mob_index = mobs;
	mobs[0].virtual_number = 17;
	char mob_name[] = "the archivist";
	char mob_keywords[] = "archivist";
	mobs[0].keys = mob_keywords;
	mobs[0].desc2 = mob_name;
	zone_data zones[1] = {};
	char area_name[] = "&+WThe First Heavens&n";
	zones[0].number = 1;
	zones[0].top = 1281;
	zones[0].reset_mode = 2;
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
	require(catalog.definitions[0].daily_eligible && catalog.definitions[1].daily_eligible,
		"supported exact item contracts did not remain daily candidates");
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

	mobs[0].desc2 = nullptr;
	mob_f = tmpfile();
	require(mob_f, "prototype text fixture could not open");
	fputs("archivist~\n&+Wthe archivist&n~\n", mob_f);
	const long saved_position = ftell(mob_f);
	const auto lazy_catalog = zone_story_quest_production::build_runtime_catalog(1, &error);
	require(lazy_catalog.definitions[0].giver_name == "the archivist" &&
			ftell(mob_f) == saved_position,
		"lazy prototype names were missing or changed the loader position");
	fclose(mob_f);
	mob_f = nullptr;
	goal_data coins{ .goal_type = QUEST_GOAL_COINS, .number = 10, .next = &give };
	first.give = &coins;
	const auto mixed_catalog = zone_story_quest_production::build_runtime_catalog(2, &error);
	require(std::any_of(mixed_catalog.definitions.begin(), mixed_catalog.definitions.end(),
			    [](const auto &definition) {
				    return definition.daily_exclusion ==
					   "Unsupported durable offering";
			    }),
		"mixed currency/item contract became a daily without a supported offering path");
	goal_data large[15] = {};
	for (int index = 0; index < 15; ++index)
	{
		large[index].goal_type = QUEST_GOAL_ITEM;
		large[index].number = 25000 + index;
		large[index].next = index < 14 ? &large[index + 1] : nullptr;
	}
	first.give = large;
	const auto oversized_catalog =
		zone_story_quest_production::build_runtime_catalog(2, &error);
	require(std::any_of(
			oversized_catalog.definitions.begin(), oversized_catalog.definitions.end(),
			[](const auto &definition)
			{ return definition.daily_exclusion == "Unsupported durable offering"; }),
		"oversized native offering contract became a daily");
	// The isolated working directory exercises real bootstrap loading without
	// changing the checkout's authored area files.
	require(argc == 2, "expected isolated fixture directory");
	std::filesystem::current_path(argv[1]);
	std::filesystem::create_directories("areas/story");
	first.give = &give;
	const char *story_path = "areas/story/runtime-zone.story.json";
	{
		std::ofstream file(story_path);
		file << R"({"schema_version":2,"revision":1,"source_area":"runtime-zone",
"introduction":"Explore the archive.","orientation":["Use look and exits."],
"contacts":[{"mob_vnum":17,"name":"The archivist","keyword":"archivist",
"description":"Ask about the archive.","topics":["archive"]}],
"coverage":"partial","stories":[{"id":"archivist","title":"The archivist",
"category":"story","summary":"Bring the lost item.","contracts":[
{"giver_vnum":17,"completion_key":"give=I:24402;receive=I:24403;disappear=0"}],
"steps":[{"id":"item","text":"Carry the lost item","kind":"carried_item",
"hint":"Look nearby.","item_vnums":[24402],"count":1}]}],"exclusions":[]})";
	}
	require(!zone_story_quest_production::bootstrap(2, &error) &&
			!zone_story_quest_production::ready() &&
			error.find("unknown item prototype") != std::string::npos,
		"missing booted object prototype did not fail closed");
	index_data objects[1] = {};
	objects[0].virtual_number = 24402;
	obj_index = objects;
	top_of_objt = 0;
	require(zone_story_quest_production::bootstrap(2, &error) &&
			zone_story_quest_production::runtime_catalog().story_mappings.size() == 1,
		"valid optional sidecar failed bootstrap");
	std::ifstream valid_file(story_path);
	const std::string valid_mapping{ std::istreambuf_iterator<char>(valid_file),
					 std::istreambuf_iterator<char>() };
	for (const auto &replacement :
	     { std::pair<std::string, std::string>{ "\"mob_vnum\":17", "\"mob_vnum\":99999" },
	       std::pair<std::string, std::string>{ "\"keyword\":\"archivist\"",
						    "\"keyword\":\"unknown\"" } })
	{
		std::string invalid_mapping = valid_mapping;
		invalid_mapping.replace(invalid_mapping.find(replacement.first),
					replacement.first.size(), replacement.second);
		{
			std::ofstream file(story_path);
			file << invalid_mapping;
		}
		require(!zone_story_quest_production::bootstrap(2, &error) &&
				error.find("unknown NPC prototype or command alias") !=
					std::string::npos,
			"invalid contact did not fail closed against booted prototypes");
	}
	{
		std::ofstream file(story_path);
		file << "{}";
	}
	require(!zone_story_quest_production::bootstrap(2, &error) &&
			!zone_story_quest_production::ready(),
		"malformed present sidecar silently fell back");
	std::filesystem::remove(story_path);
	require(zone_story_quest_production::bootstrap(2, &error) &&
			zone_story_quest_production::runtime_catalog().story_mappings.empty(),
		"absent optional sidecar broke native fallback");
	std::cout << "zone-story runtime production catalog regression passed\n";
	return 0;
}
