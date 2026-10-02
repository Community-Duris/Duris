#include "world/zone_story_quest_runtime.c"
#include <cassert>
#include <cstdarg>
#include <cstdlib>
#include <iostream>

P_room world = nullptr;
P_index obj_index = nullptr;
P_index mob_index = nullptr;
int top_of_mobt = 0;
P_char character_list = nullptr;
struct zone_data *zone_table = nullptr;
namespace
{
bool save_ok = true;
bool npc_visible = true;
int vision = 1;
int writes = 0;
std::string published;
std::string durable;
zone_story_quest_state::records durable_records;
}
namespace zone_story_quest_production
{
bool ready()
{
	return true;
}
}
uint64_t sql_season_epoch()
{
	return 7;
}
persistence_mode persistence_mode_get()
{
	return PERSISTENCE_MODE_MARIADB_PRIMARY;
}
const char *persistence_mode_flatfile_root()
{
	return "/unused";
}
sql_zone_story_quest_state_result
sql_zone_story_quest_records_save(uint32_t, const zone_story_quest_state::changes &updates,
				  std::string *)
{
	++writes;
	if (!save_ok)
		return sql_zone_story_quest_state_result::io_error;
	zone_story_quest_state::apply(&durable_records, updates);
	durable = zone_story_quest_state::document(durable_records);
	return sql_zone_story_quest_state_result::ok;
}
void send_to_char(const char *message, P_char)
{
	published += message;
}
void logit(const char *, const char *, ...) {}
bool ac_can_see(P_char, P_char, bool)
{
	return npc_visible;
}
int get_vis_mode(P_char, int)
{
	return vision;
}
int panic_corruption_int(const char *, const char *, ...)
{
	std::abort();
}

