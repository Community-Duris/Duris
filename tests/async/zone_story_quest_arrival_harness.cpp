#include "world/zone_story_quest_runtime.c"
#include "world/zone_story_quest_story.h"
#include "cmd/interp.h"
#include "magic/spells.h"
#include <cassert>
#include <cstdarg>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <cstring>
#include <stdexcept>

static void scripted_hire_regressions();

P_room world = nullptr;
P_index obj_index = nullptr;
P_index mob_index = nullptr;
int top_of_mobt = 0;
P_char character_list = nullptr;
struct zone_data *zone_table = nullptr;
namespace
{
bool save_ok = true;
bool throw_save = false;
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
const std::string *definition_id_for(const quest_complete_data *completion)
{
	static const std::string id = "daily:alatorin:1";
	return completion ? &id : nullptr;
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
	if (throw_save)
		throw std::runtime_error("injected journal storage exception");
	if (!save_ok)
		return sql_zone_story_quest_state_result::io_error;
	zone_story_quest_state::apply(&durable_records, updates);
	durable = zone_story_quest_state::document(durable_records);
	return sql_zone_story_quest_state_result::ok;
}
sql_zone_story_quest_state_result sql_zone_story_quest_state_load(uint32_t, std::string *state,
								  std::string *, bool *legacy)
{
	if (legacy)
		*legacy = false;
	if (durable.empty())
		return sql_zone_story_quest_state_result::not_found;
	*state = durable;
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
	char player_name[] = "Alice";
	player.player.name = player_name;
	player.player.level = 10;
	player.specials.position = STAT_NORMAL;
	descriptor_data descriptor = {};
	descriptor.connected = CON_PLAYING;
	descriptor.character = &player;
	player.desc = &descriptor;
	player.in_room = 0;
	// This harness links no economic accounting implementation.
	assert(ready());
	assert(service() != nullptr);
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
	tracker_ready = false;
	const int writes_before_disabled = writes;
	const auto state_before_disabled = tracker.serialize_state();
	arrived(&player);
	encountered(&player, &npc);
	assert(!daily_eligible(&player, definition.definition_id, 10, 864001));
	assert(service() == nullptr && render_daily(&player, false).empty() &&
	       writes == writes_before_disabled &&
	       tracker.serialize_state() == state_before_disabled);
	tracker_ready = true;
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
	assert(record_authoritative_completion(definition.definition_id, 831, 42, { 42, 77 }, 83450,
					       864001, "Alice", 10, 1, true, 2, 10, &error,
					       "offering:101", &frozen) &&
	       writes == completed_writes);
	assert(erase_character(77, &error));
	assert(restored.deserialize_state(durable) && restored.summary_for(7, 77).completed == 0);
	assert(service() != nullptr);
	quest_complete_data legacy_completion = {};
	assert(record_legacy_completion(&player, &legacy_completion, 83450, 864002, &error));
	assert(record_authoritative_completion(definition.definition_id, 831, 42, { 42 }, 83450,
					       864003, "Alice", 10, 1, true, 1, 10, &error));
	// Reviewed ownership corrections retain old receipts and frozen continuation
	// identity. Historical neighboring discovery must not invent a Church visit.
	zone_story_quest_catalog::catalog church_catalog;
	church_catalog.content_revision = 2;
	church_catalog.zones = { { 878, "Church", "church", 87800, 87852, true },
				 { 879, "Kelek", "kelek", 87853, 88165, true } };
	auto bishop = definition;
	bishop.definition_id = "zone-story:qst:87860:stable-contract";
	bishop.giver_vnum = 87860;
	bishop.zone_number = 878;
	bishop.source_area = "church";
	bishop.previous_zone_number = 879;
	church_catalog.definitions.push_back(bishop);
	auto old_catalog = church_catalog;
	old_catalog.definitions[0].zone_number = 879;
	old_catalog.definitions[0].source_area = "kelek";
	old_catalog.definitions[0].previous_zone_number = -1;
	zone_story_quest_feature::service old_tracker(old_catalog);
	assert(old_tracker.discover_zone(7, 42, 879, 87920, 864000, "arrival") ==
	       zone_story_quest_feature::result::applied);
	zone_story_quest_feature::completion_event old_event;
	old_event.transaction = { 2,
				  "offering:church-old",
				  bishop.definition_id,
				  879,
				  42,
				  { 42, 77 },
				  87841,
				  864001,
				  7,
				  2,
				  1,
				  { 42 } };
	old_event.character_name = "Alice";
	old_event.level = old_event.strongest_party_level = 10;
	old_event.racewar = 1;
	old_event.party_context_known = true;
	old_event.party_size = 2;
	assert(old_tracker.record_completion(old_event, &error) ==
	       zone_story_quest_feature::result::applied);
	const auto immutable_receipt =
		zone_story_quest_tracking::serialize_transaction(old_event.transaction);
	tracker = zone_story_quest_feature::service(church_catalog);
	assert(tracker.deserialize_state(old_tracker.serialize_state(), &error));
	zone_story_quest_tracking::completion_transaction receipt;
	assert(tracker.existing_transaction("offering:church-old", &receipt) &&
	       zone_story_quest_tracking::serialize_transaction(receipt) == immutable_receipt);
	assert(tracker.progress_for_zone(7, 42, 878).completed == 1 &&
	       tracker.progress_for_zone(7, 42, 879).completed == 0 &&
	       tracker.has_discovered(7, 42, 879) && !tracker.has_discovered(7, 42, 878));
	const int old_replay_writes = writes;
	const auto old_replay_state = tracker.serialize_state();
	assert(record_authoritative_completion(bishop.definition_id, 879, 42, { 42, 77 }, 87841,
					       864001, "Alice", 10, 1, true, 2, 10, &error,
					       "offering:church-old", &frozen));
	assert(writes == old_replay_writes && tracker.serialize_state() == old_replay_state);
	assert(!record_authoritative_completion(bishop.definition_id, 879, 42, { 42 }, 87841,
						864001, "Alice", 10, 1, true, 1, 10, &error,
						"offering:church-old", &frozen));
	auto stale = frozen;
	stale.catalog_revision = 1;
	assert(!record_authoritative_completion(bishop.definition_id, 879, 42, { 42, 77 }, 87841,
						864001, "Alice", 10, 1, true, 2, 10, &error,
						"offering:church-old", &stale));
	// A committed offering not yet recorded may recover with its frozen old
	// owner. New tracking uses the current owner; its economic receipt is external.
	tracker = zone_story_quest_feature::service(church_catalog);
	durable_records.clear();
	const auto pending_before = tracker.serialize_state();
	assert(!record_authoritative_completion(bishop.definition_id, 900, 42, { 42, 77 }, 87841,
						864001, "Alice", 10, 1, true, 2, 10, &error,
						"offering:church-pending", &frozen));
	assert(!record_authoritative_completion(bishop.definition_id, 879, 42, { 42, 77 }, 87841,
						864001, "Alice", 10, 1, true, 2, 10, &error, "",
						&frozen));
	assert(!record_authoritative_completion(bishop.definition_id, 879, 42, { 42, 77 }, 87841,
						864001, "Alice", 10, 1, true, 2, 10, &error,
						"offering:church-pending", nullptr));
	assert(!record_authoritative_completion(bishop.definition_id, 879, 42, { 42, 77 }, 87841,
						864001, "Alice", 10, 1, true, 2, 10, &error,
						"offering:church-pending", &stale));
	assert(tracker.serialize_state() == pending_before);
	save_ok = false;
	assert(!record_authoritative_completion(bishop.definition_id, 879, 42, { 42, 77 }, 87841,
						864001, "Alice", 10, 1, true, 2, 10, &error,
						"offering:church-pending", &frozen));
	assert(tracker.serialize_state() == pending_before && durable_records.empty());
	save_ok = true;
	assert(record_authoritative_completion(bishop.definition_id, 879, 42, { 42, 77 }, 87841,
					       864001, "Alice", 10, 1, true, 2, 10, &error,
					       "offering:church-pending", &frozen));
	assert(tracker.existing_transaction("offering:church-pending", &receipt) &&
	       receipt.zone_number == 878 && receipt.season_id == 7 &&
	       receipt.content_revision == 2 && receipt.room_vnum == 87841 &&
	       receipt.completed_at == 864001 &&
	       receipt.credited_pids == std::vector<uint32_t>({ 42, 77 }) &&
	       receipt.daily_credited_pids == std::vector<uint32_t>({ 42 }));
	assert(tracker.summary_for(7, 42).renown == 1 && tracker.summary_for(7, 77).renown == 0 &&
	       !tracker.has_discovered(7, 42, 878) && !tracker.has_discovered(7, 42, 879));
	const int pending_writes = writes;
	tracker = zone_story_quest_feature::service(church_catalog);
	assert(tracker.deserialize_state(durable, &error));
	assert(record_authoritative_completion(bishop.definition_id, 879, 42, { 42, 77 }, 87841,
					       864001, "Alice", 10, 1, true, 2, 10, &error,
					       "offering:church-pending", &frozen));
	assert(writes == pending_writes && tracker.summary_for(7, 42).renown == 1);
	assert(erase_character(77, &error));
	zone_story_quest_feature::service church_restored(church_catalog);
	assert(church_restored.deserialize_state(durable, &error) &&
	       church_restored.summary_for(7, 77).completed == 0 &&
	       church_restored.summary_for(7, 42).completed == 1 &&
	       !church_restored.existing_transaction("offering:church-pending", &receipt));
	scripted_hire_regressions();
	std::cout
		<< "native arrival suppression, failure rollback, empty area, and restart passed\n";
}

// Native callback surroundings are isolated; the three callbacks and setup_pet
// themselves are linked unchanged from production, including their rejection paths.
namespace
{
std::map<P_char, P_char> pet_owners;
std::vector<char *> allocated_names;
P_char payment_target = nullptr;
int payment = 0;
bool allow_pet = true;
bool indefinite_pet = false;
int pet_links = 0;
}
bool collector_presence_is_npc(P_char)
{
	return false;
}
bool training_dummy_capture_target_allowed(P_char)
{
	return allow_pet;
}
bool has_innate(P_char, int innate)
{
	return indefinite_pet && innate == INNATE_UNHOLY_ALLIANCE;
}
void linked_affect_to_char(P_char npc, affected_type *affect, P_char owner, int type)
{
	assert(type == LNK_PET);
	pet_owners[npc] = owner;
	npc->specials.affected_by |= affect->bitvector;
	++pet_links;
}
P_char get_linked_char(P_char npc, ush_int type)
{
	return type == LNK_PET ? pet_owners[npc] : nullptr;
}
void item_restrict_player_pet_equipment(P_char) {}
void remove_plushit_bits(P_char) {}
void clearMemory(P_char) {}
void remember(P_char, P_char) {}
int number(int low, int)
{
	return low;
}
int dice(int, int)
{
	return 1;
}
char *str_dup(const char *name)
{
	char *copy = strdup(name);
	allocated_names.push_back(copy);
	return copy;
}
void add_follower(P_char npc, P_char player)
{
	npc->following = player;
}
void stop_follower(P_char npc)
{
	npc->following = nullptr;
}
void do_give(P_char player, char *, int)
{
	if (payment_target && payment > 0 && GET_COPPER(player) >= payment)
	{
		GET_COPPER(player) -= payment;
		GET_COPPER(payment_target) += payment;
	}
}
void do_action(P_char, char *, int) {}
void mobsay(P_char, const char *) {}
void act(const char *, int, P_char, P_obj, void *, int) {}
void argument_interpreter(char *, char *first, char *second)
{
	*first = *second = 0;
}
P_char get_char_room(const char *, int)
{
	return payment_target;
}
int sex_crazed_prostitute(P_char, P_char, int, char *);
int well_built_prostitute(P_char, P_char, int, char *);
int sleezy_prostitute(P_char, P_char, int, char *);

static void scripted_hire_regressions()
{
	using namespace zone_story_quest_runtime;
	zone_story_quest_catalog::catalog catalog;
	catalog.content_revision = 2;
	catalog.zones = { { 936, "Vella's Bordello", "bordello", 93600, 93668, true } };
	std::ifstream file("areas/story/bordello.story.json");
	const std::string mapping{ std::istreambuf_iterator<char>(file),
				   std::istreambuf_iterator<char>() };
	std::string error;
	assert(zone_story_quest_story::apply(mapping, "bordello", &catalog, &error));
	assert(catalog.definitions.size() == 3);
	for (const auto &unit : zone_story_quest_catalog::quest_units(catalog))
		assert(!unit.achievement && !unit.daily_candidate);
	room_data room = {};
	room.number = 93610;
	world = &room;
	zone_data zone = {};
	zone.number = 936;
	zone_table = &zone;
	index_data indices[3] = {};
	const int providers[] = { 93602, 93603, 93611 };
	for (int i = 0; i < 3; ++i)
		indices[i].virtual_number = providers[i];
	mob_index = indices;
	top_of_mobt = 2;
	pc_only_data pc = {};
	pc.pid = 42;
	char_data player = {};
	player.only.pc = &pc;
	char player_name[] = "Alice";
	player.player.name = player_name;
	player.player.level = 10;
	player.specials.position = STAT_NORMAL;
	player.in_room = 0;
	descriptor_data descriptor = {};
	descriptor.connected = CON_PLAYING;
	descriptor.character = &player;
	player.desc = &descriptor;
	pc_only_data peer_pc = {};
	peer_pc.pid = 77;
	char_data peer = player;
	peer.only.pc = &peer_pc;
	char peer_name[] = "Bob";
	peer.player.name = peer_name;
	descriptor_data peer_descriptor = descriptor;
	peer_descriptor.character = &peer;
	peer.desc = &peer_descriptor;
	group_list peer_member = {};
	peer_member.ch = &peer;
	player.group = &peer_member;
	player.next = &peer;
	character_list = &player;
	npc_only_data npc_data = {};
	char_data npc = {};
	char npc_name[] = "prostitute";
	auto reset = [&](int index)
	{
		tracker = zone_story_quest_feature::service(catalog);
		tracker_ready = true;
		save_ok = true;
		throw_save = false;
		durable.clear();
		durable_records = {};
		writes = 0;
		published.clear();
		allow_pet = true;
		indefinite_pet = false;
		pet_owners.clear();
		pet_links = 0;
		npc = {};
		npc_data = {};
		npc.only.npc = &npc_data;
		npc_data.R_num = index;
		npc.specials.act = ACT_ISNPC;
		npc.specials.position = STAT_NORMAL;
		npc.player.level = 10;
		npc.player.name = npc_name;
		npc.in_room = 0;
		room.people = &npc;
		npc.next_in_room = &player;
		player.next_in_room = &peer;
		player.specials.position = STAT_NORMAL;
		player.player.level = 10;
		GET_COPPER(&player) = 10000;
		payment_target = &npc;
		payment = 500;
		assert(tracker.discover_zone(7, 42, 936, 93610, 100, "arrival") ==
		       zone_story_quest_feature::result::applied);
		assert(tracker.discover_zone(7, 77, 936, 93610, 100, "arrival") ==
		       zone_story_quest_feature::result::applied);
		assert(tracker.meet_npc(7, 77, providers[index], 93610, 100) ==
		       zone_story_quest_feature::result::applied);
	};
	const auto used = [&]() {
		return render_journal(&player, 936, false, false).find("[Service used]") !=
		       std::string::npos;
	};
	int (*callbacks[])(P_char, P_char, int, char *) = { sex_crazed_prostitute,
							    well_built_prostitute,
							    sleezy_prostitute };
	char args[] = "500 copper prostitute";
	for (int index = 0; index < 3; ++index)
	{
		const auto invoke = [&]()
		{ return callbacks[index](&npc, &player, CMD_GIVE, args); };
		reset(index);
		assert(invoke() && used() && npc.following == &player &&
		       GET_MASTER(&npc) == &player);
		assert(GET_COPPER(&player) == 9500 && GET_COPPER(&npc) == 500 && pet_links == 1);
		assert(npc_data.pet_charm_expires_at >= time(nullptr) + 599);
		assert(tracker.has_met_npc(7, 42, providers[index]));
		assert(tracker.summary_for(7, 42).renown == 0);
		assert(render_journal(&peer, 936, false, false).find("[Service used]") ==
		       std::string::npos); // A present groupmate did not buy the service.
		const auto id = zone_story_quest_catalog::scripted_definition_id(
			"bordello", providers[index], "hire-companion");
		assert(tracker.evidence_for(id, 2).successful_attempts == 1);
		npc.following = nullptr;
		pet_owners.clear();
		npc.specials.affected_by = 0;
		assert(used()); // Historical service use survives losing/expiring the pet.
		tracker = zone_story_quest_feature::service(catalog);
		assert(load_persisted_state(&error) && used());
		assert(invoke() && used() && pet_links == 2 && GET_COPPER(&player) == 9000);
		assert(tracker.evidence_for(id, 2).successful_attempts == 2);

		reset(index);
		payment = 250;
		assert(invoke() && invoke() && GET_COPPER(&npc) == 500);
		assert(!used() && !pet_links && writes == 0);
		payment = 700;
		assert(invoke() && used() && GET_COPPER(&npc) == 1200);

		reset(index);
		payment = 0; // Item gift or rejected monetary transfer has no cash delta.
		assert(invoke() && !used() && !pet_links && writes == 0);
		payment = 500;
		payment_target = nullptr; // The GIVE target is not this provider.
		assert(invoke() && !used() && !pet_links && writes == 0);
		payment_target = &npc;
		npc.specials.fighting = &player;
		assert(!invoke() && !used() && GET_COPPER(&player) == 10000);

		reset(index);
		allow_pet = false;
		assert(invoke() && !used() && !pet_links && writes == 0);
		assert(GET_COPPER(&player) == 9500 &&
		       npc.following == &player); // Preserve native rejection behavior.
		// A rejected renewal must not credit an already-owned pet.
		pet_owners[&npc] = &player;
		npc.specials.affected_by |= AFF_CHARM;
		assert(invoke() && !used() && writes == 0);

		reset(index);
		indefinite_pet = true;
		assert(invoke() && used() && npc_data.pet_charm_expires_at == 0);

		reset(index);
		save_ok = false;
		const auto before = tracker.serialize_state();
		assert(invoke() && npc.following == &player && GET_COPPER(&player) == 9500);
		assert(!used() && tracker.serialize_state() == before);
		assert(!tracker.has_met_npc(7, 42, providers[index]));
		save_ok = true;
		assert(invoke() && used() && GET_COPPER(&player) == 9000);

		reset(index);
		tracker_ready = false;
		assert(invoke() && !writes && npc.following == &player &&
		       GET_MASTER(&npc) == &player);
		assert(GET_COPPER(&player) == 9500 && pet_links == 1);

		reset(index);
		throw_save = true;
		assert(invoke() && !ready() && npc.following == &player &&
		       GET_COPPER(&player) == 9500);
	}
	reset(0);
	assert(!record_scripted_completion(&player, &npc, "bordello", "unknown", &error));
	assert(!record_scripted_completion(&player, &npc, "other", "hire-companion", &error));
	npc_data.R_num = 3;
	assert(!record_scripted_completion(&player, &npc, "bordello", "hire-companion", &error));
	npc_data.R_num = 0;
	player.player.level = OVERLORD;
	assert(!record_scripted_completion(&player, &npc, "bordello", "hire-companion", &error));
	player.player.level = 10;
	{
		temporary_placement preview;
		assert(!record_scripted_completion(&player, &npc, "bordello", "hire-companion",
						   &error));
	}
	assert(writes == 0);
	// The same adapter also records a non-pet story in a different zone.
	auto generic = catalog;
	generic.zones.push_back({ 937, "Archive", "archive", 93700, 93799, true });
	const std::string other = R"json({
      "schema_version":4,"revision":1,"source_area":"archive","coverage":"complete",
      "introduction":"Archive","orientation":[],
      "contacts":[{"mob_vnum":93602,"name":"Archivist","keyword":"prostitute","description":"A scripted fixture.","topics":[]}],
      "scripted_outcomes":[{"event":"open-records","giver_vnum":93602,"repeatable":false}],
      "stories":[{"id":"open","title":"Open records","category":"story","summary":"A non-pet scripted outcome.",
        "contracts":[{"giver_vnum":93602,"completion_key":"scripted:archive:open-records"}],
        "steps":[{"id":"open","text":"Open records","kind":"completion","hint":"Read the records.",
          "contracts":[{"giver_vnum":93602,"completion_key":"scripted:archive:open-records"}]}]}],"exclusions":[]})json";
	assert(zone_story_quest_story::apply(other, "archive", &generic, &error));
	auto foreign_step = other;
	foreign_step.replace(foreign_step.rfind("scripted:archive:open-records"),
			     std::strlen("scripted:archive:open-records"),
			     "scripted:bordello:hire-companion");
	auto candidate = catalog;
	candidate.zones = generic.zones;
	assert(!zone_story_quest_story::apply(foreign_step, "archive", &candidate, &error) &&
	       error == "scripted outcome: cross-area steps are not supported");
	tracker = zone_story_quest_feature::service(generic);
	const auto undiscovered = tracker.serialize_state();
	save_ok = false;
	assert(!record_scripted_completion(&player, &npc, "archive", "open-records", &error));
	assert(tracker.serialize_state() == undiscovered && !tracker.has_discovered(7, 42, 936));
	save_ok = true;
	const bool generic_recorded =
		record_scripted_completion(&player, &npc, "archive", "open-records", &error);
	if (!generic_recorded)
		std::cerr << "generic scripted result: " << error << "\n";
	assert(generic_recorded);
	assert(tracker.summary_for(7, 42).completed == 1 &&
	       tracker.summary_for(7, 77).completed == 0);
	assert(!tracker.has_discovered(7, 42, 937)); // A foreign outcome does not invent a visit.
	assert(GET_COPPER(&player) == 10000 && pet_links == 0);
	generic.unavailable_story_areas["archive"] = "invalid mapping";
	tracker = zone_story_quest_feature::service(generic);
	assert(!record_scripted_completion(&player, &npc, "archive", "open-records", &error));
	for (auto *name : allocated_names)
		free(name);
	allocated_names.clear();
	std::cout
		<< "scripted Vella hires: three native callbacks, actual pet setup, rejected/indefinite setup, payments, persistence and isolation passed\n";
}