int main()
{
	using namespace zone_story_quest_runtime;
	zone_story_quest_catalog::catalog catalog;
	catalog.content_revision = 2;
	catalog.zones = { { 831, "Alatorin", "alatorin", 83100, 84055, true },
			  { 900, "Empty Coast", "empty", 90000, 90099, true } };
	tracker = zone_story_quest_feature::service(catalog);
	tracker_ready = true;
	zone_data zones[2] = {};
	zones[0].number = 831;
	char area_name[] = "Alatorin";
	zones[0].name = area_name;
	zones[1].number = 900;
	char empty_name[] = "Empty Coast";
	zones[1].name = empty_name;
	zone_table = zones;
	room_data rooms[3] = {};
	world = rooms;
	rooms[0].number = 83450;
	rooms[0].zone = 0;
	rooms[1].number = 90010;
	rooms[1].zone = 1;
	rooms[2].number = VROOM_SHIPS_START;
	pc_only_data pc = {};
	pc.pid = 42;
	char_data player = {};
	player.only.pc = &pc;
	player.player.level = 10;
	player.specials.position = STAT_NORMAL;
	descriptor_data descriptor = {};
	descriptor.connected = CON_PLAYING;
	descriptor.character = &player;
	player.desc = &descriptor;
	player.in_room = 0;
	{
		temporary_placement remote;
		arrived(&player);
		assert(writes == 0 && published.empty());
	}
	save_ok = false;
	arrived(&player);
	assert(!tracker.has_discovered(7, 42, 831) && published.empty());
	save_ok = true;
	arrived(&player);
	assert(tracker.has_discovered(7, 42, 831) &&
	       published.find("Alatorin") != std::string::npos);
	const int committed_writes = writes;
	arrived(&player);
	assert(writes == committed_writes);
	player.in_room = 1;
	rooms[1].room_flags = ROOM_ARENA;
	arrived(&player);
	assert(!tracker.has_discovered(7, 42, 900));
	rooms[1].room_flags = 0;
	descriptor.connected = CON_NAME;
	arrived(&player);
	assert(!tracker.has_discovered(7, 42, 900));
	descriptor.connected = CON_PLAYING;
	player.player.level = OVERLORD;
	arrived(&player);
	assert(!tracker.has_discovered(7, 42, 900));
	player.player.level = 10;
	player.in_room = 2;
	arrived(&player);
	assert(writes == committed_writes);
	player.in_room = 1;
	arrived(&player);
	assert(tracker.has_discovered(7, 42, 900) &&
	       tracker.summary_for(7, 42).discovered_zones == 2);
	zone_story_quest_feature::service restored(catalog);
	std::string restore_error;
	if (!restored.deserialize_state(durable, &restore_error))
		std::cerr << restore_error << "\n" << durable;
	assert(restored.deserialize_state(durable) && restored.has_discovered(7, 42, 831) &&
	       restored.has_discovered(7, 42, 900));
	zone_story_quest_tracking::quest_definition definition;
	definition.definition_id = "daily:alatorin:1";
	definition.source_area = "alatorin";
	definition.source_system = "zone_story";
	definition.zone_number = 831;
	definition.giver_vnum = 83101;
	definition.completion_key = "give=item;receive=coins";
	definition.active = definition.repeatable = definition.eligible_for_zone_completion =
		definition.daily_eligible = true;
	definition.content_revision = 2;
	catalog.definitions.push_back(definition);
	zone_story_quest_catalog::story_mapping mapping;
	mapping.source_area = "alatorin";
	mapping.revision = 1;
	zone_story_quest_catalog::story_definition story;
	story.id = "zone-story:story:alatorin:garden";
	story.zone_number = 831;
	story.title = "Garden access";
	story.category = "story";
	story.summary = "Wear the belt to enter the garden.";
	story.contracts = { definition.definition_id };
	story.steps = { { .id = "belt",
			  .text = "Wear the belt",
			  .kind = "equipped_item",
			  .hint = "Wear it at your waist.",
			  .item_vnums = { 13521 },
			  .count = 1,
			  .slot = WEAR_WAIST },
			{ .id = "plant",
			  .text = "Carry a plant",
			  .kind = "carried_item",
			  .hint = "Find a plant.",
			  .item_vnums = { 13553 },
			  .count = 1 } };
	mapping.stories.push_back(story);
	mapping.introduction = "Explore this garden.";
	mapping.contacts.push_back(
		{ 83101, "The gardener", "gardener", "Ask about plants.", { "plants" } });
	catalog.story_mappings.push_back(mapping);
	tracker = zone_story_quest_feature::service(catalog);
	assert(tracker.discover_zone(7, 42, 831, 83450, 864000, "arrival") ==
	       zone_story_quest_feature::result::applied);
	player.in_room = 0;
	char_data npc = {};
	npc.specials.act = ACT_ISNPC;
	npc.specials.position = STAT_NORMAL;
	npc.in_room = 0;
	npc_only_data npc_data = {};
	npc.only.npc = &npc_data;
	char long_description[] = "A gardener is here.";
	npc.player.long_descr = long_description;
	index_data mobs[1] = {};
	mobs[0].virtual_number = 83101;
	mob_index = mobs;
	rooms[0].people = &npc;
	published.clear();
	assert(render_journal(&player, 831, false, false).find("gardener") == std::string::npos);
	npc_visible = false;
	encountered(&player, &npc);
	assert(!tracker.has_met_npc(7, 42, 83101));
	npc_visible = true;
	vision = 3;
	encountered(&player, &npc);
	assert(!tracker.has_met_npc(7, 42, 83101));
	vision = 1;
	{
		temporary_placement remote;
		encountered(&player, &npc);
	}
	assert(!tracker.has_met_npc(7, 42, 83101));
	save_ok = false;
	encountered(&player, &npc);
	assert(!tracker.has_met_npc(7, 42, 83101) && published.empty());
	save_ok = true;
	encountered(&player, &npc);
	assert(tracker.has_met_npc(7, 42, 83101) &&
	       published.find("quest zone Alatorin") != std::string::npos);
	assert(render_journal(&player, 831, false, false).find("ask gardener plants") !=
	       std::string::npos);
	assert(restored.deserialize_state(durable) && restored.has_met_npc(7, 42, 83101));
	index_data indices[2] = {};
	indices[0].virtual_number = 13521;
	indices[1].virtual_number = 13553;
	obj_index = indices;
	obj_data belt = {}, plant = {};
	belt.R_num = 0;
	plant.R_num = 1;
	belt.next_content = &plant;
	player.carrying = &belt;
	const int writes_before_journal = writes;
	const auto history_before_journal = tracker.serialize_state();
	std::string journal = render_journal(&player, 831, false, false);
	assert(journal.find("[Missing now] Wear the belt") != std::string::npos &&
	       journal.find("[Ready now] Carry a plant") != std::string::npos);
	player.carrying = &plant;
	player.equipment[WEAR_WAIST] = &belt;
	journal = render_journal(&player, 831, false, false);
	assert(journal.find("[Ready now] Wear the belt") != std::string::npos &&
	       writes == writes_before_journal &&
	       tracker.serialize_state() == history_before_journal);
	player.carrying = nullptr;
	player.equipment[WEAR_WAIST] = nullptr;
	frozen_daily_context frozen{ 7, 2, 1, { 42 } };
	const auto before = tracker.serialize_state();
	save_ok = false;
	std::string error;
	assert(!record_authoritative_completion(definition.definition_id, 831, 42, { 42, 77 },
						83450, 864001, "Alice", 10, 1, true, 2, 10, &error,
						"offering:101", &frozen));
	assert(tracker.serialize_state() == before &&
	       tracker.evidence_for(definition.definition_id, 2).observed_attempts == 0);
	save_ok = true;
	assert(record_authoritative_completion(definition.definition_id, 831, 42, { 42, 77 }, 83450,
					       864001, "Alice", 10, 1, true, 2, 10, &error,
					       "offering:101", &frozen));
	const int completed_writes = writes;
	assert(record_authoritative_completion(definition.definition_id, 831, 42, { 42, 77 }, 83450,
					       864001, "Alice", 10, 1, true, 2, 10, &error,
					       "offering:101", &frozen) &&
	       writes == completed_writes);
	assert(tracker.summary_for(7, 42).renown == 1 && tracker.summary_for(7, 77).renown == 0);
	assert(!record_authoritative_completion(definition.definition_id, 831, 42, { 42 }, 83450,
						864001, "Alice", 10, 1, true, 1, 10, &error,
						"offering:101", &frozen));
	std::cout
		<< "native arrival suppression, failure rollback, empty area, and restart passed\n";
}
